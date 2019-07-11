#ifndef LOGICAL_ADDRESS_PARTITIONING_UNIT_H
#define LOGICAL_ADDRESS_PARTITIONING_UNIT_H

#include <cstdint>
#include <vector>

#include "../exec/params/IOFlowParamSet.h"
#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "../sim/Sim_Defs.h"
#include "../ssd/SSD_Defs.h"
#include "../ssd/interface/Host_Interface_Defs.h"

#include "InlineTools.h"

/**
 * MQSim requires the logical address space of the SSD device to be partitioned
 * among the concurrent flows. In fact, two different storage traces may access
 * the same logical address, but this logical address should not be assumed to
 * be identical when the traces are executed together.
 */

namespace Utils
{
  /// LogicalAddressPartitionUnit's initializer always called at SSD's
  /// initializing phase. Also reset will called before its initializing phase.
  /// Maybe, I think dirty data make some errors, and author should call the
  /// reset function before its initialization.
  /// But, there is no reason the LogicalAddressPartitionUnit should placed on
  /// the static scope. So I move the LogicalAddressPartitionUnit move into the
  /// SSD_Device as a member variable to reduce the static variable collision
  /// between multiple SSD instance.
  class LogicalAddrPartition {
    typedef std::vector<int>                 ResourceListOnPlane;
    typedef std::vector<ResourceListOnPlane> ResourceListOnDie;
    typedef std::vector<ResourceListOnDie>   ResourceListOnChip;
    typedef std::vector<ResourceListOnChip>  ResourceListOnChannel;

  private:
    const HostInterface_Types __host_interface_type;

    LHA_type __total_pda_no;
    LHA_type __total_lha_no;

    std::vector<LHA_type> __pdas_per_flow;
    std::vector<LHA_type> __start_lhas_per_flow;
    std::vector<LHA_type> __end_lhas_per_flow;

    ResourceListOnChannel __resource_list;

  public:
    LogicalAddrPartition(const DeviceParameterSet& params,
                                const StreamIdInfo& stream_info,
                                uint32_t stream_count);

    LHA_type available_start_lha(stream_id_type stream) const;
    LHA_type available_end_lha(stream_id_type stream) const;
    LHA_type allocated_lha_count_from_host(stream_id_type stream) const;
    LHA_type allocated_lha_count_from_device(stream_id_type stream) const;
    LHA_type allocated_pda_count(stream_id_type stream) const;

    double share_of_physical_pages(flash_channel_ID_type channel_id,
                                   flash_chip_ID_type chip_id,
                                   flash_die_ID_type die_id,
                                   flash_plane_ID_type plane_id) const;

    LHA_type total_device_lha_count() const;
  };

  force_inline LHA_type
  LogicalAddrPartition::available_start_lha(stream_id_type stream) const
  {
    // There are no difference between SATA & NVME code
    return __start_lhas_per_flow[stream];
  }

  force_inline LHA_type
  LogicalAddrPartition::available_end_lha(stream_id_type stream) const
  {
    // There are no difference between SATA & NVME code
    return __end_lhas_per_flow[stream];
  }

  force_inline LHA_type
  LogicalAddrPartition::allocated_lha_count_from_host(stream_id_type stream) const
  {
    // There are no difference between SATA & NVME code
    return (__end_lhas_per_flow[stream] - __start_lhas_per_flow[stream] + 1);
  }

  force_inline LHA_type
  LogicalAddrPartition::allocated_lha_count_from_device(stream_id_type stream) const
  {
    return (__host_interface_type == HostInterface_Types::SATA)
             ? __total_lha_no
             : __end_lhas_per_flow[stream] - __start_lhas_per_flow[stream] + 1;
  }

  force_inline LHA_type
  LogicalAddrPartition::allocated_pda_count(stream_id_type stream) const
  {
    return (__host_interface_type == HostInterface_Types::SATA)
             ? __total_pda_no
             : __pdas_per_flow[stream];
  }

  force_inline double
  LogicalAddrPartition::share_of_physical_pages(flash_channel_ID_type channel,
                                                flash_chip_ID_type chip,
                                                flash_die_ID_type die,
                                                flash_plane_ID_type plane) const
  {
    if (__host_interface_type == HostInterface_Types::NVME)
      return 1.0 / double(__resource_list[channel][chip][die][plane]);

    return 1.0;
  }

  force_inline LHA_type
  LogicalAddrPartition::total_device_lha_count() const
  {
    return __total_lha_no;
  }

}

#endif // !LOGICAL_ADDRESS_PARTITIONING_UNIT_H

