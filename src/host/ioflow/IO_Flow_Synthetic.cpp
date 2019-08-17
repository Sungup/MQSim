#include "IO_Flow_Synthetic.h"

#include <stdexcept>
#include "../../sim/Engine.h"

using namespace Host_Components;

/// ---------------------------------------
/// 0. Abstract class for IO flow synthetic
/// ---------------------------------------
IO_Flow_Synthetic::IO_Flow_Synthetic(const sim_object_id_type& name,
                                     const HostParameterSet& host_params,
                                     const SyntheticFlowParamSet& flow_params,
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
                 LHA_type(lapu.available_start_lha(flow_id)
                           + (double(lapu.available_end_lha(flow_id)- lapu.available_start_lha(flow_id))
                               * flow_params.working_set_rate())),
                 sq_size,
                 cq_size,
                 flow_params.Total_Requests_To_Generate,
                 interface_type,
                 root_complex,
                 sata_hba),
    __run_until(flow_params.Stop_Time),
    __read_ratio(flow_params.read_rate() != 0.0
                  ? flow_params.read_rate() : -1.0),
    __working_set_ratio(flow_params.working_set_rate()),
    __req_type_generator(flow_params.gen_seed()),
    __address_distributor(build_address_distributor(flow_params,
                                                    start_lsa, end_lsa)),
    __req_size_distributor(build_req_size_distributor(flow_params))
{
  if (start_lsa > end_lsa)
    throw std::logic_error("Problem in IO Flow Synthetic, the start LBA "
                           "address is greater than the end LBA address");

  if (__working_set_ratio == 0)
    throw mqsim_error("The working set ratio is set to zero "
                      "for workload " + name);
}

force_inline HostIORequest*
IO_Flow_Synthetic::__generate_next_req()
{
  if ((__run_until > 0 && Simulator->Time() > __run_until) ||
      (__run_until == 0 && _all_request_generated()))
    return nullptr;

  HostIOReqType req_type = __req_type_generator.Uniform(0, 1) <= __read_ratio
                             ? HostIOReqType::READ
                             : HostIOReqType::WRITE;

  uint32_t lba_count = __req_size_distributor->generate();
  LHA_type start_lba = __address_distributor->generate(lba_count);

  PRINT_DEBUG("* Host: Request generated - "
                << to_string(req_type) << ", "
                << "LBA:" << start_lba << ", "
                << "Size_in_bytes:" << lba_count << "")

  return _generate_request(Simulator->Time(), start_lba, lba_count, req_type);
}

int
IO_Flow_Synthetic::_get_progress() const
{
  return __run_until == 0
         ? IO_Flow_Base::_get_progress()
         : int(double(Simulator->Time()) / __run_until * 100);
}

force_inline bool
IO_Flow_Synthetic::_submit_next_request()
{
  return _submit_io_request(__generate_next_req());
}

void
IO_Flow_Synthetic::Start_simulation()
{
  IO_Flow_Base::Start_simulation();

  __address_distributor->init();
}

void
IO_Flow_Synthetic::get_stats(Utils::Workload_Statistics& stats,
                             const Utils::LhaToLpaConverterBase& convert_lha_to_lpa,
                             const Utils::NvmAccessBitmapFinderBase& find_nvm_subunit_access_bitmap)
{
  IO_Flow_Base::get_stats(stats, convert_lha_to_lpa, find_nvm_subunit_access_bitmap);

  stats.Type = Utils::Workload_Type::SYNTHETIC;
  stats.Working_set_ratio = __working_set_ratio;
  stats.Read_ratio = __read_ratio;

  __address_distributor->get_stats(stats);
  __req_size_distributor->get_stats(stats);

  stats.random_request_type_generator_seed = __req_type_generator.seed;
}

/// -------------------------------
/// 1. QD managed synthetic io flow
/// -------------------------------
IoFlowSyntheticQD::IoFlowSyntheticQD(const sim_object_id_type& name,
                                     const HostParameterSet& host_params,
                                     const SyntheticFlowParamSet& flow_params,
                                     const Utils::LogicalAddrPartition& lapu,
                                     uint16_t flow_id,
                                     uint16_t nvme_submission_queue_size,
                                     uint16_t nvme_completion_queue_size,
                                     HostInterface_Types SSD_device_type,
                                     PCIe_Root_Complex *pcie_root_complex,
                                     SATA_HBA *sata_hba)
  : IO_Flow_Synthetic(name,
                      host_params,
                      flow_params,
                      lapu,
                      flow_id,
                      nvme_submission_queue_size,
                      nvme_completion_queue_size,
                      SSD_device_type,
                      pcie_root_complex,
                      sata_hba),
    __target_queue_depth(flow_params.Average_No_of_Reqs_in_Queue)
{ }

void
IoFlowSyntheticQD::Start_simulation()
{
  IO_Flow_Synthetic::Start_simulation();

  Simulator->Register_sim_event(1, this, nullptr, 0);
}

/*
 * In the demand based execution mode, the __generate_next_req() function
 * may return nullptr if 1) the simulation stop is met, or 2) the number of
 * generated I/O requests reaches its threshold.
 */
