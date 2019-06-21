#ifndef HOST_DEFS_H
#define HOST_DEFS_H


#define QUEUE_ID_TO_FLOW_ID(Q) Q - 1
#define FLOW_ID_TO_Q_ID(F) F + 1
#define NVME_COMP_Q_MEMORY_REGION 40U
#define DATA_MEMORY_REGION 0xFF0000000000

const uint64_t SUBMISSION_QUEUE_MEMORY_0 = 0x000000000000;
const uint64_t COMPLETION_QUEUE_MEMORY_0 = 0x00F000000000;
const uint64_t SUBMISSION_QUEUE_MEMORY_1 = 0x010000000000;
const uint64_t COMPLETION_QUEUE_MEMORY_1 = 0x01F000000000;
const uint64_t SUBMISSION_QUEUE_MEMORY_2 = 0x020000000000;
const uint64_t COMPLETION_QUEUE_MEMORY_2 = 0x02F000000000;
const uint64_t SUBMISSION_QUEUE_MEMORY_3 = 0x030000000000;
const uint64_t COMPLETION_QUEUE_MEMORY_3 = 0x03F000000000;
const uint64_t SUBMISSION_QUEUE_MEMORY_4 = 0x040000000000;
const uint64_t COMPLETION_QUEUE_MEMORY_4 = 0x04F000000000;
const uint64_t SUBMISSION_QUEUE_MEMORY_5 = 0x050000000000;
const uint64_t COMPLETION_QUEUE_MEMORY_5 = 0x05F000000000;
const uint64_t SUBMISSION_QUEUE_MEMORY_6 = 0x060000000000;
const uint64_t COMPLETION_QUEUE_MEMORY_6 = 0x06F000000000;
const uint64_t SUBMISSION_QUEUE_MEMORY_7 = 0x070000000000;
const uint64_t COMPLETION_QUEUE_MEMORY_7 = 0x07F000000000;
const uint64_t SUBMISSION_QUEUE_MEMORY_8 = 0x080000000000;
const uint64_t COMPLETION_QUEUE_MEMORY_8 = 0x08F000000000;

#endif//!HOST_DEFS_H