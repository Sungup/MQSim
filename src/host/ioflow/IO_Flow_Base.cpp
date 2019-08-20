#include "IO_Flow_Base.h"

#include "../../sim/Engine.h"

#include "../PCIe_Root_Complex.h"
#include "../SATA_HBA.h"

// Children for IO_Flow_Base
#include "IO_Flow_Synthetic.h"
#include "IO_Flow_Trace_Based.h"

using namespace Host_Components;

IO_Flow_Base::IO_Flow_Base(const sim_object_id_type& name,
                           const HostParameterSet& host_params,
                           const IOFlowParamSet& flow_params,
                           const Utils::LogicalAddrPartition& lapu,
                           uint16_t flow_id,
                           LHA_type start_lsa,
                           LHA_type end_lsa,
                           uint16_t sq_size,
                           uint16_t cq_size,
                           uint32_t max_request_count,
                           HostInterface_Types interface_type,
                           PCIe_Root_Complex* root_complex,
                           SATA_HBA* sata_hba)
  : MQSimEngine::Sim_Object(name),
    __flow_id(flow_id),
    __priority_class(flow_params.Priority_Class),
    __max_req_count(max_request_count),
    __initial_occupancy_ratio(flow_params.init_occupancy_rate()),
    __pcie_root_complex(root_complex),
    __sata_hba(sata_hba),
    __nvme_queue_pair(flow_id_to_qid(flow_id), sq_size, cq_size),
    __submission_queue(__nvme_queue_pair),
    __host_io_req_pool(),
    __sq_entry_pool(),
    __req_submitter(this,
                    interface_type == HostInterface_Types::NVME
                      ? &IO_Flow_Base::__submit_nvme_request
                      : &IO_Flow_Base::__submit_sata_request),
    __stats(),
    __log(host_params, flow_id),
    __progress_bar(ID()),
    start_lsa(start_lsa),
    end_lsa(end_lsa)
{ }

void
IO_Flow_Base::Start_simulation()
{
  __log.init();
}

force_inline void
IO_Flow_Base::__update_stats_by_request(sim_time_type now,
                                        const HostIORequest* request)
{
  sim_time_type dev_response = now - request->Enqueue_time;
  sim_time_type req_delay = now - request->Arrival_time;

  __stats.update_response(request, dev_response, req_delay);
  __log.update_response(dev_response, req_delay);

  __log.logging(now);
}

force_inline void
IO_Flow_Base::__enqueue_to_sq(HostIORequest* request)
{
  request->Enqueue_time = Simulator->Time();

  __submission_queue.enqueue(request);

  // Based on NVMe protocol definition, the updated tail pointer should be
  // informed to the device
  __pcie_root_complex->write_to_device(__nvme_queue_pair.sq_tail_register,
                                       __nvme_queue_pair.sq_tail);
}

force_inline void
IO_Flow_Base::__end_of_request(HostIORequest* request)
{
  __update_stats_by_request(Simulator->Time(), request);

  request->release();

  __progress_bar.update(this->_get_progress());
}

force_inline void
IO_Flow_Base::__update_and_submit_cq_tail()
{
  __nvme_queue_pair.move_cq_head();

  // Based on NVMe protocol definition, the updated head pointer should be
  // informed to the device
  __pcie_root_complex->write_to_device(__nvme_queue_pair.cq_head_register,
                                       __nvme_queue_pair.cq_head);
}

void
IO_Flow_Base::__submit_nvme_request(HostIORequest *request)
{
  // If either of software or hardware queue is full
  if (__submission_queue.is_full()) {
    __waiting_queue.emplace_back(request);
    return;
  }

  __enqueue_to_sq(request);
}

void
IO_Flow_Base::__submit_sata_request(HostIORequest *request)
{
  request->Source_flow_id = __flow_id;
  __sata_hba->Submit_io_request(request);
}

int
IO_Flow_Base::_get_progress() const
{
  return int(double(__stats.serviced_req()) / __max_req_count * 100);
}

