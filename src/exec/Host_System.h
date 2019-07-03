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
public:
  Host_System(HostParameterSet* parameters, bool preconditioning_required, SSD_Components::Host_Interface_Base* ssd_host_interface);
  ~Host_System();
  void Start_simulation();
  void Validate_simulation_config();
  void Execute_simulator_event(MQSimEngine::SimEvent* event);
  void Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter);

  void Attach_ssd_device(SSD_Device* ssd_device);
  const std::vector<Host_Components::IO_Flow_Base*> Get_io_flows();
private:
  Host_Components::PCIe_Root_Complex* PCIe_root_complex;
  Host_Components::PCIe_Link* Link;
  Host_Components::PCIe_Switch* PCIe_switch;
  Host_Components::SATA_HBA* SATA_hba;
  std::vector<Host_Components::IO_Flow_Base*> IO_flows;
  SSD_Device* ssd_device;
  std::vector<Utils::Workload_Statistics*> get_workloads_statistics();
  bool preconditioning_required;
};

#endif // !HOST_SYSTEM_H
