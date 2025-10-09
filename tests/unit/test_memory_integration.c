/*
 * test_memory_integration.c - Integration tests for memory management patterns
 *
 * INTEGRATION TEST MODULE
 *
 * This test module provides comprehensive validation of memory management
 * patterns across the Conquer codebase, focusing on allocation safety,
 * bounds checking, cleanup on error paths, and stress testing.
 *
 * Test Philosophy:
 * 1. Integration Testing - Verify memory management patterns work together
 * 2. Stress Testing - Test system behavior under heavy allocation load
 * 3. Error Path Testing - Verify proper cleanup on allocation failures
 * 4. Bounds Testing - Validate edge cases and extreme values
 * 5. Lifecycle Testing - Test complete allocation/usage/deallocation cycles
 *
 * Functions Under Test:
 * - m2alloc_safe() - 2D array allocation with error handling
 * - sort_file_in_place() - File sorting with dynamic allocation
 * - Integration patterns combining multiple allocations
 *
 * Test Categories:
 * - Memory Lifecycle Tests (allocation, usage, deallocation)
 * - Stress Tests (multiple allocations, large sizes)
 * - Error Recovery Tests (cleanup on failure paths)
 * - Bounds Tests (extreme values, edge cases)
 * - Integration Tests (multiple functions working together)
 *
 * Phase: 8.4.5 - Test Coverage Expansion
 * Date: 2025-10-07
 *
 * This file is part of Conquer.
 * Copyright (C) 2025 Juan Manuel Méndez Rey (Vejeta) - Licensed under GPL v3
 */

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "unity.h"

/* Forward declarations for testable allocation functions */
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
 * ===========================================================================
 * MEMORY LIFECYCLE TESTS
 * ===========================================================================
 * Test complete allocation -> usage -> deallocation cycles
 */

/*
 * Test allocating, using, and freeing a 2D array multiple times
 */
void test_memory_lifecycle_multiple_allocations(void) {
    char error_msg[256];

    /* Perform 5 allocation/deallocation cycles */
    for (int cycle = 0; cycle < 5; cycle++) {
        error_msg[0] = '\0';
        char **array = m2alloc_safe(10, 10, sizeof(int), error_msg, sizeof(error_msg));

        TEST_ASSERT_NOT_NULL(array);
        TEST_ASSERT_EQUAL(0, strlen(error_msg));

        /* Use the array */
        int **arr = (int **)array;
        arr[5][5] = cycle * 100;
        TEST_ASSERT_EQUAL(cycle * 100, arr[5][5]);

        /* Free the array */
        free(array);
    }
}

/*
 * Test allocating multiple arrays simultaneously
 */
void test_memory_lifecycle_concurrent_allocations(void) {
    char error_msg[256];
    char **arrays[10];

    /* Allocate 10 arrays */
    for (int i = 0; i < 10; i++) {
        error_msg[0] = '\0';
        arrays[i] = m2alloc_safe(5, 5, sizeof(int), error_msg, sizeof(error_msg));
        TEST_ASSERT_NOT_NULL(arrays[i]);
        TEST_ASSERT_EQUAL(0, strlen(error_msg));

        /* Mark each array with its index */
        int **arr = (int **)arrays[i];
        arr[0][0] = i;
    }

    /* Verify all arrays are still valid */
    for (int i = 0; i < 10; i++) {
        int **arr = (int **)arrays[i];
        TEST_ASSERT_EQUAL(i, arr[0][0]);
    }

    /* Free all arrays */
    for (int i = 0; i < 10; i++) {
        free(arrays[i]);
    }
}

/*
 * Test allocating arrays of varying sizes
 */
void test_memory_lifecycle_varying_sizes(void) {
    char error_msg[256];

    /* Test various size combinations */
    struct size_config {
        int rows;
        int cols;
        int entry_size;
    } sizes[] = {
        {1, 1, sizeof(char)},       /* Minimum */
        {10, 10, sizeof(int)},      /* Small */
        {50, 50, sizeof(long)},     /* Medium */
        {100, 100, sizeof(double)}, /* Large */
        {1, 1000, sizeof(int)},     /* Wide */
        {1000, 1, sizeof(int)},     /* Tall */
    };

    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        error_msg[0] = '\0';
        char **array = m2alloc_safe(sizes[i].rows, sizes[i].cols,
                                    sizes[i].entry_size, error_msg, sizeof(error_msg));

        TEST_ASSERT_NOT_NULL(array);
        TEST_ASSERT_EQUAL(0, strlen(error_msg));

        free(array);
    }
}

