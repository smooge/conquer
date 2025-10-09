/*
 * check.c - Data validation and integrity checks
 *
 * OVERVIEW:
 * This module provides comprehensive data validation and integrity checking
 * for the Conquer game system. It implements defensive programming strategies
 * to ensure game state consistency, prevent data corruption, and maintain
 * logical relationships between game entities. The module serves as a critical
 * safeguard against invalid data states that could compromise game functionality.
 *
 * CORE FUNCTIONALITY:
 * - Nation data validation (resources, armies, navies, diplomacy)
 * - Sector/map data validation (trade goods, population, ownership)
 * - Cross-platform file locking for multi-user environments
 * - Automatic correction of invalid values with error reporting
 * - Debug support for development and testing phases
 *
 * ARCHITECTURAL ROLE:
 * This module acts as a data integrity guardian, providing validation services
 * throughout the game system. It's designed to be called after data loading,
 * before critical operations, and whenever data corruption is suspected. The
 * module implements both corrective actions (fixing invalid values) and
 * reporting mechanisms (detailed error messages with source location).
 *
 * PLATFORM COMPATIBILITY:
 * - Cross-platform file locking (flock vs timestamp-based)
 * - Conditional compilation for debug features
 * - POSIX-compliant file operations with fallback mechanisms
 * - Support for various Unix-like systems and development environments
 *
 * USAGE PATTERNS:
 * - Call verifydata() after loading game state from files
 * - Use check_lock() to prevent concurrent data access
 * - Enable DEBUG compilation for detailed validation logging
 * - Integrate validation calls throughout critical game operations
 *
 * This file is part of Conquer.
 * Originally Copyright (C) 1988-1989 by Edward M. Barlow and Adam Bryant
 * Copyright (C) 2025 Juan Manuel Méndez Rey (Vejeta) - Licensed under GPL v3 with permission
 * from original authors
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

#include <ctype.h>
#include <stdio.h>
#include <fcntl.h>
#ifndef FILELOCK
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <unistd.h>
#include "header.h"
#include "data.h"

/*
 * verify_ntn - Comprehensive nation data validation and integrity checks
 *
 * Performs thorough validation of all nation data structures including resources,
 * armies, navies, and diplomatic relations. Automatically corrects invalid values
 * and reports inconsistencies to stderr with file/line debugging information.
 * This is a critical data integrity function that ensures game state consistency.
 *
 * Parameters:
 *   __file__ - Source filename for error reporting (debugging macro)
 *   __line__ - Source line number for error reporting (debugging macro)
 *
 * Returns:
 *   void - Function performs corrections in-place on global nation data
 *
 * Side Effects:
 *   - Modifies global ntn[] array to correct invalid values
 *   - Prints validation errors to stderr with precise file/line location
 *   - Ensures army-navy coordination consistency (ONBOARD status validation)
 *   - Enforces diplomatic relationship constraints for NPC nations
 *   - Corrects negative resource values (metals, jewels)
 *   - Validates army positioning and prevents armies on water
 *   - Enforces map boundary constraints for unit positions
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires full nation, army, navy, and map data
 *   Approach: Integration testing with constructed game states
 *   Key Tests: Negative resources, invalid positions, army-navy mismatches, diplomatic
 * violations Dependencies: Global ntn[], sct[][] arrays, nation/army/navy data structures Mock
 * Requirements: Complete game state with various invalid conditions Complexity: Moderate -
 * Multi-system validation with complex army-navy relationships
 *
 * Notes:
 *   - Uses debugging macros __file__ and __line__ for precise error reporting
 *   - Critical for maintaining game data integrity across save/load cycles
 *   - Handles complex army-navy coordination validation (ONBOARD status)
 *   - Enforces automatic WAR status for all NPC nations (active >= NPC_PEASANT)
 *   - Thread safety: Not thread-safe due to global data modification
 * @last_documented: 2025-09-19
 */
