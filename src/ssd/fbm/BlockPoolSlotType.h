//
// BlockPoolSlotType
// MQSim
//
// Created by 文盛業 on 2019-07-04.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__BlockPoolSlotType__
#define __MQSim__BlockPoolSlotType__

#include <cstdint>

#include "../../nvm_chip/flash_memory/FlashTypes.h"

namespace SSD_Components {
  class NvmTransactionFlashER;

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
    NvmTransactionFlashER* Erase_transaction;
    bool Hot_block = false;//Used for hot/cold separation mentioned in the "On the necessity of hot and cold data identification to reduce the write amplification in flash-based SSDs", Perf. Eval., 2014.
    int Ongoing_user_program_count;
    int Ongoing_user_read_count;

  public:
    Block_Pool_Slot_Type(uint32_t block_id, uint32_t pages_no_per_block);

    void Erase();
  };

  typedef std::vector<Block_Pool_Slot_Type>  BlockPoolSlotList;
  typedef std::vector<Block_Pool_Slot_Type*> BlockPoolSlotPtrList;

  force_inline
  Block_Pool_Slot_Type::Block_Pool_Slot_Type(uint32_t block_id,
                                             uint32_t pages_no_per_block)
    : BlockID(block_id),
      Current_page_write_index(0),
      Current_status(Block_Service_Status::IDLE),
      Erase_count(0),
      __page_vector_size(pages_no_per_block
                         / BITMAP_SIZE
                         + (pages_no_per_block % BITMAP_SIZE == 0 ? 0 : 1)),
      Invalid_page_count(0),
#if (DEBUG || PROFILE)
    // Wrapping the ALL_VALID_PAGES because C++11 cannot replace constexpr
    // directly on -O0 optimization mode.
      Invalid_page_bitmap(__page_vector_size, uint64_t(ALL_VALID_PAGE)),
#else
    Invalid_page_bitmap(__page_vector_size, ALL_VALID_PAGE),
#endif
      Holds_mapping_data(false),
      Has_ongoing_gc_wl(false),
      Erase_transaction(nullptr),
      Hot_block(false),
      Ongoing_user_program_count(0),
      Ongoing_user_read_count(0)
  { }

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

}

#endif /* Predefined include guard __MQSim__BlockPoolSlotType__ */
