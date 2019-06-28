//
// EnumTools
// MQSim
//
// Created by 文盛業 on 2019-06-28.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__EnumTools__
#define __MQSim__EnumTools__

#define ENUM_TO_STR(T, E) T::E: return #E
#define STR_TO_ENUM(T, E) do { if (v == #E) return T::E; } while (false)

#endif /* Predefined include guard __MQSim__EnumTools__ */
