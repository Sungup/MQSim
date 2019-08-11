#include "HostSystem.h"

#include "../sim/Engine.h"
#include "../ssd/interface/Host_Interface_NVMe.h"
#include "../host/PCIe_Root_Complex.h"
#include "../host/IO_Flow_Synthetic.h"
#include "../host/IO_Flow_Trace_Based.h"
#include "../utils/StringTools.h"
#include "../utils/Logical_Address_Partitioning_Unit.h"

HostSystem::HostSystem(const HostParameterSet& params,
                         const IOFlowScenario& scenario,
                         const Utils::LogicalAddrPartition& lapu,
                         SsdDevice& ssd)
  : MQSimEngine::Sim_Object("Host"),
    __io_flows(),
    __ssd_device(ssd),
    __link(ID() + ".PCIeLink",
           params.PCIe_Lane_Bandwidth,
           params.PCIe_Lane_Count),
    __root_complex(__link, __io_flows, ssd.host_interface_type()),
    __pcie_switch(__link),
    __sata_hba(Host_Components::build_sata_hba(ID() + ".SATA_HBA",
                                               ssd.host_interface_type(),
                                               ssd.sata_ncq_depth(),
                                               params.SATA_Processing_Delay,
                                               __root_complex,
                                               __io_flows))
{
  Simulator->AddObject(this);

  __link.Set_root_complex(&__root_complex);
  __link.Set_pcie_switch(&__pcie_switch);

  Simulator->AddObject(&__link);

  // No flow should ask for I/O queue id 0, it is reserved for NVMe admin
  // command queue pair. Hence, we use flow_id + 1 (which is equal to 1, 2,
  // ...) as the requested I/O queue id

  const uint16_t sq_size = ssd.nvme_sq_size();
  const uint16_t cq_size = ssd.nvme_cq_size();

  // Create IO flows
  for (uint16_t flow_id = 0; flow_id < scenario.size(); flow_id++) {
    auto flow = Host_Components::build_io_flow(ID(),
                                               params,
                                               *scenario[flow_id],
                                               lapu,
                                               flow_id,
                                               __root_complex,
                                               __sata_hba.get(),
                                               ssd.host_interface_type(),
                                               sq_size,
                                               cq_size);

    __io_flows.emplace_back(flow);

    Simulator->AddObject(flow.get());
  }

  if (ssd.host_interface_type() == HostInterface_Types::SATA)
    __root_complex.assign_sata_hba(__sata_hba.get());

  __connect_ssd();
}

force_inline void
HostSystem::__connect_ssd()
{
  __ssd_device.connect_to_host(&__pcie_switch);
  __pcie_switch.connect_ssd(&__ssd_device.host_interface());
}

force_inline void
HostSystem::__init_nvme_queue()
{
  // Make make queue information per IO flow.
  for (auto& flow : __io_flows)
    __ssd_device.create_new_stream(
      flow->priority_class(),
      flow->start_lsa,
      flow->end_lsa,
      flow->sq_base_address(),
      flow->cq_base_address()
  );
}

force_inline void
HostSystem::__init_sata_queue()
{
  __ssd_device.set_ncq_address(
    __sata_hba->sq_base_address(),
    __sata_hba->cq_base_address()
  );
}

force_inline Utils::WorkloadStatsList
HostSystem::__make_workloads_statistics()
{
  Utils::WorkloadStatsList stats(__io_flows.size());

  auto stat_iter = stats.begin();

  for (auto &workload : __io_flows) {
    workload->get_stats(*stat_iter,
                        __ssd_device.lha_to_lpa_converter,
                        __ssd_device.nvm_access_bitmap_finder);

    ++stat_iter;
  }

  return stats;
}

void
HostSystem::Start_simulation()
{
  switch (__ssd_device.host_interface_type()) {
  case HostInterface_Types::NVME: __init_nvme_queue(); break;
  case HostInterface_Types::SATA: __init_sata_queue(); break;
  }

  if (__ssd_device.preconditioning_required()) {
    auto workload_stats = __make_workloads_statistics();
    __ssd_device.perform_preconditioning(workload_stats);
  }
}

void
HostSystem::Validate_simulation_config()
{
  if (__io_flows.empty())
    throw mqsim_error("No IO flow is set for host system");

  if (!__pcie_switch.is_ssd_connected())
    throw mqsim_error("No SSD is connected to the host system");
}

void
HostSystem::Report_results_in_XML(std::string /* name_prefix */,
                                   Utils::XmlWriter& xmlwriter)
{
  xmlwriter.Write_open_tag(ID());

  for (auto &flow : __io_flows)
    flow->Report_results_in_XML("Host", xmlwriter);

  xmlwriter.Write_close_tag();
}
