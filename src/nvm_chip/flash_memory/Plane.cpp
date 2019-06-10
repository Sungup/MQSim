#include "Plane.h"

namespace NVM
{
  namespace FlashMemory
  {
    Plane::Plane(uint32_t BlocksNoPerPlane, uint32_t PagesNoPerBlock) :
      Read_count(0), Progam_count(0), Erase_count(0)
    {
      Healthy_block_no = BlocksNoPerPlane;
      Blocks = new Block*[BlocksNoPerPlane];
      for (uint32_t i = 0; i < BlocksNoPerPlane; i++)
        Blocks[i] = new Block(PagesNoPerBlock, i);
      Allocated_streams = NULL;
    }

    Plane::~Plane()
    {
      for (uint32_t i = 0; i < Healthy_block_no; i++)
        delete Blocks[i];
      delete[] Blocks;
    }
  }
}