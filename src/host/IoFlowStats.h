//
// IoFlowStats
// MQSim
//
// Created by 文盛業 on 2019-08-11.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__IOFlowStats__
#define __MQSim__IOFlowStats__

#include <cstdint>
#include <string>

#include "../sim/Sim_Defs.h"
#include "../sim/Sim_Reporter.h"
#include "../utils/CountingStats.h"

#include "HostIORequest.h"

namespace Host_Components {
  class IoFlowStats : public MQSimEngine::Sim_Reporter {
  private:
    //Variables used to collect statistics
    uint32_t __generated_req;
    uint32_t __serviced_req;

    Utils::IopsStats __generated_reads;
    Utils::IopsStats __generated_writes;

#if UNBLOCK_NOT_IN_USE
    Utils::IopsStats __generated_ignored;
#endif

    Utils::IopsStats __serviced_reads;
    Utils::IopsStats __serviced_writes;

    Utils::MinMaxAvgStats<sim_time_type> __dev_rd_response_time;
    Utils::MinMaxAvgStats<sim_time_type> __dev_wr_response_time;
    Utils::MinMaxAvgStats<sim_time_type> __rd_req_delay;
    Utils::MinMaxAvgStats<sim_time_type> __wr_req_delay;

    Utils::BandwidthStats<sim_time_type> __transferred_reads;
    Utils::BandwidthStats<sim_time_type> __transferred_writes;

  public:
    IoFlowStats();

    void update_response(const HostIORequest* request,
                         sim_time_type dev_response,
                         sim_time_type req_delay);

    void update_generate(Host_Components::HostIOReqType type);

    uint32_t generated_req() const;
    uint32_t serviced_req() const;
    uint32_t avg_response_time() const;
    uint32_t avg_request_delay() const;

    void Report_results_in_XML(std::string name_prefix,
                               Utils::XmlWriter& writer);
  };

  force_inline
  IoFlowStats::IoFlowStats()
    : __generated_req(0),
      __serviced_req(0),
      __generated_reads(),
      __generated_writes(),
      __serviced_reads(),
      __serviced_writes(),
      __dev_rd_response_time(MAXIMUM_TIME, 0),
      __dev_wr_response_time(MAXIMUM_TIME, 0),
      __rd_req_delay(MAXIMUM_TIME, 0),
      __wr_req_delay(MAXIMUM_TIME, 0),
      __transferred_reads(),
      __transferred_writes()
  { }

  force_inline void
  IoFlowStats::update_response(const HostIORequest *request,
                               sim_time_type dev_response,
                               sim_time_type req_delay)
  {
    ++__serviced_req;

    if (request->Type == HostIOReqType::READ) {
      ++__serviced_reads;
      __dev_rd_response_time += dev_response;
      __rd_req_delay += req_delay;
      __transferred_reads += request->requested_size();
    } else {
      ++__serviced_writes;
      __dev_wr_response_time += dev_response;
      __wr_req_delay += req_delay;
      __transferred_writes += request->requested_size();
    }
  }

  force_inline void
  IoFlowStats::update_generate(Host_Components::HostIOReqType type)
  {
    ++__generated_req;

    if (type == HostIOReqType::WRITE)
      ++__generated_writes;
    else
      ++__generated_reads;
  }

  force_inline uint32_t
  IoFlowStats::generated_req() const
  { return __generated_req; }

  force_inline uint32_t
  IoFlowStats::serviced_req() const
  { return __serviced_req; }

  force_inline uint32_t
  IoFlowStats::avg_response_time() const
  {
    auto response_time = __dev_rd_response_time + __dev_wr_response_time;

    return response_time.avg(SIM_TIME_TO_MICROSECONDS_COEFF);
  }

  force_inline uint32_t
  IoFlowStats::avg_request_delay() const
  {
    auto req_delay = __rd_req_delay + __wr_req_delay;

    return req_delay.avg(SIM_TIME_TO_MICROSECONDS_COEFF);
  }

}

#endif /* Predefined include guard __MQSim__IOFlowStats__ */
