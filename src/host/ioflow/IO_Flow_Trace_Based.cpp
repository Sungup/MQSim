#include "IO_Flow_Trace_Based.h"

#include <cstdlib>

#include "../../sim/Engine.h"

using namespace Host_Components;

/// ---------------
/// TraceItem class
/// ---------------

/*
 * Trace format
 * <Time>| |<Device>| |<address>| |<size>| |<io type>
 */
force_inline
TraceItem::TraceItem()
  : __time(0),
    __device(0),
    __lba(0),
    __count(0),
    __type(HostIOReqType::READ)
{ }

force_inline bool
TraceItem::check_valid(const char *string, int len)
{
  int delimiters = 0;
  const char* end = string + len;

  while (string < end) delimiters += (*(string++) == DELIMITER_CHAR) ? 1 : 0;

  return delimiters == DELIMITER_COUNT;
}

force_inline bool
TraceItem::parse(const char* line, char** end,
                 sim_time_type offset_base,
                 LHA_type start, LHA_type region_size)
{
  char* pos;

  __time   = std::strtoull (line, &pos, 10) + offset_base;
  __device = std::strtoul  (pos,  &pos, 10);
  __lba    = std::strtoull (pos,  &pos, 10) % region_size + start;
  __count  = std::strtoul  (pos,  &pos, 10);
  __type   = *(pos + DELIMITER_LEN) == READ_CODE
               ? HostIOReqType::READ
               : HostIOReqType::WRITE;

  if (end)
    *end = pos + DELIMITER_LEN + 1;

  return true;
}

force_inline bool
TraceItem::parse(const std::string& str,
                 sim_time_type offset_base,
                 LHA_type start, LHA_type region_size)
{
  return parse(str.c_str(), nullptr, offset_base, start, region_size);
}

force_inline sim_time_type
TraceItem::time() const
{ return __time; }

force_inline LHA_type
TraceItem::lba() const
{ return __lba; }

force_inline uint32_t
TraceItem::lba_count() const
{ return __count; }

force_inline HostIOReqType
TraceItem::io_type() const
{ return __type; }

force_inline bool
TraceItem::is_read() const
{ return __type == HostIOReqType::READ; }

/// -----------------------
/// IO flow for trace class
/// -----------------------
IO_Flow_Trace_Based::IO_Flow_Trace_Based(const sim_object_id_type& name,
                                         const HostParameterSet& host_params,
                                         const TraceFlowParameterSet& flow_params,
                                         const Utils::LogicalAddrPartition& lapu,
                                         uint16_t flow_id,
                                         uint16_t sq_size,
                                         uint16_t cq_size,
                                         HostInterface_Types interface_type,
                                         PCIe_Root_Complex* root_complex,
                                         SATA_HBA* sata_hba)
  : IO_Flow_Base(name,
                 host_params,
                 flow_params,
                 lapu,
                 flow_id,
                 lapu.available_start_lha(flow_id),
                 lapu.available_end_lha(flow_id),
                 sq_size,
                 cq_size,
                 0,
                 interface_type,
                 root_complex,
                 sata_hba),
    __flow_region_size(end_lsa - start_lsa),
    __trc_path(flow_params.File_Path),
    __load_percent(std::min(flow_params.Percentage_To_Be_Executed,
                            100)),
    __max_replay(flow_params.Replay_Count),
    __max_reqs_in_file(0),
    __replay_count(0),
    __req_count_in_file(0),
    __time_offset(0),
    __item(),
    __trc_stream(__trc_path, std::ios::in)
{
  if (!__trc_stream.is_open())
    throw mqsim_error("Error while opening input trace file: " + __trc_path);
}

force_inline HostIORequest*
IO_Flow_Trace_Based::__generate_next_req()
{
  if (_all_request_generated())
    return nullptr;

  return _generate_request(__item.time(),
                           __item.lba(), __item.lba_count(), __item.io_type());
}

force_inline uint32_t
IO_Flow_Trace_Based::__check_valid_requests()
{
  std::cout << "Investigating input trace file: " << __trc_path << std::endl;

  std::string line;
  uint32_t valid_reqs = 0;
  sim_time_type now = 0;
  sim_time_type last = 0;

  while (std::getline(__trc_stream, line))
  {
    if (!TraceItem::check_valid(line.c_str(), line.size()))
      break;

    ++valid_reqs;

    last = now;
    now = std::strtoll(line.c_str(), nullptr, 10);

    if (now < last)
      throw mqsim_error("Unexpected request arrival time: " + to_string(now) + "\n"
                        "MQSim expects request arrival times to be "
                        "monotonically increasing in the input trace!");
  }

  std::cout << "Trace file: " << __trc_path << " seems healthy" << std::endl;

  __move_to_begin();

  return valid_reqs;
}

force_inline void
IO_Flow_Trace_Based::__move_to_begin()
{
  __trc_stream.clear();
  __trc_stream.seekg(0, std::ios::beg);
  __req_count_in_file = 0;
}

force_inline void
IO_Flow_Trace_Based::__load_item()
{
  std::string line;
  std::getline(__trc_stream, line);

  __item.parse(line, __time_offset, start_lsa, __flow_region_size);
  ++__req_count_in_file;

  Simulator->Register_sim_event(__item.time(), this);
}

