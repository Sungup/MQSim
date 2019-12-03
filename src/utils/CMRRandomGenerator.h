#ifndef CMR_RANDOM_GENERATOR_H
#define CMR_RANDOM_GENERATOR_H

#include <cstdint>

#include "InlineTools.h"

// TODO Remove static features

namespace Utils
{
  class CMRRandomGenerator
  {
    private:
      static constexpr double norm = 2.328306549295728e-10;
      static constexpr double m1 = 4294967087.0;
      static constexpr double m2 = 4294944443.0;
      static constexpr double a12 = 1403580.0;
      static constexpr double a13 = -810728.0;
      static constexpr double a21 = 527612.0;
      static constexpr double a23 = -1370589.0;

      double s[2][3];
      static const double a[2][3][3];
      static const double m[2];
      static const double init_s[2][3];


      static double mod(double x, double m)
      {
        int64_t k = (int64_t)(x / m);
        x -= (double)k * m;
        if (x < 0.0)
          x += m;
        return x;
      }

      /*
      *   Advance CMRG one step and return next number
      */
      static int64_t ftoi(double x, double m)
      {
        if (x >= 0.0)
          return (int64_t)(x);
        else
          return (int64_t)((double)x + (double)m);
      }

      static double itof(int64_t i, int64_t m)
      {
        return (double)i;
      }

      static void v_ftoi(double* u, int64_t* v, double m)
      {
        for (int i = 0; i <= 2; i++)
          v[i] = ftoi(u[i], m);
      }

      static void v_itof(int64_t* u, double* v, int64_t m)
      {
        for (int i = 0; i <= 2; i++)
          v[i] = itof((int64_t)u[i], m);
      }

      static void v_copy(const int64_t* u, int64_t* v)
      {
        for (int i = 0; i <= 2; i++)
          v[i] = u[i];
      }

      static void m_ftoi(const double a[][3], int64_t b[][3], double m)
      {
        for (int i = 0; i <= 2; i++)
          for (int j = 0; j <= 2; j++)
            b[i][j] = ftoi(a[i][j], m);
      }

      static void m_copy(int64_t a[][3], int64_t b[][3])
      {
        for (int i = 0; i <= 2; i++)
          for (int j = 0; j <= 2; j++)
            b[i][j] = a[i][j];
      }

      static void mv_mul(int64_t a[][3], const int64_t* u, int64_t* v, int64_t m)
      {
        int64_t w[3];
        for (int i = 0; i <= 2; i++)
        {
          w[i] = 0;
          for (int j = 0; j <= 2; j++)
            w[i] = (a[i][j] * u[j] + w[i]) % m;
        }
        v_copy(w, v);
      }

      static void mm_mul(int64_t a[][3], int64_t b[][3], int64_t c[][3], int64_t m)
      {
        int64_t d [3][3];

        for (int i = 0; i <= 2; i++)
        {
          for (int j = 0; j <= 2; j++)
          {
            d[i][j] = 0;
            for (int k = 0; k <= 2; k++)
              d[i][j] = (a[i][k] * b[k][j] + d[i][j]) % m;
          }
        }
        m_copy(d, c);
      }

  public:
    force_inline
    CMRRandomGenerator(int64_t n, int e)
      : s { {0, }, }
    {
      for (int i = 0; i <= 1; i++)
        for (int j = 0; j <= 2; j++)
          s[i][j] = init_s[i][j];
      Advance(n, e);
    }

    force_inline void Advance(int64_t n, int e)
    {
      int64_t B[2][3][3];

      int64_t S[2][3];
      int64_t M[2];

      for (int i = 0; i <= 1; i++)
      {
        m_ftoi(a[i], B[i], m[i]);
        v_ftoi(s[i], S[i], m[i]);
        M[i] = (int64_t)(m[i]);
      }

      while (e-- != 0)
      {
        for (int i = 0; i <= 1; i++)
          mm_mul(B[i], B[i], B[i], M[i]);
      }

      while (n != 0)
      {
        if ((n & 0x1U) != 0)
          for (int i = 0; i <= 1; i++)
            mv_mul(B[i], S[i], S[i], M[i]);
        n >>= 1U;
        if (n != 0)
          for (int i = 0; i <= 1; i++)
            mm_mul(B[i], B[i], B[i], M[i]);
      }

      for (int i = 0; i <= 1; i++)
        v_itof(S[i], s[i], M[i]);
    }

    force_inline double NextDouble()
    {
      double p1 = mod(a12 * s[0][1] + a13 * s[0][0], m1);
      s[0][0] = s[0][1];
      s[0][1] = s[0][2];
      s[0][2] = p1;
      double p2 = mod(a21 * s[1][2] + a23 * s[1][0], m2);
      s[1][0] = s[1][1];
      s[1][1] = s[1][2];
      s[1][2] = p2;
      double p = p1 - p2;
      if (p < 0.0)
        p += m1;
      return (p + 1) * norm;
    }

  };
}

#endif // !CMR_RANDOM_GENERATOR_H

