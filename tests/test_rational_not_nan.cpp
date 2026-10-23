/**
 * @author TendTo (https://github.com/TendTo)
 * @copyright 2026 lightrat
 * @licence Apache-2.0 license
 * @file
 */
#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "lightrat/Rational.h"
#include "lightrat/errors.h"
#include "tests/utils.hpp"

using Rational = lightrat::Rational<true, false>;
using string_parsing_error = lightrat::errors::string_parsing_error;
using nan_error = lightrat::errors::nan_error;

TEST(TestRationalNotNaN, DefaultConstructor) {
  const Rational r{};
  EXPECT_EQ(r.num(), 0);
  EXPECT_EQ(r.den(), 1U);
  EXPECT_TRUE(r.is_finite());
  EXPECT_FALSE(r.is_ratgmp());
  EXPECT_TRUE(r.is_rat32());
}

TEST(TestRationalNotNaN, Infinity) {
  const Rational r{Rational::infinity()};
  EXPECT_TRUE(r.is_infinity());
  EXPECT_FALSE(r.is_ninfinity());
  EXPECT_FALSE(r.is_finite());
  EXPECT_FALSE(r.is_ratgmp());
  EXPECT_TRUE(r.is_rat32());
  EXPECT_EQ((*r).s.num, 1);
  EXPECT_EQ((*r).s.den, 0u);
}

TEST(TestRationalNotNaN, NegativeInfinity) {
  const Rational r{Rational::ninfinity()};
  EXPECT_FALSE(r.is_infinity());
  EXPECT_TRUE(r.is_ninfinity());
  EXPECT_FALSE(r.is_finite());
  EXPECT_FALSE(r.is_ratgmp());
  EXPECT_TRUE(r.is_rat32());
  EXPECT_EQ((*r).s.num, -1);
  EXPECT_EQ((*r).s.den, 0u);
}

TEST(TestRationalNotNaN, DoubleConstructor) {
  EXPECT_EQ(Rational{0.5}.str(), "1/2");
  EXPECT_EQ(Rational{-0.5}.str(), "-1/2");
  EXPECT_EQ(Rational{-0.0}.str(), "0");
  EXPECT_EQ(Rational{0.0}.str(), "0");
  EXPECT_EQ(Rational{11.0}.str(), "11");
  EXPECT_EQ(Rational{-11.0}.str(), "-11");
  EXPECT_EQ(Rational{std::numeric_limits<double>::max()}.str(),
            mpq_class{std::numeric_limits<double>::max()}.get_str());
  EXPECT_EQ(Rational{std::numeric_limits<double>::min()}.str(),
            mpq_class{std::numeric_limits<double>::min()}.get_str());
  EXPECT_EQ(Rational{std::numeric_limits<double>::infinity()}.str(), "inf");
  EXPECT_EQ(Rational{-std::numeric_limits<double>::infinity()}.str(), "-inf");
  EXPECT_THROW(Rational{std::numeric_limits<double>::quiet_NaN()}, nan_error);
}

TEST(TestRationalNotNaN, UtilitySetters) {
  EXPECT_EQ(Rational{}.set_one().str(), "1");
  EXPECT_EQ(Rational{}.set_minus_one().str(), "-1");
  EXPECT_EQ(Rational{}.set_infinity().str(), "inf");
  EXPECT_EQ(Rational{}.set_ninfinity().str(), "-inf");
  EXPECT_THROW(Rational{}.set_nan(), nan_error);
}

TEST(TestRational, Setter32) {
  Rational r{};
  EXPECT_EQ(r.set(std::int32_t{0}, 1).str(), "0");
  EXPECT_EQ(r.set(std::int32_t{-0}, 1).str(), "0");
  EXPECT_EQ(r.set(std::int32_t{5}, 7).str(), "5/7");
  EXPECT_EQ(r.set(std::int32_t{-5}, 7).str(), "-5/7");
  EXPECT_EQ(r.set(std::int32_t{10}, 5).str(), "2");
  EXPECT_EQ(r.set(std::int32_t{-10}, 5).str(), "-2");
  EXPECT_EQ(r.set(std::int32_t{12}, 8).str(), "3/2");
  EXPECT_EQ(r.set(std::int32_t{-12}, 8).str(), "-3/2");
  EXPECT_EQ(r.set(std::numeric_limits<std::int32_t>::max(), 2).str(),
            std::to_string(std::numeric_limits<std::int32_t>::max()) + "/2");
  EXPECT_EQ(r.set(std::numeric_limits<std::int32_t>::min() + 1, 2).str(),
            std::to_string(std::numeric_limits<std::int32_t>::min() + 1) + "/2");
  EXPECT_EQ(r.set(std::int32_t{1}, 0).str(), "inf");
  EXPECT_EQ(r.set(std::int32_t{-1}, 0).str(), "-inf");
  EXPECT_THROW(r.set(std::int32_t{0}, 0), nan_error);
}

