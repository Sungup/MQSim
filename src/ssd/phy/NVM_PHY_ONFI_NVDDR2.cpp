#include <stdexcept>
#include "../../sim/Engine.h"
#include "NVM_PHY_ONFI_NVDDR2.h"
#include "../Stats.h"

using namespace SSD_Components;

// ----------------------------------------
// DieBookKeepingEntry Class Implementation
// ----------------------------------------
force_inline
DieBookKeepingEntry::DieBookKeepingEntry()
  : __active_cmd(nullptr),
    __active_transactions(),
    __suspended_cmd(nullptr),
    __suspended_transactions(),
    __active_transfer(nullptr),
    __free(true),
    __suspended(false),
    __expected_finish_time(INVALID_TIME),
    __remaining_exec_time(INVALID_TIME),
    __die_interleaved_time(INVALID_TIME)
{ }

force_inline void
DieBookKeepingEntry::PrepareSuspend()
{
  __suspended_cmd = __active_cmd;
  __remaining_exec_time = __expected_finish_time - Simulator->Time();
  __suspended_transactions.insert(__suspended_transactions.begin(),
                                  __active_transactions.begin(),
                                  __active_transactions.end());
  __suspended = true;
  __active_cmd = nullptr;
  __active_transactions.clear();
  __free = true;
}

force_inline void
DieBookKeepingEntry::PrepareResume()
{
  __active_cmd = __suspended_cmd;
  __expected_finish_time = Simulator->Time() + __remaining_exec_time;
  __active_transactions.insert(__active_transactions.begin(),
                               __suspended_transactions.begin(),
                               __suspended_transactions.end());
  __suspended = false;
  __suspended_cmd = nullptr;
  __suspended_transactions.clear();
  __free = false;
}

force_inline void
DieBookKeepingEntry::ClearCommand()
{
  delete __active_cmd;
  __active_cmd = nullptr;
  __active_transactions.clear();
  __free = true;
}

force_inline bool
DieBookKeepingEntry::free() const
{
  return __free;
}

force_inline bool
DieBookKeepingEntry::suspended() const
{
  return __suspended;
}

// -------------------------------------
// ChipBookKeepingEntry Class Definition
// -------------------------------------

force_inline
ChipBookKeepingEntry::ChipBookKeepingEntry(uint32_t dies_per_chip)
  : Status(ChipStatus::IDLE),
    Die_book_keeping_records(dies_per_chip),
    Expected_command_exec_finish_time(T0),
    Last_transfer_finish_time(T0),
    HasSuspend(false),
    OngoingDieCMDTransfers(),
    WaitingReadTXCount(0),
    No_of_active_dies(0)
{ }

force_inline void
ChipBookKeepingEntry::PrepareSuspend()
{
  HasSuspend = true; No_of_active_dies = 0;
}

force_inline void
ChipBookKeepingEntry::PrepareResume() {
  HasSuspend = false;
}

// ------------------------------------
// NVM_PHY_ONFI_NVDDR2 Class Definition
// ------------------------------------
NVM_PHY_ONFI_NVDDR2::NVM_PHY_ONFI_NVDDR2(const sim_object_id_type& id,
                                         ONFI_Channel_NVDDR2** channels,
                                         Stats& stats,
                                         uint32_t ChannelCount,
                                         uint32_t chip_no_per_channel,
                                         uint32_t DieNoPerChip,
                                         uint32_t PlaneNoPerDie)
  : NVM_PHY_ONFI(id,
                 stats,
                 ChannelCount,
                 chip_no_per_channel,
                 DieNoPerChip,
                 PlaneNoPerDie),
    __ready_signal_handler(this, &NVM_PHY_ONFI_NVDDR2::__handle_ready_from_chip),
    channels(channels),
    bookKeepingTable(ChannelCount),
    WaitingReadTX(ChannelCount),
    WaitingGCRead_TX(ChannelCount),
    WaitingMappingRead_TX(ChannelCount),
    WaitingCopybackWrites(ChannelCount)
{
  for (uint32_t ch = 0; ch < channel_count; ++ch) {
    uint32_t i = 0;

    while (++i <= chip_no_per_channel)
      bookKeepingTable[ch].emplace_back(DieNoPerChip);
  }
}

force_inline void
NVM_PHY_ONFI_NVDDR2::transfer_read_data_from_chip(ChipBookKeepingEntry& chipBKE,
                                                  DieBookKeepingEntry& dieBKE,
                                                  NVM_Transaction_Flash* tr)
{
  auto sim = Simulator;

  dieBKE.__active_transfer = tr;
  channels[tr->Address.ChannelID]->Chips[tr->Address.ChipID]->StartDataOutXfer();
  chipBKE.Status = ChipStatus::DATA_OUT;

  sim->Register_sim_event(sim->Time() + NVDDR2DataOutTransferTime(tr->Data_and_metadata_size_in_byte, channels[tr->Address.ChannelID]),
                          this, &dieBKE, (int)NVDDR2_SimEventType::READ_DATA_TRANSFERRED);

  tr->STAT_transfer_time += NVDDR2DataOutTransferTime(tr->Data_and_metadata_size_in_byte, channels[tr->Address.ChannelID]);
  channels[tr->Address.ChannelID]->SetStatus(BusChannelStatus::BUSY, channels[tr->Address.ChannelID]->Chips[tr->Address.ChipID]);
}

