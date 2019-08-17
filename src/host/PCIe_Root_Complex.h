#ifndef PCIE_ROOT_COMPLEX_H
#define PCIE_ROOT_COMPLEX_H

#include "../ssd/interface/Host_Interface_Defs.h"

#include "Host_Defs.h"
#include "HostIORequest.h"
#include "PCIe_Link.h"
#include "PCIeMessage.h"
#include "ioflow/IO_Flow_Base.h"

namespace Host_Components
{
  class PCIe_Root_Complex
  {
  public:
    PCIe_Root_Complex(PCIe_Link& pcie_link,
                      IoFlowList& IO_flows,
                      HostInterface_Types SSD_device_type);
    
    void Consume_pcie_message(PCIeMessage* messages)//Modern processors support DDIO, where all writes to memory are going through LLC
    {
      switch (messages->type)
      {
      case PCIe_Message_Type::READ_REQ:
        Read_from_memory(messages->address, (uint32_t)(intptr_t)messages->payload());
        break;
      case PCIe_Message_Type::WRITE_REQ:
        Write_to_memory(messages->address, messages->payload());
        break;
      default:
        break;
      }

      // TODO Check memory leak for the payload
      messages->release();
    }
    void Write_to_device(uint64_t address, uint16_t write_value);

    void assign_sata_hba(SATA_HBA* hba);

  private:
    PCIe_Link& pcie_link;
    HostInterface_Types SSD_device_type;
    SATA_HBA * sata_hba;

    IoFlowList& IO_flows;

    PCIeMessagePool& __msg_pool;

    void Write_to_memory(uint64_t address, const void* payload);
    void Read_from_memory(uint64_t address, uint32_t size);

  };

  force_inline void
  PCIe_Root_Complex::assign_sata_hba(SATA_HBA* hba)
  {
    sata_hba =hba;
  }
}

#endif //!PCIE_ROOT_COMPLEX_H