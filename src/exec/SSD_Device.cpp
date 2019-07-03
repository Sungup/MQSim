#include <vector>
#include <stdexcept>
#include <ctime>
#include <numeric>
#include "SSD_Device.h"
#include "../ssd/ONFI_Channel_Base.h"
#include "../ssd/fbm/Flash_Block_Manager.h"
#include "../ssd/dcm/Data_Cache_Manager_Flash_Advanced.h"
#include "../ssd/dcm/Data_Cache_Manager_Flash_Simple.h"
#include "../ssd/mapping/Address_Mapping_Unit_Base.h"
#include "../ssd/mapping/Address_Mapping_Unit_Page_Level.h"
#include "../ssd/mapping/Address_Mapping_Unit_Hybrid.h"
#include "../ssd/mapping/AddressMappingUnitDefs.h"
#include "../ssd/gc_and_wl/GC_and_WL_Unit_Page_Level.h"
#include "../ssd/tsu/TSU_OutofOrder.h"
#include "../ssd/ONFI_Channel_NVDDR2.h"
#include "../ssd/phy/NVM_PHY_ONFI_NVDDR2.h"
#include "../utils/Logical_Address_Partitioning_Unit.h"

template <typename T>
force_inline T
__build_id_list(uint32_t count)
{
  T id_list(count);
  std::iota(id_list.begin(), id_list.end(), 0);

  return id_list;
}

force_inline NVM::FlashMemory::FlashChipList
__build_chips_on_channel(const DeviceParameterSet& params,
                         const sim_object_id_type& id,
                         uint32_t channel)
{
  using namespace NVM::FlashMemory;

  auto id_form = id + ".Channel." + std::to_string(channel) + ".Chip.";

  FlashChipList chips;

  for (uint32_t chip = 0; chip < params.Chip_No_Per_Channel; ++chip) {
    chips.emplace_back(id_form + std::to_string(chip),
                       params.Flash_Parameters,
                       channel, chip);

    Simulator->AddObject(&chips.back());
  }

  return chips;
}

