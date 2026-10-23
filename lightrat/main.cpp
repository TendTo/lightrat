/**
 * @author TendTo (https://github.com/TendTo)
 * @copyright 2026 lightrat
 * @licence Apache-2.0 license
 * @file
 */
#include <iostream>

#include "lightrat/Rational.h"
#include "lightrat/lightrat.h"
#include "lightrat/version.h"

/**
 * @namespace lightrat
 * The lightrat namespace contains all the classes and functions of the lightrat library.
 */
namespace lightrat {}  // namespace lightrat

int main() {
  std::cout << LIGHTRAT_PROGRAM_NAME ":  " LIGHTRAT_DESCRIPTION " (version " LIGHTRAT_VERSION_STRING ")" << std::endl;
  return 0;
}