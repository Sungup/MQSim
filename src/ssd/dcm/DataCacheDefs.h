//
// DataCacheDefs
// MQSim
//
// Created by 文盛業 on 2019-06-18.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__DataCacheDefs__
#define __MQSim__DataCacheDefs__

#include <string>

#include "../../utils/Exception.h"
#include "../../utils/InlineTools.h"
#include "../../utils/StringTools.h"

namespace SSD_Components {
  // ======================================
  // Enumerators and its string converters.
  // ======================================
  enum class Caching_Mode {
    WRITE_CACHE,
    READ_CACHE,
    WRITE_READ_CACHE,
    TURNED_OFF
  };

  enum class Caching_Mechanism {
    SIMPLE,
    ADVANCED
  };

  //How the cache space is shared among the concurrently running I/O flows/streams
  enum class Cache_Sharing_Mode {
    SHARED,//each application has access to the entire cache space
    EQUAL_PARTITIONING
  };
}

force_inline std::string
to_string(SSD_Components::Caching_Mode mode)
{
  switch (mode) {
  case SSD_Components::Caching_Mode::WRITE_CACHE:      return "WRITE_CACHE";
  case SSD_Components::Caching_Mode::READ_CACHE:       return "READ_CACHE";
  case SSD_Components::Caching_Mode::WRITE_READ_CACHE: return "WRITE_READ_CACHE";
  case SSD_Components::Caching_Mode::TURNED_OFF:       return "TURNED_OFF";
  }
}

force_inline SSD_Components::Caching_Mode
to_caching_mode(std::string v)
{
  Utils::to_upper(v);

  if (v == "WRITE_CACHE")      return SSD_Components::Caching_Mode::WRITE_CACHE;
  if (v == "READ_CACHE")       return SSD_Components::Caching_Mode::READ_CACHE;
  if (v == "WRITE_READ_CACHE") return SSD_Components::Caching_Mode::WRITE_READ_CACHE;
  if (v == "TURN_OFF")         return SSD_Components::Caching_Mode::TURNED_OFF;

  throw mqsim_error("Wrong caching mode definition for input flow");
}

#endif /* Predefined include guard __MQSim__DataCacheDefs__ */
