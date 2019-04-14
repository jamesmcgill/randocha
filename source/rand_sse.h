#pragma once

#if _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

//------------------------------------------------------------------------------
// Modified from:
// https://software.intel.com/en-us/articles/fast-random-number-generator-on-the-intel-pentiumr-4-processor/
//------------------------------------------------------------------------------
struct RandSSE
{
  static const size_t NUM_GENERATED = 8;

  RandSSE() { cur_seed = _mm_set_epi32(666, 667, 666, 667); }

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

  //----------------------------------------------------------------------------
  void rand_sse(float* result)
  {
    static const __m128i adder = _mm_setr_epi32(2531011, 10395331, 13737667, 1);
    static const __m128i multiplier
      = _mm_setr_epi32(214013, 17405, 214013, 69069);
    static const __m128i mod_mask
      = _mm_setr_epi32(0xFFFFFFFF, 0, 0xFFFFFFFF, 0);
    __m128i cur_seed_split
      = _mm_shuffle_epi32(cur_seed, _MM_SHUFFLE(2, 3, 0, 1));

    cur_seed       = _mm_mul_epu32(cur_seed, multiplier);
    __m128i mult   = _mm_shuffle_epi32(multiplier, _MM_SHUFFLE(2, 3, 0, 1));
    cur_seed_split = _mm_mul_epu32(cur_seed_split, mult);
    cur_seed       = _mm_and_si128(cur_seed, mod_mask);
    cur_seed_split = _mm_and_si128(cur_seed_split, mod_mask);
    cur_seed_split = _mm_shuffle_epi32(cur_seed_split, _MM_SHUFFLE(2, 3, 0, 1));
    cur_seed       = _mm_or_si128(cur_seed, cur_seed_split);
    cur_seed       = _mm_add_epi32(cur_seed, adder);

    m128iToScaledFloat(cur_seed, result);
  }

private:
  __m128i cur_seed;
};

//------------------------------------------------------------------------------
