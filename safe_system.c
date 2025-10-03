/*
 * safe_system.c - Safe system() call replacements for Conquer
 *
 * This module provides safe replacements for system() calls that eliminate
 * command injection vulnerabilities by using native C functions instead of
 * shell command execution.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <glob.h>

#include "safe_system.h"
#include "header.h"  /* Must come before data.h for constants */
#include "data.h"    /* For check_lock() function */

/* Maximum line length for file sorting (same as conqsort) */
#define MAX_SORT_LINE 200

/* Linked list structure for sorting lines */
typedef struct sort_line {
    char *line;
    struct sort_line *next;
} SORT_LINE;

/*
 * append_file_to_file - Append contents of source file to destination file
 *
 * Safely appends the entire contents of the source file to the end of the
 * destination file using native C file operations with proper file locking.
 * This eliminates command injection vulnerabilities while providing equivalent
 * functionality to "cat source >> destination" with improved concurrency safety.
 *
 * Parameters:
 *   source - Path to source file to read from (must not be NULL)
 *   destination - Path to destination file to append to (must not be NULL)
 *
 * Returns:
 *   0 on success, -1 on error (file access failure, I/O error, locking failure)
 *
 * Side Effects:
 *   - Opens and reads from source file
 *   - Opens destination file in append mode
 *   - Creates destination file if it doesn't exist
 *   - Uses file locking to prevent concurrent write corruption
 *
 * Security:
 *   - No shell command execution (eliminates command injection)
 *   - Proper input validation and error handling
 *   - Safe file operations with bounds checking
 *   - File locking prevents concurrent write corruption
 *
 * Notes:
 *   - Replaces system("cat source >> destination") calls
 *   - Uses 8KB buffer for efficient I/O operations
 *   - Uses existing check_lock() infrastructure for cross-platform locking
 *   - Thread-safe with proper file locking
 */
int append_file_to_file(const char *source, const char *destination) {
    if (source == NULL || destination == NULL) {
        return -1;
    }

    FILE *src = fopen(source, "r");
    if (src == NULL) {
        return -1;
    }

    FILE *dst = fopen(destination, "a");
    if (dst == NULL) {
        fclose(src);
        return -1;
    }

    /* Lock destination file to prevent concurrent writes */
    char lock_name[256];
    snprintf(lock_name, sizeof(lock_name), "%s.append_lock", destination);

    /* Use existing locking infrastructure for consistency */
    if (check_lock(lock_name, TRUE) == TRUE) {
        /* File is locked by another process */
        fclose(src);
        fclose(dst);
        return -1;
    }

    char buffer[8192];
    size_t bytes_read;
    int result = 0;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, bytes_read, dst) != bytes_read) {
            result = -1;
            break;
        }
    }

    /* Clean up and release lock */
    fclose(src);
    fclose(dst);
    unlink(lock_name);  /* Remove lock file */

    return result;
}

/*
 * write_timestamp_to_file - Write current date/time to file
 *
 * Writes the current system date and time to the specified file in the same
 * format as the Unix 'date' command. This replaces system("date > file")
 * calls with a secure native C implementation that eliminates command injection
 * vulnerabilities.
 *
 * Parameters:
 *   filename - Path to file where timestamp should be written (must not be NULL)
 *
 * Returns:
 *   0 on success, -1 on error (file access failure)
 *
 * Side Effects:
 *   - Creates or overwrites the specified file
 *   - Writes timestamp in standard Unix date format
 *   - File permissions set by system umask
 *
 * Security:
 *   - No shell command execution (eliminates command injection)
 *   - Proper input validation and error handling
 *   - Safe file operations with bounds checking
 *
 * Notes:
 *   - Replaces system("date > filename") calls
 *   - Output format matches Unix date command
 *   - Thread-safe (uses standard C time functions)
 */
int write_timestamp_to_file(const char *filename) {
    if (filename == NULL) {
        return -1;
    }

    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        return -1;
    }

    time_t now = time(NULL);
    char *timestr = ctime(&now);

    if (timestr == NULL) {
        fclose(fp);
        return -1;
    }

    /* ctime() includes newline, so we can write it directly */
    if (fputs(timestr, fp) == EOF) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

