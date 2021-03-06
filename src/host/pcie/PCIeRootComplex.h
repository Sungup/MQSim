#ifndef PCIE_ROOT_COMPLEX_H
#define PCIE_ROOT_COMPLEX_H

#include "../../ssd/interface/Host_Interface_Defs.h"

#include "../ioflow/IO_Flow_Base.h"

#include "../Host_Defs.h"
#include "PCIeLink.h"
#include "PCIeMessage.h"
#include "PCIePort.h"

namespace Host_Components
{
  class PCIeRootComplex {
  private:
    PCIePort<PCIeRootComplex> __port;

    HostInterface_Types __ssd_type;
    SATA_HBA * __sata_hba;
    IoFlowList& __io_flows;

    PCIeMessagePool& __msg_pool;

  private:
    void __consume_nvme_io(const void* payload);
    void __consume_sata_io(const void* payload);

    void* __read_nvme_sqe(uint64_t address);
    void* __read_sata_ncq(uint64_t address);

    void __write_to_memory(uint64_t address, const void* payload);
    void __read_from_memory(uint64_t address, uint32_t size);

    // Modern processors support DDIO, where all writes to memory are going
    // through LLC
    void __consume_pcie_message(PCIeMessage* messages);

  public:
    PCIeRootComplex(PCIeLink& pcie_link,
                    IoFlowList& IO_flows,
                    HostInterface_Types SSD_device_type);

    void write_to_device(uint64_t address, uint16_t write_value);

    void assign_sata_hba(SATA_HBA* hba);

  };

  force_inline void
  PCIeRootComplex::write_to_device(uint64_t address, uint16_t write_value)
  {
    // IO_Flow/SATA --> PCIe root complex -->NVMe
    __port.deliver(__msg_pool.construct(PCIe_Message_Type::WRITE_REQ,
                                        PCIeDest::DEVICE,
                                        address,
                                        write_value));
  }

  force_inline void
  PCIeRootComplex::assign_sata_hba(SATA_HBA* hba)
  {
    __sata_hba = hba;
  }
}

#endif //!PCIE_ROOT_COMPLEX_H