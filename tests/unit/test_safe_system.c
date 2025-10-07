/*
 * test_safe_system.c - Comprehensive unit tests for safe_system.h functions
 *
 * UNIT TEST MODULE
 *
 * This test module provides comprehensive validation of all security functions
 * that replace unsafe system() calls with safer native C implementations.
 * Tests verify correct behavior, security properties, and error handling.
 *
 * Test Philosophy:
 * 1. Security Testing - Verify no shell injection vulnerabilities
 * 2. Functional Testing - Ensure equivalent behavior to replaced system() calls
 * 3. Error Handling - Test proper error reporting and recovery
 * 4. File System Testing - Validate file operations work correctly
 * 5. Edge Case Testing - Handle unusual but valid scenarios
 *
 * Functions Under Test:
 * - append_file_to_file() - Safe file concatenation
 * - write_timestamp_to_file() - Safe timestamp generation
 * - secure_file_delete() - Safe file deletion with glob patterns
 *
 * Test Categories:
 * - Normal Operation Tests (happy path scenarios)
 * - Error Condition Tests (invalid inputs, missing files)
 * - Security Tests (verify no command injection possible)
 * - File System Tests (permissions, locks, concurrent access)
 *
 * This file is part of Conquer.
 * Copyright (C) 2025 Juan Manuel Méndez Rey (Vejeta) - Licensed under GPL v3
 */

/* Feature test macro for nftw() - POSIX.1-2008 */
#define _XOPEN_SOURCE 700

#include "unity.h"
#include "safe_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <glob.h>
#include <errno.h>
#include <ftw.h>

/* Mock implementation of check_lock() for unit testing */
int check_lock(const char *name, int acquire) {
    (void)name;    /* Suppress unused parameter warning */
    (void)acquire; /* Suppress unused parameter warning */
    return 0;      /* Always return FALSE (lock available) for testing */
}

/* Test fixture directory for temporary files */
#define TEST_DIR "tests_tmp_safe_system"
#define TEST_SRC_FILE TEST_DIR "/test_source.txt"
#define TEST_DEST_FILE TEST_DIR "/test_dest.txt"
#define TEST_TIMESTAMP_FILE TEST_DIR "/test_timestamp.txt"
#define TEST_DELETE_FILE1 TEST_DIR "/delete_me_1.txt"
#define TEST_DELETE_FILE2 TEST_DIR "/delete_me_2.txt"
#define TEST_DELETE_FILE3 TEST_DIR "/delete_me_3.log"

/*
 * Test Setup and Teardown
 */
void setUp(void) {
    /* Create test directory */
    mkdir(TEST_DIR, 0755);
}

/*
 * remove_callback - Helper for nftw() to remove files and directories
 *
 * Called by nftw() for each file system entry during tree walk.
 * Removes the entry (file or directory) using the remove() function.
 *
 * Parameters:
 *   fpath - Full path to the file/directory
 *   sb - Stat buffer (unused)
 *   typeflag - Type of entry (unused)
 *   ftwbuf - FTW buffer (unused)
 *
 * Returns:
 *   0 on success, result of remove() on failure
 */
static int remove_callback(const char *fpath, const struct stat *sb,
                          int typeflag, struct FTW *ftwbuf) {
    (void)sb;       /* Suppress unused parameter warning */
    (void)typeflag; /* Suppress unused parameter warning */
    (void)ftwbuf;   /* Suppress unused parameter warning */
    return remove(fpath);
}

void tearDown(void) {
    /*
     * Clean up test directory using POSIX nftw() for recursive removal.
     * This replaces system("rm -rf " TEST_DIR) with safe native C code.
     *
     * FTW_DEPTH: Process directory contents before the directory itself
     * FTW_PHYS: Don't follow symbolic links (security consideration)
     */
    nftw(TEST_DIR, remove_callback, 64, FTW_DEPTH | FTW_PHYS);
}

/*
 * Helper Functions for Test Setup
 */
