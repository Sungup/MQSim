#include "SATA_HBA.h"

#include "ioflow/IO_Flow_Base.h"
#include "PCIe_Root_Complex.h"

// --------------------
// Includes for builder
// --------------------
#include "../ssd/interface/Host_Interface_Base.h"

using namespace Host_Components;

SATA_HBA::SATA_HBA(sim_object_id_type id,
                   uint16_t ncq_size,
                   sim_time_type hba_processing_delay,
                   PCIe_Root_Complex& pcie_root_complex,
                   IoFlowList& IO_flows)
 : MQSimEngine::Sim_Object(std::move(id)),
   __hba_processing_delay(hba_processing_delay),
   __pcie_root_complex(pcie_root_complex),
   __flows(IO_flows),
   __nvme_queue_info(ncq_size),
   __submission_queue(__nvme_queue_info)
{ }

force_inline void
SATA_HBA::__issue_signal(int type)
{
  auto* sim = Simulator;

  sim->Register_sim_event(sim->Time() + __hba_processing_delay,
                          this,nullptr, type);
}

force_inline void
SATA_HBA::__end_of_request(HostIORequest *request)
{
  __flows[request->Source_flow_id]->consume_sata_io(request);
}

force_inline void
SATA_HBA::__enqueue_to_sq(HostIORequest *request)
{
  request->Enqueue_time = Simulator->Time();

  __submission_queue.enqueue(request);

  // Based on NVMe protocol definition, the updated tail pointer should be
  // informed to the device
  __pcie_root_complex.write_to_device(__nvme_queue_info.sq_tail_register,
                                      __nvme_queue_info.sq_tail);
}

force_inline void
SATA_HBA::__update_and_submit_cq_tail()
{
  __nvme_queue_info.move_cq_head();

  // Based on NVMe protocol definition, the updated head pointer should be
  // informed to the device
  __pcie_root_complex.write_to_device(__nvme_queue_info.cq_head_register,
                                      __nvme_queue_info.cq_head);
}

force_inline void
SATA_HBA::__submit_io_request(HostIORequest* request)
{
  // If either of software or hardware queue is full
  if (__submission_queue.is_full()) {
    __waiting_queue.push_back(request);
    return;
  }

  __enqueue_to_sq(request);
}

force_inline void
SATA_HBA::__consume_io_request(CQEntry* cqe)
{
  auto* request = __submission_queue.dequeue(cqe->Command_Identifier);

  __nvme_queue_info.sq_head = cqe->SQ_Head;

  /// MQSim always assumes that the request is processed correctly,
  /// so no need to check cqe->SF_P

  // If the submission queue is not full anymore, then enqueue waiting requests
  while (!__waiting_queue.empty() && !__submission_queue.is_full()) {
    __enqueue_to_sq(__waiting_queue.front());

    __waiting_queue.pop_front();
  }

  cqe->release();

  __update_and_submit_cq_tail();

  __end_of_request(request);
}

template <typename Queue, typename Handler>
force_inline void
SATA_HBA::__handle_signal(Queue queue, Handler handler, int signal)
{
  (this->*handler)(queue.front());
  queue.pop();

  if (!queue.empty())
    __issue_signal(signal);
}

template <typename Queue, typename Packet>
force_inline void
SATA_HBA::__issue_signal(Queue queue, Packet* packet, int signal)
{
  queue.push(packet);

  if (queue.size() == 1)
    __issue_signal(signal);
}

void
SATA_HBA::Execute_simulator_event(MQSimEngine::SimEvent* event)
{
  switch (event->Type) {
  case NVME_SIGNAL:
    __handle_signal(__cq, &SATA_HBA::__consume_io_request, NVME_SIGNAL);
    return;

  case HOST_SIGNAL:
    __handle_signal(__ncq, &SATA_HBA::__submit_io_request, HOST_SIGNAL);
    return;
  }
}

void
SATA_HBA::submit_sata_io(HostIORequest* request)
{
  __issue_signal(__ncq, request, HOST_SIGNAL);
}

void
SATA_HBA::consume_sata_io(CQEntry* cqe)
{
  __issue_signal(__cq, cqe, NVME_SIGNAL);
}

SQEntry*
SATA_HBA::read_sq_entry(uint64_t address)
{
  const auto* request = __submission_queue.request_of(address);

  if (request == nullptr)
    throw std::invalid_argument("SATA HBA: Request to access an NCQ entry "
                                "that does not exist.");

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

SataHbaPtr
Host_Components::build_sata_hba(const sim_object_id_type& id,
                                HostInterface_Types interface_type,
                                uint16_t ncq_depth,
                                sim_time_type sata_ctrl_delay,
                                PCIe_Root_Complex& root_complex,
                                IoFlowList& io_flows)
{
  if (interface_type == HostInterface_Types::SATA)
    return std::make_shared<SATA_HBA>(id,
                                      ncq_depth,
                                      sata_ctrl_delay,
                                      root_complex,
                                      io_flows);

  return nullptr;
}
