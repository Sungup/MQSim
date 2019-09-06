#ifndef SATA_HBA_H
#define SATA_HBA_H

#include <cstdint>
#include <list>
#include <memory>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "../../ssd/interface/Host_Interface_Defs.h"
#include "../../sim/Sim_Defs.h"
#include "../../sim/Sim_Object.h"

#include "../ioflow/HostIORequest.h"
#include "../ioflow/IO_Flow_Base.h"
#include "../pcie/PCIeRootComplex.h"
#include "../ioflow/IoQueueInfo.h"

namespace SSD_Components {
  class Host_Interface_Base;
}

namespace Host_Components {
  class SATA_HBA : MQSimEngine::Sim_Object {
  private:
    static constexpr int HOST_SIGNAL = 0;
    static constexpr int NVME_SIGNAL = 1;

  private:
    SQEntryPool __sq_entry_pool;

    // This estimates the processing delay of the whole SATA software and
    // hardware layers to send/receive a SATA message
    sim_time_type __hba_processing_delay;

    PCIeRootComplex& __pcie_root_complex;
    IoFlowList& __flows;

    IoQueueInfo __nvme_queue_info;
    SubmissionQueue __submission_queue;

    // The I/O requests that are still waiting (since the I/O queue is full) to
    // be enqueued in the I/O queue
    std::list<HostIORequest*> __waiting_queue;

    std::queue<CQEntry*>       __cq;  // Completion Queue between SATA and NVMe
    std::queue<HostIORequest*> __ncq; // Native Command Queue between Host to SATA

  private:
    void __issue_signal(int type);

    void __end_of_request(HostIORequest* request);

    /*
     * TODO some functions are exactly same with IO_Flow_Base's same name
     *      functions. So, these will be merged into new parent class for NVMe's
     *      interface name with NVMeHostDriver.
     *
     *  Target:
     *    private:
     *      __enqueue_to_sq()
     *      __update_and_submit_cq_tail()
     *
     *    private -> protected:
     *      _submit_io_request()
     *      _consume_io_request()
     *
     *    public:
     *      read_sq_entry()
     */
    void __enqueue_to_sq(HostIORequest* request);

    void __update_and_submit_cq_tail();

    void __submit_io_request(HostIORequest* request);
    void __consume_io_request(CQEntry* cqe);

    template <typename Queue, typename Handler>
    void __handle_signal(Queue queue, Handler handler, int signal);

    template <typename Queue, typename Packet>
    void __issue_signal(Queue queue, Packet* packet, int signal);

  public:
    SATA_HBA(sim_object_id_type id,
             uint16_t ncq_size,
             sim_time_type hba_processing_delay,
             PCIeRootComplex& pcie_root_complex,
             IoFlowList& IO_flows);

    ~SATA_HBA() final = default;

    void Execute_simulator_event(MQSimEngine::SimEvent*) final;

    /*
     * Submit IO request call path.
     *
     *   IOFlowBase::__submit_sata_request(HostIORequest* request)
     *     -> SataHba::submit_sata_io(HostIORequest* request)
     *     ->    ... Host processing delay time ...
     *     -> PCIeRootComplex::write_to_device() signal
     */
    void submit_sata_io(HostIORequest* request);

    /*
     * Consume IO Request call path.
     *
     *   PCIeRootComplex::__consume_sata_io(void* payload)
     *     -> SataHba::consume_sata_io(CQEntry* cqe)
     *     ->    ... Host processing delay time ...
     *     -> IoFlowBase::consume_stat_io(HostIORequest* request)
     */
    void consume_sata_io(CQEntry* cqe);

    /*
     * Read SQ item by NVMe device. This SATA HBA use NVMe protocol to
     * communicate ctrl and internal SSD device.
     *
     * +------+             +----------+             +----------+
     * | Host | <---------> | SATA HBA | <---------> | NVMe SSD |
     * +------+   SATA IO   +----------+   NVMe IO   +----------+
     *
     */
    SQEntry* read_sq_entry(uint64_t address);

    uint64_t sq_base_address() const;
    uint64_t cq_base_address() const;
  };

  force_inline uint64_t
  SATA_HBA::sq_base_address() const
  {
    return __nvme_queue_info.sq_memory_base_address;
  }

  force_inline uint64_t
  SATA_HBA::cq_base_address() const
  {
    return __nvme_queue_info.cq_memory_base_address;
  }

  // ----------------
  // SATA_HBA builder
  // ----------------
  typedef std::shared_ptr<SATA_HBA> SataHbaPtr;

  SataHbaPtr build_sata_hba(const sim_object_id_type& id,
                            HostInterface_Types interface_type,
                            uint16_t ncq_depth,
                            sim_time_type sata_ctrl_delay,
                            PCIeRootComplex& root_complex,
                            IoFlowList& io_flows);
}

#endif // !SATA_HBA_H
