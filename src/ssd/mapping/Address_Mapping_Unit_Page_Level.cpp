#include "Address_Mapping_Unit_Page_Level.h"

#include <assert.h>
#include <cmath>
#include <stdexcept>

#include "../FTL.h"

#include "../Stats.h"
#include "../../utils/Logical_Address_Partitioning_Unit.h"

using namespace SSD_Components;

force_inline
GMTEntryType::GMTEntryType()
  : PPA(NO_PPA),
    WrittenStateBitmap(UNWRITTEN_LOGICAL_PAGE),
    TimeStamp(0)
{}

force_inline
GTDEntryType::GTDEntryType()
  : MPPN(NO_MPPN),
    TimeStamp(INVALID_TIME_STAMP)
{ }

Cached_Mapping_Table::Cached_Mapping_Table(uint32_t capacity)
  : capacity(capacity)
{ }

Cached_Mapping_Table::~Cached_Mapping_Table()
{
  // std::unordered_map<LPA_type, CMTSlotType*> addressMap;
  // std::list<std::pair<LPA_type, CMTSlotType*>> lruList;

  auto entry = addressMap.begin();
  while (entry != addressMap.end())
  {
    delete (*entry).second;
    addressMap.erase(entry++);
  }
}

inline bool Cached_Mapping_Table::Exists(const stream_id_type streamID, const LPA_type lpa)
{
  LPA_type key = LPN_TO_UNIQUE_KEY(streamID, lpa);
  auto it = addressMap.find(key);
  if (it == addressMap.end())
  {
    PRINT_DEBUG("Address mapping table query - Stream ID:" << streamID << ", LPA:" << lpa << ", MISS")
      return false;
  }
  if (it->second->Status != CMTEntryStatus::VALID)
  {
    return false;
    PRINT_DEBUG("Address mapping table query - Stream ID:" << streamID << ", LPA:" << lpa << ", MISS")
  }
  PRINT_DEBUG("Address mapping table query - Stream ID:" << streamID << ", LPA:" << lpa << ", HIT")
    return true;
}
PPA_type Cached_Mapping_Table::Retrieve_ppa(const stream_id_type streamID, const LPA_type lpn)
{
  LPA_type key = LPN_TO_UNIQUE_KEY(streamID, lpn);
  auto it = addressMap.find(key);
  assert(it != addressMap.end());
  assert(it->second->Status == CMTEntryStatus::VALID);

  lruList.splice(lruList.begin(), lruList, it->second->listPtr);
  return it->second->PPA;
}
page_status_type Cached_Mapping_Table::Get_bitmap_vector_of_written_sectors(const stream_id_type streamID, const LPA_type lpn)
{
  LPA_type key = LPN_TO_UNIQUE_KEY(streamID, lpn);
  auto it = addressMap.find(key);
  assert(it != addressMap.end());
  assert(it->second->Status == CMTEntryStatus::VALID);
  return it->second->WrittenStateBitmap;
}
void Cached_Mapping_Table::Update_mapping_info(const stream_id_type streamID, const LPA_type lpa, const PPA_type ppa, const page_status_type pageWriteState)
{
  LPA_type key = LPN_TO_UNIQUE_KEY(streamID, lpa);
  auto it = addressMap.find(key);
  assert(it != addressMap.end());
  assert(it->second->Status == CMTEntryStatus::VALID);
  it->second->PPA = ppa;
  it->second->WrittenStateBitmap = pageWriteState;
  it->second->Dirty = true;
  it->second->Stream_id = streamID;
  PRINT_DEBUG("Address mapping table update entry - Stream ID:" << streamID << ", LPA:" << lpa << ", PPA:" << ppa)
}
void Cached_Mapping_Table::Insert_new_mapping_info(const stream_id_type streamID, const LPA_type lpa, const PPA_type ppa, const unsigned long long pageWriteState)
{
  LPA_type key = LPN_TO_UNIQUE_KEY(streamID, lpa);
  auto it = addressMap.find(key);
  if (it == addressMap.end())
    throw std::logic_error("No slot is reserved!");

  it->second->Status = CMTEntryStatus::VALID;
  it->second->PPA = ppa;
  it->second->WrittenStateBitmap = pageWriteState;
  it->second->Dirty = false;
  it->second->Stream_id = streamID;
  PRINT_DEBUG("Address mapping table insert entry - Stream ID:" << streamID << ", LPA:" << lpa << ", PPA:" << ppa)
}
bool Cached_Mapping_Table::Is_slot_reserved_for_lpn_and_waiting(const stream_id_type streamID, const LPA_type lpn)
{
  LPA_type key = LPN_TO_UNIQUE_KEY(streamID, lpn);
  auto it = addressMap.find(key);
  if (it != addressMap.end())
    if (it->second->Status == CMTEntryStatus::WAITING)
      return true;
  return false;
}
inline bool Cached_Mapping_Table::Check_free_slot_availability()
{
  return addressMap.size() < capacity;
}
void Cached_Mapping_Table::Reserve_slot_for_lpn(const stream_id_type streamID, const LPA_type lpn)
{
  LPA_type key = LPN_TO_UNIQUE_KEY(streamID, lpn);
  if (addressMap.find(key) != addressMap.end())
    throw std::logic_error("Duplicate lpa insertion into CMT!");
  if (addressMap.size() >= capacity)
    throw std::logic_error("CMT overfull!");

  auto* cmtEnt = new CMTSlotType();
  cmtEnt->Dirty = false;
  cmtEnt->Stream_id = streamID;
  lruList.push_front(std::pair<LPA_type, CMTSlotType*>(key, cmtEnt));
  cmtEnt->Status = CMTEntryStatus::WAITING;
  cmtEnt->listPtr = lruList.begin();
  addressMap[key] = cmtEnt;
}
CMTSlotType Cached_Mapping_Table::Evict_one_slot(LPA_type& lpa)
{
  assert(addressMap.size() > 0);
  addressMap.erase(lruList.back().first);
  lpa = UNIQUE_KEY_TO_LPN(lruList.back().second->Stream_id,lruList.back().first);
  CMTSlotType evictedItem = *lruList.back().second;
  delete lruList.back().second;
  lruList.pop_back();
  return evictedItem;
}
bool Cached_Mapping_Table::Is_dirty(const stream_id_type streamID, const LPA_type lpa)
{
  LPA_type key = LPN_TO_UNIQUE_KEY(streamID, lpa);
  auto it = addressMap.find(key);
  if (it == addressMap.end())
    throw std::logic_error("The requested slot does not exist!");

  return it->second->Dirty;
}
void Cached_Mapping_Table::Make_clean(const stream_id_type streamID, const LPA_type lpn)
{
  LPA_type key = LPN_TO_UNIQUE_KEY(streamID, lpn);
  auto it = addressMap.find(key);
  if (it == addressMap.end())
    throw std::logic_error("The requested slot does not exist!");

  it->second->Dirty = false;
}


AddressMappingDomain::AddressMappingDomain(uint32_t cmt_capacity,
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
                                           uint32_t sectors_no_per_page)
  : max_logical_sector_address(total_logical_sectors_no),
    Translation_entries_per_page(no_of_translation_entries_per_page),
    Total_logical_pages_no((max_logical_sector_address / sectors_no_per_page) + (max_logical_sector_address % sectors_no_per_page == 0? 0 : 1)),
    Total_physical_pages_no(total_physical_sectors_no / sectors_no_per_page),
    Total_translation_pages_no(MVPN_type(Total_logical_pages_no / Translation_entries_per_page)),
    GlobalTranslationDirectory(Total_translation_pages_no + 1),
    GlobalMappingTable(Total_logical_pages_no),
    CMT_entry_size(cmt_entry_size),
    No_of_inserted_entries_in_preconditioning(0),
    Channel_ids(channel_ids.begin(), channel_ids.end()),
    Chip_ids(chip_ids.begin(), chip_ids.end()),
    Die_ids(die_ids.begin(), die_ids.end()),
    Plane_ids(plane_ids.begin(), plane_ids.end()),
    Channel_no(channel_ids.size()),
    Chip_no(chip_ids.size()),
    Die_no(die_ids.size()),
    Plane_no(plane_ids.size()),
    plane_allocate(Channel_ids, Chip_ids, Die_ids, Plane_ids,
                   PlaneAllocationScheme),
    PlaneAllocationScheme(PlaneAllocationScheme)
{
  //Total_logical_pages_no = (uint32_t)((double)Total_physical_pages_no * (1 - Overprovisioning_ratio));
  //max_logical_sector_address = (LHA_type)(Sectors_no_per_page * Total_logical_pages_no - 1);

  if (CMT == nullptr)//If CMT is nullptr, then each address mapping domain should create its own CMT
    this->CMT = new Cached_Mapping_Table(cmt_capacity);//Each flow (address mapping domain) has its own CMT, so CMT is create here in the constructor
  else this->CMT = CMT;//The entire CMT space is shared among concurrently running flows (i.e., address mapping domains of all flow)
}

AddressMappingDomain::~AddressMappingDomain()
{
  delete CMT;

  auto read_entry = Waiting_unmapped_read_transactions.begin();
  while (read_entry != Waiting_unmapped_read_transactions.end())
  {
    delete read_entry->second;
    Waiting_unmapped_read_transactions.erase(read_entry++);
  }
  auto entry_write = Waiting_unmapped_program_transactions.begin();
  while (entry_write != Waiting_unmapped_program_transactions.end())
  {
    delete entry_write->second;
    Waiting_unmapped_program_transactions.erase(entry_write++);
  }
}

