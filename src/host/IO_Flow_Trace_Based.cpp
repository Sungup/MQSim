#include "IO_Flow_Trace_Based.h"

#include "ASCII_Trace_Definition.h"
#include "../sim/Engine.h"

using namespace Host_Components;
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
                 flow_id,
                 lapu.available_start_lha(flow_id),
                 lapu.available_end_lha(flow_id),
                 FLOW_ID_TO_Q_ID(flow_id),
                 sq_size,
                 cq_size,
                 flow_params.Priority_Class,
                 0,
                 flow_params.init_occupancy_rate(),
                 0,
                 interface_type,
                 root_complex,
                 sata_hba,
                 host_params.Enable_ResponseTime_Logging,
                 host_params.ResponseTime_Logging_Period_Length,
                 host_params.stream_log_path(flow_id)),
    percentage_to_be_simulated(std::min(flow_params.Percentage_To_Be_Executed,
                                        100)),
    trace_file_path(flow_params.File_Path),
    total_replay_no(flow_params.Replay_Count),
    total_requests_in_file(0),
    replay_counter(0),
    time_offset(0),
    current_trace_line(),
    trace_file()
{ }

HostIORequest* IO_Flow_Trace_Based::Generate_next_request()
{
  if (current_trace_line.empty() || _all_request_generated())
    return nullptr;

  HostIOReqType req_type;
  uint32_t lba_count;
  LHA_type start_lba;

  if (current_trace_line[ASCIITraceTypeColumn] == ASCIITraceWriteCode)
    req_type = HostIOReqType::WRITE;
  else
    req_type = HostIOReqType::READ;

  char* pEnd;
  lba_count = std::strtoul(current_trace_line[ASCIITraceSizeColumn].c_str(), &pEnd, 0);

  start_lba = std::strtoull(current_trace_line[ASCIITraceAddressColumn].c_str(), &pEnd, 0);
  if (start_lba <= (end_lsa - start_lsa))
    start_lba += start_lsa;
  else
    start_lba = start_lsa + start_lba % (end_lsa - start_lsa);

  return _generate_request(Simulator->Time() + time_offset, start_lba, lba_count, req_type);
}

void IO_Flow_Trace_Based::Start_simulation()
{
  IO_Flow_Base::Start_simulation();
  std::string trace_line;
  char* pEnd;
  trace_file.open(trace_file_path, std::ios::in);
  if (!trace_file.is_open())
    PRINT_ERROR("Error while opening input trace file: " << trace_file_path)

  PRINT_MESSAGE("Investigating input trace file: " << trace_file_path);
  sim_time_type last_request_arrival_time = 0;
  while (std::getline(trace_file, trace_line))
  {
    Utils::Helper_Functions::Remove_cr(trace_line);
    current_trace_line.clear();
    Utils::Helper_Functions::Tokenize(trace_line, ASCIILineDelimiter, current_trace_line);
    if (current_trace_line.size() != ASCIIItemsPerLine)
      break;
    total_requests_in_file++;
    sim_time_type prev_time = last_request_arrival_time;
    last_request_arrival_time = std::strtoll(current_trace_line[ASCIITraceTimeColumn].c_str(), &pEnd, 10);
    if (last_request_arrival_time < prev_time)
      PRINT_ERROR("Unexpected request arrival time: " << last_request_arrival_time << "\nMQSim expects request arrival times to be monotonically increasing in the input trace!")
  }
  trace_file.close();
  PRINT_MESSAGE("Trace file: " << trace_file_path << " seems healthy");

  _update_max_req_count(total_replay_no == 1
                          ? int(double(percentage_to_be_simulated) / 100 * total_requests_in_file)
                          : total_requests_in_file * total_replay_no);

  trace_file.open(trace_file_path);
  current_trace_line.clear();
  std::getline(trace_file, trace_line);
  Utils::Helper_Functions::Remove_cr(trace_line);
  Utils::Helper_Functions::Tokenize(trace_line, ASCIILineDelimiter, current_trace_line);
  Simulator->Register_sim_event(std::strtoll(current_trace_line[ASCIITraceTimeColumn].c_str(), &pEnd, 10), this);
}

