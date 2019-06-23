#include "GC_and_WL_Unit_Base.h"
#include "../mapping/AddressMappingUnitDefs.h"

using namespace SSD_Components;

GC_and_WL_Unit_Base::GC_and_WL_Unit_Base(const sim_object_id_type& id,
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
                                         int seed)
  : Sim_Object(id),
    address_mapping_unit(address_mapping_unit),
    block_manager(block_manager),
    tsu(tsu),
    flash_controller(flash_controller),
    __stats(stats),
    force_gc(false),
    block_selection_policy(block_selection_policy),
    gc_threshold(gc_threshold),
    use_copyback(use_copyback),
    preemptible_gc_enabled(preemptible_gc_enabled),
    gc_hard_threshold(gc_hard_threshold),
    random_generator(seed),
    max_ongoing_gc_reqs_per_plane(max_ongoing_gc_reqs_per_plane),
    channel_count(channel_count),
    chip_no_per_channel(chip_no_per_channel),
    die_no_per_chip(die_no_per_chip),
    plane_no_per_die(plane_no_per_die),
    block_no_per_plane(block_no_per_plane),
    pages_no_per_block(page_no_per_block),
    sector_no_per_page(sector_no_per_page),
    dynamic_wearleveling_enabled(dynamic_wearleveling_enabled),
    static_wearleveling_enabled(static_wearleveling_enabled),
    static_wearleveling_threshold(static_wearleveling_threshold),
    __transaction_service_handler(this, &GC_and_WL_Unit_Base::__handle_transaction_service)
{
  block_pool_gc_threshold = (uint32_t)(gc_threshold * (double)block_no_per_plane);
  if (block_pool_gc_threshold < 1)
    block_pool_gc_threshold = 1;
  block_pool_gc_hard_threshold = (uint32_t)(gc_hard_threshold * (double)block_no_per_plane);
  if (block_pool_gc_hard_threshold < 1)
    block_pool_gc_hard_threshold = 1;
  random_pp_threshold = (uint32_t)(rho * pages_no_per_block);
  if (block_pool_gc_threshold < max_ongoing_gc_reqs_per_plane)
    block_pool_gc_threshold = max_ongoing_gc_reqs_per_plane;
}

// TODO Make following 3 functions to inline
bool
GC_and_WL_Unit_Base::is_safe_gc_wl_candidate(const PlaneBookKeepingType* plane_record,
                                             const flash_block_ID_type gc_wl_candidate_block_id)
{
  //The block shouldn't be a current write frontier
  for (uint32_t stream_id = 0; stream_id < address_mapping_unit->Get_no_of_input_streams(); stream_id++)
    if ((&plane_record->Blocks[gc_wl_candidate_block_id]) == plane_record->Data_wf[stream_id]
        || (&plane_record->Blocks[gc_wl_candidate_block_id]) == plane_record->Translation_wf[stream_id]
        || (&plane_record->Blocks[gc_wl_candidate_block_id]) == plane_record->GC_wf[stream_id])
      return false;

  //The block shouldn't have an ongoing program request (all pages must already be written)
  if (plane_record->Blocks[gc_wl_candidate_block_id].Ongoing_user_program_count > 0)
    return false;

  return !(plane_record->Blocks[gc_wl_candidate_block_id].Has_ongoing_gc_wl);
}

force_inline bool
GC_and_WL_Unit_Base::check_static_wl_required(const NVM::FlashMemory::Physical_Page_Address& plane_address)
{
  return static_wearleveling_enabled &&
         (block_manager->Get_min_max_erase_difference(plane_address) >= static_wearleveling_threshold);
}

