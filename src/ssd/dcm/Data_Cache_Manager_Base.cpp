#include "Data_Cache_Manager_Base.h"
#include "../FTL.h"
#include "../NVM_Firmware.h"
#include "../interface/Host_Interface_Base.h"

// Children classes for builder
#include "Data_Cache_Manager_Flash_Simple.h"
#include "Data_Cache_Manager_Flash_Advanced.h"

using namespace SSD_Components;

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
    _mem_transfer_info_pool(),
    caching_mode_per_input_stream(caching_mode_per_istream,
                                  caching_mode_per_istream + stream_count),
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
{ }

void
Data_Cache_Manager_Base::__handle_user_request(UserRequest* request)
{
  // Pass user request to children.
  process_new_user_request(request);
}

void
Data_Cache_Manager_Base::broadcast_user_request_serviced_signal(UserRequest* request)
{
  for (auto handler : __user_req_svc_handler)
    (*handler)(request);
}

void
Data_Cache_Manager_Base::broadcast_user_memory_transaction_serviced_signal(NvmTransaction* transaction)
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

DataCacheManagerPtr
SSD_Components::build_dcm_object(const sim_object_id_type& id,
                                 const DeviceParameterSet& params,
                                 NVM_Firmware& firmware,
                                 NVM_PHY_ONFI& phy,
                                 CachingModeList& caching_modes)
{
  switch (params.Caching_Mechanism) {
  case SSD_Components::Caching_Mechanism::SIMPLE:
    return std::make_shared<Data_Cache_Manager_Flash_Simple>(
      id,
      nullptr,
      &firmware,
      &phy,
      params.Data_Cache_Capacity,
      params.Data_Cache_DRAM_Row_Size,
      params.Data_Cache_DRAM_Data_Rate,
      params.Data_Cache_DRAM_Data_Busrt_Size,
      params.Data_Cache_DRAM_tRCD,
      params.Data_Cache_DRAM_tCL,
      params.Data_Cache_DRAM_tRP,
      caching_modes.data(),
      caching_modes.size(),
      params.Flash_Parameters.page_size_in_sector(),
      params.back_pressure_buffer_max_depth()
    );

  case SSD_Components::Caching_Mechanism::ADVANCED:
    return std::make_shared<Data_Cache_Manager_Flash_Advanced>(
      id,
      nullptr,
      &firmware,
      &phy,
      params.Data_Cache_Capacity,
      params.Data_Cache_DRAM_Row_Size,
      params.Data_Cache_DRAM_Data_Rate,
      params.Data_Cache_DRAM_Data_Busrt_Size,
      params.Data_Cache_DRAM_tRCD,
      params.Data_Cache_DRAM_tCL,
      params.Data_Cache_DRAM_tRP,
      caching_modes.data(),
      params.Data_Cache_Sharing_Mode,
      caching_modes.size(),
      params.Flash_Parameters.page_size_in_sector(),
      params.back_pressure_buffer_max_depth()
    );
  }
}
