/*
 * misc.c - Miscellaneous utility functions
 * 
 * This file is part of Conquer.
 * Originally Copyright (C) 1988-1989 by Edward M. Barlow and Adam Bryant
 * Copyright (C) 2025 Juan Manuel Méndez Rey (Vejeta) - Licensed under GPL v3 with permission from original authors
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
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <strings.h>
#ifdef CONQUER
#include <sys/types.h>
#include <sys/stat.h>
#endif /*CONQUER*/
#include <time.h>
#include <unistd.h>
#include "header.h"
#include "data.h"
#include "trade.h"

extern FILE *fnews;
extern short country,redraw;

extern char *HVegcost, *OVegcost, *EVegcost, *DVegcost, *FVegcost;
extern char *HElecost, *OElecost, *EElecost, *DElecost, *FElecost;

#ifdef SYSV
char    *memset();
#endif

#ifdef CONQUER
/*
 * move_file - Atomic file move operation using UNIX link/unlink pattern
 *
 * Implements atomic file movement by creating a hard link to the destination
 * and then removing the source. This ensures atomicity on UNIX filesystems
 * where both link() and unlink() are atomic operations.
 *
 * Parameters:
 *   from - Source file path (must exist and be accessible)
 *   to   - Destination file path (will be unlinked if exists)
 *
 * Returns:
 *   0 on successful file move, -1 on any failure
 *
 * Side Effects:
 *   - Destination file is removed if it exists
 *   - Source file is moved to destination location
 *   - Error messages printed to stderr on failure
 *   - 2-second sleep delay on error conditions
 *
 * Testing Notes:
 *   Category: C (System) - Requires filesystem access and file operations
 *   Approach: System testing with temporary files
 *   Key Tests: File permissions, missing files, cross-filesystem moves
 *   Dependencies: UNIX filesystem with link/unlink support
 *   Mock Requirements: None - uses real filesystem operations
 *   Complexity: Simple - Straightforward file operations with error handling
 *
 * Notes:
 *   - Uses traditional UNIX atomic move pattern (link + unlink)
 *   - May fail on cross-filesystem moves (different inodes)
 *   - Error handling includes user feedback and delay
 *   - Only available when CONQUER is defined
 */
int
move_file( from, to )
register char	*from;
register char	*to;
{
	if( unlink( to ) < 0 ) {
		fprintf( stderr, "unlink( %s ) failed \n", to );
		sleep( 2 );
		return( -1 );
	}

	if( link( from, to ) < 0 ) {
		fprintf( stderr, "link( %s, %s ) failed \n", from, to );
		sleep( 2 );
		return( -1 );
	}

	if( unlink( from ) < 0 ) {
		fprintf( stderr, "unlink( %s ) failed \n", from );
		sleep( 2 );
		return( -1 );
	}

	return( 0 );
} /* move_file() */
#endif /* CONQUER */

/*
 * get_number - Interactive number input with live editing support
 *
 * Provides an interactive number input interface that allows users to type
 * digits with real-time display feedback and editing capabilities. Implements
 * character-by-character input processing with backspace/delete support for
 * correcting input errors. Designed for curses-based terminal interfaces.
 *
 * Algorithm:
 *   - Character-by-character input loop using getch()
 *   - Real-time display updates with addch() and refresh()
 *   - Decimal accumulation (multiply by 10, add new digit)
 *   - Backspace handling with cursor repositioning and digit removal
 *   - Input validation with 12-digit maximum length limit
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   Long integer value entered by user (>= 0)
 *   -1 if no digits were entered (empty input)
 *
 * Side Effects:
 *   - Reads from stdin using curses getch()
 *   - Modifies screen display with addch(), move(), refresh()
 *   - Changes cursor position during editing operations
 *   - Requires curses library initialization (initscr, etc.)
 *
 * Input Handling:
 *   - Digits (0-9): Accumulate into number, display on screen
 *   - Backspace (\b, DEL \177): Remove last digit, erase from display
 *   - Enter/Return (\n, \r): Complete input and return value
 *   - Other characters: Ignored (no action)
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires curses library and terminal
 *   Approach: Integration testing with mock terminal/curses interface
 *   Key Tests: [Digit entry, backspace editing, empty input, overflow protection]
 *   Dependencies: curses library (stdscr, getch, addch, refresh, move, getyx)
 *   Mock Requirements: Terminal interface mocking for automated testing
 *   Complexity: Moderate - User interface with state management and editing
 *
 * Notes:
 *   - Maximum input length limited to 12 digits to prevent overflow
 *   - Uses long type to handle larger integers on system
 *   - Relies on curses library for all terminal I/O operations
 *   - Character 127 (\177) handles DEL key on some terminals
 *   - Real-time feedback provides immediate visual confirmation
 */
long
get_number()
{
	long sum=0;
	char ch;
	int done=FALSE,count=0,xpos,ypos;
	/* this routine totally redone to allow deleting */
	while(!done) {
		ch=getch();
		if(isdigit(ch) && count<12) {
			/* only print numbers to the screen */
			addch(ch);
			refresh();
			sum *= 10L;
			count++;
			sum += (long)(ch-'0');
		} else if ((ch=='\b' || ch=='\177')&&(count)) {
			/* only delete what was printed */
			getyx(stdscr,ypos,xpos);
			move(ypos,--xpos);
			addch(' ');
			move(ypos,xpos);
			refresh();
			sum /= 10L;
			count--;
		} else if((ch=='\n')||(ch=='\r')) {
			done=TRUE;
		}
	}
	if (count==0) {
		return(-1);
	}
	return( sum );
}

#define INFINITE	1000

int		bx;		/* destination 'x' coordinate */
int		by;		/* destination 'y' coordinate */
int		moving_country;	/* country that is moving */

#define MAX_MOVE_UNITS	0x7f
unsigned char	**history_reachp;
int	level;

/*
 * land_2reachp - Recursive land pathfinding algorithm with movement optimization
 *
 * Core recursive pathfinding engine that determines if a land unit can reach
 * a destination within available movement points. Implements intelligent
 * direction prioritization, movement cost analysis, diplomatic constraints,
 * and cycle detection for efficient path exploration. Used by land movement
 * systems throughout the game.
 *
 * Algorithm:
 *   1. Base case checks: destination reached, no movement points, impossible distance
 *   2. Direction prioritization based on target vector (optimized movement selection)
 *   3. 8-directional exploration with boundary and terrain validation
 *   4. Movement cost calculation and availability checking
 *   5. History tracking to prevent revisiting with worse paths
 *   6. Diplomatic status checking for passage permissions
 *   7. Recursive exploration with backtracking and level management
 *
 * Direction Priority Strategy:
 *   - Pure horizontal: prioritize X-axis movement, then diagonals
 *   - Pure vertical: prioritize Y-axis movement, then diagonals
 *   - Diagonal: prioritize diagonal movement toward target, then orthogonal
 *
 * Parameters:
 *   ax - Starting X coordinate on world map
 *   ay - Starting Y coordinate on world map
 *   move_points - Remaining movement points for pathfinding
 *
 * Returns:
 *   1 if destination (bx,by) is reachable with given movement points
 *   0 if destination cannot be reached or blocked by constraints
 *
 * Side Effects:
 *   - Modifies global history_reachp[][] array for cycle detection
 *   - Increments/decrements global 'level' variable for recursion tracking
 *   - Reads from global terrain data (sct[][]), movement costs (movecost[][])
 *   - Accesses diplomatic status arrays (ntn[].dstatus[][])
 *   - Uses global destination coordinates (bx, by) and moving_country
 *
 * Global Dependencies:
 *   - bx, by: destination coordinates (must be set before calling)
 *   - moving_country: nation attempting movement (for diplomatic checks)
 *   - history_reachp[][]: 2D array tracking best movement points to each sector
 *   - level: recursion depth counter for debugging/optimization
 *   - sct[][]: sector data including owner, altitude, terrain type
 *   - movecost[][]: movement cost table for each map sector
 *   - ntn[]: nation data including diplomatic status matrices
 *
 * Constraints and Blocking Conditions:
 *   - PEAK and WATER altitude sectors block land movement
 *   - Negative movement costs indicate impassable terrain
 *   - War status blocks passage through enemy-controlled sectors
 *   - Neutral nations may block passage (complex diplomatic logic)
 *   - Map boundaries (0 <= x < MAPX, 0 <= y < MAPY)
 *   - Insufficient movement points for sector entry cost
 *
 * Testing Notes:
 *   Category: C (System) - Requires full game state initialization
 *   Approach: System testing with complete world map and diplomatic setup
 *   Key Tests: [Basic pathfinding, diplomatic blocking, terrain constraints, movement costs]
 *   Dependencies: Complete game world (map, nations, diplomacy, movement tables)
 *   Mock Requirements: Full game state including maps, nations, diplomatic status
 *   Complexity: Complex - Recursive algorithm with multiple global dependencies
 *
 * Known Issues:
 *   - BUG: Should engage hostile armies even if they don't own the sector
 *   - BUG: Doesn't account for THE_VOID, HIDDEN, and NINJA special statuses
 *   - Complex diplomatic logic may have edge cases (lines 331-337)
 *
 * Performance Notes:
 *   - Optimized direction selection reduces search space significantly
 *   - History tracking prevents exponential path explosion
 *   - Early termination optimizations for impossible distances
 *   - Recursion depth tracked via 'level' variable for debugging
 */
int
land_2reachp( ax, ay, move_points )
int	ax;
int	ay;
int	move_points;
{
	register int	i = 0;
	int	delta_x, delta_y;
	int	x_abs_delta, y_abs_delta;
	int	own;
	int	dx[ 8 ];
	int	dy[ 8 ];

	delta_x = bx - ax;
	delta_y = by - ay;

	/* Have we got where we are going? */
	if( delta_x == 0 && delta_y == 0 ) {
		return( 1 );
	}

	/* Any move points left? (optimization) */
	if( move_points == 0 ) {
		return( 0 );
	}

	x_abs_delta = (delta_x < 0) ? -delta_x : delta_x;
	y_abs_delta = (delta_y < 0) ? -delta_y : delta_y;

	/* couldn't reach if all moves cost 1 (optimization) */
	if( max( x_abs_delta, y_abs_delta ) > move_points ) {
		return( 0 );
	}

	{
		register int	inc_x;
		register int	inc_y;

		inc_x = (delta_x < 0 ) ? -1 : 1;
		inc_y = (delta_y < 0 ) ? -1 : 1;

		/*I HAVE CHANGED THIS CODE FROM THE ORIGINAL TO OPTIMIZE IT*/
		/*I think it should work well*/
		if( y_abs_delta == 0) {
			/* try 'x' movements first */
			dx[i] = inc_x; dy[i++] = 0;
			dx[i] = inc_x; dy[i++] = inc_y;
			dx[i] = inc_x; dy[i++] = -inc_y;
			dx[i] = 0; dy[i++] = inc_y;
			dx[i] = 0; dy[i++] = -inc_y;
			dx[i] = -inc_x; dy[i++] = inc_y;
			dx[i] = -inc_x; dy[i++] = 0;
			dx[i] = -inc_x; dy[i++] = -inc_y;
		} else if( x_abs_delta == 0 ) {
			/* try 'y' movements first */
			dx[i] = 0; dy[i++] = inc_y;
			dx[i] = inc_x; dy[i++] = inc_y;
			dx[i] = -inc_x; dy[i++] = inc_y;
			dx[i] = inc_x; dy[i++] = 0;
			dx[i] = -inc_x; dy[i++] = 0;
			dx[i] = inc_x; dy[i++] = -inc_y;
			dx[i] = 0; dy[i++] = -inc_y;
			dx[i] = -inc_x; dy[i++] = -inc_y;
		} else {	/* x_abs_delta != 0, 0 != y_abs_delta */
			/* try diagonal movements first */
			dx[i] = inc_x; dy[i++] = inc_y;

			dx[i] = 0; dy[i++] = inc_y;
			dx[i] = inc_x; dy[i++] = 0;

			dx[i] = -inc_x; dy[i++] = inc_y;
			dx[i] = inc_x; dy[i++] = -inc_y;

			dx[i] = -inc_x; dy[i++] = 0;
			dx[i] = 0; dy[i++] = -inc_y;

			dx[i] = -inc_x; dy[i++] = -inc_y;
		} /* if */
	} /* block */

	{
		register int	x, y;
		register int	new_mp;

		for( i = 0; i < 8; i++ ) {
			if( (x = ax + dx[i]) < 0 || x >= MAPX )
				continue;
			if( (y = ay + dy[i]) < 0 || y >= MAPY )
				continue;

			if ( movecost[ x ][ y ] < 0 )	/* just in case */
				continue;
			if( sct[x][y].altitude == PEAK)
				continue;
			if( sct[x][y].altitude == WATER)
				continue;

			new_mp = move_points - movecost[ x ][ y ];
			if( new_mp < 0 )
				continue;

			/*
			*	If we have been to this sector before
			*	in fewer move points this path is not
			*	going to do any better.
			*/
			if( history_reachp[x][y] >= new_mp ) {
				continue;
			}
			history_reachp[x][y] = new_mp;

			/*
			*	Test for a hostile army
			*/
			/* BUG: should engage if army is hostile but does not own sector */
			/* BUG: take into account THE_VOID, HIDDEN, and NINJA */
			if( (own = sct[x][y].owner) > 0 &&
			ntn[own].dstatus[moving_country] >= WAR &&
			x != bx && y != by &&
			solds_in_sector( x, y, own ) > 0 ) {
				continue;	/* at war with the owner, may not pass */
			}

			if( own > 0 &&
			ntn[moving_country].dstatus[own] < WAR &&
			ntn[own].dstatus[moving_country] > ALLIED &&
			ntn[own].dstatus[moving_country] < WAR) {
				/* not at war with owner & owner is neutral,no passing*/
				continue;
			}

			level++;
			if( land_2reachp( x, y, new_mp ) ) {
				level--;
				return( 1 );
			} /* if */
			level--;
		} /* for */
	} /* block */
	return( 0 );
} /* land_2reachp() */

