//
// QueueStructure
// MQSim
//
// Created by 文盛業 on 2019-07-12.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__QueueStructure__
#define __MQSim__QueueStructure__

#include <cstdint>
#include <unordered_map>

#include "../sim/Sim_Defs.h"
#include "HostIORequest.h"
#include "Host_Defs.h"

namespace Host_Components
{
  class IoQueueInfo {
  public:
    const uint16_t sq_size;
    uint16_t sq_head;
    uint16_t sq_tail;
    const uint64_t sq_tail_register;
    const uint64_t sq_memory_base_address;
    const uint16_t cq_size;
    uint16_t cq_head;
    uint16_t cq_tail;
    const uint64_t cq_head_register;
    const uint64_t cq_memory_base_address;

  public:
    IoQueueInfo(uint16_t id,
                uint16_t sq_size,
                uint16_t cq_size,
                uint64_t sq_register = INVALID_QUEUE_REGISTER,
                uint64_t cq_register = INVALID_QUEUE_REGISTER);

    void move_cq_head(uint16_t size = 1);
  };

  force_inline
  IoQueueInfo::IoQueueInfo(uint16_t id,
                           uint16_t sq_size,
                           uint16_t cq_size,
                           uint64_t sq_register,
                           uint64_t cq_register)
    : sq_size(sq_size),
      sq_head(0),
      sq_tail(0),
      sq_tail_register((sq_register == INVALID_QUEUE_REGISTER)
                         ? sq_register_address(id)
                         : sq_register),
      sq_memory_base_address(sq_memory_address(id)),
      cq_size(cq_size),
      cq_head(0),
      cq_tail(0),
      cq_head_register((cq_register == INVALID_QUEUE_REGISTER)
                         ? cq_register_address(id)
                         : cq_register),
      cq_memory_base_address(cq_memory_address(id))
  { }

  force_inline void
  IoQueueInfo::move_cq_head(uint16_t size)
  {
    cq_head += size;

    if (cq_size <= cq_head)
      cq_head -= cq_size;
  }

  // Native Command Queue for SATA
  class NCQ_Control_Structure : public IoQueueInfo
  {
  public:
    // Contains the I/O requests that are enqueued in the NCQ
    std::unordered_map<sim_time_type, HostIORequest*> queue;

  public:
    explicit NCQ_Control_Structure(uint16_t ncq_size);
  };

  force_inline
  NCQ_Control_Structure::NCQ_Control_Structure(uint16_t ncq_size)
    : IoQueueInfo(1, ncq_size, ncq_size,
                  NCQ_SUBMISSION_REGISTER, NCQ_COMPLETION_REGISTER),
      queue()
  { }

  typedef IoQueueInfo NVMe_Queue_Pair;
}

#endif /* Predefined include guard __MQSim__QueueStructure__ */
