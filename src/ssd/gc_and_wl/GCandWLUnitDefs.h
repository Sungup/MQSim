//
// GCandWLUnitDefs
// MQSim
//
// Created by 文盛業 on 2019-06-20.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__GCandWLUnitDefs__
#define __MQSim__GCandWLUnitDefs__

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

#endif /* Predefined include guard __MQSim__GCandWLUnitDefs__ */
