#ifndef PCIE_LINK_H
#define PCIE_LINK_H

#include <queue>

#include "../../sim/Sim_Defs.h"
#include "../../sim/Sim_Object.h"
#include "../../sim/SimEvent.h"

#include "PCIeMessage.h"

namespace Host_Components
{
  class PCIePortBase;

  enum class PCIe_Link_Event_Type {DELIVER};

  class PCIeLink : public MQSimEngine::Sim_Object
  {
  private:
    PCIeMessagePool __msg_pool;

    PCIePortBase* __ports[PCIeDest::DEST_MAX];

    double __lane_bandwidth; // GB/s -> MB/msec -> KB/usec -> B/nsec
    uint32_t __lane_count;

#if UNBLOCK_NOT_IN_USE
    int tlp_header_size;
    int dllp_ovehread;
    int ph_overhead;
#endif

    uint32_t __tlp_max_payload_size;
    uint32_t __packet_overhead;

    // Since the transfer delay of one byte may take lower than one nano-second,
    // we use 8-byte metric
    // sim_time_type byte_transfer_delay_per_lane;

    std::queue<PCIeMessage*> __ssd_to_root_complex_buffer;
    std::queue<PCIeMessage*> __root_complex_to_ssd_buffer;

  private:
    uint32_t __ceiling_div(uint32_t a, uint32_t b) const;
    uint32_t __payload_size(PCIeMessage* message) const;
    sim_time_type __estimate_transfer_time(PCIeMessage* message) const;

    void __register_next_event(PCIeDest target, uint32_t after_at);
  public:
    explicit PCIeLink(const sim_object_id_type& id,
                      double lane_bandwidth_GBPs = 1,
                      int lane_count = 4,
                      int tlp_header_size = 20,//tlp header size in a 64-bit machine
                      int tlp_max_payload_size = 128,
                      int dllp_ovehread = 6,
                      int ph_overhead = 2);

    ~PCIeLink() final = default;

    void Execute_simulator_event(MQSimEngine::SimEvent*) final;

    void set_port(PCIePortBase* port, PCIeDest port_destination);

    void deliver(PCIeMessage* message);

    // Share the common message pool to communicating host and device through
    // this link.
    PCIeMessagePool& shared_pcie_message_pool();

  };

  force_inline uint32_t
  PCIeLink::__ceiling_div(uint32_t a, uint32_t b) const
  {
    return (a + b - 1) / b;
  }

  force_inline uint32_t
  PCIeLink::__payload_size(PCIeMessage *message) const
  {
    if (message->type == PCIe_Message_Type::READ_REQ)
      return __packet_overhead + 4;
    else
      return message->payload_size
               + (__ceiling_div(message->payload_size, __tlp_max_payload_size)
                    * __packet_overhead);
  }

  force_inline sim_time_type
  PCIeLink::__estimate_transfer_time(PCIeMessage* message) const
  {
    uint32_t payload_size_per_lane = __ceiling_div(__payload_size(message),
                                                   __lane_count);

    return sim_time_type(payload_size_per_lane / __lane_bandwidth);
  }

  force_inline void
  PCIeLink::__register_next_event(PCIeDest target, uint32_t after_at)
  {
    auto sim = Simulator;
    sim->Register_sim_event(sim->Time() + after_at,
                            this, (void*)(intptr_t(target)),
                            static_cast<int>(PCIe_Link_Event_Type::DELIVER));
  }

  force_inline void
  PCIeLink::deliver(PCIeMessage* message)
  {
    auto& buffer = message->destination == PCIeDest::HOST
                     ? __root_complex_to_ssd_buffer
                     : __ssd_to_root_complex_buffer;

    buffer.push(message);

    // First buffer entry, wake up the simulator event handler.
    if (buffer.size() == 1)
      __register_next_event(message->destination,
                            __estimate_transfer_time(message));
  }

  force_inline void
  PCIeLink::set_port(PCIePortBase *port, PCIeDest port_destination)
  {
    __ports[port_destination] = port;
  }

  force_inline PCIeMessagePool&
  PCIeLink::shared_pcie_message_pool()
  {
    return __msg_pool;
  }

}

#endif