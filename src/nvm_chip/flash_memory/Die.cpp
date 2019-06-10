#include "Die.h"

namespace NVM
{
  namespace FlashMemory
  {
    Die::Die(uint32_t PlanesNoPerDie, uint32_t BlocksNoPerPlane, uint32_t PagesNoPerBlock) :
      Plane_no(PlanesNoPerDie),
      Status(DieStatus::IDLE), CommandFinishEvent(NULL), Expected_finish_time(INVALID_TIME), RemainingSuspendedExecTime(INVALID_TIME),
      CurrentCMD(NULL), SuspendedCMD(NULL), Suspended(false),
      STAT_TotalProgramTime(0), STAT_TotalReadTime(0), STAT_TotalEraseTime(0), STAT_TotalXferTime(0)
    {
      Planes = new Plane*[PlanesNoPerDie];
      for (uint32_t i = 0; i < PlanesNoPerDie; i++)
        Planes[i] = new Plane(BlocksNoPerPlane, PagesNoPerBlock);
    }

    Die::~Die()
    {
      for (uint32_t planeID = 0; planeID < Plane_no; planeID++)
        delete Planes[planeID];
      delete[] Planes;
    }
  }
}