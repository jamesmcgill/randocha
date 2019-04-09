#include <iostream>
#include <vector>
#include <map>
#include <numeric>
#include <algorithm>
#include <cassert>

#include "randocha.h"
#include "rand_sse.h"
#include "rand_tea.h"

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable : 4100)    // Unreferenced parameter
#include "stb_image.h"
#pragma warning(pop)

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

//------------------------------------------------------------------------------
static const size_t NUM_SAMPLES = 400000;
static const size_t NUM_FLOATS  = NUM_SAMPLES * Randocha::NUM_GENERATED;
using Results                   = std::vector<float>;

static const int VIZ_RESOLUTION = 20;

constexpr int IMAGE_WIDTH  = 640;
constexpr int IMAGE_HEIGHT = 480;
constexpr int NUM_PIXELS   = IMAGE_WIDTH * IMAGE_HEIGHT;
static_assert(
  NUM_PIXELS <= NUM_SAMPLES,
  "Must have enough random variables to fill an image");
using ImageBuffer = std::vector<uint32_t>;

//------------------------------------------------------------------------------
bool
saveImage(const Results& results, const std::string& fileName)
{
  ImageBuffer image(NUM_PIXELS);

  for (int i = 0; i < NUM_PIXELS; ++i)
  {
    const uint8_t channelVal = uint8_t(255.99f * results[i]);

    image[i] = uint32_t(
      (channelVal << 24) | (channelVal << 16) | (channelVal << 8)
      | (channelVal));

  }    // for NUM_PIXELS

  return (
    stbi_write_bmp(fileName.c_str(), IMAGE_WIDTH, IMAGE_HEIGHT, 3, image.data())
    != 0);
}

//------------------------------------------------------------------------------
void
generateRandocha(Results& results)
{
  Randocha rand;
  for (size_t i = 0; i < NUM_SAMPLES; ++i)
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
  for (size_t i = 0; i < NUM_SAMPLES; ++i)
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
  for (size_t i = 0; i < NUM_SAMPLES; ++i)
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

  for (size_t i = 0; i < VIZ_RESOLUTION; ++i)
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
  saveImage(results, "randocha.bmp");

  std::cout << "\n\n";
  std::cout << "SSE\n";
  std::cout << "========\n";
  generateSse(results);
  vizDistribution(results);
  saveImage(results, "rand_sse.bmp");

  std::cout << "\n\n";
  std::cout << "TEA\n";
  std::cout << "========\n";
  generateTea(results);
  vizDistribution(results);
  saveImage(results, "rand_tea.bmp");

  return 0;
}

//------------------------------------------------------------------------------
