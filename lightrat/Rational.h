/**
 * @author TendTo (https://github.com/TendTo)
 * @copyright 2026 lightrat
 * @licence Apache License 2.0
 * @file
 * Rational class.
 */
#pragma once

#include <gmpxx.h>

#include <cassert>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <utility>

#include "lightrat/concepts.h"
#include "lightrat/lightrat.h"

#ifndef LIGHTRAT_EXPLICIT_CONSTRUCTOR
#define LIGHTRAT_EXPLICIT_CONSTRUCTOR explicit
#endif
#ifndef LIGHTRAT_EXPLICIT_CAST
#define LIGHTRAT_EXPLICIT_CAST explicit
#endif

namespace lightrat {

/**
 * Rational number with a compact small-value representation.
 *
 * A Rational stores values in one 64-bit union. Values whose numerator and
 * denominator fit in the supported 32-bit ranges are stored directly as a
 * signed 32-bit numerator and a positive 31-bit denominator. The denominator
 * is stored shifted left by one bit; the low bit is therefore available as a
 * representation tag. A tagged pointer is used for larger values, which are
 * stored as a canonical GMP `mpq_t` allocated from an internal object store.
 *
 * The class owns the GMP value when the GMP representation is active. Copying
 * performs a deep copy and moving transfers ownership.
 *
 * # Explicit conversions
 *
 * The class uses a the macro `LIGHTRAT_EXPLICIT_CONSTRUCTOR` to control whether single-argument constructors
 * are marked as explicit.
 * If no value is provided, the constructors are marked as explicit to avoid accidental conversions.
 * If you want to allow implicit conversions, define `LIGHTRAT_EXPLICIT_CONSTRUCTOR` to be empty
 * before including this header.
 * The same applies to the conversion operators, which are controlled by the macro `LIGHTRAT_EXPLICIT_CAST`.
 *
 * ### Serialization
 *
 * The easiest way to serialize a Rational is to use the `str()` method,
 * which returns a string representation of the rational number.
 * Assuming the same template parameter as used, the string representation can then be used
 * to reconstruct the original Rational using the string constructor.
 * If the Rational is in the integer pair representation, it can also be serialized
 * as a pair of integers using the `num()` and `den()` methods
 * or copying the `lightrat_t` representation using the `operator*()`,
 * but this approach is less portable and more error-prone.
 *
 * @note
 * The compact representation with a GMP fallback is inspired by the
 * [OpenSMT FastRational implementation](https://verify.inf.usi.ch/opensmt),
 * originally credited there to David Monniaux and the Centre national de la recherche scientifique (CNRS),
 * and by the [Yices' GMP rational helpers](https://yices.csl.sri.com/), developed by SRI International,
 * which is also used in the GCD-aware arithmetic shortcuts.
 *
 * Basic arithmetic and comparison:
 * @code
 * #include <cassert>
 * #include <iostream>
 *
 * #include "lightrat/Rational.h"
 *
 * Rational a{1, 2};
 * Rational b{"3/4"};
 * Rational sum = a + b; // 5/4
 * sum *= 2;                       // 5/2
 * if (sum > 2) {
 *   std::cout << sum.str();       // 5/2
 * }
 * @endcode
 *
 * Infinity sentinels can be created and checked like any other Rational:
 * @code
 * Rational positive = Rational::infinity();
 * Rational negative = Rational::ninfinity();
 * assert(positive.is_infinity());
 * assert(negative.is_ninfinity());
 * @endcode
 *
 * Likewise, NaN sentinels can be created and checked like any other Rational:
 * @code
 * Rational nan = Rational::nan();
 * assert(nan.is_nan());
 * @endcode
 * @tparam Infinity Whether to enable infinity sentinels.
 * If `false`, any operation that would produce an infinity sentinel will instead throw a `error::infinity_error`.
 * @tparam NaN Whether to enable NaN sentinels.
 * If `false`, any operation that would produce a NaN sentinel will instead throw a `error::nan_error`.
 * Moreover, constructing a Rational from a string which is not a valid number will throw a
 * `error::string_parsing_error`.
 */
template <bool Infinity = true, bool NaN = true>
class Rational {
  static constexpr int IS_RAT32 = 0x0;   ///< Tag value for the compact numerator/denominator representation.
  static constexpr int IS_RATGMP = 0x1;  ///< Tag value OR-ed into a stored GMP pointer.

 public:
  /**
   * Construct an infinity sentinel value.
   * It is equivalent to
   * @code
   * Rational r;
   * r.set_infinity(); // +infinity
   * r.is_infinity(); // true
   * @endcode
   * @return The positive infinity sentinel.
   */
  static Rational infinity();
  /**
   * Quickly construct a negative infinity sentinel value.
   * It is equivalent to
   * @code
   * Rational r;
   * r.set_ninfinity(); // -infinity
   * r.is_ninfinity(); // true
   * @endcode
   * @return The negative infinity sentinel.
   */
  static Rational ninfinity();
  /**
   * Quickly construct a NaN sentinel value.
   * It is equivalent to
   * @code
   * Rational r;
   * r.set_nan(); // Not A Number
   * r.is_nan(); // true
   * @endcode
   * @return The NaN sentinel.
   */
  static Rational nan();

