#include <stdexcept>
#include "../../nvm_chip/NVM_Types.h"
#include "Data_Cache_Manager_Flash_Simple.h"
#include "../NVM_Transaction_Flash_RD.h"
#include "../NVM_Transaction_Flash_WR.h"
#include "../FTL.h"

using namespace SSD_Components;

Data_Cache_Manager_Flash_Simple::Data_Cache_Manager_Flash_Simple(const sim_object_id_type& id,
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
                            Cache_Sharing_Mode::SHARED,
                            stream_count),
    flash_controller(flash_controller),
    capacity_in_bytes(total_capacity_in_bytes),
    sector_no_per_page(sector_no_per_page),
    request_queue_turn(0),
    back_pressure_buffer_max_depth(back_pressure_buffer_max_depth),
    __user_transaction_handler(this, &Data_Cache_Manager_Flash_Simple::__handle_transaction_service)
{
  capacity_in_pages = capacity_in_bytes / (SECTOR_SIZE_IN_BYTE * sector_no_per_page);
  data_cache = new Data_Cache_Flash(capacity_in_pages);
  dram_execution_queue = new std::queue<Memory_Transfer_Info*>[stream_count];
  waiting_user_requests_queue_for_dram_free_slot = new std::list<User_Request*>[stream_count];
  this->back_pressure_buffer_depth = 0;
  bloom_filter = new std::set<LPA_type>[stream_count];
}

Data_Cache_Manager_Flash_Simple::~Data_Cache_Manager_Flash_Simple()
{
  for (uint32_t i = 0; i < stream_count; i++)
  {
    while (!dram_execution_queue[i].empty())
    {
      delete dram_execution_queue[i].front();
      dram_execution_queue[i].pop();
    }
    for (auto &req : waiting_user_requests_queue_for_dram_free_slot[i])
      delete req;
  }
  delete data_cache;
  delete[] dram_execution_queue;
  delete[] waiting_user_requests_queue_for_dram_free_slot;
  delete[] bloom_filter;
}

