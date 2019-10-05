#include "randocha.h"
#include "rand_sse.h"
#include "rand_tea.h"
#include "rand_mt.h"

#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cassert>
#include <cmath>

// Kernel Module
// allows disabling preemption and interrupts during benchmarking
#if AS_KERNEL_MODULE
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/hardirq.h>
#include <linux/preempt.h>
#include <linux/sched.h>
#endif

//------------------------------------------------------------------------------
// Sources:
// Intel's benchmarking whitepaper
// https://www.intel.com/content/dam/www/public/us/en/documents/white-papers/ia-32-ia-64-benchmark-code-execution-paper.pdf
//
// Standard Deviation (Statistics)
// https://www.mathsisfun.com/data/standard-deviation.html
//
//------------------------------------------------------------------------------
#define UNREFERENCED_PARAMETER(P) (P)

constexpr size_t NUM_SAMPLES = 100;
constexpr size_t NUM_ROUNDS  = 1000;

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
    = std::vector<RandomNumbers>(NUM_ROUNDS);

  std::vector<Durations> durations   = std::vector<Durations>(NUM_ROUNDS);
  std::vector<uint64_t> maxDurations = std::vector<uint64_t>(NUM_ROUNDS);
  std::vector<uint64_t> minDurations = std::vector<uint64_t>(NUM_ROUNDS);
  std::vector<uint64_t> avgDurations = std::vector<uint64_t>(NUM_ROUNDS);

  std::vector<uint64_t> variances       = std::vector<uint64_t>(NUM_ROUNDS);
  std::vector<uint64_t> deviations      = std::vector<uint64_t>(NUM_ROUNDS);
  std::vector<uint64_t> deviationRanges = std::vector<uint64_t>(NUM_ROUNDS);
};

//------------------------------------------------------------------------------
#if _MSC_VER
static bool
isRdtscpSupported()
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
  if (!(cpuInfo[3] & 0x08000000))    // check bit at INDEX 27 in EDX is set
  {
    std::cerr << "RDTSCP not supported!" << std::endl;
    return false;
  }

  std::cout << "RDTSCP supported" << std::endl;
  return true;
}

//------------------------------------------------------------------------------
#else
static bool
isRdtscpSupported()
{
  unsigned int sig;
  const int numExtendedIds = __get_cpuid_max(0x80000000, &sig);
  if (numExtendedIds < 0x80000001)
  {
    std::cerr << "Extended functions (like RDTSCP) not supported!" << std::endl;
    return false;
  }

  unsigned int eax, ebx, ecx, edx;
  __get_cpuid(0x80000001, &eax, &ebx, &ecx, &edx);
  if (!(edx & 0x08000000))
  {
    std::cerr << "RDTSCP not supported!" << std::endl;
    return false;
  }

  std::cout << "RDTSCP supported" << std::endl;
  return true;
}
#endif

//------------------------------------------------------------------------------
#if _MSC_VER
static inline void
startTiming(uint64_t& startTs, int cpuInfo[4], unsigned long irqFlags)
{
  UNREFERENCED_PARAMETER(irqFlags);

  __cpuid(cpuInfo, 0);    // Halt until all previous instructions completed
  startTs = __rdtsc();
}

//------------------------------------------------------------------------------
#else
static inline void
startTiming(uint64_t& startTs, int cpuInfo[4], unsigned long irqFlags)
{
  UNREFERENCED_PARAMETER(irqFlags);

#if AS_KERNEL_MODULE
  preempt_disable();
  raw_local_irq_save(irqFlags);
#endif

  // Halt until all previous instructions completed
  __asm__ __volatile__("cpuid" : : : "rax", "rbx", "rcx", "rdx");
  startTs = __rdtsc();
}
#endif

//------------------------------------------------------------------------------
#if _MSC_VER
static inline void
stopTiming(
  uint64_t& endTs, int cpuInfo[4], uint32_t& aux, unsigned long irqFlags)
{
  UNREFERENCED_PARAMETER(irqFlags);

  endTs = __rdtscp(&aux);    // Timestamp taken only after benchmark done
  __cpuid(cpuInfo, 0);       // Prevent further execution until timestamp taken
}

