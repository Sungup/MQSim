//
// MixMaxStats
// MQSim
//
// Created by 文盛業 on 2019-07-15.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__MixMaxStats__
#define __MQSim__MixMaxStats__

#include <algorithm>
#include <cstdint>

#include "InlineTools.h"

namespace Utils {
  // ------------------------------
  // Simple counting stats for IOPS
  // ------------------------------
  class CountingStats {
  private:
    uint64_t _cnt;

  protected:
    CountingStats();

    virtual ~CountingStats() = default;

    void _count();
    void _merge(const CountingStats& rhs);

  public:
    uint64_t count() const;

    virtual void reset();
  };

  force_inline
  CountingStats::CountingStats()
    : _cnt(0ULL)
  { }

  force_inline void
  CountingStats::_count()
  {
    ++_cnt;
  }

  force_inline void
  CountingStats::_merge(const CountingStats& rhs)
  {
    _cnt += rhs._cnt;
  }

  force_inline uint64_t
  CountingStats::count() const
  {
    return _cnt;
  }

  force_inline void
  CountingStats::reset()
  {
    _cnt = 0;
  }

  // ------------------------------
  // Counting + Sum Stats for bandwidth
  // ------------------------------
  template <typename T>
  class CountingSumStats : public CountingStats {
  private:
    T _sum;

  protected:
    CountingSumStats();
    CountingSumStats(const CountingSumStats& rhs);

    ~CountingSumStats() override = default;

    void _merge(const CountingSumStats& rhs);
    virtual void _update(const T& val);

  public:
    T avg(sim_time_type scale = 1) const;
    T sum() const;

    void reset() override;
  };

  template <typename T>
  force_inline
  CountingSumStats<T>::CountingSumStats()
    : CountingStats(),
      _sum(0)
  { }

  template <typename T>
  force_inline
  CountingSumStats<T>::CountingSumStats(const CountingSumStats<T>& rhs)
    : CountingStats(rhs),
      _sum(rhs._sum)
  { }

  template <typename T>
  force_inline void
  CountingSumStats<T>::_merge(const CountingSumStats<T>& rhs)
  {
    CountingStats::_merge(rhs);
    _sum += rhs._sum;
  }

  template <typename T>
  force_inline void
  CountingSumStats<T>::_update(const T& val)
  {
    _count();
    _sum += val;
  }

  template <typename T>
  force_inline T
  CountingSumStats<T>::avg(sim_time_type scale) const
  {
    return count() ? _sum / count() / scale : 0;
  }

  template <typename T>
  force_inline T
  CountingSumStats<T>::sum() const
  {
    return _sum;
  }

  template <typename T>
  force_inline void
  CountingSumStats<T>::reset()
  {
    CountingStats::reset();
    _sum = 0;
  }

  class IopsStats : public CountingStats {
  public:
    IopsStats();
    ~IopsStats() final = default;

    double iops(double seconds) const;

    // Merge two stats
    IopsStats operator+(const IopsStats& rhs) const;

    // Simple counting up
    IopsStats& operator++();
    IopsStats operator++(int);
  };

  force_inline
  IopsStats::IopsStats()
    : CountingStats()
  { }

  force_inline double
  IopsStats::iops(double seconds) const
  {
    return double(count()) / seconds;
  }

  force_inline IopsStats
  IopsStats::operator+(const Utils::IopsStats& rhs) const
  {
    IopsStats v = *this;

    v._merge(rhs);

    return v;
  }

  force_inline IopsStats&
  IopsStats::operator++()
  {
    this->_count();

    return *this;
  }

  force_inline IopsStats
  IopsStats::operator++(int)
  {
    IopsStats v = *this;

    ++*this;

    return v;
  }
  // ----------------------------------------------
  // Simple Average stats for bandwidth information
  // ----------------------------------------------
  template <typename T>
  class BandwidthStats : public CountingSumStats<T> {
  public:
    BandwidthStats();
    BandwidthStats(const BandwidthStats& rhs);

    ~BandwidthStats() final = default;

    double bandwidth(double second) const;

    // Merge two stats
    BandwidthStats operator+(const BandwidthStats& rhs) const;

    // For counting up with value
    BandwidthStats& operator+=(const T& val);
  };

  template <typename T>
  force_inline
  BandwidthStats<T>::BandwidthStats()
    : CountingSumStats<T>()
  { }

