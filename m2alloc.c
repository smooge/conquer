/*
 * m2alloc.c - Two-dimensional array memory allocator
 *
 * Provides safe and testable utilities for allocating contiguous
 * two-dimensional arrays in C with comprehensive bounds checking
 * and overflow detection.
 *
 * Phase: 8.4.3 - Bounds Checking Enhancement
 * Date: 2025-10-06
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "header.h"
#include "data.h"

/*
 * m2alloc_safe - Testable two-dimensional array allocator with bounds checking
 *
 * Core allocation logic for two-dimensional arrays with comprehensive parameter
 * validation, overflow detection, and error handling through return values.
 * This function enables full testing coverage of error conditions while providing
 * safe allocation for production use.
 *
 * Provides the same memory layout and allocation strategy as m2alloc(), but
 * returns NULL on errors instead of terminating the program. This design enables:
 * - Comprehensive unit testing of error conditions
 * - Graceful error handling for non-critical allocations
 * - Better error reporting with specific failure messages
 * - Standard errno-based error indication
 *
 * Allocation Strategy:
 * 1. Validate all input parameters (positive, non-zero)
 * 2. Check for integer overflow in size calculations
 * 3. Single malloc() call for all required memory (pointers + data)
 * 4. Pointer array construction for row indexing
 * 5. Contiguous data layout for cache efficiency
 *
 * Memory Layout (identical to m2alloc):
 * - First section: Array of row pointers (nrows * sizeof(char*))
 * - Second section: Actual data storage (nrows * ncols * entrysize)
 * - Row pointers calculated to point into data section
 * - Enables standard array[i][j] syntax for access
 *
 * Parameters:
 *   nrows - Number of rows in the array (must be > 0)
 *   ncols - Number of columns in the array (must be > 0)
 *   entrysize - Size of each element in bytes (must be > 0)
 *   error_msg - Optional buffer for error message (can be NULL)
 *   error_msg_size - Size of error message buffer (ignored if error_msg is NULL)
 *
 * Returns:
 *   char** - Pointer to allocated array on success
 *   NULL - On any error (invalid params, overflow, allocation failure)
 *
 * Error Handling:
 *   - Returns NULL on any error condition
 *   - Sets errno to EINVAL for parameter/overflow errors
 *   - Sets errno to ENOMEM for malloc failure
 *   - Optionally writes descriptive error message to error_msg buffer
 *   - Error messages are truncated to fit in error_msg_size
 *
 * Side Effects:
 *   - Allocates memory using malloc() that must be freed by caller
 *   - Sets errno on error conditions
 *   - Writes to error_msg buffer if provided
 *   - Modifies allocated memory to construct pointer array structure
 *
 * Testing Notes:
 *   Category: A (Unit) - Fully testable allocation utility
 *   Approach: Comprehensive unit tests for success and all error paths
 *   Key Tests: Parameter validation, overflow detection, allocation success/failure
 *   Dependencies: Standard library malloc(), errno
 *   Mock Requirements: None (all errors testable via real function calls)
 *   Complexity: Medium - Multiple validation steps and overflow checks
 *
 * Notes:
 *   - Designed for testability - all error paths return NULL
 *   - Can be used directly for non-critical allocations
 *   - Wrapped by m2alloc() for backward compatibility
 *   - All overflow checks prevent silent wraparound bugs
 *   - Compatible with existing free() calls (same memory layout)
 *   - Error messages help debugging allocation issues
 */