void
GC_and_WL_Unit_Base::run_static_wearleveling(const NVM::FlashMemory::Physical_Page_Address& plane_address)
{
  PlaneBookKeepingType* pbke = block_manager->Get_plane_bookkeeping_entry(plane_address);
  flash_block_ID_type wl_candidate_block_id = block_manager->Get_coldest_block_id(plane_address);
  if (!is_safe_gc_wl_candidate(pbke, wl_candidate_block_id))
    return;

  NVM::FlashMemory::Physical_Page_Address wl_candidate_address(plane_address);
  wl_candidate_address.BlockID = wl_candidate_block_id;
  Block_Pool_Slot_Type* block = &pbke->Blocks[wl_candidate_block_id];

  //Run the state machine to protect against race condition
  block_manager->GC_WL_started(wl_candidate_block_id);
  pbke->Ongoing_erase_operations.insert(wl_candidate_block_id);
  address_mapping_unit->Set_barrier_for_accessing_physical_block(wl_candidate_address);//Lock the block, so no user request can intervene while the GC is progressing
  if (block_manager->Can_execute_gc_wl(wl_candidate_address))//If there are ongoing requests targeting the candidate block, the gc execution should be postponed
  {
    __stats.Total_wl_executions++;
    tsu->Prepare_for_transaction_submit();

    NVM_Transaction_Flash_ER* wl_erase_tr = new NVM_Transaction_Flash_ER(Transaction_Source_Type::GC_WL, pbke->Blocks[wl_candidate_block_id].Stream_id, wl_candidate_address);
    if (block->Current_page_write_index - block->Invalid_page_count > 0)//If there are some valid pages in block, then prepare flash transactions for page movement
    {
      NVM_Transaction_Flash_RD* wl_read = nullptr;
      NVM_Transaction_Flash_WR* wl_write = nullptr;
      for (flash_page_ID_type pageID = 0; pageID < block->Current_page_write_index; pageID++)
      {
        if (block_manager->Is_page_valid(block, pageID))
        {
          __stats.Total_page_movements_for_gc;
          wl_candidate_address.PageID = pageID;
          if (use_copyback)
          {
            wl_write = new NVM_Transaction_Flash_WR(Transaction_Source_Type::GC_WL, block->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
                                                    NO_LPA, address_mapping_unit->Convert_address_to_ppa(wl_candidate_address), nullptr, 0, nullptr, 0, INVALID_TIME_STAMP);
            wl_write->ExecutionMode = WriteExecutionModeType::COPYBACK;
            tsu->Submit_transaction(wl_write);
          }
          else
          {
            wl_read = new NVM_Transaction_Flash_RD(Transaction_Source_Type::GC_WL, block->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
                                                   NO_LPA, address_mapping_unit->Convert_address_to_ppa(wl_candidate_address), wl_candidate_address, nullptr, 0, nullptr, 0, INVALID_TIME_STAMP);
            wl_write = new NVM_Transaction_Flash_WR(Transaction_Source_Type::GC_WL, block->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
                                                    NO_LPA, NO_PPA, wl_candidate_address, nullptr, 0, wl_read, 0, INVALID_TIME_STAMP);
            wl_write->ExecutionMode = WriteExecutionModeType::SIMPLE;
            wl_write->RelatedErase = wl_erase_tr;
            wl_read->RelatedWrite = wl_write;
            tsu->Submit_transaction(wl_read);//Only the read transaction would be submitted. The Write transaction is submitted when the read transaction is finished and the LPA of the target page is determined
          }
          wl_erase_tr->Page_movement_activities.push_back(wl_write);
        }
      }
    }
    block->Erase_transaction = wl_erase_tr;
    tsu->Submit_transaction(wl_erase_tr);

    tsu->Schedule();
  }
}

