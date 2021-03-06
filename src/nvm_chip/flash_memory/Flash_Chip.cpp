#include "../../sim/Sim_Defs.h"
#include "../../sim/Engine.h"
#include "Flash_Chip.h"


using namespace NVM::FlashMemory;
Flash_Chip::Flash_Chip(const sim_object_id_type& id,
                       const FlashParameterSet& params,
                       flash_channel_ID_type channelID,
                       flash_chip_ID_type localChipID,
                       sim_time_type commProtocolDelayRead,
                       sim_time_type commProtocolDelayWrite,
                       sim_time_type commProtocolDelayErase)
  : NVM_Chip(id + ".Channel." + std::to_string(channelID) + ".Chip." + std::to_string(localChipID)),
    ChannelID(channelID),
    ChipID(localChipID),
    flash_technology(params.Flash_Technology),
    status(Internal_Status::IDLE),
    idleDieNo(params.Die_No_Per_Chip),
    die_no(params.Die_No_Per_Chip),
    page_no_per_block(params.Page_No_Per_Block),
    _readLatency(params.read_latencies.data()),
    _programLatency(params.write_latencies.data()),
    _eraseLatency(params.Block_Erase_Latency),
    _suspendProgramLatency(params.Suspend_Program_Time),
    _suspendEraseLatency(params.Suspend_Erase_Time),
    _RBSignalDelayRead(commProtocolDelayRead),
    _RBSignalDelayWrite(commProtocolDelayWrite),
    _RBSignalDelayErase(commProtocolDelayErase),
    lastTransferStart(INVALID_TIME),
    executionStartTime(INVALID_TIME),
    expectedFinishTime(INVALID_TIME),
    STAT_readCount(0),
    STAT_progamCount(0),
    STAT_eraseCount(0),
    STAT_totalSuspensionCount(0),
    STAT_totalResumeCount(0),
    STAT_totalExecTime(0),
    STAT_totalXferTime(0),
    STAT_totalOverlappedXferExecTime(0)
{
  Dies.reserve(params.Die_No_Per_Chip);
  for (uint32_t die = 0; die < params.Die_No_Per_Chip; ++die)
    Dies.emplace_back(params);
}

void Flash_Chip::Start_simulation() {}

void Flash_Chip::Validate_simulation_config()
{
  if (Dies.empty() || die_no == 0)
    PRINT_ERROR("Flash chip " << ID() << ": has no dies!")
}

void Flash_Chip::Change_memory_status_preconditioning(const NVM_Memory_Address* address, const void* status_info)
{
  auto* flash_address = (Physical_Page_Address*)address;
  Dies[flash_address->DieID].Planes[flash_address->PlaneID].Blocks[flash_address->BlockID].Pages[flash_address->PageID].Metadata.LPA = *(LPA_type*)status_info;
}

void Flash_Chip::Setup_triggers() { MQSimEngine::Sim_Object::Setup_triggers(); }

void Flash_Chip::Execute_simulator_event(MQSimEngine::SimEvent* ev)
{
  Chip_Sim_Event_Type eventType = (Chip_Sim_Event_Type)ev->Type;
  FlashCommand* command = (FlashCommand*)ev->Parameters;

  switch (eventType)
  {
  case Chip_Sim_Event_Type::COMMAND_FINISHED:
    finish_command_execution(command);
    break;
  }
}

LPA_type Flash_Chip::Get_metadata(flash_die_ID_type die_id, flash_plane_ID_type plane_id, flash_block_ID_type block_id, flash_page_ID_type page_id)//A simplification to decrease the complexity of GC execution! The GC unit may need to know the metadata of a page to decide if a page is valid or invalid.
{
  Page& page = Dies[die_id].Planes[plane_id].Blocks[block_id].Pages[page_id];

  return page.Metadata.LPA;
}

