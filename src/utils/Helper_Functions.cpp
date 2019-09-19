#include "Helper_Functions.h"
#include <cmath>

void
Utils::Euler_estimation(std::vector<double>& mu,
                        uint32_t b,
                        double rho,
                        int d,
                        double h,
                        double max_diff,
                        int itr_max)
{
  std::vector<double> w_0, w;

  w_0.reserve(mu.size());
  w.reserve(mu.size());

  w_0.emplace_back(1);
  w.emplace_back(1);

  for (size_t i = 1; i <= mu.size(); i++) {
    double w_0_val = 0;

    for (size_t j = i; j < mu.size(); j++)
      w_0_val += mu[j];

    w_0.emplace_back(w_0_val);
    w.emplace_back(0);
  }

  double t = h;
  int itr = 0;
  double diff = 100000000000000;
  while (itr < itr_max && diff > max_diff) {
    double sigma = 0;
    for (uint32_t j = 1; j <= b; j++)
      sigma += std::pow(w_0[j], d);

    for (uint32_t i = 1; i < b; i++)
      w[i] = w_0[i] + h * (1 - std::pow(w_0[i], d) - (b - sigma) * ((i * (w_0[i] - w_0[i + 1])) / (b * rho)));

    diff = std::abs(w[0] - w_0[0]);
    for (uint32_t i = 1; i <= b; i++)
      if (std::abs(w[i] - w_0[i]) > diff)
        diff = std::abs(w[i] - w_0[i]);

    for (size_t i = 1; i < w_0.size(); i++)
      w_0[i] = w[i];

    t += h;
    itr++;
  }

  for (uint32_t i = 0; i <= b; i++)
    mu[i] = w_0[i] - w_0[i + 1];
}
