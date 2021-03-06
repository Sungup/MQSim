#ifndef DATA_CACHE_MANAGER_FLASH_SIMPLE_H
#define DATA_CACHE_MANAGER_FLASH_SIMPLE_H

#include <list>
#include <queue>
#include <unordered_map>
#include "../../nvm_chip/flash_memory/FlashTypes.h"
#include "../SSD_Defs.h"
#include "Data_Cache_Manager_Base.h"
#include "Data_Cache_Flash.h"
#include "../NvmTransactionFlash.h"

namespace SSD_Components
{
  class Data_Cache_Manager_Flash_Simple : public Data_Cache_Manager_Base
  {
  private:
    NVM_PHY_ONFI * flash_controller;
    uint32_t capacity_in_bytes, capacity_in_pages;
    Data_Cache_Flash* data_cache;

    // The list of DRAM transfers that are waiting to be executed
    std::queue<MemoryTransferInfo*>* dram_execution_queue;

    //The list of user requests that are waiting for free space in DRAM
    std::list<UserRequest*>* waiting_user_requests_queue_for_dram_free_slot;

    int request_queue_turn;
    uint32_t back_pressure_buffer_max_depth;
    uint32_t back_pressure_buffer_depth;
    std::set<LPA_type>* bloom_filter;
    sim_time_type bloom_filter_reset_step = 1000000000;
    sim_time_type next_bloom_filter_reset_milestone = 0;

    FlashTransactionHandler<Data_Cache_Manager_Flash_Simple> __user_transaction_handler;

    void process_new_user_request(UserRequest* user_request) final;

    // Used in the WRITE_CACHE and WRITE_READ_CACHE modes in which the DRAM
    // space is used as a destage buffer
    void write_to_destage_buffer(UserRequest& user_request);

    void service_dram_access_request(MemoryTransferInfo& request_info);

    void __handle_transaction_service(NvmTransactionFlash* nvm_transaction);

  public:
    Data_Cache_Manager_Flash_Simple(const sim_object_id_type& id,
                                    Host_Interface_Base* host_interface,
                                    NVM_Firmware* firmware,
                                    NVM_PHY_ONFI* flash_controller,
                                    uint32_t total_capacity_in_bytes,
                                    uint32_t dram_row_size,
                                    uint32_t dram_data_rate,
                                    uint32_t dram_busrt_size,
                                    sim_time_type dram_tRCD,
                                    sim_time_type dram_tCL,
                                    sim_time_type dram_tRP,
                                    Caching_Mode* caching_mode_per_input_stream,
                                    uint32_t stream_count,
                                    uint32_t sector_no_per_page,
                                    uint32_t back_pressure_buffer_max_depth);
    ~Data_Cache_Manager_Flash_Simple() final;

    void Execute_simulator_event(MQSimEngine::SimEvent* ev) final;
    void Setup_triggers() final;
  };
}
#endif // !DATA_CACHE_MANAGER_FLASH_SIMPLE_H