force_inline void
NVM_PHY_ONFI_NVDDR2::perform_interleaved_cmd_data_transfer(NVM::FlashMemory::Flash_Chip& chip,
                                                           DieBookKeepingEntry& bookKeepingEntry)
{
  ONFI_Channel_NVDDR2* target_channel = channels[bookKeepingEntry.__active_transactions.front()->Address.ChannelID];
  /*if (target_channel->Status == BusChannelStatus::BUSY)
    PRINT_ERROR("Requesting communication on a busy bus!")*/

  auto sim = Simulator;

  switch (bookKeepingEntry.__active_transactions.front()->Type)
  {
    case Transaction_Type::READ:
      chip.StartCMDXfer();
      bookKeepingTable[chip.ChannelID][chip.ChipID].Status = ChipStatus::CMD_IN;
      sim->Register_sim_event(sim->Time() + bookKeepingEntry.__die_interleaved_time,
                              this, &bookKeepingEntry,
                              (int)NVDDR2_SimEventType::READ_CMD_ADDR_TRANSFERRED);
      break;
    case Transaction_Type::WRITE:
      if (((NVM_Transaction_Flash_WR*)bookKeepingEntry.__active_transactions.front())->RelatedRead == nullptr) {
        chip.StartCMDDataInXfer();
        bookKeepingTable[chip.ChannelID][chip.ChipID].Status = ChipStatus::CMD_DATA_IN;
        sim->Register_sim_event(sim->Time() + bookKeepingEntry.__die_interleaved_time,
                                this, &bookKeepingEntry,
                                (int)NVDDR2_SimEventType::PROGRAM_CMD_ADDR_DATA_TRANSFERRED);
      }
      else
      {
        chip.StartCMDXfer();
        bookKeepingTable[chip.ChannelID][chip.ChipID].Status = ChipStatus::CMD_IN;
        sim->Register_sim_event(sim->Time() + bookKeepingEntry.__die_interleaved_time,
                                this, &bookKeepingEntry,
                                (int)NVDDR2_SimEventType::READ_CMD_ADDR_TRANSFERRED);
      }
      break;

    case Transaction_Type::ERASE:
      chip.StartCMDXfer();
      bookKeepingTable[chip.ChannelID][chip.ChipID].Status = ChipStatus::CMD_IN;
      sim->Register_sim_event(sim->Time() + bookKeepingEntry.__die_interleaved_time,
                              this, &bookKeepingEntry,
                              (int)NVDDR2_SimEventType::ERASE_SETUP_COMPLETED);
      break;

    default:
    PRINT_ERROR("NVMController_NVDDR2: Uknown flash transaction type!")
  }
  target_channel->SetStatus(BusChannelStatus::BUSY, &chip);
}

force_inline void
NVM_PHY_ONFI_NVDDR2::send_resume_command_to_chip(NVM::FlashMemory::Flash_Chip& chip,
                                                 ChipBookKeepingEntry& chipBKE)
{
  //DEBUG2("Chip " << chip->ChannelID << ", " << chip->ChipID << ": resume command " )
  for (uint32_t i = 0; i < die_no_per_chip; i++)
  {
    DieBookKeepingEntry& dieBKE = chipBKE.Die_book_keeping_records[i];
    //Since the time required to send the resume command is very small, MQSim ignores it to simplify the simulation
    dieBKE.PrepareResume();
    chipBKE.PrepareResume();
    chip.Resume(dieBKE.__active_cmd->Address[0].DieID);
    switch (dieBKE.__active_cmd->CommandCode)
    {
      case CMD_READ_PAGE:
      case CMD_READ_PAGE_MULTIPLANE:
      case CMD_READ_PAGE_COPYBACK:
      case CMD_READ_PAGE_COPYBACK_MULTIPLANE:
        chipBKE.Status = ChipStatus::READING;
        break;
      case CMD_PROGRAM_PAGE:
      case CMD_PROGRAM_PAGE_MULTIPLANE:
      case CMD_PROGRAM_PAGE_COPYBACK:
      case CMD_PROGRAM_PAGE_COPYBACK_MULTIPLANE:
        chipBKE.Status = ChipStatus::WRITING;
        break;
      case CMD_ERASE_BLOCK:
      case CMD_ERASE_BLOCK_MULTIPLANE:
        chipBKE.Status = ChipStatus::ERASING;
        break;
    }
  }
}

void
NVM_PHY_ONFI_NVDDR2::Setup_triggers()
{
  Sim_Object::Setup_triggers();

  for (uint32_t i = 0; i < channel_count; i++)
    for (uint32_t j = 0; j < chip_no_per_channel; j++)
      channels[i]->Chips[j]->connect_to_chip_ready_signal(__ready_signal_handler);
}

inline BusChannelStatus NVM_PHY_ONFI_NVDDR2::Get_channel_status(flash_channel_ID_type channelID)
{
  return channels[channelID]->GetStatus();
}

inline NVM::FlashMemory::Flash_Chip* NVM_PHY_ONFI_NVDDR2::Get_chip(flash_channel_ID_type channelID, flash_chip_ID_type chipID)
{
  return channels[channelID]->Chips[chipID];
}

