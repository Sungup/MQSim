#include <cmath>
#include <stdexcept>
#include "../sim/Engine.h"
#include "IO_Flow_Synthetic.h"

using namespace Host_Components;

IO_Flow_Synthetic::IO_Flow_Synthetic(const sim_object_id_type& name,
                                     const HostParameterSet& host_params,
                                     const SyntheticFlowParamSet& flow_params,
                                     const Utils::LogicalAddrPartition& lapu,
                                     uint16_t flow_id,
                                     uint16_t nvme_submission_queue_size,
                                     uint16_t nvme_completion_queue_size,
                                     HostInterface_Types SSD_device_type,
                                     PCIe_Root_Complex* pcie_root_complex,
                                     SATA_HBA* sata_hba)
  : IO_Flow_Base(name,
                 flow_id,
                 lapu.available_start_lha(flow_id),
                 LHA_type(lapu.available_start_lha(flow_id)
                           + (double(lapu.available_end_lha(flow_id)- lapu.available_start_lha(flow_id))
                               * flow_params.working_set_rate())),
                 FLOW_ID_TO_Q_ID(flow_id),
                 nvme_submission_queue_size,
                 nvme_completion_queue_size,
                 flow_params.Priority_Class,
                 flow_params.Stop_Time,
                 flow_params.init_occupancy_rate(),
                 flow_params.Total_Requests_To_Generate,
                 SSD_device_type,
                 pcie_root_complex,
                 sata_hba,
                 host_params.Enable_ResponseTime_Logging,
                 host_params.ResponseTime_Logging_Period_Length,
                 host_params.stream_log_path(flow_id)),
    __run_until(flow_params.Stop_Time),
    __read_ratio(flow_params.read_rate() != 0.0
                  ? flow_params.read_rate() : -1.0),
    __working_set_ratio(flow_params.working_set_rate()),
    __req_type_generator(flow_params.gen_seed()),
    __address_generator(flow_params.gen_seed()),
    __hot_addr_generator(flow_params.gen_seed()),
    __hot_cold_generator(flow_params.gen_seed()),
    __req_size_generator(flow_params.gen_seed()),
    __req_interval_generator(flow_params.gen_seed()),
    address_distribution(flow_params.Address_Distribution),
    __address_aligned(flow_params.Generated_Aligned_Addresses),
    __align_unit(flow_params.Address_Alignment_Unit),
    hot_region_ratio(flow_params.hot_region_rate()),
    hot_region_end_lsa(start_lsa
                         + LHA_type(double(end_lsa - start_lsa)
                                      * hot_region_ratio)),
    cold_region_start_lsa(hot_region_end_lsa + 1),
    streaming_next_address(0),
    request_size_distribution(flow_params.Request_Size_Distribution),
    average_request_size(flow_params.Average_Request_Size),
    variance_request_size(flow_params.Variance_Request_Size),
    __flow_control_type(flow_params.Synthetic_Generator_Type),
    __average_req_interval(flow_params.avg_arrival_time()),
    __target_queue_depth(flow_params.Average_No_of_Reqs_in_Queue)
{
  if (start_lsa > end_lsa)
    throw std::logic_error("Problem in IO Flow Synthetic, the start LBA "
                           "address is greater than the end LBA address");

  if (__working_set_ratio == 0)
    throw mqsim_error("The working set ratio is set to zero "
                      "for workload " + name);
}

force_inline LHA_type
IO_Flow_Synthetic::__align_address(LHA_type lba) const
{
  return lba - (__address_aligned ? lba % __align_unit : 0);
}

force_inline LHA_type
IO_Flow_Synthetic::__align_address(LHA_type lba, int lba_count,
                                   LHA_type start, LHA_type end) const
{
  if (lba < start || end < lba)
    throw mqsim_error("Generated address is out of range in IO_Flow_Synthetic");

  if (end < lba + lba_count)
    lba = start;

  return __align_address(lba);
}

