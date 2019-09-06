#include "PCIeLink.h"

#include "../../sim/Engine.h"
#include "../../sim/Sim_Defs.h"

#include "PCIeMessage.h"
#include "PCIePort.h"

using namespace Host_Components;

PCIeLink::PCIeLink(const sim_object_id_type& id,
                   double lane_bandwidth_GBPs,
                   int lane_count,
                   int tlp_header_size,
                   int tlp_max_payload_size,
                   int dllp_ovehread,
                   int ph_overhead)
  : Sim_Object(id),
    __msg_pool(),
    __ports { nullptr, nullptr },
    __lane_bandwidth(lane_bandwidth_GBPs),
    __lane_count(lane_count),
#if UNBLOCK_NOT_IN_USE
    tlp_header_size(tlp_header_size),
    dllp_ovehread(dllp_ovehread),
    ph_overhead(ph_overhead),
#endif
    __tlp_max_payload_size(tlp_max_payload_size),
    __packet_overhead(ph_overhead + dllp_ovehread + tlp_header_size)
{ }

void PCIeLink::Execute_simulator_event(MQSimEngine::SimEvent* event)
{
  PCIeMessage* message = nullptr;
  auto destination = PCIeDest(intptr_t(event->Parameters));

  auto& buffer = destination == PCIeDest::HOST
                   ? __root_complex_to_ssd_buffer
                   : __ssd_to_root_complex_buffer;

  message = buffer.front();
  buffer.pop();

  __ports[destination]->drain_message(message);

  // There are active transfers
  if (!buffer.empty())
    __register_next_event(destination,
                          __estimate_transfer_time(buffer.front()));
}
