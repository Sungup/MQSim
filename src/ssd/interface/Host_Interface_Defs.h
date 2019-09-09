#ifndef NVME_DEFINITIONS_H
#define NVME_DEFINITIONS_H

#include <cstdint>
#include <string>

#include "../../sim/Sim_Defs.h"
#include "../../utils/Exception.h"
#include "../../utils/InlineTools.h"
#include "../../utils/ObjectPool.h"
#include "../../utils/StringTools.h"

#include "../SSD_Defs.h"

// ======================================
// Enumerators and its string converters.
// ======================================
enum class IO_Flow_Priority_Class {
  URGENT = 1,
  HIGH = 2,
  MEDIUM = 3,
  LOW = 4
};

enum class HostInterface_Types {
  SATA,
  NVME
};

force_inline std::string
to_string(IO_Flow_Priority_Class priority)
{
  switch (priority) {
  case IO_Flow_Priority_Class::URGENT: return "URGENT";
  case IO_Flow_Priority_Class::HIGH:   return "HIGH";
  case IO_Flow_Priority_Class::MEDIUM: return "MEDIUM";
  case IO_Flow_Priority_Class::LOW:    return "LOW";
  }
}

force_inline IO_Flow_Priority_Class
to_io_flow_priority(std::string v)
{
  Utils::to_upper(v);

  if (v == "URGENT") return IO_Flow_Priority_Class::URGENT;
  if (v == "HIGH")   return IO_Flow_Priority_Class::HIGH;
  if (v == "MEDIUM") return IO_Flow_Priority_Class::MEDIUM;
  if (v == "LOW")    return IO_Flow_Priority_Class::LOW;

  throw mqsim_error("Wrong priority class definition for input flow");
}

force_inline std::string
to_string(HostInterface_Types interface)
{
  switch (interface) {
  case HostInterface_Types::SATA: return "SATA";
  case HostInterface_Types::NVME: return "NVME";
  }
}

force_inline HostInterface_Types
to_host_interface_type(std::string v)
{
  Utils::to_upper(v);

  if (v == "SATA") return HostInterface_Types::SATA;
  if (v == "NVME") return HostInterface_Types::NVME;

  throw mqsim_error("Wrong host interface type definition from configuration");

}

constexpr uint8_t __FLUSH_OPCODE = 0x0000;
constexpr uint8_t __WRITE_OPCODE = 0x0001;
constexpr uint8_t __READ_OPCODE  = 0x0002;

constexpr uint8_t NVME_FLUSH_OPCODE = __FLUSH_OPCODE;
constexpr uint8_t NVME_WRITE_OPCODE = __WRITE_OPCODE;
constexpr uint8_t NVME_READ_OPCODE  = __READ_OPCODE;

constexpr uint8_t SATA_WRITE_OPCODE = __WRITE_OPCODE;
constexpr uint8_t SATA_READ_OPCODE  = __READ_OPCODE;

constexpr uint64_t NCQ_SUBMISSION_REGISTER = 0x1000;
constexpr uint64_t NCQ_COMPLETION_REGISTER = 0x1003;
constexpr uint64_t INVALID_QUEUE_REGISTER = UINT64_MAX;

/*
 *
 *  - Register of SQ0: 0x1000, CQ0: 0x1003
 *  - Register of SQ1: 0x1010, CQ1: 0x1013
 *  - Register of SQ2: 0x1020, CQ2: 0x1023
 *  - Register of SQ3: 0x1030, CQ3: 0x1033
 *  - Register of SQ4: 0x1040, CQ4: 0x1043
 *  - Register of SQ5: 0x1050, CQ5: 0x1053
 *  - Register of SQ6: 0x1060, CQ6: 0x1063
 *  - Register of SQ7: 0x1070, CQ7: 0x1073
 *  - Register of SQ8: 0x1080, CQ8: 0x1083
 */

force_inline uint64_t
sq_register_address(uint64_t id)
{
  return 0x01000ULL | id << 4U;
}

force_inline uint64_t
cq_register_address(uint64_t id)
{
  return sq_register_address(id) | 0x03ULL;
}

force_inline bool
is_cq_address(uint64_t register_address)
{
  // CQ Address always ends with 0x0003
  return (0x0FULL & register_address);
}

force_inline uint16_t
queue_id_from_register(uint64_t register_address)
{
  return (register_address >> 4ULL) & 0x0FULL;
}

