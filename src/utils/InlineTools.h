//
// InlineTools
// MQSim
//
// Created by 文盛業 on 2019-06-11.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__InlineTools__
#define __MQSim__InlineTools__

#if FORCE_INLINE
#define force_inline __attribute__((always_inline)) inline
#else
#define force_inline inline
#endif

#endif /* Predefined include guard __MQSim__InlineTools__ */
