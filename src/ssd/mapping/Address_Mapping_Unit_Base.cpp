#include "../FTL.h"
#include "Address_Mapping_Unit_Base.h"
#include "../phy/NVM_PHY_ONFI_NVDDR2.h"
#include "../fbm/Flash_Block_Manager_Base.h"
#include "AddressMappingUnitDefs.h"

// Children classes
#include "Address_Mapping_Unit_Page_Level.h"
#include "Address_Mapping_Unit_Hybrid.h"

using namespace SSD_Components;

Address_Mapping_Unit_Base::Address_Mapping_Unit_Base(const sim_object_id_type& id, FTL* ftl, NVM_PHY_ONFI* flash_controller, Flash_Block_Manager_Base* block_manager,
  Stats& stats, bool ideal_mapping_table, uint32_t no_of_input_streams,
  uint32_t ChannelCount, uint32_t chip_no_per_channel, uint32_t DieNoPerChip, uint32_t PlaneNoPerDie,
  uint32_t Block_no_per_plane, uint32_t Page_no_per_block, uint32_t SectorsPerPage, uint32_t PageSizeInBytes,
  double Overprovisioning_ratio, CMT_Sharing_Mode sharing_mode, bool fold_large_addresses)
  : Sim_Object(id), ftl(ftl), flash_controller(flash_controller), block_manager(block_manager),
  ideal_mapping_table(ideal_mapping_table), no_of_input_streams(no_of_input_streams),
  channel_count(ChannelCount), chip_no_per_channel(chip_no_per_channel), die_no_per_chip(DieNoPerChip), plane_no_per_die(PlaneNoPerDie),
  block_no_per_plane(Block_no_per_plane), pages_no_per_block(Page_no_per_block), sector_no_per_page(SectorsPerPage), page_size_in_byte(PageSizeInBytes),
  overprovisioning_ratio(Overprovisioning_ratio), sharing_mode(sharing_mode), fold_large_addresses(fold_large_addresses),
  mapping_table_stored_on_flash(false),
    __stats(stats)
{
  page_no_per_plane = pages_no_per_block * block_no_per_plane;
  page_no_per_die = page_no_per_plane * plane_no_per_die;
  page_no_per_chip = page_no_per_die * die_no_per_chip;
  page_no_per_channel = page_no_per_chip * chip_no_per_channel;
  total_physical_pages_no = page_no_per_channel * ChannelCount;
  total_logical_pages_no = (uint32_t)((double)total_physical_pages_no * (1 - overprovisioning_ratio));
  max_logical_sector_address = (LHA_type)(SectorsPerPage * total_logical_pages_no - 1);
}

uint32_t Address_Mapping_Unit_Base::Get_device_physical_pages_count()
{
  return total_physical_pages_no;
}

bool Address_Mapping_Unit_Base::Is_ideal_mapping_table()
{
  return ideal_mapping_table;
}

CMT_Sharing_Mode Address_Mapping_Unit_Base::Get_CMT_sharing_mode()
{
  return sharing_mode;
}

// -----------
// AMU builder
// -----------
AddressMappingUnitPtr
SSD_Components::build_amu_object(const DeviceParameterSet& params,
                                 const Utils::LogicalAddrPartition& lapu,
                                 const StreamIdInfo& stream_info,
                                 FTL& ftl,
                                 NVM_PHY_ONFI& phy,
                                 Flash_Block_Manager_Base& fbm,
                                 Stats& stats)
{
  switch (params.Address_Mapping) {
  case SSD_Components::Flash_Address_Mapping_Type::PAGE_LEVEL:
    return std::make_shared<Address_Mapping_Unit_Page_Level>(
      ftl.ID() + ".AddressMappingUnit",
      &ftl,
      &phy,
      &fbm,
      lapu,
      stats,
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
      params.CMT_Sharing_Mode
    );

  case SSD_Components::Flash_Address_Mapping_Type::HYBRID:
    return std::make_shared<Address_Mapping_Unit_Hybrid>(
      ftl.ID() + ".AddressMappingUnit",
      &ftl,
      &phy,
      &fbm,
      stats,
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
      params.Overprovisioning_Ratio
    );
  }
}