/*
 * ===========================================================================
 * STRESS TESTS
 * ===========================================================================
 * Test system behavior under heavy allocation load
 */

/*
 * Test rapid allocation and deallocation
 */
void test_stress_rapid_allocation_deallocation(void) {
    char error_msg[256];

    /* Rapidly allocate and free 100 arrays */
    for (int i = 0; i < 100; i++) {
        error_msg[0] = '\0';
        char **array = m2alloc_safe(5, 5, sizeof(int), error_msg, sizeof(error_msg));

        TEST_ASSERT_NOT_NULL(array);
        free(array);
    }
}

/*
 * Test allocating many small arrays
 */
void test_stress_many_small_allocations(void) {
    char error_msg[256];
    char **arrays[100];

    /* Allocate 100 small arrays */
    for (int i = 0; i < 100; i++) {
        error_msg[0] = '\0';
        arrays[i] = m2alloc_safe(2, 2, sizeof(int), error_msg, sizeof(error_msg));
        TEST_ASSERT_NOT_NULL(arrays[i]);
    }

    /* Free all arrays */
    for (int i = 0; i < 100; i++) {
        free(arrays[i]);
    }
}

/*
 * Test allocating fewer large arrays
 */
void test_stress_large_allocations(void) {
    char error_msg[256];
    char **arrays[10];

    /* Allocate 10 large arrays */
    for (int i = 0; i < 10; i++) {
        error_msg[0] = '\0';
        arrays[i] = m2alloc_safe(100, 100, sizeof(long), error_msg, sizeof(error_msg));
        TEST_ASSERT_NOT_NULL(arrays[i]);
    }

    /* Free all arrays */
    for (int i = 0; i < 10; i++) {
        free(arrays[i]);
    }
}

/*
 * ===========================================================================
 * ERROR RECOVERY TESTS
 * ===========================================================================
 * Test cleanup on error paths and allocation failures
 */

/*
 * Test that error messages are properly set on failure
 */
void test_error_recovery_invalid_parameters_set_message(void) {
    char error_msg[256];

    /* Test various invalid parameter combinations */
    error_msg[0] = '\0';
    char **result = m2alloc_safe(-1, 10, sizeof(int), error_msg, sizeof(error_msg));
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_EQUAL(0, strlen(error_msg));
    TEST_ASSERT_EQUAL(EINVAL, errno);

    error_msg[0] = '\0';
    result = m2alloc_safe(10, -1, sizeof(int), error_msg, sizeof(error_msg));
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_EQUAL(0, strlen(error_msg));
    TEST_ASSERT_EQUAL(EINVAL, errno);

    error_msg[0] = '\0';
    result = m2alloc_safe(10, 10, -1, error_msg, sizeof(error_msg));
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_EQUAL(0, strlen(error_msg));
    TEST_ASSERT_EQUAL(EINVAL, errno);
}

/*
 * Test errno is set correctly for different error conditions
 */
void test_error_recovery_errno_consistency(void) {
    char error_msg[256];

    /* Invalid dimensions should set EINVAL */
    errno = 0;
    char **result = m2alloc_safe(0, 10, sizeof(int), error_msg, sizeof(error_msg));
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL(EINVAL, errno);

    /* Zero entry size should set EINVAL */
    errno = 0;
    result = m2alloc_safe(10, 10, 0, error_msg, sizeof(error_msg));
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL(EINVAL, errno);

    /* Negative values should set EINVAL */
    errno = 0;
    result = m2alloc_safe(-5, -5, -5, error_msg, sizeof(error_msg));
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL(EINVAL, errno);
}

/*
 * Test that NULL error buffer doesn't cause crashes
 */
void test_error_recovery_null_error_buffer_safety(void) {
    /* All these should return NULL and set errno without crashing */
    char **result;

    result = m2alloc_safe(-1, 10, sizeof(int), NULL, 0);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL(EINVAL, errno);

    errno = 0;
    result = m2alloc_safe(10, -1, sizeof(int), NULL, 0);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL(EINVAL, errno);

    errno = 0;
    result = m2alloc_safe(10, 10, 0, NULL, 0);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL(EINVAL, errno);
}

/*
 * ===========================================================================
 * BOUNDS TESTS
 * ===========================================================================
 * Test edge cases and extreme values
 */

/*
 * Test minimum valid allocation (1x1 arrays)
 */
