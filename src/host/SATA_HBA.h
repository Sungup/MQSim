#ifndef SATA_HBA_H
#define SATA_HBA_H

#include <cstdint>
#include <list>
#include <memory>
#include <queue>
#include <set>
#include <unordered_map>

#include "../ssd/interface/Host_Interface_Defs.h"
#include "../sim/Sim_Defs.h"
#include "../sim/Sim_Object.h"

#include "HostIORequest.h"
#include "IO_Flow_Base.h"
#include "PCIe_Root_Complex.h"
#include "IoQueueInfo.h"

namespace SSD_Components {
  class Host_Interface_Base;
}

namespace Host_Components
{
#define SATA_SQ_FULL(Q) (Q.Submission_queue_tail < Q.Submission_queue_size - 1 ? Q.Submission_queue_tail + 1 == Q.Submission_queue_head : Q.Submission_queue_head == 0)
#define SATA_UPDATE_SQ_TAIL(Q)  Q.Submission_queue_tail++;\
            if (Q.Submission_queue_tail == Q.Submission_queue_size)\
              Q.Submission_queue_tail = 0;

  enum class HBA_Sim_Events {SUBMIT_IO_REQUEST, CONSUME_IO_REQUEST};

  class SATA_HBA : MQSimEngine::Sim_Object
  {
  public:
    SATA_HBA(sim_object_id_type id,
             uint16_t ncq_size,
             sim_time_type hba_processing_delay,
             PCIe_Root_Complex& pcie_root_complex,
             IoFlowList& IO_flows);

    ~SATA_HBA() final = default;
    void Start_simulation();
    void Validate_simulation_config();
    void Execute_simulator_event(MQSimEngine::SimEvent*);
    void Submit_io_request(HostIORequest* request);
    void SATA_consume_io_request(CompletionQueueEntry* cqe);
    SubmissionQueueEntry* Read_ncq_entry(uint64_t address);
    const IoQueueInfo& queue_info();
  private:
    uint16_t ncq_size;
    sim_time_type hba_processing_delay;//This estimates the processing delay of the whole SATA software and hardware layers to send/receive a SATA message
    PCIe_Root_Complex& pcie_root_complex;
    IoFlowList& IO_flows;
    NCQ_Control_Structure sata_ncq;
    std::set<uint16_t> available_command_ids;
    std::vector<HostIORequest*> request_queue_in_memory;
    std::list<HostIORequest*> waiting_requests_for_submission;//The I/O requests that are still waiting (since the I/O queue is full) to be enqueued in the I/O queue 
    void Update_and_submit_ncq_completion_info();
    std::queue<CompletionQueueEntry*> consume_requests;
    std::queue<HostIORequest*> host_requests;
  };

  // ----------------
  // SATA_HBA builder
  // ----------------
  typedef std::shared_ptr<SATA_HBA> SataHbaPtr;

  SataHbaPtr build_sata_hba(const sim_object_id_type& id,
                            HostInterface_Types interface_type,
                            uint16_t ncq_depth,
                            sim_time_type sata_ctrl_delay,
                            PCIe_Root_Complex& root_complex,
                            IoFlowList& io_flows);
}

#endif // !SATA_HBA_H
