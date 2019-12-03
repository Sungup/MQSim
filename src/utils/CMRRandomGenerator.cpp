#include "CMRRandomGenerator.h"

using namespace Utils;

const double CMRRandomGenerator::a[2][3][3] = {
  {
    {0.0, 1.0, 0.0},
    {0.0, 0.0, 1.0},
    {a13, a12, 0.0}
  },
  {
    {0.0, 1.0, 0.0},
    {0.0, 0.0, 1.0},
    {a23, 0.0, a21}}
};

const double CMRRandomGenerator::m[2] = { m1, m2 };

const double CMRRandomGenerator::init_s[2][3] = {
  {1.0, 1.0, 1.0},
  {1.0, 1.0, 1.0}
};
