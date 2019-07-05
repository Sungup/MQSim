#ifndef LOGICAL_ADDRESS_PARTITIONING_UNIT_H
#define LOGICAL_ADDRESS_PARTITIONING_UNIT_H

#include <cstdint>
#include <vector>
#include "../sim/Sim_Defs.h"
#include "../ssd/SSD_Defs.h"
#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "../ssd/interface/Host_Interface_Defs.h"
#include "InlineTools.h"

/**
 * MQSim requires the logical address space of the SSD device to be partitioned
 * among the concurrent flows. In fact, two different storage traces may access
 * the same logical address, but this logical address should not be assumed to
 * be identical when the traces are executed together.
 */

// TODO Remove static features

namespace Utils
{
  /// LogicalAddressPartitionUnit's initializer always called at SSD's
  /// initializing phase. Also reset will called before its initializing phase.
  /// Maybe, I think dirty data make some errors, and author should call the
  /// reset function before its initialization.
  /// But, there is no reason the LogicalAddressPartitionUnit should placed on
  /// the static scope. So I move the LogicalAddressPartitionUnit move into the
  /// SSD_Device as a member variable to reduce the static variable collison
  /// between multiple SSD instance.
  class LogicalAddressPartitionUnit {
    typedef std::vector<int>                 ResourceListOnPlane;
    typedef std::vector<ResourceListOnPlane> ResourceListOnDie;
    typedef std::vector<ResourceListOnDie>   ResourceListOnChip;
    typedef std::vector<ResourceListOnChip>  ResourceListOnChannel;

  private:
    const HostInterface_Types __hostinterface_type;

    LHA_type __total_pda_no;
    LHA_type __total_lha_no;

    std::vector<LHA_type> __pdas_per_flow;
    std::vector<LHA_type> __start_lhas_per_flow;
    std::vector<LHA_type> __end_lhas_per_flow;

    ResourceListOnChannel __resource_list;

  public:
    LogicalAddressPartitionUnit(HostInterface_Types host_interface_type,
                                uint32_t concurrent_streams,
                                uint32_t channels,
                                uint32_t chips_on_channel,
                                uint32_t dies_on_chip,
                                uint32_t planes_on_die,
                                uint32_t blocks_on_plane,
                                uint32_t pages_on_block,
                                uint32_t sectors_on_page,
                                double over_provisioning_ratio,
                                const StreamChannelIDs& stream_channel_ids,
                                const StreamChipIDs&    stream_chip_ids,
                                const StreamDieIDs&     stream_die_ids,
                                const StreamPlaneIDs&   stream_plane_ids);

    LHA_type start_lha_available_to_flow(stream_id_type stream);
    LHA_type end_lha_available_to_flow(stream_id_type stream);
    LHA_type flow_allocated_lha_count_from_host(stream_id_type stream);
    LHA_type flow_allocated_lha_count_from_device(stream_id_type stream);
    LHA_type flow_allocated_pda_count(stream_id_type stream);

    double share_of_physical_pages(flash_channel_ID_type channel_id,
                                   flash_chip_ID_type chip_id,
                                   flash_die_ID_type die_id,
                                   flash_plane_ID_type plane_id) const;

    LHA_type get_total_device_lha_count() const;
  };

  force_inline LHA_type
  LogicalAddressPartitionUnit::start_lha_available_to_flow(stream_id_type stream)
  {
    // There are no difference between SATA & NVME code
    return __start_lhas_per_flow[stream];
  }

  force_inline LHA_type
  LogicalAddressPartitionUnit::end_lha_available_to_flow(stream_id_type stream)
  {
    // There are no difference between SATA & NVME code
    return __end_lhas_per_flow[stream];
  }

  force_inline LHA_type
  LogicalAddressPartitionUnit::flow_allocated_lha_count_from_host(stream_id_type stream)
  {
    // There are no difference between SATA & NVME code
    return (__end_lhas_per_flow[stream] - __start_lhas_per_flow[stream] + 1);
  }

  force_inline LHA_type
  LogicalAddressPartitionUnit::flow_allocated_lha_count_from_device(stream_id_type stream)
  {
    return (__hostinterface_type == HostInterface_Types::SATA)
             ? __total_lha_no
             : __end_lhas_per_flow[stream] - __start_lhas_per_flow[stream] + 1;
  }

  force_inline LHA_type
  LogicalAddressPartitionUnit::flow_allocated_pda_count(stream_id_type stream)
  {
    return (__hostinterface_type == HostInterface_Types::SATA)
           ? __total_pda_no
           : __pdas_per_flow[stream];
  }

  force_inline double
  LogicalAddressPartitionUnit::share_of_physical_pages(flash_channel_ID_type channel,
                                                       flash_chip_ID_type chip,
                                                       flash_die_ID_type die,
                                                       flash_plane_ID_type plane) const
  {
    if (__hostinterface_type == HostInterface_Types::NVME)
      return 1.0 / double(__resource_list[channel][chip][die][plane]);

    return 1.0;
  }

  force_inline LHA_type
  LogicalAddressPartitionUnit::get_total_device_lha_count() const
  {
    return __total_lha_no;
  }