void
IO_Flow_Base::consume_nvme_io(CQEntry* cqe)
{
  auto* request = __submission_queue.dequeue(cqe->Command_Identifier);

  __nvme_queue_pair.sq_head = cqe->SQ_Head;

  /// MQSim always assumes that the request is processed correctly,
  /// so no need to check cqe->SF_P

  // If the submission queue is not full anymore, then enqueue waiting requests
  while(!__waiting_queue.empty() && !__submission_queue.is_full()) {
    __enqueue_to_sq(__waiting_queue.front());

    __waiting_queue.pop_front();
  }

  cqe->release();

  __update_and_submit_cq_tail();

  __end_of_request(request);
}

void
IO_Flow_Base::consume_sata_io(HostIORequest* request)
{
  __end_of_request(request);
}

void
IO_Flow_Base::get_stats(Utils::Workload_Statistics& stats,
                        const Utils::LhaToLpaConverterBase& convert_lha_to_lpa,
                        const Utils::NvmAccessBitmapFinderBase& find_nvm_subunit_access_bitmap)
{
  stats.Stream_id = __flow_id;
  stats.Initial_occupancy_ratio = __initial_occupancy_ratio;

  stats.Min_LHA = start_lsa;
  stats.Max_LHA = end_lsa;
}

SQEntry* IO_Flow_Base::read_nvme_sqe(uint64_t address)
{
  const auto* request = __submission_queue.request_of(address);

  if (request == nullptr)
    throw std::invalid_argument(ID() + ": Request to access a submission "
                                       "queue entry that does not exist.");

  /// Currently generated sq entries information are same between READ and WRITE
  uint8_t op_code = (request->Type == HostIOReqType::READ)
                      ? NVME_READ_OPCODE : NVME_WRITE_OPCODE;

  return __sq_entry_pool.construct(op_code,
                                   request->IO_queue_info,
                                   (DATA_MEMORY_REGION),
                                   (DATA_MEMORY_REGION + 0x1000),
                                   lsb_of_lba(request->Start_LBA),
                                   msb_of_lba(request->Start_LBA),
                                   lsb_of_lba_count(request->LBA_count));
}

void IO_Flow_Base::Report_results_in_XML(std::string name_prefix,
                                         Utils::XmlWriter& xmlwriter)
{
  xmlwriter.Write_open_tag(name_prefix + ".IO_Flow");

  xmlwriter.Write_attribute_string("Name", ID());

  __stats.Report_results_in_XML(name_prefix, xmlwriter);

  xmlwriter.Write_close_tag();
}

// ---------------
// IO Flow builder
// ---------------

IoFlowPtr
Host_Components::build_io_flow(const sim_object_id_type& host_id,
                               const HostParameterSet& host_params,
                               const IOFlowParamSet& flow_params,
                               const Utils::LogicalAddrPartition& lapu,
                               uint16_t flow_id,
                               Host_Components::PCIe_Root_Complex& root_complex,
                               Host_Components::SATA_HBA* sata_hba,
                               HostInterface_Types interface_type,
                               uint16_t sq_size,
                               uint16_t cq_size)
{
  auto& synthetic_params = (SyntheticFlowParamSet&) flow_params;
  auto& trace_params     = (TraceFlowParameterSet&) flow_params;

  switch (flow_params.Type) {
  case Flow_Type::SYNTHETIC:
    return Host_Components::build_synthetic_flow(
      host_id + ".IO_Flow.Synth.No_" + std::to_string(flow_id),
      host_params,
      synthetic_params,
      lapu,
      flow_id,
      sq_size,
      cq_size,
      interface_type,
      root_complex,
      sata_hba
    );

  case Flow_Type::TRACE:
    return std::make_shared<IO_Flow_Trace_Based>(
      host_id + ".IO_Flow.Trace." + trace_params.File_Path,
      host_params,
      trace_params,
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
