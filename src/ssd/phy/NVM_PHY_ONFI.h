#ifndef NVM_PHY_ONFI_ONFI_H
#define NVM_PHY_ONFI_ONFI_H

#include <vector>
#include "../../nvm_chip/flash_memory/FlashCommand.h"
#include "../../nvm_chip/flash_memory/Flash_Chip.h"
#include "../NVM_Transaction_Flash.h"
#include "../NVM_Transaction_Flash_RD.h"
#include "../NVM_Transaction_Flash_WR.h"
#include "../NVM_Transaction_Flash_ER.h"
#include "NVM_PHY_Base.h"
#include "../ONFI_Channel_Base.h"
#include "../Stats.h"

#include "PhyHandler.h"

namespace SSD_Components
{
  enum class ChipStatus {
    IDLE,
    CMD_IN,
    CMD_DATA_IN,
    DATA_OUT,
    READING,
    WRITING,
    ERASING,
    WAIT_FOR_DATA_OUT,
    WAIT_FOR_COPYBACK_CMD
  };

  class NVM_PHY_ONFI : public NVM_PHY_Base
  {
  protected:
    Stats& __stats;
    uint32_t channel_count;
    uint32_t chip_no_per_channel;
    uint32_t die_no_per_chip;
    uint32_t plane_no_per_die;

    NVM::FlashMemory::FlashCommandPool __cmd_pool;

    FlashTransactionHandlerList  __transaction_service_handlers;
    ChannelIdleSignalHandlerList __channel_idle_signal_handlers;
    ChipIdleSignalHandlerList    __chip_idle_signal_handlers;

    void broadcastTransactionServicedSignal(NVM_Transaction_Flash* transaction);
    void broadcastChannelIdleSignal(flash_channel_ID_type);
    void broadcastChipIdleSignal(NVM::FlashMemory::Flash_Chip& chip);

  public:
    NVM_PHY_ONFI(const sim_object_id_type& id,
                 Stats& stats,
                 uint32_t ChannelCount,
                 uint32_t chip_no_per_channel,
                 uint32_t DieNoPerChip,
                 uint32_t PlaneNoPerDie);
    ~NVM_PHY_ONFI() override = default;

    void connect_to_transaction_service_signal(FlashTransactionHandlerBase& handler);
    void connect_to_channel_idle_signal(ChannelIdleSignalHandlerBase& handler);
    void connect_to_chip_idle_signal(ChipIdleSignalHandlerBase& handler);

    virtual BusChannelStatus Get_channel_status(flash_channel_ID_type) = 0;

    virtual NVM::FlashMemory::Flash_Chip* Get_chip(flash_channel_ID_type channel_id,
                                                   flash_chip_ID_type chip_id) = 0;

    // A simplification to decrease the complexity of GC execution! The GC unit
    // may need to know the metadata of a page to decide if a page is valid
    // or invalid.
    virtual LPA_type Get_metadata(flash_channel_ID_type channe_id,
                                  flash_chip_ID_type chip_id,
                                  flash_die_ID_type die_id,
                                  flash_plane_ID_type plane_id,
                                  flash_block_ID_type block_id,
                                  flash_page_ID_type page_id) = 0;

    virtual ChipStatus GetChipStatus(const NVM::FlashMemory::Flash_Chip& chip) = 0;

    virtual bool HasSuspendedCommand(const NVM::FlashMemory::Flash_Chip& chip) = 0;

    virtual sim_time_type Expected_finish_time(const NVM::FlashMemory::Flash_Chip& chip) = 0;
    virtual sim_time_type Expected_finish_time(NVM_Transaction_Flash* transaction) = 0;
    virtual sim_time_type Expected_transfer_time(NVM_Transaction_Flash* transaction) = 0;

    // Provides communication between controller and NVM chips for a simple
    // read/write/erase command.
    virtual void Send_command_to_chip(std::list<NVM_Transaction_Flash*>& transactionList) = 0;

    virtual void Change_flash_page_status_for_preconditioning(const NVM::FlashMemory::Physical_Page_Address& page_address,
                                                              LPA_type lpa) = 0;

  };

  force_inline void
  NVM_PHY_ONFI::broadcastTransactionServicedSignal(NVM_Transaction_Flash* transaction)
  {
    for (auto& handler : __transaction_service_handlers)
      (*handler)(transaction);

    // This transaction has been consumed and no more needed
    delete transaction;
  }

  force_inline void
  NVM_PHY_ONFI::broadcastChannelIdleSignal(flash_channel_ID_type channelID)
  {
    for (auto& handler : __channel_idle_signal_handlers)
      (*handler)(channelID);
  }

  force_inline void
  NVM_PHY_ONFI::broadcastChipIdleSignal(NVM::FlashMemory::Flash_Chip& chip)
  {
    for (auto& handler : __chip_idle_signal_handlers)
      (*handler)(chip);
  }

  force_inline void
  NVM_PHY_ONFI::connect_to_transaction_service_signal(FlashTransactionHandlerBase& handler)
  {
    __transaction_service_handlers.emplace_back(&handler);
  }

  /*
   * Different FTL components maybe waiting for a transaction to be finished:
   * HostInterface: For user reads and writes
   * Address_Mapping_Unit: For mapping reads and writes
   * TSU: For the reads that must be finished for partial writes
   *      (first read non updated parts of page data and then merge and write
   *      them into the new page)
   * GarbageCollector: For gc reads, writes, and erases
   */
  force_inline void
  NVM_PHY_ONFI::connect_to_channel_idle_signal(ChannelIdleSignalHandlerBase &handler)
  {
    __channel_idle_signal_handlers.emplace_back(&handler);
  }

  force_inline void
  NVM_PHY_ONFI::connect_to_chip_idle_signal(SSD_Components::ChipIdleSignalHandlerBase &handler)
  {
    __chip_idle_signal_handlers.emplace_back(&handler);
  }

}


#endif // !NVM_PHY_ONFI_ONFI_H