void Flash_Chip::start_command_execution(FlashCommand* command)
{
  Die& targetDie = Dies[command->Address[0].DieID];

  //If this is a simple command (not multiplane) then there should be only one address
  if (command->Address.size() > 1
    && (command->CommandCode == CMD_READ_PAGE
      || command->CommandCode == CMD_PROGRAM_PAGE
      || command->CommandCode == CMD_ERASE_BLOCK))
    PRINT_ERROR("Flash chip " << ID() << ": executing a flash operation on a busy die!")

  auto sim = Simulator;

  targetDie.Expected_finish_time = sim->Time() + Get_command_execution_latency(command->CommandCode, command->Address[0].PageID);
  targetDie.CommandFinishEvent = sim->Register_sim_event(targetDie.Expected_finish_time,
    this, command, static_cast<int>(Chip_Sim_Event_Type::COMMAND_FINISHED));
  targetDie.CurrentCMD = command;
  targetDie.Status = DieStatus::BUSY;
  idleDieNo--;

  if (status == Internal_Status::IDLE)
  {
    executionStartTime = sim->Time();
    expectedFinishTime = targetDie.Expected_finish_time;
    status = Internal_Status::BUSY;
  }

  PRINT_DEBUG("Command execution started on channel: " << this->ChannelID << " chip: " << this->ChipID)
}

void Flash_Chip::finish_command_execution(FlashCommand* command)
{
  Die& targetDie = Dies[command->Address[0].DieID];

  targetDie.STAT_TotalReadTime += Get_command_execution_latency(command->CommandCode, command->Address[0].PageID);
  targetDie.Expected_finish_time = INVALID_TIME;
  targetDie.CommandFinishEvent = NULL;
  targetDie.CurrentCMD = NULL;
  targetDie.Status = DieStatus::IDLE;
  this->idleDieNo++;
  if (idleDieNo == die_no)
  {
    auto sim = Simulator;

    this->status = Internal_Status::IDLE;

    STAT_totalExecTime += sim->Time() - executionStartTime;
    if (this->lastTransferStart != INVALID_TIME)
      STAT_totalOverlappedXferExecTime += sim->Time() - lastTransferStart;
  }

  switch (command->CommandCode)
  {
  case CMD_READ_PAGE:
  case CMD_READ_PAGE_MULTIPLANE:
  case CMD_READ_PAGE_COPYBACK:
  case CMD_READ_PAGE_COPYBACK_MULTIPLANE:
    PRINT_DEBUG("Channel " << this->ChannelID << " Chip " << this->ChipID << "- Finished executing read command")
    for (uint32_t planeCntr = 0; planeCntr < command->Address.size(); planeCntr++)
    {
      STAT_readCount++;
      targetDie.Planes[command->Address[planeCntr].PlaneID].Read_count++;
      targetDie.Planes[command->Address[planeCntr].PlaneID].Blocks[command->Address[planeCntr].BlockID].Pages[command->Address[planeCntr].PageID].Read_metadata(command->Meta_data[planeCntr]);
    }
    break;
  case CMD_PROGRAM_PAGE:
  case CMD_PROGRAM_PAGE_MULTIPLANE:
  case CMD_PROGRAM_PAGE_COPYBACK:
  case CMD_PROGRAM_PAGE_COPYBACK_MULTIPLANE:
    PRINT_DEBUG("Channel " << this->ChannelID << " Chip " << this->ChipID << "- Finished executing program command")
    for (uint32_t planeCntr = 0; planeCntr < command->Address.size(); planeCntr++)
    {
      STAT_progamCount++;
      targetDie.Planes[command->Address[planeCntr].PlaneID].Progam_count++;
      targetDie.Planes[command->Address[planeCntr].PlaneID].Blocks[command->Address[planeCntr].BlockID].Pages[command->Address[planeCntr].PageID].Write_metadata(command->Meta_data[planeCntr]);
    }
    break;
  case CMD_ERASE_BLOCK:
  case CMD_ERASE_BLOCK_MULTIPLANE:
  {
    for (uint32_t planeCntr = 0; planeCntr < command->Address.size(); planeCntr++)
    {
      STAT_eraseCount++;
      targetDie.Planes[command->Address[planeCntr].PlaneID].Erase_count++;
      auto& targetBlock = targetDie.Planes[command->Address[planeCntr].PlaneID].Blocks[command->Address[planeCntr].BlockID];
      for (uint32_t i = 0; i < page_no_per_block; i++)
      {
        //targetBlock->Pages[i].Metadata.SourceStreamID = NO_STREAM;
        //targetBlock->Pages[i].Metadata.Status = FREE_PAGE;
        targetBlock.Pages[i].Metadata.LPA = NO_LPA;
      }
    }
    break;
  }
  default:
    PRINT_ERROR("Flash chip " << ID() << ": unhandled flash command type!")
  }

  //In MQSim, flash chips always announce their status using the ready/busy signal; the controller does not issue a die status read command
  broadcast_ready_signal(command);
}