  template <typename T>
  force_inline
  BandwidthStats<T>::BandwidthStats(const BandwidthStats<T>& rhs)
    : CountingSumStats<T>(rhs)
  { }

  template <typename T>
  force_inline double
  BandwidthStats<T>::bandwidth(double second) const
  {
    return double(this->sum()) / second;
  }

  template <typename T>
  force_inline BandwidthStats<T>
  BandwidthStats<T>::operator+(const BandwidthStats<T>& rhs) const
  {
    BandwidthStats<T> v = *this;

    v._merge(rhs);

    return v;
  }

  template <typename T>
  force_inline BandwidthStats<T>&
  BandwidthStats<T>::operator+=(const T& val)
  {
    this->_update(val);

    return *this;
  }

  // --------------------------------------------
  // Simple average stats for latency information
  // --------------------------------------------
  template <typename T>
  class AvgStats : public CountingSumStats<T> {
  public:
    AvgStats();
    AvgStats(const AvgStats& rhs);

    ~AvgStats() final = default;

    // Merge two stats
    AvgStats operator+(const AvgStats& rhs) const;

    // For counting up
    AvgStats& operator+=(const T& val);
  };

  template <typename T>
  force_inline
  AvgStats<T>::AvgStats()
    : CountingSumStats<T>()
  { }

  template <typename T>
  force_inline
  AvgStats<T>::AvgStats(const AvgStats<T>& rhs)
    : CountingSumStats<T>(rhs)
  { }

  template <typename T>
  force_inline AvgStats<T>
  AvgStats<T>::operator+(const AvgStats<T>& rhs) const
  {
    AvgStats<T> v = *this;

    v._merge(rhs);

    return v;
  }

  template <typename T>
  force_inline AvgStats<T>&
  AvgStats<T>::operator+=(const T& val)
  {
    this->_update(val);

    return *this;
  }

  // -----------------
  // Min/Max/Avg stats
  // -----------------
  template <typename T>
  class MinMaxAvgStats : public CountingSumStats<T> {
  protected:
    T __min;
    T __max;

    void _merge(const MinMaxAvgStats& rhs);
    void _update(const T& val) final;

  public:
    explicit MinMaxAvgStats(T max = UINT32_MAX, T min = 0);
    MinMaxAvgStats(const MinMaxAvgStats& rhs);

    ~MinMaxAvgStats() final = default;

    T min(sim_time_type scale = 1) const;
    T max(sim_time_type scale = 1) const;

    // Merge two stats
    MinMaxAvgStats operator+(const MinMaxAvgStats& rhs) const;

    // For counting up
    MinMaxAvgStats& operator+=(const T& val);
  };

  template <typename T>
  force_inline
  MinMaxAvgStats<T>::MinMaxAvgStats(T max, T min)
    : CountingSumStats<T>(),
      __min(max),
      __max(min)
  { }

  template <typename T>
  force_inline
  MinMaxAvgStats<T>::MinMaxAvgStats(const MinMaxAvgStats<T>& rhs)
    : CountingSumStats<T>(rhs),
      __min(rhs.__min),
      __max(rhs.__max)
  { }

  template <typename T>
  force_inline void
  MinMaxAvgStats<T>::_merge(const MinMaxAvgStats<T>& rhs)
  {
    CountingSumStats<T>::_merge(rhs);

    __min = std::min(__min, rhs.__min);
    __max = std::max(__max, rhs.__max);
  }

  template <typename T>
  force_inline void
  MinMaxAvgStats<T>::_update(const T& val)
  {
    CountingSumStats<T>::_update(val);

    __min = std::min(__min, val);
    __max = std::max(__max, val);
  }

  template <typename T>
  force_inline MinMaxAvgStats<T>
  MinMaxAvgStats<T>::operator+(const MinMaxAvgStats<T>& rhs) const
  {
    MinMaxAvgStats<T> v = *this;

    v._merge(rhs);

    return v;
  }

  template <typename T>
  force_inline MinMaxAvgStats<T>&
  MinMaxAvgStats<T>::operator+=(const T& val)
  {
    _update(val);

    return *this;
  }

  template <typename T>
  force_inline T
  MinMaxAvgStats<T>::min(sim_time_type scale) const
  {
    return __min / scale;
  }

  template <typename T>
  force_inline T
  MinMaxAvgStats<T>::max(sim_time_type scale) const
  {
    return __max / scale;
  }
}

#endif /* Predefined include guard __MQSim__MixMaxStats__ */
