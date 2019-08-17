//
// SubmissionQueue
// MQSim
//
// Created by 文盛業 on 2019-08-11.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__SubmissionQueue__
#define __MQSim__SubmissionQueue__

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "IoQueueInfo.h"
#include "../HostIORequest.h"

namespace Host_Components {
  class SubmissionQueue {
  private:
    NVMe_Queue_Pair& __pair;
    std::vector<HostIORequest*> __queue;

    // The I/O requests that are enqueued in the I/O queue of the SSD device
    std::unordered_map<uint16_t, HostIORequest*> __map;

    std::unordered_set<uint16_t> __ready_ids;

    uint16_t __get_ready_id();
    void __put_ready_id(uint16_t id);

  public:
    explicit SubmissionQueue(NVMe_Queue_Pair& pair);

    bool is_full() const;
    uint16_t enqueue(HostIORequest* request);
    HostIORequest* dequeue(uint16_t id);

    const HostIORequest* request_of(uint64_t address) const;
  };

  force_inline
  SubmissionQueue::SubmissionQueue(Host_Components::NVMe_Queue_Pair& pair)
    : __pair(pair),
      __queue(pair.sq_size, nullptr)
  {
    __ready_ids.reserve(UINT16_MAX);
    for (uint16_t id = 0; id < UINT16_MAX; ++id)
      __ready_ids.insert(id);
  }

  force_inline uint16_t
  SubmissionQueue::__get_ready_id()
  {
    auto cmd_iter = __ready_ids.begin();
    auto cmd_id = *cmd_iter;

    if (__map[cmd_id])
      throw mqsim_error("Unexpected situation in IO_Flow_Base! "
                        "Overwriting an unhandled I/O request in the queue!");

    __ready_ids.erase(cmd_iter);

    return cmd_id;
  }

  force_inline void
  SubmissionQueue::__put_ready_id(uint16_t id)
  {
    __ready_ids.insert(id);
  }

  force_inline bool
  SubmissionQueue::is_full() const
  {
    return __pair.sq_is_full() || __ready_ids.empty();
  }

  force_inline uint16_t
  SubmissionQueue::enqueue(HostIORequest* request)
  {
    uint16_t id = __get_ready_id();

    request->IO_queue_info = id;
    __map[id] = request;

    __queue[__pair.sq_tail] = request;

    __pair.move_sq_tail();

    return id;
  }

  force_inline HostIORequest*
  SubmissionQueue::dequeue(uint16_t id)
  {
    auto* request = __map[id];

    __map.erase(id);

    __put_ready_id(id);

    return request;
  }

  force_inline const HostIORequest*
  SubmissionQueue::request_of(uint64_t address) const
  {
    return __queue[__pair.index_of(address)];
  }

}

#endif /* Predefined include guard __MQSim__SubmissionQueue__ */
