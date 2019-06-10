#include "Block.h"

namespace NVM
{
  namespace FlashMemory
  {
    Block::Block(uint32_t PagesNoPerBlock, flash_block_ID_type BlockID)
    {
      ID = BlockID;
      Pages = new Page[PagesNoPerBlock];
    }

    Block::~Block()
    {
    delete[] Pages;
    }
  }
}