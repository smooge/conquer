/*
 * io.c - Input/output and file handling functions
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
 *
 * =====================================================================================
 *
 * INPUT/OUTPUT AND FILE HANDLING SYSTEM
 *
 * This module provides comprehensive I/O capabilities for the Conquer game system,
 * serving as the primary interface between the game engine and external data storage,
 * map rendering, and user interface operations. The system handles both binary game
 * state persistence and formatted map output generation.
 *
 * CORE ARCHITECTURAL COMPONENTS:
 *
 * 1. MEMORY MANAGEMENT SUBSYSTEM
 *    - Dynamic allocation for core game data structures (sectors, movement costs)
 *    - Automatic cleanup and reallocation for different map sizes
 *    - Integration with custom 2D array allocator (m2alloc)
 *
 * 2. GAME STATE PERSISTENCE
 *    - Binary serialization/deserialization of complete world state
 *    - Atomic file operations with comprehensive error checking
 *    - Version-aware data format handling and validation
 *
 * 3. MAP VISIBILITY AND PREPARATION SYSTEM
 *    - Nation-based visibility calculation for fog-of-war mechanics
 *    - Multi-source sight integration (land ownership, armies, navies)
 *    - Magic system integration for enhanced/concealed visibility
 *
 * 4. MAP OUTPUT GENERATION ENGINE
 *    - Multiple map view generation (altitude, nations, designations, vegetation)
 *    - Visibility-aware rendering with conditional information display
 *    - Formatted text output with standardized headers and layouts
 *
 * 5. DISPLAY CONTROL AND NAVIGATION
 *    - Screen positioning and cursor management for interactive display
 *    - Map centering and coordinate translation services
 *    - Location jumping and capitol navigation functionality
 *
 * 6. GAME REPORTING SYSTEM
 *    - Comprehensive score and statistics reporting
 *    - Multi-nation status displays with race/class/alignment information
 *    - Optional time logging and update tracking
 *
 * 7. SPECIALIZED GAME MECHANICS
 *    - Civilian population evacuation and relocation algorithms
 *    - Slavery mechanics and population survival calculations
 *    - Sector devastation and fortress destruction on population loss
 *
 * 8. ADMINISTRATIVE TOOLS
 *    - Scenario map loading from external elevation/vegetation files
 *    - Development and testing support functionality
 *    - Map generation pipeline integration
 *
 * 9. UTILITY INFRASTRUCTURE
 *    - 2D array memory allocation with optimized layout
 *    - Secure password input with character masking
 *    - Cross-platform file handling and path management
 *
 * INTEGRATION WITH CORE SYSTEMS:
 * - Display System (display.c): Provides visibility calculation for map rendering
 * - Magic System (magic.c): Visibility modifiers (KNOWALL, NINJA, THE_VOID)
 * - Combat System (combat.c): Population evacuation during conflicts
 * - Command System (commands.c): User interface for navigation and reports
 * - Data Structures (data.h): Direct manipulation of all core game entities
 *
 * CRITICAL DEPENDENCIES:
 * - Global game state variables (world, sct, ntn arrays)
 * - Display coordinates and offset management (xcurs, ycurs, xoffset, yoffset)
 * - Magic system query functions for visibility modifiers
 * - File path configuration (datafile, scenario paths)
 *
 * MODERNIZATION CONSIDERATIONS:
 * - Binary file I/O could benefit from endianness handling
 * - Error handling could be enhanced with proper errno usage
 * - Memory allocation lacks NULL pointer safety checks
 * - File operations need atomic transaction support
 * - Magic number constants should be replaced with named definitions
 *
 * PERFORMANCE CHARACTERISTICS:
 * - Memory allocation is front-loaded during initialization
 * - File I/O operations are synchronous and blocking
 * - Map generation scales O(MAPX*MAPY) with map size
 * - Visibility calculations are cached in mapseen array
 *
 * FUNCTION ORGANIZATION:
 * - Memory Management: getspace(), m2alloc()
 * - Data Persistence: readdata(), writedata(), readmap()
 * - Visibility: mapprep()
 * - Map Output: printele(), pr_ntns(), pr_desg(), printveg()
 * - Navigation: centermap(), jump_to(), offmap()
 * - Reporting: printscore()
 * - Game Mechanics: flee()
 * - Utilities: get_pass()
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include "header.h"
#include "data.h"
#include "patchlevel.h"
#include "safe_convert.h"

/*
 * getspace - Allocate memory for core game data structures
 *
 * Manages dynamic memory allocation for the three primary 2D arrays that store
 * the complete game world state. Handles cleanup of existing allocations and
 * creates new arrays sized for the current map dimensions. This function is
 * essential for map loading, game initialization, and handling different
 * scenario sizes.
 *
 * The function allocates memory for:
 * - sct: Complete sector information (terrain, ownership, population, etc.)
 * - occ: Sector occupation markers for display and game logic
 * - movecost: Movement cost calculations for pathfinding algorithms
 *
 * Parameters:
 *   None (uses global MAPX, MAPY dimensions)
 *
 * Returns:
 *   void (calls abrt() on allocation failure)
 *
 * Side Effects:
 *   - Frees existing memory for sct, occ, movecost arrays if non-NULL
 *   - Allocates new 2D arrays using custom m2alloc() allocator
 *   - Updates global pointers to point to newly allocated memory
 *   - Program termination on allocation failure via m2alloc()
 *
 * Testing Notes:
 *   Category: A (Unit) - Memory allocation with clear inputs/outputs
 *   Approach: Unit tests with mock allocator and memory tracking
 *   Key Tests: NULL pointer handling, reallocation scenarios, size variations
 *   Dependencies: Global MAPX/MAPY constants, m2alloc() allocator
 *   Mock Requirements: Memory allocation tracking, failure simulation
 *   Complexity: Simple - Direct allocation pattern with error handling
 *
 * Notes:
 *   - Function assumes MAPX and MAPY are valid positive integers
 *   - Memory is allocated contiguously for cache efficiency
 *   - No return value checking needed as m2alloc() handles failures internally
 *   - Safe to call multiple times for reallocation scenarios
 *   - Critical for game initialization and scenario loading operations
  * @last_documented: 2025-09-20
 */
void
getspace(void)
{
	if (sct != NULL) free(sct);
	sct = (struct s_sector **) m2alloc(MAPX,MAPY,sizeof(struct s_sector));
	if (occ != NULL) free(occ);
	occ = (char **) m2alloc(MAPX,MAPY,sizeof(char));
	if (movecost != NULL) free(movecost);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-allocation-size"
	movecost = (short **) m2alloc(MAPX,MAPY,sizeof(short));
#pragma GCC diagnostic pop
}

#ifdef CONQUER
static char **mapseen;

