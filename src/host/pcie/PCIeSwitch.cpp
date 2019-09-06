#include "PCIeSwitch.h"

#include "../../ssd/interface/Host_Interface_Base.h"

using namespace Host_Components;

PCIeSwitch::PCIeSwitch(PCIeLink& pcie_link)
  : __port(this, &PCIeSwitch::__deliver_to_dev,
           pcie_link, PCIeDest::DEVICE),
    __interface(nullptr),
    __msg_pool(pcie_link.shared_pcie_message_pool())
{ }

void
PCIeSwitch::__deliver_to_dev(PCIeMessage* message)
{
  __interface->Consume_pcie_message(message);
}

