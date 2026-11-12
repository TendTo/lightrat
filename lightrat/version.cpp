/**
 * @author TendTo (https://github.com/TendTo)
 * @copyright 2026 lightrat
 * @licence Apache-2.0 license
 * @file
 */
#include "lightrat/version.h"

#ifdef NDEBUG
#define LIGHTRAT_BUILD_MODE "release"
#else
#define LIGHTRAT_BUILD_MODE "debug"
#endif

const char* lightrat_status_string = LIGHTRAT_PROGRAM_NAME
    ":  " LIGHTRAT_DESCRIPTION " (version " LIGHTRAT_VERSION_STRING ") (build: " LIGHTRAT_BUILD_MODE ")";
