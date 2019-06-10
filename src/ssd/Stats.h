#ifndef STATS_H
#define STATS_H

#include "SSD_Defs.h"

namespace SSD_Components
{
  class Stats
  {
  public:
    static void Init_stats(uint32_t channel_no, uint32_t chip_no_per_channel, uint32_t die_no_per_chip, uint32_t plane_no_per_die, uint32_t block_no_per_plane, uint32_t page_no_per_block, uint32_t max_allowed_block_erase_count);
    static void Clear_stats(uint32_t channel_no, uint32_t chip_no_per_channel, uint32_t die_no_per_chip, uint32_t plane_no_per_die, uint32_t block_no_per_plane, uint32_t page_no_per_block, uint32_t max_allowed_block_erase_count);
    static unsigned long IssuedReadCMD, IssuedCopybackReadCMD, IssuedInterleaveReadCMD, IssuedMultiplaneReadCMD, IssuedMultiplaneCopybackReadCMD;
    static unsigned long IssuedProgramCMD, IssuedInterleaveProgramCMD, IssuedMultiplaneProgramCMD, IssuedInterleaveMultiplaneProgramCMD, IssuedCopybackProgramCMD, IssuedMultiplaneCopybackProgramCMD;
    static unsigned long IssuedEraseCMD, IssuedInterleaveEraseCMD, IssuedMultiplaneEraseCMD, IssuedInterleaveMultiplaneEraseCMD;

    static unsigned long IssuedSuspendProgramCMD, IssuedSuspendEraseCMD;

    static unsigned long Total_flash_reads_for_mapping, Total_flash_writes_for_mapping;
    static unsigned long Total_flash_reads_for_mapping_per_stream[MAX_SUPPORT_STREAMS], Total_flash_writes_for_mapping_per_stream[MAX_SUPPORT_STREAMS];

    static uint32_t CMT_hits, readTR_CMT_hits, writeTR_CMT_hits;
    static uint32_t CMT_miss, readTR_CMT_miss, writeTR_CMT_miss;
    static uint32_t total_CMT_queries, total_readTR_CMT_queries, total_writeTR_CMT_queries;
    
    static uint32_t CMT_hits_per_stream[MAX_SUPPORT_STREAMS], readTR_CMT_hits_per_stream[MAX_SUPPORT_STREAMS], writeTR_CMT_hits_per_stream[MAX_SUPPORT_STREAMS];
    static uint32_t CMT_miss_per_stream[MAX_SUPPORT_STREAMS], readTR_CMT_miss_per_stream[MAX_SUPPORT_STREAMS], writeTR_CMT_miss_per_stream[MAX_SUPPORT_STREAMS];
    static uint32_t total_CMT_queries_per_stream[MAX_SUPPORT_STREAMS], total_readTR_CMT_queries_per_stream[MAX_SUPPORT_STREAMS], total_writeTR_CMT_queries_per_stream[MAX_SUPPORT_STREAMS];
    

    static uint32_t Total_gc_executions, Total_gc_executions_per_stream[MAX_SUPPORT_STREAMS];
    static uint32_t Total_page_movements_for_gc, Total_gc_page_movements_per_stream[MAX_SUPPORT_STREAMS];

    static uint32_t Total_wl_executions, Total_wl_executions_per_stream[MAX_SUPPORT_STREAMS];
    static uint32_t Total_page_movements_for_wl, Total_wl_page_movements_per_stream[MAX_SUPPORT_STREAMS];

    static uint32_t***** Block_erase_histogram;
  };
}

#endif // !STATS_H
