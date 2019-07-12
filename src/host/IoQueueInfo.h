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

namespace Host_Components
{
  class IoQueueInfo {
  public:
    uint16_t Submission_queue_head;
    uint16_t Submission_queue_tail;
    uint16_t Submission_queue_size;
    uint64_t Submission_tail_register_address_on_device;
    uint64_t Submission_queue_memory_base_address;
    uint16_t Completion_queue_head;
    uint16_t Completion_queue_tail;
    uint16_t Completion_queue_size;
    uint64_t Completion_head_register_address_on_device;
    uint64_t Completion_queue_memory_base_address;
  };

  // Native Command Queue for SATA
  class NCQ_Control_Structure : public IoQueueInfo
  {
  public:
    // Contains the I/O requests that are enqueued in the NCQ
    std::unordered_map<sim_time_type, HostIORequest*> queue;
  };

  typedef IoQueueInfo NVMe_Queue_Pair;
}

#endif /* Predefined include guard __MQSim__QueueStructure__ */