  // ===========================================================================
  // Wrapping class before removing from static variable structure.
  // ===========================================================================
  class Logical_Address_Partitioning_Unit
  {
  public:
    static void Reset();
    static void Allocate_logical_address_for_flows(HostInterface_Types hostinterface_type,
                                                   uint32_t concurrent_stream_no,
                                                   uint32_t channel_count,
                                                   uint32_t chip_no_per_channel,
                                                   uint32_t die_no_per_chip,
                                                   uint32_t plane_no_per_die,
                                                   const std::vector<std::vector<flash_channel_ID_type>>& stream_channel_ids,
                                                   const std::vector<std::vector<flash_chip_ID_type>>& stream_chip_ids,
                                                   const std::vector<std::vector<flash_die_ID_type>>& stream_die_ids,
                                                   const std::vector<std::vector<flash_plane_ID_type>>& stream_plane_ids,
                                                   uint32_t block_no_per_plane,
                                                   uint32_t page_no_per_block,
                                                   uint32_t sector_no_per_page,
                                                   double overprovisioning_ratio);
    static LHA_type Start_lha_available_to_flow(stream_id_type stream_id);
    static LHA_type End_lha_available_to_flow(stream_id_type stream_id);
    static LHA_type LHA_count_allocate_to_flow_from_host_view(stream_id_type stream_id);
    static LHA_type LHA_count_allocate_to_flow_from_device_view(stream_id_type stream_id);
    static PDA_type PDA_count_allocate_to_flow(stream_id_type stream_id);
    static double Get_share_of_physcial_pages_in_plane(flash_channel_ID_type channel_id, flash_chip_ID_type chip_id, flash_die_ID_type die_id, flash_plane_ID_type plane_id);
    static LHA_type Get_total_device_lha_count();
  private:
    static LogicalAddressPartitionUnit* __real_unit;
  };

  force_inline void
  Logical_Address_Partitioning_Unit::Reset()
  {
    if (__real_unit) {
      delete __real_unit;
      __real_unit = nullptr;
    }
  }

  force_inline void
  Logical_Address_Partitioning_Unit::Allocate_logical_address_for_flows(HostInterface_Types hostinterface_type,
                                                                        uint32_t concurrent_stream_no,
                                                                        uint32_t channel_count,
                                                                        uint32_t chip_no_per_channel,
                                                                        uint32_t die_no_per_chip,
                                                                        uint32_t plane_no_per_die,
                                                                        const std::vector<std::vector<flash_channel_ID_type>>& stream_channel_ids,
                                                                        const std::vector<std::vector<flash_chip_ID_type>>& stream_chip_ids,
                                                                        const std::vector<std::vector<flash_die_ID_type>>& stream_die_ids,
                                                                        const std::vector<std::vector<flash_plane_ID_type>>& stream_plane_ids,
                                                                        uint32_t block_no_per_plane,
                                                                        uint32_t page_no_per_block,
                                                                        uint32_t sector_no_per_page,
                                                                        double overprovisioning_ratio)
  {
    if (!__real_unit)
      __real_unit = new LogicalAddressPartitionUnit(hostinterface_type,
                                                    concurrent_stream_no,
                                                    channel_count,
                                                    chip_no_per_channel,
                                                    die_no_per_chip,
                                                    plane_no_per_die,
                                                    block_no_per_plane,
                                                    page_no_per_block,
                                                    sector_no_per_page,
                                                    overprovisioning_ratio,
                                                    stream_channel_ids,
                                                    stream_chip_ids,
                                                    stream_die_ids,
                                                    stream_plane_ids);
  }

  force_inline double
  Logical_Address_Partitioning_Unit::Get_share_of_physcial_pages_in_plane(flash_channel_ID_type channel_id,
                                                                          flash_chip_ID_type chip_id,
                                                                          flash_die_ID_type die_id,
                                                                          flash_plane_ID_type plane_id)
  {
    return __real_unit->share_of_physical_pages(channel_id, chip_id, die_id, plane_id);
  }

  force_inline LHA_type
  Logical_Address_Partitioning_Unit::Start_lha_available_to_flow(stream_id_type stream_id)
  {
    return __real_unit->start_lha_available_to_flow(stream_id);
  }

  force_inline LHA_type
  Logical_Address_Partitioning_Unit::End_lha_available_to_flow(stream_id_type stream_id)
  {
    return __real_unit->end_lha_available_to_flow(stream_id);
  }

  force_inline LHA_type
  Logical_Address_Partitioning_Unit::LHA_count_allocate_to_flow_from_host_view(stream_id_type stream_id)
  {
    return __real_unit->flow_allocated_lha_count_from_host(stream_id);
  }

  force_inline LHA_type
  Logical_Address_Partitioning_Unit::LHA_count_allocate_to_flow_from_device_view(stream_id_type stream_id)
  {
    return __real_unit->flow_allocated_lha_count_from_device(stream_id);
  }

  force_inline PDA_type
  Logical_Address_Partitioning_Unit::PDA_count_allocate_to_flow(stream_id_type stream_id)
  {
    return __real_unit->flow_allocated_pda_count(stream_id);
  }

  force_inline LHA_type
  Logical_Address_Partitioning_Unit::Get_total_device_lha_count()
  {
    return __real_unit->get_total_device_lha_count();
  }

}

#endif // !LOGICAL_ADDRESS_PARTITIONING_UNIT_H