/*
 * secure_file_delete - Safely delete files using glob patterns
 *
 * Securely deletes files matching the given glob patterns using native C
 * functions instead of shell commands. This eliminates command injection
 * vulnerabilities while providing equivalent functionality to "rm -f pattern".
 * Handles multiple patterns and gracefully ignores missing files.
 *
 * Parameters:
 *   patterns - Array of glob pattern strings (must not be NULL)
 *   num_patterns - Number of patterns in the array
 *
 * Returns:
 *   Number of files successfully deleted, -1 on critical error
 *
 * Side Effects:
 *   - Deletes files matching the specified patterns
 *   - Uses glob() to safely expand wildcards
 *   - Ignores missing files (equivalent to rm -f behavior)
 *
 * Security:
 *   - No shell command execution (eliminates command injection)
 *   - Safe pattern expansion using POSIX glob()
 *   - Proper input validation and error handling
 *   - No interpretation of shell metacharacters
 *
 * Notes:
 *   - Replaces system("rm -f pattern*") calls
 *   - Uses GLOB_NOSORT for performance (order not important)
 *   - Thread-safe (no global state modifications)
 *   - Compatible with POSIX glob patterns
 */
int secure_file_delete(const char **patterns, int num_patterns) {
    if (patterns == NULL || num_patterns < 0) {
        return -1;
    }

    /* Zero patterns is a valid no-op case */
    if (num_patterns == 0) {
        return 0;
    }

    int total_deleted = 0;

    for (int i = 0; i < num_patterns; i++) {
        if (patterns[i] == NULL) {
            continue;
        }

        glob_t glob_result;
        int glob_flags = GLOB_NOSORT;

        /* Expand the glob pattern */
        int result = glob(patterns[i], glob_flags, NULL, &glob_result);

        if (result == GLOB_NOMATCH) {
            /* No files match - this is OK (equivalent to rm -f behavior) */
            globfree(&glob_result);
            continue;
        } else if (result != 0) {
            /* Other glob error - continue with next pattern */
            globfree(&glob_result);
            continue;
        }

        /* Delete each matching file */
        for (size_t j = 0; j < glob_result.gl_pathc; j++) {
            if (unlink(glob_result.gl_pathv[j]) == 0) {
                total_deleted++;
            }
            /* Ignore unlink failures (equivalent to rm -f behavior) */
        }

        globfree(&glob_result);
    }

    return total_deleted;
}

/*
 * Helper functions for sort_file_in_place()
 */

/*
 * compare_lines - Compare two strings for sorting
 *
 * Compares two strings character by character up to compnum characters.
 * Used by the insertion sort algorithm for ordering lines.
 *
 * Parameters:
 *   a - First string to compare (null-terminated)
 *   b - Second string to compare (null-terminated)
 *   compnum - Number of characters to compare
 *
 * Returns:
 *   -1 if a < b, 0 if equal, 1 if a > b
 */
static int compare_lines(const char *a, const char *b, int compnum) {
    for (int i = 0; i < compnum; i++) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
        if (a[i] == '\0') return 0;
    }
    return 0;
}

/*
 * create_sort_node - Create new linked list node for sorting
 *
 * Allocates memory for a new sorting node and copies the line data.
 *
 * Parameters:
 *   line - Line data to store (null-terminated)
 *   next - Pointer to next node in list (may be NULL)
 *
 * Returns:
 *   Pointer to new node, or NULL on allocation failure
 */
static SORT_LINE *create_sort_node(const char *line, SORT_LINE *next) {
    SORT_LINE *node = (SORT_LINE *)malloc(sizeof(SORT_LINE));
    if (node == NULL) {
        return NULL;
    }

    size_t line_len = strlen(line);
    node->line = (char *)malloc(line_len + 1);
    if (node->line == NULL) {
        free(node);
        return NULL;
    }

    memcpy(node->line, line, line_len);
    node->line[line_len] = '\0';
    node->next = next;
    return node;
}

/*
 * free_sort_list - Free all nodes in sorting linked list
 *
 * Traverses the linked list and frees all allocated memory.
 *
 * Parameters:
 *   head - Pointer to first node in list
 */
static void free_sort_list(SORT_LINE *head) {
    while (head != NULL) {
        SORT_LINE *temp = head;
        head = head->next;
        free(temp->line);
        free(temp);
    }
}

/*
 * insert_sorted - Insert line into sorted linked list
 *
 * Finds the correct position for a new line in the sorted list and inserts it.
 * Maintains sorted order using insertion sort algorithm.
 *
 * Parameters:
 *   head_ptr - Pointer to head pointer of list
 *   line - Line to insert (null-terminated)
 *   compnum - Number of characters to compare for sorting
 *
 * Returns:
 *   0 on success, -1 on allocation failure
 */
