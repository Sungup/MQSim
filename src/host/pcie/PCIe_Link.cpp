#include "PCIe_Link.h"

#include "../../sim/Engine.h"
#include "../../sim/Sim_Defs.h"

#include "PCIeMessage.h"
#include "PCIePort.h"

using namespace Host_Components;

PCIe_Link::PCIe_Link(const sim_object_id_type& id,
                     double lane_bandwidth_GBPs,
                     int lane_count,
                     int tlp_header_size,
                     int tlp_max_payload_size,
                     int dllp_ovehread,
                     int ph_overhead)
  : Sim_Object(id),
    __msg_pool(),
    __ports { nullptr, nullptr },
    lane_bandwidth_GBPs(lane_bandwidth_GBPs),
    lane_count(lane_count),
#if UNBLOCK_NOT_IN_USE
    tlp_header_size(tlp_header_size),
    dllp_ovehread(dllp_ovehread),
    ph_overhead(ph_overhead),
#endif
    tlp_max_payload_size(tlp_max_payload_size),
    packet_overhead(ph_overhead + dllp_ovehread + tlp_header_size)
{ }

void PCIe_Link::Execute_simulator_event(MQSimEngine::SimEvent* event)
{
  PCIeMessage* message = nullptr;
  auto destination = PCIeDest(intptr_t(event->Parameters));

  auto& buffer = destination == PCIeDest::HOST
                   ? Message_buffer_toward_root_complex
                   : Message_buffer_toward_ssd_device;

  message = buffer.front();
  buffer.pop();

  __ports[destination]->drain_message(message);

  // There are active transfers
  if (!buffer.empty())
    __register_next_event(destination,
                          __estimate_transfer_time(buffer.front()));
}
