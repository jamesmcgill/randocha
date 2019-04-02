#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cassert>

#include "randocha.h"
#include "rand_sse.h"
#include "rand_tea.h"
//------------------------------------------------------------------------------
// Sources:
// Intel's benchmarking whitepaper
// https://www.intel.com/content/dam/www/public/us/en/documents/white-papers/ia-32-ia-64-benchmark-code-execution-paper.pdf
//
// Standard Deviation (Statistics)
// https://www.mathsisfun.com/data/standard-deviation.html
//
//------------------------------------------------------------------------------
static const size_t NUM_ENSEMBLES = 100;
static const size_t NUM_SAMPLES   = 10000;

//------------------------------------------------------------------------------
using Durations     = std::vector<uint64_t>;
using RandomNumbers = std::vector<float>;
using ReturnValues  = float[Randocha::NUM_GENERATED];

struct Results
{
  uint64_t avgDuration         = 0;
  uint64_t avgMinDuration      = 0;
  uint64_t avgVariance         = 0;
  uint64_t maxDeviationRange   = 0;
  uint64_t varianceOfVariances = 0;
  uint64_t varianceOfMins      = 0;
  uint64_t numOutliers         = 0;
  double varianceDeviation     = 0.0;

  std::vector<RandomNumbers> randomNumbers
    = std::vector<RandomNumbers>(NUM_ENSEMBLES);

  std::vector<Durations> durations   = std::vector<Durations>(NUM_ENSEMBLES);
  std::vector<uint64_t> maxDurations = std::vector<uint64_t>(NUM_ENSEMBLES);
  std::vector<uint64_t> minDurations = std::vector<uint64_t>(NUM_ENSEMBLES);
  std::vector<uint64_t> avgDurations = std::vector<uint64_t>(NUM_ENSEMBLES);

  std::vector<uint64_t> variances       = std::vector<uint64_t>(NUM_ENSEMBLES);
  std::vector<uint64_t> deviations      = std::vector<uint64_t>(NUM_ENSEMBLES);
  std::vector<uint64_t> deviationRanges = std::vector<uint64_t>(NUM_ENSEMBLES);
};

//------------------------------------------------------------------------------
bool
checkCpuSupport()
{
  int cpuInfo[4];

  __cpuid(cpuInfo, 0);
  const int numIds = cpuInfo[0];

  __cpuid(cpuInfo, 0x80000000);
  const int numExtendedIds = cpuInfo[0];

  if (numExtendedIds < 0x80000001)
  {
    std::cerr << "Extended functions (like RDTSCP) not supported!" << std::endl;
    return false;
  }

  __cpuid(cpuInfo, 0x80000001);
  if (!(cpuInfo[3] & 0x08000000))    // check bit at index 27 in EDX is set
  {
    std::cerr << "RDTSCP not supported!" << std::endl;
    return false;
  }

  std::cout << "RDTSCP supported" << std::endl;
  return true;
}

//------------------------------------------------------------------------------
template <typename Func>
Results
runBenchmark(Func _funcToBenchmark)
{
  Results results;

  uint64_t start = 0;
  uint64_t end   = 0;

  uint32_t aux;
  int cpuInfo[4];

  // Warm-up
  __cpuid(cpuInfo, 0);
  start = __rdtsc();
  __cpuid(cpuInfo, 0);
  end = __rdtscp(&aux);
  __cpuid(cpuInfo, 0);
  start = __rdtsc();

  // Run benchmarks
  for (size_t i = 0; i < NUM_ENSEMBLES; ++i)
  {
    RandomNumbers randomNumbers(NUM_SAMPLES * Randocha::NUM_GENERATED);
    Durations durations(NUM_SAMPLES);
    ReturnValues values;
    for (size_t j = 0; j < NUM_SAMPLES; ++j)
    {
      __cpuid(cpuInfo, 0);    // Halt until all previous instructions completed
      start = __rdtsc();

      _funcToBenchmark(values);

      end = __rdtscp(&aux);    // Timestamp taken only after benchmark done
      __cpuid(cpuInfo, 0);    // Prevent further execution until timestamp taken

      // store results
      for (size_t r = 0; r < Randocha::NUM_GENERATED; ++r)
      {
        randomNumbers[(j * Randocha::NUM_GENERATED) + r] = values[r];
      }

      uint64_t duration = (end > start) ? end - start : 0;
      durations[j]      = duration;
    }
    results.randomNumbers[i] = std::move(randomNumbers);
    results.durations[i]     = std::move(durations);
  }

  return results;
}