  /**
   * Zero-initialized rational number.
   * @code
   * Rational r; // 0/1
   * @endcode
   */
  Rational();
  /**
   * Construct a Rational from a C representation.
   * @code
   * lightrat_t rat;
   * lightrat_init(rat); // 0/1
   * Rational r{rat}; // 0/1
   * lightrat_clear(rat);
   * @endcode
   * @pre `rat` must be initialized.
   * @param rat A C representation of a rational number.
   */
  LIGHTRAT_EXPLICIT_CONSTRUCTOR Rational(const lightrat_t rat);
  /**
   * Construct a Rational from a numerator and denominator.
   * @code
   * Rational r{3, 4}; // 3/4
   * @endcode
   * @pre `den` must be non-zero.
   * @param num The numerator.
   * @param den The denominator.
   */
  Rational(std::int32_t num, std::uint32_t den);
  /**
   * Construct a Rational from a numerator and denominator.
   * @code
   * Rational r{3, 4}; // 3/4
   * @endcode
   * @pre `den` must be non-zero.
   * @param num The numerator.
   * @param den The denominator.
   */
  Rational(std::int64_t num, std::uint64_t den);
  /**
   * Construct a Rational from a string representation.
   * Depending on the template parameters `Infinity` and `NaN`, the string parsing will behave differently.
   *
   * ### `Infinity`
   *
   * If `Infinity` is enabled, the strings "inf", "-inf" will be accepted
   * as positive and negative infinity sentinels, respectively, and values of the form "num/0" will produce
   * either a positive or negative infinity sentinel, depending on the sign of `num`.
   * If `Infinity` is disabled, the strings "inf", "-inf" will not be considered valid.
   * Thus, they will return a NaN sentinel if `NaN` is enabled, or will throw a `error::string_parsing_error` otherwise.
   * Values of the form "num/0" will produce a floating-point exception (division by 0) and terminate the program.
   *
   * ### `NaN`
   *
   * If `NaN` is enabled, any string that is not a valid rational number and "0/0" will return a NaN sentinel.
   * If `NaN` is disabled, any string that is not a valid rational number will throw a `error::string_parsing_error`,
   * while "0/0" will produce a floating-point exception (division by 0) and terminate the program.
   * @code
   * Rational r{"3/4"}; // 3/4
   * Rational r{"0"}; // 0/1
   * Rational r{"-42"}; // -42/1
   * Rational r{"12345678901234567890/9876543210987654321"}; // large rational
   * Rational r{"-inf"}; // -infinity (if `Infinity` is enabled)
   * Rational r{"inf"}; // +infinity (if `Infinity` is enabled)
   * Rational r{"not a number, a random string"}; // NaN (if `NaN` is enabled)
   * @endcode
   * @pre `str` must be a valid string representation of a rational number.
   * @param str A string representation of a rational number.
   */
  LIGHTRAT_EXPLICIT_CONSTRUCTOR Rational(const std::string& str);
  /**
   * Construct a Rational from a string representation.
   * Depending on the template parameters `Infinity` and `NaN`, the string parsing will behave differently.
   *
   * ### `Infinity`
   *
   * If `Infinity` is enabled, the strings "inf", "-inf" will be accepted
   * as positive and negative infinity sentinels, respectively, and values of the form "num/0" will produce
   * either a positive or negative infinity sentinel, depending on the sign of `num`.
   * If `Infinity` is disabled, the strings "inf", "-inf" will not be considered valid.
   * Thus, they will return a NaN sentinel if `NaN` is enabled, or will throw a `error::string_parsing_error` otherwise.
   * Values of the form "num/0" will produce a floating-point exception (division by 0) and terminate the program.
   *
   * ### `NaN`
   *
   * If `NaN` is enabled, any string that is not a valid rational number and "0/0" will return a NaN sentinel.
   * If `NaN` is disabled, any string that is not a valid rational number will throw a `error::string_parsing_error`,
   * while "0/0" will produce a floating-point exception (division by 0) and terminate the program.
   * @code
   * Rational r{"3/4"}; // 3/4
   * Rational r{"0"}; // 0/1
   * Rational r{"-42"}; // -42/1
   * Rational r{"12345678901234567890/9876543210987654321"}; // large rational
   * Rational r{"-inf"}; // -infinity (if `Infinity` is enabled)
   * Rational r{"inf"}; // +infinity (if `Infinity` is enabled)
   * Rational r{"not a number, a random string"}; // NaN (if `NaN` is enabled)
   * @endcode
   * @pre `str` must be a valid string representation of a rational number.
   * @param str A string representation of a rational number.
   */
  LIGHTRAT_EXPLICIT_CONSTRUCTOR Rational(const char* str);
  /**
   * Construct a Rational from an integer value.
   * @code
   * Rational r{42}; // 42/1
   * Rational r{-42}; // -42/1
   * @endcode
   * @param num An integer value.
   */
  LIGHTRAT_EXPLICIT_CONSTRUCTOR Rational(std::int32_t num);
  /**
   * Construct a Rational from an integer value.
   * @code
   * Rational r{42}; // 42/1
   * Rational r{-42}; // -42/1
   * @endcode
   * @param num An integer value.
   */
  LIGHTRAT_EXPLICIT_CONSTRUCTOR Rational(std::int64_t num);
  /**
   * Construct a Rational from an unsigned integer value.
   * @code
   * Rational r{42}; // 42/1
   * @endcode
   * @param num An unsigned integer value.
   */
  LIGHTRAT_EXPLICIT_CONSTRUCTOR Rational(std::uint32_t num);
  /**
   * Construct a Rational from an unsigned integer value.
   * @code
   * Rational r{42}; // 42/1
   * @endcode
   * @param num An unsigned integer value.
   */
  LIGHTRAT_EXPLICIT_CONSTRUCTOR Rational(std::uint64_t num);
  /**
   * Construct a Rational from a double.
   * @code
   * Rational r{0.75}; // 3/4
   * Rational r{-42.0}; // -42/1
   * @endcode
   * @note To maximize compatibility, we use the GMP [`mpq_set_d`](https://gmplib.org/manual/Rational-Conversions)
   * function to convert the double to a rational, but note that this is still an approximation, since there may be
   * no way of knowing the intended rational value from a double.
   * @param d A double value.
   */
  LIGHTRAT_EXPLICIT_CONSTRUCTOR Rational(double d);
  /**
   * Construct a Rational from a GMP rational value.
   * @code
   * mpq_t q;
   * mpq_init(q);
   * mpq_set_str(q, "6/8", 10);
   * mpq_canonicalize(q); // Rational assumes the GMP rational is canonicalized!
   * Rational r{q}; // 3/4
   * mpq_clear(q);
   * @endcode
   * @note The Rational does NOT take ownership of the GMP rational.
   * @pre `q` must be initialized and canonicalized.
   * @param q A GMP rational value.
   */
  LIGHTRAT_EXPLICIT_CONSTRUCTOR Rational(const mpq_t q);
  /**
   * Construct a Rational from a GMP integer value.
   * @code
   * mpz_t z;
   * mpz_init(z);
   * mpz_set_str(z, "42", 10);
   * Rational r{z}; // 42/1
   * mpz_clear(z);
   * @endcode
   * @note The Rational does NOT take ownership of the GMP integer.
   * @pre `z` must be initialized.
   * @param z A GMP integer value.
   */
  LIGHTRAT_EXPLICIT_CONSTRUCTOR Rational(const mpz_t z);
  /**
   * Construct a Rational from GMP numerator and denominator values.
   * @code
   * mpz_t num, den;
   * mpz_init(num);
   * mpz_init(den);
   * mpz_set_str(num, "3", 10);
   * mpz_set_str(den, "4", 10);
   * Rational r{num, den}; // 3/4
   * mpz_clear(num);
   * mpz_clear(den);
   * @endcode
   * @note The Rational does NOT take ownership of the GMP values.
   * @pre `num` and `den` must be initialized.
   * @param num A GMP integer numerator value.
   * @param den A GMP integer denominator value.
   */
  Rational(const mpz_t num, const mpz_t den);
  /**
   * Construct a Rational from a GMP rational value.
   * @code
   * mpq_class q{3, 4};
   * Rational r{q}; // 3/4
   * @endcode
   * @pre `q` must be canonicalized.
   * @param q A GMP rational value.
   */
  LIGHTRAT_EXPLICIT_CONSTRUCTOR Rational(const mpq_class& q);
  /**
   * Construct a Rational from a GMP integer value.
   * @code
   * mpz_class z{42};
   * Rational r{z}; // 42/1
   * @endcode
   * @param z A GMP integer value.
   */
  LIGHTRAT_EXPLICIT_CONSTRUCTOR Rational(const mpz_class& z);
  /**
   * Construct a Rational from GMP numerator and denominator values.
   * @code
   * mpz_class num{3};
   * mpz_class den{4};
   * Rational r{num, den}; // 3/4
   * @endcode
   * @param num A GMP integer numerator value.
   * @param den A GMP integer denominator value.
   */
  Rational(const mpz_class& num, const mpz_class& den);
  /**
   * Copy-construct a Rational from another Rational.
   * @code
   * Rational r1{3, 4};
   * Rational r2{r1}; // 3/4
   * @endcode
   * @param r2 Another Rational value.
   */
  Rational(const Rational& r2);
  /**
   * Move-construct a Rational from another Rational.
   * The moved-from Rational is given the old value of the moved-to Rational.
   * @code
   * Rational r1{3, 4};
   * Rational r2{std::move(r1)}; // 3/4, r1 is now 0/1
   * @endcode
   * @param r2 Another Rational value.
   */
  Rational(Rational&& r2) noexcept;

