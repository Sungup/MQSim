#ifndef GC_AND_WL_UNIT_PAGE_LEVEL_H
#define GC_AND_WL_UNIT_PAGE_LEVEL_H

#include "GC_and_WL_Unit_Base.h"
#include "../NVM_PHY_ONFI.h"
#include "../../utils/RandomGenerator.h"
#include <queue>


namespace SSD_Components
{
  class GC_and_WL_Unit_Page_Level : public GC_and_WL_Unit_Base
  {
  public:
    GC_and_WL_Unit_Page_Level(const sim_object_id_type& id,
      Address_Mapping_Unit_Base* address_mapping_unit, Flash_Block_Manager_Base* block_manager, TSU_Base* tsu, NVM_PHY_ONFI* flash_controller,
      GC_Block_Selection_Policy_Type block_selection_policy, double gc_threshold, bool preemptible_gc_enabled, double gc_hard_threshold,
      uint32_t channel_count, uint32_t chip_no_per_channel, uint32_t die_no_per_chip, uint32_t plane_no_per_die,
      uint32_t block_no_per_plane, uint32_t page_no_per_block, uint32_t sectors_per_page,
      bool use_copyback, double rho, uint32_t max_ongoing_gc_reqs_per_plane = 10,
      bool dynamic_wearleveling_enabled = true, bool static_wearleveling_enabled = true, uint32_t static_wearleveling_threshold = 100, int seed = 432);

    /*This function is used for implementing preemptible GC execution. If for a flash chip the free block
    * pool becomes close to empty, then the GC requests for that flash chip should be prioritized and
    * GC should go on in non-preemptible mode.*/
    bool GC_is_in_urgent_mode(const NVM::FlashMemory::Flash_Chip*);

    void Check_gc_required(const uint32_t free_block_pool_size, const NVM::FlashMemory::Physical_Page_Address& plane_address);
  private:
    NVM_PHY_ONFI * flash_controller;
  };
}
#endif // !GC_AND_WL_UNIT_PAGE_LEVEL_H
