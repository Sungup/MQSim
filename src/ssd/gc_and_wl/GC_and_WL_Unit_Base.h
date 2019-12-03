#ifndef GC_AND_WL_UNIT_BASE_H
#define GC_AND_WL_UNIT_BASE_H

#include "../../sim/Sim_Object.h"
#include "../../nvm_chip/flash_memory/Flash_Chip.h"
#include "../../nvm_chip/flash_memory/Physical_Page_Address.h"
#include "../mapping/Address_Mapping_Unit_Base.h"
#include "../phy/NVM_PHY_ONFI.h"
#include "../Stats.h"

// Renewed header list
#include "../fbm/BlockPoolSlotType.h"
#include "../fbm/Flash_Block_Manager_Base.h"
#include "../fbm/PlaneBookKeepingType.h"
#include "../tsu/TSU_Base.h"

#include "../NvmTransactionFlashER.h"
#include "../NvmTransactionFlashRD.h"
#include "../NvmTransactionFlashWR.h"

#include "../../utils/RandomGenerator.h"

#include "GCandWLUnitDefs.h"

namespace SSD_Components
{
  class NVM_PHY_ONFI;

  /*
   * This class implements that the Garbage Collection and Wear Leveling module'
   * of MQSim.
   */
  class GC_and_WL_Unit_Base : public MQSimEngine::Sim_Object {
    class MakeWriteTrFunctor {
      typedef NvmTransactionFlashWR* (GC_and_WL_Unit_Base::*Generator) (
        stream_id_type stream_id,
        const NVM::FlashMemory::Physical_Page_Address& address,
        NvmTransactionFlashER* erase_tr
      );

    private:
      GC_and_WL_Unit_Base* __callee;
      Generator __generator;

    public:
      force_inline
      MakeWriteTrFunctor(GC_and_WL_Unit_Base* callee, Generator gen)
        : __callee(callee),
          __generator(gen)
      { }

      force_inline
      NvmTransactionFlashWR* operator()(stream_id_type stream_id,
                                        const NVM::FlashMemory::Physical_Page_Address& address,
                                        NvmTransactionFlashER* erase_tr)
      { return (__callee->*__generator)(stream_id, address, erase_tr); }
    };

  protected:
    GC_Block_Selection_Policy_Type block_selection_policy;

    Address_Mapping_Unit_Base* address_mapping_unit;
    Flash_Block_Manager_Base* block_manager;
    TSU_Base* tsu;
    NVM_PHY_ONFI* flash_controller;

    Stats& _stats;

    bool force_gc;
    double gc_threshold;//As the ratio of free pages to the total number of physical pages
    uint32_t block_pool_gc_threshold;

    MakeWriteTrFunctor _make_write_tr;

    bool dynamic_wearleveling_enabled;
    bool static_wearleveling_enabled;
    uint32_t static_wearleveling_threshold;

    //Used to implement: "Preemptible I/O Scheduling of Garbage Collection for Solid State Drives", TCAD 2013.
    bool preemptible_gc_enabled;

#if UNBLOCK_NOT_IN_USE
    double gc_hard_threshold;
#endif
    uint32_t block_pool_gc_hard_threshold;
    uint32_t max_ongoing_gc_reqs_per_plane;//This value has two important usages: 1) maximum number of concurrent gc operations per plane, and 2) the value that determines urgent GC execution when there is a shortage of flash blocks. If the block bool size drops below this value, all incomming user writes should be blocked

    //Following variabels are used based on the type of GC block selection policy
    uint32_t rga_set_size;//The number of random flash blocks that are radnomly selected
    Utils::RandomGenerator random_generator;
    const uint32_t random_pp_threshold;

#if UNBLOCK_NOT_IN_USE
    std::queue<Block_Pool_Slot_Type*> block_usage_fifo;

    const uint32_t channel_count;
    const uint32_t chip_no_per_channel;
#endif
    const uint32_t die_no_per_chip;
    const uint32_t plane_no_per_die;
    const uint32_t block_no_per_plane;
    const uint32_t pages_no_per_block;

  private:
    const uint32_t __page_size_in_byte;

    FlashTransactionHandler<GC_and_WL_Unit_Base> __transaction_service_handler;

