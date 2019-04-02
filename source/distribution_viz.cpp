#include <iostream>
#include <vector>
#include <map>
#include <numeric>
#include <algorithm>
#include <cassert>

#include "randocha.h"
#include "rand_sse.h"
#include "rand_tea.h"
//------------------------------------------------------------------------------
static const size_t NUM_SAMPLES = 100000;
static const size_t NUM_FLOATS  = NUM_SAMPLES * Randocha::NUM_GENERATED;
using Results                   = std::vector<float>;

static const int VIZ_RESOLUTION = 20;

//------------------------------------------------------------------------------
void
generateRandocha(Results& results)
{
  Randocha rand;
  for (int i = 0; i < NUM_SAMPLES; ++i)
  {
    rand.generate();
    results[(i * Randocha::NUM_GENERATED) + 0] = rand.get(0);
    results[(i * Randocha::NUM_GENERATED) + 1] = rand.get(1);
    results[(i * Randocha::NUM_GENERATED) + 2] = rand.get(2);
    results[(i * Randocha::NUM_GENERATED) + 3] = rand.get(3);
  }    // for NUM_SAMPLES
}

//------------------------------------------------------------------------------
void
generateSse(Results& results)
{
  float values[4];

  RandSSE rand;
  for (int i = 0; i < NUM_SAMPLES; ++i)
  {
    rand.rand_sse(values);
    results[(i * Randocha::NUM_GENERATED) + 0] = values[0];
    results[(i * Randocha::NUM_GENERATED) + 1] = values[1];
    results[(i * Randocha::NUM_GENERATED) + 2] = values[2];
    results[(i * Randocha::NUM_GENERATED) + 3] = values[3];
  }    // for NUM_SAMPLES
}

//------------------------------------------------------------------------------
void
generateTea(Results& results)
{
  RandTea rand;
  for (int i = 0; i < NUM_SAMPLES; ++i)
  {
    rand.generate();
    results[(i * Randocha::NUM_GENERATED) + 0] = rand.getF(0);
    results[(i * Randocha::NUM_GENERATED) + 1] = rand.getF(1);
    results[(i * Randocha::NUM_GENERATED) + 2] = rand.getF(2);
    results[(i * Randocha::NUM_GENERATED) + 3] = rand.getF(3);
  }    // for NUM_SAMPLES
}

//------------------------------------------------------------------------------
void
vizDistribution(Results& results)
{
  std::vector<int> distributionCounts(VIZ_RESOLUTION);

  for (int i = 0; i < NUM_FLOATS; ++i)
  {
    assert(results[i] >= 0.0f);
    assert(results[i] < 1.0f);

    // Quantize the values to easier count and display
    size_t idx = static_cast<size_t>(results[i] * VIZ_RESOLUTION);
    if (idx >= VIZ_RESOLUTION)    // TODO(James): Remove: Generators should not
                                  // return >=1.0f, so index should be fine
    {
      continue;
    }

    distributionCounts[idx]++;
  }    // for NUM_FLOATS

  for (int i = 0; i < VIZ_RESOLUTION; ++i)
  {
    static const int MAX_WIDTH     = 80;
    static const int TARGET_OF_MAX = 4;    // 1/4 of max_width
    static const int SCALE
      = ((NUM_FLOATS / VIZ_RESOLUTION) * TARGET_OF_MAX) / MAX_WIDTH;

    int scaledCount = distributionCounts[i] / SCALE;

    std::cout << i << "\t:";
    for (int j = 0; j < scaledCount; ++j)
    {
      std::cout << "*";
    }
    std::cout << "\n";
  }    // for VIZ_RESOLUTION
  std::cout << "\n";
}

//------------------------------------------------------------------------------
int
main()
{
  Results results(NUM_FLOATS);
  std::cout << "\n\n";
  std::cout << "Randocha\n";
  std::cout << "========\n";
  generateRandocha(results);
  vizDistribution(results);

  std::cout << "\n\n";
  std::cout << "SSE\n";
  std::cout << "========\n";
  generateSse(results);
  vizDistribution(results);

  std::cout << "\n\n";
  std::cout << "TEA\n";
  std::cout << "========\n";
  generateTea(results);
  vizDistribution(results);

  return 0;
}

//------------------------------------------------------------------------------
