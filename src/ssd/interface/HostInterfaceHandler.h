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
#include "../User_Request.h"

namespace SSD_Components {
  typedef Utils::ServiceHandlerBase<User_Request*>    UserRequestServiceHandlerBase;
  typedef Utils::ServiceHandlerBase<NVM_Transaction*> UserTransactionServiceHandlerBase;

  template <typename T>
  using UserRequestServiceHandler = Utils::ServiceHandler<T, User_Request*>;

  template <typename T>
  using UserTransactionServiceHandler = Utils::ServiceHandler<T, NVM_Transaction*>;

  typedef std::vector<UserRequestServiceHandlerBase*>     UserRequestServiceHandlerList;
  typedef std::vector<UserTransactionServiceHandlerBase*> UserTransactionServiceHandlerList;
}

#endif /* Predefined include guard __MQSim__HostInterfaceHandler__ */
