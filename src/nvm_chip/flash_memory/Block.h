#ifndef BLOCK_H
#define BLOCK_H

#include "FlashTypes.h"
#include "Page.h"


namespace NVM {
  namespace FlashMemory {
    class Block {
    public:
      // Again this variable is required in list based garbage collections
      const flash_block_ID_type ID;

      // Records the status of each sub-page
      std::vector<Page> Pages;

      // BlockMetadata Metadata;
      // ??????

      Block(uint32_t PagesNoPerBlock, flash_block_ID_type BlockID);
    };

    force_inline
    Block::Block(uint32_t PagesNoPerBlock, flash_block_ID_type BlockID)
      : ID(BlockID),
        Pages(PagesNoPerBlock)
    { }
  }
}
#endif // ! BLOCK_H
