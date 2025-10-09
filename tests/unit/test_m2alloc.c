/*
 * test_m2alloc.c - Comprehensive test suite for m2alloc_safe() function
 *
 * Tests the testable two-dimensional array allocator with all error
 * conditions, parameter validation, overflow detection, and successful
 * allocation scenarios.
 *
 * Phase: 8.4.3 - Bounds Checking Enhancement
 * Date: 2025-10-06
 */

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "unity.h"

/* Forward declarations for m2alloc functions */
extern char **m2alloc_safe(int nrows, int ncols, int entrysize, char *error_msg, size_t error_msg_size);
extern char **m2alloc(int nrows, int ncols, int entrysize);

/* Test setup and teardown */
void setUp(void) {
    /* Reset errno before each test */
    errno = 0;
}

void tearDown(void) {
    /* Nothing to clean up */
}

/*
 * Parameter Validation Tests
 */

void test_m2alloc_safe_negative_rows(void) {
    char error_msg[256] = {0};
    char **result = m2alloc_safe(-5, 10, sizeof(int),
                                  error_msg, sizeof(error_msg));

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL(EINVAL, errno);
    TEST_ASSERT_NOT_EQUAL(0, strlen(error_msg));
    TEST_ASSERT_TRUE(strstr(error_msg, "Invalid dimensions") != NULL);
}

void test_m2alloc_safe_negative_cols(void) {
    char error_msg[256] = {0};
    char **result = m2alloc_safe(10, -5, sizeof(int),
                                  error_msg, sizeof(error_msg));

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL(EINVAL, errno);
    TEST_ASSERT_NOT_EQUAL(0, strlen(error_msg));
}

void test_m2alloc_safe_negative_entrysize(void) {
    char error_msg[256] = {0};
    char **result = m2alloc_safe(10, 10, -4,
                                  error_msg, sizeof(error_msg));

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL(EINVAL, errno);
    TEST_ASSERT_NOT_EQUAL(0, strlen(error_msg));
}

void test_m2alloc_safe_zero_rows(void) {
    char error_msg[256] = {0};
    char **result = m2alloc_safe(0, 10, sizeof(int),
                                  error_msg, sizeof(error_msg));

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL(EINVAL, errno);
    TEST_ASSERT_TRUE(strstr(error_msg, "Invalid dimensions") != NULL);
}

void test_m2alloc_safe_zero_cols(void) {
    char error_msg[256] = {0};
    char **result = m2alloc_safe(10, 0, sizeof(int),
                                  error_msg, sizeof(error_msg));

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL(EINVAL, errno);
}

void test_m2alloc_safe_zero_entrysize(void) {
    char error_msg[256] = {0};
    char **result = m2alloc_safe(10, 10, 0,
                                  error_msg, sizeof(error_msg));

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL(EINVAL, errno);
}

/*
 * Integer Overflow Tests
 */

void test_m2alloc_safe_overflow_ncols_times_entrysize(void) {
    char error_msg[256] = {0};
    /*
     * Attempt to trigger overflow with large values
     * On 64-bit systems with int params, may get ENOMEM instead of EINVAL
     * because INT_MAX * INT_MAX < SIZE_MAX on 64-bit
     * This tests that huge allocations fail safely
     */
    char **result = m2alloc_safe(2, INT_MAX, INT_MAX / 2,
                                  error_msg, sizeof(error_msg));

    TEST_ASSERT_NULL(result);
    /* Accept either EINVAL (overflow) or ENOMEM (malloc failure) */
    TEST_ASSERT_TRUE(errno == EINVAL || errno == ENOMEM);
    TEST_ASSERT_NOT_EQUAL(0, error_msg[0]);  /* Error message should be set */
}

void test_m2alloc_safe_overflow_nrows_times_row_data_size(void) {
    char error_msg[256] = {0};
    /*
     * Attempt to trigger overflow with large row and column counts
     * On 64-bit systems with int params, may get ENOMEM instead of EINVAL
     * This tests that huge allocations fail safely
     */
    char **result = m2alloc_safe(INT_MAX / 100, INT_MAX / 100, sizeof(long),
                                  error_msg, sizeof(error_msg));

    TEST_ASSERT_NULL(result);
    /* Accept either EINVAL (overflow) or ENOMEM (malloc failure) */
    TEST_ASSERT_TRUE(errno == EINVAL || errno == ENOMEM);
    TEST_ASSERT_NOT_EQUAL(0, error_msg[0]);  /* Error message should be set */
}

