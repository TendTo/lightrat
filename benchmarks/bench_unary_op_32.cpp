/**
 * @author TendTo (https://github.com/TendTo)
 * @copyright 2026 lightrat
 * @licence Apache License 2.0
 * @file
 */
#include <benchmark/benchmark.h>

#include "benchmarks/utils.hpp"

#define RANGES                                                                                 \
  ArgsProduct({benchmark::CreateDenseRange(-8, 8, 16), benchmark::CreateDenseRange(0, 16, 8)}) \
      ->Complexity(benchmark::o1);

BENCHMARK(unary_op<true, true, 'a'>)->RANGES;
BENCHMARK(unary_op<true, true, 'i'>)->RANGES;
BENCHMARK(unary_op<true, true, 'n'>)->RANGES;

BENCHMARK(unary_op<false, true, 'a'>)->RANGES;
BENCHMARK(unary_op<false, true, 'i'>)->RANGES;
BENCHMARK(unary_op<false, true, 'n'>)->RANGES;

BENCHMARK(unary_op<true, false, 'a'>)->RANGES;
BENCHMARK(unary_op<true, false, 'i'>)->RANGES;
BENCHMARK(unary_op<true, false, 'n'>)->RANGES;

BENCHMARK(unary_op<false, false, 'a'>)->RANGES;
BENCHMARK(unary_op<false, false, 'i'>)->RANGES;
BENCHMARK(unary_op<false, false, 'n'>)->RANGES;
