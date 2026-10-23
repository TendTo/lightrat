/**
 * @author TendTo (https://github.com/TendTo)
 * @copyright 2026 lightrat
 * @licence Apache-2.0 license
 * @file
 */
#include <gtest/gtest.h>

#include <cstdint>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <string>

#include "lightrat/lightrat.h"

namespace {

struct TestRat {
  lightrat_t value{};

  TestRat() { lightrat_init(value); }
  ~TestRat() { lightrat_clear(value); }
  lightrat_t& operator*() { return value; }

  TestRat(const TestRat&) = delete;
  TestRat& operator=(const TestRat&) = delete;
};

std::string str(const lightrat_t rat) {
  char* const value = lightrat_to_str(rat);
  EXPECT_NE(value, nullptr);
  if (value == nullptr) {
    return {};
  }
  std::string result{value};
  std::free(value);
  return result;
}

}  // namespace

TEST(TestLightrat, DefaultConstructor) {
  TestRat r;
  EXPECT_FALSE(lightrat_is_nan(*r));
  EXPECT_EQ(lightrat_get_num(*r), 0);
  EXPECT_EQ(lightrat_get_den(*r), 1U);
  EXPECT_TRUE(lightrat_is_zero(*r));
  EXPECT_FALSE(lightrat_is_gmp(*r));
  EXPECT_TRUE(lightrat_is_rat32(*r));
  EXPECT_EQ(str(*r), "0");
}

TEST(TestLightrat, Infinity) {
  TestRat r;
  lightrat_set_infinity(*r);
  EXPECT_TRUE(lightrat_is_infinity(*r));
  EXPECT_FALSE(lightrat_is_ninfinity(*r));
  EXPECT_FALSE(lightrat_is_gmp(*r));
  EXPECT_TRUE(lightrat_is_rat32(*r));
  EXPECT_EQ((*r)->s.num, 1);
  EXPECT_EQ((*r)->s.den, 0U);
  EXPECT_EQ(str(*r), "inf");
}

TEST(TestLightrat, NegativeInfinity) {
  TestRat r;
  lightrat_set_ninfinity(*r);
  EXPECT_FALSE(lightrat_is_infinity(*r));
  EXPECT_TRUE(lightrat_is_ninfinity(*r));
  EXPECT_FALSE(lightrat_is_gmp(*r));
  EXPECT_TRUE(lightrat_is_rat32(*r));
  EXPECT_EQ((*r)->s.num, -1);
  EXPECT_EQ((*r)->s.den, 0U);
  EXPECT_EQ(str(*r), "-inf");
}

TEST(TestLightrat, ConstructsAndNormalizesValues) {
  TestRat r;
  lightrat_set_si(*r, 6, 8);
  EXPECT_EQ(str(*r), "3/4");

  lightrat_set_si(*r, -6, 8);
  EXPECT_EQ(str(*r), "-3/4");

  lightrat_set_d(*r, 0.5);
  EXPECT_EQ(str(*r), "1/2");

  mpq_t q;
  mpq_init(q);
  mpq_set_str(q, "123456789012345678901234567890", 10);
  mpq_canonicalize(q);
  lightrat_set_gmp(*r, q);
  EXPECT_EQ(str(*r), "123456789012345678901234567890");

  EXPECT_TRUE(lightrat_is_gmp(*r));
  mpq_clear(q);

  lightrat_set_si(*r, std::numeric_limits<std::int32_t>::max() - 1, 2);
  EXPECT_TRUE(lightrat_is_rat32(*r));
  EXPECT_EQ(str(*r), "1073741823");
}

TEST(TestLightrat, ConstructsNotNormalizedValues) {
  // We assume that the input mpq_t is normalized!
  TestRat r;
  mpq_t q;
  mpq_init(q);
  mpq_set_str(q, "10/20", 10);
  lightrat_set_gmp(*r, q);
  EXPECT_EQ(str(*r), "10/20");

  mpq_set_si(q, std::numeric_limits<std::int32_t>::max() - 1, 2);
  lightrat_set_gmp(*r, q);
  EXPECT_EQ(str(*r), "2147483646/2");
  mpq_clear(q);
}

TEST(TestLightrat, InvalidInputs) {
  TestRat r;
  lightrat_set_si(*r, 1, 0);
  EXPECT_TRUE(lightrat_is_infinity(*r));
  lightrat_set_si(*r, -1, 0);
  EXPECT_TRUE(lightrat_is_ninfinity(*r));
  lightrat_set_si(*r, 0, 0);
  EXPECT_TRUE(lightrat_is_nan(*r));
}