void Flash_Chip::broadcast_ready_signal(FlashCommand* command)
{
  for (auto it : __connected_ready_handlers)
    (*it)(*this, *command);
}

void Flash_Chip::Suspend(flash_die_ID_type dieID)
{
  auto sim = Simulator;

  STAT_totalExecTime += sim->Time() - executionStartTime;

  Die& targetDie = Dies[dieID];
  if (targetDie.Suspended)
    PRINT_ERROR("Flash chip" << ID() << ": suspending a previously suspended flash chip! This is illegal.")

  /*if (targetDie.CurrentCMD & CMD_READ != 0)
  throw "Suspend is not supported for read operations!";*/

  targetDie.RemainingSuspendedExecTime = targetDie.Expected_finish_time - sim->Time();
  sim->Ignore_sim_event(targetDie.CommandFinishEvent);//The simulator engine should not execute the finish event for the suspended command
  targetDie.CommandFinishEvent = NULL;

  targetDie.SuspendedCMD = targetDie.CurrentCMD;
  targetDie.CurrentCMD = NULL;
  targetDie.Suspended = true;
  STAT_totalSuspensionCount++;

  targetDie.Status = DieStatus::IDLE;
  this->idleDieNo++;
  if (this->idleDieNo == die_no)
    this->status = Internal_Status::IDLE;

  executionStartTime = INVALID_TIME;
  expectedFinishTime = INVALID_TIME;
}

void Flash_Chip::Resume(flash_die_ID_type dieID)
{
  Die& targetDie = Dies[dieID];
  if (!targetDie.Suspended)
    PRINT_ERROR("Flash chip " << ID() << ": resume flash command is requested, but there is no suspended flash command!")


  targetDie.CurrentCMD = targetDie.SuspendedCMD;
  targetDie.SuspendedCMD = NULL;
  targetDie.Suspended = false;
  STAT_totalResumeCount++;

  auto sim = Simulator;

  targetDie.Expected_finish_time = sim->Time() + targetDie.RemainingSuspendedExecTime;
  targetDie.CommandFinishEvent = sim->Register_sim_event(targetDie.Expected_finish_time,
    this, targetDie.CurrentCMD, static_cast<int>(Chip_Sim_Event_Type::COMMAND_FINISHED));
  if (targetDie.Expected_finish_time > this->expectedFinishTime)
    this->expectedFinishTime = targetDie.Expected_finish_time;



  targetDie.Status = DieStatus::BUSY;
  this->idleDieNo--;
  this->status = Internal_Status::BUSY;
  executionStartTime = sim->Time();
}

sim_time_type Flash_Chip::GetSuspendProgramTime()
{
  return _suspendProgramLatency;
}

sim_time_type Flash_Chip::GetSuspendEraseTime()
{
  return _suspendEraseLatency;
}

void Flash_Chip::Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter)
{
  auto sim = Simulator;

  std::string tmp = name_prefix;
  xmlwriter.Write_start_element_tag(tmp + ".FlashChips");

  std::string attr = "ID";
  std::string val = "@" + std::to_string(ChannelID) + "@" + std::to_string(ChipID);
  xmlwriter.Write_attribute_string_inline(attr, val);

  attr = "Fraction_of_Time_in_Execution";
  val = std::to_string(STAT_totalExecTime / double(sim->Time()));
  xmlwriter.Write_attribute_string_inline(attr, val);

  attr = "Fraction_of_Time_in_DataXfer";
  val = std::to_string(STAT_totalXferTime / double(sim->Time()));
  xmlwriter.Write_attribute_string_inline(attr, val);

  attr = "Fraction_of_Time_in_DataXfer_and_Execution";
  val = std::to_string(STAT_totalOverlappedXferExecTime / double(sim->Time()));
  xmlwriter.Write_attribute_string_inline(attr, val);

  attr = "Fraction_of_Time_Idle";
  val = std::to_string((sim->Time() - STAT_totalOverlappedXferExecTime - STAT_totalXferTime) / double(sim->Time()));
  xmlwriter.Write_attribute_string_inline(attr, val);

  xmlwriter.Write_end_element_tag();
}
