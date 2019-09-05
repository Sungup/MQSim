#ifndef IO_FLOW_SYNTHETIC_H
#define IO_FLOW_SYNTHETIC_H

#include <string>

#include "../../utils/RandomGenerator.h"
#include "../../utils/DistributionTypes.h"

#include "IO_Flow_Base.h"
#include "SynFlowRandGen.h"

namespace Host_Components
{
  /// ---------------------------------------
  /// 0. Abstract class for IO flow synthetic
  /// ---------------------------------------
  class IO_Flow_Synthetic : public IO_Flow_Base
  {
  private:
    // The flow stops generating request when simulation time reaches
    // __run_until
    const sim_time_type __run_until;

    const double __read_ratio;
    const double __working_set_ratio;

    // Request type random generator
    Utils::RandomGenerator __req_type_generator;

    AddressDistributorPtr __address_distributor;
    ReqSizeDistributorPtr __req_size_distributor;

  private:
    HostIORequest* __generate_next_req();

  protected:
    int _get_progress() const final;

    bool _submit_next_request();

  public:
    IO_Flow_Synthetic(const sim_object_id_type& name,
                      const HostParameterSet& host_params,
                      const SyntheticFlowParamSet& flow_params,
                      const Utils::LogicalAddrPartition& lapu,
                      uint16_t flow_id,
                      uint16_t sq_size,
                      uint16_t cq_size,
                      HostInterface_Types interface_type,
                      PCIe_Root_Complex& root_complex,
                      SATA_HBA* sata_hba);

    ~IO_Flow_Synthetic() override = default;

    void Start_simulation() override;

    void get_stats(Utils::Workload_Statistics& stats,
                   const Utils::LhaToLpaConverterBase& convert_lha_to_lpa,
                   const Utils::NvmAccessBitmapFinderBase& find_nvm_subunit_access_bitmap) override;
  };

  /// -------------------------------
  /// 1. QD managed synthetic io flow
  /// -------------------------------
  class IoFlowSyntheticQD : public IO_Flow_Synthetic {
  private:
    const uint32_t __target_queue_depth;

  public:
    IoFlowSyntheticQD(const sim_object_id_type& name,
                      const HostParameterSet& host_params,
                      const SyntheticFlowParamSet& flow_params,
                      const Utils::LogicalAddrPartition& lapu,
                      uint16_t flow_id,
                      uint16_t nvme_submission_queue_size,
                      uint16_t nvme_completion_queue_size,
                      HostInterface_Types SSD_device_type,
                      PCIe_Root_Complex& pcie_root_complex,
                      SATA_HBA* sata_hba);

    ~IoFlowSyntheticQD() override = default;

    void Start_simulation() override;
    void Execute_simulator_event(MQSimEngine::SimEvent* event) override;

    void consume_nvme_io(CQEntry* cqe) override;
    void consume_sata_io(HostIORequest* request) override;

    void get_stats(Utils::Workload_Statistics& stats,
                   const Utils::LhaToLpaConverterBase& convert_lha_to_lpa,
                   const Utils::NvmAccessBitmapFinderBase& find_nvm_subunit_access_bitmap) override;
  };

  /// --------------------------------------
  /// 2. Bandwidth managed synthetic io flow
  /// --------------------------------------
  class IoFlowSyntheticBW : public IO_Flow_Synthetic {
  private:
    const sim_time_type    __average_req_interval;
    Utils::RandomGenerator __req_interval_generator;

  private:
    sim_time_type __next_trigger_time();

  public:
    IoFlowSyntheticBW(const sim_object_id_type& name,
                      const HostParameterSet& host_params,
                      const SyntheticFlowParamSet& flow_params,
                      const Utils::LogicalAddrPartition& lapu,
                      uint16_t flow_id,
                      uint16_t nvme_submission_queue_size,
                      uint16_t nvme_completion_queue_size,
                      HostInterface_Types SSD_device_type,
                      PCIe_Root_Complex& pcie_root_complex,
                      SATA_HBA* sata_hba);

    ~IoFlowSyntheticBW() override = default;

    void Start_simulation() override;
    void Execute_simulator_event(MQSimEngine::SimEvent* event) override;

    void get_stats(Utils::Workload_Statistics& stats,
                   const Utils::LhaToLpaConverterBase& convert_lha_to_lpa,
                   const Utils::NvmAccessBitmapFinderBase& find_nvm_subunit_access_bitmap) override;
  };

  /// ----------------------------
  /// 3. Synthetic IO flow builder
  /// ----------------------------
  IoFlowPtr build_synthetic_flow(const sim_object_id_type& name,
                                 const HostParameterSet& host_params,
                                 const SyntheticFlowParamSet& flow_params,
                                 const Utils::LogicalAddrPartition& lapu,
                                 uint16_t flow_id,
                                 uint16_t sq_size,
                                 uint16_t cq_size,
                                 HostInterface_Types interface_type,
                                 PCIe_Root_Complex& root_complex,
                                 SATA_HBA* sata_hba);
}

#endif // !IO_FLOW_SYNTHETIC_H