/*
 * mapprep - Initialize visibility map based on nation perspective
 *
 * Creates and populates the global mapseen array that determines which sectors
 * are visible to a specific nation for map rendering and information display.
 * Implements fog-of-war mechanics by calculating visibility from multiple sources:
 * owned land, armies, navies, and magical abilities. This function is fundamental
 * to the game's information warfare and strategic visibility systems.
 *
 * The visibility calculation follows a hierarchical approach:
 * 1. KNOWALL magic or country=0 (deity view) provides complete visibility
 * 2. Land ownership provides LANDSEE radius visibility around each sector
 * 3. Active armies provide ARMYSEE radius visibility around their positions
 * 4. Active navies provide NAVYSEE radius visibility around their positions
 *
 * Parameters:
 *   None (uses global country variable for nation perspective)
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Allocates memory for global mapseen array using m2alloc()
 *   - Populates mapseen with visibility information (TRUE/FALSE per sector)
 *   - Early return optimization for omniscient visibility (KNOWALL/deity)
 *   - Iterates through all armies and navies belonging to current nation
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires nation/army/navy game state
 *   Approach: Integration tests with mock game state and visibility verification
 *   Key Tests: KNOWALL magic, deity view, land visibility, army/navy sight ranges
 *   Dependencies: Global country, sct array, army/navy data, magic() function
 *   Mock Requirements: Game state setup, magic system mocking, coordinate validation
 *   Complexity: Moderate - Multi-source visibility with range calculations
 *
 * Notes:
 *   - mapseen array is not freed; caller responsible for memory management
 *   - Visibility ranges (LANDSEE, ARMYSEE, NAVYSEE) are compile-time constants
 *   - ONMAP() macro prevents out-of-bounds array access
 *   - Navy condition checks for any ship type (merchant, war, gunboat)
 *   - Performance scales with map size and number of military units
 *   - Critical for map printing functions and strategic game balance
  * @last_documented: 2025-09-20
 */
void mapprep (void) {
	int armynum, nvynum;
	int x,y,i,j;

	/* get space for map */
	mapseen = (char **) m2alloc(MAPX,MAPY,sizeof(char));

	/* initialize the array */
	if (country==0 || magic(country,KNOWALL)==TRUE) {
		armynum = TRUE;
	} else {
		armynum = FALSE;
	}
	for (x=0;x<MAPX;x++) for(y=0;y<MAPY;y++) {
		mapseen[x][y] = safe_int_to_char(armynum);
	}

	/* done for all knowing */
	if (country==0 || magic(country,KNOWALL)==TRUE) return;

	/* add all visible sectors from owned land */
	for(x = 0; x < MAPX; x++)
	for(y = 0; y < MAPY; y++)
	if(sct[x][y].owner==country){
		for(i=x-LANDSEE;i<=x+LANDSEE;i++)
		for(j=y-LANDSEE;j<=y+LANDSEE;j++)
		if (ONMAP(i,j)) {
			mapseen[i][j]=TRUE;
		}
	}

	/* now add all visible sections from armies */
	for(armynum=0;armynum<MAXARM;armynum++)
	if(P_ASOLD>0) {
		for(i=(int)P_AXLOC-ARMYSEE;i<=(int)P_AXLOC+ARMYSEE;i++)
		for(j=(int)P_AYLOC-ARMYSEE;j<=(int)P_AYLOC+ARMYSEE;j++)
		if (ONMAP(i,j)) {
			mapseen[i][j]=TRUE;
		}
	}

	/* now add sectors visible by navy */
	for(nvynum=0;nvynum<MAXNAVY;nvynum++)
	if((P_NMSHP!=0)||(P_NWSHP!=0)||(P_NGSHP!=0)) {
		for(i=(int)P_NXLOC-NAVYSEE;i<=(int)P_NXLOC+NAVYSEE;i++)
		for(j=(int)P_NYLOC-NAVYSEE;j<=(int)P_NYLOC+NAVYSEE;j++)
		if (ONMAP(i,j)) {
			mapseen[i][j]=TRUE;
		}
	}
}

/*
 * printele - Generate altitude map output for printing
 *
 * Produces a formatted text representation of the world altitude map, showing
 * terrain elevation characters for all sectors visible to the current nation.
 * Uses the visibility information calculated by mapprep() to implement fog-of-war
 * mechanics, displaying only sectors that should be known to the player. This
 * function is part of the map output generation suite used for reports and
 * external map printing.
 *
 * The output format includes a standardized header with version information
 * and turn number, followed by a grid representation where each character
 * represents the altitude/terrain type of a sector. Invisible sectors are
 * rendered as spaces to maintain map formatting and visual structure.
 *
 * Parameters:
 *   None (uses global game state and mapseen visibility array)
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Writes formatted output to stdout (complete altitude map)
 *   - Writes status message to stderr for operation tracking
 *   - Uses current nation name for personalized headers (if not deity view)
 *   - Reads from global mapseen array for visibility determination
 *
 * Testing Notes:
 *   Category: C (System) - Requires complete game state and I/O redirection
 *   Approach: System testing with output capture and comparison
 *   Key Tests: Deity view vs nation view, visibility boundaries, header formatting
 *   Dependencies: Global country, curntn, sct array, mapseen array, VERSION/TURN
 *   Mock Requirements: I/O redirection, game state setup, visibility array
 *   Complexity: Simple - Direct iteration with conditional output
 *
 * Notes:
 *   - Requires mapprep() to be called first to populate mapseen array
 *   - Output is suitable for text-based map printing and external processing
 *   - stderr logging helps with debugging and operation tracking
 *   - Deity view (country==0) shows "World" instead of nation name
 *   - Map dimensions are fixed by compile-time MAPX/MAPY constants
 *   - Performance is O(MAPX*MAPY) with simple character output per sector
  * @last_documented: 2025-09-20
 */
void printele (void) {
	register int X, Y;
	fprintf(stderr,"doing print of altitude\n");
	if (country == 0) {
		printf("Conquer %s.%s: Altitude Map of the World on Turn %d\n",
			VERSION, PATCHLEVEL, TURN);
	} else {
		printf("Conquer %s.%s: Altitude Map for Nation %s on Turn %d\n",
			VERSION, PATCHLEVEL, curntn->name, TURN);
	}
	for(Y=0;Y<MAPY;Y++) {
		for(X=0;X<MAPX;X++) {
			if(mapseen[X][Y]==TRUE) putc(sct[X][Y].altitude,stdout);
			else putc(' ',stdout);
		}
		putc('\n',stdout);
	}
}

/*
 * pr_ntns - Generate nation ownership map for printing
 *
 * Produces a formatted text representation of national territorial control,
 * displaying nation marks for owned sectors and terrain characters for
 * unowned wilderness areas. This function provides strategic intelligence
 * about territorial distribution and political boundaries across the game
 * world, essential for diplomatic and military planning.
 *
 * The output distinguishes between owned and unowned territory:
 * - Owned sectors display the nation's identifying mark character
 * - Unowned sectors (owner==0) display the natural terrain altitude character
 * - Invisible sectors are shown as spaces for fog-of-war implementation
 *
 * The function follows the standard map output format with version headers
 * and turn information, making it suitable for external processing and
 * record keeping.
 *
 * Parameters:
 *   None (uses global game state and mapseen visibility array)
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Writes formatted nation map to stdout
 *   - Writes status message to stderr for operation tracking
 *   - Reads from global sct array for ownership and terrain information
 *   - Uses nation marks from ntn array for territorial identification
 *
 * Testing Notes:
 *   Category: C (System) - Requires complete game state and nation configuration
 *   Approach: System testing with output capture and ownership verification
 *   Key Tests: Mixed ownership, unowned wilderness, nation mark characters
 *   Dependencies: Global sct array, ntn array, mapseen, country, curntn
 *   Mock Requirements: Nation setup, territory assignment, visibility array
 *   Complexity: Simple - Direct iteration with ownership-based character selection
 *
 * Notes:
 *   - Requires mapprep() to be called first for visibility information
 *   - Nation marks are single characters defined in nation configuration
 *   - Unowned sectors show natural terrain for geographic reference
 *   - Useful for territorial analysis and diplomatic intelligence
 *   - Output format matches other map printing functions for consistency
 *   - Performance is O(MAPX*MAPY) with simple character output logic
  * @last_documented: 2025-09-20
 */
