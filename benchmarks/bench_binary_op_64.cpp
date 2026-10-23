/**
 * @author TendTo (https://github.com/TendTo)
 * @copyright 2026 lightrat
 * @licence Apache License 2.0
 * @file
 */
#include <benchmark/benchmark.h>

#include "benchmarks/utils.hpp"

#define RANGES                                                                                    \
  ArgsProduct({benchmark::CreateDenseRange(-40, 40, 80), benchmark::CreateDenseRange(0, 62, 31),  \
               benchmark::CreateDenseRange(-40, 40, 80), benchmark::CreateDenseRange(0, 62, 31)}) \
      ->Complexity(benchmark::o1);

BENCHMARK(binary_op<true, true, '+'>)->RANGES;
BENCHMARK(binary_op<true, true, '-'>)->RANGES;
BENCHMARK(binary_op<true, true, '*'>)->RANGES;
BENCHMARK(binary_op<true, true, '/'>)->RANGES;

BENCHMARK(binary_op<false, true, '+'>)->RANGES;
BENCHMARK(binary_op<false, true, '-'>)->RANGES;
BENCHMARK(binary_op<false, true, '*'>)->RANGES;
BENCHMARK(binary_op<false, true, '/'>)->RANGES;

BENCHMARK(binary_op<true, false, '+'>)->RANGES;
BENCHMARK(binary_op<true, false, '-'>)->RANGES;
BENCHMARK(binary_op<true, false, '*'>)->RANGES;
BENCHMARK(binary_op<true, false, '/'>)->RANGES;

BENCHMARK(binary_op<false, false, '+'>)->RANGES;
BENCHMARK(binary_op<false, false, '-'>)->RANGES;
BENCHMARK(binary_op<false, false, '*'>)->RANGES;
BENCHMARK(binary_op<false, false, '/'>)->RANGES;
