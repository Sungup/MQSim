#ifndef NVM_TRANSACTION_H
#define NVM_TRANSACTION_H

#include <list>

#include "../sim/Sim_Defs.h"
#include "../sim/Engine.h"

#include "request/UserRequest.h"

namespace SSD_Components
{
  enum class Transaction_Type {
    READ,
    WRITE,
    ERASE,
    UNKOWN
  };

  enum class Transaction_Source_Type {
    USERIO,
    CACHE,
    GC_WL,
    MAPPING
  };

  class NvmTransaction {
  public:
    stream_id_type Stream_id;
    Transaction_Source_Type Source;
    Transaction_Type Type;
    UserRequest* UserIORequest;

    // Just used for high performance linked-list insertion/deletion
    // std::list<NvmTransaction*>::iterator RelatedNodeInQueue;

    /*
     * Used to calculate service time and transfer time for a normal
     * read/program operation used to respond to the host IORequests. In other
     * words, these variables are not important if FlashTransactions is used
     * for garbage collection.
     */
    sim_time_type Issue_time;

    sim_time_type STAT_execution_time;
    sim_time_type STAT_transfer_time;

  protected:
    NvmTransaction(stream_id_type stream_id,
                   Transaction_Source_Type source,
                   Transaction_Type type,
                   UserRequest* user_request);

  public:
    virtual ~NvmTransaction() = default;

    sim_time_type STAT_waiting_time() const;
  };

  force_inline
  NvmTransaction::NvmTransaction(stream_id_type stream_id,
                                 Transaction_Source_Type source,
                                 Transaction_Type type,
                                 UserRequest* user_request)
    : Stream_id(stream_id),
      Source(source),
      Type(type),
      UserIORequest(user_request),
      Issue_time(Simulator->Time()),
      STAT_execution_time(INVALID_TIME),
      STAT_transfer_time(INVALID_TIME)
  { }

  force_inline sim_time_type
  NvmTransaction::STAT_waiting_time() const
  {
    return (Simulator->Time() - Issue_time)
             - STAT_execution_time - STAT_transfer_time;
  }
}

#endif //!NVM_TRANSACTION_H