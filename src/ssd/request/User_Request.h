#ifndef USER_REQUEST_H
#define USER_REQUEST_H

#include <string>
#include <list>
#include "../SSD_Defs.h"
#include "../../sim/Sim_Defs.h"
#include "../Host_Interface_Defs.h"
#include "../NVM_Transaction.h"

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
}

#endif // !USER_REQUEST_H