force_inline stream_id_type
register_addr_to_stream_id(uint64_t register_address)
{
  return queue_id_from_register(register_address) - 1;
}

force_inline bool
is_valid_nvme_stream_id(stream_id_type stream_id)
{
  return stream_id < 8;
}

// ----------------------------
// Class definition for CQEntry
// ----------------------------
class CQEntryBase
{
public:
  // Currently reserved field.
  const uint32_t Command_specific;
  const uint32_t Reserved;

  // SQ Head Pointer, Indicates the current Submission Queue Head pointer for
  // the Submission Queue indicated in the SQ Identifier field
  const uint16_t SQ_Head;

  // SQ Identifier, Indicates the Submission Queue to which the associated
  // command was issued to.
  const uint16_t SQ_ID;

  // Command Identifier, Indicates the identifier of the command that is being
  // completed
  const uint16_t Command_Identifier;

  // Status Field (SF)+ Phase Tag(P)
  //   SF: Indicates status for the command that is being completed
  //   P:Identifies whether a Completion Queue entry is new
  const uint16_t SF_P;

public:
  CQEntryBase(uint16_t head,
              uint16_t flow_id,
              uint16_t SF_P,
              uint16_t command_identifier);

  static size_t size();
};

force_inline
CQEntryBase::CQEntryBase(uint16_t head,
                         uint16_t flow_id,
                         uint16_t SF_P,
                         uint16_t command_identifier)
  : Command_specific(0),
    Reserved(0),
    SQ_Head(head),
    SQ_ID(flow_id),
    Command_Identifier(command_identifier),
    SF_P(SF_P)
{ }

force_inline size_t
CQEntryBase::size()
{
  return sizeof(CQEntryBase);
}

typedef Utils::ObjectPool<CQEntryBase> CQEntryPool;
typedef CQEntryPool::item_t            CQEntry;

// ----------------------------
// Class definition for SQEntry
// ----------------------------
class SQEntryBase {
public:
  // Is it a read or write request
  const uint8_t Opcode;
  const uint8_t PRP_FUSE;

  //The id of the command in the I/O submission queue
  const uint16_t Command_Identifier;

  const uint32_t Namespace_identifier;

  const uint64_t Reserved;

  const uint64_t Metadata_pointer_1;

  const uint64_t PRP_entry_1;
  const uint64_t PRP_entry_2;

  const uint32_t Command_specific[6];

public:
  SQEntryBase(uint8_t opcode,
              uint16_t command_identifier,
              uint64_t prp_entry_1,
              uint64_t prp_entry_2,
              uint32_t cdw10 = 0, // lsb of lba
              uint32_t cdw11 = 0, // msb of lba
              uint32_t cdw12 = 0, // lsb of lba_count (only lsb 16bit)
              uint32_t cdw13 = 0,
              uint32_t cdw14 = 0,
              uint32_t cdw15 = 0);

  static size_t size();
};

force_inline uint64_t
to_start_lba(const SQEntryBase& item)
{
  return merge_lba(item.Command_specific[1], item.Command_specific[0]);
}

force_inline uint32_t
to_lba_count(const SQEntryBase& item)
{
  return lsb_of_lba_count(item.Command_specific[2]);
}

force_inline
SQEntryBase::SQEntryBase(uint8_t opcode,
                         uint16_t command_identifier,
                         uint64_t prp_entry_1,
                         uint64_t prp_entry_2,
                         uint32_t cdw10,
                         uint32_t cdw11,
                         uint32_t cdw12,
                         uint32_t cdw13,
                         uint32_t cdw14,
                         uint32_t cdw15)
  : Opcode(opcode),
    PRP_FUSE(0),
    Command_Identifier(command_identifier),
    Namespace_identifier(0),
    Reserved(0),
    Metadata_pointer_1(0),
    PRP_entry_1(prp_entry_1),
    PRP_entry_2(prp_entry_2),
    Command_specific {
      cdw10, cdw11,
      cdw12, cdw13,
      cdw14, cdw15
    }
{ }

force_inline size_t
SQEntryBase::size()
{
  return sizeof(SQEntryBase);
}

typedef Utils::ObjectPool<SQEntryBase> SQEntryPool;
typedef SQEntryPool::item_t            SQEntry;

#endif // !NVME_DEFINISIONS_H
