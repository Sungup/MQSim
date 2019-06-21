#ifndef PCIE_MESSAGE_H
#define PCIE_MESSAGE_H

#include <cstdint>

#include "../sim/Engine.h"
#include "../utils/InlineTools.h"

namespace Host_Components
{
  enum class PCIe_Destination_Type {HOST, DEVICE};
  enum class PCIe_Message_Type {READ_REQ, WRITE_REQ, READ_COMP};
  class PCIe_Message
  {
  public:
    /// For read transaction
    PCIe_Message(PCIe_Message_Type type,
                 PCIe_Destination_Type dest,
                 uint64_t address,
                 uint32_t size);

    /// For write transaction
    PCIe_Message(PCIe_Message_Type type,
                 PCIe_Destination_Type dest,
                 uint64_t address,
                 uint32_t payload_size,
                 void* payload);

    PCIe_Message_Type Type;
    PCIe_Destination_Type Destination;
    uint64_t Address;
    uint32_t Payload_size;
    void* Payload;

  };

  force_inline
  PCIe_Message::PCIe_Message(PCIe_Message_Type type,
                             PCIe_Destination_Type dest,
                             uint64_t address,
                             uint32_t size)
    : Type(type),
      Destination(dest),
      Address(address),
      Payload_size(sizeof(size)),
      Payload((void*)(intptr_t(size)))
  { }

  force_inline
  PCIe_Message::PCIe_Message(PCIe_Message_Type type,
                             PCIe_Destination_Type dest,
                             uint64_t address,
                             uint32_t payload_size,
                             void* payload)
    : Type(type),
      Destination(dest),
      Address(address),
      Payload_size(payload_size),
      Payload(payload)
  { }
}

#endif //!PCIE_MESSAGE_H