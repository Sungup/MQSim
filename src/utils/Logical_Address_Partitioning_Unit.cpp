#include "Logical_Address_Partitioning_Unit.h"

#include <string>

#include "Exception.h"
#include "InlineTools.h"

using namespace Utils;

#define __LAPU_INIT_RESOURCE_ERR_MSG(TARGET, STREAM_ID)         \
  std::string("Invalid " #TARGET " ID specified for I/O flow ") \
    + std::to_string(stream_id)

#define __CHECK_LAPU_VALID_ID(VAL, LIMIT, STREAM_ID) \
  if ((LIMIT) <= (VAL))                              \
    throw mqsim_error(__LAPU_INIT_RESOURCE_ERR_MSG(VAL, STREAM_ID))

LogicalAddressPartitionUnit::LogicalAddressPartitionUnit(HostInterface_Types host_interface_type,
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
                                                         const StreamChipIDs& stream_chip_ids,
                                                         const StreamDieIDs& stream_die_ids,
                                                         const StreamPlaneIDs& stream_plane_ids)
  : __hostinterface_type(host_interface_type),
    __total_pda_no(0),
    __total_lha_no(0),
    __resource_list(channels,
      ResourceListOnChip(
        chips_on_channel,
        ResourceListOnDie(dies_on_chip,
                          ResourceListOnPlane(planes_on_die, 0)
        )
      )
    )
{
  const double reverse_over_provisioning = 1.0 - over_provisioning_ratio;

  for (uint32_t stream_id = 0; stream_id < concurrent_streams; ++stream_id) {
    // 1. Resource List initializing
    // TODO Is this meaningful????  Maybe all resource_list entries will be set
    //  to the number of concurrent_streams at the end.
    for (auto channel : stream_channel_ids[stream_id]) {
      __CHECK_LAPU_VALID_ID(channel, channels, stream_id);

      for (auto chip : stream_chip_ids[stream_id]) {
        __CHECK_LAPU_VALID_ID(chip, chips_on_channel, stream_id);

        for (auto die : stream_die_ids[stream_id]) {
          __CHECK_LAPU_VALID_ID(die, dies_on_chip, stream_id);

          for (auto plane : stream_plane_ids[stream_id]) {
            __CHECK_LAPU_VALID_ID(plane, planes_on_die, stream_id);

            ++__resource_list[channel][chip][die][plane];
          }
        }
      }
    }
  }

  for (uint32_t stream_id = 0; stream_id < concurrent_streams; ++stream_id) {
    // 2. Initialize pdas_per_flow and pda_no
    // TODO Check lsa_count's type can be double not LHA_type
    LHA_type lsa_count = 0;
    for (auto channel : stream_channel_ids[stream_id])
      for (auto chip : stream_chip_ids[stream_id])
        for (auto die : stream_die_ids[stream_id])
          for (auto plane : stream_plane_ids[stream_id])
            lsa_count += LHA_type(
              double(blocks_on_plane * pages_on_block * sectors_on_page)
                * reverse_over_provisioning
                / __resource_list[channel][chip][die][plane]
             );

    __pdas_per_flow.emplace_back(LHA_type(double(lsa_count)
                                            / reverse_over_provisioning));

    __total_pda_no += __pdas_per_flow[stream_id];

    // 3. Initialize start_lhas_per_flow
    __start_lhas_per_flow.emplace_back(__total_lha_no);
    __total_lha_no += lsa_count;
    __end_lhas_per_flow.emplace_back(__total_lha_no - 1);
  }
}

// =============================================================================
// Wrapping class before removing from static variable structure.
// =============================================================================
LogicalAddressPartitionUnit* Logical_Address_Partitioning_Unit::__real_unit = nullptr;