inline void AddressMappingDomain::Update_mapping_info(const bool ideal_mapping, const stream_id_type stream_id, const LPA_type lpa, const PPA_type ppa, const page_status_type page_status_bitmap)
{
  if (ideal_mapping)
  {
    GlobalMappingTable[lpa].PPA = ppa;
    GlobalMappingTable[lpa].WrittenStateBitmap = page_status_bitmap;
    GlobalMappingTable[lpa].TimeStamp = CurrentTimeStamp;
  }
  else
    CMT->Update_mapping_info(stream_id, lpa, ppa, page_status_bitmap);

}
inline page_status_type AddressMappingDomain::Get_page_status(const bool ideal_mapping, const stream_id_type stream_id, const LPA_type lpa)
{
  if (ideal_mapping)
    return GlobalMappingTable[lpa].WrittenStateBitmap;
  else
    return CMT->Get_bitmap_vector_of_written_sectors(stream_id, lpa);
}
inline PPA_type AddressMappingDomain::Get_ppa(const bool ideal_mapping, const stream_id_type stream_id, const LPA_type lpa)
{
  if (ideal_mapping)
    return GlobalMappingTable[lpa].PPA;
  else
    return CMT->Retrieve_ppa(stream_id, lpa);
}
inline PPA_type AddressMappingDomain::Get_ppa_for_preconditioning(const stream_id_type /* stream_id */, const LPA_type lpa)
{
  return GlobalMappingTable[lpa].PPA;
}
inline bool AddressMappingDomain::Mapping_entry_accessible(const bool ideal_mapping, const stream_id_type stream_id, const LPA_type lpa)
{
  if (ideal_mapping)
    return true;
  else
    return CMT->Exists(stream_id, lpa);
}

// =====================================================
// Address Mapping Unit Page Level class implementations
// =====================================================
Address_Mapping_Unit_Page_Level::Address_Mapping_Unit_Page_Level(const sim_object_id_type& id,
                                                                 FTL* ftl,
                                                                 NVM_PHY_ONFI* flash_controller,
                                                                 Flash_Block_Manager_Base* block_manager,
                                                                 Stats& stats,
                                                                 bool ideal_mapping_table,
                                                                 uint32_t cmt_capacity_in_byte,
                                                                 Flash_Plane_Allocation_Scheme_Type PlaneAllocationScheme,
                                                                 uint32_t concurrent_stream_no,
                                                                 uint32_t channel_count,
                                                                 uint32_t chip_no_per_channel,
                                                                 uint32_t die_no_per_chip,
                                                                 uint32_t plane_no_per_die,
                                                                 const StreamChannelIDs& stream_channel_ids,
                                                                 const StreamChipIDs& stream_chip_ids,
                                                                 const StreamDieIDs& stream_die_ids,
                                                                 const StreamPlaneIDs& stream_plane_ids,
                                                                 uint32_t Block_no_per_plane,
                                                                 uint32_t Page_no_per_block,
                                                                 uint32_t SectorsPerPage,
                                                                 uint32_t PageSizeInByte,
                                                                 double Overprovisioning_ratio,
                                                                 CMT_Sharing_Mode sharing_mode,
                                                                 bool fold_large_addresses)
  : Address_Mapping_Unit_Base(id,
                              ftl,
                              flash_controller,
                              block_manager,
                              stats,
                              ideal_mapping_table,
                              concurrent_stream_no,
                              channel_count,
                              chip_no_per_channel,
                              die_no_per_chip,
                              plane_no_per_die,
                              Block_no_per_plane,
                              Page_no_per_block,
                              SectorsPerPage,
                              PageSizeInByte,
                              Overprovisioning_ratio,
                              sharing_mode,
                              fold_large_addresses),
    __transaction_service_handler(this, &Address_Mapping_Unit_Page_Level::__handle_transaction_service_signal)
{
  Write_transactions_for_overfull_planes = new std::set<NvmTransactionFlashWR*>***[channel_count];
  for (uint32_t channel_id = 0; channel_id < channel_count; channel_id++)
  {
    Write_transactions_for_overfull_planes[channel_id] = new std::set<NvmTransactionFlashWR*>**[chip_no_per_channel];
    for (uint32_t chip_id = 0; chip_id < chip_no_per_channel; chip_id++)
    {
      Write_transactions_for_overfull_planes[channel_id][chip_id] = new std::set<NvmTransactionFlashWR*>*[die_no_per_chip];
      for (uint32_t die_id = 0; die_id < die_no_per_chip; die_id++)
        Write_transactions_for_overfull_planes[channel_id][chip_id][die_id] = new std::set<NvmTransactionFlashWR*>[plane_no_per_die];
    }
  }


  /* Since we want to have the same mapping table entry size for all streams, the entry size
  *  is calculated at this level and then pass it to the constructors of mapping domains
  * entry size = sizeOf(lpa) + sizeOf(ppn) + sizeOf(bit vector that shows written sectors of a page)
  */
  CMT_entry_size = (uint32_t)std::ceil(((2 * std::log2(total_physical_pages_no)) + sector_no_per_page) / 8);
  //In GTD we do not need to store lpa
  GTD_entry_size = (uint32_t)std::ceil((std::log2(total_physical_pages_no) + sector_no_per_page) / 8);
  no_of_translation_entries_per_page = (SectorsPerPage * SECTOR_SIZE_IN_BYTE) / GTD_entry_size;

  cmt_capacity = cmt_capacity_in_byte / CMT_entry_size;

  // domains = new AddressMappingDomain*[no_of_input_streams];
  domains.reserve(no_of_input_streams);

  for (uint32_t domainID = 0; domainID < no_of_input_streams; domainID++)
  {
    Cached_Mapping_Table* sharedCMT = nullptr;
    uint32_t per_stream_cmt_capacity = 0;
    switch (sharing_mode)
    {
    case CMT_Sharing_Mode::SHARED:
      per_stream_cmt_capacity = cmt_capacity;
      sharedCMT = new Cached_Mapping_Table(cmt_capacity);
      break;
    case CMT_Sharing_Mode::EQUAL_SIZE_PARTITIONING:
      per_stream_cmt_capacity = cmt_capacity / no_of_input_streams;
      break;
    }

    for (auto v : stream_channel_ids[domainID])
      if (channel_count <= v)
        throw mqsim_error("Invalid channel ID specified for I/O flow "
                            + std::to_string(domainID));

    for (auto v : stream_chip_ids[domainID])
      if (chip_no_per_channel <= v)
        throw mqsim_error("Invalid chip ID specified for I/O flow "
                            + std::to_string(domainID));

    for (auto v : stream_die_ids[domainID])
      if (die_no_per_chip <= v)
        throw mqsim_error("Invalid domain ID specified for I/O flow "
                            + std::to_string(domainID));

    for (auto v : stream_plane_ids[domainID])
      if (plane_no_per_die <= v)
        throw mqsim_error("Invalid plane ID specified for I/O flow "
                            + std::to_string(domainID));

    domains.emplace_back(per_stream_cmt_capacity,
                         CMT_entry_size,
                         no_of_translation_entries_per_page,
                         sharedCMT,
                         PlaneAllocationScheme,
                         stream_channel_ids[domainID],
                         stream_chip_ids[domainID],
                         stream_die_ids[domainID],
                         stream_plane_ids[domainID],
                         Utils::Logical_Address_Partitioning_Unit::PDA_count_allocate_to_flow(domainID),
                         Utils::Logical_Address_Partitioning_Unit::LHA_count_allocate_to_flow_from_device_view(domainID),
                         sector_no_per_page);
  }
}

// --------------------------
// Internal private functions
// --------------------------
force_inline void
Address_Mapping_Unit_Page_Level::allocate_plane_for_user_write(NvmTransactionFlashWR* transaction)
{
  domains[transaction->Stream_id].plane_allocate(transaction->Address,
                                                 transaction->LPA);
}

force_inline void
Address_Mapping_Unit_Page_Level::allocate_page_in_plane_for_user_write(NvmTransactionFlashWR* transaction,
                                                                       bool is_for_gc)
{
  AddressMappingDomain& domain = domains[transaction->Stream_id];
  PPA_type old_ppa = domain.Get_ppa(ideal_mapping_table, transaction->Stream_id, transaction->LPA);

  if (old_ppa == NO_PPA)  /*this is the first access to the logical page*/
  {
    if (is_for_gc)
    PRINT_ERROR("Unexpected mapping table status in allocate_page_in_plane_for_user_write function for a GC/WL write!")
  }
  else
  {
    if (is_for_gc)
    {
      NVM::FlashMemory::Physical_Page_Address addr;
      Convert_ppa_to_address(old_ppa, addr);
      block_manager->Invalidate_page_in_block(transaction->Stream_id, addr);
      page_status_type page_status_in_cmt = domain.Get_page_status(ideal_mapping_table, transaction->Stream_id, transaction->LPA);
      if (page_status_in_cmt != transaction->write_sectors_bitmap)
      PRINT_ERROR("Unexpected mapping table status in allocate_page_in_plane_for_user_write for a GC/WL write!")
    }
    else
    {
      page_status_type prev_page_status = domain.Get_page_status(ideal_mapping_table, transaction->Stream_id, transaction->LPA);
      page_status_type status_intersection = transaction->write_sectors_bitmap & prev_page_status;
      if (status_intersection == prev_page_status)//check if an update read is required
      {
        NVM::FlashMemory::Physical_Page_Address addr;
        Convert_ppa_to_address(old_ppa, addr);
        block_manager->Invalidate_page_in_block(transaction->Stream_id, addr);
      }
      else
      {
        page_status_type read_pages_bitmap = status_intersection ^ prev_page_status;

        auto* read_tr = _make_update_read_tr(transaction,
                                             read_pages_bitmap,
                                             domain.GlobalMappingTable[transaction->LPA].TimeStamp,
                                             old_ppa);

        block_manager->Read_transaction_issued(read_tr->Address);//Inform block manager about a new transaction as soon as the transaction's target address is determined
        block_manager->Invalidate_page_in_block(transaction->Stream_id, read_tr->Address);
      }
    }
  }

  /*The following lines should not be ordered with respect to the block_manager->Invalidate_page_in_block
  * function call in the above code blocks. Otherwise, GC may be invoked (due to the call to Allocate_block_....) and
  * may decide to move a page that is just invalidated.*/
  if (is_for_gc)
    block_manager->Allocate_block_and_page_in_plane_for_gc_write(transaction->Stream_id, transaction->Address);
  else
    block_manager->Allocate_block_and_page_in_plane_for_user_write(transaction->Stream_id, transaction->Address);
  transaction->PPA = Convert_address_to_ppa(transaction->Address);
  domain.Update_mapping_info(ideal_mapping_table, transaction->Stream_id, transaction->LPA, transaction->PPA,
                             ((NvmTransactionFlashWR*)transaction)->write_sectors_bitmap | domain.Get_page_status(ideal_mapping_table, transaction->Stream_id, transaction->LPA));
}

