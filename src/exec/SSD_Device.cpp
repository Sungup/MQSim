#include <vector>
#include <stdexcept>
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
    Memory_Type(params.Memory_Type),
    Channel_count(params.Flash_Channel_Count),
    Chip_no_per_channel(params.Chip_No_Per_Channel),
    lha_to_lpa_converter(this, &SSD_Device::__convert_lha_to_lpa),
    nvm_access_bitmap_finder(this, &SSD_Device::__find_nvm_subunit_access_bitmap)
{
  Simulator->AddObject(this);

  Simulator->AddObject(__phy.get());

  Simulator->AddObject(&__ftl);

  //Step 5: create TSU
  auto tsu = SSD_Components::build_tsu_object(__ftl.ID() + ".TSU",
                                              params,
                                              __ftl,
                                              *__phy);
  Simulator->AddObject(tsu.get());
  __ftl.assign_tsu(tsu);

  //Step 6: create Flash_Block_Manager
  auto fbm = SSD_Components::build_fbm_object(params,
                                              io_flows.size(),
                                              __stats);
  __ftl.assign_fbm(fbm);

  //Step 7: create Address_Mapping_Unit
  SSD_Components::Address_Mapping_Unit_Base* amu;

  switch (params.Address_Mapping)
  {
  case SSD_Components::Flash_Address_Mapping_Type::PAGE_LEVEL:
    amu = new SSD_Components::Address_Mapping_Unit_Page_Level(__ftl.ID() + ".AddressMappingUnit",
                                                              &__ftl,
                                                              __phy.get(),
                                                              fbm.get(),
                                                              lapu,
                                                              __stats,
                                                              params.Ideal_Mapping_Table,
                                                              params.CMT_Capacity,
                                                              params.Plane_Allocation_Scheme,
                                                              stream_info.stream_count,
                                                              params.Flash_Channel_Count,
                                                              params.Chip_No_Per_Channel,
                                                              params.Flash_Parameters.Die_No_Per_Chip,
                                                              params.Flash_Parameters.Plane_No_Per_Die,
                                                              stream_info.stream_channel_ids(),
                                                              stream_info.stream_chip_ids(),
                                                              stream_info.stream_die_ids(),
                                                              stream_info.stream_plane_ids(),
                                                              params.Flash_Parameters.Block_No_Per_Plane,
                                                              params.Flash_Parameters.Page_No_Per_Block,
                                                              params.Flash_Parameters.page_size_in_sector(),
                                                              params.Flash_Parameters.Page_Capacity,
                                                              params.Overprovisioning_Ratio,
      params.CMT_Sharing_Mode);
    break;
  case SSD_Components::Flash_Address_Mapping_Type::HYBRID:
    amu = new SSD_Components::Address_Mapping_Unit_Hybrid(__ftl.ID() + ".AddressMappingUnit",
                                                          &__ftl,
                                                          __phy.get(),
                                                          fbm.get(),
                                                          __stats,
                                                          params.Ideal_Mapping_Table,
                                                          stream_info.stream_count,
                                                          params.Flash_Channel_Count,
                                                          params.Chip_No_Per_Channel,
                                                          params.Flash_Parameters.Die_No_Per_Chip,
                                                          params.Flash_Parameters.Plane_No_Per_Die,
                                                          params.Flash_Parameters.Block_No_Per_Plane,
                                                          params.Flash_Parameters.Page_No_Per_Block,
                                                          params.Flash_Parameters.page_size_in_sector(),
                                                          params.Flash_Parameters.Page_Capacity,
                                                          params.Overprovisioning_Ratio);
    break;
  default:
    throw std::invalid_argument("No implementation is available fo the secified address mapping strategy");
  }
  Simulator->AddObject(amu);
  __ftl.Address_Mapping_Unit = amu;

  //Step 8: create GC_and_WL_unit
  double max_rho = 0;
  for (uint32_t i = 0; i < io_flows.size(); i++)
    if (io_flows[i]->Initial_Occupancy_Percentage > max_rho)
      max_rho = io_flows[i]->Initial_Occupancy_Percentage;
  max_rho /= 100;//Convert from percentage to a value between zero and 1
  SSD_Components::GC_and_WL_Unit_Base* gcwl;
  gcwl = new SSD_Components::GC_and_WL_Unit_Page_Level(__ftl.ID() + ".GCandWLUnit", amu, fbm.get(), tsu.get(), __phy.get(),
    __stats,
    params.GC_Block_Selection_Policy, params.GC_Exec_Threshold, params.Preemptible_GC_Enabled, params.GC_Hard_Threshold,
    params.Flash_Channel_Count, params.Chip_No_Per_Channel,
    params.Flash_Parameters.Die_No_Per_Chip, params.Flash_Parameters.Plane_No_Per_Die,
    params.Flash_Parameters.Block_No_Per_Plane, params.Flash_Parameters.Page_No_Per_Block,
    params.Flash_Parameters.Page_Capacity / SECTOR_SIZE_IN_BYTE, params.Use_Copyback_for_GC, max_rho, 10,
    params.Seed++);
  Simulator->AddObject(gcwl);
  fbm->Set_GC_and_WL_Unit(gcwl);
  __ftl.GC_and_WL_Unit = gcwl;

  //Step 9: create Data_Cache_Manager
  SSD_Components::Data_Cache_Manager_Base* dcm;
  SSD_Components::Caching_Mode* caching_modes = new SSD_Components::Caching_Mode[io_flows.size()];
  for (uint32_t i = 0; i < io_flows.size(); i++)
    caching_modes[i] = io_flows[i]->Device_Level_Data_Caching_Mode;

  switch (params.Caching_Mechanism)
  {
  case SSD_Components::Caching_Mechanism::SIMPLE:
    dcm = new SSD_Components::Data_Cache_Manager_Flash_Simple(ID() + ".DataCache", nullptr, &__ftl, __phy.get(),
      params.Data_Cache_Capacity, params.Data_Cache_DRAM_Row_Size, params.Data_Cache_DRAM_Data_Rate,
      params.Data_Cache_DRAM_Data_Busrt_Size, params.Data_Cache_DRAM_tRCD, params.Data_Cache_DRAM_tCL, params.Data_Cache_DRAM_tRP,
      caching_modes, (uint32_t)io_flows.size(),
      params.Flash_Parameters.Page_Capacity / SECTOR_SIZE_IN_BYTE, params.Flash_Channel_Count * params.Chip_No_Per_Channel * params.Flash_Parameters.Die_No_Per_Chip * params.Flash_Parameters.Plane_No_Per_Die * params.Flash_Parameters.Page_Capacity / SECTOR_SIZE_IN_BYTE);

    break;
  case SSD_Components::Caching_Mechanism::ADVANCED:
    dcm = new SSD_Components::Data_Cache_Manager_Flash_Advanced(ID() + ".DataCache", nullptr, &__ftl, __phy.get(),
      params.Data_Cache_Capacity, params.Data_Cache_DRAM_Row_Size, params.Data_Cache_DRAM_Data_Rate,
      params.Data_Cache_DRAM_Data_Busrt_Size, params.Data_Cache_DRAM_tRCD, params.Data_Cache_DRAM_tCL, params.Data_Cache_DRAM_tRP,
      caching_modes, params.Data_Cache_Sharing_Mode, (uint32_t)io_flows.size(),
      params.Flash_Parameters.Page_Capacity / SECTOR_SIZE_IN_BYTE, params.Flash_Channel_Count * params.Chip_No_Per_Channel * params.Flash_Parameters.Die_No_Per_Chip * params.Flash_Parameters.Plane_No_Per_Die * params.Flash_Parameters.Page_Capacity / SECTOR_SIZE_IN_BYTE);

    break;
  default:
    PRINT_ERROR("Unknown data caching mechanism!")
  }    Simulator->AddObject(dcm);
  __ftl.assign(dcm);
  Cache_manager = dcm;

  //Step 10: create Host_Interface
  switch (params.HostInterface_Type)
  {
  case HostInterface_Types::NVME:
    Host_interface = new SSD_Components::Host_Interface_NVMe(ID() + ".HostInterface",
      __addr_partitioner.get_total_device_lha_count(), params.IO_Queue_Depth, params.IO_Queue_Depth,
      (uint32_t)io_flows.size(), params.Queue_Fetch_Size, params.Flash_Parameters.Page_Capacity / SECTOR_SIZE_IN_BYTE, dcm);
    break;
  case HostInterface_Types::SATA:
    Host_interface = new SSD_Components::Host_Interface_SATA(ID() + ".HostInterface",
      params.IO_Queue_Depth, __addr_partitioner.get_total_device_lha_count(), params.Flash_Parameters.Page_Capacity / SECTOR_SIZE_IN_BYTE, dcm);

    break;
  default:
    break;
  }
  Simulator->AddObject(Host_interface);
  dcm->Set_host_interface(Host_interface);
}

