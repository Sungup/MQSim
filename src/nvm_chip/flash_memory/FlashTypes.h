#ifndef FLASH_TYPES_H
#define FLASH_TYPES_H

#include <cstdint>
#include <vector>
#include "../../sim/Sim_Defs.h"
#include "../NVM_Types.h"

namespace NVM
{
  namespace FlashMemory
  {
    enum class Command_Suspension_Mode { NONE, PROGRAM, PROGRAM_ERASE, ERASE };
  }
}

typedef uint64_t page_status_type;
#define FULL_PROGRAMMED_PAGE 0xffffffffffffffffULL
typedef uint32_t flash_channel_ID_type;
typedef uint32_t flash_chip_ID_type;
typedef uint32_t flash_die_ID_type;
typedef uint32_t flash_plane_ID_type;
typedef uint32_t flash_block_ID_type;
typedef uint32_t flash_page_ID_type;
typedef uint64_t LPA_type;
typedef uint64_t PPA_type;
typedef uint64_t command_code_type;

// Device structure ID list and ID lists per stream.
typedef std::vector<flash_channel_ID_type> ChannelIDs;
typedef std::vector<flash_chip_ID_type>    ChipIDs;
typedef std::vector<flash_die_ID_type>     DieIDs;
typedef std::vector<flash_plane_ID_type>   PlaneIDs;

typedef std::vector<ChannelIDs> StreamChannelIDs;
typedef std::vector<ChipIDs>    StreamChipIDs;
typedef std::vector<DieIDs>     StreamDieIDs;
typedef std::vector<PlaneIDs>   StreamPlaneIDs;

enum class Flash_Technology_Type { SLC = 1, MLC = 2, TLC = 3 };

#define FREE_PAGE 0x0000000000000000ULL
#define NO_LPA 0xffffffffffffffffULL
#define NO_STREAM 0xff
#define UNWRITTEN_LOGICAL_PAGE 0x0000000000000000ULL
#define NO_PPA 0xffffffffffffffffULL
#define NO_MPPN 0xffffffffULL

#endif // !FLASH_TYPES_H
