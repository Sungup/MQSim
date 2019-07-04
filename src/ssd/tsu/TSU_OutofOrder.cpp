#include "TSU_OutofOrder.h"

namespace SSD_Components
{

  TSU_OutOfOrder::TSU_OutOfOrder(const sim_object_id_type& id, FTL* ftl, NVM_PHY_ONFI_NVDDR2* NVMController, uint32_t ChannelCount, uint32_t chip_no_per_channel,
    uint32_t DieNoPerChip, uint32_t PlaneNoPerDie,
    sim_time_type WriteReasonableSuspensionTimeForRead,
    sim_time_type EraseReasonableSuspensionTimeForRead,
    sim_time_type EraseReasonableSuspensionTimeForWrite, 
    bool EraseSuspensionEnabled, bool ProgramSuspensionEnabled)
    : TSU_Base(id, ftl, NVMController, Flash_Scheduling_Type::OUT_OF_ORDER, ChannelCount, chip_no_per_channel, DieNoPerChip, PlaneNoPerDie,
      WriteReasonableSuspensionTimeForRead, EraseReasonableSuspensionTimeForRead, EraseReasonableSuspensionTimeForWrite,
      EraseSuspensionEnabled, ProgramSuspensionEnabled)
  {
    UserReadTRQueue = new Flash_Transaction_Queue*[channel_count];
    UserWriteTRQueue = new Flash_Transaction_Queue*[channel_count];
    GCReadTRQueue = new Flash_Transaction_Queue*[channel_count];
    GCWriteTRQueue = new Flash_Transaction_Queue*[channel_count];
    GCEraseTRQueue = new Flash_Transaction_Queue*[channel_count];
    MappingReadTRQueue = new Flash_Transaction_Queue*[channel_count];
    MappingWriteTRQueue = new Flash_Transaction_Queue*[channel_count];
    for (uint32_t channelID = 0; channelID < channel_count; channelID++)
    {
      UserReadTRQueue[channelID] = new Flash_Transaction_Queue[chip_no_per_channel];
      UserWriteTRQueue[channelID] = new Flash_Transaction_Queue[chip_no_per_channel];
      GCReadTRQueue[channelID] = new Flash_Transaction_Queue[chip_no_per_channel];
      GCWriteTRQueue[channelID] = new Flash_Transaction_Queue[chip_no_per_channel];
      GCEraseTRQueue[channelID] = new Flash_Transaction_Queue[chip_no_per_channel];
      MappingReadTRQueue[channelID] = new Flash_Transaction_Queue[chip_no_per_channel];
      MappingWriteTRQueue[channelID] = new Flash_Transaction_Queue[chip_no_per_channel];
      for (uint32_t chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
      {
        UserReadTRQueue[channelID][chip_cntr].Set_id("User_Read_TR_Queue@" + std::to_string(channelID) + "@" + std::to_string(chip_cntr));
        UserWriteTRQueue[channelID][chip_cntr].Set_id("User_Write_TR_Queue@" + std::to_string(channelID) + "@" + std::to_string(chip_cntr));
        GCReadTRQueue[channelID][chip_cntr].Set_id("GC_Read_TR_Queue@" + std::to_string(channelID) + "@" + std::to_string(chip_cntr));
        MappingReadTRQueue[channelID][chip_cntr].Set_id("Mapping_Read_TR_Queue@" + std::to_string(channelID) + "@" + std::to_string(chip_cntr));
        MappingWriteTRQueue[channelID][chip_cntr].Set_id("Mapping_Write_TR_Queue@" + std::to_string(channelID) + "@" + std::to_string(chip_cntr));
        GCWriteTRQueue[channelID][chip_cntr].Set_id("GC_Write_TR_Queue@" + std::to_string(channelID) + "@" + std::to_string(chip_cntr));
        GCEraseTRQueue[channelID][chip_cntr].Set_id("GC_Erase_TR_Queue@" + std::to_string(channelID) + "@" + std::to_string(chip_cntr));
      }
    }
  }
  
  TSU_OutOfOrder::~TSU_OutOfOrder()
  {
    for (uint32_t channelID = 0; channelID < channel_count; channelID++)
    {
      delete[] UserReadTRQueue[channelID];
      delete[] UserWriteTRQueue[channelID];
      delete[] GCReadTRQueue[channelID];
      delete[] GCWriteTRQueue[channelID];
      delete[] GCEraseTRQueue[channelID];
      delete[] MappingReadTRQueue[channelID];
      delete[] MappingWriteTRQueue[channelID];
    }
    delete[] UserReadTRQueue;
    delete[] UserWriteTRQueue;
    delete[] GCReadTRQueue;
    delete[] GCWriteTRQueue;
    delete[] GCEraseTRQueue;
    delete[] MappingReadTRQueue;
    delete[] MappingWriteTRQueue;
  }

  void TSU_OutOfOrder::Start_simulation() {}

  void TSU_OutOfOrder::Validate_simulation_config() {}

  void TSU_OutOfOrder::Execute_simulator_event(MQSimEngine::SimEvent* event) {}

  void TSU_OutOfOrder::Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter)
  {
    name_prefix = name_prefix + +".TSU";
    xmlwriter.Write_open_tag(name_prefix);

    TSU_Base::Report_results_in_XML(name_prefix, xmlwriter);

    for (uint32_t channelID = 0; channelID < channel_count; channelID++)
      for (uint32_t chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
        UserReadTRQueue[channelID][chip_cntr].Report_results_in_XML(name_prefix + ".User_Read_TR_Queue", xmlwriter);

    for (uint32_t channelID = 0; channelID < channel_count; channelID++)
      for (uint32_t chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
        UserWriteTRQueue[channelID][chip_cntr].Report_results_in_XML(name_prefix + ".User_Write_TR_Queue", xmlwriter);

    for (uint32_t channelID = 0; channelID < channel_count; channelID++)
      for (uint32_t chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
        MappingReadTRQueue[channelID][chip_cntr].Report_results_in_XML(name_prefix + ".Mapping_Read_TR_Queue", xmlwriter);

    for (uint32_t channelID = 0; channelID < channel_count; channelID++)
      for (uint32_t chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
        MappingWriteTRQueue[channelID][chip_cntr].Report_results_in_XML(name_prefix + ".Mapping_Write_TR_Queue", xmlwriter);

    for (uint32_t channelID = 0; channelID < channel_count; channelID++)
      for (uint32_t chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
        GCReadTRQueue[channelID][chip_cntr].Report_results_in_XML(name_prefix + ".GC_Read_TR_Queue", xmlwriter);

    for (uint32_t channelID = 0; channelID < channel_count; channelID++)
      for (uint32_t chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
        GCWriteTRQueue[channelID][chip_cntr].Report_results_in_XML(name_prefix + ".GC_Write_TR_Queue", xmlwriter);

    for (uint32_t channelID = 0; channelID < channel_count; channelID++)
      for (uint32_t chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
        GCEraseTRQueue[channelID][chip_cntr].Report_results_in_XML(name_prefix + ".GC_Erase_TR_Queue", xmlwriter);
  
    xmlwriter.Write_close_tag();
  }

  void TSU_OutOfOrder::Prepare_for_transaction_submit()
  {
    opened_scheduling_reqs++;
    if (opened_scheduling_reqs > 1)
      return;
    transaction_receive_slots.clear();
  }

  inline void TSU_OutOfOrder::Submit_transaction(NvmTransactionFlash* transaction)
  {
    transaction_receive_slots.push_back(transaction);
  }

  void TSU_OutOfOrder::Schedule()
  {
    opened_scheduling_reqs--;
    if (opened_scheduling_reqs > 0)
      return;

    // TSU_OutOfOrder: Illegal status!
    assert(opened_scheduling_reqs >= 0);

    if (transaction_receive_slots.empty())
      return;

    for (auto transaction : transaction_receive_slots) {
      auto& address = transaction->Address;
      switch (transaction->Type)
      {
      case Transaction_Type::READ:
        switch (transaction->Source)
        {
        case Transaction_Source_Type::CACHE:
        case Transaction_Source_Type::USERIO:
          UserReadTRQueue[address.ChannelID][address.ChipID].push_back(transaction);
          break;
        case Transaction_Source_Type::MAPPING:
          MappingReadTRQueue[address.ChannelID][address.ChipID].push_back(transaction);
          break;
        case Transaction_Source_Type::GC_WL:
          GCReadTRQueue[address.ChannelID][address.ChipID].push_back(transaction);
          break;
        }
        break;
      case Transaction_Type::WRITE:
        switch (transaction->Source)
        {
        case Transaction_Source_Type::CACHE:
        case Transaction_Source_Type::USERIO:
          UserWriteTRQueue[address.ChannelID][address.ChipID].push_back(transaction);
          break;
        case Transaction_Source_Type::MAPPING:
          MappingWriteTRQueue[address.ChannelID][address.ChipID].push_back(transaction);
          break;
        case Transaction_Source_Type::GC_WL:
          GCWriteTRQueue[address.ChannelID][address.ChipID].push_back(transaction);
          break;
        }
        break;
      case Transaction_Type::ERASE:
        GCEraseTRQueue[address.ChannelID][address.ChipID].push_back(transaction);
        break;
      default:
        break;
      }
    }

    for (flash_channel_ID_type channelID = 0; channelID < channel_count; channelID++)
    {
      if (_is_idle_channel(channelID))
        _handle_idle_channel(channelID);
    }
  }
  
  bool TSU_OutOfOrder::service_read_transaction(const NVM::FlashMemory::Flash_Chip& chip)
  {
    Flash_Transaction_Queue *sourceQueue1 = nullptr, *sourceQueue2 = nullptr;

    /// Priority
    /// 1. MappingReadTRQueue is not empty
    ///    1) GC_and_WL_Unit is urgent and GCReadTRQueue is not empty.
    ///    2) User Read Queue is not empty.
    /// 2. GCandWLUnit is in urgent mode.
    ///    1) GCReadTRQueue is not empty.
    ///    2) GCWriteTRQueue is not empty. --> Read service stop
    ///    3) GCEraseTRQueue is not empty. --> Read service stop
    ///    4) Select User Read Queue
    ///    5) No Read Transaction
    /// 3. Others
    ///    1) UserReadTRQueue is not empty
    ///    2) UserWriteTRQueue is not empty --> Read service stop
    ///    3) GCReadTRQueue is not empty
    ///    4) No Read Transaction
    if (!MappingReadTRQueue[chip.ChannelID][chip.ChipID].empty()) {
      //Flash transactions that are related to FTL mapping data have the highest priority
      sourceQueue1 = &MappingReadTRQueue[chip.ChannelID][chip.ChipID];

      if (ftl->GC_and_WL_Unit->GC_is_in_urgent_mode(&chip) && GCReadTRQueue[chip.ChannelID][chip.ChipID].size() > 0)
        sourceQueue2 = &GCReadTRQueue[chip.ChannelID][chip.ChipID];
      else if (UserReadTRQueue[chip.ChannelID][chip.ChipID].size() > 0)
        sourceQueue2 = &UserReadTRQueue[chip.ChannelID][chip.ChipID];
    }
    else if (ftl->GC_and_WL_Unit->GC_is_in_urgent_mode(&chip)) {
      //If flash transactions related to GC are prioritzed (non-preemptive execution mode of GC), then GC queues are checked first
      if (GCReadTRQueue[chip.ChannelID][chip.ChipID].size() > 0)
      {
        sourceQueue1 = &GCReadTRQueue[chip.ChannelID][chip.ChipID];
        if (UserReadTRQueue[chip.ChannelID][chip.ChipID].size() > 0)
          sourceQueue2 = &UserReadTRQueue[chip.ChannelID][chip.ChipID];
      }
      else if (GCWriteTRQueue[chip.ChannelID][chip.ChipID].size() > 0)
        return false;
      else if (GCEraseTRQueue[chip.ChannelID][chip.ChipID].size() > 0)
        return false;
      else if (UserReadTRQueue[chip.ChannelID][chip.ChipID].size() > 0)
        sourceQueue1 = &UserReadTRQueue[chip.ChannelID][chip.ChipID];
      else return false;
    } 
    else //If GC is currently executed in the preemptive mode, then user IO transaction queues are checked first
    {
      if (UserReadTRQueue[chip.ChannelID][chip.ChipID].size() > 0)
      {
        sourceQueue1 = &UserReadTRQueue[chip.ChannelID][chip.ChipID];
        if (GCReadTRQueue[chip.ChannelID][chip.ChipID].size() > 0)
          sourceQueue2 = &GCReadTRQueue[chip.ChannelID][chip.ChipID];
      }
      else if (UserWriteTRQueue[chip.ChannelID][chip.ChipID].size() > 0)
          return false;
      else if (GCReadTRQueue[chip.ChannelID][chip.ChipID].size() > 0)
        sourceQueue1 = &GCReadTRQueue[chip.ChannelID][chip.ChipID];
      else return false;
    }

    /// Currently following chip status check routine always fall into return
    /// false except IDLE status. Commented break phrase has been added by
    /// sungup at 2019.07.02. Is this right procedure?
    bool suspensionRequired = false;
    ChipStatus cs = _NVMController->GetChipStatus(chip);
    switch (cs)
    {
    case ChipStatus::IDLE:
      break;
    case ChipStatus::WRITING:
      if (!programSuspensionEnabled || _NVMController->HasSuspendedCommand(chip))
        return false;
      if (_NVMController->Expected_finish_time(chip) - Simulator->Time() < writeReasonableSuspensionTimeForRead)
        return false;
      suspensionRequired = true;
      // break;
    case ChipStatus::ERASING:
      if (!eraseSuspensionEnabled || _NVMController->HasSuspendedCommand(chip))
        return false;
      if (_NVMController->Expected_finish_time(chip) - Simulator->Time() < eraseReasonableSuspensionTimeForRead)
        return false;
      suspensionRequired = true;
      // break;
    default:
      return false;
    }
    
    flash_die_ID_type dieID = sourceQueue1->front()->Address.DieID;
    flash_page_ID_type pageID = sourceQueue1->front()->Address.PageID;
    uint32_t planeVector = 0;
    for (uint32_t i = 0; i < die_no_per_chip; i++)
    {
      transaction_dispatch_slots.clear();
      planeVector = 0;

      for (auto it = sourceQueue1->begin(); it != sourceQueue1->end();)
      {
        if ((*it)->Address.DieID == dieID && !(planeVector & 1U << (*it)->Address.PlaneID))
        {
          if (planeVector == 0 || (*it)->Address.PageID == pageID)//Check for identical pages when running multiplane command
          {
            (*it)->SuspendRequired = suspensionRequired;
            planeVector |= 1U << (*it)->Address.PlaneID;
            transaction_dispatch_slots.push_back(*it);
            sourceQueue1->remove(it++);
            continue;
          }
        }
        it++;
      }

      if (sourceQueue2 != nullptr && transaction_dispatch_slots.size() < plane_no_per_die)
        for (auto it = sourceQueue2->begin(); it != sourceQueue2->end();)
        {
          if ((*it)->Address.DieID == dieID && !(planeVector & 1U << (*it)->Address.PlaneID))
          {
            if (planeVector == 0 || (*it)->Address.PageID == pageID)//Check for identical pages when running multiplane command
            {
              (*it)->SuspendRequired = suspensionRequired;
              planeVector |= 1U << (*it)->Address.PlaneID;
              transaction_dispatch_slots.push_back(*it);
              sourceQueue2->remove(it++);
              continue;
            }
          }
          it++;
        }

      if (!transaction_dispatch_slots.empty())
        _NVMController->Send_command_to_chip(transaction_dispatch_slots);

      transaction_dispatch_slots.clear();
      dieID = (dieID + 1) % die_no_per_chip;
    }

    return true;
  }

  bool TSU_OutOfOrder::service_write_transaction(const NVM::FlashMemory::Flash_Chip& chip)
  {
    Flash_Transaction_Queue *sourceQueue1 = nullptr, *sourceQueue2 = nullptr;

    if (ftl->GC_and_WL_Unit->GC_is_in_urgent_mode(&chip))//If flash transactions related to GC are prioritzed (non-preemptive execution mode of GC), then GC queues are checked first
    {
      if (GCWriteTRQueue[chip.ChannelID][chip.ChipID].size() > 0)
      {
        sourceQueue1 = &GCWriteTRQueue[chip.ChannelID][chip.ChipID];
        if (UserWriteTRQueue[chip.ChannelID][chip.ChipID].size() > 0)
          sourceQueue2 = &UserWriteTRQueue[chip.ChannelID][chip.ChipID];
      }
      else if (GCEraseTRQueue[chip.ChannelID][chip.ChipID].size() > 0)
        return false;
      else if (UserWriteTRQueue[chip.ChannelID][chip.ChipID].size() > 0)
        sourceQueue1 = &UserWriteTRQueue[chip.ChannelID][chip.ChipID];
      else return false;
    }
    else //If GC is currently executed in the preemptive mode, then user IO transaction queues are checked first
    {
      if (UserWriteTRQueue[chip.ChannelID][chip.ChipID].size() > 0)
      {
        sourceQueue1 = &UserWriteTRQueue[chip.ChannelID][chip.ChipID];
        if (GCWriteTRQueue[chip.ChannelID][chip.ChipID].size() > 0)
          sourceQueue2 = &GCWriteTRQueue[chip.ChannelID][chip.ChipID];
      }
      else if (GCWriteTRQueue[chip.ChannelID][chip.ChipID].size() > 0)
        sourceQueue1 = &GCWriteTRQueue[chip.ChannelID][chip.ChipID];
      else return false;
    }


    bool suspensionRequired = false;
    ChipStatus cs = _NVMController->GetChipStatus(chip);
    switch (cs)
    {
    case ChipStatus::IDLE:
      break;
    case ChipStatus::ERASING:
      if (!eraseSuspensionEnabled || _NVMController->HasSuspendedCommand(chip))
        return false;
      if (_NVMController->Expected_finish_time(chip) - Simulator->Time() < eraseReasonableSuspensionTimeForWrite)
        return false;
      suspensionRequired = true;
    default:
      return false;
    }

    flash_die_ID_type dieID = sourceQueue1->front()->Address.DieID;
    flash_page_ID_type pageID = sourceQueue1->front()->Address.PageID;
    uint32_t planeVector = 0;
    for (uint32_t i = 0; i < die_no_per_chip; i++)
    {
      transaction_dispatch_slots.clear();
      planeVector = 0;

      for (Flash_Transaction_Queue::iterator it = sourceQueue1->begin(); it != sourceQueue1->end(); )
      {
        if (((NvmTransactionFlashWR*)*it)->RelatedRead == nullptr && (*it)->Address.DieID == dieID && !(planeVector & 1 << (*it)->Address.PlaneID))
        {
          if (planeVector == 0 || (*it)->Address.PageID == pageID)//Check for identical pages when running multiplane command
          {
            (*it)->SuspendRequired = suspensionRequired;
            planeVector |= 1 << (*it)->Address.PlaneID;
            transaction_dispatch_slots.push_back(*it);
            sourceQueue1->remove(it++);
            continue;
          }
        }
        it++;
      }

      if (sourceQueue2 != nullptr)
        for (Flash_Transaction_Queue::iterator it = sourceQueue2->begin(); it != sourceQueue2->end(); )
        {
          if (((NvmTransactionFlashWR*)*it)->RelatedRead == nullptr && (*it)->Address.DieID == dieID && !(planeVector & 1 << (*it)->Address.PlaneID))
          {
            if (planeVector == 0 || (*it)->Address.PageID == pageID)//Check for identical pages when running multiplane command
            {
              (*it)->SuspendRequired = suspensionRequired;
              planeVector |= 1 << (*it)->Address.PlaneID;
              transaction_dispatch_slots.push_back(*it);
              sourceQueue2->remove(it++);
              continue;
            }
          }
          it++;
        }

      if (transaction_dispatch_slots.size() > 0)
        _NVMController->Send_command_to_chip(transaction_dispatch_slots);
      transaction_dispatch_slots.clear();
      dieID = (dieID + 1) % die_no_per_chip;
    }
    return true;
  }

  bool TSU_OutOfOrder::service_erase_transaction(const NVM::FlashMemory::Flash_Chip& chip)
  {
    if (_NVMController->GetChipStatus(chip) != ChipStatus::IDLE)
      return false;

    Flash_Transaction_Queue* source_queue = &GCEraseTRQueue[chip.ChannelID][chip.ChipID];
    if (source_queue->size() == 0)
      return false;

    flash_die_ID_type dieID = source_queue->front()->Address.DieID;
    uint32_t planeVector = 0;
    for (uint32_t i = 0; i < die_no_per_chip; i++)
    {
      transaction_dispatch_slots.clear();
      planeVector = 0;

      for (Flash_Transaction_Queue::iterator it = source_queue->begin(); it != source_queue->end(); )
      {
        if (((NvmTransactionFlashER*)*it)->Page_movement_activities.size() == 0 && (*it)->Address.DieID == dieID && !(planeVector & 1 << (*it)->Address.PlaneID))
        {
          planeVector |= 1 << (*it)->Address.PlaneID;
          transaction_dispatch_slots.push_back(*it);
          source_queue->remove(it++);
        }
        it++;
      }
      if (transaction_dispatch_slots.size() > 0)
        _NVMController->Send_command_to_chip(transaction_dispatch_slots);
      transaction_dispatch_slots.clear();
      dieID = (dieID + 1) % die_no_per_chip;
    }
    return true;
  }
}