#ifndef ONFI_CHANNEL_BASE_H
#define ONFI_CHANNEL_BASE_H

#include "../nvm_chip/flash_memory/Flash_Chip.h"
#include "../utils/InlineTools.h"
#include "ONFIChannelDefs.h"

namespace SSD_Components {
  enum class BusChannelStatus {
    BUSY,
    IDLE
  };

  class ONFI_Channel_Base {
  public:
    flash_channel_ID_type ChannelID;

    NVM::FlashMemory::Flash_Chip** Chips;

    ONFI_Protocol Type;

  private:
    BusChannelStatus __status;

    NVM::FlashMemory::Flash_Chip* __current_active;

  public:
    ONFI_Channel_Base(flash_channel_ID_type channelID,
                      NVM::FlashMemory::Flash_Chip** flashChips,
                      ONFI_Protocol type);

    BusChannelStatus GetStatus();

    void SetStatus(BusChannelStatus new_status,
                   NVM::FlashMemory::Flash_Chip* target_chip);
  };

  force_inline BusChannelStatus
  ONFI_Channel_Base::GetStatus()
  {
    return __status;
  }

  force_inline void
  ONFI_Channel_Base::SetStatus(BusChannelStatus new_status,
                               NVM::FlashMemory::Flash_Chip* target_chip)
  {
#if RUN_EXCEPTION_CHECK
    if ((__status != new_status)
          // Currently this line is meaningless code because BusChannelStatus
          // can choose only IDLE or BUSY.
          // && (__status == BusChannelStatus::IDLE || __status == BusChannelStatus::BUSY)
          && (__current_active != target_chip))
      throw mqsim_error(std::string("Illegal bus status transition: Bus ")
                          + std::to_string(ChannelID));
#endif

    __status = new_status;

    __current_active = __status == BusChannelStatus::BUSY
                         ? target_chip
                         : nullptr;
  }
}

#endif // !CHANNEL_H
