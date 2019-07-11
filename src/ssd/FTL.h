#ifndef FTL_H
#define FTL_H

#include "../utils/RandomGenerator.h"
#include "mapping/Address_Mapping_Unit_Base.h"
#include "gc_and_wl/GC_and_WL_Unit_Base.h"
#include "phy/NVM_PHY_ONFI.h"
#include "Stats.h"

// Refined header list
#include "../exec/params/DeviceParameterSet.h"
#include "tsu/TSU_Base.h"
#include "fbm/Flash_Block_Manager_Base.h"
#include "NvmTransactionFlash.h"

#include "NVM_Firmware.h"

namespace SSD_Components
{
  enum class SimulationMode { STANDALONE, FULL_SYSTEM };

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

  private:
    TSUPtr                __tsu;
    FlashBlockManagerPtr  __block_manager;
    AddressMappingUnitPtr __address_mapper;
    GCnWLUnitPtr          __gc_and_wl;

  public:
    FTL(const sim_object_id_type& id,
        const DeviceParameterSet& params,
        const Stats& stats);
    ~FTL() final = default;

    void dispatch_transactions(const std::list<NvmTransaction*>& transactionList) final;

    void Perform_precondition(Utils::WorkloadStatsList& workload_stats);
    void Validate_simulation_config();
    LPA_type Convert_host_logical_address_to_device_address(LHA_type lha) const;
    page_status_type Find_NVM_subunit_access_bitmap(LHA_type lha) const;

    void Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter);


    // TSU related passing functions
    void assign_tsu(TSUPtr& tsu);
    void preparing_for_transaction_submit();
    void submit_transaction(NvmTransactionFlash* transaction);
    void schedule_transaction();

    // FBM related passing functions
    void assign_fbm(FlashBlockManagerPtr& fbm);

    // AMU related passing functions
    void assign_amu(AddressMappingUnitPtr& amu);

    // GC and WL Unit passing function
    void assign_gcwl(GCnWLUnitPtr& gcwl);
    uint32_t minimum_free_pages_before_gc() const;
    bool stop_write_services_for_gc(const NVM::FlashMemory::Physical_Page_Address& plane);
    bool is_in_urgent_gc(const NVM::FlashMemory::Flash_Chip* chip) const;
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

  force_inline void
  FTL::assign_fbm(FlashBlockManagerPtr& fbm)
  { __block_manager = fbm; }

  force_inline void
  FTL::assign_amu(AddressMappingUnitPtr& amu)
  { __address_mapper = amu; }

  force_inline void
  FTL::assign_gcwl(SSD_Components::GCnWLUnitPtr& gcwl)
  { __gc_and_wl = gcwl; }

  force_inline uint32_t
  FTL::minimum_free_pages_before_gc() const
  { return __gc_and_wl->Get_minimum_number_of_free_pages_before_GC(); }

  force_inline bool
  FTL::stop_write_services_for_gc(const NVM::FlashMemory::Physical_Page_Address& plane)
  { return __gc_and_wl->Stop_servicing_writes(plane); }

  force_inline bool
  FTL::is_in_urgent_gc(const NVM::FlashMemory::Flash_Chip* chip) const
  { return __gc_and_wl->GC_is_in_urgent_mode(chip); }
}


#endif // !FTL_H
