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

using lightrat::Rational;

template <bool Infinity, bool NaN>
void divide_and_invert(benchmark::State& state) {
  const Rational<Infinity, NaN> r1{get_int<std::int64_t>(state.range(0)), get_int<std::uint64_t>(state.range(1)) + 1};
  mpq_class q;
  mpq_set_i64(q.get_mpq_t(), get_int<std::int64_t>(state.range(2)), get_int<std::uint64_t>(state.range(3)) + 1);
  for (auto _ : state) {
    Rational<Infinity, NaN> res_left = r1 / q;              // r1 / q
    Rational<Infinity, NaN> res_right = (r1 / q).invert();  // q / r1
    bool res = res_left == res_right.invert();
    benchmark::DoNotOptimize(res);
  }
  state.SetComplexityN(r1.size() + Rational{q}.size());
}
template <bool Infinity, bool NaN>
void convert_and_divide(benchmark::State& state) {
  const Rational<Infinity, NaN> r1{get_int<std::int64_t>(state.range(0)), get_int<std::uint64_t>(state.range(1)) + 1};
  mpq_class q;
  mpq_set_i64(q.get_mpq_t(), get_int<std::int64_t>(state.range(2)), get_int<std::uint64_t>(state.range(3)) + 1);
  for (auto _ : state) {
    Rational<Infinity, NaN> res_left = r1 / q;                            // r1 / q
    Rational<Infinity, NaN> res_right = Rational<Infinity, NaN>{q} / r1;  // q / r1
    bool res = res_left == res_right.invert();
    benchmark::DoNotOptimize(res);
  }
  state.SetComplexityN(r1.size() + Rational{q}.size());
}

BENCHMARK(divide_and_invert<true, true>)->RANGES;
BENCHMARK(convert_and_divide<true, true>)->RANGES;
