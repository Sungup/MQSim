//
// ReqSizeGenerator
// MQSim
//
// Created by 文盛業 on 2019/12/01.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#include "ReqSizeGenerator.h"

using namespace SSD_Components;

ReqSizeGenBase::ReqSizeGenBase(const Utils::Workload_Statistics& stat)
  : _avg_req_sectors(stat.avg_request_sectors)
{ }

uint32_t
ReqSizeGenBase::generate() const
{
  return _avg_req_sectors;
}

NormalSizeGenerator::NormalSizeGenerator(const Utils::Workload_Statistics& stat)
  : ReqSizeGenBase(stat),
    __gen(stat.random_request_size_generator_seed),
    __stdev_req_sectors(stat.STDEV_reuqest_size)
{ }

uint32_t
NormalSizeGenerator::generate() const
{
  auto size = uint32_t(std::ceil(__gen.Normal(_avg_req_sectors,
                                              __stdev_req_sectors)));

  return (size > 0) ? size : 1;
}
