#ifndef IO_FLOW_BASE_H
#define IO_FLOW_BASE_H

#include <iostream>
#include <list>
#include <set>
#include <string>
#include <vector>
#include <unordered_map>

#include "../exec/params/HostParameterSet.h"
#include "../sim/Sim_Defs.h"
#include "../sim/Sim_Object.h"
#include "../sim/Sim_Reporter.h"
#include "../ssd/interface/Host_Interface_Defs.h"
#include "../ssd/SSD_Defs.h"
#include "../utils/Logical_Address_Partitioning_Unit.h"
#include "../utils/CountingStats.h"
#include "../utils/Workload_Statistics.h"
#include "HostIORequest.h"
#include "IoQueueInfo.h"
//#include "../sim/Engine.h"


namespace Host_Components
{
  class SATA_HBA;
  class PCIe_Root_Complex;


  class IO_Flow_Base : public MQSimEngine::Sim_Object
  {
  private:
    const uint16_t               __flow_id;
    const IO_Flow_Priority_Class __priority_class;

  protected:
    const LHA_type __start_lsa_on_dev;
    const LHA_type __end_lsa_on_dev;

  private:
    PCIe_Root_Complex* __pcie_root_complex;
    SATA_HBA*          __sata_hba;

    HostIOReqPool __host_io_req_pool;
    SQEntryPool   __sq_entry_pool;

  protected:

    // The initial amount of valid logical pages after preconditioning is
    // performed
    const double _initial_occupancy_ratio;

    sim_time_type stop_time;//The flow stops generating request when simulation time reaches stop_time
    uint32_t total_requests_to_be_generated;//If stop_time is zero, then the flow stops generating request when the number of generated requests is equal to total_req_count
    HostInterface_Types SSD_device_type;
    //NVMe host-to-device communication variables
    NVMe_Queue_Pair nvme_queue_pair;
    uint16_t io_queue_id;
    std::set<uint16_t> available_command_ids;
    std::vector<HostIORequest*> request_queue_in_memory;
    std::list<HostIORequest*> waiting_requests;//The I/O requests that are still waiting to be enqueued in the I/O queue (the I/O queue is full)
    std::unordered_map<sim_time_type, HostIORequest*> nvme_software_request_queue;//The I/O requests that are enqueued in the I/O queue of the SSD device

  private:
    //Variables used to collect statistics
    uint32_t _generated_req;

    uint32_t _serviced_req;

    Utils::IopsStats _stat_generated_reads;
    Utils::IopsStats _stat_generated_writes;

#if UNBLOCK_NOT_IN_USE
    Utils::IopsStats _stat_generated_ignored;
#endif

    Utils::IopsStats _stat_serviced_reads;
    Utils::IopsStats _stat_serviced_writes;

    Utils::MinMaxAvgStats<sim_time_type> _stat_dev_rd_response_time;
    Utils::MinMaxAvgStats<sim_time_type> _stat_dev_wr_response_time;
    Utils::MinMaxAvgStats<sim_time_type> _stat_rd_req_delay;
    Utils::MinMaxAvgStats<sim_time_type> _stat_wr_req_delay;
    Utils::AvgStats<sim_time_type> _stat_short_term_dev_resp;
    Utils::AvgStats<sim_time_type> _stat_short_term_req_delay;

    Utils::BandwidthStats<sim_time_type> _stat_transferred_reads;
    Utils::BandwidthStats<sim_time_type> _stat_transferred_writes;

  protected:
    int progress;
    int next_progress_step = 0;

    //Variables used to log response time changes
    bool enabled_logging;
    sim_time_type logging_period;
    sim_time_type next_logging_milestone;
    std::string logging_file_path;
    std::ofstream log_file;

  private:
    bool __sq_is_full(const NVMe_Queue_Pair& Q) const;
    void __update_sq_tail(NVMe_Queue_Pair& Q);

    void __update_stats_by_request(sim_time_type now,
                                   const HostIORequest* request);

    void __announce_progress();

  protected:
    HostIORequest* _generate_request(sim_time_type time,
                                     LHA_type lba,
                                     uint32_t count,
                                     HostIOReqType type);

    bool _all_request_generated() const;

    void Submit_io_request(HostIORequest*);
    void NVMe_update_and_submit_completion_queue_tail();

  public:
    IO_Flow_Base(const sim_object_id_type& name,
                 uint16_t flow_id,
                 LHA_type start_lsa_on_device,
                 LHA_type end_lsa_address_on_device,
                 uint16_t io_queue_id,
                 uint16_t nvme_submission_queue_size,
                 uint16_t nvme_completion_queue_size,
                 IO_Flow_Priority_Class priority_class,
                 sim_time_type stop_time,
                 double initial_occupancy_ratio,
                 uint32_t total_requets_to_be_generated,
                 HostInterface_Types SSD_device_type,
                 PCIe_Root_Complex* pcie_root_complex,
                 SATA_HBA* sata_hba,
                 bool enabled_logging,
                 sim_time_type logging_period,
                 const std::string& logging_file_path);

    ~IO_Flow_Base() override = default;

    void Start_simulation() override;
    void Report_results_in_XML(std::string name_prefix,
                               Utils::XmlWriter& xmlwriter) override;

    IO_Flow_Priority_Class priority_class() const;

    SQEntry* NVMe_read_sqe(uint64_t address);
    const IoQueueInfo& queue_info();

    virtual HostIORequest* Generate_next_request() = 0;
    virtual void NVMe_consume_io_request(CQEntry*);
    virtual void SATA_consume_io_request(HostIORequest* request);

    virtual void get_stats(Utils::Workload_Statistics& stats,
                           const Utils::LhaToLpaConverterBase& convert_lha_to_lpa,
                           const Utils::NvmAccessBitmapFinderBase& find_nvm_subunit_access_bitmap) = 0;

    LHA_type Get_start_lsa_on_device();
    LHA_type Get_end_lsa_address_on_device();
    uint32_t Get_generated_request_count();
    uint32_t Get_serviced_request_count();

    uint32_t Get_device_response_time();//in microseconds

    uint32_t Get_end_to_end_request_delay();//in microseconds
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
    ++_generated_req;

    if (type == HostIOReqType::WRITE)
      ++_stat_generated_writes;
    else
      ++_stat_generated_reads;

    return __host_io_req_pool.construct(time, lba, count, type);
  }

  force_inline bool
  IO_Flow_Base::_all_request_generated() const
  {
    return total_requests_to_be_generated <= _generated_req;
  }

  force_inline LHA_type
  IO_Flow_Base::Get_start_lsa_on_device()
  {
    return __start_lsa_on_dev;
  }

  force_inline LHA_type
  IO_Flow_Base::Get_end_lsa_address_on_device()
  {
    return __end_lsa_on_dev;
  }

  force_inline uint32_t
  IO_Flow_Base::Get_generated_request_count()
  {
    return _generated_req;
  }

  force_inline uint32_t
  IO_Flow_Base::Get_serviced_request_count()
  {
    return _serviced_req;
  }

  force_inline uint32_t
  IO_Flow_Base::Get_device_response_time()
  {
    auto response_time = _stat_dev_rd_response_time
                           + _stat_dev_wr_response_time;

    return response_time.avg(SIM_TIME_TO_MICROSECONDS_COEFF);
  }

  force_inline uint32_t
  IO_Flow_Base::Get_end_to_end_request_delay()
  {
    auto req_delay = _stat_rd_req_delay + _stat_wr_req_delay;

    return req_delay.avg(SIM_TIME_TO_MICROSECONDS_COEFF);
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
