#include "Data_Cache_Manager_Flash_Advanced.h"

using namespace SSD_Components;

Data_Cache_Manager_Flash_Advanced::Data_Cache_Manager_Flash_Advanced(const sim_object_id_type& id,
                                                                     Host_Interface_Base* host_interface, NVM_Firmware* firmware, NVM_PHY_ONFI* flash_controller,
                                                                     uint32_t total_capacity_in_bytes, uint32_t dram_row_size,
                                                                     uint32_t dram_data_rate,
                                                                     uint32_t dram_busrt_size,
                                                                     sim_time_type dram_tRCD,
                                                                     sim_time_type dram_tCL,
                                                                     sim_time_type dram_tRP,
                                                                     Caching_Mode* caching_mode_per_input_stream,
                                                                     Cache_Sharing_Mode sharing_mode,
                                                                     uint32_t stream_count,
                                                                     uint32_t sector_no_per_page,
                                                                     uint32_t back_pressure_buffer_max_depth)
  : Data_Cache_Manager_Base(id,
                            host_interface,
                            firmware,
                            dram_row_size,
                            dram_data_rate,
                            dram_busrt_size,
                            dram_tRCD,
                            dram_tCL,
                            dram_tRP,
                            caching_mode_per_input_stream,
                            sharing_mode,
                            stream_count),
    flash_controller(flash_controller),
    capacity_in_bytes(total_capacity_in_bytes),
    memory_channel_is_busy(false),
    dram_execution_list_turn(0),
    back_pressure_buffer_max_depth(back_pressure_buffer_max_depth),
    __user_transaction_handler(this, &Data_Cache_Manager_Flash_Advanced::__handle_transaction_service)
{
  capacity_in_pages = capacity_in_bytes / (SECTOR_SIZE_IN_BYTE * sector_no_per_page);
  switch (sharing_mode)
  {
  case SSD_Components::Cache_Sharing_Mode::SHARED:
  {
    auto* sharedCache = new Data_Cache_Flash(capacity_in_pages);
    per_stream_cache = new Data_Cache_Flash*[stream_count];
    for (uint32_t i = 0; i < stream_count; i++)
      per_stream_cache[i] = sharedCache;
    dram_execution_queue = new std::queue<MemoryTransferInfo*>[1];
    waiting_user_requests_queue_for_dram_free_slot = new std::list<UserRequest*>[1];
    this->back_pressure_buffer_depth = new uint32_t[1];
    this->back_pressure_buffer_depth[0] = 0;
    shared_dram_request_queue = true;
    break;
  }
  case SSD_Components::Cache_Sharing_Mode::EQUAL_PARTITIONING:
    per_stream_cache = new Data_Cache_Flash*[stream_count];
    for (uint32_t i = 0; i < stream_count; i++)
      per_stream_cache[i] = new Data_Cache_Flash(capacity_in_pages / stream_count);
    dram_execution_queue = new std::queue<MemoryTransferInfo*>[stream_count];
    waiting_user_requests_queue_for_dram_free_slot = new std::list<UserRequest*>[stream_count];
    this->back_pressure_buffer_depth = new uint32_t[stream_count];
    for (uint32_t i = 0; i < stream_count; i++)
      this->back_pressure_buffer_depth[i] = 0;
    shared_dram_request_queue = false;
    break;
  default:
    break;
  }

  bloom_filter = new std::set<LPA_type>[stream_count];
}

Data_Cache_Manager_Flash_Advanced::~Data_Cache_Manager_Flash_Advanced()
{
  switch (sharing_mode)
  {
  case SSD_Components::Cache_Sharing_Mode::SHARED:
  {
    delete per_stream_cache[0];
    while (!dram_execution_queue[0].empty())
    {
      dram_execution_queue[0].pop();
    }
    break;
  }
  case SSD_Components::Cache_Sharing_Mode::EQUAL_PARTITIONING:
    for (uint32_t i = 0; i < stream_count; i++)
    {
      delete per_stream_cache[i];
      while (!dram_execution_queue[i].empty())
      {
        dram_execution_queue[i].pop();
      }
    }
    break;
  default:
    break;
  }

  delete per_stream_cache;
  delete[] dram_execution_queue;
  delete[] waiting_user_requests_queue_for_dram_free_slot;
  delete[] bloom_filter;
}

