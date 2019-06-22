#include "Data_Cache_Manager_Base.h"
#include "../FTL.h"

using namespace SSD_Components;

Caching_Mode* Data_Cache_Manager_Base::caching_mode_per_input_stream;

Data_Cache_Manager_Base::Data_Cache_Manager_Base(const sim_object_id_type& id,
                                                 Host_Interface_Base* host_interface,
                                                 NVM_Firmware* nvm_firmware,
                                                 uint32_t dram_row_size,
                                                 uint32_t dram_data_rate,
                                                 uint32_t dram_busrt_size,
                                                 sim_time_type dram_tRCD,
                                                 sim_time_type dram_tCL,
                                                 sim_time_type dram_tRP,
                                                 Caching_Mode* caching_mode_per_istream,
                                                 Cache_Sharing_Mode sharing_mode,
                                                 uint32_t stream_count)
  : MQSimEngine::Sim_Object(id),
    __user_request_handler(this, &Data_Cache_Manager_Base::__handle_user_request),
    __user_req_svc_handler(),
    __user_mem_tr_svc_handler(),
    host_interface(host_interface),
    nvm_firmware(nvm_firmware),
    dram_row_size(dram_row_size),
    dram_data_rate(dram_data_rate),
    dram_busrt_size(dram_busrt_size),
    dram_burst_transfer_time_ddr(double(ONE_SECOND) / (dram_data_rate * 1000 * 1000)),
    dram_tRCD(dram_tRCD),
    dram_tCL(dram_tCL),
    dram_tRP(dram_tRP),
    sharing_mode(sharing_mode),
    stream_count(stream_count)
{
  caching_mode_per_input_stream = new Caching_Mode[stream_count];
  for (uint32_t i = 0; i < stream_count; i++)
    caching_mode_per_input_stream[i] = caching_mode_per_istream[i];
}

Data_Cache_Manager_Base::~Data_Cache_Manager_Base()
{
  delete [] caching_mode_per_input_stream;
}

void
Data_Cache_Manager_Base::__handle_user_request(User_Request* request)
{
  // Pass user request to children.
  process_new_user_request(request);
}

void
Data_Cache_Manager_Base::broadcast_user_request_serviced_signal(User_Request* request)
{
  for (auto handler : __user_req_svc_handler)
    (*handler)(request);
}

void
Data_Cache_Manager_Base::broadcast_user_memory_transaction_serviced_signal(NVM_Transaction* transaction)
{
  for (auto handler : __user_mem_tr_svc_handler)
    (*handler)(transaction);
}

void
Data_Cache_Manager_Base::Setup_triggers()
{
  Sim_Object::Setup_triggers();

  host_interface->connect_to_user_request_signal(__user_request_handler);
}

void
Data_Cache_Manager_Base::Set_host_interface(Host_Interface_Base* interface)
{
  host_interface = interface;
}

void
Data_Cache_Manager_Base::Do_warmup(const std::vector<Utils::Workload_Statistics *>& workload_stats)
{ /* Default do nothing */}