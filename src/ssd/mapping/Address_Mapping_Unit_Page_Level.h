#ifndef ADDRESS_MAPPING_UNIT_PAGE_LEVEL
#define ADDRESS_MAPPING_UNIT_PAGE_LEVEL

#include <unordered_map>
#include <map>
#include <queue>
#include <set>
#include <list>
#include "Address_Mapping_Unit_Base.h"
#include "../fbm/Flash_Block_Manager_Base.h"
#include "../SSD_Defs.h"
#include "../NvmTransactionFlashRD.h"
#include "../NvmTransactionFlashWR.h"

#include "AddressMappingUnitDefs.h"
#include "AllocationScheme.h"

namespace SSD_Components
{
#define MAKE_TABLE_INDEX(LPN,STREAM)

  enum class CMTEntryStatus {FREE, WAITING, VALID};

  struct GTDEntryType //Entry type for the Global Translation Directory
  {
    MPPN_type MPPN;
    data_timestamp_type TimeStamp;

    GTDEntryType();
  };

  struct CMTSlotType
  {
    PPA_type PPA;
    unsigned long long WrittenStateBitmap;
    bool Dirty;
    CMTEntryStatus Status;
    std::list<std::pair<LPA_type, CMTSlotType*>>::iterator listPtr;//used for fast implementation of LRU
    stream_id_type Stream_id;
  };

  struct GMTEntryType//Entry type for the Global Mapping Table
  {
    PPA_type PPA;
    uint64_t WrittenStateBitmap;
    data_timestamp_type TimeStamp;

    GMTEntryType();
  };
  
  class Cached_Mapping_Table
  {
  public:
    Cached_Mapping_Table(uint32_t capacity);
    ~Cached_Mapping_Table();
    bool Exists(const stream_id_type streamID, const LPA_type lpa);
    PPA_type Retrieve_ppa(const stream_id_type streamID, const LPA_type lpa);
    void Update_mapping_info(const stream_id_type streamID, const LPA_type lpa, const PPA_type ppa, const page_status_type pageWriteState);
    void Insert_new_mapping_info(const stream_id_type streamID, const LPA_type lpa, const PPA_type ppa, const unsigned long long pageWriteState);
    page_status_type Get_bitmap_vector_of_written_sectors(const stream_id_type streamID, const LPA_type lpa);

    bool Is_slot_reserved_for_lpn_and_waiting(const stream_id_type streamID, const LPA_type lpa);
    bool Check_free_slot_availability();
    void Reserve_slot_for_lpn(const stream_id_type streamID, const LPA_type lpa);
    CMTSlotType Evict_one_slot(LPA_type& lpa);
    
    bool Is_dirty(const stream_id_type streamID, const LPA_type lpa);
    void Make_clean(const stream_id_type streamID, const LPA_type lpa);
  private:
    std::unordered_map<LPA_type, CMTSlotType*> addressMap;
    std::list<std::pair<LPA_type, CMTSlotType*>> lruList;
    uint32_t capacity;
  };

  /* Each stream has its own address mapping domain. It helps isolation of GC interference
  * (e.g., multi-streamed SSD HotStorage 2014, and OPS isolation in FAST 2015)
  * However, CMT is shared among concurrent streams in two ways: 1) each address mapping domain
  * shares the whole CMT space with other domains, and 2) each address mapping domain has
  * its own share of CMT (equal partitioning of CMT space among concurrent streams).*/
  typedef std::multimap<LPA_type, NvmTransactionFlash*> FlashTransactionLpaMap;
  class AddressMappingDomain
  {
  public:
    AddressMappingDomain(uint32_t cmt_capacity,
                         uint32_t cmt_entry_size,
                         uint32_t no_of_translation_entries_per_page,
                         Cached_Mapping_Table* CMT,
                         Flash_Plane_Allocation_Scheme_Type PlaneAllocationScheme,
                         const ChannelIDs& channel_ids,
                         const ChipIDs& chip_ids,
                         const DieIDs& die_ids,
                         const PlaneIDs& plane_ids,
                         PPA_type total_physical_sectors_no,
                         LHA_type total_logical_sectors_no,
                         uint32_t sectors_no_per_page);
    ~AddressMappingDomain();

    LHA_type max_logical_sector_address;
    uint32_t Translation_entries_per_page;
    LPA_type Total_logical_pages_no;
    PPA_type Total_physical_pages_no;
    MVPN_type Total_translation_pages_no;

    /*Stores the mapping of Virtual Translation Page Number (MVPN) to Physical Translation Page Number (MPPN).
    * It is always kept in volatile memory.*/
    std::vector<GTDEntryType> GlobalTranslationDirectory;

    /*The logical to physical address mapping of all data pages that is implemented based on the DFTL (Gupta et al., ASPLOS 2009(
    * proposal. It is always stored in non-volatile flash memory.*/
    std::vector<GMTEntryType> GlobalMappingTable;

