cmake_minimum_required(VERSION 3.15...3.28)

# Project definition matching strategy document
project(Conquer
    VERSION 4.12
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

# Basic compiler and build type setup
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Project configuration variables
set(CONQUER_PATCHLEVEL "12" CACHE STRING "Game patch level")
set(CONQUER_DEFAULT_DIR "${CMAKE_INSTALL_PREFIX}/share/conquer" CACHE PATH "Default game data directory")
set(CONQUER_EXE_DIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH "Executable installation directory")

# User login for build-time embedding
execute_process(
    COMMAND whoami
    OUTPUT_VARIABLE CONQUER_LOGIN
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Basic build information
message(STATUS "Conquer ${PROJECT_VERSION} - Modern CMake Build System")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C standard: C${CMAKE_C_STANDARD}")
message(STATUS "Phase 4 warning preservation: ENABLED")

# For now, create a simple test target to verify compilation
# This will be expanded in subsequent phases
# Using a Phase 4 warning-eliminated file for testing
add_executable(conquer_test
    cexecute.c
)

# Apply Phase 4 warning levels to test target
target_link_libraries(conquer_test PRIVATE conquer_warnings)

# Add basic compile definitions
target_compile_definitions(conquer_test PRIVATE
    DEFAULTDIR="${CONQUER_DEFAULT_DIR}"
    EXEDIR="${CONQUER_EXE_DIR}"
    PATCHLEVEL="${CONQUER_PATCHLEVEL}"
    LOGIN="${CONQUER_LOGIN}"
    CONQUER  # Game mode for initial test
)

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

# Test for warning regressions in compilation
add_custom_target(test_warning_regression
    COMMAND ${CMAKE_COMMAND} -E echo "=== Testing for Warning Regressions ==="
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target conquer_test --verbose 2>&1 | grep -i warning || ${CMAKE_COMMAND} -E echo "SUCCESS: No warnings detected"
    COMMAND ${CMAKE_COMMAND} -E echo "=== Warning Regression Test Complete ==="
    COMMENT "Testing for any new warnings introduced since Phase 4"
    VERBATIM
)

# Comprehensive compilation test with verbose output
add_custom_target(verify_phase4_compliance
    COMMAND ${CMAKE_COMMAND} -E echo "=== Phase 4 Compliance Verification ==="
    COMMAND ${CMAKE_COMMAND} -E echo "Compiler: ${CMAKE_C_COMPILER}"
    COMMAND ${CMAKE_COMMAND} -E echo "C Standard: C${CMAKE_C_STANDARD}"
    COMMAND ${CMAKE_COMMAND} -E echo "Build Type: ${CMAKE_BUILD_TYPE}"
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target conquer_test --verbose
    COMMAND ${CMAKE_COMMAND} -E echo "=== Phase 4 Compliance Test COMPLETE ==="
    COMMENT "Comprehensive Phase 4 warning compliance verification"
    VERBATIM
)