#ifndef BLOCK_POOL_MANAGER_BASE_H
#define BLOCK_POOL_MANAGER_BASE_H

#include <cstdint>
#include <memory>

#include "../../nvm_chip/flash_memory/Physical_Page_Address.h"
#include "../Stats.h"

#include "BlockPoolSlotType.h"
#include "PlaneBookKeepingType.h"

namespace SSD_Components
{
  class GC_and_WL_Unit_Base;

  class Flash_Block_Manager_Base {
    friend class Address_Mapping_Unit_Page_Level;
    friend class GC_and_WL_Unit_Page_Level;
    friend class GC_and_WL_Unit_Base;
  public:
    Flash_Block_Manager_Base(GC_and_WL_Unit_Base* gc_and_wl_unit, Stats& stats, uint32_t max_allowed_block_erase_count, uint32_t total_concurrent_streams_no,
      uint32_t channel_count, uint32_t chip_no_per_channel, uint32_t die_no_per_chip, uint32_t plane_no_per_die,
      uint32_t block_no_per_plane, uint32_t page_no_per_block);
    virtual ~Flash_Block_Manager_Base() = default;
    virtual void Allocate_block_and_page_in_plane_for_user_write(const stream_id_type streamID, NVM::FlashMemory::Physical_Page_Address& address) = 0;
    virtual void Allocate_block_and_page_in_plane_for_gc_write(const stream_id_type streamID, NVM::FlashMemory::Physical_Page_Address& address) = 0;
    virtual void Allocate_block_and_page_in_plane_for_translation_write(const stream_id_type streamID, NVM::FlashMemory::Physical_Page_Address& address, bool is_for_gc) = 0;
    virtual void Allocate_Pages_in_block_and_invalidate_remaining_for_preconditioning(const stream_id_type stream_id, const NVM::FlashMemory::Physical_Page_Address& plane_address, std::vector<NVM::FlashMemory::Physical_Page_Address>& page_addresses) = 0;
    virtual void Invalidate_page_in_block(const stream_id_type streamID, const NVM::FlashMemory::Physical_Page_Address& address) = 0;
    virtual void Invalidate_page_in_block_for_preconditioning(const stream_id_type streamID, const NVM::FlashMemory::Physical_Page_Address& address) = 0;
    virtual void Add_erased_block_to_pool(const NVM::FlashMemory::Physical_Page_Address& address) = 0;
    virtual uint32_t Get_pool_size(const NVM::FlashMemory::Physical_Page_Address& plane_address) const = 0;
    flash_block_ID_type Get_coldest_block_id(const NVM::FlashMemory::Physical_Page_Address& plane_address);
    uint32_t Get_min_max_erase_difference(const NVM::FlashMemory::Physical_Page_Address& plane_address);
    void Set_GC_and_WL_Unit(GC_and_WL_Unit_Base* );
    PlaneBookKeepingType& Get_plane_bookkeeping_entry(const NVM::FlashMemory::Physical_Page_Address& ppa);
    bool Block_has_ongoing_gc_wl(const NVM::FlashMemory::Physical_Page_Address& block_address);//Checks if there is an ongoing gc for block_address
    bool Can_execute_gc_wl(const NVM::FlashMemory::Physical_Page_Address& block_address);//Checks if the gc request can be executed on block_address (there shouldn't be any ongoing user read/program requests targeting block_address)
    void GC_WL_started(const NVM::FlashMemory::Physical_Page_Address& block_address);//Updates the block bookkeeping record
    void GC_WL_finished(const NVM::FlashMemory::Physical_Page_Address& block_address);//Updates the block bookkeeping record
    void Read_transaction_issued(const NVM::FlashMemory::Physical_Page_Address& page_address);//Updates the block bookkeeping record
    void Read_transaction_serviced(const NVM::FlashMemory::Physical_Page_Address& page_address);//Updates the block bookkeeping record
    void Program_transaction_serviced(const NVM::FlashMemory::Physical_Page_Address& page_address);//Updates the block bookkeeping record
    bool Is_having_ongoing_program(const NVM::FlashMemory::Physical_Page_Address& block_address);//Cheks if block has any ongoing program request
    bool Is_page_valid(const Block_Pool_Slot_Type& block, flash_page_ID_type page_id);//Make the page invalid in the block bookkeeping record
  protected:
    // Keeps track of plane block usage information
    // .... PlaneBookKeepingType ****plane_manager;
    GC_and_WL_Unit_Base *gc_and_wl_unit;
    uint32_t max_allowed_block_erase_count;
    uint32_t total_concurrent_streams_no;
    uint32_t channel_count;
    uint32_t chip_no_per_channel;
    uint32_t die_no_per_chip;
    uint32_t plane_no_per_die;
    uint32_t block_no_per_plane;
    uint32_t pages_no_per_block;
    Stats& __stats;
    PlaneBookKeepingOnManager plane_manager;

    void program_transaction_issued(const NVM::FlashMemory::Physical_Page_Address& page_address);//Updates the block bookkeeping record
  };

  typedef std::shared_ptr<Flash_Block_Manager_Base> FlashBlockManagerPtr;

  force_inline PlaneBookKeepingType&
  Flash_Block_Manager_Base::Get_plane_bookkeeping_entry(const NVM::FlashMemory::Physical_Page_Address& ppa)
  {
    return plane_manager[ppa.ChannelID][ppa.ChipID][ppa.DieID][ppa.PlaneID];
  }

  FlashBlockManagerPtr build_fbm_object(const DeviceParameterSet& params,
                                        uint32_t concurrent_stream_count,
                                        Stats& stats);
}

#endif//!BLOCK_POOL_MANAGER_BASE_H