  Rational& operator=(const lightrat_t rat);
  Rational& operator=(const std::string& str);
  Rational& operator=(const char* str);
  Rational& operator=(std::int32_t num);
  Rational& operator=(std::int64_t num);
  Rational& operator=(std::uint32_t num);
  Rational& operator=(std::uint64_t num);
  Rational& operator=(double d);
  Rational& operator=(const mpq_t q);
  Rational& operator=(const mpz_t z);
  Rational& operator=(const mpz_class& z);
  Rational& operator=(const mpq_class& q);
  Rational& operator=(const Rational& r2);
  Rational& operator=(Rational&& r2) noexcept;

  Rational& operator+=(const Rational& r2);
  Rational& operator-=(const Rational& r2);
  Rational& operator*=(const Rational& r2);
  Rational& operator/=(const Rational& r2);
  template <concepts::is_not<Rational> V>
  Rational& operator+=(V&& r2) {
    return *this += Rational{std::forward<V>(r2)};
  }
  template <concepts::is_not<Rational> V>
  Rational& operator-=(V&& r2) {
    return *this -= Rational{std::forward<V>(r2)};
  }
  template <concepts::is_not<Rational> V>
  Rational& operator*=(V&& r2) {
    return *this *= Rational{std::forward<V>(r2)};
  }
  template <concepts::is_not<Rational> V>
  Rational& operator/=(V&& r2) {
    return *this /= Rational{std::forward<V>(r2)};
  }