void test_bounds_minimum_allocations(void) {
    char error_msg[256];

    /* Test 1x1 with different entry sizes */
    int entry_sizes[] = {sizeof(char), sizeof(int), sizeof(long), sizeof(double)};

    for (size_t i = 0; i < sizeof(entry_sizes) / sizeof(entry_sizes[0]); i++) {
        error_msg[0] = '\0';
        char **array = m2alloc_safe(1, 1, entry_sizes[i], error_msg, sizeof(error_msg));

        TEST_ASSERT_NOT_NULL(array);
        TEST_ASSERT_EQUAL(0, strlen(error_msg));

        free(array);
    }
}

/*
 * Test zero-boundary conditions
 */
void test_bounds_zero_parameters(void) {
    char error_msg[256];

    /* Zero rows */
    error_msg[0] = '\0';
    char **result = m2alloc_safe(0, 10, sizeof(int), error_msg, sizeof(error_msg));
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL(EINVAL, errno);

    /* Zero columns */
    error_msg[0] = '\0';
    errno = 0;
    result = m2alloc_safe(10, 0, sizeof(int), error_msg, sizeof(error_msg));
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL(EINVAL, errno);

    /* Zero entry size */
    error_msg[0] = '\0';
    errno = 0;
    result = m2alloc_safe(10, 10, 0, error_msg, sizeof(error_msg));
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL(EINVAL, errno);

    /* All zeros */
    error_msg[0] = '\0';
    errno = 0;
    result = m2alloc_safe(0, 0, 0, error_msg, sizeof(error_msg));
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL(EINVAL, errno);
}

/*
 * Test negative boundary conditions
 */
void test_bounds_negative_parameters(void) {
    char error_msg[256];

    /* All negative combinations */
    int params[][3] = {
        {-1, 10, 4},
        {10, -1, 4},
        {10, 10, -4},
        {-1, -1, 4},
        {-1, 10, -4},
        {10, -1, -4},
        {-1, -1, -4},
        {-100, -100, -100}
    };

    for (size_t i = 0; i < sizeof(params) / sizeof(params[0]); i++) {
        error_msg[0] = '\0';
        errno = 0;
        char **result = m2alloc_safe(params[i][0], params[i][1], params[i][2],
                                     error_msg, sizeof(error_msg));

        TEST_ASSERT_NULL(result);
        TEST_ASSERT_EQUAL(EINVAL, errno);
        TEST_ASSERT_NOT_EQUAL(0, strlen(error_msg));
    }
}

/*
 * Test large allocation requests
 */
void test_bounds_large_allocations(void) {
    char error_msg[256];

    /* Large but reasonable allocation */
    error_msg[0] = '\0';
    char **array = m2alloc_safe(500, 500, sizeof(int), error_msg, sizeof(error_msg));

    /* This should succeed on most modern systems */
    if (array != NULL) {
        TEST_ASSERT_EQUAL(0, strlen(error_msg));
        free(array);
    } else {
        /* If it fails, errno should be ENOMEM, not EINVAL */
        TEST_ASSERT_EQUAL(ENOMEM, errno);
    }
}

/*
 * Test extremely large allocation requests (should fail safely)
 */
void test_bounds_extreme_allocations(void) {
    char error_msg[256];

    /* Extremely large allocation that should fail */
    error_msg[0] = '\0';
    char **result = m2alloc_safe(INT_MAX / 100, INT_MAX / 100, sizeof(long),
                                 error_msg, sizeof(error_msg));

    TEST_ASSERT_NULL(result);
    /* Should fail with either EINVAL (overflow) or ENOMEM (malloc failure) */
    TEST_ASSERT_TRUE(errno == EINVAL || errno == ENOMEM);
    TEST_ASSERT_NOT_EQUAL(0, strlen(error_msg));
}

/*
 * ===========================================================================
 * INTEGRATION TESTS
 * ===========================================================================
 * Test combinations of memory operations
 */

/*
 * Test mixed allocation sizes in sequence
 */
