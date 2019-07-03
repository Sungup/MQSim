#ifndef SIMULATOR_EVENT_H
#define SIMULATOR_EVENT_H

#include "Sim_Defs.h"

#include "../utils/ObjectPool.h"

namespace MQSimEngine
{
  class Sim_Object;
  class SimEventBase;

  typedef Utils::ObjectPool<SimEventBase> SimEventPool;
  typedef SimEventPool::item_t            SimEvent;

  class SimEventBase
  {
  public:
    SimEventBase(sim_time_type fireTime,
                 Sim_Object* targetObject,
                 void* parameters = nullptr,
                 int type = 0)
      : Fire_time(fireTime),
        Target_sim_object(targetObject),
        Parameters(parameters),
        Type(type),
        Next_event(nullptr),
        Ignore(false)
    { }

    sim_time_type Fire_time;
    Sim_Object* Target_sim_object;
    void* Parameters = nullptr;
    int Type;

    //Used to store event list in the MQSim's engine
    SimEvent* Next_event;

    //If true, this event will not be executed
    bool Ignore;
  };

}

#endif // !SIMULATOR_EVENT_H
