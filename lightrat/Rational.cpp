/**
 * @author TendTo (https://github.com/TendTo)
 * @copyright 2026 lightrat
 * @licence Apache License 2.0
 * @file
 */
#include "lightrat/Rational.h"

#include <cmath>
#include <limits>
#include <ostream>
#include <string>
#include <utility>

#include "lightrat/MpqStore.hpp"
#include "lightrat/errors.h"
#include "lightrat/hash.h"

namespace lightrat {

/* The test for unicity in the denominator occurs frequently. The denominator is one if it is 2 :-) */
constexpr int ONE_DEN = 0x2;
/* Number of rationals in a block */
constexpr int INF_DEN = 0;
constexpr int INF_NUM = 1;
constexpr int NINF_NUM = -1;
constexpr int NAN_NUM = 0;
constexpr int NAN_DEN = 0;

#define MAX_NUMERATOR (INT32_MAX >> 1)
#define MIN_NUMERATOR (-MAX_NUMERATOR)
#define MAX_DENOMINATOR MAX_NUMERATOR

namespace {

// We need to use long, despite it being 32-bit on Windows, because the GMP API uses long for its integer types.
void mpq_set_i64(mpq_ptr q, const std::int64_t num, const std::uint64_t den) {
  if constexpr (sizeof(long) == sizeof(num) && sizeof(unsigned long) == sizeof(den)) {  // NOLINT(runtime/int)
    mpq_set_si(q, static_cast<long>(num), static_cast<unsigned long>(den));             // NOLINT(runtime/int)
  } else {
    mpz_ptr num_q = mpq_numref(q);
    const std::int64_t abs_num = std::abs(num);
    mpz_import(num_q, 1, 1, sizeof(num), 0, 0, &abs_num);
    if (num < 0) mpz_neg(num_q, num_q);
    mpz_import(mpq_denref(q), 1, 1, sizeof(den), 0, 0, &den);
  }
}
void mpq_set_u64(mpq_ptr q, const std::uint64_t num, const std::uint64_t den) {
  if constexpr (sizeof(unsigned long) == sizeof(num) && sizeof(unsigned long) == sizeof(den)) {  // NOLINT(runtime/int)
    mpq_set_ui(q, static_cast<unsigned long>(num), static_cast<unsigned long>(den));             // NOLINT(runtime/int)
  } else {
    mpz_import(mpq_numref(q), 1, 1, sizeof(num), 0, 0, &num);
    mpz_import(mpq_denref(q), 1, 1, sizeof(den), 0, 0, &den);
  }
}

void mpq_add_si(mpq_t q, const long num, const unsigned long den) {  // NOLINT(runtime/int)
  mpz_t z0;
  mpz_init(z0);

  mpz_ptr num_q = mpq_numref(q);
  mpz_ptr den_q = mpq_denref(q);

  // special cases: den = 1 should be common
  //                num = 0 should be rare
  if (den == 1) {
    // a/b + d --> (a + bd)/b
    mpz_mul_si(z0, den_q, num);
    mpz_add(num_q, num_q, z0);

    mpz_clear(z0);
    return;
  }

  auto gcd = mpz_gcd_ui(nullptr, den_q, den);
  //  printf("gcd = %lu\n", gcd);

  if (gcd == 1) {
    // a/b + c/d  --> (a d + b c) / bd
    mpz_mul_si(z0, den_q, num);
    mpz_mul_ui(num_q, num_q, den);
    mpz_add(num_q, num_q, z0);
    mpz_mul_ui(den_q, den_q, den);

    mpz_clear(z0);
    return;
  }

  mpz_divexact_ui(den_q, den_q, gcd);  // b0 = b/gcd
  mpz_mul_si(z0, den_q, num);          // z0 = b0 * num

  mpz_mul_ui(num_q, num_q, den / gcd);
  mpz_add(num_q, num_q, z0);  // num_q := (den_q/gcd) * num + (den/gcd) * num_q

  gcd = mpz_gcd_ui(nullptr, num_q, gcd);
  if (gcd == 1) {
    mpz_mul_ui(den_q, den_q, den);
  } else {
    mpz_divexact_ui(num_q, num_q, gcd);
    mpz_mul_ui(den_q, den_q, den / gcd);
  }

  mpz_clear(z0);
}

void mpq_sub_si(mpq_t q, const long num, const unsigned long den) {  // NOLINT(runtime/int)
  mpq_add_si(q, -num, den);
}

/*
 * Multiply q by num/den
 * - num must be more than LONG_MIN
 * - den must be non-zero
 * - num and den must have no common factor
 */
void mpq_mul_si(mpq_t q, const long num, unsigned long den) {  // NOLINT(runtime/int)
  if (num == 0) {
    mpq_set_i64(q, 0, 1);
    return;
  }

  mpz_ptr num_q = mpq_numref(q);
  mpz_ptr den_q = mpq_denref(q);
  auto abs_num = static_cast<unsigned long>(std::labs(num));  // NOLINT(runtime/int)

  if (abs_num != 1) {
    const auto gcd = mpz_gcd_ui(nullptr, den_q, abs_num);
    abs_num /= gcd;
    mpz_divexact_ui(den_q, den_q, gcd);
  }

  if (den != 1) {
    const auto gcd = mpz_gcd_ui(nullptr, num_q, den);
    den /= gcd;
    mpz_divexact_ui(num_q, num_q, gcd);
  }

  mpz_mul_ui(num_q, num_q, abs_num);
  mpz_mul_ui(den_q, den_q, den);
  if (num < 0) {
    mpz_neg(num_q, num_q);
  }
}

/*
 * Divide q by num/den
 * - num must be more than LONG_MIN
 * - den must be non-zero
 * - num and den must have no common factor
 */
void mpq_div_si(mpq_t q, const long num, unsigned long den) {  // NOLINT(runtime/int)
  mpz_ptr num_q = mpq_numref(q);
  mpz_ptr den_q = mpq_denref(q);
  auto abs_num = static_cast<unsigned long>(std::labs(num));  // NOLINT(runtime/int)

  if (abs_num != 1) {
    const auto gcd = mpz_gcd_ui(nullptr, num_q, abs_num);
    abs_num /= gcd;
    mpz_divexact_ui(num_q, num_q, gcd);
  }

  if (den != 1) {
    const auto gcd = mpz_gcd_ui(nullptr, den_q, den);
    den /= gcd;
    mpz_divexact_ui(den_q, den_q, gcd);
  }

  mpz_mul_ui(num_q, num_q, den);
  mpz_mul_ui(den_q, den_q, abs_num);
  if (num < 0) {
    mpz_neg(num_q, num_q);
  }
}

}  // namespace

template <bool Infinity, bool NaN>
Rational<Infinity, NaN> Rational<Infinity, NaN>::infinity() {
  static Rational infinity{std::numeric_limits<double>::infinity()};
  return infinity;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN> Rational<Infinity, NaN>::ninfinity() {
  static Rational ninfinity{-std::numeric_limits<double>::infinity()};
  return ninfinity;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN> Rational<Infinity, NaN>::nan() {
  static Rational nan{std::numeric_limits<double>::quiet_NaN()};
  return nan;
}

template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::Rational()
    : u{.s{
#ifdef LIGHTRAT_WORDS_BIGENDIAN
          .num = 0,
          .den = ONE_DEN,
#else
          .den = ONE_DEN,
          .num = 0,
#endif
      }} {
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::Rational(const lightrat_t rat) : Rational{} {
  *this = rat;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::Rational(const std::int32_t num, const std::uint32_t den) : Rational{} {
  set(num, den);
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::Rational(const std::int64_t num, const std::uint64_t den) : Rational{} {
  set(num, den);
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::Rational(const std::string& str) : Rational{} {
  *this = str;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::Rational(const char* str) : Rational{} {
  *this = str;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::Rational(const std::int32_t num) : Rational{} {
  *this = num;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::Rational(const std::int64_t num) : Rational{} {
  *this = num;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::Rational(const std::uint32_t num) : Rational{} {
  *this = num;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::Rational(const std::uint64_t num) : Rational{} {
  *this = num;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::Rational(const double d) : Rational{} {
  *this = d;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::Rational(const mpq_t q) : Rational{} {
  *this = q;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::Rational(const mpz_t z) : Rational{} {
  *this = z;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::Rational(const mpz_t num, const mpz_t den) : Rational{} {
  set(num, den);
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::Rational(const mpq_class& q) : Rational{q.get_mpq_t()} {}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::Rational(const mpz_class& z) : Rational{z.get_mpz_t()} {}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::Rational(const mpz_class& num, const mpz_class& den) : Rational{} {
  set(num.get_mpz_t(), den.get_mpz_t());
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::Rational(const Rational& r2) : Rational{} {
  *this = r2;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::Rational(Rational&& r2) noexcept : Rational{} {
  *this = std::move(r2);
}

template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::operator=(const lightrat_t rat) {
  return *this = *reinterpret_cast<const Rational*>(rat);
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::operator=(const std::string& str) {
  return *this = str.c_str();
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::operator=(const char* const str) {
  if constexpr (Infinity) {
    if (const std::string_view str_view{str}; str_view == "-inf") {
      return *this = Rational::ninfinity();
    } else if (str_view == "inf") {
      return *this = Rational::infinity();
    }
  }
  mpq_ptr q = ensure_gmp();
  if (0 != mpq_set_str(q, str, 10)) {
    if constexpr (NaN) {
      return *this = Rational::nan();
    } else {
      throw errors::string_parsing_error(str);
    }
  }
  if constexpr (Infinity) {  // Handle special cases for infinity and NaN if the template parameter allows it
    if (mpz_sgn(mpq_numref(q)) > 0 && mpz_sgn(mpq_denref(q)) == 0) {
      return *this = Rational::infinity();
    }
    if (mpz_sgn(mpq_numref(q)) < 0 && mpz_sgn(mpq_denref(q)) == 0) {
      return *this = Rational::ninfinity();
    }
  }
  if constexpr (NaN) {  // Handle special case for NaN if the template parameter allows it
    if (mpz_sgn(mpq_numref(q)) == 0 && mpz_sgn(mpq_denref(q)) == 0) {
      return *this = Rational::nan();
    }
  }
  mpq_canonicalize(q);
  normalize();
  return *this;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::operator=(const std::int32_t num) {
  if (MIN_NUMERATOR <= num && num <= MAX_NUMERATOR) {
    clear();
    unchecked_set(num, 1);
  } else {
    mpq_ptr q = ensure_gmp();
    mpq_set_i64(q, num, 1);
  }
  return *this;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::operator=(const std::int64_t num) {
  if (MIN_NUMERATOR <= num && num <= MAX_NUMERATOR) {
    clear();
    unchecked_set(static_cast<std::int32_t>(num), 1);
  } else {
    mpq_ptr q = ensure_gmp();
    mpq_set_i64(q, num, 1);
  }
  return *this;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::operator=(const std::uint32_t num) {
  if (num <= MAX_NUMERATOR) {
    clear();
    unchecked_set(static_cast<std::int32_t>(num), 1);
  } else {
    mpq_ptr q = ensure_gmp();
    mpq_set_u64(q, num, 1);
  }
  return *this;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::operator=(const std::uint64_t num) {
  if (num <= MAX_NUMERATOR) {
    clear();
    unchecked_set(static_cast<std::int32_t>(num), 1);
  } else {
    mpq_ptr q = ensure_gmp();
    mpq_set_u64(q, num, 1);
  }
  return *this;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::operator=(const double d) {
  if (d >= std::numeric_limits<double>::infinity()) {
    set_infinity();
    return *this;
  }
  if (d <= -std::numeric_limits<double>::infinity()) {
    set_ninfinity();
    return *this;
  }
  if (std::isnan(d)) {
    set_nan();
    return *this;
  }

  mpq_ptr qt = ensure_gmp();
  mpq_set_d(qt, d);
  normalize();
  return *this;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::operator=(const mpq_class& q) {
  return *this = q.get_mpq_t();
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::operator=(const mpq_t q) {
  // If the numerator and denominator of q fit in 32-bit values,
  // we can store it as a pair of integers
  if (mpz_fits_ulong_p(mpq_denref(q)) && mpz_fits_slong_p(mpq_numref(q))) {
    const auto num = mpz_get_si(mpq_numref(q));
    const auto den = mpz_get_ui(mpq_denref(q));
    if (MIN_NUMERATOR <= num && num <= MAX_NUMERATOR && den <= MAX_DENOMINATOR) {
      clear();
      unchecked_set(static_cast<std::int32_t>(num), static_cast<std::uint32_t>(den));
      assert(is_rat32());
      return *this;
    }
  }

  // Otherwise, we need to allocate a new mpq number and copy q into it
  mpq_ptr qt = ensure_gmp();
  mpq_set(qt, q);
  assert(is_ratgmp());
  return *this;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::operator=(const mpz_class& z) {
  return *this = z.get_mpz_t();
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::operator=(const mpz_t z) {
  if (mpz_fits_slong_p(z)) {
    const auto num = mpz_get_si(z);
    if (MIN_NUMERATOR <= num && num <= MAX_NUMERATOR) {
      clear();
      unchecked_set(static_cast<std::int32_t>(num), 1);
      assert(is_rat32());
      return *this;
    }
  }

  mpq_ptr q = ensure_gmp();
  mpq_set_z(q, z);
  assert(is_ratgmp());
  return *this;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::operator=(const Rational& r2) {
  if (this == &r2) return *this;

  if (r2.is_ratgmp()) {
    mpq_ptr q1 = ensure_gmp();
    mpq_ptr q2 = r2.get_gmp();
    mpq_set(q1, q2);
  } else {
    clear();
    u = r2.u;
  }
  return *this;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::operator=(Rational&& r2) noexcept {
  if (this == &r2) return *this;
  std::swap(u, r2.u);
  return *this;
}

template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::operator+=(const Rational& r2) {
  if (nan_in_op(r2)) return *this = nan();
  if ((is_infinity() && !r2.is_ninfinity()) || (is_ninfinity() && !r2.is_infinity())) {
    // r1 is infinity, and r2 is not the opposite infinity, so the result is r1
    return *this;
  }
  if ((r2.is_infinity() && !is_ninfinity()) || (r2.is_ninfinity() && !is_infinity())) {
    // r2 is infinity, and r1 is not the opposite infinity, so the result is r2
    return *this = r2;
  }
  if (!is_finite() && !r2.is_finite()) {
    // If there is still an infinity, they must be the opposite infinity, so the result is NaN
    return *this = nan();
  }
  assert(is_finite() && r2.is_finite());

  if (u.s.den == ONE_DEN && r2.u.s.den == ONE_DEN) {
    assert(r2.is_rat32() && is_rat32());
    u.s.num += r2.u.s.num;
    if (u.s.num < MIN_NUMERATOR || u.s.num > MAX_NUMERATOR) {
      convert_to_gmp();
    }
    return *this;
  }

  if (r2.is_ratgmp()) {
    if (!is_ratgmp()) {
      convert_to_gmp();
    }
    mpq_ptr q1 = get_gmp();
    mpq_ptr q2 = r2.get_gmp();
    mpq_add(q1, q1, q2);
  } else if (is_ratgmp()) {
    mpq_ptr q1 = get_gmp();
    mpq_add_si(q1, r2.num(), r2.den());
  } else {
    const std::uint64_t d = den() * static_cast<std::uint64_t>(r2.den());
    const std::int64_t n = den() * static_cast<std::int64_t>(r2.num()) + r2.den() * static_cast<std::int64_t>(num());
    set(n, d);
  }
  return *this;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::operator-=(const Rational& r2) {
  if (nan_in_op(r2) || !is_finite() || !r2.is_finite()) return *this += -r2;
  assert(is_finite() && r2.is_finite());

  if (u.s.den == ONE_DEN && r2.u.s.den == ONE_DEN) {
    assert(is_rat32() && r2.is_rat32());
    u.s.num -= r2.u.s.num;
    if (u.s.num < MIN_NUMERATOR || u.s.num > MAX_NUMERATOR) {
      convert_to_gmp();
    }
    return *this;
  }

  if (r2.is_ratgmp()) {
    if (!is_ratgmp()) {
      convert_to_gmp();
    }
    mpq_ptr q1 = get_gmp();
    mpq_ptr q2 = r2.get_gmp();
    mpq_sub(q1, q1, q2);
  } else if (is_ratgmp()) {
    mpq_ptr q1 = get_gmp();
    mpq_sub_si(q1, r2.num(), r2.den());
  } else {
    const std::uint64_t d = den() * static_cast<std::uint64_t>(r2.den());
    const std::int64_t n = r2.den() * static_cast<std::int64_t>(num()) - den() * static_cast<std::int64_t>(r2.num());
    set(n, d);
  }
  return *this;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::operator*=(const Rational& r2) {
  if (!is_finite()) {
    if (r2.is_zero()) return *this = nan();
    return r2.is_pos() ? *this : this->negate();
  }
  if (!r2.is_finite()) {
    if (is_zero()) return *this = nan();
    return is_pos() ? (*this = r2) : (*this = -r2);
  }
  assert(is_finite() && r2.is_finite());

  if (u.s.den == ONE_DEN && r2.u.s.den == ONE_DEN) {
    assert(is_rat32() && r2.is_rat32());
    const std::int64_t n = u.s.num * static_cast<std::int64_t>(r2.u.s.num);
    if (MIN_NUMERATOR <= n && n <= MAX_NUMERATOR) {
      u.s.num = static_cast<std::int32_t>(n);
    } else {
      mpq_ptr q = ensure_gmp();
      mpq_set_i64(q, n, 1);
    }
    return *this;
  }

  if (r2.is_ratgmp()) {
    if (is_rat32()) {
      convert_to_gmp();
    }
    mpq_ptr q1 = get_gmp();
    mpq_ptr q2 = r2.get_gmp();
    mpq_mul(q1, q1, q2);
  } else if (is_ratgmp()) {
    mpq_ptr q1 = get_gmp();
    mpq_mul_si(q1, r2.num(), r2.den());
  } else {
    const std::uint64_t d = den() * static_cast<std::uint64_t>(r2.den());
    const std::int64_t n = num() * static_cast<std::int64_t>(r2.num());
    set(n, d);
  }
  return *this;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::operator/=(const Rational& r2) {
  if (nan_in_op(r2) || (is_zero() && r2.is_zero()) || (!is_finite() && !r2.is_finite())) return *this = nan();
  if (r2.is_zero()) return *this = is_pos() ? infinity() : ninfinity();  // We already know that r1 != 0
  if (is_finite() && !r2.is_finite()) return *this = 0;
  if (!is_finite()) {
    assert(!r2.is_zero());
    return r2.is_pos() ? *this : this->negate();
  }
  assert(is_finite() && r2.is_finite());

  if (r2.is_ratgmp()) {
    if (is_rat32()) {
      convert_to_gmp();
    }
    mpq_ptr q1 = get_gmp();
    mpq_ptr q2 = r2.get_gmp();
    mpq_div(q1, q1, q2);
  } else if (is_ratgmp()) {
    mpq_ptr q1 = get_gmp();
    mpq_div_si(q1, r2.num(), r2.den());
  } else if (r2.num() > 0) {
    const std::uint64_t d = den() * static_cast<std::uint64_t>(r2.num());
    const std::int64_t n = num() * static_cast<std::int64_t>(r2.den());
    set(n, d);
  } else if (r2.num() < 0) {
    const std::uint64_t d = den() * static_cast<std::uint64_t>(-r2.num());
    const std::int64_t n = num() * -static_cast<std::int64_t>(r2.den());
    set(n, d);
  } else {
    assert(r2.num() == 0);
    return *this = nan();
  }
  return *this;
}

template <bool Infinity, bool NaN>
std::size_t Rational<Infinity, NaN>::size() const {
  if (is_rat32()) return sizeof(u);

  assert(is_ratgmp());
  mpq_ptr q = get_gmp();
  const std::size_t n_limbs = mpz_size(mpq_numref(q)) + mpz_size(mpq_denref(q));
  // Convert to bytes based on your architecture's limb size
  const std::size_t n_bytes = n_limbs * sizeof(mp_limb_t);
  return sizeof(u) + n_bytes + sizeof(mpq_t);
}

template <bool Infinity, bool NaN>
Rational<Infinity, NaN> Rational<Infinity, NaN>::operator+() const {
  return *this;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN> Rational<Infinity, NaN>::operator-() const {
  Rational out(*this);
  if (out.is_ratgmp()) {
    mpq_ptr q = out.get_gmp();
    mpq_neg(q, q);
  } else {
    out.u.s.num = -out.u.s.num;
  }
  return out;
}

template <bool Infinity, bool NaN>
bool Rational<Infinity, NaN>::is_one() const {
  return (u.s.den == ONE_DEN && u.s.num == 1) || (is_ratgmp() && mpq_cmp_ui(get_gmp(), 1, 1) == 0);
}
template <bool Infinity, bool NaN>
bool Rational<Infinity, NaN>::is_minus_one() const {
  return (u.s.den == ONE_DEN && u.s.num == -1) || (is_ratgmp() && mpq_cmp_si(get_gmp(), -1, 1) == 0);
}
template <bool Infinity, bool NaN>
bool Rational<Infinity, NaN>::is_integer() const {
  return (is_rat32() && u.s.den == ONE_DEN) || (is_ratgmp() && mpz_cmp_ui(mpq_denref(get_gmp()), 1) == 0);
}
template <bool Infinity, bool NaN>
bool Rational<Infinity, NaN>::is_infinity() const {
  if constexpr (!Infinity) {
    return false;
  } else {
    return is_rat32() && u.s.den == INF_DEN && u.s.num == INF_NUM;
  }
}
template <bool Infinity, bool NaN>
bool Rational<Infinity, NaN>::is_ninfinity() const {
  if constexpr (!Infinity) {
    return false;
  } else {
    return is_rat32() && u.s.den == INF_DEN && u.s.num == NINF_NUM;
  }
}
template <bool Infinity, bool NaN>
bool Rational<Infinity, NaN>::is_nan() const {
  if constexpr (!NaN) {
    return false;
  } else {
    return is_rat32() && u.s.den == NAN_DEN && u.s.num == NAN_NUM;
  }
}

template <bool Infinity, bool NaN>
void Rational<Infinity, NaN>::convert_to_gmp() {
  assert(is_finite());
  if (is_ratgmp()) return;  // Already in GMP representation

  assert(is_rat32());
  mpq_ptr q = MpqStore::alloc();
  mpq_set_i64(q, num(), den());
  unchecked_set(q);
}
template <bool Infinity, bool NaN>
bool Rational<Infinity, NaN>::nan_in_op(const Rational& r2) const {
  return is_nan() || r2.is_nan();
}

template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::set(std::int32_t num, std::uint32_t den) {
  if (den == 0) {
    if (num > 0) return *this = infinity();
    if (num < 0) return *this = ninfinity();
    return *this = nan();
  }
  assert(den != 0);

  std::uint32_t abs_a;
  bool a_positive;

  if (num == 0 || (den == 1 && MIN_NUMERATOR <= num && num <= MAX_NUMERATOR)) {
    clear();
    unchecked_set(num, 1);
    return *this;
  }

  // absolute value and sign of a.
  if (num >= 0) {
    abs_a = static_cast<std::uint32_t>(num);
    a_positive = true;
  } else {
    abs_a = static_cast<std::uint32_t>(-num);  // Note: this works even when a = -2^31
    a_positive = false;
  }

  // abs_a and b are positive. remove powers of 2
  while (((abs_a | den) & 15) == 0) {
    abs_a >>= 4;
    den >>= 4;
  }
  switch ((abs_a | den) & 7) {
    case 0:
      abs_a >>= 3;
      den >>= 3;
      break;
    case 1:
      break;
    case 2:
      abs_a >>= 1;
      den >>= 1;
      break;
    case 3:
      break;
    case 4:
      abs_a >>= 2;
      den >>= 2;
      break;
    case 5:
      break;
    case 6:
      abs_a >>= 1;
      den >>= 1;
      break;
    default:  // Includes case 7
      break;
  }

  // abs_a and b are positive, and at least one of them is odd.
  // if abs_a <= 2 or b <= 2 then gcd = 1.
  if (abs_a > 2 && den > 2) {
    std::uint32_t a_1 = abs_a;
    std::uint32_t b_1 = den;

    // compute gcd of abs_a and b
    // loop invariant: abs_a is odd or b is odd (or both)
    for (;;) {
      if ((a_1 & 1) == 0) {
        a_1 >>= 1;
      } else if ((b_1 & 1) == 0) {
        b_1 >>= 1;
      } else if (a_1 >= b_1) {
        a_1 = (a_1 - b_1) >> 1;
        if (a_1 == 0) break;

      } else {
        b_1 = (b_1 - a_1) >> 1;
      }
    }

    // b_1 is gcd(abs_a, b)
    if (b_1 != 1) {
      abs_a /= b_1;
      den /= b_1;
    }
  }

  // abs_a and b are mutually prime and positive

  // restore a
  num = a_positive ? static_cast<std::int32_t>(abs_a) : -static_cast<std::int32_t>(abs_a);

  // assign to r
  if (abs_a <= MAX_NUMERATOR && den <= MAX_DENOMINATOR) {
    clear();
    unchecked_set(num, den);
  } else {
    mpq_ptr q = ensure_gmp();
    mpq_set_i64(q, num, den);
  }
  return *this;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::set(std::int64_t num, std::uint64_t den) {
  if (den == 0) {
    if (num > 0) return *this = infinity();
    if (num < 0) return *this = ninfinity();
    return *this = nan();
  }
  assert(den != 0);

  std::uint64_t abs_a;
  bool a_positive;

  if (num == 0 || (den == 1 && MIN_NUMERATOR <= num && num <= MAX_NUMERATOR)) {
    clear();
    unchecked_set(static_cast<std::int32_t>(num), 1);
    return *this;
  }

  // absolute value and sign of a.
  if (num >= 0) {
    abs_a = static_cast<std::uint64_t>(num);
    a_positive = true;
  } else {
    abs_a = static_cast<std::uint64_t>(-num);  // Note: this works even when a = -2^63
    a_positive = false;
  }

  // abs_a and b are positive. remove powers of 2
  while (((abs_a | den) & 15) == 0) {
    abs_a >>= 4;
    den >>= 4;
  }
  switch ((abs_a | den) & 7) {
    case 0:
      abs_a >>= 3;
      den >>= 3;
      break;
    case 1:
      break;
    case 2:
      abs_a >>= 1;
      den >>= 1;
      break;
    case 3:
      break;
    case 4:
      abs_a >>= 2;
      den >>= 2;
      break;
    case 5:
      break;
    case 6:
      abs_a >>= 1;
      den >>= 1;
      break;
    default:  // Includes case 7
      break;
  }

  // abs_a and b are positive, and at least one of them is odd.
  // if abs_a <= 2 or b <= 2 then gcd = 1.
  if (abs_a > 2 && den > 2) {
    std::uint64_t a_1 = abs_a;
    std::uint64_t b_1 = den;

    // compute gcd of abs_a and b
    // loop invariant: abs_a is odd or b is odd (or both)
    for (;;) {
      if ((a_1 & 1) == 0) {
        a_1 >>= 1;
      } else if ((b_1 & 1) == 0) {
        b_1 >>= 1;
      } else if (a_1 >= b_1) {
        a_1 = (a_1 - b_1) >> 1;
        if (a_1 == 0) break;
      } else {
        b_1 = (b_1 - a_1) >> 1;
      }
    }

    // b_1 is gcd(abs_a, b)
    if (b_1 != 1) {
      abs_a /= b_1;
      den /= b_1;
    }
  }

  // abs_a and b are mutually prime and positive

  // restore a
  num = a_positive ? static_cast<std::int64_t>(abs_a) : -static_cast<std::int64_t>(abs_a);

  // assign to r
  if (abs_a <= MAX_NUMERATOR && den <= MAX_DENOMINATOR) {
    clear();
    unchecked_set(static_cast<std::int32_t>(num), static_cast<std::uint32_t>(den));
  } else {
    mpq_ptr q = ensure_gmp();
    mpq_set_i64(q, num, den);
  }
  return *this;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::set(const mpz_t num, const mpz_t den) {
  if (mpz_sgn(den) == 0) {
    if (mpz_sgn(num) > 0) return *this = infinity();
    if (mpz_sgn(num) < 0) return *this = ninfinity();
    return *this = nan();
  }
  assert(mpz_sgn(den) != 0);

  if (mpz_fits_slong_p(num) && mpz_fits_ulong_p(den)) {
    set(static_cast<std::int64_t>(mpz_get_si(num)), mpz_get_ui(den));
  } else {
    mpq_ptr q = ensure_gmp();
    mpq_set_num(q, num);
    mpq_set_den(q, den);
    mpq_canonicalize(q);
    normalize();
  }
  return *this;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::set(const mpz_class& num, const mpz_class& den) {
  return set(num.get_mpz_t(), den.get_mpz_t());
}

template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::set_infinity() {
  if (!Infinity) throw errors::infinity_error();
  clear();
  u.s.den = INF_DEN;
  u.s.num = INF_NUM;
  return *this;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::set_ninfinity() {
  if (!Infinity) throw errors::infinity_error();
  clear();
  u.s.den = INF_DEN;
  u.s.num = NINF_NUM;
  return *this;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::set_nan() {
  if (!NaN) throw errors::nan_error();
  clear();
  u.s.den = NAN_DEN;
  u.s.num = NAN_NUM;
  return *this;
}

template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::~Rational() {
  clear();
}

template <bool Infinity, bool NaN>
void Rational<Infinity, NaN>::clear() {
  if (is_ratgmp()) MpqStore::free(get_gmp());
  u.s.num = 0;
  u.s.den = ONE_DEN;
}

template <bool Infinity, bool NaN>
void Rational<Infinity, NaN>::normalize() {
  if (!is_ratgmp()) return;  // Non-gmp Rational is already normalized

  mpq_ptr q = get_gmp();
  if (mpz_fits_ulong_p(mpq_denref(q)) && mpz_fits_slong_p(mpq_numref(q))) {
    const auto num = mpz_get_si(mpq_numref(q));
    const auto den = mpz_get_ui(mpq_denref(q));
    if (MIN_NUMERATOR <= num && num <= MAX_NUMERATOR && den <= MAX_DENOMINATOR) {
      clear();
      unchecked_set(static_cast<std::int32_t>(num), static_cast<std::uint32_t>(den));
    }
  }
}

template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::operator double() const {
  if (is_nan()) return std::numeric_limits<double>::quiet_NaN();
  if (is_ninfinity()) return -std::numeric_limits<double>::infinity();
  if (is_infinity()) return std::numeric_limits<double>::infinity();
  if (is_rat32() && den() == 1) return static_cast<double>(num());
  return is_rat32() ? mpq_class{num(), den()}.get_d() : mpq_get_d(get_gmp());
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::operator long double() const {
  if (is_rat32() && den() == 1) return static_cast<long double>(num());
  return static_cast<double>(*this);
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::operator std::int32_t() const {
  if (is_rat32() && den() == 1) return static_cast<std::int32_t>(num());
  return static_cast<std::int32_t>(static_cast<double>(*this));
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::operator std::uint32_t() const {
  if (is_rat32() && den() == 1) return static_cast<std::uint32_t>(num());
  return static_cast<std::uint32_t>(static_cast<double>(*this));
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::operator std::int64_t() const {
  if (is_rat32() && den() == 1) return static_cast<std::int64_t>(num());
  return static_cast<std::int64_t>(static_cast<double>(*this));
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::operator std::uint64_t() const {
  if (is_rat32() && den() == 1) return static_cast<std::uint64_t>(num());
  return static_cast<std::uint64_t>(static_cast<double>(*this));
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>::operator mpq_class() const {
  if (is_nan()) return mpq_class{std::numeric_limits<double>::quiet_NaN()};
  if (is_ninfinity()) return mpq_class{-std::numeric_limits<double>::infinity()};
  if (is_infinity()) return mpq_class{std::numeric_limits<double>::infinity()};
  return is_rat32() ? mpq_class{num(), den()} : mpq_class{get_gmp()};
}

template <bool Infinity, bool NaN>
std::string Rational<Infinity, NaN>::str() const {
  if (is_nan()) return "nan";
  if (is_infinity()) return "inf";
  if (is_ninfinity()) return "-inf";
  if (is_rat32()) {
    assert(den() != 0);
    return den() != 1 ? std::to_string(num()) + "/" + std::to_string(den()) : std::to_string(num());
  }
  const auto& q = reinterpret_cast<const mpq_class&>(*get_gmp());
  return q.get_str();
}

template <bool Infinity, bool NaN>
bool Rational<Infinity, NaN>::operator<(const Rational& r2) const {
  return nan_in_op(r2) ? false : this->compare(r2) < 0;
}
template <bool Infinity, bool NaN>
bool Rational<Infinity, NaN>::operator>(const Rational& r2) const {
  return nan_in_op(r2) ? false : this->compare(r2) > 0;
}
template <bool Infinity, bool NaN>
bool Rational<Infinity, NaN>::operator<=(const Rational& r2) const {
  return nan_in_op(r2) ? false : this->compare(r2) <= 0;
}
template <bool Infinity, bool NaN>
bool Rational<Infinity, NaN>::operator>=(const Rational& r2) const {
  return nan_in_op(r2) ? false : this->compare(r2) >= 0;
}
template <bool Infinity, bool NaN>
bool Rational<Infinity, NaN>::operator==(const Rational& r2) const {
  return nan_in_op(r2) ? false : this->compare(r2) == 0;
}
template <bool Infinity, bool NaN>
bool Rational<Infinity, NaN>::operator<(const mpq_t r2) const {
  return this->is_nan() ? false : this->compare(r2) < 0;
}
template <bool Infinity, bool NaN>
bool Rational<Infinity, NaN>::operator>(const mpq_t r2) const {
  return this->is_nan() ? false : this->compare(r2) > 0;
}
template <bool Infinity, bool NaN>
bool Rational<Infinity, NaN>::operator<=(const mpq_t r2) const {
  return this->is_nan() ? false : this->compare(r2) <= 0;
}
template <bool Infinity, bool NaN>
bool Rational<Infinity, NaN>::operator>=(const mpq_t r2) const {
  return this->is_nan() ? false : this->compare(r2) >= 0;
}
template <bool Infinity, bool NaN>
bool Rational<Infinity, NaN>::operator==(const mpq_t r2) const {
  return this->is_nan() ? false : this->compare(r2) == 0;
}

template <bool Infinity, bool NaN>
int Rational<Infinity, NaN>::compare(const Rational& r2) const {
  assert(!is_nan() && !r2.is_nan());
  if (is_infinity()) return r2.is_infinity() ? 0 : 1;
  if (is_ninfinity()) return r2.is_ninfinity() ? 0 : -1;
  // We have already checked for infinity and ninfinity above
  if (r2.is_infinity()) return -1;
  if (r2.is_ninfinity()) return 1;

  if (u.s.den == ONE_DEN && r2.u.s.den == ONE_DEN) {
    assert(is_rat32() && r2.is_rat32());
    return u.s.num - r2.u.s.num;
  }

  if (is_ratgmp()) {
    mpq_ptr q1;
    if (r2.is_ratgmp()) {
      q1 = get_gmp();
      mpq_ptr q2 = r2.get_gmp();
      return mpq_cmp(q1, q2);
    }
    q1 = get_gmp();
    return mpq_cmp_si(q1, r2.num(), r2.den());
  }

  if (r2.is_ratgmp()) {
    mpq_ptr q2 = r2.get_gmp();
    return -mpq_cmp_si(q2, num(), den());
  }

  const std::int64_t n = r2.den() * static_cast<std::int64_t>(num()) - den() * static_cast<std::int64_t>(r2.num());
  return (n < 0 ? -1 : (n > 0));
}
template <bool Infinity, bool NaN>
int Rational<Infinity, NaN>::compare(const mpq_t q) const {
  if (is_infinity()) return 1;
  if (is_ninfinity()) return -1;
  if (is_ratgmp()) return mpq_cmp(get_gmp(), q);
  return -mpq_cmp_si(q, num(), den());
}

template <bool Infinity, bool NaN>
std::size_t Rational<Infinity, NaN>::hash() const noexcept {
  if (is_rat32()) return std::hash<std::pair<std::int32_t, std::uint32_t>>{}(u.s.num, u.s.den);
  assert(is_ratgmp());
  return std::hash<mpq_t>{}(get_gmp());
}

template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::invert() {
  std::uint32_t abs_num;
  if (is_zero() || !is_finite()) return *this = nan();
  if (is_ratgmp()) {
    mpq_ptr q = get_gmp();
    mpq_inv(q, q);
  } else if (u.s.num < 0) {
    abs_num = static_cast<std::uint32_t>(-u.s.num);
    unchecked_set(-static_cast<std::int32_t>(den()), abs_num);
  } else if (u.s.num > 0) {
    abs_num = static_cast<std::uint32_t>(u.s.num);
    unchecked_set(static_cast<std::int32_t>(den()), abs_num);
  }
  return *this;
}

template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::negate() {
  if (is_ratgmp()) {
    mpq_ptr q = get_gmp();
    mpq_neg(q, q);
  } else {
    u.s.num = -u.s.num;
  }
  return *this;
}
template <bool Infinity, bool NaN>
Rational<Infinity, NaN>& Rational<Infinity, NaN>::abs() {
  if (is_nonneg()) {  // Already non-negative, nothing to do
    return *this;
  }
  if (is_ratgmp()) {
    mpq_ptr q = get_gmp();
    mpq_abs(q, q);
    return *this;
  }
  assert(is_rat32());
  u.s.num = std::abs(u.s.num);
  return *this;
}

template <bool Infinity, bool NaN>
void Rational<Infinity, NaN>::unchecked_set(const std::int32_t num, const std::uint32_t den) {
  assert(MIN_NUMERATOR <= num && num <= MAX_NUMERATOR);
  assert(0 < den && den <= MAX_DENOMINATOR);
  u.s.den = den << 1;
  u.s.num = num;
}

template <bool Infinity, bool NaN>
void Rational<Infinity, NaN>::unchecked_set(mpq_ptr q) {
  assert(is_rat32());
  u.p.gmp = reinterpret_cast<intptr_t>(q) | IS_RATGMP;
}
template <bool Infinity, bool NaN>
mpq_ptr Rational<Infinity, NaN>::ensure_gmp() {
  if (!is_ratgmp()) {
    unchecked_set(MpqStore::alloc());
  }
  assert(is_ratgmp());
  return get_gmp();
}

template <bool Infinity, bool NaN>
std::ostream& operator<<(std::ostream& os, const Rational<Infinity, NaN>& r) {
  return os << r.str();
}

template class Rational<true, true>;
template class Rational<false, true>;
template class Rational<true, false>;
template class Rational<false, false>;
template std::ostream& operator<<(std::ostream& os, const Rational<true, true>& r);
template std::ostream& operator<<(std::ostream& os, const Rational<false, true>& r);
template std::ostream& operator<<(std::ostream& os, const Rational<true, false>& r);
template std::ostream& operator<<(std::ostream& os, const Rational<false, false>& r);

}  // namespace lightrat