force_inline void
Address_Mapping_Unit_Page_Level::allocate_plane_for_translation_write(NvmTransactionFlash* transaction)
{
  allocate_plane_for_user_write((NvmTransactionFlashWR*)transaction);
}

force_inline void
Address_Mapping_Unit_Page_Level::allocate_page_in_plane_for_translation_write(NvmTransactionFlash* transaction, MVPN_type mvpn, bool is_for_gc)
{
  AddressMappingDomain& domain = domains[transaction->Stream_id];

  MPPN_type old_MPPN = domain.GlobalTranslationDirectory[mvpn].MPPN;
  if (old_MPPN == NO_MPPN)  /*this is the first access to the mvpn*/
  {
    if (is_for_gc)
    PRINT_ERROR("Unexpected mapping table status in allocate_page_in_plane_for_translation_write for a GC/WL write!")
  }
  else
  {
    NVM::FlashMemory::Physical_Page_Address prevAddr;
    Convert_ppa_to_address(old_MPPN, prevAddr);
    block_manager->Invalidate_page_in_block(transaction->Stream_id, prevAddr);
  }

  block_manager->Allocate_block_and_page_in_plane_for_translation_write(transaction->Stream_id, transaction->Address, false);
  transaction->PPA = Convert_address_to_ppa(transaction->Address);
  domain.GlobalTranslationDirectory[mvpn].MPPN = (MPPN_type)transaction->PPA;
  domain.GlobalTranslationDirectory[mvpn].TimeStamp = CurrentTimeStamp;
}

force_inline void
Address_Mapping_Unit_Page_Level::allocate_plane_for_preconditioning(stream_id_type stream_id, LPA_type lpn, NVM::FlashMemory::Physical_Page_Address& targetAddress)
{
  domains[stream_id].plane_allocate(targetAddress, lpn);
}

force_inline bool
Address_Mapping_Unit_Page_Level::request_mapping_entry(stream_id_type stream_id, LPA_type lpa)
{
  AddressMappingDomain& domain = domains[stream_id];
  MVPN_type mvpn = get_MVPN(lpa, stream_id);

  /*This is the first time that a user request accesses this address.
  Just create an entry in cache! No flash read is needed.*/
  if (domain.GlobalTranslationDirectory[mvpn].MPPN == NO_MPPN)
  {
    if (!domain.CMT->Check_free_slot_availability())
    {
      LPA_type evicted_lpa;
      CMTSlotType evictedItem = domain.CMT->Evict_one_slot(evicted_lpa);
      if (evictedItem.Dirty)
      {
        /* In order to eliminate possible race conditions for the requests that
        * will access the evicted lpa in the near future (before the translation
        * write finishes), MQSim updates GMT (the on flash mapping table) right
        * after eviction happens.*/
        domain.GlobalMappingTable[evicted_lpa].PPA = evictedItem.PPA;
        domain.GlobalMappingTable[evicted_lpa].WrittenStateBitmap = evictedItem.WrittenStateBitmap;
        if (domain.GlobalMappingTable[evicted_lpa].TimeStamp > CurrentTimeStamp)
          throw std::logic_error("Unexpected situation occurred in handling GMT!");
        domain.GlobalMappingTable[evicted_lpa].TimeStamp = CurrentTimeStamp;
        generate_flash_writeback_request_for_mapping_data(stream_id, evicted_lpa);
      }
    }
    domain.CMT->Reserve_slot_for_lpn(stream_id, lpa);
    domain.CMT->Insert_new_mapping_info(stream_id, lpa, NO_PPA, UNWRITTEN_LOGICAL_PAGE);
    return true;
  }

  /*A read transaction is already under process to retrieve the MVP content.
  * This situation may happen in two different cases:
  * 1. A read has been issued to retrieve unchanged parts of the mapping data and merge them
  *     with the changed parts (i.e., an update read of MVP). This read will be followed
  *     by a writeback of MVP content to a new flash page.
  * 2. A read has been issued to retrieve the mapping data for some previous user requests*/
  if (domain.ArrivingMappingEntries.find(mvpn) != domain.ArrivingMappingEntries.end())
  {
    if (domain.CMT->Is_slot_reserved_for_lpn_and_waiting(stream_id, lpa))
      return false;
    else //An entry should be created in the cache
    {
      if (!domain.CMT->Check_free_slot_availability())
      {
        LPA_type evicted_lpa;
        CMTSlotType evictedItem = domain.CMT->Evict_one_slot(evicted_lpa);
        if (evictedItem.Dirty)
        {
          /* In order to eliminate possible race conditions for the requests that
          * will access the evicted lpa in the near future (before the translation
          * write finishes), MQSim updates GMT (the on flash mapping table) right
          * after eviction happens.*/
          domain.GlobalMappingTable[evicted_lpa].PPA = evictedItem.PPA;
          domain.GlobalMappingTable[evicted_lpa].WrittenStateBitmap = evictedItem.WrittenStateBitmap;
          if (domain.GlobalMappingTable[evicted_lpa].TimeStamp > CurrentTimeStamp)
            throw std::logic_error("Unexpected situation occured in handling GMT!");
          domain.GlobalMappingTable[evicted_lpa].TimeStamp = CurrentTimeStamp;
          generate_flash_writeback_request_for_mapping_data(stream_id, evicted_lpa);
        }
      }
      domain.CMT->Reserve_slot_for_lpn(stream_id, lpa);
      domain.ArrivingMappingEntries.insert(std::pair<MVPN_type, LPA_type>(mvpn, lpa));

      return false;
    }
  }

  /*MQSim assumes that the data of all departing (evicted from CMT) translation pages are in memory, until
  the flash program operation finishes and the entry it is cleared from DepartingMappingEntries.*/
  if (domain.DepartingMappingEntries.find(mvpn) != domain.DepartingMappingEntries.end())
  {
    if (!domain.CMT->Check_free_slot_availability())
    {
      LPA_type evicted_lpa;
      CMTSlotType evictedItem = domain.CMT->Evict_one_slot(evicted_lpa);
      if (evictedItem.Dirty)
      {
        /* In order to eliminate possible race conditions for the requests that
        * will access the evicted lpa in the near future (before the translation
        * write finishes), MQSim updates GMT (the on flash mapping table) right
        * after eviction happens.*/
        domain.GlobalMappingTable[evicted_lpa].PPA = evictedItem.PPA;
        domain.GlobalMappingTable[evicted_lpa].WrittenStateBitmap = evictedItem.WrittenStateBitmap;
        if (domain.GlobalMappingTable[evicted_lpa].TimeStamp > CurrentTimeStamp)
          throw std::logic_error("Unexpected situation occured in handling GMT!");
        domain.GlobalMappingTable[lpa].TimeStamp = CurrentTimeStamp;
        generate_flash_writeback_request_for_mapping_data(stream_id, evicted_lpa);
      }
    }
    domain.CMT->Reserve_slot_for_lpn(stream_id, lpa);
    /*Hack: since we do not actually save the values of translation requests, we copy the mapping
    data from GlobalMappingTable (which actually must be stored on flash)*/
    domain.CMT->Insert_new_mapping_info(stream_id, lpa,
                                         domain.GlobalMappingTable[lpa].PPA, domain.GlobalMappingTable[lpa].WrittenStateBitmap);
    return true;
  }

  //Non of the above options provide mapping data. So, MQSim, must read the translation data from flash memory
  if (!domain.CMT->Check_free_slot_availability())
  {
    LPA_type evicted_lpa;
    CMTSlotType evictedItem = domain.CMT->Evict_one_slot(evicted_lpa);
    if (evictedItem.Dirty)
    {
      /* In order to eliminate possible race conditions for the requests that
      * will access the evicted lpa in the near future (before the translation
      * write finishes), MQSim updates GMT (the on flash mapping table) right
      * after eviction happens.*/
      domain.GlobalMappingTable[evicted_lpa].PPA = evictedItem.PPA;
      domain.GlobalMappingTable[evicted_lpa].WrittenStateBitmap = evictedItem.WrittenStateBitmap;
      if (domain.GlobalMappingTable[evicted_lpa].TimeStamp > CurrentTimeStamp)
        throw std::logic_error("Unexpected situation occured in handling GMT!");
      domain.GlobalMappingTable[evicted_lpa].TimeStamp = CurrentTimeStamp;
      generate_flash_writeback_request_for_mapping_data(stream_id, evicted_lpa);
    }
  }
  domain.CMT->Reserve_slot_for_lpn(stream_id, lpa);
  generate_flash_read_request_for_mapping_data(stream_id, lpa);//consult GTD and create read transaction
  return false;
}

