#include "NVM_PHY_ONFI.h"

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
    plane_no_per_die(PlaneNoPerDie)
{ }
