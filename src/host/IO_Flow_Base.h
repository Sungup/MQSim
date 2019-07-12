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
#include "../utils/Workload_Statistics.h"
#include "HostIORequest.h"
#include "IoQueueInfo.h"

namespace Host_Components
{
  class SATA_HBA;
  class PCIe_Root_Complex;

#define NVME_SQ_FULL(Q) (Q.Submission_queue_tail < Q.Submission_queue_size - 1 ? Q.Submission_queue_tail + 1 == Q.Submission_queue_head : Q.Submission_queue_head == 0)
#define NVME_UPDATE_SQ_TAIL(Q)  Q.Submission_queue_tail++;\
            if (Q.Submission_queue_tail == Q.Submission_queue_size)\
              nvme_queue_pair.Submission_queue_tail = 0;

  class IO_Flow_Base : public MQSimEngine::Sim_Object
  {
  public:
    IO_Flow_Base(const sim_object_id_type& name, uint16_t flow_id, LHA_type start_lsa_on_device, LHA_type end_lsa_address_on_device, uint16_t io_queue_id,
      uint16_t nvme_submission_queue_size, uint16_t nvme_completion_queue_size, IO_Flow_Priority_Class priority_class,
      sim_time_type stop_time, double initial_occupancy_ratio, uint32_t total_requets_to_be_generated,
      HostInterface_Types SSD_device_type, PCIe_Root_Complex* pcie_root_complex, SATA_HBA* sata_hba,
      bool enabled_logging, sim_time_type logging_period, std::string logging_file_path);
    virtual ~IO_Flow_Base();
    void Start_simulation();
    IO_Flow_Priority_Class Priority_class() { return priority_class; }
    virtual HostIORequest* Generate_next_request() = 0;
    virtual void NVMe_consume_io_request(CompletionQueueEntry*);
    SubmissionQueueEntry* NVMe_read_sqe(uint64_t address);
    const IoQueueInfo& queue_info();
    virtual void SATA_consume_io_request(HostIORequest* request);
    void Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter);

    virtual void get_stats(Utils::Workload_Statistics& stats,
                           const Utils::LhaToLpaConverterBase& convert_lha_to_lpa,
                           const Utils::NvmAccessBitmapFinderBase& find_nvm_subunit_access_bitmap) = 0;
  protected:
    HostIOReqPool _host_io_req_pool;

    uint16_t flow_id;
    double initial_occupancy_ratio;//The initial amount of valid logical pages when pereconditioning is performed
    sim_time_type stop_time;//The flow stops generating request when simulation time reaches stop_time
    uint32_t total_requests_to_be_generated;//If stop_time is zero, then the flow stops generating request when the number of generated requests is equal to total_req_count
    HostInterface_Types SSD_device_type;
    PCIe_Root_Complex* pcie_root_complex;
    SATA_HBA* sata_hba;
    LHA_type start_lsa_on_device, end_lsa_on_device;

    void Submit_io_request(HostIORequest*);

    //NVMe host-to-device communication variables
    IO_Flow_Priority_Class priority_class;
    NVMe_Queue_Pair nvme_queue_pair;
    uint16_t io_queue_id;
    uint16_t nvme_submission_queue_size;
    uint16_t nvme_completion_queue_size;
    std::set<uint16_t> available_command_ids;
    std::vector<HostIORequest*> request_queue_in_memory;
    std::list<HostIORequest*> waiting_requests;//The I/O requests that are still waiting to be enqueued in the I/O queue (the I/O queue is full)
    std::unordered_map<sim_time_type, HostIORequest*> nvme_software_request_queue;//The I/O requests that are enqueued in the I/O queue of the SSD device
    void NVMe_update_and_submit_completion_queue_tail();

