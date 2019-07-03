#ifndef NVM_TRANSACTION_FLASH_H
#define NVM_TRANSACTION_FLASH_H

#include <string>
#include <list>
#include<cstdint>
#include "../sim/Sim_Defs.h"
#include "../sim/SimEvent.h"
#include "../sim/Engine.h"
#include "../nvm_chip/flash_memory/FlashTypes.h"
#include "../nvm_chip/flash_memory/Flash_Chip.h"
#include "../nvm_chip/flash_memory/Physical_Page_Address.h"
#include "request/UserRequest.h"
#include "NVM_Transaction.h"

namespace SSD_Components
{
  class NVM_Transaction_Flash : public NVM_Transaction
  {
  public:
    NVM_Transaction_Flash(Transaction_Source_Type source, Transaction_Type type, stream_id_type stream_id,
      uint32_t data_size_in_byte, LPA_type lpa, PPA_type ppa, UserRequest* user_request);
    NVM_Transaction_Flash(Transaction_Source_Type source, Transaction_Type type, stream_id_type stream_id,
      uint32_t data_size_in_byte, LPA_type lpa, PPA_type ppa, const NVM::FlashMemory::Physical_Page_Address& address, UserRequest* user_request);
    NVM::FlashMemory::Physical_Page_Address Address;
    uint32_t Data_and_metadata_size_in_byte; //number of bytes contained in the request: bytes in the real page + bytes of metadata

    LPA_type LPA;
    PPA_type PPA;

    bool SuspendRequired;
    bool Physical_address_determined;
    sim_time_type Estimated_alone_waiting_time;//Used in scheduling methods, such as FLIN, where fairness and QoS is considered in scheduling
    bool FLIN_Barrier;//Especially used in queue reordering inf FLIN scheduler
  private:

  };
}

#endif // !FLASH_TRANSACTION_H
