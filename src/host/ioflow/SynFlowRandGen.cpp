//
// IoFlowRandomGenerator
// MQSim
//
// Created by 文盛業 on 2019-08-16.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#include "SynFlowRandGen.h"

#include <cmath>

using namespace Host_Components;

/// =========================
/// Request Size Distributors
/// =========================

/// 0. Default Request size distributor (and fixed size distributor)
ReqSizeDistributor::ReqSizeDistributor(const SyntheticFlowParamSet& params)
  : _avg_size(params.Average_Request_Size)
{ }

uint32_t
ReqSizeDistributor::generate()
{
  return _avg_size;
}

void
ReqSizeDistributor::get_stats(Utils::Workload_Statistics& stats) const
{
  stats.Request_size_distribution_type = Utils::Request_Size_Distribution_Type::FIXED;
  stats.avg_request_sectors = _avg_size;
}

/// 2. Normalized size distributor
NormalSizeDistributor::NormalSizeDistributor(const SyntheticFlowParamSet& params)
  : ReqSizeDistributor(params),
    __variance(params.Variance_Request_Size),
    __generator(params.gen_seed())
{ }

uint32_t
NormalSizeDistributor::generate()
{
  return std::max(uint32_t(std::ceil(__generator.Normal(_avg_size,
                                                        __variance))),
                  1U);
}

void
NormalSizeDistributor::get_stats(Utils::Workload_Statistics& stats) const
{
  ReqSizeDistributor::get_stats(stats);

  stats.Request_size_distribution_type = Utils::Request_Size_Distribution_Type::NORMAL;

  stats.STDEV_reuqest_size = __variance;
  stats.random_request_size_generator_seed = __generator.seed;
}

/// 3. Request Size Distributor Builder
ReqSizeDistributorPtr
Host_Components::build_req_size_distributor(const SyntheticFlowParamSet& params)
{
  switch (params.Request_Size_Distribution) {
  case Utils::Request_Size_Distribution_Type::FIXED:
    return std::make_shared<FixedSizeDistributor>(params);

  case Utils::Request_Size_Distribution_Type::NORMAL:
    return std::make_shared<NormalSizeDistributor>(params);
  }
}

/// ====================
/// Address Distributors
/// ====================

/// 0. Default uniform address distributor (and uniform distributor)
AddressDistributor::AddressDistributor(const SyntheticFlowParamSet& params,
                                       LHA_type start,
                                       LHA_type end)
  : _enable_align(params.Generated_Aligned_Addresses),
    _align_unit(params.Address_Alignment_Unit),
    _start(start),
    _end(end),
    _generator(params.gen_seed())
{ }

force_inline LHA_type
AddressDistributor::_align_address(LHA_type lba) const
{
  return lba - (_enable_align ? lba % _align_unit : 0);
}

force_inline LHA_type
AddressDistributor::_align_address(LHA_type lba, int lba_count,
                                   LHA_type start, LHA_type end) const
{
  if (lba < start || end < lba)
    throw mqsim_error("Generated address is out of range in IO_Flow_Synthetic");

  if (end < lba + lba_count)
    lba = start;

  return _align_address(lba);
}

void
AddressDistributor::init()
{ /* Nothing to do for default uniform random generator. */ }

LHA_type
AddressDistributor::generate(uint32_t lba_count)
{
  return _align_address(_generator.Uniform_ulong(_start, _end),
                        lba_count, _start, _end);
}

void
AddressDistributor::get_stats(Utils::Workload_Statistics& stats) const
{
  stats.Address_distribution_type = Utils::Address_Distribution_Type::RANDOM_UNIFORM;

  stats.random_address_generator_seed = _generator.seed;
  stats.generate_aligned_addresses = _enable_align;
  stats.alignment_value = _align_unit;
}

/// 2. Stream address distributor
StreamDistributor::StreamDistributor(const SyntheticFlowParamSet& params,
                                     LHA_type start,
                                     LHA_type end)
  : AddressDistributor(params, start, end),
    __next_address(0)
{ }

void
StreamDistributor::init()
{
  __next_address = _align_address(_generator.Uniform_ulong(_start, _end));
}

LHA_type
StreamDistributor::generate(uint32_t lba_count)
{
  LHA_type lba = (_end < __next_address + lba_count) ? _start : __next_address;

  __next_address = lba + lba_count;

  /*
   * Next streaming address should be larger than start_lsa. But aligning
   * address using __align_address return smaller address than argument named
   * lba. So, streaming_next_address should add __align_unit of return of
   * __align_address()
   */
  if (_enable_align)
    __next_address = _align_address(__next_address) + _align_unit;

  if(__next_address == lba)
    std::cout << "Synthetic Message Generator: The same address is always "
                 "repeated due to configuration parameters!" << std::endl;

  return _align_address(lba);
}

void
StreamDistributor::get_stats(Utils::Workload_Statistics& stats) const
{
  AddressDistributor::get_stats(stats);

  stats.Address_distribution_type = Utils::Address_Distribution_Type::STREAMING;
}

/// 3. Hot/Cold address distributor
HotColdDistributor::HotColdDistributor(const SyntheticFlowParamSet& params,
                                       LHA_type start,
                                       LHA_type end)
  : AddressDistributor(params, start, end),
    __hot_region_ratio(params.hot_region_rate()),
    __hot_end(_start + LHA_type(double(_end - _start) * __hot_region_ratio)),
    __cold_start(__hot_end + 1),
    __hot_cold_generator(params.gen_seed())
{ }

LHA_type
HotColdDistributor::generate(uint32_t lba_count)
{
  LHA_type region_start;
  LHA_type region_end;

  /*
   * Hot/Cold Region
   *
   * address: |<---------------------------->|<------------------------------->|
   *          |<- start_lsa  hot_region_end->|<- cold_region_start    end_lsa->|
   *          |<---- (100 - hot_ratio%) ---->|<---------- hot_ratio% --------->|
   *
   * (100-hot)% of requests going to hot% of the address space
   *   - example: Only 10% address generate 90% IO request.
   */

  if (__hot_cold_generator.Uniform(0, 1) < __hot_region_ratio) {
    region_start = __cold_start;
    region_end = _end;
  } else {
    region_start = _start;
    region_end = __hot_end;
  }

  return _align_address(_generator.Uniform_ulong(region_start, region_end),
                        lba_count, region_start, region_end);
}

void
HotColdDistributor::get_stats(Utils::Workload_Statistics& stats) const
{
  AddressDistributor::get_stats(stats);

  stats.Address_distribution_type = Utils::Address_Distribution_Type::RANDOM_HOTCOLD;

  stats.Ratio_of_hot_addresses_to_whole_working_set = __hot_region_ratio;
  stats.Ratio_of_traffic_accessing_hot_region = 1 - __hot_region_ratio;

  stats.random_hot_address_generator_seed = _generator.seed;
  stats.random_hot_cold_generator_seed = __hot_cold_generator.seed;
}

/// 4. Address Distributor Builder
AddressDistributorPtr
Host_Components::build_address_distributor(const SyntheticFlowParamSet& params,
                                           LHA_type start, LHA_type end)
{
  switch (params.Address_Distribution) {
  case Utils::Address_Distribution_Type::RANDOM_UNIFORM:
    return std::make_shared<UniformDistributor>(params, start, end);

  case Utils::Address_Distribution_Type::STREAMING:
    return std::make_shared<StreamDistributor>(params, start, end);

  case Utils::Address_Distribution_Type::RANDOM_HOTCOLD:
    return std::make_shared<HotColdDistributor>(params, start, end);
  }
}