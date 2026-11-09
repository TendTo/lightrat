# Quick start

lightrat is a small, exact rational-number library built for fast arithmetic on small values and reliable fallback handling for larger values. The same API is available in C++ and Python, and it supports normalization, comparisons, and special values such as infinity and NaN.

## Core ideas

- Rational values are always kept in canonical form.
- Small rationals are stored compactly.
- Larger values automatically fall back to [GMP](https://gmplib.org/)-backed arithmetic.
- Special values are supported by default: `inf`, `-inf`, and `nan`.

### Python

Install the Python bindings via pip or similar package managers:

```bash
pip install lightrat
# or
uv add lightrat
```

To verify that the installation was successful, run

```shell
# Linux / macOS
python3 -m lightrat
# Windows
py -m lightrat
```

Then, create a script like the following

```python
# script.py file

from lightrat import Rational

# Construct values from pair of integers, strings or floats.
a = Rational(1, 3)
b = Rational("3/4")
c = Rational(5.4)

print(a)              # 1/3
print(a + b)          # 13/12
print(a - b)          # -5/12
print(a * c)          # 2/15
print(a / b)          # 4/9
print(a < b)          # True
print(6.4 <= c)       # False
print(a == "2/6")     # True

# Useful predicates.
print(Rational(4, 2).is_integer)   # True
print(Rational(0, 0).is_nan)       # True
print(Rational.infinity().str())   # inf
print(Rational.ninfinity().str())  # -inf
```

Run the script via python:

```bash
# Linux / macOS
python3 script.py
# Windows
py script.py
```

### C++

Install the C++ library from source:

```bash
git clone https://github.com/TendTo/lightrat.git
cd lightrat

# Linux / macOS
bazel run //lightrat:install -- --destdir=$(pwd)/build
# Windows (PowerShell or cmd)
bazel run //lightrat:install -- --destdir=%cd%\build
```

Then, create a source file like the following

```cpp
// main.cpp

#include <iostream>
#include <lightrat/lightrat.hpp>

int main() {
  lightrat::Rational<> a{1, 3};
  lightrat::Rational<> b{"3/4"};

  auto sum = a + b;
  auto product = a * b;

  std::cout << a.str() << "\n";                    // 1/3
  std::cout << sum.str() << "\n";                  // 13/12
  std::cout << product.str() << "\n";              // 1/4
  std::cout << std::boolalpha << (a < b) << "\n";  // true

  auto inf = lightrat::Rational<>::infinity();
  std::cout << inf.str() << "\n";  // inf
}
```

Compile and run the resulting binary

```bash
# gcc
g++ -std=c++20 -Ibuild/include main.cpp -Lbuild/lib -llightrat -o example
./example
# clang
clang++ -std=c++20 -Ibuild/include main.cpp -Lbuild/lib -llightrat -o example
./example
```

## Working with special values and predicates

Both APIs expose predicates for common checks:

```python
from lightrat import Rational

value = Rational(-6, 8)
print(value.str())      # -3/4
print(value.is_neg)     # True
print(value.is_nonzero) # True
print(value.is_integer) # False
print(value.abs().str()) # 3/4

print(Rational(1, 0).is_infinity) # True
print(Rational(0, 0).is_nan)      # True
```

```cpp
lightrat::Rational<> value{-6, 8};
assert(value.str() == "-3/4");
assert(value.is_neg());
assert(value.is_nonzero());
assert(!value.is_integer());
assert(value.abs().str() == "3/4");

assert(lightrat::Rational<>::infinity().is_infinity());
assert(lightrat::Rational<>::nan().is_nan());
```

## Typical operations

lightrat supports the standard operations you would expect from a rational-number type:

- construction from integers, strings, and floating-point values
- exact addition, subtraction, multiplication, and division
- comparison against other rationals, integers, strings, or floats
- normalization and simplification of fractions, applied upon construction
- conversion to strings and common numeric types
- detection of zero, sign, integer, infinity, and NaN states

That is enough to begin using lightrat for exact numeric calculations, symbolic math helpers, and any code that needs rational arithmetic without floating-point rounding errors.
See the [class documentation](https://tendto.github.io/lightrat/classlightrat_1_1Rational.html) to know more.
