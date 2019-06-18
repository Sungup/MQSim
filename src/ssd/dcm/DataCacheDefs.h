//
// DataCacheDefs
// MQSim
//
// Created by 文盛業 on 2019-06-18.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__DataCacheDefs__
#define __MQSim__DataCacheDefs__

namespace SSD_Components {
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

#endif /* Predefined include guard __MQSim__DataCacheDefs__ */