force_inline LHA_type
IO_Flow_Synthetic::__gen_streaming_start_lba(uint32_t lba_count)
{
  LHA_type lba;

  if (end_lsa < streaming_next_address + lba_count) {
    lba = start_lsa;
    streaming_next_address = start_lsa;
  } else {
    lba = streaming_next_address;
    streaming_next_address += lba_count;
  }

  /*
   * Next streaming address should be larger than start_lsa. But aligning
   * address using __align_address return smaller address than argument named
   * lba. So, streaming_next_address should add __align_unit of return of
   * __align_address()
   */
  if (__address_aligned)
    streaming_next_address = __align_address(streaming_next_address)
                               + __align_unit;

  if(streaming_next_address == lba)
    PRINT_MESSAGE("Synthetic Message Generator: The same address is "
                  "always repeated due to configuration parameters!")

  return __align_address(lba);
}

force_inline LHA_type
IO_Flow_Synthetic::__gen_hot_cold_start_lba(uint32_t lba_count)
{
  LHA_type region_start;
  LHA_type region_end;

  /*
   * Hot/Cold Region
   *
   * address: |<---------------------------->|<------------------------------->|
   *          |<- start_lsa  hot_region_end->|<- cold_region_start    end_lsa->|
   *          |<---- (100 - hot_ratio%) ---->|<---------- hot_ratio% --------->|
   *
   * (100-hot)% of requests going to hot% of the address space
   *   - example: Only 10% address generate 90% IO request.
   */

  if (__hot_cold_generator.Uniform(0, 1) < hot_region_ratio) {
    region_start = cold_region_start_lsa;
    region_end = end_lsa;
  } else {
    region_start = start_lsa;
    region_end = hot_region_end_lsa;
  }

  return __align_address(__hot_addr_generator.Uniform_ulong(region_start,
                                                            region_end),
                         lba_count, region_start, region_end);
}

force_inline LHA_type
IO_Flow_Synthetic::__gen_uniform_start_lba(uint32_t lba_count)
{
  return __align_address(__address_generator.Uniform_ulong(start_lsa, end_lsa),
                         lba_count, start_lsa, end_lsa);
}

force_inline LHA_type
IO_Flow_Synthetic::__gen_start_lba(uint32_t lba_count)
{
  // Get request address
  switch (address_distribution) {
  case Utils::Address_Distribution_Type::STREAMING:
    return __gen_streaming_start_lba(lba_count);

  case Utils::Address_Distribution_Type::RANDOM_HOTCOLD:
    return __gen_hot_cold_start_lba(lba_count);

  case Utils::Address_Distribution_Type::RANDOM_UNIFORM:
    return __gen_uniform_start_lba(lba_count);
  }
}

force_inline uint32_t
IO_Flow_Synthetic::__gen_req_size()
{
  switch (request_size_distribution) {
    case Utils::Request_Size_Distribution_Type::FIXED:
      return average_request_size;

    case Utils::Request_Size_Distribution_Type::NORMAL:
      return std::max(uint32_t(ceil(__req_size_generator.Normal(average_request_size,
                                                                variance_request_size))),
                      1U);
  }
}

force_inline sim_time_type
IO_Flow_Synthetic::__next_trigger_time()
{
  return Simulator->Time()
         + __req_interval_generator.Exponential(__average_req_interval);
}

force_inline void
IO_Flow_Synthetic::__register_next_sim_event(sim_time_type next)
{
  Simulator->Register_sim_event(next, this, nullptr, 0);
}

force_inline HostIOReqType
IO_Flow_Synthetic::__gen_req_type()
{
  return __req_type_generator.Uniform(0, 1) <= __read_ratio
         ? HostIOReqType::READ
         : HostIOReqType::WRITE;
}

force_inline HostIORequest*
IO_Flow_Synthetic::__generate_next_req()
{
  if ((__run_until > 0 && Simulator->Time() > __run_until) ||
      (__run_until == 0 && _all_request_generated()))
    return nullptr;

  uint32_t lba_count = __gen_req_size();
  LHA_type start_lba = __gen_start_lba(lba_count);
  HostIOReqType req_type = __gen_req_type();

  PRINT_DEBUG("* Host: Request generated - "
                << to_string(req_type) << ", "
                << "LBA:" << start_lba << ", "
                << "Size_in_bytes:" << lba_count << "")

  return _generate_request(Simulator->Time(), start_lba, lba_count, req_type);
}

