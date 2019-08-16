#ifndef IO_FLOW_SYNTHETIC_H
#define IO_FLOW_SYNTHETIC_H

#include <string>
#include "IO_Flow_Base.h"
#include "../utils/RandomGenerator.h"
#include "../utils/DistributionTypes.h"

namespace Host_Components
{
  // TODO 1. Make address generator classes
  //         1) Hot/cold generator
  //         2) Stream generator
  //         3) Uniform generator

  // TODO 2. Make request size generator classes
  //         1) Fixed size generator
  //         2) Normalized size generator

  class IO_Flow_Synthetic : public IO_Flow_Base
  {
  private:
    // The flow stops generating request when simulation time reaches
    // __run_until
    const sim_time_type __run_until;

    double __read_ratio;
    double __working_set_ratio;

    Utils::RandomGenerator __req_type_generator;
    Utils::RandomGenerator __address_generator;
    Utils::RandomGenerator __hot_addr_generator; // It can be shared with __address_generator
    Utils::RandomGenerator __hot_cold_generator;
    Utils::RandomGenerator __req_size_generator;
    Utils::RandomGenerator __req_interval_generator;

    // ------------------------------------
    // Address distribution related members
    // ------------------------------------
    const Utils::Address_Distribution_Type address_distribution;

    // Common members
    const bool __address_aligned;
    const uint32_t __align_unit;

    // for Hot/cold distribution
    const double hot_region_ratio;
    const LHA_type hot_region_end_lsa;
    const LHA_type cold_region_start_lsa;

    // For stream distribution
    LHA_type streaming_next_address;

    // -----------------------------------------
    // Request size distribution related members
    // -----------------------------------------
    const Utils::Request_Size_Distribution_Type request_size_distribution;
    const uint32_t average_request_size;

    // Only for Normal distribution
    const uint32_t variance_request_size;

    // ------------------------------------
    // Request flow control related members
    // ------------------------------------
    const Utils::RequestFlowControlType __flow_control_type;
    const sim_time_type                 __average_req_interval;
    const uint32_t                      __target_queue_depth;

  private:
    // -------------------------------------------
    // Request size distribution related functions
    // -------------------------------------------
    LHA_type __align_address(LHA_type lba) const;
    LHA_type __align_address(LHA_type lba, int lba_count,
                             LHA_type start, LHA_type end) const;

    LHA_type __gen_streaming_start_lba(uint32_t lba_count);
    LHA_type __gen_hot_cold_start_lba(uint32_t lba_count);
    LHA_type __gen_uniform_start_lba(uint32_t lba_count);
    LHA_type __gen_start_lba(uint32_t lba_count);

    // -------------------------------------------
    // Request size distribution related functions
    // -------------------------------------------
    uint32_t __gen_req_size();

    // --------------------------------------
    // Request flow control related functions
    // --------------------------------------
    sim_time_type __next_trigger_time();
    void __register_next_sim_event(sim_time_type time);

    HostIOReqType __gen_req_type();

    HostIORequest* __generate_next_req();

    void __submit_next_request();

  protected:
    int _get_progress() const final;

  public:
    IO_Flow_Synthetic(const sim_object_id_type& name,
                      const HostParameterSet& host_params,
                      const SyntheticFlowParamSet& flow_params,
                      const Utils::LogicalAddrPartition& lapu,
                      uint16_t flow_id,
                      uint16_t nvme_submission_queue_size,
                      uint16_t nvme_completion_queue_size,
                      HostInterface_Types SSD_device_type,
                      PCIe_Root_Complex* pcie_root_complex,
                      SATA_HBA* sata_hba);

    ~IO_Flow_Synthetic() final = default;

    void Start_simulation() final;
    void Execute_simulator_event(MQSimEngine::SimEvent* event) final;

    void consume_nvme_io(CQEntry* cqe) final;
    void consume_sata_io(HostIORequest* request) final;

    void get_stats(Utils::Workload_Statistics& stats,
                   const Utils::LhaToLpaConverterBase& convert_lha_to_lpa,
                   const Utils::NvmAccessBitmapFinderBase& find_nvm_subunit_access_bitmap) final;

  };
}

#endif // !IO_FLOW_SYNTHETIC_H