/*
 * This function should be invoked only if the address translation entry exists
 * in CMT. Otherwise, the call to the CMT->Rerieve_ppa, within this function,
 * will throw an exception.
 */
force_inline bool
Address_Mapping_Unit_Page_Level::translate_lpa_to_ppa(stream_id_type streamID, NvmTransactionFlash* transaction)
{
  PPA_type ppa = domains[streamID].Get_ppa(ideal_mapping_table, streamID, transaction->LPA);

  if (transaction->Type == Transaction_Type::READ)
  {
    if (ppa == NO_PPA)
      ppa = online_create_entry_for_reads(transaction->LPA, streamID, transaction->Address, ((NvmTransactionFlashRD*)transaction)->read_sectors_bitmap);
    transaction->PPA = ppa;
    Convert_ppa_to_address(transaction->PPA, transaction->Address);
    block_manager->Read_transaction_issued(transaction->Address);
    transaction->Physical_address_determined = true;
    return true;
  }
  else//This is a write transaction
  {
    allocate_plane_for_user_write((NvmTransactionFlashWR*)transaction);
    if (ftl->GC_and_WL_Unit->Stop_servicing_writes(transaction->Address))//there are too few free pages remaining only for GC
      return false;
    allocate_page_in_plane_for_user_write((NvmTransactionFlashWR*)transaction, false);
    transaction->Physical_address_determined = true;
    return true;
  }
}

force_inline void
Address_Mapping_Unit_Page_Level::generate_flash_read_request_for_mapping_data(stream_id_type stream_id, LPA_type lpn)
{
  MVPN_type mvpn = get_MVPN(lpn, stream_id);

  if (mvpn >= domains[stream_id].Total_translation_pages_no)
  PRINT_ERROR("Out of range virtual translation page number!")

  domains[stream_id].ArrivingMappingEntries.insert(std::pair<MVPN_type, LPA_type>(mvpn, lpn));

  if (is_mvpn_locked_for_gc(stream_id, mvpn))
  {
    manage_mapping_transaction_facing_barrier(stream_id, mvpn, true);
  }
  else
  {
    ftl->preparing_for_transaction_submit();

    PPA_type ppn = domains[stream_id].GlobalTranslationDirectory[mvpn].MPPN;

    if (ppn == NO_MPPN)
    PRINT_ERROR("Reading an invalid physical flash page address in function generate_flash_read_request_for_mapping_data!")

    auto* readTR = _make_mapping_read_tr(stream_id, SECTOR_SIZE_IN_BYTE, mvpn,
                                         ((page_status_type) 0x01) << sector_no_per_page,
                                         ppn);

    block_manager->Read_transaction_issued(readTR->Address);//Inform block_manager as soon as the transaction's target address is determined
    ftl->submit_transaction(readTR);

    __stats.Total_flash_reads_for_mapping++;
    __stats.Total_flash_reads_for_mapping_per_stream[stream_id]++;

    ftl->schedule_transaction();
  }
}

force_inline void
Address_Mapping_Unit_Page_Level::generate_flash_writeback_request_for_mapping_data(stream_id_type stream_id, LPA_type lpn)
{
  MVPN_type mvpn = get_MVPN(lpn, stream_id);

  auto& domain = domains[stream_id];

  if (is_mvpn_locked_for_gc(stream_id, mvpn))
  {
    manage_mapping_transaction_facing_barrier(stream_id, mvpn, false);
    domain.DepartingMappingEntries.insert(get_MVPN(lpn, stream_id));
  }
  else
  {
    ftl->preparing_for_transaction_submit();

    //Writing back all dirty CMT entries that fall into the same translation virtual page (MVPN)
    uint32_t read_size = 0;
    page_status_type readSectorsBitmap = 0;
    LPA_type startLPN = get_start_LPN_in_MVP(mvpn);
    LPA_type endLPN = get_end_LPN_in_MVP(mvpn);
    for (LPA_type lpn_itr = startLPN; lpn_itr <= endLPN; lpn_itr++)
      if (domain.CMT->Exists(stream_id, lpn_itr))
      {
        if (domain.CMT->Is_dirty(stream_id, lpn_itr))
        {
          domain.CMT->Make_clean(stream_id, lpn_itr);
          domain.GlobalMappingTable[lpn_itr].PPA = domain.CMT->Retrieve_ppa(stream_id, lpn_itr);
        }
        else
        {
          page_status_type bitlocation = (((page_status_type)0x1) << (((lpn_itr - startLPN) * GTD_entry_size) / SECTOR_SIZE_IN_BYTE));
          if ((readSectorsBitmap & bitlocation) == 0)
          {
            readSectorsBitmap |= bitlocation;
            read_size += SECTOR_SIZE_IN_BYTE;
          }
        }
      }

    //Read the unchaged mapping entries from flash to merge them with updated parts of MVPN
    NvmTransactionFlashRD* readTR = nullptr;
    MPPN_type mppn = domain.GlobalTranslationDirectory[mvpn].MPPN;
    if (mppn != NO_MPPN)
    {
      readTR = _make_mapping_read_tr(stream_id, read_size, mvpn,
                                     readSectorsBitmap,
                                     mppn, mvpn);

      block_manager->Read_transaction_issued(readTR->Address);//Inform block_manager as soon as the transaction's target address is determined
      domain.ArrivingMappingEntries.insert(std::pair<MVPN_type, LPA_type>(mvpn, lpn));
      ftl->submit_transaction(readTR);
    }

    auto writeTR = _make_mapping_write_tr(stream_id,
                                          sector_no_per_page * SECTOR_SIZE_IN_BYTE,
                                          mvpn,
                                          (((page_status_type)0x1) << sector_no_per_page) - 1,
                                          mppn,
                                          mvpn,
                                          readTR);

    allocate_plane_for_translation_write(writeTR);
    allocate_page_in_plane_for_translation_write(writeTR, mvpn, false);
    domain.DepartingMappingEntries.insert(get_MVPN(lpn, stream_id));
    ftl->submit_transaction(writeTR);

    __stats.Total_flash_reads_for_mapping++;
    __stats.Total_flash_writes_for_mapping++;
    __stats.Total_flash_reads_for_mapping_per_stream[stream_id]++;
    __stats.Total_flash_writes_for_mapping_per_stream[stream_id]++;

    ftl->schedule_transaction();
  }
}

force_inline MVPN_type
Address_Mapping_Unit_Page_Level::get_MVPN(LPA_type lpn, stream_id_type /* stream_id */) const
{
  //return (MVPN_type)((lpn % (domains[stream_id]->Total_logical_pages_no)) / no_of_translation_entries_per_page);
  return MVPN_type(lpn / no_of_translation_entries_per_page);
}

force_inline LPA_type
Address_Mapping_Unit_Page_Level::get_start_LPN_in_MVP(MVPN_type mvpn) const
{
  return MVPN_type(mvpn * no_of_translation_entries_per_page);
}

force_inline LPA_type
Address_Mapping_Unit_Page_Level::get_end_LPN_in_MVP(MVPN_type mvpn) const
{
  return MVPN_type(mvpn * no_of_translation_entries_per_page + no_of_translation_entries_per_page - 1);
}

force_inline std::set<NvmTransactionFlashWR*>&
Address_Mapping_Unit_Page_Level::__wr_transaction_for_overfull_plane(const NVM::FlashMemory::Physical_Page_Address& address)
{
  return Write_transactions_for_overfull_planes[address.ChannelID][address.ChipID][address.DieID][address.PlaneID];
}

force_inline void
Address_Mapping_Unit_Page_Level::manage_unsuccessful_translation(NvmTransactionFlash *transaction)
{
  // Currently, the only unsuccessfull translation would be for program
  // translations that are accessing a plane that is running out of free pages
  __wr_transaction_for_overfull_plane(transaction->Address).insert((NvmTransactionFlashWR*)transaction);
}

void
Address_Mapping_Unit_Page_Level::__ppa_to_address(PPA_type ppn,
                                                  NVM::FlashMemory::Physical_Page_Address& address)
{
  address.ChannelID = (flash_channel_ID_type)(ppn / page_no_per_channel); ppn %= page_no_per_channel;
  address.ChipID    = (flash_chip_ID_type)(ppn / page_no_per_chip);       ppn %= page_no_per_chip;
  address.DieID     = (flash_die_ID_type)(ppn / page_no_per_die);         ppn %= page_no_per_die;
  address.PlaneID   = (flash_plane_ID_type)(ppn / page_no_per_plane);     ppn %= page_no_per_plane;

  address.BlockID   = (flash_block_ID_type)(ppn / pages_no_per_block);
  address.PageID    = (flash_page_ID_type)(ppn % pages_no_per_block);
}

