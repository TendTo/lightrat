/**
 * @author TendTo (https://github.com/TendTo)
 * @copyright 2026 lightrat
 * @licence Apache License 2.0
 * @file
 * Collection of concepts and type traits used in lightrat.
 */
#pragma once

#include <concepts>

/**
 * @namespace lightrat::concepts
 * The `lightrat::concepts` namespace contains custom concepts for the lightrat library.
 * These concepts are used to constrain template parameters and provide compile-time type checking.
 */
namespace lightrat::concepts {

/**
 * Ensure the type `T` is not the types `U`.
 * @code
 * template <is_not<int> T>
 * void foo(T t); // T can cannot be int
 * @endcode
 * @tparam T type to check
 * @tparam U any number of types to check against
 */
template <class T, class U>
concept is_not = !std::same_as<std::remove_reference_t<T>, U>;

/**
 * Ensure the type `T` is any of the types `U`.
 * @code
 * template <is_any_of<int, float, bool> T>
 * void foo(T t); // T can be either int, float or bool
 * @endcode
 * @tparam T type to check
 * @tparam U any number of types to check against
 */
template <class T, class... U>
concept is_any_of = (std::same_as<std::remove_reference_t<T>, U> || ...);

/**
 * Ensure the type `T` is not any of the types `U`.
 * @code
 * template <is_not_any_of<int, float, bool> T>
 * void foo(T t); // T can be any type except int, float or bool
 * @endcode
 * @tparam T type to check
 * @tparam U any number of types to check against
 */
template <class T, class... U>
concept is_not_any_of = !is_any_of<T, U...>;

}  // namespace lightrat::concepts
