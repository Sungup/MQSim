#ifndef HOST_IO_REQUEST_H
#define HOST_IO_REQUEST_H

#include "../sim/Sim_Defs.h"
#include "../ssd/SSD_Defs.h"
#include "../utils/InlineTools.h"
#include "../utils/ObjectPool.h"

namespace Host_Components
{
  enum class HostIOReqType { READ, WRITE };

  class HostIORequestBase {
  public:
    sim_time_type Arrival_time;//The time that the request has been generated
    sim_time_type Enqueue_time;//The time that the request enqueued into the I/O queue
    LHA_type Start_LBA;
    uint32_t LBA_count;
    HostIOReqType Type;
    uint16_t IO_queue_info;
    uint16_t Source_flow_id;//Only used in SATA host interface

  public:
    HostIORequestBase(sim_time_type arrival,
                      LHA_type start_lba,
                      uint32_t lba_count,
                      HostIOReqType type);

    uint32_t requested_size() const;
  };

  force_inline
  HostIORequestBase::HostIORequestBase(sim_time_type arrival,
                                       LHA_type start_lba,
                                       uint32_t lba_count,
                                       HostIOReqType type)
    : Arrival_time(arrival),
      Enqueue_time(0),
      Start_LBA(start_lba),
      LBA_count(lba_count),
      Type(type),
      IO_queue_info(0),
      Source_flow_id(0)
  { }

  force_inline
  uint32_t HostIORequestBase::requested_size() const
  {
    return LBA_count * SECTOR_SIZE_IN_BYTE;
  }

  typedef Utils::ObjectPool<HostIORequestBase> HostIOReqPool;
  typedef HostIOReqPool::item_t                HostIORequest;
}

#endif // !HOST_IO_REQUEST_H