//// service Handler!!!!
template <typename function>
force_inline void
Address_Mapping_Unit_Page_Level::__processing_unmapped_transactions(FlashTransactionLpaMap& unmapped_ios,
                                                                    LPA_type lpa, uint32_t stream_id,
                                                                    function lambda)
{
  auto it2 = unmapped_ios.find(lpa);
  while (it2 != unmapped_ios.end() && (*it2).first == lpa) {
    if (is_lpa_locked_for_gc(stream_id, lpa)) {
      manage_user_transaction_facing_barrier(it2->second);
    } else {
      if (translate_lpa_to_ppa(stream_id, it2->second)) {
        ftl->submit_transaction(it2->second);

        lambda(it2->second);
      } else {
        manage_unsuccessful_translation(it2->second);
      }
    }

    unmapped_ios.erase(it2++);
  }
}

void
Address_Mapping_Unit_Page_Level::__handle_transaction_service_signal(NvmTransactionFlash* transaction)
{
  //First check if the transaction source is Mapping Module
  if (transaction->Source != Transaction_Source_Type::MAPPING)
    return;

  if (ideal_mapping_table)
    throw std::logic_error("There should not be any flash read/write when ideal mapping is enabled!");

  auto  stream_id = transaction->Stream_id;
  auto& domain = domains[stream_id];

  if (transaction->Type == Transaction_Type::WRITE)
  {
    domain.DepartingMappingEntries.erase((MVPN_type)((NvmTransactionFlashWR*)transaction)->Content);
  }
  else
  {
    /*If this is a read for an MVP that is required for merging unchanged mapping enries
    * (stored on flash) with those updated entries that are evicted from CMT*/
    if (((NvmTransactionFlashRD*)transaction)->RelatedWrite != nullptr)
      ((NvmTransactionFlashRD*)transaction)->RelatedWrite->RelatedRead = nullptr;

    ftl->preparing_for_transaction_submit();
    auto  mvpn = (MVPN_type)((NvmTransactionFlashRD*)transaction)->Content;
    auto  it = domain.ArrivingMappingEntries.find(mvpn);

    while (it != domain.ArrivingMappingEntries.end())
    {
      if ((*it).first != mvpn)
        break;

      LPA_type lpa = (*it).second;

      // This mapping entry may arrived due to an update read request that is
      // required for merging new and old mapping entries. If that is the case,
      // we should not insert it into CMT
      if (domain.CMT->Is_slot_reserved_for_lpn_and_waiting(stream_id, lpa))
      {
        domain.CMT->Insert_new_mapping_info(stream_id, lpa,
                                             domain.GlobalMappingTable[lpa].PPA,
                                             domain.GlobalMappingTable[lpa].WrittenStateBitmap);

        __processing_unmapped_transactions(domain.Waiting_unmapped_read_transactions,
                                           lpa, stream_id,
                                           [&](NvmTransactionFlash* transaction) {});

        __processing_unmapped_transactions(domain.Waiting_unmapped_program_transactions,
                                           lpa, stream_id,
                                           [&](NvmTransactionFlash* transaction) {
          auto* wr_transaction = (NvmTransactionFlashWR*)(transaction);

          if (wr_transaction->RelatedRead != nullptr)
            ftl->submit_transaction(wr_transaction->RelatedRead);
        });
      }

      domain.ArrivingMappingEntries.erase(it++);
    }
    ftl->schedule_transaction();
  }
}

// --------------------------
// Over-ridden from SimObject
// --------------------------
void
Address_Mapping_Unit_Page_Level::Start_simulation()
{
  Store_mapping_table_on_flash_at_start();
}

void
Address_Mapping_Unit_Page_Level::Setup_triggers()
{
  Sim_Object::Setup_triggers();

  flash_controller->connect_to_transaction_service_signal(__transaction_service_handler);
}

