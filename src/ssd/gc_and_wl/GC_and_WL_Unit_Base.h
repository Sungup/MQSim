#ifndef GC_AND_WL_UNIT_BASE_H
#define GC_AND_WL_UNIT_BASE_H

#include "../../sim/Sim_Object.h"
#include "../../nvm_chip/flash_memory/Flash_Chip.h"
#include "../../nvm_chip/flash_memory/Physical_Page_Address.h"
#include "../mapping/Address_Mapping_Unit_Base.h"
#include "../Flash_Block_Manager_Base.h"
#include "../tsu/TSU_Base.h"
#include "../NVM_PHY_ONFI.h"

#include "GCandWLUnitDefs.h"

namespace SSD_Components
{
  class Address_Mapping_Unit_Base;
  class Flash_Block_Manager_Base;
  class TSU_Base;
  class NVM_PHY_ONFI;
  class PlaneBookKeepingType;
  class Block_Pool_Slot_Type;

  /*
   * This class implements that the Garbage Collection and Wear Leveling module'
   * of MQSim.
   */
  class GC_and_WL_Unit_Base : public MQSimEngine::Sim_Object {
  protected:
    GC_Block_Selection_Policy_Type block_selection_policy;
    static GC_and_WL_Unit_Base * _my_instance;
    Address_Mapping_Unit_Base* address_mapping_unit;
    Flash_Block_Manager_Base* block_manager;
    TSU_Base* tsu;
    NVM_PHY_ONFI* flash_controller;
    bool force_gc;
    double gc_threshold;//As the ratio of free pages to the total number of physical pages
    uint32_t block_pool_gc_threshold;

    bool use_copyback;
    bool dynamic_wearleveling_enabled;
    bool static_wearleveling_enabled;
    uint32_t static_wearleveling_threshold;

    //Used to implement: "Preemptible I/O Scheduling of Garbage Collection for Solid State Drives", TCAD 2013.
    bool preemptible_gc_enabled;
    double gc_hard_threshold;
    uint32_t block_pool_gc_hard_threshold;
    uint32_t max_ongoing_gc_reqs_per_plane;//This value has two important usages: 1) maximum number of concurrent gc operations per plane, and 2) the value that determines urgent GC execution when there is a shortage of flash blocks. If the block bool size drops below this value, all incomming user writes should be blocked

    //Following variabels are used based on the type of GC block selection policy
    uint32_t rga_set_size;//The number of random flash blocks that are radnomly selected
    Utils::RandomGenerator random_generator;
    std::queue<Block_Pool_Slot_Type*> block_usage_fifo;
    uint32_t random_pp_threshold;

    uint32_t channel_count;
    uint32_t chip_no_per_channel;
    uint32_t die_no_per_chip;
    uint32_t plane_no_per_die;
    uint32_t block_no_per_plane;
    uint32_t pages_no_per_block;
    uint32_t sector_no_per_page;

    FlashTransactionHandler<GC_and_WL_Unit_Base> __transaction_service_handler;

    // Checks if block_address is a safe candidate for gc execution, i.e., 1) it
    // is not a write frontier, and 2) there is no ongoing program operation
    bool is_safe_gc_wl_candidate(const PlaneBookKeepingType* pbke,
                                 const flash_block_ID_type gc_wl_candidate_block_id);
    bool check_static_wl_required(const NVM::FlashMemory::Physical_Page_Address& plane_address);
    void run_static_wearleveling(const NVM::FlashMemory::Physical_Page_Address& plane_address);

    void __handle_transaction_service(NVM_Transaction_Flash* transaction);

    static void handle_transaction_serviced_signal_from_PHY(NVM_Transaction_Flash* transaction);

  public:
    GC_and_WL_Unit_Base(const sim_object_id_type& id,
                        Address_Mapping_Unit_Base* address_mapping_unit,
                        Flash_Block_Manager_Base* block_manager,
                        TSU_Base* tsu,
                        NVM_PHY_ONFI* flash_controller,
                        GC_Block_Selection_Policy_Type block_selection_policy,
                        double gc_threshold,
                        bool preemptible_gc_enabled,
                        double gc_hard_threshold,
                        uint32_t channel_count,
                        uint32_t chip_no_per_channel,
                        uint32_t die_no_per_chip,
                        uint32_t plane_no_per_die,
                        uint32_t block_no_per_plane,
                        uint32_t page_no_per_block,
                        uint32_t sector_no_per_page,
                        bool use_copyback,
                        double rho,
                        uint32_t max_ongoing_gc_reqs_per_plane,
                        bool dynamic_wearleveling_enabled,
                        bool static_wearleveling_enabled,
                        uint32_t static_wearleveling_threshold,
                        int seed);

    virtual ~GC_and_WL_Unit_Base() = default;

    void Setup_triggers();

    virtual bool GC_is_in_urgent_mode(const NVM::FlashMemory::Flash_Chip*) = 0;
    virtual void Check_gc_required(const uint32_t BlockPoolSize, const NVM::FlashMemory::Physical_Page_Address& planeAddress) = 0;

    GC_Block_Selection_Policy_Type Get_gc_policy() const;

    // Returns the parameter specific to the GC block selection policy:
    // threshold for random_pp, set_size for RGA
    uint32_t Get_GC_policy_specific_parameter() const;
    uint32_t Get_minimum_number_of_free_pages_before_GC() const;
    bool Use_dynamic_wearleveling() const;
    bool Use_static_wearleveling() const;
    bool Stop_servicing_writes(const NVM::FlashMemory::Physical_Page_Address& plane_address) const;
  };

  force_inline GC_Block_Selection_Policy_Type
  GC_and_WL_Unit_Base::Get_gc_policy() const
  {
    return block_selection_policy;
  }

  force_inline uint32_t
  GC_and_WL_Unit_Base::Get_GC_policy_specific_parameter() const
  {
    switch (block_selection_policy) {
      case GC_Block_Selection_Policy_Type::RGA:       return rga_set_size;
      case GC_Block_Selection_Policy_Type::RANDOM_PP: return random_pp_threshold;
      default:                                        return 0;
    }
  }

  force_inline uint32_t
  GC_and_WL_Unit_Base::Get_minimum_number_of_free_pages_before_GC() const
  {
    return block_pool_gc_threshold;
#if 0
    if (preemptible_gc_enabled)
      return block_pool_gc_hard_threshold;
    else
      return block_pool_gc_threshold;
#endif
  }

  force_inline bool
  GC_and_WL_Unit_Base::Use_dynamic_wearleveling() const
  {
    return dynamic_wearleveling_enabled;
  }

  force_inline bool
  GC_and_WL_Unit_Base::Use_static_wearleveling() const
  {
    return static_wearleveling_enabled;
  }
}

#endif // !GC_AND_WL_UNIT_BASE_H