SSD_Device::SSD_Device(DeviceParameterSet& params, IOFlowScenario& io_flows)
  : MQSimEngine::Sim_Object("SSDDevice"),
    ftl(ID() + ".FTL", params),
    Preconditioning_required(params.Enabled_Preconditioning),
    Memory_Type(params.Memory_Type),
    Channel_count(params.Flash_Channel_Count),
    Chip_no_per_channel(params.Chip_No_Per_Channel),
    lha_to_lpa_converter(this, &SSD_Device::__convert_lha_to_lpa),
    nvm_access_bitmap_finder(this, &SSD_Device::__find_nvm_subunit_access_bitmap)
{
  Simulator->AddObject(this);

  //Steps 4 - 8: create FTL components and connect them together
  Simulator->AddObject(&ftl);

  //Step 2: create memory channels to connect chips to the controller
  SSD_Components::ONFI_Channel_NVDDR2** channels = new SSD_Components::ONFI_Channel_NVDDR2*[params.Flash_Channel_Count];
  for (uint32_t channel_cntr = 0; channel_cntr < params.Flash_Channel_Count; channel_cntr++)
  {
    NVM::FlashMemory::Flash_Chip** chips = new NVM::FlashMemory::Flash_Chip*[params.Chip_No_Per_Channel];

    for (uint32_t chip_cntr = 0; chip_cntr < params.Chip_No_Per_Channel; chip_cntr++)
    {
      chips[chip_cntr] = new NVM::FlashMemory::Flash_Chip(ID(),
                                                          params.Flash_Parameters,
                                                          channel_cntr,
                                                          chip_cntr);

      Simulator->AddObject(chips[chip_cntr]);//Each simulation object (a child of MQSimEngine::Sim_Object) should be added to the engine
    }

    channels[channel_cntr] = new SSD_Components::ONFI_Channel_NVDDR2(channel_cntr,
                                                                     chips,
                                                                     params.Flash_Channel_Width,
                                                                     (sim_time_type)((double)1000 / params.Channel_Transfer_Rate) * 2,
                                                                     (sim_time_type)((double)1000 / params.Channel_Transfer_Rate) * 2);

    Channels.push_back(channels[channel_cntr]);//Channels should not be added to the simulator core, they are passive object that do not handle any simulation event
  }

  //Step 3: create channel controller and connect channels to it
  auto* phy = new SSD_Components::NVM_PHY_ONFI_NVDDR2(ID() + ".PHY",
                                                      channels,
                                                      ftl.get_stats_reference(),
                                                      params.Flash_Channel_Count,
                                                      params.Chip_No_Per_Channel,
                                                      params.Flash_Parameters.Die_No_Per_Chip,
                                                      params.Flash_Parameters.Plane_No_Per_Die);
  PHY = phy;
  Simulator->AddObject(PHY);

  ftl.PHY = phy;

  //Step 5: create TSU
  SSD_Components::TSU_Base* tsu;

  tsu = new SSD_Components::TSU_OutOfOrder(ftl.ID() + ".TSU",
                                           &ftl,
                                           phy,
                                           params.Flash_Channel_Count,
                                           params.Chip_No_Per_Channel,
                                           params.Flash_Parameters.Die_No_Per_Chip,
                                           params.Flash_Parameters.Plane_No_Per_Die,
                                           params.Preferred_suspend_write_time_for_read,
                                           params.Preferred_suspend_erase_time_for_read,
                                           params.Preferred_suspend_erase_time_for_write,
                                           params.Flash_Parameters.erase_suspension_support(),
                                           params.Flash_Parameters.program_suspension_support());

  Simulator->AddObject(tsu);
  ftl.TSU = tsu;

  //Step 6: create Flash_Block_Manager
  SSD_Components::Flash_Block_Manager_Base* fbm;
  fbm = new SSD_Components::Flash_Block_Manager(nullptr, ftl.get_stats_reference(), params.Flash_Parameters.Block_PE_Cycles_Limit,
    (uint32_t)io_flows.size(), params.Flash_Channel_Count, params.Chip_No_Per_Channel,
    params.Flash_Parameters.Die_No_Per_Chip, params.Flash_Parameters.Plane_No_Per_Die,
    params.Flash_Parameters.Block_No_Per_Plane, params.Flash_Parameters.Page_No_Per_Block);
  ftl.BlockManager = fbm;


  //Step 7: create Address_Mapping_Unit
  SSD_Components::Address_Mapping_Unit_Base* amu;
  StreamChannelIDs flow_channel_id_assignments;
  StreamChipIDs    flow_chip_id_assignments;
  StreamDieIDs     flow_die_id_assignments;
  StreamPlaneIDs   flow_plane_id_assignments;
  uint32_t stream_count = 0;

  if (params.HostInterface_Type == HostInterface_Types::SATA) {
    stream_count = 1;

    // for SATA
    for (auto& flow : io_flows) {
      flow_channel_id_assignments.emplace_back(
        __build_id_list<ChannelIDs>(params.Flash_Channel_Count)
      );

      flow_chip_id_assignments.emplace_back(
        __build_id_list<ChipIDs>(params.Chip_No_Per_Channel)
      );

      flow_die_id_assignments.emplace_back(
        __build_id_list<DieIDs>(params.Flash_Parameters.Die_No_Per_Chip)
      );

      flow_plane_id_assignments.emplace_back(
        __build_id_list<PlaneIDs>(params.Flash_Parameters.Plane_No_Per_Die)
      );
    }
  } else {
    stream_count = (uint32_t)io_flows.size();

    // for NVMe
    for (auto& flow : io_flows) {
      flow_channel_id_assignments.emplace_back(
        ChannelIDs(flow->Channel_IDs.begin(), flow->Channel_IDs.end())
      );

      flow_chip_id_assignments.emplace_back(
        ChipIDs(flow->Chip_IDs.begin(), flow->Chip_IDs.end())
      );

      flow_die_id_assignments.emplace_back(
        DieIDs(flow->Die_IDs.begin(), flow->Die_IDs.end())
      );

      flow_plane_id_assignments.emplace_back(
        PlaneIDs(flow->Plane_IDs.begin(), flow->Plane_IDs.end())
      );
    }
  }

  Utils::Logical_Address_Partitioning_Unit::Allocate_logical_address_for_flows(params.HostInterface_Type,
                                                                               (uint32_t)io_flows.size(),
                                                                               params.Flash_Channel_Count,
                                                                               params.Chip_No_Per_Channel,
                                                                               params.Flash_Parameters.Die_No_Per_Chip,
                                                                               params.Flash_Parameters.Plane_No_Per_Die,
                                                                               flow_channel_id_assignments,
                                                                               flow_chip_id_assignments,
                                                                               flow_die_id_assignments,
                                                                               flow_plane_id_assignments,
                                                                               params.Flash_Parameters.Block_No_Per_Plane,
                                                                               params.Flash_Parameters.Page_No_Per_Block,
                                                                               params.Flash_Parameters.page_size_in_sector(),
                                                                               params.Overprovisioning_Ratio);
  switch (params.Address_Mapping)
  {
  case SSD_Components::Flash_Address_Mapping_Type::PAGE_LEVEL:
    amu = new SSD_Components::Address_Mapping_Unit_Page_Level(ftl.ID() + ".AddressMappingUnit",
                                                              &ftl,
                                                              (SSD_Components::NVM_PHY_ONFI*) PHY,
                                                              fbm,
                                                              ftl.get_stats_reference(),
                                                              params.Ideal_Mapping_Table,
                                                              params.CMT_Capacity,
                                                              params.Plane_Allocation_Scheme,
                                                              stream_count,
                                                              params.Flash_Channel_Count,
                                                              params.Chip_No_Per_Channel,
                                                              params.Flash_Parameters.Die_No_Per_Chip,
                                                              params.Flash_Parameters.Plane_No_Per_Die,
                                                              flow_channel_id_assignments,
                                                              flow_chip_id_assignments,
                                                              flow_die_id_assignments,
                                                              flow_plane_id_assignments,
                                                              params.Flash_Parameters.Block_No_Per_Plane,
                                                              params.Flash_Parameters.Page_No_Per_Block,
                                                              params.Flash_Parameters.Page_Capacity / SECTOR_SIZE_IN_BYTE,
                                                              params.Flash_Parameters.Page_Capacity,
                                                              params.Overprovisioning_Ratio,
      params.CMT_Sharing_Mode);
    break;
  case SSD_Components::Flash_Address_Mapping_Type::HYBRID:
    amu = new SSD_Components::Address_Mapping_Unit_Hybrid(ftl.ID() + ".AddressMappingUnit",
                                                          &ftl,
                                                          (SSD_Components::NVM_PHY_ONFI*) PHY,
                                                          fbm,
                                                          ftl.get_stats_reference(),
                                                          params.Ideal_Mapping_Table,
                                                          stream_count,
                                                          params.Flash_Channel_Count,
                                                          params.Chip_No_Per_Channel,
                                                          params.Flash_Parameters.Die_No_Per_Chip,
                                                          params.Flash_Parameters.Plane_No_Per_Die,
                                                          params.Flash_Parameters.Block_No_Per_Plane,
                                                          params.Flash_Parameters.Page_No_Per_Block,
                                                          params.Flash_Parameters.Page_Capacity / SECTOR_SIZE_IN_BYTE,
                                                          params.Flash_Parameters.Page_Capacity,
                                                          params.Overprovisioning_Ratio);
    break;
  default:
    throw std::invalid_argument("No implementation is available fo the secified address mapping strategy");
  }
  Simulator->AddObject(amu);
  ftl.Address_Mapping_Unit = amu;

  //Step 8: create GC_and_WL_unit
  double max_rho = 0;
  for (uint32_t i = 0; i < io_flows.size(); i++)
    if (io_flows[i]->Initial_Occupancy_Percentage > max_rho)
      max_rho = io_flows[i]->Initial_Occupancy_Percentage;
  max_rho /= 100;//Convert from percentage to a value between zero and 1
  SSD_Components::GC_and_WL_Unit_Base* gcwl;
  gcwl = new SSD_Components::GC_and_WL_Unit_Page_Level(ftl.ID() + ".GCandWLUnit", amu, fbm, tsu, (SSD_Components::NVM_PHY_ONFI*)PHY,
    ftl.get_stats_reference(),
    params.GC_Block_Selection_Policy, params.GC_Exec_Threshold, params.Preemptible_GC_Enabled, params.GC_Hard_Threshold,
    params.Flash_Channel_Count, params.Chip_No_Per_Channel,
    params.Flash_Parameters.Die_No_Per_Chip, params.Flash_Parameters.Plane_No_Per_Die,
    params.Flash_Parameters.Block_No_Per_Plane, params.Flash_Parameters.Page_No_Per_Block,
    params.Flash_Parameters.Page_Capacity / SECTOR_SIZE_IN_BYTE, params.Use_Copyback_for_GC, max_rho, 10,
    params.Seed++);
  Simulator->AddObject(gcwl);
  fbm->Set_GC_and_WL_Unit(gcwl);
  ftl.GC_and_WL_Unit = gcwl;

  //Step 9: create Data_Cache_Manager
  SSD_Components::Data_Cache_Manager_Base* dcm;
  SSD_Components::Caching_Mode* caching_modes = new SSD_Components::Caching_Mode[io_flows.size()];
  for (uint32_t i = 0; i < io_flows.size(); i++)
    caching_modes[i] = io_flows[i]->Device_Level_Data_Caching_Mode;

  switch (params.Caching_Mechanism)
  {
  case SSD_Components::Caching_Mechanism::SIMPLE:
    dcm = new SSD_Components::Data_Cache_Manager_Flash_Simple(ID() + ".DataCache", nullptr, &ftl, (SSD_Components::NVM_PHY_ONFI*) PHY,
      params.Data_Cache_Capacity, params.Data_Cache_DRAM_Row_Size, params.Data_Cache_DRAM_Data_Rate,
      params.Data_Cache_DRAM_Data_Busrt_Size, params.Data_Cache_DRAM_tRCD, params.Data_Cache_DRAM_tCL, params.Data_Cache_DRAM_tRP,
      caching_modes, (uint32_t)io_flows.size(),
      params.Flash_Parameters.Page_Capacity / SECTOR_SIZE_IN_BYTE, params.Flash_Channel_Count * params.Chip_No_Per_Channel * params.Flash_Parameters.Die_No_Per_Chip * params.Flash_Parameters.Plane_No_Per_Die * params.Flash_Parameters.Page_Capacity / SECTOR_SIZE_IN_BYTE);

    break;
  case SSD_Components::Caching_Mechanism::ADVANCED:
    dcm = new SSD_Components::Data_Cache_Manager_Flash_Advanced(ID() + ".DataCache", nullptr, &ftl, (SSD_Components::NVM_PHY_ONFI*) PHY,
      params.Data_Cache_Capacity, params.Data_Cache_DRAM_Row_Size, params.Data_Cache_DRAM_Data_Rate,
      params.Data_Cache_DRAM_Data_Busrt_Size, params.Data_Cache_DRAM_tRCD, params.Data_Cache_DRAM_tCL, params.Data_Cache_DRAM_tRP,
      caching_modes, params.Data_Cache_Sharing_Mode, (uint32_t)io_flows.size(),
      params.Flash_Parameters.Page_Capacity / SECTOR_SIZE_IN_BYTE, params.Flash_Channel_Count * params.Chip_No_Per_Channel * params.Flash_Parameters.Die_No_Per_Chip * params.Flash_Parameters.Plane_No_Per_Die * params.Flash_Parameters.Page_Capacity / SECTOR_SIZE_IN_BYTE);

    break;
  default:
    PRINT_ERROR("Unknown data caching mechanism!")
  }    Simulator->AddObject(dcm);
  ftl.assign(dcm);
  Cache_manager = dcm;

  //Step 10: create Host_Interface
  switch (params.HostInterface_Type)
  {
  case HostInterface_Types::NVME:
    Host_interface = new SSD_Components::Host_Interface_NVMe(ID() + ".HostInterface",
      Utils::Logical_Address_Partitioning_Unit::Get_total_device_lha_count(), params.IO_Queue_Depth, params.IO_Queue_Depth,
      (uint32_t)io_flows.size(), params.Queue_Fetch_Size, params.Flash_Parameters.Page_Capacity / SECTOR_SIZE_IN_BYTE, dcm);
    break;
  case HostInterface_Types::SATA:
    Host_interface = new SSD_Components::Host_Interface_SATA(ID() + ".HostInterface",
      params.IO_Queue_Depth, Utils::Logical_Address_Partitioning_Unit::Get_total_device_lha_count(), params.Flash_Parameters.Page_Capacity / SECTOR_SIZE_IN_BYTE, dcm);

    break;
  default:
    break;
  }
  Simulator->AddObject(Host_interface);
  dcm->Set_host_interface(Host_interface);
}

