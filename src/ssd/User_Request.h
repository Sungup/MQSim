#ifndef USER_REQUEST_H
#define USER_REQUEST_H

#include <string>
#include <list>
#include "SSD_Defs.h"
#include "../sim/Sim_Defs.h"
#include "Host_Interface_Defs.h"
#include "NVM_Transaction.h"

namespace SSD_Components
{
  enum class UserRequestType { READ, WRITE };
  class NVM_Transaction;
  class User_Request
  {
  public:
    User_Request();
    IO_Flow_Priority_Class Priority_class;
    io_request_id_type ID;
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
  private:
    static uint32_t lastId;
  };
}

#endif // !USER_REQUEST_H