TEST(TestRational, Setter64) {
  Rational r{};
  EXPECT_EQ(r.set(std::int64_t{0}, 1).str(), "0");
  EXPECT_EQ(r.set(std::int64_t{-0}, 1).str(), "0");
  EXPECT_EQ(r.set(std::int64_t{5}, 7).str(), "5/7");
  EXPECT_EQ(r.set(std::int64_t{-5}, 7).str(), "-5/7");
  EXPECT_EQ(r.set(std::int64_t{10}, 5).str(), "2");
  EXPECT_EQ(r.set(std::int64_t{-10}, 5).str(), "-2");
  EXPECT_EQ(r.set(std::int64_t{12}, 8).str(), "3/2");
  EXPECT_EQ(r.set(std::int64_t{-12}, 8).str(), "-3/2");
  EXPECT_EQ(r.set(std::numeric_limits<std::int64_t>::max(), 2).str(),
            std::to_string(std::numeric_limits<std::int64_t>::max()) + "/2");
  EXPECT_EQ(r.set(std::numeric_limits<std::int64_t>::min() + 1, 2).str(),
            std::to_string(std::numeric_limits<std::int64_t>::min() + 1) + "/2");
  EXPECT_EQ(r.set(std::int64_t{1}, 0).str(), "inf");
  EXPECT_EQ(r.set(std::int64_t{-1}, 0).str(), "-inf");
  EXPECT_THROW(r.set(std::int64_t{0}, 0), nan_error);
}

TEST(TestRational, SetterMpz) {
  Rational r{};
  EXPECT_EQ(r.set(mpz_class{0}, 1).str(), "0");
  EXPECT_EQ(r.set(mpz_class{-0}, 1).str(), "0");
  EXPECT_EQ(r.set(mpz_class{5}, 7).str(), "5/7");
  EXPECT_EQ(r.set(mpz_class{-5}, 7).str(), "-5/7");
  EXPECT_EQ(r.set(mpz_class{10}, 5).str(), "2");
  EXPECT_EQ(r.set(mpz_class{-10}, 5).str(), "-2");
  EXPECT_EQ(r.set(mpz_class{12}, 8).str(), "3/2");
  EXPECT_EQ(r.set(mpz_class{-12}, 8).str(), "-3/2");
  EXPECT_EQ(r.set(mpz_class{1}, 0).str(), "inf");
  EXPECT_EQ(r.set(mpz_class{-1}, 0).str(), "-inf");
  EXPECT_THROW(r.set(mpz_class{0}, 0), nan_error);
}

TEST(TestRational, StringConstructorMatchesStringRepresentation) {
  const std::array rs{Rational{"0"},    Rational{"-0"},    Rational{"5/7"},  Rational{"-5/7"},
                      Rational{"10/5"}, Rational{"-10/5"}, Rational{"12/8"}, Rational{"-12/8"},
                      Rational{"1/0"},  Rational{"-1/0"},  Rational{"-inf"}, Rational{"inf"}};
  for (const auto& r : rs) {
    EXPECT_EQ(Rational{r.str()}.str(), r.str());
    EXPECT_EQ(Rational{r.str()}, r);
  }
}

TEST(TestRational, StringParsingFailures) {
  EXPECT_THROW(Rational{"nan"}, string_parsing_error);
  EXPECT_EXIT(Rational{"0/0"}, ExitedWithError, "");
}

TEST(TestRationalNotNaN, ConstructsAndNormalizesValues) {
  EXPECT_EQ((Rational{6, 8}).str(), "3/4");
  EXPECT_EQ((Rational{-6, 8}).str(), "-3/4");
  EXPECT_EQ((Rational{std::int64_t{1} << 40, std::uint64_t{1}}).str(), "1099511627776");
  EXPECT_EQ((Rational{std::numeric_limits<std::int32_t>::min(), 2}).str(), "-1073741824");
  EXPECT_EQ((Rational{std::numeric_limits<std::int64_t>::min(), std::uint64_t{2}}).str(), "-4611686018427387904");
  EXPECT_EQ(Rational{"-12/18"}.str(), "-2/3");
  EXPECT_EQ(Rational{mpz_class{"123456789012345678901234567890"}}.str(), "123456789012345678901234567890");
  EXPECT_EQ(Rational{0.5}.str(), "1/2");
  EXPECT_TRUE((Rational{std::int64_t{1} << 40, 1}).is_ratgmp());
  EXPECT_TRUE((Rational{std::numeric_limits<std::int32_t>::max() - 1, 2}).is_rat32());
}

