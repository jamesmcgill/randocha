#pragma once

#if _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include <cstdint>
//------------------------------------------------------------------------------
struct RandTea
{
  static const size_t NUM_GENERATED = 4;

  void generate()
  {
    static const int NUM_ROUNDS = 4;
    static const uint32_t DELTA = 0x9E3779B9;
    static const uint32_t k[NUM_GENERATED]
      = {0xA341316C, 0xC8013EA4, 0xAD90777D, 0x7E95761E};

    uint32_t sum = 0;
    for (int i = 0; i < NUM_ROUNDS; ++i)
    {
      sum += DELTA;
      auto& v = m_values;
      v[0] += ((v[1] << 4) + k[0]) ^ (v[1] + sum) ^ ((v[1] >> 5) + k[1]);
      v[1] += ((v[0] << 4) + k[2]) ^ (v[0] + sum) ^ ((v[0] >> 5) + k[3]);

      v[2] += ((v[3] << 4) + k[0]) ^ (v[3] + sum) ^ ((v[3] >> 5) + k[1]);
      v[3] += ((v[2] << 4) + k[2]) ^ (v[2] + sum) ^ ((v[2] >> 5) + k[3]);
    }

    return;
  }

  uint32_t get(size_t index)
  {
    assert(index < NUM_GENERATED);
    return m_values[index];
  }

  float getF(size_t index)
  {
    assert(index < NUM_GENERATED);

    static const double RANGE = (double)(UINT32_MAX) + 1.0;
    return static_cast<float>((double)m_values[index] / RANGE);
  }

  uint32_t m_values[NUM_GENERATED] = {};
};

//------------------------------------------------------------------------------
