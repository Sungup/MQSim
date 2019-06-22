#include <string>
#include "TSU_Base.h"

using namespace SSD_Components;

TSU_Base::TSU_Base(const sim_object_id_type& id, FTL* ftl, NVM_PHY_ONFI_NVDDR2* NVMController, Flash_Scheduling_Type Type,
  uint32_t ChannelCount, uint32_t chip_no_per_channel, uint32_t DieNoPerChip, uint32_t PlaneNoPerDie,
  bool EraseSuspensionEnabled, bool ProgramSuspensionEnabled,
  sim_time_type WriteReasonableSuspensionTimeForRead,
  sim_time_type EraseReasonableSuspensionTimeForRead,
  sim_time_type EraseReasonableSuspensionTimeForWrite)
  : Sim_Object(id),
    __transaction_service_handler(this, &TSU_Base::__handle_transaction_serviced_signal),
    __channel_idle_signal_handler(this, &TSU_Base::__handle_channel_idle_signal),
    __chip_idle_signal_handler(this, &TSU_Base::__handle_chip_idle_signal),
    ftl(ftl), _NVMController(NVMController), type(Type),
  channel_count(ChannelCount), chip_no_per_channel(chip_no_per_channel), die_no_per_chip(DieNoPerChip), plane_no_per_die(PlaneNoPerDie),
  eraseSuspensionEnabled(EraseSuspensionEnabled), programSuspensionEnabled(ProgramSuspensionEnabled),
  writeReasonableSuspensionTimeForRead(WriteReasonableSuspensionTimeForRead), eraseReasonableSuspensionTimeForRead(EraseReasonableSuspensionTimeForRead),
  eraseReasonableSuspensionTimeForWrite(EraseReasonableSuspensionTimeForWrite), opened_scheduling_reqs(0)
{
  __channel_rr_turn = new flash_chip_ID_type[channel_count];
  for (uint32_t channelID = 0; channelID < channel_count; channelID++)
    __channel_rr_turn[channelID] = 0;
}

TSU_Base::~TSU_Base()
{
  delete[] __channel_rr_turn;
}

void TSU_Base::Setup_triggers()
{
  Sim_Object::Setup_triggers();

  _NVMController->connect_to_transaction_service_signal(__transaction_service_handler);
  _NVMController->connect_to_channel_idle_signal(__channel_idle_signal_handler);
  _NVMController->connect_to_chip_idle_signal(__chip_idle_signal_handler);
}

force_inline void
TSU_Base::__handle_transaction_serviced_signal(NVM_Transaction_Flash* /* transaction */)
{ /* TSU does nothing. The generator of the transaction will handle it. */ }

force_inline void
TSU_Base::__handle_channel_idle_signal(flash_channel_ID_type channelID)
{
  _handle_idle_channel(channelID);
}

force_inline void
TSU_Base::__handle_chip_idle_signal(const NVM::FlashMemory::Flash_Chip& chip)
{
  if (_is_idle_channel(chip.ChannelID))
    _service_chip_transaction(chip);
}

