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

  // How the cache space is shared among the concurrently running I/O
  // flows/streams
  enum class Cache_Sharing_Mode {
    SHARED,//each application has access to the entire cache space
    EQUAL_PARTITIONING
  };
}

force_inline std::string
to_string(SSD_Components::Caching_Mode mode)
{
  namespace sc = SSD_Components;

  switch (mode) {
  case sc::Caching_Mode::WRITE_CACHE:      return "WRITE_CACHE";
  case sc::Caching_Mode::READ_CACHE:       return "READ_CACHE";
  case sc::Caching_Mode::WRITE_READ_CACHE: return "WRITE_READ_CACHE";
  case sc::Caching_Mode::TURNED_OFF:       return "TURNED_OFF";
  }
}

force_inline std::string
to_string(SSD_Components::Caching_Mechanism mechanism)
{
  namespace sc = SSD_Components;

  switch (mechanism) {
  case sc::Caching_Mechanism::SIMPLE:   return "SIMPLE";
  case sc::Caching_Mechanism::ADVANCED: return "ADVANCED";
  }
}

force_inline std::string
to_string(SSD_Components::Cache_Sharing_Mode mode)
{
  namespace sc = SSD_Components;

  switch (mode) {
  case sc::Cache_Sharing_Mode::SHARED:             return "SHARED";
  case sc::Cache_Sharing_Mode::EQUAL_PARTITIONING: return "EQUAL_PARTITIONING";
  }
}

force_inline SSD_Components::Caching_Mode
to_caching_mode(std::string v)
{
  namespace sc = SSD_Components;

  Utils::to_upper(v);

  if (v == "WRITE_CACHE")      return sc::Caching_Mode::WRITE_CACHE;
  if (v == "READ_CACHE")       return sc::Caching_Mode::READ_CACHE;
  if (v == "WRITE_READ_CACHE") return sc::Caching_Mode::WRITE_READ_CACHE;
  if (v == "TURN_OFF")         return sc::Caching_Mode::TURNED_OFF;

  throw mqsim_error("Wrong caching mode definition for input flow");
}

force_inline SSD_Components::Caching_Mechanism
to_cacheing_mechanism(std::string v)
{
  namespace sc = SSD_Components;

  Utils::to_upper(v);

  if (v == "SIMPLE")   return sc::Caching_Mechanism::SIMPLE;
  if (v == "ADVANCED") return sc::Caching_Mechanism::ADVANCED;

  throw mqsim_error("Unknown data caching mechanism specified in the SSD "
                    "configuration file");
}

force_inline SSD_Components::Cache_Sharing_Mode
to_cache_sharing_mode(std::string v)
{
  namespace sc = SSD_Components;

  Utils::to_upper(v);

  if (v == "SHARED")             return sc::Cache_Sharing_Mode::SHARED;
  if (v == "EQUAL_PARTITIONING") return sc::Cache_Sharing_Mode::EQUAL_PARTITIONING;

  throw mqsim_error("Unknown data cache sharing mode specified in the SSD"
                    " configuration file");
}

#endif /* Predefined include guard __MQSim__DataCacheDefs__ */