TEST(TestRationalNotNaN, ConstructsNotNormalizedValues) {
  // We assume that the input mpq_class is normalized!
  EXPECT_EQ(Rational{mpq_class{"10/20"}}.str(), "10/20");
  EXPECT_EQ((Rational{mpq_class{std::numeric_limits<std::int32_t>::max() - 1, 2}}).str(), "2147483646/2");
}

TEST(TestRationalNotNaN, InvalidInputs) {
  EXPECT_TRUE((Rational{1, 0}).is_infinity());
  EXPECT_TRUE((Rational{-1, 0}).is_ninfinity());
  EXPECT_THROW((Rational{0, 0}), nan_error);
  EXPECT_TRUE((Rational{std::int64_t{1}, std::uint64_t{0}}).is_infinity());
  EXPECT_TRUE((Rational{std::int64_t{-1}, std::uint64_t{0}}).is_ninfinity());
  EXPECT_THROW((Rational{std::int64_t{0}, std::uint64_t{0}}), nan_error);
  EXPECT_THROW(Rational{"not a rational"}, string_parsing_error);
  EXPECT_THROW(Rational{"infinity"}, string_parsing_error);
  EXPECT_THROW(Rational{"-infinity"}, string_parsing_error);
  EXPECT_THROW(Rational{"inf/2"}, string_parsing_error);
  EXPECT_TRUE((Rational{mpz_class{1}, mpz_class{0}}).is_infinity());
  EXPECT_TRUE((Rational{mpz_class{-1}, mpz_class{0}}).is_ninfinity());
  EXPECT_THROW((Rational{mpz_class{0}, mpz_class{0}}), nan_error);
}

TEST(TestRationalNotNaN, SetAfterNaN) {
  Rational value{5, 7};
  EXPECT_THROW(value.set(0, 0), nan_error);
  EXPECT_EQ(value.str(), "5/7");

  mpz_class numerator{0};
  mpz_class zero{0};
  EXPECT_THROW(value.set(numerator, zero), nan_error);
  EXPECT_EQ(value.str(), "5/7");

  value = Rational{5, 7};
  EXPECT_TRUE((value /= Rational{}).is_infinity());
  EXPECT_EQ(value.str(), "inf");
  EXPECT_THROW(value.invert(), nan_error);
  EXPECT_EQ(value.str(), "inf");
}

TEST(TestRationalNotNaN, PredicatesAndMutators) {
  Rational r{-6, 8};
  EXPECT_TRUE(r.is_neg());
  EXPECT_TRUE(r.is_nonpos());
  EXPECT_FALSE(r.is_pos());
  EXPECT_FALSE(r.is_integer());
  EXPECT_FALSE(r.is_zero());
  EXPECT_TRUE(r.is_nonzero());
  EXPECT_FALSE(r.is_one());
  EXPECT_FALSE(r.is_minus_one());

  r.abs();
  EXPECT_EQ(r.str(), "3/4");
  r.negate();
  EXPECT_EQ(r.str(), "-3/4");
  r.invert();
  EXPECT_EQ(r.str(), "-4/3");
  r.set_one();
  EXPECT_TRUE(r.is_one());
  r.set_minus_one();
  EXPECT_TRUE(r.is_minus_one());
  r.clear();
  EXPECT_TRUE(r.is_zero());
  EXPECT_EQ(r.str(), "0");
  r.set_infinity();
  EXPECT_TRUE(r.is_infinity());
  r.set_ninfinity();
  EXPECT_TRUE(r.is_ninfinity());
  r.set(12, 18);
  EXPECT_EQ(r.str(), "2/3");
  r.normalize();
  EXPECT_EQ(r.str(), "2/3");

  EXPECT_THROW(Rational{}.invert(), nan_error);
  EXPECT_THROW(Rational::infinity().invert(), nan_error);
}

