//
// MemoryTransferInfo
// MQSim
//
// Created by 文盛業 on 2019-07-10.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__MemoryTransferInfo__
#define __MQSim__MemoryTransferInfo__

#include <cstdint>
#include "../../sim/Sim_Defs.h"
#include "../../utils/InlineTools.h"
#include "../../utils/ObjectPool.h"

namespace SSD_Components {
  enum class Data_Cache_Simulation_Event_Type {
    MEMORY_READ_FOR_CACHE_EVICTION_FINISHED, // Use pointer of list on the related_req
    MEMORY_WRITE_FOR_CACHE_FINISHED,
    MEMORY_READ_FOR_USERIO_FINISHED,
    MEMORY_WRITE_FOR_USERIO_FINISHED
  };

  class MemoryTransferInfoBase {
  public:
    const uint32_t Size_in_bytes;
    const Data_Cache_Simulation_Event_Type next_event_type;
    const stream_id_type Stream_id;

  private:
    void* __related_request;

  public:
    MemoryTransferInfoBase(uint32_t size,
                           Data_Cache_Simulation_Event_Type type,
                           stream_id_type stream_id,
                           void* related_request = nullptr);

    virtual ~MemoryTransferInfoBase();

    UserRequest* related_user_request() const;
    std::list<NvmTransaction*>* related_write_transactions() const;

    void clear_eviction_list();
  };

  force_inline
  MemoryTransferInfoBase::MemoryTransferInfoBase(uint32_t size,
                                                 Data_Cache_Simulation_Event_Type type,
                                                 stream_id_type stream_id,
                                                 void* related_request)
    : Size_in_bytes(size),
      next_event_type(type),
      Stream_id(stream_id),
      __related_request(related_request)
  { }

  force_inline
  MemoryTransferInfoBase::~MemoryTransferInfoBase()
  {
    if (__related_request
          && next_event_type == Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_CACHE_EVICTION_FINISHED)
      clear_eviction_list();
  }

  force_inline UserRequest*
  MemoryTransferInfoBase::related_user_request() const
  {
    assert(next_event_type == Data_Cache_Simulation_Event_Type::MEMORY_WRITE_FOR_USERIO_FINISHED ||
           next_event_type == Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_USERIO_FINISHED);

    return (UserRequest*)__related_request;
  }

  force_inline std::list<NvmTransaction*>*
  MemoryTransferInfoBase::related_write_transactions() const
  {
    assert(next_event_type == Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_CACHE_EVICTION_FINISHED);

    return (std::list<NvmTransaction*>*) __related_request;
  }

  force_inline void
  MemoryTransferInfoBase::clear_eviction_list()
  {
    assert(next_event_type == Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_CACHE_EVICTION_FINISHED);

    delete (std::list<NvmTransaction*>*)__related_request;

    __related_request = nullptr;
  }

  typedef Utils::ObjectPool<MemoryTransferInfoBase> MemoryTransferInfoPool;
  typedef MemoryTransferInfoPool::item_t            MemoryTransferInfo;
}

#endif /* Predefined include guard __MQSim__MemoryTransferInfo__ */

