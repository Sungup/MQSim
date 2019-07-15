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
  template <typename T>
  class AvgStats {
  private:
    T __sum;
    uint64_t __cnt;

    void __update(const T& val);

  public:
    AvgStats();
    AvgStats(const AvgStats& rhs);

    T avg(sim_time_type scale = 1) const;
    T sum() const;
    uint64_t count() const;

    void reset();

    // Merge two stats
    AvgStats operator+(const AvgStats& rhs) const;

    // Scale down by sim_time_type unit time
    AvgStats operator/(const T& val) const;

    // For counting up
    AvgStats& operator+=(const T& val);
  };

  template <typename T>
  force_inline
  AvgStats<T>::AvgStats()
    : __sum(0),
      __cnt(0)
  { }

  template <typename T>
  force_inline
  AvgStats<T>::AvgStats(const AvgStats<T>& rhs)
    : __sum(rhs.__sum),
      __cnt(rhs.__cnt)
  { }

  template <typename T>
  force_inline void
  AvgStats<T>::__update(const T& val)
  {
    __sum += val;
    ++__cnt;
  }

  template <typename T>
  force_inline AvgStats<T>
  AvgStats<T>::operator+(const AvgStats<T>& rhs) const
  {
    AvgStats<T> v;

    v.__sum = __sum + rhs.__sum;
    v.__cnt = __cnt + rhs.__cnt;

    return v;
  }

  template <typename T>
  force_inline AvgStats<T>
  AvgStats<T>::operator/(const T& val) const
  {
    auto v = *this;

    v.__sum /= val;

    return v;
  }

  template <typename T>
  force_inline AvgStats<T>&
  AvgStats<T>::operator+=(const T& val)
  {
    __update(val);

    return *this;
  }

  template <typename T>
  force_inline T
  AvgStats<T>::avg(sim_time_type scale) const
  {
    return __cnt ? __sum / __cnt / scale : 0;
  }

  template <typename T>
  force_inline T
  AvgStats<T>::sum() const
  {
    return __sum;
  }

  template <typename T>
  force_inline uint64_t
  AvgStats<T>::count() const
  {
    return __cnt;
  }

  template <typename T>
  force_inline void
  AvgStats<T>::reset()
  {
    __sum = __cnt = 0;
  }

  // -----------------
  // Min/Max/Avg stats
  // -----------------
  template <typename T>
  class MinMaxAvgStats {
  private:
    T __min;
    T __max;
    T __sum;
    uint64_t __cnt;

    void __update(const T& val);

  public:
    explicit MinMaxAvgStats(T max = UINT32_MAX, T min = 0);
    MinMaxAvgStats(const MinMaxAvgStats& rhs);

    T min(sim_time_type scale = 1) const;
    T max(sim_time_type scale = 1) const;
    T avg(sim_time_type scale = 1) const;
    T sum() const;
    uint64_t count() const;

    // Merge two stats
    MinMaxAvgStats operator+(const MinMaxAvgStats& rhs) const;

    // Scale down by sim_time_type unit time
    MinMaxAvgStats operator/(const T& val) const;

    // For counting up
    MinMaxAvgStats& operator+=(const T& val);
  };

  template <typename T>
  force_inline
  MinMaxAvgStats<T>::MinMaxAvgStats(T max, T min)
    : __min(max),
      __max(min),
      __sum(0),
      __cnt(0)
  { }

  template <typename T>
  force_inline
  MinMaxAvgStats<T>::MinMaxAvgStats(const MinMaxAvgStats<T>& rhs)
    : __min(rhs.__min),
      __max(rhs.__max),
      __sum(rhs.__sum),
      __cnt(rhs.__cnt)
  { }

  template <typename T>
  force_inline void
  MinMaxAvgStats<T>::__update(const T& val)
  {
    __min = std::min(__min, val);
    __max = std::max(__max, val);
    __sum += val;
    ++__cnt;
  }

  template <typename T>
  force_inline MinMaxAvgStats<T>
  MinMaxAvgStats<T>::operator+(const MinMaxAvgStats<T>& rhs) const
  {
    MinMaxAvgStats<T> v;

    v.__min = std::min(__min, rhs.__min);
    v.__max = std::max(__max, rhs.__max);
    v.__sum = __sum + rhs.__sum;
    v.__cnt = __cnt + rhs.__cnt;

    return v;
  }

  template <typename T>
  force_inline MinMaxAvgStats<T>
  MinMaxAvgStats<T>::operator/(const T& val) const
  {
    auto v = *this;

    v.__min /= val;
    v.__max /= val;
    v.__sum /= val;

    return v;
  }

  template <typename T>
  force_inline MinMaxAvgStats<T>&
  MinMaxAvgStats<T>::operator+=(const T& val)
  {
    __update(val);

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

  template <typename T>
  force_inline T
  MinMaxAvgStats<T>::avg(sim_time_type scale) const
  {
    return __cnt ? __sum / __cnt / scale : 0;
  }

  template <typename T>
  force_inline T
  MinMaxAvgStats<T>::sum() const
  {
    return __sum;
  }

  template <typename T>
  force_inline uint64_t
  MinMaxAvgStats<T>::count() const
  {
    return __cnt;
  }

}

#endif /* Predefined include guard __MQSim__MixMaxStats__ */