TEST(TestRationalNotNaN, ArithmeticWithRationals) {
  EXPECT_EQ((Rational{1, 3} + Rational{1, 6}).str(), "1/2");
  EXPECT_EQ((Rational{1, 3} - Rational{1, 6}).str(), "1/6");
  EXPECT_EQ((Rational{2, 3} * Rational{-9, 4}).str(), "-3/2");
  EXPECT_EQ((Rational{2, 3} / Rational{-4, 9}).str(), "-3/2");

  Rational r{1, 3};
  r += Rational{1, 6};
  r -= Rational{-1, 2};
  r *= Rational{-4, 3};
  r /= Rational{2, 1};
  EXPECT_EQ(r.str(), "-2/3");
  EXPECT_TRUE((Rational{1, 3} + Rational{2, 3}).is_integer());
  EXPECT_EQ((-Rational{1, 3}).str(), "-1/3");
}

TEST(TestRationalNotNaN, ArithmeticWithScalarAndGmpValues) {
  mpq_class q{"5/6"};
  mpz_class z{2};
  EXPECT_EQ((Rational{1, 6} + 1).str(), "7/6");
  EXPECT_EQ((Rational{1, 6} - 1).str(), "-5/6");
  EXPECT_EQ((Rational{3, 2} * 4).str(), "6");
  EXPECT_EQ((Rational{3, 2} / 4).str(), "3/8");
  EXPECT_EQ((Rational{1, 6} + q).str(), "1");
  EXPECT_EQ((Rational{1, 6} - q).str(), "-2/3");
  EXPECT_EQ((Rational{3, 2} * z).str(), "3");
  EXPECT_EQ((Rational{3, 2} / z).str(), "3/4");

  const Rational large{std::int64_t{1} << 40};
  EXPECT_EQ((large + 1).str(), "1099511627777");
  EXPECT_EQ((large - 1).str(), "1099511627775");
  EXPECT_EQ((large * 2).str(), "2199023255552");
  EXPECT_EQ((large / 2).str(), "549755813888");
  EXPECT_EQ((Rational{0.25} + 0.25).str(), "1/2");
}

TEST(TestRationalNotNaN, ExhaustiveSmallArithmeticMatchesGmp) {
  for (std::int32_t left_num = -7; left_num <= 7; ++left_num) {
    for (std::uint32_t left_den = 1; left_den <= 7; ++left_den) {
      Rational left{left_num, left_den};
      mpq_class mpq_left{left_num, left_den};
      mpq_left.canonicalize();
      EXPECT_EQ(static_cast<mpq_class>(left), mpq_left);
      EXPECT_TRUE(left.is_rat32());

      for (std::int32_t right_num = -7; right_num <= 7; ++right_num) {
        for (std::uint32_t right_den = 1; right_den <= 7; ++right_den) {
          const Rational right{right_num, right_den};
          mpq_class mpq_right{right_num, right_den};
          mpq_right.canonicalize();
          EXPECT_TRUE(right.is_rat32());
          EXPECT_EQ(static_cast<mpq_class>(left + right), mpq_left + mpq_right);
          EXPECT_EQ(static_cast<mpq_class>(left - right), mpq_left - mpq_right);
          EXPECT_EQ(static_cast<mpq_class>(left * right), mpq_left * mpq_right);
          if (right_num != 0) {
            EXPECT_EQ(static_cast<mpq_class>(left / right), mpq_left / mpq_right);
          }
          const int expected_compare = mpq_left < mpq_right ? -1 : (mpq_left > mpq_right ? 1 : 0);
          const int actual_compare = left.compare(right) < 0 ? -1 : (left.compare(right) > 0 ? 1 : 0);
          EXPECT_EQ(actual_compare, expected_compare);
        }
      }
    }
  }
}