void verify_ntn(char __file__[], int __line__) {
    register struct s_nation *nptr;
    register int i;
    register int cntry;
    int j, k;
    struct army *a;

    for (cntry = 0; cntry < NTOTAL; cntry++) {
        nptr = &ntn[cntry];

        if (cntry != 0 && nptr->active == 0)
            continue;
        if (nptr->metals < 0L) {
            fprintf(stderr, "file %s: line %d: nation[%d] metal = %ld\n", __file__, __line__,
                    cntry, nptr->metals);
            nptr->metals = 0L;
        }

        if (nptr->jewels < 0) {
            fprintf(stderr, "file %s: line %d: nation[%d] jewels = %ld\n", __file__, __line__,
                    cntry, nptr->jewels);
            nptr->jewels = 0.0;
        }

        for (i = 0; i < MAXARM; i++) {
            a = &nptr->arm[i];
            if (a->sold < 0) {
                fprintf(stderr, "file %s: line %d: nation[%d] army[%d] sold = %ld\n", __file__,
                        __line__, cntry, i, a->sold);
                a->sold = 0;
            }
            if (a->sold == 0)
                continue;
            if (a->xloc >= MAPX) {
                fprintf(stderr, "file %s: line %d: nation[%d] army[%d] xlocation = %d\n",
                        __file__, __line__, cntry, i, a->xloc);
                a->xloc = 0;
            }
            if (a->yloc >= MAPY) {
                fprintf(stderr, "file %s: line %d: nation[%d] army[%d] ylocation = %d\n",
                        __file__, __line__, cntry, i, a->yloc);
                a->yloc = 0;
            }
            if (a->stat == ONBOARD) {
                if (a->smove != 0) {
                    fprintf(stderr, "file %s: line %d: nation[%d] army[%d] onboard move = %d\n",
                            __file__, __line__, cntry, i, a->smove);
                    a->smove = 0;
                }
                k = 0;
                for (j = 0; j < MAXNAVY; j++) {
                    if (nptr->nvy[j].warships == 0 && nptr->nvy[j].merchant == 0
                        && nptr->nvy[j].galleys == 0)
                        continue;
                    if (nptr->nvy[j].armynum == i) {
                        k = 1;
                    }
                }
                if (k == 0) {
                    fprintf(stderr, "files %s: line %d: nation[%d] army[%d] onboard nothing\n",
                            __file__, __line__, cntry, i);
                    a->stat = DEFEND;
                }
            }
            if (a->stat != ONBOARD && sct[a->xloc][a->yloc].altitude == WATER) {
                fprintf(stderr,
                        "file %s: line %d: nation[%d] army[%d] loc=%d,%d (water) men=%ld\n",
                        __file__, __line__, cntry, i, a->xloc, a->yloc, a->sold);
                a->sold = 0;
            }
        } /* for */

        for (i = 0; i < MAXNAVY; i++) {
            if (nptr->nvy[i].warships == 0 && nptr->nvy[i].merchant == 0
                && nptr->nvy[i].galleys == 0)
                continue;
            if (nptr->nvy[i].armynum != MAXARM) {
                a = &(nptr->arm[nptr->nvy[i].armynum]);
                if (a->sold == 0 || a->stat != ONBOARD) {
                    fprintf(stderr,
                            "file %s: line %d: nation[%d] navy[%d] carrying invalid troop\n",
                            __file__, __line__, cntry, i);
                    nptr->nvy[i].armynum = MAXARM;
                }
            }
        } /* for */

        for (i = 0; i < NTOTAL; i++) {
            if (ntn[i].active >= NPC_PEASANT) {
                if (nptr->dstatus[i] != WAR) {
                    nptr->dstatus[i] = WAR;
                }
                if (ntn[i].dstatus[cntry] != WAR) {
                    ntn[i].dstatus[cntry] = WAR;
                }
            }
            if (nptr->dstatus[i] > JIHAD) {
                fprintf(stderr, "file %s: line %d: nation[%d] diplomatic status with %d = %d\n",
                        __file__, __line__, cntry, i, nptr->dstatus[i]);
                nptr->dstatus[i] = WAR;
            }
        } /* for */
    } /* for */
} /* verify_ntn() */

/*
 * verify_sct - Comprehensive sector data validation and integrity checks
 *
 * Performs thorough validation of all map sector data including trade goods,
 * resources, population, ownership, and fortress levels. Automatically corrects
 * invalid values and ensures logical consistency between sector properties.
 * This function maintains map data integrity and prevents impossible game states.
 *
 * Parameters:
 *   __file__ - Source filename for error reporting (debugging macro)
 *   __line__ - Source line number for error reporting (debugging macro)
 *
 * Returns:
 *   void - Function performs corrections in-place on global sector data
 *
 * Side Effects:
 *   - Modifies global sct[][] array to correct invalid values
 *   - Prints validation errors to stderr with precise file/line location
 *   - Enforces trade good and resource relationships (metals with mines, jewels with wealth)
 *   - Caps population at ABSMAXPEOPLE and corrects negative populations
 *   - Prevents ownership of water sectors (sets owner to 0)
 *   - Limits fortress levels to maximum of 12
 *   - Clears invalid trade good values
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires full map data and trade good definitions
 *   Approach: Integration testing with various invalid sector configurations
 *   Key Tests: Invalid trade goods, resource mismatches, water ownership, extreme populations
 *   Dependencies: Global sct[][] array, trade good constants, nation data for ownership
 * validation Mock Requirements: Map sectors with various invalid property combinations
 *   Complexity: Moderate - Resource-tradegood relationship validation with boundary checking
 *
 * Notes:
 *   - Validates complex relationships between trade goods and available resources
 *   - Uses trade good range constants (END_MINE, END_WEALTH, END_NORMAL)
 *   - Critical for preventing impossible economic conditions
 *   - Handles population overflow and underflow with appropriate corrections
 *   - Thread safety: Not thread-safe due to global map data modification
 * @last_documented: 2025-09-19
 */