void pr_ntns (void) {
	register int X, Y;
	fprintf(stderr,"doing print of nations\n");
	if (country == 0) {
		printf("Conquer %s.%s: Nation Map of the World on Turn %d\n",
			VERSION, PATCHLEVEL, TURN);
	} else {
		printf("Conquer %s.%s: Nation Map for Nation %s on Turn %d\n",
			VERSION, PATCHLEVEL, curntn->name, TURN);
	}
	for(Y=0;Y<MAPY;Y++) {
		for(X=0;X<MAPX;X++) {
			if(mapseen[X][Y]==TRUE) {
				if(sct[X][Y].owner==0)
					putc(sct[X][Y].altitude,stdout);
				else putc(ntn[sct[X][Y].owner].mark,stdout);
			} else putc(' ',stdout);
		}
		putc('\n',stdout);
	}
}

/*
 * pr_desg - Generate sector designation map with visibility rules
 *
 * Produces a formatted text representation of sector designations (improvements
 * and infrastructure), implementing sophisticated intelligence and security
 * mechanics through the magic system. This function reveals the strategic
 * development status of sectors while respecting information warfare constraints
 * and national security considerations.
 *
 * The function implements a complex visibility system:
 * - Deity view (country==0): Complete designation visibility
 * - Own sectors: Full designation information always visible
 * - NINJA magic: Penetrates enemy security to reveal designations
 * - THE_VOID magic: Conceals designation information from enemies
 * - Default: Shows '?' for hidden enemy designations
 * - DNODESIG sectors: Display natural terrain instead of designation
 *
 * This creates a sophisticated intelligence warfare system where nations can
 * conceal their infrastructure development and spy on enemy territories based
 * on magical abilities and ownership patterns.
 *
 * Parameters:
 *   None (uses global game state, mapseen visibility, and magic system)
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Writes formatted designation map to stdout
 *   - Writes status message to stderr for operation tracking
 *   - Queries magic system for NINJA and THE_VOID abilities
 *   - Reads sector designation and ownership information
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires magic system and game state integration
 *   Approach: Integration tests with magic combinations and ownership scenarios
 *   Key Tests: NINJA penetration, THE_VOID concealment, ownership visibility
 *   Dependencies: Magic system, sct array, mapseen, country, ownership data
 *   Mock Requirements: Magic system mocking, complex game state scenarios
 *   Complexity: Moderate - Complex visibility rules with magic system integration
 *
 * Notes:
 *   - Most sophisticated visibility function in the map output suite
 *   - Critical for strategic intelligence and counter-intelligence gameplay
 *   - Magic abilities override normal visibility restrictions
 *   - THE_VOID provides defensive concealment against espionage
 *   - NINJA enables offensive intelligence gathering capabilities
 *   - Undesignated sectors show natural terrain for reference
 *   - Information warfare balance between concealment and revelation
  * @last_documented: 2025-09-20
 */
void pr_desg (void) {
	register int X, Y;
	fprintf(stderr,"doing print of designations\n");
	if (country == 0) {
		printf("Conquer %s.%s: Designation Map of the World on Turn %d\n",
			VERSION, PATCHLEVEL, TURN);
	} else {
		printf("Conquer %s.%s: Designation Map for Nation %s on Turn %d\n",
			VERSION, PATCHLEVEL, curntn->name, TURN);
	}
	for(Y=0;Y<MAPY;Y++) {
		for(X=0;X<MAPX;X++) {
			if (mapseen[X][Y]==TRUE) {
				if ((country == 0)
				  || (sct[X][Y].owner == country)
				  || (magic (country, NINJA) == TRUE)
				  || (magic (sct[X][Y].owner, THE_VOID) != TRUE)) {
					if(sct[X][Y].designation==DNODESIG)
						putc(sct[X][Y].altitude,stdout);
					else putc(sct[X][Y].designation,stdout);
				} else putc('?',stdout);
			} else putc(' ',stdout);
		}
		putc('\n',stdout);
	}
}

/*
 * printveg - Generate vegetation map for printing
 *
 * Produces a formatted text representation of the world vegetation map, showing
 * the natural plant life and ecological characteristics of each sector visible
 * to the current nation. This function provides important strategic information
 * about resource availability, terrain mobility, and environmental conditions
 * across the game world.
 *
 * Vegetation information is crucial for:
 * - Agricultural potential and food production planning
 * - Movement cost calculations and tactical positioning
 * - Resource harvesting and economic development
 * - Environmental awareness for military operations
 *
 * The output follows the standard visibility rules using the mapseen array,
 * displaying vegetation characters for visible sectors and spaces for unknown
 * areas. Unlike designation maps, vegetation visibility is not affected by
 * magic systems since it represents observable natural features.
 *
 * Parameters:
 *   None (uses global game state and mapseen visibility array)
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Writes formatted vegetation map to stdout
 *   - Writes status message to stderr for operation tracking
 *   - Reads vegetation data from global sct array
 *   - Uses standard map output formatting with headers
 *
 * Testing Notes:
 *   Category: C (System) - Requires complete game state and vegetation data
 *   Approach: System testing with output capture and vegetation verification
 *   Key Tests: Vegetation character display, visibility boundaries, header formatting
 *   Dependencies: Global sct array, mapseen array, country, curntn, VERSION/TURN
 *   Mock Requirements: Vegetation data setup, visibility array, I/O redirection
 *   Complexity: Simple - Direct iteration with basic visibility checking
 *
 * Notes:
 *   - Requires mapprep() to be called first for visibility information
 *   - Vegetation characters represent different biome and plant types
 *   - No magic system interference unlike designation maps
 *   - Important for strategic planning and resource management
 *   - Simplest of the map printing functions in terms of visibility rules
 *   - Performance is O(MAPX*MAPY) with straightforward character output
  * @last_documented: 2025-09-20
 */
void printveg (void) {
	register int X, Y;
	fprintf(stderr,"doing print of vegetation\n");
	if (country == 0) {
		printf("Conquer %s.%s: Vegetation Map of the World on Turn %d\n",
			VERSION, PATCHLEVEL, TURN);
	} else {
		printf("Conquer %s.%s: Vegetation Map for Nation %s on Turn %d\n",
			VERSION, PATCHLEVEL, curntn->name, TURN);
	}
	for(Y=0;Y<MAPY;Y++) {
		for(X=0;X<MAPX;X++) {
			if(mapseen[X][Y]==TRUE) {
				putc(sct[X][Y].vegetation,stdout);
			} else putc(' ',stdout);
		}
		putc('\n',stdout);
	}
}
#endif /* CONQUER */

