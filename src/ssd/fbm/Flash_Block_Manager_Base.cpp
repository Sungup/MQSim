#include "Flash_Block_Manager_Base.h"

// Children classes
#include "Flash_Block_Manager.h"

using namespace SSD_Components;

Flash_Block_Manager_Base::Flash_Block_Manager_Base(GC_and_WL_Unit_Base* gc_and_wl_unit,
                                                   Stats& stats,
                                                   uint32_t max_allowed_block_erase_count,
                                                   uint32_t total_concurrent_streams_no,
                                                   uint32_t channel_count,
                                                   uint32_t chip_no_per_channel,
                                                   uint32_t die_no_per_chip,
                                                   uint32_t plane_no_per_die,
                                                   uint32_t block_no_per_plane,
                                                   uint32_t page_no_per_block)
  : gc_and_wl_unit(gc_and_wl_unit),
    max_allowed_block_erase_count(max_allowed_block_erase_count),
    total_concurrent_streams_no(total_concurrent_streams_no),
    channel_count(channel_count),
    chip_no_per_channel(chip_no_per_channel),
    die_no_per_chip(die_no_per_chip),
    plane_no_per_die(plane_no_per_die),
    block_no_per_plane(block_no_per_plane),
    pages_no_per_block(page_no_per_block),
    __stats(stats),
    plane_manager(
      channel_count,
      PlaneBookKeepingOnChannel(
        chip_no_per_channel,
        PlaneBookKeepingOnChip(
          die_no_per_chip,
          PlaneBookKeepingOnDie()
        )
      )
    )
{
  for (uint32_t channelID = 0; channelID < channel_count; channelID++) {
    for (uint32_t chipID = 0; chipID < chip_no_per_channel; chipID++) {
      for (uint32_t dieID = 0; dieID < die_no_per_chip; dieID++) {
        auto& manager = plane_manager[channelID][chipID][dieID];

        manager.reserve(plane_no_per_die);

        // Initialize plane book keeping data structure
        for (uint32_t planeID = 0; planeID < plane_no_per_die; planeID++)
          manager.emplace_back(total_concurrent_streams_no,
                               block_no_per_plane,
                               page_no_per_block);
      }
    }
  }
}

void Flash_Block_Manager_Base::Set_GC_and_WL_Unit(GC_and_WL_Unit_Base* gcwl) { this->gc_and_wl_unit = gcwl; }

uint32_t Flash_Block_Manager_Base::Get_min_max_erase_difference(const NVM::FlashMemory::Physical_Page_Address& plane_address)
{
  uint32_t min_erased_block = 0;
  uint32_t max_erased_block = 0;
  PlaneBookKeepingType *plane_record = &plane_manager[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID];

  for (uint32_t i = 1; i < block_no_per_plane; i++)
  {
    if (plane_record->Blocks[i].Erase_count > plane_record->Blocks[max_erased_block].Erase_count)
      max_erased_block = i;
    if (plane_record->Blocks[i].Erase_count < plane_record->Blocks[min_erased_block].Erase_count)
      min_erased_block = i;
  }
  return max_erased_block - min_erased_block;
}

flash_block_ID_type Flash_Block_Manager_Base::Get_coldest_block_id(const NVM::FlashMemory::Physical_Page_Address& plane_address)
{
  uint32_t min_erased_block = 0;
  PlaneBookKeepingType *plane_record = &plane_manager[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID];

  for (uint32_t i = 1; i < block_no_per_plane; i++)
  {
    if (plane_record->Blocks[i].Erase_count < plane_record->Blocks[min_erased_block].Erase_count)
      min_erased_block = i;
  }
  return min_erased_block;
}

bool Flash_Block_Manager_Base::Block_has_ongoing_gc_wl(const NVM::FlashMemory::Physical_Page_Address& block_address)
{
  PlaneBookKeepingType *plane_record = &plane_manager[block_address.ChannelID][block_address.ChipID][block_address.DieID][block_address.PlaneID];
  return plane_record->Blocks[block_address.BlockID].Has_ongoing_gc_wl;
}