SSD_Device::~SSD_Device()
{
  delete __ftl.Address_Mapping_Unit;
  delete __ftl.GC_and_WL_Unit;
  delete this->Cache_manager;
  delete this->Host_interface;
}

void SSD_Device::Attach_to_host(Host_Components::PCIe_Switch* pcie_switch)
{
  this->Host_interface->Attach_to_device(pcie_switch);
}

void SSD_Device::Perform_preconditioning(const std::vector<Utils::Workload_Statistics*>& workload_stats)
{
  if (Preconditioning_required)
  {
    time_t start_time = time(0);
    PRINT_MESSAGE("SSD Device preconditioning started .........");
    __ftl.Perform_precondition(workload_stats);
    Cache_manager->Do_warmup(workload_stats);
    time_t end_time = time(0);
    uint64_t duration = (uint64_t)difftime(end_time, start_time);
    PRINT_MESSAGE("Finished preconditioning. Duration of preconditioning: " << duration / 3600 << ":" << (duration % 3600) / 60 << ":" << ((duration % 3600) % 60));
  }
}

void SSD_Device::Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter)
{
  std::string tmp;
  tmp = ID();
  xmlwriter.Write_open_tag(tmp);

  this->Host_interface->Report_results_in_XML(ID(), xmlwriter);
  if (Memory_Type == NVM::NVM_Type::FLASH)
  {
    __ftl.Report_results_in_XML(ID(), xmlwriter);

    for (uint32_t channel_cntr = 0; channel_cntr < Channel_count; channel_cntr++)
      for (uint32_t chip_cntr = 0; chip_cntr < Chip_no_per_channel; chip_cntr++)
        __channels[channel_cntr].Chips[chip_cntr].Report_results_in_XML(ID(), xmlwriter);
  }
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