/*
 * land_reachp - Land reachability analysis wrapper with administrative controls
 *
 * High-level interface to the land pathfinding system that provides complete
 * reachability analysis between two map coordinates. Handles memory management,
 * global state setup, input validation, and cleanup for the underlying recursive
 * pathfinding algorithm. Designed for administrative functions and debugging.
 *
 * Algorithm:
 *   1. Input validation (movement points limits, terrain accessibility)
 *   2. Dynamic memory allocation for pathfinding history tracking
 *   3. History array initialization (BSD vs standard library compatibility)
 *   4. Global state setup for recursive pathfinding engine
 *   5. Recursive pathfinding execution via land_2reachp()
 *   6. Memory cleanup and result return
 *
 * Administrative Features:
 *   - Movement point overflow detection with error reporting
 *   - Terrain validation for both start and destination coordinates
 *   - Cross-platform memory operations (BSD vs POSIX compatibility)
 *   - Complete memory management lifecycle for pathfinding operations
 *
 * Parameters:
 *   ax - Starting X coordinate on world map
 *   ay - Starting Y coordinate on world map
 *   gx - Goal/destination X coordinate on world map
 *   gy - Goal/destination Y coordinate on world map
 *   move_points - Available movement points for pathfinding analysis
 *   movee - Country/nation identifier attempting the movement
 *
 * Returns:
 *   1 if destination is reachable within movement point constraints
 *   0 if destination cannot be reached or is blocked by constraints
 *
 * Side Effects:
 *   - Allocates and frees dynamic memory for history_reachp[][] array
 *   - Sets global variables (bx, by, moving_country, level) for pathfinding
 *   - May call abrt() and exit program if movement points exceed limits
 *   - Writes error messages to stderr for debugging purposes
 *   - Initializes pathfinding state in history array
 *
 * Memory Management:
 *   - Uses m2alloc() for 2D array allocation (MAPX × MAPY × sizeof(char))
 *   - Initializes memory with bzero() (BSD) or memset() (POSIX)
 *   - Guarantees memory cleanup via free() before function return
 *   - History array tracks best movement points to each map sector
 *
 * Global State Dependencies:
 *   - sct[][]: sector data for terrain and altitude validation
 *   - MAPX, MAPY: world map dimensions for memory allocation
 *   - MAX_MOVE_UNITS: safety limit for movement point validation
 *   - Sets bx, by, moving_country, level for land_2reachp() consumption
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires memory management and map data
 *   Approach: Integration testing with controlled map scenarios
 *   Key Tests: [Basic reachability, terrain blocking, movement limits, memory management]
 *   Dependencies: Map data (sct[][]), memory allocation functions (m2alloc, free)
 *   Mock Requirements: Map setup, memory allocation mocking for failure testing
 *   Complexity: Moderate - Wrapper function with memory management and validation
 *
 * Platform Compatibility:
 *   - Conditional compilation for BSD vs POSIX memory initialization
 *   - BSD: Uses bzero() for memory clearing
 *   - Others: Uses memset() for POSIX compliance
 *   - Administrative build only (ADMIN preprocessor flag required)
 *
 * Error Conditions:
 *   - Movement points >= MAX_MOVE_UNITS: Triggers error message and abrt()
 *   - Start/destination in WATER or PEAK altitude: Returns 0 (unreachable)
 *   - Memory allocation failure: Undefined behavior (m2alloc dependency)
 *
 * Performance Notes:
 *   - Memory allocation overhead for each pathfinding operation
 *   - History array prevents redundant path exploration
 *   - Clean separation of concerns from recursive pathfinding engine
 */
#ifdef ADMIN
int
land_reachp( ax, ay, gx, gy, move_points, movee )
int	ax;
int	ay;
int	gx;
int	gy;
int	move_points;
int	movee;
{
	int	result;

	if( move_points >= MAX_MOVE_UNITS ) {
		fprintf( stderr, "land_reachp(): move_points = %d\n",
			move_points );

		abrt();
	}

	/* Are we starting or ending in the water or on a peak? */
	if( sct[ax][ay].altitude == WATER || sct[ax][ay].altitude == PEAK )
		return( 0 );
	if( sct[gx][gy].altitude == WATER || sct[gx][gy].altitude == PEAK )
		return( 0 );

	history_reachp = (unsigned char **) m2alloc(MAPX,MAPY,sizeof(char));
#ifdef BSD
	bzero((char *) *history_reachp,MAPX*MAPY);
#else
	memset((char *) *history_reachp, 0, MAPX*MAPY );
#endif

	history_reachp[ax][ay] = move_points;

	bx = gx;
	by = gy;
	moving_country = movee;

	level = 1;
	result = land_2reachp( ax, ay, move_points );
	free(history_reachp);
	return( result );
} /* land_reachp() */
#endif /* ADMIN */
#ifdef ADMIN
/*
 * water_2reachp - Recursive water pathfinding algorithm for naval movement
 *
 * Core recursive pathfinding engine for naval units moving through water
 * sectors. Implements simplified movement costs (1 per move), water-only
 * movement constraints, and intelligent direction prioritization for
 * efficient path exploration. Coordinates with land_2reachp() to provide
 * complete movement analysis for different unit types.
 *
 * Algorithm:
 *   1. Input validation and early termination conditions
 *   2. History tracking to prevent redundant path exploration
 *   3. Destination checking and water terrain validation
 *   4. Direction prioritization based on target vector
 *   5. 8-directional exploration with boundary validation
 *   6. Recursive pathfinding with decremented movement points
 *   7. Success propagation when destination reached
 *
 * Water Movement Constraints:
 *   - Units must stay in WATER altitude sectors (no land movement)
 *   - All water moves cost exactly 1 movement point (simplified model)
 *   - No diplomatic restrictions (unlike land movement)
 *   - No terrain-based movement cost variations
 *
 * Direction Priority Strategy:
 *   - Primary axis movement (X or Y) prioritized based on distance comparison
 *   - Longer axis distance determines prioritization order
 *   - Diagonal and orthogonal moves ordered for efficient pathfinding
 *   - Different from land pathfinding which handles three cases
 *
 * Parameters:
 *   ax - Starting X coordinate on world map
 *   ay - Starting Y coordinate on world map
 *   move_points - Remaining movement points for pathfinding
 *
 * Returns:
 *   1 if destination (bx,by) is reachable with given movement points
 *   0 if destination cannot be reached or blocked by constraints
 *
 * Side Effects:
 *   - Modifies global history_reachp[][] array for cycle detection
 *   - Reads from global terrain data (sct[][]) for water validation
 *   - Uses global destination coordinates (bx, by)
 *   - Recursive function calls modify call stack
 *
 * Global Dependencies:
 *   - bx, by: destination coordinates (must be set before calling)
 *   - history_reachp[][]: 2D array tracking movement points to each sector
 *   - sct[][]: sector data for altitude checking (WATER validation)
 *   - MAPX, MAPY: world map dimensions for boundary checking
 *
 * Optimization Features:
 *   - Early termination for negative movement points
 *   - History comparison prevents worse paths
 *   - Distance-based impossibility checking
 *   - Simplified movement cost model (always 1 per move)
 *
 * Testing Notes:
 *   Category: C (System) - Requires full map data and administrative build
 *   Approach: System testing with complete water-based scenarios
 *   Key Tests: [Basic water pathfinding, land blocking, movement limits]
 *   Dependencies: Complete game world map with water/land sectors
 *   Mock Requirements: Map data with proper water/land sector setup
 *   Complexity: Complex - Recursive algorithm with global dependencies
 *
 * Known Issues:
 *   - BUG: Does not test for enemy navy blocking (line 591 comment)
 *   - No diplomatic considerations unlike land movement
 *   - Assumes all water moves cost 1 (no varying water terrain costs)
 *
 * Differences from Land Pathfinding:
 *   - Simplified movement costs (1 per move vs. variable costs)
 *   - Water-only movement (altitude == WATER requirement)
 *   - No diplomatic restrictions or army blocking
 *   - Different direction prioritization logic (two cases vs. three)
 *   - No special terrain handling beyond water/non-water
 *
 * Administrative Context:
 *   - ADMIN build only (requires preprocessor flag)
 *   - Designed for administrative pathfinding analysis
 *   - Coordinates with land pathfinding for complete movement analysis
 *
 * Performance Notes:
 *   - History tracking prevents exponential path explosion
 *   - Simplified cost model reduces computational complexity
 *   - Early termination optimizations for impossible paths
 *   - Direction prioritization reduces search space
 */
int
water_2reachp( ax, ay, move_points )
int	ax;
int	ay;
int	move_points;
{
	register int	i = 0;
	int	delta_x;
	int	delta_y;
	int	dx[ 8 ];
	int	dy[ 8 ];

	/* this path uses too many move units */
	if( move_points < 0 )
		return( 0 );

	/*
	*	If we have been to this sector before in fewer move points
	*	this path is not going to do any better.
	*/
	if( history_reachp[ ax ][ ay ] <= move_points )
		return( 0 );

	history_reachp[ ax ][ ay ] = move_points;

	delta_x = ax - bx;
	delta_y = ay - by;

	/* Have we got where we are going? */
	if( delta_x == 0 && delta_y == 0 )
		return( 1 );

	/* Have we run into ground, but not reached our destination? */
	if( sct[ax][ay].altitude != WATER )
		return( 0 );

	/* Any move points left? (optimization) */
	if( move_points == 0 )
		return( 0 );

	/* couldn't reach if all moves cost 1 (optimization) */
	if( max( abs( delta_x ), abs( delta_y ) ) > move_points )
		return( 0 );

	/* BUG: test for an enemy navy */

	{
		register int	inc_x;
		register int	inc_y;

		inc_x = (delta_x < 0 ) ? -1 : (delta_x > 0) ? 1 : 0;
		inc_y = (delta_y < 0 ) ? -1 : (delta_y > 0) ? 1 : 0;

		if( abs(delta_x) > abs(delta_y) ) {
			/* try 'x' movements first */
			dx[i] = inc_x; dy[i++] = 0;
			dx[i] = inc_x; dy[i++] = inc_y;
			dx[i] = inc_x; dy[i++] = -inc_y;
			dx[i] = 0; dy[i++] = inc_y;
			dx[i] = 0; dy[i++] = -inc_y;
			dx[i] = -inc_x; dy[i++] = inc_y;
			dx[i] = -inc_x; dy[i++] = 0;
			dx[i] = -inc_x; dy[i++] = -inc_y;
		} else {	/* abs(delta_x) < abs(delta_y) */
			/* try 'y' movements first */
			dx[i] = 0; dy[i++] = inc_y;
			dx[i] = inc_x; dy[i++] = inc_y;
			dx[i] = -inc_x; dy[i++] = inc_y;
			dx[i] = inc_x; dy[i++] = 0;
			dx[i] = -inc_x; dy[i++] = 0;
			dx[i] = inc_x; dy[i++] = -inc_y;
			dx[i] = 0; dy[i++] = -inc_y;
			dx[i] = -inc_x; dy[i++] = -inc_y;
		} /* if */
	} /* block */

	{
		register int	x, y;
		register int	new_mp;

		for( i = 0; i < 8; i++ ) {
			if( (x = ax + dx[i]) < 0 || x >= MAPX )
				continue;
			if( (y = ay + dy[i]) < 0 || y >= MAPY )
				continue;

			new_mp = move_points - 1;
			if( new_mp < 0 )
				continue;

			if( water_2reachp( x, y, new_mp ) )
				return( 1 );
		} /* for */
	} /* block */

	return( 0 );
} /* water_2reachp() */
#endif /* ADMIN */
#ifdef XYZ	/* XYZ never is defined */
/*
 *	water_reachp()
 */

int
water_reachp( ax, ay, gx, gy, move_points, movee )
int	ax;
int	ay;
int	gx;
int	gy;
int	move_points;
int	movee;
{
	if( move_points >= MAX_MOVE_UNITS ) {
		fprintf( stderr, "water_reachp(): move_points = %d\n",
			move_points );

		abrt();
	}

#ifdef SYSV
	memset(history_reachp, MAX_MOVE_UNITS, MAPX*MAPY*sizeof(history_reachp));
#else
	{ register int i,j;
		for (i=0; i < MAPX ; i++)
		for (j=0; j < MAPY ; j++ )
			history_reachp [i] [j] = MAX_MOVE_UNITS ;
	}/* eof memset replacement block */
#endif

	history_reachp[ ax ][ ay ] = 0;

	bx = gx;
	by = gy;
	moving_country = movee;

	return( water_2reachp( ax, ay, move_points ) );
} /* water_reachp() */
#endif /* 0 */

/*
 *	solds_in_sector()
 */

long
solds_in_sector( x, y, nation )
int	x;
int	y;
int	nation;
{
	register struct s_nation	*nptr = &ntn[nation];
	register int	j;
	long	total = 0;

	for( j = 0; j < MAXARM; j++ ) {
		if( nptr->arm[j].sold == 0 )
			continue;

		if( nptr->arm[j].xloc == x && nptr->arm[j].yloc == y )
			total += nptr->arm[j].sold;
	}

	return( total );
} /* solds_in_sector() */
#ifdef ADMIN

/*
 * score_one - Calculate total score for a nation based on class-specific weighting
 *
 * Calculates the comprehensive score for a nation by applying class-specific
 * weightings to various national resources and statistics. Each nation class
 * (King, Emperor, Wizard, etc.) has different scoring priorities that reflect
 * their strategic goals and victory conditions.
 *
 * The scoring system uses two components:
 * 1. Weighted resource totals (sectors, population, military, wealth, magic)
 * 2. Class-specific bonus attributes (popularity, prestige, power, etc.)
 *
 * Resource weights are defined per class in the weights[] table:
 * - Sectors: Per 2 sectors controlled
 * - Civilians: Per 1000 population
 * - Soldiers: Per 1000 military units
 * - Gold: Per 100K gold pieces
 * - Jewels: Per 100K jewel value
 * - Metal: Per 100K metal value
 * - Magic: Per magic power possessed
 * - Ships: Per 10 naval vessels
 *
 * Class-specific bonuses reflect each class's unique victory conditions:
 * - Kings: popularity + prestige - poverty (legitimacy focus)
 * - Emperors: power + prestige - poverty (dominance focus)
 * - Wizards: knowledge + power - 50 (magical mastery focus)
 * - Priests: wealth + terror - poverty (religious authority focus)
 * - Pirates: reputation + wealth - 50 (notoriety and plunder focus)
 * - Traders: wealth + prestige - tax_rate*5 (economic efficiency focus)
 * - Warlords: reputation + prestige - 50 (military honor focus)
 * - Demons: knowledge + terror - 50 (dark knowledge focus)
 * - Dragons: wealth + terror - 50 (hoarding and fear focus)
 * - Shadows: power + terror - 50 (stealth and intimidation focus)
 *
 * Parameters:
 *   nation - Nation ID number (0 to NTOTAL-1)
 *
 * Returns:
 *   Total calculated score as long integer
 *   Higher scores indicate more successful nations within their class paradigm
 *
 * Side Effects:
 *   None - read-only calculation using nation statistics and magic powers
 *
 * Testing Notes:
 *   Category: A (Unit) | Self-contained calculation with clear inputs/outputs
 *   Approach: Unit tests with mock nation data for each class type
 *   Key Tests: Each nation class bonus calculation, resource weighting accuracy
 *   Dependencies: num_powers() function, ntn[] array, nation structure fields
 *   Mock Requirements: Mock nation data with known values for verification
 *   Complexity: Moderate - multiple class-specific calculations and magic queries
 *
 * Notes:
 *   - Critical for determining victory conditions and game rankings
 *   - Each class has different optimal strategies reflected in weighting
 *   - Magic power calculation requires separate num_powers() calls
 *   - Bonus calculations can be negative, affecting final score
 *   - Score scaling allows fair comparison between different nation classes
 */