void test_m2alloc_safe_overflow_total_size(void) {
    char error_msg[256] = {0};
    /*
     * Test that extremely large allocations fail safely
     * On 64-bit systems with int params, may get ENOMEM instead of EINVAL
     * This tests that huge allocations fail safely
     */
    char **result = m2alloc_safe(INT_MAX / 10, INT_MAX / 10, sizeof(long),
                                  error_msg, sizeof(error_msg));

    TEST_ASSERT_NULL(result);
    /* Accept either EINVAL (overflow) or ENOMEM (malloc failure) */
    TEST_ASSERT_TRUE(errno == EINVAL || errno == ENOMEM);
    TEST_ASSERT_NOT_EQUAL(0, error_msg[0]);  /* Error message should be set */
}

/*
 * Error Message Buffer Tests
 */

void test_m2alloc_safe_null_error_msg_buffer(void) {
    /*
     * Test that NULL error_msg buffer is handled gracefully
     * Function should still return NULL and set errno
     */
    char **result = m2alloc_safe(-5, 10, sizeof(int), NULL, 0);

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL(EINVAL, errno);
    /* Should not crash despite NULL buffer */
}

void test_m2alloc_safe_small_error_msg_buffer(void) {
    char error_msg[10];  /* Very small buffer */
    char **result = m2alloc_safe(-5, 10, sizeof(int),
                                  error_msg, sizeof(error_msg));

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL(EINVAL, errno);
    /* Message should be truncated but not crash */
    TEST_ASSERT_NOT_EQUAL(0, strlen(error_msg));
}

/*
 * Successful Allocation Tests
 */

void test_m2alloc_safe_small_array_int(void) {
    char error_msg[256] = {0};
    char **result = m2alloc_safe(10, 10, sizeof(int),
                                  error_msg, sizeof(error_msg));

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(0, strlen(error_msg));  /* No error message */

    /* Verify we can access the array */
    int **arr = (int **)result;
    arr[0][0] = 42;
    arr[9][9] = 99;
    TEST_ASSERT_EQUAL(42, arr[0][0]);
    TEST_ASSERT_EQUAL(99, arr[9][9]);

    free(result);
}

void test_m2alloc_safe_small_array_char(void) {
    char error_msg[256] = {0};
    char **result = m2alloc_safe(5, 20, sizeof(char),
                                  error_msg, sizeof(error_msg));

    TEST_ASSERT_NOT_NULL(result);

    /* Verify we can access and write characters */
    char **arr = result;
    arr[0][0] = 'A';
    arr[4][19] = 'Z';
    TEST_ASSERT_EQUAL('A', arr[0][0]);
    TEST_ASSERT_EQUAL('Z', arr[4][19]);

    free(result);
}

void test_m2alloc_safe_small_array_struct(void) {
    struct test_struct {
        int x;
        int y;
        long data;
    };

    char error_msg[256] = {0};
    char **result = m2alloc_safe(3, 3, (int)sizeof(struct test_struct),
                                  error_msg, sizeof(error_msg));

    TEST_ASSERT_NOT_NULL(result);

    /* Cast to proper type and use */
    struct test_struct **arr = (struct test_struct **)result;
    arr[1][1].x = 10;
    arr[1][1].y = 20;
    arr[1][1].data = 100L;

    TEST_ASSERT_EQUAL(10, arr[1][1].x);
    TEST_ASSERT_EQUAL(20, arr[1][1].y);
    TEST_ASSERT_EQUAL(100L, arr[1][1].data);

    free(result);
}

void test_m2alloc_safe_rectangular_array(void) {
    char error_msg[256] = {0};
    /* Non-square array (more typical use case) */
    char **result = m2alloc_safe(100, 50, sizeof(int),
                                  error_msg, sizeof(error_msg));

    TEST_ASSERT_NOT_NULL(result);

    /* Verify corners are accessible */
    int **arr = (int **)result;
    arr[0][0] = 1;
    arr[0][49] = 2;
    arr[99][0] = 3;
    arr[99][49] = 4;

    TEST_ASSERT_EQUAL(1, arr[0][0]);
    TEST_ASSERT_EQUAL(2, arr[0][49]);
    TEST_ASSERT_EQUAL(3, arr[99][0]);
    TEST_ASSERT_EQUAL(4, arr[99][49]);

    free(result);
}

void test_m2alloc_safe_single_element(void) {
    char error_msg[256] = {0};
    /* Edge case: 1x1 array */
    char **result = m2alloc_safe(1, 1, sizeof(long),
                                  error_msg, sizeof(error_msg));

    TEST_ASSERT_NOT_NULL(result);

    long **arr = (long **)result;
    arr[0][0] = 12345678L;
    TEST_ASSERT_EQUAL(12345678L, arr[0][0]);

    free(result);
}

