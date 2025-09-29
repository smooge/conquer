/*
 * mock_infrastructure.h - Mock Infrastructure Templates for Legacy C Testing
 *
 * This file provides templates and utilities for creating mocks when testing
 * legacy C functions that have global dependencies. These templates are
 * designed for Level 1 testing (functions requiring mocking) in Phase 6.5+.
 *
 * Usage Guidelines:
 * 1. Use these templates for functions that can't be tested in isolation
 * 2. Adapt templates to specific global state requirements
 * 3. Focus on minimal viable mocks rather than complete system simulation
 * 4. Document what aspects of real behavior are NOT mocked
 *
 * Categories Supported:
 * - Nation data mocking (for functions using nation arrays)
 * - World data mocking (for functions using world state)
 * - File I/O mocking (for functions with file dependencies)
 * - Display/curses mocking (for UI functions)
 */

#ifndef MOCK_INFRASTRUCTURE_H
#define MOCK_INFRASTRUCTURE_H

#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* =============================================================================
 * Nation Data Mocking Templates
 * =============================================================================
 */

/*
 * Mock Nation Structure
 * Simplified version of the real nation structure for testing
 * Only includes fields commonly used by testable functions
 */
typedef struct {
    char name[16];        /* Nation name */
    int active;           /* Is nation active */
    int power;            /* Nation power level */
    int cash;             /* Nation treasury */
    int x, y;             /* Nation coordinates */
} mock_nation_t;

/*
 * Nation Mock Infrastructure
 * Provides minimal nation data for testing functions that require nation arrays
 */
typedef struct {
    mock_nation_t nations[64];   /* Array of mock nations */
    int nation_count;            /* Number of active nations */
    int current_nation;          /* Currently selected nation */
} nation_mock_t;

/*
 * nation_mock_init - Initialize nation mock infrastructure
 *
 * Sets up minimal nation data for testing. Creates a few test nations
 * with predictable data that tests can rely on.
 */
static inline void nation_mock_init(nation_mock_t *mock) {
    memset(mock, 0, sizeof(nation_mock_t));

    /* Create test nation 0 */
    strcpy(mock->nations[0].name, "TestNation0");
    mock->nations[0].active = 1;
    mock->nations[0].power = 100;
    mock->nations[0].cash = 1000;
    mock->nations[0].x = 10;
    mock->nations[0].y = 20;

    /* Create test nation 1 */
    strcpy(mock->nations[1].name, "TestNation1");
    mock->nations[1].active = 1;
    mock->nations[1].power = 200;
    mock->nations[1].cash = 2000;
    mock->nations[1].x = 30;
    mock->nations[1].y = 40;

    mock->nation_count = 2;
    mock->current_nation = 0;
}

/*
 * nation_mock_cleanup - Clean up nation mock infrastructure
 */
static inline void nation_mock_cleanup(nation_mock_t *mock) {
    /* No dynamic allocation to clean up in basic template */
    (void)mock; /* Suppress unused parameter warning */
}

/* =============================================================================
 * World Data Mocking Templates
 * =============================================================================
 */

/*
 * Mock World Structure
 * Simplified world state for testing functions that depend on game world
 */
typedef struct {
    int world_size_x;     /* World width */
    int world_size_y;     /* World height */
    int turn_number;      /* Current game turn */
    int season;           /* Current season */
    char *world_data;     /* Simple world map data */
} world_mock_t;

/*
 * world_mock_init - Initialize world mock infrastructure
 */
static inline void world_mock_init(world_mock_t *mock, int size_x, int size_y) {
    mock->world_size_x = size_x;
    mock->world_size_y = size_y;
    mock->turn_number = 1;
    mock->season = 0;

    /* Allocate simple world data */
    mock->world_data = calloc((size_t)(size_x * size_y), sizeof(char));
    if (mock->world_data) {
        /* Initialize with basic terrain */
        memset(mock->world_data, '.', (size_t)(size_x * size_y));
    }
}

/*
 * world_mock_cleanup - Clean up world mock infrastructure
 */
static inline void world_mock_cleanup(world_mock_t *mock) {
    if (mock->world_data) {
        free(mock->world_data);
        mock->world_data = NULL;
    }
}

/* =============================================================================
 * File I/O Mocking Templates
 * =============================================================================
 */

/*
 * Mock File System
 * Simple file content simulation for testing file-dependent functions
 */
typedef struct {
    char content[1024];   /* Mock file content */
    size_t size;          /* Content size */
    size_t position;      /* Read position */
    int is_open;          /* File state */
} mock_file_t;

