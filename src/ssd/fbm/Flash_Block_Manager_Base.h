#ifndef BLOCK_POOL_MANAGER_BASE_H
#define BLOCK_POOL_MANAGER_BASE_H

#include <list>
#include <cstdint>
#include <queue>
#include <set>
#include <sstream>
#include "../../nvm_chip/flash_memory/FlashTypes.h"
#include "../../nvm_chip/flash_memory/Physical_Page_Address.h"
#include "../gc_and_wl/GC_and_WL_Unit_Base.h"
#include "../Stats.h"

namespace SSD_Components
{
  class GC_and_WL_Unit_Base;
  /*
  * Block_Service_Status is used to impelement a state machine for each physical block in order to
  * eliminate race conditions between GC page movements and normal user I/O requests.
  * Allowed transitions:
  * 1: IDLE -> GC, IDLE -> USER
  * 2: GC -> IDLE, GC -> GC_UWAIT
  * 3: USER -> IDLE, USER -> GC_USER
  * 4: GC_UWAIT -> GC, GC_UWAIT -> GC_UWAIT
  * 5: GC_USER -> GC
  */
  enum class Block_Service_Status {IDLE, GC_WL, USER, GC_USER, GC_UWAIT, GC_USER_UWAIT};

  // =====================================
  // Block_Pool_Slot_Type Class Definition
  // =====================================
  class Block_Pool_Slot_Type
  {
  private:
    static constexpr uint64_t ALL_VALID_PAGE = 0x0000000000000000ULL;
    static constexpr uint32_t BITMAP_SIZE = (sizeof(uint64_t) * 8);

  public:
    const flash_block_ID_type BlockID;
    flash_page_ID_type Current_page_write_index;
    Block_Service_Status Current_status;
    uint32_t Erase_count;

  private:
    uint32_t __page_vector_size;

  public:
    uint32_t Invalid_page_count;

    // A bit sequence that keeps track of valid/invalid status of pages in the
    // block. A "0" means valid, and a "1" means invalid.
    std::vector<uint64_t> Invalid_page_bitmap;

    stream_id_type Stream_id = NO_STREAM;
    bool Holds_mapping_data = false;
    bool Has_ongoing_gc_wl = false;
    NVM_Transaction_Flash_ER* Erase_transaction;
    bool Hot_block = false;//Used for hot/cold separation mentioned in the "On the necessity of hot and cold data identification to reduce the write amplification in flash-based SSDs", Perf. Eval., 2014.
    int Ongoing_user_program_count;
    int Ongoing_user_read_count;

    Block_Pool_Slot_Type(uint32_t block_id, uint32_t pages_no_per_block);
    void Erase();
  };

  typedef std::vector<Block_Pool_Slot_Type>  BlockPoolSlotList;
  typedef std::vector<Block_Pool_Slot_Type*> BlockPoolSlotPtrList;

  force_inline void
  Block_Pool_Slot_Type::Erase()
  {
    Current_page_write_index = 0;
    Invalid_page_count = 0;
    Erase_count++;
    for (uint32_t i = 0; i < __page_vector_size; i++)
      Invalid_page_bitmap[i] = ALL_VALID_PAGE;
    Stream_id = NO_STREAM;
    Holds_mapping_data = false;
    Erase_transaction = nullptr;
  }

  // =====================================
  // PlaneBookKeepingType Class Definition
  // =====================================
  class PlaneBookKeepingType
  {
    typedef std::multimap<uint32_t, Block_Pool_Slot_Type*> FreeBlockPoolMap;
    typedef std::pair<uint32_t, Block_Pool_Slot_Type*>     FreeBlockPoolPair;

  public:
    uint32_t Total_pages_count;
    uint32_t Free_pages_count;
    uint32_t Valid_pages_count;
    uint32_t Invalid_pages_count;

    BlockPoolSlotList Blocks;
    // Block_Pool_Slot_Type* Blocks;

    FreeBlockPoolMap Free_block_pool;

    uint32_t __tmp__block_no_per_plane;

    BlockPoolSlotPtrList Data_wf;

    // The write frontier blocks for data and GC pages. MQSim adopts Double
    // Write Frontier approach for user and GC writes which is shown very
    // advantages in: B. Van Houdt, "On the necessity of hot and cold data
    // identification to reduce the write amplification in flash - based SSDs",
    // Perf. Eval., 2014
    BlockPoolSlotPtrList GC_wf;

    //The write frontier blocks for translation GC pages
    BlockPoolSlotPtrList Translation_wf;

    // A fifo queue that keeps track of flash blocks based on their usage
    // history
    std::queue<flash_block_ID_type> Block_usage_history;
    std::set<flash_block_ID_type> Ongoing_erase_operations;

  public:
    PlaneBookKeepingType(uint32_t total_concurrent_streams_no,
                         uint32_t block_no_per_plane,
                         uint32_t pages_no_per_block);

