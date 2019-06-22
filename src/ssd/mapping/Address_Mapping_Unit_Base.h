#ifndef ADDRESS_MAPPING_UNIT_BASE_H
#define ADDRESS_MAPPING_UNIT_BASE_H

#include "../../sim/Sim_Object.h"
#include "../../nvm_chip/flash_memory/Physical_Page_Address.h"
#include "../../nvm_chip/flash_memory/FlashTypes.h"
#include "../SSD_Defs.h"
#include "../NVM_Transaction_Flash.h"
#include "../phy/NVM_PHY_ONFI_NVDDR2.h"
#include "../FTL.h"
#include "../Flash_Block_Manager_Base.h"
#include "AddressMappingUnitDefs.h"

namespace SSD_Components
{
  class FTL;
  class Flash_Block_Manager_Base;

  class Address_Mapping_Unit_Base : public MQSimEngine::Sim_Object
  {
  protected:
    FTL* ftl;
    NVM_PHY_ONFI* flash_controller;
    Flash_Block_Manager_Base* block_manager;
    CMT_Sharing_Mode sharing_mode;

    // If mapping is ideal, then all the mapping entries are found in the DRAM
    // and there is no need to read mapping entries from flash
    bool ideal_mapping_table;

    uint32_t no_of_input_streams;
    LHA_type max_logical_sector_address;
    uint32_t total_logical_pages_no = 0;

    uint32_t channel_count;
    uint32_t chip_no_per_channel;
    uint32_t die_no_per_chip;
    uint32_t plane_no_per_die;
    uint32_t block_no_per_plane;
    uint32_t pages_no_per_block;
    uint32_t sector_no_per_page;
    uint32_t page_size_in_byte;
    uint32_t total_physical_pages_no = 0;
    uint32_t page_no_per_channel = 0;
    uint32_t page_no_per_chip = 0;
    uint32_t page_no_per_die = 0;
    uint32_t page_no_per_plane = 0;
    double overprovisioning_ratio;
    bool fold_large_addresses;
    bool mapping_table_stored_on_flash;

  public:
    Address_Mapping_Unit_Base(const sim_object_id_type& id, FTL* ftl, NVM_PHY_ONFI* FlashController, Flash_Block_Manager_Base* block_manager,
      bool ideal_mapping_table, uint32_t no_of_input_streams,
      uint32_t ChannelCount, uint32_t chip_no_per_channel, uint32_t DieNoPerChip, uint32_t PlaneNoPerDie,
      uint32_t Block_no_per_plane, uint32_t Page_no_per_block, uint32_t SectorsPerPage, uint32_t PageSizeInBytes,
      double Overprovisioning_ratio, CMT_Sharing_Mode sharing_mode = CMT_Sharing_Mode::SHARED, bool fold_large_addresses = true);
    ~Address_Mapping_Unit_Base() override = default;

    //Functions used for preconditioning
    virtual void Allocate_address_for_preconditioning(stream_id_type stream_id,
                                                      std::map<LPA_type, page_status_type>& lpa_list,
                                                      std::vector<double>& steady_state_distribution) = 0;

    //Used for warming up the cached mapping table during preconditioning
    virtual int Bring_to_CMT_for_preconditioning(stream_id_type stream_id,
                                                 LPA_type lpa) = 0;

    // It should only be invoked at the beginning of the simulation to store
    // mapping table entries on the flash space
    virtual void Store_mapping_table_on_flash_at_start() = 0;

    // Returns the maximum number of entries that could be stored in
    // the cached mapping table
    virtual uint32_t Get_cmt_capacity() = 0;

    virtual uint32_t Get_current_cmt_occupancy_for_stream(stream_id_type stream_id) = 0;

    //Returns the number of logical pages allocated to an I/O stream
    virtual LPA_type Get_logical_pages_count(stream_id_type stream_id) = 0;

    //Address translation functions
    virtual void Translate_lpa_to_ppa_and_dispatch(const std::list<NVM_Transaction*>& transactionList) = 0;

    virtual void Get_data_mapping_info_for_gc(stream_id_type stream_id,
                                              LPA_type lpa,
                                              PPA_type& ppa,
                                              page_status_type& page_state) = 0;

    virtual void Get_translation_mapping_info_for_gc(stream_id_type stream_id,
                                                     MVPN_type mvpn,
                                                     MPPN_type& mppa,
                                                     sim_time_type& timestamp) = 0;

