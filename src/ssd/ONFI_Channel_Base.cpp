#include "ONFI_Channel_Base.h"

// Children of ONFI_Channel_Base
#include "ONFI_Channel_NVDDR2.h"

using namespace SSD_Components;
ONFI_Channel_Base::ONFI_Channel_Base(const DeviceParameterSet& params,
                                     const sim_object_id_type& id,
                                     flash_channel_ID_type channelID,
                                     ONFI_Protocol type)
  : ChannelID(channelID),
    Type(type),
    Chips(),
    __status(BusChannelStatus::IDLE),
    __current_active(nullptr)
{
  auto* sim = Simulator;

  Chips.reserve(params.Chip_No_Per_Channel);

  for (flash_chip_ID_type c = 0; c < params.Chip_No_Per_Channel; ++c) {
    Chips.emplace_back(id, params.Flash_Parameters, channelID, c);

    sim->AddObject(&Chips.back());
  }
}

OnfiChannelList
SSD_Components::build_onfi_channels(const DeviceParameterSet& params,
                                    const sim_object_id_type& id)
{
  OnfiChannelList channels;

  channels.reserve(params.Flash_Channel_Count);

  for (flash_channel_ID_type c = 0; c < params.Flash_Channel_Count; ++c)
    channels.emplace_back(params, id, c, ONFI_Protocol::NVDDR2);

  return channels;
}
