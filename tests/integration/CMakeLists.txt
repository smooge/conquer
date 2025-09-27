# Integration Tests for Conquer 4.x
# Tests interactions between multiple modules

# Example integration test - remove when real tests are added
add_conquer_test(test_example_integration test_example_integration.c)
set_tests_properties(test_example_integration PROPERTIES LABELS "integration;example")

# Add real integration tests here as they are created
# Example:
# add_conquer_test(test_file_system_integration
#     integration/test_file_system.c
#     ../src/file_io.c
#     ../src/data_management.c
# )
# set_tests_properties(test_file_system_integration PROPERTIES LABELS "integration")