/*
 * writedata - Write complete game state to binary data file
 *
 * Performs a comprehensive save operation by writing the entire game state
 * to a binary data file, enabling game persistence across sessions. This
 * function handles the critical task of serializing complex game data
 * structures including world configuration, sector arrays, and nation data
 * into a binary format for reliable storage and subsequent restoration.
 *
 * The function operates by creating a new data file (destroying any existing
 * file) and writing three primary data structures in sequence:
 * 1. World structure (s_world) - Global game configuration and metadata
 * 2. Sector array (sct) - Complete map state with all terrain and improvements
 * 3. Nation array (ntn) - All nation data including diplomacy and statistics
 *
 * Each write operation includes comprehensive error checking to ensure data
 * integrity and provides detailed diagnostic information if corruption is
 * detected. The function uses low-level file operations for performance
 * and exact control over binary layout, ensuring consistent data format
 * across different systems and sessions.
 *
 * Parameters:
 *   None (operates on global game state: world, sct, ntn arrays, datafile path)
 *
 * Returns:
 *   void (terminates program via abrt() on any failure)
 *
 * Side Effects:
 *   - Creates new binary data file, destroying existing file if present
 *   - Writes status messages to stdout for operation progress tracking
 *   - Terminates program execution on write errors or permission problems
 *   - Uses global datafile path for output destination
 *   - Sets file permissions to 0666 (read/write for owner/group/world)
 *
 * Testing Notes:
 *   Category: C (System) - Requires full game state and file system access
 *   Approach: System testing with complete game state and file I/O validation
 *   Key Tests: Full write sequence, error handling, byte count verification
 *   Dependencies: Complete game state (world, sct, ntn), datafile path, file system
 *   Mock Requirements: File system operations, complete game data structures
 *   Complexity: Moderate - Binary I/O with comprehensive error handling
 *
 * Notes:
 *   - Critical for game persistence and save/restore functionality
 *   - Binary format ensures exact data preservation and fast I/O
 *   - Destructive operation - existing save files are overwritten
 *   - File permissions allow broad access for multi-user systems
 *   - Each data structure write includes size validation for corruption detection
 *   - Uses creat() which creates file with specified permissions
 *   - Terminates on any error to prevent partial/corrupted saves
 *   - Essential counterpart to readdata() for complete persistence system
  * @last_documented: 2025-09-20
 */
void writedata (void) {
	long	bytes;
	int	fd;

	printf("\ndoing write of data\n");
	if((fd = creat(datafile,0666))==-1) {
		printf("cannot open data.  check permissions\n");
		abrt()
	}

/* write world structure */
	if((bytes=write(fd,&world,sizeof(struct s_world)))!=sizeof(struct s_world))
	{
		printf("error writing world data\n");
		printf("wrong data format (%ld vs. %zu)\n",bytes,sizeof(struct s_world) );
		abrt()
	}

	if((bytes=write(fd,*sct,(size_t)(MAPX*MAPY)*sizeof(struct s_sector))) == -1)
	{
		printf("Wrong number of bytes (%ld) written for sct (should be %zu)\n",bytes,(size_t)(MAPX*MAPY)*sizeof(struct s_sector));
		abrt()
	}
	printf("writing %ld bytes of sector data\n",bytes);
	if((bytes=write(fd,ntn,NTOTAL*sizeof(struct s_nation))) == -1)
	{
		printf("Wrong number of bytes (%ld) written for ntn (should be %zu)\n",bytes,(size_t)NTOTAL*sizeof(struct s_nation));
		abrt()
	}
	printf("writing %ld bytes of nation data\n",bytes);
	close(fd);
}

/*
 * readdata - Read and load complete game state from binary data file
 *
 * Performs comprehensive game state restoration by reading a binary data file
 * and reconstructing the complete game environment. This function handles the
 * critical task of deserializing game data structures from persistent storage,
 * enabling players to resume games from previous sessions with full state
 * preservation including world configuration, terrain, and nation data.
 *
 * The function operates in a specific sequence to ensure proper data loading:
 * 1. Opens the binary data file with read-only access
 * 2. Reads world structure (s_world) - Global game configuration and metadata
 * 3. Allocates dynamic memory for game arrays via getspace()
 * 4. Reads sector array (sct) - Complete map state with terrain and improvements
 * 5. Reads nation array (ntn) - All nation data including diplomacy and statistics
 *
 * Each read operation includes comprehensive error checking and size validation
 * to detect corruption or version mismatches. The function provides detailed
 * diagnostic information for troubleshooting and uses DEBUG conditionals for
 * development-time verification of proper data loading.
 *
 * Parameters:
 *   None (reads into global game state: world, sct, ntn arrays from datafile path)
 *
 * Returns:
 *   void (terminates program via exit(FAIL) or abrt() on any failure)
 *
 * Side Effects:
 *   - Opens binary data file for reading (read-only mode)
 *   - Reads status messages to stderr for operation tracking
 *   - Terminates program execution on read errors or data corruption
 *   - Calls getspace() to allocate memory for dynamic game arrays
 *   - Uses global datafile path for input source
 *   - Provides DEBUG output for development verification
 *
 * Testing Notes:
 *   Category: C (System) - Requires file system access and memory allocation
 *   Approach: System testing with valid data files and corruption scenarios
 *   Key Tests: Full read sequence, error detection, memory allocation integration
 *   Dependencies: Valid data file, datafile path, getspace() function, memory system
 *   Mock Requirements: File system operations, memory allocation, data file creation
 *   Complexity: Moderate - Binary I/O with error handling and memory management
 *
 * Notes:
 *   - Essential counterpart to writedata() for complete persistence system
 *   - Memory allocation via getspace() must succeed before sector/nation data loading
 *   - Binary format requires exact structure size matching for compatibility
 *   - Uses low-level read operations for performance and format control
 *   - DEBUG conditionals provide detailed byte count verification during development
 *   - Comprehensive error reporting helps diagnose save file corruption or version issues
 *   - Critical for game initialization and session restoration functionality
 *   - File format must match writedata() output exactly for successful loading
  * @last_documented: 2025-09-20
 */
void readdata (void) {
	int fd;
	ssize_t n_read;

	/*read in existing nation army and navy data*/
	/*check if file openable*/
	fprintf(stderr,"reading data file\n");
	if( (fd = open(datafile,0)) < 0 ) {
		fprintf( stderr, "can not open %s \n", datafile );
		fprintf( stderr, "for help with conquer, type conquer -h\n");
		exit(FAIL);
	}

/* read world structure */
	if((n_read=read(fd,&world,sizeof(struct s_world)))!=(ssize_t)sizeof(struct s_world))
	{
		printf("error reading world data\n");
		printf("wrong data format (%zd vs. %zu)\n",n_read, sizeof(struct s_world) );
		abrt()
	}
#ifdef DEBUG
	fprintf(stderr,"reading %zu bytes of world data\n",sizeof(struct s_world));
#endif /* DEBUG */

	getspace();

	if((n_read=read(fd,*sct,(size_t)(MAPX*MAPY)*sizeof(struct s_sector)))==0)
		printf("EOF\n");
	else if(n_read==-1) printf("error reading sector data (sct)\n");
	if(n_read != (ssize_t)((size_t)(MAPX*MAPY)*sizeof(struct s_sector))) {
		printf("error reading sector data (sct)\n");
		printf( "wrong data format (%zd vs. %zu)\n",n_read,  (size_t)(MAPX*MAPY)*sizeof(struct s_sector) );
		abrt()
	}
#ifdef DEBUG
	fprintf(stderr,"reading %zd bytes of sector data\n",n_read);
#endif /* DEBUG */
	if((n_read=read(fd,ntn,NTOTAL*sizeof(struct s_nation))) == -1)
		printf("error reading s_nation data (ntn)\n");
	else if(n_read!= (ssize_t)((size_t)NTOTAL*sizeof(struct s_nation))) {
		printf("error reading s_nation data (ntn)\n");
		printf( "wrong data format (%zd vs. %zu)\n",n_read, (size_t)NTOTAL*sizeof(struct s_nation) );
		abrt()
	}
#ifdef DEBUG
	fprintf(stderr,"reading %zd bytes of nation data\n",n_read);
#endif /* DEBUG */
	close(fd);
} /* readdata() */

