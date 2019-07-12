#include "PCIeSwitch.h"

#include "../ssd/interface/Host_Interface_Base.h"

using namespace Host_Components;

PCIeSwitch::PCIeSwitch(PCIe_Link& pcie_link)
  : __link(pcie_link),
    __interface(nullptr),
    __msg_pool(pcie_link.shared_pcie_message_pool())
{ }

void
PCIeSwitch::deliver_to_device(PCIeMessage* message)
{
  __interface->Consume_pcie_message(message);
}

void
PCIeSwitch::send_to_host(PCIeMessage* message)
{
  __link.Deliver(message);
}
