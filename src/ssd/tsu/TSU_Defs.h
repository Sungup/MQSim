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

#include "../../utils/InlineTools.h"

// ======================================
// Enumerators and its string converters.
// ======================================
namespace SSD_Components {
  enum class Flash_Scheduling_Type {OUT_OF_ORDER, FLIN};
}


force_inline std::string
to_string(SSD_Components::Flash_Scheduling_Type type)
{
  switch (type) {
    case SSD_Components::Flash_Scheduling_Type::OUT_OF_ORDER: return "OUT_OF_ORDER";
    case SSD_Components::Flash_Scheduling_Type::FLIN:         return "FLIN";
  }
}

force_inline SSD_Components::Flash_Scheduling_Type
to_flash_scheduling_type(std::string v)
{
  std::transform(v.begin(), v.end(), v.begin(), ::toupper);

  if (v == "OUT_OR_ORDER") return SSD_Components::Flash_Scheduling_Type::OUT_OF_ORDER;
  // Currently FLIN Scheduling type had been all commented out!
  // else if ("FLIN")         return SSD_Components::Flash_Scheduling_Type::FLIN;

  throw std::logic_error("Unknown transaction scheduling type specified in the SSD configuration file");
}

#endif /* Predefined include guard __MQSim__TSU_Defs__ */