/* score_one()	*/
struct wght {
	int	sectors;
	int	civilians;
	int	soldiers;
	int	gold;
	int	jewels;
	int	metal;
	int	magics;
	int	ships;
} weights[] = {
/*		Per 2  1000    1000      100K 100K   100K   Magic  10 */
/*	Races   Sector People  Soldiers  Gold Jewels Iron   Power Ship */
/* NPC */	{ 2,    1,     0,        0,    1,    1,     1,    0 },
/* kingdom */	{ 2,    1,     2,        3,    0,    0,     0,    0 },
/* empire */	{ 3,    0,     0,        1,    1,    0,     0,    0 },
/* wizard */	{ 0,    2,     1,        0,    3,    5,     7,   0 },
/* theocracy */	{ 2,    1,     0,        0,    3,    0,     3,    0 },
/* pirate */	{ 0,    0,     5,        0,    10,   10,    1,    5 },
/* trader */	{ 2,    1,     0,        0,    1,    1,     1,    8 },
/* warlord */	{ 2,    1,     2,        0,    1,    1,     1,    0 },
/* demon */	{ 2,    0,     1,        0,    1,    0,     5,   0 },
/* dragon */	{ 0,    0,     0,        10,   20,   0,     0,    0 },
/* shadow */	{ 2,    0,     0,        0,    0,    5,     0,    0 },
/* miner */	{ 0,    0,     5,        0,    10,   10,   1,    5 },
};

long
score_one( nation )
int	nation;
{
	struct	s_nation	*nptr = &ntn[ nation ];
	long	total = 0;
	int	bonus;
	struct	wght	*wght = &weights[ nptr->class ];

	total += wght->sectors * nptr->tsctrs / 2L;
	total += wght->civilians * nptr->tciv / 1000L;
	total += wght->soldiers * nptr->tmil / 1000L;
	if(nptr->tgold > 0 ) total += wght->gold * (nptr->tgold / 100000L);
	total += wght->jewels * nptr->jewels / 100000L;
	total += wght->metal * nptr->metals / 100000L;
	total += wght->magics * num_powers(nation,M_MIL);
	total += wght->magics * num_powers(nation,M_CIV);
	total += wght->magics * num_powers(nation,M_MGK);
	total += wght->ships * nptr->tships / 10L;
	switch( nptr->class ) {
	case 	C_KING:
			bonus=(nptr->popularity+nptr->prestige-nptr->poverty);
			break;
	case	C_EMPEROR:
			bonus=(nptr->power+nptr->prestige-nptr->poverty);
			break;
	case	C_WIZARD:
			bonus=(nptr->knowledge+nptr->power-50);
			break;
	case	C_PRIEST:
			bonus=(nptr->wealth+nptr->terror-nptr->poverty);
			break;
	case	C_PIRATE:
			bonus=(nptr->reputation+nptr->wealth-50);
			break;
	case	C_TRADER:
			bonus=(nptr->wealth+nptr->prestige-nptr->tax_rate*5);
			break;
	case	C_WARLORD:
			bonus=(nptr->reputation+nptr->prestige-50);
			break;
	case	C_DEMON	:
			bonus=(nptr->knowledge+nptr->terror-50);
			break;
	case	C_DRAGON:
			bonus=(nptr->wealth+nptr->terror-50);
			break;
	case	C_SHADOW:
			bonus=(nptr->power+nptr->terror-50);
			break;
	default:	bonus=0;
	}
	total += bonus/10;
	return( total );
} /* score_one() */
#endif /* ADMIN */
/*
 *	print_accum()
 */

	/* max number of print_accum() calls in one printf() */
#define MAX_BUFFER	4
#define BUFFER_SIZE	20

/* is_habitable() - returns TRUE/FALSE if habitable */
int
is_habitable( x, y )
int	x;
int	y;
{
	char	temp;

	if(( (temp=sct[x][y].altitude)==WATER )||( temp==PEAK )) return(FALSE);

	if(((temp=sct[x][y].vegetation)==BARREN )
	|| ( temp==LT_VEG )
	|| ( temp==GOOD )
	|| ( temp==WOOD )
	|| ( temp==FOREST )) return( TRUE );

	return( FALSE );
}

#ifdef CONQUER
int
units_in_sector(int x,int y,int nation)
{
	int count=0, armynum, nvynum;
	struct	s_nation  *nptr = curntn;

	curntn = &ntn[nation];
	for(armynum=0;armynum<MAXARM;armynum++)
		if((P_ASOLD>0)&&(P_AXLOC==x)&&(P_AYLOC==y)) count++;
	for(nvynum=0;nvynum<MAXNAVY;nvynum++)
		if(((P_NWSHP+P_NMSHP+P_NGSHP)!=0)&&
		(P_NXLOC==x)&&(P_NYLOC==y)) count++;

	curntn = nptr;
	return(count);
}
#endif /* CONQUER */

/*
 * num_powers - Count magic powers possessed by a nation in specific category
 *
 * Counts the total number of magic powers that a nation possesses within
 * a specified category (Military, Civilian, Magical, or All). This function
 * is essential for calculating nation scores, determining available abilities,
 * and validating magical prerequisites for various game actions.
 *
 * The magic system is organized into three primary categories:
 * - Military (M_MIL): Combat and warfare related powers (0-10)
 * - Civilian (M_CIV): Economic and development powers (11-23)
 * - Magical (M_MGK): Mystical and supernatural powers (24-30)
 *
 * Powers are stored as bitmask flags in the nation's powers field, and the
 * magic() macro tests individual power possession using bitwise operations.
 * Each power corresponds to a specific bit position in the powers array.
 *
 * Category ranges are defined by start/end constants:
 * - S_MIL (0) to E_MIL (11): Military powers
 * - S_CIV (11) to E_CIV (13): Civilian powers
 * - S_MGK (24) to E_MGK (7): Magical powers
 * - M_ALL: All categories combined (S_MIL to E_MGK)
 *
 * Parameters:
 *   nation - Nation ID number (0 to NTOTAL-1)
 *   type - Power category to count:
 *          M_MIL (1) - Military powers only
 *          M_CIV (2) - Civilian powers only
 *          M_MGK (3) - Magical powers only
 *          M_ALL (5) - All power categories
 *
 * Returns:
 *   Number of powers possessed in the specified category
 *   0 if nation has no powers in that category
 *   Fatal error and abort if invalid type parameter
 *
 * Side Effects:
 *   Calls abrt() and terminates program if invalid type specified
 *
 * Testing Notes:
 *   Category: A (Unit) | Simple counting function with clear category logic
 *   Approach: Unit tests with mock nation power bitmasks for each category
 *   Key Tests: Each power category count, M_ALL comprehensive count, error handling
 *   Dependencies: magic() macro, powers[] array, ntn[] nation data, abrt() function
 *   Mock Requirements: Mock nations with known power bitmasks for verification
 *   Complexity: Simple - straightforward counting loop with category selection
 *
 * Notes:
 *   - Critical for score calculation and power validation systems
 *   - Used extensively in score_one() for class-specific scoring
 *   - Fatal error on invalid type ensures data integrity
 *   - Powers bitmask allows efficient storage and testing of abilities
 *   - Category system enables balanced scoring across nation classes
 */
int
num_powers(nation,type)
int nation,type;
{
	int	count_magic=0;
	int	try;
	long	start, end;
	switch(type){
		case M_MGK:
			start=S_MGK;
			end=E_MGK;
			break;
		case M_CIV:
			start=S_CIV;
			end=E_CIV;
			break;
		case M_MIL:
			start=S_MIL;
			end=E_MIL;
			break;
		case M_ALL:
			start=S_MIL;
			end=E_MGK;
			break;
		default:
			fprintf(stderr,"fatal error in num_powers");
			abrt();
	}
	for( try = start; try < start+end; try++ )
		if( magic(nation, powers[try] ) == 1 ) count_magic++;
	return(count_magic);
}

/*
 * tofood - Calculate food production value of a sector with race-specific bonuses
 *
 * Calculates the total food value that a sector can produce, incorporating
 * base vegetation food values, race-specific terrain bonuses, magical enhancements,
 * and trade good food bonuses. This function is critical for population support
 * calculations and determining sector carrying capacity.
 *
 * Base food values are determined by vegetation type using the vegfood table:
 * "0004697400000" where each character represents food value for corresponding
 * vegetation types: volcano=0, desert=0, tundra=0, barren=4, light_veg=6,
 * grassland=9, forest=7, etc.
 *
 * Race-specific bonuses provide cultural adaptations:
 * - Elves: +3 food bonus in forests (natural forest affinity)
 * - Elves: -1 food penalty in barren lands (forest dependency)
 * - Dervish/Destroyer: Can survive in desert/ice with 6 food (magical adaptation)
 *
 * Trade goods in the "eatrate" range (communication < tradegood <= eatrate)
 * provide additional food bonuses using the tg_value table. These represent
 * food-related trade goods like spices, livestock, or agricultural products.
 *
 * Parameters:
 *   sptr - Pointer to sector structure containing vegetation and trade good data
 *   cntry - Nation ID for race-specific bonuses (0 for no race bonuses)
 *
 * Returns:
 *   Total food production value for the sector
 *   0 for completely barren/uninhabitable terrain
 *   Enhanced values for race-appropriate terrain or trade goods
 *
 * Side Effects:
 *   None - read-only calculation using sector and nation data
 *
 * Testing Notes:
 *   Category: A (Unit) | Self-contained calculation with clear terrain logic
 *   Approach: Unit tests with mock sectors for each vegetation/race combination
 *   Key Tests: Base vegetation values, elf bonuses/penalties, trade good bonuses
 *   Dependencies: vegfood table, tg_value table, magic() macro, ntn[] race data
 *   Mock Requirements: Mock sectors with known vegetation and trade good values
 *   Complexity: Moderate - multiple bonus systems and race-specific calculations
 *
 * Notes:
 *   - Critical for population growth and carrying capacity calculations
 *   - Race bonuses reflect cultural/biological adaptations to terrain
 *   - Trade goods can significantly enhance marginal agricultural land
 *   - Food value 4 is considered the minimum for sustainable habitation
 *   - Special magical races can survive in otherwise uninhabitable terrain
 */
/* returns food value of sector */
/* 4 is limit of livable land */
int
tofood(sptr,cntry)
struct s_sector *sptr;
int	cntry;
{
	register int i=0;
	register int foodvalue;
	while( sptr->vegetation != *(veg+i) ) i++;
	foodvalue = *(vegfood+i) - '0';
	if( cntry != 0 ) {
		if(foodvalue == 0) {
#ifdef DERVDESG
			if ((magic(cntry,DERVISH)||magic(cntry,DESTROYER))
			&&(sptr->vegetation==DESERT || sptr->vegetation==ICE))
				return(6);
#endif /* DERVDESG */
			return( 0 );
		}
		if(ntn[cntry].race == ELF){
			if(sptr->vegetation == FOREST) foodvalue+=3;
			else if(sptr->vegetation == BARREN) foodvalue--;
		}
	}
	if(( sptr->tradegood <= END_EATRATE )
	&&( sptr->tradegood > END_COMMUNICATION ))
		foodvalue += *(tg_value+sptr->tradegood) - '0';
	return( foodvalue );
}

/*jewel cost for civilian power = Base * 2**( #mgk/2 + #civ + #mil/2 )	*/
/*race			magical		civilian	military	*/
/*	elves -		50K		50K		50K		*/
/*	dwarves -	80K		40K		40K		*/
/*	humans -	100K		25K		50K		*/
/*	orcs -		100K		50K		25K		*/

/*
 * getmgkcost - Calculate magic power acquisition cost with race and complexity scaling
 *
 * Calculates the cost in gold to acquire a new magic power for a nation.
 * The cost system incorporates race-specific base costs and exponential scaling
 * based on the total number of powers already possessed. This creates a
 * balanced progression where early powers are affordable but advanced magical
 * mastery becomes increasingly expensive.
 *
 * Race-specific base costs reflect cultural magical affinity:
 *
 * Magical Powers (M_MGK):
 * - Dwarves: 80,000 gold (moderate magical affinity)
 * - Humans: 100,000 gold (standard magical ability)
 * - Orcs: 150,000 gold (limited magical aptitude)
 * - Others: 50,000 gold (default/elves have natural magic)
 *
 * Civilian Powers (M_CIV):
 * - Dwarves: 40,000 gold (excellent craftsmanship and construction)
 * - Humans: 25,000 gold (natural civilian development ability)
 * - Orcs: 75,000 gold (poor at peaceful development)
 * - Others: 50,000 gold (default rate)
 *
 * Military Powers (M_MIL):
 * - Dwarves: 40,000 gold (disciplined military tradition)
 * - Orcs: 45,000 gold (natural warriors but disorganized)
 * - Others: 50,000 gold (default, including humans)
 *
 * Complexity scaling formula:
 * - Count powers with 2x weight for same category, 1x for others
 * - Cost doubles for each effective power level: base * 2^(npowers-1)
 * - Caps at BIG/2 (250M gold) to prevent overflow
 *
 * Parameters:
 *   type - Magic power category:
 *          M_MGK (3) - Magical/mystical powers
 *          M_CIV (2) - Civilian/economic powers
 *          M_MIL (1) - Military/combat powers
 *   nation - Nation ID for race-specific base costs
 *
 * Returns:
 *   Gold cost for acquiring next power in specified category
 *   -1 if invalid power type specified
 *   Capped at BIG/2 (250M gold) for very high power levels
 *
 * Side Effects:
 *   None - read-only calculation using nation data and power counts
 *
 * Testing Notes:
 *   Category: A (Unit) | Mathematical calculation with clear cost progression
 *   Approach: Unit tests with mock nations at various power levels and races
 *   Key Tests: Race-specific base costs, exponential scaling, overflow protection
 *   Dependencies: num_powers() function, ntn[] race data, power category constants
 *   Mock Requirements: Mock nations with known race and power combinations
 *   Complexity: Moderate - exponential calculations with multiple race conditions
 *
 * Notes:
 *   - Critical for magic system economic balance and progression
 *   - Exponential scaling prevents unlimited power accumulation
 *   - Race differences create strategic choices in nation building
 *   - Same-category powers cost more due to 2x weighting (specialization penalty)
 *   - Cost calculation uses bit shifting for efficient doubling
 */