void
Data_Cache_Manager_Flash_Advanced::process_new_user_request(UserRequest* user_request)
{
  //This condition shouldn't happen, but we check it
  if (user_request->Transaction_list.empty())
    return;

  if (user_request->Type == UserRequestType::READ)
  {
    switch (caching_mode_per_input_stream[user_request->Stream_id])
    {
      case Caching_Mode::TURNED_OFF:
        nvm_firmware->dispatch_transactions(user_request->Transaction_list);
        return;
      case Caching_Mode::WRITE_CACHE:
      case Caching_Mode::READ_CACHE:
      case Caching_Mode::WRITE_READ_CACHE:
      {
        auto it = user_request->Transaction_list.begin();
        while (it != user_request->Transaction_list.end())
        {
          auto* tr = (NvmTransactionFlashRD*)(*it);
          if (per_stream_cache[tr->Stream_id]->Exists(tr->Stream_id, tr->LPA))
          {
            page_status_type available_sectors_bitmap = per_stream_cache[tr->Stream_id]->Get_slot(tr->Stream_id, tr->LPA).State_bitmap_of_existing_sectors & tr->read_sectors_bitmap;
            if (available_sectors_bitmap == tr->read_sectors_bitmap)
            {
              user_request->Sectors_serviced_from_cache += count_sector_no_from_status_bitmap(tr->read_sectors_bitmap);
              user_request->Transaction_list.erase(it++);//the ++ operation should happen here, otherwise the iterator will be part of the list after erasing it from the list
            }
            else if (available_sectors_bitmap != 0)
            {
              user_request->Sectors_serviced_from_cache += count_sector_no_from_status_bitmap(available_sectors_bitmap);
              tr->read_sectors_bitmap = (tr->read_sectors_bitmap & ~available_sectors_bitmap);
              tr->Data_and_metadata_size_in_byte -= count_sector_no_from_status_bitmap(available_sectors_bitmap) * SECTOR_SIZE_IN_BYTE;
              it++;
            }
            else it++;
          }
          else it++;
        }
        if (user_request->Sectors_serviced_from_cache > 0)
        {
          auto* transfer_info = _mem_transfer_info_pool.construct(
            user_request->Sectors_serviced_from_cache * SECTOR_SIZE_IN_BYTE,
            Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_USERIO_FINISHED,
            user_request->Stream_id,
            user_request
          );
          service_dram_access_request(*transfer_info);
        }
        if (!user_request->Transaction_list.empty())
          nvm_firmware->dispatch_transactions(user_request->Transaction_list);

        return;
      }
    }
  }
  else//This is a write request
  {
    switch (caching_mode_per_input_stream[user_request->Stream_id])
    {
      case Caching_Mode::TURNED_OFF:
      case Caching_Mode::READ_CACHE:
        nvm_firmware->dispatch_transactions(user_request->Transaction_list);
        return;
      case Caching_Mode::WRITE_CACHE://The data cache manger unit performs like a destage buffer
      case Caching_Mode::WRITE_READ_CACHE:
      {
        write_to_destage_buffer(*user_request);

        int queue_id = user_request->Stream_id;
        if (shared_dram_request_queue)
          queue_id = 0;
        if (!user_request->Transaction_list.empty())
          waiting_user_requests_queue_for_dram_free_slot[queue_id].push_back(user_request);
        return;
      }
    }
  }
}