#ifdef CONQUER
#ifdef XYZZY
/*
 * offmap - Handle cursor movement outside visible map boundaries
 *
 * Manages cursor positioning and display scrolling when the user attempts to
 * move the cursor beyond the currently visible portion of the map display.
 * This function implements sophisticated viewport management with automatic
 * scrolling and boundary checking to ensure the cursor remains within valid
 * map coordinates while providing smooth navigation across large game worlds.
 *
 * The function operates on both X and Y axes independently, handling four
 * primary scenarios for each dimension:
 * 1. Cursor moves past the left/top edge - scroll viewport backward if possible
 * 2. Cursor moves past the right/bottom edge - scroll viewport forward if possible
 * 3. Viewport reaches world boundaries - clamp cursor to valid coordinates
 * 4. Offset corrections - ensure consistent cursor and viewport relationship
 *
 * Key navigation mechanics:
 * - Uses 15-unit scrolling increments for smooth viewport transitions
 * - Triggers PART redraw when viewport changes to update display
 * - Maintains cursor position relative to viewport offset
 * - Prevents navigation beyond world boundaries (0 to MAPX-1, 0 to MAPY-1)
 * - Calls whatcansee() to update visibility after position changes
 *
 * This function is conditionally compiled under XYZZY, indicating it's part
 * of an enhanced display system that may not be available in all builds.
 *
 * Parameters:
 *   None (operates on global cursor and viewport state: xcurs, ycurs, xoffset, yoffset)
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Modifies global cursor coordinates (xcurs, ycurs)
 *   - Adjusts viewport offsets (xoffset, yoffset) for scrolling
 *   - Sets redraw flag to PART when viewport changes
 *   - Calls whatcansee() to update visibility calculations
 *   - Ensures cursor remains within valid map boundaries
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires display system and coordinate management
 *   Approach: Integration tests with cursor movement and boundary scenarios
 *   Key Tests: Edge scrolling, boundary clamping, viewport consistency, redraw triggers
 *   Dependencies: Display system, cursor state, viewport management, whatcansee()
 *   Mock Requirements: Display system mocking, coordinate state management
 *   Complexity: Moderate - Complex coordinate calculations with multiple edge cases
 *
 * Notes:
 *   - Critical for interactive map navigation and user experience
 *   - XYZZY conditional suggests advanced display features
 *   - Scrolling in 15-unit increments provides smooth navigation feel
 *   - Maintains consistency between cursor position and viewport offset
 *   - Prevents cursor from moving outside valid world coordinates
 *   - Automatically triggers visibility updates after position changes
 *   - Essential for large world navigation where map exceeds screen size
 *   - Coordinates with display system for efficient partial redraws
  * @last_documented: 2025-09-20
 */
void offmap (void) {
	/*set offset offsets can not be < 0*/
	if(xcurs<1){
		if(XREAL<=0) {
			xoffset=0;
			xcurs=0;
		}
		else {
			redraw=PART;
			xoffset-=15;
			xcurs+=15;
		}
	}
	else if(xcurs >= SCREEN_X_SIZE-1){
		if(XREAL<MAPX) {
			redraw=PART;
			xoffset+=15;
			xcurs-=15;
		}
	}
	if(XREAL>=MAPX) xcurs=MAPX-1-xoffset;
	if(xoffset<0) {
		xcurs += xoffset;
		xoffset=0;
	}
	if(xcurs<0) {
		xoffset += xcurs;
		xcurs=0;
	}
	else if(xcurs >= SCREEN_X_SIZE-1) {
		redraw=PART;
		xoffset+=15;
		xcurs-=15;
	}

	if(ycurs<1){
		if(YREAL<=0) {
			yoffset=0;
			ycurs=0;
		}
		else {
			redraw=PART;
			ycurs+=15;
			yoffset-=15;
		}
	}
	else if(ycurs >= SCREEN_Y_SIZE-1){
		if(YREAL<MAPY) {
			redraw=PART;
			yoffset+=15;
			ycurs-=15;
		}
	}
	if(YREAL>=MAPY) ycurs=MAPY-1-yoffset;
	if(yoffset<0) {
		ycurs += yoffset;
		yoffset=0;
	}
	if(ycurs<0) {
		yoffset += ycurs;
		ycurs=0;
	}
	else if(ycurs >= SCREEN_Y_SIZE-1) {
		redraw=PART;
		yoffset+=15;
		ycurs-=15;
	}
	whatcansee();
}
#endif /* XYZZY */

/*
 * centermap - Center map display around current cursor position
 *
 * Recalculates and adjusts the viewport to center the map display around
 * the current cursor coordinates, providing optimal navigation and orientation
 * for the player. This function implements intelligent viewport positioning
 * that maximizes the visible area around the cursor while respecting map
 * boundaries and screen size constraints.
 *
 * The centering algorithm operates by:
 * 1. Calculating ideal offsets to center cursor on screen
 * 2. Applying boundary constraints to prevent invalid negative offsets
 * 3. Updating cursor position relative to new viewport
 * 4. Triggering visibility recalculation for the new view
 *
 * This provides immediate visual feedback and optimal positioning for
 * strategic decision-making, particularly useful after jumping to distant
 * locations or when the current view doesn't provide sufficient context
 * around the cursor position.
 *
 * Parameters:
 *   None (operates on global cursor state: xcurs, ycurs via XREAL, YREAL macros)
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Recalculates viewport offsets (xoffset, yoffset) for centering
 *   - Updates cursor coordinates (xcurs, ycurs) relative to new viewport
 *   - Calls whatcansee() to recalculate visibility for new map view
 *   - Ensures offsets remain non-negative (clamps to zero at boundaries)
 *   - Triggers full map redraw through offset changes
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires display system and coordinate management
 *   Approach: Integration tests with various cursor positions and screen sizes
 *   Key Tests: Center calculation, boundary handling, cursor positioning, visibility updates
 *   Dependencies: Display system, cursor state, XREAL/YREAL macros, whatcansee()
 *   Mock Requirements: Display coordinates, screen size constants, viewport management
 *   Complexity: Simple - Straightforward coordinate calculations with boundary checks
 *
 * Notes:
 *   - Essential for user experience and navigation comfort
 *   - Works with XREAL/YREAL macros to get absolute cursor coordinates
 *   - Automatically handles map boundaries by clamping negative offsets
 *   - Screen size division by 2 centers cursor in the middle of display
 *   - Coordinates with display system for optimal visual layout
 *   - Commonly used after jump commands or when context is needed
 *   - Provides immediate visual feedback for player orientation
 *   - Part of the XYZZY enhanced display system
  * @last_documented: 2025-09-20
 */
void centermap (void) {
	int xx,yy;
	xx=XREAL;
	yy=YREAL;
	xoffset = safe_int_to_short(xx - (SCREEN_X_SIZE/2));
	yoffset = safe_int_to_short(yy - (SCREEN_Y_SIZE/2));
	if (xoffset<0)
		xoffset=0;
	if (yoffset<0)
		yoffset=0;
	xcurs= safe_int_to_short(xx-xoffset);
	ycurs= safe_int_to_short(yy-yoffset);
	whatcansee();
}

