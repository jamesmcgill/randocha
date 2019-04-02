#pragma once

#if _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

//------------------------------------------------------------------------------
struct RandSSE
{
  static const uint8_t NUM_GENERATED = 4;

  RandSSE() { cur_seed = _mm_set_epi32(666, 667, 666, 667); }

  // Modified from:
  // https://software.intel.com/en-us/articles/fast-random-number-generator-on-the-intel-pentiumr-4-processor/
  void rand_sse(float* result)
  {
    static const __m128i INT_MAX_VEC_MM = _mm_set1_epi32(INT_MAX);
    static const __m128 F_MAX_VEC_MM    = _mm_cvtepi32_ps(INT_MAX_VEC_MM);
    static const __m128 VEC_UNIT_MM     = _mm_set1_ps(1.0f);
    static const __m128 VEC_HALF_MM     = _mm_set1_ps(0.5f);

    __m128i adder      = _mm_setr_epi32(2531011, 10395331, 13737667, 1);
    __m128i multiplier = _mm_setr_epi32(214013, 17405, 214013, 69069);
    __m128i mod_mask   = _mm_setr_epi32(0xFFFFFFFF, 0, 0xFFFFFFFF, 0);
    __m128i sra_mask   = _mm_set1_epi32(0x00007FFF);
    __m128i cur_seed_split
      = _mm_shuffle_epi32(cur_seed, _MM_SHUFFLE(2, 3, 0, 1));

    cur_seed       = _mm_mul_epu32(cur_seed, multiplier);
    multiplier     = _mm_shuffle_epi32(multiplier, _MM_SHUFFLE(2, 3, 0, 1));
    cur_seed_split = _mm_mul_epu32(cur_seed_split, multiplier);
    cur_seed       = _mm_and_si128(cur_seed, mod_mask);
    cur_seed_split = _mm_and_si128(cur_seed_split, mod_mask);
    cur_seed_split = _mm_shuffle_epi32(cur_seed_split, _MM_SHUFFLE(2, 3, 0, 1));
    cur_seed       = _mm_or_si128(cur_seed, cur_seed_split);
    cur_seed       = _mm_add_epi32(cur_seed, adder);

    // CUSTOM CODE for returning floats in range [0 -> 1)
    __m128 realConversion = _mm_cvtepi32_ps(cur_seed);
    realConversion        = _mm_div_ps(realConversion, F_MAX_VEC_MM);
    realConversion        = _mm_add_ps(realConversion, VEC_UNIT_MM);
    realConversion        = _mm_mul_ps(realConversion, VEC_HALF_MM);

    _mm_store_ps(result, realConversion);

    return;
  }

private:
  __m128i cur_seed;
};

//------------------------------------------------------------------------------
