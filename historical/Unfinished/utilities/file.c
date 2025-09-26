/*
 * file.c - Multi-map file manipulation routines
 * 
 * This file is part of Conquer Utilities.
 * Originally Copyright (C) 1989 by Richard Caley
 * Copyright (C) 2025 Juan Manuel Méndez Rey (Vejeta) - Licensed under GPL v3 with permission from original license
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include "file.h"

/*********************************************************************
*                                                                    *
* MULTI-MAP FILE MANIPULATION SYSTEM                                 *
*                                                                    *
* This file implements a multi-file management system that allows    *
* multiple independent file pointers and positions within the same   *
* physical file. This is designed to support multi-map operations    *
* where different map regions need independent file access patterns. *
*                                                                    *
* Key Components:                                                     *
* - Virtual File Management: myopen(), myclose()                     *
* - Position Tracking: myrewind(), independent offset management     *
* - Data Access: mygets() with per-handle positioning                *
* - Handle Validation: myisopen() for status checking                *
*                                                                    *
* Architecture Overview:                                              *
* The system maintains a table of file handles (struct file) that    *
* wrap standard FILE* pointers with additional state tracking for    *
* independent positioning. Each handle tracks its current offset     *
* and logical start position (top), enabling multiple readers to     *
* access different regions of the same file simultaneously.          *
*                                                                    *
* Usage Pattern:                                                      *
* 1. myopen(handle_id, FILE*) - Associate handle with file          *
* 2. mygets(buffer, size, handle_id) - Read with handle positioning *
* 3. myrewind(handle_id) - Reset to logical start position          *
* 4. myclose(handle_id) - Close handle (manages shared FILE*)       *
*                                                                    *
* Multi-Map Context:                                                  *
* This system enables the conqrast graphics utility to read          *
* different map layers or regions from a single map file, with       *
* each map component maintaining its own read position and state.    *
*                                                                    *
*********************************************************************/

#define MAX_FILES 10

static struct file 
    {
    FILE *fd;			/* which file */
    long offset,		/* where are we reading from now */
         top;			/* where is the notional start */
    } file_table[MAX_FILES];
    

/*
 * myopen - Initialize virtual file handle with physical FILE pointer
 *
 * Associates a virtual file handle (identified by numeric ID) with a physical
 * FILE pointer, enabling multiple independent access patterns to the same file.
 * Records the current file position as both the starting offset and current
 * position for this virtual handle.
 *
 * This function enables multiple map readers to access different regions of
 * the same file by maintaining separate position tracking for each handle.
 * The current file position becomes the logical "start" position for this
 * virtual handle, allowing myrewind() to return to this point.
 *
 * Parameters:
 *   which - Virtual file handle ID (0 to MAX_FILES-1)
 *           No bounds checking performed - caller must ensure valid range
 *   file  - Physical FILE pointer to associate with this handle
 *           Must be a valid, open FILE pointer (no validation performed)
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Modifies file_table[which] entry
 *   - Records current file position via ftell() as offset and top
 *   - Does not modify the physical file position
 *
 * Testing Notes:
 *   Category: A (Unit) - Direct function with deterministic behavior
 *   Approach: Unit tests with mock FILE operations and position verification
 *   Key Tests: [Handle association, position recording, multiple handles]
 *   Dependencies: FILE pointer management, ftell() function
 *   Mock Requirements: Mock FILE structure and ftell() for testing
 *   Complexity: Simple - Direct assignment and position recording
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to shared file_table access
 *   - Performance: Single ftell() call overhead
 *   - Error handling: No validation of which parameter or file pointer
 *   - Resource management: Does not take ownership of FILE pointer
 */
void
myopen(which,file)

int which;
FILE *file;

{
file_table[which].fd=file;
file_table[which].offset=file_table[which].top=ftell(file);
}

/*
 * myisopen - Check if virtual file handle is associated with a FILE pointer
 *
 * Tests whether a virtual file handle has been initialized and is currently
 * associated with a valid FILE pointer. This provides a way to verify handle
 * state before performing file operations.
 *
 * The function simply checks if the fd field in the file_table entry is
 * non-NULL, indicating that myopen() has been called to associate this
 * handle with a physical file.
 *
 * Parameters:
 *   which - Virtual file handle ID to check (0 to MAX_FILES-1)
 *           No bounds checking performed - caller must ensure valid range
 *
 * Returns:
 *   int - Non-zero if handle is associated with FILE pointer, 0 if not
 *         Returns result of (file_table[which].fd != NULL)
 *
 * Side Effects:
 *   None - read-only operation
 *
 * Testing Notes:
 *   Category: A (Unit) - Simple boolean check with deterministic output
 *   Approach: Unit tests with initialized and uninitialized handles
 *   Key Tests: [Open handle check, closed handle check, invalid handle]
 *   Dependencies: file_table state management
 *   Mock Requirements: Mock file_table entries for testing
 *   Complexity: Simple - Single boolean comparison
 *
 * Notes:
 *   - Thread safety: Safe for read-only access (assuming no concurrent writes)
 *   - Performance: Single pointer comparison
 *   - Error handling: No bounds checking on which parameter
 *   - Return type: Missing explicit return type declaration (int implied)
 */
myisopen(which)

int which;

{
return file_table[which].fd != NULL;
}

