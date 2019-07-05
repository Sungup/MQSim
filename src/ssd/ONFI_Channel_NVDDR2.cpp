#include "ONFI_Channel_NVDDR2.h"

using namespace SSD_Components;

OnfiNvddr2Spec::OnfiNvddr2Spec(uint32_t ChannelWidth,
                               sim_time_type t_RC,
                               sim_time_type t_DSC,
                               sim_time_type t_DBSY,
                               sim_time_type t_CS,
                               sim_time_type t_RR,
                               sim_time_type t_WB,
                               sim_time_type t_WC,
                               sim_time_type t_ADL,
                               sim_time_type t_CALS,
#if 0 // Currently not in use
                               sim_time_type t_DQSRE,
                               sim_time_type t_RPRE,
                               sim_time_type t_RHW,
                               sim_time_type t_CCS,
#endif
                               sim_time_type t_WPST,
                               sim_time_type t_WPSTH)
#if 0 // Currently not in use
ReadDataOutSetupTime(t_RPRE + t_DQSRE),
    ReadDataOutSetupTime_2Planes(ReadDataOutSetupTime
                                   + t_RHW + 6 * t_WC + t_CCS + t_RPRE + t_DQSRE),
    ReadDataOutSetupTime_3Planes(ReadDataOutSetupTime_2Planes
                                   + t_RHW + 6 * t_WC + t_CCS + t_RPRE + t_DQSRE),
    ReadDataOutSetupTime_4Planes(ReadDataOutSetupTime_3Planes
                                   + t_RHW + 6 * t_WC + t_CCS + t_RPRE + t_DQSRE),
#endif
  : TwoUnitDataOutTime(t_DSC),
    TwoUnitDataInTime(t_RC),
    ProgramSuspendCommandTime(t_CS + t_WC * 3),
    EraseSuspendCommandTime(t_CS + t_WC * 3),
    ReadCommandTime {
      0,
      t_CS + 6 * t_WC + (t_DBSY + 6 * t_WC) * 0 + t_WB + t_RR,
      t_CS + 6 * t_WC + (t_DBSY + 6 * t_WC) * 1 + t_WB + t_RR,
      t_CS + 6 * t_WC + (t_DBSY + 6 * t_WC) * 2 + t_WB + t_RR,
      t_CS + 6 * t_WC + (t_DBSY + 6 * t_WC) * 3 + t_WB + t_RR
    },
    ProgramCommandTime {
      0,
      t_CS + ((t_WC * 5 + t_ADL + t_WPST + t_CALS + t_WB) + t_DBSY) * 0 + (t_WC * 6 + t_ADL + t_WPST + t_WPSTH + t_WB),
      t_CS + ((t_WC * 5 + t_ADL + t_WPST + t_CALS + t_WB) + t_DBSY) * 1 + (t_WC * 6 + t_ADL + t_WPST + t_WPSTH + t_WB),
      t_CS + ((t_WC * 5 + t_ADL + t_WPST + t_CALS + t_WB) + t_DBSY) * 2 + (t_WC * 6 + t_ADL + t_WPST + t_WPSTH + t_WB),
      t_CS + ((t_WC * 5 + t_ADL + t_WPST + t_CALS + t_WB) + t_DBSY) * 3 + (t_WC * 6 + t_ADL + t_WPST + t_WPSTH + t_WB)
    },
    EraseCommandTime {
      0,
      t_CS + t_WC * 4 + t_WB + (t_DBSY + t_WC * 4 + t_WB) * 0,
      t_CS + t_WC * 4 + t_WB + (t_DBSY + t_WC * 4 + t_WB) * 1,
      t_CS + t_WC * 4 + t_WB + (t_DBSY + t_WC * 4 + t_WB) * 2,
      t_CS + t_WC * 4 + t_WB + (t_DBSY + t_WC * 4 + t_WB) * 3
    },
    ChannelWidth(ChannelWidth)
{ }
