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

    explicit UserRequestBase(uint64_t id);
    bool is_finished() const;
    void assign_data(void* payload, size_t payload_size);

  };

  force_inline
  UserRequestBase::UserRequestBase(uint64_t id)
    : ID(id),
      Sectors_serviced_from_cache(0),
      ToBeIgnored(false)
  { }

  force_inline bool
  UserRequestBase::is_finished() const
  {
    return (Transaction_list.empty()) && (Sectors_serviced_from_cache == 0);
  }

  force_inline void
  UserRequestBase::assign_data(void* payload, size_t payload_size)
  {
    Data = MQSimEngine::copy_data(payload, payload_size);
  }

  class UserReqPool : protected Utils::ObjectPool<UserRequestBase> {
  public:
    typedef Utils::ObjectPool<UserRequestBase>::item_t item_t;

  private:
    uint64_t __last_id;

  public:
    UserReqPool();

    item_t* construct();
  };

  force_inline
  UserReqPool::UserReqPool()
    : __last_id(0ULL)
  { }

  force_inline UserReqPool::item_t*
  UserReqPool::construct()
  {
    return Utils::ObjectPool<UserRequestBase>::construct(__last_id++);
  }

  typedef UserReqPool::item_t                UserRequest;

  // TODO Check following sequence can move to the request's destructor;
  force_inline void
  delete_request_nvme(UserRequest* request) {
    delete (SubmissionQueueEntry*)request->IO_command_info;

    if (Simulator->Is_integrated_execution_mode() && request->Data)
      delete[] (char*)request->Data;

    if (!request->Transaction_list.empty())
      throw mqsim_error("Deleting an unhandled user requests in the host interface! MQSim thinks something is going wrong!");

    request->release();
  }

}

#endif // !USER_REQUEST_H
