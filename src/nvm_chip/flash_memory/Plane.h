#ifndef PLANE_H
#define PLANE_H

#include "../NVM_Types.h"
#include "FlashTypes.h"
#include "Block.h"
#include "Flash_Command.h"

namespace NVM
{
  namespace FlashMemory
  {
    class Plane
    {
    public:
      Plane(uint32_t BlocksNoPerPlane, uint32_t PagesNoPerBlock);
      ~Plane();
      Block** Blocks;
      uint32_t Healthy_block_no;
      unsigned long Read_count;                     //how many read count in the process of workload
      unsigned long Progam_count;
      unsigned long Erase_count;
      stream_id_type* Allocated_streams;
    };
  }
}
#endif // !PLANE_H
