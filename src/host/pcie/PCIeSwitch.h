#ifndef PCIE_SWITCH_H
#define PCIE_SWITCH_H

#include <memory>

#include "../../utils/InlineTools.h"

#include "PCIeLink.h"
#include "PCIeMessage.h"
#include "PCIePort.h"

namespace SSD_Components
{
  class Host_Interface_Base;
}

namespace Host_Components
{
  class PCIeSwitch {
  private:
    PCIePort<PCIeSwitch> __port;

    SSD_Components::Host_Interface_Base* __interface;

    PCIeMessagePool& __msg_pool;

  private:
    void __deliver_to_dev(PCIeMessage* message);

  public:
    explicit PCIeSwitch(PCIeLink& pcie_link);

    void send_to_host(PCIeMessage* message);

    void connect_ssd(SSD_Components::Host_Interface_Base* interface);
    bool is_ssd_connected() const;

    template <typename... _Args>
    PCIeMessage* make_pcie_message(_Args&& ... args);
  };

  force_inline void
  PCIeSwitch::send_to_host(PCIeMessage* message)
  {
    __port.deliver(message);
  }

  force_inline void
  PCIeSwitch::connect_ssd(SSD_Components::Host_Interface_Base* interface)
  {
    __interface = interface;
  }

  force_inline bool
  PCIeSwitch::is_ssd_connected() const
  {
    return __interface != nullptr;
  }

  template <typename... _Args>
  force_inline PCIeMessage*
  PCIeSwitch::make_pcie_message(_Args&& ... args)
  {
    return __msg_pool.construct(args...);
  }
}

#endif //!PCIE_SWITCH_H