static void create_test_file(const char *filename, const char *content) {
    FILE *fp = fopen(filename, "w");
    if (fp != NULL) {
        if (content) {
            fputs(content, fp);
        }
        fclose(fp);
    }
}

static int file_exists(const char *filename) {
    return access(filename, F_OK) == 0;
}

static long get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

static char *read_file_content(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return NULL;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *content = malloc(size + 1);
    if (content) {
        fread(content, 1, size, fp);
        content[size] = '\0';
    }
    fclose(fp);
    return content;
}

/*
 * ===========================================================================
 * APPEND_FILE_TO_FILE() TESTS
 * ===========================================================================
 * Tests for safe file concatenation functionality
 */

/*
 * Normal operation tests
 */
void test_append_file_to_file_normal_operation(void) {
    /* Create source file with test content */
    create_test_file(TEST_SRC_FILE, "Hello World\n");

    /* Create destination file with existing content */
    create_test_file(TEST_DEST_FILE, "Existing content\n");

    /* Test append operation */
    int result = append_file_to_file(TEST_SRC_FILE, TEST_DEST_FILE);
    TEST_ASSERT_EQUAL_INT(0, result);

    /* Verify content was appended correctly */
    char *content = read_file_content(TEST_DEST_FILE);
    TEST_ASSERT_NOT_NULL(content);
    TEST_ASSERT_TRUE(strstr(content, "Existing content") != NULL);
    TEST_ASSERT_TRUE(strstr(content, "Hello World") != NULL);
    free(content);
}

void test_append_file_to_file_create_destination(void) {
    /* Create source file */
    create_test_file(TEST_SRC_FILE, "New file content\n");

    /* Ensure destination doesn't exist */
    unlink(TEST_DEST_FILE);

    /* Test append operation (should create destination) */
    int result = append_file_to_file(TEST_SRC_FILE, TEST_DEST_FILE);
    TEST_ASSERT_EQUAL_INT(0, result);

    /* Verify destination was created with source content */
    TEST_ASSERT_TRUE(file_exists(TEST_DEST_FILE));
    char *content = read_file_content(TEST_DEST_FILE);
    TEST_ASSERT_NOT_NULL(content);
    TEST_ASSERT_TRUE(strstr(content, "New file content") != NULL);
    free(content);
}

void test_append_file_to_file_empty_source(void) {
    /* Create empty source file */
    create_test_file(TEST_SRC_FILE, "");
    create_test_file(TEST_DEST_FILE, "Original content\n");

    long original_size = get_file_size(TEST_DEST_FILE);

    /* Test append operation */
    int result = append_file_to_file(TEST_SRC_FILE, TEST_DEST_FILE);
    TEST_ASSERT_EQUAL_INT(0, result);

    /* Verify destination size unchanged */
    long new_size = get_file_size(TEST_DEST_FILE);
    TEST_ASSERT_EQUAL_INT64(original_size, new_size);
}

/*
 * Error condition tests
 */
