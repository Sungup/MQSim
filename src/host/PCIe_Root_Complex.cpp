#include "PCIe_Root_Complex.h"

#include "IO_Flow_Base.h"
#include "SATA_HBA.h"

using namespace Host_Components;

PCIe_Root_Complex::PCIe_Root_Complex(PCIe_Link& pcie_link,
                                     IoFlowList& IO_flows,
                                     HostInterface_Types SSD_device_type)
  : pcie_link(pcie_link),
    SSD_device_type(SSD_device_type),
    sata_hba(nullptr),
    IO_flows(IO_flows),
    __msg_pool(pcie_link.shared_pcie_message_pool())
{ }

void PCIe_Root_Complex::Write_to_memory(uint64_t address, const void* payload)
{
  if (address >= DATA_MEMORY_REGION)//this is a request to write back a read request data into memory (in modern systems the write is done to LLC)
  {
    //nothing to do
  }
  else
  {
    switch (SSD_device_type)
    {
    case HostInterface_Types::NVME:
    {
      uint32_t flow_id = QUEUE_ID_TO_FLOW_ID(((CQEntry*)payload)->SQ_ID);
      IO_flows[flow_id]->NVMe_consume_io_request((CQEntry*)payload);
      break;
    }
    case HostInterface_Types::SATA:
      sata_hba->SATA_consume_io_request((CQEntry*)payload);
      break;
    default:
      PRINT_ERROR("Uknown Host Interface type in PCIe_Root_Complex")
    }
  }
}

void PCIe_Root_Complex::Write_to_device(uint64_t address, uint16_t write_value)
{
  pcie_link.Deliver(__msg_pool.construct(PCIe_Message_Type::WRITE_REQ,
                                         PCIe_Destination_Type::DEVICE,
                                         address,
                                         write_value));
}

void PCIe_Root_Complex::Read_from_memory(const uint64_t address, const uint32_t read_size)
{
  uint32_t payload_size;
  void*    payload;

  if (address >= DATA_MEMORY_REGION)//this is a request to read the data of a write request
  {
    //nothing to do
    payload_size = read_size;
    payload = nullptr;//No need to transfer data in the standalone mode of MQSim
  }
  else
  {
    switch (SSD_device_type)
    {
    case HostInterface_Types::NVME:
    {
      uint16_t flow_id = QUEUE_ID_TO_FLOW_ID(uint16_t(address >> NVME_COMP_Q_MEMORY_REGION));
      payload = IO_flows[flow_id]->NVMe_read_sqe(address);
      payload_size = SQEntry::size();
      break;
    }
    case HostInterface_Types::SATA:
      payload = sata_hba->Read_ncq_entry(address);
      payload_size = SQEntry::size();
      break;
    }
  }

  pcie_link.Deliver(__msg_pool.construct(PCIe_Message_Type::READ_COMP,
                                         PCIe_Destination_Type::DEVICE,
                                         address,
                                         payload_size,
                                         payload));
}
