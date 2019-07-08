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

LogicalAddressPartitionUnit::LogicalAddressPartitionUnit(const DeviceParameterSet& params,
                                                         const StreamIdInfo& stream_info,
                                                         uint32_t stream_count)
  : __hostinterface_type(params.HostInterface_Type),
    __total_pda_no(0),
    __total_lha_no(0),
    __resource_list(
      params.Flash_Channel_Count,
      ResourceListOnChip(
        params.Chip_No_Per_Channel,
        ResourceListOnDie(params.Flash_Parameters.Die_No_Per_Chip,
                          ResourceListOnPlane(params.Flash_Parameters.Plane_No_Per_Die, 0)
        )
      )
    )
{
  const double reverse_over_provisioning = 1.0 - params.Overprovisioning_Ratio;

  for (uint32_t stream_id = 0; stream_id < stream_count; ++stream_id) {
    // 1. Resource List initializing
    // TODO Is this meaningful????  Maybe all resource_list entries will be set
    //  to the number of concurrent_streams at the end.
    for (auto channel : stream_info.stream_channel_ids()[stream_id]) {
      __CHECK_LAPU_VALID_ID(channel, params.Flash_Channel_Count, stream_id);

      for (auto chip : stream_info.stream_chip_ids()[stream_id]) {
        __CHECK_LAPU_VALID_ID(chip, params.Chip_No_Per_Channel, stream_id);

        for (auto die : stream_info.stream_die_ids()[stream_id]) {
          __CHECK_LAPU_VALID_ID(die, params.Flash_Parameters.Die_No_Per_Chip, stream_id);

          for (auto plane : stream_info.stream_plane_ids()[stream_id]) {
            __CHECK_LAPU_VALID_ID(plane, params.Flash_Parameters.Plane_No_Per_Die, stream_id);

            ++__resource_list[channel][chip][die][plane];
          }
        }
      }
    }
  }

  for (uint32_t stream_id = 0; stream_id < stream_count; ++stream_id) {
    // 2. Initialize pdas_per_flow and pda_no
    // TODO Check lsa_count's type can be double not LHA_type
    LHA_type lsa_count = 0;
    for (auto channel : stream_info.stream_channel_ids()[stream_id])
      for (auto chip : stream_info.stream_chip_ids()[stream_id])
        for (auto die : stream_info.stream_die_ids()[stream_id])
          for (auto plane : stream_info.stream_plane_ids()[stream_id])
            lsa_count += LHA_type(
              double(params.Flash_Parameters.plane_size_in_sector())
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
