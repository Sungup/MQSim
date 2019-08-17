#include "SATA_HBA.h"

#include "ioflow/IO_Flow_Base.h"
#include "PCIe_Root_Complex.h"

// --------------------
// Includes for builder
// --------------------
#include "../ssd/interface/Host_Interface_Base.h"
#include "../ssd/interface/Host_Interface_SATA.h"

using namespace Host_Components;

SATA_HBA::SATA_HBA(sim_object_id_type id,
                   uint16_t ncq_size,
                   sim_time_type hba_processing_delay,
                   PCIe_Root_Complex& pcie_root_complex,
                   IoFlowList& IO_flows)
 : MQSimEngine::Sim_Object(id),
   ncq_size(ncq_size),
   hba_processing_delay(hba_processing_delay),
   pcie_root_complex(pcie_root_complex),
   IO_flows(IO_flows),
   sata_ncq(ncq_size)
{
  for (uint16_t cmdid = 0; cmdid < (uint16_t)(0xffffffff); cmdid++)
    available_command_ids.insert(cmdid);
  HostIORequest* t = nullptr;
  for (uint16_t cmdid = 0; cmdid < ncq_size; cmdid++)
    request_queue_in_memory.push_back(t);
}

void SATA_HBA::Start_simulation()
{}

void SATA_HBA::Validate_simulation_config()
{}

void SATA_HBA::Execute_simulator_event(MQSimEngine::SimEvent* event)
{
  auto* sim = Simulator;
  switch ((HBA_Sim_Events)event->Type)
  {
  case HBA_Sim_Events::CONSUME_IO_REQUEST:
  {
    CQEntry* cqe = consume_requests.front();
    consume_requests.pop();
    //Find the request and update statistics
    HostIORequest* request = sata_ncq.queue[cqe->Command_Identifier];
    sata_ncq.queue.erase(cqe->Command_Identifier);
    available_command_ids.insert(cqe->Command_Identifier);
    sata_ncq.sq_head = cqe->SQ_Head;

    IO_flows[request->Source_flow_id]->consume_sata_io(request);

    Update_and_submit_ncq_completion_info();

    //If the submission queue is not full anymore, then enqueue waiting requests
    while (waiting_requests_for_submission.size() > 0)
      if (!sata_ncq.sq_is_full() && available_command_ids.size() > 0)
      {
        HostIORequest* new_req = waiting_requests_for_submission.front();
        waiting_requests_for_submission.pop_front();
        if (sata_ncq.queue[*available_command_ids.begin()] != nullptr)
          PRINT_ERROR("Unexpteced situation in SATA_HBA! Overwriting a waiting I/O request in the queue!")
        else
        {
          new_req->IO_queue_info = *available_command_ids.begin();
          sata_ncq.queue[*available_command_ids.begin()] = new_req;
          available_command_ids.erase(available_command_ids.begin());
          request_queue_in_memory[sata_ncq.sq_tail] = new_req;
          sata_ncq.move_sq_tail();
        }
        new_req->Enqueue_time = sim->Time();
        pcie_root_complex.Write_to_device(sata_ncq.sq_tail_register, sata_ncq.sq_tail);//Based on NVMe protocol definition, the updated tail pointer should be informed to the device
      }
      else break;

      cqe->release();

      if(consume_requests.size() > 0)
        sim->Register_sim_event(sim->Time() + hba_processing_delay, this, nullptr, static_cast<int>(HBA_Sim_Events::CONSUME_IO_REQUEST));
    break;
  }
  case HBA_Sim_Events::SUBMIT_IO_REQUEST:
  {
    HostIORequest* request = host_requests.front();
    host_requests.pop();
    if (sata_ncq.sq_is_full() || available_command_ids.size() == 0)//If the hardware queue is full
      waiting_requests_for_submission.push_back(request);
    else
    {
      if (sata_ncq.queue[*available_command_ids.begin()] != nullptr)
        PRINT_ERROR("Unexpteced situation in IO_Flow_Base! Overwriting an unhandled I/O request in the queue!")
      else
      {
        request->IO_queue_info = *available_command_ids.begin();
        sata_ncq.queue[*available_command_ids.begin()] = request;
        available_command_ids.erase(available_command_ids.begin());
        request_queue_in_memory[sata_ncq.sq_tail] = request;
        sata_ncq.move_sq_tail();
      }
      request->Enqueue_time = sim->Time();
      pcie_root_complex.Write_to_device(sata_ncq.sq_tail_register, sata_ncq.sq_tail);//Based on NVMe protocol definition, the updated tail pointer should be informed to the device
    }

    if (host_requests.size() > 0)
      sim->Register_sim_event(sim->Time() + hba_processing_delay, this, nullptr, static_cast<int>(HBA_Sim_Events::SUBMIT_IO_REQUEST));

    break;
  }
  }
}

void SATA_HBA::Submit_io_request(HostIORequest* request)
{
  host_requests.push(request);
  if (host_requests.size() == 1) {
    auto* sim = Simulator;
    sim->Register_sim_event(sim->Time() + hba_processing_delay, this, nullptr, static_cast<int>(HBA_Sim_Events::SUBMIT_IO_REQUEST));
  }
}
void SATA_HBA::SATA_consume_io_request(CQEntry* cqe)
{
  consume_requests.push(cqe);
  if (consume_requests.size() == 1) {
    auto* sim = Simulator;
    sim->Register_sim_event(sim->Time() + hba_processing_delay, this, nullptr, static_cast<int>(HBA_Sim_Events::CONSUME_IO_REQUEST));
  }
}
SQEntry* SATA_HBA::Read_ncq_entry(uint64_t address)
{
  HostIORequest* request = request_queue_in_memory[sata_ncq.index_of(address)];
  if (request == nullptr)
    throw std::invalid_argument("SATA HBA: Request to access an NCQ entry that does not exist.");

  if (request->Type == HostIOReqType::READ)//For simplicity, MQSim's SATA host interface uses NVMe opcodes
  {
    return __sq_entry_pool.construct(NVME_READ_OPCODE,
                                     request->IO_queue_info,
                                     (DATA_MEMORY_REGION),
                                     (DATA_MEMORY_REGION + 0x1000),
                                     (uint32_t)request->Start_LBA,
                                     (uint32_t)(request->Start_LBA >> 32U),
                                     ((uint32_t)((uint16_t)request->LBA_count)) & (uint32_t)(0x0000ffff));
  }
  else
  {
    return __sq_entry_pool.construct(NVME_WRITE_OPCODE,
                                     request->IO_queue_info,
                                     (DATA_MEMORY_REGION),
                                     (DATA_MEMORY_REGION + 0x1000),
                                     (uint32_t)request->Start_LBA,
                                     (uint32_t)(request->Start_LBA >> 32U),
                                     ((uint32_t)((uint16_t)request->LBA_count)) & (uint32_t)(0x0000ffff));
  }
}
void SATA_HBA::Update_and_submit_ncq_completion_info()
{
  sata_ncq.cq_head++;
  if (sata_ncq.cq_head == sata_ncq.cq_size)
    sata_ncq.cq_head = 0;
  pcie_root_complex.Write_to_device(sata_ncq.cq_head_register, sata_ncq.cq_head);//Based on NVMe protocol definition, the updated head pointer should be informed to the device
}

uint64_t
SATA_HBA::sq_base_address() const
{
  return sata_ncq.sq_memory_base_address;
}

uint64_t
SATA_HBA::cq_base_address() const
{
  return sata_ncq.cq_memory_base_address;
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
