//
// IoFlowRandomGenerator
// MQSim
//
// Created by 文盛業 on 2019-08-16.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__IoFlowRandomGenerator__
#define __MQSim__IoFlowRandomGenerator__

#include <cstdint>
#include <memory>

#include "../../exec/params/IOFlowParamSet.h"
#include "../../ssd/SSD_Defs.h"
#include "../../utils/RandomGenerator.h"
#include "../../utils/DistributionTypes.h"

namespace Host_Components {
  /// =========================
  /// Request Size Distributors
  /// =========================

  /// 0. Request size distributor base class
  class ReqSizeDistributor {
  protected:
    const uint32_t _avg_size;

  public:
    explicit ReqSizeDistributor(const SyntheticFlowParamSet& params);
    virtual ~ReqSizeDistributor() = default;

    virtual uint32_t generate();

    virtual void get_stats(Utils::Workload_Statistics& stats) const;
  };

  /// 1. Fixed size distributor
  using FixedSizeDistributor = ReqSizeDistributor;

  /// 2. Normalized size distributor
  class NormalSizeDistributor : public ReqSizeDistributor {
  private:
    const uint32_t __variance;

    Utils::RandomGenerator __generator;

  public:
    explicit NormalSizeDistributor(const SyntheticFlowParamSet& params);
    ~NormalSizeDistributor() final = default;

    uint32_t generate() final;

    void get_stats(Utils::Workload_Statistics& stats) const final;
  };

  /// 3. Request Size Distributor Builder
  typedef std::shared_ptr<ReqSizeDistributor> ReqSizeDistributorPtr;

  ReqSizeDistributorPtr
  build_req_size_distributor(const SyntheticFlowParamSet& params);

  /// ====================
  /// Address Distributors
  /// ====================

  /// 0. Address Distributor base class
  class AddressDistributor {
  protected:
    const bool     _enable_align;
    const uint32_t _align_unit;

    const LHA_type _start;
    const LHA_type _end;

    Utils::RandomGenerator _generator;

  protected:
    LHA_type _align_address(LHA_type lba) const;
    LHA_type _align_address(LHA_type lba, int lba_count,
                            LHA_type start, LHA_type end) const;
  public:
    AddressDistributor(const SyntheticFlowParamSet& params,
                       LHA_type start,
                       LHA_type end);

    virtual ~AddressDistributor() = default;

    virtual void init();

    virtual LHA_type generate(uint32_t lba_count);

    virtual void get_stats(Utils::Workload_Statistics& stats) const;
  };

  /// 1. Uniform address distributor
  using UniformDistributor = AddressDistributor;

  /// 2. Stream address distributor
  class StreamDistributor : public AddressDistributor {
  private:
    LHA_type __next_address;

  public:
    StreamDistributor(const SyntheticFlowParamSet& params,
                      LHA_type start,
                      LHA_type end);

    ~StreamDistributor() final = default;

    void init() final;

    LHA_type generate(uint32_t lba_count) final;

    void get_stats(Utils::Workload_Statistics& stats) const final;
  };

  /// 3. Hot/Cold address distributor
  class HotColdDistributor : public AddressDistributor {
  private:
    const double __hot_region_ratio;
    const LHA_type __hot_end;
    const LHA_type __cold_start;

    Utils::RandomGenerator __hot_cold_generator;

  public:
    HotColdDistributor(const SyntheticFlowParamSet& params,
                       LHA_type start,
                       LHA_type end);

    ~HotColdDistributor() final = default;

    LHA_type generate(uint32_t lba_count) final;

    void get_stats(Utils::Workload_Statistics& stats) const final;
  };

  /// 4. Address Distributor Builder
  typedef std::shared_ptr<AddressDistributor> AddressDistributorPtr;

  AddressDistributorPtr
  build_address_distributor(const SyntheticFlowParamSet& params,
                            LHA_type start, LHA_type end);
}

#endif /* Predefined include guard __MQSim__IoFlowRandomGenerator__ */