    /*The cached mapping table that is implemented based on the DFLT (Gupta et al., ASPLOS 2009) proposal.
    * It is always stored in volatile memory.*/
    uint32_t CMT_entry_size;
    Cached_Mapping_Table* CMT;
    uint32_t No_of_inserted_entries_in_preconditioning;


    void Update_mapping_info(const bool ideal_mapping, const stream_id_type stream_id, const LPA_type lpa, const PPA_type ppa, const page_status_type page_status_bitmap);
    page_status_type Get_page_status(const bool ideal_mapping, const stream_id_type stream_id, const LPA_type lpa);
    PPA_type Get_ppa(const bool ideal_mapping, const stream_id_type stream_id, const LPA_type lpa);
    PPA_type Get_ppa_for_preconditioning(const stream_id_type stream_id, const LPA_type lpa);
    bool Mapping_entry_accessible(const bool ideal_mapping, const stream_id_type stream_id, const LPA_type lpa);


    FlashTransactionLpaMap Waiting_unmapped_read_transactions;
    FlashTransactionLpaMap Waiting_unmapped_program_transactions;
    std::multimap<MVPN_type, LPA_type> ArrivingMappingEntries;
    std::set<MVPN_type> DepartingMappingEntries;
    std::set<LPA_type> Locked_LPAs;//Used to manage race conditions, i.e. a user request accesses and LPA while GC is moving that LPA 
    std::set<MVPN_type> Locked_MVPNs;//Used to manage race conditions
    FlashTransactionLpaMap Read_transactions_behind_LPA_barrier;
    FlashTransactionLpaMap Write_transactions_behind_LPA_barrier;
    std::set<MVPN_type> MVPN_read_transactions_waiting_behind_barrier;
    std::set<MVPN_type> MVPN_write_transaction_waiting_behind_barrier;

    ChannelIDs Channel_ids;
    ChipIDs    Chip_ids;
    DieIDs     Die_ids;
    PlaneIDs   Plane_ids;

    uint32_t Channel_no;
    uint32_t Chip_no;
    uint32_t Die_no;
    uint32_t Plane_no;

    PlaneAllocator plane_allocate;
    Flash_Plane_Allocation_Scheme_Type PlaneAllocationScheme;

  };

  class Address_Mapping_Unit_Page_Level : public Address_Mapping_Unit_Base
  {
    friend class GC_and_WL_Unit_Page_Level;

  private:
    FlashTransactionHandler<Address_Mapping_Unit_Page_Level> __transaction_service_handler;

    uint32_t cmt_capacity;

    std::vector<AddressMappingDomain> domains;
    uint32_t CMT_entry_size, GTD_entry_size;//In CMT MQSim stores (lpn, ppn, page status bits) but in GTD it only stores (ppn, page status bits)

    std::set<NvmTransactionFlashWR*>**** Write_transactions_for_overfull_planes;
    uint32_t no_of_translation_entries_per_page;

  private:
    // --------------------------
    // Internal private functions
    // --------------------------
    void allocate_plane_for_user_write(NvmTransactionFlashWR* transaction);
    void allocate_page_in_plane_for_user_write(NvmTransactionFlashWR* transaction, bool is_for_gc);
    void allocate_plane_for_translation_write(NvmTransactionFlash* transaction);
    void allocate_page_in_plane_for_translation_write(NvmTransactionFlash* transaction, MVPN_type mvpn, bool is_for_gc);
    void allocate_plane_for_preconditioning(stream_id_type stream_id, LPA_type lpn, NVM::FlashMemory::Physical_Page_Address& targetAddress);
    bool request_mapping_entry(stream_id_type streamID, LPA_type lpn);
    bool translate_lpa_to_ppa(stream_id_type streamID, NvmTransactionFlash* transaction);

    void generate_flash_read_request_for_mapping_data(stream_id_type streamID, LPA_type lpn);
    void generate_flash_writeback_request_for_mapping_data(stream_id_type streamID, LPA_type lpn);

    MVPN_type get_MVPN(LPA_type lpn, stream_id_type stream_id) const;
    LPA_type get_start_LPN_in_MVP(MVPN_type mvpn) const;
    LPA_type get_end_LPN_in_MVP(MVPN_type mvpn) const;

    std::set<NvmTransactionFlashWR*>& __wr_transaction_for_overfull_plane(const NVM::FlashMemory::Physical_Page_Address& address);

    void manage_unsuccessful_translation(NvmTransactionFlash *transaction);

    template <typename function>
    void __processing_unmapped_transactions(FlashTransactionLpaMap& unmapped_ios,
                                            LPA_type lpa, uint32_t stream_id,
                                            function lambda);

    void __handle_transaction_service_signal(NvmTransactionFlash* transaction);

    void __ppa_to_address(PPA_type ppn,
                          NVM::FlashMemory::Physical_Page_Address& address);