//------------------------------------------------------------------------------
uint64_t
calculateMean(std::vector<uint64_t> values)
{
  assert(!values.empty());
  return std::accumulate(std::begin(values), std::end(values), 0ULL)
         / values.size();
}

//------------------------------------------------------------------------------
uint64_t
calculateVariance(
  std::vector<uint64_t> values,
  uint64_t meanValue,
  bool fromPartialSamples = false)
{
  if (values.size() <= 1)
  {
    return 0;
  }

  uint64_t squaredTotals = 0;
  for (auto& value : values)
  {
    uint64_t difference
      = (value > meanValue) ? value - meanValue : meanValue - value;

    squaredTotals += difference * difference;
  }

  // Divide by N-1 when using sample of data (rather than full population)
  assert(values.size() > 1);
  size_t denom = (fromPartialSamples) ? values.size() - 1 : values.size();
  return squaredTotals / denom;
}

//------------------------------------------------------------------------------
uint64_t
calculateVariance(std::vector<uint64_t> values, bool fromPartialSamples = false)
{
  return calculateVariance(values, calculateMean(values), fromPartialSamples);
}

//------------------------------------------------------------------------------
void
calculateVarianceInfo(Results& results)
{
  for (size_t i = 0; i < NUM_ENSEMBLES; ++i)
  {
    // Calculate mean, min, max durations
    uint64_t total       = 0;
    uint64_t maxDuration = 0;
    uint64_t minDuration = UINT64_MAX;

    auto& durations = results.durations[i];
    for (auto& duration : durations)
    {
      total += duration;
      if (duration < minDuration)
      {
        minDuration = duration;
      }
      if (duration > maxDuration)
      {
        maxDuration = duration;
      }
    }    // for durations

    assert(!durations.empty());
    const uint64_t mean = total / durations.size();

    results.avgDurations[i]    = mean;
    results.maxDurations[i]    = maxDuration;
    results.minDurations[i]    = minDuration;
    results.deviationRanges[i] = maxDuration - minDuration;

    results.variances[i]  = calculateVariance(durations, mean, true);
    results.deviations[i] = static_cast<uint64_t>(sqrt(results.variances[i]));
  }    // for ensembles

  results.varianceOfMins = calculateVariance(results.minDurations);

  // Remove the outlier results
  // Copy the data into new vectors skipping the outliers
  std::vector<uint64_t> filteredVariances;
  std::vector<uint64_t> filteredRanges;
  std::vector<uint64_t> filteredMinDurations;
  std::vector<uint64_t> filteredAvgDurations;
  filteredVariances.reserve(NUM_ENSEMBLES);
  filteredRanges.reserve(NUM_ENSEMBLES);
  filteredMinDurations.reserve(NUM_ENSEMBLES);
  filteredAvgDurations.reserve(NUM_ENSEMBLES);

  assert(results.deviations.size() == NUM_ENSEMBLES);
  uint64_t numOutliers = 0;
  for (size_t i = 0; i < NUM_ENSEMBLES; ++i)
  {
    static const uint64_t DEVIATION_LIMIT = 3;
    while (i < NUM_ENSEMBLES && results.deviations[i] > DEVIATION_LIMIT)
    {
      ++i;    // skip and don't copy these ones
      ++numOutliers;
    }
    if (i >= NUM_ENSEMBLES)
    {
      break;
    }

    filteredVariances.push_back(results.variances[i]);
    filteredRanges.push_back(results.deviationRanges[i]);
    filteredMinDurations.push_back(results.minDurations[i]);
    filteredAvgDurations.push_back(results.avgDurations[i]);
  }
  results.numOutliers = numOutliers;

  if (
    filteredVariances.empty() || filteredRanges.empty()
    || filteredMinDurations.empty() || filteredAvgDurations.empty())
  {
    std::cerr << "All results were outliers. Results will be empty!\n";
    return;
  }

  // Perform calculations on filtered results
  results.avgDuration    = calculateMean(filteredAvgDurations);
  results.avgMinDuration = calculateMean(filteredMinDurations);
  results.avgVariance    = calculateMean(filteredVariances);
  results.maxDeviationRange
    = *std::max_element(std::begin(filteredRanges), std::end(filteredRanges));

  results.varianceOfVariances
    = calculateVariance(filteredVariances, results.avgVariance);
  results.varianceDeviation = sqrt(results.varianceOfVariances);
}

