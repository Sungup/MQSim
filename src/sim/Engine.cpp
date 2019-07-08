#include <stdexcept>
#include "Engine.h"
#include "../utils/Logical_Address_Partitioning_Unit.h"

using namespace MQSimEngine;

Engine* Engine::_instance = nullptr;


void Engine::Reset()
{
  __pool.destroy_all();

  _EventList->Clear();
  _ObjectList.clear();
  _sim_time = 0;
  stop = false;
  started = false;
}

//Add an object to the simulator object list
void Engine::AddObject(Sim_Object* obj)
{
  if (_ObjectList.find(obj->ID()) != _ObjectList.end())
    throw std::invalid_argument("Duplicate object key: " + obj->ID());
  _ObjectList.insert(std::pair<sim_object_id_type, Sim_Object*>(obj->ID(), obj));
}

Sim_Object* Engine::GetObject(const sim_object_id_type& object_id)
{
  auto itr = _ObjectList.find(object_id);

  if (itr == _ObjectList.end())
    return nullptr;

  return (*itr).second;
}

void Engine::RemoveObject(Sim_Object* obj)
{
  auto it = _ObjectList.find(obj->ID());

  if (it == _ObjectList.end())
    throw std::invalid_argument("Removing an unregistered object.");

  _ObjectList.erase(it);
}

/// This is the main method of simulator which starts simulation process.
void Engine::Start_simulation()
{
  started = true;

  for (auto& obj : _ObjectList)
    if (!obj.second->IsTriggersSetUp())
      obj.second->Setup_triggers();

  for (auto& obj : _ObjectList)
    obj.second->Validate_simulation_config();

  for (auto& obj : _ObjectList)
    obj.second->Start_simulation();

  while (!stop && _EventList->is_exist()) {
    EventTreeNode* minNode = _EventList->Get_min_node();
    auto* ev = minNode->FirstSimEvent;

    _sim_time = ev->Fire_time;

    while (ev != nullptr) {
      if(!ev->Ignore)
        ev->Target_sim_object->Execute_simulator_event(ev);

      SimEvent* consumed_event = ev;
      ev = ev->Next_event;

      consumed_event->release();
    }

    _EventList->Remove(minNode);
  }
}

void Engine::Stop_simulation()
{
  stop = true;
}

void Engine::Ignore_sim_event(SimEvent* ev)
{
  ev->Ignore = true;
}
