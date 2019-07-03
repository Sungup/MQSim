#include "ONFI_Channel_Base.h"


using namespace SSD_Components;
ONFI_Channel_Base::ONFI_Channel_Base(flash_channel_ID_type channelID,
                                     NVM::FlashMemory::Flash_Chip** flashChips,
                                     ONFI_Protocol type)
  : ChannelID(channelID),
    Chips(flashChips),
    Type(type),
    __status(BusChannelStatus::IDLE),
    __current_active(nullptr)
{ }
