//
// ONFIChannelDefs
// MQSim
//
// Created by 文盛業 on 2019-06-28.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__ONFIChannelDefs__
#define __MQSim__ONFIChannelDefs__

#include "../utils/EnumTools.h"
#include "../utils/InlineTools.h"
#include "../utils/StringTools.h"

namespace SSD_Components
{
  enum class ONFI_Protocol {
    NVDDR2
  };
}

force_inline std::string
to_string(SSD_Components::ONFI_Protocol /* protocol */)
{
  // Currently only support Flash memory
  return "NVDDR2";
}

force_inline SSD_Components::ONFI_Protocol
to_onfi_protocol(std::string v)
{
  Utils::to_upper(v);

  // Currently only support Flash memory
  if (v != "NVDDR2")
    throw mqsim_error("Unknown flash communication protocol type specified in "
                      "the SSD configuration file");

  return SSD_Components::ONFI_Protocol::NVDDR2;
}

#endif /* Predefined include guard __MQSim__ONFIChannelDefs__ */