force_inline void
IO_Flow_Synthetic::__submit_next_request()
{
  /*
   * In the demand based execution mode, the __generate_next_req() function
   * may return nullptr if 1) the simulation stop is met, or 2) the number of
   * generated I/O requests reaches its threshold.
   */
  auto* request = __generate_next_req();

  if (request != nullptr)
    _submit_io_request(request);
}

void
IO_Flow_Synthetic::consume_nvme_io(CQEntry* io_request)
{
  IO_Flow_Base::consume_nvme_io(io_request);

  if (__flow_control_type == Utils::RequestFlowControlType::QUEUE_DEPTH)
    __submit_next_request();
}

void
IO_Flow_Synthetic::consume_sata_io(HostIORequest* io_request)
{
  IO_Flow_Base::consume_sata_io(io_request);

  if (__flow_control_type == Utils::RequestFlowControlType::QUEUE_DEPTH)
    __submit_next_request();
}

void
IO_Flow_Synthetic::Start_simulation()
{
  IO_Flow_Base::Start_simulation();

  if (address_distribution == Utils::Address_Distribution_Type::STREAMING)
    streaming_next_address = __align_address(__address_generator.Uniform_ulong(start_lsa, end_lsa));

  if (__flow_control_type == Utils::RequestFlowControlType::QUEUE_DEPTH)
    __register_next_sim_event(1);

  else
    __register_next_sim_event(__next_trigger_time());
}

void
IO_Flow_Synthetic::Execute_simulator_event(MQSimEngine::SimEvent* /* event */)
{
  if (__flow_control_type == Utils::RequestFlowControlType::QUEUE_DEPTH){
    /*
     * QUEUE_DEPTH policy generate next request when a request would have
     * completed. So there is no need register next simulator event.
     * Also, this block only entered only one time.
     */
    for (uint32_t i = 0; i < __target_queue_depth; i++)
      _submit_io_request(__generate_next_req());

  } else {
    /*
     * BANDWIDTH policy generate next request periodically. So, this block
     * entered periodically enqueuing event into simulator object after
     * _submit_io_request().
     */
    auto* req = __generate_next_req();

    if (req) {
      _submit_io_request(req);

      __register_next_sim_event(__next_trigger_time());
    }
  }
}

void
IO_Flow_Synthetic::get_stats(Utils::Workload_Statistics& stats,
                             const Utils::LhaToLpaConverterBase& convert_lha_to_lpa,
                             const Utils::NvmAccessBitmapFinderBase& find_nvm_subunit_access_bitmap)
{
  IO_Flow_Base::get_stats(stats, convert_lha_to_lpa, find_nvm_subunit_access_bitmap);

  stats.Type = Utils::Workload_Type::SYNTHETIC;
  stats.generator_type = __flow_control_type;
  stats.Working_set_ratio = __working_set_ratio;
  stats.Read_ratio = __read_ratio;
  stats.random_request_type_generator_seed = __req_type_generator.seed;
  stats.Address_distribution_type = address_distribution;
  stats.Ratio_of_hot_addresses_to_whole_working_set = hot_region_ratio;
  stats.Ratio_of_traffic_accessing_hot_region = 1 - hot_region_ratio;
  stats.random_address_generator_seed = __address_generator.seed;
  stats.random_hot_address_generator_seed = __hot_addr_generator.seed;
  stats.random_hot_cold_generator_seed = __hot_cold_generator.seed;
  stats.generate_aligned_addresses = __address_aligned;
  stats.alignment_value = __align_unit;
  stats.Request_size_distribution_type = request_size_distribution;
  stats.Average_request_size_sector = average_request_size;
  stats.STDEV_reuqest_size = variance_request_size;
  stats.random_request_size_generator_seed = __req_size_generator.seed;
  stats.Request_queue_depth = __target_queue_depth;
  stats.random_time_interval_generator_seed = __req_interval_generator.seed;
  stats.Average_inter_arrival_time_nano_sec = __average_req_interval;
}

int
IO_Flow_Synthetic::_get_progress() const
{
  return __run_until == 0
           ? IO_Flow_Base::_get_progress()
           : int(double(Simulator->Time()) / __run_until * 100);
}
