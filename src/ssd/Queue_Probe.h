#ifndef QUEUE_PROBE_H
#define QUEUE_PROBE_H

#include <unordered_map>
#include <vector>
#include <string>
#include "../sim/Sim_Defs.h"
#include "NvmTransaction.h"
#include "../utils/XMLWriter.h"

namespace SSD_Components
{
  class StateStatistics
  {
  public:
    StateStatistics();
    unsigned long nEnterances;
    sim_time_type totalTime;
  };

  class Queue_Probe
  {
  private:
    typedef std::unordered_map<NvmTransaction*, sim_time_type> TransactionTimeMap;
    uint32_t count = 0;
    unsigned long nRequests = 0;
    unsigned long nDepartures = 0;
    sim_time_type totalWaitingTime = 0;
    sim_time_type lastCountChange = 0;
    sim_time_type lastCountChangeReference = 0;
    unsigned long nRequestsEpoch = 0;
    unsigned long nDeparturesEpoch = 0;
    sim_time_type totalWaitingTimeEpoch = 0;
    sim_time_type epochStartTime;
    std::vector<StateStatistics> states;
    std::vector<StateStatistics> statesEpoch;
    TransactionTimeMap currentObjectsInQueue;
    uint32_t maxQueueLength = 0;
    sim_time_type maxWaitingTime = 0;

    void setCount(int val);

  public:
    Queue_Probe();
    void EnqueueRequest(NvmTransaction* transaction);
    void DequeueRequest(NvmTransaction* transaction);
    void ResetEpochStatistics();
    void Snapshot(std::string id, Utils::XmlWriter& writer);
    unsigned long NRequests();
    unsigned long NDepartures();
    int QueueLength();
    std::vector<StateStatistics> States();
    uint32_t MaxQueueLength();
    double AvgQueueLength();
    double STDevQueueLength();
    double AvgQueueLengthEpoch();
    sim_time_type MaxWaitingTime();
    sim_time_type AvgWaitingTime();
    sim_time_type AvgWaitingTimeEpoch();
    sim_time_type TotalWaitingTime();
  };

}

#endif // !QUEUE_PROBE