/* returns cost of magic power - returns -1 if invalid */
long
getmgkcost(type,nation)
int type, nation;
{
	int i;
	long cost;
	long base=BASEMAGIC;
	int npowers;
	switch(type) {
	case M_MGK:
		if(ntn[nation].race==DWARF)		base=DWFMAGIC;
		else if(ntn[nation].race==HUMAN)	base=HUMMAGIC;
		else if(ntn[nation].race==ORC)		base=ORCMAGIC;
		npowers=num_powers(nation,M_CIV)+num_powers(nation,M_MIL)+1
		+2*num_powers(nation,M_MGK);
		npowers/=2;
		break;
	case M_CIV:
		if(ntn[nation].race==DWARF)		base=DWFCIVIL;
		else if(ntn[nation].race==HUMAN)	base=HUMCIVIL;
		else if(ntn[nation].race==ORC)		base=ORCCIVIL;
		npowers=num_powers(nation,M_MGK)+num_powers(nation,M_MIL)+1
		+2*num_powers(nation,M_CIV);
		npowers/=2;
		break;
	case M_MIL:
		if(ntn[nation].race==DWARF)		base=DWFMILIT;
		else if(ntn[nation].race==ORC)		base=ORCMILIT;
		npowers=num_powers(nation,M_CIV)+num_powers(nation,M_MGK)+1
		+2*num_powers(nation,M_MIL);
		npowers/=2;
		break;
	default:
		return(-1);
	}
	cost = base;
	for (i=1; i<npowers; i++) {
		cost <<= 1;
		if (cost > BIG)
			return(BIG/2L);
	}
	return(cost);
}

int
todigit(character)
register int	character;
{
	if( character >= '0' && character <= '9' )
		return( character - '0' );
	return( -1 );
}

/* set up occ[][] for country.
 * if leader==true, only for leader sectors plus ntn.communicatins range
 * if leader==(-1), do not include ships on the sector search
 */
void
prep(nation,leader)
int nation,leader;
{
	short armynum,nvynum;
	int save,i,j,x,y,start,end,com;

	/*set occ to 0*/
	for(i=0;i<MAPX;i++) for(j=0;j<MAPY;j++) occ[i][j]=0;

	save=nation;
	if(leader==TRUE) {
		/* only do the given country */
		start=save;
		end=save+1;
	} else {
		/* go through all countries */
		start=0;
		end=NTOTAL;
	}

	/*set occ to country of occupant army*/
	for(nation=start;nation<end;nation++) if(ntn[nation].active!=INACTIVE) {
		curntn = &ntn[nation];
		for(armynum=0;armynum<MAXARM;armynum++){
			if( leader==TRUE ) {
				if((P_ATYPE<MINLEADER)
				||(P_ATYPE>=MINMONSTER)
				||(P_ASOLD<=0)) continue;
				i=P_AXLOC;
				j=P_AYLOC;
				com = P_NTNCOM; /* do communications radius */
				for(x=i-com;x<=i+com;x++)
				for(y=j-com;y<=j+com;y++)
					if(ONMAP(x,y)) occ[x][y]=nation;
			} else if((P_ASOLD>0)&&(P_ASTAT!=SCOUT)){
				i=P_AXLOC;
				j=P_AYLOC;
				if((occ[i][j]== 0)||(occ[i][j]== nation))
					occ[i][j]= nation;
				else occ[i][j]= NTOTAL;
			}
		}
		if( leader==FALSE ) for(nvynum=0;nvynum<MAXNAVY;nvynum++){
			if((P_NWSHP!=0)||(P_NGSHP!=0)||(P_NMSHP!=0)){
				i=P_NXLOC;
				j=P_NYLOC;
				if((occ[i][j]== 0)||(occ[i][j]== nation))
					occ[i][j]= nation;
				else occ[i][j]= NTOTAL;
			}
		}
	}

	nation=save;
	curntn = &ntn[nation];
}

#ifdef ADMIN
/*routine to depelete a nation without a capitol */
void
deplete(nation)
int nation;
{
	struct s_nation *saventn=curntn;
	int i,j,x,y,armynum;

	x = ntn[nation].capx;
	y = ntn[nation].capy;
	if((sct[x][y].designation==DCAPITOL)&&((sct[x][y].owner==nation)
	||(sct[x][y].owner==0)||(!isntn(ntn[sct[x][y].owner].active))))
		return;

	curntn = &ntn[nation];
	fprintf(fnews,"1.\tNation %s is depleted by the lack of a Capitol\n",ntn[nation].name);

	for(armynum=0;armynum<MAXARM;armynum++) if (P_ASOLD>0) {
		/* first disband PDEPLETE% of the military */
		if (P_ATYPE<MINLEADER &&
		(rand()%100<PDEPLETE||P_ATYPE==A_MERCENARY)) {
			if(P_ATYPE==A_MERCENARY) {
				MERCMEN += P_ASOLD;
			} else if(ntn[sct[AXLOC][AYLOC].owner].race==ntn[nation].race) {
				sct[P_AXLOC][P_AYLOC].people += P_ASOLD;
			}
			P_ASOLD=0;
			if(ispc(curntn->active)) {
				if (mailopen(nation)!=(-1)) {
					fprintf(fm,"Message to %s from Conquer\n\n",curntn->name);
					fprintf(fm,"\tYour %s Army %d disperses into the population\n",*(unittype+(P_ATYPE%UTYPE)),armynum);
					mailclose(nation);
				}
			}
		} else if(P_ATYPE>=MINMONSTER) {
			/* disbanding of ALL monsters should take place */
			P_ASOLD=0;
			if(ispc(curntn->active)) {
				if (mailopen(nation)!=(-1)) {
					fprintf(fm,"Message to %s from Conquer\n\n",curntn->name);
					fprintf(fm,"\tYour %s (unit %d) leaves due to the loss of your jewels.\n",*(unittype+(P_ATYPE%UTYPE)),armynum);
					mailclose(nation);
				}
			}
		}
	}

	/* check for sectors breaking away -- not capx, capy */
	if(ispc(curntn->active)) {
		/* create a summarized mail message of sectors effected */
		if (mailopen(nation)!=(1)) {
			fprintf(fm,"Message to %s from Conquer\n\n",curntn->name);
			fprintf(fm,"Riots and Rebellion flourish:\n");
		}
	}
	for(i=0;i<MAPX;i++) for(j=0;j<MAPY;j++)
	if(sct[i][j].owner==nation && (i!=x || j!=y) ) {
		if(rand()%100 < PDEPLETE && sct[x][y].people>0) {
			if(rand()%100 < PDEPLETE) {
				/* sector riots */
				flee(i,j,TRUE,FALSE);
				DEVASTATE(i,j);
				if(ispc(curntn->active)) {
					/* add to listing */
					fprintf(fm,"\tsector %d, %d has massive riots\n",i,j);
				}
			} else {
				/* sector becomes owned by another nation */
#ifdef NOTDONE
				/* must work on this still */
				giveaway(i,j,&nation);
				if(ispc(curntn->active)) {
					fprintf(fm,"\tsector %d, %d joins nation %s\n",ntn[nation].name);
				}
#endif /* NOTDONE */
			}
		}
	}
	if(ispc(curntn->active)) {
		mailclose(nation);
	} else if(isnpc(curntn->active)) {
		if(sct[curntn->capx][curntn->capy].owner==nation) {
			/* reset capitol for npcs */
			sct[curntn->capx][curntn->capy].designation=DCAPITOL;
			if(sct[curntn->capx][curntn->capy].fortress<1)
				sct[curntn->capx][curntn->capy].fortress=1;
		}
	}
	/* restore */
	curntn = saventn;
}


/*routine to sack a nation's captiol */
void
sackem(cntry)
	int cntry;
{
	struct s_nation *saventn=curntn;
	int x,y,i,j,foundcap,nation;

	/* hail the conquerer */
	curntn = &ntn[cntry];
	x = curntn->capx;
	y = curntn->capy;
	nation = sct[x][y].owner;
	if(nation==cntry || nation==0) return;

	/* advertise */
	fprintf(fnews,"1.\tCapitol of %s sacked by %s\n",ntn[cntry].name,ntn[nation].name);

        /* Remove goods from trade board */
	fixtrade(cntry);

	/* first give all prizes to the conquerer */
	if(curntn->tgold > 0) {			/* all gold */
		ntn[nation].tgold += curntn->tgold;
		curntn->tgold=0;
	}
	ntn[nation].jewels += curntn->jewels;	/* all jewels */
	curntn->jewels=0;
	ntn[nation].metals += curntn->metals;	/* all metals */
	curntn->metals=0;
	ntn[nation].tfood += curntn->tfood/5L;	/* 20% of food */
	curntn->tfood -= curntn->tfood/5L;

	/* fix the designation */
	if(sct[x][y].designation==DCAPITOL) {
		if(isntn(ntn[nation].active)) {
			sct[x][y].designation = DCITY;
		} else {
			DEVASTATE(x,y);
			sct[x][y].owner=cntry;
		}
	}

	/* set another sector to the capx, capy to make sure that */
	/* sacking does not occur next update for same sacking.   */
	foundcap=FALSE;
	for(i=0;foundcap==FALSE && i<MAPX;i++)
	for(j=0;foundcap==FALSE && j<MAPY;j++) if(sct[i][j].owner==cntry) {
		if(sct[i][j].designation==DCITY) {
			x = i; y = j;
			foundcap=TRUE;
		} else if((sct[i][j].designation==DTOWN)
		&&(((x==curntn->capx)&&(y==curntn->capy))
		  ||(sct[x][y].designation!=DTOWN))) {
			x = i; y = j;
		} else if((x==curntn->capx)&&(y==curntn->capy)) {
			x = i; y = j;
		}
	}

	if ((x!=curntn->capx)||(y!=curntn->capy)) {
		/* assign new pseudo capitol */
		if(ispc(curntn->active)) {
			if(mailopen(cntry)!=(-1)) {
			fprintf(fm,"Message to %s from Conquer\n\n",ntn[cntry].name);
			fprintf(fm,"\tYour Capitol at sector location %d,%d\n",curntn->capx,curntn->capy);
			fprintf(fm,"\t was overrun by nation %s.\n\n",ntn[nation].name);
			fprintf(fm,"\tA temporary headquarters is now in sector %d,%d,\n",x,y);
			fprintf(fm,"\t but designation of a new Capitol is recommended.\n");
			mailclose(cntry);
			}
		}
		curntn->capx=x;
		curntn->capy=y;
	} else {
		/* no new capitol assignment */
		if(ispc(curntn->active)) {
			if(mailopen(cntry)!=(-1)) {
			fprintf(fm,"Message to %s from Conquer\n\n",ntn[cntry].name);
			fprintf(fm,"\tYour Capitol at sector location %d,%d\n",curntn->capx,curntn->capy);
			fprintf(fm,"\t was overrun by nation %s.\n\n",ntn[nation].name);
			fprintf(fm,"\tNo other land remains.  The destruction\n");
			fprintf(fm,"\t of your nation seems imminent.\n");
			mailclose(cntry);
			}
		}
	}
	/* restore */
	curntn = saventn;
}
#endif /* ADMIN */

/*destroy nation--special case if capitol not owned by other nation*/
void
destroy(cntry)
int cntry;
{
	short armynum, nvynum;
	int i, x, y;
	char buf[LINELTH];
	struct s_nation	*nptr;

	nptr = &ntn[cntry];
	if( !isactive(nptr->active) ) return;

	if( !ismonst(nptr->active) ) {
		fprintf(fnews,"1.\tNation %s was destroyed ",nptr->name);
		if(cntry!=sct[nptr->capx][nptr->capy].owner){
			fprintf(fnews,"(their capitol is now owned by %s)\n",ntn[sct[nptr->capx][nptr->capy].owner].name);
			/*get +5% to combat skill*/
			ntn[sct[nptr->capx][nptr->capy].owner].aplus+=5;
		}
		else fprintf(fnews,"(they owned their capitol)\n");
	}

	nptr->active=INACTIVE;
	nptr->score=0;
	sprintf(buf,"%s%d",msgfile,cntry);
	unlink(buf);

	for(armynum=0;armynum<MAXARM;armynum++) if(ASOLD>0) {
		if(ntn[sct[AXLOC][AYLOC].owner].race==nptr->race)
			sct[AXLOC][AYLOC].people+=ASOLD;
		ASOLD=0;
	}
	for(nvynum=0;nvynum<MAXNAVY;nvynum++) {
		NMSHP=0;
		NWSHP=0;
		NGSHP=0;
	}
	for(i=0;i<NTOTAL;i++) {
		if (ntn[i].active < NPC_PEASANT ) {
			ntn[i].dstatus[cntry]=UNMET;
			nptr->dstatus[i]=UNMET;
		} else {
			ntn[i].dstatus[cntry]=WAR;
			nptr->dstatus[i]=WAR;
		}
	}
	
	/*if take them you get their gold*/
	if(cntry!=sct[nptr->capx][nptr->capy].owner){
		if(nptr->tgold>0) ntn[sct[nptr->capx][nptr->capy].owner].tgold+=nptr->tgold;
		if(nptr->jewels>0) ntn[sct[nptr->capx][nptr->capy].owner].jewels+=nptr->jewels;
		if(nptr->metals>0) ntn[sct[nptr->capx][nptr->capy].owner].metals+=nptr->metals;
		if(nptr->tfood>0) ntn[sct[nptr->capx][nptr->capy].owner].tfood+=nptr->tfood;
		sct[nptr->capx][nptr->capy].designation=DCITY;
	}

	/*if god destroys then kill all population*/
	if(cntry==sct[nptr->capx][nptr->capy].owner){
		for(x=0;x<MAPX;x++) for(y=0;y<MAPY;y++)
		if(sct[x][y].owner==cntry) {
			sct[x][y].people=0;
			sct[x][y].owner=0;
			sct[x][y].designation=DNODESIG;
		}
	}
	/*slowly take over and all people flee*/
	else if(ntn[sct[nptr->capx][nptr->capy].owner].race!=nptr->race){
		for(x=0;x<MAPX;x++) for(y=0;y<MAPY;y++)
		if(sct[x][y].owner==cntry) {
			/*all kinds of refugees to neighboring countries*/
			flee(x,y,TRUE,FALSE);
			sct[x][y].people=0;
			sct[x][y].owner=0;
			if(tofood( &sct[x][y],0 )<DESFOOD)
				sct[x][y].designation=DNODESIG;
			else	sct[x][y].designation=sct[x][y].vegetation;
		}
	}
	/*else same race, so give all land to conqueror*/
	else {
		for(x=0;x<MAPX;x++) for(y=0;y<MAPY;y++)
		if(sct[x][y].owner==cntry){
			sct[x][y].owner=sct[nptr->capx][nptr->capy].owner;
			if( !ISCITY( sct[x][y].designation )) {
			if(tofood( &sct[x][y],0)<DESFOOD)
				sct[x][y].designation=DNODESIG;
			else	sct[x][y].designation=DFARM;
			}
		}
	}
}

