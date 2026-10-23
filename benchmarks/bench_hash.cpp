/**
 * @author TendTo (https://github.com/TendTo)
 * @copyright 2026 lightrat
 * @licence Apache License 2.0
 * @file
 */
#include <benchmark/benchmark.h>

#include <array>

#include "benchmarks/utils.hpp"

#define RANGES                                                                    \
  Args({-100, 100, 1, 100})                                                       \
      ->Args({std::int64_t{1} << 40, (std::int64_t{1} << 40) + 100, 13, 113})     \
      ->Args({-(std::int64_t{1} << 40), -(std::int64_t{1} << 40) + 100, 13, 113}) \
      ->Complexity(benchmark::o1)

using lightrat::Rational;

template <bool Infinity, bool NaN>
struct opensmt_hash {
  static std::size_t hash(const mpq_t q) noexcept {
    std::uint32_t h_n = 2166136261U;
    for (int i = 0; i < q->_mp_num._mp_size; i++) {
      h_n *= 16777619U;
      h_n ^= q->_mp_num._mp_d[i];
    }
    std::uint32_t h_d = 2166136261U;
    for (int i = 0; i < q->_mp_den._mp_size; i++) {
      h_d *= 16777619U;
      h_d ^= q->_mp_den._mp_d[i];
    }
    return h_n + h_d;
  }

  std::size_t operator()(const Rational<Infinity, NaN>& r) const noexcept {
    if (r.is_nan()) return 0;
    if (r.is_infinity()) return 1;
    if (r.is_ninfinity()) return 2;
    // From OpenSMT
    if (r.is_rat32()) return 37 * static_cast<std::uint32_t>((*r).s.num) + 13 * static_cast<std::uint32_t>((*r).s.den);
    assert(r.is_ratgmp());
    return hash(r.get_gmp());
  }
};

template <bool Infinity, bool NaN>
struct gmp_hash {
  static constexpr std::size_t pi_size_t() noexcept {
    if constexpr (sizeof(std::size_t) == 4) {
      return 0xc90fdaa2;  // floor(pi/4 * 2^32)
    }
    if constexpr (sizeof(std::size_t) == 8) {
      return 0xc90fdaa22168c234;  // floor(pi/4 * 2^64)
    }
    static_assert(sizeof(std::size_t) == 4 || sizeof(std::size_t) == 8, "Unsupported size_t size");
  }

  static std::size_t scramble(std::size_t v) { return v ^ (pi_size_t() + (v << 6) + (v >> 2)); }

  static std::size_t hash(const mpz_t z) noexcept {
    const std::string_view view{reinterpret_cast<char*>(z->_mp_d), std::abs(z->_mp_size) * sizeof(mp_limb_t)};
    return std::hash<std::string_view>{}(view);
  }

  static std::size_t hash(const mpq_t q) noexcept { return hash(mpq_numref(q)) ^ hash(mpq_denref(q)); }

  std::size_t operator()(const Rational<Infinity, NaN>& r) const noexcept {
    if (r.is_nan()) return 0;
    if (r.is_infinity()) return 1;
    if (r.is_ninfinity()) return 2;
    if (r.is_rat32())
      return std::hash<std::string_view>{}(std::string_view{reinterpret_cast<const char*>(&(*r).s), sizeof((*r).s)});
    assert(r.is_ratgmp());
    mpq_ptr q = r.get_gmp();

    std::size_t result = hash(q);
    // produce different hashes for negative x
    if (mpq_sgn(q) < 0) {
      result = scramble(result);
    }

    return result;
  }
};

template <bool Infinity, bool NaN>
struct simple_hash {
  static constexpr std::size_t pi() noexcept {
    if constexpr (sizeof(std::size_t) == 4) return 0x9e3779b9;
    if constexpr (sizeof(std::size_t) == 8) return 0x517cc1b727220a95;
    static_assert(sizeof(std::size_t) == 4 || sizeof(std::size_t) == 8, "Unsupported size_t size");
  }