TEST(TestLightrat, SetAfterNaN) {
  TestRat value;
  lightrat_set_si(*value, 5, 7);
  lightrat_set_si(*value, 1, 0);
  EXPECT_TRUE(lightrat_is_infinity(*value));

  TestRat zero;
  TestRat valid;
  lightrat_set_si(*valid, 0, 2);
  lightrat_div(*valid, zero.value);
  EXPECT_TRUE(lightrat_is_nan(*valid));

  lightrat_invert(*zero);
  EXPECT_TRUE(lightrat_is_nan(*zero));
}

TEST(TestLightrat, PredicatesAndMutators) {
  TestRat r;
  lightrat_set_si(*r, -6, 8);
  EXPECT_TRUE(lightrat_is_pos(*r) == false);
  EXPECT_FALSE(lightrat_is_zero(*r));
  EXPECT_EQ(lightrat_get_num(*r), -3);

  lightrat_negate(*r);
  EXPECT_EQ(str(*r), "3/4");

  lightrat_invert(*r);
  EXPECT_EQ(str(*r), "4/3");

  lightrat_set_infinity(*r);
  EXPECT_TRUE(lightrat_is_infinity(*r));
  lightrat_set_ninfinity(*r);
  EXPECT_TRUE(lightrat_is_ninfinity(*r));
  lightrat_set_si(*r, 12, 18);
  EXPECT_EQ(str(*r), "2/3");

  lightrat_clear(*r);
  lightrat_init(*r);
  EXPECT_TRUE(lightrat_is_zero(*r));
}

TEST(TestLightrat, ArithmeticWithRationals) {
  {
    TestRat left;
    TestRat right;
    lightrat_set_si(left.value, 1, 3);
    lightrat_set_si(right.value, 1, 6);
    lightrat_add(left.value, right.value);
    EXPECT_EQ(str(left.value), "1/2");
  }
  {
    TestRat left;
    TestRat right;
    lightrat_set_si(left.value, 1, 3);
    lightrat_set_si(right.value, 1, 6);
    lightrat_sub(left.value, right.value);
    EXPECT_EQ(str(left.value), "1/6");
  }
  {
    TestRat left;
    TestRat right;
    lightrat_set_si(left.value, 2, 3);
    lightrat_set_si(right.value, -9, 4);
    lightrat_mul(left.value, right.value);
    EXPECT_EQ(str(left.value), "-3/2");
  }
  {
    TestRat left;
    TestRat right;
    lightrat_set_si(left.value, 2, 3);
    lightrat_set_si(right.value, -4, 9);
    lightrat_div(left.value, right.value);
    EXPECT_EQ(str(left.value), "-3/2");
  }
}

TEST(TestLightrat, ArithmeticWithScalarAndGmpValues) {
  TestRat r;
  lightrat_set_si(*r, 1, 6);
  lightrat_add_si(*r, 1, 1);
  EXPECT_EQ(str(*r), "7/6");

  lightrat_set_si(*r, 1, 6);
  lightrat_sub_si(*r, 1, 1);
  EXPECT_EQ(str(*r), "-5/6");

  lightrat_set_si(*r, 3, 2);
  lightrat_mul_si(*r, 4, 1);
  EXPECT_EQ(str(*r), "6");

  lightrat_set_si(*r, 3, 2);
  lightrat_div_si(*r, 4, 1);
  EXPECT_EQ(str(*r), "3/8");

  mpq_t q;
  mpq_init(q);
  mpq_set_str(q, "5/6", 10);
  mpq_canonicalize(q);
  lightrat_set_si(*r, 1, 6);
  lightrat_add_gmp(*r, q);
  EXPECT_EQ(str(*r), "1");

  lightrat_set_si(*r, 1, 6);
  lightrat_sub_gmp(*r, q);
  EXPECT_EQ(str(*r), "-2/3");

  mpq_clear(q);

  mpq_t q_op;
  mpq_init(q_op);
  mpq_set_str(q_op, "3/2", 10);
  mpq_canonicalize(q_op);

  lightrat_set_si(*r, 1, 2);
  lightrat_mul_gmp(*r, q_op);
  EXPECT_EQ(str(*r), "3/4");

  lightrat_div_gmp(*r, q_op);
  EXPECT_EQ(str(*r), "1/2");
  mpq_clear(q_op);
}