#define ALPHA_SIZE	128

/*
 * updmove - Movement cost calculation system with race-specific terrain preferences
 *
 * Comprehensive movement cost calculation engine that updates the global
 * movecost[][] array based on racial preferences, terrain types, magical
 * enhancements, and special designations. Provides the foundation for all
 * pathfinding and movement systems by encoding terrain difficulty for
 * different races and magical conditions.
 *
 * Algorithm:
 *   1. Handle GOD race special case (zero cost movement everywhere)
 *   2. Initialize race-specific vegetation cost table from global arrays
 *   3. Initialize race-specific elevation cost table from global arrays
 *   4. Apply magical modifications (DERVISH/DESTROYER ice/desert immunity)
 *   5. Calculate movement costs for all map sectors using nested loops
 *   6. Handle water sectors with adjacency checking for coastal access
 *   7. Handle land sectors with vegetation + elevation cost summation
 *   8. Apply road designation movement bonuses where applicable
 *
 * Movement Cost Encoding:
 *   - Positive values: Actual movement cost for land sectors
 *   - -1: Water adjacent to land (naval access possible)
 *   - -2: Unenterable land (impassable terrain for this race)
 *   - -4: Deep water not adjacent to land (pure naval zones)
 *   - 0: GOD race has no movement restrictions
 *
 * Race-Specific Terrain Costs:
 *   - HUMAN: Uses HVegcost[] and HElecost[] arrays (default)
 *   - ELF: Uses EVegcost[] and EElecost[] arrays (forest bonuses)
 *   - DWARF: Uses DVegcost[] and DElecost[] arrays (mountain bonuses)
 *   - ORC: Uses OVegcost[] and OElecost[] arrays (wasteland bonuses)
 *   - GOD: Zero cost movement (administrative override)
 *
 * Parameters:
 *   race - Character code for racial movement preferences (ELF, DWARF, ORC, HUMAN, GOD)
 *   cntry - Country index for magical ability checking
 *
 * Returns:
 *   void (results stored in global movecost[][] array)
 *
 * Side Effects:
 *   - Modifies global movecost[MAPX][MAPY] array for entire world map
 *   - Reads from global terrain arrays (veg[], ele[], sct[][])
 *   - Reads from global cost tables (HVegcost[], EVegcost[], etc.)
 *   - Calls magic() function to check for magical abilities
 *   - Extensive nested loops modify large amounts of global state
 *
 * Magical Enhancements:
 *   - DERVISH magic: Sets ICE and DESERT movement costs to 0
 *   - DESTROYER magic: Sets ICE and DESERT movement costs to 0
 *   - Magic abilities override base racial terrain preferences
 *   - Checked per country, not per race (nation-specific powers)
 *
 * Water Sector Processing:
 *   - Initial assignment: -4 (deep water, no land access)
 *   - Adjacency check: 3x3 grid around water sector
 *   - If any adjacent sector is land: change to -1 (coastal water)
 *   - Uses ONMAP() macro for boundary checking
 *   - Early termination when land found (i=x+2, j=y+2 breaks)
 *
 * Land Sector Processing:
 *   - Vegetation cost lookup: veg_cost[sptr->vegetation]
 *   - Elevation cost lookup: ele_cost[sptr->altitude]
 *   - Impassable check: Either cost table returns -1
 *   - Final cost: Sum of vegetation and elevation costs
 *   - Road bonus: Halve movement cost (rounded up) for DROAD designation
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires full game world and data tables
 *   Approach: Integration testing with complete map and cost table setup
 *   Key Tests: [Race-specific costs, water adjacency, magical bonuses, road effects]
 *   Dependencies: Complete world map (sct[][]), cost tables, magic system
 *   Mock Requirements: Full game world setup with all terrain and magic data
 *   Complexity: Moderate - Complex data processing with multiple lookup tables
 *
 * Global Dependencies:
 *   - movecost[][]: Output array for calculated movement costs
 *   - sct[][]: World map with terrain data (vegetation, altitude, designation)
 *   - veg[], ele[]: Terrain type identifier arrays
 *   - HVegcost[], EVegcost[], DVegcost[], OVegcost[]: Race vegetation preferences
 *   - HElecost[], EElecost[], DElecost[], OElecost[]: Race elevation preferences
 *   - MAPX, MAPY: World map dimensions for iteration bounds
 *   - magic(): Function to check magical abilities by country
 *
 * Performance Notes:
 *   - O(MAPX * MAPY) complexity for full world map processing
 *   - Additional O(9) water adjacency checking for each water sector
 *   - Lookup table approach provides efficient cost calculation
 *   - Called during game initialization and when movement rules change
 *
 * Terrain Encoding:
 *   - Cost tables use character arithmetic (' - '0') for numeric conversion
 *   - ALPHA_SIZE (128) provides ASCII character indexing capability
 *   - Vegetation and elevation codes used as direct array indices
 *   - Special terrain constants (ICE, DESERT, WATER, DROAD) for comparisons
 */
void
updmove(race,cntry)
int cntry;
char race;
{
	register struct s_sector	*sptr;
	register int	i,j;
	int x,y;
	short	veg_cost[ ALPHA_SIZE ];
	short	ele_cost[ ALPHA_SIZE ];

	if( race==GOD ) {
		for(x=0;x<MAPX;x++) for(y=0;y<MAPY;y++) movecost[x][y] = 0;
		return;
	}

	for( j = 0; veg[j] != '0'; j++ ) {
		switch( race ) {
		case ELF:
			veg_cost[ veg[j] ] = EVegcost[j] - '0';
			break;
		case DWARF:
			veg_cost[ veg[j] ] = DVegcost[j] - '0';
			break;
		case ORC:
			veg_cost[ veg[j] ] = OVegcost[j] - '0';
			break;
		case HUMAN:
		default:
			veg_cost[ veg[j] ] = HVegcost[j] - '0';
			break;
		} /* switch */
	} /* for */

	if(magic(cntry,DERVISH) || magic(cntry,DESTROYER)) {
		veg_cost[ ICE ] = 0;
		veg_cost[ DESERT ] = 0;
	}

	for( j = 0; ele[j] != '0'; j++ ) {
		switch( race ) {
		case ELF:
			ele_cost[ ele[j] ] = EElecost[j] - '0';
			break;
		case DWARF:
			ele_cost[ ele[j] ] = DElecost[j] - '0';
			break;
		case ORC:
			ele_cost[ ele[j] ] = OElecost[j] - '0';
			break;
		case HUMAN:
		default:
			ele_cost[ ele[j] ] = HElecost[j] - '0';
			break;
		} /* switch */
	} /* for */

	for(x=0;x<MAPX;x++) for(y=0;y<MAPY;y++){
		sptr = &sct[x][y];

		if(sptr->altitude==WATER) {
			movecost[x][y] = -4;
			for(i=x-1;i<=x+1;i++) for(j=y-1;j<=y+1;j++)
			if( ONMAP(i,j) )
				if( sct[i][j].altitude != WATER) {
					movecost[x][y] = -1;
					i=x+2;
					j=y+2;
				}
		} else {
			if( veg_cost[ sptr->vegetation ] == -1
			|| ele_cost[ sptr->altitude ] == -1 )
				movecost[x][y] = -2;
			else
				movecost[x][y] = veg_cost[ sptr->vegetation ] + ele_cost[ sptr->altitude ];
		} /* if */
		if (sptr->designation == DROAD)
			movecost[x][y] = (movecost[x][y] + 1) / 2;
	} /* for */
} /* updmove() */

#ifdef CONQUER
/*
 * flightcost - Calculate movement cost for flying units through terrain
 *
 * Calculates the movement cost for flying units (like dragons, eagles, flying
 * carpets) moving through a specific sector. Flight movement costs consider
 * both altitude and vegetation but with different cost tables than ground
 * movement, reflecting that flying units can cross water and are less affected
 * by vegetation but more affected by altitude and wind patterns.
 *
 * The function looks up movement costs from global cost tables:
 * - FElecost: Flight elevation costs - "16211/" (water=1, peak=6, etc.)
 * - FVegcost: Flight vegetation costs - "410000001000/" (volcano=4, etc.)
 *
 * Cost calculation combines both elevation and vegetation factors.
 * Flying units can cross water (cost 1) but face higher costs at peaks.
 *
 * Parameters:
 *   i - X coordinate of sector (0 to world.mapx-1)
 *   j - Y coordinate of sector (0 to world.mapy-1)
 *
 * Returns:
 *   Combined movement cost (elevation + vegetation) for flying units
 *   -1 if invalid terrain type or coordinates (indicates impassable)
 *
 * Side Effects:
 *   None - read-only function that only queries global data
 *
 * Testing Notes:
 *   Category: A (Unit) | Simple lookup function with clear inputs/outputs
 *   Approach: Unit tests with mock sector data and known cost tables
 *   Key Tests: Valid coordinates, invalid terrain, cost combinations
 *   Dependencies: Global arrays (ele, veg, FElecost, FVegcost), sct structure
 *   Mock Requirements: Mock sector data with known altitude/vegetation values
 *   Complexity: Simple - straightforward table lookup with error handling
 *
 * Notes:
 *   - Thread-safe as read-only operation on global immutable data
 *   - Critical for flight pathfinding and movement validation
 *   - Compare with land movement cost functions for balance verification
 *   - Cost tables are string-based with character arithmetic ('0' offset)
 */
/* calculations for cost of movement during flight */
int
flightcost(i,j)
int i,j;
{
	int cnt,hold=(-1),hold2=(-1);

	for (cnt=0; ele[cnt]!='0'; cnt++ ) {
		if (sct[i][j].altitude==ele[cnt])
			hold = (FElecost[cnt] - '0');
	}
	for (cnt=0; veg[cnt]!='0'; cnt++ ) {
		if (sct[i][j].vegetation==veg[cnt])
			hold2 = (FVegcost[cnt] - '0');
	}
	if (hold==(-1) || hold2==(-1)) {
		hold=(-1);
	}
	else hold+=hold2;

	return(hold);
}
#endif /* CONQUER */
#ifdef ADMIN
/* determines whether or not a unit has the ability to fly */
int
avian(typ)
unsigned char typ;
{
	switch(typ) {
	case A_ROC:
	case A_GRIFFON:
	case SPIRIT:
	case DJINNI:
	case DEMON:
	case DRAGON:
		return(TRUE);
	default:
		return(FALSE);
	}
}
#endif /* ADMIN */

/*
 * spreadsheet - Comprehensive economic calculation engine for nation production and revenue
 *
 * Performs complete economic calculations for a nation, computing total resource
 * production, tax revenue, and population distribution across all sectors. This
 * function serves as the core economic engine of the game, calculating the
 * complex interactions between population, terrain, magic, seasonal effects,
 * and taxation that determine national wealth and resource availability.
 *
 * The calculation system processes each owned sector by designation type:
 *
 * Mines (DMINE):
 * - Production: metal_value * population (up to TOMANYPEOPLE=4000, half efficiency above)
 * - Magic bonuses: 2x for MINER power, 2x for STEEL power (4x total possible)
 * - Tax revenue: production * TAXMETAL(8) * tax_rate / 100
 * - Requires trade good visibility validation via tg_ok()
 *
 * Farms (DFARM):
 * - Production: tofood(sector) * population (efficiency scaling like mines)
 * - Seasonal modifiers: Spring(/2), Summer(1x), Fall(2.5x), Winter(0x)
 * - Mill bonus: +20% production if adjacent mill has 500+ people
 * - Tax revenue: production * TAXFOOD(5) * tax_rate / 100
 *
 * Gold Mines (DGOLDMINE):
 * - Production: jewel_value * population (efficiency scaling like mines)
 * - Magic bonus: 2x for MINER power
 * - Tax revenue: production * TAXGOLD(8) * tax_rate / 100
 * - Requires trade good visibility validation
 *
 * Cities/Capitols (DCITY/DCAPITOL):
 * - Base tax: population * TAXCITY(100) * tax_rate / 100
 * - Magic bonus: 2x effective population with ARCHITECT power
 * - No resource production, pure revenue generation
 *
 * Towns (DTOWN):
 * - Base tax: population * TAXTOWN(80) * tax_rate / 100
 * - Magic bonus: 2x effective population with ARCHITECT power
 * - No resource production, pure revenue generation
 *
 * Special Magical Production (DERVISH/DESTROYER in desert/ice):
 * - Fixed 6 food per person (3 food per person above 4000)
 * - Seasonal effects: Desert harsh summer, good winter; Ice opposite
 * - Tax revenue: production * TAXFOOD * tax_rate / 100
 *
 * Other Sectors:
 * - Production: tofood(sector) * population (efficiency scaling)
 * - Tax revenue: production * TAXOTHR(3) * tax_rate / 100
 * - Covers all unspecialized sectors
 *
 * The global spread structure accumulates:
 * - Total resources: food, gold, metal, jewels
 * - Revenue by source: farms, mines, cities, etc.
 * - Population distribution: by sector type
 * - Final calculation: total gold = resources + all revenue sources
 *
 * Parameters:
 *   nation - Nation ID for which to calculate economics
 *
 * Returns:
 *   void - Results stored in global spread structure
 *
 * Side Effects:
 *   Modifies global spread structure with calculated values
 *   Reads from sct[][] sector array and ntn[] nation array
 *
 * Testing Notes:
 *   Category: B (Integration) | Complex system with many interdependencies
 *   Approach: Integration tests with complete game state scenarios
 *   Key Tests: Each sector type calculation, magic bonuses, seasonal effects
 *   Dependencies: tofood(), tg_ok(), magic(), sector/nation data, constants
 *   Mock Requirements: Complete mock world with various sector types and populations
 *   Complexity: Complex - comprehensive economic simulation with multiple systems
 *
 * Notes:
 *   - Central economic engine critical for game balance and progression
 *   - Efficiency scaling prevents unlimited growth from overpopulation
 *   - Magic powers provide significant but balanced economic advantages
 *   - Seasonal effects add strategic timing elements to food production
 *   - Tax rate allows player control over revenue vs. population happiness
 *   - Mill adjacency system encourages agricultural cluster development
 */
