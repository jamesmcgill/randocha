#include "randocha.h"
#include "rand_sse.h"
#include "rand_tea.h"
#include "rand_mt.h"

#if _MSC_VER
#define STBI_MSC_SECURE_CRT
#endif

#define STB_IMAGE_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable : 4100)    // Unreferenced parameter
#include "stb_image.h"
#pragma warning(pop)

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <numeric>
#include <algorithm>
#include <cassert>
//------------------------------------------------------------------------------
constexpr size_t NUM_FLOATS = 4'000'000;
using Results               = std::vector<float>;

constexpr size_t VIZ_RESOLUTION = 50;

constexpr int IMAGE_WIDTH  = 640;
constexpr int IMAGE_HEIGHT = 480;
constexpr int NUM_PIXELS   = IMAGE_WIDTH * IMAGE_HEIGHT;
static_assert(
  NUM_PIXELS <= NUM_FLOATS,
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
  constexpr size_t numSamples = NUM_FLOATS / Randocha::NUM_GENERATED;
  static_assert(
    (NUM_FLOATS % Randocha::NUM_GENERATED) == 0,
    "Please ensure to use an exact multiple of the number generated per call");

  Randocha rand;
  for (size_t i = 0; i < numSamples; ++i)
  {
    rand.generate(results.data() + (i * Randocha::NUM_GENERATED));
  }
}

//------------------------------------------------------------------------------
void
generateSse(Results& results)
{
  constexpr size_t numSamples = NUM_FLOATS / RandSSE::NUM_GENERATED;
  static_assert(
    (NUM_FLOATS % RandSSE::NUM_GENERATED) == 0,
    "Please ensure to use an exact multiple of the number generated per call");

  RandSSE rand;
  for (size_t i = 0; i < numSamples; ++i)
  {
    rand.rand_sse(results.data() + (i * RandSSE::NUM_GENERATED));
  }
}

//------------------------------------------------------------------------------
void
generateTea(Results& results)
{
  constexpr size_t numSamples = NUM_FLOATS / RandTea::NUM_GENERATED;
  static_assert(
    (NUM_FLOATS % RandTea::NUM_GENERATED) == 0,
    "Please ensure to use an exact multiple of the number generated per call");

  RandTea rand;
  for (size_t i = 0; i < numSamples; ++i)
  {
    rand.generate();
    results[(i * RandTea::NUM_GENERATED) + 0] = rand.getF(0);
    results[(i * RandTea::NUM_GENERATED) + 1] = rand.getF(1);
  }
}

//------------------------------------------------------------------------------
void
generateMT(Results& results)
{
  constexpr size_t numSamples = NUM_FLOATS / RandMT::NUM_GENERATED;
  static_assert(
    (NUM_FLOATS % RandMT::NUM_GENERATED) == 0,
    "Please ensure to use an exact multiple of the number generated per call");

  RandMT rand;
  for (size_t i = 0; i < numSamples; ++i)
  {
    results[i] = rand.generate();
  }
}

//------------------------------------------------------------------------------
void
vizDistribution(Results& results)
{
  std::vector<int> distributionCounts(VIZ_RESOLUTION);

  double total = 0.0;
  float min    = 2.0f;
  float max    = -2.0f;
  for (int i = 0; i < NUM_FLOATS; ++i)
  {
    assert(results[i] >= 0.0f);
    assert(results[i] < 1.0f);
    total += results[i];

    if (results[i] < min)
    {
      min = results[i];
    }
    if (results[i] > max)
    {
      max = results[i];
    }

    // Quantize the values to easier count and display
    size_t idx = static_cast<size_t>((double)results[i] * VIZ_RESOLUTION);
    distributionCounts[idx]++;
  }    // for NUM_FLOATS

  std::cout << std::fixed << std::setw(11) << std::setprecision(8);
  std::cout << "Mean: " << (total / NUM_FLOATS) << " Min: " << min
            << " Max: " << max << "\n";

  for (size_t i = 0; i < VIZ_RESOLUTION; ++i)
  {
    constexpr int MAX_WIDTH     = 80;
    constexpr int TARGET_OF_MAX = 2;    // 1/2 of max_width (if uniform)
    constexpr int SCALE
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
  if (!randocha__isAesSupported())
  {
    std::cout
      << "AES-NI instruction set not supported on this CPU. Terminating\n";
    return 1;
  }

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

  std::cout << "\n\n";
  std::cout << "Mersenne Twister\n";
  std::cout << "================\n";
  generateMT(results);
  vizDistribution(results);
  saveImage(results, "rand_mt.bmp");

  return 0;
}

//------------------------------------------------------------------------------