void
Data_Cache_Manager_Flash_Advanced::write_to_destage_buffer(UserRequest& user_request)
{
  //To eliminate race condition, MQSim assumes the management information and user data are stored in separate DRAM modules
  uint32_t cache_eviction_read_size_in_sectors = 0;//The size of data evicted from cache
  uint32_t flash_written_back_write_size_in_sectors = 0;//The size of data that is both written back to flash and written to DRAM
  uint32_t dram_write_size_in_sectors = 0;//The size of data written to DRAM (must be >= flash_written_back_write_size_in_sectors)
  auto evicted_cache_slots = new std::list<NvmTransaction*>;
  std::list<NvmTransaction*> writeback_transactions;
  auto it = user_request.Transaction_list.begin();

  int queue_id = user_request.Stream_id;
  if (shared_dram_request_queue)
    queue_id = 0;

  while (it != user_request.Transaction_list.end()
         && (back_pressure_buffer_depth[queue_id] + cache_eviction_read_size_in_sectors + flash_written_back_write_size_in_sectors) < back_pressure_buffer_max_depth)
  {
    auto tr = (NvmTransactionFlashWR*)(*it);
    if (per_stream_cache[tr->Stream_id]->Exists(tr->Stream_id, tr->LPA))//If the logical address already exists in the cache
    {
      /*MQSim should get rid of writting stale data to the cache.
      * This situation may result from out-of-order transaction execution*/
      Data_Cache_Slot_Type slot = per_stream_cache[tr->Stream_id]->Get_slot(tr->Stream_id, tr->LPA);
      sim_time_type timestamp = slot.Timestamp;
      NVM::memory_content_type content = slot.Content;
      if (tr->DataTimeStamp > timestamp)
      {
        timestamp = tr->DataTimeStamp;
        content = tr->Content;
      }
      per_stream_cache[tr->Stream_id]->Update_data(tr->Stream_id, tr->LPA, content, timestamp, tr->write_sectors_bitmap | slot.State_bitmap_of_existing_sectors);
    }
    else//the logical address is not in the cache
    {
      if (!per_stream_cache[tr->Stream_id]->Check_free_slot_availability())
      {
        Data_Cache_Slot_Type evicted_slot = per_stream_cache[tr->Stream_id]->Evict_one_slot_lru();
        if (evicted_slot.Status == Cache_Slot_Status::DIRTY_NO_FLASH_WRITEBACK)
        {
          auto evicted_sectors = count_sector_no_from_status_bitmap(evicted_slot.State_bitmap_of_existing_sectors);
          evicted_cache_slots->emplace_back(
            _make_evict_tr(Transaction_Source_Type::CACHE,
                           tr->Stream_id,
                           evicted_sectors * SECTOR_SIZE_IN_BYTE,
                           evicted_slot)
          );

          cache_eviction_read_size_in_sectors += evicted_sectors;
        }
      }
      per_stream_cache[tr->Stream_id]->Insert_write_data(tr->Stream_id, tr->LPA, tr->Content, tr->DataTimeStamp, tr->write_sectors_bitmap);
    }
    dram_write_size_in_sectors += count_sector_no_from_status_bitmap(tr->write_sectors_bitmap);
    if (bloom_filter[tr->Stream_id].find(tr->LPA) == bloom_filter[tr->Stream_id].end())//hot/cold data separation
    {
      per_stream_cache[tr->Stream_id]->Change_slot_status_to_writeback(tr->Stream_id, tr->LPA); //Eagerly write back cold data
      flash_written_back_write_size_in_sectors += count_sector_no_from_status_bitmap(tr->write_sectors_bitmap);
      bloom_filter[user_request.Stream_id].insert(tr->LPA);
      writeback_transactions.push_back(tr);
    }
    user_request.Transaction_list.erase(it++);
  }

  user_request.Sectors_serviced_from_cache += dram_write_size_in_sectors;//This is very important update. It is used to decide when all data sectors of a user request are serviced
  back_pressure_buffer_depth[queue_id] += cache_eviction_read_size_in_sectors + flash_written_back_write_size_in_sectors;

  if (!evicted_cache_slots->empty())//Issue memory read for cache evictions
  {
    auto read_transfer_info = _mem_transfer_info_pool.construct(
      cache_eviction_read_size_in_sectors * SECTOR_SIZE_IN_BYTE,
      Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_CACHE_EVICTION_FINISHED,
      user_request.Stream_id,
      evicted_cache_slots
    );
    service_dram_access_request(*read_transfer_info);
  }

  if (dram_write_size_in_sectors)//Issue memory write to write data to DRAM
  {
    auto write_transfer_info = _mem_transfer_info_pool.construct(
      dram_write_size_in_sectors * SECTOR_SIZE_IN_BYTE,
      Data_Cache_Simulation_Event_Type::MEMORY_WRITE_FOR_USERIO_FINISHED,
      user_request.Stream_id,
      &user_request
    );
    service_dram_access_request(*write_transfer_info);
  }

  if (!writeback_transactions.empty())//If any writeback should be performed, then issue flash write transactions
    nvm_firmware->dispatch_transactions(writeback_transactions);

  auto sim_time = Simulator->Time();

  if (sim_time > next_bloom_filter_reset_milestone)//Reset control data structures used for hot/cold separation
  {
    bloom_filter[user_request.Stream_id].clear();
    next_bloom_filter_reset_milestone = sim_time + bloom_filter_reset_step;
  }
}