    Block_Pool_Slot_Type* Get_a_free_block(stream_id_type stream_id, bool for_mapping_data);
    uint32_t Get_free_block_pool_size() const;
    void Check_bookkeeping_correctness(const NVM::FlashMemory::Physical_Page_Address& plane_address);
    void Add_to_free_block_pool(Block_Pool_Slot_Type& block, bool consider_dynamic_wl);
  };

  typedef std::vector<PlaneBookKeepingType>      PlaneBookKeepingOnDie;
  typedef std::vector<PlaneBookKeepingOnDie>     PlaneBookKeepingOnChip;
  typedef std::vector<PlaneBookKeepingOnChip>    PlaneBookKeepingOnChannel;
  typedef std::vector<PlaneBookKeepingOnChannel> PlaneBookKeepingOnManager;

  force_inline Block_Pool_Slot_Type*
  PlaneBookKeepingType::Get_a_free_block(stream_id_type stream_id,
                                         bool for_mapping_data)
  {
    Block_Pool_Slot_Type* new_block = nullptr;

    // Assign a new write frontier block
    new_block = (*Free_block_pool.begin()).second;

    if (Free_block_pool.empty())
      throw mqsim_error("Requesting a free block from an empty pool!");

    Free_block_pool.erase(Free_block_pool.begin());
    new_block->Stream_id = stream_id;
    new_block->Holds_mapping_data = for_mapping_data;
    Block_usage_history.push(new_block->BlockID);

    return new_block;
  }

  force_inline uint32_t
  PlaneBookKeepingType::Get_free_block_pool_size() const
  {
    return uint32_t(Free_block_pool.size());
  }

  force_inline void
  PlaneBookKeepingType::Check_bookkeeping_correctness(const NVM::FlashMemory::Physical_Page_Address& plane_address)
  {
    // TODO Need to change test-only mode
    if (Total_pages_count != Free_pages_count
                             + Valid_pages_count + Invalid_pages_count) {
      throw mqsim_error("Inconsistent status in the plane bookkeeping record!");
    }

    if (Free_pages_count == 0) {
      std::stringstream errs("");

      errs << "Plane "
           << "@" << plane_address.ChannelID
           << "@" << plane_address.ChipID
           << "@" << plane_address.DieID
           << "@" << plane_address.PlaneID
           << " pool size: " << Get_free_block_pool_size()
           << " ran out of free pages! Bad resource management!"
           << " It is not safe to continue simulation!";

      throw mqsim_error(errs.str());
    }
  }

  force_inline void
  PlaneBookKeepingType::Add_to_free_block_pool(Block_Pool_Slot_Type& block,
                                               bool consider_dynamic_wl)
  {
    Free_block_pool.insert(
      FreeBlockPoolPair(consider_dynamic_wl ? block.Erase_count : 0, &block)
    );
  }

  // =========================================
  // Flash_Block_Manager_Base Class Definition
  // =========================================
  class Flash_Block_Manager_Base
  {
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
    virtual uint32_t Get_pool_size(const NVM::FlashMemory::Physical_Page_Address& plane_address) = 0;
    flash_block_ID_type Get_coldest_block_id(const NVM::FlashMemory::Physical_Page_Address& plane_address);
    uint32_t Get_min_max_erase_difference(const NVM::FlashMemory::Physical_Page_Address& plane_address);
    void Set_GC_and_WL_Unit(GC_and_WL_Unit_Base* );
    PlaneBookKeepingType* Get_plane_bookkeeping_entry(const NVM::FlashMemory::Physical_Page_Address& plane_address);
    bool Block_has_ongoing_gc_wl(const NVM::FlashMemory::Physical_Page_Address& block_address);//Checks if there is an ongoing gc for block_address
    bool Can_execute_gc_wl(const NVM::FlashMemory::Physical_Page_Address& block_address);//Checks if the gc request can be executed on block_address (there shouldn't be any ongoing user read/program requests targeting block_address)
    void GC_WL_started(const NVM::FlashMemory::Physical_Page_Address& block_address);//Updates the block bookkeeping record
    void GC_WL_finished(const NVM::FlashMemory::Physical_Page_Address& block_address);//Updates the block bookkeeping record
    void Read_transaction_issued(const NVM::FlashMemory::Physical_Page_Address& page_address);//Updates the block bookkeeping record
    void Read_transaction_serviced(const NVM::FlashMemory::Physical_Page_Address& page_address);//Updates the block bookkeeping record
    void Program_transaction_serviced(const NVM::FlashMemory::Physical_Page_Address& page_address);//Updates the block bookkeeping record
    bool Is_having_ongoing_program(const NVM::FlashMemory::Physical_Page_Address& block_address);//Cheks if block has any ongoing program request
    bool Is_page_valid(Block_Pool_Slot_Type* block, flash_page_ID_type page_id);//Make the page invalid in the block bookkeeping record
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
}

#endif//!BLOCK_POOL_MANAGER_BASE_H
