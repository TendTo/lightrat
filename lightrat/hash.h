/**
 * @author TendTo (https://github.com/TendTo)
 * @copyright 2026 lightrat
 * @licence Apache License 2.0
 * @file
 * Utility hash functions.
 */
#pragma once

#include <gmp.h>
#include <gmpxx.h>

#include <bit>
#include <functional>
#include <utility>

/**
 * Hash specialization for GMP integer types.
 * The size of the mpz_t is negative for negative numbers.
 * Including it in the hash ensures that negated numbers have different hashes.
 * @see https://gmplib.org/list-archives/gmp-discuss/2021-June/006703.html
 */
template <>
struct std::hash<mpz_t> {
  size_t operator()(const mpz_t z) const noexcept {
    std::size_t h = std::hash<decltype(z->_mp_size)>{}(z->_mp_size);
    for (std::size_t i = 0; i < mpz_size(z); ++i) h ^= std::hash<mp_limb_t>()(mpz_getlimbn(z, static_cast<int>(i)));
    return h;
  }
};

/**
 * Hash specialization for GMP rational types.
 * It combines the hashes of the numerator and denominator using a bitwise XOR and a right rotation.
 */
template <>
struct std::hash<mpq_t> {
  size_t operator()(const mpq_t q) const noexcept {
    return std::hash<mpz_t>()(mpq_numref(q)) ^ std::rotr(std::hash<mpz_t>()(mpq_denref(q)), sizeof(std::size_t) / 2);
  }
};

/** Hash specialization for GMP integer types for C++ wrapper classes. */
template <>
struct std::hash<mpz_class> {
  size_t operator()(const mpz_class& z) const noexcept { return std::hash<mpz_t>()(z.get_mpz_t()); }
};

/** Hash specialization for GMP rational types for C++ wrapper classes. */
template <>
struct std::hash<mpq_class> {
  size_t operator()(const mpq_class& q) const noexcept { return std::hash<mpq_t>()(q.get_mpq_t()); }
};

/**
 * Hash specialization for pairs of int32_t and uint32_t.
 * Depending on the size of size_t, it either combines the two 32-bit integers into a single 64-bit value,
 * interweaving their bits to avoid favoring one over the other and ensuring that the hash is unique for each pair,
 * or it uses a mixing function to combine them into a size_t hash value.
 * @see https://chromium.googlesource.com/chromium/src/base/+/e0998c057ffcbb6a47f867fe7e760856a8a88bc3/hash.h
 * @see https://stackoverflow.com/a/39490836
 */
template <>
struct std::hash<std::pair<std::int32_t, std::uint32_t>> {
  size_t operator()(const std::int32_t num, const std::uint32_t den) const noexcept {
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
      return static_cast<std::size_t>(hash64 >> (8 * (sizeof(std::uint64_t) - sizeof(size_t))));
    } else {
      // From StackOverflow - https://stackoverflow.com/a/39490836
      // In principle, we could just do `return (x << 32) | y;`,
      // but by interweaving the bits we do not favor the denominator too strongly over the numerator.
      // Depending on the implementation of the hash table,
      // this could save a lot of space at the cost of a few more CPU cycles.
      for (std::size_t i = sizeof(num); i > 0; i--) {
        x = (x | (x << shift[i])) & mask[i];
        y = (y | (y << shift[i])) & mask[i];
      }
      x = (x | (x << shift[0])) & mask[0];
      y = (y | (y << shift[0])) & mask[0];

      return x | (y << 1);
    }
  }
  size_t operator()(const std::pair<std::int32_t, std::uint32_t>& p) const noexcept {
    return (*this)(p.first, p.second);
  }
};