// ----------------------------------------------------
// Over-ridden from Address_Mapping_Unit_Base on public
// ----------------------------------------------------
void
Address_Mapping_Unit_Page_Level::Allocate_address_for_preconditioning(stream_id_type stream_id, std::map<LPA_type, page_status_type>& lpa_list, std::vector<double>& steady_state_distribution)
{
  std::vector<LPA_type>**** assigned_lpas = new std::vector<LPA_type>***[channel_count];
  for (uint32_t channel_cntr = 0; channel_cntr < channel_count; channel_cntr++)
  {
    assigned_lpas[channel_cntr] = new std::vector<LPA_type>**[chip_no_per_channel];
    for (uint32_t chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
    {
      assigned_lpas[channel_cntr][chip_cntr] = new std::vector<LPA_type>*[die_no_per_chip];
      for (uint32_t die_cntr = 0; die_cntr < die_no_per_chip; die_cntr++)
        assigned_lpas[channel_cntr][chip_cntr][die_cntr] = new std::vector<LPA_type>[plane_no_per_die];
    }
  }

  //First: distribute LPAs to planes
  NVM::FlashMemory::Physical_Page_Address plane_address;
  auto& domain = domains[stream_id];
  for (auto lpa = lpa_list.begin(); lpa != lpa_list.end();)
  {
    if ((*lpa).first >= domain.Total_logical_pages_no)
    PRINT_ERROR("Out of range LPA specified for preconditioning! LPA shoud be smaller than " << domain.Total_logical_pages_no << ", but it is " << (*lpa).first)
    PPA_type ppa = domain.Get_ppa_for_preconditioning(stream_id, (*lpa).first);
    if (ppa != NO_LPA)
    PRINT_ERROR("Calling address allocation for a previously allocated LPA during preconditioning!")
    allocate_plane_for_preconditioning(stream_id, (*lpa).first, plane_address);
    if (LPA_type(Utils::Logical_Address_Partitioning_Unit::Get_share_of_physcial_pages_in_plane(plane_address.ChannelID, plane_address.ChipID, plane_address.DieID, plane_address.PlaneID) * page_no_per_plane)
        > assigned_lpas[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].size())
    {
      assigned_lpas[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].push_back((*lpa).first);
      lpa++;
    }
    else
      lpa_list.erase(lpa++);
  }

  //Second: distribute LPAs within planes based on the steady-state status of blocks
  //uint32_t safe_guard_band = __ftl->GC_and_WL_Unit->Get_minimum_number_of_free_pages_before_GC();
  for (uint32_t channel_cntr = 0; channel_cntr < domain.Channel_no; channel_cntr++)
  {
    for (uint32_t chip_cntr = 0; chip_cntr < domain.Chip_no; chip_cntr++)
    {
      for (uint32_t die_cntr = 0; die_cntr < domain.Die_no; die_cntr++)
      {
        for (uint32_t plane_cntr = 0; plane_cntr < domain.Plane_no; plane_cntr++)
        {
          plane_address.ChannelID = domain.Channel_ids[channel_cntr];
          plane_address.ChipID = domain.Chip_ids[chip_cntr];
          plane_address.DieID = domain.Die_ids[die_cntr];
          plane_address.PlaneID = domain.Plane_ids[plane_cntr];

          uint32_t physical_block_consumption_goal = (uint32_t)(double(block_no_per_plane - ftl->GC_and_WL_Unit->Get_minimum_number_of_free_pages_before_GC() / 2)
                                                                * Utils::Logical_Address_Partitioning_Unit::Get_share_of_physcial_pages_in_plane(plane_address.ChannelID, plane_address.ChipID, plane_address.DieID, plane_address.PlaneID));

          //Adjust the average
          double model_average = 0;
          std::vector<double> adjusted_steady_state_distribution;
          for (uint32_t i = 0; i <= pages_no_per_block; i++)//Check if probability distribution is correct
          {
            model_average += steady_state_distribution[i] * double(i) / double(pages_no_per_block);
            adjusted_steady_state_distribution.push_back(steady_state_distribution[i]);
          }
          double real_average = double(assigned_lpas[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].size()) / (physical_block_consumption_goal * pages_no_per_block);
          if (std::abs(model_average - real_average) * pages_no_per_block > 0.9999)
          {
            int displacement_index = int((real_average - model_average) * pages_no_per_block);
            if (displacement_index > 0)
            {
              for (int i = 0; i < displacement_index; i++)
                adjusted_steady_state_distribution[i] = 0;
              for (int i = displacement_index; i < int(pages_no_per_block); i++)
                adjusted_steady_state_distribution[i] = steady_state_distribution[i - displacement_index];
            }
            else
            {
              displacement_index *= -1;
              for (int i = 0; i < int(pages_no_per_block) - displacement_index; i++)
                adjusted_steady_state_distribution[i] = steady_state_distribution[i + displacement_index];
              for (int i = int(pages_no_per_block) - displacement_index; i < int(pages_no_per_block); i++)
                adjusted_steady_state_distribution[i] = 0;
            }
          }

          //Check if it is possible to find a PPA for each LPA with current proability assignments
          uint32_t total_valid_pages = 0;
          for (int valid_pages_in_block = pages_no_per_block; valid_pages_in_block >= 0; valid_pages_in_block--)
            total_valid_pages += valid_pages_in_block * (uint32_t)(adjusted_steady_state_distribution[valid_pages_in_block] * physical_block_consumption_goal);
          uint32_t pages_need_PPA = 0;//The number of LPAs that remain unassigned due to imperfect probability assignments
          if (total_valid_pages < assigned_lpas[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].size())
            pages_need_PPA = (uint32_t)(assigned_lpas[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].size()) - total_valid_pages;

          uint32_t remaining_blocks_to_consume = physical_block_consumption_goal;
          for (int valid_pages_in_block = pages_no_per_block; valid_pages_in_block >= 0; valid_pages_in_block--)
          {
            uint32_t block_no_with_x_valid_page = (uint32_t)(adjusted_steady_state_distribution[valid_pages_in_block] * physical_block_consumption_goal);
            if (block_no_with_x_valid_page > 0 && pages_need_PPA > 0)
            {
              block_no_with_x_valid_page += (pages_need_PPA / valid_pages_in_block) + (pages_need_PPA % valid_pages_in_block == 0 ? 0 : 1);
              pages_need_PPA = 0;
            }

            if (block_no_with_x_valid_page <= remaining_blocks_to_consume)
            {
              remaining_blocks_to_consume -= block_no_with_x_valid_page;
            }
            else
            {
              block_no_with_x_valid_page = remaining_blocks_to_consume;
              remaining_blocks_to_consume = 0;
            }
            for (uint32_t block_cntr = 0; block_cntr < block_no_with_x_valid_page; block_cntr++)
            {
              //Assign physical addresses
              std::vector<NVM::FlashMemory::Physical_Page_Address> addresses;
              if (assigned_lpas[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].size() < valid_pages_in_block)
                valid_pages_in_block = int(assigned_lpas[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].size());
              for (int page_cntr = 0; page_cntr < valid_pages_in_block; page_cntr++)
              {
                NVM::FlashMemory::Physical_Page_Address addr(plane_address.ChannelID, plane_address.ChipID, plane_address.DieID, plane_address.PlaneID, 0, 0);
                addresses.push_back(addr);
              }
              block_manager->Allocate_Pages_in_block_and_invalidate_remaining_for_preconditioning(stream_id, plane_address, addresses);

              //Update mapping table
              for (auto const &address : addresses)
              {
                LPA_type lpa = assigned_lpas[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].back();
                assigned_lpas[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].pop_back();
                PPA_type ppa = Convert_address_to_ppa(address);
                flash_controller->Change_memory_status_preconditioning(&address, &lpa);
                domain.GlobalMappingTable[lpa].PPA = ppa;
                domain.GlobalMappingTable[lpa].WrittenStateBitmap = (*lpa_list.find(lpa)).second;
                domain.GlobalMappingTable[lpa].TimeStamp = 0;
              }
            }
          }
          if (assigned_lpas[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].size() > 0)
          PRINT_ERROR("It is not possible to assign PPA to all LPAs in Allocate_address_for_preconditioning! It is not safe to continue preconditioning." << assigned_lpas[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID].size())
        }
      }
    }
  }


  for (uint32_t channel_cntr = 0; channel_cntr < channel_count; channel_cntr++)
  {
    for (uint32_t chip_cntr = 0; chip_cntr < chip_no_per_channel; chip_cntr++)
    {
      for (uint32_t die_cntr = 0; die_cntr < die_no_per_chip; die_cntr++)
        delete[] assigned_lpas[channel_cntr][chip_cntr][die_cntr];
      delete[] assigned_lpas[channel_cntr][chip_cntr];
    }
    delete[] assigned_lpas[channel_cntr];
  }
  delete[] assigned_lpas;
}

int
Address_Mapping_Unit_Page_Level::Bring_to_CMT_for_preconditioning(stream_id_type stream_id, LPA_type lpa)
{
  auto& domain = domains[stream_id];

  if (domain.GlobalMappingTable[lpa].PPA == NO_PPA)
    PRINT_ERROR("Touching an unallocated logical address in preconditioning!")

  if (domain.CMT->Exists(stream_id, lpa))
    return domain.No_of_inserted_entries_in_preconditioning;

  if (domain.CMT->Check_free_slot_availability())
  {
    domain.CMT->Reserve_slot_for_lpn(stream_id, lpa);
    domain.CMT->Insert_new_mapping_info(stream_id, lpa,
                                         domain.GlobalMappingTable[lpa].PPA,
                                         domain.GlobalMappingTable[lpa].WrittenStateBitmap);
  }
  else
  {
    LPA_type evicted_lpa;
    domain.CMT->Evict_one_slot(evicted_lpa);
    domain.CMT->Reserve_slot_for_lpn(stream_id, lpa);
    domain.CMT->Insert_new_mapping_info(stream_id, lpa,
                                         domain.GlobalMappingTable[lpa].PPA,
                                         domain.GlobalMappingTable[lpa].WrittenStateBitmap);
  }
  domain.No_of_inserted_entries_in_preconditioning++;
  return domain.No_of_inserted_entries_in_preconditioning;
}

void
Address_Mapping_Unit_Page_Level::Store_mapping_table_on_flash_at_start()
{
  if (mapping_table_stored_on_flash)
    return;

  //Since address translation functions work on flash transactions
  auto* dummy_tr = _make_dummy_write_tr();

  for (uint32_t stream_id = 0; stream_id < no_of_input_streams; stream_id++)
  {
    dummy_tr->Stream_id = stream_id;
    auto& domain = domains[stream_id];

    for (MVPN_type translation_page_id = 0; translation_page_id < domain.Total_translation_pages_no; translation_page_id++)
    {
      dummy_tr->LPA = (LPA_type)translation_page_id;
      allocate_plane_for_translation_write(dummy_tr);
      allocate_page_in_plane_for_translation_write(dummy_tr, (MVPN_type)dummy_tr->LPA, false);
      flash_controller->Change_flash_page_status_for_preconditioning(dummy_tr->Address, dummy_tr->LPA);
    }
  }

  mapping_table_stored_on_flash = true;
}

uint32_t
Address_Mapping_Unit_Page_Level::Get_cmt_capacity()
{
  return cmt_capacity;
}

uint32_t
Address_Mapping_Unit_Page_Level::Get_current_cmt_occupancy_for_stream(stream_id_type stream_id)
{
  return domains[stream_id].No_of_inserted_entries_in_preconditioning;
}

LPA_type
Address_Mapping_Unit_Page_Level::Get_logical_pages_count(stream_id_type stream_id)
{
  return domains[stream_id].Total_logical_pages_no;
}

void
Address_Mapping_Unit_Page_Level::Translate_lpa_to_ppa_and_dispatch(const std::list<NvmTransaction*>& transactions)
{
  for (auto it = transactions.cbegin(); it != transactions.cend(); ) {
    //iterator should be post-incremented since the iterator may be deleted from list
    if (is_lpa_locked_for_gc((*it)->Stream_id, ((NvmTransactionFlash*)(*it))->LPA))
      manage_user_transaction_facing_barrier((NvmTransactionFlash*)*(it++));
    else query_cmt((NvmTransactionFlash*)(*it++));
  }

  if (!transactions.empty()) {
    ftl->preparing_for_transaction_submit();

    for (auto& it : transactions) {
      auto* transaction = (NvmTransactionFlash*)(it);

      if (transaction->Physical_address_determined) {
        ftl->submit_transaction(transaction);

        if (transaction->Type == Transaction_Type::WRITE) {
          auto* wr_transaction = (NvmTransactionFlashWR*)(transaction);

          if (wr_transaction->RelatedRead != nullptr)
            ftl->submit_transaction(wr_transaction->RelatedRead);
        }
      }
    }

    ftl->schedule_transaction();
  }
}

void
Address_Mapping_Unit_Page_Level::Get_data_mapping_info_for_gc(stream_id_type stream_id,
                                                              LPA_type lpa,
                                                              PPA_type& ppa,
                                                              page_status_type& page_state)
{
  auto& domain = domains[stream_id];

  if (domain.Mapping_entry_accessible(ideal_mapping_table, stream_id, lpa)) {
    ppa = domain.Get_ppa(ideal_mapping_table, stream_id, lpa);
    page_state = domain.Get_page_status(ideal_mapping_table, stream_id, lpa);
  } else {
    ppa = domain.GlobalMappingTable[lpa].PPA;
    page_state = domain.GlobalMappingTable[lpa].WrittenStateBitmap;
  }
}

void
Address_Mapping_Unit_Page_Level::Get_translation_mapping_info_for_gc(stream_id_type stream_id,
                                                                     MVPN_type mvpn,
                                                                     MPPN_type& mppa,
                                                                     sim_time_type& timestamp)
{
  auto& domain = domains[stream_id];
  mppa = domain.GlobalTranslationDirectory[mvpn].MPPN;
  timestamp = domain.GlobalTranslationDirectory[mvpn].TimeStamp;
}

void
Address_Mapping_Unit_Page_Level::Allocate_new_page_for_gc(NvmTransactionFlashWR* transaction, bool is_translation_page)
{
  auto& domain = domains[transaction->Stream_id];

  if (is_translation_page)
  {
    MPPN_type mppn = domain.GlobalTranslationDirectory[transaction->LPA].MPPN;
    if (mppn == NO_MPPN)
      PRINT_ERROR("Unexpected situation occured for gc write in Allocate_new_page_for_gc function!")

    allocate_page_in_plane_for_translation_write(transaction, (MVPN_type)transaction->LPA, true);
    transaction->Physical_address_determined = true;
  }
  else
  {
    if (!domain.Mapping_entry_accessible(ideal_mapping_table, transaction->Stream_id, transaction->LPA))
    {
      if (!domain.CMT->Check_free_slot_availability())
      {
        LPA_type evicted_lpa;
        CMTSlotType evictedItem = domain.CMT->Evict_one_slot(evicted_lpa);
        if (evictedItem.Dirty)
        {
          /* In order to eliminate possible race conditions for the requests that
          * will access the evicted lpa in the near future (before the translation
          * write finishes), MQSim updates GMT (the on flash mapping table) right
          * after eviction happens.*/
          domain.GlobalMappingTable[evicted_lpa].PPA = evictedItem.PPA;
          domain.GlobalMappingTable[evicted_lpa].WrittenStateBitmap = evictedItem.WrittenStateBitmap;
          if (domain.GlobalMappingTable[evicted_lpa].TimeStamp > CurrentTimeStamp)
            throw std::logic_error("Unexpected situation occured in handling GMT!");
          domain.GlobalMappingTable[evicted_lpa].TimeStamp = CurrentTimeStamp;
          generate_flash_writeback_request_for_mapping_data(transaction->Stream_id, evicted_lpa);
        }
      }
      domain.CMT->Reserve_slot_for_lpn(transaction->Stream_id, transaction->LPA);
      domain.CMT->Insert_new_mapping_info(transaction->Stream_id, transaction->LPA, Convert_address_to_ppa(transaction->Address), transaction->write_sectors_bitmap);
    }

    allocate_page_in_plane_for_user_write(transaction, true);
    transaction->Physical_address_determined = true;

    //The mapping entry should be updated
    stream_id_type stream_id = transaction->Stream_id;
    __stats.total_CMT_queries++;
    __stats.total_CMT_queries_per_stream[stream_id]++;

    if (domain.Mapping_entry_accessible(ideal_mapping_table, stream_id, transaction->LPA))//either limited or unlimited mapping
    {
      __stats.CMT_hits++;
      __stats.CMT_hits_per_stream[stream_id]++;
      __stats.total_writeTR_CMT_queries++;
      __stats.total_writeTR_CMT_queries_per_stream[stream_id]++;
      __stats.writeTR_CMT_hits++;
      __stats.writeTR_CMT_hits_per_stream[stream_id]++;
      domain.Update_mapping_info(ideal_mapping_table, stream_id, transaction->LPA, transaction->PPA, transaction->write_sectors_bitmap);
    }
    else//the else block only executed for non-ideal mapping table in which CMT has a limited capacity and mapping data is read/written from/to the flash storage
    {
      if (!domain.CMT->Check_free_slot_availability())
      {
        LPA_type evicted_lpa;
        CMTSlotType evictedItem = domain.CMT->Evict_one_slot(evicted_lpa);
        if (evictedItem.Dirty)
        {
          /* In order to eliminate possible race conditions for the requests that
          * will access the evicted lpa in the near future (before the translation
          * write finishes), MQSim updates GMT (the on flash mapping table) right
          * after eviction happens.*/
          domain.GlobalMappingTable[evicted_lpa].PPA = evictedItem.PPA;
          domain.GlobalMappingTable[evicted_lpa].WrittenStateBitmap = evictedItem.WrittenStateBitmap;
          if (domain.GlobalMappingTable[evicted_lpa].TimeStamp > CurrentTimeStamp)
            throw std::logic_error("Unexpected situation occured in handling GMT!");
          domain.GlobalMappingTable[evicted_lpa].TimeStamp = CurrentTimeStamp;
          generate_flash_writeback_request_for_mapping_data(stream_id, evicted_lpa);
        }
      }
      domain.CMT->Reserve_slot_for_lpn(stream_id, transaction->LPA);
      domain.CMT->Insert_new_mapping_info(stream_id, transaction->LPA, transaction->PPA, transaction->write_sectors_bitmap);
    }

  }
}

void
Address_Mapping_Unit_Page_Level::Convert_ppa_to_address(PPA_type ppn,
                                                        NVM::FlashMemory::Physical_Page_Address& address)
{
  __ppa_to_address(ppn, address);
}

NVM::FlashMemory::Physical_Page_Address
Address_Mapping_Unit_Page_Level::Convert_ppa_to_address(const PPA_type ppa)
{
  NVM::FlashMemory::Physical_Page_Address target;

  __ppa_to_address(ppa, target);

  return target;
}

PPA_type
Address_Mapping_Unit_Page_Level::Convert_address_to_ppa(const NVM::FlashMemory::Physical_Page_Address& pageAddress)
{
  return (PPA_type)this->page_no_per_chip * (PPA_type)(pageAddress.ChannelID * this->chip_no_per_channel + pageAddress.ChipID)
           + this->page_no_per_die * pageAddress.DieID
           + this->page_no_per_plane * pageAddress.PlaneID
           + this->pages_no_per_block * pageAddress.BlockID
           + pageAddress.PageID;
}

void
Address_Mapping_Unit_Page_Level::Set_barrier_for_accessing_lpa(stream_id_type stream_id, LPA_type lpa)
{
  auto& domain = domains[stream_id];
  auto itr = domain.Locked_LPAs.find(lpa);
  if (itr != domain.Locked_LPAs.end())
    PRINT_ERROR("Illegal operation: Locking an LPA that has already been locked!");
  domain.Locked_LPAs.insert(lpa);
}

void
Address_Mapping_Unit_Page_Level::Set_barrier_for_accessing_mvpn(stream_id_type stream_id, MVPN_type mvpn)
{
  auto& domain = domains[stream_id];
  auto itr = domain.Locked_MVPNs.find(mvpn);
  if (itr != domain.Locked_MVPNs.end())
    PRINT_ERROR("Illegal operation: Locking an MVPN that has already been locked!");
  domain.Locked_MVPNs.insert(mvpn);
}

void
Address_Mapping_Unit_Page_Level::Set_barrier_for_accessing_physical_block(const NVM::FlashMemory::Physical_Page_Address& block_address)
{
  //The LPAs are actually not known until they are read one-by-one from flash storage. But, to reduce MQSim's complexity, we assume that LPAs are stored in DRAM and thus no read from flash storage is needed.
  Block_Pool_Slot_Type& block = (block_manager->plane_manager[block_address.ChannelID][block_address.ChipID][block_address.DieID][block_address.PlaneID].Blocks[block_address.BlockID]);
  NVM::FlashMemory::Physical_Page_Address addr(block_address);

  auto& domain = domains[block.Stream_id];

  for (flash_page_ID_type pageID = 0; pageID < block.Current_page_write_index; pageID++)
  {
    if (block_manager->Is_page_valid(block, pageID))
    {
      addr.PageID = pageID;
      if (block.Holds_mapping_data)
      {
        auto mpvn = (MVPN_type)flash_controller->Get_metadata(addr.ChannelID, addr.ChipID, addr.DieID, addr.PlaneID, addr.BlockID, addr.PageID);
        if (domain.GlobalTranslationDirectory[mpvn].MPPN != Convert_address_to_ppa(addr))
          PRINT_ERROR("Inconsistency in the global translation directory when locking an MPVN!")
          Set_barrier_for_accessing_mvpn(block.Stream_id, mpvn);
      }
      else
      {
        LPA_type lpa = flash_controller->Get_metadata(addr.ChannelID, addr.ChipID, addr.DieID, addr.PlaneID, addr.BlockID, addr.PageID);
        LPA_type ppa = domain.GlobalMappingTable[lpa].PPA;
        if (domain.CMT->Exists(block.Stream_id, lpa))
          ppa = domain.CMT->Retrieve_ppa(block.Stream_id, lpa);
        if (ppa != Convert_address_to_ppa(addr))
          PRINT_ERROR("Inconsistency in the global mapping table when locking an LPA!")
        Set_barrier_for_accessing_lpa(block.Stream_id, lpa);
      }
    }
  }
}

void
Address_Mapping_Unit_Page_Level::Remove_barrier_for_accessing_lpa(stream_id_type stream_id, LPA_type lpa)
{
  auto& domain = domains[stream_id];
  auto itr = domain.Locked_LPAs.find(lpa);
  if (itr == domain.Locked_LPAs.end())
    PRINT_ERROR("Illegal operation: Unlocking an LPA that has not been locked!");
  domain.Locked_LPAs.erase(itr);

  //If there are read requests waiting behind the barrier, then MQSim assumes they can be serviced with the actual page data that is accessed during GC execution
  auto read_tr = domain.Read_transactions_behind_LPA_barrier.find(lpa);
  while (read_tr != domain.Read_transactions_behind_LPA_barrier.end())
  {
    __handle_transaction_service_signal((*read_tr).second);
    delete (*read_tr).second;
    domain.Read_transactions_behind_LPA_barrier.erase(read_tr);
    read_tr = domain.Read_transactions_behind_LPA_barrier.find(lpa);
  }

  //If there are write requests waiting behind the barrier, then MQSim assumes they can be serviced with the actual page data that is accessed during GC execution. This may not be 100% true for all write requests, but, to avoid more complexity in the simulation, we accept this assumption.
  auto write_tr = domain.Write_transactions_behind_LPA_barrier.find(lpa);
  while (write_tr != domain.Write_transactions_behind_LPA_barrier.end())
  {
    __handle_transaction_service_signal((*write_tr).second);
    delete (*write_tr).second;
    domain.Write_transactions_behind_LPA_barrier.erase(write_tr);
    write_tr = domain.Write_transactions_behind_LPA_barrier.find(lpa);
  }
}

void
Address_Mapping_Unit_Page_Level::Remove_barrier_for_accessing_mvpn(stream_id_type stream_id, MVPN_type mvpn)
{
  auto& domain = domains[stream_id];
  auto itr = domain.Locked_MVPNs.find(mvpn);
  if (itr == domain.Locked_MVPNs.end())
    PRINT_ERROR("Illegal operation: Unlocking an MVPN that has not been locked!");
  domain.Locked_MVPNs.erase(itr);

  //If there are read requests waiting behind the barrier, then MQSim assumes they can be serviced with the actual page data that is accessed during GC execution
  if (domain.MVPN_read_transactions_waiting_behind_barrier.find(mvpn) != domain.MVPN_read_transactions_waiting_behind_barrier.end())
  {
    domain.MVPN_read_transactions_waiting_behind_barrier.erase(mvpn);
    PPA_type ppn = domain.GlobalTranslationDirectory[mvpn].MPPN;
    if (ppn == NO_MPPN)
      PRINT_ERROR("Reading an invalid physical flash page address in function generate_flash_read_request_for_mapping_data!")

    auto* readTR = _make_mapping_read_tr(stream_id, SECTOR_SIZE_IN_BYTE, mvpn,
                                         ((page_status_type)0x1) << sector_no_per_page,
                                         ppn);

    __stats.Total_flash_reads_for_mapping++;
    __stats.Total_flash_reads_for_mapping_per_stream[stream_id]++;

    __handle_transaction_service_signal(readTR);

    delete readTR;
  }

  //If there are write requests waiting behind the barrier, then MQSim assumes they can be serviced with the actual page data that is accessed during GC execution. This may not be 100% true for all write requests, but, to avoid more complexity in the simulation, we accept this assumption.
  if (domain.MVPN_write_transaction_waiting_behind_barrier.find(mvpn) != domain.MVPN_write_transaction_waiting_behind_barrier.end())
  {
    domain.MVPN_write_transaction_waiting_behind_barrier.erase(mvpn);
    //Writing back all dirty CMT entries that fall into the same translation virtual page (MVPN)
    uint32_t read_size = 0;
    page_status_type readSectorsBitmap = 0;
    LPA_type start_lpn = get_start_LPN_in_MVP(mvpn);
    LPA_type end_lpn = get_end_LPN_in_MVP(mvpn);
    for (LPA_type lpn_itr = start_lpn; lpn_itr <= end_lpn; lpn_itr++)
      if (domain.CMT->Exists(stream_id, lpn_itr))
      {
        if (domain.CMT->Is_dirty(stream_id, lpn_itr))
          domain.CMT->Make_clean(stream_id, lpn_itr);
        else
        {
          page_status_type bitlocation = (((page_status_type)0x1) << (((lpn_itr - start_lpn) * GTD_entry_size) / SECTOR_SIZE_IN_BYTE));
          if ((readSectorsBitmap & bitlocation) == 0)
          {
            readSectorsBitmap |= bitlocation;
            read_size += SECTOR_SIZE_IN_BYTE;
          }
        }
      }

    //Read the unchaged mapping entries from flash to merge them with updated parts of MVPN
    MPPN_type mppn = domain.GlobalTranslationDirectory[mvpn].MPPN;

    auto* writeTR = _make_mapping_write_tr(stream_id,
                                           sector_no_per_page,
                                           mvpn,
                                           (((page_status_type)0x1) << sector_no_per_page) - 1,
                                           mppn,
                                           mvpn);

    __stats.Total_flash_reads_for_mapping++;
    __stats.Total_flash_writes_for_mapping++;
    __stats.Total_flash_reads_for_mapping_per_stream[stream_id]++;
    __stats.Total_flash_writes_for_mapping_per_stream[stream_id]++;

    __handle_transaction_service_signal(writeTR);
    delete writeTR;
  }
}

void
Address_Mapping_Unit_Page_Level::Start_servicing_writes_for_overfull_plane(const NVM::FlashMemory::Physical_Page_Address plane_address)
{
  auto& waiting_write_list = __wr_transaction_for_overfull_plane(plane_address);

  ftl->preparing_for_transaction_submit();

  auto program = waiting_write_list.begin();
  while (program != waiting_write_list.end()) {
    if (!translate_lpa_to_ppa((*program)->Stream_id, *program))
      break;

    ftl->submit_transaction(*program);

    if ((*program)->RelatedRead != nullptr)
      ftl->submit_transaction((*program)->RelatedRead);

    waiting_write_list.erase(program++);
  }

  ftl->schedule_transaction();
}

// -------------------------------------------------------
// Over-ridden from Address_Mapping_Unit_Base on protected
// -------------------------------------------------------
bool
Address_Mapping_Unit_Page_Level::query_cmt(NvmTransactionFlash* transaction)
{
  stream_id_type stream_id = transaction->Stream_id;
  __stats.total_CMT_queries++;
  __stats.total_CMT_queries_per_stream[stream_id]++;

  auto& domain = domains[stream_id];

  //Either limited or unlimited CMT
  if (domain.Mapping_entry_accessible(ideal_mapping_table, stream_id, transaction->LPA))
  {
    __stats.CMT_hits_per_stream[stream_id]++;
    __stats.CMT_hits++;
    if (transaction->Type == Transaction_Type::READ)
    {
      __stats.total_readTR_CMT_queries_per_stream[stream_id]++;
      __stats.total_readTR_CMT_queries++;
      __stats.readTR_CMT_hits_per_stream[stream_id]++;
      __stats.readTR_CMT_hits++;
    }
    else//This is a write transaction
    {
      __stats.total_writeTR_CMT_queries++;
      __stats.total_writeTR_CMT_queries_per_stream[stream_id]++;
      __stats.writeTR_CMT_hits++;
      __stats.writeTR_CMT_hits_per_stream[stream_id]++;
    }
    if (translate_lpa_to_ppa(stream_id, transaction))
      return true;
    else
    {
      manage_unsuccessful_translation(transaction);
      return false;
    }
  }
  else//Limited CMT
  {
    if (request_mapping_entry(stream_id, transaction->LPA))//Maybe we can catch mapping data from an on-the-fly write back request
    {
      __stats.CMT_miss++;
      __stats.CMT_miss_per_stream[stream_id]++;
      if (transaction->Type == Transaction_Type::READ)
      {
        __stats.total_readTR_CMT_queries++;
        __stats.total_readTR_CMT_queries_per_stream[stream_id]++;
        __stats.readTR_CMT_miss++;
        __stats.readTR_CMT_miss_per_stream[stream_id]++;
      }
      else//This is a write transaction
      {
        __stats.total_writeTR_CMT_queries++;
        __stats.total_writeTR_CMT_queries_per_stream[stream_id]++;
        __stats.writeTR_CMT_miss++;
        __stats.writeTR_CMT_miss_per_stream[stream_id]++;
      }
      if (translate_lpa_to_ppa(stream_id, transaction))
        return true;
      else
      {
        manage_unsuccessful_translation(transaction);
        return false;
      }
    }
    else
    {
      if (transaction->Type == Transaction_Type::READ)
      {
        __stats.total_readTR_CMT_queries++;
        __stats.total_readTR_CMT_queries_per_stream[stream_id]++;
        __stats.readTR_CMT_miss++;
        __stats.readTR_CMT_miss_per_stream[stream_id]++;
        domain.Waiting_unmapped_read_transactions.insert(std::pair<LPA_type, NvmTransactionFlash*>(transaction->LPA, transaction));
      }
      else//This is a write transaction
      {
        __stats.total_writeTR_CMT_queries++;
        __stats.total_writeTR_CMT_queries_per_stream[stream_id]++;
        __stats.writeTR_CMT_miss++;
        __stats.writeTR_CMT_miss_per_stream[stream_id]++;
        domain.Waiting_unmapped_program_transactions.insert(std::pair<LPA_type, NvmTransactionFlash*>(transaction->LPA, transaction));
      }
    }
    return false;
  }
}

PPA_type
Address_Mapping_Unit_Page_Level::online_create_entry_for_reads(LPA_type lpa, const stream_id_type stream_id, NVM::FlashMemory::Physical_Page_Address& read_address, uint64_t read_sectors_bitmap)
{
  AddressMappingDomain& domain = domains[stream_id];

  domain.plane_allocate(read_address, lpa);

  block_manager->Allocate_block_and_page_in_plane_for_user_write(stream_id, read_address);

  PPA_type ppa = Convert_address_to_ppa(read_address);

  domain.Update_mapping_info(ideal_mapping_table, stream_id, lpa, ppa, read_sectors_bitmap);

  return ppa;
}

void
Address_Mapping_Unit_Page_Level::manage_user_transaction_facing_barrier(NvmTransactionFlash* transaction)
{
  std::pair<LPA_type, NvmTransactionFlash*> entry(transaction->LPA, transaction);
  if (transaction->Type == Transaction_Type::READ)
    domains[transaction->Stream_id].Read_transactions_behind_LPA_barrier.insert(entry);
  else
    domains[transaction->Stream_id].Write_transactions_behind_LPA_barrier.insert(entry);
}

void
Address_Mapping_Unit_Page_Level::manage_mapping_transaction_facing_barrier(stream_id_type stream_id, MVPN_type mvpn, bool read)
{
  if (read)
    domains[stream_id].MVPN_read_transactions_waiting_behind_barrier.insert(mvpn);
  else
    domains[stream_id].MVPN_write_transaction_waiting_behind_barrier.insert(mvpn);
}

bool
Address_Mapping_Unit_Page_Level::is_lpa_locked_for_gc(stream_id_type stream_id, LPA_type lpa)
{
  auto& domain = domains[stream_id];
  return domain.Locked_LPAs.find(lpa) != domain.Locked_LPAs.end();
}

bool
Address_Mapping_Unit_Page_Level::is_mvpn_locked_for_gc(stream_id_type stream_id, MVPN_type mvpn)
{
  auto& domain = domains[stream_id];
  return domain.Locked_MVPNs.find(mvpn) != domain.Locked_MVPNs.end();
}
