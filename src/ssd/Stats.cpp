#include "Stats.h"

#include "../utils/InlineTools.h"

using namespace SSD_Components;

#ifdef GATHER_BLOCK_ERASE_HISTO
force_inline BlockEraseHisto
__make_block_erase_histo(const FlashParameterSet& params)
{
  BlockEraseHisto histo(params.Block_PE_Cycles_Limit, 0);
  histo[0] =  params.Block_No_Per_Plane * params.Page_No_Per_Block;

  return histo;
}
#endif

Stats::Stats(const DeviceParameterSet& params)
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
#ifdef GATHER_BLOCK_ERASE_HISTO
    Block_erase_histogram(
      params.Flash_Channel_Count,
      BlockEraseHistoOnChannel(
        params.Chip_No_Per_Channel,
        BlockEraseHistoOnChip(
          params.Flash_Parameters.Die_No_Per_Chip,
          BlockEraseHistoOnDie(
            params.Flash_Parameters.Plane_No_Per_Die,
            __make_block_erase_histo(params.Flash_Parameters)
          )
        )
      )
    ),
#endif
    Total_gc_executions_per_stream { 0, },
    Total_wl_executions_per_stream { 0, },
    Total_gc_page_movements_per_stream { 0, },
    Total_wl_page_movements_per_stream { 0, }
{ }