TEST(TestLightrat, ExhaustiveSmallArithmeticMatchesGmp) {
  TestRat left;
  TestRat right;
  mpq_t expected_left;
  mpq_t expected_right;
  mpq_t expected;
  mpq_t actual;
  mpq_init(expected_left);
  mpq_init(expected_right);
  mpq_init(expected);
  mpq_init(actual);

  for (std::int32_t left_num = -7; left_num <= 7; ++left_num) {
    for (std::uint32_t left_den = 1; left_den <= 7; ++left_den) {
      lightrat_set_si(left.value, left_num, left_den);
      mpq_set_si(expected_left, left_num, left_den);
      mpq_canonicalize(expected_left);
      lightrat_to_gmp(left.value, actual);
      EXPECT_EQ(mpq_cmp(actual, expected_left), 0);

      for (std::int32_t right_num = -7; right_num <= 7; ++right_num) {
        for (std::uint32_t right_den = 1; right_den <= 7; ++right_den) {
          lightrat_set_si(right.value, right_num, right_den);
          mpq_set_si(expected_right, right_num, right_den);
          mpq_canonicalize(expected_right);

          lightrat_set_si(left.value, left_num, left_den);
          lightrat_add(left.value, right.value);
          lightrat_to_gmp(left.value, actual);
          mpq_add(expected, expected_left, expected_right);
          EXPECT_EQ(mpq_cmp(actual, expected), 0);

          lightrat_set_si(left.value, left_num, left_den);
          lightrat_sub(left.value, right.value);
          lightrat_to_gmp(left.value, actual);
          mpq_sub(expected, expected_left, expected_right);
          EXPECT_EQ(mpq_cmp(actual, expected), 0);

          lightrat_set_si(left.value, left_num, left_den);
          lightrat_mul(left.value, right.value);
          lightrat_to_gmp(left.value, actual);
          mpq_mul(expected, expected_left, expected_right);
          EXPECT_EQ(mpq_cmp(actual, expected), 0);

          if (right_num != 0) {
            lightrat_set_si(left.value, left_num, left_den);
            lightrat_div(left.value, right.value);
            lightrat_to_gmp(left.value, actual);
            mpq_div(expected, expected_left, expected_right);
            EXPECT_EQ(mpq_cmp(actual, expected), 0);
          }
        }
      }
    }
  }

  mpq_clear(actual);
  mpq_clear(expected);
  mpq_clear(expected_right);
  mpq_clear(expected_left);
}

TEST(TestLightrat, DivisionByZeroAndOverflowBoundaries) {
  TestRat half;
  lightrat_set_si(*half, 1, 2);
  TestRat zero;
  lightrat_set_si(*zero, 0, 1);
  lightrat_div(*half, *zero);
  EXPECT_TRUE(lightrat_is_infinity(*half));
  lightrat_set_si(*zero, 0, 1);
  lightrat_invert(*zero);
  EXPECT_TRUE(lightrat_is_nan(*zero));
  lightrat_set_si(*half, -1, 2);
  lightrat_div_si(*half, 0, 1);
  EXPECT_TRUE(lightrat_is_ninfinity(*half));

  const std::int32_t max = std::numeric_limits<std::int32_t>::max() >> 1;
  lightrat_set_si(half.value, max, 1);
  lightrat_add_si(half.value, 1, 1);
  EXPECT_TRUE(lightrat_is_gmp(half.value));
  EXPECT_EQ(str(half.value), "1073741824");
}

TEST(TestLightrat, InfinityArithmeticAndErrors) {
  TestRat inf;
  TestRat ninf;
  lightrat_set_infinity(inf.value);
  lightrat_set_ninfinity(ninf.value);
  TestRat one;
  lightrat_set_si(one.value, 1, 1);

  lightrat_add(inf.value, one.value);
  EXPECT_EQ(str(inf.value), "inf");

  lightrat_mul(ninf.value, one.value);
  EXPECT_EQ(str(ninf.value), "-inf");

  lightrat_add(inf.value, ninf.value);
  EXPECT_TRUE(lightrat_is_nan(*inf));
  lightrat_set_infinity(inf.value);

  TestRat zero;
  lightrat_mul(inf.value, zero.value);
  EXPECT_TRUE(lightrat_is_nan(*inf));
  lightrat_set_ninfinity(ninf.value);
  lightrat_mul(ninf.value, zero.value);
  EXPECT_TRUE(lightrat_is_nan(*ninf));
}