//------------------------------------------------------------------------------
#else
static inline void
stopTiming(
  uint64_t& endTs, int cpuInfo[4], uint32_t& aux, unsigned long irqFlags)
{
  UNREFERENCED_PARAMETER(irqFlags);

  endTs = __rdtscp(&aux);    // Timestamp taken only after benchmark done

  // Prevent further execution until timestamp taken
  __asm__ __volatile__("cpuid" : : : "rax", "rbx", "rcx", "rdx");

#if AS_KERNEL_MODULE
  raw_local_irq_restore(irqFlags);
  preempt_enable();
#endif
}
#endif

//------------------------------------------------------------------------------
#if _MSC_VER
static inline void
warmup(uint64_t& startTs, uint64_t& endTs, int cpuInfo[4], uint32_t& aux)
{
  __cpuid(cpuInfo, 0);
  startTs = __rdtsc();
  __cpuid(cpuInfo, 0);
  endTs = __rdtscp(&aux);
  __cpuid(cpuInfo, 0);
  startTs = __rdtsc();
}

//------------------------------------------------------------------------------
#else
static inline void
warmup(uint64_t& startTs, uint64_t& endTs, int cpuInfo[4], uint32_t& aux)
{
  __asm__ __volatile__("cpuid" : : : "rax", "rbx", "rcx", "rdx");
  startTs = __rdtsc();
  __asm__ __volatile__("cpuid" : : : "rax", "rbx", "rcx", "rdx");
  endTs = __rdtscp(&aux);
  __asm__ __volatile__("cpuid" : : : "rax", "rbx", "rcx", "rdx");
  startTs = __rdtsc();
}
#endif