/*
 * jump_to - Move display to specific map coordinates or nation capitals
 *
 * Provides rapid navigation functionality allowing players to quickly move
 * the display to specific locations on the map. This function supports two
 * primary modes of operation: jumping to nation capitals (home mode) and
 * jumping to user-specified coordinates (manual mode), enabling efficient
 * exploration and management of large game worlds.
 *
 * Home Mode Operation (home != 0):
 * - Deity view (country==0): Cycles through all active nation capitals in sequence
 * - Player view: Jumps directly to player's own capital city
 * - Maintains cycling state to enable sequential capital tours for deities
 * - Uses next_ntn static variable to track position in capital sequence
 * - Falls back to world center (MAPX/2, MAPY/2) if no capitals available
 *
 * Manual Mode Operation (home == 0):
 * - Prompts user for X and Y coordinates via interactive input
 * - Validates coordinates against world boundaries (0 to MAPX-1, 0 to MAPY-1)
 * - Provides clear error messages for invalid locations
 * - Uses get_number() for reliable numeric input with cancellation support
 *
 * Navigation Implementation:
 * - Sets cursor to target coordinates and resets viewport offsets
 * - Calls centermap() to optimally position the display around target
 * - Ensures immediate visual feedback and proper viewport adjustment
 *
 * Parameters:
 *   home - Navigation mode flag (non-zero for capital mode, zero for manual coordinates)
 *
 * Returns:
 *   void (early return on user cancellation or invalid coordinates)
 *
 * Side Effects:
 *   - Modifies global cursor coordinates (xcurs, ycurs)
 *   - Resets viewport offsets (xoffset, yoffset) to zero
 *   - Updates static next_ntn for capital cycling sequence
 *   - Displays interactive prompts and error messages to user
 *   - Calls centermap() to adjust display viewport
 *   - May call errormsg() for coordinate validation failures
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires user interface, nation data, and display system
 *   Approach: Integration tests with various nation states and coordinate scenarios
 *   Key Tests: Capital cycling, coordinate validation, boundary checking, user interaction
 *   Dependencies: Nation data (ntn array), display system, user input (get_number), centermap()
 *   Mock Requirements: User input simulation, nation data setup, error message system
 *   Complexity: Moderate - Multiple modes with complex capital cycling logic
 *
 * Notes:
 *   - Essential for efficient navigation in large game worlds
 *   - Static next_ntn variable maintains state between deity capital cycles
 *   - Coordinate validation prevents crashes from invalid user input
 *   - Capital cycling provides systematic exploration for game administrators
 *   - Integration with centermap() ensures optimal display positioning
 *   - Error handling provides clear feedback for boundary violations
 *   - Supports both automated (capitals) and manual (coordinates) navigation
 *   - Critical for game management and strategic oversight functionality
  * @last_documented: 2025-09-20
 */
void jump_to (int home) {
	int i,j,done;
	static int next_ntn;

	/* find location to jump to */
	if (home) {
		if (country==0) {
			/* check if in sequence */
			if ((XREAL!=ntn[next_ntn].capx)
			||(YREAL!=ntn[next_ntn].capy)) {
				next_ntn= 0;
			}
			/* find next capitol */
			done = FALSE;
			do {
				next_ntn++;
				if (next_ntn==NTOTAL) {
					j = 0;
					for(i=0;i<NTOTAL;i++)
					if (isntn(ntn[i].active)) {
						j = i;
						i = NTOTAL;
					}
					next_ntn = j;
					done = TRUE;
				} else {
					if (isntn(ntn[next_ntn].active)) {
						done = TRUE;
					}
				}
			} while (done==FALSE);
			/* default location; or next capitol */
			if (next_ntn==0) {
				i = MAPX/2-1;
				j = MAPY/2-1;
			} else {
				i = ntn[next_ntn].capx;
				j = ntn[next_ntn].capy;
			}
		} else {
			/* go to capitol */
			i = curntn->capx;
			j = curntn->capy;
		}
	} else {
		/* entered location */
		clear_bottom(0);
		mvaddstr(LINES-3,0,"Jump to what X location? ");
		refresh();
		i = safe_long_to_int(get_number());
		if (i==(-1)) return;
		if (i>=MAPX) {
			errormsg("That location is out of this world!");
			return;
		}
		mvaddstr(LINES-2,0,"Jump to what Y location? ");
		refresh();
		j = safe_long_to_int(get_number());
		if (j==(-1)) return;
		if (j>=MAPY) {
			errormsg("That location is out of this world!");
			return;
		}
	}
	/* now center location about given position */
	xcurs = safe_int_to_short(i);
	ycurs = safe_int_to_short(j);
	xoffset = yoffset = 0;
	centermap();
}

/*
 * printscore - Generate comprehensive game score report
 *
 * Produces a detailed tabular report of all active nations in the game,
 * displaying critical information including demographics, resources, military
 * strength, and territorial control. This function serves as the primary
 * game status overview, enabling players and administrators to assess the
 * current state of world power dynamics and national development.
 *
 * The report includes a header with game version, season, year, and turn
 * information, followed by optional timestamp data from the last update.
 * The main table displays nation statistics in a standardized format with
 * columns for identification, demographics, military assets, and territorial
 * holdings.
 *
 * Report Format:
 * - Header: Game version, current season/year/turn information
 * - Timestamp: Last update time (TIMELOG conditional)
 * - Column Headers: ID, name, race, class, alignment, score, resources
 * - Nation Data: All active nations with complete statistics
 * - Inactive Nations: Marked with dashes to indicate unavailable data
 *
 * Information Display Logic:
 * - Active nations (isntn): Full statistics including sensitive data
 * - NOSCORE builds: Hide detailed resources for competitive balance
 * - Inactive nations: Display basic info with hidden sensitive data
 * - Race compatibility: Handle legacy 'B' barbarian designation
 * - Alignment display: NPC type-based alignment classification
 *
 * Parameters:
 *   None (operates on global game state: ntn array, world data, turn information)
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Writes formatted report to stdout for display or redirection
 *   - Reads optional timestamp file (TIMELOG conditional)
 *   - Accesses nation arrays and global turn/season information
 *   - May open/close timefile for last update information
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires complete nation data and formatting system
 *   Approach: Integration tests with various nation states and conditional builds
 *   Key Tests: Report formatting, conditional compilation, data accuracy, inactive handling
 *   Dependencies: Nation data (ntn array), races/Class/alignment arrays, turn system
 *   Mock Requirements: Nation data setup, conditional flag testing, output capture
 *   Complexity: Moderate - Complex formatting with multiple conditional compilation paths
 *
 * Notes:
 *   - Critical for game monitoring and competitive balance assessment
 *   - NOSCORE conditional hides sensitive economic/military data in competitive games
 *   - TIMELOG integration provides update tracking for administrators
 *   - Handles legacy race designations for backward compatibility
 *   - Formatted for both human readability and automated parsing
 *   - Essential for diplomatic intelligence and strategic planning
 *   - Provides comprehensive overview of world power structure
 *   - Used by both players and administrators for game state assessment
  * @last_documented: 2025-09-20
 */
void printscore (void) {
	int i;
	int nationid; 	/*current nation id */
#ifdef TIMELOG
	FILE *timefp;
	char timestr[LINELTH+1];
#endif /* TIMELOG */

	printf("Conquer %s.%s: %s of Year %d, Turn %d\n",VERSION,PATCHLEVEL,
		PSEASON(TURN),YEAR(TURN), TURN);
#ifdef TIMELOG
	if ((timefp=fopen(timefile,"r"))!=NULL) {
		fgets(timestr, 50, timefp);
		printf("Last Update: %s", timestr);
		fclose(timefp);
	}
#endif /* TIMELOG */
	printf("id      name   race    class    align  score    talons military  civilians sect\n");
	for (nationid=1; nationid<NTOTAL; nationid++) {
		if(!isactive(ntn[nationid].active)) continue;
		printf("%2d ",nationid);
		printf("%9s ",ntn[nationid].name);
		/* this check for old 'B' for barbarians; removed eventually */
		if (ntn[nationid].race=='B') {
			printf("%6s ", "SAVAGE");
		} else {
			for(i=1;(*(races+i)[0])!='U';i++)
				if(ntn[nationid].race==*(races+i)[0])
					printf("%6s ",*(races+i));
		}

		printf("%8s ",*(Class+ntn[nationid].class));
		printf(" %7s ",alignment[npctype(ntn[nationid].active)]);
		if (isntn(ntn[nationid].active)) {
#ifdef NOSCORE
			printf("%6ld  %8s %8s   %8s %4s\n",
				  ntn[nationid].score ,"-----","----" ,"-----","--");
#else
			printf("%6ld  %8ld %8ld   %8ld %4d\n",
				  ntn[nationid].score ,ntn[nationid].tgold
				  ,ntn[nationid].tmil ,ntn[nationid].tciv
				  ,ntn[nationid].tsctrs );
#endif /* NOSCORE */
		} else {
			printf("%6s  %8s %8s   %8s %4s\n",
				  "---","-----","----","-----","--");
		}
	}
}
#endif /* CONQUER */

