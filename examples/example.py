from lightrat import Rational

# Construct values from pair of integers, strings or floats.
a = Rational(1, 3)
b = Rational("3/4")
c = Rational(5.4)

print(a)  # 1/3
print(a + b)  # 13/12
print(a - b)  # -5/12
print(a * c)  # 2/15
print(a / b)  # 4/9
print(a < b)  # True
print(6.4 <= c)  # False
print(a == "2/6")  # True

# Useful predicates.
print(Rational(4, 2).is_integer)  # True
print(Rational(0, 0).is_nan)  # True
print(Rational.infinity().str())  # inf
print(Rational.ninfinity().str())  # -inf

value = Rational(-6, 8)
print(value.str())      # -3/4
print(value.is_neg)     # True
print(value.is_nonzero) # True
print(value.is_integer) # False
print(value.abs().str()) # 3/4

print(Rational(1, 0).is_infinity) # True
print(Rational(0, 0).is_nan)      # True
