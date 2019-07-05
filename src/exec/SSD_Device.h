#ifndef SSD_DEVICE_H
#define SSD_DEVICE_H

#include "../ssd/SSD_Defs.h"
#include "../ssd/interface/Host_Interface_Base.h"
#include "../ssd/interface/Host_Interface_SATA.h"
#include "../ssd/interface/Host_Interface_NVMe.h"
#include "../ssd/dcm/Data_Cache_Manager_Base.h"
#include "../ssd/dcm/Data_Cache_Flash.h"
#include "../ssd/FTL.h"
#include "../host/PCIe_Switch.h"
#include "../nvm_chip/NVM_Types.h"
#include "params/DeviceParameterSet.h"
#include "params/IOFlowParamSet.h"
#include "../utils/Workload_Statistics.h"

// Refine header list
#include <vector>

#include "../nvm_chip/flash_memory/Flash_Chip.h"
#include "../ssd/phy/NVM_PHY_ONFI.h"
#include "../ssd/FTL.h"
#include "../ssd/ONFI_Channel_Base.h"

#include "../sim/Sim_Object.h"


/*********************************************************************************************************
* An SSD device has the following components:
* 
* Host_Interface <---> Data_Cache_Manager <----> NVM_Firmware <---> NVM_PHY <---> NVM_Channel <---> Chips
*
*********************************************************************************************************/

// TODO Remove static features

class SSD_Device : public MQSimEngine::Sim_Object
{
  SSD_Components::Stats __stats;
  SSD_Components::OnfiChannelList __channels;
  SSD_Components::FTL __ftl;
  SSD_Components::OnfiPhyPtr __phy;

public:
  const bool Preconditioning_required;
  const NVM::NVM_Type Memory_Type;

  const uint32_t Channel_count;
  const uint32_t Chip_no_per_channel;

  const Utils::LhaToLpaConverter<SSD_Device>     lha_to_lpa_converter;
  const Utils::NvmAccessBitmapFinder<SSD_Device> nvm_access_bitmap_finder;

  SSD_Components::Host_Interface_Base *Host_interface;
  SSD_Components::Data_Cache_Manager_Base *Cache_manager;

public:
  SSD_Device(DeviceParameterSet& parameters, IOFlowScenario& io_flows);
  ~SSD_Device() final;

  void Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter) final;
  uint32_t Get_no_of_LHAs_in_an_NVM_write_unit();

  void Attach_to_host(Host_Components::PCIe_Switch* pcie_switch);
  void Perform_preconditioning(const std::vector<Utils::Workload_Statistics*>& workload_stats);

private:
  LPA_type __convert_lha_to_lpa(LHA_type lha) const;
  page_status_type __find_nvm_subunit_access_bitmap(LHA_type lha) const;

};

#endif //!SSD_DEVICE_H