  protected:

    // Checks if block_address is a safe candidate for gc execution, i.e., 1) it
    // is not a write frontier, and 2) there is no ongoing program operation
    bool is_safe_gc_wl_candidate(const PlaneBookKeepingType& pbke,
                                 flash_block_ID_type gc_wl_candidate_block_id);
    bool check_static_wl_required(const NVM::FlashMemory::Physical_Page_Address& plane_address);
    void run_static_wearleveling(const NVM::FlashMemory::Physical_Page_Address& plane_address);

    NvmTransactionFlashER* _make_erase_tr(Block_Pool_Slot_Type& block,
                                          NVM::FlashMemory::Physical_Page_Address& address);

    template <bool WITH_SUBMIT>
    void _issue_erase_tr(Block_Pool_Slot_Type& block,
                         NVM::FlashMemory::Physical_Page_Address& address);
  private:
    NvmTransactionFlashWR* __make_copyback_write_tr(stream_id_type stream_id,
                                                    const NVM::FlashMemory::Physical_Page_Address& address,
                                                    NvmTransactionFlashER* erase_tr);

    NvmTransactionFlashWR* __make_simple_write_tr(stream_id_type stream_id,
                                                  const NVM::FlashMemory::Physical_Page_Address& address,
                                                  NvmTransactionFlashER* erase_tr);

    void __handle_transaction_service(NvmTransactionFlash* transaction);

  public:
    GC_and_WL_Unit_Base(const sim_object_id_type& id,
                        Address_Mapping_Unit_Base* address_mapping_unit,
                        Flash_Block_Manager_Base* block_manager,
                        TSU_Base* tsu,
                        NVM_PHY_ONFI* flash_controller,
                        Stats& stats,
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

    virtual bool GC_is_in_urgent_mode(const NVM::FlashMemory::Flash_Chip*) const = 0;
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

  force_inline NvmTransactionFlashER*
  GC_and_WL_Unit_Base::_make_erase_tr(Block_Pool_Slot_Type& block,
                                      NVM::FlashMemory::Physical_Page_Address& address)
  {
    auto* tr = new NvmTransactionFlashER(Transaction_Source_Type::GC_WL,
                                         block.Stream_id,
                                         address);
    // If there are some valid pages in block, then prepare flash transactions for
    // page movement
    if (block.Current_page_write_index - block.Invalid_page_count > 0) {
      // Lock the block, so no user request can intervene while the GC is progressing
      // address_mapping_unit->Lock_physical_block_for_gc(gc_candidate_address);

      for (flash_page_ID_type page = 0; page < block.Current_page_write_index; ++page) {
        if (block_manager->Is_page_valid(block, page)) {
#if UNBLOCK_NOT_IN_USE
          _stats.Total_page_movements_for_gc;
#endif
          address.PageID = page;
          tr->Page_movement_activities.emplace_back(_make_write_tr(block.Stream_id, address, tr));
        }
      }
    }

    block.Erase_transaction = tr;

    return tr;
  }

  template <bool WITH_SUBMIT> void
  GC_and_WL_Unit_Base::_issue_erase_tr(Block_Pool_Slot_Type& block,
                                       NVM::FlashMemory::Physical_Page_Address& address)
  {
    tsu->Prepare_for_transaction_submit();

    auto* tr = _make_erase_tr(block, address);

    if (WITH_SUBMIT)
      tsu->Submit_transaction(tr);

    tsu->Schedule();
  }

  // ----------------------
  // GC and WL Unit builder
  // ----------------------
  typedef std::shared_ptr<GC_and_WL_Unit_Base> GCnWLUnitPtr;
  GCnWLUnitPtr build_gc_and_wl_object(const DeviceParameterSet& params,
                                      FTL& ftl,
                                      Address_Mapping_Unit_Base& amu,
                                      Flash_Block_Manager_Base& fbm,
                                      TSU_Base& tsu,
                                      NVM_PHY_ONFI& phy,
                                      Stats& stats,
                                      double rho,
                                      uint32_t max_ongoing_gc_reqs_per_plane);
}

#endif // !GC_AND_WL_UNIT_BASE_H
