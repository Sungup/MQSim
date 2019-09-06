#ifndef HOST_SYSTEM_H
#define HOST_SYSTEM_H

#include "../host/ioflow/HostIORequest.h"
#include "../host/ioflow/IO_Flow_Base.h"
#include "../host/pcie/PCIeLink.h"
#include "../host/pcie/PCIeRootComplex.h"
#include "../host/pcie/PCIeSwitch.h"
#include "../host/sata/SATA_HBA.h"
#include "../sim/Sim_Object.h"
#include "../ssd/interface/Host_Interface_Base.h"
#include "../utils/Workload_Statistics.h"
#include "params/HostParameterSet.h"
#include "SsdDevice.h"

class HostSystem : public MQSimEngine::Sim_Object
{
private:
  Host_Components::IoFlowList __io_flows;
  SsdDevice&                  __ssd_device;

  Host_Components::PCIeLink         __link;
  Host_Components::PCIeRootComplex __root_complex;
  Host_Components::PCIeSwitch        __pcie_switch;
  Host_Components::SataHbaPtr        __sata_hba;

private:
  void __connect_ssd();

  void __init_nvme_queue();
  void __init_sata_queue();

  Utils::WorkloadStatsList __make_workloads_statistics();

public:
  HostSystem(const HostParameterSet& params,
             const IOFlowScenario& scenario,
             const Utils::LogicalAddrPartition& lapu,
             SsdDevice& ssd);
  ~HostSystem() final = default;

  void Start_simulation() final;
  void Validate_simulation_config() final;
  void Report_results_in_XML(std::string name_prefix,
                             Utils::XmlWriter& xmlwriter) final;

  const Host_Components::IoFlowList& io_flows();

};

force_inline const Host_Components::IoFlowList&
HostSystem::io_flows()
{
  return __io_flows;
}

#endif // !HOST_SYSTEM_H
