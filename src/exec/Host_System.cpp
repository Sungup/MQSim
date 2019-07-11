#include "../sim/Engine.h"
#include "Host_System.h"
#include "../ssd/interface/Host_Interface_NVMe.h"
#include "../ssd/interface/Host_Interface_SATA.h"
#include "../host/PCIe_Root_Complex.h"
#include "../host/IO_Flow_Synthetic.h"
#include "../host/IO_Flow_Trace_Based.h"
#include "../utils/StringTools.h"
#include "../utils/Logical_Address_Partitioning_Unit.h"



Host_System::Host_System(const HostParameterSet& params,
                         const IOFlowScenario& scenario,
                         bool preconditioning_required,
                         SSD_Components::Host_Interface_Base& host_interface,
                         const Utils::LogicalAddrPartition& lapu)
  : MQSimEngine::Sim_Object("Host"),
    __sata_hba(Host_Components::build_sata_hba(ID() + ".SATA_HBA",
                                               host_interface,
                                               params.SATA_Processing_Delay)),
    __link(ID() + ".PCIeLink",
           nullptr,
           nullptr,
           params.PCIe_Lane_Bandwidth,
           params.PCIe_Lane_Count),
    __root_complex(&__link,
                   host_interface.GetType(),
                   __sata_hba.get(),
                   nullptr),
    __pcie_switch(&__link,
                  &host_interface),
    __ssd_device(nullptr),
    __preconditioning_required(preconditioning_required)
{
  Simulator->AddObject(this);

  auto& nvme_interface = (SSD_Components::Host_Interface_NVMe&) host_interface;

  __link.Set_root_complex(&__root_complex);
  __link.Set_pcie_switch(&__pcie_switch);

  Simulator->AddObject(&__link);

  // No flow should ask for I/O queue id 0, it is reserved for NVMe admin
  // command queue pair. Hence, we use flow_id + 1 (which is equal to 1, 2,
  // ...) as the requested I/O queue id

  const uint16_t sq_size = host_interface.GetType() == HostInterface_Types::NVME
                             ? nvme_interface.Get_submission_queue_depth()
                             : 0;
  const uint16_t cq_size = host_interface.GetType() == HostInterface_Types::NVME
                             ? nvme_interface.Get_completion_queue_depth()
                             : 0;

  // Create IO flows
  for (uint16_t flow_id = 0; flow_id < scenario.size(); flow_id++) {
    Host_Components::IO_Flow_Base* io_flow = nullptr;

    switch (scenario[flow_id]->Type)
    {
    case Flow_Type::SYNTHETIC:
    {
      auto* flow_param = (SyntheticFlowParamSet*)scenario[flow_id].get();

      io_flow = new Host_Components::IO_Flow_Synthetic(ID() + ".IO_Flow.Synth.No_" + std::to_string(flow_id),
                                                       flow_id,
                                                       lapu.available_start_lha(flow_id),
                                                       lapu.available_end_lha(flow_id),
                                                       flow_param->working_set_rate(),
                                                       FLOW_ID_TO_Q_ID(flow_id),
                                                       sq_size,
                                                       cq_size,
                                                       flow_param->Priority_Class,
                                                       flow_param->read_rate(),
                                                       flow_param->Address_Distribution,
                                                       flow_param->hot_region_rate(),
                                                       flow_param->Request_Size_Distribution,
                                                       flow_param->Average_Request_Size,
                                                       flow_param->Variance_Request_Size,
                                                       flow_param->Synthetic_Generator_Type,
                                                       flow_param->avg_arrival_time(),
                                                       flow_param->Average_No_of_Reqs_in_Queue,
                                                       flow_param->Generated_Aligned_Addresses,
                                                       flow_param->Address_Alignment_Unit,
                                                       flow_param->Seed,
                                                       flow_param->Stop_Time,
                                                       flow_param->init_occupancy_rate(),
                                                       flow_param->Total_Requests_To_Generate,
                                                       host_interface.GetType(),
                                                       &__root_complex,
                                                       __sata_hba.get(),
                                                       params.Enable_ResponseTime_Logging,
                                                       params.ResponseTime_Logging_Period_Length,
                                                       params.stream_log_path(flow_id));
      this->IO_flows.push_back(io_flow);
      break;
    }
    case Flow_Type::TRACE:
    {
      auto* flow_param = (TraceFlowParameterSet*)scenario[flow_id].get();

      io_flow = new Host_Components::IO_Flow_Trace_Based(ID() + ".IO_Flow.Trace." + flow_param->File_Path,
                                                         flow_id,
                                                         lapu.available_start_lha(flow_id),
                                                         lapu.available_end_lha(flow_id),
                                                         FLOW_ID_TO_Q_ID(flow_id),
                                                         sq_size,
                                                         cq_size,
                                                         flow_param->Priority_Class,
                                                         flow_param->init_occupancy_rate(),
                                                         flow_param->File_Path,
                                                         flow_param->Time_Unit,
                                                         flow_param->Relay_Count,
                                                         flow_param->Percentage_To_Be_Executed,
                                                         host_interface.GetType(),
                                                         &__root_complex,
                                                         __sata_hba.get(),
                                                         params.Enable_ResponseTime_Logging,
                                                         params.ResponseTime_Logging_Period_Length,
                                                         params.stream_log_path(flow_id));

      IO_flows.push_back(io_flow);
      break;
    }
    }

    Simulator->AddObject(io_flow);
  }

  __root_complex.Set_io_flows(&IO_flows);

  if (host_interface.GetType() == HostInterface_Types::SATA)
  {
    __sata_hba->Set_io_flows(&IO_flows);
    __sata_hba->Set_root_complex(&__root_complex);
  }
}

