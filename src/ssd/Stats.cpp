#include "Stats.h"

#include "../utils/InlineTools.h"

using namespace SSD_Components;

force_inline BlockEraseHisto
__make_block_erase_histo(uint32_t blocks_per_plane,
                         uint32_t pages_per_block,
                         uint32_t max_allowed_erase_count)
{
  BlockEraseHisto histo(max_allowed_erase_count, 0);
  histo[0] =  blocks_per_plane * pages_per_block;

  return histo;
}

Stats::Stats(uint32_t channel_no,
             uint32_t chip_no_per_channel,
             uint32_t die_no_per_chip,
             uint32_t plane_no_per_die,
             uint32_t block_no_per_plane,
             uint32_t page_no_per_block,
             uint32_t max_allowed_block_erase_count)
  : IssuedReadCMD(0),
    IssuedCopybackReadCMD(0),
    IssuedInterleaveReadCMD(0),
    IssuedMultiplaneReadCMD(0),
    IssuedMultiplaneCopybackReadCMD(0),
    IssuedProgramCMD(0),
    IssuedInterleaveProgramCMD(0),
    IssuedMultiplaneProgramCMD(0),
    IssuedInterleaveMultiplaneProgramCMD(0),
    IssuedCopybackProgramCMD(0),
    IssuedMultiplaneCopybackProgramCMD(0),
    IssuedEraseCMD(0),
    IssuedInterleaveEraseCMD(0),
    IssuedMultiplaneEraseCMD(0),
    IssuedInterleaveMultiplaneEraseCMD(0),
    IssuedSuspendProgramCMD(0),
    IssuedSuspendEraseCMD(0),
    Total_flash_reads_for_mapping(0),
    Total_flash_writes_for_mapping(0),
    Total_flash_reads_for_mapping_per_stream { 0, },
    Total_flash_writes_for_mapping_per_stream { 0, },
    CMT_hits(0),
    readTR_CMT_hits(0),
    writeTR_CMT_hits(0),
    CMT_miss(0),
    readTR_CMT_miss(0),
    writeTR_CMT_miss(0),
    total_CMT_queries(0),
    total_readTR_CMT_queries(0),
    total_writeTR_CMT_queries(0),
    CMT_hits_per_stream { 0, },
    readTR_CMT_hits_per_stream { 0, },
    writeTR_CMT_hits_per_stream { 0, },
    CMT_miss_per_stream { 0, },
    readTR_CMT_miss_per_stream { 0, },
    writeTR_CMT_miss_per_stream { 0, },
    total_CMT_queries_per_stream { 0, },
    total_readTR_CMT_queries_per_stream { 0, },
    total_writeTR_CMT_queries_per_stream { 0, },
    Total_gc_executions(0),
    Total_wl_executions(0),
    Total_page_movements_for_gc(0),
    Total_page_movements_for_wl(0),
    Total_gc_executions_per_stream { 0, },
    Total_wl_executions_per_stream { 0, },
    Total_gc_page_movements_per_stream { 0, },
    Total_wl_page_movements_per_stream { 0, },
    Block_erase_histogram(
      channel_no,
      BlockEraseHistoOnChannel(
        chip_no_per_channel,
        BlockEraseHistoOnChip(
          die_no_per_chip,
          BlockEraseHistoOnDie(
            plane_no_per_die,
            __make_block_erase_histo(block_no_per_plane,
                                     page_no_per_block,
                                     max_allowed_block_erase_count)
          )
        )
      )
    )
{ }
