#ifndef RANDOM_GENERATOR
#define RANDOM_GENERATOR

#include <cstdint>
#include "CMRRandomGenerator.h"

namespace Utils
{
  class RandomGenerator
  {
  private:
    CMRRandomGenerator rand;
    double Normal_z2;

  public:
    const int seed;

  public:
    explicit RandomGenerator(int seed);
    ~RandomGenerator() = default;

    uint32_t Get_uint(uint32_t max_value);
    int32_t Get_int(int32_t max_value);
    double FloatRandom();
    double Uniform(double a, double b);
    uint32_t Uniform_uint(uint32_t m, uint32_t n);
    int64_t Uniform_long(int64_t m, int64_t n);
    uint64_t Uniform_ulong(uint64_t m, uint64_t n);
    double Exponential(double mean);
    double Erlang(double x, double s);
    double HyperExponential(double x, double s);
    double Normal(double x, double s);
    double LogNormal(double x, double s);
    int64_t Geometric0(double x);
    int64_t Geometric1(double x);
    double Hyper_geometric(double x, double s);
    int64_t Binomial(int64_t n, double u);
    int64_t Poisson(double x);
    double Weibull(double alpha, double beta);
    double Pareto(double alpha, double beta);
    double Inverse(double min, double max);
    double Triangular(double min, double middle, double max);
  };
}

#endif // !RANDOM_GENERATOR
