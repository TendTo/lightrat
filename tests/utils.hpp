/**
 * @author TendTo (https://github.com/TendTo)
 * @copyright 2026 lightrat
 * @licence Apache License 2.0
 * @file
 * Utility functions for testing lightrat.
 */
#pragma once

#include <gmp.h>

#include <cmath>
#include <cstdint>

#define EXPECT_NAN(rat, d)     \
  EXPECT_TRUE((rat).is_nan()); \
  EXPECT_TRUE(std::isnan((d)));

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

/**
 * Check if a process exited with an error based on its exit status.
 * Used in the `EXPECT_EXIT` macro.
 * @param status The exit status of the process.
 * @return true if the process exited with an error (status > 0)
 * @return false if the process exited successfully (status == 0)
 */
inline bool ExitedWithError(const int status) { return status > 0; }
