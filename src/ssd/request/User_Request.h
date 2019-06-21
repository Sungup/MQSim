#ifndef USER_REQUEST_H
#define USER_REQUEST_H

#include <string>
#include <list>
#include "../SSD_Defs.h"
#include "../../sim/Sim_Defs.h"
#include "../interface/Host_Interface_Defs.h"
#include "../NVM_Transaction.h"

#include "../../utils/Exception.h"

namespace SSD_Components
{
  enum class UserRequestType {
    READ,
    WRITE
  };

  class NVM_Transaction;
  class User_Request
  {
  private:
    // TODO Change static id generator to global id generator
    static uint32_t lastId;

  public:
    IO_Flow_Priority_Class Priority_class;
    const io_request_id_type ID;
    LHA_type Start_LBA;

    sim_time_type STAT_InitiationTime;
    sim_time_type STAT_ResponseTime;

    std::list<NVM_Transaction*> Transaction_list;
    uint32_t Sectors_serviced_from_cache;

    uint32_t Size_in_byte;
    uint32_t SizeInSectors;
    UserRequestType Type;
    stream_id_type Stream_id;
    bool ToBeIgnored;
    void* IO_command_info;//used to store host I/O command info
    void* Data;

    User_Request();
    bool is_finished() const;
    void assign_data(void* payload, size_t payload_size);

  };

  force_inline
  User_Request::User_Request()
    : ID(std::to_string(lastId++)),
      Sectors_serviced_from_cache(0),
      ToBeIgnored(false)
  { }

  force_inline bool
  User_Request::is_finished() const
  {
    return (Transaction_list.empty()) && (Sectors_serviced_from_cache == 0);
  }

  force_inline void
  User_Request::assign_data(void* payload, size_t payload_size)
  {
    Data = MQSimEngine::copy_data(payload, payload_size);
  }

  // TODO Check following sequence can move to the request's destructor;
  force_inline void
  delete_request_nvme(User_Request* request) {
    delete (Submission_Queue_Entry*)request->IO_command_info;

    if (Simulator->Is_integrated_execution_mode() && request->Data)
      delete[] (char*)request->Data;

    if (!request->Transaction_list.empty())
      throw mqsim_error("Deleting an unhandled user requests in the host interface! MQSim thinks something is going wrong!");

    delete request;
  }

}

#endif // !USER_REQUEST_H
