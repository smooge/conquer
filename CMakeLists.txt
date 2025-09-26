cmake_minimum_required(VERSION 3.15...3.28)

# Parse version from patchlevel.h - Single Source of Truth
file(READ "patchlevel.h" PATCHLEVEL_CONTENT)
string(REGEX MATCH "#define PATCHLEVEL[ \t]+\"([0-9]+)\"" _ ${PATCHLEVEL_CONTENT})
if(NOT CMAKE_MATCH_1)
    message(FATAL_ERROR "Could not parse PATCHLEVEL from patchlevel.h")
endif()
set(CONQUER_PATCHLEVEL ${CMAKE_MATCH_1})

# Project definition with version from patchlevel.h
project(Conquer
    VERSION 4.${CONQUER_PATCHLEVEL}
    DESCRIPTION "Classic strategy game - modernized"
    LANGUAGES C
)

# Set C standard to C2x (C23) as required by Phase 4
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# POSIX compliance as required by project standards
add_compile_definitions(_POSIX_C_SOURCE=200809L)

# Add our CMake modules directory
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Phase 4 warning preservation - CRITICAL REQUIREMENT
include(CompilerWarnings)

# Phase 5.2: Dual compilation system implementation
include(DualCompilation)

# Basic compiler and build type setup
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Project configuration variables (PATCHLEVEL now parsed from patchlevel.h)
# Note: CONQUER_PATCHLEVEL is automatically set from patchlevel.h above
set(CONQUER_DEFAULT_DIR "${CMAKE_INSTALL_PREFIX}/share/conquer" CACHE PATH "Default game data directory")
set(CONQUER_EXE_DIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH "Executable installation directory")

# User login for build-time embedding
execute_process(
    COMMAND whoami
    OUTPUT_VARIABLE CONQUER_LOGIN
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Validate LOGIN detection for config.h generation
if(NOT CONQUER_LOGIN)
    message(WARNING "LOGIN detection failed - using fallback")
    set(CONQUER_LOGIN "unknown")
endif()

# Set LOGIN as a global cache variable to ensure it persists
set(CONQUER_LOGIN "${CONQUER_LOGIN}" CACHE STRING "Build user login name" FORCE)
message(STATUS "LOGIN for config.h: CONQUER_LOGIN='${CONQUER_LOGIN}'")

# Build timestamp for configuration
string(TIMESTAMP CONQUER_BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)

# Generate configuration header from template
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
    @ONLY
)

# Make generated config.h available to all targets
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# Basic build information
message(STATUS "Conquer ${PROJECT_VERSION} - Modern CMake Build System")
message(STATUS "Patchlevel: ${CONQUER_PATCHLEVEL} (from patchlevel.h)")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C standard: C${CMAKE_C_STANDARD}")
message(STATUS "Phase 4 warning preservation: ENABLED")
message(STATUS "Config header: ${CMAKE_CURRENT_BINARY_DIR}/config.h")

# Dual compilation system creates conquer, conqrun, and conqsort executables
# All configuration is handled by the DualCompilation module

#
# VERIFICATION TARGETS - Phase 4 Warning Compliance Validation
#

# Display active warning flags for verification
add_custom_target(verify_phase4_warnings
    COMMAND ${CMAKE_COMMAND} -E echo "=== Phase 4 Warning Flag Verification ==="
    COMMAND ${CMAKE_C_COMPILER} -Q --help=warning | grep -E "(Wall|Wextra|Wpedantic|Wformat|Wconversion|Wsign-conversion|Wimplicit-fallthrough|Wstrict-prototypes|Wold-style|Wshadow|Wmissing-prototypes|Wcast-qual|fanalyzer)" || true
    COMMAND ${CMAKE_COMMAND} -E echo "=== End Warning Flag Verification ==="
    COMMENT "Displaying active Phase 4 warning flags for verification"
    VERBATIM
)

# Test for warning regressions in dual compilation
add_custom_target(test_warning_regression
    COMMAND ${CMAKE_COMMAND} -E echo "=== Testing for Warning Regressions ==="
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target conquer --verbose 2>&1 | grep -i warning || ${CMAKE_COMMAND} -E echo "SUCCESS: No warnings in game executable"
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target conqrun --verbose 2>&1 | grep -i warning || ${CMAKE_COMMAND} -E echo "SUCCESS: No warnings in admin executable"
    COMMAND ${CMAKE_COMMAND} -E echo "=== Warning Regression Test Complete ==="
    COMMENT "Testing for any new warnings introduced since Phase 4"
    VERBATIM
)

# Comprehensive dual compilation test with verbose output
add_custom_target(verify_phase4_compliance
    COMMAND ${CMAKE_COMMAND} -E echo "=== Phase 4 Compliance Verification ==="
    COMMAND ${CMAKE_COMMAND} -E echo "Compiler: ${CMAKE_C_COMPILER}"
    COMMAND ${CMAKE_COMMAND} -E echo "C Standard: C${CMAKE_C_STANDARD}"
    COMMAND ${CMAKE_COMMAND} -E echo "Build Type: ${CMAKE_BUILD_TYPE}"
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target conquer --verbose
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target conqrun --verbose
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target conqsort --verbose
    COMMAND ${CMAKE_COMMAND} -E echo "=== Phase 4 Compliance Test COMPLETE ==="
    COMMENT "Comprehensive Phase 4 warning compliance verification"
    VERBATIM
)

#
# INSTALLATION CONFIGURATION - Complete Makefile Conversion
#

# Install all executables to bin directory
# Note: CMake doesn't support setuid bits (4751), those must be set manually after installation
install(TARGETS conquer conqrun conqsort conqps
    RUNTIME DESTINATION bin
    COMPONENT Executables
)

# Install PostScript data file alongside conqps executable
install(FILES psmap.ps
    DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    COMPONENT Data
)

# Install manual page
install(FILES man.pag
    DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    COMPONENT Documentation
)

# TODO: Help file generation and installation
# The original Makefile generates help0-help5 from txt0-txt5 using sed scripts
# This requires implementing the helpfile target from Makefile
# install(FILES help0 help1 help2 help3 help4 help5
#     DESTINATION share/conquer
#     PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
#     COMPONENT Data
# )

# Installation summary message
install(CODE "message(STATUS \"Installation complete to \${CMAKE_INSTALL_PREFIX}\")")
install(CODE "message(STATUS \"Note: Set setuid permissions manually if needed:\")")
install(CODE "message(STATUS \"  chmod 4755 \${CMAKE_INSTALL_PREFIX}/bin/conquer\")")
install(CODE "message(STATUS \"  chmod 4755 \${CMAKE_INSTALL_PREFIX}/bin/conqrun\")")