#include <algorithm>
#include <vector>
#include <stdexcept>
#include <cmath>
#include <ctime>
#include <numeric>
#include "SsdDevice.h"

#include "../ssd/fbm/Flash_Block_Manager.h"
#include "../ssd/gc_and_wl/GC_and_WL_Unit_Base.h"
#include "../ssd/mapping/Address_Mapping_Unit_Base.h"
#include "../ssd/tsu/TSU_Base.h"

// To control NVMe and SATA interface
#include "../ssd/interface/Host_Interface_NVMe.h"
#include "../ssd/interface/Host_Interface_SATA.h"

SsdDevice::SsdDevice(const DeviceParameterSet& params,
                       const IOFlowScenario& io_flows,
                       const Utils::LogicalAddrPartition& lapu,
                       const StreamIdInfo& stream_info)
  : MQSimEngine::Sim_Object("SSDDevice"),
    __channels(SSD_Components::build_onfi_channels(params, ID())),
    __phy(SSD_Components::build_onfi_phy(ID() + ".PHY",
                                         params,
                                         __channels,
                                         __stats)),
    __ftl(ID() + ".FTL", params, __stats),
    __stats(params),
    __preconditioning_required(params.Enabled_Preconditioning),
    lha_to_lpa_converter(this,
                         &SsdDevice::__convert_lha_to_lpa),
    nvm_access_bitmap_finder(this,
                             &SsdDevice::__find_nvm_subunit_access_bitmap)
{
  using namespace SSD_Components;

  // 0. Initialize temporary variables to build objects.
  double max_rho = 0;

  CachingModeList caching_modes;
  caching_modes.reserve(io_flows.size());

  for (const auto& flow : io_flows) {
    max_rho = std::max(max_rho, flow->init_occupancy_rate());
    caching_modes.emplace_back(flow->Device_Level_Data_Caching_Mode);
  }

  // 1. Register SSD
  Simulator->AddObject(this);

  // 2. Register PHY
  Simulator->AddObject(__phy.get());

  // 3. Register FTL
  Simulator->AddObject(&__ftl);

  // 4. Create and register TSU
  auto tsu = build_tsu_object(params, __ftl, *__phy);

  __ftl.assign_tsu(tsu);
  Simulator->AddObject(tsu.get());

  // 5. Create and register Flash_Block_Manager
  auto fbm = build_fbm_object(params, io_flows.size(), __stats);

  __ftl.assign_fbm(fbm);

  // 6. Create and register Address_Mapping_Unit
  auto amu = build_amu_object(params,
                              lapu,
                              stream_info,
                              __ftl,
                              *__phy,
                              *fbm,
                              __stats);

  __ftl.assign_amu(amu);
  Simulator->AddObject(amu.get());


  // 7. Create and register GC_and_WL_Unit
  auto gcwl = build_gc_and_wl_object(params,
                                     __ftl,
                                     *amu,
                                     *fbm,
                                     *tsu,
                                     *__phy,
                                     __stats,
                                     max_rho,
                                     10);

  __ftl.assign_gcwl(gcwl);
  Simulator->AddObject(gcwl.get());
  fbm->Set_GC_and_WL_Unit(gcwl.get());

  // 8. Create and register Data_Cache_Manager
  __cache_manager = build_dcm_object(ID() + ".DataCache",
                                     params,
                                     __ftl,
                                     *__phy,
                                     caching_modes);

  Simulator->AddObject(__cache_manager.get());
  __ftl.assign_dcm(__cache_manager.get());

  // 9. Create and register Host_Interface
  __host_interface = build_hil_object(ID() + ".HostInterface",
                                      params,
                                      lapu.total_device_lha_count(),
                                      io_flows.size(),
                                      *__cache_manager);

  Simulator->AddObject(__host_interface.get());
  __cache_manager->connect_host_interface(*__host_interface);
}

LPA_type
SsdDevice::__convert_lha_to_lpa(LHA_type lha) const
{
  return __ftl.convert_lha_to_lpa(lha);
}

page_status_type
SsdDevice::__find_nvm_subunit_access_bitmap(LHA_type lha) const
{
  return __ftl.Find_NVM_subunit_access_bitmap(lha);
}

uint16_t
SsdDevice::nvme_sq_size() const
{
  using namespace SSD_Components;

  return (__host_interface->GetType() == HostInterface_Types::NVME)
           ? to_nvme_interface(__host_interface)->Get_submission_queue_depth()
           : 0;
}

uint16_t
SsdDevice::nvme_cq_size() const
{
  using namespace SSD_Components;

  return (__host_interface->GetType() == HostInterface_Types::NVME)
           ? to_nvme_interface(__host_interface)->Get_completion_queue_depth()
           : 0;
}

uint16_t
SsdDevice::sata_ncq_depth() const
{
  using namespace SSD_Components;

  return (__host_interface->GetType() == HostInterface_Types::SATA)
           ? to_sata_interface(__host_interface)->Get_ncq_depth()
           : 0;
}

void
SsdDevice::create_new_stream(IO_Flow_Priority_Class priority,
                              LHA_type start_logical_sector,
                              LHA_type end_logical_sector,
                              uint64_t sq_base_address,
                              uint64_t cq_base_address)
{
  auto interface = SSD_Components::to_nvme_interface(__host_interface);

  if (!interface)
    throw mqsim_error("Unexpected HIL type casting. HIL is not NVMe");

  interface->Create_new_stream(priority,
                               start_logical_sector,
                               end_logical_sector,
                               sq_base_address,
                               cq_base_address);
}

void
SsdDevice::set_ncq_address(uint64_t sq_base_address, uint64_t cq_base_address)
{
  auto interface = SSD_Components::to_sata_interface(__host_interface);

  if (!interface)
    throw mqsim_error("Unexpected HIL type casting. HIL is not SATA");

  interface->Set_ncq_address(sq_base_address, cq_base_address);
}

void
SsdDevice::connect_to_host(Host_Components::PCIeSwitch* pcie_switch)
{
  __host_interface->connect_to_switch(pcie_switch);
}

void
SsdDevice::perform_preconditioning(Utils::WorkloadStatsList& workload_stats)
{
  if (!__preconditioning_required)
    return;

  std::cout << "SSD Device preconditioning started ........." << std::endl;

  time_t start_time = time(nullptr);

  __ftl.Perform_precondition(workload_stats);
  __cache_manager->Do_warmup(workload_stats);

  time_t end_time = time(nullptr);

  auto duration = (uint64_t)difftime(end_time, start_time);

  std::cout << "Finished preconditioning. Duration of preconditioning: "
            << duration / 3600 << ":"
            << (duration % 3600) / 60 << ":"
            << ((duration % 3600) % 60) << std::endl;
}

void
SsdDevice::Report_results_in_XML(std::string /* name_prefix */,
                                  Utils::XmlWriter& xmlwriter)
{
  xmlwriter.Write_open_tag(ID());

  __host_interface->Report_results_in_XML(ID(), xmlwriter);

  __ftl.Report_results_in_XML(ID(), xmlwriter);

  for (auto& channel : __channels)
    for (auto& chip : channel.Chips)
      chip.Report_results_in_XML(ID(), xmlwriter);

  xmlwriter.Write_close_tag();
}
