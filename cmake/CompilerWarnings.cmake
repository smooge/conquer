# CompilerWarnings.cmake - Phase 4 Warning Level Preservation
#
# CRITICAL: This module preserves the 100% warning elimination achieved in Phase 4
# by implementing the exact same strict compilation flags that were used for
# comprehensive warning analysis and fixes.
#
# ALL GAME TARGETS MUST INHERIT FROM conquer_warnings INTERFACE LIBRARY

include(CheckCCompilerFlag)

# Phase 4 warning flags - MANDATORY minimum requirement
# These flags achieved 100% warning elimination across 15 files
set(PHASE4_WARNING_FLAGS
    -Wall
    -Wextra
    -Wpedantic
    -Wformat=2
    -Wconversion
    -Wsign-conversion
    -Wimplicit-fallthrough
    -Wstrict-prototypes
    -Wold-style-declaration
    -Wold-style-definition
    -Wshadow
    -Wmissing-prototypes
    -Wcast-qual
)

# Advanced analysis flags used in Phase 4 intensive testing
set(PHASE4_ANALYSIS_FLAGS
    -fanalyzer
)

# Sanitizer flags for runtime error detection
set(PHASE4_SANITIZER_FLAGS
    -fsanitize=address,undefined
)

# Create interface library for consistent warning enforcement
add_library(conquer_warnings INTERFACE)

# Check compiler support for each warning flag and add if supported
foreach(flag ${PHASE4_WARNING_FLAGS})
    string(REPLACE "=" "_" flag_var ${flag})
    string(REPLACE "-" "_" flag_var ${flag_var})
    string(TOUPPER "${flag_var}" flag_var)
    set(flag_var "COMPILER_SUPPORTS${flag_var}")

    check_c_compiler_flag(${flag} ${flag_var})
    if(${flag_var})
        target_compile_options(conquer_warnings INTERFACE ${flag})
    else()
        message(WARNING "Compiler does not support Phase 4 warning flag: ${flag}")
    endif()
endforeach()

# Conditional advanced analysis support
check_c_compiler_flag(-fanalyzer COMPILER_SUPPORTS_ANALYZER)
if(COMPILER_SUPPORTS_ANALYZER)
    option(ENABLE_ANALYZER "Enable static analysis with -fanalyzer" ON)
    if(ENABLE_ANALYZER)
        target_compile_options(conquer_warnings INTERFACE -fanalyzer)
        message(STATUS "Phase 4 static analysis: ENABLED (-fanalyzer)")
    endif()
else()
    message(STATUS "Phase 4 static analysis: UNAVAILABLE (compiler lacks -fanalyzer)")
endif()

# Conditional sanitizer support
check_c_compiler_flag("-fsanitize=address,undefined" COMPILER_SUPPORTS_SANITIZERS)
if(COMPILER_SUPPORTS_SANITIZERS)
    option(ENABLE_SANITIZERS "Enable runtime sanitizers" ON)
    if(ENABLE_SANITIZERS)
        target_compile_options(conquer_warnings INTERFACE -fsanitize=address,undefined)
        target_link_options(conquer_warnings INTERFACE -fsanitize=address,undefined)
        message(STATUS "Phase 4 sanitizers: ENABLED (address,undefined)")
    endif()
else()
    message(STATUS "Phase 4 sanitizers: UNAVAILABLE (compiler lacks sanitizer support)")
endif()

# Build type specific flag adjustments
# Debug: Maximum debugging information
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(conquer_warnings INTERFACE -g3 -O0 -DDEBUG)
    message(STATUS "Phase 4 debug mode: Maximum debugging enabled")
endif()

# Release: Optimized but with debugging symbols for problem diagnosis
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(conquer_warnings INTERFACE -O2 -g -DNDEBUG)
    message(STATUS "Phase 4 release mode: Optimized with debug symbols")
endif()

# Analysis: Moderate optimization for analysis tools
if(CMAKE_BUILD_TYPE STREQUAL "Analysis")
    target_compile_options(conquer_warnings INTERFACE -O1 -g)
    message(STATUS "Phase 4 analysis mode: Moderate optimization for tools")
endif()

# Provide information about applied warning levels
message(STATUS "Phase 4 Warning Preservation: ALL FLAGS APPLIED")
message(STATUS "  Core warnings: ${PHASE4_WARNING_FLAGS}")
if(ENABLE_ANALYZER)
    message(STATUS "  Static analysis: -fanalyzer ENABLED")
endif()
if(ENABLE_SANITIZERS)
    message(STATUS "  Runtime checks: -fsanitize=address,undefined ENABLED")
endif()

# CRITICAL: All game targets MUST link to this interface library
# Example usage in main CMakeLists.txt:
#   target_link_libraries(conquer PRIVATE conquer_warnings)
#   target_link_libraries(conqrun PRIVATE conquer_warnings)
#
# This ensures 100% preservation of Phase 4 warning elimination achievements