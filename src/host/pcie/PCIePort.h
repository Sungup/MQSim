//
// PCIePort
// MQSim
//
// Created by 文盛業 on 2019-08-19.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__PCIePort__
#define __MQSim__PCIePort__

#include "PCIeLink.h"

namespace Host_Components {
  class PCIePortBase {
  private:
    PCIeLink& __link;

  public:
    PCIePortBase(PCIeLink& link, PCIeDest port_direction)
      : __link(link)
    {
      __link.set_port(this, port_direction);
    }

    virtual ~PCIePortBase() = default;

    virtual void drain_message(PCIeMessage* message) = 0;

    void deliver(PCIeMessage* message)
    { __link.deliver(message); }
  };

  template <typename T>
  class PCIePort : public PCIePortBase {
    typedef void (T::*Handler)(PCIeMessage* message);

  private:
    T* __callee;
    Handler __handler;

  public:
    PCIePort(T* device, Handler handler,
             PCIeLink& link, PCIeDest port_direction)
      : PCIePortBase(link, port_direction),
        __callee(device),
        __handler(handler)
    { }

    ~PCIePort() final = default;

    void drain_message(PCIeMessage* message) final
    { (__callee->*__handler)(message); }
  };

}

#endif /* Predefined include guard __MQSim__PCIePort__ */