void IO_Flow_Trace_Based::Execute_simulator_event(MQSimEngine::SimEvent*)
{
  HostIORequest* request = Generate_next_request();
  if (request != nullptr)
    _submit_io_request(request);

  if (_all_request_generated())
    return;

  auto* sim = Simulator;

  std::string trace_line;
  if (std::getline(trace_file, trace_line))
  {
    Utils::Helper_Functions::Remove_cr(trace_line);
    current_trace_line.clear();
    Utils::Helper_Functions::Tokenize(trace_line, ASCIILineDelimiter, current_trace_line);
  }
  else
  {
    trace_file.close();
    trace_file.open(trace_file_path);
    replay_counter++;
    time_offset = sim->Time();
    std::getline(trace_file, trace_line);
    Utils::Helper_Functions::Remove_cr(trace_line);
    current_trace_line.clear();
    Utils::Helper_Functions::Tokenize(trace_line, ASCIILineDelimiter, current_trace_line);
    PRINT_MESSAGE("* Replay round "<< replay_counter << "of "<< total_replay_no << " started  for" << ID())
  }
  char* pEnd;
  sim->Register_sim_event(time_offset + std::strtoll(current_trace_line[ASCIITraceTimeColumn].c_str(), &pEnd, 10), this);
}

void
IO_Flow_Trace_Based::get_stats(Utils::Workload_Statistics& stats,
                               const Utils::LhaToLpaConverterBase& convert_lha_to_lpa,
                               const Utils::NvmAccessBitmapFinderBase& find_nvm_subunit_access_bitmap)
{
  IO_Flow_Base::get_stats(stats, convert_lha_to_lpa, find_nvm_subunit_access_bitmap);

  //In MQSim, there is a simple relation between stream id and the io_queue_id of NVMe
  stats.Type = Utils::Workload_Type::TRACE_BASED;

  for (int i = 0; i < MAX_ARRIVAL_TIME_HISTOGRAM + 1; i++)
  {
    stats.Write_arrival_time.push_back(0);
    stats.Read_arrival_time.push_back(0);
  }
  for (int i = 0; i < MAX_REQSIZE_HISTOGRAM_ITEMS + 1; i++)
  {
    stats.Write_size_histogram.push_back(0);
    stats.Read_size_histogram.push_back(0);
  }
  stats.Total_generated_reqeusts = 0;
  stats.Total_accessed_lbas = 0;

  std::ifstream trace_file_temp;
  trace_file_temp.open(trace_file_path, std::ios::in);
  if (!trace_file_temp.is_open())
  PRINT_ERROR("Error while opening the input trace file!")

  std::string trace_line;
  char* pEnd;
  sim_time_type last_request_arrival_time = 0;
  sim_time_type sum_inter_arrival = 0;
  uint64_t sum_request_size = 0;
  std::vector<std::string> line_splitted;
  while (std::getline(trace_file_temp, trace_line))
  {
    Utils::Helper_Functions::Remove_cr(trace_line);
    line_splitted.clear();
    Utils::Helper_Functions::Tokenize(trace_line, ASCIILineDelimiter, line_splitted);
    if (line_splitted.size() != ASCIIItemsPerLine)
      break;
    sim_time_type prev_time = last_request_arrival_time;
    last_request_arrival_time = std::strtoull(line_splitted[ASCIITraceTimeColumn].c_str(), &pEnd, 10);
    if (last_request_arrival_time < prev_time)
    PRINT_ERROR("Unexpected request arrival time: " << last_request_arrival_time << "\nMQSim expects request arrival times to be monotonic increasing in the input trace!")
    sim_time_type diff = (last_request_arrival_time - prev_time) / 1000;//The arrival rate histogram is stored in the microsecond unit
    sum_inter_arrival += last_request_arrival_time - prev_time;

    uint32_t LBA_count = std::strtoul(line_splitted[ASCIITraceSizeColumn].c_str(), &pEnd, 0);
    sum_request_size += LBA_count;
    LHA_type start_LBA = std::strtoull(line_splitted[ASCIITraceAddressColumn].c_str(), &pEnd, 0);
    if (start_LBA <= (end_lsa - start_lsa))
      start_LBA += start_lsa;
    else
      start_LBA = start_lsa + start_LBA % (end_lsa - start_lsa);
    LHA_type end_LBA = start_LBA + LBA_count - 1;
    if (end_LBA > end_lsa)
      end_LBA = start_lsa + (end_LBA - end_lsa) - 1;


    //Address access pattern statistics
    while (start_LBA <= end_LBA)
    {
      LPA_type device_address = convert_lha_to_lpa(start_LBA);
      page_status_type access_status_bitmap = find_nvm_subunit_access_bitmap(start_LBA);
      if (line_splitted[ASCIITraceTypeColumn] == ASCIITraceWriteCode)
      {
        if (stats.Write_address_access_pattern.find(device_address) == stats.Write_address_access_pattern.end())
        {
          Utils::Address_Histogram_Unit hist;
          hist.Access_count = 1;
          hist.Accessed_sub_units = access_status_bitmap;
          stats.Write_address_access_pattern[device_address] = hist;
        }
        else
        {
          stats.Write_address_access_pattern[device_address].Access_count = stats.Write_address_access_pattern[device_address].Access_count + 1;
          stats.Write_address_access_pattern[device_address].Accessed_sub_units = stats.Write_address_access_pattern[device_address].Accessed_sub_units | access_status_bitmap;
        }

        if (stats.Read_address_access_pattern.find(device_address) != stats.Read_address_access_pattern.end())
          stats.Write_read_shared_addresses.insert(device_address);
      }
      else
      {
        if (stats.Read_address_access_pattern.find(device_address) == stats.Read_address_access_pattern.end())
        {
          Utils::Address_Histogram_Unit hist;
          hist.Access_count = 1;
          hist.Accessed_sub_units = access_status_bitmap;
          stats.Read_address_access_pattern[device_address] = hist;
        }
        else
        {
          stats.Read_address_access_pattern[device_address].Access_count = stats.Read_address_access_pattern[device_address].Access_count + 1;
          stats.Read_address_access_pattern[device_address].Accessed_sub_units = stats.Read_address_access_pattern[device_address].Accessed_sub_units | access_status_bitmap;
        }

        if (stats.Write_address_access_pattern.find(device_address) != stats.Write_address_access_pattern.end())
          stats.Write_read_shared_addresses.insert(device_address);
      }
      stats.Total_accessed_lbas++;
      start_LBA++;
      if (start_LBA > end_lsa)
        start_LBA = start_lsa;
    }

    //Request size statistics
    if (line_splitted[ASCIITraceTypeColumn] == ASCIITraceWriteCode)
    {
      if (diff < MAX_ARRIVAL_TIME_HISTOGRAM)
        stats.Write_arrival_time[diff]++;
      else
        stats.Write_arrival_time[MAX_ARRIVAL_TIME_HISTOGRAM]++;
      if (LBA_count < MAX_REQSIZE_HISTOGRAM_ITEMS)
        stats.Write_size_histogram[LBA_count]++;
      else stats.Write_size_histogram[MAX_REQSIZE_HISTOGRAM_ITEMS]++;
    }
    else
    {
      if (diff < MAX_ARRIVAL_TIME_HISTOGRAM)
        stats.Read_arrival_time[diff]++;
      else
        stats.Read_arrival_time[MAX_ARRIVAL_TIME_HISTOGRAM]++;
      if (LBA_count < MAX_REQSIZE_HISTOGRAM_ITEMS)
        stats.Read_size_histogram[LBA_count]++;
      else stats.Read_size_histogram[(uint32_t)MAX_REQSIZE_HISTOGRAM_ITEMS]++;
    }
    stats.Total_generated_reqeusts++;
  }
  trace_file_temp.close();
  stats.Average_request_size_sector = (uint32_t)(sum_request_size / stats.Total_generated_reqeusts);
  stats.Average_inter_arrival_time_nano_sec = sum_inter_arrival / stats.Total_generated_reqeusts;

  stats.Replay_no = total_replay_no;
}