//------------------------------------------------------------------------------
template <typename Func>
static Results
runBenchmark(Func _funcToBenchmark)
{
  Results results;

  uint64_t start = 0;
  uint64_t end   = 0;

  unsigned long irqFlags = 0L;
  uint32_t aux;
  int cpuInfo[4];
  warmup(start, end, cpuInfo, aux);

  // Run benchmarks
  for (size_t i = 0; i < NUM_ROUNDS; ++i)
  {
    RandomNumbers randomNumbers(NUM_SAMPLES * Randocha::NUM_GENERATED);
    Durations durations(NUM_SAMPLES);
    ReturnValues values;
    for (size_t j = 0; j < NUM_SAMPLES; ++j)
    {
      startTiming(start, cpuInfo, irqFlags);

      _funcToBenchmark(values);

      stopTiming(end, cpuInfo, aux, irqFlags);

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
static uint64_t
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
static uint64_t
calculateVariance(std::vector<uint64_t> values, bool fromPartialSamples = false)
{
  return calculateVariance(values, calculateMean(values), fromPartialSamples);
}

//------------------------------------------------------------------------------
static void
calculateVarianceInfo(Results& results)
{
  for (size_t i = 0; i < NUM_ROUNDS; ++i)
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
  filteredVariances.reserve(NUM_ROUNDS);
  filteredRanges.reserve(NUM_ROUNDS);
  filteredMinDurations.reserve(NUM_ROUNDS);
  filteredAvgDurations.reserve(NUM_ROUNDS);

  assert(results.deviations.size() == NUM_ROUNDS);
  uint64_t numOutliers = 0;
  for (size_t i = 0; i < NUM_ROUNDS; ++i)
  {
    static const uint64_t DEVIATION_LIMIT = 3;
    while (i < NUM_ROUNDS && results.deviations[i] > DEVIATION_LIMIT)
    {
      ++i;    // skip and don't copy these ones
      ++numOutliers;
    }
    if (i >= NUM_ROUNDS)
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
static void
printResults(Results& results)
{
  for (size_t i = 0; i < NUM_ROUNDS; ++i)
  {
    std::cout << i << ":->  mean: " << results.avgDurations[i]
              << ", min: " << results.minDurations[i]
              << ", max: " << results.maxDurations[i]
              << ", variance: " << results.variances[i]
              << ", deviation: " << results.deviations[i]
              << ", maxDev: " << results.deviationRanges[i] << "\n";
  }    // for NUM_ROUNDS
  std::cout << "\n";
}

//------------------------------------------------------------------------------
static void
printSummary(Results& results)
{
  std::cout << "Benchmark Results:\n";
  std::cout << "==================\n";
  std::cout << "average duration: " << results.avgDuration << "\n"
            << "average min duration: " << results.avgMinDuration << "\n";

  std::cout << "\n";
  std::cout << "Quality of Results:\n";
  std::cout << "===================\n";
  std::cout << "number of outlier rounds removed: " << results.numOutliers
            << " (from " << NUM_ROUNDS << ")\n"
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
  if (!isRdtscpSupported())
  {
    return 1;
  }

  if (!randocha__isAesSupported())
  {
    std::cout << "AES-NI not supported on this CPU. Terminating.\n";
    return 1;
  }
  else
  {
    std::cout << "AES-NI supported\n";
  }

  // Baseline benchmark, with no instructions
  Results baselineResults = runBenchmark([](ReturnValues& values) {
    values[0] = 0.0f;
    values[1] = 1.0f;
    values[2] = 2.0f;
    values[3] = 3.0f;
    values[4] = 4.0f;
    values[5] = 5.0f;
    values[6] = 6.0f;
    values[7] = 7.0f;
  });
  calculateVarianceInfo(baselineResults);
  std::cout << "\n\n";
  std::cout << "Baseline\n";
  std::cout << "========\n";
  // printResults(baselineResults);
  printSummary(baselineResults);

  Randocha rand;
  Results randochaResults
    = runBenchmark([&rand](ReturnValues& values) { rand.generate(values); });
  calculateVarianceInfo(randochaResults);
  std::cout << "\n\n";
  std::cout << "Randocha\n";
  std::cout << "========\n";
  // printResults(randochaResults);
  printSummary(randochaResults);

  static_assert(
    Randocha::NUM_GENERATED == RandSSE::NUM_GENERATED,
    "Can't use ReturnValues for the SSE Benchmark as it has a different output size");
  RandSSE randSseGen;
  Results sseResults = runBenchmark(
    [&randSseGen](ReturnValues& values) { randSseGen.rand_sse(values); });
  calculateVarianceInfo(sseResults);
  std::cout << "\n\n";
  std::cout << "SSE\n";
  std::cout << "===\n";
  // printResults(sseResults);
  printSummary(sseResults);

  static_assert(
    Randocha::NUM_GENERATED == RandTea::NUM_GENERATED * 4,
    "TEA Benchmark only generates 2 values, need to call repeatedly to match required output size");
  RandTea randTeaGen;
  Results teaResults = runBenchmark([&randTeaGen](ReturnValues& values) {
    randTeaGen.generate();
    values[0] = randTeaGen.getF(0);
    values[1] = randTeaGen.getF(1);
    randTeaGen.generate();
    values[2] = randTeaGen.getF(0);
    values[3] = randTeaGen.getF(1);
    randTeaGen.generate();
    values[4] = randTeaGen.getF(0);
    values[5] = randTeaGen.getF(1);
    randTeaGen.generate();
    values[6] = randTeaGen.getF(0);
    values[7] = randTeaGen.getF(1);
  });
  calculateVarianceInfo(teaResults);
  std::cout << "\n\n";
  std::cout << "TEA\n";
  std::cout << "===\n";
  // printResults(teaResults);
  printSummary(teaResults);

  static_assert(
    Randocha::NUM_GENERATED == RandMT::NUM_GENERATED * 8,
    "MT Benchmark only generates 1 value, need to call repeatedly to match required output size");
  RandMT randMtGen;
  Results mtResults = runBenchmark([&randMtGen](ReturnValues& values) {
    values[0] = randMtGen.generate();
    values[1] = randMtGen.generate();
    values[2] = randMtGen.generate();
    values[3] = randMtGen.generate();
    values[4] = randMtGen.generate();
    values[5] = randMtGen.generate();
    values[6] = randMtGen.generate();
    values[7] = randMtGen.generate();
  });
  calculateVarianceInfo(mtResults);
  std::cout << "\n\n";
  std::cout << "Mersenne Twister\n";
  std::cout << "================\n";
  printResults(mtResults);
  printSummary(mtResults);

  return 0;
}

//------------------------------------------------------------------------------
#if AS_KERNEL_MODULE
static int __init
bench_start(void)
{
  return main();
}

static void __exit
bench_end(void)
{
  printk(KERN_INFO "Benchmark Module exiting\n");
}
module_init(bench_start);
module_exit(bench_end);
#endif
//------------------------------------------------------------------------------
