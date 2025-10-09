/*
 * test_sort_utils.c - Unit tests for utility functions
 *
 * Phase 6.5 Session 3: Testing Level 0 functions
 *
 * Functions tested:
 * - build_node() - Linked list node creation utility (from sort.c)
 * - parsepagesize() - Page size string parser (from psmap.c)
 *
 * Testing approach: Copy static function implementation for testing
 * Rationale: Pure utility functions with no global dependencies
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sysexits.h>
#include "unity.h"

/* Copy data structure definition from sort.c */
typedef struct holder {
    char *line;
    struct holder *next;
} L_DATA, *L_PTR;

/* Copy constant from sort.c */
#define MAX_STR 200

/* Copy build_node implementation from sort.c for testing (Phase 8.4.3.2 - with validation) */
static L_PTR build_node(char data[], L_PTR nptr) {
    L_PTR temp;

    /* Validate input parameter */
    if (data == NULL) {
        fprintf(stderr, "build_node: NULL data parameter\n");
        exit(EX_SOFTWARE);
    }

    /* Check string length and bounds */
    size_t data_len = strlen(data);
    if (data_len > MAX_STR) {
        fprintf(stderr, "build_node: String too long (%zu > %d)\n", data_len, MAX_STR);
        exit(EX_SOFTWARE);
    }

    /* Check for overflow in allocation size calculation */
    if (data_len >= SIZE_MAX - 1) {
        fprintf(stderr, "build_node: String length overflow\n");
        exit(EX_SOFTWARE);
    }

    /* build the memory space */
    if ((temp = (L_PTR)malloc(sizeof(L_DATA))) == (L_PTR)NULL) {
        fprintf(stderr, "Error in creating structure memory!\n");
        exit(EX_SOFTWARE);
    }
    if ((temp->line = (char *)malloc((strlen(data) + 1) * sizeof(char))) == NULL) {
        fprintf(stderr, "Error in creating data memory!\n");
        exit(EX_SOFTWARE);
    }

    /* assign the values */
    memcpy(temp->line, data, data_len);
    temp->line[data_len] = '\0';
    temp->next = nptr;
    return (temp);
}

/* Helper function to free a node for cleanup */
static void free_node(L_PTR node) {
    if (node) {
        free(node->line);
        free(node);
    }
}

/* Test setUp - called before each test */
void setUp(void) {
    /* No setup needed for these pure functions */
}

/* Test tearDown - called after each test */
void tearDown(void) {
    /* Tests handle their own cleanup */
}

/*
 * Test build_node with basic string and NULL next pointer
 */
void test_build_node_basic_string_null_next(void) {
    char test_data[] = "test string";
    L_PTR result = build_node(test_data, NULL);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->line);
    TEST_ASSERT_EQUAL_STRING("test string", result->line);
    TEST_ASSERT_NULL(result->next);

    /* Verify memory independence */
    strncpy(test_data, "changed", sizeof(test_data) - 1);
    test_data[sizeof(test_data) - 1] = '\0';
    TEST_ASSERT_EQUAL_STRING("test string", result->line);

    free_node(result);
}

/*
 * Test build_node with empty string
 */
void test_build_node_empty_string(void) {
    char test_data[] = "";
    L_PTR result = build_node(test_data, NULL);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->line);
    TEST_ASSERT_EQUAL_STRING("", result->line);
    TEST_ASSERT_NULL(result->next);

    free_node(result);
}

/*
 * Test build_node with long string
 */
void test_build_node_long_string(void) {
    char test_data[] = "This is a very long string that should still be handled correctly by "
                       "the build_node function";
    L_PTR result = build_node(test_data, NULL);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->line);
    TEST_ASSERT_EQUAL_STRING(test_data, result->line);
    TEST_ASSERT_NULL(result->next);

    free_node(result);
}

/*
 * Test build_node with special characters
 */
void test_build_node_special_characters(void) {
    char test_data[] = "!@#$%^&*()_+-=[]{}|;':\",./<>?";
    L_PTR result = build_node(test_data, NULL);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->line);
    TEST_ASSERT_EQUAL_STRING(test_data, result->line);
    TEST_ASSERT_NULL(result->next);

    free_node(result);
}

/*
 * Test build_node with whitespace characters
 */
void test_build_node_whitespace_characters(void) {
    char test_data[] = "  \t\n  spaces and tabs  \t  ";
    L_PTR result = build_node(test_data, NULL);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->line);
    TEST_ASSERT_EQUAL_STRING(test_data, result->line);
    TEST_ASSERT_NULL(result->next);

    free_node(result);
}

/*
 * Test build_node with non-NULL next pointer (linked list building)
 */
void test_build_node_with_next_pointer(void) {
    /* Create first node */
    L_PTR first = build_node("first", NULL);
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_EQUAL_STRING("first", first->line);
    TEST_ASSERT_NULL(first->next);

    /* Create second node pointing to first */
    L_PTR second = build_node("second", first);
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_EQUAL_STRING("second", second->line);
    TEST_ASSERT_EQUAL_PTR(first, second->next);

    /* Verify first node unchanged */
    TEST_ASSERT_EQUAL_STRING("first", first->line);
    TEST_ASSERT_NULL(first->next);

    free_node(second);
    free_node(first);
}