void
Data_Cache_Manager_Flash_Advanced::service_dram_access_request(MemoryTransferInfo& request_info)
{
  if (memory_channel_is_busy)
  {
    if(shared_dram_request_queue)
      dram_execution_queue[0].push(&request_info);
    else
      dram_execution_queue[request_info.Stream_id].push(&request_info);
  }
  else
  {
    auto sim = Simulator;
    sim->Register_sim_event(sim->Time() + estimate_dram_access_time(request_info.Size_in_bytes, dram_row_size,
                                                                    dram_busrt_size, dram_burst_transfer_time_ddr, dram_tRCD, dram_tCL, dram_tRP),
                            this, &request_info, static_cast<int>(request_info.next_event_type));
    memory_channel_is_busy = true;
    dram_execution_list_turn = request_info.Stream_id;
  }
}

void
Data_Cache_Manager_Flash_Advanced::__handle_transaction_service(NvmTransactionFlash* transaction)
{
  //First check if the transaction source is a user request or the cache itself
  if (transaction->Source != Transaction_Source_Type::USERIO &&
      transaction->Source != Transaction_Source_Type::CACHE)
    return;

  if (transaction->Source == Transaction_Source_Type::USERIO)
    broadcast_user_memory_transaction_serviced_signal(transaction);

  /*
   * This is an update read (a read that is generated for a write request that
   * partially updates page data).
   * An update read transaction is issued in Address Mapping Unit, but is
   * consumed in data cache manager.
   */
  if (transaction->Type == Transaction_Type::READ)
  {
    if (((NvmTransactionFlashRD*)transaction)->RelatedWrite != nullptr)
    {
      ((NvmTransactionFlashRD*)transaction)->RelatedWrite->RelatedRead = nullptr;
      return;
    }
    switch (caching_mode_per_input_stream[transaction->Stream_id])
    {
      case Caching_Mode::TURNED_OFF:
      case Caching_Mode::WRITE_CACHE:
        transaction->UserIORequest->Transaction_list.remove(transaction);
        if (transaction->UserIORequest->is_finished())
          broadcast_user_request_serviced_signal(transaction->UserIORequest);
        break;
      case Caching_Mode::READ_CACHE:
      case Caching_Mode::WRITE_READ_CACHE:
      {
        auto* transfer_info = _mem_transfer_info_pool.construct(
          count_sector_no_from_status_bitmap(((NvmTransactionFlashRD*)transaction)->read_sectors_bitmap) * SECTOR_SIZE_IN_BYTE,
          Data_Cache_Simulation_Event_Type::MEMORY_WRITE_FOR_CACHE_FINISHED,
          transaction->Stream_id
        );

        service_dram_access_request(*transfer_info);

        if (per_stream_cache[transaction->Stream_id]->Exists(transaction->Stream_id, transaction->LPA))
        {
          /*MQSim should get rid of writting stale data to the cache.
          * This situation may result from out-of-order transaction execution*/
          Data_Cache_Slot_Type slot = per_stream_cache[transaction->Stream_id]->Get_slot(transaction->Stream_id, transaction->LPA);
          sim_time_type timestamp = slot.Timestamp;
          NVM::memory_content_type content = slot.Content;
          if (((NvmTransactionFlashRD*)transaction)->DataTimeStamp > timestamp)
          {
            timestamp = ((NvmTransactionFlashRD*)transaction)->DataTimeStamp;
            content = ((NvmTransactionFlashRD*)transaction)->Content;
          }

          per_stream_cache[transaction->Stream_id]->Update_data(transaction->Stream_id, transaction->LPA, content,
                                                               timestamp, ((NvmTransactionFlashRD*)transaction)->read_sectors_bitmap | slot.State_bitmap_of_existing_sectors);
        }
        else
        {
          if (!per_stream_cache[transaction->Stream_id]->Check_free_slot_availability())
          {
            auto evicted_cache_slots = new std::list<NvmTransaction*>;
            Data_Cache_Slot_Type evicted_slot = per_stream_cache[transaction->Stream_id]->Evict_one_slot_lru();

            if (evicted_slot.Status == Cache_Slot_Status::DIRTY_NO_FLASH_WRITEBACK)
            {
              auto tr_info = _mem_transfer_info_pool.construct(
                count_sector_no_from_status_bitmap(evicted_slot.State_bitmap_of_existing_sectors) * SECTOR_SIZE_IN_BYTE,
                Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_CACHE_EVICTION_FINISHED,
                transaction->Stream_id,
                evicted_cache_slots
              );

              evicted_cache_slots->emplace_back(
                _make_evict_tr(Transaction_Source_Type::USERIO,
                               transaction->Stream_id,
                               transfer_info->Size_in_bytes,
                               evicted_slot)
              );

              service_dram_access_request(*tr_info);
            }
          }
          per_stream_cache[transaction->Stream_id]->Insert_write_data(transaction->Stream_id, transaction->LPA,
                                                                     ((NvmTransactionFlashRD*)transaction)->Content,
                                                                     ((NvmTransactionFlashRD*)transaction)->DataTimeStamp,
                                                                     ((NvmTransactionFlashRD*)transaction)->read_sectors_bitmap);

          auto tr_info = _mem_transfer_info_pool.construct(
            count_sector_no_from_status_bitmap(((NvmTransactionFlashRD*)transaction)->read_sectors_bitmap) * SECTOR_SIZE_IN_BYTE,
            Data_Cache_Simulation_Event_Type::MEMORY_WRITE_FOR_CACHE_FINISHED,
            transaction->Stream_id
          );
          service_dram_access_request(*tr_info);
        }

        transaction->UserIORequest->Transaction_list.remove(transaction);
        if (transaction->UserIORequest->is_finished())
          broadcast_user_request_serviced_signal(transaction->UserIORequest);
        break;
      }
    }
  }
  else//This is a write request
  {
    switch (caching_mode_per_input_stream[transaction->Stream_id])
    {
      case Caching_Mode::TURNED_OFF:
      case Caching_Mode::READ_CACHE:
        transaction->UserIORequest->Transaction_list.remove(transaction);
        if (transaction->UserIORequest->is_finished())
          broadcast_user_request_serviced_signal(transaction->UserIORequest);
        break;
      case Caching_Mode::WRITE_CACHE:
      case Caching_Mode::WRITE_READ_CACHE:
      {
        int sharing_id = transaction->Stream_id;
        if (shared_dram_request_queue)
          sharing_id = 0;
        back_pressure_buffer_depth[sharing_id] -= transaction->Data_and_metadata_size_in_byte / SECTOR_SIZE_IN_BYTE + (transaction->Data_and_metadata_size_in_byte % SECTOR_SIZE_IN_BYTE == 0 ? 0 : 1);


        if (per_stream_cache[transaction->Stream_id]->Exists(transaction->Stream_id, ((NvmTransactionFlashWR*)transaction)->LPA))
        {
          Data_Cache_Slot_Type slot = per_stream_cache[transaction->Stream_id]->Get_slot(transaction->Stream_id, ((NvmTransactionFlashWR*)transaction)->LPA);
          sim_time_type timestamp = slot.Timestamp;
          // Add comment for unused variable line
          // NVM::memory_content_type content = slot.Content;
          if (((NvmTransactionFlashWR*)transaction)->DataTimeStamp >= timestamp)
            per_stream_cache[transaction->Stream_id]->Remove_slot(transaction->Stream_id, ((NvmTransactionFlashWR*)transaction)->LPA);
        }

        auto user_request = waiting_user_requests_queue_for_dram_free_slot[sharing_id].begin();
        while (user_request != waiting_user_requests_queue_for_dram_free_slot[sharing_id].end())
        {
          write_to_destage_buffer(**user_request);
          if ((*user_request)->Transaction_list.empty())
            waiting_user_requests_queue_for_dram_free_slot[sharing_id].erase(user_request++);
          else user_request++;
          if (back_pressure_buffer_depth[sharing_id] > back_pressure_buffer_max_depth)//The traffic load on the backend is high and the waiting requests cannot be serviced
            break;
        }

        break;
      }
    }
  }

}

