//
// TSU_Defs
// MQSim
//
// Created by 文盛業 on 2019-06-12.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__TSU_Defs__
#define __MQSim__TSU_Defs__

#include <string>

#include "../../utils/Exception.h"
#include "../../utils/EnumTools.h"
#include "../../utils/InlineTools.h"
#include "../../utils/StringTools.h"

// ======================================
// Enumerators and its string converters.
// ======================================
namespace SSD_Components {
  enum class Flash_Scheduling_Type {OUT_OF_ORDER, FLIN};
}


force_inline std::string
to_string(SSD_Components::Flash_Scheduling_Type type)
{
  using namespace SSD_Components;

  switch (type) {
  case ENUM_TO_STR(Flash_Scheduling_Type, OUT_OF_ORDER);
  case ENUM_TO_STR(Flash_Scheduling_Type, FLIN);
  }
}

force_inline SSD_Components::Flash_Scheduling_Type
to_scheduling_policy(std::string v)
{
  using namespace SSD_Components;

  Utils::to_upper(v);

  STR_TO_ENUM(Flash_Scheduling_Type, OUT_OF_ORDER);
  // Currently FLIN Scheduling type had been all commented out!
  // STR_TO_ENUM(Flash_Scheduling_Type, FLIN);

  throw mqsim_error("Unknown transaction scheduling type specified in the SSD "
                    "configuration file");
}

#endif /* Predefined include guard __MQSim__TSU_Defs__ */
