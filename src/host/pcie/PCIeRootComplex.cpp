#include "PCIeRootComplex.h"

#include "../ioflow/IO_Flow_Base.h"
#include "../sata/SATA_HBA.h"

using namespace Host_Components;

PCIeRootComplex::PCIeRootComplex(PCIeLink& pcie_link,
                                 IoFlowList& IO_flows,
                                 HostInterface_Types SSD_device_type)
  : __port(this,
           &PCIeRootComplex::__consume_pcie_message,
           pcie_link,
           PCIeDest::HOST),
    __ssd_type(SSD_device_type),
    __sata_hba(nullptr),
    __io_flows(IO_flows),
    __msg_pool(pcie_link.shared_pcie_message_pool())
{ }

force_inline void*
PCIeRootComplex::__read_nvme_sqe(uint64_t address)
{
  uint16_t flow_id = qid_to_flow_id(cq_addr_to_qid(address));

  return __io_flows[flow_id]->read_sq_entry(address);
}

force_inline void*
PCIeRootComplex::__read_sata_ncq(uint64_t address)
{
  return __sata_hba->read_sq_entry(address);
}

force_inline void
PCIeRootComplex::__consume_nvme_io(const void *payload)
{
  uint32_t flow_id = qid_to_flow_id(((CQEntry*)payload)->SQ_ID);

  __io_flows[flow_id]->consume_nvme_io((CQEntry *) payload);
}

force_inline void
PCIeRootComplex::__consume_sata_io(const void *payload)
{
  __sata_hba->consume_sata_io((CQEntry *) payload);
}

force_inline void
PCIeRootComplex::__write_to_memory(uint64_t address, const void* payload)
{
  // this is a request to write back a read request data into memory
  // (in modern systems the write is done to LLC)
  if (address >= DATA_MEMORY_REGION)
    return;

  switch (__ssd_type) {
  case HostInterface_Types::NVME: __consume_nvme_io(payload); break;
  case HostInterface_Types::SATA: __consume_sata_io(payload); break;
  }
}

force_inline void
PCIeRootComplex::__read_from_memory(uint64_t address, uint32_t read_size)
{
  uint32_t payload_size;
  void*    payload;

  // this is a request to read the data of a write request
  if (address >= DATA_MEMORY_REGION) {
    //No need to transfer data in the standalone mode of MQSim
    payload = nullptr;

    payload_size = read_size;

  } else {
    switch (__ssd_type) {
    case HostInterface_Types::NVME: payload = __read_nvme_sqe(address); break;
    case HostInterface_Types::SATA: payload = __read_sata_ncq(address); break;
    }

    payload_size = SQEntry::size();
  }

  __port.deliver(__msg_pool.construct(PCIe_Message_Type::READ_COMP,
                                      PCIeDest::DEVICE,
                                      address,
                                      payload_size,
                                      payload));
}

void
PCIeRootComplex::__consume_pcie_message(PCIeMessage* messages)
{
  // PCIe link --> PCIe root complex
  //     1) READ_REQ:  PCIe root complex --> NVMe as READ_COMP
  //     2) WRITE_REQ: PCIe root complex --> IO Flow / SATA
  switch (messages->type) {
    case PCIe_Message_Type::READ_REQ:
      __read_from_memory(messages->address,
                         (uint32_t) (intptr_t) messages->payload());
      break;

    case PCIe_Message_Type::WRITE_REQ:
      __write_to_memory(messages->address, messages->payload());
      break;

    default:
      break;
  }

  // TODO Check memory leak for the payload
  messages->release();
}