  Rational operator+(const Rational& r2) const { return Rational(*this) += r2; }
  Rational operator-(const Rational& r2) const { return Rational(*this) -= r2; }
  Rational operator*(const Rational& r2) const { return Rational(*this) *= r2; }
  Rational operator/(const Rational& r2) const { return Rational(*this) /= r2; }
  template <concepts::is_not<Rational> V>
  Rational operator+(V&& r2) const {
    return *this + Rational{std::forward<V>(r2)};
  }
  template <concepts::is_not<Rational> V>
  Rational operator-(V&& r2) const {
    return *this - Rational{std::forward<V>(r2)};
  }
  template <concepts::is_not<Rational> V>
  Rational operator*(V&& r2) const {
    return *this * Rational{std::forward<V>(r2)};
  }
  template <concepts::is_not<Rational> V>
  Rational operator/(V&& r2) const {
    return *this / Rational{std::forward<V>(r2)};
  }

  bool operator==(const Rational& r2) const;
  bool operator<(const Rational& r2) const;
  bool operator>(const Rational& r2) const;
  bool operator<=(const Rational& r2) const;
  bool operator>=(const Rational& r2) const;
  bool operator==(const mpq_t r2) const;
  bool operator<(const mpq_t r2) const;
  bool operator>(const mpq_t r2) const;
  bool operator<=(const mpq_t r2) const;
  bool operator>=(const mpq_t r2) const;
  bool operator==(const mpq_class& r2) const { return *this == r2.get_mpq_t(); }
  bool operator<(const mpq_class& r2) const { return *this < r2.get_mpq_t(); }
  bool operator>(const mpq_class& r2) const { return *this > r2.get_mpq_t(); }
  bool operator<=(const mpq_class& r2) const { return *this <= r2.get_mpq_t(); }
  bool operator>=(const mpq_class& r2) const { return *this >= r2.get_mpq_t(); }
  template <concepts::is_not<Rational> V>
  bool operator==(V&& r2) const {
    return *this == Rational{std::forward<V>(r2)};
  }
  template <concepts::is_not<Rational> V>
  bool operator<(V&& r2) const {
    return *this < Rational{std::forward<V>(r2)};
  }
  template <concepts::is_not<Rational> V>
  bool operator>(V&& r2) const {
    return *this > Rational{std::forward<V>(r2)};
  }
  template <concepts::is_not<Rational> V>
  bool operator<=(V&& r2) const {
    return *this <= Rational{std::forward<V>(r2)};
  }
  template <concepts::is_not<Rational> V>
  bool operator>=(V&& r2) const {
    return *this >= Rational{std::forward<V>(r2)};
  }

