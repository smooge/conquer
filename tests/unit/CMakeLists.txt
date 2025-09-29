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



# Add additional unit tests here as they are created
# Example:
# add_conquer_test(test_string_utils test_string_utils.c ../src/string_utils.c)
# set_tests_properties(test_string_utils PROPERTIES LABELS "unit")