SSD_Device::~SSD_Device()
{
  for (uint32_t channel_cntr = 0; channel_cntr < Channel_count; channel_cntr++)
  {
    for (uint32_t chip_cntr = 0; chip_cntr < Chip_no_per_channel; chip_cntr++)
      
      delete ((SSD_Components::ONFI_Channel_NVDDR2*)this->Channels[channel_cntr])->Chips[chip_cntr];
    delete this->Channels[channel_cntr];
  }

  delete this->PHY;
  delete ftl.TSU;
  delete ftl.BlockManager;
  delete ftl.Address_Mapping_Unit;
  delete ftl.GC_and_WL_Unit;
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
    ftl.Perform_precondition(workload_stats);
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
    ftl.Report_results_in_XML(ID(), xmlwriter);
    ftl.TSU->Report_results_in_XML(ID(), xmlwriter);

    for (uint32_t channel_cntr = 0; channel_cntr < Channel_count; channel_cntr++)
      for (uint32_t chip_cntr = 0; chip_cntr < Chip_no_per_channel; chip_cntr++)
        ((SSD_Components::ONFI_Channel_NVDDR2*)Channels[channel_cntr])->Chips[chip_cntr]->Report_results_in_XML(ID(), xmlwriter);
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
  return ftl.Convert_host_logical_address_to_device_address(lha);
}

page_status_type
SSD_Device::__find_nvm_subunit_access_bitmap(LHA_type lha) const
{
  return ftl.Find_NVM_subunit_access_bitmap(lha);
}
