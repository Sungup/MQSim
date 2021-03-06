#ifndef FLASH_CHIP_H
#define FLASH_CHIP_H

#include "../../exec/params/FlashParameterSet.h"
#include "../../sim/Sim_Defs.h"
#include "../../sim/SimEvent.h"
#include "../../sim/Engine.h"
#include "../../sim/Sim_Reporter.h"
#include "../NVM_Chip.h"
#include "FlashTypes.h"
#include "Die.h"
#include "FlashCommand.h"
#include <vector>
#include <stdexcept>

namespace NVM
{
  namespace FlashMemory
  {
    class Flash_Chip;

    class ChipReadySignalHandlerBase {
    public:
      virtual ~ChipReadySignalHandlerBase() = default;
      virtual void operator()(Flash_Chip& chip, FlashCommand& command) = 0;
    };

    template <typename T>
    class ChipReadySignalHandler : public ChipReadySignalHandlerBase {
      typedef void(T::*Handler)(Flash_Chip& chip, FlashCommand& command);

    private:
      T* __callee;
      Handler __handler;

    public:
      ChipReadySignalHandler(T* callee, Handler handler)
        : ChipReadySignalHandlerBase(),
          __callee(callee),
          __handler(handler)
      { }

      ~ChipReadySignalHandler() final = default;

      void operator()(Flash_Chip& chip, FlashCommand& command) final
      { (__callee->*__handler)(chip, command); }
    };

    typedef std::vector<ChipReadySignalHandlerBase*> ChipReadySignalHandlerList;

    class Flash_Chip : public NVM_Chip
    {
      enum class Internal_Status { IDLE, BUSY };
      enum class Chip_Sim_Event_Type { COMMAND_FINISHED };

    public:
      flash_channel_ID_type ChannelID;
      flash_chip_ID_type ChipID;         //Flashchip position in its related channel

    private:
      Flash_Technology_Type flash_technology;
      Internal_Status status;
      uint32_t idleDieNo;
      uint32_t die_no;
      uint32_t page_no_per_block;                 //indicate how many pages in a block
      std::vector<Die> Dies;

      const sim_time_type* _readLatency;
      const sim_time_type* _programLatency;
      sim_time_type _eraseLatency;

      sim_time_type _suspendProgramLatency, _suspendEraseLatency;
      sim_time_type _RBSignalDelayRead, _RBSignalDelayWrite, _RBSignalDelayErase;
      sim_time_type lastTransferStart;
      sim_time_type executionStartTime, expectedFinishTime;

      unsigned long STAT_readCount, STAT_progamCount, STAT_eraseCount;
      unsigned long STAT_totalSuspensionCount, STAT_totalResumeCount;
      sim_time_type STAT_totalExecTime, STAT_totalXferTime, STAT_totalOverlappedXferExecTime;

      ChipReadySignalHandlerList __connected_ready_handlers;

      void start_command_execution(FlashCommand* command);
      void finish_command_execution(FlashCommand* command);
      void broadcast_ready_signal(FlashCommand* command);

    public:
      Flash_Chip(const sim_object_id_type& id,
                 const FlashParameterSet& param,
                 flash_channel_ID_type channelID,
                 flash_chip_ID_type localChipID,
                 sim_time_type commProtocolDelayRead = 20,
                 sim_time_type commProtocolDelayWrite = 0,
                 sim_time_type commProtocolDelayErase = 0);
      ~Flash_Chip() final = default;

