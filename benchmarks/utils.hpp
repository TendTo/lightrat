/**
 * @author TendTo (https://github.com/TendTo)
 * @copyright 2026 lightrat
 * @licence Apache License 2.0
 * @file
 * Utility functions for benchmarking lightrat.
 */
#pragma once

#include <benchmark/benchmark.h>

#include "lightrat/Rational.h"

// We need to use long, despite it being 32 bits on Windows, because the GMP API uses long for its integer types.
inline void mpq_set_i64(mpq_ptr q, const std::int64_t num, const std::uint64_t den) {
  if constexpr (sizeof(long) == sizeof(num) && sizeof(unsigned long) == sizeof(den)) {  // NOLINT(runtime/int)
    mpq_set_si(q, static_cast<long>(num), static_cast<unsigned long>(den));             // NOLINT(runtime/int)
  } else {
    mpz_ptr num_q = mpq_numref(q);
    const std::int64_t abs_num = std::abs(num);
    mpz_import(num_q, 1, 1, sizeof(num), 0, 0, &abs_num);
    if (num < 0) mpz_neg(num_q, num_q);
    mpz_import(mpq_denref(q), 1, 1, sizeof(den), 0, 0, &den);
  }
  mpq_canonicalize(q);
}

template <class T>
T get_int(std::int64_t n);
template <>
std::int64_t get_int(const std::int64_t n) {
  return (n > 0) ? std::int64_t{1} << n : -(std::int64_t{1} << std::abs(n));
}
template <>
std::uint64_t get_int(const std::int64_t n) {
  return std::uint64_t{1} << n;
}

template <bool Infinity, bool NaN, char Op>
void binary_op(benchmark::State& state) {
  const lightrat::Rational<Infinity, NaN> r1{get_int<std::int64_t>(state.range(0)),
                                             get_int<std::uint64_t>(state.range(1)) + 1};
  const lightrat::Rational<Infinity, NaN> r2{get_int<std::int64_t>(state.range(2)),
                                             get_int<std::uint64_t>(state.range(3)) + 1};
  for (auto _ : state) {
    if constexpr (Op == '+')
      benchmark::DoNotOptimize(r1 + r2);
    else if constexpr (Op == '-')
      benchmark::DoNotOptimize(r1 - r2);
    else if constexpr (Op == '*')
      benchmark::DoNotOptimize(r1 * r2);
    else if constexpr (Op == '/')
      benchmark::DoNotOptimize(r1 / r2);
    else {
      static_assert(Op == '+' || Op == '-' || Op == '*' || Op == '/', "Invalid operation");
    }
  }
  state.SetComplexityN(r1.size() + r2.size());
}

template <bool Infinity, bool NaN, char Op>
void unary_op(benchmark::State& state) {
  lightrat::Rational<Infinity, NaN> r{get_int<std::int64_t>(state.range(0)),
                                      get_int<std::uint64_t>(state.range(1)) + 1};
  for (auto _ : state) {
    if constexpr (Op == 'a')
      benchmark::DoNotOptimize(r.abs());
    else if constexpr (Op == 'i')
      benchmark::DoNotOptimize(r.invert());
    else if constexpr (Op == 'n')
      benchmark::DoNotOptimize(r.negate());
    else {
      static_assert(Op == 'a' || Op == 'i' || Op == 'n', "Invalid operation");
    }
  }
  state.SetComplexityN(r.size());
}
