#ifndef ENGINE_H
#define ENGINE_H

#include <cstring>
#include <iostream>
#include <unordered_map>
#include "Sim_Defs.h"
#include "SimEvent.h"
#include "EventTree.h"
#include "Sim_Object.h"

#define Simulator MQSimEngine::Engine::Instance()

namespace MQSimEngine {
  class Engine
  {
    friend class EventTree;

  private:
    static Engine* _instance;

    sim_time_type _sim_time;
    EventTree* _EventList;
    std::unordered_map<sim_object_id_type, Sim_Object*> _ObjectList;
    bool stop;
    bool started;

    SimEventPool __pool;

  public:
    Engine();
    ~Engine();

    static Engine* Instance();
    sim_time_type Time() const;
    double seconds() const;

    SimEvent* Register_sim_event(sim_time_type fireTime,
                                 Sim_Object* targetObject,
                                 void* parameters = nullptr,
                                 int type = 0);
    void Ignore_sim_event(SimEvent*);
    void Reset();
    void AddObject(Sim_Object* obj);
    Sim_Object* GetObject(const sim_object_id_type& object_id);
    void RemoveObject(Sim_Object* obj);
    void Start_simulation();
    void Stop_simulation();
    bool Has_started() const;
    bool Is_integrated_execution_mode() const;

  };

  force_inline
  Engine::Engine()
    : _sim_time(0),
      _EventList(new EventTree()),
      _ObjectList(),
      stop(false),
      started(false)
  { }

  force_inline
  Engine::~Engine()
  {
    delete _EventList;
  }

  force_inline Engine*
  Engine::Instance() {
    if (_instance == nullptr)
      _instance = new Engine();

    return _instance;
  }

  force_inline SimEvent*
  Engine::Register_sim_event(sim_time_type fireTime,
                             Sim_Object* targetObject,
                             void* parameters,
                             int type)
  {
    auto* ev = __pool.construct(fireTime, targetObject, parameters, type);

    PRINT_DEBUG("RegisterEvent " << fireTime << " " << targetObject)

    _EventList->Insert_sim_event(ev);

    return ev;
  }

  force_inline sim_time_type
  Engine::Time() const
  {
    return _sim_time;
  }

  force_inline double
  Engine::seconds() const
  {
    return double(_sim_time) / NSEC_TO_SEC_COEFF;
  }

  force_inline bool
  Engine::Has_started() const
  {
    return started;
  }

  force_inline bool
  Engine::Is_integrated_execution_mode() const
  {
    return false;
  }

  force_inline void*
  copy_data(void* src, size_t size) {
    void* dest = src;

#if UNBLOCK_NOT_IN_USE
    // TODO This block can make the memory leak while runnign simulator.
    if (Simulator->Is_integrated_execution_mode()) {
      dest = new char[size];
      memcpy(dest, src, size);
    }
#endif

    return dest;
  }
}

#endif // !ENGINE_H
