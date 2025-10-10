# Testing Infrastructure for Conquer 4.x Modernization
# Phase 6.1: Unity Testing Framework Integration

# Enable CTest support for automated testing
enable_testing()

# Unity Testing Framework Configuration
set(UNITY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/framework")
set(UNITY_SOURCES
    "${UNITY_ROOT}/unity.c"
)
set(UNITY_HEADERS
    "${UNITY_ROOT}/unity.h"
    "${UNITY_ROOT}/unity_internals.h"
)

# Create Unity library target
add_library(unity STATIC ${UNITY_SOURCES})
target_include_directories(unity PUBLIC ${UNITY_ROOT})

# Set Unity compile options to match project standards
target_compile_features(unity PRIVATE c_std_23)
target_compile_definitions(unity PRIVATE _POSIX_C_SOURCE=200809L)

# Apply project warning flags to Unity (but allow some Unity-specific warnings)
if(TARGET enable_project_warnings)
    # Apply most warnings but exclude some that Unity may trigger
    target_compile_options(unity PRIVATE
        -Wall -Wextra -Wpedantic
        # Allow Unity's coding style differences
        -Wno-missing-prototypes
        -Wno-cast-qual
    )
endif()

# Test executable helper function
function(add_conquer_test test_name source_files)
    # Create test executable
    add_executable(${test_name} ${source_files})

    # Link with Unity
    target_link_libraries(${test_name} PRIVATE unity)

    # Apply project compile settings
    target_compile_features(${test_name} PRIVATE c_std_23)
    target_compile_definitions(${test_name} PRIVATE
        _POSIX_C_SOURCE=200809L
        PATCHLEVEL="${CONQUER_PATCHLEVEL}"
    )

    # Include project source directory for testing source files
    target_include_directories(${test_name} PRIVATE
        "${CMAKE_SOURCE_DIR}"
        "${CMAKE_BINARY_DIR}"  # For config.h
    )

    # Apply project warning flags
    if(COMMAND enable_project_warnings)
        enable_project_warnings(${test_name})
    endif()

    # Register with CTest
    add_test(NAME ${test_name} COMMAND ${test_name})

    # Set test properties
    set_tests_properties(${test_name} PROPERTIES
        TIMEOUT 30
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endfunction()

# Subdirectories for different test types
add_subdirectory(unit)
add_subdirectory(integration)
add_subdirectory(regression)

# Build target that explicitly builds all test executables
# This ensures tests are compiled before attempting to run them
add_custom_target(build_all_tests
    DEPENDS
        # Unit tests
        test_example_unit
        test_safe_convert
        test_comp_line
        test_spew_utils
        test_sort_utils
        test_safe_system
        test_m2alloc
        test_memory_integration
        # Integration tests
        test_example_integration
        # Regression tests
        test_example_regression
    COMMENT "Building all test executables"
    VERBATIM
)

# Test runner targets
add_custom_target(run_unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CMAKE_CURRENT_BINARY_DIR}" -L "unit" --output-on-failure
    COMMENT "Running unit tests"
    VERBATIM
)

add_custom_target(run_integration_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CMAKE_CURRENT_BINARY_DIR}" -L "integration" --output-on-failure
    COMMENT "Running integration tests"
    VERBATIM
)

add_custom_target(run_regression_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CMAKE_CURRENT_BINARY_DIR}" -L "regression" --output-on-failure
    COMMENT "Running regression tests"
    VERBATIM
)

add_custom_target(run_all_tests
    DEPENDS build_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CMAKE_CURRENT_BINARY_DIR}" --output-on-failure
    COMMENT "Running all tests"
    VERBATIM
)

# Documentation target
add_custom_target(test_info
    COMMAND ${CMAKE_COMMAND} -E echo "=== Conquer Testing Infrastructure ==="
    COMMAND ${CMAKE_COMMAND} -E echo "Unity Framework: ${UNITY_ROOT}"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Build test targets:"
    COMMAND ${CMAKE_COMMAND} -E echo "  build_all_tests     - Build all test executables"
    COMMAND ${CMAKE_COMMAND} -E echo "  all                 - Build everything including tests"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Test runner targets:"
    COMMAND ${CMAKE_COMMAND} -E echo "  run_unit_tests      - Build and run unit tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  run_integration_tests - Build and run integration tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  run_regression_tests - Build and run regression tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  run_all_tests       - Build and run all tests"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Information targets:"
    COMMAND ${CMAKE_COMMAND} -E echo "  test_info           - This information"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Direct CTest usage:"
    COMMAND ${CMAKE_COMMAND} -E echo "  ctest --test-dir tests --output-on-failure"
    COMMAND ${CMAKE_COMMAND} -E echo "=== End Testing Information ==="
    COMMENT "Display testing infrastructure information"
    VERBATIM
)