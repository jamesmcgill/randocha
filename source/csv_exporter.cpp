#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>

#include "randocha.h"

constexpr size_t NUM_FLOATS  = 100'000;
constexpr size_t NUM_SAMPLES = NUM_FLOATS / Randocha::NUM_GENERATED;
static_assert(
  (NUM_FLOATS % Randocha::NUM_GENERATED) == 0,
  "Please ensure that the buffer size will be an exact multiple of the number generated per call");

//------------------------------------------------------------------------------
int
main()
{
  std::vector<float> results(NUM_FLOATS);

  Randocha rand;
  for (size_t i = 0; i < NUM_SAMPLES; ++i)
  {
    rand.generate(results.data() + (i * Randocha::NUM_GENERATED));
  }

  std::ofstream file("random_numbers.csv");
  file << std::fixed << std::setprecision(9);
  for (float f : results)
  {
    file << f << "\n";
  }

  return 0;
}

//------------------------------------------------------------------------------