  /** @checker{value, = 0, @code bool zero = Rational{0}.is_zero(); // true @endcode} */
  [[nodiscard]] bool is_zero() const {
    return is_ratgmp() ? mpq_sgn(get_gmp()) == 0 : u.s.num == 0 && (!NaN || u.s.den != 0);
  }
  /** @checker{value, != 0, @code bool nonzero = Rational{1}.is_nonzero(); // true @endcode} */
  [[nodiscard]] bool is_nonzero() const { return is_ratgmp() ? mpq_sgn(get_gmp()) != 0 : u.s.num != 0; }
  /** @checker{value, > 0, @code bool positive = Rational{1}.is_pos(); @endcode} */
  [[nodiscard]] bool is_pos() const { return is_ratgmp() ? mpq_sgn(get_gmp()) > 0 : u.s.num > 0; }
  /** @checker{value, >= 0, @code bool nonnegative = Rational{0}.is_nonneg(); // true @endcode} */
  [[nodiscard]] bool is_nonneg() const {
    return is_ratgmp() ? mpq_sgn(get_gmp()) >= 0 : NaN ? u.s.num > 0 || (u.s.num >= 0 && u.s.den != 0) : u.s.num >= 0;
  }
  /** @checker{value, < 0, @code bool negative = Rational{-1}.is_neg(); // true @endcode} */
  [[nodiscard]] bool is_neg() const { return is_ratgmp() ? mpq_sgn(get_gmp()) < 0 : u.s.num < 0; }
  /** @checker{value, <= 0, @code bool nonpositive = Rational{0}.is_nonpos(); // true @endcode} */
  [[nodiscard]] bool is_nonpos() const {
    return is_ratgmp() ? mpq_sgn(get_gmp()) <= 0 : NaN ? u.s.num < 0 || (u.s.num <= 0 && u.s.den != 0) : u.s.num <= 0;
  }
  /** @checker{value, = 1, @code bool one = Rational{1}.is_one(); // true @endcode} */
  [[nodiscard]] bool is_one() const;
  /** @checker{value, = -1, @code bool minus_one = Rational{-1}.is_minus_one(); // true  @endcode} */
  [[nodiscard]] bool is_minus_one() const;
  /** @checker{value, an integer, This corresponds to checking that `num() == 0 && den() != 0`.
   * @code bool integer = Rational{2}.is_integer(); // true @endcode} */
  [[nodiscard]] bool is_integer() const;
  /** @checker{value, infinity, This corresponds to checking that `den() == 0` and `num() == 1`.
   * @code bool infinity = Rational::infinity().is_infinity(); //true @endcode} */
  [[nodiscard]] bool is_infinity() const;
  /** @checker{value, negative infinity, This corresponds to checking that `den() == 0` and `num() == -1`.
   * @code bool infinity = Rational::ninfinity().is_ninfinity(); // true @endcode} */
  [[nodiscard]] bool is_ninfinity() const;
  /** @checker{value, NaN, This corresponds to checking that `den() == 0` and `num() == 0`.
   * @code bool nan = Rational::nan().is_nan(); //true @endcode} */
  [[nodiscard]] bool is_nan() const;
  /** @checker{value, is a finite value, @code bool finite = Rational{1}.is_finite(); // true @endcode} */
  [[nodiscard]] bool is_finite() const { return !is_infinity() && !is_ninfinity() && !is_nan(); }
  /**
   * @getter{total size in bytes, rational value,
   * If the Rational is stored in the cheap integer pair representation,
   * this returns the size of the Rational object itself.
   * If the rational is stored in the GMP representation,
   * this returns the size of the Rational object plus the size of the GMP rational
   * (in terms of both the size of the limbs used and the size of the struct used to store the GMP rational),
   * including the size of the struct used to store the internal linked list of GMP rationals.
   * @code
   * Rational r{3, 4};
   * std::size_t size = r.size(); // size of the Rational object itself
   * Rational r2{"12345678901234567890/9876543210987654321"};
   * std::size_t size2 = r2.size(); // size of the Rational object plus the size of the GMP rational and its limbs
   * @endcode}
   */
  [[nodiscard]] std::size_t size() const;
  /**
   * Return the additive inverse of the value.
   * @code
   * Rational opposite = -Rational{3, 4}; // -3/4
   * @endcode
   * @return A new Rational with the additive inverse of the value.
   */
  Rational operator-() const;
  /**
   * Return a copy of the value.
   * @code
   * Rational opposite = +Rational{3, 4}; // 3/4
   * @endcode
   * @return A new Rational with the same value.
   */
  Rational operator+() const;

  /**
   * Releases any GMP payload and resets the value to zero.
   * @code
   * {
   *   Rational r{"12345678901234567890/1"};
   * } // r is destroyed here and the GMP struct is released.
   * @endcode
   */
  ~Rational();

  /**
   * Swap the value with another Rational.
   * @code
   * Rational a{1, 2}, b{3, 4};
   * a.swap(b); // a is 3/4, b is 1/2
   * @endcode
   * @param r2 Another Rational value.
   */
  void swap(Rational& r2) noexcept { std::swap(u, r2.u); }

  /**
   * Set the value to the rational number represented by the numerator and denominator.
   * @code
   * Rational r;
   * r.set(3, 4); // 3/4
   * @endcode
   * @pre `den` must be non-zero.
   * @param num The numerator.
   * @param den The denominator.
   * @return A reference to the updated Rational.
   */
  Rational& set(std::int32_t num, std::uint32_t den);
  /**
   * Set the value to the rational number represented by the numerator and denominator.
   * @code
   * Rational r;
   * r.set(std::int64_t{3}, std::uint64_t{4}); // 3/4
   * @endcode
   * @pre `den` must be non-zero.
   * @param num The numerator.
   * @param den The denominator.
   * @return A reference to the updated Rational.
   */
  Rational& set(std::int64_t num, std::uint64_t den);
  /**
   * Set the value to the rational number represented by the numerator and denominator.
   * @code
   * mpz_class num{3}, den{4};
   * Rational r;
   * r.set(num.get_mpz_t(), den.get_mpz_t()); // 3/4
   * @endcode
   * @pre `den` must be non-zero.
   * @param num The numerator.
   * @param den The denominator.
   * @return A reference to the updated Rational.
   */
  Rational& set(const mpz_t num, const mpz_t den);
  /**
   * Set the value to the rational number represented by the numerator and denominator.
   * @code
   * mpz_class num{3}, den{4};
   * Rational r;
   * r.set(num, den); // 3/4
   * @endcode
   * @pre `den` must be non-zero.
   * @param num The numerator.
   * @param den The denominator.
   * @return A reference to the updated Rational.
   */
  Rational& set(const mpz_class& num, const mpz_class& den);
  /**
   * Set the value of the Rational to the infinity sentinel.
   * @code
   * Rational r;
   * r.set_infinity(); // +infinity
   * @endcode
   * @return A reference to the updated Rational.
   * @throw error::infinity_error if infinity support is disabled.
   */
  Rational& set_infinity();
  /**
   * Set the value of the Rational to the negative infinity sentinel.
   * @code
   * Rational r;
   * r.set_ninfinity(); // -infinity
   * @endcode
   * @return A reference to the updated Rational.
   * @throw error::infinity_error if infinity support is disabled.
   */
  Rational& set_ninfinity();
  /**
   * Set the value of the Rational to the NaN sentinel.
   * @code
   * Rational r;
   * r.set_nan(); // Not A Number
   * @endcode
   * @return A reference to the updated Rational.
   * @throw error::nan_error if NaN support is disabled.
   */
  Rational& set_nan();

