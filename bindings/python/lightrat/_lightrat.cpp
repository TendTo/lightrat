/**
 * @author TendTo (https://github.com/TendTo)
 * @copyright 2026 lightrat
 * @licence Apache License 2.0
 * @file
 */
#ifndef LIGHTRAT_PYTHON
#error LIGHTRAT_PYTHON is not defined. Ensure you are building with the option '--config=py'
#endif

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>

#include "lightrat/Rational.h"
#include "lightrat/errors.h"
#include "lightrat/version.h"

#define COMPARE_OVERLOADS(r) \
  def(nb::self == r)         \
      .def(nb::self != r)    \
      .def(nb::self < r)     \
      .def(nb::self <= r)    \
      .def(nb::self > r)     \
      .def(nb::self >= r)    \
      .def(r == nb::self)    \
      .def(r != nb::self)    \
      .def(r < nb::self)     \
      .def(r <= nb::self)    \
      .def(r > nb::self)     \
      .def(r >= nb::self)

#define ARITHMETIC_OVERLOADS(r) \
  def(nb::self + r)             \
      .def(nb::self - r)        \
      .def(nb::self* r)         \
      .def(nb::self / r)        \
      .def(r + nb::self)        \
      .def(r - nb::self)        \
      .def(r* nb::self)         \
      .def(r / nb::self)

namespace nb = nanobind;
using namespace nb::literals;

using Rational = lightrat::Rational<>;

