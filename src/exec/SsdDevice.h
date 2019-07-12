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
  class PCIeSwitch;
}

class SsdDevice : public MQSimEngine::Sim_Object
{
private:
  SSD_Components::OnfiChannelList     __channels;
  SSD_Components::OnfiPhyPtr          __phy;
  SSD_Components::FTL                 __ftl;
  SSD_Components::DataCacheManagerPtr __cache_manager;
  SSD_Components::HostInterfacePtr    __host_interface;

  SSD_Components::Stats __stats;

  const bool __preconditioning_required;

public:
  const Utils::LhaToLpaConverter<SsdDevice>     lha_to_lpa_converter;
  const Utils::NvmAccessBitmapFinder<SsdDevice> nvm_access_bitmap_finder;

private:
  LPA_type __convert_lha_to_lpa(LHA_type lha) const;
  page_status_type __find_nvm_subunit_access_bitmap(LHA_type lha) const;

public:
  SsdDevice(const DeviceParameterSet& parameters,
             const IOFlowScenario& io_flows,
             const Utils::LogicalAddrPartition& lapu,
             const StreamIdInfo& stream_info);
  ~SsdDevice() final = default;

  void Report_results_in_XML(std::string name_prefix,
                             Utils::XmlWriter& xmlwriter) final;

  uint32_t total_lha_in_nvm_wr_unit() const;

  // Host<->Device queue information query and managements
  uint16_t nvme_sq_size() const;
  uint16_t nvme_cq_size() const;
  uint16_t sata_ncq_depth() const;

  void create_new_stream(IO_Flow_Priority_Class priority,
                         LHA_type start_logical_sector,
                         LHA_type end_logical_sector,
                         uint64_t sq_base_address,
                         uint64_t cq_base_address);

  void set_ncq_address(uint64_t sq_base_address, uint64_t cq_base_address);

  // Host<->Device interconnection
  void connect_to_host(Host_Components::PCIeSwitch* pcie_switch);

  SSD_Components::Host_Interface_Base& host_interface();
  HostInterface_Types host_interface_type() const;

  // Device preconditioning
  bool preconditioning_required() const;
  void perform_preconditioning(Utils::WorkloadStatsList& workload_stats);

};

force_inline uint32_t
SsdDevice::total_lha_in_nvm_wr_unit() const
{
  return __host_interface->total_lha_in_nvm_wr_unit();
}

force_inline SSD_Components::Host_Interface_Base&
SsdDevice::host_interface()
{
  return *__host_interface;
}

force_inline HostInterface_Types
SsdDevice::host_interface_type() const
{
  return __host_interface->GetType();
}

force_inline bool
SsdDevice::preconditioning_required() const
{
  return __preconditioning_required;
}

#endif //!SSD_DEVICE_H