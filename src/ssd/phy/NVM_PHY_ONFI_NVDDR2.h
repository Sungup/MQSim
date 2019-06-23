#ifndef NVM_PHY_ONFI_NVDDR2_H
#define NVM_PHY_ONFI_NVDDR2_H

#include <queue>
#include <list>
#include "../../sim/Sim_Defs.h"
#include "../../nvm_chip/flash_memory/FlashTypes.h"
#include "../../nvm_chip/flash_memory/Flash_Command.h"
#include "NVM_PHY_ONFI.h"
#include "../ONFI_Channel_NVDDR2.h"
#include "../Flash_Transaction_Queue.h"

namespace SSD_Components
{
  enum class NVDDR2_SimEventType
  {
    READ_DATA_TRANSFERRED,
    READ_CMD_ADDR_TRANSFERRED,
    PROGRAM_CMD_ADDR_DATA_TRANSFERRED, 
    PROGRAM_COPYBACK_CMD_ADDR_TRANSFERRED, 
    ERASE_SETUP_COMPLETED
  };

  // ------------------------------------
  // DieBookKeepingEntry Class Definition
  // ------------------------------------
  class DieBookKeepingEntry
  {
  public:
    /*
     * The current transactions that are being serviced. For the set of
     * transactions in __active_transactions, there is one __active_cmd that is
     * getting executed on the die. Transaction is a FTL-level concept, and
     * command is a flash chip-level concept
     */
    // The current command that is executing on the die
    NVM::FlashMemory::Flash_Command* __active_cmd;
    std::list<NVM_Transaction_Flash*> __active_transactions;

    NVM::FlashMemory::Flash_Command* __suspended_cmd;
    std::list<NVM_Transaction_Flash*> __suspended_transactions;

    // The current transaction
    NVM_Transaction_Flash* __active_transfer;

    bool __free;

  private:
    bool __suspended;

  public:
    sim_time_type __expected_finish_time;
    sim_time_type __remaining_exec_time;

    // If the command transfer is done in die-interleaved mode, the transfer
    // time is recorded in this temporary variable.
    sim_time_type __die_interleaved_time;

  public:
    DieBookKeepingEntry();

    // TODO Make complete destructor to avoid memory leak
    ~DieBookKeepingEntry() = default;

    void PrepareSuspend();
    void PrepareResume();
    void ClearCommand();

    bool free() const;
    bool suspended() const;
  };

  typedef std::vector<DieBookKeepingEntry>  DieBookKeepingList;
  typedef std::list<DieBookKeepingEntry*> DieBookKeepingPtrList;

  // -------------------------------------
  // ChipBookKeepingEntry Class Definition
  // -------------------------------------
  class ChipBookKeepingEntry
  {
  public:
    ChipStatus Status;
    DieBookKeepingList Die_book_keeping_records;
    sim_time_type Expected_command_exec_finish_time;
    sim_time_type Last_transfer_finish_time;
    bool HasSuspend;
    std::queue<DieBookKeepingEntry*> OngoingDieCMDTransfers;
    uint32_t WaitingReadTXCount;
    uint32_t No_of_active_dies;

    explicit ChipBookKeepingEntry(uint32_t dies_per_chip);

    void PrepareSuspend();
    void PrepareResume();
  };

  typedef std::vector<ChipBookKeepingEntry> ChipBookKeepingList;

  // ------------------------------------
  // NVM_PHY_ONFI_NVDDR2 Class Definition
  // ------------------------------------
  class NVM_PHY_ONFI_NVDDR2 : public NVM_PHY_ONFI
  {
  private:
    NVM::FlashMemory::ChipReadySignalHandler<NVM_PHY_ONFI_NVDDR2> __ready_signal_handler;

    ONFI_Channel_NVDDR2** channels;

    std::vector<ChipBookKeepingList> bookKeepingTable;

    FlashTransactionQueueList WaitingReadTX;
    FlashTransactionQueueList WaitingGCRead_TX;
    FlashTransactionQueueList WaitingMappingRead_TX;
    std::vector<DieBookKeepingPtrList> WaitingCopybackWrites;

