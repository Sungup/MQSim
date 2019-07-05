#ifndef FTL_H
#define FTL_H

#include "../utils/RandomGenerator.h"
#include "mapping/Address_Mapping_Unit_Base.h"
#include "fbm/Flash_Block_Manager_Base.h"
#include "gc_and_wl/GC_and_WL_Unit_Base.h"
#include "phy/NVM_PHY_ONFI.h"
#include "Stats.h"

// Refined header list
#include "../exec/params/DeviceParameterSet.h"
#include "tsu/TSU_Base.h"
#include "NvmTransactionFlash.h"

#include "NVM_Firmware.h"

namespace SSD_Components
{
  enum class SimulationMode { STANDALONE, FULL_SYSTEM };

  class Flash_Block_Manager_Base;
  class Address_Mapping_Unit_Base;
  class GC_and_WL_Unit_Base;
  class TSU_Base;

  class FTL : public NVM_Firmware {
  private:
    uint32_t block_no_per_plane;
    uint32_t page_no_per_block;
    uint32_t page_size_in_sectors;

    int preconditioning_seed;
    Utils::RandomGenerator random_generator;

    double over_provisioning_ratio;
    sim_time_type avg_flash_read_latency;
    sim_time_type avg_flash_program_latency;

    const Stats& __stats;

  public:
    Address_Mapping_Unit_Base* Address_Mapping_Unit;
    Flash_Block_Manager_Base* BlockManager;
    GC_and_WL_Unit_Base* GC_and_WL_Unit;

  private:
    TSUPtr __tsu;

  public:
    FTL(const sim_object_id_type& id,
        const DeviceParameterSet& params,
        const Stats& stats);
    ~FTL() final = default;

    void Perform_precondition(std::vector<Utils::Workload_Statistics*> workload_stats);
    void Validate_simulation_config();
    LPA_type Convert_host_logical_address_to_device_address(LHA_type lha) const;
    page_status_type Find_NVM_subunit_access_bitmap(LHA_type lha) const;

    void Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter);

    // TSU related passing functions
    void assign_tsu(TSUPtr& tsu);
    void preparing_for_transaction_submit();
    void submit_transaction(NvmTransactionFlash* transaction);
    void schedule_transaction();
  };

  force_inline void
  FTL::assign_tsu(SSD_Components::TSUPtr& tsu)
  { __tsu = tsu; }

  force_inline void
  FTL::preparing_for_transaction_submit()
  { __tsu->Prepare_for_transaction_submit(); }

  force_inline void
  FTL::submit_transaction(NvmTransactionFlash* transaction)
  { __tsu->Submit_transaction(transaction); }

  force_inline void
  FTL::schedule_transaction()
  { __tsu->Schedule(); }

}


#endif // !FTL_H