  /**
   * Release any GMP payload and reset the value to zero.
   * @code
   * Rational r{3, 4};
   * r.clear(); // 0/1
   * @endcode
   */
  void clear();
  /**
   * Convert a GMP value to the compact integer pair representation when it fits.
   * There is usually no need to call this function explicitly,
   * as the Rational class will automatically use the compact representation when possible.
   * @code
   * Rational r{123, 1};
   * r.normalize(); // 123/1, stored in the compact representation
   * @endcode
   */
  void normalize();

  /**
   * Set the value to 1/1.
   * @code
   * Rational r;
   * r.set_one(); // 1/1
   * @endcode
   * @return A reference to the updated Rational.
   */
  Rational& set_one() {
    clear();
    unchecked_set(1, 1);
    return *this;
  }
  /**
   * Set the value to -1/1.
   * @code
   * Rational r;
   * r.set_minus_one(); // -1/1
   * @endcode
   * @return A reference to the updated Rational.
   */
  Rational& set_minus_one() {
    clear();
    unchecked_set(-1, 1);
    return *this;
  }

  /**
   * @checker{value, in the compact integer pair representation,
   * @code bool compact = Rational{"1/2"}.is_rat32(); // true @endcode}
   */
  [[nodiscard]] bool is_rat32() const { return (u.p.gmp & IS_RATGMP) != IS_RATGMP; }
  /**
   * @checker{value, using the mpq GMP representation,
   * @code bool using_gmp = Rational{"12345678901234567890/1"}.is_ratgmp(); //true @endcode}
   */
  [[nodiscard]] bool is_ratgmp() const { return (u.p.gmp & IS_RATGMP) == IS_RATGMP; }
  /**
   * @getter{mpq GMP representation, rational value,
   * It is possible, but discouraged, to alter the value of the GMP representation
   * by operating on the return value of this function.
   * @code
   * Rational r{"12345678901234567890/1"};
   * mpq_ptr q = r.get_gmp();
   * @endcode
   * @pre `is_ratgmp()`}
   */
  [[nodiscard]] mpq_ptr get_gmp() const {
    assert(is_ratgmp());
    return reinterpret_cast<mpq_ptr>(u.p.gmp ^ IS_RATGMP);
  }
  /**
   * @getter{mpz GMP representation of the numerator, rational value,
   * @code
   * Rational r{"12345678901234567890/1"};
   * mpz_srcptr n = r.gmp_num(); // 12345678901234567890
   * @endcode
   * @pre `is_ratgmp()`}
   */
  [[nodiscard]] mpz_srcptr gmp_num() const {
    assert(is_ratgmp());
    return mpq_numref(get_gmp());
  }
  /**
   * @getter{mpz GMP representation of the denominator, rational value,
   * @code
   * Rational r{"1/9876543210987654321"};
   * mpz_srcptr d = r.gmp_den(); // 9876543210987654321
   * @endcode
   * @pre `is_ratgmp()`}
   */
  [[nodiscard]] mpz_srcptr gmp_den() const {
    assert(is_ratgmp());
    return mpq_denref(get_gmp());
  }
  /**
   * @getter{integer representation of the numerator, rational value,
   * @code
   * std::int32_t n = Rational{3\, 4}.num(); // 3
   * @endcode
   * @pre `is_rat32() && is_finite()`}
   */
  [[nodiscard]] std::int32_t num() const {
    assert(is_rat32() && is_finite());
    return u.s.num;
  }
  /**
   * @getter{integer representation of the denominator, rational value,
   * @code
   * std::uint32_t d = Rational{3\, 4}.den(); // 4
   * @endcode
   * @pre `is_rat32() && is_finite()`}
   */
  [[nodiscard]] std::uint32_t den() const {
    assert(is_rat32() && is_finite());
    return u.s.den >> 1;
  }
  /**
   * @getter{pointer to the internal representation, rational value,
   * @code
   * Rational r{3\, 4};
   * const lightrat_u* data = r.data();
   * @endcode}
   */
  [[nodiscard]] const lightrat_u* data() const { return &u; }
  /**
   * @getter{reference to the internal representation, rational value,
   * @code
   * Rational r{3\, 4};
   * const lightrat_u& data = *r;
   * @endcode}
   */
  [[nodiscard]] const lightrat_u& operator*() const { return u; }

