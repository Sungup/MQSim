#ifndef FLASH_TYPES_H
#define FLASH_TYPES_H

#include <cstdint>
#include <vector>
#include "../../sim/Sim_Defs.h"
#include "../../utils/Exception.h"
#include "../../utils/StringTools.h"
#include "../../utils/InlineTools.h"
#include "../NVM_Types.h"

namespace NVM
{
  namespace FlashMemory
  {
    enum class Command_Suspension_Mode {
      NONE = 0,           // 0b000
      PROGRAM = 1,        // 0b001
      ERASE = 2,          // 0b010
      PROGRAM_ERASE = 3   // 0b011
    };
  }
}

force_inline bool
operator&(const NVM::FlashMemory::Command_Suspension_Mode& lhs,
          const NVM::FlashMemory::Command_Suspension_Mode& rhs)
{
  return static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs);
}

enum class Flash_Technology_Type {
  SLC = 1,
  MLC = 2,
  TLC = 3
};

force_inline std::string
to_string(NVM::FlashMemory::Command_Suspension_Mode mode)
{
  namespace nf = NVM::FlashMemory;

  switch (mode) {
  case nf::Command_Suspension_Mode::NONE:          return "NONE";
  case nf::Command_Suspension_Mode::PROGRAM:       return "PROGRAM";
  case nf::Command_Suspension_Mode::PROGRAM_ERASE: return "PROGRAM_ERASE";
  case nf::Command_Suspension_Mode::ERASE:         return "ERASE";
  }
}

force_inline std::string
to_string(Flash_Technology_Type type)
{
  switch (type) {
  case Flash_Technology_Type::SLC: return "SLC";
  case Flash_Technology_Type::MLC: return "MLC";
  case Flash_Technology_Type::TLC: return "TLC";
  }
}

force_inline NVM::FlashMemory::Command_Suspension_Mode
to_command_suspension_mode(std::string v)
{
  namespace nf = NVM::FlashMemory;

  Utils::to_upper(v);

  if (v == "NONE")          return nf::Command_Suspension_Mode::NONE;
  if (v == "PROGRAM")       return nf::Command_Suspension_Mode::PROGRAM;
  if (v == "PROGRAM_ERASE") return nf::Command_Suspension_Mode::PROGRAM_ERASE;
  if (v == "ERASE")         return nf::Command_Suspension_Mode::ERASE;

  throw mqsim_error("Unknown command suspension mode specified in the input"
                    " file");
}

force_inline Flash_Technology_Type
to_flash_type(std::string v)
{
  Utils::to_upper(v);

  if (v == "SLC") return Flash_Technology_Type::SLC;
  if (v == "MLC") return Flash_Technology_Type::MLC;
  if (v == "TLC") return Flash_Technology_Type::TLC;

  throw mqsim_error("Unknown flash technology type specified in the input"
                    " file");
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

#define FREE_PAGE 0x0000000000000000ULL
#define NO_LPA 0xffffffffffffffffULL
#define NO_STREAM 0xff
#define UNWRITTEN_LOGICAL_PAGE 0x0000000000000000ULL
#define NO_PPA 0xffffffffffffffffULL
#define NO_MPPN 0xffffffffULL

#endif // !FLASH_TYPES_H
