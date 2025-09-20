/*
 * file.h - Multi-map file manipulation routines
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

#include <stdio.h>  /* Required for FILE type definitions */

/*********************************************************************
*                                                                    *
* MULTI-MAP FILE MANIPULATION SYSTEM - HEADER DEFINITIONS            *
*                                                                    *
* This header declares the interface for a virtual file handle       *
* system that enables multiple independent access patterns to the    *
* same physical file. This is essential for multi-map graphics       *
* operations where different map components need independent file     *
* positioning within shared map data files.                          *
*                                                                    *
* Core Interface Functions:                                           *
* - myopen()   : Associate virtual handle with FILE pointer          *
* - myisopen() : Check handle initialization status                   *
* - mygets()   : Read line with independent handle positioning        *
* - myrewind() : Reset handle to its logical start position          *
* - myclose()  : Close handle with shared FILE pointer management    *
*                                                                    *
* System Architecture:                                                *
* Each virtual handle maintains independent position tracking within  *
* a shared physical file, enabling multiple map readers to access    *
* different regions without interference. The system provides         *
* reference counting for shared FILE pointers to prevent resource    *
* leaks and premature file closure.                                  *
*                                                                    *
* Usage Pattern:                                                      *
* 1. Open physical file with standard fopen()                        *
* 2. Associate virtual handles using myopen()                        *
* 3. Use mygets() for independent reading operations                  *
* 4. Use myrewind() to reset handle to start position                *
* 5. Close handles with myclose() (manages shared resources)         *
*                                                                    *
* Thread Safety: Not thread-safe - requires external synchronization *
* Memory Management: Does not take ownership of FILE pointers        *
* Resource Limits: Maximum 10 concurrent virtual handles (MAX_FILES) *
*                                                                    *
*********************************************************************/

/*
 * Virtual File Handle Management Functions
 *
 * These functions provide a virtual file handle layer on top of standard
 * C FILE operations, enabling multiple independent access patterns to
 * the same physical file for multi-map graphics operations.
 */

/*
 * myopen - Initialize virtual file handle with physical FILE pointer
 *
 * Parameters:
 *   which - Virtual handle ID (0 to MAX_FILES-1)
 *   file  - Physical FILE pointer to associate
 */
void myopen(int which, FILE *file);

/*
 * myisopen - Check if virtual file handle is initialized
 *
 * Parameters:
 *   which - Virtual handle ID to check
 *
 * Returns:
 *   int - Non-zero if handle is open, 0 if closed
 *
 * Note: Function name corrected from 'isopen' to 'myisopen' for consistency
 */
int myisopen(int which);

/*
 * mygets - Read line from virtual handle with independent positioning
 *
 * Parameters:
 *   buffer - Character buffer for line storage
 *   num    - Maximum characters to read (including null terminator)
 *   which  - Virtual handle ID for reading
 *
 * Returns:
 *   char* - Pointer to buffer on success, NULL on EOF/error
 */
char *mygets(char *buffer, int num, int which);

/*
 * myrewind - Reset virtual handle to its logical start position
 *
 * Parameters:
 *   which - Virtual handle ID to rewind
 */
void myrewind(int which);

/*
 * myclose - Close virtual handle with shared FILE pointer management
 *
 * Implements reference counting to ensure shared FILE pointers are
 * only closed when no other virtual handles reference them.
 *
 * Parameters:
 *   which - Virtual handle ID to close
 */
void myclose(int which);