TEST(TestRationalNotNaN, ExhaustiveBigArithmeticMatchesGmp) {
  for (std::int64_t left_num = std::uint64_t{1} << 40; left_num <= std::int64_t{1} << 50; left_num <<= 1) {
    for (std::uint64_t left_den = (std::uint64_t{1} << 40) + 1; left_den <= std::uint64_t{1} << 50;
         left_den = ((left_den - 1) << 1) + 1) {  // Ensure left_den is odd to avoid gcd simplification
      Rational left{left_num, left_den};
      mpq_class mpq_left;
      mpq_set_i64(mpq_left.get_mpq_t(), left_num, left_den);
      EXPECT_EQ(static_cast<mpq_class>(left), mpq_left);
      EXPECT_TRUE(left.is_ratgmp());

      for (std::int64_t right_num = std::int64_t{1} << 40; right_num <= std::int64_t{1} << 50; right_num <<= 1) {
        for (std::uint64_t right_den = (std::uint64_t{1} << 40) + 1; right_den <= std::uint64_t{1} << 50;
             right_den = ((right_den - 1) << 1) + 1) {  // Ensure right_den is odd to avoid gcd simplification
          const Rational right{right_num, right_den};
          mpq_class mpq_right;
          mpq_set_i64(mpq_right.get_mpq_t(), right_num, right_den);
          EXPECT_TRUE(right.is_ratgmp());
          EXPECT_EQ(static_cast<mpq_class>(right), mpq_right);
          EXPECT_EQ(static_cast<mpq_class>(left + right), mpq_left + mpq_right);
          EXPECT_EQ(static_cast<mpq_class>(left - right), mpq_left - mpq_right);
          EXPECT_EQ(static_cast<mpq_class>(left * right), mpq_left * mpq_right);
          if (right_num != 0) {
            EXPECT_EQ(static_cast<mpq_class>(left / right), mpq_left / mpq_right);
          }
          const int expected_compare = mpq_left < mpq_right ? -1 : (mpq_left > mpq_right ? 1 : 0);
          const int actual_compare = left.compare(right) < 0 ? -1 : (left.compare(right) > 0 ? 1 : 0);
          EXPECT_EQ(actual_compare, expected_compare);
        }
      }
    }
  }
}

TEST(TestRationalNotNaN, ExhaustiveMixedArithmeticMatchesGmp) {
  for (std::int64_t left_num = std::uint64_t{1} << 40; left_num <= std::int64_t{1} << 50; left_num <<= 1) {
    for (std::uint64_t left_den = (std::uint64_t{1} << 40) + 1; left_den <= std::uint64_t{1} << 50;
         left_den = ((left_den - 1) << 1) + 1) {  // Ensure left_den is odd to avoid gcd simplification
      Rational left{left_num, left_den};
      mpq_class mpq_left;
      mpq_set_i64(mpq_left.get_mpq_t(), left_num, left_den);
      EXPECT_EQ(static_cast<mpq_class>(left), mpq_left);
      EXPECT_TRUE(left.is_ratgmp());

      for (std::int32_t right_num = -7; right_num <= 7; ++right_num) {
        for (std::uint32_t right_den = 1; right_den <= 7; ++right_den) {
          const Rational right{right_num, right_den};
          mpq_class mpq_right{right_num, right_den};
          mpq_right.canonicalize();
          EXPECT_TRUE(right.is_rat32());
          EXPECT_EQ(static_cast<mpq_class>(left + right), mpq_left + mpq_right);
          EXPECT_EQ(static_cast<mpq_class>(left - right), mpq_left - mpq_right);
          EXPECT_EQ(static_cast<mpq_class>(left * right), mpq_left * mpq_right);
          if (right_num != 0) {
            EXPECT_EQ(static_cast<mpq_class>(left / right), mpq_left / mpq_right);
          }
          const int expected_compare = mpq_left < mpq_right ? -1 : (mpq_left > mpq_right ? 1 : 0);
          const int actual_compare = left.compare(right) < 0 ? -1 : (left.compare(right) > 0 ? 1 : 0);
          EXPECT_EQ(actual_compare, expected_compare);
        }
      }
    }
  }
}

TEST(TestRationalNotNaN, DivisionByZeroAndOverflowBoundaries) {
  EXPECT_TRUE((Rational{1, 2} / Rational{}).is_infinity());
  EXPECT_THROW(Rational{}.invert(), nan_error);
  EXPECT_TRUE(Rational(1, 2).operator/=(0).is_infinity());

  const auto max = std::numeric_limits<std::int32_t>::max() >> 1;
  Rational sum{max};
  sum += 1;
  EXPECT_TRUE(sum.is_ratgmp());
  EXPECT_EQ(sum.str(), "1073741824");
  Rational difference{-max};
  difference -= 1;
  EXPECT_TRUE(difference.is_ratgmp());
  EXPECT_EQ(difference.str(), "-1073741824");
}

