#ifndef NVM_TRANSACTION_FLASH_H
#define NVM_TRANSACTION_FLASH_H

#include <cstdint>
#include <list>
#include <string>

#include "../nvm_chip/flash_memory/Flash_Chip.h"
#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "../nvm_chip/flash_memory/Physical_Page_Address.h"
#include "../sim/Engine.h"
#include "../sim/Sim_Defs.h"
#include "../sim/SimEvent.h"

#include "request/UserRequest.h"

#include "NvmTransaction.h"

namespace SSD_Components
{
  class NvmTransactionFlash : public NvmTransaction {
  public:
    NVM::FlashMemory::Physical_Page_Address Address;

    // number of bytes contained in the request: bytes in the real page + bytes
    // of metadata
    uint32_t Data_and_metadata_size_in_byte;

    LPA_type LPA;
    PPA_type PPA;

#if UNBLOCK_NOT_IN_USE
    // Used in scheduling methods, such as FLIN, where fairness and QoS is
    // considered in scheduling
    sim_time_type Estimated_alone_waiting_time;

    // Especially used in queue reordering inf FLIN scheduler
    bool FLIN_Barrier;
#endif

    bool SuspendRequired;
    bool Physical_address_determined;

  protected:
    NvmTransactionFlash(Transaction_Source_Type source,
                        Transaction_Type type,
                        stream_id_type stream_id,
                        uint32_t data_size_in_byte,
                        LPA_type lpa,
                        PPA_type ppa,
                        UserRequest* user_request);

    NvmTransactionFlash(Transaction_Source_Type source,
                        Transaction_Type type,
                        stream_id_type stream_id,
                        uint32_t data_size_in_byte,
                        LPA_type lpa,
                        PPA_type ppa,
                        const NVM::FlashMemory::Physical_Page_Address& address,
                        UserRequest* user_request);

  public:
    ~NvmTransactionFlash() override = default;
  };

  force_inline
  NvmTransactionFlash::NvmTransactionFlash(Transaction_Source_Type source,
                                           Transaction_Type type,
                                           stream_id_type stream_id,
                                           uint32_t data_size_in_byte,
                                           LPA_type lpa,
                                           PPA_type ppa,
                                           UserRequest* user_request)
    : NvmTransaction(stream_id, source, type, user_request),
      Data_and_metadata_size_in_byte(data_size_in_byte),
      LPA(lpa),
      PPA(ppa),
#if UNBLOCK_NOT_IN_USE
      FLIN_Barrier(false),
#endif
      SuspendRequired(false),
      Physical_address_determined(false)
  { }

  force_inline
  NvmTransactionFlash::NvmTransactionFlash(Transaction_Source_Type source,
                                           Transaction_Type type,
                                           stream_id_type stream_id,
                                           uint32_t data_size_in_byte,
                                           LPA_type lpa,
                                           PPA_type ppa,
                                           const NVM::FlashMemory::Physical_Page_Address& address,
                                           UserRequest* user_request)
    : NvmTransaction(stream_id, source, type, user_request),
      Address(address),
      Data_and_metadata_size_in_byte(data_size_in_byte),
      LPA(lpa),
      PPA(ppa),
#if UNBLOCK_NOT_IN_USE
      FLIN_Barrier(false),
#endif
      SuspendRequired(false),
      Physical_address_determined(false)
  { }
}

#endif // !FLASH_TRANSACTION_H
