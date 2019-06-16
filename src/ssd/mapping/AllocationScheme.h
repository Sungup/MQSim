//
// AllocationScheme
// MQSim
//
// Created by 文盛業 on 2019-06-13.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__AllocationScheme__
#define __MQSim__AllocationScheme__

#include <vector>

#include "../../utils/InlineTools.h"
#include "../../nvm_chip/flash_memory/FlashTypes.h"
#include "../../nvm_chip/flash_memory/Physical_Page_Address.h"

#include "AddressMappingUnitDefs.h"

namespace SSD_Components
{
  // =======================
  // Plane Allocator Functor
  // =======================
  class PlaneAllocator {
  public:
    struct AddressInfo {
      uint16_t channel;
      uint16_t chip;
      uint16_t die;
      uint16_t plane;
    };

  typedef AddressInfo (*AllocScheme)(LPA_type lpn, const AddressInfo& in);

  private:
    std::vector<flash_channel_ID_type> __channel_ids;
    std::vector<flash_chip_ID_type>    __chip_ids;
    std::vector<flash_die_ID_type>     __die_ids;
    std::vector<flash_plane_ID_type>   __plane_ids;

    const AddressInfo __structure;
    AllocScheme __alloc;

  public:
    PlaneAllocator(flash_channel_ID_type* channel_ids,
                   flash_chip_ID_type*    chip_ids,
                   flash_die_ID_type*     die_ids,
                   flash_plane_ID_type*   plane_ids,
                   uint16_t channel_no,
                   uint16_t chip_no,
                   uint16_t die_no,
                   uint16_t plane_no,
                   Flash_Plane_Allocation_Scheme_Type scheme);

    void operator()(NVM::FlashMemory::Physical_Page_Address& target,
                    LPA_type lpn);
  };

  force_inline void
  PlaneAllocator::operator()(NVM::FlashMemory::Physical_Page_Address& target,
                             LPA_type lpn)
  {
    AddressInfo idx = __alloc(lpn, __structure);

    target.ChannelID = __channel_ids[idx.channel];
    target.ChipID    = __chip_ids[idx.chip];
    target.DieID     = __die_ids[idx.die];
    target.PlaneID   = __plane_ids[idx.plane];
  }
}

#endif /* Predefined include guard __MQSim__AllocationScheme__ */
