#ifndef SSD_DEVICE_H
#define SSD_DEVICE_H

#include <vector>

#include "../ssd/dcm/Data_Cache_Manager_Base.h"
#include "../ssd/interface/Host_Interface_Base.h"
#include "../ssd/phy/NVM_PHY_ONFI.h"
#include "../ssd/FTL.h"
#include "../ssd/ONFI_Channel_Base.h"
#include "../utils/Logical_Address_Partitioning_Unit.h"
#include "../utils/Workload_Statistics.h"

#include "params/DeviceParameterSet.h"
#include "params/IOFlowParamSet.h"

#include "../sim/Sim_Object.h"

/*******************************************************************************
 * An SSD device has the following components:
 *
 * +------------------------------------------------------------------------+
 * | Host Interface <-> DCM <-> Firmware(FTL) <-> PHY <-> Channel <-> Chips |
 * +------------------------------------------------------------------------+
 *
 * Notes
 *  - DCM: Data Cache Manager
 *  - Basically, SSD should use FTL throw the Firmware class, but this version
 *    use FTL directly, because of the lack of interface on Firmware.
 *  - Same reason, SSD use NVM_PHY_ONFI than NVM_PHY_Base.
 *
*******************************************************************************/

namespace Host_Components {
  class PCIe_Switch;
}

class SSD_Device : public MQSimEngine::Sim_Object
{
  SSD_Components::OnfiChannelList     __channels;
  SSD_Components::OnfiPhyPtr          __phy;
  SSD_Components::FTL                 __ftl;
  SSD_Components::DataCacheManagerPtr __cache_manager;
  SSD_Components::HostInterfacePtr    __host_interface;

  SSD_Components::Stats __stats;

  const bool __preconditioning_required;

public:
  const Utils::LhaToLpaConverter<SSD_Device>     lha_to_lpa_converter;
  const Utils::NvmAccessBitmapFinder<SSD_Device> nvm_access_bitmap_finder;

private:
  LPA_type __convert_lha_to_lpa(LHA_type lha) const;
  page_status_type __find_nvm_subunit_access_bitmap(LHA_type lha) const;

public:
  SSD_Device(const DeviceParameterSet& parameters,
             const IOFlowScenario& io_flows,
             const Utils::LogicalAddrPartition& lapu,
             const StreamIdInfo& stream_info);
  ~SSD_Device() final = default;

  void Report_results_in_XML(std::string name_prefix,
                             Utils::XmlWriter& xmlwriter) final;

  uint32_t Get_no_of_LHAs_in_an_NVM_write_unit();

  void Attach_to_host(Host_Components::PCIe_Switch* pcie_switch);
  void Perform_preconditioning(Utils::WorkloadStatsList& workload_stats);

  SSD_Components::Host_Interface_Base& host_interface();
  HostInterface_Types host_interface_type() const;
};

force_inline uint32_t
SSD_Device::Get_no_of_LHAs_in_an_NVM_write_unit()
{
  return __host_interface->Get_no_of_LHAs_in_an_NVM_write_unit();
}

force_inline SSD_Components::Host_Interface_Base&
SSD_Device::host_interface()
{
  return *__host_interface;
}

force_inline HostInterface_Types
SSD_Device::host_interface_type() const
{
  return __host_interface->GetType();
}

#endif //!SSD_DEVICE_H