void verify_sct(char __file__[], int __line__) {
    register struct s_sector *sptr;
    register int x, y;

    for (x = 0; x < MAPX; x++) {
        for (y = 0; y < MAPY; y++) {
            sptr = &sct[x][y];

            if (sptr->tradegood > TG_none) {
                fprintf(stderr, "file %s: line %d: sct[%d][%d].tradegood = %d (invalid)\n",
                        __file__, __line__, x, y, sptr->tradegood);
                sptr->tradegood = TG_none;
            }
            if ((sptr->metal != 0)
                && ((sptr->tradegood > END_MINE) || (sptr->tradegood <= END_NORMAL))) {
                fprintf(stderr, "file %s: line %d: sct[%d][%d].metal = %d with no tradegood\n",
                        __file__, __line__, x, y, sptr->metal);
                sptr->metal = 0;
            }

            if ((sptr->jewels != 0)
                && ((sptr->tradegood > END_WEALTH) || (sptr->tradegood <= END_MINE))) {
                fprintf(stderr, "file %s: line %d: sct[%d][%d].jewels = %d with no tradegood\n",
                        __file__, __line__, x, y, sptr->jewels);
                sptr->jewels = 0;
            }

            if (sptr->people > ABSMAXPEOPLE)
                sptr->people = ABSMAXPEOPLE;

            if (sptr->people < 0) {
                fprintf(stderr, "file %s: line %d: sct[%d][%d].people = %ld\n", __file__,
                        __line__, x, y, sptr->people);
                if (sptr->people < -1 * ABSMAXPEOPLE)
                    sptr->people = ABSMAXPEOPLE;
                else
                    sptr->people = 0;
            }

            if (sptr->owner != 0 && sptr->altitude == WATER) {
                fprintf(stderr, "file %s: line %d: sct[%d][%d].owner = %s (a water sector)\n",
                        __file__, __line__, x, y, ntn[sptr->owner].name);
                sptr->owner = 0;
            }
            if (sptr->fortress > 12) {
                fprintf(stderr, "file %s: line %d: sct[%d][%d].fortress = %d \n", __file__,
                        __line__, x, y, sptr->fortress);
                sptr->fortress = 12;
            }
        } /* for */
    } /* for */
} /* verify_sct() */

/*
 * verifydata - Master data validation coordinator function
 *
 * Primary entry point for comprehensive game data validation. Orchestrates
 * complete integrity checking of all critical game data structures by
 * calling specialized validation functions. This function ensures overall
 * game state consistency and should be called after loading game data
 * or when data corruption is suspected.
 *
 * Parameters:
 *   __file__ - Source filename for error reporting (debugging macro)
 *   __line__ - Source line number for error reporting (debugging macro)
 *
 * Returns:
 *   void - Function coordinates validation through specialized functions
 *
 * Side Effects:
 *   - Triggers comprehensive validation of all game data via subfunctions
 *   - All corrections and error reporting handled by called validation functions
 *   - Ensures both nation and sector data integrity through coordinated checks
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires complete game state for comprehensive validation
 *   Approach: Integration testing with full game data including corrupted scenarios
 *   Key Tests: Complete data validation, error recovery, coordinator functionality
 *   Dependencies: verify_ntn(), verify_sct(), complete game state
 *   Mock Requirements: Full game world with intentionally corrupted data
 *   Complexity: Simple - Straightforward coordinator without complex logic
 *
 * Notes:
 *   - Central validation entry point for the entire game system
 *   - Delegates actual validation work to specialized functions
 *   - Should be called after data loading and before critical game operations
 *   - Part of defensive programming strategy for data integrity
 *   - Thread safety: Inherits thread safety characteristics of called functions
 * @last_documented: 2025-09-19
 */
void verifydata(char __file__[], int __line__) {
    /* check for invalid values */
    verify_ntn(__file__, __line__);
    verify_sct(__file__, __line__);
} /* verifydata() */

