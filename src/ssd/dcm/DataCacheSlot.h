//
// DataCacheSlot
// MQSim
//
// Created by 文盛業 on 2019-07-05.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__DataCacheSlot__
#define __MQSim__DataCacheSlot__

#include <cstdint>
#include <list>

#include "../../sim/Sim_Defs.h"
#include "../../nvm_chip/flash_memory/FlashTypes.h"

#include "../SSD_Defs.h"

namespace SSD_Components {
  enum class Cache_Slot_Status {
    EMPTY,
    CLEAN,
    DIRTY_NO_FLASH_WRITEBACK,
    DIRTY_FLASH_WRITEBACK
  };

  struct Data_Cache_Slot_Type {
    uint64_t State_bitmap_of_existing_sectors;
    LPA_type LPA;
    data_cache_content_type Content;
    data_timestamp_type Timestamp;
    Cache_Slot_Status Status;

    //used for fast implementation of LRU
    std::list<std::pair<LPA_type, Data_Cache_Slot_Type*>>::iterator lru_list_ptr;
  };
}

#endif /* Predefined include guard __MQSim__DataCacheSlot__ */