//------------------------------------------------------------------------------
void
printResults(Results& results)
{
  for (size_t i = 0; i < NUM_ENSEMBLES; ++i)
  {
    std::cout << i << ":->  mean: " << results.avgDurations[i]
              << ", min: " << results.minDurations[i]
              << ", max: " << results.maxDurations[i]
              << ", variance: " << results.variances[i]
              << ", deviation: " << results.deviations[i]
              << ", maxDev: " << results.deviationRanges[i] << "\n";
  }    // for NUM_ENSEMBLES
  std::cout << "\n";
}

//------------------------------------------------------------------------------
void
printSummary(Results& results)
{
  std::cout << "Total number of rounds: " << NUM_ENSEMBLES << "\n";

  std::cout << "\n";
  std::cout << "Benchmark Results:\n";
  std::cout << "==================\n";
  std::cout << "average duration: " << results.avgDuration << "\n"
            << "average min duration: " << results.avgMinDuration << "\n";

  std::cout << "\n";
  std::cout << "Quality of Results:\n";
  std::cout << "===================\n";
  std::cout << "number of outlier rounds removed: " << results.numOutliers
            << "\n"
            << "average variance: " << results.avgVariance << " (error: +/-"
            << sqrt(results.avgVariance) << " cycles)\n"
            << "absolute max deviation: " << results.maxDeviationRange << "\n"
            << "variance of variances: " << results.varianceOfVariances
            << " (error: +/-" << results.varianceDeviation << " cycles)\n"
            << "variance of min values: " << results.varianceOfMins << "\n"
            << "\n"
            << std::flush;
}

//------------------------------------------------------------------------------
int
main()
{
  if (!checkCpuSupport())
  {
    return 1;
  }

  // Baseline benchmark, with no instructions
  Results baselineResults = runBenchmark([](ReturnValues& values) {
    values[0] = 1.0f;
    values[1] = 2.0f;
    values[2] = 3.0f;
    values[3] = 4.0f;
  });
  calculateVarianceInfo(baselineResults);
  printResults(baselineResults);
  printSummary(baselineResults);

  Randocha rand;
  Results randochaResults = runBenchmark([&rand](ReturnValues& values) {
    rand.generate();
    values[0] = rand.get(0);
    values[1] = rand.get(1);
    values[2] = rand.get(2);
    values[3] = rand.get(3);
  });
  calculateVarianceInfo(randochaResults);
  printResults(randochaResults);
  printSummary(randochaResults);

  assert(
    Randocha::NUM_GENERATED == RandSSE::NUM_GENERATED
    || "Can't use ReturnValues for the SSE Benchmark as it has a different output size");
  RandSSE randSseGen;
  Results sseResults = runBenchmark(
    [&randSseGen](ReturnValues& values) { randSseGen.rand_sse(values); });
  calculateVarianceInfo(sseResults);
  printResults(sseResults);
  printSummary(sseResults);

  assert(
    Randocha::NUM_GENERATED == RandTea::NUM_GENERATED
    || "Can't use ReturnValues for the TEA Benchmark as it has a different output size");
  RandTea randTeaGen;
  Results teaResults = runBenchmark([&randTeaGen](ReturnValues& values) {
    randTeaGen.generate();
    values[0] = randTeaGen.getF(0);
    values[1] = randTeaGen.getF(1);
    values[2] = randTeaGen.getF(2);
    values[3] = randTeaGen.getF(3);
  });
  calculateVarianceInfo(teaResults);
  printResults(teaResults);
  printSummary(teaResults);

  return 0;
}

//------------------------------------------------------------------------------
