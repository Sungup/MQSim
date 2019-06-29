#ifndef PLANE_H
#define PLANE_H

#include "../../exec/params/FlashParameterSet.h"
#include "../NVM_Types.h"
#include "FlashTypes.h"
#include "Block.h"
#include "Flash_Command.h"

namespace NVM {
  namespace FlashMemory {
    class Plane {
    public:
      std::vector<Block> Blocks;
      uint32_t Healthy_block_no;
      unsigned long Read_count;                     //how many read count in the process of workload
      unsigned long Progam_count;
      unsigned long Erase_count;
      stream_id_type* Allocated_streams;

      explicit Plane(const FlashParameterSet& params);
    };

    force_inline
    Plane::Plane(const FlashParameterSet& params)
      : Healthy_block_no(params.Block_No_Per_Plane),
        Read_count(0),
        Progam_count(0),
        Erase_count(0),
        Allocated_streams(nullptr)
    {
      Blocks.reserve(params.Block_No_Per_Plane);
      for (uint32_t i = 0; i < params.Block_No_Per_Plane; i++)
        Blocks.emplace_back(params.Page_No_Per_Block, i);
    }
  }
}
#endif // !PLANE_H
