/**
 * @author TendTo (https://github.com/TendTo)
 * @copyright 2026 lightrat
 * @licence Apache License 2.0
 * @file
 * Custom errors for lightrat.
 */
#pragma once

#include <stdexcept>
#include <string>

/**
 * @namespace lightrat::errors
 * The `lightrat::errors` namespace contains custom error classes for the lightrat library.
 * These errors are used to signal exceptional conditions that can occur during the use of lightrat,
 * such as numerical errors, parsing errors, and other domain-specific issues.
 */
namespace lightrat::errors {
/** Base class for all lightrat errors. */
class lightrat_error {};

/** Base class for all numerical errors in lightrat. */
class numerical_error : public std::domain_error, public lightrat_error {
 public:
  explicit numerical_error(const std::string& what_arg) : std::domain_error(what_arg) {}
  explicit numerical_error(const char* what_arg) : std::domain_error(what_arg) {}
};

/** Error produced by operations that would produce a NaN when NaN is not enabled (e.g., 0/0, inf/inf, inf - inf). */
class nan_error : public numerical_error {
 public:
  explicit nan_error(const char* what_arg = "NaN is not enabled for this Rational type.") : numerical_error(what_arg) {}
};

/** Error produced by operations that would produce an infinity when infinity is not enabled (e.g., 1/0, -1/0). */
class infinity_error : public numerical_error {
 public:
  explicit infinity_error(const char* what_arg = "Infinity is not enabled for this Rational type.")
      : numerical_error(what_arg) {}
};

/** Error produced when attempting to build a rational number from an invalid string when NaN is not enabled */
class string_parsing_error : public std::invalid_argument, public lightrat_error {
 public:
  explicit string_parsing_error(const std::string& str)
      : std::invalid_argument("Failed to parse string '" + str + "' as a rational number.") {}
};

}  // namespace lightrat::errors
