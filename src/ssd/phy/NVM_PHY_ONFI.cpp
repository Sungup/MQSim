#include "NVM_PHY_ONFI.h"

// Children classes
#include "NVM_PHY_ONFI_NVDDR2.h"

using namespace SSD_Components;

NVM_PHY_ONFI::NVM_PHY_ONFI(const sim_object_id_type& id,
                           Stats& stats,
                           uint32_t ChannelCount,
                           uint32_t chip_no_per_channel,
                           uint32_t DieNoPerChip,
                           uint32_t PlaneNoPerDie)
  : NVM_PHY_Base(id),
    __stats(stats),
    channel_count(ChannelCount),
    chip_no_per_channel(chip_no_per_channel),
    die_no_per_chip(DieNoPerChip),
    plane_no_per_die(PlaneNoPerDie),
    __cmd_pool()
{ }

OnfiPhyPtr
SSD_Components::build_onfi_phy(const sim_object_id_type& id,
                               const DeviceParameterSet& params,
                               OnfiChannelList& channels,
                               Stats& stats)
{
  return std::make_shared<NVM_PHY_ONFI_NVDDR2>(id,
                                               params,
                                               channels,
                                               stats,
                                               params.Flash_Channel_Count,
                                               params.Chip_No_Per_Channel,
                                               params.Flash_Parameters.Die_No_Per_Chip,
                                               params.Flash_Parameters.Plane_No_Per_Die);
}