/*
 * Test build_node creating a chain of nodes
 */
void test_build_node_chain_creation(void) {
    L_PTR node1 = build_node("node1", NULL);
    L_PTR node2 = build_node("node2", node1);
    L_PTR node3 = build_node("node3", node2);

    /* Verify chain structure */
    TEST_ASSERT_NOT_NULL(node3);
    TEST_ASSERT_EQUAL_STRING("node3", node3->line);
    TEST_ASSERT_EQUAL_PTR(node2, node3->next);

    TEST_ASSERT_NOT_NULL(node2);
    TEST_ASSERT_EQUAL_STRING("node2", node2->line);
    TEST_ASSERT_EQUAL_PTR(node1, node2->next);

    TEST_ASSERT_NOT_NULL(node1);
    TEST_ASSERT_EQUAL_STRING("node1", node1->line);
    TEST_ASSERT_NULL(node1->next);

    /* Clean up chain */
    free_node(node3);
    free_node(node2);
    free_node(node1);
}

/*
 * Test build_node memory independence
 */
void test_build_node_memory_independence(void) {
    char original[] = "original data";
    L_PTR result = build_node(original, NULL);

    /* Verify original content */
    TEST_ASSERT_EQUAL_STRING("original data", result->line);

    /* Modify original buffer */
    strncpy(original, "modified data", sizeof(original) - 1);
    original[sizeof(original) - 1] = '\0';

    /* Node should be unchanged */
    TEST_ASSERT_EQUAL_STRING("original data", result->line);

    /* Modify node data */
    /* Note: result->line is dynamically allocated with exact size needed */
    strncpy(result->line, "node modified", strlen(result->line));
    result->line[strlen("node modified")] = '\0';

    /* Original buffer should be unchanged */
    TEST_ASSERT_EQUAL_STRING("modified data", original);

    free_node(result);
}

/*
 * Test build_node with numeric strings
 */
void test_build_node_numeric_strings(void) {
    char test_data[] = "12345 67890 -123.456";
    L_PTR result = build_node(test_data, NULL);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->line);
    TEST_ASSERT_EQUAL_STRING(test_data, result->line);
    TEST_ASSERT_NULL(result->next);

    free_node(result);
}

/*
 * Test build_node proper memory allocation
 */
void test_build_node_memory_allocation_size(void) {
    char test_data[] = "exact size test";
    L_PTR result = build_node(test_data, NULL);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->line);

    /* Verify we can read the entire string */
    TEST_ASSERT_EQUAL_STRING(test_data, result->line);

    /* Verify string length matches exactly */
    TEST_ASSERT_EQUAL_size_t(strlen(test_data), strlen(result->line));

    free_node(result);
}

/*
 * ===========================================================================
 * parsepagesize() tests - Page size string parser from psmap.c
 * ===========================================================================
 */

/* Copy parsepagesize implementation from psmap.c for testing */
static int parsepagesize(char *buf) {
    if (!strcmp(buf, "A4"))
        return (1);
    if (!strcmp(buf, "a4"))
        return (1);
    if (!strcmp(buf, "LETTER"))
        return (2);
    if (!strcmp(buf, "letter"))
        return (2);
    return (0);
}

/*
 * Test parsepagesize with valid A4 uppercase
 */
void test_parsepagesize_a4_uppercase(void) {
    char input[] = "A4";
    int result = parsepagesize(input);
    TEST_ASSERT_EQUAL_INT(1, result);
}

/*
 * Test parsepagesize with valid a4 lowercase
 */
void test_parsepagesize_a4_lowercase(void) {
    char input[] = "a4";
    int result = parsepagesize(input);
    TEST_ASSERT_EQUAL_INT(1, result);
}

/*
 * Test parsepagesize with valid LETTER uppercase
 */
void test_parsepagesize_letter_uppercase(void) {
    char input[] = "LETTER";
    int result = parsepagesize(input);
    TEST_ASSERT_EQUAL_INT(2, result);
}

/*
 * Test parsepagesize with valid letter lowercase
 */
void test_parsepagesize_letter_lowercase(void) {
    char input[] = "letter";
    int result = parsepagesize(input);
    TEST_ASSERT_EQUAL_INT(2, result);
}

/*
 * Test parsepagesize with invalid input - mixed case A4
 */
