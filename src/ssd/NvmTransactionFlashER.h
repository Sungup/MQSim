#ifndef NVM_TRANSACTION_FLASH_ER_H
#define NVM_TRANSACTION_FLASH_ER_H

#include <list>
#include "../nvm_chip/flash_memory/FlashTypes.h"

#include "NvmTransactionFlash.h"

namespace SSD_Components
{
  class NvmTransactionFlashWR;

  class NvmTransactionFlashER : public NvmTransactionFlash {
  public:
    std::list<NvmTransactionFlashWR *> Page_movement_activities;

  public:
    NvmTransactionFlashER(Transaction_Source_Type source,
                          stream_id_type streamID,
                          const NVM::FlashMemory::Physical_Page_Address& address);

  };

  force_inline
  NvmTransactionFlashER::NvmTransactionFlashER(Transaction_Source_Type source,
                                               stream_id_type streamID,
                                               const NVM::FlashMemory::Physical_Page_Address& address)
    : NvmTransactionFlash(source,
                          Transaction_Type::ERASE,
                          streamID,
                          0,
                          NO_LPA,
                          NO_PPA,
                          address,
                          nullptr)
  { }
}

#endif // !ERASE_TRANSACTION_H