  public:
    Address_Mapping_Unit_Page_Level(const sim_object_id_type& id,
                                    FTL* ftl,
                                    NVM_PHY_ONFI* flash_controller,
                                    Flash_Block_Manager_Base* block_manager,
                                    Stats& stats,
                                    bool ideal_mapping_table,
                                    uint32_t cmt_capacity_in_byte,
                                    Flash_Plane_Allocation_Scheme_Type PlaneAllocationScheme,
                                    uint32_t ConcurrentStreamNo,
                                    uint32_t ChannelCount,
                                    uint32_t chip_no_per_channel,
                                    uint32_t DieNoPerChip,
                                    uint32_t PlaneNoPerDie,
                                    const StreamChannelIDs& stream_channel_ids,
                                    const StreamChipIDs& stream_chip_ids,
                                    const StreamDieIDs& stream_die_ids,
                                    const StreamPlaneIDs& stream_plane_ids,
                                    uint32_t Block_no_per_plane,
                                    uint32_t Page_no_per_block,
                                    uint32_t SectorsPerPage,
                                    uint32_t PageSizeInBytes,
                                    double Overprovisioning_ratio,
                                    CMT_Sharing_Mode sharing_mode = CMT_Sharing_Mode::SHARED,
                                    bool fold_large_addresses = true);
    ~Address_Mapping_Unit_Page_Level() final = default;

    // --------------------------
    // Over-ridden from SimObject
    // --------------------------
    void Start_simulation() final;
    void Setup_triggers() final;

    // ------------------------------------------
    // Over-ridden from Address_Mapping_Unit_Base
    // ------------------------------------------
    void Allocate_address_for_preconditioning(stream_id_type stream_id,
                                              std::map<LPA_type, page_status_type>& lpa_list,
                                              std::vector<double>& steady_state_distribution) final;

    int Bring_to_CMT_for_preconditioning(stream_id_type stream_id,
                                         LPA_type lpa) final;

    void Store_mapping_table_on_flash_at_start() final;

    uint32_t Get_cmt_capacity() final;

    uint32_t Get_current_cmt_occupancy_for_stream(stream_id_type stream_id) final;

    LPA_type Get_logical_pages_count(stream_id_type stream_id) final;

    void Translate_lpa_to_ppa_and_dispatch(const std::list<NvmTransaction*>& transactions) final;

    void Get_data_mapping_info_for_gc(stream_id_type stream_id,
                                      LPA_type lpa,
                                      PPA_type& ppa,
                                      page_status_type& page_state) final;

    void Get_translation_mapping_info_for_gc(stream_id_type stream_id,
                                             MVPN_type mvpn,
                                             MPPN_type& mppa,
                                             sim_time_type& timestamp) final;

    void Allocate_new_page_for_gc(NvmTransactionFlashWR* transaction,
                                  bool is_translation_page) final;

    NVM::FlashMemory::Physical_Page_Address Convert_ppa_to_address(PPA_type ppa) final;

    void Convert_ppa_to_address(PPA_type ppn,
                                NVM::FlashMemory::Physical_Page_Address& address) final;

    PPA_type Convert_address_to_ppa(const NVM::FlashMemory::Physical_Page_Address& pageAddress) final;

    // Barrier control functions
    void Set_barrier_for_accessing_physical_block(const NVM::FlashMemory::Physical_Page_Address& block_address) final;
    void Set_barrier_for_accessing_lpa(stream_id_type stream_id, LPA_type lpa) final;
    void Set_barrier_for_accessing_mvpn(stream_id_type stream_id, MVPN_type mpvn) final;
    void Remove_barrier_for_accessing_lpa(stream_id_type stream_id, LPA_type lpa) final;
    void Remove_barrier_for_accessing_mvpn(stream_id_type stream_id, MVPN_type mpvn) final;
    void Start_servicing_writes_for_overfull_plane(NVM::FlashMemory::Physical_Page_Address plane_address) final;

  protected:
    bool query_cmt(NvmTransactionFlash* transaction) final;

    PPA_type online_create_entry_for_reads(LPA_type lpa,
                                           stream_id_type stream_id,
                                           NVM::FlashMemory::Physical_Page_Address& read_address,
                                           uint64_t read_sectors_bitmap) final;

    void manage_user_transaction_facing_barrier(NvmTransactionFlash* transaction) final;

    void manage_mapping_transaction_facing_barrier(stream_id_type stream_id,
                                                   MVPN_type mvpn,
                                                   bool read) final;

    bool is_lpa_locked_for_gc(stream_id_type stream_id,
                              LPA_type lpa) final;

    bool is_mvpn_locked_for_gc(stream_id_type stream_id,
                               MVPN_type mvpn) final;

  };

}

#endif // !ADDRESS_MAPPING_UNIT_PAGE_LEVEL