#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H
#include <vector>

namespace Utils
{
  double Combination_count(double n, double k);
  double Combination_count(uint32_t n, uint32_t k);
  void Euler_estimation(std::vector<double>& mu, uint32_t b, double rho, int d, double h, double max_diff, int itr_max);
  class Helper_Functions
  {
  public:
  };
}

#endif // !HELPER_FUNCTIONS_H
