#ifndef FTL_H
#define FTL_H

#include "../sim/Sim_Reporter.h"
#include "../utils/RandomGenerator.h"
#include "NVM_Firmware.h"
#include "tsu/TSU_Base.h"
#include "mapping/Address_Mapping_Unit_Base.h"
#include "fbm/Flash_Block_Manager_Base.h"
#include "gc_and_wl/GC_and_WL_Unit_Base.h"
#include "phy/NVM_PHY_ONFI.h"
#include "Stats.h"

namespace SSD_Components
{
  enum class SimulationMode { STANDALONE, FULL_SYSTEM };

  class Flash_Block_Manager_Base;
  class Address_Mapping_Unit_Base;
  class GC_and_WL_Unit_Base;
  class TSU_Base;
  class FTL : public NVM_Firmware
  {
  public:
    FTL(const sim_object_id_type& id, Data_Cache_Manager_Base* data_cache, 
      uint32_t channel_no, uint32_t chip_no_per_channel, uint32_t die_no_per_chip, uint32_t plane_no_per_die,
      uint32_t block_no_per_plane, uint32_t page_no_per_block, uint32_t page_size_in_sectors,
      sim_time_type avg_flash_read_latency, sim_time_type avg_flash_program_latency, double over_provisioning_ratio, uint32_t max_allowed_block_erase_count, int seed);
    ~FTL();

    Stats& get_stats_reference();

    void Perform_precondition(std::vector<Utils::Workload_Statistics*> workload_stats);
    void Validate_simulation_config();
    void Start_simulation();
    void Execute_simulator_event(MQSimEngine::SimEvent*);
    LPA_type Convert_host_logical_address_to_device_address(LHA_type lha);
    page_status_type Find_NVM_subunit_access_bitmap(LHA_type lha);
    Address_Mapping_Unit_Base* Address_Mapping_Unit;
    Flash_Block_Manager_Base* BlockManager;
    GC_and_WL_Unit_Base* GC_and_WL_Unit;
    TSU_Base * TSU;
    NVM_PHY_ONFI* PHY;
    void Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter);

  private:
    uint32_t channel_no, chip_no_per_channel, die_no_per_chip, plane_no_per_die;
    uint32_t block_no_per_plane, page_no_per_block, page_size_in_sectors;
    uint32_t max_allowed_block_erase_count;
    int preconditioning_seed;
    Utils::RandomGenerator random_generator;
    double over_provisioning_ratio;
    sim_time_type avg_flash_read_latency;
    sim_time_type avg_flash_program_latency;

    Stats __stats;
  };

  force_inline Stats&
  FTL::get_stats_reference()
  {
    return __stats;
  }
}


#endif // !FTL_H