/*
 * mygets - Read line from virtual file handle with independent positioning
 *
 * Reads a line of text from the file associated with the specified virtual
 * handle, maintaining independent file positioning for each handle. This
 * enables multiple readers to access different regions of the same file
 * without interfering with each other's read positions.
 *
 * The function first seeks to the handle's current offset position, reads
 * a line using fgets(), then updates the handle's position tracking for
 * subsequent reads. This allows each virtual handle to maintain its own
 * independent read pointer within the shared physical file.
 *
 * Parameters:
 *   buffer - Character buffer to store the read line
 *            Must be allocated with sufficient space (no validation)
 *   num    - Maximum number of characters to read (including null terminator)
 *            Passed directly to fgets() (must be positive)
 *   which  - Virtual file handle ID (0 to MAX_FILES-1)
 *            No bounds checking performed
 *
 * Returns:
 *   char* - Pointer to buffer on success, NULL on EOF or error
 *           Returns the result of fgets() operation
 *
 * Side Effects:
 *   - Seeks physical file to handle's current offset position
 *   - Updates handle's offset to new position after read
 *   - Modifies physical file position (affects other handles temporarily)
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires FILE operations and position management
 *   Approach: Integration tests with real/mock files and multiple handles
 *   Key Tests: [Line reading, position tracking, handle independence, EOF handling]
 *   Dependencies: FILE operations (fseek, fgets, ftell), file_table state
 *   Mock Requirements: Mock FILE operations for unit testing
 *   Complexity: Moderate - File I/O with position management
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to shared FILE pointer usage
 *   - Performance: fseek + fgets + ftell overhead per call
 *   - Error handling: No validation of parameters, relies on fgets() behavior
 *   - Position management: Critical for multi-handle independence
 */
char *
mygets(buffer,num,which)

char *buffer;
int num;
int which;
{
    fseek(file_table[which].fd,file_table[which].offset,0);

    buffer=fgets(buffer,num,file_table[which].fd);

    file_table[which].offset=ftell(file_table[which].fd);

    return buffer;
    }

/*
 * myrewind - Reset virtual file handle to its logical start position
 *
 * Resets the specified virtual file handle to its logical starting position
 * (the position that was current when myopen() was called). This allows
 * each virtual handle to have its own independent "beginning" within the
 * shared physical file.
 *
 * The function seeks the physical file to the handle's "top" position
 * (recorded during myopen()) and resets the handle's current offset to
 * match. This enables re-reading of the handle's designated region without
 * affecting other handles' positions.
 *
 * Parameters:
 *   which - Virtual file handle ID to rewind (0 to MAX_FILES-1)
 *           No bounds checking performed - caller must ensure valid range
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Seeks physical file to handle's top position
 *   - Resets handle's offset to match top position
 *   - Temporarily affects physical file position (impacts other handles)
 *
 * Testing Notes:
 *   Category: A (Unit) - Direct position manipulation with deterministic behavior
 *   Approach: Unit tests with position verification before and after rewind
 *   Key Tests: [Position reset, multiple handle independence, repeated rewinds]
 *   Dependencies: FILE operations (fseek), file_table state management
 *   Mock Requirements: Mock fseek() and file_table for testing
 *   Complexity: Simple - Direct position reset operation
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to shared FILE pointer usage
 *   - Performance: Single fseek() call overhead
 *   - Error handling: No validation of which parameter or fseek() result
 *   - Usage pattern: Enables re-reading of handle's designated file region
 */
void
myrewind(which)

int which;

{
    fseek(file_table[which].fd,file_table[which].top,0);
    file_table[which].offset=file_table[which].top;
    }

/*
 * myclose - Close virtual file handle with shared FILE pointer management
 *
 * Closes the specified virtual file handle, carefully managing shared FILE
 * pointer resources. If multiple virtual handles share the same physical
 * FILE pointer, only the handle entry is cleared. The physical file is
 * only closed when no other handles reference it.
 *
 * This function implements reference counting for shared FILE pointers,
 * preventing premature closure of files that are still in use by other
 * virtual handles. It scans the entire file_table to check for other
 * handles using the same FILE pointer before deciding whether to actually
 * close the physical file.
 *
 * Parameters:
 *   which - Virtual file handle ID to close (0 to MAX_FILES-1)
 *           No bounds checking performed - caller must ensure valid range
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Clears file_table[which].fd to NULL (marks handle as closed)
 *   - May close physical FILE pointer if no other handles reference it
 *   - Scans entire file_table array for reference counting
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires multi-handle state management
 *   Approach: Integration tests with multiple handles sharing files
 *   Key Tests: [Single handle close, shared file close, reference counting]
 *   Dependencies: file_table state, FILE pointer management, fclose()
 *   Mock Requirements: Mock fclose() and file_table for testing
 *   Complexity: Moderate - Reference counting with array scanning
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to shared file_table access
 *   - Performance: O(MAX_FILES) scan for reference counting
 *   - Error handling: No validation of which parameter
 *   - Resource management: Prevents FILE pointer leaks through reference counting
 *   - Critical function: Improper use can cause file handle leaks or double-close
 */
void
myclose(which)

int which;

{
    int i;

    for(i=0;i<MAX_FILES;i++)
	if ( i != which && file_table[i].fd == file_table[which].fd)
	    {
	    file_table[which].fd=NULL;
	    return;
	    }

    fclose(file_table[which].fd);
    }