TEST(TestRationalNotNaN, InfinityArithmeticAndErrors) {
  const Rational inf = Rational::infinity();
  const Rational ninf = Rational::ninfinity();
  EXPECT_EQ((inf + 1).str(), "inf");
  EXPECT_EQ((ninf + 1).str(), "-inf");
  EXPECT_EQ((inf + inf).str(), "inf");
  EXPECT_EQ((ninf + ninf).str(), "-inf");
  EXPECT_THROW(inf + ninf, nan_error);
  EXPECT_THROW(ninf + inf, nan_error);
  EXPECT_EQ((inf - ninf).str(), "inf");
  EXPECT_EQ((ninf - inf).str(), "-inf");
  EXPECT_EQ(Rational{1} + inf.str(), "inf");
  EXPECT_EQ(Rational{1} + ninf.str(), "-inf");
  EXPECT_EQ((inf - 1).str(), "inf");
  EXPECT_EQ((ninf - 1).str(), "-inf");
  EXPECT_EQ((ninf - inf).str(), "-inf");
  EXPECT_EQ((inf - ninf).str(), "inf");
  EXPECT_EQ(Rational{1} - inf.str(), "-inf");
  EXPECT_EQ((inf * inf).str(), "inf");
  EXPECT_EQ((inf * ninf).str(), "-inf");
  EXPECT_EQ((ninf * inf).str(), "-inf");
  EXPECT_EQ((ninf * ninf).str(), "inf");
  EXPECT_EQ((inf * 2).str(), "inf");
  EXPECT_EQ((inf * -2).str(), "-inf");
  EXPECT_EQ((inf / -2).str(), "-inf");
  EXPECT_THROW(inf + ninf, nan_error);
  EXPECT_THROW(inf - inf, nan_error);
  EXPECT_THROW(inf * 0, nan_error);
  EXPECT_THROW(Rational{} * inf, nan_error);
  EXPECT_THROW(inf / inf, nan_error);
}

TEST(TestRationalNotNaN, ComparisonsConversionsAndOutput) {
  const Rational r{2, 3};
  const Rational same{2, 3};
  EXPECT_LT(r, 1);
  EXPECT_GT(r, 0);
  EXPECT_LE(r, same);
  EXPECT_GE(r, same);
  EXPECT_EQ(r, mpq_class{"4/6"});
  EXPECT_NE(r, mpz_class{1});
  EXPECT_EQ(r.compare(mpq_class{"2/3"}.get_mpq_t()), 0);
  EXPECT_LT(Rational::ninfinity(), r);
  EXPECT_GT(Rational::infinity(), r);
  EXPECT_EQ(Rational::infinity(), Rational::infinity());
  EXPECT_EQ(Rational::ninfinity(), Rational::ninfinity());
  EXPECT_LT(Rational::ninfinity(), Rational::infinity());
  EXPECT_EQ(static_cast<double>(r), 2.0 / 3.0);
  EXPECT_EQ(static_cast<std::int64_t>(Rational{9, 2}), 4);
  EXPECT_EQ(static_cast<std::uint64_t>(Rational{9, 2}), 4U);
  EXPECT_EQ(static_cast<mpq_class>(r), mpq_class{"2/3"});
  EXPECT_EQ(static_cast<double>(Rational::infinity()), std::numeric_limits<double>::infinity());

  std::ostringstream output;
  output << r;
  EXPECT_EQ(output.str(), "2/3");
}

TEST(TestRationalNotNaN, CopyMoveSwapAndAssignments) {
  Rational original{std::int64_t{1} << 40, 3};
  Rational copy{original};
  Rational assigned;
  assigned = original;
  EXPECT_EQ(copy.str(), "1099511627776/3");
  EXPECT_EQ(assigned.str(), "1099511627776/3");
  Rational moved{std::move(copy)};
  EXPECT_EQ(moved.str(), "1099511627776/3");
  Rational move_assigned;
  move_assigned = std::move(assigned);
  EXPECT_EQ(move_assigned.str(), "1099511627776/3");

  Rational left{1, 2};
  Rational right{3, 4};
  left.swap(right);
  EXPECT_EQ(left.str(), "3/4");
  EXPECT_EQ(right.str(), "1/2");
  left = "5/6";
  right = std::int64_t{1} << 40;
  EXPECT_EQ(left.str(), "5/6");
  EXPECT_EQ(right.str(), "1099511627776");
}

TEST(TestRationalNotNaN, GmpPayloadLifecycleAndRepresentationTransitions) {
  Rational value{std::int64_t{1} << 40, 3};
  ASSERT_TRUE(value.is_ratgmp());

  value = std::int64_t{1} << 40;
  ASSERT_TRUE(value.is_ratgmp());
  value += Rational{1, 3};
  EXPECT_EQ(value.str(), "3298534883329/3");
  value.clear();
  EXPECT_TRUE(value.is_rat32());
  EXPECT_EQ(value.str(), "0");

  value = Rational{std::int64_t{1} << 40, 3};
  Rational moved{std::move(value)};
  EXPECT_EQ(moved.str(), "1099511627776/3");
  EXPECT_EQ(value.str(), "0");
  value = std::move(moved);
  EXPECT_EQ(value.str(), "1099511627776/3");
  EXPECT_EQ(moved.str(), "0");
}