  static std::size_t hash(const mpq_t q) noexcept {
    std::uint32_t h_n = 2166136261U;
    for (int i = 0; i < std::abs(q->_mp_num._mp_size); i++) {
      h_n *= 16777619U;
      h_n ^= q->_mp_num._mp_d[i];
    }
    std::uint32_t h_d = 2166136261U;
    for (int i = 0; i < q->_mp_den._mp_size; i++) {
      h_d *= 16777619U;
      h_d ^= q->_mp_den._mp_d[i];
    }
    return h_n + h_d;
  }

  std::size_t operator()(const Rational<Infinity, NaN>& r) const noexcept {
    if (r.is_nan()) return 0;
    if (r.is_infinity()) return 1;
    if (r.is_ninfinity()) return 2;
    if (r.is_rat32()) {
      // if (r.den() == 1) return static_cast<std::size_t>(r.num());
      auto num = (*r).s.num;
      num ^= (*r).s.den + pi() + (num << 6) + (num >> 2);
      return num;
    }
    assert(r.is_ratgmp());
    return hash(r.get_gmp());
  }
};

template <bool Infinity, bool NaN>
struct modulo_hash {
  static constexpr std::size_t pi() noexcept { return std::numeric_limits<std::size_t>::max(); }

  static std::size_t hash(const mpq_t q) noexcept {
    std::uint32_t h_n = 2166136261U;
    for (int i = 0; i < std::abs(q->_mp_num._mp_size); i++) {
      h_n *= 16777619U;
      h_n ^= q->_mp_num._mp_d[i];
    }
    std::uint32_t h_d = 2166136261U;
    for (int i = 0; i < q->_mp_den._mp_size; i++) {
      h_d *= 16777619U;
      h_d ^= q->_mp_den._mp_d[i];
    }
    return h_n + h_d;
  }

  static std::size_t inv(const std::size_t a) { return a <= 1 ? a : pi() - pi() / a * inv(pi() % a) % pi(); }

  std::size_t operator()(const Rational<Infinity, NaN>& r) const noexcept {
    if (r.is_nan()) return 0;
    if (r.is_infinity()) return 1;
    if (r.is_ninfinity()) return 2;
    if (r.is_rat32()) {
      const std::size_t mod_den = (*r).s.den % pi();
      if (mod_den == 0) return pi();
      auto mod_hash = std::abs((*r).s.num) % pi() * inv(mod_den) % pi();
      if ((*r).s.num < 0) mod_hash = std::size_t{0} - mod_hash;
      if (mod_hash == pi()) mod_hash = pi();
      return mod_hash;
    }
    assert(r.is_ratgmp());
    return hash(r.get_gmp());
  }
};

template <bool Infinity, bool NaN>
struct chrome_hash {
  static std::size_t hash(const mpq_t q) noexcept {
    std::uint32_t h_n = 2166136261U;
    for (int i = 0; i < std::abs(q->_mp_num._mp_size); i++) {
      h_n *= 16777619U;
      h_n ^= q->_mp_num._mp_d[i];
    }
    std::uint32_t h_d = 2166136261U;
    for (int i = 0; i < q->_mp_den._mp_size; i++) {
      h_d *= 16777619U;
      h_d ^= q->_mp_den._mp_d[i];
    }
    return h_n + h_d;
  }

  static std::size_t hash(const std::int32_t num, const std::uint32_t den) noexcept {
    const auto x = static_cast<std::int64_t>(num);
    const auto y = static_cast<std::uint64_t>(den);
    std::uint64_t hash64 = (x << 32) | y;
    if constexpr (sizeof(size_t) >= sizeof(std::uint64_t)) {
      return static_cast<std::size_t>(hash64);
    } else {
      constexpr std::uint64_t odd_random = 481046412LL << 32 | 1025306955LL;
      constexpr std::uint32_t shift_random = 10121U << 16;
      hash64 *= odd_random + shift_random;
      return static_cast<size_t>(hash64 >> (8 * (sizeof(std::uint64_t) - sizeof(size_t))));
    }
  }