void
Data_Cache_Manager_Flash_Advanced::Execute_simulator_event(MQSimEngine::SimEvent* ev)
{
  auto eventType = (Data_Cache_Simulation_Event_Type)ev->Type;
  auto transfer_info = (MemoryTransferInfo*)ev->Parameters;

  switch (eventType)
  {
    case Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_USERIO_FINISHED://A user read is service from DRAM cache
    case Data_Cache_Simulation_Event_Type::MEMORY_WRITE_FOR_USERIO_FINISHED:
      transfer_info->related_user_request()->Sectors_serviced_from_cache -= transfer_info->Size_in_bytes / SECTOR_SIZE_IN_BYTE;
      if (transfer_info->related_user_request()->is_finished())
        broadcast_user_request_serviced_signal(transfer_info->related_user_request());
      break;
    case Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_CACHE_EVICTION_FINISHED://Reading data from DRAM and writing it back to the flash storage
      nvm_firmware->dispatch_transactions(*transfer_info->related_write_transactions());
      transfer_info->clear_eviction_list();
      break;
    case Data_Cache_Simulation_Event_Type::MEMORY_WRITE_FOR_CACHE_FINISHED://The recently read data from flash is written back to memory to support future user read requests
      break;
  }

  transfer_info->release();

  memory_channel_is_busy = false;

  auto sim = Simulator;

  if (shared_dram_request_queue)
  {
    if (!dram_execution_queue[0].empty())
    {
      MemoryTransferInfo* info = dram_execution_queue[0].front();
      dram_execution_queue[0].pop();
      sim->Register_sim_event(sim->Time() + estimate_dram_access_time(info->Size_in_bytes, dram_row_size, dram_busrt_size,
                                                                      dram_burst_transfer_time_ddr, dram_tRCD, dram_tCL, dram_tRP),
                              this, info, static_cast<int>(info->next_event_type));
      memory_channel_is_busy = true;
    }
  }
  else
  {
    for (uint32_t i = 0; i < stream_count; i++)
    {
      dram_execution_list_turn++;
      dram_execution_list_turn %= stream_count;
      if (!dram_execution_queue[dram_execution_list_turn].empty())
      {
        MemoryTransferInfo* info = dram_execution_queue[dram_execution_list_turn].front();
        dram_execution_queue[dram_execution_list_turn].pop();
        sim->Register_sim_event(sim->Time() + estimate_dram_access_time(info->Size_in_bytes, dram_row_size, dram_busrt_size,
                                                                        dram_burst_transfer_time_ddr, dram_tRCD, dram_tCL, dram_tRP),
                                this, info, static_cast<int>(info->next_event_type));
        memory_channel_is_busy = true;
        break;
      }
    }
  }
}