    virtual void Allocate_new_page_for_gc(NVM_Transaction_Flash_WR* transaction,
                                          bool is_translation_page) = 0;

    virtual NVM::FlashMemory::Physical_Page_Address Convert_ppa_to_address(PPA_type ppa) = 0;

    virtual void Convert_ppa_to_address(PPA_type ppa,
                                        NVM::FlashMemory::Physical_Page_Address& address) = 0;

    virtual PPA_type Convert_address_to_ppa(const NVM::FlashMemory::Physical_Page_Address& pageAddress) = 0;

    uint32_t Get_no_of_input_streams()
    { return no_of_input_streams; }

    // Checks if ideal mapping table is enabled in which all address
    // translations entries are always in CMT (i.e., CMT is infinite in size)
    // and thus all address translation requests are always successful
    bool Is_ideal_mapping_table();

    //Returns the number of physical pages in the device
    uint32_t Get_device_physical_pages_count();
    CMT_Sharing_Mode Get_CMT_sharing_mode();

    /***************************************************************************
     These are system state consistency control functions that are used for
     garbage collection and wear-leveling execution.
     Once the GC_and_WL_Unit_Base starts moving a logical page (LPA) from one
     physical location to another physcial location, no new request should be
     allowed to the moving LPA. Otherwise, the system may become inconsistent.

     GC starts on a physical block
       ---->  set barrier for physical block
       ----(LPAs are read from flash)---->  set barrier for LPA
       ----(LPA is written into its new location)---->  remove barrier for LPA

    ***************************************************************************/
    // At the very beginning of executing a GC request, the GC target physical
    // block (that is selected for erase) should be protected by a barrier.
    // The LPAs within this block are unknown until the content of the physical
    // pages within the block are read one-by-one. Therefore, at the start of
    // the GC execution, the barrier is set for the physical block. Later,
    // when the LPAs are read from the physical block, the above functions
    // are used to lock each of the LPAs.
    virtual void Set_barrier_for_accessing_physical_block(const NVM::FlashMemory::Physical_Page_Address& block_address) = 0;

    // It sets a barrier for accessing an LPA and MVPN, when the GC unit
    // (i.e., GC_and_WL_Unit_Base) starts moving an LPA or a MVPN from one
    // physical page to another physical page. This type of barrier is pretty
    // much like a memory barrier in CPU, i.e., all accesses to the lpa that
    // issued before setting the barrier still can be executed, but no new
    // access is allowed.
    virtual void Set_barrier_for_accessing_lpa(stream_id_type stream_id,
                                               LPA_type lpa) = 0;

    virtual void Set_barrier_for_accessing_mvpn(stream_id_type stream_id,
                                                MVPN_type mvpn) = 0;

    // Removes the barrier that has already been set for accessing an LPA and
    // MVPN (i.e., the GC_and_WL_Unit_Base unit successfully finished relocating
    // LPA or MVPN from one physical location to another physical location).
    virtual void Remove_barrier_for_accessing_lpa(stream_id_type stream_id,
                                                  LPA_type lpa) = 0;

    virtual void Remove_barrier_for_accessing_mvpn(stream_id_type stream_id,
                                                   MVPN_type mvpn) = 0;

    // This function is invoked when GC execution is finished on a plane and
    // the plane has enough number of free pages to service writes.
    virtual void Start_servicing_writes_for_overfull_plane(NVM::FlashMemory::Physical_Page_Address plane_address) = 0;

  protected:
    virtual bool query_cmt(NVM_Transaction_Flash* transaction) = 0;

    virtual PPA_type online_create_entry_for_reads(LPA_type lpa,
                                                   stream_id_type stream_id,
                                                   NVM::FlashMemory::Physical_Page_Address& read_address,
                                                   uint64_t read_sectors_bitmap) = 0;

    virtual void manage_user_transaction_facing_barrier(NVM_Transaction_Flash* transaction) = 0;

    virtual void manage_mapping_transaction_facing_barrier(stream_id_type stream_id,
                                                           MVPN_type mvpn,
                                                           bool read) = 0;

    virtual bool is_lpa_locked_for_gc(stream_id_type stream_id,
                                      LPA_type lpa) = 0;

    virtual bool is_mvpn_locked_for_gc(stream_id_type stream_id,
                                       MVPN_type mvpn) = 0;
  };
}

#endif // !ADDRESS_MAPPING_UNIT_BASE_H