  LIGHTRAT_EXPLICIT_CAST operator double() const;
  LIGHTRAT_EXPLICIT_CAST operator mpq_class() const;
  LIGHTRAT_EXPLICIT_CAST operator long double() const;
  LIGHTRAT_EXPLICIT_CAST operator std::int32_t() const;
  LIGHTRAT_EXPLICIT_CAST operator std::uint32_t() const;
  LIGHTRAT_EXPLICIT_CAST operator std::int64_t() const;
  LIGHTRAT_EXPLICIT_CAST operator std::uint64_t() const;
  LIGHTRAT_EXPLICIT_CAST operator bool() const { return is_nonzero(); }

  /**
   * @getter{string representation, rational value,
   * @code std::string value = Rational{3\, 4}.str(); // "3/4" @endcode}
   */
  [[nodiscard]] std::string str() const;

  /**
   * Compare with another Rational.
   * It will return a negative value if `*this < r2`, zero if `*this == r2`, and a positive value if `*this > r2`.
   * @code
   * int result = Rational{1, 2}.compare(Rational{3, 4}); // negative
   * @endcode
   * @pre `r2` must be initialized and canonicalized.
   * @param r2 Another Rational value.
   * @return 0 if the two values are equal.
   * @return < 0 if `*this < r2`.
   * @return > 0 if `*this > r2`.
   */
  [[nodiscard]] int compare(const Rational& r2) const;
  /**
   * Compare with a GMP rational value.
   * It will return a negative value if `*this < q`, zero if `*this == q`, and a positive value if `*this > q`.
   * @code
   * mpq_class q{3, 4};
   * int result = Rational{1, 2}.compare(q.get_mpq_t()); // negative
   * @endcode
   * @pre `q` must be initialized and canonicalized.
   * @param q A GMP rational value.
   * @return 0 if the two values are equal.
   * @return < 0 if `*this < q`.
   * @return > 0 if `*this > q`.
   */
  [[nodiscard]] int compare(const mpq_t q) const;
  /**
   * Compare with a GMP rational value.
   * It will return a negative value if `*this < q`, zero if `*this == q`, and a positive value if `*this > q`.
   * @code
   * mpq_class q{3, 4};
   * int result = Rational{1, 2}.compare(q); // negative
   * @endcode
   * @pre `q` must be canonicalized.
   * @param q A GMP rational value.
   * @return 0 if the two values are equal.
   * @return < 0 if `*this < q`.
   * @return > 0 if `*this > q`.
   */
  [[nodiscard]] int compare(const mpq_class& q) const { return compare(q.get_mpq_t()); }
  /**
   * Compare with a value convertible to Rational.
   * @code
   * int result = Rational{1, 2}.compare(0.75); // negative
   * @endcode
   * @param r2 A value convertible to Rational.
   * @return 0 if the two values are equal.
   * @return < 0 if `*this < r2`.
   * @return > 0 if `*this > r2`.
   */
  template <concepts::is_not<Rational> V>
  [[nodiscard]] int compare(V&& r2) const {
    return compare(Rational{std::forward<V>(r2)});
  }

  /**
   * Compute the hash of this Rational.
   * The hash is computed based on whether the Rational is in the integer pair representation or the GMP representation.
   * - In the integer pair representation, the hash is computed by interweaving the numerator and denominator
   * (32-bits each) into a single 64-bit value, alternating bits from both numbers
   * to avoid favoring too much weight to either.
   * If the std::size_t is 64-bits, all hashes computed this way uniquely identify the Rational.
   * - In the GMP representation, the hash is computed by hashing the numerator and denominator separately
   * and combining the two hashes using a simple mixing function.
   *
   * @code
   * std::size_t h = Rational{3, 4}.hash();
   * @endcode
   * @see https://stackoverflow.com/a/39490836
   * @see https://chromium.googlesource.com/chromium/src/base/+/e0998c057ffcbb6a47f867fe7e760856a8a88bc3/hash.h
   * @see
   * https://github.com/usi-verification-and-security/opensmt/blob/15b42c6f339679a4547d3cc9955547e7ca03505c/src/common/numbers/FastRational.h#L271
   * @return A hash value for this Rational.
   */
  [[nodiscard]] std::size_t hash() const noexcept;

  /**
   * Update the value with its reciprocal.
   * @code
   * Rational r{3, 4};
   * r.invert(); // 4/3
   * @endcode
   * @pre `!is_zero() && is_finite()`.
   * @return A reference to the updated Rational.
   */
  Rational& invert();
  /**
   * Update the value with its additive inverse.
   * @code
   * Rational r{3, 4};
   * r.negate(); // -3/4
   * @endcode
   * @return A reference to the updated Rational.
   */
  Rational& negate();
  /**
   * Update the value with its absolute value.
   * If the value is already non-negative, nothing is done.
   * @code
   * Rational r{-3, 4};
   * r.abs(); // 3/4
   * @endcode
   * @return A reference to the updated Rational.
   */
  Rational& abs();