void
GC_and_WL_Unit_Base::__handle_transaction_service(NVM_Transaction_Flash* transaction)
{
  PlaneBookKeepingType* pbke = &(block_manager->plane_manager[transaction->Address.ChannelID][transaction->Address.ChipID][transaction->Address.DieID][transaction->Address.PlaneID]);

  switch (transaction->Source)
  {
  case Transaction_Source_Type::USERIO:
  case Transaction_Source_Type::MAPPING:
  case Transaction_Source_Type::CACHE:
    switch (transaction->Type)
    {
    case Transaction_Type::READ:
      block_manager->Read_transaction_serviced(transaction->Address);
      break;
    case Transaction_Type::WRITE:
      block_manager->Program_transaction_serviced(transaction->Address);
      break;
    default:
      PRINT_ERROR("Unexpected situation in the GC_and_WL_Unit_Base function!")
    }
    if (block_manager->Block_has_ongoing_gc_wl(transaction->Address))
      if (block_manager->Can_execute_gc_wl(transaction->Address))
      {
        NVM::FlashMemory::Physical_Page_Address gc_wl_candidate_address(transaction->Address);
        Block_Pool_Slot_Type* block = &pbke->Blocks[transaction->Address.BlockID];
        __stats.Total_gc_executions++;
        tsu->Prepare_for_transaction_submit();
        auto gc_wl_erase_tr = new NVM_Transaction_Flash_ER(Transaction_Source_Type::GC_WL, block->Stream_id, gc_wl_candidate_address);
        if (block->Current_page_write_index - block->Invalid_page_count > 0)//If there are some valid pages in block, then prepare flash transactions for page movement
        {
          //address_mapping_unit->Lock_physical_block_for_gc(gc_candidate_address);//Lock the block, so no user request can intervene while the GC is progressing
          NVM_Transaction_Flash_RD* gc_wl_read = nullptr;
          NVM_Transaction_Flash_WR* gc_wl_write = nullptr;
          for (flash_page_ID_type pageID = 0; pageID < block->Current_page_write_index; pageID++)
          {
            if (block_manager->Is_page_valid(block, pageID))
            {
              gc_wl_candidate_address.PageID = pageID;
              if (use_copyback)
              {
                gc_wl_write = new NVM_Transaction_Flash_WR(Transaction_Source_Type::GC_WL, block->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
                  NO_LPA, address_mapping_unit->Convert_address_to_ppa(gc_wl_candidate_address), nullptr, 0, nullptr, 0, INVALID_TIME_STAMP);
                gc_wl_write->ExecutionMode = WriteExecutionModeType::COPYBACK;
                tsu->Submit_transaction(gc_wl_write);
              }
              else
              {
                gc_wl_read = new NVM_Transaction_Flash_RD(Transaction_Source_Type::GC_WL, block->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
                  NO_LPA, address_mapping_unit->Convert_address_to_ppa(gc_wl_candidate_address), gc_wl_candidate_address, nullptr, 0, nullptr, 0, INVALID_TIME_STAMP);
                gc_wl_write = new NVM_Transaction_Flash_WR(Transaction_Source_Type::GC_WL, block->Stream_id, sector_no_per_page * SECTOR_SIZE_IN_BYTE,
                  NO_LPA, NO_PPA, gc_wl_candidate_address, nullptr, 0, gc_wl_read, 0, INVALID_TIME_STAMP);
                gc_wl_write->ExecutionMode = WriteExecutionModeType::SIMPLE;
                gc_wl_write->RelatedErase = gc_wl_erase_tr;
                gc_wl_read->RelatedWrite = gc_wl_write;
                tsu->Submit_transaction(gc_wl_read);//Only the read transaction would be submitted. The Write transaction is submitted when the read transaction is finished and the LPA of the target page is determined
              }
              gc_wl_erase_tr->Page_movement_activities.push_back(gc_wl_write);
            }
          }
        }
        block->Erase_transaction = gc_wl_erase_tr;
        tsu->Schedule();
      }
    return;

  default:
    break;
  }

  switch (transaction->Type)
  {
  case Transaction_Type::READ:
  {
    PPA_type ppa;
    MPPN_type mppa;
    page_status_type page_status_bitmap;
    if (pbke->Blocks[transaction->Address.BlockID].Holds_mapping_data)
    {
      address_mapping_unit->Get_translation_mapping_info_for_gc(transaction->Stream_id, (MVPN_type)transaction->LPA, mppa, page_status_bitmap);
      if (mppa == transaction->PPA)//There has been no write on the page since GC start, and it is still valid
      {
        tsu->Prepare_for_transaction_submit();
        ((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->write_sectors_bitmap = FULL_PROGRAMMED_PAGE;
        ((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->LPA = transaction->LPA;
        ((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->RelatedRead = nullptr;
        address_mapping_unit->Allocate_new_page_for_gc(((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite, pbke->Blocks[transaction->Address.BlockID].Holds_mapping_data);
        tsu->Submit_transaction(((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite);
        tsu->Schedule();
      }
      else
        PRINT_ERROR("Inconsistency found when moving a page for GC/WL!")
    }
    else
    {
      address_mapping_unit->Get_data_mapping_info_for_gc(transaction->Stream_id, transaction->LPA, ppa, page_status_bitmap);
      if (ppa == transaction->PPA)//There has been no write on the page since GC start, and it is still valid
      {
        tsu->Prepare_for_transaction_submit();
        ((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->write_sectors_bitmap = page_status_bitmap;
        ((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->LPA = transaction->LPA;
        ((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite->RelatedRead = nullptr;
        address_mapping_unit->Allocate_new_page_for_gc(((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite, pbke->Blocks[transaction->Address.BlockID].Holds_mapping_data);
        tsu->Submit_transaction(((NVM_Transaction_Flash_RD*)transaction)->RelatedWrite);
        tsu->Schedule();
      }
      else
        PRINT_ERROR("Inconsistency found when moving a page for GC/WL!")
    }
    break;
  }
  case Transaction_Type::WRITE:
    if (pbke->Blocks[((NVM_Transaction_Flash_WR*)transaction)->RelatedErase->Address.BlockID].Holds_mapping_data)
    {
      address_mapping_unit->Remove_barrier_for_accessing_mvpn(transaction->Stream_id, (MVPN_type)transaction->LPA);
      DEBUG(Simulator->Time() << ": MVPN=" << (MVPN_type)transaction->LPA << " unlocked!!");
    }
    else
    {
      address_mapping_unit->Remove_barrier_for_accessing_lpa(transaction->Stream_id, transaction->LPA);
      DEBUG(Simulator->Time() << ": LPA=" << (MVPN_type)transaction->LPA << " unlocked!!");
    }
    pbke->Blocks[((NVM_Transaction_Flash_WR*)transaction)->RelatedErase->Address.BlockID].Erase_transaction->Page_movement_activities.remove((NVM_Transaction_Flash_WR*)transaction);
    break;
  case Transaction_Type::ERASE:
    pbke->Ongoing_erase_operations.erase(pbke->Ongoing_erase_operations.find(transaction->Address.BlockID));
    block_manager->Add_erased_block_to_pool(transaction->Address);
    block_manager->GC_WL_finished(transaction->Address);
    if (check_static_wl_required(transaction->Address))
      run_static_wearleveling(transaction->Address);

    // Must be inovked after above statements since it may lead to flash page
    // consumption for waiting program transactions
    address_mapping_unit->Start_servicing_writes_for_overfull_plane(transaction->Address);

    if (Stop_servicing_writes(transaction->Address))
      Check_gc_required(pbke->Get_free_block_pool_size(), transaction->Address);
    break;

  default:
    break;
  }
}

void
GC_and_WL_Unit_Base::Setup_triggers()
{
  Sim_Object::Setup_triggers();

  flash_controller->connect_to_transaction_service_signal(__transaction_service_handler);
}

bool
GC_and_WL_Unit_Base::Stop_servicing_writes(const NVM::FlashMemory::Physical_Page_Address& plane_address) const
{
#if 0
  // Unused variable
  PlaneBookKeepingType* pbke = &(block_manager->plane_manager[plane_address.ChannelID][plane_address.ChipID][plane_address.DieID][plane_address.PlaneID]);
#endif

  return block_manager->Get_pool_size(plane_address) < max_ongoing_gc_reqs_per_plane;
}