void test_parsepagesize_invalid_mixed_case_a4(void) {
    char input[] = "A4"; /* This should work */
    int result = parsepagesize(input);
    TEST_ASSERT_EQUAL_INT(1, result);

    /* Test actual mixed case */
    char mixed[] = "a4"; /* This should work */
    result = parsepagesize(mixed);
    TEST_ASSERT_EQUAL_INT(1, result);

    /* Test truly mixed case that should fail */
    char invalid[] = "A4x";
    result = parsepagesize(invalid);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/*
 * Test parsepagesize with invalid input - unknown format
 */
void test_parsepagesize_invalid_unknown_format(void) {
    char input[] = "TABLOID";
    int result = parsepagesize(input);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/*
 * Test parsepagesize with empty string
 */
void test_parsepagesize_empty_string(void) {
    char input[] = "";
    int result = parsepagesize(input);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/*
 * Test parsepagesize with invalid partial matches
 */
void test_parsepagesize_partial_matches(void) {
    /* Test strings that contain valid formats but have extra characters */
    char input1[] = "A4EXTRA";
    int result1 = parsepagesize(input1);
    TEST_ASSERT_EQUAL_INT(0, result1);

    char input2[] = "LETTERING";
    int result2 = parsepagesize(input2);
    TEST_ASSERT_EQUAL_INT(0, result2);

    char input3[] = "a4x";
    int result3 = parsepagesize(input3);
    TEST_ASSERT_EQUAL_INT(0, result3);
}

/*
 * Test parsepagesize with whitespace variations
 */
void test_parsepagesize_whitespace_variations(void) {
    char input1[] = " A4";
    int result1 = parsepagesize(input1);
    TEST_ASSERT_EQUAL_INT(0, result1);

    char input2[] = "A4 ";
    int result2 = parsepagesize(input2);
    TEST_ASSERT_EQUAL_INT(0, result2);

    char input3[] = " LETTER ";
    int result3 = parsepagesize(input3);
    TEST_ASSERT_EQUAL_INT(0, result3);
}

/*
 * Test parsepagesize with numeric and special character inputs
 */
void test_parsepagesize_numeric_and_special(void) {
    char input1[] = "1";
    int result1 = parsepagesize(input1);
    TEST_ASSERT_EQUAL_INT(0, result1);

    char input2[] = "2";
    int result2 = parsepagesize(input2);
    TEST_ASSERT_EQUAL_INT(0, result2);

    char input3[] = "@#$%";
    int result3 = parsepagesize(input3);
    TEST_ASSERT_EQUAL_INT(0, result3);
}

/*
 * Test parsepagesize behavior consistency (multiple calls)
 */
void test_parsepagesize_consistency(void) {
    char input[] = "A4";

    /* Call multiple times to ensure consistent behavior */
    int result1 = parsepagesize(input);
    int result2 = parsepagesize(input);
    int result3 = parsepagesize(input);

    TEST_ASSERT_EQUAL_INT(1, result1);
    TEST_ASSERT_EQUAL_INT(1, result2);
    TEST_ASSERT_EQUAL_INT(1, result3);
    TEST_ASSERT_EQUAL_INT(result1, result2);
    TEST_ASSERT_EQUAL_INT(result2, result3);
}

/*
 * Test parsepagesize with all valid formats for completeness
 */
void test_parsepagesize_all_valid_formats(void) {
    /* Test all four valid inputs */
    TEST_ASSERT_EQUAL_INT(1, parsepagesize("A4"));
    TEST_ASSERT_EQUAL_INT(1, parsepagesize("a4"));
    TEST_ASSERT_EQUAL_INT(2, parsepagesize("LETTER"));
    TEST_ASSERT_EQUAL_INT(2, parsepagesize("letter"));

    /* Verify they return the expected distinct values */
    TEST_ASSERT_NOT_EQUAL(parsepagesize("A4"), parsepagesize("LETTER"));
    TEST_ASSERT_EQUAL_INT(parsepagesize("A4"), parsepagesize("a4"));
    TEST_ASSERT_EQUAL_INT(parsepagesize("LETTER"), parsepagesize("letter"));
}

/*
 * Main test runner
 */
int main(void) {
    UNITY_BEGIN();

    /* build_node() tests */
    RUN_TEST(test_build_node_basic_string_null_next);
    RUN_TEST(test_build_node_empty_string);
    RUN_TEST(test_build_node_long_string);
    RUN_TEST(test_build_node_special_characters);
    RUN_TEST(test_build_node_whitespace_characters);
    RUN_TEST(test_build_node_with_next_pointer);
    RUN_TEST(test_build_node_chain_creation);
    RUN_TEST(test_build_node_memory_independence);
    RUN_TEST(test_build_node_numeric_strings);
    RUN_TEST(test_build_node_memory_allocation_size);

    /* parsepagesize() tests */
    RUN_TEST(test_parsepagesize_a4_uppercase);
    RUN_TEST(test_parsepagesize_a4_lowercase);
    RUN_TEST(test_parsepagesize_letter_uppercase);
    RUN_TEST(test_parsepagesize_letter_lowercase);
    RUN_TEST(test_parsepagesize_invalid_mixed_case_a4);
    RUN_TEST(test_parsepagesize_invalid_unknown_format);
    RUN_TEST(test_parsepagesize_empty_string);
    RUN_TEST(test_parsepagesize_partial_matches);
    RUN_TEST(test_parsepagesize_whitespace_variations);
    RUN_TEST(test_parsepagesize_numeric_and_special);
    RUN_TEST(test_parsepagesize_consistency);
    RUN_TEST(test_parsepagesize_all_valid_formats);

    return UNITY_END();
}