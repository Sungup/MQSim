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

      AddressInfo(uint16_t channel,
                  uint16_t chip,
                  uint16_t die,
                  uint16_t plane);
    };

  typedef AddressInfo (*AllocScheme)(LPA_type lpn, const AddressInfo& in);

  private:
    const ChannelIDs __channel_ids;
    const ChipIDs    __chip_ids;
    const DieIDs     __die_ids;
    const PlaneIDs   __plane_ids;

    const AddressInfo __structure;
    AllocScheme __alloc;

  public:
    PlaneAllocator(const ChannelIDs& channel_ids,
                   const ChipIDs&    chip_ids,
                   const DieIDs&     die_ids,
                   const PlaneIDs&   plane_ids,
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