void
IoFlowSyntheticQD::consume_nvme_io(CQEntry *cqe)
{
  IO_Flow_Base::consume_nvme_io(cqe);

  _submit_next_request();
}

void
IoFlowSyntheticQD::consume_sata_io(Host_Components::HostIORequest *request)
{
  IO_Flow_Base::consume_sata_io(request);

  _submit_next_request();
}

void
IoFlowSyntheticQD::Execute_simulator_event(MQSimEngine::SimEvent* /* event */)
{
  /*
   * QUEUE_DEPTH policy generate next request when a request would have
   * completed. So there is no need register next simulator event.
   * Also, this block only entered only one time.
   */
  for (uint32_t i = 0; i < __target_queue_depth; i++)
    _submit_next_request();
}

void
IoFlowSyntheticQD::get_stats(Utils::Workload_Statistics& stats,
                             const Utils::LhaToLpaConverterBase& convert_lha_to_lpa,
                             const Utils::NvmAccessBitmapFinderBase& find_nvm_subunit_access_bitmap)
{
  IO_Flow_Synthetic::get_stats(stats, convert_lha_to_lpa, find_nvm_subunit_access_bitmap);

  stats.generator_type = Utils::RequestFlowControlType::QUEUE_DEPTH;
  stats.Request_queue_depth = __target_queue_depth;
}

/// --------------------------------------
/// 2. Bandwidth managed synthetic io flow
/// --------------------------------------
IoFlowSyntheticBW::IoFlowSyntheticBW(const sim_object_id_type& name,
                                     const HostParameterSet& host_params,
                                     const SyntheticFlowParamSet& flow_params,
                                     const Utils::LogicalAddrPartition& lapu,
                                     uint16_t flow_id,
                                     uint16_t nvme_submission_queue_size,
                                     uint16_t nvme_completion_queue_size,
                                     HostInterface_Types SSD_device_type,
                                     PCIe_Root_Complex *pcie_root_complex,
                                     SATA_HBA *sata_hba)
  : IO_Flow_Synthetic(name,
                      host_params,
                      flow_params,
                      lapu,
                      flow_id,
                      nvme_submission_queue_size,
                      nvme_completion_queue_size,
                      SSD_device_type,
                      pcie_root_complex,
                      sata_hba),
    __average_req_interval(flow_params.avg_arrival_time()),
    __req_interval_generator(flow_params.gen_seed())
{ }

force_inline sim_time_type
IoFlowSyntheticBW::__next_trigger_time()
{
  return Simulator->Time()
           + __req_interval_generator.Exponential(__average_req_interval);
}

void
IoFlowSyntheticBW::Start_simulation()
{
  IO_Flow_Synthetic::Start_simulation();

  Simulator->Register_sim_event(__next_trigger_time(), this, nullptr, 0);
}

void
IoFlowSyntheticBW::Execute_simulator_event(MQSimEngine::SimEvent* /* event */)
{
  /*
   * BANDWIDTH policy generate next request periodically. So, this block
   * entered periodically enqueuing event into simulator object after
   * _submit_io_request().
   */
  if (_submit_next_request())
    Simulator->Register_sim_event(__next_trigger_time(), this, nullptr, 0);
}

void
IoFlowSyntheticBW::get_stats(Utils::Workload_Statistics& stats,
                             const Utils::LhaToLpaConverterBase& convert_lha_to_lpa,
                             const Utils::NvmAccessBitmapFinderBase& find_nvm_subunit_access_bitmap)
{
  IO_Flow_Synthetic::get_stats(stats, convert_lha_to_lpa, find_nvm_subunit_access_bitmap);

  stats.generator_type = Utils::RequestFlowControlType::BANDWIDTH;
  stats.Average_inter_arrival_time_nano_sec = __average_req_interval;
  stats.random_time_interval_generator_seed = __req_interval_generator.seed;
}

/// ----------------------------
/// 3. Synthetic IO flow builder
/// ----------------------------
IoFlowPtr
Host_Components::build_synthetic_flow(const sim_object_id_type& name,
                                      const HostParameterSet& host_params,
                                      const SyntheticFlowParamSet& flow_params,
                                      const Utils::LogicalAddrPartition& lapu,
                                      uint16_t flow_id,
                                      uint16_t sq_size,
                                      uint16_t cq_size,
                                      HostInterface_Types interface_type,
                                      PCIe_Root_Complex& root_complex,
                                      SATA_HBA* sata_hba)
{
  switch (flow_params.Synthetic_Generator_Type) {
  case Utils::RequestFlowControlType::QUEUE_DEPTH:
    return std::make_shared<IoFlowSyntheticQD>(
      name,
      host_params,
      flow_params,
      lapu,
      flow_id,
      sq_size,
      cq_size,
      interface_type,
      &root_complex,
      sata_hba
    );

  case Utils::RequestFlowControlType::BANDWIDTH:
    return std::make_shared<IoFlowSyntheticBW>(
      name,
      host_params,
      flow_params,
      lapu,
      flow_id,
      sq_size,
      cq_size,
      interface_type,
      &root_complex,
      sata_hba
    );
  }
}
