#ifndef DIE_H
#define DIE_H

#include "../../sim/Sim_Event.h"
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
      Die(uint32_t PlanesNoPerDie, uint32_t BlocksNoPerPlane, uint32_t PagesNoPerBlock);
      ~Die();
      Plane** Planes;
      uint32_t Plane_no;
      DieStatus Status;
      MQSimEngine::Sim_Event* CommandFinishEvent;
      sim_time_type Expected_finish_time;
      sim_time_type RemainingSuspendedExecTime;//used to support suspend command
      Flash_Command* CurrentCMD, *SuspendedCMD;
      bool Suspended;

      sim_time_type STAT_TotalProgramTime, STAT_TotalReadTime, STAT_TotalEraseTime, STAT_TotalXferTime;
    };
  }
}
#endif // !DIE_H