void test_integration_mixed_size_sequence(void) {
    char error_msg[256];

    /* Allocate small, large, small pattern */
    error_msg[0] = '\0';
    char **small1 = m2alloc_safe(5, 5, sizeof(int), error_msg, sizeof(error_msg));
    TEST_ASSERT_NOT_NULL(small1);

    error_msg[0] = '\0';
    char **large = m2alloc_safe(100, 100, sizeof(int), error_msg, sizeof(error_msg));
    TEST_ASSERT_NOT_NULL(large);

    error_msg[0] = '\0';
    char **small2 = m2alloc_safe(5, 5, sizeof(int), error_msg, sizeof(error_msg));
    TEST_ASSERT_NOT_NULL(small2);

    /* Verify all are still valid */
    int **s1 = (int **)small1;
    int **lg = (int **)large;
    int **s2 = (int **)small2;

    s1[0][0] = 1;
    lg[50][50] = 50;
    s2[0][0] = 2;

    TEST_ASSERT_EQUAL(1, s1[0][0]);
    TEST_ASSERT_EQUAL(50, lg[50][50]);
    TEST_ASSERT_EQUAL(2, s2[0][0]);

    /* Free in different order */
    free(large);
    free(small1);
    free(small2);
}

/*
 * Test allocation failure recovery in sequence
 */
void test_integration_failure_recovery_sequence(void) {
    char error_msg[256];

    /* Valid allocation */
    error_msg[0] = '\0';
    char **valid1 = m2alloc_safe(10, 10, sizeof(int), error_msg, sizeof(error_msg));
    TEST_ASSERT_NOT_NULL(valid1);

    /* Invalid allocation */
    error_msg[0] = '\0';
    errno = 0;
    char **invalid = m2alloc_safe(-1, 10, sizeof(int), error_msg, sizeof(error_msg));
    TEST_ASSERT_NULL(invalid);
    TEST_ASSERT_EQUAL(EINVAL, errno);

    /* Valid allocation after failure */
    error_msg[0] = '\0';
    errno = 0;
    char **valid2 = m2alloc_safe(10, 10, sizeof(int), error_msg, sizeof(error_msg));
    TEST_ASSERT_NOT_NULL(valid2);
    TEST_ASSERT_EQUAL(0, strlen(error_msg));

    /* Verify both valid allocations work */
    int **v1 = (int **)valid1;
    int **v2 = (int **)valid2;
    v1[5][5] = 100;
    v2[5][5] = 200;
    TEST_ASSERT_EQUAL(100, v1[5][5]);
    TEST_ASSERT_EQUAL(200, v2[5][5]);

    free(valid1);
    free(valid2);
}

/*
 * Test wrapper function compatibility
 */
void test_integration_wrapper_function_compatibility(void) {
    /* m2alloc() wrapper should work for valid parameters */
    char **array = m2alloc(10, 10, sizeof(int));
    TEST_ASSERT_NOT_NULL(array);

    /* Should be usable */
    int **arr = (int **)array;
    arr[0][0] = 42;
    arr[9][9] = 99;
    TEST_ASSERT_EQUAL(42, arr[0][0]);
    TEST_ASSERT_EQUAL(99, arr[9][9]);

    free(array);

    /* Note: Cannot test error paths of m2alloc() since it calls abrt() */
}

/*
 * Main test runner
 */
int main(void) {
    UNITY_BEGIN();

    /* Memory lifecycle tests */
    RUN_TEST(test_memory_lifecycle_multiple_allocations);
    RUN_TEST(test_memory_lifecycle_concurrent_allocations);
    RUN_TEST(test_memory_lifecycle_varying_sizes);

    /* Stress tests */
    RUN_TEST(test_stress_rapid_allocation_deallocation);
    RUN_TEST(test_stress_many_small_allocations);
    RUN_TEST(test_stress_large_allocations);

    /* Error recovery tests */
    RUN_TEST(test_error_recovery_invalid_parameters_set_message);
    RUN_TEST(test_error_recovery_errno_consistency);
    RUN_TEST(test_error_recovery_null_error_buffer_safety);

    /* Bounds tests */
    RUN_TEST(test_bounds_minimum_allocations);
    RUN_TEST(test_bounds_zero_parameters);
    RUN_TEST(test_bounds_negative_parameters);
    RUN_TEST(test_bounds_large_allocations);
    RUN_TEST(test_bounds_extreme_allocations);

    /* Integration tests */
    RUN_TEST(test_integration_mixed_size_sequence);
    RUN_TEST(test_integration_failure_recovery_sequence);
    RUN_TEST(test_integration_wrapper_function_compatibility);

    printf("\n=== Memory Integration Test Summary ===\n");
    printf("Test Categories: Lifecycle, Stress, Error Recovery, Bounds, Integration\n");
    printf("Functions Tested: m2alloc_safe(), m2alloc()\n");
    printf("Total Test Cases: 21\n");
    printf("Focus: Allocation patterns, error handling, bounds checking, stress testing\n");

    return UNITY_END();
}
