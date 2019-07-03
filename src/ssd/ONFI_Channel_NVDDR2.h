#ifndef ONFI_CHANNEL_NVDDR2_H
#define ONFI_CHANNEL_NVDDR2_H

#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "ONFI_Channel_Base.h"


#define NVDDR2DataInTransferTime(X,Y) ((X / Y->ChannelWidth / 2) * Y->TwoUnitDataInTime)
#define NVDDR2DataOutTransferTime(X,Y) ((X / Y->ChannelWidth / 2) * Y->TwoUnitDataOutTime)


namespace SSD_Components
{
  class ONFI_Channel_NVDDR2 : public ONFI_Channel_Base {
  public:
#if 0 // Currently not in use
    const sim_time_type ReadDataOutSetupTime;
    const sim_time_type ReadDataOutSetupTime_2Planes;
    const sim_time_type ReadDataOutSetupTime_3Planes;
    const sim_time_type ReadDataOutSetupTime_4Planes;
#endif

    //The DDR delay for two-unit device data out/in
    const sim_time_type TwoUnitDataOutTime;
    const sim_time_type TwoUnitDataInTime;

    const sim_time_type ProgramSuspendCommandTime;
    const sim_time_type EraseSuspendCommandTime;

    // Read/Program/Erase command transfer time for different number of planes.
    const sim_time_type ReadCommandTime[5];
    const sim_time_type ProgramCommandTime[5];
    const sim_time_type EraseCommandTime[5];

    const uint32_t ChannelWidth; //channel width in bytes

    /**
     * ONFI_CHANNEL_NVDDR2 constructor
     *
     * Arguments
     *   Data input/ouput timing parameters related to bus frequency
     *    - t_RC: Average RE cycle time, e.g. 6ns
     *    - t_DSC: Average DQS cycle time, e.g. 6ns
     *
     *   Flash timing parameters.
     *    - t_DBSY: Dummy busy time, e.g. 500ns
     *
     *   ONFI NVDDR2 command and address protocol timing parameters
     *     - t_CS: CE setup, e.g. 20ns
     *     - t_RR: Ready to data output, e.g. 20ns
     *     - t_WB: CLK HIGH to R/B LOW, e.g. 100ns
     *     - t_WC: WE cycle time, e.g. 25ns
     *     - t_ADL: ALE to data loading time, e.g. 70ns
     *     - t_CALS: ALE, CLE setup with ODT disabled, e.g. 15
     *
     *   ONFI NVDDR2 data output protocol timing paramters
     *     - t_DQSRE: Access window of DQS from RE, e.g. 15ns
     *     - t_RPRE: Read preamble, e.g. 15ns
     *     - t_RHW: Data output to command, address, or data input, 100ns
     *     - t_CCS: Change column setup time to data in/out or next command, 300ns
     *
     *   ONFI NVDDR2 data input protocol timing paramters
     *     - t_WPST: DQS write postamble, e.g. 6ns
     *     - t_WPSTH: DQS write postamble hold time, e.g. 15ns
     */
    ONFI_Channel_NVDDR2(flash_channel_ID_type channelID,
                        NVM::FlashMemory::Flash_Chip** flashChips,
                        uint32_t ChannelWidth,
                        sim_time_type t_RC = 6,
                        sim_time_type t_DSC = 6,
                        sim_time_type t_DBSY = 500,
                        sim_time_type t_CS = 20,
                        sim_time_type t_RR = 20,
                        sim_time_type t_WB = 100,
                        sim_time_type t_WC = 25,
                        sim_time_type t_ADL = 70,
                        sim_time_type t_CALS = 15,
#if 0 // Currently not in use
                        sim_time_type t_DQSRE = 15,
                        sim_time_type t_RPRE = 15,
                        sim_time_type t_RHW = 100,
                        sim_time_type t_CCS = 300,
#endif
                        sim_time_type t_WPST = 6,
                        sim_time_type t_WPSTH = 15);

  };
}

#endif // !ONFI_CHANNEL_NVDDR2_H
