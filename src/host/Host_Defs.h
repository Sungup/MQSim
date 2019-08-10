#ifndef HOST_DEFS_H
#define HOST_DEFS_H

#include "../utils/Exception.h"

#define QUEUE_ID_TO_FLOW_ID(Q) Q - 1
#define FLOW_ID_TO_Q_ID(F) F + 1
#define NVME_COMP_Q_MEMORY_REGION 40U
#define DATA_MEMORY_REGION 0xFF0000000000

/*
 * Following addresses is the sq and cq memory address on system. Original MQSim
 * uses the constant address values, but this values can be calculated following
 * functions. So, refactored version uses the address calculation function to
 * initialize in the IoQueueInfo's constructor.
 *  - Memory address of the SQ0: 0x000000000000, CQ0: 0x00F000000000
 *  - Memory address of the SQ1: 0x010000000000, CQ1: 0x01F000000000
 *  - Memory address of the SQ2: 0x020000000000, CQ2: 0x02F000000000
 *  - Memory address of the SQ3: 0x030000000000, CQ3: 0x03F000000000
 *  - Memory address of the SQ4: 0x040000000000, CQ4: 0x04F000000000
 *  - Memory address of the SQ5: 0x050000000000, CQ5: 0x05F000000000
 *  - Memory address of the SQ6: 0x060000000000, CQ6: 0x06F000000000
 *  - Memory address of the SQ7: 0x070000000000, CQ7: 0x07F000000000
 *  - Memory address of the SQ8: 0x080000000000, CQ8: 0x08F000000000
 */

force_inline uint64_t
sq_memory_address(uint64_t id)
{
  if (id == 0) throw mqsim_error("I/O queue id 0 is reserved for NVMe admin "
                                 "queues and should not be used for I/O flows");
  if (id > 8)  throw mqsim_error("Assigned id is too large");

  return id << 40U;
}

force_inline uint64_t
cq_memory_address(uint64_t id)
{
  return sq_memory_address(id) | 0x00F0ULL << 32U;
}

#endif//!HOST_DEFS_H