void
Data_Cache_Manager_Flash_Simple::process_new_user_request(User_Request* user_request)
{
  if (user_request->Transaction_list.empty())//This condition shouldn't happen, but we check it
    return;

  if (user_request->Type == UserRequestType::READ)
  {
    switch (caching_mode_per_input_stream[user_request->Stream_id])
    {
    case Caching_Mode::TURNED_OFF:
      static_cast<FTL*>(nvm_firmware)->Address_Mapping_Unit->Translate_lpa_to_ppa_and_dispatch(user_request->Transaction_list);
      return;
    case Caching_Mode::WRITE_CACHE:
    {
      auto it = user_request->Transaction_list.begin();
      while (it != user_request->Transaction_list.end())
      {
        auto* tr = (NVM_Transaction_Flash_RD*)(*it);
        if (data_cache->Exists(tr->Stream_id, tr->LPA))
        {
          page_status_type available_sectors_bitmap = data_cache->Get_slot(tr->Stream_id, tr->LPA).State_bitmap_of_existing_sectors & tr->read_sectors_bitmap;
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
        auto tr_info = new Memory_Transfer_Info;
        tr_info->Size_in_bytes = user_request->Sectors_serviced_from_cache * SECTOR_SIZE_IN_BYTE;
        tr_info->Related_request = user_request;
        tr_info->next_event_type = Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_USERIO_FINISHED;
        tr_info->Stream_id = user_request->Stream_id;
        service_dram_access_request(*tr_info);
      }
      if (!user_request->Transaction_list.empty())
        static_cast<FTL*>(nvm_firmware)->Address_Mapping_Unit->Translate_lpa_to_ppa_and_dispatch(user_request->Transaction_list);

      return;
    }
    default:
      PRINT_ERROR("The specified caching mode is not not support in simple cache manager!")
    }
  }
  else//This is a write request
  {
    switch (caching_mode_per_input_stream[user_request->Stream_id])
    {
    case Caching_Mode::TURNED_OFF:
      static_cast<FTL*>(nvm_firmware)->Address_Mapping_Unit->Translate_lpa_to_ppa_and_dispatch(user_request->Transaction_list);
      return;
    case Caching_Mode::WRITE_CACHE://The data cache manger unit performs like a destage buffer
    {
      write_to_destage_buffer(*user_request);

      if (!user_request->Transaction_list.empty())
        waiting_user_requests_queue_for_dram_free_slot[user_request->Stream_id].push_back(user_request);
      return;
    }
    default:
      PRINT_ERROR("The specified caching mode is not not support in simple cache manager!")
    }
  }
}

void
Data_Cache_Manager_Flash_Simple::write_to_destage_buffer(User_Request& user_request)
{
  //To eliminate race condition, MQSim assumes the management information and user data are stored in separate DRAM modules
  uint32_t cache_eviction_read_size_in_sectors = 0;//The size of data evicted from cache
  uint32_t flash_written_back_write_size_in_sectors = 0;//The size of data that is both written back to flash and written to DRAM
  uint32_t dram_write_size_in_sectors = 0;//The size of data written to DRAM (must be >= flash_written_back_write_size_in_sectors)
  auto evicted_cache_slots = new std::list<NVM_Transaction*>;
  std::list<NVM_Transaction*> writeback_transactions;
  auto it = user_request.Transaction_list.begin();

  while (it != user_request.Transaction_list.end()
    && (back_pressure_buffer_depth + cache_eviction_read_size_in_sectors + flash_written_back_write_size_in_sectors) < back_pressure_buffer_max_depth)
  {
    auto tr = (NVM_Transaction_Flash_WR*)(*it);
    if (data_cache->Exists(tr->Stream_id, tr->LPA))//If the logical address already exists in the cache
    {
      /*MQSim should get rid of writting stale data to the cache.
      * This situation may result from out-of-order transaction execution*/
      Data_Cache_Slot_Type slot = data_cache->Get_slot(tr->Stream_id, tr->LPA);
      sim_time_type timestamp = slot.Timestamp;
      NVM::memory_content_type content = slot.Content;
      if (tr->DataTimeStamp > timestamp)
      {
        timestamp = tr->DataTimeStamp;
        content = tr->Content;
      }
      data_cache->Update_data(tr->Stream_id, tr->LPA, content, timestamp, tr->write_sectors_bitmap | slot.State_bitmap_of_existing_sectors);
    }
    else//the logical address is not in the cache
    {
      if (!data_cache->Check_free_slot_availability())
      {
        Data_Cache_Slot_Type evicted_slot = data_cache->Evict_one_slot_lru();
        if (evicted_slot.Status == Cache_Slot_Status::DIRTY_NO_FLASH_WRITEBACK)
        {
          evicted_cache_slots->push_back(new NVM_Transaction_Flash_WR(Transaction_Source_Type::CACHE,
            tr->Stream_id, count_sector_no_from_status_bitmap(evicted_slot.State_bitmap_of_existing_sectors) * SECTOR_SIZE_IN_BYTE,
            evicted_slot.LPA, nullptr, evicted_slot.Content, evicted_slot.State_bitmap_of_existing_sectors, evicted_slot.Timestamp));
          cache_eviction_read_size_in_sectors += count_sector_no_from_status_bitmap(evicted_slot.State_bitmap_of_existing_sectors);
          //DEBUG2("Evicting page" << evicted_slot.LPA << " from write buffer ")
        }
      }
      data_cache->Insert_write_data(tr->Stream_id, tr->LPA, tr->Content, tr->DataTimeStamp, tr->write_sectors_bitmap);
    }
    dram_write_size_in_sectors += count_sector_no_from_status_bitmap(tr->write_sectors_bitmap);
    if (bloom_filter[0].find(tr->LPA) == bloom_filter[0].end())//hot/cold data separation
    {
      data_cache->Change_slot_status_to_writeback(tr->Stream_id, tr->LPA); //Eagerly write back cold data
      flash_written_back_write_size_in_sectors += count_sector_no_from_status_bitmap(tr->write_sectors_bitmap);
      bloom_filter[0].insert(tr->LPA);
      writeback_transactions.push_back(tr);
    }
    user_request.Transaction_list.erase(it++);
  }

  user_request.Sectors_serviced_from_cache += dram_write_size_in_sectors;//This is very important update. It is used to decide when all data sectors of a user request are serviced
  back_pressure_buffer_depth += cache_eviction_read_size_in_sectors + flash_written_back_write_size_in_sectors;

  if (!evicted_cache_slots->empty())//Issue memory read for cache evictions
  {
    auto read_transfer_info = new Memory_Transfer_Info;
    read_transfer_info->Size_in_bytes = cache_eviction_read_size_in_sectors * SECTOR_SIZE_IN_BYTE;
    read_transfer_info->Related_request = evicted_cache_slots;
    read_transfer_info->next_event_type = Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_CACHE_EVICTION_FINISHED;
    read_transfer_info->Stream_id = user_request.Stream_id;
    service_dram_access_request(*read_transfer_info);
  }

  if (dram_write_size_in_sectors)//Issue memory write to write data to DRAM
  {
    auto write_transfer_info = new Memory_Transfer_Info;
    write_transfer_info->Size_in_bytes = dram_write_size_in_sectors * SECTOR_SIZE_IN_BYTE;
    write_transfer_info->Related_request = &user_request;
    write_transfer_info->next_event_type = Data_Cache_Simulation_Event_Type::MEMORY_WRITE_FOR_USERIO_FINISHED;
    write_transfer_info->Stream_id = user_request.Stream_id;
    service_dram_access_request(*write_transfer_info);
  }

  if (!writeback_transactions.empty())//If any writeback should be performed, then issue flash write transactions
    static_cast<FTL*>(nvm_firmware)->Address_Mapping_Unit->Translate_lpa_to_ppa_and_dispatch(writeback_transactions);

  auto sim_time = Simulator->Time();

  if (sim_time > next_bloom_filter_reset_milestone)//Reset control data structures used for hot/cold separation
  {
    bloom_filter[0].clear();
    next_bloom_filter_reset_milestone = sim_time + bloom_filter_reset_step;
  }
}

void
Data_Cache_Manager_Flash_Simple::service_dram_access_request(Memory_Transfer_Info& request_info)
{

  dram_execution_queue[request_info.Stream_id].push(&request_info);
  if(dram_execution_queue[request_info.Stream_id].size() == 1)
  {
    auto sim = Simulator;
    sim->Register_sim_event(sim->Time() + estimate_dram_access_time(request_info.Size_in_bytes, dram_row_size,
      dram_busrt_size, dram_burst_transfer_time_ddr, dram_tRCD, dram_tCL, dram_tRP),
      this, &request_info, static_cast<int>(request_info.next_event_type));
  }
}

void
Data_Cache_Manager_Flash_Simple::__handle_transaction_service(NVM_Transaction_Flash* transaction)
{
  //First check if the transaction source is a user request or the cache itself
  if (transaction->Source != Transaction_Source_Type::USERIO && transaction->Source != Transaction_Source_Type::CACHE)
    return;

  if (transaction->Source == Transaction_Source_Type::USERIO)
    broadcast_user_memory_transaction_serviced_signal(transaction);
  /* This is an update read (a read that is generated for a write request that partially updates page data).
  *  An update read transaction is issued in Address Mapping Unit, but is consumed in data cache manager.*/
  if (transaction->Type == Transaction_Type::READ)
  {
    if (((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite != nullptr)
    {
      ((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->RelatedRead = nullptr;
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
      default:
      PRINT_ERROR("The specified caching mode is not not support in simple cache manager!")
    }
  }
  else//This is a write request
  {
    switch (caching_mode_per_input_stream[transaction->Stream_id])
    {
      case Caching_Mode::TURNED_OFF:
        transaction->UserIORequest->Transaction_list.remove(transaction);
        if (transaction->UserIORequest->is_finished())
          broadcast_user_request_serviced_signal(transaction->UserIORequest);
        break;
      case Caching_Mode::WRITE_CACHE:
      {
        back_pressure_buffer_depth -= transaction->Data_and_metadata_size_in_byte / SECTOR_SIZE_IN_BYTE + (transaction->Data_and_metadata_size_in_byte % SECTOR_SIZE_IN_BYTE == 0 ? 0 : 1);


        if (data_cache->Exists(transaction->Stream_id, ((NVM_Transaction_Flash_WR*)transaction)->LPA))
        {
          Data_Cache_Slot_Type slot = data_cache->Get_slot(transaction->Stream_id, ((NVM_Transaction_Flash_WR*)transaction)->LPA);
          sim_time_type timestamp = slot.Timestamp;
          // Comment out unused variables.
          // NVM::memory_content_type content = slot.Content;
          if (((NVM_Transaction_Flash_WR*)transaction)->DataTimeStamp >= timestamp)
            data_cache->Remove_slot(transaction->Stream_id, ((NVM_Transaction_Flash_WR*)transaction)->LPA);
        }


        for (uint32_t i = 0; i < stream_count; i++)
        {
          request_queue_turn++;
          request_queue_turn %= stream_count;
          if (!waiting_user_requests_queue_for_dram_free_slot[request_queue_turn].empty())
          {
            auto user_request = waiting_user_requests_queue_for_dram_free_slot[request_queue_turn].begin();
            write_to_destage_buffer(**user_request);
            if ((*user_request)->Transaction_list.empty())
              waiting_user_requests_queue_for_dram_free_slot[request_queue_turn].remove(*user_request);
            if (back_pressure_buffer_depth >= back_pressure_buffer_max_depth)//The traffic load on the backend is high and the waiting requests cannot be serviced
              break;
          }
        }

        break;
      }
      default:
      PRINT_ERROR("The specified caching mode is not not support in simple cache manager!")
    }
  }
}

void
Data_Cache_Manager_Flash_Simple::Execute_simulator_event(MQSimEngine::SimEvent* ev)
{
  auto eventType = (Data_Cache_Simulation_Event_Type)ev->Type;
  auto* transfer_inf = (Memory_Transfer_Info*)ev->Parameters;

  switch (eventType)
  {
  case Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_USERIO_FINISHED://A user read is service from DRAM cache
  case Data_Cache_Simulation_Event_Type::MEMORY_WRITE_FOR_USERIO_FINISHED:
    ((User_Request*)(transfer_inf)->Related_request)->Sectors_serviced_from_cache -= transfer_inf->Size_in_bytes / SECTOR_SIZE_IN_BYTE;
    if (((User_Request*)(transfer_inf)->Related_request)->is_finished())
      broadcast_user_request_serviced_signal(((User_Request*)(transfer_inf)->Related_request));
    break;
  case Data_Cache_Simulation_Event_Type::MEMORY_READ_FOR_CACHE_EVICTION_FINISHED://Reading data from DRAM and writing it back to the flash storage
    static_cast<FTL*>(nvm_firmware)->Address_Mapping_Unit->Translate_lpa_to_ppa_and_dispatch(*((std::list<NVM_Transaction*>*)(transfer_inf->Related_request)));
    delete (std::list<NVM_Transaction*>*)transfer_inf->Related_request;
    break;
  case Data_Cache_Simulation_Event_Type::MEMORY_WRITE_FOR_CACHE_FINISHED://The recently read data from flash is written back to memory to support future user read requests
    break;
  }

  dram_execution_queue[transfer_inf->Stream_id].pop();
  if (!dram_execution_queue[transfer_inf->Stream_id].empty())
  {
    Memory_Transfer_Info* new_transfer_info = dram_execution_queue[transfer_inf->Stream_id].front();

    auto sim = Simulator;

    sim->Register_sim_event(sim->Time() + estimate_dram_access_time(new_transfer_info->Size_in_bytes, dram_row_size, dram_busrt_size,
      dram_burst_transfer_time_ddr, dram_tRCD, dram_tCL, dram_tRP),
      this, new_transfer_info, static_cast<int>(new_transfer_info->next_event_type));
  }
  delete transfer_inf;
}

void
Data_Cache_Manager_Flash_Simple::Setup_triggers()
{
  Data_Cache_Manager_Base::Setup_triggers();

  flash_controller->connect_to_transaction_service_signal(__user_transaction_handler);
}
