#ifndef SIMULATOR_OBJECT_H
#define SIMULATOR_OBJECT_H

#include <string>
#include "Sim_Event.h"
#include "Sim_Reporter.h"

#include "../utils/InlineTools.h"

namespace MQSimEngine
{
  class Sim_Event;
  class Sim_Object : public Sim_Reporter
  {
  private:
    sim_object_id_type __id;
    bool __triggersSetUp;

  public:
    explicit Sim_Object(const sim_object_id_type &id);
    virtual ~Sim_Object() = default;

    sim_object_id_type ID();

    bool IsTriggersSetUp();

    // The Start function is invoked at the start phase of simulation
    // to perform initialization
    virtual void Start_simulation() = 0;

    // The Validate_simulation_config function is invoked to check
    // if the objected is correctly configured or not.
    virtual void Validate_simulation_config();

    // The object connects its internal functions to the outside
    // triggering events from other objects
    virtual void Setup_triggers();

    virtual void Execute_simulator_event(Sim_Event* event);
     
  };

  force_inline
  Sim_Object::Sim_Object(const sim_object_id_type& id)
    : __id(id),
      __triggersSetUp(false)
  { }

  force_inline sim_object_id_type
  Sim_Object::ID()
  {
    return __id;
  }

  force_inline bool
  Sim_Object::IsTriggersSetUp()
  {
    return __triggersSetUp;
  }
}

#endif // !SIMULATOR_OBJECT_H