bool Flash_Block_Manager_Base::Can_execute_gc_wl(const NVM::FlashMemory::Physical_Page_Address& block_address)
{
  auto& block_record = Get_plane_bookkeeping_entry(block_address).Blocks[block_address.BlockID];
  return (block_record.Ongoing_user_program_count + block_record.Ongoing_user_read_count == 0);
}

void Flash_Block_Manager_Base::GC_WL_started(const NVM::FlashMemory::Physical_Page_Address& block_address)
{
  auto& block_record = Get_plane_bookkeeping_entry(block_address).Blocks[block_address.BlockID];
  block_record.Has_ongoing_gc_wl = true;
}

void Flash_Block_Manager_Base::program_transaction_issued(const NVM::FlashMemory::Physical_Page_Address& page_address)
{
  PlaneBookKeepingType *plane_record = &plane_manager[page_address.ChannelID][page_address.ChipID][page_address.DieID][page_address.PlaneID];
  plane_record->Blocks[page_address.BlockID].Ongoing_user_program_count++;
}

void Flash_Block_Manager_Base::Read_transaction_issued(const NVM::FlashMemory::Physical_Page_Address& page_address)
{
  PlaneBookKeepingType *plane_record = &plane_manager[page_address.ChannelID][page_address.ChipID][page_address.DieID][page_address.PlaneID];
  plane_record->Blocks[page_address.BlockID].Ongoing_user_read_count++;
}

void Flash_Block_Manager_Base::Program_transaction_serviced(const NVM::FlashMemory::Physical_Page_Address& page_address)
{
  PlaneBookKeepingType *plane_record = &plane_manager[page_address.ChannelID][page_address.ChipID][page_address.DieID][page_address.PlaneID];
  plane_record->Blocks[page_address.BlockID].Ongoing_user_program_count--;
}

void Flash_Block_Manager_Base::Read_transaction_serviced(const NVM::FlashMemory::Physical_Page_Address& page_address)
{
  PlaneBookKeepingType *plane_record = &plane_manager[page_address.ChannelID][page_address.ChipID][page_address.DieID][page_address.PlaneID];
  plane_record->Blocks[page_address.BlockID].Ongoing_user_read_count--;
}

bool Flash_Block_Manager_Base::Is_having_ongoing_program(const NVM::FlashMemory::Physical_Page_Address& block_address)
{
  PlaneBookKeepingType *plane_record = &plane_manager[block_address.ChannelID][block_address.ChipID][block_address.DieID][block_address.PlaneID];
  return plane_record->Blocks[block_address.BlockID].Ongoing_user_program_count > 0;
}

void Flash_Block_Manager_Base::GC_WL_finished(const NVM::FlashMemory::Physical_Page_Address& block_address)
{
  auto& block_record = Get_plane_bookkeeping_entry(block_address).Blocks[block_address.BlockID];
  block_record.Has_ongoing_gc_wl = false;
}

bool Flash_Block_Manager_Base::Is_page_valid(const Block_Pool_Slot_Type& block, flash_page_ID_type page_id)
{
  return ((block.Invalid_page_bitmap[page_id / 64] & (((uint64_t)1) << page_id)) == 0);
}

FlashBlockManagerPtr
SSD_Components::build_fbm_object(const DeviceParameterSet& params,
                                 uint32_t concurrent_stream_count,
                                 Stats& stats)
{
  return std::make_shared<Flash_Block_Manager>(nullptr,
                                               stats,
                                               params.Flash_Parameters.Block_PE_Cycles_Limit,
                                               concurrent_stream_count,
                                               params.Flash_Channel_Count,
                                               params.Chip_No_Per_Channel,
                                               params.Flash_Parameters.Die_No_Per_Chip,
                                               params.Flash_Parameters.Plane_No_Per_Die,
                                               params.Flash_Parameters.Block_No_Per_Plane,
                                               params.Flash_Parameters.Page_No_Per_Block);
}
