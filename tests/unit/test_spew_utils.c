/*
 * test_spew_utils.c - Unit tests for spew.c static utility functions
 *
 * This file contains unit tests for testable static functions from spew.c.
 * These functions are Level 0 testable (minimal dependencies, pure functions).
 *
 * Functions tested:
 * - duplicate_string() - String duplication with dynamic allocation
 * - compare_classes() - qsort comparator for text_class structures
 *
 * IMPORTANT: The production duplicate_string() now uses exit() on errors
 * (NULL input, overlength strings, malloc failure). The test version
 * returns NULL instead of exit() to enable testability. Production behavior:
 * - NULL input -> exit(EX_SOFTWARE)
 * - String > MAX_DEF_LEN (1000) -> exit(EX_SOFTWARE)
 * - malloc() failure -> exit(EX_SOFTWARE)
 *
 * Testing Category: Level 0 (Static Functions)
 * Testing Approach: Direct function testing with controlled inputs
 * Testing Framework: Unity
 */

#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Include safe_convert for the functions being tested */
#include "../../safe_convert.h"

/*
 * Structure definition copied from spew.c for testing
 * This represents the text_class structure used by compare_classes()
 */
struct text_class {
    char *name; /* name of this class */
    char *variants; /* string of variant tags */
    int total_weight; /* total weight of all definitions */
    void *defs; /* linked list of definitions (not used in tests) */
};

/*
 * Static function declarations copied from spew.c
 * These are the functions we're testing
 */
static char *duplicate_string(const char *str);
static int compare_classes(const void *a, const void *b);

/*
 * duplicate_string - Create dynamic copy of string with memory allocation
 *
 * Copied implementation from spew.c for testing
 * NOTE: This implementation now exits on errors for production use,
 * but test version returns NULL for testability
 */
static char *duplicate_string(const char *str) {
    if (!str)
        return NULL; /* Test version returns NULL instead of exit */

    size_t str_len = strlen(str);
    /* Test version: Production code limits to MAX_DEF_LEN and exits */
    if (str_len > 1000) {
        return NULL; /* Test version returns NULL instead of exit */
    }

    int len = safe_size_to_int(str_len);
    char *copy = malloc(safe_int_to_size(len + 1));
    if (!copy) {
        return NULL; /* Test version returns NULL instead of exit */
    }

    memcpy(copy, str, safe_int_to_size(len + 1));
    return copy;
}

/*
 * compare_classes - Compare two text_class structures by name
 *
 * Copied implementation from spew.c for testing
 */
static int compare_classes(const void *a, const void *b) {
    const struct text_class *cls_a = (const struct text_class *)a;
    const struct text_class *cls_b = (const struct text_class *)b;
    return strcmp(cls_a->name, cls_b->name);
}

/* Test setup and teardown */
void setUp(void) {
    /* No setup required for these pure functions */
}

void tearDown(void) {
    /* No teardown required */
}

/* =============================================================================
 * duplicate_string() Tests
 * =============================================================================
 */

/*
 * Test duplicate_string with NULL input
 * Should return NULL without crashing
 */
void test_duplicate_string_null_input(void) {
    char *result = duplicate_string(NULL);
    TEST_ASSERT_NULL(result);
}

/*
 * Test duplicate_string with empty string
 * Should return allocated empty string
 */
void test_duplicate_string_empty_string(void) {
    char *result = duplicate_string("");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
    free(result);
}

/*
 * Test duplicate_string with simple string
 * Should create exact copy
 */
void test_duplicate_string_simple_string(void) {
    const char *original = "hello";
    char *result = duplicate_string(original);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(original, result);
    TEST_ASSERT_NOT_EQUAL(original, result); /* Different memory addresses */

    free(result);
}

/*
 * Test duplicate_string with longer string
 * Should handle strings of various lengths
 */
void test_duplicate_string_long_string(void) {
    const char *original = "This is a longer string with spaces and punctuation!";
    char *result = duplicate_string(original);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(original, result);
    TEST_ASSERT_NOT_EQUAL(original, result);

    free(result);
}

/*
 * Test duplicate_string with special characters
 * Should preserve all characters including nulls and escapes
 */
void test_duplicate_string_special_characters(void) {
    const char *original = "String\twith\nspecial\rcharacters";
    char *result = duplicate_string(original);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(original, result);

    free(result);
}

/*
 * Test duplicate_string preserves single character
 * Edge case testing
 */
void test_duplicate_string_single_character(void) {
    const char *original = "X";
    char *result = duplicate_string(original);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(original, result);
    TEST_ASSERT_EQUAL_CHAR('X', result[0]);
    TEST_ASSERT_EQUAL_CHAR('\0', result[1]);

    free(result);
}

/*
 * Test duplicate_string memory independence
 * Modification of original should not affect copy
 */
void test_duplicate_string_memory_independence(void) {
    char original[] = "modifiable";
    char *result = duplicate_string(original);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("modifiable", result);

    /* Modify original */
    original[0] = 'X';

    /* Copy should be unchanged */
    TEST_ASSERT_EQUAL_STRING("modifiable", result);
    TEST_ASSERT_EQUAL_STRING("Xodifiable", original);

    free(result);
}

/* =============================================================================
 * compare_classes() Tests
 * =============================================================================
 */