TEST(TestRational, ComparisonsConversionsAndOutput) {
  TestRat r;
  lightrat_set_si(r.value, 2, 3);
  TestRat same;
  lightrat_set_si(same.value, 2, 3);
  EXPECT_EQ(lightrat_compare(r.value, same.value), 0);
  EXPECT_LT(lightrat_compare_si(r.value, 1, 1), 0);
  EXPECT_GT(lightrat_compare_si(r.value, 0, 1), 0);
  mpq_t v;
  mpq_init(v);
  mpq_set_str(v, "4/6", 10);
  mpq_canonicalize(v);
  EXPECT_EQ(lightrat_compare_gmp(r.value, v), 0);
  mpq_set_si(v, 1, 1);
  mpq_canonicalize(v);
  EXPECT_LT(lightrat_compare_gmp(r.value, v), 0);
  mpq_set_si(v, 0, 1);
  mpq_canonicalize(v);
  EXPECT_GT(lightrat_compare_gmp(r.value, v), 0);
  mpq_clear(v);
  TestRat inf;
  lightrat_set_infinity(inf.value);
  EXPECT_GT(lightrat_compare(inf.value, r.value), 0);
  TestRat ninf;
  lightrat_set_ninfinity(ninf.value);
  EXPECT_LT(lightrat_compare(ninf.value, r.value), 0);
  EXPECT_EQ(lightrat_compare(inf.value, inf.value), 0);
  EXPECT_EQ(lightrat_compare(ninf.value, ninf.value), 0);
  EXPECT_LT(lightrat_compare(ninf.value, inf.value), 0);
  EXPECT_EQ(lightrat_compare_d(inf.value, std::numeric_limits<double>::infinity()), 0);
  EXPECT_EQ(lightrat_compare_d(ninf.value, -std::numeric_limits<double>::infinity()), 0);
  EXPECT_GT(lightrat_compare_d(r.value, 0.5), 0);
  EXPECT_LT(lightrat_compare_d(r.value, 0.7), 0);
  EXPECT_GT(lightrat_compare_d(inf.value, std::numeric_limits<double>::max()), 0);
  EXPECT_LT(lightrat_compare_d(ninf.value, std::numeric_limits<double>::min()), 0);
}

TEST(TestLightrat, GmpPayloadLifecycleAndRepresentationTransitions) {
  TestRat value;
  lightrat_set_si(*value, std::numeric_limits<std::int32_t>::max(), 1);
  lightrat_add_si(*value, 1, 1);
  ASSERT_TRUE(lightrat_is_gmp(*value));
  EXPECT_EQ(str(*value), "2147483648");

  lightrat_clear(*value);
  lightrat_init(*value);
  EXPECT_TRUE(lightrat_is_rat32(*value));
  EXPECT_EQ(str(*value), "0");

  TestRat copy;
  lightrat_set_si(copy.value, 5, 6);
  lightrat_set(*value, copy.value);
  EXPECT_EQ(str(*value), "5/6");

  lightrat_clear(copy.value);
  lightrat_init(copy.value);
  EXPECT_EQ(str(*value), "5/6");
}

TEST(TestLightrat, Hash) {
  for (std::int32_t num = -7; num <= 7; ++num) {
    for (std::uint32_t den = 1; den <= 7; ++den) {
      TestRat r;
      lightrat_set_si(r.value, num, den);
      TestRat copy;
      lightrat_set(copy.value, r.value);
      EXPECT_EQ(lightrat_hash(r.value), lightrat_hash(copy.value));
    }
  }
}

TEST(TestLightrat, IsFinite) {
  TestRat r;
  EXPECT_TRUE(lightrat_is_finite(*r));
  lightrat_set_si(*r, 3, 4);
  EXPECT_TRUE(lightrat_is_finite(*r));
  lightrat_set_infinity(*r);
  EXPECT_FALSE(lightrat_is_finite(*r));
  lightrat_set_ninfinity(*r);
  EXPECT_FALSE(lightrat_is_finite(*r));
}

TEST(TestLightrat, ToDouble) {
  TestRat r;
  lightrat_set_si(*r, 1, 2);
  EXPECT_DOUBLE_EQ(lightrat_to_d(*r), 0.5);
  lightrat_set_si(*r, -3, 4);
  EXPECT_DOUBLE_EQ(lightrat_to_d(*r), -0.75);
}

TEST(TestLightrat, GmpAccessorsAndConversion) {
  TestRat r;
  mpq_t q;
  mpq_init(q);
  mpq_set_str(q, "123456789012345678901234567890/9876543210987654321", 10);
  mpq_canonicalize(q);
  lightrat_set_gmp(*r, q);
  EXPECT_TRUE(lightrat_is_gmp(*r));
  EXPECT_NE(lightrat_get_gmp(*r), nullptr);

  mpq_t out;
  mpq_init(out);
  lightrat_to_gmp(*r, out);
  EXPECT_EQ(mpq_cmp(out, q), 0);
  mpq_clear(out);
  mpq_clear(q);
}
