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
#include "../request/User_Request.h"
#include "../NVM_Transaction_Flash.h"

namespace SSD_Components {
  typedef Utils::ServiceHandlerBase<User_Request&> UserRequestServiceHandlerBase;

  template <typename T>
  using UserRequestServiceHandler = Utils::ServiceHandler<T, User_Request&>;

  typedef std::vector<UserRequestServiceHandlerBase*> UserRequestServiceHandlerList;
}

#endif /* Predefined include guard __MQSim__HostInterfaceHandler__ */