TEST(TestRationalNotNaN, SelfAssignmentAndSelfMoveAreNoOps) {
  Rational value{std::int64_t{1} << 40, 3};
  value = value;
  EXPECT_EQ(value.str(), "1099511627776/3");
  value = std::move(value);
  EXPECT_EQ(value.str(), "1099511627776/3");
}

TEST(TestRationalNotNaN, ZeroBehavior) {
  const Rational zero{};
  constexpr double zero_double = 0.0;
  EXPECT_TRUE(zero.is_zero());
  EXPECT_FALSE(zero.is_nonzero());
  EXPECT_FALSE(zero.is_pos());
  EXPECT_TRUE(zero.is_nonneg());
  EXPECT_FALSE(zero.is_neg());
  EXPECT_TRUE(zero.is_nonpos());
  EXPECT_FALSE(zero.is_one());
  EXPECT_FALSE(zero.is_minus_one());
  EXPECT_TRUE(zero.is_integer());
  EXPECT_FALSE(zero.is_infinity());
  EXPECT_FALSE(zero.is_ninfinity());
  EXPECT_FALSE(zero.is_nan());
  EXPECT_TRUE(zero.is_finite());

  EXPECT_EQ(zero, zero_double);
  EXPECT_EQ(static_cast<double>(zero), zero_double);

  EXPECT_EQ(zero + zero, zero_double + zero_double);
  EXPECT_EQ(zero - zero, zero_double - zero_double);
  EXPECT_EQ(zero * zero, zero_double * zero_double);
  EXPECT_THROW(zero / zero, nan_error);

  EXPECT_EQ(zero + Rational{1}, zero_double + 1.0);
  EXPECT_EQ(zero - Rational{1}, zero_double - 1.0);
  EXPECT_EQ(zero * Rational{2}, zero_double * 2.0);
  EXPECT_EQ(zero * Rational{-2}, zero_double * -2.0);
  EXPECT_EQ(zero / Rational{2}, zero_double * 2.0);
  EXPECT_EQ(zero / Rational{-2}, zero_double / -2.0);

  EXPECT_EQ(Rational{1} + zero, 1.0 + zero_double);
  EXPECT_EQ(Rational{1} - zero, 1.0 - zero_double);
  EXPECT_EQ(Rational{1} * zero, 1.0 * zero_double);
#ifdef _MSC_VER
  EXPECT_TRUE((Rational{1} / zero).is_infinity());
  EXPECT_TRUE((Rational{-1} / zero).is_ninfinity());
#else
  EXPECT_EQ(Rational{1} / zero, 1.0 / zero_double);
  EXPECT_EQ(Rational{-1} / zero, -1.0 / zero_double);
#endif
}

TEST(TestRationalNotNaN, InfinityBehavior) {
  const Rational inf = Rational::infinity();
  constexpr double inf_double = std::numeric_limits<double>::infinity();
  EXPECT_FALSE(inf.is_zero());
  EXPECT_TRUE(inf.is_nonzero());
  EXPECT_TRUE(inf.is_pos());
  EXPECT_TRUE(inf.is_nonneg());
  EXPECT_FALSE(inf.is_neg());
  EXPECT_FALSE(inf.is_nonpos());
  EXPECT_FALSE(inf.is_one());
  EXPECT_FALSE(inf.is_minus_one());
  EXPECT_FALSE(inf.is_integer());
  EXPECT_TRUE(inf.is_infinity());
  EXPECT_FALSE(inf.is_ninfinity());
  EXPECT_FALSE(inf.is_nan());
  EXPECT_FALSE(inf.is_finite());

  EXPECT_TRUE(std::isinf(static_cast<double>(inf)));

  EXPECT_EQ(inf, inf_double);
  EXPECT_EQ(inf + inf, inf_double + inf_double);
  EXPECT_THROW(inf - inf, nan_error);
  EXPECT_EQ(inf * inf, inf * inf_double);
  EXPECT_THROW(inf / inf, nan_error);

  EXPECT_EQ(inf + Rational{1}, inf_double + 1.0);
  EXPECT_EQ(inf - Rational{1}, inf_double - 1.0);
  EXPECT_EQ(inf * Rational{2}, inf_double * 2.0);
  EXPECT_EQ(inf * Rational{-2}, inf_double * -2.0);
  EXPECT_EQ(inf / Rational{2}, inf_double * 2.0);
  EXPECT_EQ(inf / Rational{-2}, inf_double / -2.0);

  EXPECT_EQ(Rational{1} + inf, 1.0 + inf_double);
  EXPECT_EQ(Rational{1} - inf, 1.0 - inf_double);
  EXPECT_EQ(Rational{1} * inf, 1.0 * inf_double);
  EXPECT_EQ(Rational{1} / inf, 1.0 / inf_double);

  EXPECT_THROW(Rational{0} * inf, nan_error);
  EXPECT_THROW(inf * Rational{0}, nan_error);
#ifdef _MSC_VER
  EXPECT_TRUE((inf / Rational{0}).is_infinity());
#else
  EXPECT_EQ(inf / Rational{0}, inf_double / 0.0);
#endif
  EXPECT_EQ(Rational{0} / inf, 0.0 / inf_double);
}

