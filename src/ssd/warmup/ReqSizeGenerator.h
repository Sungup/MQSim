//
// ReqSizeGenerator
// MQSim
//
// Created by 文盛業 on 2019/12/01.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__ReqSizeGenerator__
#define __MQSim__ReqSizeGenerator__

#include <cstdint>
#include <cmath>
#include <memory>

#include "../../utils/Workload_Statistics.h"
#include "../../utils/RandomGenerator.h"

namespace SSD_Components {
  class ReqSizeGenBase {
  protected:
    const uint32_t _avg_req_sectors;

  public:
    explicit ReqSizeGenBase(const Utils::Workload_Statistics& stat);
    virtual ~ReqSizeGenBase() = default;

    virtual uint32_t generate() const;
  };

  typedef std::shared_ptr<ReqSizeGenBase> ReqSizeGenerator;

  /// Children classes
  using FixedSizeGenertor = ReqSizeGenBase;

  class NormalSizeGenerator : public ReqSizeGenBase {
  private:
    mutable Utils::RandomGenerator __gen;

    const uint32_t __stdev_req_sectors;

  public:
    explicit NormalSizeGenerator(const Utils::Workload_Statistics& stat);
    ~NormalSizeGenerator() final = default;

    uint32_t generate() const final;
  };

  force_inline ReqSizeGenerator
  build_req_size_gen(const Utils::Workload_Statistics& stat)
  {
    switch (stat.Request_size_distribution_type) {
    case Utils::Request_Size_Distribution_Type::FIXED:
      return std::make_shared<FixedSizeGenertor>(stat);

    case Utils::Request_Size_Distribution_Type::NORMAL:
      return std::make_shared<NormalSizeGenerator>(stat);
    }
  }
}

#endif /* Predefined include guard __MQSim__ReqSizeGenerator__ */
