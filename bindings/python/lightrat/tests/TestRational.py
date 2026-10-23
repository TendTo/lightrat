import math
import pickle

from lightrat import Rational, infinity_error, nan_error, string_parsing_error


class TestRational:
    def test_constructors_and_normalization(self):
        assert Rational().str() == "0"
        assert Rational(-0.0).str() == "0"
        assert Rational(0.0).str() == "0"
        assert Rational(11.0).str() == "11"
        assert Rational(-11.0).str() == "-11"
        assert Rational(6, 8).str() == "3/4"
        assert Rational(-6, 8).str() == "-3/4"
        assert Rational(0.5).str() == "1/2"
        assert Rational(-0.5).str() == "-1/2"
        assert Rational("-12/18").str() == "-2/3"
        assert Rational(-(2 ** 31), 2).str() == "-1073741824"
        assert Rational(-(1 << 63), 2).str() == "-4611686018427387904"
        assert Rational(1 << 40).str() == str(1 << 40)
        assert Rational(1 << 40).is_ratgmp
        assert Rational(2 ** 31 - 2, 2).is_rat32
        assert Rational(float.fromhex("0x1.fffffffffffffp+1023")).str() == str(
            Rational(float.fromhex("0x1.fffffffffffffp+1023"))
        )
        assert Rational(float.fromhex("0x0.0000000000001p-1022")).str() == str(
            Rational(float.fromhex("0x0.0000000000001p-1022"))
        )

    def test_special_values(self):
        infinity = Rational.infinity()
        negative_infinity = Rational.ninfinity()
        nan = Rational.nan()

        assert infinity.str() == "inf"
        assert infinity.is_infinity
        assert not infinity.is_ninfinity
        assert not infinity.is_finite
        assert negative_infinity.str() == "-inf"
        assert negative_infinity.is_ninfinity
        assert not negative_infinity.is_infinity
        assert nan.str() == "nan"
        assert nan.is_nan
        assert not nan.is_finite

        assert Rational(1, 0).is_infinity
        assert Rational(-1, 0).is_ninfinity
        assert Rational(0, 0).is_nan
        assert Rational("not a rational").is_nan
        assert Rational("inf/2").is_nan

    def test_predicates_and_mutators(self):
        value = Rational(-6, 8)
        assert value.is_neg
        assert value.is_nonpos
        assert not value.is_pos
        assert not value.is_integer
        assert value.is_nonzero

        assert value.abs().str() == "3/4"
        assert value.negate().str() == "-3/4"
        assert value.invert().str() == "-4/3"
        assert value.set_one().is_one
        assert value.set_minus_one().is_minus_one
        value.clear()
        assert value.is_zero
        assert value.set(12, 18).str() == "2/3"
        assert value.set_infinity().is_infinity
        assert value.set_ninfinity().is_ninfinity
        assert value.set_nan().is_nan
        value.normalize()
        assert value.str() == "nan"
        assert Rational().invert().is_nan
        assert Rational.infinity().invert().is_nan

    def test_zero_predicates(self):
        zero = Rational()
        assert zero.is_zero
        assert not zero.is_nonzero
        assert not zero.is_pos
        assert zero.is_nonneg
        assert not zero.is_neg
        assert zero.is_nonpos
        assert not zero.is_one
        assert not zero.is_minus_one
        assert zero.is_integer
        assert zero.is_finite
        assert zero.num == 0
        assert zero.den == 1

    def test_arithmetic_operators_left(self):
        left = Rational(1, 3)
        right = Rational(1, 6)
        assert (left + right).str() == "1/2"
        assert (left - right).str() == "1/6"
        assert (Rational(2, 3) * Rational(-9, 4)).str() == "-3/2"
        assert (Rational(2, 3) / Rational(-4, 9)).str() == "-3/2"
        assert (Rational(2, 3) / Rational(-4, 9)).str() == "-3/2"
        assert (-left).str() == "-1/3"

        assert (Rational(1, 6) + 1).str() == "7/6"
        assert (Rational(1, 6) - 1).str() == "-5/6"
        assert (Rational(3, 2) * 4).str() == "6"
        assert (Rational(3, 2) / 4).str() == "3/8"
        assert (Rational(1, 4) + 0.25).str() == "1/2"
        assert (Rational(1, 6) + Rational(5, 6)).is_integer

        value = Rational(1, 3)
        value += right
        value -= Rational(-1, 2)
        value *= Rational(-4, 3)
        value /= Rational(2)
        assert value.str() == "-2/3"

    def test_arithmetic_operators_right(self):
        left = Rational(1, 3)
        right = Rational(1, 6)
        assert (left + right).str() == "1/2"
        assert (left - right).str() == "1/6"
        assert (Rational(2, 3) * Rational(-9, 4)).str() == "-3/2"
        assert (Rational(2, 3) / Rational(-4, 9)).str() == "-3/2"
        assert (-left).str() == "-1/3"

        assert (1 + Rational(1, 6)).str() == "7/6"
        assert (1 - Rational(1, 6)).str() == "5/6"
        assert (4 * Rational(3, 2)).str() == "6"
        assert (4 / Rational(3, 2)).str() == "8/3"
        assert (0.25 + Rational(1, 4)).str() == "1/2"
        assert (Rational(1, 6) + Rational(5, 6)).is_integer

    def test_addition(self):
        assert (Rational(1, 3) + Rational(1, 6)).str() == "1/2"
        assert (Rational(1, 3) + "1/6").str() == "1/2"
        assert (Rational(1, 3) + (1.0 / 4.0)).str() == "7/12"

        assert (Rational(1, 6) + Rational(1, 3)).str() == "1/2"
        assert ("1/6" + Rational(1, 3)).str() == "1/2"
        assert ((1.0 / 4.0) + Rational(1, 3)).str() == "7/12"

    def test_subtraction(self):
        assert (Rational(1, 3) - Rational(1, 6)).str() == "1/6"
        assert (Rational(1, 3) - "1/6").str() == "1/6"
        assert (Rational(1, 3) - (1.0 / 4.0)).str() == "1/12"

        assert (Rational(1, 6) - Rational(1, 3)).str() == "-1/6"
        assert ("1/6" - Rational(1, 3)).str() == "-1/6"
        assert ((1.0 / 4.0) - Rational(1, 3)).str() == "-1/12"

    def test_multiplication(self):
        assert (Rational(1, 3) * Rational(1, 6)).str() == "1/18"
        assert (Rational(1, 3) * "1/6").str() == "1/18"
        assert (Rational(1, 3) * (1.0 / 4.0)).str() == "1/12"

        assert (Rational(1, 6) * Rational(1, 3)).str() == "1/18"
        assert ("1/6" * Rational(1, 3)).str() == "1/18"
        assert ((1.0 / 4.0) * Rational(1, 3)).str() == "1/12"

    def test_division(self):
        assert (Rational(1, 3) / Rational(1, 6)).str() == "2"
        assert (Rational(1, 3) / "1/6").str() == "2"
        assert (Rational(1, 3) / (1.0 / 4.0)).str() == "4/3"

        assert (Rational(1, 6) / Rational(1, 3)).str() == "1/2"
        assert ("1/6" / Rational(1, 3)).str() == "1/2"
        assert ((1.0 / 4.0) / Rational(1, 3)).str() == "3/4"

    def test_eq(self):
        assert Rational(2, 3) == Rational(2, 3)
        assert Rational(1, 4) == 0.25
        assert Rational(4, 1) == 4
        assert Rational(20, 31) == "20/31"

        assert 4 == Rational(4, 1)
        assert 0.25 == Rational(1, 4)
        assert "20/31" == Rational(20, 31)

    def test_ne(self):
        assert Rational(2, 3) != Rational(3, 3)
        assert Rational(1, 5) != 0.25
        assert Rational(4, 1) != 5
        assert Rational(20, 33) != "20/31"

        assert 5 != Rational(4, 1)
        assert 0.25 != Rational(1, 5)
        assert "20/31" != Rational(20, 33)

    def test_lt(self):
        assert Rational(2, 3) < Rational(3, 3)
        assert Rational(1, 5) < 0.25
        assert Rational(4, 1) < 5
        assert Rational(20, 33) < "20/31"

        assert 4 <= Rational(5, 1)
        assert 0.25 <= Rational(1, 3)
        assert "20/31" <= Rational(20, 30)

    def test_le(self):
        assert Rational(2, 3) <= Rational(2, 3)
        assert Rational(1, 5) <= 0.25
        assert Rational(4, 1) <= 4
        assert Rational(20, 33) <= "20/31"

        assert 4 < Rational(5, 1)
        assert 0.25 < Rational(1, 3)
        assert "20/31" < Rational(20, 30)

    def test_gt(self):
        assert Rational(4, 3) > Rational(2, 3)
        assert Rational(1, 4) > 0.23
        assert Rational(4, 1) > 3
        assert Rational(20, 33) > "20/35"

        assert 4 > Rational(3, 1)
        assert 0.25 > Rational(1, 5)
        assert "20/31" > Rational(20, 33)

    def test_ge(self):
        assert Rational(4, 3) >= Rational(2, 3)
        assert Rational(1, 4) >= 0.23
        assert Rational(4, 1) >= 3
        assert Rational(20, 33) >= "20/35"

        assert 4 >= Rational(3, 1)
        assert 0.25 >= Rational(1, 5)
        assert "20/31" >= Rational(20, 33)

    def test_comparisons_and_compare(self):
        value = Rational(2, 3)
        same = Rational(4, 6)
        assert value < 1
        assert value > 0
        assert value <= same
        assert value >= same
        assert value == same
        assert value != Rational(1)
        assert value.compare(same) == 0
        assert value.compare(0.5) > 0
        assert Rational.ninfinity() < value < Rational.infinity()

    def test_infinity_and_nan_arithmetic(self):
        infinity = Rational.infinity()
        negative_infinity = Rational.ninfinity()
        nan = Rational.nan()

        assert (infinity + infinity).str() == "inf"
        assert (negative_infinity + negative_infinity).str() == "-inf"
        assert (infinity + negative_infinity).is_nan
        assert (infinity - negative_infinity).str() == "inf"
        assert (infinity * negative_infinity).str() == "-inf"
        assert (infinity / infinity).is_nan
        assert (infinity * 0).is_nan
        assert (Rational() * infinity).is_nan
        assert (nan + 1).is_nan
        assert (nan * nan).is_nan
        assert nan != Rational(1)

    def test_division_by_zero_and_boundaries(self):
        assert (Rational(1, 2) / Rational()).is_infinity
        assert (Rational(-1, 2) / Rational()).is_ninfinity

        maximum = (2 ** 31 - 1) >> 1
        value = Rational(maximum)
        value += 1
        assert value.is_ratgmp
        assert value.str() == "1073741824"
        value = Rational(-maximum)
        value -= 1
        assert value.is_ratgmp
        assert value.str() == "-1073741824"

    def test_swap_and_string_representation(self):
        left = Rational(1, 2)
        right = Rational(3, 4)
        left.swap(right)
        assert left.str() == "3/4"
        assert right.str() == "1/2"
        assert str(left) == "3/4"
        assert repr(left) == "3/4"

    def test_copy_and_assignment(self):
        original = Rational(1 << 40, 3)
        copy = Rational(original)
        assigned = Rational()
        assigned = original
        assert copy.str() == "1099511627776/3"
        assert assigned.str() == "1099511627776/3"

    def test_zero_behavior(self):
        zero = Rational()
        assert not zero.is_infinity
        assert not zero.is_ninfinity
        assert not zero.is_nan
        assert zero == 0.0
        assert float(zero) == 0.0
        assert (zero + zero).str() == "0"
        assert (zero - zero).str() == "0"
        assert (zero * zero).str() == "0"
        assert (zero / Rational(2)).str() == "0"
        assert (Rational(1) / zero).is_infinity
        assert (Rational(-1) / zero).is_ninfinity

    def test_special_value_predicates_and_arithmetic(self):
        infinity = Rational.infinity()
        negative_infinity = Rational.ninfinity()
        nan = Rational.nan()

        assert infinity.is_nonzero and infinity.is_pos and infinity.is_nonneg
        assert not infinity.is_neg and not infinity.is_nonpos
        assert not infinity.is_integer and not infinity.is_one and not infinity.is_minus_one
        assert negative_infinity.is_nonzero and negative_infinity.is_neg and negative_infinity.is_nonpos
        assert not negative_infinity.is_pos and not negative_infinity.is_nonneg
        assert not negative_infinity.is_integer
        assert (infinity + 1).str() == "inf"
        assert (negative_infinity + 1).str() == "-inf"
        assert (negative_infinity - infinity).str() == "-inf"
        assert (infinity - negative_infinity).str() == "inf"
        assert (infinity * infinity).str() == "inf"
        assert (infinity * negative_infinity).str() == "-inf"
        assert (negative_infinity * negative_infinity).str() == "inf"
        assert (infinity / 2).str() == "inf"
        assert (infinity / -2).str() == "-inf"
        assert (Rational(1) + infinity).str() == "inf"
        assert (Rational(1) - infinity).str() == "-inf"
        assert (Rational(1) / infinity).str() == "0"
        assert (Rational(0) / infinity).str() == "0"
        assert (infinity - infinity).is_nan
        assert (infinity * 0).is_nan
        assert (Rational() * infinity).is_nan
        assert nan.is_nan
        assert not nan.is_nonzero
        assert (nan + Rational(1)).is_nan
        assert (Rational(1) / nan).is_nan
        assert nan != nan
        assert nan != Rational(1)

    def test_exposed_exception_types(self):
        assert issubclass(nan_error, ArithmeticError)
        assert issubclass(infinity_error, ArithmeticError)
        assert issubclass(string_parsing_error, ValueError)

    def test_float_special_values(self):
        assert Rational(float("inf")).is_infinity
        assert Rational(float("-inf")).is_ninfinity
        assert Rational(float("nan")).is_nan
        assert math.isinf(float(Rational.infinity()))
        assert math.isnan(float(Rational.nan()))

    def test_no_hash_collision_int_pairs(self):
        collisions = {}
        for num in range(-1000, 1001):
            for den in range(1, 1001):
                pair = (num, den)
                hash_value = hash(pair)
                if hash_value not in collisions:
                    collisions[hash_value] = [0, []]
                if pair in collisions[hash_value][1]:
                    collisions[hash_value][0] += 1
                    collisions[hash_value][1].append(pair)
        assert sum(count for count, _ in collisions.values() if count > 1) == 0

    def test_dunder_methods(self):
        assert hash(Rational(1, 2)) == hash(Rational(30, 60))
        assert abs(Rational(-3, 4)) == Rational(3, 4)
        assert math.floor(Rational(7, 3)) == 2
        assert len(Rational(1, 2)) == 8  # 32-bit integer representation of 1/2 is (32+32) bits => 8 bytes
        assert bool(Rational(1, 2)) is True
        assert bool(Rational(0)) is False
        assert int(Rational(7, 3)) == 2
        assert float(Rational(1, 2)) == 0.5
        assert str(Rational(3, 4)) == "3/4"
        assert repr(Rational(3, 4)) == str(Rational(3, 4))

    def test_pickle_small(self):
        r = Rational(1, 2)
        pickled = pickle.dumps(r)
        unpickled = pickle.loads(pickled)
        assert unpickled == r

    def test_pickle_large(self):
        r = Rational(2 ** 40, 3)
        pickled = pickle.dumps(r)
        unpickled = pickle.loads(pickled)
        assert unpickled == r

    def test_pickle_special_values(self):
        r = Rational.infinity()
        pickled = pickle.dumps(r)
        unpickled = pickle.loads(pickled)
        assert unpickled == r

        r = Rational.ninfinity()
        pickled = pickle.dumps(r)
        unpickled = pickle.loads(pickled)
        assert unpickled == r

        r = Rational.nan()
        pickled = pickle.dumps(r)
        unpickled = pickle.loads(pickled)
        assert unpickled.is_nan
        assert r.is_nan
