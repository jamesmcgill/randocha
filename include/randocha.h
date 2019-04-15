#pragma once

#if _MSC_VER
#  include <intrin.h>
#elif defined(__ICC) || defined(__INTEL_COMPILER)
#  include <immintrin.h>
#else
#  include <x86intrin.h>
#  include <cpuid.h>
#endif

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
#define randocha__NUM_GENERATED 8

//------------------------------------------------------------------------------
void
randocha__init(__m128i& curRoundKey)
{
  curRoundKey = _mm_set_epi32(0xA341316C, 0xC8013EA4, 0xAD90777D, 0x7E95761E);
}

//------------------------------------------------------------------------------
__m128i
randocha__generate128i(__m128i& curRoundKey)
{
  static const __m128i MAGIC_CONST = _mm_set1_epi32(0x9E3779B9);
  __m128i randomBits               = _mm_aesenc_si128(curRoundKey, curRoundKey);
  curRoundKey                      = _mm_add_epi32(curRoundKey, MAGIC_CONST);

  return randomBits;
}

//----------------------------------------------------------------------------
// convert to floats in range [0 -> 1)
// A float can't precisely represent the full range of an int32_t.
// A float's range retaining integer precision is -16777216 -> 16777215
// Rather than dropping bits to just 25bits, we instead drop
// all the way down to just 16bits of randomness, but double the amount
// of floats generated.
//----------------------------------------------------------------------------
void
randocha__m128iToScaledFloat(
  const __m128i& input, float out[randocha__NUM_GENERATED])
{
  static const __m128i mask     = _mm_set1_epi32(0x0000FFFF);
  static const __m128 RANGE     = _mm_set1_ps(65535.f + 0.01f);
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

//------------------------------------------------------------------------------
void
randocha__generateFloat(
  __m128i& curRoundKey, float result[randocha__NUM_GENERATED])
{
  randocha__m128iToScaledFloat(randocha__generate128i(curRoundKey), result);
}

//------------------------------------------------------------------------------
#if _MSC_VER
bool
randocha__isAesSupported()
{
  int cpuInfo[4];

  __cpuid(cpuInfo, 0);
  const int numIds = cpuInfo[0];
  if (numIds < 1)
  {
    return false;
  }
  __cpuid(cpuInfo, 1);
  if (!(cpuInfo[2] & 0x2000000))    // check bit at index 25 in ECX is set
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
#elif defined(__ICC) || defined(__INTEL_COMPILER)
bool
randocha__isAesSupported()
{
  return _may_i_use_cpu_feature(_FEATURE_AES);
}

//------------------------------------------------------------------------------
#else
bool
randocha__isAesSupported()
{
  unsigned int sig;
  const int numIds = __get_cpuid_max(0, &sig);
  if (numIds < 1)
  {
    return false;
  }

  unsigned int eax, ebx, ecx, edx;
  __get_cpuid(1, &eax, &ebx, &ecx, &edx);
  if (!(ecx & bit_AES))
  {
    return false;
  }

  return true;
}
#endif
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// C++ Helper object, with internal state and some storage
// Interface only returns floats in range [0 -> 1)
//------------------------------------------------------------------------------
struct Randocha
{
  static const size_t NUM_GENERATED = randocha__NUM_GENERATED;

  Randocha() { randocha__init(m_curRoundKey); }

  //----------------------------------------------------------------------------
  // Generate random numbers and store the results in a user provided buffer
  //----------------------------------------------------------------------------
  void generate(float result[NUM_GENERATED])
  {
    randocha__generateFloat(m_curRoundKey, result);
  }

  //----------------------------------------------------------------------------
  // Generate random numbers and store the results internally
  //----------------------------------------------------------------------------
  void generate() { generate(m_internalBuffer); }

  //----------------------------------------------------------------------------
  // Because getters...
  //----------------------------------------------------------------------------
  float get(size_t index)
  {
    assert(index < NUM_GENERATED);
    return m_internalBuffer[index];
  }

  //----------------------------------------------------------------------------
  // Returns a single random float in range [0 -> 1)
  // Pulls the numbers from internal storage and only generates new random
  // numbers when it runs out. Therefore this will have uneven performance
  // characteristics -> generating random numbers only on every 8th call.
  //----------------------------------------------------------------------------
  float next()
  {
    if (++m_counter < NUM_GENERATED)
    {
      return m_internalBuffer[m_counter];
    }

    generate(m_internalBuffer);
    m_counter = 0;
    return m_internalBuffer[m_counter];
  }

  //----------------------------------------------------------------------------
  __m128i m_curRoundKey;

  size_t m_counter = NUM_GENERATED;    // force generate on first call to next()
  float m_internalBuffer[NUM_GENERATED] = {};
};

//------------------------------------------------------------------------------
