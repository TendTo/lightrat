/**
 * @author TendTo (https://github.com/TendTo)
 * @copyright 2026 lightrat
 * @licence Apache-2.0 license
 * @file
 */
#include <iostream>

#include "lightrat/lightrat.hpp"

int main() {
  std::cout << LIGHTRAT_PROGRAM_NAME ":  " LIGHTRAT_DESCRIPTION " (version " LIGHTRAT_VERSION_STRING ") (build: " LIGHTRAT_BUILD_MODE ")" << std::endl;
  return 0;
}