#ifdef DEBUG
/*
 * checkout - Debug verification function with conditional compilation
 *
 * Debug-only function that provides verbose data validation with explicit
 * file and line number reporting. Only compiled when DEBUG macro is defined,
 * allowing developers to trigger comprehensive data validation with detailed
 * debugging output during development and testing phases.
 *
 * Parameters:
 *   file - Source filename for debugging identification
 *   line - Source line number for debugging identification
 *
 * Returns:
 *   void - Function prints debug information and triggers validation
 *
 * Side Effects:
 *   - Prints debug message to stderr with file and line information
 *   - Triggers complete data validation via verifydata()
 *   - All validation corrections and errors reported through verifydata()
 *
 * Testing Notes:
 *   Category: D (Mock) - Debug function requiring controlled testing environment
 *   Approach: Unit testing with DEBUG compilation and stderr capture
 *   Key Tests: Debug output format, verifydata() delegation, conditional compilation
 *   Dependencies: DEBUG macro definition, verifydata() function, stderr output
 *   Mock Requirements: Captured stderr output for verification
 *   Complexity: Simple - Straightforward debug wrapper with output and delegation
 *
 * Notes:
 *   - Only available when compiled with DEBUG macro defined
 *   - Provides verbose debugging output for development use
 *   - Acts as a wrapper around verifydata() with added debug information
 *   - Parameter order differs from other functions (line, file vs file, line)
 *   - Thread safety: Inherits characteristics from verifydata() and fprintf()
 * @last_documented: 2025-09-19
 */
void checkout(char *file, int line) {
    fprintf(stderr, "file %s line %d\n", file, line);
    verifydata(file, line);
}
#endif /* DEBUG */

#ifdef FILELOCK
/* Modern file locking using flock() - POSIX standard across all target platforms */
#include <sys/types.h>
#include <sys/file.h>
#define do_lock(fd) flock(fd, LOCK_EX | LOCK_NB)
#endif /* FILELOCK */

/*
 * check_lock - Cross-platform file locking verification and management
 *
 * Implements robust file locking mechanism with support for multiple locking
 * strategies depending on compile-time configuration. Provides both lock
 * detection and optional lock acquisition with automatic cleanup of stale
 * locks. Critical for preventing concurrent access to game data files and
 * ensuring data integrity in multi-user environments.
 *
 * Parameters:
 *   filename - Path to the file to check/lock (lock file name)
 *   keeplock - If TRUE, maintain lock after checking; if FALSE, release immediately
 *
 * Returns:
 *   TRUE (1) if file is currently locked by another process
 *   FALSE (0) if file is not locked (and lock acquired if keeplock=TRUE)
 *
 * Side Effects:
 *   - May create lock file if keeplock=TRUE and no lock exists
 *   - Removes stale lock files (older than TIME_DEAD*3 seconds)
 *   - Exits program with FAIL status on critical file operation errors
 *   - Uses platform-specific locking mechanism (flock vs stat-based)
 *
 * Testing Notes:
 *   Category: C (System) - Requires filesystem access and platform-specific locking
 *   Approach: System testing with actual file operations and timing
 *   Key Tests: Lock detection, stale lock cleanup, platform compatibility, error handling
 *   Dependencies: File system, platform locking APIs, TIME_DEAD constant
 *   Mock Requirements: Filesystem simulation, timing control, platform-specific mocking
 *   Complexity: Moderate - Platform-specific implementation with timing considerations
 *
 * Notes:
 *   - Two implementation strategies based on FILELOCK compilation flag
 *   - FILELOCK defined: Uses flock() for true file locking
 *   - FILELOCK undefined: Uses file timestamps for lock detection (fallback)
 *   - Handles stale lock cleanup automatically (TIME_DEAD*3 threshold)
 *   - Critical error conditions cause program termination (defensive programming)
 *   - Thread safety: Platform-dependent (filesystem operations)
 * @last_documented: 2025-09-19
 */
int check_lock(char *filename, int keeplock) {
    int hold = FALSE;
#ifdef FILELOCK
    int fd;

    if ((fd = open(filename, O_WRONLY | O_CREAT, 0600)) != (-1)) {
        if (do_lock(fd) == (-1)) {
            hold = TRUE;
        }
        /* remove lock after checking */
        if (keeplock == FALSE && hold == FALSE) {
            close(fd);
            unlink(filename);
        }
    } else {
        printf("error opening lock file <%s>\n", filename);
        exit(FAIL);
    }
#else
    struct stat fst;

    if (stat(filename, &fst) == 0) {
        long now;
        now = time(0);
        if (now - fst.st_mtime < TIME_DEAD * 3) {
            hold = TRUE;
        } else {
            /* remove useless file */
            unlink(filename);
        }
    }
    if (hold == FALSE && keeplock == TRUE) {
        /* create lock file */
        if (open(filename, O_CREAT, 0600) == (-1)) {
            printf("error opening lock file <%s>\n", filename);
            exit(FAIL);
        }
    }
#endif /* FILELOCK */
    return (hold);
}