void
Data_Cache_Manager_Flash_Advanced::Setup_triggers()
{
  Data_Cache_Manager_Base::Setup_triggers();

  flash_controller->connect_to_transaction_service_signal(__user_transaction_handler);
}

void
Data_Cache_Manager_Flash_Advanced::Do_warmup(const Utils::WorkloadStatsList& workload_stats)
{
  // double total_write_arrival_rate = 0, total_read_arrival_rate = 0;
  switch (sharing_mode)
  {
  case Cache_Sharing_Mode::SHARED:
    //Estimate read arrival and write arrival rate
    //Estimate the queue length based on the arrival rate
    for (auto &stat : workload_stats)
    {
      switch (caching_mode_per_input_stream[stat.Stream_id])
      {
      case Caching_Mode::TURNED_OFF:
        break;
      case Caching_Mode::READ_CACHE:
#if 0
        if (stat->Type == Utils::Workload_Type::SYNTHETIC)
        {
        }
        else
        {
        }
#endif
        break;
      case Caching_Mode::WRITE_CACHE:
#if 0
        if (stat->Type == Utils::Workload_Type::SYNTHETIC)
        {
          uint32_t total_pages_accessed = 1;
          switch (stat->Address_distribution_type)
          {
          case Utils::Address_Distribution_Type::STREAMING:
            break;
          case Utils::Address_Distribution_Type::RANDOM_HOTCOLD:
            break;
          case Utils::Address_Distribution_Type::RANDOM_UNIFORM:
            break;
          }
        }
        else
        {

        }
#endif
        break;
      case Caching_Mode::WRITE_READ_CACHE:
#if 0
        //Put items on cache based on the accessed addresses
        if (stat->Type == Utils::Workload_Type::SYNTHETIC)
        {
        }
        else
        {
        }
#endif
        break;
      }
    }
    break;
  case Cache_Sharing_Mode::EQUAL_PARTITIONING:
    for (auto &stat : workload_stats)
    {
      switch (caching_mode_per_input_stream[stat.Stream_id])
      {
      case Caching_Mode::TURNED_OFF:
        break;
      case Caching_Mode::READ_CACHE:
#if 0
        //Put items on cache based on the accessed addresses
        if (stat->Type == Utils::Workload_Type::SYNTHETIC)
        {
        }
        else
        {
        }
#endif
        break;
      case Caching_Mode::WRITE_CACHE:
        //Estimate the request arrival rate
        //Estimate the request service rate
        //Estimate the average size of requests in the cache
        //Fillup the cache space based on accessed adddresses to the estimated average cache size
#if 0
        if (stat->Type == Utils::Workload_Type::SYNTHETIC)
        {
          //Estimate average write service rate
          uint32_t total_pages_accessed = 1;
          double average_write_arrival_rate, stdev_write_arrival_rate;
          double average_read_arrival_rate, stdev_read_arrival_rate;
          double average_write_service_time, average_read_service_time;
          switch (stat->Address_distribution_type)
          {
          case Utils::Address_Distribution_Type::STREAMING:
            break;
          case Utils::Address_Distribution_Type::RANDOM_HOTCOLD:
            break;
          case Utils::Address_Distribution_Type::RANDOM_UNIFORM:
            break;
          }
        }
        else
        {
        }
#endif
        break;
      case Caching_Mode::WRITE_READ_CACHE:
#if 0
        //Put items on cache based on the accessed addresses
        if (stat->Type == Utils::Workload_Type::SYNTHETIC)
        {
        }
        else
        {
        }
#endif
        break;
      }
    }
    break;
  }
}
