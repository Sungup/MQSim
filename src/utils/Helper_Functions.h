#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include <cstdint>
#include <vector>

#include "InlineTools.h"

namespace Utils
{
  void Euler_estimation(std::vector<double>& mu,
                        uint32_t b,
                        double rho,
                        int d,
                        double h,
                        double max_diff,
                        int itr_max);

  force_inline double
  Combination_count(double n, double k)
  {
    if (k > n) return 0;
    if (k * 2 > n) k = n - k;
    if (k == 0) return 1;

    double result = n;
    for (int i = 2; i <= k; ++i) {
      result *= (n - i + 1);
      result /= i;
    }
    return result;
  }

  force_inline double
  Combination_count(uint32_t n, uint32_t k)
  {
    return Utils::Combination_count(double(n), double(k));
  }

}

#endif // !HELPER_FUNCTIONS_H
