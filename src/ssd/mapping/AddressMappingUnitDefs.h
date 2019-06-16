//
// AddressMappingUnitDefs
// MQSim
//
// Created by 文盛業 on 2019-06-16.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__AddressMappingUnitDefs__
#define __MQSim__AddressMappingUnitDefs__

#include <cstdint>

namespace SSD_Components {
  typedef uint32_t MVPN_type;
  typedef uint32_t MPPN_type;
  enum class Flash_Address_Mapping_Type {PAGE_LEVEL, HYBRID};
  enum class Flash_Plane_Allocation_Scheme_Type
  {
    CWDP, CWPD, CDWP, CDPW, CPWD, CPDW,
    WCDP, WCPD, WDCP, WDPC, WPCD, WPDC,
    DCWP, DCPW, DWCP, DWPC, DPCW, DPWC,
    PCWD, PCDW, PWCD, PWDC, PDCW, PDWC
  };
  enum class CMT_Sharing_Mode { SHARED, EQUAL_SIZE_PARTITIONING };
  enum class Moving_LPA_Status { GC_IS_READING_PHYSICAL_BLOCK, GC_IS_READING_DATA, GC_IS_WRITING_DATA,
    GC_IS_READING_PHYSICAL_BLOCK_AND_THERE_IS_USER_READ, GC_IS_READING_DATA_AND_THERE_IS_USER_READ,
    GC_IS_READING_PHYSICAL_BLOCK_AND_PAGE_IS_INVALIDATED, GC_IS_READING_DATA_AND_PAGE_IS_INVALIDATED, GC_IS_WRITING_DATA_AND_PAGE_IS_INVALIDATED};
}

#endif /* Predefined include guard __MQSim__AddressMappingUnitDefs__ */