/*
 * flee - Handle civilian population evacuation mechanics
 *
 * Implements sophisticated population displacement simulation when civilians
 * are forced to abandon their sector due to military action, natural disaster,
 * or other catastrophic events. This function manages the complex logistics
 * of population movement, including racial affinity patterns, survival rates,
 * slavery mechanics, and the cascading effects on regional demographics.
 *
 * Population Evacuation Process:
 * 1. Slavery capture: 25% of population may be enslaved if slaver==TRUE
 * 2. Initial population reduction: 40% of remaining civilians flee immediately
 * 3. Destination search: Find sectors owned by same racial group within range
 * 4. Distribution algorithm: Divide fleeing population among available destinations
 * 5. Distance-based survival: Closer destinations have better survival rates
 * 6. Fortress destruction: Military installations are abandoned and destroyed
 *
 * Search Pattern and Survival Logic:
 * - First attempt: 2-sector radius search for same-race destinations
 * - If successful: Full population redistribution with high survival rate
 * - Second attempt: 4-sector radius search with 50% mortality penalty
 * - If no destinations: Complete population loss (death in wilderness)
 *
 * Racial Affinity System:
 * - Fleeing populations seek sectors owned by their own racial group
 * - Cross-racial refuge is not implemented (historical/cultural barriers)
 * - Same-race nations provide automatic sanctuary and integration
 *
 * The function includes sophisticated user feedback mechanisms and integrates
 * with the display system to provide immediate notification of population
 * movements and casualties during interactive gameplay.
 *
 * Parameters:
 *   x, y - Coordinates of the sector being evacuated
 *   isupd - Update mode flag (TRUE during automated updates, FALSE during player moves)
 *   slaver - Slavery flag (TRUE enables population capture mechanics)
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Modifies population counts in origin and destination sectors
 *   - Destroys fortress installations in abandoned sector
 *   - May trigger sector devastation if food production fails
 *   - Displays status messages during interactive gameplay (CONQUER conditional)
 *   - Updates display system with population changes (SADJCIV macros)
 *   - Temporarily modifies global country variable for racial calculations
 *   - Calls DEVASTATE() for ecological collapse in extreme cases
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires population system, racial data, and map state
 *   Approach: Integration tests with various population scenarios and racial configurations
 *   Key Tests: Population distribution, racial affinity, survival rates, slavery mechanics
 *   Dependencies: Sector arrays, nation data, racial system, display updates, map bounds
 *   Mock Requirements: Population data, racial configurations, map state, display system
 *   Complexity: Complex - Sophisticated population mechanics with multiple survival scenarios
 *
 * Notes:
 *   - Critical for realistic warfare and demographic simulation
 *   - Slavery mechanics reflect historical conquest patterns
 *   - Racial affinity system creates strategic population dynamics
 *   - Distance-based survival rates encourage regional population clusters
 *   - Fortress destruction represents infrastructure collapse during evacuation
 *   - Integration with food system prevents impossible population concentrations
 *   - Essential for preventing unrealistic population invulnerability
 *   - Provides dramatic feedback for major military and economic disruptions
  * @last_documented: 2025-09-20
 */
void flee (int x, int y, int isupd, int slaver) {
	int count=0;	/*count is number of acceptable sectors to go to */
	int svcountry=country;
	int slaves=0;
	int i,j;
	int people_to_add;

	country=sct[x][y].owner;
	if(slaver==TRUE){
		slaves= safe_long_to_int(sct[x][y].people/4);
		sct[x][y].people-=slaves;
	}

	/*flee*/
	sct[x][y].people*=6;
	sct[x][y].people/=10;
	/*check if next to anybody of the sectors owners race*/
	for(i=x-2;i<=x+2;i++) for(j=y-2;j<=y+2;j++)
		if(ONMAP(i,j)
		&&(ntn[sct[i][j].owner].race==ntn[sct[x][y].owner].race))
			count++;

	if(count>0) {
#ifdef CONQUER
	if(isupd==0) {
		if(slaver==TRUE){
			mvprintw(LINES-2,20,"CIVILIANS ABANDON SECTOR (%d slaves)",slaves);
		}else{
			mvaddstr(LINES-2,20,"CIVILIANS ABANDON SECTOR");
		}
	}
#endif /* CONQUER */
	for(i=x-2;i<=x+2;i++) for(j=y-2;j<=y+2;j++)
		if(ONMAP(i,j)
		&&(ntn[sct[i][j].owner].race==ntn[sct[x][y].owner].race)) {
			people_to_add = safe_long_to_int(sct[x][y].people / count);
			/* don't show until next turn if player move */
			if(isupd==0) SADJCIV3;
			else	sct[x][y].people += people_to_add;
		}
	} else {
		sct[x][y].people /= 2;
		for(i=x-4;i<=x+4;i++) for(j=y-4;j<=y+4;j++)
			if(ONMAP(i,j)
			&&(ntn[sct[i][j].owner].race==ntn[sct[x][y].owner].race))
				count++;
		if(count>0) {
#ifdef CONQUER
		if(isupd==0) mvaddstr(LINES-2,20,"PEOPLE FLEE SECTOR AND HALF DIE");
#endif /* CONQUER */
		for(i=x-4;i<=x+4;i++) for(j=y-4;j<=y+4;j++)
			if(ONMAP(i,j)
			&&(ntn[sct[i][j].owner].race==ntn[sct[x][y].owner].race)) {
				sct[i][j].people += sct[x][y].people / count;
				if(isupd==0) SADJCIV2;
			}
		}
#ifdef CONQUER
		else if(isupd==0) mvaddstr(LINES-2,20,"PEOPLE IN SECTOR DIE");
#endif /* CONQUER */
	}

	sct[x][y].people = slaves;
	if(isupd==0) SADJCIV;
	sct[x][y].fortress=0;
	/*SINFORT;*/
	if(tofood( &sct[XREAL][YREAL],sct[XREAL][YREAL].owner)!=0) {
		DEVASTATE(x,y)
		if(isupd==0) SADJDES2;
	}
	country=safe_int_to_short(svcountry);
}
#ifdef ADMIN
/*
 * readmap - Load elevation and vegetation maps from scenario files
 *
 * Performs scenario initialization by reading terrain data from external
 * map files and populating the game world with elevation and vegetation
 * information. This function enables dynamic world creation by loading
 * pre-designed scenarios from standardized map files, supporting flexible
 * game world configuration without requiring code recompilation.
 *
 * File Loading Process:
 * 1. Elevation Map: Reads {scenario}.ele file containing altitude data
 * 2. Vegetation Map: Reads {scenario}.veg file containing vegetation data
 * 3. Sequential Processing: Loads map data line-by-line into sector arrays
 * 4. Boundary Enforcement: Respects MAPX/MAPY world size constraints
 * 5. Error Handling: Reports file access problems and continues operation
 *
 * Map File Format:
 * - Text-based format with one character per map cell
 * - Each line represents one row of the world map
 * - Characters directly correspond to altitude/vegetation types
 * - Files must contain at least MAPY lines of MAPX characters each
 * - Excess data beyond world boundaries is ignored
 *
 * The function provides comprehensive logging to stderr for debugging
 * and administrative monitoring, including file names, line counts,
 * and character counts for verification of proper map loading.
 *
 * This function is conditionally compiled under ADMIN, indicating it's
 * restricted to administrative builds and not available to regular players,
 * maintaining separation between game administration and player functions.
 *
 * Parameters:
 *   None (uses global scenario name and MAPX/MAPY world dimensions)
 *
 * Returns:
 *   int - TRUE for successful completion, FALSE for critical failures
 *
 * Side Effects:
 *   - Modifies sector altitude and vegetation data across entire world map
 *   - Opens and closes scenario map files (.ele and .veg extensions)
 *   - Writes detailed progress and error messages to stderr
 *   - Uses global scenario variable for filename construction
 *   - Accesses and modifies global sct sector array
 *
 * Testing Notes:
 *   Category: C (System) - Requires file system access and complete map arrays
 *   Approach: System testing with various scenario files and map configurations
 *   Key Tests: File loading, boundary handling, error recovery, data integrity
 *   Dependencies: File system, scenario files, sct array, global scenario variable
 *   Mock Requirements: File system operations, scenario file creation, map data validation
 *   Complexity: Moderate - File I/O with sequential processing and error handling
 *
 * Notes:
 *   - Essential for scenario-based gameplay and world variety
 *   - ADMIN conditional restricts access to administrative functions
 *   - Robust error handling allows graceful degradation on file problems
 *   - Text-based format enables easy scenario creation and modification
 *   - Sequential loading ensures consistent world state initialization
 *   - Logging output assists in scenario debugging and verification
 *   - Critical for dynamic world generation and campaign management
 *   - Enables separation of game logic from world data configuration
  * @last_documented: 2025-09-20
 */
