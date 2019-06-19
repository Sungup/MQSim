#ifndef DATA_CACHE_MANAGER_BASE_H
#define DATA_CACHE_MANAGER_BASE_H

#include <vector>
#include "../../sim/Sim_Object.h"
#include "../Host_Interface_Base.h"
#include "../User_Request.h"
#include "../NVM_Firmware.h"
#include "../NVM_PHY_ONFI.h"
#include "../../utils/Workload_Statistics.h"

#include "DataCacheDefs.h"
#include "../interface/HostInterfaceHandler.h"

namespace SSD_Components
{
  class NVM_Firmware;
  class Host_Interface_Base;

  class Data_Cache_Manager_Base: public MQSimEngine::Sim_Object
  {
    friend class Data_Cache_Manager_Flash_Advanced;
    friend class Data_Cache_Manager_Flash_Simple;

  public:
    typedef void(*UserRequestServicedSignalHanderType) (User_Request*);
    typedef void(*MemoryTransactionServicedSignalHanderType) (NVM_Transaction*);

  private:
    UserRequestServiceHandler<Data_Cache_Manager_Base> __user_request_handler;

    UserRequestServiceHandlerList __user_req_svc_handler;
    TransactionServiceHandlerList __user_mem_tr_svc_handler;

    void __handle_user_request(User_Request& request);

  protected:
    static Data_Cache_Manager_Base* _my_instance;
    static Caching_Mode* caching_mode_per_input_stream;

    Host_Interface_Base* host_interface;
    NVM_Firmware* nvm_firmware;

    const uint32_t dram_row_size;     //The size of the DRAM rows in bytes
    const uint32_t dram_data_rate;    //in MT/s
    const uint32_t dram_busrt_size;

    // The transfer time of two bursts, changed from sim_time_type
    // to double to increase precision
    const double dram_burst_transfer_time_ddr;

    //DRAM access parameters in nano-seconds
    const sim_time_type dram_tRCD;
    const sim_time_type dram_tCL;
    const sim_time_type dram_tRP;

    Cache_Sharing_Mode sharing_mode;
    uint32_t stream_count;

    std::vector<UserRequestServicedSignalHanderType> connected_user_request_serviced_signal_handlers;
    std::vector<MemoryTransactionServicedSignalHanderType> connected_user_memory_transaction_serviced_signal_handlers;

    void broadcast_user_request_serviced_signal(User_Request& user_request);
    void broadcast_user_memory_transaction_serviced_signal(NVM_Transaction& transaction);

    static void handle_user_request_arrived_signal(User_Request* user_request);
    virtual void process_new_user_request(User_Request& user_request) = 0;

  public:
    Data_Cache_Manager_Base(const sim_object_id_type& id,
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
                            uint32_t stream_count);

    virtual ~Data_Cache_Manager_Base();

    void Setup_triggers();

    void Connect_to_user_request_serviced_signal(UserRequestServicedSignalHanderType);
    void Connect_to_user_memory_transaction_serviced_signal(MemoryTransactionServicedSignalHanderType);

    void connect_to_user_request_service_handler(UserRequestServiceHandlerBase& handler);
    void connect_to_user_transaction_service_handler(TransactionServiceHandlerBase& handler);
    void Set_host_interface(Host_Interface_Base* interface);

    virtual void Do_warmup(const std::vector<Utils::Workload_Statistics*>& workload_stats);
  };

  force_inline void
  Data_Cache_Manager_Base::connect_to_user_request_service_handler(UserRequestServiceHandlerBase& handler)
  {
    __user_req_svc_handler.emplace_back(&handler);
  }

  force_inline void
  Data_Cache_Manager_Base::connect_to_user_transaction_service_handler(TransactionServiceHandlerBase& handler)
  {
    __user_mem_tr_svc_handler.emplace_back(&handler);
  }

  force_inline sim_time_type
  estimate_dram_access_time(uint32_t memory_access_size_in_byte,
                            uint32_t dram_row_size,
                            uint32_t dram_burst_size_in_bytes,
                            double dram_burst_transfer_time_ddr,
                            sim_time_type tRCD,
                            sim_time_type tCL,
                            sim_time_type tRP)
  {
    // TODO Make simple calculating dram access time
    if (memory_access_size_in_byte <= dram_row_size)
      return sim_time_type(tRCD + tCL + sim_time_type((double)(memory_access_size_in_byte / dram_burst_size_in_bytes / 2) * dram_burst_transfer_time_ddr));
    else
      return sim_time_type((tRCD + tCL + sim_time_type((double)(dram_row_size / dram_burst_size_in_bytes / 2 * dram_burst_transfer_time_ddr) + tRP) * (double)(memory_access_size_in_byte / dram_row_size / 2))
                            + tRCD + tCL + sim_time_type((double)(memory_access_size_in_byte % dram_row_size) / ((double)dram_burst_size_in_bytes * dram_burst_transfer_time_ddr)));
  }
}

#endif // !DATA_CACHE_MANAGER_BASE_H
