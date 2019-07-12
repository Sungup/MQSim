#ifndef PCIE_MESSAGE_H
#define PCIE_MESSAGE_H

#include <cstdint>

#include "../sim/Engine.h"
#include "../utils/InlineTools.h"
#include "../utils/ObjectPool.h"

namespace Host_Components
{
  enum class PCIe_Destination_Type {HOST, DEVICE};
  enum class PCIe_Message_Type {READ_REQ, WRITE_REQ, READ_COMP};

  class PCIeMessageBase
  {
  public:
    const PCIe_Message_Type type;
    const PCIe_Destination_Type destination;
    const uint64_t address;
    const uint32_t payload_size;

  private:
    void* __payload;

  public:
    /// For read transaction
    PCIeMessageBase(PCIe_Message_Type type,
                    PCIe_Destination_Type dest,
                    uint64_t address,
                    uint32_t size);

    /// For write transaction
    PCIeMessageBase(PCIe_Message_Type type,
                    PCIe_Destination_Type dest,
                    uint64_t address,
                    uint32_t payload_size,
                    void* payload);

    void* payload() const;
  };

  force_inline
  PCIeMessageBase::PCIeMessageBase(PCIe_Message_Type type,
                                   PCIe_Destination_Type dest,
                                   uint64_t address,
                                   uint32_t size)
    : type(type),
      destination(dest),
      address(address),
      payload_size(sizeof(size)),
      __payload((void*)(intptr_t(size)))
  { }

  force_inline
  PCIeMessageBase::PCIeMessageBase(PCIe_Message_Type type,
                                   PCIe_Destination_Type dest,
                                   uint64_t address,
                                   uint32_t payload_size,
                                   void* payload)
    : type(type),
      destination(dest),
      address(address),
      payload_size(payload_size),
      __payload(payload)
  { }

  force_inline void*
  PCIeMessageBase::payload() const
  { return __payload; }

  typedef Utils::ObjectPool<PCIeMessageBase> PCIeMessagePool;
  typedef PCIeMessagePool::item_t            PCIeMessage;
}

#endif //!PCIE_MESSAGE_H