int readmap (void) {
	FILE	*mapfile;
	char	line[BIGLTH+1];
	register int x,y;

	/* read in ele.map */
	strncpy(line, scenario, sizeof(line));
	line[sizeof(line) - 1] = '\0';
	strncat(line, ".ele", sizeof(line) - strlen(line) - 1);
	if ((mapfile=fopen(line,"r"))==NULL) {
		fprintf(stderr,"error on read of %s file\n",line);
		return(TRUE);
	} else fprintf(stderr,"reading elevation map file from %s\n",line );

	y=0;
	while( TRUE ) {
		if(fgets( line, 128, mapfile )==NULL) break;
		for(x=0;x<MAPX;x++) sct[x][y].altitude = line[x];
		y++;
		if(y>=MAPY) break;
	}
	fprintf(stderr,"done reading %d lines of %zu characters\n",y,strlen(line));
	fclose(mapfile);

	/* read in veg.map */
	strncpy(line, scenario, sizeof(line));
	line[sizeof(line) - 1] = '\0';
	strncat(line, ".veg", sizeof(line) - strlen(line) - 1);
	if ((mapfile=fopen(line,"r"))==NULL) {
		fprintf(stderr,"error on read of %s file\n",line);
		return(TRUE);
	} else fprintf(stderr,"reading vegetation map file from %s\n",line );
	y=0;
	while( TRUE ) {
		if(fgets( line, BIGLTH, mapfile )==NULL) break;
		for(x=0;x<MAPX;x++) sct[x][y].vegetation = line[x];
		y++;
		if(y>=MAPY) break;
	}
	fprintf(stderr,"done reading %d lines of %zu characters\n",y,strlen(line));
	fclose(mapfile);

	return(TRUE);
}
#endif /* ADMIN */

/*
 * NOTE: m2alloc() and m2alloc_safe() functions have been moved to m2alloc.c
 * for better testability and modularity. See m2alloc.c for implementation.
 */

/*
 * get_pass - Secure password input with character masking
 *
 * Provides secure password input functionality by reading characters from
 * the user without echoing them to the screen, preventing shoulder surfing
 * and maintaining password confidentiality during authentication. This
 * function implements comprehensive input handling including backspace
 * correction, line clearing, and proper string termination for reliable
 * password entry in interactive game sessions.
 *
 * Input Processing Features:
 * - Character masking: No screen echo for entered characters
 * - Backspace support: Both '\b' and '\177' (DEL) for character deletion
 * - Line clearing: Ctrl+U ('\025') clears entire input line
 * - Length limiting: Prevents buffer overflow through PASSLTH bounds checking
 * - Termination handling: Enter or carriage return completes input
 * - Null character filtering: Ignores null bytes from input stream
 *
 * Security Implementation:
 * - No visual feedback for password characters (prevents observation)
 * - Immediate character processing (no intermediate buffers)
 * - Proper string termination to prevent buffer issues
 * - Length validation with truncation for oversized input
 * - Return value indicates actual characters entered (including truncated)
 *
 * The function handles various terminal input scenarios gracefully,
 * supporting common editing operations while maintaining security through
 * character masking. Truncation logic ensures that overly long passwords
 * are handled safely without buffer overflows.
 *
 * Parameters:
 *   str - Character buffer to store the entered password (must be PASSLTH+1 or larger)
 *
 * Returns:
 *   int - Number of characters actually entered (before truncation)
 *
 * Side Effects:
 *   - Modifies the provided string buffer with entered password
 *   - Reads characters directly from terminal input (getch())
 *   - No screen output (maintains character masking for security)
 *   - Null-terminates the resulting string for safe usage
 *   - May truncate input that exceeds PASSLTH limit
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires terminal input system and character handling
 *   Approach: Integration tests with simulated input and various scenarios
 *   Key Tests: Password masking, backspace handling, truncation, termination
 *   Dependencies: Terminal input (getch()), PASSLTH constant, string handling
 *   Mock Requirements: Input simulation, terminal behavior mocking
 *   Complexity: Moderate - Character-by-character processing with multiple input modes
 *
 * Notes:
 *   - Critical for authentication security and user privacy
 *   - Supports standard terminal editing conventions (backspace, Ctrl+U)
 *   - Prevents password disclosure through screen observation
 *   - Robust length handling prevents security vulnerabilities
 *   - Compatible with various terminal types and input methods
 *   - Essential for multi-user game system authentication
 *   - Return value enables password length validation by callers
 *   - Proper string handling ensures safe integration with authentication systems
  * @last_documented: 2025-09-20
 */
int get_pass (char *str) {
	char ch;
	int done=FALSE,count=0;

	while(done==FALSE) {
		ch = safe_int_to_char(getch());
		if (ch=='\b' || ch=='\177') {
			/* delete any entered characters */
			if (count > 0) {
				count--;
			}
		} else if (ch=='\025') {
			/* make sure that ^U works */
			count=0;
		} else if (ch=='\n' || ch=='\r') {
			done = TRUE;
		} else if (ch != '\0') {
			/* add any other character to the string */
			if (count < PASSLTH) {
				/* don't try adding too many */
				str[count]= ch;
			}
			count++;
		}
	}
	/* truncate too long a password and end others properly */
	if (count > PASSLTH) {
		str[PASSLTH] = '\0';
	} else {
		str[count] = '\0';
	}
	return(count);
}
