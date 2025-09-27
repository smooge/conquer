/*
 * test_example_unit.c - Example unit test for Unity infrastructure verification
 *
 * This is a simple example test to verify that the Unity testing framework
 * is properly integrated with the CMake build system. Once the infrastructure
 * is verified, this file should be removed and replaced with real unit tests.
 *
 * Phase 6.1: Testing Infrastructure Setup
 */

#include "unity.h"
#include <stdio.h>
#include <string.h>

/*
 * Unity test setup and teardown functions
 */
void setUp(void) {
    /* This function runs before each test */
}

void tearDown(void) {
    /* This function runs after each test */
}

/*
 * Example test functions to verify Unity integration
 */

void test_unity_basic_assertions(void) {
    /* Test basic assertion types */
    TEST_ASSERT_TRUE(1);
    TEST_ASSERT_FALSE(0);
    TEST_ASSERT_EQUAL(42, 42);
    TEST_ASSERT_NOT_EQUAL(42, 24);
}

void test_unity_string_assertions(void) {
    /* Test string comparison capabilities */
    const char* expected = "Hello, World!";
    const char* actual = "Hello, World!";

    TEST_ASSERT_EQUAL_STRING(expected, actual);
    TEST_ASSERT_EQUAL_STRING_LEN("Hello", "Hello, Unity!", 5);
}

void test_unity_memory_assertions(void) {
    /* Test memory comparison capabilities */
    unsigned char expected[] = {0x01, 0x02, 0x03, 0x04};
    unsigned char actual[] = {0x01, 0x02, 0x03, 0x04};

    TEST_ASSERT_EQUAL_MEMORY(expected, actual, sizeof(expected));
}

void test_unity_float_assertions(void) {
    /* Test floating point comparison capabilities */
    float expected = 3.14159f;
    float actual = 3.14159f;

    TEST_ASSERT_EQUAL_FLOAT(expected, actual);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.14f, 3.141f);
}

void test_unity_pointer_assertions(void) {
    /* Test pointer assertion capabilities */
    int value = 42;
    int* valid_ptr = &value;
    int* null_ptr = NULL;

    TEST_ASSERT_NOT_NULL(valid_ptr);
    TEST_ASSERT_NULL(null_ptr);
    TEST_ASSERT_EQUAL_PTR(valid_ptr, &value);
}

/*
 * Simple function to test with Unity
 */
static int add_numbers(int a, int b) {
    return a + b;
}

void test_simple_function(void) {
    /* Test a simple function to demonstrate testing approach */
    TEST_ASSERT_EQUAL(5, add_numbers(2, 3));
    TEST_ASSERT_EQUAL(0, add_numbers(-5, 5));
    TEST_ASSERT_EQUAL(-10, add_numbers(-5, -5));
}

/*
 * Test project configuration accessibility
 */
void test_config_header_access(void) {
    /* Verify that config.h is accessible from tests */
    #ifdef PATCHLEVEL
        /* If PATCHLEVEL is defined, we have config.h access */
        TEST_ASSERT_TRUE(1);
    #else
        /* This should not happen if CMake integration is correct */
        TEST_FAIL_MESSAGE("config.h not accessible from test");
    #endif
}

/*
 * Main test runner function
 * Unity will call this to run all tests
 */
int main(void) {
    UNITY_BEGIN();

    /* Run all test functions */
    RUN_TEST(test_unity_basic_assertions);
    RUN_TEST(test_unity_string_assertions);
    RUN_TEST(test_unity_memory_assertions);
    RUN_TEST(test_unity_float_assertions);
    RUN_TEST(test_unity_pointer_assertions);
    RUN_TEST(test_simple_function);
    RUN_TEST(test_config_header_access);

    return UNITY_END();
}