    //Variables used to collect statistics
    uint32_t STAT_generated_request_count, STAT_generated_read_request_count, STAT_generated_write_request_count;
    uint32_t STAT_ignored_request_count;
    uint32_t STAT_serviced_request_count, STAT_serviced_read_request_count, STAT_serviced_write_request_count;
    sim_time_type STAT_sum_device_response_time, STAT_sum_device_response_time_read, STAT_sum_device_response_time_write;
    sim_time_type STAT_min_device_response_time, STAT_min_device_response_time_read, STAT_min_device_response_time_write;
    sim_time_type STAT_max_device_response_time, STAT_max_device_response_time_read, STAT_max_device_response_time_write;
    sim_time_type STAT_sum_request_delay, STAT_sum_request_delay_read, STAT_sum_request_delay_write;
    sim_time_type STAT_min_request_delay, STAT_min_request_delay_read, STAT_min_request_delay_write;
    sim_time_type STAT_max_request_delay, STAT_max_request_delay_read, STAT_max_request_delay_write;
    sim_time_type STAT_transferred_bytes_total, STAT_transferred_bytes_read, STAT_transferred_bytes_write;
    int progress;
    int next_progress_step = 0;

    //Variables used to log response time changes
    bool enabled_logging;
    sim_time_type logging_period;
    sim_time_type next_logging_milestone;
    std::string logging_file_path;
    std::ofstream log_file;
    uint32_t Get_device_response_time_short_term();//in microseconds
    uint32_t Get_end_to_end_request_delay_short_term();//in microseconds
    sim_time_type STAT_sum_device_response_time_short_term, STAT_sum_request_delay_short_term;
    uint32_t STAT_serviced_request_count_short_term;

  public:
    LHA_type Get_start_lsa_on_device();
    LHA_type Get_end_lsa_address_on_device();
    uint32_t Get_generated_request_count();
    uint32_t Get_serviced_request_count();//in microseconds
    uint32_t Get_device_response_time();//in microseconds
    uint32_t Get_min_device_response_time();//in microseconds
    uint32_t Get_max_device_response_time();//in microseconds
    uint32_t Get_end_to_end_request_delay();//in microseconds
    uint32_t Get_min_end_to_end_request_delay();//in microseconds
    uint32_t Get_max_end_to_end_request_delay();//in microseconds
  };

  force_inline LHA_type
  IO_Flow_Base::Get_start_lsa_on_device()
  {
    return start_lsa_on_device;
  }

  force_inline LHA_type
  IO_Flow_Base::Get_end_lsa_address_on_device()
  {
    return end_lsa_on_device;
  }

  force_inline uint32_t
  IO_Flow_Base::Get_generated_request_count()
  {
    return STAT_generated_request_count;
  }

  force_inline uint32_t
  IO_Flow_Base::Get_serviced_request_count()
  {
    return STAT_serviced_request_count;
  }

  force_inline uint32_t
  IO_Flow_Base::Get_device_response_time()
  {
    return (STAT_serviced_request_count == 0)
             ? 0
             : STAT_sum_device_response_time
                 / STAT_serviced_request_count
                 / SIM_TIME_TO_MICROSECONDS_COEFF;
  }

  force_inline uint32_t
  IO_Flow_Base::Get_min_device_response_time()
  {
    return STAT_min_device_response_time / SIM_TIME_TO_MICROSECONDS_COEFF;
  }

  force_inline uint32_t
  IO_Flow_Base::Get_max_device_response_time()
  {
    return STAT_max_device_response_time / SIM_TIME_TO_MICROSECONDS_COEFF;
  }

  force_inline uint32_t
  IO_Flow_Base::Get_end_to_end_request_delay()
  {
    return (STAT_serviced_request_count == 0)
             ? 0
             : STAT_sum_request_delay
                 / STAT_serviced_request_count
                 / SIM_TIME_TO_MICROSECONDS_COEFF;
  }

  force_inline uint32_t
  IO_Flow_Base::Get_min_end_to_end_request_delay()
  {
    return STAT_min_request_delay / SIM_TIME_TO_MICROSECONDS_COEFF;
  }

  force_inline uint32_t
  IO_Flow_Base::Get_max_end_to_end_request_delay()
  {
    return STAT_max_request_delay / SIM_TIME_TO_MICROSECONDS_COEFF;
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
