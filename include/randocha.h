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
//  Randocha (Rand + Japanese TEA)
//
//  Random number generator built for speed
//  Therefore it is NOT cryptographically secure
//
//  Generates fairly decent white noise images (using included distribution_viz)
//  Generates random numbers in the range [0.0f -> 1.0f)
//  8 random numbers are generated at a time
//
//------------------------------------------------------------------------------
// Usage:
//
// 1) Create an instance of Randocha (one per thread, as there is state)
// 2) Call the member function generate() giving a buffer big enough to store
//    the results
// 3) The buffer will contain 8 random numbers between [0.0f -> 1.0f)
//
// e.g.
// Randocha rand;
// float buffer[Randocha::NUM_GENERATED];
// rand.generate(buffer);
//
//------------------------------------------------------------------------------
// TODO:
//
// Implement on GCC/Clang
//
//------------------------------------------------------------------------------
struct Randocha
{
  static const size_t NUM_GENERATED = 8;

  Randocha()
  {
    m_curRoundKey
      = _mm_set_epi32(0xA341316C, 0xC8013EA4, 0xAD90777D, 0x7E95761E);
  }

  //----------------------------------------------------------------------------
  // convert to floats in range [0 -> 1)
  // A float can't precisely represent the full range of an int32_t.
  // A float's range retaining integer precision is -16777216 -> 16777215
  // Rather than dropping bits to just 25bits, we instead drop
  // all the way down to just 16bits of randomness, but double the amount
  // of floats generated.
  //----------------------------------------------------------------------------
  void m128iToScaledFloat(const __m128i& input, float out[NUM_GENERATED])
  {
    static const __m128i mask     = _mm_set1_epi32(0x0000FFFF);
    static const __m128 RANGE     = _mm_set1_ps(65535.f + 1.f);
    static const __m128 INV_RANGE = _mm_div_ps(_mm_set1_ps(1.f), RANGE);

    __m128i rightSide      = _mm_and_si128(mask, input);
    __m128 rRealConversion = _mm_cvtepi32_ps(rightSide);
    rRealConversion        = _mm_mul_ps(rRealConversion, INV_RANGE);
    _mm_store_ps(out, rRealConversion);

    __m128i leftSide       = _mm_srli_epi32(input, 16);
    __m128 lRealConversion = _mm_cvtepi32_ps(leftSide);
    lRealConversion        = _mm_mul_ps(lRealConversion, INV_RANGE);
    _mm_store_ps(out + 4, lRealConversion);
  }

  //----------------------------------------------------------------------------
  // Generate random numbers and store the results in a user provided buffer
  //----------------------------------------------------------------------------
  void generate(float result[NUM_GENERATED])
  {
    static const __m128i MAGIC_CONST = _mm_set1_epi32(0x9E3779B9);
    __m128i randomBits = _mm_aesenc_si128(m_curRoundKey, m_curRoundKey);
    m_curRoundKey      = _mm_add_epi32(m_curRoundKey, MAGIC_CONST);

    m128iToScaledFloat(randomBits, result);
  }

  //----------------------------------------------------------------------------
  // Generate random numbers and store the results internally
  //----------------------------------------------------------------------------
  void generate()
  {
    generate(m_internalBuffer);
  }

  //----------------------------------------------------------------------------
  float get(size_t index)
  {
    assert(index < NUM_GENERATED);
    return m_internalBuffer[index];
  }

  //----------------------------------------------------------------------------
  __m128i m_curRoundKey;
  float m_internalBuffer[NUM_GENERATED] = {};
};

//------------------------------------------------------------------------------
