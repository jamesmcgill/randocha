#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>

#include "randocha.h"

static const size_t NUM_SAMPLES = 10000;
//------------------------------------------------------------------------------
int
main()
{
  Randocha rand;
  std::vector<float> results(NUM_SAMPLES * Randocha::NUM_GENERATED);

  for (int i = 0; i < NUM_SAMPLES; ++i)
  {
    rand.generate();

    assert(rand.get(0) >= 0.0f);
    assert(rand.get(1) >= 0.0f);
    assert(rand.get(2) >= 0.0f);
    assert(rand.get(3) >= 0.0f);

    assert(rand.get(0) < 1.0f);
    assert(rand.get(1) < 1.0f);
    assert(rand.get(2) < 1.0f);
    assert(rand.get(3) < 1.0f);

    results[(i * Randocha::NUM_GENERATED) + 0] = rand.get(0);
    results[(i * Randocha::NUM_GENERATED) + 1] = rand.get(1);
    results[(i * Randocha::NUM_GENERATED) + 2] = rand.get(2);
    results[(i * Randocha::NUM_GENERATED) + 3] = rand.get(3);
  }    // for NUM_SAMPLES

  std::ofstream file("random_numbers.csv");
  file << std::fixed << std::setprecision(9);
  for (float f : results)
  {
    file << f << "\n";
  }

  return 0;
}

//------------------------------------------------------------------------------
