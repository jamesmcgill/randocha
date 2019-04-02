#pragma once

#if _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include <cstdint>
#include <climits>
#include <assert.h>

//------------------------------------------------------------------------------
// Usage:
//
// 1) Create an instance of Randocha (one per thread)
// 2) Call the member function generate()
//    This will generate 4 random numbers between [0.0f -> 1.0f)
// 3) The Randocha object holds these 4 floats as internal member variables.
//    Access them with the get(idx) member function.
// 4) Call generate() again to replace those internal floats with
//    new random values.
//
// Randocha rand;
// rand.generate();
// float f0 = rand.get(0);
// float f1 = rand.get(1);
// float f2 = rand.get(2);
// float f3 = rand.get(3);
// rand.generate();
// ...
// ...
//
//------------------------------------------------------------------------------
// Sources:
// Manny Ko's GDC presentation Parallel Random Number Generation
// https://gdcvault.com/play/1022146/Math-for-Game-Programmers-Parallel
//
// TEA
// https://en.wikipedia.org/wiki/Tiny_Encryption_Algorithm
//
//------------------------------------------------------------------------------
// TODO:
//
// Allow streaming into a larger user-provided buffer
//  to get more than 4 values at a time.
//
// Implement on GCC/Clang
//
// Performance is not really better than SSE (any improvements, check asm)
//
// Fix SSE returning 1.0f as a valid value
//
// Make it truly header-only
//
//------------------------------------------------------------------------------
struct Randocha
{
  static const uint8_t NUM_GENERATED = 4;

  Randocha()
  {
    m_curRoundKey
      = _mm_set_epi32(0xA341316C, 0xC8013EA4, 0xAD90777D, 0x7E95761E);
  }

  void generate()
  {
    __m128i m;
    m = _mm_aesenc_si128(m_curRoundKey, m_curRoundKey);
    m = _mm_aesenc_si128(m, m_curRoundKey);
    m = _mm_aesenc_si128(m, m_curRoundKey);
    m = _mm_aesenclast_si128(m, m_curRoundKey);

    // TODO(James): Is this required at all when not using a Fiesel Network
    m_curRoundKey = _mm_add_epi32(m_curRoundKey, _mm_set1_epi32(0x9E3779B9));

    // convert to floats in range [0 -> 1)
    __m128 realConversion = _mm_cvtepi32_ps(m);

    static const __m128 MAX_RANGE = _mm_cvtepi32_ps(_mm_set1_epi32(INT_MAX));
    realConversion                = _mm_div_ps(realConversion, MAX_RANGE);
    realConversion = _mm_add_ps(realConversion, _mm_set1_ps(1.0f));
    realConversion = _mm_mul_ps(realConversion, _mm_set1_ps(0.5f));

    _mm_store_ps(m_generated, realConversion);
  }

  float get(uint8_t index)
  {
    assert(index < NUM_GENERATED);
    return m_generated[index];
  }

  __m128i m_curRoundKey;
  float m_generated[NUM_GENERATED] = {};
};

//------------------------------------------------------------------------------
