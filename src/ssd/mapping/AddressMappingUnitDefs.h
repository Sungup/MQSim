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

#include "../../utils/EnumTools.h"
#include "../../utils/InlineTools.h"
#include "../../utils/StringTools.h"

namespace SSD_Components {
  typedef uint32_t MVPN_type;
  typedef uint32_t MPPN_type;

  enum class Flash_Address_Mapping_Type {
    PAGE_LEVEL,
    HYBRID
  };

  enum class Flash_Plane_Allocation_Scheme_Type {
    CWDP, CWPD, CDWP, CDPW, CPWD, CPDW,
    WCDP, WCPD, WDCP, WDPC, WPCD, WPDC,
    DCWP, DCPW, DWCP, DWPC, DPCW, DPWC,
    PCWD, PCDW, PWCD, PWDC, PDCW, PDWC
  };

  enum class CMT_Sharing_Mode {
    SHARED,
    EQUAL_SIZE_PARTITIONING
  };

#if 0 // Not in use currently
  enum class Moving_LPA_Status {
    GC_IS_READING_PHYSICAL_BLOCK,
    GC_IS_READING_DATA,
    GC_IS_WRITING_DATA,
    GC_IS_READING_PHYSICAL_BLOCK_AND_THERE_IS_USER_READ,
    GC_IS_READING_DATA_AND_THERE_IS_USER_READ,
    GC_IS_READING_PHYSICAL_BLOCK_AND_PAGE_IS_INVALIDATED,
    GC_IS_READING_DATA_AND_PAGE_IS_INVALIDATED,
    GC_IS_WRITING_DATA_AND_PAGE_IS_INVALIDATED
  };
#endif
}

force_inline std::string
to_string(SSD_Components::Flash_Address_Mapping_Type type)
{
  using namespace SSD_Components;

  switch (type) {
  case ENUM_TO_STR(Flash_Address_Mapping_Type, PAGE_LEVEL);
  case ENUM_TO_STR(Flash_Address_Mapping_Type, HYBRID);
  }
}

force_inline SSD_Components::Flash_Address_Mapping_Type
to_flash_addr_mapping(std::string v)
{
  using namespace SSD_Components;

  Utils::to_upper(v);

  STR_TO_ENUM(Flash_Address_Mapping_Type, PAGE_LEVEL);
  STR_TO_ENUM(Flash_Address_Mapping_Type, HYBRID);

  throw mqsim_error("Unknown address mapping type specified in the SSD "
                    "configuration file");
}

force_inline std::string
to_string(SSD_Components::Flash_Plane_Allocation_Scheme_Type scheme)
{
  using namespace SSD_Components;

  switch (scheme) {
  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, CWDP);
  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, CWPD);
  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, CDWP);
  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, CDPW);
  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, CPWD);
  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, CPDW);

  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, WCDP);
  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, WCPD);
  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, WDCP);
  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, WDPC);
  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, WPCD);
  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, WPDC);

  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, DCWP);
  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, DCPW);
  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, DWCP);
  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, DWPC);
  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, DPCW);
  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, DPWC);

  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, PCWD);
  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, PCDW);
  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, PWCD);
  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, PWDC);
  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, PDCW);
  case ENUM_TO_STR(Flash_Plane_Allocation_Scheme_Type, PDWC);
  }
}

force_inline SSD_Components::Flash_Plane_Allocation_Scheme_Type
to_flash_plane_alloc_scheme(std::string v)
{
  using namespace SSD_Components;

  Utils::to_upper(v);

  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, CWDP);
  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, CWPD);
  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, CDWP);
  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, CDPW);
  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, CPWD);
  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, CPDW);

  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, WCDP);
  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, WCPD);
  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, WDCP);
  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, WDPC);
  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, WPCD);
  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, WPDC);

  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, DCWP);
  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, DCPW);
  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, DWCP);
  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, DWPC);
  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, DPCW);
  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, DPWC);

  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, PCWD);
  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, PCDW);
  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, PWCD);
  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, PWDC);
  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, PDCW);
  STR_TO_ENUM(Flash_Plane_Allocation_Scheme_Type, PDWC);

  throw mqsim_error("Unknown plane allocation scheme type specified in the SSD "
                    "configuration file");
}

force_inline std::string
to_string(SSD_Components::CMT_Sharing_Mode mode)
{
  using namespace SSD_Components;

  switch (mode) {
  case ENUM_TO_STR(CMT_Sharing_Mode, SHARED);
  case ENUM_TO_STR(CMT_Sharing_Mode, EQUAL_SIZE_PARTITIONING);
  }
}

force_inline SSD_Components::CMT_Sharing_Mode
to_cmt_sharing_mode(std::string v)
{
  using namespace SSD_Components;

  Utils::to_upper(v);

  STR_TO_ENUM(CMT_Sharing_Mode, SHARED);
  STR_TO_ENUM(CMT_Sharing_Mode, EQUAL_SIZE_PARTITIONING);

  throw mqsim_error("Unknown CMT sharing mode specified in the SSD "
                    "configuration file");
}

#endif /* Predefined include guard __MQSim__AddressMappingUnitDefs__ */