void test_m2alloc_safe_single_row(void) {
    char error_msg[256] = {0};
    /* Edge case: 1 row, multiple columns */
    char **result = m2alloc_safe(1, 100, sizeof(int),
                                  error_msg, sizeof(error_msg));

    TEST_ASSERT_NOT_NULL(result);

    int **arr = (int **)result;
    arr[0][0] = 111;
    arr[0][99] = 999;
    TEST_ASSERT_EQUAL(111, arr[0][0]);
    TEST_ASSERT_EQUAL(999, arr[0][99]);

    free(result);
}

void test_m2alloc_safe_single_column(void) {
    char error_msg[256] = {0};
    /* Edge case: multiple rows, 1 column */
    char **result = m2alloc_safe(100, 1, sizeof(int),
                                  error_msg, sizeof(error_msg));

    TEST_ASSERT_NOT_NULL(result);

    int **arr = (int **)result;
    arr[0][0] = 111;
    arr[99][0] = 999;
    TEST_ASSERT_EQUAL(111, arr[0][0]);
    TEST_ASSERT_EQUAL(999, arr[99][0]);

    free(result);
}

/*
 * Wrapper Function Test
 */

void test_m2alloc_wrapper_maintains_compatibility(void) {
    /*
     * This test verifies m2alloc() wrapper works correctly
     * with valid parameters (never returns NULL)
     *
     * Cannot test error paths since wrapper calls abrt()
     */
    char **result = m2alloc(10, 10, sizeof(int));

    TEST_ASSERT_NOT_NULL(result);

    /* Verify functionality */
    int **arr = (int **)result;
    arr[5][5] = 555;
    TEST_ASSERT_EQUAL(555, arr[5][5]);

    free(result);
}

/*
 * Memory Layout Verification Tests
 */

void test_m2alloc_safe_contiguous_memory_layout(void) {
    char error_msg[256] = {0};
    char **result = m2alloc_safe(10, 10, sizeof(int),
                                  error_msg, sizeof(error_msg));

    TEST_ASSERT_NOT_NULL(result);

    /*
     * Verify contiguous layout:
     * - Row pointers should point to contiguous data section
     * - Each row should be adjacent in memory
     */
    int **arr = (int **)result;

    /* Distance between rows should be cols * sizeof(type) */
    size_t row_stride = (char *)arr[1] - (char *)arr[0];
    size_t expected_stride = 10 * sizeof(int);
    TEST_ASSERT_EQUAL(expected_stride, row_stride);

    /* All rows should have same stride */
    for (int i = 1; i < 9; i++) {
        size_t stride = (char *)arr[i+1] - (char *)arr[i];
        TEST_ASSERT_EQUAL(expected_stride, stride);
    }

    free(result);
}

/*
 * Main test runner
 */
int main(void) {
    UNITY_BEGIN();

    /* Parameter validation tests */
    RUN_TEST(test_m2alloc_safe_negative_rows);
    RUN_TEST(test_m2alloc_safe_negative_cols);
    RUN_TEST(test_m2alloc_safe_negative_entrysize);
    RUN_TEST(test_m2alloc_safe_zero_rows);
    RUN_TEST(test_m2alloc_safe_zero_cols);
    RUN_TEST(test_m2alloc_safe_zero_entrysize);

    /* Integer overflow tests */
    RUN_TEST(test_m2alloc_safe_overflow_ncols_times_entrysize);
    RUN_TEST(test_m2alloc_safe_overflow_nrows_times_row_data_size);
    RUN_TEST(test_m2alloc_safe_overflow_total_size);

    /* Error message buffer tests */
    RUN_TEST(test_m2alloc_safe_null_error_msg_buffer);
    RUN_TEST(test_m2alloc_safe_small_error_msg_buffer);

    /* Successful allocation tests */
    RUN_TEST(test_m2alloc_safe_small_array_int);
    RUN_TEST(test_m2alloc_safe_small_array_char);
    RUN_TEST(test_m2alloc_safe_small_array_struct);
    RUN_TEST(test_m2alloc_safe_rectangular_array);
    RUN_TEST(test_m2alloc_safe_single_element);
    RUN_TEST(test_m2alloc_safe_single_row);
    RUN_TEST(test_m2alloc_safe_single_column);

    /* Wrapper function test */
    RUN_TEST(test_m2alloc_wrapper_maintains_compatibility);

    /* Memory layout verification */
    RUN_TEST(test_m2alloc_safe_contiguous_memory_layout);

    return UNITY_END();
}
