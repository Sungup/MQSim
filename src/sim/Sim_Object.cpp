//
// Sim_Object
// MQSim
//
// Created by 文盛業 on 2019-06-13.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#include "Sim_Object.h"

using namespace MQSimEngine;

void
Sim_Object::Validate_simulation_config()
{ /* Default don't do anything */ }

void
Sim_Object::Setup_triggers()
{
  __triggersSetUp = true;
}

void
Sim_Object::Execute_simulator_event(MQSimEngine::Sim_Event* /* event */)
{ /* Default don't do anything */ }
