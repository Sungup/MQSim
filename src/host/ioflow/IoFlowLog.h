//
// IoFlowLog
// MQSim
//
// Created by 文盛業 on 2019-08-11.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__IoFlowLog__
#define __MQSim__IoFlowLog__

#include <fstream>
#include <string>

#include "../../exec/params/HostParameterSet.h"
#include "../../sim/Sim_Defs.h"
#include "../../utils/CountingStats.h"

namespace Host_Components {
  class IoFlowLog {
  private:
    const bool          __enable;
    const sim_time_type __period;
    const std::string   __path;

    sim_time_type __logging_at;
    std::ofstream __log;

    Utils::AvgStats<sim_time_type> __dev_response;
    Utils::AvgStats<sim_time_type> __req_delay;

  public:
    IoFlowLog(const HostParameterSet& params, uint16_t flow_id);

    void init();

    void logging(sim_time_type now);

    void update_response(sim_time_type dev_response, sim_time_type req_delay);
  };

  force_inline void
  IoFlowLog::logging(sim_time_type now)
  {
    if (__logging_at < now) {
      __log << now / NSEC_TO_USEC_COEFF << "\t"
            << __dev_response.avg(NSEC_TO_USEC_COEFF) << "\t"
            << __req_delay.avg(NSEC_TO_USEC_COEFF) <<std::endl;

      __dev_response.reset();
      __req_delay.reset();

      __logging_at = now + __period;
    }
  }

  force_inline void
  IoFlowLog::update_response(sim_time_type dev_response,
                             sim_time_type req_delay)
  {
    __dev_response += dev_response;
    __req_delay += req_delay;
  }
}

#endif /* Predefined include guard __MQSim__IoFlowLog__ */
