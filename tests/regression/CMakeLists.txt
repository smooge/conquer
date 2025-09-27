# Regression Tests for Conquer 4.x
# Tests to ensure modernization doesn't break existing functionality

# Example regression test - remove when real tests are added
add_conquer_test(test_example_regression test_example_regression.c)
set_tests_properties(test_example_regression PROPERTIES LABELS "regression;example")

# Add real regression tests here as they are created
# These should test current behavior before modernization
# Example:
# add_conquer_test(test_game_mechanics_regression
#     regression/test_game_mechanics.c
#     ../src/game_logic.c
#     ../src/combat.c
# )
# set_tests_properties(test_game_mechanics_regression PROPERTIES LABELS "regression")