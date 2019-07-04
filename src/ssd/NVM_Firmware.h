#ifndef NVM_FIRMWARE_H
#define NVM_FIRMWARE_H

#include <memory>
#include <vector>

#include "../exec/params/DeviceParameterSet.h"
#include "../sim/Sim_Object.h"
#include "../utils/InlineTools.h"
#include "../utils/Workload_Statistics.h"

namespace SSD_Components
{
  class Data_Cache_Manager_Base;

  class NVM_Firmware : public MQSimEngine::Sim_Object {
  protected:
    Data_Cache_Manager_Base* Data_cache_manager;

  public:
    explicit NVM_Firmware(const sim_object_id_type& id);
    ~NVM_Firmware() override = default;

    void Validate_simulation_config() override;

    virtual LPA_type Convert_host_logical_address_to_device_address(LHA_type lha) const = 0;

    // Returns a bitstring with only one bit in it and determines which subunit
    // (e.g., sub-page in flash memory) is accessed with the target NVM unit
    // (e.g., page in flash memory). If the NVM access unit is B_nvm bytes in
    // size and the LHA_type unit is B_lha bytes in size, then the returned
    // bit-stream has b bits where b = ceiling(B_nvm / B_lha).
    virtual page_status_type Find_NVM_subunit_access_bitmap(LHA_type lha) const = 0;

    virtual void Perform_precondition(std::vector<Utils::Workload_Statistics*> workload_stats) = 0;

    void assign(Data_Cache_Manager_Base* dcm);
  };

  typedef std::shared_ptr<NVM_Firmware> NvmFirmwarePtr;

  force_inline
  NVM_Firmware::NVM_Firmware(const sim_object_id_type& id)
    : MQSimEngine::Sim_Object(id),
      Data_cache_manager(nullptr)
  { }

  force_inline void
  NVM_Firmware::Validate_simulation_config()
  { }

  force_inline void
  NVM_Firmware::assign(Data_Cache_Manager_Base *dcm)
  {
    Data_cache_manager = dcm;
  }

  NvmFirmwarePtr build_firmware(const sim_object_id_type& id,
                                const DeviceParameterSet& params);
}

#endif // !NVM_FIRMWARE_H
