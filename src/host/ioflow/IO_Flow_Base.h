#ifndef IO_FLOW_BASE_H
#define IO_FLOW_BASE_H

#include <deque>
#include <string>
#include <vector>

#include "../../exec/params/HostParameterSet.h"
#include "../../sim/Sim_Defs.h"
#include "../../sim/Sim_Object.h"
#include "../../ssd/interface/Host_Interface_Defs.h"
#include "../../ssd/SSD_Defs.h"
#include "../../utils/CountingStats.h"
#include "../../utils/Logical_Address_Partitioning_Unit.h"
#include "../../utils/ProgressBar.h"
#include "../../utils/ServiceHandler.h"
#include "../../utils/Workload_Statistics.h"
#include "HostIORequest.h"
#include "IoQueueInfo.h"
#include "IoFlowStats.h"
#include "IoFlowLog.h"
#include "SubmissionQueue.h"

namespace Host_Components
{
  class SATA_HBA;
  class PCIe_Root_Complex;

  class IO_Flow_Base : public MQSimEngine::Sim_Object
  {
    typedef Utils::ServiceHandler<IO_Flow_Base, HostIORequest*> RequestSubmitter;

  private:
    const uint16_t               __flow_id;
    const IO_Flow_Priority_Class __priority_class;

    // If stop_time is zero, then the flow stops generating request when
    // the number of generated requests is equal to total_req_count
    const uint32_t __max_req_count;

    // The initial amount of valid logical pages after preconditioning is
    // performed
    const double __initial_occupancy_ratio;

    PCIe_Root_Complex* __pcie_root_complex;
    SATA_HBA*          __sata_hba;

    NVMe_Queue_Pair __nvme_queue_pair;
    SubmissionQueue __submission_queue;

    // The I/O requests that are still waiting to be enqueued in the I/O queue
    // (the I/O queue is full)
    std::deque<HostIORequest*> __waiting_queue;

    HostIOReqPool __host_io_req_pool;
    SQEntryPool   __sq_entry_pool;

    RequestSubmitter __req_submitter;

    IoFlowStats __stats;
    IoFlowLog   __log;
    Utils::ProgressBar __progress_bar;

  public:
    const LHA_type start_lsa;
    const LHA_type end_lsa;

  private:
    void __update_stats_by_request(sim_time_type now,
                                   const HostIORequest* request);

    void __enqueue_to_sq(HostIORequest* request);

    void __end_of_request(HostIORequest* request);

    void __update_and_submit_cq_tail();

    void __submit_nvme_request(HostIORequest* request);
    void __submit_sata_request(HostIORequest* request);

  protected:
    HostIORequest* _generate_request(sim_time_type time,
                                     LHA_type lba,
                                     uint32_t count,
                                     HostIOReqType type);

    void _update_max_req_count(uint32_t max_req_count);

    bool _all_request_generated() const;

    bool _submit_io_request(HostIORequest* request);

    virtual int _get_progress() const;

  public:
    IO_Flow_Base(const sim_object_id_type& name,
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
                 SATA_HBA* sata_hba);

    ~IO_Flow_Base() override = default;

    void Start_simulation() override;
    void Report_results_in_XML(std::string name_prefix,
                               Utils::XmlWriter& xmlwriter) override;

    IO_Flow_Priority_Class priority_class() const;

    SQEntry* read_nvme_sqe(uint64_t address);

    uint64_t sq_base_address() const;
    uint64_t cq_base_address() const;

    virtual void consume_nvme_io(CQEntry* cqe);
    virtual void consume_sata_io(HostIORequest* request);

    virtual void get_stats(Utils::Workload_Statistics& stats,
                           const Utils::LhaToLpaConverterBase& convert_lha_to_lpa,
                           const Utils::NvmAccessBitmapFinderBase& find_nvm_subunit_access_bitmap);

    uint32_t generated_requests() const;
    uint32_t serviced_requests() const;
    uint32_t average_response_time() const;     //in microseconds
    uint32_t average_end_to_end_delay() const;  //in microseconds
  };

  force_inline IO_Flow_Priority_Class
  IO_Flow_Base::priority_class() const
  {
    return __priority_class;
  }

  force_inline HostIORequest*
  IO_Flow_Base::_generate_request(sim_time_type time,
                                  LHA_type lba,
                                  uint32_t count,
                                  Host_Components::HostIOReqType type)
  {
    __stats.update_generate(type);

    return __host_io_req_pool.construct(time, lba, count, type);
  }

  force_inline void
  IO_Flow_Base::_update_max_req_count(uint32_t max_req_count)
  {
    const_cast<uint32_t&>(__max_req_count) = max_req_count;
  }

  force_inline bool
  IO_Flow_Base::_all_request_generated() const
  {
    return __max_req_count <= __stats.generated_req();
  }

  force_inline bool
  IO_Flow_Base::_submit_io_request(HostIORequest* request)
  {
    if (!request)
      return false;

    __req_submitter(request);

    return true;
  }

  force_inline uint64_t
  IO_Flow_Base::sq_base_address() const
  {
    return __nvme_queue_pair.sq_memory_base_address;
  }

  force_inline uint64_t
  IO_Flow_Base::cq_base_address() const
  {
    return __nvme_queue_pair.cq_memory_base_address;
  }

  force_inline uint32_t
  IO_Flow_Base::generated_requests() const
  {
    return __stats.generated_req();
  }

  force_inline uint32_t
  IO_Flow_Base::serviced_requests() const
  {
    return __stats.serviced_req();
  }

  force_inline uint32_t
  IO_Flow_Base::average_response_time() const
  {
    return __stats.avg_response_time();
  }

  force_inline uint32_t
  IO_Flow_Base::average_end_to_end_delay() const
  {
    return __stats.avg_request_delay();
  }

  // ---------------
  // IO Flow Builder
  // ---------------
  typedef std::shared_ptr<IO_Flow_Base> IoFlowPtr;
  typedef std::vector<IoFlowPtr>        IoFlowList;

  // Passing SATA_HBA by pointer not reference, reference can't pass the
  // nullptr for the NVMe environment.
  IoFlowPtr build_io_flow(const sim_object_id_type& host_id,
                          const HostParameterSet& host_params,
                          const IOFlowParamSet& flow_params,
                          const Utils::LogicalAddrPartition& lapu,
                          uint16_t flow_id,
                          PCIe_Root_Complex& root_complex,
                          SATA_HBA* sata_hba,
                          HostInterface_Types interface_type,
                          uint16_t sq_size,
                          uint16_t cq_size);
}

#endif // !IO_FLOW_BASE_H