/*
 * Test compare_classes with identical names
 * Should return 0 for equal strings
 */
void test_compare_classes_identical_names(void) {
    struct text_class cls_a = {"test", NULL, 0, NULL};
    struct text_class cls_b = {"test", NULL, 0, NULL};

    int result = compare_classes(&cls_a, &cls_b);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/*
 * Test compare_classes with first name alphabetically before second
 * Should return negative value
 */
void test_compare_classes_first_before_second(void) {
    struct text_class cls_a = {"apple", NULL, 0, NULL};
    struct text_class cls_b = {"banana", NULL, 0, NULL};

    int result = compare_classes(&cls_a, &cls_b);
    TEST_ASSERT_TRUE(result < 0);
}

/*
 * Test compare_classes with first name alphabetically after second
 * Should return positive value
 */
void test_compare_classes_first_after_second(void) {
    struct text_class cls_a = {"zebra", NULL, 0, NULL};
    struct text_class cls_b = {"apple", NULL, 0, NULL};

    int result = compare_classes(&cls_a, &cls_b);
    TEST_ASSERT_TRUE(result > 0);
}

/*
 * Test compare_classes with case sensitivity
 * Should be case sensitive (uppercase before lowercase in ASCII)
 */
void test_compare_classes_case_sensitivity(void) {
    struct text_class cls_a = {"Apple", NULL, 0, NULL};
    struct text_class cls_b = {"apple", NULL, 0, NULL};

    int result = compare_classes(&cls_a, &cls_b);
    TEST_ASSERT_TRUE(result < 0); /* 'A' < 'a' in ASCII */
}

/*
 * Test compare_classes with empty strings
 * Should handle empty names properly
 */
void test_compare_classes_empty_names(void) {
    struct text_class cls_a = {"", NULL, 0, NULL};
    struct text_class cls_b = {"", NULL, 0, NULL};

    int result = compare_classes(&cls_a, &cls_b);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/*
 * Test compare_classes with one empty string
 * Empty string should come before non-empty
 */
void test_compare_classes_one_empty(void) {
    struct text_class cls_a = {"", NULL, 0, NULL};
    struct text_class cls_b = {"test", NULL, 0, NULL};

    int result = compare_classes(&cls_a, &cls_b);
    TEST_ASSERT_TRUE(result < 0);

    /* Test reverse order */
    result = compare_classes(&cls_b, &cls_a);
    TEST_ASSERT_TRUE(result > 0);
}

/*
 * Test compare_classes with prefix strings
 * Shorter string that is prefix of longer should come first
 */
void test_compare_classes_prefix_strings(void) {
    struct text_class cls_a = {"test", NULL, 0, NULL};
    struct text_class cls_b = {"testing", NULL, 0, NULL};

    int result = compare_classes(&cls_a, &cls_b);
    TEST_ASSERT_TRUE(result < 0);
}

/*
 * Test compare_classes ignores other fields
 * Only name field should matter for comparison
 */
void test_compare_classes_ignores_other_fields(void) {
    struct text_class cls_a = {"test", "different", 42, (void *)0x1234};
    struct text_class cls_b = {"test", "variants", -1, NULL};

    int result = compare_classes(&cls_a, &cls_b);
    TEST_ASSERT_EQUAL_INT(0, result); /* Should be equal based on name only */
}

/*
 * Test compare_classes sorting behavior
 * Verify it works correctly for qsort ordering
 */
void test_compare_classes_sorting_order(void) {
    struct text_class classes[3] = {
        {"zebra", NULL, 0, NULL}, {"apple", NULL, 0, NULL}, {"banana", NULL, 0, NULL}};

    /* Test all pairwise comparisons for correct ordering */
    TEST_ASSERT_TRUE(compare_classes(&classes[1], &classes[2]) < 0); /* apple < banana */
    TEST_ASSERT_TRUE(compare_classes(&classes[2], &classes[0]) < 0); /* banana < zebra */
    TEST_ASSERT_TRUE(compare_classes(&classes[1], &classes[0]) < 0); /* apple < zebra */
}

/* =============================================================================
 * Main test runner
 * =============================================================================
 */

int main(void) {
    UNITY_BEGIN();

    /* duplicate_string() tests */
    RUN_TEST(test_duplicate_string_null_input);
    RUN_TEST(test_duplicate_string_empty_string);
    RUN_TEST(test_duplicate_string_simple_string);
    RUN_TEST(test_duplicate_string_long_string);
    RUN_TEST(test_duplicate_string_special_characters);
    RUN_TEST(test_duplicate_string_single_character);
    RUN_TEST(test_duplicate_string_memory_independence);

    /* compare_classes() tests */
    RUN_TEST(test_compare_classes_identical_names);
    RUN_TEST(test_compare_classes_first_before_second);
    RUN_TEST(test_compare_classes_first_after_second);
    RUN_TEST(test_compare_classes_case_sensitivity);
    RUN_TEST(test_compare_classes_empty_names);
    RUN_TEST(test_compare_classes_one_empty);
    RUN_TEST(test_compare_classes_prefix_strings);
    RUN_TEST(test_compare_classes_ignores_other_fields);
    RUN_TEST(test_compare_classes_sorting_order);

    return UNITY_END();
}