/*
 * file_mock_init - Initialize file mock with content
 */
static inline void file_mock_init(mock_file_t *mock, const char *content) {
    if (content) {
        strncpy(mock->content, content, sizeof(mock->content) - 1);
        mock->content[sizeof(mock->content) - 1] = '\0';
        mock->size = strlen(mock->content);
    } else {
        mock->content[0] = '\0';
        mock->size = 0;
    }
    mock->position = 0;
    mock->is_open = 1;
}

/*
 * file_mock_read_line - Simulate reading a line from mock file
 */
static inline char *file_mock_read_line(mock_file_t *mock, char *buffer, size_t buffer_size) {
    if (!mock->is_open || mock->position >= mock->size) {
        return NULL;
    }

    size_t line_length = 0;
    while (mock->position + line_length < mock->size &&
           mock->content[mock->position + line_length] != '\n' &&
           line_length < buffer_size - 1) {
        line_length++;
    }

    if (line_length > 0) {
        memcpy(buffer, &mock->content[mock->position], line_length);
        buffer[line_length] = '\0';
        mock->position += line_length;

        /* Skip newline if present */
        if (mock->position < mock->size && mock->content[mock->position] == '\n') {
            mock->position++;
        }

        return buffer;
    }

    return NULL;
}

/* =============================================================================
 * Display/UI Mocking Templates
 * =============================================================================
 */

/*
 * Mock Display State
 * Simple display simulation for testing UI-dependent functions
 */
typedef struct {
    int screen_width;     /* Mock screen width */
    int screen_height;    /* Mock screen height */
    int cursor_x;         /* Mock cursor position */
    int cursor_y;
    char last_output[256]; /* Last output message */
} display_mock_t;

/*
 * display_mock_init - Initialize display mock
 */
static inline void display_mock_init(display_mock_t *mock) {
    mock->screen_width = 80;
    mock->screen_height = 24;
    mock->cursor_x = 0;
    mock->cursor_y = 0;
    mock->last_output[0] = '\0';
}

/*
 * display_mock_output - Simulate display output
 */
static inline void display_mock_output(display_mock_t *mock, const char *message) {
    if (message) {
        strncpy(mock->last_output, message, sizeof(mock->last_output) - 1);
        mock->last_output[sizeof(mock->last_output) - 1] = '\0';
    }
}

/* =============================================================================
 * Test Utility Macros
 * =============================================================================
 */

/*
 * Mock setup and teardown macros for Unity tests
 * Use these in your test files for consistent mock management
 */
#define DECLARE_MOCK_SETUP(mock_type, mock_var) \
    static mock_type mock_var; \
    void setUp(void) { \
        memset(&mock_var, 0, sizeof(mock_var)); \
    } \
    void tearDown(void) { \
        /* Cleanup handled by test-specific teardown */ \
    }

/*
 * Assertion macros for mock testing
 */
#define TEST_ASSERT_MOCK_NATION_VALID(mock, nation_id) \
    TEST_ASSERT_TRUE((nation_id) >= 0 && (nation_id) < (mock)->nation_count); \
    TEST_ASSERT_TRUE((mock)->nations[nation_id].active)

#define TEST_ASSERT_MOCK_WORLD_COORDS_VALID(mock, x, y) \
    TEST_ASSERT_TRUE((x) >= 0 && (x) < (mock)->world_size_x); \
    TEST_ASSERT_TRUE((y) >= 0 && (y) < (mock)->world_size_y)

/* =============================================================================
 * Usage Examples and Documentation
 * =============================================================================
 */

/*
 * EXAMPLE USAGE:
 *
 * // In your test file:
 * #include "mock_infrastructure.h"
 *
 * static nation_mock_t nation_mock;
 *
 * void setUp(void) {
 *     nation_mock_init(&nation_mock);
 * }
 *
 * void tearDown(void) {
 *     nation_mock_cleanup(&nation_mock);
 * }
 *
 * void test_function_needing_nations(void) {
 *     // Use mock.nations[0] and mock.nations[1] in your test
 *     int result = function_under_test(&nation_mock.nations[0]);
 *     TEST_ASSERT_EQUAL_INT(expected_value, result);
 * }
 *
 * GUIDELINES:
 * - Keep mocks simple and focused on what the function actually needs
 * - Don't try to simulate complete game state
 * - Document what aspects of real behavior are NOT mocked
 * - Consider whether the function should be refactored instead of mocked
 */

#endif /* MOCK_INFRASTRUCTURE_H */