    void transfer_read_data_from_chip(ChipBookKeepingEntry& chipBKE,
                                      DieBookKeepingEntry& dieBKE,
                                      NVM_Transaction_Flash* tr);

    void perform_interleaved_cmd_data_transfer(NVM::FlashMemory::Flash_Chip& chip,
                                               DieBookKeepingEntry& bookKeepingEntry);

    void send_resume_command_to_chip(NVM::FlashMemory::Flash_Chip& chip,
                                     ChipBookKeepingEntry& chipBKE);

    void __handle_ready_from_chip(NVM::FlashMemory::Flash_Chip& chip, NVM::FlashMemory::Flash_Command& command);

  public:
    NVM_PHY_ONFI_NVDDR2(const sim_object_id_type& id,
                        ONFI_Channel_NVDDR2** channels,
                        Stats& stats,
                        uint32_t ChannelCount,
                        uint32_t chip_no_per_channel,
                        uint32_t DieNoPerChip,
                        uint32_t PlaneNoPerDie);

    ~NVM_PHY_ONFI_NVDDR2() final = default;

    void Setup_triggers() final;

    /// 1. Override from NVM_PHY_ONFI
    BusChannelStatus Get_channel_status(flash_channel_ID_type channelID) final;
    NVM::FlashMemory::Flash_Chip* Get_chip(flash_channel_ID_type channel_id,
                                           flash_chip_ID_type chip_id) final;

    // A simplification to decrease the complexity of GC execution! The GC unit
    // may need to know the metadata of a page to decide if a page is valid
    // or invalid.
    LPA_type Get_metadata(flash_channel_ID_type channe_id,
                          flash_chip_ID_type chip_id,
                          flash_die_ID_type die_id,
                          flash_plane_ID_type plane_id,
                          flash_block_ID_type block_id,
                          flash_page_ID_type page_id) final;

    ChipStatus GetChipStatus(const NVM::FlashMemory::Flash_Chip& chip) final;

    bool HasSuspendedCommand(const NVM::FlashMemory::Flash_Chip& chip) final;

    sim_time_type Expected_finish_time(const NVM::FlashMemory::Flash_Chip& chip) final;
    sim_time_type Expected_finish_time(NVM_Transaction_Flash* transaction) final;
    sim_time_type Expected_transfer_time(NVM_Transaction_Flash* transaction) final;

    void Send_command_to_chip(std::list<NVM_Transaction_Flash*>& transactionList) final;

    void Change_flash_page_status_for_preconditioning(const NVM::FlashMemory::Physical_Page_Address& page_address,
                                                      LPA_type lpa) final;

    /// 2. Override from NVM_PHY_Base
    void Change_memory_status_preconditioning(const NVM::NVM_Memory_Address* address,
                                              const void* status_info) final;

    /// 3. Override from Sim_Object
    void Execute_simulator_event(MQSimEngine::Sim_Event*) final;

    /// 4. Non-override functions
    NVM_Transaction_Flash* Is_chip_busy_with_stream(NVM_Transaction_Flash* transaction);
    bool Is_chip_busy(NVM_Transaction_Flash* transaction);
  };

  force_inline NVM_Transaction_Flash*
  NVM_PHY_ONFI_NVDDR2::Is_chip_busy_with_stream(NVM_Transaction_Flash* transaction)
  {
    auto& chipBKE = bookKeepingTable[transaction->Address.ChannelID][transaction->Address.ChipID];
    stream_id_type stream_id = transaction->Stream_id;

    for (uint32_t die_id = 0; die_id < die_no_per_chip; die_id++)
      for (auto &tr : chipBKE.Die_book_keeping_records[die_id].__active_transactions)
        if (tr->Stream_id == stream_id)
          return tr;

    return nullptr;
  }

  force_inline bool
  NVM_PHY_ONFI_NVDDR2::Is_chip_busy(NVM_Transaction_Flash* transaction)
  {
    auto& chipBKE = bookKeepingTable[transaction->Address.ChannelID][transaction->Address.ChipID];
    return (chipBKE.Status != ChipStatus::IDLE);
  }
}

#endif // !NVM_PHY_ONFI_NVDDR2_H