  std::size_t operator()(const Rational<Infinity, NaN>& r) const noexcept {
    if (r.is_nan()) return 0;
    if (r.is_infinity()) return 1;
    if (r.is_ninfinity()) return 2;
    if (r.is_rat32()) return hash((*r).s.num, (*r).s.den);
    assert(r.is_ratgmp());
    return hash(r.get_gmp());
  }
};

template <bool Infinity, bool NaN>
struct interweave_hash {
  static std::size_t hash(const mpq_t q) noexcept {
    std::uint32_t h_n = 2166136261U;
    for (int i = 0; i < std::abs(q->_mp_num._mp_size); i++) {
      h_n *= 16777619U;
      h_n ^= q->_mp_num._mp_d[i];
    }
    std::uint32_t h_d = 2166136261U;
    for (int i = 0; i < q->_mp_den._mp_size; i++) {
      h_d *= 16777619U;
      h_d ^= q->_mp_den._mp_d[i];
    }
    return h_n + h_d;
  }

  static std::size_t hash(const std::int32_t num, const std::uint32_t den) noexcept {
    static constexpr std::uint64_t mask[] = {0x5555555555555555, 0x3333333333333333, 0x0F0F0F0F0F0F0F0F,
                                             0x00FF00FF00FF00FF, 0x0000FFFF0000FFFF, 0x00000000FFFFFFFF};
    static constexpr std::uint64_t shift[] = {1, 2, 4, 8, 16, 32};
    static_assert(sizeof(num) == sizeof(den), "num and den must be the same size");
    static_assert(sizeof(num) <= 5, "num and den must be at most 5 bytes");
    static_assert(sizeof(std::size_t) >= sizeof(num), "size_t must be at least as large as num");

    auto x = static_cast<std::uint64_t>(num);
    auto y = static_cast<std::uint64_t>(den);
    if constexpr (sizeof(std::size_t) == sizeof(num)) {  // Num and size_t are the same size, we need to combine them
      // From Chromium
      // https://chromium.googlesource.com/chromium/src/base/+/e0998c057ffcbb6a47f867fe7e760856a8a88bc3/hash.h
      std::uint64_t hash64 = (x << 32) | y;
      constexpr std::uint64_t odd_random = 481046412LL << 32 | 1025306955LL;
      constexpr std::uint32_t shift_random = 10121U << 16;
      hash64 *= odd_random + shift_random;
      return static_cast<size_t>(hash64 >> (8 * (sizeof(std::uint64_t) - sizeof(size_t))));
    } else {
      // From StackOverflow - https://stackoverflow.com/a/39490836
      for (std::size_t i = sizeof(num); i > 0; i--) {
        x = (x | (x << shift[i])) & mask[i];
        y = (y | (y << shift[i])) & mask[i];
      }
      x = (x | (x << shift[0])) & mask[0];
      y = (y | (y << shift[0])) & mask[0];

      return x | (y << 1);
    }
  }

  std::size_t operator()(const Rational<Infinity, NaN>& r) const noexcept {
    if (r.is_rat32()) return hash((*r).s.num, (*r).s.den);
    assert(r.is_ratgmp());
    return hash(r.get_gmp());
  }
};

template <bool Infinity, bool NaN, template <bool, bool> class hash_fun>
void hash(benchmark::State& state) {
  for (auto _ : state) {
    for (std::int64_t num = state.range(0); num <= state.range(1); ++num) {
      for (std::int64_t den = state.range(2); den <= state.range(3); ++den) {
        benchmark::DoNotOptimize(hash_fun<Infinity, NaN>{}(Rational{num, static_cast<std::uint64_t>(den)}));
      }
    }
  }
  state.SetComplexityN(std::abs(state.range(0) - state.range(1)) + std::abs(state.range(2) - state.range(3)));
}

BENCHMARK(hash<true, true, opensmt_hash>)->RANGES;
BENCHMARK(hash<true, true, gmp_hash>)->RANGES;
BENCHMARK(hash<true, true, simple_hash>)->RANGES;
BENCHMARK(hash<true, true, modulo_hash>)->RANGES;
BENCHMARK(hash<true, true, chrome_hash>)->RANGES;
BENCHMARK(hash<true, true, interweave_hash>)->RANGES;
