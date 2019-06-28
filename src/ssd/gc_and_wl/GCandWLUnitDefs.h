//
// GCandWLUnitDefs
// MQSim
//
// Created by 文盛業 on 2019-06-20.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__GCandWLUnitDefs__
#define __MQSim__GCandWLUnitDefs__

#include "../../utils/EnumTools.h"
#include "../../utils/InlineTools.h"
#include "../../utils/StringTools.h"

namespace SSD_Components {
  enum class GC_Block_Selection_Policy_Type {
    GREEDY,
    /*
     * The randomized-greedy algorithm described in: "B. Van Houdt, A Mean Field
     * Model for a Class of Garbage Collection Algorithms in Flash - based Solid
     * State Drives, SIGMETRICS, 2013" and "Stochastic Modeling of Large-Scale
     * Solid-State Storage Systems: Analysis, Design Tradeoffs and Optimization,
     * SIGMETRICS, 2013".
     */
    RGA,

    /*
     * The RANDOM, RANDOM+, and RANDOM++ algorithms described in: "B. Van Houdt,
     * A Mean Field Model  for a Class of Garbage Collection Algorithms in Flash
     * - based Solid State Drives, SIGMETRICS, 2013".
     */
    RANDOM,
    RANDOM_P,
    RANDOM_PP,

    /*
     * The FIFO algortihm described in P. Desnoyers, "Analytic Modeling of SSD
     * Write Performance, SYSTOR, 2012".
     */
    FIFO
  };
}

force_inline std::string
to_string(SSD_Components::GC_Block_Selection_Policy_Type policy)
{
  using namespace SSD_Components;

  switch (policy) {
  case ENUM_TO_STR(GC_Block_Selection_Policy_Type, GREEDY);
  case ENUM_TO_STR(GC_Block_Selection_Policy_Type, RGA);
  case ENUM_TO_STR(GC_Block_Selection_Policy_Type, RANDOM);
  case ENUM_TO_STR(GC_Block_Selection_Policy_Type, RANDOM_P);
  case ENUM_TO_STR(GC_Block_Selection_Policy_Type, RANDOM_PP);
  case ENUM_TO_STR(GC_Block_Selection_Policy_Type, FIFO);
  }
}

force_inline SSD_Components::GC_Block_Selection_Policy_Type
to_gc_block_selection_policy(std::string v)
{
  using namespace SSD_Components;

  Utils::to_upper(v);

  STR_TO_ENUM(GC_Block_Selection_Policy_Type, GREEDY);
  STR_TO_ENUM(GC_Block_Selection_Policy_Type, RGA);
  STR_TO_ENUM(GC_Block_Selection_Policy_Type, RANDOM);
  STR_TO_ENUM(GC_Block_Selection_Policy_Type, RANDOM_P);
  STR_TO_ENUM(GC_Block_Selection_Policy_Type, RANDOM_PP);
  STR_TO_ENUM(GC_Block_Selection_Policy_Type, FIFO);

  throw mqsim_error("Unknown GC block selection policy specified in the SSD "
                    "configuration file");
}

#endif /* Predefined include guard __MQSim__GCandWLUnitDefs__ */