      void StartCMDXfer()
      {
        this->lastTransferStart = Simulator->Time();
      }
      void StartCMDDataInXfer()
      {
        this->lastTransferStart = Simulator->Time();
      }
      void StartDataOutXfer()
      {
        this->lastTransferStart = Simulator->Time();
      }
      void EndCMDXfer(FlashCommand* command)//End transferring write data to the Flash chip
      {
        auto sim = Simulator;

        STAT_totalXferTime += (sim->Time() - lastTransferStart);

        if (idleDieNo != die_no)
          STAT_totalOverlappedXferExecTime += (sim->Time() - lastTransferStart);

        Dies[command->Address[0].DieID].STAT_TotalXferTime += (sim->Time() - lastTransferStart);

        start_command_execution(command);

        lastTransferStart = INVALID_TIME;
      }
      void EndCMDDataInXfer(FlashCommand* command)//End transferring write data of a group of multi-plane transactions to the Flash chip
      {
        auto sim = Simulator;

        STAT_totalXferTime += (sim->Time() - lastTransferStart);
        if (idleDieNo != die_no)
          STAT_totalOverlappedXferExecTime += (sim->Time() - lastTransferStart);
        Dies[command->Address[0].DieID].STAT_TotalXferTime += (sim->Time() - lastTransferStart);

        start_command_execution(command);

        lastTransferStart = INVALID_TIME;
      }
      void EndDataOutXfer(FlashCommand* command)
      {
        auto sim = Simulator;

        STAT_totalXferTime += (sim->Time() - lastTransferStart);

        if (idleDieNo != die_no)
          STAT_totalOverlappedXferExecTime += (sim->Time() - lastTransferStart);

        Dies[command->Address[0].DieID].STAT_TotalXferTime += (sim->Time() - lastTransferStart);

        lastTransferStart = INVALID_TIME;
      }
      void Change_memory_status_preconditioning(const NVM_Memory_Address* address, const void* status_info);
      void Start_simulation();
      void Validate_simulation_config();
      void Setup_triggers();
      void Execute_simulator_event(MQSimEngine::SimEvent*);
      sim_time_type Get_command_execution_latency(command_code_type CMDCode, flash_page_ID_type pageID)
      {
        int latencyType = 0;
        if (flash_technology == Flash_Technology_Type::MLC)
        {
          latencyType = pageID % 2;
        }
        else if (flash_technology == Flash_Technology_Type::TLC)
        {
          //From: Yaakobi et al., "Characterization and Error-Correcting Codes for TLC Flash Memories", ICNC 2012
          latencyType = (pageID <= 5) ? 0 : ((pageID <= 7) ? 1 : (((pageID - 8) >> 1U) % 3));;
        }

        switch (CMDCode)
        {
        case CMD_READ_PAGE:
        case CMD_READ_PAGE_MULTIPLANE:
        case CMD_READ_PAGE_COPYBACK:
        case CMD_READ_PAGE_COPYBACK_MULTIPLANE:
          return _readLatency[latencyType] + _RBSignalDelayRead;
        case CMD_PROGRAM_PAGE:
        case CMD_PROGRAM_PAGE_MULTIPLANE:
        case CMD_PROGRAM_PAGE_COPYBACK:
        case CMD_PROGRAM_PAGE_COPYBACK_MULTIPLANE:
          return _programLatency[latencyType] + _RBSignalDelayWrite;
        case CMD_ERASE_BLOCK:
        case CMD_ERASE_BLOCK_MULTIPLANE:
          return _eraseLatency + _RBSignalDelayErase;
        default:
          throw std::invalid_argument("Unsupported command for flash chip.");
        }
      }
      void Suspend(flash_die_ID_type dieID);
      void Resume(flash_die_ID_type dieID);
      sim_time_type GetSuspendProgramTime();
      sim_time_type GetSuspendEraseTime();
      void Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter);
      LPA_type Get_metadata(flash_die_ID_type die_id, flash_plane_ID_type plane_id, flash_block_ID_type block_id, flash_page_ID_type page_id);//A simplification to decrease the complexity of GC execution! The GC unit may need to know the metadata of a page to decide if a page is valid or invalid.

      void connect_to_chip_ready_signal(ChipReadySignalHandlerBase& handler);
    };

    typedef std::vector<Flash_Chip> FlashChipList;

    force_inline void
    Flash_Chip::connect_to_chip_ready_signal(ChipReadySignalHandlerBase& handler)
    {
      __connected_ready_handlers.emplace_back(&handler);
    }
  }
}

#endif // !FLASH_CHIP_H
