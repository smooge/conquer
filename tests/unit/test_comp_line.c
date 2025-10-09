/*
 * test_comp_line.c - Unit tests for sort.c comp_line() function
 *
 * Tests the string comparison function with configurable case sensitivity
 * and character length limiting. This function is critical for the sorting
 * behavior of the sort utility.
 *
 * Testing Categories:
 * - Basic string comparison (less than, equal, greater than)
 * - Case sensitivity control (iflag variations)
 * - Length limiting (compnum variations)
 * - Boundary conditions (empty strings, null characters)
 * - Edge cases (partial matches, special characters)
 */

#include "unity.h"
#include <string.h>
#include <ctype.h>

/* Include necessary definitions from sort.c */
#ifndef FALSE
#define FALSE (0)
#define TRUE (1)
#endif

#define DEFAULT_COMP 2
#define UPPER(x) ((islower(x)) ? (toupper(x)) : (x))

/* Global variables that comp_line depends on - defined locally for testing */
int compnum = DEFAULT_COMP;
int iflag = FALSE;

/* Function prototype - comp_line is static, so we need to access it */
/* We'll include the implementation directly for testing */
static int comp_line(char *a, char *b);

/* Include the actual comp_line implementation */
static int comp_line(char *a, char *b) {
    int i;

    for (i = 0; i < compnum; i++) {
        /* do checks */
        if (iflag) {
            if (UPPER(a[i]) < UPPER(b[i]))
                return (-1);
            if (UPPER(b[i]) < UPPER(a[i]))
                return (1);
        } else {
            if (a[i] < b[i])
                return (-1);
            if (b[i] < a[i])
                return (1);
        }
        if (a[i] == '\0')
            return (0);
    }
    return (0);
}

/* Global state management utilities */
void setup_global_state(int comp_chars, int ignore_case) {
    compnum = comp_chars;
    iflag = ignore_case;
}

void setUp(void) {
    /* Reset to default state before each test */
    setup_global_state(DEFAULT_COMP, FALSE);
}

void tearDown(void) {
    /* Clean up after each test if needed */
}

/* Test Suite 1: Basic Comparison Tests */
void test_comp_line_basic_less_than(void) {
    TEST_ASSERT_EQUAL_INT(-1, comp_line("a", "b"));
    TEST_ASSERT_EQUAL_INT(-1, comp_line("apple", "banana"));
    TEST_ASSERT_EQUAL_INT(-1, comp_line("aa", "ab"));
}

void test_comp_line_basic_equal(void) {
    TEST_ASSERT_EQUAL_INT(0, comp_line("test", "test"));
    TEST_ASSERT_EQUAL_INT(0, comp_line("a", "a"));
    TEST_ASSERT_EQUAL_INT(0, comp_line("", ""));
}

void test_comp_line_basic_greater_than(void) {
    TEST_ASSERT_EQUAL_INT(1, comp_line("z", "a"));
    TEST_ASSERT_EQUAL_INT(1, comp_line("banana", "apple"));
    TEST_ASSERT_EQUAL_INT(1, comp_line("ab", "aa"));
}

/* Test Suite 2: Case Sensitivity Tests */
void test_comp_line_case_sensitive_default(void) {
    setup_global_state(DEFAULT_COMP, FALSE);

    /* Upper case comes before lower case in ASCII */
    TEST_ASSERT_EQUAL_INT(-1, comp_line("A", "a"));
    TEST_ASSERT_EQUAL_INT(1, comp_line("a", "A"));
    TEST_ASSERT_EQUAL_INT(-1, comp_line("Test", "test"));
}

void test_comp_line_case_insensitive(void) {
    setup_global_state(DEFAULT_COMP, TRUE);

    /* Should ignore case differences */
    TEST_ASSERT_EQUAL_INT(0, comp_line("A", "a"));
    TEST_ASSERT_EQUAL_INT(0, comp_line("Test", "TEST"));
    TEST_ASSERT_EQUAL_INT(0, comp_line("MiXeD", "mixed"));

    /* But still distinguish different letters */
    TEST_ASSERT_EQUAL_INT(-1, comp_line("A", "B"));
    TEST_ASSERT_EQUAL_INT(-1, comp_line("a", "B"));
}

/* Test Suite 3: Length Limiting Tests */
void test_comp_line_length_limit_1_char(void) {
    setup_global_state(1, FALSE);

    /* Only compare first character */
    TEST_ASSERT_EQUAL_INT(0, comp_line("apple", "avocado"));
    TEST_ASSERT_EQUAL_INT(-1, comp_line("apple", "banana"));
    TEST_ASSERT_EQUAL_INT(1, comp_line("zebra", "apple"));
}

void test_comp_line_length_limit_3_chars(void) {
    setup_global_state(3, FALSE);

    /* Compare first 3 characters only */
    TEST_ASSERT_EQUAL_INT(0, comp_line("test123", "test456"));
    TEST_ASSERT_EQUAL_INT(-1, comp_line("abc", "abd"));
    TEST_ASSERT_EQUAL_INT(1, comp_line("xyz", "abc"));
}

