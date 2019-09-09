#ifndef USER_REQUEST_H
#define USER_REQUEST_H

#include <string>
#include <list>

#include "../SSD_Defs.h"
#include "../../sim/Engine.h"
#include "../../sim/Sim_Defs.h"
#include "../../utils/Exception.h"
#include "../../utils/InlineTools.h"
#include "../../utils/ObjectPool.h"

#include "../interface/Host_Interface_Defs.h"

namespace SSD_Components
{
  enum class UserRequestType {
    READ,
    WRITE
  };

  force_inline UserRequestType
  opcode_to_req_type(uint8_t opcode) {
    switch (opcode) {
    case __READ_OPCODE:  return UserRequestType::READ;
    case __WRITE_OPCODE: return UserRequestType::WRITE;
    default: throw mqsim_error("Unexpected user request type");
    }
  }

  class NvmTransaction;
  class UserRequestBase
  {
  public:
    IO_Flow_Priority_Class Priority_class;
    const io_request_id_type ID;
    LHA_type Start_LBA;

    sim_time_type STAT_InitiationTime;
    sim_time_type STAT_ResponseTime;

    std::list<NvmTransaction*> Transaction_list;
    uint32_t Sectors_serviced_from_cache;

    uint32_t Size_in_byte;
    uint32_t SizeInSectors;
    UserRequestType Type;
    stream_id_type Stream_id;
    bool ToBeIgnored;
    void* IO_command_info;//used to store host I/O command info
    void* Data;

    UserRequestBase(uint64_t id,
                    stream_id_type stream_id,
                    IO_Flow_Priority_Class priority,
                    UserRequestType type,
                    LHA_type start_lba,
                    uint32_t lba_count,
                    void* payload,
                    sim_time_type initiate_time);

    bool is_finished() const;
    void assign_data(void* payload, size_t payload_size);
  };

  force_inline
  UserRequestBase::UserRequestBase(uint64_t id,
                                   stream_id_type stream_id,
                                   IO_Flow_Priority_Class priority,
                                   UserRequestType type,
                                   LHA_type start_lba,
                                   uint32_t lba_count,
                                   void* payload,
                                   sim_time_type initiate_time)
    : Priority_class(priority),
      ID(id),
      Start_LBA(start_lba),
      STAT_InitiationTime(initiate_time),
      STAT_ResponseTime(0),
      Sectors_serviced_from_cache(0),
      Size_in_byte(lba_count * SECTOR_SIZE_IN_BYTE),
      SizeInSectors(lba_count),
      Type(type),
      Stream_id(stream_id),
      ToBeIgnored(false),
      IO_command_info(payload),
      Data(nullptr)
  { }

  force_inline bool
  UserRequestBase::is_finished() const
  {
    return (Transaction_list.empty()) && (Sectors_serviced_from_cache == 0);
  }

  force_inline void
  UserRequestBase::assign_data(void* payload, size_t payload_size)
  {
    // TODO This block can make the memory leak while runnign simulator.
    Data = MQSimEngine::copy_data(payload, payload_size);
  }

  // -----------------
  // User Request Pool
  // -----------------
  class UserReqPool : protected Utils::ObjectPool<UserRequestBase> {
  public:
    typedef Utils::ObjectPool<UserRequestBase>::item_t item_t;

  private:
    uint64_t __last_id;

  public:
    UserReqPool();

    item_t* construct(stream_id_type stream_id,
                      IO_Flow_Priority_Class priority,
                      void* payload);
  };

  typedef UserReqPool::item_t UserRequest;

  force_inline
  UserReqPool::UserReqPool()
    : __last_id(0ULL)
  { }

  force_inline UserReqPool::item_t*
  UserReqPool::construct(stream_id_type stream_id,
                         IO_Flow_Priority_Class priority,
                         void *payload)
  {
    auto* sqe = static_cast<SQEntry*>(payload);

    return Utils::ObjectPool<UserRequestBase>::construct(
      __last_id++,
      stream_id,
      priority,
      opcode_to_req_type(sqe->Opcode),
      to_start_lba(*sqe),
      to_lba_count(*sqe),
      payload,
      Simulator->Time()
    );
  }

  // TODO Check following sequence can move to the request's destructor;
  force_inline void
  delete_request_nvme(UserRequest* request) {
    ((SQEntry*)request->IO_command_info)->release();

    if (Simulator->Is_integrated_execution_mode() && request->Data)
      delete[] (char*)request->Data;

    if (!request->Transaction_list.empty())
      throw mqsim_error("Deleting an unhandled user requests in the host "
                        "interface! MQSim thinks something is going wrong!");

    request->release();
  }

}

#endif // !USER_REQUEST_H