NB_MODULE(_lightrat, m) {
  nb::exception<lightrat::errors::numerical_error> numerical_error(m, "numerical_error", PyExc_ArithmeticError);
  nb::exception<lightrat::errors::nan_error> nan_error(m, "nan_error", numerical_error);
  nb::exception<lightrat::errors::infinity_error> infinity_error(m, "infinity_error", numerical_error);
  nb::exception<lightrat::errors::string_parsing_error> string_parsing_error(m, "string_parsing_error",
                                                                             PyExc_ValueError);

  auto RationalClass = nb::class_<Rational>(m, "Rational");

  RationalClass.def_static("infinity", &Rational::infinity)
      .def_static("ninfinity", &Rational::ninfinity)
      .def_static("nan", &Rational::nan)
      .def(nb::init<>())
      .def(nb::init<std::int32_t>(), "num"_a)
      .def(nb::init<std::int64_t, std::uint64_t>(), "num"_a, "den"_a)
      .def(nb::init<std::int64_t>(), "num"_a)
      .def(nb::init<std::uint32_t>(), "num"_a)
      .def(nb::init<std::uint64_t>(), "num"_a)
      .def(nb::init<double>(), "val"_a)
      .def(nb::init<const Rational&>(), "rat"_a)
      .def(nb::init<const std::string&>(), "s"_a)
      .def(nb::self += nb::self, nb::rv_policy::none)
      .def(nb::self -= nb::self, nb::rv_policy::none)
      .def(nb::self *= nb::self, nb::rv_policy::none)
      .def(nb::self /= nb::self, nb::rv_policy::none)
      .def(nb::self += float(), nb::rv_policy::none)
      .def(nb::self -= float(), nb::rv_policy::none)
      .def(nb::self *= float(), nb::rv_policy::none)
      .def(nb::self /= float(), nb::rv_policy::none)
      .def(nb::self += int(), nb::rv_policy::none)
      .def(nb::self -= int(), nb::rv_policy::none)
      .def(nb::self *= int(), nb::rv_policy::none)
      .def(nb::self /= int(), nb::rv_policy::none)
      .def(nb::self += std::string(), nb::rv_policy::none)
      .def(nb::self -= std::string(), nb::rv_policy::none)
      .def(nb::self *= std::string(), nb::rv_policy::none)
      .def(nb::self /= std::string(), nb::rv_policy::none)
      .def(nb::self + nb::self)
      .def(nb::self - nb::self)
      .def(nb::self * nb::self)
      .def(nb::self / nb::self)
      .ARITHMETIC_OVERLOADS(float())
      .ARITHMETIC_OVERLOADS(int())
      .ARITHMETIC_OVERLOADS(std::string())
      .def(nb::self == nb::self)
      .def(nb::self != nb::self)
      .def(nb::self <= nb::self)
      .def(nb::self < nb::self)
      .def(nb::self > nb::self)
      .def(nb::self >= nb::self)
      .COMPARE_OVERLOADS(float())
      .COMPARE_OVERLOADS(int())
      .COMPARE_OVERLOADS(std::string())
      .def(-nb::self)
      .def(+nb::self)
      .def_prop_ro("is_zero", &Rational::is_zero)
      .def_prop_ro("is_nonzero", &Rational::is_nonzero)
      .def_prop_ro("is_pos", &Rational::is_pos)
      .def_prop_ro("is_nonneg", &Rational::is_nonneg)
      .def_prop_ro("is_neg", &Rational::is_neg)
      .def_prop_ro("is_nonpos", &Rational::is_nonpos)
      .def_prop_ro("is_one", &Rational::is_one)
      .def_prop_ro("is_minus_one", &Rational::is_minus_one)
      .def_prop_ro("is_integer", &Rational::is_integer)
      .def_prop_ro("is_infinity", &Rational::is_infinity)
      .def_prop_ro("is_ninfinity", &Rational::is_ninfinity)
      .def_prop_ro("is_nan", &Rational::is_nan)
      .def_prop_ro("is_finite", &Rational::is_finite)
      .def_prop_ro("is_rat32", &Rational::is_rat32)
      .def_prop_ro("is_ratgmp", &Rational::is_ratgmp)
      .def_prop_ro("num", &Rational::num)
      .def_prop_ro("den", &Rational::den)
      .def_prop_ro("size", &Rational::size)
      .def("set", nb::overload_cast<std::int32_t, std::uint32_t>(&Rational::set), "num"_a, "den"_a, nb::rv_policy::none)
      .def("set", nb::overload_cast<std::int64_t, std::uint64_t>(&Rational::set), "num"_a, "den"_a, nb::rv_policy::none)
      .def("set", nb::overload_cast<std::int32_t>(&Rational::operator=), "num"_a, nb::rv_policy::none)
      .def("set", nb::overload_cast<std::int64_t>(&Rational::operator=), "num"_a, nb::rv_policy::none)
      .def("set", nb::overload_cast<double>(&Rational::operator=), "val"_a, nb::rv_policy::none)
      .def("set", nb::overload_cast<const std::string&>(&Rational::operator=), "s"_a, nb::rv_policy::none)
      .def("set", nb::overload_cast<const Rational&>(&Rational::operator=), "rat"_a, nb::rv_policy::none)
      .def("set_infinity", &Rational::set_infinity, nb::rv_policy::none)
      .def("set_ninfinity", &Rational::set_ninfinity, nb::rv_policy::none)
      .def("set_nan", &Rational::set_nan, nb::rv_policy::none)
      .def("clear", &Rational::clear)
      .def("normalize", &Rational::normalize)
      .def("set_one", &Rational::set_one, nb::rv_policy::none)
      .def("set_minus_one", &Rational::set_minus_one, nb::rv_policy::none)
      .def("str", &Rational::str)
      .def("compare", &Rational::compare<const Rational&>, "other"_a)
      .def("compare", &Rational::compare<std::int64_t>, "other"_a)
      .def("compare", &Rational::compare<double>, "other"_a)
      .def("swap", &Rational::swap, "other"_a)
      .def("invert", &Rational::invert, nb::rv_policy::none)
      .def("negate", &Rational::negate, nb::rv_policy::none)
      .def("abs", &Rational::abs, nb::rv_policy::none)
      .def("inverted", &Rational::inverted)
      .def("negated", &Rational::negated)
      .def("absed", &Rational::absed)
      .def("__hash__", &Rational::hash)
      .def("__abs__", &Rational::absed)
      .def("__floor__", [](const Rational& self) { return static_cast<std::int64_t>(self); })
      .def("__len__", &Rational::size)
      .def("__bool__", [](const Rational& self) { return static_cast<bool>(self); })
      .def("__int__", [](const Rational& self) { return static_cast<std::int64_t>(self); })
      .def("__float__", [](const Rational& self) { return static_cast<double>(self); })
      .def("__str__", &Rational::str)
      .def("__repr__", &Rational::str)
      .def("__getstate__", &Rational::str)
      .def("__setstate__", [](Rational& pet, const std::string& state) { new (&pet) Rational{state}; });

  m.doc() = LIGHTRAT_DESCRIPTION;
#ifdef LIGHTRAT_VERSION_STRING
  m.attr("__version__") = LIGHTRAT_VERSION_STRING;
#else
#error "LIGHTRAT_VERSION_STRING is not defined"
#endif
}