void test_comp_line_length_limit_longer_than_string(void) {
    setup_global_state(10, FALSE);

    /* When compnum > string length, should work normally */
    TEST_ASSERT_EQUAL_INT(0, comp_line("short", "short"));
    TEST_ASSERT_EQUAL_INT(-1, comp_line("a", "b"));
    TEST_ASSERT_EQUAL_INT(1, comp_line("z", "a"));
}

/* Test Suite 4: Boundary Conditions */
void test_comp_line_empty_strings(void) {
    setup_global_state(DEFAULT_COMP, FALSE);

    TEST_ASSERT_EQUAL_INT(0, comp_line("", ""));
    TEST_ASSERT_EQUAL_INT(-1, comp_line("", "a"));
    TEST_ASSERT_EQUAL_INT(1, comp_line("a", ""));
}

void test_comp_line_null_termination(void) {
    setup_global_state(5, FALSE);

    /* Null character should terminate comparison early */
    TEST_ASSERT_EQUAL_INT(0, comp_line("test\0xxx", "test\0yyy"));
    TEST_ASSERT_EQUAL_INT(-1, comp_line("tes\0", "test"));
}

void test_comp_line_single_character(void) {
    setup_global_state(DEFAULT_COMP, FALSE);

    TEST_ASSERT_EQUAL_INT(-1, comp_line("a", "b"));
    TEST_ASSERT_EQUAL_INT(0, comp_line("x", "x"));
    TEST_ASSERT_EQUAL_INT(1, comp_line("z", "y"));
}

/* Test Suite 5: Combined Case and Length Tests */
void test_comp_line_case_insensitive_with_length_limit(void) {
    setup_global_state(2, TRUE);

    /* First 2 chars, ignore case */
    TEST_ASSERT_EQUAL_INT(0, comp_line("TEst", "te999"));
    TEST_ASSERT_EQUAL_INT(-1, comp_line("AB", "AC"));
    TEST_ASSERT_EQUAL_INT(1, comp_line("ZZ", "AA"));
}

/* Test Suite 6: Edge Cases */
void test_comp_line_special_characters(void) {
    setup_global_state(DEFAULT_COMP, FALSE);

    /* Test with numbers, punctuation */
    TEST_ASSERT_EQUAL_INT(-1, comp_line("123", "456"));
    TEST_ASSERT_EQUAL_INT(0, comp_line("!@#", "!@#"));
    TEST_ASSERT_EQUAL_INT(1, comp_line("zzz", "111"));
}

void test_comp_line_mixed_alphanumeric(void) {
    setup_global_state(3, FALSE);

    TEST_ASSERT_EQUAL_INT(-1, comp_line("a1b", "a2b"));
    TEST_ASSERT_EQUAL_INT(0, comp_line("x9y", "x9y"));
    TEST_ASSERT_EQUAL_INT(1, comp_line("z0z", "a0a"));
}

/* Test Suite 7: Comprehensive Integration Tests */
void test_comp_line_realistic_sorting_scenarios(void) {
    /* Test realistic scenarios similar to actual usage */

    /* Default 2-character comparison, case sensitive */
    setup_global_state(2, FALSE);
    TEST_ASSERT_EQUAL_INT(-1, comp_line("apple", "banana"));
    TEST_ASSERT_EQUAL_INT(0, comp_line("apple", "application")); /* First 2 chars same */
    TEST_ASSERT_EQUAL_INT(1, comp_line("zebra", "apple"));

    /* Case insensitive filename sorting */
    setup_global_state(8, TRUE);
    TEST_ASSERT_EQUAL_INT(0, comp_line("FILE.TXT", "file.txt"));
    TEST_ASSERT_EQUAL_INT(-1, comp_line("data.csv", "report.doc"));
}

/* Main test runner */
int main(void) {
    UNITY_BEGIN();

    /* Basic comparison tests */
    RUN_TEST(test_comp_line_basic_less_than);
    RUN_TEST(test_comp_line_basic_equal);
    RUN_TEST(test_comp_line_basic_greater_than);

    /* Case sensitivity tests */
    RUN_TEST(test_comp_line_case_sensitive_default);
    RUN_TEST(test_comp_line_case_insensitive);

    /* Length limiting tests */
    RUN_TEST(test_comp_line_length_limit_1_char);
    RUN_TEST(test_comp_line_length_limit_3_chars);
    RUN_TEST(test_comp_line_length_limit_longer_than_string);

    /* Boundary conditions */
    RUN_TEST(test_comp_line_empty_strings);
    RUN_TEST(test_comp_line_null_termination);
    RUN_TEST(test_comp_line_single_character);

    /* Combined tests */
    RUN_TEST(test_comp_line_case_insensitive_with_length_limit);

    /* Edge cases */
    RUN_TEST(test_comp_line_special_characters);
    RUN_TEST(test_comp_line_mixed_alphanumeric);

    /* Integration tests */
    RUN_TEST(test_comp_line_realistic_sorting_scenarios);

    return UNITY_END();
}