TEST(TestRationalNotNaN, NegativeInfinityBehavior) {
  const Rational ninf = Rational::ninfinity();
  constexpr double ninf_double = -std::numeric_limits<double>::infinity();
  EXPECT_FALSE(ninf.is_zero());
  EXPECT_TRUE(ninf.is_nonzero());
  EXPECT_FALSE(ninf.is_pos());
  EXPECT_FALSE(ninf.is_nonneg());
  EXPECT_TRUE(ninf.is_neg());
  EXPECT_TRUE(ninf.is_nonpos());
  EXPECT_FALSE(ninf.is_one());
  EXPECT_FALSE(ninf.is_minus_one());
  EXPECT_FALSE(ninf.is_integer());
  EXPECT_FALSE(ninf.is_infinity());
  EXPECT_TRUE(ninf.is_ninfinity());
  EXPECT_FALSE(ninf.is_nan());
  EXPECT_FALSE(ninf.is_finite());

  EXPECT_TRUE(std::isinf(static_cast<double>(ninf)));

  EXPECT_EQ(ninf, ninf_double);
  EXPECT_EQ(ninf + ninf, ninf + ninf_double);
  EXPECT_THROW(ninf - ninf, nan_error);
  EXPECT_EQ(ninf * ninf, ninf * ninf_double);
  EXPECT_THROW(ninf / ninf, nan_error);

  EXPECT_EQ(ninf + Rational{1}, ninf_double + 1.0);
  EXPECT_EQ(ninf - Rational{1}, ninf_double - 1.0);
  EXPECT_EQ(ninf * Rational{2}, ninf_double * 2.0);
  EXPECT_EQ(ninf * Rational{-2}, ninf_double * -2.0);
  EXPECT_EQ(ninf / Rational{2}, ninf_double * 2.0);
  EXPECT_EQ(ninf / Rational{-2}, ninf_double / -2.0);

  EXPECT_EQ(Rational{1} + ninf, 1.0 + ninf_double);
  EXPECT_EQ(Rational{1} - ninf, 1.0 - ninf_double);
  EXPECT_EQ(Rational{1} * ninf, 1.0 * ninf_double);
  EXPECT_EQ(Rational{1} / ninf, 1.0 / ninf_double);

  EXPECT_THROW(Rational{0} * ninf, nan_error);
  EXPECT_THROW(ninf * Rational{0}, nan_error);
  EXPECT_EQ(Rational{0} / ninf, 0.0 / ninf_double);
#ifdef _MSC_VER
  EXPECT_TRUE((ninf / Rational{0}).is_ninfinity());
#else
  EXPECT_EQ(ninf / Rational{0}, ninf_double / 0.0);
#endif
}

TEST(TestRationalNotNaN, DoubleInfinityBehavior) {
  const Rational inf = Rational::infinity();
  const Rational ninf = Rational::ninfinity();
  constexpr double inf_double = std::numeric_limits<double>::infinity();
  constexpr double ninf_double = -std::numeric_limits<double>::infinity();
  EXPECT_NE(inf, ninf);
  EXPECT_LT(ninf, inf);
  EXPECT_LT(ninf, inf_double);
  EXPECT_GT(inf, ninf_double);

  EXPECT_THROW(inf + ninf, nan_error);
  EXPECT_THROW(ninf + inf, nan_error);
  EXPECT_EQ(inf - ninf, inf_double - ninf_double);
  EXPECT_EQ(ninf - inf, ninf_double - inf_double);
  EXPECT_EQ(inf * ninf, inf_double * ninf_double);
  EXPECT_EQ(ninf * inf, ninf_double * inf_double);
  EXPECT_THROW(inf / ninf, nan_error);
  EXPECT_THROW(ninf / inf, nan_error);
}