LPA_type NVM_PHY_ONFI_NVDDR2::Get_metadata(flash_channel_ID_type channe_id, flash_chip_ID_type chip_id, flash_die_ID_type die_id, flash_plane_ID_type plane_id, flash_block_ID_type block_id, flash_page_ID_type page_id)//A simplification to decrease the complexity of GC execution! The GC unit may need to know the metadata of a page to decide if a page is valid or invalid.
{
  return channels[channe_id]->Chips[chip_id]->Get_metadata(die_id, plane_id, block_id, page_id);
}

inline bool NVM_PHY_ONFI_NVDDR2::HasSuspendedCommand(const NVM::FlashMemory::Flash_Chip& chip)
{
  return bookKeepingTable[chip.ChannelID][chip.ChipID].HasSuspend;
}

inline ChipStatus NVM_PHY_ONFI_NVDDR2::GetChipStatus(const NVM::FlashMemory::Flash_Chip& chip)
{
  return bookKeepingTable[chip.ChannelID][chip.ChipID].Status;
}

inline sim_time_type NVM_PHY_ONFI_NVDDR2::Expected_finish_time(const NVM::FlashMemory::Flash_Chip& chip)
{
  return bookKeepingTable[chip.ChannelID][chip.ChipID].Expected_command_exec_finish_time;
}

sim_time_type NVM_PHY_ONFI_NVDDR2::Expected_finish_time(NVM_Transaction_Flash* transaction)
{
  auto& channel = channels[transaction->Address.ChannelID];

  return Expected_finish_time(*(channel->Chips[transaction->Address.ChipID]));
}


sim_time_type NVM_PHY_ONFI_NVDDR2::Expected_transfer_time(NVM_Transaction_Flash* transaction)
{
  return NVDDR2DataInTransferTime(transaction->Data_and_metadata_size_in_byte, channels[transaction->Address.ChannelID]);
}

void NVM_PHY_ONFI_NVDDR2::Change_flash_page_status_for_preconditioning(const NVM::FlashMemory::Physical_Page_Address& page_address, LPA_type lpa)
{
  channels[page_address.ChannelID]->Chips[page_address.ChipID]->Change_memory_status_preconditioning(&page_address, &lpa);
}

