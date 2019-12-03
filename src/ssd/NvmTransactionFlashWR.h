#ifndef NVM_TRANSACTION_FLASH_WR
#define NVM_TRANSACTION_FLASH_WR

#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "../utils/InlineTools.h"

#include "dcm/DataCacheSlot.h"

#include "NvmTransactionFlash.h"

namespace SSD_Components
{
  class NvmTransactionFlashRD;
  class NvmTransactionFlashER;

  enum class WriteExecutionModeType { SIMPLE, COPYBACK };

  class NvmTransactionFlashWR : public NvmTransactionFlash
  {
  public:
    // The content of this transaction
    NVM::memory_content_type Content;

    // If this write request must be preceded by a read (for partial page
    // write), this variable is used to point to the corresponding read request
    NvmTransactionFlashRD* RelatedRead;
    NvmTransactionFlashER* RelatedErase;

    page_status_type write_sectors_bitmap;
    data_timestamp_type DataTimeStamp;
    WriteExecutionModeType ExecutionMode;

  public:
    NvmTransactionFlashWR(Transaction_Source_Type source,
                          stream_id_type stream_id,
                          uint32_t data_size_in_byte,
                          SSD_Components::UserRequest* user_io_request,
                          NVM::memory_content_type content,
                          page_status_type written_sectors_bitmap,
                          data_timestamp_type data_timestamp,
                          LPA_type lpa,
                          PPA_type ppa = NO_PPA,
                          NvmTransactionFlashRD* related_read = nullptr);

    // Constructor for GC
    NvmTransactionFlashWR(Transaction_Source_Type source,
                          stream_id_type stream_id,
                          uint32_t data_size_in_byte,
                          const NVM::FlashMemory::Physical_Page_Address& address,
                          NvmTransactionFlashRD* related_read);

    // Constructor for DCM
    NvmTransactionFlashWR(Transaction_Source_Type source,
                          stream_id_type stream_id,
                          uint32_t data_size_in_byte,
                          const Data_Cache_Slot_Type& evicted_slot);
  };

  force_inline
  NvmTransactionFlashWR::NvmTransactionFlashWR(Transaction_Source_Type source,
                                               stream_id_type stream_id,
                                               uint32_t data_size_in_byte,
                                               SSD_Components::UserRequest* user_io_request,
                                               NVM::memory_content_type content,
                                               page_status_type written_sectors_bitmap,
                                               data_timestamp_type data_timestamp,
                                               LPA_type lpa,
                                               PPA_type ppa,
                                               NvmTransactionFlashRD* related_read)
    : NvmTransactionFlash(source,
                          Transaction_Type::WRITE,
                          stream_id,
                          data_size_in_byte,
                          lpa,
                          ppa,
                          user_io_request),
      Content(content),
      RelatedRead(related_read),
      RelatedErase(nullptr),
      write_sectors_bitmap(written_sectors_bitmap),
      DataTimeStamp(data_timestamp),
      ExecutionMode(WriteExecutionModeType::SIMPLE)
  { }

  // Constructor for GC
  force_inline
  NvmTransactionFlashWR::NvmTransactionFlashWR(Transaction_Source_Type source,
                                               stream_id_type stream_id,
                                               uint32_t data_size_in_byte,
                                               const NVM::FlashMemory::Physical_Page_Address& address,
                                               NvmTransactionFlashRD* related_read)
    : NvmTransactionFlash(source,
                          Transaction_Type::WRITE,
                          stream_id,
                          data_size_in_byte,
                          NO_LPA,
                          NO_PPA,
                          address,
                          nullptr),
      Content(0),
      RelatedRead(related_read),
      RelatedErase(nullptr),
      write_sectors_bitmap(0),
      DataTimeStamp(INVALID_TIME_STAMP),
      ExecutionMode(WriteExecutionModeType::SIMPLE)
  { }

  force_inline
  NvmTransactionFlashWR::NvmTransactionFlashWR(Transaction_Source_Type source,
                                               stream_id_type stream_id,
                                               uint32_t data_size_in_byte,
                                               const Data_Cache_Slot_Type& evicted)
    : NvmTransactionFlash(source,
                          Transaction_Type::WRITE,
                          stream_id,
                          data_size_in_byte,
                          evicted.LPA,
                          NO_PPA,
                          nullptr),
      Content(evicted.Content),
      RelatedRead(nullptr),
      RelatedErase(nullptr),
      write_sectors_bitmap(evicted.State_bitmap_of_existing_sectors),
      DataTimeStamp(evicted.Timestamp),
      ExecutionMode(WriteExecutionModeType::SIMPLE)
  { }
}

#endif // !WRITE_TRANSACTION_H
