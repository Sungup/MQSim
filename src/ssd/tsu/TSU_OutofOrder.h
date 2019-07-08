#ifndef TSU_OUTOFORDER_H
#define TSU_OUTOFORDER_H

#include <list>

#include "../Flash_Transaction_Queue.h"
#include "../NvmTransactionFlash.h"

#include "TSU_Base.h"

namespace SSD_Components
{
  class FTL;

  /*
  * This class implements a transaction scheduling unit which supports:
  * 1. Out-of-order execution of flash transactions, similar to the Sprinkler proposal
  *    described in "Jung et al., Sprinkler: Maximizing resource utilization in many-chip
  *    solid state disks, HPCA, 2014".
  * 2. Program and erase suspension, similar to the proposal described in "G. Wu and X. He,
  *    Reducing SSD read latency via NAND flash program and erase suspension, FAST 2012".
  */
  class TSU_OutOfOrder : public TSU_Base
  {
  private:
    Flash_Transaction_Queue** UserReadTRQueue;
    Flash_Transaction_Queue** UserWriteTRQueue;
    Flash_Transaction_Queue** GCReadTRQueue;
    Flash_Transaction_Queue** GCWriteTRQueue;
    Flash_Transaction_Queue** GCEraseTRQueue;
    Flash_Transaction_Queue** MappingReadTRQueue;
    Flash_Transaction_Queue** MappingWriteTRQueue;

  protected:
    bool service_read_transaction(const NVM::FlashMemory::Flash_Chip& chip) final;
    bool service_write_transaction(const NVM::FlashMemory::Flash_Chip& chip) final;
    bool service_erase_transaction(const NVM::FlashMemory::Flash_Chip& chip) final;

  public:
    TSU_OutOfOrder(const sim_object_id_type& id,
                   FTL& ftl,
                   NVM_PHY_ONFI& NVMController,
                   uint32_t Channel_no,
                   uint32_t chip_no_per_channel,
                   uint32_t DieNoPerChip,
                   uint32_t PlaneNoPerDie,
                   sim_time_type WriteReasonableSuspensionTimeForRead,
                   sim_time_type EraseReasonableSuspensionTimeForRead,
                   sim_time_type EraseReasonableSuspensionTimeForWrite,
                   bool EraseSuspensionEnabled,
                   bool ProgramSuspensionEnabled);

    ~TSU_OutOfOrder() final;
    void Prepare_for_transaction_submit() final;
    void Submit_transaction(NvmTransactionFlash* transaction) final;
    void Schedule() final;

    void Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter) final;

  };
}

#endif // TSU_OUTOFORDER_H
