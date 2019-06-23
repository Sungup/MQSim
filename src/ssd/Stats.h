#ifndef STATS_H
#define STATS_H

#include <array>
#include <vector>

#include "SSD_Defs.h"

namespace SSD_Components
{
  typedef std::vector<uint32_t>                 BlockEraseHisto;
  typedef std::vector<BlockEraseHisto>          BlockEraseHistoOnDie;
  typedef std::vector<BlockEraseHistoOnDie>     BlockEraseHistoOnChip;
  typedef std::vector<BlockEraseHistoOnChip>    BlockEraseHistoOnChannel;
  typedef std::vector<BlockEraseHistoOnChannel> BlockEraseHistoOnSSD;

  class Stats {
  public:
    uint64_t IssuedReadCMD;
    uint64_t IssuedCopybackReadCMD;
    uint64_t IssuedInterleaveReadCMD;
    uint64_t IssuedMultiplaneReadCMD;
    uint64_t IssuedMultiplaneCopybackReadCMD;

    uint64_t IssuedProgramCMD;
    uint64_t IssuedInterleaveProgramCMD;
    uint64_t IssuedMultiplaneProgramCMD;
    uint64_t IssuedInterleaveMultiplaneProgramCMD;
    uint64_t IssuedCopybackProgramCMD;
    uint64_t IssuedMultiplaneCopybackProgramCMD;

    uint64_t IssuedEraseCMD;
    uint64_t IssuedInterleaveEraseCMD;
    uint64_t IssuedMultiplaneEraseCMD;
    uint64_t IssuedInterleaveMultiplaneEraseCMD;

    uint64_t IssuedSuspendProgramCMD;
    uint64_t IssuedSuspendEraseCMD;

    uint64_t Total_flash_reads_for_mapping;
    uint64_t Total_flash_writes_for_mapping;

    std::array<uint64_t, MAX_SUPPORT_STREAMS> Total_flash_reads_for_mapping_per_stream;
    std::array<uint64_t, MAX_SUPPORT_STREAMS> Total_flash_writes_for_mapping_per_stream;

    uint32_t CMT_hits;
    uint32_t readTR_CMT_hits;
    uint32_t writeTR_CMT_hits;

    uint32_t CMT_miss;
    uint32_t readTR_CMT_miss;
    uint32_t writeTR_CMT_miss;

    uint32_t total_CMT_queries;
    uint32_t total_readTR_CMT_queries;
    uint32_t total_writeTR_CMT_queries;
    
    std::array<uint32_t, MAX_SUPPORT_STREAMS> CMT_hits_per_stream;
    std::array<uint32_t, MAX_SUPPORT_STREAMS> readTR_CMT_hits_per_stream;
    std::array<uint32_t, MAX_SUPPORT_STREAMS> writeTR_CMT_hits_per_stream;

    std::array<uint32_t, MAX_SUPPORT_STREAMS> CMT_miss_per_stream;
    std::array<uint32_t, MAX_SUPPORT_STREAMS> readTR_CMT_miss_per_stream;
    std::array<uint32_t, MAX_SUPPORT_STREAMS> writeTR_CMT_miss_per_stream;

    std::array<uint32_t, MAX_SUPPORT_STREAMS> total_CMT_queries_per_stream;
    std::array<uint32_t, MAX_SUPPORT_STREAMS> total_readTR_CMT_queries_per_stream;
    std::array<uint32_t, MAX_SUPPORT_STREAMS> total_writeTR_CMT_queries_per_stream;

    uint32_t Total_gc_executions;
    uint32_t Total_wl_executions;
    uint32_t Total_page_movements_for_gc;
    uint32_t Total_page_movements_for_wl;

    std::array<uint32_t, MAX_SUPPORT_STREAMS> Total_gc_executions_per_stream;
    std::array<uint32_t, MAX_SUPPORT_STREAMS> Total_wl_executions_per_stream;
    std::array<uint32_t, MAX_SUPPORT_STREAMS> Total_gc_page_movements_per_stream;
    std::array<uint32_t, MAX_SUPPORT_STREAMS> Total_wl_page_movements_per_stream;

    BlockEraseHistoOnSSD Block_erase_histogram;

  public:
    Stats(uint32_t channel_no,
          uint32_t chip_no_per_channel,
          uint32_t die_no_per_chip,
          uint32_t plane_no_per_die,
          uint32_t block_no_per_plane,
          uint32_t page_no_per_block,
          uint32_t max_allowed_block_erase_count);
    ~Stats() = default;
  };
}

#endif // !STATS_H
