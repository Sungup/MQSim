#ifndef HOST_SYSTEM_H
#define HOST_SYSTEM_H

#include <vector>
#include "../sim/Sim_Object.h"
#include "../sim/Sim_Reporter.h"
#include "../host/PCIe_Root_Complex.h"
#include "../host/PCIe_Link.h"
#include "../host/PCIe_Switch.h"
#include "../host/PCIe_Message.h"
#include "../host/IO_Flow_Base.h"
#include "../host/HostIORequest.h"
#include "../ssd/interface/Host_Interface_Base.h"
#include "params/HostParameterSet.h"
#include "SSD_Device.h"
#include "../utils/Workload_Statistics.h"

class Host_System : public MQSimEngine::Sim_Object
{
private:
  Host_Components::SataHbaPtr        __sata_hba;
  Host_Components::PCIe_Link         __link;
  Host_Components::PCIe_Root_Complex __root_complex;
  Host_Components::PCIe_Switch       __pcie_switch;

  std::vector<Host_Components::IO_Flow_Base*> IO_flows;
  SSD_Device* __ssd_device;

  const bool __preconditioning_required;

  Utils::WorkloadStatsList __make_workloads_statistics();

public:
  Host_System(const HostParameterSet& params,
              const IOFlowScenario& scenario,
              bool preconditioning_required,
              SSD_Components::Host_Interface_Base& host_interface,
              const Utils::LogicalAddrPartition& lapu);
  ~Host_System();
  void Start_simulation();
  void Validate_simulation_config();
  void Report_results_in_XML(std::string name_prefix,
                             Utils::XmlWriter& xmlwriter) final;

  void Attach_ssd_device(SSD_Device* ssd_device);
  const std::vector<Host_Components::IO_Flow_Base*> Get_io_flows();

};

#endif // !HOST_SYSTEM_H