char **m2alloc_safe(
    int nrows,
    int ncols,
    int entrysize,
    char *error_msg,
    size_t error_msg_size
) {
	char **baseaddr;
	int j;

	/* Parameter validation - reject invalid inputs */
	if (nrows <= 0 || ncols <= 0 || entrysize <= 0) {
		if (error_msg != NULL && error_msg_size > 0) {
			snprintf(error_msg, error_msg_size,
			        "Invalid dimensions: %dx%d blocks of %d bytes",
			        nrows, ncols, entrysize);
		}
		errno = EINVAL;
		return NULL;
	}

	/*
	 * Prevent integer overflow in row_data_size calculation.
	 * Check: ncols * entrysize <= SIZE_MAX
	 * Rearranged: ncols <= SIZE_MAX / entrysize
	 */
	if ((size_t)ncols > SIZE_MAX / (size_t)entrysize) {
		if (error_msg != NULL && error_msg_size > 0) {
			snprintf(error_msg, error_msg_size,
			        "Dimension overflow: %d cols * %d bytes exceeds SIZE_MAX",
			        ncols, entrysize);
		}
		errno = EINVAL;
		return NULL;
	}

	size_t row_data_size = (size_t)ncols * (size_t)entrysize;

	/* Calculate pointer array size */
	size_t pointer_size = (size_t)nrows * sizeof(char *);

	/*
	 * Prevent integer overflow in data section calculation.
	 * Check: nrows * row_data_size <= SIZE_MAX
	 * Rearranged: nrows <= SIZE_MAX / row_data_size
	 */
	if ((size_t)nrows > SIZE_MAX / row_data_size) {
		if (error_msg != NULL && error_msg_size > 0) {
			snprintf(error_msg, error_msg_size,
			        "Allocation too large: %d rows * %zu bytes per row exceeds SIZE_MAX",
			        nrows, row_data_size);
		}
		errno = EINVAL;
		return NULL;
	}

	size_t data_size = (size_t)nrows * row_data_size;

	/*
	 * Prevent integer overflow in total size calculation.
	 * Check: pointer_size + data_size <= SIZE_MAX
	 * Rearranged: pointer_size <= SIZE_MAX - data_size
	 */
	if (pointer_size > SIZE_MAX - data_size) {
		if (error_msg != NULL && error_msg_size > 0) {
			snprintf(error_msg, error_msg_size,
			        "Total allocation overflow: %zu + %zu bytes exceeds SIZE_MAX",
			        pointer_size, data_size);
		}
		errno = EINVAL;
		return NULL;
	}

	size_t total_size = pointer_size + data_size;

	/* Perform allocation */
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wanalyzer-allocation-size"
	baseaddr = (char **) malloc(total_size);
	#pragma GCC diagnostic pop

	if (baseaddr == NULL) {
		if (error_msg != NULL && error_msg_size > 0) {
			snprintf(error_msg, error_msg_size,
			        "Cannot allocate %d by %d blocks of %d bytes (%zu total)",
			        nrows, ncols, entrysize, total_size);
		}
		errno = ENOMEM;
		return NULL;
	}

	/* Set up row pointers - point into data section */
	*baseaddr = (char *) (baseaddr + nrows);
	for (j = 1; j < nrows; j++) {
		baseaddr[j] = baseaddr[j-1] + (int)row_data_size;
	}

	return baseaddr;
}

/*
 * m2alloc - Two-dimensional array memory allocator (backward-compatible wrapper)
 *
 * Provides backward-compatible interface to existing m2alloc() usage throughout
 * the codebase. Wraps m2alloc_safe() and maintains original behavior of
 * terminating the program on any allocation failure or invalid parameters.
 *
 * This wrapper ensures zero code changes required for existing callers while
 * enabling new code to use m2alloc_safe() for better error handling. All
 * validation, overflow checking, and allocation logic is delegated to
 * m2alloc_safe().
 *
 * Allocation Strategy:
 * 1. Delegates to m2alloc_safe() for all allocation work
 * 2. Terminates program via abrt() if m2alloc_safe() returns NULL
 * 3. Prints descriptive error message before termination
 * 4. Returns pointer on success (never returns NULL)
 *
 * Memory Layout (identical to original):
 * - First section: Array of row pointers (nrows * sizeof(char*))
 * - Second section: Actual data storage (nrows * ncols * entrysize)
 * - Row pointers calculated to point into data section
 * - Enables standard array[i][j] syntax for access
 *
 * Parameters:
 *   nrows - Number of rows in the two-dimensional array
 *   ncols - Number of columns in each row
 *   entrysize - Size in bytes of each individual array element
 *
 * Returns:
 *   char** - Pointer to allocated array (never returns NULL)
 *
 * Side Effects:
 *   - Allocates memory using malloc() that must be freed by caller
 *   - Terminates program via abrt() if allocation fails or parameters invalid
 *   - Writes error message to stdout on any error
 *   - Modifies allocated memory to construct pointer array structure
 *
 * Testing Notes:
 *   Category: B (Integration) - Wrapper function, test m2alloc_safe() instead
 *   Approach: Integration tests verify wrapper behavior with valid inputs
 *   Key Tests: Successful allocation, proper delegation to m2alloc_safe()
 *   Dependencies: m2alloc_safe(), abrt()
 *   Mock Requirements: Cannot test error paths (calls abrt())
 *   Complexity: Simple - Thin wrapper with error handling
 *
 * Notes:
 *   - Maintains exact behavior of original m2alloc() implementation
 *   - Essential utility for dynamic game world arrays (sct, occ, movecost)
 *   - Contiguous allocation improves cache performance for large arrays
 *   - Single allocation/free cycle simplifies memory management
 *   - Generic interface supports any data type through entrysize parameter
 *   - Error handling prevents silent allocation failures
 *   - Widely used throughout game system for dynamic data structures
 *   - Critical infrastructure for scalable world sizes
 *   - See m2alloc_safe() for testable version with NULL return on error
 */
char **m2alloc(
    int nrows,		/* row dimension */
    int ncols,		/* column dimension */
    int entrysize	/* # bytes in items to be stored */
) {
	char error_msg[256];
	char **result = m2alloc_safe(nrows, ncols, entrysize,
	                             error_msg, sizeof(error_msg));

	if (result == NULL) {
		printf("OOPS - %s\n", error_msg);
		abrt();
	}

	return result;
}