void
IO_Flow_Trace_Based::Start_simulation()
{
  IO_Flow_Base::Start_simulation();

  // 1. Check trace file healthy
  const_cast<uint32_t&>(__max_reqs_in_file) = __check_valid_requests();

  // 2. Update total max request
  _update_max_req_count(__max_replay == 1
                          ? int(__load_percent / 100 * __max_reqs_in_file)
                          : __max_reqs_in_file * __max_replay);

  // 3. Load item from file
  __move_to_begin();
  __load_item();
}

void
IO_Flow_Trace_Based::Execute_simulator_event(MQSimEngine::SimEvent*)
{
  // 1. Submit next request
  if (!_submit_io_request(__generate_next_req()))
    return;

  // 2. Reload trace file if load all request in file
  if (__req_count_in_file == __max_reqs_in_file) {

    // If all requests have been generated, stop file loading.
    if (_all_request_generated())
      return;

    ++__replay_count;

    __move_to_begin();
    __time_offset = Simulator->Time();

    std::cout << "* Replay round " << (__replay_count + 1)
              << " of " << __max_replay << " started  for" << ID() << std::endl;
  }

  // 3. Load trace item
  __load_item();
}

void
IO_Flow_Trace_Based::get_stats(Utils::Workload_Statistics& stats,
                               const Utils::LhaToLpaConverterBase& convert_lha_to_lpa,
                               const Utils::NvmAccessBitmapFinderBase& find_nvm_subunit_access_bitmap)
{
  IO_Flow_Base::get_stats(stats, convert_lha_to_lpa, find_nvm_subunit_access_bitmap);

  auto& rd_arrival = stats.Read_arrival_time;
  auto& wr_arrival = stats.Write_arrival_time;
  auto& rd_sz_histo = stats.Read_size_histogram;
  auto& wr_sz_histo = stats.Write_size_histogram;
  auto& rd_acc_pattern = stats.Read_address_access_pattern;
  auto& wr_acc_pattern = stats.Write_address_access_pattern;
  auto& shared_addrs = stats.Write_read_shared_addresses;

  rd_arrival.resize(MAX_ARRIVAL_TIME_HISTOGRAM + 1, 0);
  wr_arrival.resize(MAX_ARRIVAL_TIME_HISTOGRAM + 1, 0);
  rd_sz_histo.resize(MAX_REQSIZE_HISTOGRAM_ITEMS + 1, 0);
  wr_sz_histo.resize(MAX_REQSIZE_HISTOGRAM_ITEMS + 1, 0);

  uint32_t      total_lbas = 0;
  uint32_t      total_reqs = 0;
  uint64_t      sum_request_size = 0;
  sim_time_type sum_arrival_interval = 0;
  sim_time_type last_arrival = 0;

  std::ifstream trc_stream(__trc_path, std::ios::in);
  TraceItem   item;
  std::string line;

  while (std::getline(trc_stream, line)) {
    if (!TraceItem::check_valid(line.c_str(), line.size()))
      break;

    last_arrival = item.time();

    item.parse(line, 0, start_lsa, __flow_region_size);

    /// 0. Check arrival time validation
    if (item.time() < last_arrival)
      throw mqsim_error("Unexpected request arrival time: " + to_string(item.time()) + "\n"
                        "MQSim expects request arrival times to be "
                        "monotonically increasing in the input trace!");

    /// 1. Address access pattern statistics
    auto& major_pattern   = item.is_read() ? rd_acc_pattern : wr_acc_pattern;
    auto& counter_pattern = item.is_read() ? wr_acc_pattern : rd_acc_pattern;

    LHA_type lba = item.lba();

    for (uint32_t touched = 0; touched < item.lba_count(); ++touched) {
      auto device_address = convert_lha_to_lpa(lba);
      auto access_bitmap  = find_nvm_subunit_access_bitmap(lba);

      if (major_pattern.find(device_address) == major_pattern.end())
        major_pattern[device_address] = Utils::Address_Histogram_Unit();

      ++major_pattern[device_address].Access_count;
      major_pattern[device_address].Accessed_sub_units |= access_bitmap;

      if (counter_pattern.find(device_address) != counter_pattern.end())
        shared_addrs.insert(device_address);

      ++total_lbas;

      lba = (end_lsa == lba) ? start_lsa : (lba + 1);
    }

    /// 2. Histogram statistics
    auto& arrival_time   = item.is_read() ? rd_arrival  : wr_arrival;
    auto& size_histogram = item.is_read() ? rd_sz_histo : wr_sz_histo;

    // The arrival rate histogram is stored in the microsecond unit
    sim_time_type arrival_diff = (item.time() - last_arrival) / 1000;

    ++arrival_time[std::max(arrival_diff, MAX_ARRIVAL_TIME_HISTOGRAM)];
    ++size_histogram[std::max(item.lba_count(), MAX_REQSIZE_HISTOGRAM_ITEMS)];

    /// 3. Gathering average/summation information
    sum_arrival_interval += item.time() - last_arrival;
    sum_request_size += item.lba_count();

    ++total_reqs;
  }

  trc_stream.close();

  stats.Type = Utils::Workload_Type::TRACE_BASED;
  stats.Total_generated_reqeusts = total_reqs;
  stats.Total_accessed_lbas = total_lbas;
  stats.Average_request_size_sector = uint32_t(sum_request_size / total_reqs);
  stats.Average_inter_arrival_time_nano_sec = sum_arrival_interval / total_reqs;

  stats.Replay_no = __max_replay;
}
