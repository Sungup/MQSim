#include "Host_Interface_Base.h"

#include "../../host/pcie/PCIeSwitch.h"
#include "../dcm/Data_Cache_Manager_Base.h"

// Children classes for builder
#include "Host_Interface_NVMe.h"
#include "Host_Interface_SATA.h"

using namespace SSD_Components;

// ==============================
// Declare of Host_Interface_Base
// ==============================
Host_Interface_Base::Host_Interface_Base(const sim_object_id_type& id,
                                         HostInterface_Types type,
                                         LHA_type max_logical_sector_address,
                                         uint32_t sectors_per_page,
                                         Data_Cache_Manager_Base* cache)
  : MQSimEngine::Sim_Object(id),
    type(type),
    max_logical_sector_address(max_logical_sector_address),
    sectors_per_page(sectors_per_page),
    __user_request_handler(this, &Host_Interface_Base::__handle_user_request_signal_from_cache),
    __user_transaction_handler(this, &Host_Interface_Base::__handle_user_transaction_signal_from_cache),
    __connected_user_req_signal_handlers(),
    cache(cache),
    __pcie_switch(nullptr),
    input_stream_manager(nullptr),
    request_fetch_unit(nullptr)
{ }

Host_Interface_Base::~Host_Interface_Base()
{
  delete input_stream_manager;
  delete request_fetch_unit;
}

void
Host_Interface_Base::__handle_user_request_signal_from_cache(UserRequest* request)
{
  input_stream_manager->Handle_serviced_request(request);
}

void
Host_Interface_Base::__handle_user_transaction_signal_from_cache(NvmTransaction* transaction)
{
  input_stream_manager->Update_transaction_statistics(transaction);
}

void Host_Interface_Base::Setup_triggers()
{
  Sim_Object::Setup_triggers();

  cache->connect_to_user_request_service_handler(__user_request_handler);
  cache->connect_to_user_transaction_service_handler(__user_transaction_handler);
}

void
Host_Interface_Base::Send_read_message_to_host(uint64_t address, uint32_t request_read_data_size)
{
  using namespace Host_Components;

  __pcie_switch->send_to_host(
    __pcie_switch->make_pcie_message(PCIe_Message_Type::READ_REQ,
                                     PCIeDest::HOST,
                                     address,
                                     request_read_data_size)
                             );
}

void
Host_Interface_Base::Send_write_message_to_host(uint64_t address, void* message, uint32_t message_size)
{
  using namespace Host_Components;

  __pcie_switch->send_to_host(
    // TODO This block can make the memory leak while runnign simulator.
    __pcie_switch->make_pcie_message(PCIe_Message_Type::WRITE_REQ,
                                     PCIeDest::HOST,
                                     address,
                                     message_size,
                                     MQSimEngine::copy_data(message,
                                                            message_size))
                             );
}

void
Host_Interface_Base::Consume_pcie_message(Host_Components::PCIeMessage* message)
{
  if (message->type == Host_Components::PCIe_Message_Type::READ_COMP)
    request_fetch_unit->Process_pcie_read_message(message->address, message->payload(), message->payload_size);
  else
    request_fetch_unit->Process_pcie_write_message(message->address, message->payload(), message->payload_size);

  // TODO Check need to free for the write message's payload data
  message->release();
}

// -------------------------
// HostInterfaceBase builder
// -------------------------
HostInterfacePtr
SSD_Components::build_hil_object(const sim_object_id_type& id,
                                 const DeviceParameterSet& params,
                                 LHA_type max_logical_sector_address,
                                 uint32_t total_flows,
                                 Data_Cache_Manager_Base& dcm)
{
  switch (params.HostInterface_Type)
  {
  case HostInterface_Types::NVME:
    return std::make_shared<Host_Interface_NVMe>(id,
                                                 max_logical_sector_address,
                                                 params.IO_Queue_Depth,
                                                 params.IO_Queue_Depth,
                                                 total_flows,
                                                 params.Queue_Fetch_Size,
                                                 params.Flash_Parameters.page_size_in_sector(),
                                                 &dcm);
  case HostInterface_Types::SATA:
    return std::make_shared<Host_Interface_SATA>(id,
                                                 params.IO_Queue_Depth,
                                                 max_logical_sector_address,
                                                 params.Flash_Parameters.page_size_in_sector(),
                                                 &dcm);
  }
}

NVMeInterfacePtr
SSD_Components::to_nvme_interface(HostInterfacePtr& hil)
{
  return std::dynamic_pointer_cast<Host_Interface_NVMe>(hil);
}

SataInterfacePtr
SSD_Components::to_sata_interface(HostInterfacePtr& hil)
{
  return std::dynamic_pointer_cast<Host_Interface_SATA>(hil);
}

const NVMeInterfacePtr
SSD_Components::to_nvme_interface(const HostInterfacePtr& hil)
{
  return std::dynamic_pointer_cast<Host_Interface_NVMe>(hil);
}

const SataInterfacePtr
SSD_Components::to_sata_interface(const HostInterfacePtr& hil)
{
  return std::dynamic_pointer_cast<Host_Interface_SATA>(hil);
}
