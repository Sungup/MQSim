#ifndef TSU_H
#define TSU_H

#include <list>
#include <memory>
#include "../../sim/Sim_Defs.h"
#include "../../sim/Sim_Object.h"
#include "../../nvm_chip/flash_memory/Flash_Chip.h"
#include "../../sim/Sim_Reporter.h"
#include "../FTL.h"
#include "../phy/NVM_PHY_ONFI_NVDDR2.h"
#include "../Flash_Transaction_Queue.h"

#include "../phy/PhyHandler.h"
#include "TSU_Defs.h"

namespace SSD_Components
{
  class FTL;
  class TSU_Base : public MQSimEngine::Sim_Object
  {
  public:
    TSU_Base(const sim_object_id_type& id, FTL* ftl, NVM_PHY_ONFI_NVDDR2* NVMController, Flash_Scheduling_Type Type,
      uint32_t Channel_no, uint32_t chip_no_per_channel, uint32_t DieNoPerChip, uint32_t PlaneNoPerDie,
      bool EraseSuspensionEnabled, bool ProgramSuspensionEnabled,
      sim_time_type WriteReasonableSuspensionTimeForRead,
      sim_time_type EraseReasonableSuspensionTimeForRead,
      sim_time_type EraseReasonableSuspensionTimeForWrite);
    virtual ~TSU_Base();
    void Setup_triggers();

    /*When an MQSim needs to send a set of transactions for execution, the following
    * three funcitons should be invoked in this order:
    * Prepare_for_transaction_submit()
    * Submit_transaction(transaction)
    * .....
    * Submit_transaction(transaction)
    * Schedule()
    *
    * The above mentioned mechanism helps to exploit die-level and plane-level parallelism.
    * More precisely, first the transactions are queued and then, when the Schedule function
    * is invoked, the TSU has that opportunity to schedule them together to exploit multiplane
    * and die-interleaved execution.
    */
    virtual void Prepare_for_transaction_submit() = 0;
    virtual void Submit_transaction(NVM_Transaction_Flash* transaction) = 0;
    
    /* Shedules the transactions currently stored in inputTransactionSlots. The transactions could
    * be mixes of reads, writes, and erases.
    */
    virtual void Schedule() = 0;

  private:
    FlashTransactionHandler<TSU_Base> __transaction_service_handler;
    ChannelIdleSignalHandler<TSU_Base>  __channel_idle_signal_handler;
    ChipIdleSignalHandler<TSU_Base>     __chip_idle_signal_handler;

    void __handle_transaction_serviced_signal(NVM_Transaction_Flash* transaction);
    void __handle_channel_idle_signal(flash_channel_ID_type channelID);
    void __handle_chip_idle_signal(const NVM::FlashMemory::Flash_Chip& chip);

  protected:
    bool _is_busy_channel(flash_channel_ID_type channelID);
    bool _is_idle_channel(flash_channel_ID_type channelID);

    void _service_chip_transaction(const NVM::FlashMemory::Flash_Chip& chip);

    NVM::FlashMemory::Flash_Chip& _get_chip_on_rr_turn(uint32_t channelID);
    void _update_rr_turn(uint32_t channelID);
    void _handle_idle_channel(flash_channel_ID_type channelID);

    FTL* ftl;
    NVM_PHY_ONFI_NVDDR2* _NVMController;
    Flash_Scheduling_Type type;
    uint32_t channel_count;
    uint32_t chip_no_per_channel;
    uint32_t die_no_per_chip;
    uint32_t plane_no_per_die;
    bool eraseSuspensionEnabled, programSuspensionEnabled;
    sim_time_type writeReasonableSuspensionTimeForRead;
    sim_time_type eraseReasonableSuspensionTimeForRead;//the time period 
    sim_time_type eraseReasonableSuspensionTimeForWrite;

  private:
    flash_chip_ID_type* __channel_rr_turn;//Used for round-robin service of the chips in channels

  protected:
    std::list<NVM_Transaction_Flash*> transaction_receive_slots;//Stores the transactions that are received for sheduling
    std::list<NVM_Transaction_Flash*> transaction_dispatch_slots;//Used to submit transactions to the channel controller
    virtual bool service_read_transaction(const NVM::FlashMemory::Flash_Chip& chip) = 0;
    virtual bool service_write_transaction(const NVM::FlashMemory::Flash_Chip& chip) = 0;
    virtual bool service_erase_transaction(const NVM::FlashMemory::Flash_Chip& chip) = 0;

    int opened_scheduling_reqs;

  };

  force_inline bool
  TSU_Base::_is_busy_channel(flash_channel_ID_type channelID)
  {
    return _NVMController->Get_channel_status(channelID) == BusChannelStatus::BUSY;
  }

  force_inline bool
  TSU_Base::_is_idle_channel(flash_channel_ID_type channelID)
  {
    return _NVMController->Get_channel_status(channelID) == BusChannelStatus::IDLE;
  }

  force_inline void
  TSU_Base::_service_chip_transaction(const NVM::FlashMemory::Flash_Chip& chip)
  {
    if (service_read_transaction(chip))  return;

    if (service_write_transaction(chip)) return;

    service_erase_transaction(chip);
  }

  force_inline NVM::FlashMemory::Flash_Chip&
  TSU_Base::_get_chip_on_rr_turn(uint32_t channelID)
  {
    return *(_NVMController->Get_chip(channelID, __channel_rr_turn[channelID]));
  }

  force_inline void
  TSU_Base::_update_rr_turn(uint32_t channelID)
  {
    auto chipID = flash_chip_ID_type(__channel_rr_turn[channelID] + 1);

    __channel_rr_turn[channelID] = chipID % chip_no_per_channel;
  }

  force_inline void
  TSU_Base::_handle_idle_channel(flash_channel_ID_type channelID)
  {
    for (uint32_t i = 0; i < chip_no_per_channel; i++) {
      auto& chip = _get_chip_on_rr_turn(channelID);

      // The TSU does not check if the chip is idle or not since it is possible
      // to suspend a busy chip and issue a new command
      _service_chip_transaction(chip);

      _update_rr_turn(channelID);

      // A transaction has been started, so TSU should stop searching for another chip
      if (_is_busy_channel(chip.ChannelID))
        break;
    }
  }

  typedef std::shared_ptr<TSU_Base> TSUPtr;
}

#endif //TSU_H