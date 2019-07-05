#include "NVM_Firmware.h"

#include "FTL.h"

using namespace SSD_Components;

NvmFirmwarePtr
SSD_Components::build_firmware(const sim_object_id_type& id,
                               const DeviceParameterSet& params,
                               const Stats& stats)
{
  return std::make_shared<FTL>(id, params, stats);
}
