#ifndef DATA_CACHE_FLASH_H
#define DATA_CACHE_FLASH_H

#include <list>
#include <queue>
#include <unordered_map>
#include "../../nvm_chip/flash_memory/FlashTypes.h"
#include "../SSD_Defs.h"
#include "Data_Cache_Manager_Base.h"
#include "../NvmTransactionFlash.h"

// Refined header list
#include "DataCacheSlot.h"
#include "../../utils/InlineTools.h"
#include "../../utils/ObjectPool.h"

namespace SSD_Components
{
  class Data_Cache_Flash
  {
  public:
    Data_Cache_Flash(uint32_t capacity_in_pages = 0);
    ~Data_Cache_Flash();
    bool Exists(const stream_id_type streamID, const LPA_type lpn);
    bool Check_free_slot_availability();
    bool Check_free_slot_availability(uint32_t no_of_slots);
    bool Empty();
    bool Full();
    Data_Cache_Slot_Type Get_slot(const stream_id_type stream_id, const LPA_type lpn);
    Data_Cache_Slot_Type Evict_one_dirty_slot();
    Data_Cache_Slot_Type Evict_one_slot_lru();
    void Change_slot_status_to_writeback(const stream_id_type stream_id, const LPA_type lpn);
    void Remove_slot(const stream_id_type stream_id, const LPA_type lpn);
    void Insert_read_data(const stream_id_type stream_id, const LPA_type lpn, const data_cache_content_type content, const data_timestamp_type timestamp, const page_status_type state_bitmap_of_read_sectors);
    void Insert_write_data(const stream_id_type stream_id, const LPA_type lpn, const data_cache_content_type content, const data_timestamp_type timestamp, const page_status_type state_bitmap_of_write_sectors);
    void Update_data(const stream_id_type stream_id, const LPA_type lpn, const data_cache_content_type content, const data_timestamp_type timestamp, const page_status_type state_bitmap_of_write_sectors);
  private:
    std::unordered_map<LPA_type, Data_Cache_Slot_Type*> slots;
    std::list<std::pair<LPA_type, Data_Cache_Slot_Type*>> lru_list;
    uint32_t capacity_in_pages;
  };
}

#endif // !DATA_CACHE_FLASH_H