Host_System::~Host_System() 
{
  for (uint16_t flow_id = 0; flow_id < IO_flows.size(); flow_id++)
    delete IO_flows[flow_id];
}

force_inline Utils::WorkloadStatsList
Host_System::__make_workloads_statistics()
{
  Utils::WorkloadStatsList stats(IO_flows.size());

  auto stat = stats.begin();

  for (auto &workload : IO_flows) {
    workload->get_stats(*stat,
                        __ssd_device->lha_to_lpa_converter,
                        __ssd_device->nvm_access_bitmap_finder);

    ++stat;
  }

  return stats;
}

void Host_System::Attach_ssd_device(SSD_Device* ssd_device)
{
  ssd_device->Attach_to_host(&__pcie_switch);
  __pcie_switch.Attach_ssd_device(&ssd_device->host_interface());

  __ssd_device = ssd_device;
}

const std::vector<Host_Components::IO_Flow_Base*> Host_System::Get_io_flows()
{
  return IO_flows;
}

void Host_System::Start_simulation()
{
  switch (__ssd_device->host_interface_type())
  {
  case HostInterface_Types::NVME:
    for (uint16_t flow_cntr = 0; flow_cntr < IO_flows.size(); flow_cntr++)
      ((SSD_Components::Host_Interface_NVMe&) __ssd_device->host_interface()).Create_new_stream(
        IO_flows[flow_cntr]->Priority_class(),
        IO_flows[flow_cntr]->Get_start_lsa_on_device(), IO_flows[flow_cntr]->Get_end_lsa_address_on_device(),
        IO_flows[flow_cntr]->Get_nvme_queue_pair_info()->Submission_queue_memory_base_address, IO_flows[flow_cntr]->Get_nvme_queue_pair_info()->Completion_queue_memory_base_address);
    break;
  case HostInterface_Types::SATA:
    ((SSD_Components::Host_Interface_SATA&) __ssd_device->host_interface()).Set_ncq_address(
      __sata_hba->Get_sata_ncq_info()->Submission_queue_memory_base_address, __sata_hba->Get_sata_ncq_info()->Completion_queue_memory_base_address);

  default:
    break;
  }

  if (__preconditioning_required)
  {
    auto workload_stats = __make_workloads_statistics();
    __ssd_device->Perform_preconditioning(workload_stats);
  }
}

void Host_System::Validate_simulation_config() 
{
  if (IO_flows.empty())
    throw mqsim_error("No IO flow is set for host system");

  if (!__pcie_switch.Is_ssd_connected())
    throw mqsim_error("No SSD is connected to the host system");
}

void
Host_System::Report_results_in_XML(std::string /* name_prefix */,
                                   Utils::XmlWriter& xmlwriter)
{
  xmlwriter.Write_open_tag(ID());

  for (auto &flow : IO_flows)
    flow->Report_results_in_XML("Host", xmlwriter);

  xmlwriter.Write_close_tag();
}
