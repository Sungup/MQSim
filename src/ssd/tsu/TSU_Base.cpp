#include "TSU_Base.h"

#include "../FTL.h"

// Children classes for builder
#include "TSU_OutofOrder.h"

using namespace SSD_Components;

TSU_Base::TSU_Base(const sim_object_id_type& id,
                   FTL& ftl,
                   NVM_PHY_ONFI& NVMController,
                   Flash_Scheduling_Type Type,
                   uint32_t ChannelCount,
                   uint32_t chip_no_per_channel,
                   uint32_t DieNoPerChip,
                   uint32_t PlaneNoPerDie,
                   bool EraseSuspensionEnabled,
                   bool ProgramSuspensionEnabled,
                   sim_time_type WriteReasonableSuspensionTimeForRead,
                   sim_time_type EraseReasonableSuspensionTimeForRead,
                   sim_time_type EraseReasonableSuspensionTimeForWrite)
  : Sim_Object(id),
#if UNBLOCK_NOT_IN_USE
    __transaction_service_handler(this, &TSU_Base::__handle_transaction_serviced_signal),
#endif
    __channel_idle_signal_handler(this, &TSU_Base::__handle_channel_idle_signal),
    __chip_idle_signal_handler(this, &TSU_Base::__handle_chip_idle_signal),
    __type(Type),
    __channel_rr_turn(ChannelCount, 0),
    channel_count(ChannelCount),
    chip_no_per_channel(chip_no_per_channel),
    die_no_per_chip(DieNoPerChip),
    plane_no_per_die(PlaneNoPerDie),
    eraseSuspensionEnabled(EraseSuspensionEnabled),
    programSuspensionEnabled(ProgramSuspensionEnabled),
    writeReasonableSuspensionTimeForRead(WriteReasonableSuspensionTimeForRead),
    eraseReasonableSuspensionTimeForRead(EraseReasonableSuspensionTimeForRead),
    eraseReasonableSuspensionTimeForWrite(EraseReasonableSuspensionTimeForWrite),
    ftl(ftl),
    _NVMController(NVMController),
    opened_scheduling_reqs(0)
{ }

void TSU_Base::Setup_triggers()
{
  Sim_Object::Setup_triggers();

#if UNBLOCK_NOT_IN_USE
  _NVMController->connect_to_transaction_service_signal(__transaction_service_handler);
#endif
  _NVMController.connect_to_channel_idle_signal(__channel_idle_signal_handler);
  _NVMController.connect_to_chip_idle_signal(__chip_idle_signal_handler);
}

#if UNBLOCK_NOT_IN_USE
void
TSU_Base::__handle_transaction_serviced_signal(NvmTransactionFlash* /* transaction */)
{ /* TSU does nothing. The generator of the transaction will handle it. */ }
#endif

void
TSU_Base::__handle_channel_idle_signal(flash_channel_ID_type channelID)
{
  _handle_idle_channel(channelID);
}

void
TSU_Base::__handle_chip_idle_signal(const NVM::FlashMemory::Flash_Chip& chip)
{
  if (_is_idle_channel(chip.ChannelID))
    _service_chip_transaction(chip);
}

TSUPtr
SSD_Components::build_tsu_object(const sim_object_id_type& id,
                                 const DeviceParameterSet& params,
                                 FTL& ftl,
                                 NVM_PHY_ONFI& nvm_controller)
{
  return std::make_shared<TSU_OutOfOrder>(id, ftl, nvm_controller,
                                          params.Flash_Channel_Count,
                                          params.Chip_No_Per_Channel,
                                          params.Flash_Parameters.Die_No_Per_Chip,
                                          params.Flash_Parameters.Plane_No_Per_Die,
                                          params.Preferred_suspend_write_time_for_read,
                                          params.Preferred_suspend_erase_time_for_read,
                                          params.Preferred_suspend_erase_time_for_write,
                                          params.Flash_Parameters.erase_suspension_support(),
                                          params.Flash_Parameters.program_suspension_support());
}