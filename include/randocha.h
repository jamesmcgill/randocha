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
//    This will generate 8 random numbers between [0.0f -> 1.0f)
// 3) The Randocha object holds these floats as internal member variables.
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
// float f4 = rand.get(4);
// float f5 = rand.get(5);
// float f6 = rand.get(6);
// float f7 = rand.get(7);
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
// Implement on GCC/Clang
//
// Make it truly header-only
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