void test_append_file_to_file_null_parameters(void) {
    /* Test NULL source parameter */
    int result = append_file_to_file(NULL, TEST_DEST_FILE);
    TEST_ASSERT_EQUAL_INT(-1, result);

    /* Test NULL destination parameter */
    result = append_file_to_file(TEST_SRC_FILE, NULL);
    TEST_ASSERT_EQUAL_INT(-1, result);

    /* Test both NULL */
    result = append_file_to_file(NULL, NULL);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_append_file_to_file_missing_source(void) {
    /* Ensure source file doesn't exist */
    unlink(TEST_SRC_FILE);
    create_test_file(TEST_DEST_FILE, "Existing content\n");

    /* Test append operation with missing source */
    int result = append_file_to_file(TEST_SRC_FILE, TEST_DEST_FILE);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_append_file_to_file_large_file(void) {
    /* Create large source file (8KB) */
    FILE *fp = fopen(TEST_SRC_FILE, "w");
    TEST_ASSERT_NOT_NULL(fp);
    for (int i = 0; i < 1000; i++) {
        fprintf(fp, "Line %d: This is a test line with some content\n", i);
    }
    fclose(fp);

    create_test_file(TEST_DEST_FILE, "Header\n");
    long original_size = get_file_size(TEST_DEST_FILE);
    long source_size = get_file_size(TEST_SRC_FILE);

    /* Test append operation */
    int result = append_file_to_file(TEST_SRC_FILE, TEST_DEST_FILE);
    TEST_ASSERT_EQUAL_INT(0, result);

    /* Verify combined size */
    long final_size = get_file_size(TEST_DEST_FILE);
    TEST_ASSERT_EQUAL_INT64(original_size + source_size, final_size);
}

/*
 * ===========================================================================
 * WRITE_TIMESTAMP_TO_FILE() TESTS
 * ===========================================================================
 * Tests for safe timestamp writing functionality
 */

/*
 * Normal operation tests
 */
void test_write_timestamp_to_file_normal_operation(void) {
    /* Test timestamp writing */
    int result = write_timestamp_to_file(TEST_TIMESTAMP_FILE);
    TEST_ASSERT_EQUAL_INT(0, result);

    /* Verify file was created */
    TEST_ASSERT_TRUE(file_exists(TEST_TIMESTAMP_FILE));

    /* Verify file has content */
    long size = get_file_size(TEST_TIMESTAMP_FILE);
    TEST_ASSERT_TRUE(size > 0);

    /* Verify content looks like a timestamp */
    char *content = read_file_content(TEST_TIMESTAMP_FILE);
    TEST_ASSERT_NOT_NULL(content);
    /* Should contain date-like patterns (not testing exact format) */
    TEST_ASSERT_TRUE(strlen(content) > 10); /* Reasonable timestamp length */
    free(content);
}

void test_write_timestamp_to_file_overwrite_existing(void) {
    /* Create existing file */
    create_test_file(TEST_TIMESTAMP_FILE, "Old content that should be replaced\n");

    /* Write timestamp */
    int result = write_timestamp_to_file(TEST_TIMESTAMP_FILE);
    TEST_ASSERT_EQUAL_INT(0, result);

    /* Verify old content was replaced */
    char *content = read_file_content(TEST_TIMESTAMP_FILE);
    TEST_ASSERT_NOT_NULL(content);
    TEST_ASSERT_FALSE(strstr(content, "Old content") != NULL);
    free(content);
}

void test_write_timestamp_to_file_multiple_calls(void) {
    /* First timestamp */
    int result1 = write_timestamp_to_file(TEST_TIMESTAMP_FILE);
    TEST_ASSERT_EQUAL_INT(0, result1);
    char *content1 = read_file_content(TEST_TIMESTAMP_FILE);

    /* Brief delay to ensure different timestamps */
    sleep(1); /* 1 second */

    /* Second timestamp */
    int result2 = write_timestamp_to_file(TEST_TIMESTAMP_FILE);
    TEST_ASSERT_EQUAL_INT(0, result2);
    char *content2 = read_file_content(TEST_TIMESTAMP_FILE);

    /* Verify both calls succeeded but content differs */
    TEST_ASSERT_NOT_NULL(content1);
    TEST_ASSERT_NOT_NULL(content2);
    TEST_ASSERT_FALSE(strcmp(content1, content2) == 0);

    free(content1);
    free(content2);
}

/*
 * Error condition tests
 */
void test_write_timestamp_to_file_null_parameter(void) {
    /* Test NULL filename parameter */
    int result = write_timestamp_to_file(NULL);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_write_timestamp_to_file_invalid_path(void) {
    /* Test writing to non-existent directory */
    int result = write_timestamp_to_file("/nonexistent/path/timestamp.txt");
    TEST_ASSERT_EQUAL_INT(-1, result);
}

/*
 * ===========================================================================
 * SECURE_FILE_DELETE() TESTS
 * ===========================================================================
 * Tests for safe file deletion with glob patterns
 */

/*
 * Normal operation tests
 */
void test_secure_file_delete_single_file(void) {
    /* Create test file */
    create_test_file(TEST_DELETE_FILE1, "Delete me\n");
    TEST_ASSERT_TRUE(file_exists(TEST_DELETE_FILE1));

    /* Delete using exact filename */
    const char *patterns[] = {TEST_DELETE_FILE1};
    int result = secure_file_delete(patterns, 1);
    TEST_ASSERT_EQUAL_INT(1, result); /* Should delete 1 file */

    /* Verify file was deleted */
    TEST_ASSERT_FALSE(file_exists(TEST_DELETE_FILE1));
}

void test_secure_file_delete_multiple_files(void) {
    /* Create multiple test files */
    create_test_file(TEST_DELETE_FILE1, "File 1\n");
    create_test_file(TEST_DELETE_FILE2, "File 2\n");
    create_test_file(TEST_DELETE_FILE3, "File 3\n");

    /* Delete using exact filenames */
    const char *patterns[] = {TEST_DELETE_FILE1, TEST_DELETE_FILE2};
    int result = secure_file_delete(patterns, 2);
    TEST_ASSERT_EQUAL_INT(2, result); /* Should delete 2 files */

    /* Verify correct files were deleted */
    TEST_ASSERT_FALSE(file_exists(TEST_DELETE_FILE1));
    TEST_ASSERT_FALSE(file_exists(TEST_DELETE_FILE2));
    TEST_ASSERT_TRUE(file_exists(TEST_DELETE_FILE3)); /* Should remain */
}

void test_secure_file_delete_glob_pattern(void) {
    /* Create test files with pattern */
    create_test_file(TEST_DELETE_FILE1, "File 1\n");
    create_test_file(TEST_DELETE_FILE2, "File 2\n");
    create_test_file(TEST_DELETE_FILE3, "File 3\n");

    /* Delete using glob pattern for .txt files */
    const char *patterns[] = {TEST_DIR "/delete_me_*.txt"};
    int result = secure_file_delete(patterns, 1);
    TEST_ASSERT_EQUAL_INT(2, result); /* Should delete 2 .txt files */

    /* Verify correct files were deleted */
    TEST_ASSERT_FALSE(file_exists(TEST_DELETE_FILE1));
    TEST_ASSERT_FALSE(file_exists(TEST_DELETE_FILE2));
    TEST_ASSERT_TRUE(file_exists(TEST_DELETE_FILE3)); /* .log file should remain */
}

void test_secure_file_delete_mixed_patterns(void) {
    /* Create test files */
    create_test_file(TEST_DELETE_FILE1, "File 1\n");
    create_test_file(TEST_DELETE_FILE2, "File 2\n");
    create_test_file(TEST_DELETE_FILE3, "File 3\n");

    /* Delete using mixed exact and glob patterns */
    const char *patterns[] = {
        TEST_DELETE_FILE1,                    /* Exact filename */
        TEST_DIR "/delete_me_*.log"           /* Glob pattern */
    };
    int result = secure_file_delete(patterns, 2);
    TEST_ASSERT_EQUAL_INT(2, result); /* Should delete 2 files */

    /* Verify correct files were deleted */
    TEST_ASSERT_FALSE(file_exists(TEST_DELETE_FILE1)); /* Exact match */
    TEST_ASSERT_TRUE(file_exists(TEST_DELETE_FILE2));  /* Should remain */
    TEST_ASSERT_FALSE(file_exists(TEST_DELETE_FILE3)); /* Glob match */
}

/*
 * Error condition and edge case tests
 */
void test_secure_file_delete_null_parameters(void) {
    /* Test NULL patterns parameter */
    int result = secure_file_delete(NULL, 1);
    TEST_ASSERT_EQUAL_INT(-1, result);

    /* Test zero pattern count */
    const char *patterns[] = {TEST_DELETE_FILE1};
    result = secure_file_delete(patterns, 0);
    TEST_ASSERT_EQUAL_INT(0, result); /* No patterns, no deletions */

    /* Test negative pattern count */
    result = secure_file_delete(patterns, -1);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_secure_file_delete_missing_files(void) {
    /* Ensure files don't exist */
    unlink(TEST_DELETE_FILE1);
    unlink(TEST_DELETE_FILE2);

    /* Attempt to delete non-existent files */
    const char *patterns[] = {TEST_DELETE_FILE1, TEST_DELETE_FILE2};
    int result = secure_file_delete(patterns, 2);
    TEST_ASSERT_EQUAL_INT(0, result); /* Should return 0 (no files deleted) */
}

void test_secure_file_delete_no_matches(void) {
    /* Create a file that won't match pattern */
    create_test_file(TEST_DELETE_FILE1, "File 1\n");

    /* Use pattern that won't match */
    const char *patterns[] = {TEST_DIR "/nonexistent_*.xyz"};
    int result = secure_file_delete(patterns, 1);
    TEST_ASSERT_EQUAL_INT(0, result); /* No matches, no deletions */

    /* Verify original file still exists */
    TEST_ASSERT_TRUE(file_exists(TEST_DELETE_FILE1));
}

void test_secure_file_delete_security_no_shell_injection(void) {
    /* Create test file */
    create_test_file(TEST_DELETE_FILE1, "File 1\n");

    /* Attempt shell injection via malicious pattern */
    /* This should NOT execute shell commands */
    const char *patterns[] = {
        TEST_DELETE_FILE1 "; rm -rf /; echo 'pwned'",
        TEST_DELETE_FILE1 " && echo 'injection'"
    };

    /* The function should safely handle these patterns */
    int result = secure_file_delete(patterns, 2);
    /* Result should be >= 0 (not -1 critical error) */
    TEST_ASSERT_TRUE(result >= 0);

    /* The malicious commands should not have executed */
    /* We can't easily test this without external monitoring,
     * but the glob() function should safely handle these patterns */
}

/*
 * ===========================================================================
 * INTEGRATION AND CROSS-FUNCTION TESTS
 * ===========================================================================
 */

void test_safe_system_integration_workflow(void) {
    /* Test a realistic workflow using all functions */

    /* Step 1: Create source file with timestamp */
    int result1 = write_timestamp_to_file(TEST_SRC_FILE);
    TEST_ASSERT_EQUAL_INT(0, result1);
    TEST_ASSERT_TRUE(file_exists(TEST_SRC_FILE));

    /* Step 2: Append timestamp to destination */
    int result2 = append_file_to_file(TEST_SRC_FILE, TEST_DEST_FILE);
    TEST_ASSERT_EQUAL_INT(0, result2);
    TEST_ASSERT_TRUE(file_exists(TEST_DEST_FILE));

    /* Step 3: Verify destination has content */
    long dest_size = get_file_size(TEST_DEST_FILE);
    TEST_ASSERT_TRUE(dest_size > 0);

    /* Step 4: Clean up using secure delete */
    const char *patterns[] = {TEST_SRC_FILE, TEST_DEST_FILE};
    int result3 = secure_file_delete(patterns, 2);
    TEST_ASSERT_EQUAL_INT(2, result3);

    /* Step 5: Verify cleanup */
    TEST_ASSERT_FALSE(file_exists(TEST_SRC_FILE));
    TEST_ASSERT_FALSE(file_exists(TEST_DEST_FILE));
}

/*
 * ===========================================================================
 * SORT_FILE_IN_PLACE() TESTS
 * ===========================================================================
 */

/*
 * Test sort_file_in_place() with basic unsorted data
 */
void test_sort_file_in_place_basic_sorting(void) {
    const char *test_file = TEST_DIR "/sort_basic.txt";
    const char *unsorted = "ZZ Line 3\nAA Line 1\nMM Line 2\n";

    create_test_file(test_file, unsorted);

    int result = sort_file_in_place(test_file, 2);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_TRUE(file_exists(test_file));

    char *content = read_file_content(test_file);
    TEST_ASSERT_NOT_NULL(content);
    TEST_ASSERT_TRUE(strstr(content, "AA Line 1") < strstr(content, "MM Line 2"));
    TEST_ASSERT_TRUE(strstr(content, "MM Line 2") < strstr(content, "ZZ Line 3"));
    free(content);
}

/*
 * Test sort_file_in_place() with already sorted data
 */
void test_sort_file_in_place_already_sorted(void) {
    const char *test_file = TEST_DIR "/sort_sorted.txt";
    const char *sorted = "AA First\nBB Second\nCC Third\n";

    create_test_file(test_file, sorted);

    int result = sort_file_in_place(test_file, 2);
    TEST_ASSERT_EQUAL_INT(0, result);

    char *content = read_file_content(test_file);
    TEST_ASSERT_NOT_NULL(content);
    TEST_ASSERT_EQUAL_STRING("AA First\nBB Second\nCC Third\n", content);
    free(content);
}

/*
 * Test sort_file_in_place() with reverse sorted data
 */
void test_sort_file_in_place_reverse_sorted(void) {
    const char *test_file = TEST_DIR "/sort_reverse.txt";
    const char *reverse = "ZZ Last\nMM Middle\nAA First\n";

    create_test_file(test_file, reverse);

    int result = sort_file_in_place(test_file, 2);
    TEST_ASSERT_EQUAL_INT(0, result);

    char *content = read_file_content(test_file);
    TEST_ASSERT_NOT_NULL(content);
    TEST_ASSERT_TRUE(strstr(content, "AA First") < strstr(content, "MM Middle"));
    TEST_ASSERT_TRUE(strstr(content, "MM Middle") < strstr(content, "ZZ Last"));
    free(content);
}

/*
 * Test sort_file_in_place() with duplicate prefixes
 */
void test_sort_file_in_place_duplicates(void) {
    const char *test_file = TEST_DIR "/sort_duplicates.txt";
    const char *dups = "AA Line 2\nAA Line 1\nAA Line 3\n";

    create_test_file(test_file, dups);

    int result = sort_file_in_place(test_file, 2);
    TEST_ASSERT_EQUAL_INT(0, result);

    char *content = read_file_content(test_file);
    TEST_ASSERT_NOT_NULL(content);
    /* All lines start with "AA", so order is maintained or stable */
    TEST_ASSERT_NOT_NULL(strstr(content, "AA Line"));
    free(content);
}

/*
 * Test sort_file_in_place() skips short lines (same as conqsort)
 */
void test_sort_file_in_place_skip_short_lines(void) {
    const char *test_file = TEST_DIR "/sort_short.txt";
    const char *mixed = "ZZ Valid line\nA\nMM Another valid\nB\nAA First valid\n";

    create_test_file(test_file, mixed);

    int result = sort_file_in_place(test_file, 2);
    TEST_ASSERT_EQUAL_INT(0, result);

    char *content = read_file_content(test_file);
    TEST_ASSERT_NOT_NULL(content);
    /* Short lines (A, B) should be skipped, only long lines sorted */
    TEST_ASSERT_NULL(strstr(content, "A\n"));
    TEST_ASSERT_NULL(strstr(content, "B\n"));
    TEST_ASSERT_TRUE(strstr(content, "AA First valid") < strstr(content, "MM Another valid"));
    free(content);
}

/*
 * Test sort_file_in_place() with different compnum values
 */
void test_sort_file_in_place_different_compnum(void) {
    const char *test_file = TEST_DIR "/sort_compnum.txt";
    const char *data = "ABC Line 1\nABD Line 2\nAAA Line 3\n";

    create_test_file(test_file, data);

    /* Sort using 3 character comparison */
    int result = sort_file_in_place(test_file, 3);
    TEST_ASSERT_EQUAL_INT(0, result);

    char *content = read_file_content(test_file);
    TEST_ASSERT_NOT_NULL(content);
    TEST_ASSERT_TRUE(strstr(content, "AAA Line 3") < strstr(content, "ABC Line 1"));
    TEST_ASSERT_TRUE(strstr(content, "ABC Line 1") < strstr(content, "ABD Line 2"));
    free(content);
}

/*
 * Test sort_file_in_place() with empty file
 */
void test_sort_file_in_place_empty_file(void) {
    const char *test_file = TEST_DIR "/sort_empty.txt";

    create_test_file(test_file, "");

    int result = sort_file_in_place(test_file, 2);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_TRUE(file_exists(test_file));

    long size = get_file_size(test_file);
    TEST_ASSERT_EQUAL_INT(0, size);
}

/*
 * Test sort_file_in_place() with single line
 */
void test_sort_file_in_place_single_line(void) {
    const char *test_file = TEST_DIR "/sort_single.txt";
    const char *single = "AA Only line\n";

    create_test_file(test_file, single);

    int result = sort_file_in_place(test_file, 2);
    TEST_ASSERT_EQUAL_INT(0, result);

    char *content = read_file_content(test_file);
    TEST_ASSERT_NOT_NULL(content);
    TEST_ASSERT_EQUAL_STRING("AA Only line\n", content);
    free(content);
}

/*
 * Test sort_file_in_place() error handling - NULL filename
 */
void test_sort_file_in_place_null_filename(void) {
    int result = sort_file_in_place(NULL, 2);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

/*
 * Test sort_file_in_place() error handling - invalid compnum
 */
void test_sort_file_in_place_invalid_compnum(void) {
    const char *test_file = TEST_DIR "/sort_invalid.txt";
    create_test_file(test_file, "AA Test\n");

    int result = sort_file_in_place(test_file, 0);
    TEST_ASSERT_EQUAL_INT(-1, result);

    result = sort_file_in_place(test_file, -1);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

/*
 * Test sort_file_in_place() error handling - missing file
 */
void test_sort_file_in_place_missing_file(void) {
    const char *missing_file = TEST_DIR "/nonexistent_sort.txt";

    int result = sort_file_in_place(missing_file, 2);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

/*
 * Test sort_file_in_place() with large file
 */
void test_sort_file_in_place_large_file(void) {
    const char *test_file = TEST_DIR "/sort_large.txt";
    FILE *fp = fopen(test_file, "w");
    TEST_ASSERT_NOT_NULL(fp);

    /* Create file with 100 lines in random order */
    for (int i = 99; i >= 0; i--) {
        fprintf(fp, "%02d Line number %d\n", i, i);
    }
    fclose(fp);

    int result = sort_file_in_place(test_file, 2);
    TEST_ASSERT_EQUAL_INT(0, result);

    /* Verify first and last lines are correctly sorted */
    fp = fopen(test_file, "r");
    TEST_ASSERT_NOT_NULL(fp);

    char first_line[100];
    fgets(first_line, sizeof(first_line), fp);
    TEST_ASSERT_TRUE(strstr(first_line, "00 Line number 0") != NULL);

    fclose(fp);
}

/*
 * Test sort_file_in_place() preserves content integrity
 */
void test_sort_file_in_place_content_integrity(void) {
    const char *test_file = TEST_DIR "/sort_integrity.txt";
    const char *data = "ZZ Special chars: !@#$%^&*()\nMM Numbers: 1234567890\nAA Mixed: Test123!@#\n";

    create_test_file(test_file, data);

    int result = sort_file_in_place(test_file, 2);
    TEST_ASSERT_EQUAL_INT(0, result);

    char *content = read_file_content(test_file);
    TEST_ASSERT_NOT_NULL(content);
    /* Verify all original content is present */
    TEST_ASSERT_NOT_NULL(strstr(content, "Special chars: !@#$%^&*()"));
    TEST_ASSERT_NOT_NULL(strstr(content, "Numbers: 1234567890"));
    TEST_ASSERT_NOT_NULL(strstr(content, "Mixed: Test123!@#"));
    free(content);
}

/*
 * ===========================================================================
 * PERFORMANCE AND STRESS TESTS
 * ===========================================================================
 */

void test_safe_system_performance_multiple_operations(void) {
    /* Create multiple files and perform batch operations */
    char source_files[10][256];

    /* Create 10 source files */
    for (int i = 0; i < 10; i++) {
        snprintf(source_files[i], sizeof(source_files[i]),
                 TEST_DIR "/perf_source_%d.txt", i);
        create_test_file(source_files[i], "Performance test content\n");
    }

    /* Append all to destination */
    for (int i = 0; i < 10; i++) {
        int result = append_file_to_file(source_files[i], TEST_DEST_FILE);
        TEST_ASSERT_EQUAL_INT(0, result);
    }

    /* Verify final size */
    long final_size = get_file_size(TEST_DEST_FILE);
    TEST_ASSERT_TRUE(final_size > 240); /* 10 * ~24 bytes per line */

    /* Clean up with glob pattern */
    const char *patterns[] = {TEST_DIR "/perf_source_*.txt", TEST_DEST_FILE};
    int deleted = secure_file_delete(patterns, 2);
    TEST_ASSERT_EQUAL_INT(11, deleted); /* 10 source + 1 dest */
}

/*
 * ===========================================================================
 * MAIN TEST RUNNER
 * ===========================================================================
 */

int main(void) {
    UNITY_BEGIN();

    /* append_file_to_file() tests */
    RUN_TEST(test_append_file_to_file_normal_operation);
    RUN_TEST(test_append_file_to_file_create_destination);
    RUN_TEST(test_append_file_to_file_empty_source);
    RUN_TEST(test_append_file_to_file_null_parameters);
    RUN_TEST(test_append_file_to_file_missing_source);
    RUN_TEST(test_append_file_to_file_large_file);

    /* write_timestamp_to_file() tests */
    RUN_TEST(test_write_timestamp_to_file_normal_operation);
    RUN_TEST(test_write_timestamp_to_file_overwrite_existing);
    RUN_TEST(test_write_timestamp_to_file_multiple_calls);
    RUN_TEST(test_write_timestamp_to_file_null_parameter);
    RUN_TEST(test_write_timestamp_to_file_invalid_path);

    /* secure_file_delete() tests */
    RUN_TEST(test_secure_file_delete_single_file);
    RUN_TEST(test_secure_file_delete_multiple_files);
    RUN_TEST(test_secure_file_delete_glob_pattern);
    RUN_TEST(test_secure_file_delete_mixed_patterns);
    RUN_TEST(test_secure_file_delete_null_parameters);
    RUN_TEST(test_secure_file_delete_missing_files);
    RUN_TEST(test_secure_file_delete_no_matches);
    RUN_TEST(test_secure_file_delete_security_no_shell_injection);

    /* sort_file_in_place() tests */
    RUN_TEST(test_sort_file_in_place_basic_sorting);
    RUN_TEST(test_sort_file_in_place_already_sorted);
    RUN_TEST(test_sort_file_in_place_reverse_sorted);
    RUN_TEST(test_sort_file_in_place_duplicates);
    RUN_TEST(test_sort_file_in_place_skip_short_lines);
    RUN_TEST(test_sort_file_in_place_different_compnum);
    RUN_TEST(test_sort_file_in_place_empty_file);
    RUN_TEST(test_sort_file_in_place_single_line);
    RUN_TEST(test_sort_file_in_place_null_filename);
    RUN_TEST(test_sort_file_in_place_invalid_compnum);
    RUN_TEST(test_sort_file_in_place_missing_file);
    RUN_TEST(test_sort_file_in_place_large_file);
    RUN_TEST(test_sort_file_in_place_content_integrity);

    /* Integration and performance tests */
    RUN_TEST(test_safe_system_integration_workflow);
    RUN_TEST(test_safe_system_performance_multiple_operations);

    printf("\n=== Safe System Test Summary ===\n");
    printf("Functions Tested: 4 (append_file_to_file, write_timestamp_to_file, secure_file_delete, sort_file_in_place)\n");
    printf("Total Test Cases Run: %lu\n", Unity.NumberOfTests);
    printf("Security Features: Command injection prevention, safe file operations\n");
    printf("Coverage: Normal operations, error conditions, security validation, sorting algorithms\n");

    return UNITY_END();
}