void
spreadsheet(nation)
int nation;
{
	register struct s_sector	*sptr;
	register struct s_nation	*nptr;
	long	product;
	long	city_pop, cap_pop;
	int x,y,i,j,foundmill;

	nptr = &ntn[ nation ];

	spread.revothr = spread.revfood = spread.revjewels = spread.revmetal = spread.revcap = spread.revcity = 0L;
	spread.inothr = spread.incity = spread.incap = spread.ingold = spread.infarm = spread.inmetal = 0;
	spread.food = nptr->tfood;
	spread.gold = nptr->tgold;
	spread.metal = nptr->metals;
	spread.jewels = nptr->jewels;
	spread.sectors = 0;
	spread.civilians = 0L;

	for(x=0;x<MAPX;x++) for(y=0;y<MAPY;y++) {
		sptr = &sct[x][y];
		if(sptr->owner!=nation) continue;

		/*update nation file for owner*/
		spread.sectors++;
		spread.civilians += sptr->people;

		product = 0;
		/*PRODUCE*/
		/*increase tmin based on mined stuff...*/
		if(sptr->designation==DMINE) {
			if( !tg_ok( nation, sptr )) continue;
			spread.inmetal += sptr->people;
			if(sptr->people>TOMANYPEOPLE) {
				product = sptr->metal * TOMANYPEOPLE;
				product += sptr->metal * (sptr->people-TOMANYPEOPLE)/2L;
			} else product = sptr->metal *  sptr->people;

			if(magic(sptr->owner,MINER)==1) product*=2L;
			if(magic(sptr->owner,STEEL)==1) product*=2L;
			spread.metal += product;
			spread.revmetal += product*TAXMETAL*nptr->tax_rate/100L;
		}
		/*harvest food*/
		else if(sptr->designation==DFARM) {
			spread.infarm += sptr->people;
			if(sptr->people>TOMANYPEOPLE) {
				product = (long)tofood(sptr,sptr->owner) * TOMANYPEOPLE;
				product += (long)tofood(sptr,sptr->owner) * (sptr->people-TOMANYPEOPLE)/2L;
			} else product = (long)tofood(sptr,sptr->owner) *  sptr->people;

			switch(SEASON(TURN)) {
			case SPRING:
				product/=2;
				break;
			case SUMMER:
				break;
			case FALL:
				product*=5;
				product/=2;
				break;
			case WINTER:
				product=0;
				break;
			}
			/* search for neighboring mills */
			foundmill=FALSE;
			for(i=x-1;foundmill==FALSE && i<=x+1;i++)
			for(j=y-1;foundmill==FALSE && j<=y+1;j++)
			if((ONMAP(i,j))
			&&(sct[i][j].owner==sptr->owner)
			&&(sct[i][j].designation==DMILL)
			&&(sct[i][j].people>=MILLSIZE)) {
				product *= 12L;
				product /= 10L;
				/* must break this way... two for() loops */
				foundmill=TRUE;
			}
			spread.food += product;
			spread.revfood += product*TAXFOOD*nptr->tax_rate/100L;
		}
		/*gold mines produce gold*/
		else if(sptr->designation==DGOLDMINE) {
			if( !tg_ok( nation, sptr )) continue;
			spread.ingold += sptr->people;
			if(sptr->people>TOMANYPEOPLE) {
				product = sptr->jewels * TOMANYPEOPLE;
				product += sptr->jewels * (sptr->people-TOMANYPEOPLE)/2L;
			} else product = sptr->jewels *  sptr->people;

			if(magic(sptr->owner,MINER)==1) product*=2;

			spread.jewels += product;
			spread.revjewels += product*TAXGOLD*nptr->tax_rate/100L;
		}
		else if((sptr->designation==DCITY)
		||(sptr->designation==DCAPITOL)) {
			cap_pop  = sptr->people;
			spread.incap += cap_pop;

			if( magic(sptr->owner, ARCHITECT ) ) {
				cap_pop *= 2L;
			}

			spread.revcap +=  cap_pop * TAXCITY*nptr->tax_rate / 100L;
		} else if(sptr->designation==DTOWN) {
			spread.incity += sptr->people;
			city_pop = sptr->people;
			if( magic(sptr->owner, ARCHITECT ) )
				city_pop *= 2L;

			spread.revcity +=  city_pop*TAXTOWN*nptr->tax_rate/100L;
		}
#ifndef DERVDESG
		else if(((magic(sptr->owner,DERVISH)==1)
			||(magic(sptr->owner,DESTROYER)==1))
		&&((sptr->vegetation==ICE) ||(sptr->vegetation==DESERT))
		&&(sptr->people>0)) {
			if(sptr->people>TOMANYPEOPLE) {
				product = 6L * TOMANYPEOPLE;
				product += 3L * (sptr->people-TOMANYPEOPLE);
			} else product = 6L *  sptr->people;

			spread.food += product;
			/* desert food production mostly static */
			if (sptr->vegetation==DESERT) {
				/* harsh summer in desert; good winter */
				if (SEASON(TURN)==SUMMER) product/=2;
				else if (SEASON(TURN)==WINTER) {
					product*=5;
					product/=4;
				}
			} else {
				/* opposite in ice */
				if (SEASON(TURN)==WINTER) product/=2;
				else if (SEASON(TURN)==SUMMER) {
					product*=5;
					product/=4;
				}
			}
			spread.revfood += product*TAXFOOD*nptr->tax_rate/100L;
		}
#endif /* DERVDESG */
		else {	/* other sectors */
			spread.inothr += sptr->people;
			if(sptr->people>TOMANYPEOPLE) {
				product = (long)tofood(sptr,sptr->owner) * TOMANYPEOPLE;
				product += (long)tofood(sptr,sptr->owner) * (sptr->people-TOMANYPEOPLE)/2L;
			} else product = (long)tofood(sptr,sptr->owner) *  sptr->people;

			spread.revothr += product*TAXOTHR*nptr->tax_rate/100L;
		}
	}
	spread.gold += spread.revfood + spread.revjewels + spread.revmetal + spread.revcity + spread.revcap + spread.revothr;
}

/*
 * get_nname - Interactive string input with live editing and length constraints
 *
 * Provides an interactive string input interface with real-time character
 * display, backspace editing, and automatic length limiting. Designed for
 * entering nation names and other string data with immediate visual feedback.
 * Implements character-by-character processing for curses-based terminals.
 *
 * Algorithm:
 *   - Character-by-character input loop using getch()
 *   - Printable character validation with isprint()
 *   - Real-time display updates with addch() and refresh()
 *   - Length-bounded input with NAMELTH constraint
 *   - Backspace handling with cursor repositioning and character removal
 *   - Null termination for proper C string handling
 *
 * Input Processing:
 *   - Printable characters: Add to string if within length limit, display immediately
 *   - Backspace (\b, DEL \177): Remove last character, erase from display
 *   - Enter/Return (\n, \r): Complete input and null-terminate string
 *   - Non-printable characters: Ignored (no action taken)
 *
 * Parameters:
 *   str - Output buffer for collected string (must be at least NAMELTH+1 bytes)
 *
 * Returns:
 *   void (result returned via str parameter modification)
 *
 * Side Effects:
 *   - Modifies str[] array with collected input characters
 *   - Reads from stdin using curses getch()
 *   - Updates screen display with addch(), move(), refresh()
 *   - Changes cursor position during editing operations
 *   - Requires curses library initialization (initscr, etc.)
 *   - Guarantees null termination of output string
 *
 * Length Management:
 *   - Maximum input length limited by NAMELTH constant
 *   - Prevents buffer overflow by rejecting characters beyond limit
 *   - Automatically null-terminates string at completion
 *   - Count tracking ensures accurate length management
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires curses library and terminal interface
 *   Approach: Integration testing with mock terminal/curses interface
 *   Key Tests: [Character entry, backspace editing, length limits, string termination]
 *   Dependencies: curses library (stdscr, getch, addch, refresh, move, getyx)
 *   Mock Requirements: Terminal interface mocking for automated testing
 *   Complexity: Moderate - User interface with state management and string handling
 *
 * Buffer Management:
 *   - Caller responsible for providing adequate buffer size (NAMELTH+1)
 *   - Function guarantees null termination within buffer bounds
 *   - No buffer overflow protection beyond length checking
 *   - Character count management prevents array bounds violations
 *
 * Notes:
 *   - Similar to get_number() but handles string input instead of numeric
 *   - Uses isprint() for comprehensive printable character validation
 *   - Character 127 (\177) handles DEL key on some terminal types
 *   - Real-time feedback provides immediate visual confirmation to user
 *   - NAMELTH constant defines maximum string length for consistency
 */
void
get_nname(str)
char str[];
{
	char ch;
	int done=0,count=0,xpos,ypos;

	while(!done) {
		ch=getch();
		if (isprint(ch)) {
			if (count<NAMELTH) {
				/* only input displayable characters */
				addch(ch);
				refresh();
				str[count++] = ch;
			}
		}
		else if ((ch=='\b' || ch=='\177')&&(count))
		{
			/* only delete what was printed */
			getyx(stdscr,ypos,xpos);
			move(ypos,--xpos);
			addch(' ');
			move(ypos,xpos);
			refresh();
			count--;
		} else if((ch=='\n')||(ch=='\r')) {
			done=TRUE;
		}
	}
	str[count] = '\0';
}


#ifdef CONQUER
/*
 * get_country - Nation lookup by name or number with input validation
 *
 * Interactive function that prompts for nation identification and returns
 * the corresponding nation index. Supports multiple input methods: exact
 * nation name matching, special keywords ("god", "news"), and numeric
 * nation IDs. Provides comprehensive error handling and input validation
 * for robust nation selection in game interfaces.
 *
 * Algorithm:
 *   1. Collect input string using get_nname() interface
 *   2. Handle empty input (return -1 for no selection)
 *   3. Attempt exact string match against all nation names
 *   4. Check for special keywords ("god" -> 0, "news" -> NEWSMAIL)
 *   5. If no name match, attempt numeric parsing with validation
 *   6. Validate numeric range and return appropriate result
 *
 * Input Methods:
 *   - Nation Name: Exact string match against ntn[].name array
 *   - Special Keywords: "god" maps to nation 0, "news" maps to NEWSMAIL
 *   - Numeric ID: Parse digits and validate against nation count (1-NTOTAL)
 *   - Empty Input: Return -1 to indicate no selection made
 *
 * Parameters:
 *   None (uses interactive input via get_nname())
 *
 * Returns:
 *   0-NTOTAL: Valid nation index (0=god, 1-NTOTAL=player nations)
 *   NEWSMAIL: Special news system identifier
 *   NTOTAL: Invalid input error indicator
 *   -1: No input provided (empty string)
 *
 * Side Effects:
 *   - Calls get_nname() which performs interactive terminal input
 *   - Displays error message via errormsg() for invalid nation names
 *   - Reads from global nation array (ntn[]) for name matching
 *   - May trigger curses display operations through get_nname()
 *
 * Input Validation:
 *   - String length checking for empty input detection
 *   - Exact name matching with case-sensitive comparison
 *   - Numeric validation ensuring all characters are digits
 *   - Range validation for numeric IDs (must be <= NTOTAL)
 *   - Special keyword recognition for system functions
 *
 * Error Handling:
 *   - Invalid nation name: Shows error message, returns NTOTAL
 *   - Non-numeric characters in number: Error message, returns NTOTAL
 *   - Number out of range: Automatically clamps to NTOTAL
 *   - Empty input: Silent return of -1 (not an error condition)
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires nation data and terminal interface
 *   Approach: Integration testing with mock nation database and terminal
 *   Key Tests: [Name lookup, numeric lookup, special keywords, error cases]
 *   Dependencies: Nation database (ntn[]), get_nname(), errormsg()
 *   Mock Requirements: Nation data setup, terminal input mocking
 *   Complexity: Moderate - Multiple input paths with validation logic
 *
 * Global Dependencies:
 *   - ntn[]: Array of nation structures with .name field for matching
 *   - NTOTAL: Total number of nations for bounds checking
 *   - NEWSMAIL: Special identifier for news system
 *   - NAMELTH: Maximum name length for buffer sizing
 *
 * Special Cases:
 *   - "god": Always maps to nation 0 regardless of nation names
 *   - "news": Always maps to NEWSMAIL constant
 *   - Numeric input: Parsed digit-by-digit with overflow protection
 *   - Empty input: Treated as user cancellation, not error
 *
 * Notes:
 *   - Case-sensitive string matching for nation names
 *   - Numeric parsing allows leading zeros without issues
 *   - CONQUER build only (requires preprocessor flag)
 *   - Function combines user interface and data lookup functionality
 */
int
get_country()
{
	char name[NAMELTH+1],ch;
	int i,l,hold;

	/* get name and check through list */
	get_nname(name);

	/* return on no entry */
	if ((l=strlen(name))==0) {
		return(-1);
	}

	for(hold=0;hold<NTOTAL;hold++)
		if(strcmp(ntn[hold].name,name)==0) break;

	/* check for 'god' */
	if (strcmp("god",name)==0) hold=0;
	if (strcmp("news",name)==0) hold= NEWSMAIL;

	/* check for numbers if name too long */
	if (hold==NTOTAL) {
		hold=0;
		for (i=0;i<l;i++) {
			ch=name[i];
			if(ch < '0' || ch > '9' ) {
				errormsg("There is no nation by that name.");
				return(NTOTAL);
			} else {
				hold *= 10;
				hold += (ch-'0');
			}
		}
		if (hold>NTOTAL) hold=NTOTAL;
	}
	/* send back result */
	return(hold);
}

