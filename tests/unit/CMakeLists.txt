# Unit Tests for Conquer 4.x
# Tests individual functions in isolation

# Example unit test - remove when real tests are added
add_conquer_test(test_example_unit test_example_unit.c)
set_tests_properties(test_example_unit PROPERTIES LABELS "unit;example")

# Add real unit tests here as they are created
# Example:
# add_conquer_test(test_string_utils unit/test_string_utils.c ../src/string_utils.c)
# set_tests_properties(test_string_utils PROPERTIES LABELS "unit")