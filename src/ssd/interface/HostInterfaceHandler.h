//
// HostInterfaceHandler
// MQSim
//
// Created by 文盛業 on 2019-06-18.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__HostInterfaceHandler__
#define __MQSim__HostInterfaceHandler__

#include "../../utils/ServiceHandler.h"
#include "../request/UserRequest.h"
#include "../NvmTransactionFlash.h"

namespace SSD_Components {
  typedef Utils::ServiceHandlerBase<UserRequest*> UserRequestServiceHandlerBase;

  template <typename T>
  using UserRequestServiceHandler = Utils::ServiceHandler<T, UserRequest*>;

  typedef std::vector<UserRequestServiceHandlerBase*> UserRequestServiceHandlerList;
}

#endif /* Predefined include guard __MQSim__HostInterfaceHandler__ */
