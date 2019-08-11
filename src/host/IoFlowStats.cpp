//
// IoFlowStats
// MQSim
//
// Created by 文盛業 on 2019-08-11.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#include "IoFlowStats.h"

#include "../sim/Engine.h"

using namespace std;
using namespace Host_Components;

void
IoFlowStats::Report_results_in_XML(string /* name_prefix */,
                                   Utils::XmlWriter& writer)
{
  double seconds = Simulator->seconds();

  // 1. Generated requests
  auto gen_reqs = __generated_reads + __generated_writes;

  writer.Write_attribute_string("Request_Count", gen_reqs.count());

  writer.Write_attribute_string("Read_Request_Count",
                                __generated_reads.count());

  writer.Write_attribute_string("Write_Request_Count",
                                __generated_writes.count());

  writer.Write_attribute_string("IOPS", gen_reqs.iops(seconds));

  writer.Write_attribute_string("IOPS_Read",
                                __generated_reads.iops(seconds));

  writer.Write_attribute_string("IOPS_Write",
                                __generated_writes.iops(seconds));

  // 2. Transferred IOs
  auto transferred = __transferred_reads + __transferred_writes;

  writer.Write_attribute_string("Bytes_Transferred", transferred.sum());

  writer.Write_attribute_string("Bytes_Transferred_Read",
                                __transferred_reads.sum());

  writer.Write_attribute_string("Bytes_Transferred_Write",
                                __transferred_writes.sum());

  writer.Write_attribute_string("Bandwidth",
                                transferred.bandwidth(seconds));

  writer.Write_attribute_string("Bandwidth_Read",
                                __transferred_reads.bandwidth(seconds));

  writer.Write_attribute_string("Bandwidth_Write",
                                __transferred_writes.bandwidth(seconds));

  // 3. Response time
  auto dev_resp  = __dev_rd_response_time + __dev_wr_response_time;

  writer.Write_attribute_string("Device_Response_Time",
                                dev_resp.avg(NSEC_TO_USEC_COEFF));

  writer.Write_attribute_string("Min_Device_Response_Time",
                                dev_resp.min(NSEC_TO_USEC_COEFF));

  writer.Write_attribute_string("Max_Device_Response_Time",
                                dev_resp.max(NSEC_TO_USEC_COEFF));

  // 4. Request delay time
  auto req_delay = __rd_req_delay + __wr_req_delay;
  writer.Write_attribute_string("End_to_End_Request_Delay",
                                req_delay.avg(NSEC_TO_USEC_COEFF));

  writer.Write_attribute_string("Min_End_to_End_Request_Delay",
                                req_delay.min(NSEC_TO_USEC_COEFF));

  writer.Write_attribute_string("Max_End_to_End_Request_Delay",
                                req_delay.max(NSEC_TO_USEC_COEFF));

}