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

  lightrat::Rational<> value{-6, 8};
  assert(value.str() == "-3/4");
  assert(value.is_neg());
  assert(value.is_nonzero());
  assert(!value.is_integer());
  assert(value.abs().str() == "3/4");

  assert(lightrat::Rational<>::infinity().is_infinity());
  assert(lightrat::Rational<>::nan().is_nan());
}