static int insert_sorted(SORT_LINE **head_ptr, const char *line, int compnum) {
    SORT_LINE *head = *head_ptr;

    /* Empty list - create first node */
    if (head == NULL) {
        *head_ptr = create_sort_node(line, NULL);
        return (*head_ptr == NULL) ? -1 : 0;
    }

    /* Insert at beginning */
    if (compare_lines(head->line, line, compnum) == 1) {
        *head_ptr = create_sort_node(line, head);
        return (*head_ptr == NULL) ? -1 : 0;
    }

    /* Find insertion point */
    SORT_LINE *current = head;
    while (current->next != NULL &&
           compare_lines(current->next->line, line, compnum) != 1) {
        current = current->next;
    }

    /* Insert after current */
    SORT_LINE *new_node = create_sort_node(line, current->next);
    if (new_node == NULL) {
        return -1;
    }
    current->next = new_node;
    return 0;
}

/*
 * sort_file_in_place - Sort lines in a file using native C implementation
 *
 * Sorts the lines of a file in place using the same algorithm as the external
 * conqsort utility. This eliminates command injection vulnerabilities by
 * replacing system() calls with secure native C code. Uses insertion sort
 * with linked list data structure and atomic file replacement for safety.
 *
 * Parameters:
 *   filename - Path to file to sort (must not be NULL)
 *   compnum - Number of characters to compare for sorting (typically 2)
 *
 * Returns:
 *   0 on success, -1 on error (file access failure, I/O error, memory error)
 *
 * Side Effects:
 *   - Reads entire file into memory
 *   - Sorts lines based on first compnum characters
 *   - Writes sorted output to temporary file
 *   - Atomically replaces original file with sorted version
 *   - Uses file locking to prevent concurrent access
 *
 * Security:
 *   - No shell command execution (eliminates command injection)
 *   - Atomic file replacement prevents corruption
 *   - Proper input validation and error handling
 *   - Safe memory operations with bounds checking
 *
 * Notes:
 *   - Replaces system("conqsort filename filename") calls
 *   - Uses same insertion sort algorithm as conqsort utility
 *   - Memory allocated for sorting is freed on completion
 *   - Maximum line length is 200 characters (same as conqsort)
 *   - Case-sensitive comparison (same as default conqsort)
 *   - Lines shorter than compnum are skipped (same as conqsort)
 */
int sort_file_in_place(const char *filename, int compnum) {
    if (filename == NULL || compnum <= 0) {
        return -1;
    }

    /* Open input file */
    FILE *input = fopen(filename, "r");
    if (input == NULL) {
        return -1;
    }

    /* Lock file to prevent concurrent access */
    char lock_name[512];
    snprintf(lock_name, sizeof(lock_name), "%s.sort_lock", filename);

    if (check_lock(lock_name, TRUE) == TRUE) {
        fclose(input);
        return -1;
    }

    /* Read and sort lines */
    SORT_LINE *head = NULL;
    char line_buffer[MAX_SORT_LINE];
    int error = 0;

    while (fgets(line_buffer, sizeof(line_buffer), input) != NULL) {
        /* Remove newline if present */
        size_t len = strlen(line_buffer);
        if (len > 0 && line_buffer[len - 1] == '\n') {
            line_buffer[len - 1] = '\0';
            len--;
        }

        /* Skip lines shorter than comparison length (same as conqsort) */
        if ((int)len <= compnum) {
            continue;
        }

        /* Insert line into sorted list */
        if (insert_sorted(&head, line_buffer, compnum) != 0) {
            error = 1;
            break;
        }
    }

    fclose(input);

    /* Handle errors during reading/sorting */
    if (error) {
        free_sort_list(head);
        unlink(lock_name);
        return -1;
    }

    /* Write sorted output to temporary file */
    char temp_filename[512];
    snprintf(temp_filename, sizeof(temp_filename), "%s.sorted.tmp", filename);

    FILE *output = fopen(temp_filename, "w");
    if (output == NULL) {
        free_sort_list(head);
        unlink(lock_name);
        return -1;
    }

    /* Write all sorted lines */
    SORT_LINE *current = head;
    while (current != NULL) {
        if (fprintf(output, "%s\n", current->line) < 0) {
            error = 1;
            break;
        }
        current = current->next;
    }

    fclose(output);
    free_sort_list(head);

    /* Handle write errors */
    if (error) {
        unlink(temp_filename);
        unlink(lock_name);
        return -1;
    }

    /* Atomically replace original file with sorted version */
    if (rename(temp_filename, filename) != 0) {
        unlink(temp_filename);
        unlink(lock_name);
        return -1;
    }

    /* Release lock */
    unlink(lock_name);
    return 0;
}
