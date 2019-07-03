#ifndef DIE_H
#define DIE_H

#include "../../exec/params/FlashParameterSet.h"
#include "../../sim/SimEvent.h"
#include "FlashTypes.h"
#include "Plane.h"

namespace NVM
{
  namespace FlashMemory
  {
    enum class DieStatus { BUSY, IDLE };

    class Die
    {
    public:
      std::vector<Plane> Planes;
      DieStatus Status;
      MQSimEngine::SimEvent* CommandFinishEvent;
      sim_time_type Expected_finish_time;
      sim_time_type RemainingSuspendedExecTime;//used to support suspend command

      FlashCommand* CurrentCMD;
      FlashCommand* SuspendedCMD;

      bool Suspended;

      sim_time_type STAT_TotalProgramTime;
      sim_time_type STAT_TotalReadTime;
      sim_time_type STAT_TotalEraseTime;
      sim_time_type STAT_TotalXferTime;

      explicit Die(const FlashParameterSet& params);

    };

    force_inline
    Die::Die(const FlashParameterSet& params)
      : Status(DieStatus::IDLE),
        CommandFinishEvent(nullptr),
        Expected_finish_time(INVALID_TIME),
        RemainingSuspendedExecTime(INVALID_TIME),
        CurrentCMD(nullptr),
        SuspendedCMD(nullptr),
        Suspended(false),
        STAT_TotalProgramTime(0),
        STAT_TotalReadTime(0),
        STAT_TotalEraseTime(0),
        STAT_TotalXferTime(0)
    {
      Planes.reserve(params.Plane_No_Per_Die);
      for (uint32_t plane = 0; plane < params.Plane_No_Per_Die; ++plane)
        Planes.emplace_back(params);
    }
  }
}
#endif // !DIE_H