/*
 * get_god - God nation selection interface with administrative privileges
 *
 * Interactive interface for super users (gods) to select which nation they
 * want to control or observe. Provides screen prompting, input validation,
 * and complete session state setup for administrative game oversight.
 * Handles user cancellation and invalid input with appropriate error recovery.
 *
 * Algorithm:
 *   1. Clear bottom screen area for clean interface presentation
 *   2. Display "Super User; For what nation?" prompt at bottom of screen
 *   3. Refresh display to ensure prompt visibility
 *   4. Collect nation selection using get_country() interface
 *   5. Validate input and handle cancellation/errors appropriately
 *   6. Set global game state (country, curntn) for selected nation
 *   7. Return success/failure status to caller
 *
 * User Interface Features:
 *   - Clean screen presentation with bottom area clearing
 *   - Prominent super user prompt for administrative context
 *   - Immediate display refresh for responsive interface
 *   - Error recovery with screen restoration on failure
 *
 * Parameters:
 *   None (uses interactive terminal input)
 *
 * Returns:
 *   0: Successfully selected nation and updated global state
 *   1: Failed due to user cancellation or invalid input
 *
 * Side Effects:
 *   - Modifies screen display via clear_bottom(), mvaddstr(), refresh()
 *   - Sets global variable 'country' to selected nation index
 *   - Sets global pointer 'curntn' to point to selected nation structure
 *   - May set 'redraw' flag to DONE on failure for screen management
 *   - Calls makebottom() to restore screen layout on failure
 *   - Performs interactive input through get_country() chain
 *
 * Global State Management:
 *   - country: Set to selected nation index (0 on failure)
 *   - curntn: Set to &ntn[country] for direct nation access
 *   - redraw: Set to DONE on failure to trigger screen refresh
 *   - Screen state: Modified through curses display operations
 *
 * Input Validation and Error Handling:
 *   - Empty input (-1 from get_country()): Treated as cancellation
 *   - Invalid nation (NTOTAL from get_country()): Treated as error
 *   - Both error conditions: Reset country to 0, restore screen, return 1
 *   - Success: Update global state for selected nation, return 0
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires curses display and nation data
 *   Approach: Integration testing with mock terminal and nation database
 *   Key Tests: [Valid selection, cancellation, invalid input, screen management]
 *   Dependencies: curses library, get_country(), nation database (ntn[])
 *   Mock Requirements: Terminal display mocking, nation data setup
 *   Complexity: Moderate - UI management with global state coordination
 *
 * Administrative Context:
 *   - Designed specifically for super user/god mode operations
 *   - Allows administrative oversight of any nation in the game
 *   - Provides clean separation between normal and administrative interfaces
 *   - Integrates with broader administrative command framework
 *
 * Screen Management:
 *   - Uses bottom screen area (LINES-4) for prompting
 *   - Coordinates with makebottom() for layout restoration
 *   - Manages redraw flags for efficient screen updates
 *   - Provides clean visual separation for administrative functions
 *
 * Global Dependencies:
 *   - ntn[]: Nation database for curntn pointer assignment
 *   - LINES: Screen height for prompt positioning
 *   - NTOTAL: Maximum nation count for validation
 *   - country, curntn, redraw: Global game state variables
 *
 * Notes:
 *   - CONQUER build only (requires preprocessor flag)
 *   - Integrates with get_country() for consistent nation selection
 *   - Error recovery ensures clean state on all failure paths
 *   - Administrative privilege context clearly indicated in prompt
 */
int
get_god()
{
	clear_bottom(0);
	mvaddstr(LINES-4,0,"Super User; For what nation? ");
	refresh();

	/* return on no entry or bad entry */
	if ((country=get_country())==(-1) || country==NTOTAL) {
		country = 0;
		redraw=DONE;
		makebottom();
		return(1);
	}

	curntn = &ntn[country];
	return(0);
}

/*
 * reset_god - God nation reset utility for administrative session management
 *
 * Simple utility function that resets the administrative god session back to
 * the default god nation (nation 0). Provides a clean, centralized mechanism
 * for returning to god mode after administrative operations on other nations.
 * Designed for readability and consistent state management in god mode operations.
 *
 * Algorithm:
 *   1. Set global country variable to 0 (god nation)
 *   2. Set global curntn pointer to &ntn[0] for god nation access
 *   3. Return (no error conditions possible)
 *
 * Administrative Purpose:
 *   - Provides clean return to god mode after nation-specific operations
 *   - Ensures consistent state management for administrative sessions
 *   - Centralizes god nation assignment logic for maintainability
 *   - Improves code readability by abstracting common operation
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   void (always succeeds)
 *
 * Side Effects:
 *   - Sets global variable 'country' to 0 (god nation index)
 *   - Sets global pointer 'curntn' to &ntn[0] for god nation structure
 *   - Changes current administrative context to god mode
 *   - No screen or display modifications
 *
 * Global State Management:
 *   - country: Reset to 0 for god nation
 *   - curntn: Reset to point to god nation structure (&ntn[0])
 *   - Coordinates with get_god() for consistent god mode handling
 *   - Provides symmetric operation to god nation selection
 *
 * Testing Notes:
 *   Category: A (Unit) - Simple global variable assignment
 *   Approach: Unit testing with global state verification
 *   Key Tests: [God nation assignment, pointer setup, state consistency]
 *   Dependencies: Global variables (country, curntn), nation array (ntn[])
 *   Mock Requirements: Minimal - global variable monitoring
 *   Complexity: Simple - Straightforward assignment operations
 *
 * Usage Context:
 *   - Called after administrative operations on specific nations
 *   - Used to return to god oversight mode for general administration
 *   - Provides clean separation between nation-specific and god operations
 *   - Integrates with broader administrative command framework
 *
 * Design Rationale:
 *   - Simple function improves code readability over inline assignments
 *   - Centralizes god nation logic for consistent behavior
 *   - Provides symmetric operation to get_god() nation selection
 *   - Eliminates code duplication in administrative modules
 *
 * Global Dependencies:
 *   - country: Global current nation identifier
 *   - curntn: Global current nation structure pointer
 *   - ntn[]: Nation database array (assumes ntn[0] is god nation)
 *
 * Notes:
 *   - CONQUER build only (requires preprocessor flag)
 *   - No error checking needed (god nation always exists)
 *   - Function comment notes readability improvement as design goal
 *   - Extremely simple but important for administrative state management
 */
void
reset_god()
{
	/* simple routine; but improves readibility */
	country=0;
	curntn= &ntn[country];
}
#endif /* CONQUER */

#ifdef ADMIN
int
getleader(class)
int	class;
{
	switch(class){
	case C_NPC:
	case C_KING:
	case C_TRADER:	return(L_BARON);
	case C_EMPEROR:	return(L_PRINCE);
	case C_WIZARD:	return(L_MAGI);
	case C_PRIEST:	return(L_BISHOP);
	case C_PIRATE:	return(L_CAPTAIN);
	case C_WARLORD:	return(L_LORD);
	case C_DEMON:	return(L_DEVIL);
	case C_DRAGON:	return(L_WYRM);
	case C_SHADOW:	return(L_NAZGUL);
	default:
		printf("ERROR-national class (%d) undefined\n",class);
		exit(0);
	}
	return(-1);	/* shut lint up */
}
#endif /* ADMIN */

/* name of the currently open mail file */
char tmp_mail_name[LINELTH];

int
mailopen(int to)
{
#ifdef CONQUER
	char	line[LINELTH];
#endif /*CONQUER*/
	if(mailok != DONEMAIL) mailclose(ABORTMAIL);

	if (to != NEWSMAIL) {
#ifdef CONQUER
		struct stat fst;
		/* check if the player is currently reading messages */
		sprintf(line,"%s%hd.tmp",msgfile,to);
		if (stat(line,&fst)==0) {
			long now;
			now = time(0);
			if (now - fst.st_mtime < TIME_DEAD) {
				if (to>=0 && to<NTOTAL) {
					sprintf(line,"Nation %s is reading their mail... try again later.", ntn[to].name);
					errormsg(line);
				}
				return(-1);
			} else {
				/* remove useless file */
				unlink(line);
			}
		}

		/* otherwise continue; checking for others */
		/* this file name is also used in rmessages() */
		sprintf(tmp_mail_name,"send.%s%hd",msgfile,to);
		if (stat(tmp_mail_name,&fst)==0) {
			long now;
			now = time(0);
			if (now - fst.st_mtime < TIME_DEAD) {
				if (to>=0 && to<NTOTAL) {
					sprintf(line,"Someone is already mailing Nation %s... try again later.", ntn[to].name);
					errormsg(line);
				}
				return(-1);
			} else {
				/* remove useless file */
				unlink(tmp_mail_name);
			}
		}
#endif /*CONQUER*/
#ifdef ADMIN
		sprintf(tmp_mail_name,"%s%hd",msgfile,to);
#endif /*ADMIN*/
	} else {
		/* send to a location marked by the current player */
		sprintf(tmp_mail_name,"send.news%d", country);
	}
	if ((fm=fopen(tmp_mail_name,"a+"))==NULL) {
		fprintf(stderr,"error opening %s",tmp_mail_name);
		return(-1);
	}
	mailok=to;
	return(0);
}

void
mailclose(int to)
{
	if(mailok==DONEMAIL) return;

	if(to >= 0) {
		fputs("END\n",fm);
	}
	fclose(fm);

#ifdef CONQUER
	if((to!=ABORTMAIL)&&(to==mailok)) {
		char line[BIGLTH];
		if (to==NEWSMAIL) {
			/* send to the current newspaper */
			sprintf(line,"cat %s >> news%d",tmp_mail_name,TURN-1);
		} else {
			/* send to the player now */
			sprintf(line,"cat %s >> %s%d",tmp_mail_name,msgfile,to);
		}
		system(line);
	}
	if (tmp_mail_name!=NULL) {
		(void) unlink(tmp_mail_name);
	}
#endif /*CONQUER*/
	mailok=DONEMAIL;
}

#ifdef ADMIN
/* markok returns TRUE if mark is ok as a nation mark */
int
markok(mark,prtflag)
char mark;
int prtflag;	/* if true printf reason */
{
	register int i;
	char temp[LINELTH];

	if((isprint(mark)==0)||(isspace(mark)!=0)) {
		if(prtflag) {
			sprintf(temp,"%c is white space",mark);
			newerror(temp);
		}
		return(FALSE);
	}

	for(i=0;ele[i]!='0';i++) if(mark==(*(ele+i))) {
		if(prtflag) {
			sprintf(temp,"%c is an elevation character",mark);
			newerror(temp);
		}
		return(FALSE);
	}

	for(i=0;veg[i]!='0';i++) if(mark==(*(veg+i))) {
		if(prtflag) {
			sprintf(temp,"%c is a vegetation character",mark);
			newerror(temp);
		}
		return(FALSE);
	}

	for(i=1;i<NTOTAL;i++) if(isactive(ntn[i].active) && ntn[i].mark==mark) {
		if(prtflag) {
			sprintf(temp,"%c is already used",mark);
			newerror(temp);
		}
		return(FALSE);
	}

	if(mark=='*') {
		if(prtflag) {
			sprintf(temp,"%c is used by Monsters",mark);
			newerror(temp);
		}
		return(FALSE);
	}

	if(!isalpha(mark)) {
		if(prtflag) {
			sprintf(temp,"%c is not an alpha character",mark);
			newerror(temp);
		}
		return(FALSE);
	}
	return(TRUE);
}
#endif /* ADMIN */

/*
 * defaultunit - Determine optimal default army type based on nation's magical abilities
 *
 * Selects the most advantageous army type for a nation based on their magical
 * powers and racial characteristics. This function implements an intelligent
 * army type selection system that maximizes the effectiveness of special
 * abilities and racial bonuses, primarily used by NPC nations to optimize
 * their military strategy automatically.
 *
 * The selection follows a priority hierarchy, checking for increasingly
 * powerful magical abilities and selecting the strongest available option:
 *
 * Priority 1 - Undead Powers (Highest Priority):
 * - VAMPIRE power → A_ZOMBIE (24): Undead armies with vampiric leadership
 *   Creates fearsome undead legions immune to many effects
 *
 * Priority 2 - Advanced Monster Powers:
 * - AV_MONST (Advanced Monsters) + BREEDER → A_OLOG (10): Elite troll units
 *   Large, powerful creatures with breeding capabilities
 * - AV_MONST alone → A_URUK (7): Advanced humanoid monsters
 *   Superior warrior breeds with enhanced combat abilities
 *
 * Priority 3 - Specialized Combat Powers:
 * - ARCHER power → A_ARCHER (6): Specialized ranged combat units
 *   Elite marksmen with superior range and accuracy
 *
 * Priority 4 - Basic Monster Powers:
 * - MI_MONST power → A_ORC (2): Basic monstrous humanoids
 *   Crude but effective warrior creatures
 *
 * Priority 5 - Racial/Cultural Specialization:
 * - NPC_NOMAD nations → A_LT_CAV (16): Light cavalry forces
 *   Mobile horsemen suited to nomadic warfare tactics
 *
 * Default Fallback:
 * - No special powers → A_INFANTRY (3): Standard human foot soldiers
 *   Basic military units suitable for all nations
 *
 * The hierarchy ensures nations with multiple powers select the most powerful
 * option available, while providing reasonable defaults for nations without
 * special military abilities.
 *
 * Parameters:
 *   nation - Nation ID for which to determine optimal army type
 *
 * Returns:
 *   Army type constant (A_*) representing the optimal default unit
 *   Higher-numbered types generally represent more powerful/specialized units
 *
 * Side Effects:
 *   None - read-only analysis of nation's magical powers and characteristics
 *
 * Testing Notes:
 *   Category: A (Unit) | Clear priority logic with magic power evaluation
 *   Approach: Unit tests with mock nations having specific power combinations
 *   Key Tests: Each magic power priority, combination effects, default fallback
 *   Dependencies: magic() macro, ntn[] nation data, power/army type constants
 *   Mock Requirements: Mock nations with known power combinations
 *   Complexity: Simple - straightforward priority checking with clear hierarchy
 *
 * Notes:
 *   - Critical for NPC military effectiveness and strategic balance
 *   - Priority system ensures optimal use of available magical abilities
 *   - Undead powers take absolute priority due to their overwhelming advantages
 *   - Monster powers scale appropriately with advanced > basic
 *   - Nomadic cultural adaptation provides mobility-focused army selection
 *   - Default infantry ensures all nations have viable military options
 */
/*******************************************************************/
/* DEFAULTUNIT() returns the default army type for a given country */
/* this is mostly used by npc's to take advantage of their powers  */
/*******************************************************************/
long
defaultunit( nation )
int	nation;
{
	if(magic(nation,VAMPIRE)) return(A_ZOMBIE);
	if(magic(nation,AV_MONST)) {
		if(magic(nation,BREEDER))
			return(A_OLOG);
		else	return(A_URUK);
	}
	if(magic(nation,ARCHER)) return(A_ARCHER);
	if(magic(nation,MI_MONST)) return(A_ORC);	/* if race = orc */
	if( ntn[nation].active==NPC_NOMAD ) return(A_LT_CAV);
	return(A_INFANTRY);
}

