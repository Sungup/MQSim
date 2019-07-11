#ifndef DATA_CACHE_MANAGER_BASE_H
#define DATA_CACHE_MANAGER_BASE_H

#include "../request/UserRequest.h"
#include "../phy/NVM_PHY_ONFI.h"
#include "../../utils/Workload_Statistics.h"

#include "../interface/HostInterfaceHandler.h"

// Refined header list
#include <cstdint>
#include <vector>

#include "../../utils/InlineTools.h"
#include "../NvmTransactionFlashWR.h"
#include "../NVM_Firmware.h"

#include "DataCacheDefs.h"
#include "MemoryTransferInfo.h"

#include "../../sim/Sim_Object.h"


namespace SSD_Components
{
  class Host_Interface_Base;

  class Data_Cache_Manager_Base: public MQSimEngine::Sim_Object
  {
    friend class Data_Cache_Manager_Flash_Advanced;
    friend class Data_Cache_Manager_Flash_Simple;

  private:
    UserRequestServiceHandler<Data_Cache_Manager_Base> __user_request_handler;

    UserRequestServiceHandlerList __user_req_svc_handler;
    NvmTransactionHandlerList __user_mem_tr_svc_handler;

  protected:
    MemoryTransferInfoPool _mem_transfer_info_pool;

    std::vector<Caching_Mode> caching_mode_per_input_stream;

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

  private:
    void __handle_user_request(UserRequest* request);

  protected:
    void broadcast_user_request_serviced_signal(UserRequest* user_request);
    void broadcast_user_memory_transaction_serviced_signal(NvmTransaction* transaction);

    NvmTransactionFlashWR* _make_evict_tr(Transaction_Source_Type source,
                                          stream_id_type stream_id,
                                          uint32_t data_size_in_byte,
                                          const Data_Cache_Slot_Type& evicted);

    virtual void process_new_user_request(UserRequest* user_request) = 0;

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

    ~Data_Cache_Manager_Base() override = default;

    void Setup_triggers() override;

    void connect_to_user_request_service_handler(UserRequestServiceHandlerBase& handler);
    void connect_to_user_transaction_service_handler(NvmTransactionHandlerBase& handler);

    void connect_host_interface(Host_Interface_Base& interface);

    virtual void Do_warmup(const Utils::WorkloadStatsList& workload_stats);
  };

  force_inline void
  Data_Cache_Manager_Base::connect_to_user_request_service_handler(UserRequestServiceHandlerBase& handler)
  {
    __user_req_svc_handler.emplace_back(&handler);
  }

  force_inline void
  Data_Cache_Manager_Base::connect_to_user_transaction_service_handler(NvmTransactionHandlerBase& handler)
  {
    __user_mem_tr_svc_handler.emplace_back(&handler);
  }

  force_inline NvmTransactionFlashWR*
  Data_Cache_Manager_Base::_make_evict_tr(Transaction_Source_Type source,
                                          stream_id_type stream_id,
                                          uint32_t data_size_in_byte,
                                          const Data_Cache_Slot_Type& evicted)
  {
    auto* tr = new NvmTransactionFlashWR(source,
                                         stream_id,
                                         data_size_in_byte,
                                         evicted);

    return tr;
  }

  force_inline void
  Data_Cache_Manager_Base::connect_host_interface(Host_Interface_Base& interface)
  {
    host_interface = &interface;
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

  // --------------------------
  // Data Cache Manager builder
  // --------------------------
  typedef std::shared_ptr<Data_Cache_Manager_Base> DataCacheManagerPtr;

  DataCacheManagerPtr build_dcm_object(const sim_object_id_type& id,
                                       const DeviceParameterSet& params,
                                       NVM_Firmware& firmware,
                                       NVM_PHY_ONFI& phy,
                                       CachingModeList& caching_modes);
}

#endif // !DATA_CACHE_MANAGER_BASE_H
