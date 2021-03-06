//
// IoFlowLog
// MQSim
//
// Created by 文盛業 on 2019-08-11.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#include "IoFlowLog.h"

using namespace std;
using namespace Host_Components;

IoFlowLog::IoFlowLog(const HostParameterSet& params, uint16_t flow_id)
  : __enable(params.Enable_ResponseTime_Logging),
    __period(__enable ? params.ResponseTime_Logging_Period : MAXIMUM_TIME),
    __path(params.stream_log_path(flow_id)),
    __logging_at(0),
    __log(),
    __dev_response(),
    __req_delay()
{ }

void
IoFlowLog::init()
{
  __logging_at = __period;

  if (__enable) {
    __log.open(__path, std::ofstream::out);
    __log << "SimulationTime(us)\t" << "ReponseTime(us)\t" << "EndToEndDelay(us)"<< endl;
  }

  __dev_response.reset();
  __req_delay.reset();
}