void NVM_PHY_ONFI_NVDDR2::Send_command_to_chip(std::list<NVM_Transaction_Flash*>& transaction_list)
{
  ONFI_Channel_NVDDR2* target_channel = channels[transaction_list.front()->Address.ChannelID];

  NVM::FlashMemory::Flash_Chip* targetChip = target_channel->Chips[transaction_list.front()->Address.ChipID];
  ChipBookKeepingEntry* chipBKE = &bookKeepingTable[transaction_list.front()->Address.ChannelID][transaction_list.front()->Address.ChipID];
  DieBookKeepingEntry* dieBKE = &chipBKE->Die_book_keeping_records[transaction_list.front()->Address.DieID];

  /*If this is not a die-interleaved command execution, and the channel is already busy,
  * then something illegarl is happening*/
  if (target_channel->GetStatus() == BusChannelStatus::BUSY && chipBKE->OngoingDieCMDTransfers.size() == 0)
    PRINT_ERROR("Bus " << target_channel->ChannelID << ": starting communication on a busy flash channel!");

  sim_time_type suspendTime = 0;
  if (!dieBKE->free())
  {
    if (transaction_list.front()->SuspendRequired)
    {
      switch (dieBKE->__active_transactions.front()->Type)
      {
      case Transaction_Type::WRITE:
        __stats.IssuedSuspendProgramCMD++;
        suspendTime = target_channel->ProgramSuspendCommandTime + targetChip->GetSuspendProgramTime();
        break;
      case Transaction_Type::ERASE:
        __stats.IssuedSuspendEraseCMD++;
        suspendTime = target_channel->EraseSuspendCommandTime + targetChip->GetSuspendEraseTime();
        break;
      default:
        PRINT_ERROR("Read suspension is not supported!")
      }
      targetChip->Suspend(transaction_list.front()->Address.DieID);
      dieBKE->PrepareSuspend();
      if (chipBKE->OngoingDieCMDTransfers.size())
        chipBKE->PrepareSuspend();
    }
    else PRINT_ERROR("Read suspension is not supported!")
  }

  dieBKE->__free = false;
  dieBKE->__active_cmd = new NVM::FlashMemory::Flash_Command();
  for (std::list<NVM_Transaction_Flash*>::iterator it = transaction_list.begin();
    it != transaction_list.end(); it++)
  {
    dieBKE->__active_transactions.push_back(*it);
    dieBKE->__active_cmd->Address.push_back((*it)->Address);
    NVM::FlashMemory::PageMetadata metadata;
    metadata.LPA = (*it)->LPA;
    dieBKE->__active_cmd->Meta_data.push_back(metadata);
  }

  auto sim = Simulator;

  switch (transaction_list.front()->Type)
  {
  case Transaction_Type::READ:
    if (transaction_list.size() == 1) {
      __stats.IssuedReadCMD++;
      dieBKE->__active_cmd->CommandCode = CMD_READ_PAGE;
      DEBUG("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << transaction_list.front()->Address.DieID << ": Sending read command to chip for LPA: " << transaction_list.front()->LPA)
    }
    else
    {
      __stats.IssuedMultiplaneReadCMD++;
      dieBKE->__active_cmd->CommandCode = CMD_READ_PAGE_MULTIPLANE;
      DEBUG("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << transaction_list.front()->Address.DieID << ": Sending multi-plane read command to chip for LPA: " << transaction_list.front()->LPA)
    }

    for (std::list<NVM_Transaction_Flash*>::iterator it = transaction_list.begin();
      it != transaction_list.end(); it++)
      (*it)->STAT_transfer_time += target_channel->ReadCommandTime[transaction_list.size()];

    if (chipBKE->OngoingDieCMDTransfers.size() == 0)
    {
      targetChip->StartCMDXfer();
      chipBKE->Status = ChipStatus::CMD_IN;
      chipBKE->Last_transfer_finish_time = sim->Time() + suspendTime + target_channel->ReadCommandTime[transaction_list.size()];
      sim->Register_sim_event(sim->Time() + suspendTime + target_channel->ReadCommandTime[transaction_list.size()], this,
        dieBKE, (int)NVDDR2_SimEventType::READ_CMD_ADDR_TRANSFERRED);
    }
    else
    {
      dieBKE->__die_interleaved_time = suspendTime + target_channel->ReadCommandTime[transaction_list.size()];
      chipBKE->Last_transfer_finish_time += suspendTime + target_channel->ReadCommandTime[transaction_list.size()];
    }
    chipBKE->OngoingDieCMDTransfers.push(dieBKE);

    dieBKE->__expected_finish_time = chipBKE->Last_transfer_finish_time + targetChip->Get_command_execution_latency(dieBKE->__active_cmd->CommandCode, dieBKE->__active_cmd->Address[0].PageID);
    if (chipBKE->Expected_command_exec_finish_time < dieBKE->__expected_finish_time)
      chipBKE->Expected_command_exec_finish_time = dieBKE->__expected_finish_time;
    break;
  case Transaction_Type::WRITE:
    if (((NVM_Transaction_Flash_WR*)transaction_list.front())->ExecutionMode == WriteExecutionModeType::SIMPLE)
    {
      if (transaction_list.size() == 1) {
        __stats.IssuedProgramCMD++;
        dieBKE->__active_cmd->CommandCode = CMD_PROGRAM_PAGE;
        DEBUG("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << transaction_list.front()->Address.DieID << ": Sending program command to chip for LPA: " << transaction_list.front()->LPA)
      }
      else
      {
        __stats.IssuedMultiplaneProgramCMD++;
        dieBKE->__active_cmd->CommandCode = CMD_PROGRAM_PAGE_MULTIPLANE;
        DEBUG("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << transaction_list.front()->Address.DieID << ": Sending multi-plane program command to chip for LPA: " << transaction_list.front()->LPA)
      }

      sim_time_type data_transfer_time = 0;

      for (std::list<NVM_Transaction_Flash*>::iterator it = transaction_list.begin();
        it != transaction_list.end(); it++) {
        (*it)->STAT_transfer_time += target_channel->ProgramCommandTime[transaction_list.size()] + NVDDR2DataInTransferTime((*it)->Data_and_metadata_size_in_byte, target_channel);
        data_transfer_time += NVDDR2DataInTransferTime((*it)->Data_and_metadata_size_in_byte, target_channel);
      }
      if (chipBKE->OngoingDieCMDTransfers.size() == 0)
      {
        targetChip->StartCMDDataInXfer();
        chipBKE->Status = ChipStatus::CMD_DATA_IN;
        chipBKE->Last_transfer_finish_time = sim->Time() + suspendTime + target_channel->ProgramCommandTime[transaction_list.size()] + data_transfer_time;
        sim->Register_sim_event(sim->Time() + suspendTime + target_channel->ProgramCommandTime[transaction_list.size()] + data_transfer_time,
          this, dieBKE, (int)NVDDR2_SimEventType::PROGRAM_CMD_ADDR_DATA_TRANSFERRED);
      }
      else
      {
        dieBKE->__die_interleaved_time = suspendTime + target_channel->ProgramCommandTime[transaction_list.size()] + data_transfer_time;
        chipBKE->Last_transfer_finish_time += suspendTime + target_channel->ProgramCommandTime[transaction_list.size()] + data_transfer_time;
      }
      chipBKE->OngoingDieCMDTransfers.push(dieBKE);

      dieBKE->__expected_finish_time = chipBKE->Last_transfer_finish_time + targetChip->Get_command_execution_latency(dieBKE->__active_cmd->CommandCode, dieBKE->__active_cmd->Address[0].PageID);
      if (chipBKE->Expected_command_exec_finish_time < dieBKE->__expected_finish_time)
        chipBKE->Expected_command_exec_finish_time = dieBKE->__expected_finish_time;
    }
    else//Copyback write for GC
    {
      if (transaction_list.size() == 1) {
        __stats.IssuedCopybackReadCMD++;
        dieBKE->__active_cmd->CommandCode = CMD_READ_PAGE_COPYBACK;
      }
      else {
        __stats.IssuedMultiplaneCopybackProgramCMD++;
        dieBKE->__active_cmd->CommandCode = CMD_READ_PAGE_COPYBACK_MULTIPLANE;
      }

      for (std::list<NVM_Transaction_Flash*>::iterator it = transaction_list.begin();
        it != transaction_list.end(); it++)
        (*it)->STAT_transfer_time += target_channel->ReadCommandTime[transaction_list.size()];
      if (chipBKE->OngoingDieCMDTransfers.size() == 0)
      {
        targetChip->StartCMDXfer();
        chipBKE->Status = ChipStatus::CMD_IN;
        chipBKE->Last_transfer_finish_time = sim->Time() + suspendTime + target_channel->ReadCommandTime[transaction_list.size()];
        sim->Register_sim_event(sim->Time() + suspendTime + target_channel->ReadCommandTime[transaction_list.size()], this,
          dieBKE, (int)NVDDR2_SimEventType::READ_CMD_ADDR_TRANSFERRED);
      }
      else
      {
        dieBKE->__die_interleaved_time = suspendTime + target_channel->ReadCommandTime[transaction_list.size()];
        chipBKE->Last_transfer_finish_time += suspendTime + target_channel->ReadCommandTime[transaction_list.size()];
      }
      chipBKE->OngoingDieCMDTransfers.push(dieBKE);

      dieBKE->__expected_finish_time = chipBKE->Last_transfer_finish_time + targetChip->Get_command_execution_latency(dieBKE->__active_cmd->CommandCode, dieBKE->__active_cmd->Address[0].PageID);
      if (chipBKE->Expected_command_exec_finish_time < dieBKE->__expected_finish_time)
        chipBKE->Expected_command_exec_finish_time = dieBKE->__expected_finish_time;
    }
    break;
  case Transaction_Type::ERASE:
    //DEBUG2("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << transaction_list.front()->Address.DieID << ": Sending erase command to chip")
    if (transaction_list.size() == 1) {
      __stats.IssuedEraseCMD++;
      dieBKE->__active_cmd->CommandCode = CMD_ERASE_BLOCK;
    }
    else
    {
      __stats.IssuedMultiplaneEraseCMD++;
      dieBKE->__active_cmd->CommandCode = CMD_ERASE_BLOCK_MULTIPLANE;
    }

    for (std::list<NVM_Transaction_Flash*>::iterator it = transaction_list.begin();
      it != transaction_list.end(); it++)
      (*it)->STAT_transfer_time += target_channel->EraseCommandTime[transaction_list.size()];
    if (chipBKE->OngoingDieCMDTransfers.size() == 0)
    {
      targetChip->StartCMDXfer();
      chipBKE->Status = ChipStatus::CMD_IN;
      chipBKE->Last_transfer_finish_time = sim->Time() + suspendTime + target_channel->EraseCommandTime[transaction_list.size()];
      sim->Register_sim_event(sim->Time() + suspendTime + target_channel->EraseCommandTime[transaction_list.size()],
        this, dieBKE, (int)NVDDR2_SimEventType::ERASE_SETUP_COMPLETED);
    }
    else
    {
      dieBKE->__die_interleaved_time = suspendTime + target_channel->EraseCommandTime[transaction_list.size()];
      chipBKE->Last_transfer_finish_time += suspendTime + target_channel->EraseCommandTime[transaction_list.size()];
    }
    chipBKE->OngoingDieCMDTransfers.push(dieBKE);

    dieBKE->__expected_finish_time = chipBKE->Last_transfer_finish_time + targetChip->Get_command_execution_latency(dieBKE->__active_cmd->CommandCode, dieBKE->__active_cmd->Address[0].PageID);
    if (chipBKE->Expected_command_exec_finish_time < dieBKE->__expected_finish_time)
      chipBKE->Expected_command_exec_finish_time = dieBKE->__expected_finish_time;
    break;
  default:
    throw std::invalid_argument("NVM_PHY_ONFI_NVDDR2: Unhandled event specified!");
  }

  target_channel->SetStatus(BusChannelStatus::BUSY, targetChip);
}

void NVM_PHY_ONFI_NVDDR2::Change_memory_status_preconditioning(const NVM::NVM_Memory_Address* address, const void* status_info)
{
  channels[((NVM::FlashMemory::Physical_Page_Address*)address)->ChannelID]->Chips[((NVM::FlashMemory::Physical_Page_Address*)address)->ChipID]->Change_memory_status_preconditioning(address, status_info);
}

void copy_read_data_to_transaction(NVM_Transaction_Flash_RD* read_transaction, NVM::FlashMemory::Flash_Command* command)
{
  int i = 0;
  for (auto &address : command->Address)
  {
    if (address.PlaneID == read_transaction->Address.PlaneID)
      read_transaction->LPA = command->Meta_data[i].LPA;
    i++;
  }
}

void NVM_PHY_ONFI_NVDDR2::Execute_simulator_event(MQSimEngine::Sim_Event* ev)
{
  auto* dieBKE = (DieBookKeepingEntry*)ev->Parameters;
  flash_channel_ID_type channel_id = dieBKE->__active_transactions.front()->Address.ChannelID;
  ONFI_Channel_NVDDR2* targetChannel = channels[channel_id];
  NVM::FlashMemory::Flash_Chip* targetChip = targetChannel->Chips[dieBKE->__active_transactions.front()->Address.ChipID];
  ChipBookKeepingEntry *chipBKE = &bookKeepingTable[channel_id][targetChip->ChipID];

  auto sim = Simulator;

  switch ((NVDDR2_SimEventType)ev->Type)
  {
  case NVDDR2_SimEventType::READ_CMD_ADDR_TRANSFERRED:
    //DEBUG2("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << dieBKE->__active_transactions.front()->Address.DieID << ": READ_CMD_ADDR_TRANSFERRED ")
    targetChip->EndCMDXfer(dieBKE->__active_cmd);
    for (auto tr : dieBKE->__active_transactions)
      tr->STAT_execution_time = dieBKE->__expected_finish_time - sim->Time();
    chipBKE->OngoingDieCMDTransfers.pop();
    chipBKE->No_of_active_dies++;
    if (chipBKE->OngoingDieCMDTransfers.size() > 0)
    {
      perform_interleaved_cmd_data_transfer(*targetChip, *chipBKE->OngoingDieCMDTransfers.front());
      return;
    }
    else
    {
      chipBKE->Status = ChipStatus::READING;
      targetChannel->SetStatus(BusChannelStatus::IDLE, targetChip);
    }
    break;
  case NVDDR2_SimEventType::ERASE_SETUP_COMPLETED:
    //DEBUG2("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << dieBKE->__active_transactions.front()->Address.DieID << ": ERASE_SETUP_COMPLETED ")
    targetChip->EndCMDXfer(dieBKE->__active_cmd);
    for (auto &tr : dieBKE->__active_transactions)
      tr->STAT_execution_time = dieBKE->__expected_finish_time - sim->Time();
    chipBKE->OngoingDieCMDTransfers.pop();
    chipBKE->No_of_active_dies++;
    if (chipBKE->OngoingDieCMDTransfers.size() > 0)
    {
      perform_interleaved_cmd_data_transfer(*targetChip, *chipBKE->OngoingDieCMDTransfers.front());
      return;
    }
    else
    {
      chipBKE->Status = ChipStatus::ERASING;
      targetChannel->SetStatus(BusChannelStatus::IDLE, targetChip);
    }
    break;
  case NVDDR2_SimEventType::PROGRAM_CMD_ADDR_DATA_TRANSFERRED:
  case NVDDR2_SimEventType::PROGRAM_COPYBACK_CMD_ADDR_TRANSFERRED:
    //DEBUG2("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << dieBKE->__active_transactions.front()->Address.DieID <<  ": PROGRAM_CMD_ADDR_DATA_TRANSFERRED " )
    targetChip->EndCMDDataInXfer(dieBKE->__active_cmd);
    for (auto &tr : dieBKE->__active_transactions)
      tr->STAT_execution_time = dieBKE->__expected_finish_time - sim->Time();
    chipBKE->OngoingDieCMDTransfers.pop();
    chipBKE->No_of_active_dies++;
    if (chipBKE->OngoingDieCMDTransfers.size() > 0)
    {
      perform_interleaved_cmd_data_transfer(*targetChip, *chipBKE->OngoingDieCMDTransfers.front());
      return;
    }
    else
    {
      chipBKE->Status = ChipStatus::WRITING;
      targetChannel->SetStatus(BusChannelStatus::IDLE, targetChip);
    }
    break;
  case NVDDR2_SimEventType::READ_DATA_TRANSFERRED:
    //DEBUG2("Chip " << targetChip->ChannelID << ", " << targetChip->ChipID << ", " << dieBKE->__active_transactions.front()->Address.DieID << ": READ_DATA_TRANSFERRED ")
    targetChip->EndDataOutXfer(dieBKE->__active_cmd);
    copy_read_data_to_transaction((NVM_Transaction_Flash_RD*)dieBKE->__active_transfer, dieBKE->__active_cmd);
#if 0
    if (tr->ExecutionMode != ExecutionModeType::COPYBACK)
#endif
    broadcastTransactionServicedSignal(dieBKE->__active_transfer);

    for (std::list<NVM_Transaction_Flash*>::iterator it = dieBKE->__active_transactions.begin();
      it != dieBKE->__active_transactions.end(); it++)
      if ((*it) == dieBKE->__active_transfer) {
        dieBKE->__active_transactions.erase(it);
        break;
      }
    dieBKE->__active_transfer = nullptr;
    if (dieBKE->__active_transactions.size() == 0)
      dieBKE->ClearCommand();

    chipBKE->WaitingReadTXCount--;
    if (chipBKE->No_of_active_dies == 0)
    {
      if (chipBKE->WaitingReadTXCount == 0)
        chipBKE->Status = ChipStatus::IDLE;
      else chipBKE->Status = ChipStatus::WAIT_FOR_DATA_OUT;
    }
    if (chipBKE->Status == ChipStatus::IDLE)
      if (dieBKE->suspended())
        send_resume_command_to_chip(*targetChip, *chipBKE);
    targetChannel->SetStatus(BusChannelStatus::IDLE, targetChip);
    break;
  default:
    PRINT_ERROR("Unknown simulation event specified for NVM_PHY_ONFI_NVDDR2!")
  }

  /* Copyback requests are prioritized over other type of requests since they need very short transfer time.
  In addition, they are just used for GC purpose. */
  if (WaitingCopybackWrites[channel_id].size() > 0)
  {
    DieBookKeepingEntry* waitingBKE = WaitingCopybackWrites[channel_id].front();
    targetChip = channels[channel_id]->Chips[waitingBKE->__active_transactions.front()->Address.ChipID];
    ChipBookKeepingEntry* waitingChipBKE = &bookKeepingTable[channel_id][targetChip->ChipID];
    if (waitingBKE->__active_transactions.size() > 1)
    {
      __stats.IssuedMultiplaneCopybackProgramCMD++;
      waitingBKE->__active_cmd->CommandCode = CMD_PROGRAM_PAGE_COPYBACK_MULTIPLANE;
    }
    else
    {
      __stats.IssuedCopybackProgramCMD++;
      waitingBKE->__active_cmd->CommandCode = CMD_PROGRAM_PAGE_COPYBACK;
    }
    targetChip->StartCMDXfer();
    waitingChipBKE->Status = ChipStatus::CMD_IN;

    sim->Register_sim_event(sim->Time() + this->channels[channel_id]->ProgramCommandTime[waitingBKE->__active_transactions.size()],
      this, waitingBKE, (int)NVDDR2_SimEventType::PROGRAM_COPYBACK_CMD_ADDR_TRANSFERRED);
    waitingChipBKE->OngoingDieCMDTransfers.push(waitingBKE);

    waitingBKE->__expected_finish_time = sim->Time() + this->channels[channel_id]->ProgramCommandTime[waitingBKE->__active_transactions.size()]
      + targetChip->Get_command_execution_latency(waitingBKE->__active_cmd->CommandCode, waitingBKE->__active_cmd->Address[0].PageID);
    if (waitingChipBKE->Expected_command_exec_finish_time < waitingBKE->__expected_finish_time)
      waitingChipBKE->Expected_command_exec_finish_time = waitingBKE->__expected_finish_time;

    WaitingCopybackWrites[channel_id].pop_front();
    channels[channel_id]->SetStatus(BusChannelStatus::BUSY, targetChip);
    return;
  }
  else if (WaitingMappingRead_TX[channel_id].size() > 0)
  {
    NVM_Transaction_Flash_RD* waitingTR = (NVM_Transaction_Flash_RD*)WaitingMappingRead_TX[channel_id].front();
    WaitingMappingRead_TX[channel_id].pop_front();
    transfer_read_data_from_chip(bookKeepingTable[channel_id][waitingTR->Address.ChipID],
                                 bookKeepingTable[channel_id][waitingTR->Address.ChipID].Die_book_keeping_records[waitingTR->Address.DieID],
                                 waitingTR);
    return;
  }
  else if (WaitingReadTX[channel_id].size() > 0)
  {
    NVM_Transaction_Flash_RD* waitingTR = (NVM_Transaction_Flash_RD*)WaitingReadTX[channel_id].front();
    WaitingReadTX[channel_id].pop_front();
    transfer_read_data_from_chip(bookKeepingTable[channel_id][waitingTR->Address.ChipID],
                                 bookKeepingTable[channel_id][waitingTR->Address.ChipID].Die_book_keeping_records[waitingTR->Address.DieID],
                                 waitingTR);
    return;
  }
  else if (WaitingGCRead_TX[channel_id].size() > 0)
  {
    NVM_Transaction_Flash_RD* waitingTR = (NVM_Transaction_Flash_RD*)WaitingGCRead_TX[channel_id].front();
    WaitingGCRead_TX[channel_id].pop_front();
    transfer_read_data_from_chip(bookKeepingTable[channel_id][waitingTR->Address.ChipID],
                                 bookKeepingTable[channel_id][waitingTR->Address.ChipID].Die_book_keeping_records[waitingTR->Address.DieID],
                                 waitingTR);
    return;
  }

  //If the execution reaches here, then the bus channel became idle
  broadcastChannelIdleSignal(channel_id);
}

void
NVM_PHY_ONFI_NVDDR2::__handle_ready_from_chip(NVM::FlashMemory::Flash_Chip& chip,
                                              NVM::FlashMemory::Flash_Command& command)

{
  ChipBookKeepingEntry& chipBKE = bookKeepingTable[chip.ChannelID][chip.ChipID];
  DieBookKeepingEntry& dieBKE = chipBKE.Die_book_keeping_records[command.Address[0].DieID];

  switch (command.CommandCode)
  {
  case CMD_READ_PAGE:
  case CMD_READ_PAGE_MULTIPLANE:
    DEBUG("Chip " << chip->ChannelID << ", " << chip->ChipID << ": finished  read command")

    chipBKE.No_of_active_dies--;
    if (chipBKE.No_of_active_dies == 0)//After finishing the last command, the chip state is changed
      chipBKE.Status = ChipStatus::WAIT_FOR_DATA_OUT;

    for (auto it = dieBKE.__active_transactions.begin(); it != dieBKE.__active_transactions.end(); it++)
    {
      chipBKE.WaitingReadTXCount++;
      if (channels[chip.ChannelID]->GetStatus() == BusChannelStatus::IDLE)
        transfer_read_data_from_chip(chipBKE, dieBKE, (*it));
      else
      {
        switch (dieBKE.__active_transactions.front()->Source)
        {
        case Transaction_Source_Type::CACHE:
        case Transaction_Source_Type::USERIO:
          WaitingReadTX[chip.ChannelID].push_back((*it));
          break;
        case Transaction_Source_Type::GC_WL:
          WaitingGCRead_TX[chip.ChannelID].push_back((*it));
          break;
        case Transaction_Source_Type::MAPPING:
          WaitingMappingRead_TX[chip.ChannelID].push_back((*it));
          break;
        }
      }
    }
    break;
  case CMD_READ_PAGE_COPYBACK:
  case CMD_READ_PAGE_COPYBACK_MULTIPLANE:
    chipBKE.No_of_active_dies--;
    if (chipBKE.No_of_active_dies == 0)
      chipBKE.Status = ChipStatus::WAIT_FOR_COPYBACK_CMD;

    if (channels[chip.ChannelID]->GetStatus() == BusChannelStatus::IDLE)
    {
      if (dieBKE.__active_transactions.size() > 1)
      {
        __stats.IssuedMultiplaneCopybackProgramCMD++;
        dieBKE.__active_cmd->CommandCode = CMD_PROGRAM_PAGE_COPYBACK_MULTIPLANE;
      }
      else
      {
        __stats.IssuedCopybackProgramCMD++;
        dieBKE.__active_cmd->CommandCode = CMD_PROGRAM_PAGE_COPYBACK;
      }

      for (auto it = dieBKE.__active_transactions.begin(); it != dieBKE.__active_transactions.end(); it++) {
        (*it)->STAT_transfer_time += channels[chip.ChannelID]->ProgramCommandTime[dieBKE.__active_transactions.size()];
      }

      chip.StartCMDXfer();
      chipBKE.Status = ChipStatus::CMD_IN;

      auto sim = Simulator;
      sim->Register_sim_event(sim->Time() + channels[chip.ChannelID]->ProgramCommandTime[dieBKE.__active_transactions.size()],
                              this, &dieBKE, (int)NVDDR2_SimEventType::PROGRAM_COPYBACK_CMD_ADDR_TRANSFERRED);

      chipBKE.OngoingDieCMDTransfers.push(&dieBKE);
      channels[chip.ChannelID]->SetStatus(BusChannelStatus::BUSY, &chip);

      dieBKE.__expected_finish_time = sim->Time()
                                        + channels[chip.ChannelID]->ProgramCommandTime[dieBKE.__active_transactions.size()]
                                        + chip.Get_command_execution_latency(dieBKE.__active_cmd->CommandCode, dieBKE.__active_cmd->Address[0].PageID);

      if (chipBKE.Expected_command_exec_finish_time < dieBKE.__expected_finish_time)
        chipBKE.Expected_command_exec_finish_time = dieBKE.__expected_finish_time;

#if 0  
      //Copyback data should be read out in order to get rid of bit error propagation
      Simulator->RegisterEvent(Simulator->Time() + channels[targetChip->ChannelID]->ProgramCommandTime + NVDDR2DataOutTransferTime(targetTransaction->SizeInByte, channels[targetChip->ChannelID]),
        this, targetTransaction, (int)NVDDR2_SimEventType::READ_DATA_TRANSFERRED);
      targetTransaction->STAT_TransferTime += NVDDR2DataOutTransferTime(targetTransaction->SizeInByte, channels[targetChip->ChannelID]);
#endif

    } else {
      WaitingCopybackWrites[chip.ChannelID].push_back(&dieBKE);
    }

    break;
  case CMD_PROGRAM_PAGE:
  case CMD_PROGRAM_PAGE_MULTIPLANE:
  case CMD_PROGRAM_PAGE_COPYBACK:
  case CMD_PROGRAM_PAGE_COPYBACK_MULTIPLANE:
  {
    DEBUG("Chip " << chip->ChannelID << ", " << chip->ChipID << ": finished program command")
    int i = 0;
    for (auto it = dieBKE.__active_transactions.begin(); it != dieBKE.__active_transactions.end(); it++, i++)
    {
      ((NVM_Transaction_Flash_WR*)(*it))->Content = command.Meta_data[i].LPA;
      broadcastTransactionServicedSignal(*it);
    }

    dieBKE.__active_transactions.clear();
    dieBKE.ClearCommand();

    chipBKE.No_of_active_dies--;

    if (chipBKE.No_of_active_dies == 0 && chipBKE.WaitingReadTXCount == 0)
      chipBKE.Status = ChipStatus::IDLE;

    //Since the time required to send the resume command is very small, we ignore it
    if (chipBKE.Status == ChipStatus::IDLE)
      if (chipBKE.HasSuspend)
        send_resume_command_to_chip(chip, chipBKE);
    break;
  }
  case CMD_ERASE_BLOCK:
  case CMD_ERASE_BLOCK_MULTIPLANE:
    DEBUG("Chip " << chip->ChannelID << ", " << chip->ChipID << ": finished erase command")

    for (auto it = dieBKE.__active_transactions.begin(); it != dieBKE.__active_transactions.end(); it++)
      broadcastTransactionServicedSignal(*it);

    dieBKE.__active_transactions.clear();
    dieBKE.ClearCommand();

    chipBKE.No_of_active_dies--;
    if (chipBKE.No_of_active_dies == 0 && chipBKE.WaitingReadTXCount == 0)
      chipBKE.Status = ChipStatus::IDLE;

    //Since the time required to send the resume command is very small, we ignore it
    if (chipBKE.Status == ChipStatus::IDLE)
      if (chipBKE.HasSuspend)
        send_resume_command_to_chip(chip, chipBKE);
    break;
  default:
    break;
  }

  if (channels[chip.ChannelID]->GetStatus() == BusChannelStatus::IDLE)
    broadcastChannelIdleSignal(chip.ChannelID);

  else if (chipBKE.Status == ChipStatus::IDLE)
    broadcastChipIdleSignal(chip);
}
