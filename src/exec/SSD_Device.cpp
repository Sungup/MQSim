#include <algorithm>
#include <vector>
#include <stdexcept>
#include <cmath>
#include <ctime>
#include <numeric>
#include "SSD_Device.h"
#include "../ssd/fbm/Flash_Block_Manager.h"
#include "../ssd/dcm/Data_Cache_Manager_Flash_Advanced.h"
#include "../ssd/dcm/Data_Cache_Manager_Flash_Simple.h"
#include "../ssd/mapping/Address_Mapping_Unit_Base.h"
#include "../ssd/mapping/Address_Mapping_Unit_Page_Level.h"
#include "../ssd/mapping/Address_Mapping_Unit_Hybrid.h"
#include "../ssd/mapping/AddressMappingUnitDefs.h"
#include "../ssd/gc_and_wl/GC_and_WL_Unit_Page_Level.h"
#include "../ssd/tsu/TSU_OutofOrder.h"

SSD_Device::SSD_Device(const DeviceParameterSet& params,
                       const IOFlowScenario& io_flows,
                       const Utils::LogicalAddressPartitionUnit& lapu,
                       const StreamIdInfo& stream_info)
  : MQSimEngine::Sim_Object("SSDDevice"),
    __addr_partitioner(lapu),
    __stats(params),
    __channels(SSD_Components::build_onfi_channels(params, ID())),
    __phy(SSD_Components::build_onfi_phy(ID() + ".PHY",
                                         params,
                                         __channels,
                                         __stats)),
    __ftl(ID() + ".FTL", params, __stats),
    Preconditioning_required(params.Enabled_Preconditioning),
    lha_to_lpa_converter(this, &SSD_Device::__convert_lha_to_lpa),
    nvm_access_bitmap_finder(this, &SSD_Device::__find_nvm_subunit_access_bitmap)
{
  // 0. Initialize temporary variables to build objects.
  double max_rho = 0;
  SSD_Components::CachingModeList caching_modes;
  caching_modes.reserve(io_flows.size());

  for (const auto& flow : io_flows) {
    max_rho = std::max(max_rho, double(flow->Initial_Occupancy_Percentage));
    caching_modes.emplace_back(flow->Device_Level_Data_Caching_Mode);
  }

  // 1. Register SSD
  Simulator->AddObject(this);

  // 2. Register PHY
  Simulator->AddObject(__phy.get());

  // 3. Register FTL
  Simulator->AddObject(&__ftl);

  // 4. Create and register TSU
  auto tsu = SSD_Components::build_tsu_object(params,
                                              __ftl,
                                              *__phy);

  __ftl.assign_tsu(tsu);
  Simulator->AddObject(tsu.get());

  // 5. Create and register Flash_Block_Manager
  auto fbm = SSD_Components::build_fbm_object(params,
                                              io_flows.size(),
                                              __stats);

  __ftl.assign_fbm(fbm);

  // 6. Create and register Address_Mapping_Unit
  auto amu = SSD_Components::build_amu_object(params,
                                              lapu,
                                              stream_info,
                                              __ftl,
                                              *__phy,
                                              *fbm,
                                              __stats);

  __ftl.assign_amu(amu);
  Simulator->AddObject(amu.get());


  // 7. Create and register GC_and_WL_Unit
  auto gcwl = SSD_Components::build_gc_and_wl_object(params,
                                                     __ftl,
                                                     *amu,
                                                     *fbm,
                                                     *tsu,
                                                     *__phy,
                                                     __stats,
                                                     max_rho / 100,
                                                     10);

  __ftl.assign_gcwl(gcwl);
  Simulator->AddObject(gcwl.get());
  fbm->Set_GC_and_WL_Unit(gcwl.get());

  // 8. Create and register Data_Cache_Manager
  __cache_manager = SSD_Components::build_dcm_object(ID() + ".DataCache",
                                                     params,
                                                     __ftl,
                                                     *__phy,
                                                     caching_modes);

  Simulator->AddObject(__cache_manager.get());
  __ftl.assign_dcm(__cache_manager.get());

  // 9. Create and register Host_Interface
  switch (params.HostInterface_Type)
  {
  case HostInterface_Types::NVME:
    Host_interface = new SSD_Components::Host_Interface_NVMe(ID() + ".HostInterface",
                                                             __addr_partitioner.get_total_device_lha_count(),
                                                             params.IO_Queue_Depth,
                                                             params.IO_Queue_Depth,
                                                             (uint32_t)io_flows.size(),
                                                             params.Queue_Fetch_Size,
                                                             params.Flash_Parameters.page_size_in_sector(),
                                                             __cache_manager.get());
    break;
  case HostInterface_Types::SATA:
    Host_interface = new SSD_Components::Host_Interface_SATA(ID() + ".HostInterface",
                                                             params.IO_Queue_Depth,
                                                             __addr_partitioner.get_total_device_lha_count(),
                                                             params.Flash_Parameters.page_size_in_sector(),
                                                             __cache_manager.get());

    break;
  default:
    break;
  }
  Simulator->AddObject(Host_interface);
  __cache_manager->Set_host_interface(Host_interface);
}

SSD_Device::~SSD_Device()
{
  delete Host_interface;
}

void SSD_Device::Attach_to_host(Host_Components::PCIe_Switch* pcie_switch)
{
  Host_interface->Attach_to_device(pcie_switch);
}

void SSD_Device::Perform_preconditioning(const std::vector<Utils::Workload_Statistics*>& workload_stats)
{
  if (Preconditioning_required)
  {
    time_t start_time = time(nullptr);
    PRINT_MESSAGE("SSD Device preconditioning started .........");
    __ftl.Perform_precondition(workload_stats);
    __cache_manager->Do_warmup(workload_stats);
    time_t end_time = time(nullptr);
    auto duration = (uint64_t)difftime(end_time, start_time);
    PRINT_MESSAGE("Finished preconditioning. Duration of preconditioning: " << duration / 3600 << ":" << (duration % 3600) / 60 << ":" << ((duration % 3600) % 60));
  }
}

void SSD_Device::Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter)
{
  std::string tmp;
  tmp = ID();
  xmlwriter.Write_open_tag(tmp);

  Host_interface->Report_results_in_XML(ID(), xmlwriter);

  __ftl.Report_results_in_XML(ID(), xmlwriter);

  for (auto& channel : __channels)
    for (auto& chip : channel.Chips)
      chip.Report_results_in_XML(ID(), xmlwriter);

  xmlwriter.Write_close_tag();
}

uint32_t SSD_Device::Get_no_of_LHAs_in_an_NVM_write_unit()
{
  return Host_interface->Get_no_of_LHAs_in_an_NVM_write_unit();
}


LPA_type
SSD_Device::__convert_lha_to_lpa(LHA_type lha) const
{
  return __ftl.Convert_host_logical_address_to_device_address(lha);
}

page_status_type
SSD_Device::__find_nvm_subunit_access_bitmap(LHA_type lha) const
{
  return __ftl.Find_NVM_subunit_access_bitmap(lha);
}
