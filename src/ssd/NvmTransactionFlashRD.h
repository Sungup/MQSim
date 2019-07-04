#ifndef NVM_TRANSACTION_FLASH_RD_H
#define NVM_TRANSACTION_FLASH_RD_H

#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "../utils/InlineTools.h"
#include "../utils/ObjectPool.h"

#include "NvmTransactionFlash.h"

namespace SSD_Components
{
  class NvmTransactionFlashWR;

  class NvmTransactionFlashRD : public NvmTransactionFlash {
  public:
    // The content of this transaction
    NVM::memory_content_type Content;

    page_status_type read_sectors_bitmap;
    data_timestamp_type DataTimeStamp;

    // Is this read request related to another write request and provides
    // update data (for partial page write)
    NvmTransactionFlashWR* RelatedWrite;

  public:
    NvmTransactionFlashRD(Transaction_Source_Type source,
                          stream_id_type stream_id,
                          uint32_t data_size_in_byte,
                          SSD_Components::UserRequest* related_user_IO_request,
                          NVM::memory_content_type content,
                          page_status_type read_sectors_bitmap,
                          data_timestamp_type data_timestamp,
                          LPA_type lpa = NO_LPA,
                          PPA_type ppa = NO_PPA,
                          NvmTransactionFlashWR* related_write = nullptr);

    // Constructor for GC
    NvmTransactionFlashRD(Transaction_Source_Type source,
                          stream_id_type stream_id,
                          uint32_t data_size_in_byte,
                          const NVM::FlashMemory::Physical_Page_Address& address,
                          PPA_type ppa);

    ~NvmTransactionFlashRD() override = default;
  };

  force_inline
  NvmTransactionFlashRD::NvmTransactionFlashRD(Transaction_Source_Type source,
                                               stream_id_type stream_id,
                                               uint32_t data_size_in_byte,
                                               SSD_Components::UserRequest* related_user_IO_request,
                                               NVM::memory_content_type content,
                                               page_status_type read_sectors_bitmap,
                                               data_timestamp_type data_timestamp,
                                               LPA_type lpa,
                                               PPA_type ppa,
                                               NvmTransactionFlashWR* related_write)
    : NvmTransactionFlash(source,
                          Transaction_Type::READ,
                          stream_id,
                          data_size_in_byte,
                          lpa,
                          ppa,
                          related_user_IO_request),
      Content(content),
      read_sectors_bitmap(read_sectors_bitmap),
      DataTimeStamp(data_timestamp),
      RelatedWrite(related_write)
  { }

  // Constructor for GC
  force_inline
  NvmTransactionFlashRD::NvmTransactionFlashRD(Transaction_Source_Type source,
                                               stream_id_type stream_id,
                                               uint32_t data_size_in_byte,
                                               const NVM::FlashMemory::Physical_Page_Address& address,
                                               PPA_type ppa)
    : NvmTransactionFlash(source,
                          Transaction_Type::READ,
                          stream_id,
                          data_size_in_byte,
                          NO_LPA,
                          ppa,
                          address,
                          nullptr),
      Content(0),
      read_sectors_bitmap(0),
      DataTimeStamp(INVALID_TIME_STAMP),
      RelatedWrite(nullptr)
  { }
}

#endif