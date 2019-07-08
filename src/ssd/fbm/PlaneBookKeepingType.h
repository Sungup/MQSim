//
// PlaneBookKeepingType
// MQSim
//
// Created by 文盛業 on 2019-07-04.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__PlaneBookKeepingType__
#define __MQSim__PlaneBookKeepingType__

#include <map>
#include <set>
#include <queue>

#include "BlockPoolSlotType.h"

namespace SSD_Components {
  class PlaneBookKeepingType {
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

  force_inline
  PlaneBookKeepingType::PlaneBookKeepingType(uint32_t total_concurrent_streams_no,
                                             uint32_t block_no_per_plane,
                                             uint32_t pages_no_per_block)
    : Total_pages_count(block_no_per_plane * pages_no_per_block),
      Free_pages_count(Total_pages_count),
      Valid_pages_count(0),
      Invalid_pages_count(0),
      Blocks(),
      __tmp__block_no_per_plane(block_no_per_plane),
      Data_wf(total_concurrent_streams_no, nullptr),
      GC_wf(total_concurrent_streams_no, nullptr),
      Translation_wf(total_concurrent_streams_no, nullptr),
      Ongoing_erase_operations()
  {
    //Initialize block pool for plane
    Blocks.reserve(block_no_per_plane);

    for (uint32_t blockID = 0; blockID < block_no_per_plane; blockID++) {
      Blocks.emplace_back(blockID, pages_no_per_block);
      Add_to_free_block_pool(Blocks[blockID], false);
    }

    for (uint32_t cnt = 0; cnt < total_concurrent_streams_no; ++cnt) {
      Data_wf[cnt] = Get_a_free_block(cnt, false);
      Translation_wf[cnt] = Get_a_free_block(cnt, true);
      GC_wf[cnt] = Get_a_free_block(cnt, false);
    }
  }

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
#if RUN_EXCEPTION_CHECK
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
#endif
  }

  force_inline void
  PlaneBookKeepingType::Add_to_free_block_pool(Block_Pool_Slot_Type& block,
                                               bool consider_dynamic_wl)
  {
    Free_block_pool.insert(
      FreeBlockPoolPair(consider_dynamic_wl ? block.Erase_count : 0, &block)
                          );
  }

}

#endif /* Predefined include guard __MQSim__PlaneBookKeepingType__ */
