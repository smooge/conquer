/*
 * safe_system.h - Safe system() call replacements for Conquer
 *
 * This header provides declarations for functions that replace unsafe
 * system() calls with safer native C implementations.
 *
 * Functions in this module:
 * - append_file_to_file() - Safe file concatenation (replaces "cat source >> dest")
 * - write_timestamp_to_file() - Safe timestamp writing (replaces "date > file")
 * - secure_file_delete() - Safe file deletion (replaces "rm -f pattern*")
 */

#ifndef SAFE_SYSTEM_H
#define SAFE_SYSTEM_H

/*
 * append_file_to_file - Append contents of source file to destination file
 *
 * Safely appends the entire contents of the source file to the destination
 * file using native C file operations. Replaces system("cat source >> dest").
 * Includes file locking to prevent concurrent write conflicts.
 *
 * Parameters:
 *   source - Path to source file (must not be NULL)
 *   destination - Path to destination file (must not be NULL)
 *
 * Returns:
 *   0 on success, -1 on error
 *
 * Side Effects:
 *   - Creates destination file if it doesn't exist
 *   - Uses file locking during operation
 *   - May block if destination is locked by another process
 */
int append_file_to_file(const char *source, const char *destination);

/*
 * write_timestamp_to_file - Write current date/time to file
 *
 * Writes the current date and time to the specified file in a human-readable
 * format. Replaces system("date > filename") with native C time functions.
 *
 * Parameters:
 *   filename - Path to output file (must not be NULL)
 *
 * Returns:
 *   0 on success, -1 on error
 *
 * Side Effects:
 *   - Creates or overwrites the specified file
 *   - Uses local timezone for timestamp
 */
int write_timestamp_to_file(const char *filename);

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
 *   - Deletes files matching the glob patterns
 *   - No interpretation of shell metacharacters
 *   - Thread-safe (no global state modifications)
 */
int secure_file_delete(const char **patterns, int num_patterns);

/*
 * sort_file_in_place - Sort lines in a file using native C implementation
 *
 * Sorts the lines of a file in place using the same algorithm as the external
 * conqsort utility. This eliminates command injection vulnerabilities by
 * replacing system() calls with secure native C code.
 *
 * Parameters:
 *   filename - Path to file to sort (must not be NULL)
 *   compnum - Number of characters to compare for sorting (typically 2)
 *
 * Returns:
 *   0 on success, -1 on error
 *
 * Side Effects:
 *   - Reads entire file into memory
 *   - Atomically replaces file with sorted version
 *   - Uses file locking to prevent concurrent access
 *   - Lines shorter than compnum characters are skipped
 */
int sort_file_in_place(const char *filename, int compnum);

#endif /* SAFE_SYSTEM_H */
