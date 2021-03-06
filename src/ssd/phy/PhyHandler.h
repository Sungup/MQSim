//
// PhyHandler
// MQSim
//
// Created by 文盛業 on 2019-06-13.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__PhyHandler__
#define __MQSim__PhyHandler__

#include "../../utils/ServiceHandler.h"
#include "../NvmTransactionFlash.h"

namespace SSD_Components {
  typedef Utils::ServiceHandlerBase<NvmTransaction*>                    NvmTransactionHandlerBase;
  typedef Utils::ServiceHandlerBase<NvmTransactionFlash*>              FlashTransactionHandlerBase;
  typedef Utils::ServiceHandlerBase<flash_channel_ID_type>               ChannelIdleSignalHandlerBase;
  typedef Utils::ServiceHandlerBase<const NVM::FlashMemory::Flash_Chip&> ChipIdleSignalHandlerBase;

  template <typename T>
  using NvmTransactionHandler = Utils::ServiceHandler<T, NvmTransaction*>;

  template <typename T>
  using FlashTransactionHandler = Utils::ServiceHandler<T, NvmTransactionFlash*>;

  template <typename T>
  using ChannelIdleSignalHandler = Utils::ServiceHandler<T, flash_channel_ID_type>;

  template <typename T>
  using ChipIdleSignalHandler = Utils::ServiceHandler<T, const NVM::FlashMemory::Flash_Chip&>;

  typedef std::vector<NvmTransactionHandlerBase*>     NvmTransactionHandlerList;
  typedef std::vector<FlashTransactionHandlerBase*>   FlashTransactionHandlerList;
  typedef std::vector<ChannelIdleSignalHandlerBase*>  ChannelIdleSignalHandlerList;
  typedef std::vector<ChipIdleSignalHandlerBase*>     ChipIdleSignalHandlerList;
}

#endif /* Predefined include guard __MQSim__PhyHandler__ */
