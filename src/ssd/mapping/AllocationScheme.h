//
// AllocationScheme
// MQSim
//
// Created by 文盛業 on 2019-06-13.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__AllocationScheme__
#define __MQSim__AllocationScheme__

#include "../../nvm_chip/flash_memory/FlashTypes.h"
#include "../../nvm_chip/flash_memory/Physical_Page_Address.h"
#include "../Address_Mapping_Unit_Page_Level.h"

namespace SSD_Components
{
  force_inline void
  allocate_addr(NVM::FlashMemory::Physical_Page_Address& target,
                const AddressMappingDomain& domain,
                uint32_t channel_idx,
                uint32_t chip_idx,
                uint32_t die_idx,
                uint32_t plane_idx)
  {
    target.ChannelID = domain.Channel_ids[channel_idx];
    target.ChipID    = domain.Chip_ids[chip_idx];
    target.DieID     = domain.Die_ids[die_idx];
    target.PlaneID   = domain.Plane_ids[plane_idx];
  }

  force_inline uint32_t
  idx(LPA_type lpn, uint32_t div, uint32_t mod)
  {
    return (lpn / div) % mod;
  }

  // =============
  // Channel First
  // =============
  force_inline void
  CWDP_allocation(NVM::FlashMemory::Physical_Page_Address& target,
                  const AddressMappingDomain& dom, LPA_type lpn)
  {

    allocate_addr(
      target,
      dom,
      idx(lpn, 1,                                           dom.Channel_no),
      idx(lpn, dom.Channel_no,                              dom.Chip_no),
      idx(lpn, dom.Channel_no * dom.Chip_no,                dom.Die_no),
      idx(lpn, dom.Channel_no * dom.Chip_no * dom.Die_no,   dom.Plane_no)
    );
  }

  force_inline void
  CWPD_allocation(NVM::FlashMemory::Physical_Page_Address& target,
                  const AddressMappingDomain& dom, LPA_type lpn)
  {

    allocate_addr(
      target,
      dom,
      idx(lpn, 1,                                           dom.Channel_no),
      idx(lpn, dom.Channel_no,                              dom.Chip_no),
      idx(lpn, dom.Channel_no * dom.Chip_no * dom.Plane_no, dom.Die_no),
      idx(lpn, dom.Channel_no * dom.Chip_no,                dom.Plane_no)
    );
  }

  force_inline void
  CDWP_allocation(NVM::FlashMemory::Physical_Page_Address& target,
                  const AddressMappingDomain& dom, LPA_type lpn)
  {

    allocate_addr(
      target,
      dom,
      idx(lpn, 1,                                           dom.Channel_no),
      idx(lpn, dom.Channel_no * dom.Die_no,                 dom.Chip_no),
      idx(lpn, dom.Channel_no,                              dom.Die_no),
      idx(lpn, dom.Channel_no * dom.Chip_no * dom.Die_no,   dom.Plane_no)
    );
  }

  force_inline void
  CDPW_allocation(NVM::FlashMemory::Physical_Page_Address& target,
                  const AddressMappingDomain& dom, LPA_type lpn)
  {

    allocate_addr(
      target,
      dom,
      idx(lpn, 1,                                           dom.Channel_no),
      idx(lpn, dom.Channel_no * dom.Die_no * dom.Plane_no,  dom.Chip_no),
      idx(lpn, dom.Channel_no,                              dom.Die_no),
      idx(lpn, dom.Channel_no * dom.Die_no,                 dom.Plane_no)
    );
  }

  force_inline void
  CPWD_allocation(NVM::FlashMemory::Physical_Page_Address& target,
                  const AddressMappingDomain& dom, LPA_type lpn)
  {

    allocate_addr(
      target,
      dom,
      idx(lpn, 1,                                           dom.Channel_no),
      idx(lpn, dom.Channel_no * dom.Plane_no,               dom.Chip_no),
      idx(lpn, dom.Channel_no * dom.Chip_no * dom.Plane_no, dom.Die_no),
      idx(lpn, dom.Channel_no,                              dom.Plane_no)
    );
  }

  force_inline void
  CPDW_allocation(NVM::FlashMemory::Physical_Page_Address& target,
                  const AddressMappingDomain& dom, LPA_type lpn)
  {

    allocate_addr(
      target,
      dom,
      idx(lpn, 1,                                           dom.Channel_no),
      idx(lpn, dom.Channel_no * dom.Die_no * dom.Plane_no,  dom.Chip_no),
      idx(lpn, dom.Channel_no * dom.Plane_no,               dom.Die_no),
      idx(lpn, dom.Channel_no,                              dom.Plane_no)
    );
  }

  // =========
  // Way First
  // =========
  force_inline void
  WCDP_allocation(NVM::FlashMemory::Physical_Page_Address& target,
                  const AddressMappingDomain& dom, LPA_type lpn)
  {

    allocate_addr(
      target,
      dom,
      idx(lpn, dom.Chip_no,                                 dom.Channel_no),
      idx(lpn, 1,                                           dom.Chip_no),
      idx(lpn, dom.Channel_no * dom.Chip_no,                dom.Die_no),
      idx(lpn, dom.Channel_no * dom.Chip_no * dom.Die_no,   dom.Plane_no)
    );
  }

  force_inline void
  WCPD_allocation(NVM::FlashMemory::Physical_Page_Address& target,
                  const AddressMappingDomain& dom, LPA_type lpn)
  {

    allocate_addr(
      target,
      dom,
      idx(lpn, dom.Chip_no,                                 dom.Channel_no),
      idx(lpn, 1,                                           dom.Chip_no),
      idx(lpn, dom.Channel_no * dom.Chip_no * dom.Plane_no, dom.Die_no),
      idx(lpn, dom.Channel_no * dom.Chip_no,                dom.Plane_no)
    );
  }

  force_inline void
  WDCP_allocation(NVM::FlashMemory::Physical_Page_Address& target,
                  const AddressMappingDomain& dom, LPA_type lpn)
  {

    allocate_addr(
      target,
      dom,
      idx(lpn, dom.Chip_no * dom.Die_no,                    dom.Channel_no),
      idx(lpn, 1,                                           dom.Chip_no),
      idx(lpn, dom.Chip_no,                                 dom.Die_no),
      idx(lpn, dom.Channel_no * dom.Chip_no * dom.Die_no,   dom.Plane_no)
    );
  }

}

#endif /* Predefined include guard __MQSim__AllocationScheme__ */
