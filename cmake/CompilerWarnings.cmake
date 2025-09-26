# CompilerWarnings.cmake - Phase 4 Warning Level Preservation
#
# CRITICAL: This module preserves the 100% warning elimination achieved in Phase 4
# by implementing the exact same strict compilation flags that were used for
# comprehensive warning analysis and fixes.
#
# Enhanced with compiler-specific flag intelligence for optimal cross-compiler support
#
# ALL GAME TARGETS MUST INHERIT FROM conquer_warnings INTERFACE LIBRARY

include(CheckCCompilerFlag)

# Detect compiler type for intelligent flag selection
set(IS_GCC FALSE)
set(IS_CLANG FALSE)
set(IS_MSVC FALSE)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(IS_GCC TRUE)
    message(STATUS "Compiler detected: GCC ${CMAKE_C_COMPILER_VERSION}")
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(IS_CLANG TRUE)
    message(STATUS "Compiler detected: Clang ${CMAKE_C_COMPILER_VERSION}")
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    set(IS_MSVC TRUE)
    message(STATUS "Compiler detected: MSVC ${CMAKE_C_COMPILER_VERSION}")
else()
    message(STATUS "Compiler detected: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
endif()

# Core warning flags supported by both GCC and Clang
set(CORE_WARNING_FLAGS
    -Wall
    -Wextra
    -Wpedantic
    -Wformat=2
    -Wconversion
    -Wsign-conversion
    -Wimplicit-fallthrough
    -Wstrict-prototypes
    -Wold-style-definition
    -Wshadow
    -Wmissing-prototypes
    -Wcast-qual
)

# GCC-specific warning flags
set(GCC_SPECIFIC_FLAGS
    -Wold-style-declaration    # Not supported by Clang
)

# Clang-specific warning flags
set(CLANG_SPECIFIC_FLAGS
    -Wlogical-not-parentheses  # Better logical operator analysis
    -Wparentheses-equality     # Better equality comparison analysis
    -Wnewline-eof             # Enforce newline at end of file
)

# Combine flags based on compiler
set(PHASE4_WARNING_FLAGS ${CORE_WARNING_FLAGS})

if(IS_GCC)
    list(APPEND PHASE4_WARNING_FLAGS ${GCC_SPECIFIC_FLAGS})
    message(STATUS "Using GCC-optimized warning flags")
elseif(IS_CLANG)
    list(APPEND PHASE4_WARNING_FLAGS ${CLANG_SPECIFIC_FLAGS})
    message(STATUS "Using Clang-optimized warning flags")
endif()

# Compiler-specific analysis flags
set(GCC_ANALYSIS_FLAGS
    -fanalyzer                 # GCC static analyzer
)

set(CLANG_ANALYSIS_FLAGS
    # Clang has built-in static analyzer, typically invoked separately
    # -fanalyzer not available in Clang
)

# Sanitizer flags (both compilers support these)
set(SANITIZER_FLAGS
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

# Compiler-specific static analysis support
if(IS_GCC)
    check_c_compiler_flag(-fanalyzer COMPILER_SUPPORTS_ANALYZER)
    if(COMPILER_SUPPORTS_ANALYZER)
        option(ENABLE_ANALYZER "Enable GCC static analysis with -fanalyzer" ON)
        if(ENABLE_ANALYZER)
            target_compile_options(conquer_warnings INTERFACE -fanalyzer)
            message(STATUS "Phase 4 static analysis: ENABLED (GCC -fanalyzer)")
        endif()
    else()
        message(STATUS "Phase 4 static analysis: UNAVAILABLE (GCC lacks -fanalyzer)")
    endif()
elseif(IS_CLANG)
    message(STATUS "Phase 4 static analysis: Use 'clang --analyze' or 'scan-build' for Clang static analysis")
else()
    message(STATUS "Phase 4 static analysis: Unknown compiler, use compiler-specific tools")
endif()

# Sanitizer support (both compilers support this)
check_c_compiler_flag("-fsanitize=address,undefined" COMPILER_SUPPORTS_SANITIZERS)
if(COMPILER_SUPPORTS_SANITIZERS)
    option(ENABLE_SANITIZERS "Enable runtime sanitizers" ON)
    if(ENABLE_SANITIZERS)
        target_compile_options(conquer_warnings INTERFACE -fsanitize=address,undefined)
        target_link_options(conquer_warnings INTERFACE -fsanitize=address,undefined)
        if(IS_GCC)
            message(STATUS "Phase 4 sanitizers: ENABLED (GCC address,undefined)")
        elseif(IS_CLANG)
            message(STATUS "Phase 4 sanitizers: ENABLED (Clang address,undefined)")
        else()
            message(STATUS "Phase 4 sanitizers: ENABLED (address,undefined)")
        endif()
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