  /**
   * Return a new Rational with the reciprocal of the current value.
   * @code
   * Rational r{3, 4};
   * r.inverted(); // 4/3
   * @endcode
   * @pre `!is_zero() && is_finite()`.
   * @return A new Rational with the reciprocal of the current value.
   */
  [[nodiscard]] Rational inverted() const { return Rational(*this).invert(); }
  /**
   * Return a new Rational with the additive inverse of the current value.
   * @code
   * Rational r{3, 4};
   * r.negated(); // -3/4
   * @endcode
   * @return A new Rational with the additive inverse of the current value.
   */
  [[nodiscard]] Rational negated() const { return Rational(*this).negate(); }
  /**
   * Return a new Rational with the absolute value of the current value.
   * @code
   * Rational r{-3, 4};
   * r.absed(); // 3/4
   * @endcode
   * @return A new Rational with the absolute value of the current value.
   */
  [[nodiscard]] Rational absed() const { return Rational(*this).abs(); }

 private:
  /**
   * Set the value to a numerator and denominator without checking for zero denominators or normalizing.
   * @pre `den != 0`.
   * @post `is_rat32() && is_finite()`.
   * @param num The numerator.
   * @param den The denominator.
   */
  void unchecked_set(std::int32_t num, std::uint32_t den);
  /**
   * Copy a GMP value into the Rational, taking ownership of the GMP value.
   * @pre `q` must be initialized and canonicalized.
   * @post `is_ratgmp()`.
   * @param q A GMP rational value.
   */
  void unchecked_set(mpq_ptr q);
  /**
   * Ensure that the current value is in the GMP representation, allocating a new GMP value if necessary.
   * @note The new GMP value is initialized and thus ready for use, but it is not set to any specific value.
   * @post `is_ratgmp()`.
   * @return A pointer to the GMP value.
   */
  mpq_ptr ensure_gmp();
  /**
   * Convert the current value to a mpq_t GMP representation if it is not already in that representation.
   * After this call, `is_ratgmp()` will be true.
   * @pre `is_finite() && is_rat32()`.
   * @post `is_ratgmp()`.
   */
  void convert_to_gmp();
  /**
   * Sanity check to ensure that there are no NaN values in the operation.
   * This includes both the current Rational and `r2`.
   * @param r2 Another Rational value.
   * @return true if either `*this` or `r2` is NaN
   * @return false if both `*this` and `r2` are not NaN
   */
  bool nan_in_op(const Rational& r2) const;

  /**
   * Internal 64-bit representation.
   * When the low tag bit is zero, `u.s` contains a signed 32-bit numerator
   * and a positive 31-bit denominator stored as `den << 1`.
   * When the low tag bit is one, `u.p.gmp` contains an object-store pointer OR-ed with `IS_RATGMP`,
   * and the pointed-to object is a GMP `mpq_t`.
   * The pointer is aligned, so its low bit is available for this tag.
   */
  lightrat_u u;
};

template <bool Infinity, bool NaN, concepts::is_not<Rational<Infinity, NaN>> V>
Rational<Infinity, NaN> operator+(V&& o, const Rational<Infinity, NaN>& r) {
  return r + std::forward<V>(o);
}
template <bool Infinity, bool NaN, concepts::is_not<Rational<Infinity, NaN>> V>
Rational<Infinity, NaN> operator-(V&& o, const Rational<Infinity, NaN>& r) {
  return -(r - std::forward<V>(o));
}
template <bool Infinity, bool NaN, concepts::is_not<Rational<Infinity, NaN>> V>
Rational<Infinity, NaN> operator*(V&& o, const Rational<Infinity, NaN>& r) {
  return r * std::forward<V>(o);
}
template <bool Infinity, bool NaN, concepts::is_not<Rational<Infinity, NaN>> V>
Rational<Infinity, NaN> operator/(V&& o, const Rational<Infinity, NaN>& r) {
  return Rational<Infinity, NaN>{std::forward<V>(o)} / r;
}

template <bool Infinity, bool NaN, concepts::is_not<Rational<Infinity, NaN>> V>
bool operator<(V&& o, const Rational<Infinity, NaN>& r) {
  return r > std::forward<V>(o);
}
template <bool Infinity, bool NaN, concepts::is_not<Rational<Infinity, NaN>> V>
bool operator<=(V&& o, const Rational<Infinity, NaN>& r) {
  return r >= std::forward<V>(o);
}
template <bool Infinity, bool NaN, concepts::is_not<Rational<Infinity, NaN>> V>
bool operator>(V&& o, const Rational<Infinity, NaN>& r) {
  return r < std::forward<V>(o);
}
template <bool Infinity, bool NaN, concepts::is_not<Rational<Infinity, NaN>> V>
bool operator>=(V&& o, const Rational<Infinity, NaN>& r) {
  return r <= std::forward<V>(o);
}

template <bool Infinity, bool NaN>
std::ostream& operator<<(std::ostream& os, const Rational<Infinity, NaN>& r);
}  // namespace lightrat

/**
 * Hash specialization for lightrat::Rational.
 * @tparam Infinity Whether the Rational supports infinity values.
 * @tparam NaN Whether the Rational supports NaN values.
 */
template <bool Infinity, bool NaN>
struct std::hash<lightrat::Rational<Infinity, NaN>> {
  std::size_t operator()(const lightrat::Rational<Infinity, NaN>& rat) const noexcept { return rat.hash(); }
};
