# Unit Tests for Conquer 4.x
# Tests individual functions in isolation

# Example unit test - remove when real tests are added
add_conquer_test(test_example_unit test_example_unit.c)
set_tests_properties(test_example_unit PROPERTIES LABELS "unit;example")

# Phase 6.3: Safe Conversion Function Tests
add_conquer_test(test_safe_convert test_safe_convert.c)
set_tests_properties(test_safe_convert PROPERTIES
    LABELS "unit;safe_convert;phase6"
    TIMEOUT 60  # Allow extra time for performance tests
)

# Phase 6.5: Core Function Unit Tests
add_conquer_test(test_comp_line test_comp_line.c)
set_tests_properties(test_comp_line PROPERTIES
    LABELS "unit;comp_line;phase6;level0"
    TIMEOUT 30
)

# Phase 6.5 Session 2: Additional Static Function Tests
add_conquer_test(test_spew_utils test_spew_utils.c)
set_tests_properties(test_spew_utils PROPERTIES
    LABELS "unit;spew_utils;phase6;level0"
    TIMEOUT 30
)

# Phase 6.5 Session 3: Sort Utilities Tests
add_conquer_test(test_sort_utils test_sort_utils.c)
set_tests_properties(test_sort_utils PROPERTIES
    LABELS "unit;sort_utils;phase6;level0"
    TIMEOUT 30
)

# Phase 7.2: Security System Function Tests
add_executable(test_safe_system test_safe_system.c ../../safe_system.c)
target_link_libraries(test_safe_system PRIVATE unity)
target_compile_features(test_safe_system PRIVATE c_std_23)
target_compile_definitions(test_safe_system PRIVATE
    _POSIX_C_SOURCE=200809L
    PATCHLEVEL="${CONQUER_PATCHLEVEL}"
)
target_include_directories(test_safe_system PRIVATE
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_BINARY_DIR}"  # For config.h
)
if(COMMAND enable_project_warnings)
    enable_project_warnings(test_safe_system)
endif()
add_test(NAME test_safe_system COMMAND test_safe_system)
set_tests_properties(test_safe_system PROPERTIES
    LABELS "unit;safe_system;phase7;security"
    TIMEOUT 60  # Allow extra time for file operations
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# Add additional unit tests here as they are created
# Example:
# add_conquer_test(test_string_utils test_string_utils.c ../src/string_utils.c)
# set_tests_properties(test_string_utils PROPERTIES LABELS "unit")