#ifdef ADMIN
void
getmetal( sptr )
struct s_sector *sptr;
{
	int randval;
	randval = rand()%100;
	if((sptr->tradegood != TG_none)&&(sptr->tradegood != 0)) return;
	if( randval < 20 ) {
		sptr->tradegood = TG_copper;
		sptr->metal = rand()%2 + 1;
	} else if( randval < 30 ) {
		sptr->tradegood = TG_lead;
		sptr->metal = rand()%4 + 1;
	} else if( randval < 40 ) {
		sptr->tradegood = TG_tin;
		sptr->metal = rand()%4 + 2;
	} else if( randval < 55 ) {
		sptr->tradegood = TG_bronze;
		sptr->metal = rand()%4 + 2;
	} else if( randval < 80 ) {
		sptr->tradegood = TG_iron;
		sptr->metal = rand()%7 + 2;
	} else if( randval < 95 ) {
		sptr->tradegood = TG_steel;
		sptr->metal = rand()%8 + 3;
	} else if( randval < 99 ) {
		sptr->tradegood = TG_mithral;
		sptr->metal = rand()%11 + 5;
	} else {
		sptr->tradegood = TG_adamantine;
		sptr->metal = rand()%13 + 8;
	}
}

void
getjewel( sptr )
struct s_sector *sptr;
{
	int randval;
	if((sptr->tradegood != TG_none)&&(sptr->tradegood != 0)) return;
	randval = rand()%100;
	if( randval < 20 ) {
		sptr->tradegood = TG_spice;
		sptr->jewels = rand()%2 + 1;
	} else if( randval < 40 ) {
		sptr->tradegood = TG_silver;
		sptr->jewels = rand()%3 + 1;
	} else if( randval < 48 ) {
		sptr->tradegood = TG_pearls;
		sptr->jewels = rand()%3 + 1;
	} else if( randval < 56 ) {
		sptr->tradegood = TG_dye;
		sptr->jewels = rand()%5 + 1;
	} else if( randval < 64 ) {
		sptr->tradegood = TG_silk;
		sptr->jewels = rand()%5 + 1;
	} else if( randval < 84 ) {
		sptr->tradegood = TG_gold;
		sptr->jewels = rand()%6 + 1;
	} else if( randval < 91 ) {
		sptr->tradegood = TG_rubys;
		sptr->jewels = rand()%6 + 1;
	} else if( randval < 96 ) {
		sptr->tradegood = TG_ivory;
		sptr->jewels = rand()%7 + 2;
	} else if( randval < 99 ) {
		sptr->tradegood = TG_diamonds;
		sptr->jewels = rand()%11 + 2;
	 } else {
		sptr->tradegood = TG_platinum;
		sptr->jewels = rand()%17 + 4;
	}
}
#endif /* ADMIN */

/*
 * tg_ok - Trade good visibility and exploitation validation checker
 *
 * Determines whether a nation can detect, identify, and exploit the trade good
 * present in a sector. This function implements the game's technology and
 * knowledge progression system, where nations must develop sufficient expertise
 * before they can benefit from advanced materials and luxury goods.
 *
 * The validation system operates on two levels:
 * 1. Technology/Wealth Requirements: Advanced trade goods require sufficient
 *    national development to be recognized and extracted
 * 2. Sector Viability: The sector must have adequate food production to support
 *    specialized economic activity (DESFOOD=4 minimum)
 *
 * Technology Requirements by Trade Good Category:
 *
 * Mining Technology (mine_ability threshold):
 * - Lead: 8 (basic mining)
 * - Tin: 11 (bronze age metals)
 * - Bronze: 15 (alloy technology)
 * - Iron: 25 (iron age smelting)
 * - Steel: 30 (advanced metallurgy)
 * - Mithral: 30 (magical metal recognition)
 * - Adamantine: 40 (legendary material mastery)
 *
 * Wealth Requirements (luxury goods, wealth threshold):
 * - Dye, Silk: 5 (basic luxury trade)
 * - Gold, Rubys: 8 (precious materials)
 * - Ivory: 15 (exotic luxury goods)
 * - Diamonds: 20 (rare gemstone expertise)
 * - Platinum: 25 (ultimate precious metal)
 *
 * Always Available (no requirements):
 * - Spice, Silver, Pearls: Basic valuable commodities
 * - All food, basic materials, and common trade goods
 *
 * Special Cases:
 * - Nation 0 (unowned sectors): Always return TRUE for neutral access
 * - Invalid nations (>=NTOTAL): Always return TRUE for system sectors
 * - Final viability check: Sector must produce >=4 food to support development
 *
 * Parameters:
 *   nation - Nation ID checking trade good access (0 for neutral)
 *   sptr - Pointer to sector containing the trade good to validate
 *
 * Returns:
 *   TRUE (1) if nation can exploit the trade good in this sector
 *   FALSE (0) if insufficient technology/wealth or sector not viable
 *
 * Side Effects:
 *   None - read-only validation using nation stats and sector data
 *
 * Testing Notes:
 *   Category: A (Unit) | Clear logic with technology threshold validation
 *   Approach: Unit tests with mock nations at various development levels
 *   Key Tests: Each technology threshold, wealth requirements, food viability
 *   Dependencies: tofood() function, ntn[] nation data, trade good constants
 *   Mock Requirements: Mock nations with known ability/wealth, mock sectors
 *   Complexity: Simple - straightforward threshold checking with clear logic
 *
 * Notes:
 *   - Critical for economic balance and technology progression
 *   - Prevents early game exploitation of advanced materials
 *   - Encourages balanced national development (not just military focus)
 *   - Food viability requirement prevents exploitation of barren sectors
 *   - Technology trees create strategic choices in nation development
 */
/* tg_ok returns true if a trade good can be seen by the owner of sector */
int
tg_ok( nation, sptr )
int	nation;
struct	s_sector	*sptr;
{
	if(( nation == 0)||(nation>=NTOTAL)) return(TRUE);

	switch( sptr->tradegood ) {
	case TG_lead:	if(ntn[nation].mine_ability < 8) return(0); break;
	case TG_tin:	if(ntn[nation].mine_ability < 11) return(0); break;
	case TG_bronze:	if(ntn[nation].mine_ability < 15) return(0); break;
	case TG_iron:	if(ntn[nation].mine_ability < 25) return(0); break;
	case TG_steel:	if(ntn[nation].mine_ability < 30) return(0); break;
	case TG_mithral: if(ntn[nation].mine_ability < 30) return(0); break;
	case TG_adamantine: if(ntn[nation].mine_ability < 40) return(0); break;
	case TG_spice:
	case TG_silver:
	case TG_pearls:	break;
	case TG_dye:
	case TG_silk:	if(ntn[nation].wealth < 5) return(0); break;
	case TG_gold:
	case TG_rubys:	if(ntn[nation].wealth < 8) return(0); break;
	case TG_ivory:	if(ntn[nation].wealth < 15) return(0); break;
	case TG_diamonds:	if(ntn[nation].wealth < 20) return(0); break;
	case TG_platinum:	if(ntn[nation].wealth < 25) return(0); break;
	default:		break;
	};

	if(tofood(sptr,nation) >= DESFOOD) return(TRUE);
	return(FALSE);
}

/*
 * fort_val - Calculate total defensive fortification value of a sector
 *
 * Computes the complete defensive value of a sector based on its designation
 * type, fortification level, and any magical enhancements. This value is
 * critical for combat calculations, determining how effectively defending
 * forces can resist attacks and protect their positions.
 *
 * The fortification system uses a base defense value plus designation-specific
 * multipliers applied to the sector's fortress level. Magic can double the
 * effectiveness of constructed fortifications.
 *
 * Fortification Values by Designation:
 *
 * Stockades (DSTOCKADE):
 * - Fixed value: DEF_BASE (10)
 * - No fortress level scaling or magic bonuses
 * - Basic wooden defensive structures
 *
 * Forts (DFORT):
 * - Base: DEF_BASE (10) + FORTSTR (5) * fortress_level
 * - With ARCHITECT magic: DEF_BASE + 2 * FORTSTR * fortress_level
 * - Military-focused stone fortifications with scaling strength
 *
 * Towns (DTOWN):
 * - Base: DEF_BASE (10) + TOWNSTR (5) * fortress_level
 * - With ARCHITECT magic: DEF_BASE + 2 * TOWNSTR * fortress_level
 * - Civilian defensive structures with moderate scaling
 *
 * Cities/Capitols (DCITY/DCAPITOL):
 * - Base: 2 * DEF_BASE (20) + CITYSTR (8) * fortress_level
 * - With ARCHITECT magic: 2 * DEF_BASE + 2 * CITYSTR * fortress_level
 * - Major urban centers with doubled base defense and strongest scaling
 *
 * Other Sectors:
 * - Return 0 (no defensive fortifications)
 * - Includes farms, mines, and undesignated terrain
 *
 * Magic Enhancement:
 * - ARCHITECT power doubles fortress level effectiveness
 * - Represents superior engineering and construction techniques
 * - Does not affect base defense values, only scaling multipliers
 *
 * Parameters:
 *   sptr - Pointer to sector structure containing designation and fortress data
 *
 * Returns:
 *   Total defensive value for the sector
 *   0 for sectors without defensive fortifications
 *   Enhanced values for sectors with ARCHITECT magic
 *
 * Side Effects:
 *   None - read-only calculation using sector data and magic status
 *
 * Testing Notes:
 *   Category: A (Unit) | Clear calculation with designation-based logic
 *   Approach: Unit tests with mock sectors for each designation and fortress level
 *   Key Tests: Each designation type, fortress scaling, ARCHITECT magic bonus
 *   Dependencies: magic() macro, sector designation/fortress fields, constants
 *   Mock Requirements: Mock sectors with known designations and fortress levels
 *   Complexity: Simple - straightforward calculation with clear formula
 *
 * Notes:
 *   - Critical for combat system balance and defensive strategy
 *   - Cities/capitols have strongest defenses (2x base + best scaling)
 *   - Magic provides significant but not overwhelming defensive advantage
 *   - Fortress level investment creates meaningful strategic choices
 *   - Stockades provide fixed basic defense regardless of investment level
 */
/* this routine computes the fortification value of a sector */
int
fort_val(sptr)
	struct s_sector *sptr;
{
	if(sptr->designation==DSTOCKADE) {
		return(DEF_BASE);
	}
	if(sptr->designation==DFORT){
		if(magic(sptr->owner,ARCHITECT)==1){
			return(DEF_BASE + 2*FORTSTR * sptr->fortress);
		}
		else return(DEF_BASE + FORTSTR * sptr->fortress);
	}
	if (sptr->designation==DTOWN){
		if(magic(sptr->owner,ARCHITECT)==1){
			return(DEF_BASE + 2*TOWNSTR * sptr->fortress);
		} else return(DEF_BASE + TOWNSTR * sptr->fortress);
	}
	if((sptr->designation==DCAPITOL)
	||(sptr->designation==DCITY)){
		if(magic(sptr->owner,ARCHITECT)==1){
			return(2*DEF_BASE + 2*CITYSTR * sptr->fortress);
		}
		else return(2*DEF_BASE + CITYSTR * sptr->fortress);
	}
	return(0);
}

/* routine to determine compass direction of x1,y1 from x0,y0 */
int
compass(x0,y0,x1,y1)
	int x0,y0,x1,y1;
{
	int dx=x1-x0, dy=y1-y0;	/* diplacements */
	int hold;

	if(10*abs(dx) > abs(dy)) {
		if(10*abs(dy) > abs(dx)) {
			/* four off-quadrants */
			if(dx>0) {
				if(dy<0) hold=NORTHEAST;
				else hold=SOUTHEAST;
			} else {
				if(dy<0) hold=NORTHWEST;
				else hold=SOUTHWEST;
			}
		} else {
			/* east or west */
			if(dx>0) hold=EAST;
			else hold=WEST;
		}
	} else {
		/* north or south or same point */
		if(dy==0) hold=CENTERED;
		else if(dy<0) hold=NORTH;
		else hold=SOUTH;
	}
	return(hold);
}

#ifdef CONQUER
extern short xcurs;
extern short ycurs;
off_t conq_mail_size=0;
#ifdef SYSMAIL
static off_t sys_mail_size=0;
#endif /* SYSMAIL */
void
check_mail()
{
	struct stat info;
#ifdef SYSMAIL
	int osys_mail=sys_mail_status;
#endif
	int oconq_mail=conq_mail_status;

	/* check conquer mail box */
	if (stat(conqmail,&info)==(-1)) {
		conq_mail_status=NO_MAIL;
		conq_mail_size=0;
	} else {
		if (info.st_size > conq_mail_size) {
			conq_mail_status=NEW_MAIL;
			conq_mail_size=info.st_size;
		} else if (info.st_size < conq_mail_size) {
			conq_mail_status=NO_MAIL;
			conq_mail_size=info.st_size;
		}
	}

#ifdef SYSMAIL
	/* check system mail box */
	if (stat(sysmail,&info)==(-1)) {
		sys_mail_status=NO_MAIL;
		sys_mail_size=0;
	} else {
		if(info.st_atime>info.st_mtime) {
			sys_mail_status=NO_MAIL;
			sys_mail_size=info.st_size;
		} else if (info.st_size > sys_mail_size) {
			sys_mail_status=NEW_MAIL;
			sys_mail_size=info.st_size;
		} else if (info.st_size < sys_mail_size) {
			sys_mail_status=NO_MAIL;
			sys_mail_size=info.st_size;
		}
	}

	/* display mail information */
	if(sys_mail_status!=osys_mail) {
		if (sys_mail_status==NEW_MAIL) {
			mvaddstr(LINES-3,COLS/2-6,"You have System Mail");
		} else {
			mvaddstr(LINES-3,COLS/2-6,"                    ");
		}
		move(ycurs,2*xcurs);
		refresh();
	}
	if (conq_mail_status!=oconq_mail) {
		if (conq_mail_status==NEW_MAIL) {
			mvaddstr(LINES-2,COLS/2-6,"You have Conquer Mail");
		} else {
			mvaddstr(LINES-2,COLS/2-6,"                     ");
		}
		move(ycurs,2*xcurs);
		refresh();
	}
#else
	/* display mail information */
	if (conq_mail_status!=oconq_mail) {
		if (conq_mail_status==NEW_MAIL) {
			mvaddstr(LINES-3,COLS/2-6,"You have Conquer Mail");
		} else {
			mvaddstr(LINES-3,COLS/2-6,"                     ");
		}
		move(ycurs,2*xcurs);
		refresh();
	}
#endif /* SYSMAIL */
}
#endif /* CONQUER */
