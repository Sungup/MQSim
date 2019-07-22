#ifndef NVME_DEFINITIONS_H
#define NVME_DEFINITIONS_H

#include <cstdint>
#include <string>

#include "../../utils/Exception.h"
#include "../../utils/InlineTools.h"
#include "../../utils/ObjectPool.h"
#include "../../utils/StringTools.h"

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

#define NVME_FLUSH_OPCODE 0x0000
#define NVME_WRITE_OPCODE 0x0001
#define NVME_READ_OPCODE 0x0002

#define SATA_WRITE_OPCODE 0x0001
#define SATA_READ_OPCODE 0x0002

constexpr uint64_t NCQ_SUBMISSION_REGISTER = 0x1000;
constexpr uint64_t NCQ_COMPLETION_REGISTER = 0x1003;
constexpr uint64_t SUBMISSION_QUEUE_REGISTER_0 = 0x1000;
constexpr uint64_t COMPLETION_QUEUE_REGISTER_0 = 0x1003;
constexpr uint64_t SUBMISSION_QUEUE_REGISTER_1 = 0x1010;
constexpr uint64_t COMPLETION_QUEUE_REGISTER_1 = 0x1013;
constexpr uint64_t SUBMISSION_QUEUE_REGISTER_2 = 0x1020;
constexpr uint64_t COMPLETION_QUEUE_REGISTER_2 = 0x1023;
constexpr uint64_t SUBMISSION_QUEUE_REGISTER_3 = 0x1030;
constexpr uint64_t COMPLETION_QUEUE_REGISTER_3 = 0x1033;
constexpr uint64_t SUBMISSION_QUEUE_REGISTER_4 = 0x1040;
constexpr uint64_t COMPLETION_QUEUE_REGISTER_4 = 0x1043;
constexpr uint64_t SUBMISSION_QUEUE_REGISTER_5 = 0x1050;
constexpr uint64_t COMPLETION_QUEUE_REGISTER_5 = 0x1053;
constexpr uint64_t SUBMISSION_QUEUE_REGISTER_6 = 0x1060;
constexpr uint64_t COMPLETION_QUEUE_REGISTER_6 = 0x1063;
constexpr uint64_t SUBMISSION_QUEUE_REGISTER_7 = 0x1070;
constexpr uint64_t COMPLETION_QUEUE_REGISTER_7 = 0x1073;
constexpr uint64_t SUBMISSION_QUEUE_REGISTER_8 = 0x1080;
constexpr uint64_t COMPLETION_QUEUE_REGISTER_8 = 0x1083;

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
  uint8_t Opcode;
  uint8_t PRP_FUSE;

  //The id of the command in the I/O submission queue
  uint16_t Command_Identifier;

  uint32_t Namespace_identifier;

  uint64_t Reserved;

  uint64_t Metadata_pointer_1;

  uint64_t PRP_entry_1;
  uint64_t PRP_entry_2;

  uint32_t Command_specific[6];

public:
  SQEntryBase(uint8_t opcode,
              uint16_t command_identifier,
              uint64_t prp_entry_1,
              uint64_t prp_entry_2,
              uint32_t cdw10 = 0,
              uint32_t cdw11 = 0,
              uint32_t cdw12 = 0,
              uint32_t cdw13 = 0,
              uint32_t cdw14 = 0,
              uint32_t cdw15 = 0);

  static size_t size();
};

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
