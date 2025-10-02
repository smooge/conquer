/*
 * display.c - Screen display and map rendering system
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
 * ============================================================================
 * DISPLAY SYSTEM ARCHITECTURE
 * ============================================================================
 *
 * This file implements the complete visual display system for the Conquer game,
 * providing map rendering, user interface elements, and visual feedback systems.
 * The display system is built on top of the curses library and supports both
 * single and dual display modes with comprehensive highlighting capabilities.
 *
 * CORE ARCHITECTURAL COMPONENTS:
 *
 * 1. VISIBILITY SYSTEM
 * --------------------
 * The visibility system determines what parts of the map each player can see
 * based on land ownership, army positions, naval units, and magic powers.
 *
 * - hasseen array: Dynamic screen-sized array tracking visible positions
 * - whatcansee(): Core visibility calculation engine
 * - canbeseen(): Simple visibility query interface
 * - HAS_SEEN macro: Fast array access for visibility checking
 *
 * Visibility Sources:
 * - Owned land: LANDSEE radius around owned sectors
 * - Armies: ARMYSEE radius around army positions
 * - Navies: NAVYSEE radius around naval units
 * - Magic: KNOWALL provides complete visibility, NINJA penetrates concealment
 * - God mode: Administrators see entire map
 *
 * 2. DISPLAY MODE SYSTEM
 * ----------------------
 * Supports 11 different map display modes, each emphasizing different aspects
 * of the game world through specialized character representations.
 *
 * Display Modes:
 * - DI_FOOD: Food production potential (0-9, +)
 * - DI_VEGE: Vegetation/terrain types (symbols)
 * - DI_DESI: Designations and land use (letters)
 * - DI_CONT: Contour/altitude map (elevation symbols)
 * - DI_NATI: Nation ownership (nation marks)
 * - DI_RACE: Racial distribution (race characters)
 * - DI_MOVE: Movement costs (0-9, +, X for impassable)
 * - DI_DEFE: Defensive bonuses (calculated terrain values)
 * - DI_PEOP: Population density (0-9, I, V, X)
 * - DI_GOLD: Precious resources/jewels (0-9, +)
 * - DI_METAL: Metal resources (0-9, +)
 * - DI_ITEMS: Trade goods and special items (symbols)
 *
 * 3. HIGHLIGHTING SYSTEM
 * ----------------------
 * Visual emphasis system that helps players identify specific map features
 * using curses standout mode for enhanced visibility.
 *
 * Highlight Modes:
 * - HI_MOVE: Armies with movement points remaining
 * - HI_YARM: Player's armies
 * - HI_GOOD: Trade goods and special resources
 * - HI_OWN: Land ownership (different logic for god vs players)
 * - HI_ARMY: Any armies present
 * - HI_NONE: No highlighting
 *
 * 4. DUAL DISPLAY SUPPORT
 * -----------------------
 * Advanced feature allowing simultaneous display of two different map modes
 * side-by-side for comprehensive information viewing.
 *
 * - Primary display: Left character of each position
 * - Secondary display: Right character of each position
 * - Toggle functionality: Switch between display modes
 * - Independent highlighting: Each display can have different highlighting
 *
 * 5. COORDINATE SYSTEMS
 * ---------------------
 * The display system manages multiple coordinate systems for proper rendering:
 *
 * - Map coordinates: Global world coordinates (MAPX × MAPY)
 * - Screen coordinates: Current view window (SCREEN_X_SIZE × SCREEN_Y_SIZE)
 * - Cursor coordinates: Current player position (xcurs, ycurs)
 * - Offset coordinates: Screen-to-map translation (xoffset, yoffset)
 * - Display coordinates: 2x horizontal scaling for dual character display
 *
 * 6. MAGIC SYSTEM INTEGRATION
 * ---------------------------
 * The display system integrates deeply with the game's magic system to
 * control information visibility and strategic concealment.
 *
 * - THE_VOID: Hides detailed information from enemies (shows '?' instead)
 * - NINJA: Penetrates THE_VOID concealment for espionage
 * - KNOWALL: Provides complete map visibility regardless of normal limits
 *
 * 7. PERFORMANCE OPTIMIZATION
 * ---------------------------
 * The display system includes several performance optimizations for smooth
 * gameplay on period hardware:
 *
 * - Selective rendering: Only updates visible/changed sectors
 * - Efficient coordinate translation: Minimizes calculation overhead
 * - Cached visibility: Hasseen array prevents redundant calculations
 * - Partial vs full redraws: Different clearing strategies based on changes
 *
 * FUNCTION ORGANIZATION:
 *
 * Memory Management:
 * - init_hasseen(): Dynamic memory allocation for visibility tracking
 *
 * Core Rendering:
 * - makemap(): Complete map rendering coordinator
 * - see(): Single sector rendering with dual display support
 * - get_display_for(): Character determination for display modes
 *
 * User Interface:
 * - get_display_mode(): Interactive mode selection interface
 * - newdisplay(): Display change coordination
 *
 * Visual Effects:
 * - highlight(): Highlighting system implementation
 *
 * Screen Management:
 * - coffmap(): Screen refresh and cursor boundary management
 *
 * Visibility System:
 * - whatcansee(): Complete visibility calculation engine
 * - canbeseen(): Simple visibility query interface
 *
 * INTEGRATION WITH OTHER SYSTEMS:
 *
 * - Combat System: Defensive bonus calculations, army visibility
 * - Magic System: Visibility rules, information concealment
 * - Movement System: Cursor tracking, boundary management
 * - Command System: Display mode changes, user interface
 * - Map System: Coordinate translation, terrain data access
 * - Nation System: Ownership display, racial information
 * - Trade System: Resource display, trade goods highlighting
 *
 * TECHNICAL DEPENDENCIES:
 *
 * - curses: Complete screen management and terminal interface
 * - data.h: Game data structures and constants
 * - header.h: System configuration and compatibility
 *
 * MODERNIZATION NOTES:
 *
 * - Platform compatibility: BSD vs non-BSD memory clearing
 * - Legacy K&R function definitions ready for modernization
 * - Global variable dependencies suitable for refactoring
 * - Performance optimizations appropriate for modern systems
 * - Error handling suitable for enhancement
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curses.h>
#include "header.h"
#include "data.h"
#include "safe_convert.h"

/*redraw map in this turn if redraw is a 1*/
extern short redraw;
/*display state SEE data.h FOR CURRENT VALUES OF THESE */

static char *hasseen;

/*
 * init_hasseen - Initialize dynamic memory for screen visibility tracking
 *
 * Allocates and initializes the hasseen array which tracks which screen
 * positions have been rendered during the current display cycle. The array
 * size is calculated based on the current terminal dimensions to optimize
 * memory usage for different screen sizes.
 *
 * The allocation formula ((COLS-10)/2) * (LINES-5) accounts for:
 * - COLS-10: Reserve space for side panel and borders
 * - /2: Two-character wide map display (each coordinate uses 2 screen chars)
 * - LINES-5: Reserve space for command lines and status displays
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   None (void function)
 *
 * Side Effects:
 *   - Allocates dynamic memory for global hasseen array
 *   - Initializes all array elements to zero (unseen state) using memset()
 *   - Calls bye(FALSE) and exits program on allocation failure
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires terminal environment and global state
 *   Approach: Integration testing with various terminal sizes
 *   Key Tests: Memory allocation success/failure, proper array initialization
 *   Dependencies: COLS, LINES terminal variables, global hasseen pointer
 *   Mock Requirements: Terminal dimensions, memory allocation testing
 *   Complexity: Simple - Basic memory management with platform compatibility
 *
 * Notes:
 *   - Must be called before any screen rendering functions
 *   - Memory is not explicitly freed (program termination cleanup)
 *   - Critical function - program exits on allocation failure
 */
void init_hasseen(void) {
	hasseen = (char *)malloc(safe_int_to_size(((COLS-10)/2) * (LINES-5)));
	memset(hasseen, 0, safe_int_to_size(((COLS-10)/2) * (LINES-5)));
	if (hasseen == (char *)NULL) {
		errormsg("Cannot allocate memory.");
		bye(FALSE);
	}
}

/*
 * makemap - Render complete map display with highlighting and positioning
 *
 * Iterates through all screen coordinates and renders visible sectors using
 * the current display mode and highlighting settings. This is the primary
 * map rendering function that coordinates between the visibility system
 * (hasseen tracking), sector rendering (see function), and highlighting
 * system to produce the complete map display.
 *
 * The function uses a double loop through SCREEN_X_SIZE and SCREEN_Y_SIZE
 * to process each potential screen position, but only renders sectors that
 * have been marked as seen in the hasseen visibility tracking array.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   None (void function)
 *
 * Side Effects:
 *   - Updates curses screen display for all visible map sectors
 *   - Applies highlighting based on current hilmode setting
 *   - Positions cursor at current xcurs, ycurs coordinates (with 2x scaling)
 *   - Modifies screen state through see() and highlight() function calls
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires curses, global state, and display systems
 *   Approach: Integration testing with various map states and visibility settings
 *   Key Tests: Complete map rendering, visibility filtering, cursor positioning
 *   Dependencies: Global variables (hilmode), hasseen array, see() and highlight() functions
 *   Mock Requirements: Curses environment, map data, visibility state
 *   Complexity: Moderate - Coordinates multiple display subsystems
 *
 * Notes:
 *   - Core rendering function called whenever map needs complete redraw
 *   - Performance depends on number of visible sectors and complexity of see()
 *   - Cursor positioning uses 2x horizontal scaling for dual-character display
 *   - Must be called after whatcansee() to ensure proper visibility state
 */
void makemap (void) {
	register int x,y;

	for(x=0;x<SCREEN_X_SIZE;x++) for(y=0;y<SCREEN_Y_SIZE;y++)
	if( HAS_SEEN(x,y) ) {
		highlight(x,y,hilmode);
		see(x,y);
	}
	move(ycurs,2*xcurs);
}


/*
 * get_display_mode - Interactive display and highlight mode selection interface
 *
 * Presents a comprehensive user interface for selecting map display modes and
 * highlighting options. This function handles both single and dual display
 * configurations, allowing users to configure primary/secondary display modes
 * and their associated highlighting settings through keyboard interaction.
 *
 * The interface supports 11 display modes (food, vegetation, designation,
 * contour, nation, race, movement cost, defense, people, gold/jewels, metal,
 * items) and 6 highlighting modes (owned land, armies, your armies, armies
 * with moves left, special goods, none).
 *
 * For dual display mode, users can toggle between displays ('/'), configure
 * primary display ('1'), or secondary display ('2'). The function recursively
 * calls itself to handle secondary display configuration.
 *
 * Parameters:
 *   dmode - Pointer to primary display mode variable (modified)
 *   hmode - Pointer to primary highlight mode variable (modified)
 *   odmode - Pointer to secondary display mode variable (NULL for single mode)
 *   ohmode - Pointer to secondary highlight mode variable (modified)
 *
 * Returns:
 *   None (void function)
 *
 * Side Effects:
 *   - Displays multi-line interactive menu on screen (bottom 4 lines)
 *   - Clears menu lines and prompts for user input
 *   - Modifies display and highlight mode variables through pointers
 *   - Sets redraw flag to PART for most changes, DONE for errors
 *   - Calls prep() function for army-related highlight modes
 *   - Produces error beep for invalid selections
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires curses interface and user interaction
 *   Approach: Integration testing with simulated keyboard input
 *   Key Tests: All menu options, dual display toggling, recursive calls, error handling
 *   Dependencies: Curses functions, global redraw variable, prep() function
 *   Mock Requirements: Keyboard input simulation, screen state verification
 *   Complexity: Complex - Multi-mode interface with recursive dual display handling
 *
 * Notes:
 *   - Central UI function for display configuration - high user interaction
 *   - Recursive design for dual display configuration can be confusing
 *   - Uses goto statements for code organization (changed_display, changed_highlight)
 *   - Army preparation required for army-based highlighting modes
 *   - Dual display mode signs control which display is active
 */
void get_display_mode (short *dmode, short *hmode, short *odmode, short *ohmode) {
	short temp;

	mvaddstr(LINES-4,0,"viewing options:  (d)esignation, (r)ace, (M)ove cost, (p)eople, (D)efense");
	clrtoeol();
	mvaddstr(LINES-3,0,"   (f)ood, (c)ontour, (v)egetation, (m)etal, (n)ation mark, (j)ewels, (i)tems");
	clrtoeol();
	mvaddstr(LINES-2,0,"highlight option: (o)wned, (a)rmy, (y)our Army, move (l)eft, (s)pecial,(x)=none");
	clrtoeol();
	if ( odmode !=NULL) {
		mvaddstr(LINES-1,0,"Toggle second display: '/'; set display: '1' or '2'");
	} else {
		mvprintw(LINES-1,0,"     choose selection for the %s display",
			(*ohmode == 1) ? "primary" : "secondary");
	}
	clrtoeol();
	standout();
	mvaddstr(LINES-1,COLS-25,odmode!=NULL?"what display?:":"display?:");
	standend();
	refresh();
	redraw=PART;
	switch(getch()) {
	case '/':
		if (odmode !=NULL)
			*odmode = - *odmode;
		else
			goto error;
		break;
	case '2':
		temp = 2;
		if (odmode !=NULL)
			get_display_mode(odmode,ohmode,NULL,&temp);
		else
			goto error;
		break;
	case '1':
		temp = 1;
		if (odmode !=NULL)
			get_display_mode(dmode,hmode,NULL,&temp);
		else
			goto error;
		break;
	case 'f':
		*dmode=DI_FOOD;
		goto changed_display;
	case 'v':	/* vegetation map*/
		*dmode=DI_VEGE;
		goto changed_display;
	case 'd':	/* designations map*/
		*dmode=DI_DESI;
		goto changed_display;
	case 'c':	/* contour map of world */
		*dmode=DI_CONT;
		goto changed_display;
	case 'n':	/* nations map*/
		*dmode=DI_NATI;
		goto changed_display;
	case 'r':	/* race map*/
		*dmode=DI_RACE;
		goto changed_display;
	case 'M': /* move cost map */
		*dmode=DI_MOVE;
		goto changed_display;
	case 'D':
		*dmode=DI_DEFE;
		goto changed_display;
	case 'p':
		*dmode=DI_PEOP;
		goto changed_display;
	case 'j':
		*dmode=DI_GOLD;
		goto changed_display;
	case 'm':
		*dmode=DI_METAL;
		goto changed_display;
	case 'i':
		*dmode=DI_ITEMS;
	changed_display:
		if ( odmode !=NULL && *odmode > 0) /* just 'd' not 'd1 or 'd2' */
			*odmode= - *odmode;
		break;
	case 'a':	/* armies hilighted map*/
		prep(country,FALSE);
		*hmode=HI_ARMY;
		goto changed_highlight;
	case 'o':	/* owners hilighted map*/
		*hmode=HI_OWN;
		goto changed_highlight;
	case 's':	/* hilight tradegoods */
		*hmode=HI_GOOD;
		goto changed_highlight;
	case 'x':	/*no highlighting*/
		*hmode=HI_NONE;
		goto changed_highlight;
	case 'y':	/* your armies hilighted map*/
		prep(country,FALSE);
		*hmode=HI_YARM;
		goto changed_highlight;
	case 'l':	/* your armies with moves left hilighted map*/
		prep(country,FALSE);
		*hmode=HI_MOVE;
	changed_highlight:
		if ( odmode !=NULL ) /* just 'd' not 'd1 or 'd2' */
			*ohmode= *hmode;
		break;
	default:
	error:
		beep();
		redraw=DONE;
	}
}

/*
 * newdisplay - Handle display mode changes and screen updates
 *
 * Coordinates the display mode selection process and subsequent screen
 * updates when the user requests a display mode change. This function
 * serves as a high-level interface that combines the interactive mode
 * selection (get_display_mode) with the necessary screen refreshing
 * (makebottom) to complete the display change operation.
 *
 * The function operates on the global display mode variables (dismode,
 * hilmode, otherdismode, otherhilmode) and updates them based on user
 * selection through the interactive interface.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   None (void function)
 *
 * Side Effects:
 *   - Modifies global display mode variables through get_display_mode()
 *   - Updates bottom screen area through makebottom() call
 *   - May trigger screen redraw operations through get_display_mode()
 *   - User interaction occurs through get_display_mode() interface
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires display system and user interface
 *   Approach: Integration testing with various display mode changes
 *   Key Tests: Mode changes, screen updates, global variable modifications
 *   Dependencies: Global display variables, get_display_mode(), makebottom()
 *   Mock Requirements: User interface simulation, screen state verification
 *   Complexity: Simple - High-level coordinator function
 *
 * Notes:
 *   - High-level wrapper function for display mode management
 *   - Typically called in response to user display change commands
 *   - Delegates actual mode selection to get_display_mode()
 *   - Ensures screen consistency through makebottom() call
 *   - Part of the user command processing workflow
 */
void newdisplay (void) {
	get_display_mode(&dismode,&hilmode,&otherdismode,&otherhilmode);
	makebottom();
}

/*
 * get_display_for - Determine display character for specific map coordinate and mode
 *
 * Calculates the appropriate display character for a given map coordinate based
 * on the specified display mode and current game state. This function implements
 * the core display logic that converts game data (terrain, resources, ownership,
 * armies, etc.) into visual characters for map rendering.
 *
 * The function supports 11 different display modes, each emphasizing different
 * aspects of the game world: food production, vegetation, designations, contour
 * (altitude), nation ownership, race, movement costs, defensive bonuses, population,
 * precious resources (gold/jewels), metal resources, and trade goods.
 *
 * Special handling includes THE_VOID magic power which hides detailed information
 * from enemy players (showing '?' instead of actual values) and NINJA magic
 * which can penetrate this concealment. Water sectors and unproductive land
 * receive special character representations.
 *
 * Parameters:
 *   x - Screen X coordinate for display calculation
 *   y - Screen Y coordinate for display calculation
 *   dmode - Display mode constant (DI_FOOD, DI_VEGE, DI_DESI, etc.)
 *
 * Returns:
 *   char - Single character representing the map content for the specified mode
 *          Returns '?' for information hidden by THE_VOID magic
 *          Returns numeric characters (0-9), '+' for large values, or terrain symbols
 *
 * Side Effects:
 *   - Accesses global map data arrays (sct, movecost, ntn)
 *   - Calls helper functions (tofood, tg_ok, magic, fort_val)
 *   - Uses global coordinate offsets (xoffset, yoffset)
 *   - Reads global country variable for ownership/magic checks
 *
 * Testing Notes:
 *   Category: A (Unit) - Testable with mock data and coordinate inputs
 *   Approach: Unit testing with comprehensive mode and data combinations
 *   Key Tests: All display modes, magic concealment, boundary conditions, special cases
 *   Dependencies: Map data structures, helper functions, global coordinate system
 *   Mock Requirements: Map data, magic system, coordinate offsets, game state
 *   Complexity: Complex - Large switch statement with intricate display logic per mode
 *
 * Notes:
 *   - Core display logic - determines what players see on the map
 *   - Performance critical - called for every visible map position
 *   - Complex magic system interaction affects information visibility
 *   - Each display mode has unique calculation and representation logic
 *   - Defense mode performs complex terrain bonus calculations
 *   - Resource modes check accessibility and availability
 */
char get_display_for(int x,int y,short dmode)
{
	int armbonus;

	char ch= '{';
	if((magic(sct[x+xoffset][y+yoffset].owner,THE_VOID)==TRUE)
	   &&((dmode==DI_DEFE)||(dmode==DI_GOLD)||(dmode==DI_METAL)
		 ||(dmode==DI_PEOP)||(dmode==DI_FOOD)||(dmode==DI_ITEMS))
	   &&(country!=sct[x+xoffset][y+yoffset].owner)
	   &&(magic(country,NINJA)!=TRUE)
	   &&(country!=0)) {
		ch='?';
	} else {
		switch(dmode){
		case DI_FOOD:	/*food */
			if(tofood( &sct[x+xoffset][y+yoffset],country)==0)
				ch=sct[x+xoffset][y+yoffset].vegetation;
			else if (tofood( &sct[x+xoffset][y+yoffset],country)<10)
				ch=safe_int_to_char(tofood( &sct[x+xoffset][y+yoffset],country)+'0');
			else ch='+';
			break;
		case DI_VEGE: /*vegetation*/
			ch=sct[x+xoffset][y+yoffset].vegetation;
			break;
		case DI_DESI: /*designation*/
			if(sct[x+xoffset][y+yoffset].owner==0){
				if(tofood( &sct[x+xoffset][y+yoffset],country)!=0)
					ch=sct[x+xoffset][y+yoffset].altitude;
				else ch=sct[x+xoffset][y+yoffset].vegetation;
			}
			else if((country==0)
				   ||(sct[x+xoffset][y+yoffset].owner==country))
				ch=sct[x+xoffset][y+yoffset].designation;
			else ch=ntn[sct[x+xoffset][y+yoffset].owner].mark;
			break;
		case DI_CONT: /*contour*/
			ch=sct[x+xoffset][y+yoffset].altitude;
			break;
		case DI_NATI: /*ownership*/
			if(sct[x+xoffset][y+yoffset].owner==0)
				ch=sct[x+xoffset][y+yoffset].altitude;
			else ch=ntn[sct[x+xoffset][y+yoffset].owner].mark;
			break;
		case DI_RACE: /*race*/
			if(sct[x+xoffset][y+yoffset].owner==0)
				ch=sct[x+xoffset][y+yoffset].altitude;
			else ch=ntn[sct[x+xoffset][y+yoffset].owner].race;
			break;
		case DI_MOVE:	/*movement cost map*/
			if(movecost[x+xoffset][y+yoffset]>=0) {
				if(movecost[x+xoffset][y+yoffset]>=10)
					ch='+';
				else ch=safe_int_to_char(movecost[x+xoffset][y+yoffset]+'0');
			} else if(sct[x+xoffset][y+yoffset].altitude==WATER)
				ch=WATER;
			else
				ch='X';
			break;
		case DI_DEFE:	 /*Defence*/
			if (sct[x+xoffset][y+yoffset].altitude==WATER)
				ch=WATER;
			else if(movecost[x+xoffset][y+yoffset]<0)
				ch='X';
			else {
				/*Racial combat bonus due to terrain (the faster you move the better)*/
				armbonus=0;
				armbonus+=5*(9-movecost[x+xoffset][y+yoffset]);

				if(sct[x+xoffset][y+yoffset].altitude==MOUNTAIN)
					armbonus+=40;
				else if(sct[x+xoffset][y+yoffset].altitude==HILL)
					armbonus+=20;

				if(sct[x+xoffset][y+yoffset].vegetation==JUNGLE)
					armbonus+=30;
				else if(sct[x+xoffset][y+yoffset].vegetation==FOREST)
					armbonus+=20;
				else if(sct[x+xoffset][y+yoffset].vegetation==WOOD)
					armbonus+=10;

				armbonus+=fort_val(&sct[x+xoffset][y+yoffset]);

				if(armbonus<200) ch=safe_int_to_char(armbonus/20+'0');
				else ch='+';
			}
			break;
		case DI_PEOP:	 /*People*/
			if (sct[x+xoffset][y+yoffset].altitude==WATER)
				ch=WATER;
			else if (sct[x+xoffset][y+yoffset].people>=9950)
				ch='X';
			else if (sct[x+xoffset][y+yoffset].people>=4950)
				ch='V';
			else if (sct[x+xoffset][y+yoffset].people>=950)
				ch='I';
			else
				ch=safe_long_to_char((50+sct[x+xoffset][y+yoffset].people)/100+'0');
			break;
		case DI_GOLD:	/*Gold*/
			if (sct[x+xoffset][y+yoffset].altitude==WATER)
				ch=WATER;
			else if(tofood( &sct[x+xoffset][y+yoffset],country)==0)
				ch='X';
			else if (tg_ok(country,&sct[x+xoffset][y+yoffset])){
				if (sct[x+xoffset][y+yoffset].jewels>=10)
					ch='+';
				else
					ch=sct[x+xoffset][y+yoffset].jewels+'0';
			} else ch='0';
			break;
		case DI_METAL:	 /*Metal*/
			if (sct[x+xoffset][y+yoffset].altitude==WATER)
				ch=WATER;
			else if(tofood( &sct[x+xoffset][y+yoffset],country)==0)
				ch='X';
			else if (tg_ok(country,&sct[x+xoffset][y+yoffset])){
				if (sct[x+xoffset][y+yoffset].metal>=10)
					ch='+';
				else
					ch=sct[x+xoffset][y+yoffset].metal+'0';
			} else ch='0';
			break;
		case DI_ITEMS:	/* designations needed for tradegoods */
			if (sct[x+xoffset][y+yoffset].altitude==WATER)
				ch=WATER;
			else if(tofood( &sct[x+xoffset][y+yoffset],country)==0)
				ch='X';
			else if (sct[x+xoffset][y+yoffset].tradegood!=TG_none
			&& (*(tg_stype+sct[x+xoffset][y+yoffset].tradegood)!='x')
				&& tg_ok(country,&sct[x+xoffset][y+yoffset]))
				ch= *(tg_stype+sct[x+xoffset][y+yoffset].tradegood);
			else ch='-';
			break;
		default:
			break;
		}
	}
	return ch;
}

/*
 * see - Render single map sector with dual display mode support
 *
 * Renders a single map coordinate to the screen using the current display
 * modes and highlighting settings. This function handles both primary and
 * secondary display modes when dual display is enabled, placing characters
 * at screen positions that account for the two-character wide display format.
 *
 * The function performs comprehensive bounds checking for both screen
 * coordinates and map coordinates, ensuring safe rendering even at map
 * edges. When coordinates are out of bounds, it renders blank spaces to
 * clear the screen position.
 *
 * Parameters:
 *   x - Screen X coordinate (0 to SCREEN_X_SIZE-1)
 *   y - Screen Y coordinate (0 to SCREEN_Y_SIZE-1)
 *
 * Returns:
 *   None (void function)
 *
 * Side Effects:
 *   - Updates specific screen positions at (y, 2*x) and potentially (y, 2*x+1)
 *   - Calls highlight() to apply highlighting effects before rendering
 *   - Uses get_display_for() to determine appropriate display characters
 *   - Renders blank spaces for out-of-bounds coordinates
 *
 * Testing Notes:
 *   Category: A (Unit) - Can be tested with mock coordinates and display modes
 *   Approach: Unit testing with various coordinate combinations and display settings
 *   Key Tests: Bounds checking, dual display rendering, character positioning
 *   Dependencies: Global variables (dismode, otherdismode, otherhilmode), curses functions
 *   Mock Requirements: Display modes, coordinate systems, get_display_for() and highlight()
 *   Complexity: Moderate - Handles multiple display modes and coordinate transformations
 *
 * Notes:
 *   - Core rendering primitive called by makemap() for each visible sector
 *   - Screen positioning uses 2x horizontal scaling (2*x, 2*x+1) for dual characters
 *   - Dual display mode renders second character when otherdismode > 0
 *   - Bounds checking prevents crashes when rendering near map boundaries
 *   - Performance critical - called frequently during map updates
 */
void see(int x,int y) {
	char ch;
	if((x<0)||(y<0)||(x>=SCREEN_X_SIZE)||(y>=SCREEN_Y_SIZE)
	||((y+yoffset)>=MAPY)||((x+xoffset)>=MAPX)) return;
	if(((y+yoffset)<MAPY)&&((x+xoffset)<MAPX)) {
		ch=get_display_for(x,y,dismode);
		mvaddch(y,2*x,ch);
		if ( otherdismode > 0 ) {
			highlight(x,y,otherhilmode);
			ch=get_display_for(x,y,otherdismode);
			mvaddch(y,2*x+1,ch);
		}
	}
	else {
		mvaddch(y,2*x,' ');
		mvaddch(y,2*x+1,' ');
	}
}

/*
 * highlight - Apply highlighting effects based on highlight mode and map conditions
 *
 * Determines whether a specific map coordinate should be highlighted based on
 * the current highlight mode and game state, then applies the appropriate
 * curses highlighting (standout mode) if conditions are met. This function
 * implements the visual emphasis system that helps players identify specific
 * map features like armies, ownership, movement capabilities, and resources.
 *
 * The function supports 6 highlighting modes: armies with movement remaining,
 * player's armies, trade goods, land ownership, any armies, and no highlighting.
 * Each mode has specific criteria for determining when highlighting should be
 * applied, often involving ownership checks, army presence, magic powers, and
 * resource availability.
 *
 * Parameters:
 *   x - Screen X coordinate for highlight evaluation
 *   y - Screen Y coordinate for highlight evaluation
 *   hmode - Highlight mode constant (HI_MOVE, HI_YARM, HI_GOOD, HI_OWN, HI_ARMY, HI_NONE)
 *
 * Returns:
 *   None (void function)
 *
 * Side Effects:
 *   - Calls standend() to reset highlighting state before evaluation
 *   - Calls standout() to enable highlighting when conditions are met
 *   - Accesses global arrays (armies, sectors, occupation) for state checking
 *   - Uses global variables (country, xoffset, yoffset) for coordinate translation
 *   - Interacts with magic system for visibility and concealment checks
 *
 * Testing Notes:
 *   Category: A (Unit) - Testable with mock game state and coordinate inputs
 *   Approach: Unit testing with various highlight modes and game conditions
 *   Key Tests: All highlight modes, army conditions, ownership scenarios, magic interactions
 *   Dependencies: Army data, map data, magic system, curses highlighting functions
 *   Mock Requirements: Game state simulation, army positions, ownership data, magic powers
 *   Complexity: Moderate - Multiple modes with distinct logic and magic system integration
 *
 * Notes:
 *   - Visual feedback system - critical for gameplay usability
 *   - Always calls standend() first to ensure clean highlighting state
 *   - Army-based highlighting requires iterating through army arrays
 *   - Magic system (THE_VOID, NINJA) affects trade goods highlighting
 *   - Bounds checking prevents crashes at map edges
 *   - Performance impact when highlighting large areas with many armies
 */
void highlight(int x,int y,short hmode) {
	int	armynum;
	if((x<0)||(y<0)||(x>=SCREEN_X_SIZE)||(y>=SCREEN_Y_SIZE)
	||((y+yoffset)>=MAPY)||((x+xoffset)>=MAPX)) return;
	standend();
	switch(hmode){
	case HI_MOVE:	/* your armies w/ move left */
		for(armynum=0;armynum<MAXARM;armynum++)
			if(( P_ASOLD != 0 )
			&&( P_AMOVE != 0 )
			&&( P_AXLOC==(x+xoffset ))
			&&( P_AYLOC==(y+yoffset ))) break;
		if(armynum<MAXARM) standout();
		break;
	case HI_YARM:	/* your armies */
		for(armynum=0;armynum<MAXARM;armynum++)
			if(( P_ASOLD != 0)
			&&( P_AXLOC==x+xoffset)
			&&( P_AYLOC==y+yoffset)) break;
		if(armynum<MAXARM) standout();
		break;
	case HI_GOOD:	/* trade goods */
		if(tg_ok( country, &sct[x+xoffset][y+yoffset])
		   &&(sct[x+xoffset][y+yoffset].tradegood != TG_none)
		   &&(magic(sct[x+xoffset][y+yoffset].owner,THE_VOID)!=TRUE
		    || country==0
		    || country==sct[x+xoffset][y+yoffset].owner
		    || magic(country,NINJA)==TRUE)
		   &&(sct[x+xoffset][y+yoffset].altitude!=WATER)) standout();
		break;
	case HI_OWN: /* ownership */
		if(country==0) {
			if(sct[x+xoffset][y+yoffset].owner!=0)
				standout();
		} else if(sct[x+xoffset][y+yoffset].owner==country)
			standout();
		break;
	case HI_ARMY: /* any armies */
		if(occ[x+xoffset][y+yoffset]!=0) standout();
		break;
	default:
		break;
	}
}

/*
 * coffmap - Screen redraw coordination and cursor boundary management
 *
 * Performs comprehensive screen management by checking cursor boundaries,
 * triggering map recentering when necessary, and coordinating full screen
 * updates when the redraw flag indicates changes are needed. This function
 * serves as the central screen refresh coordinator that ensures the display
 * remains consistent and properly positioned.
 *
 * The function checks if the cursor has moved outside valid screen boundaries
 * or beyond map edges, automatically recentering the map view when needed.
 * It then handles different types of screen redraws (FULL vs PART) and
 * coordinates the complete screen update process including map, bottom area,
 * and side panel refreshing.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   None (void function)
 *
 * Side Effects:
 *   - Calls centermap() and sets redraw=PART when cursor is out of bounds
 *   - Clears screen (clear() for FULL, clrtobot() for PART redraw)
 *   - Calls makemap() to update the map display
 *   - Calls makebottom() to update bottom screen area
 *   - Calls makeside(FALSE) to update side panel
 *   - Positions cursor at current xcurs, ycurs coordinates (with 2x scaling)
 *   - Calls refresh() to update physical screen
 *   - Sets redraw=DONE after completing update cycle
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires curses environment and screen state
 *   Approach: Integration testing with various cursor positions and redraw states
 *   Key Tests: Boundary checking, recentering logic, redraw coordination, screen updates
 *   Dependencies: Global variables (xcurs, ycurs, redraw), screen functions, curses
 *   Mock Requirements: Cursor state, screen dimensions, redraw flags, coordinate systems
 *   Complexity: Moderate - Coordinates multiple screen subsystems and state management
 *
 * Notes:
 *   - Central screen management function - called frequently during gameplay
 *   - Boundary checking uses complex conditions involving map and screen edges
 *   - Different clearing strategies for FULL vs PART redraws optimize performance
 *   - Cursor positioning uses 2x horizontal scaling for dual-character display
 *   - Complete screen update cycle ensures all display elements stay synchronized
 */
void coffmap (void) {
	if( xcurs<0 || (XREAL!=0 && xcurs==0) || (ycurs<0) ||
	   (YREAL!=0 && ycurs==0) || xcurs>=SCREEN_X_SIZE-1 ||
	   ycurs>=SCREEN_Y_SIZE-1 || XREAL>=MAPX || YREAL>=MAPY) {
		centermap();
		redraw=PART;
	}

	if(redraw!=DONE) {
		if (redraw==FULL) {
			clear();	/* clear real screen */
		} else {
			move(0,0);	/* clear curses screen but not real screen */
			clrtobot();
		}
		makemap();	/* update map */
		makebottom();
		redraw=DONE;
	}
	move(ycurs,2*xcurs);
	makeside(FALSE);  /*update side*/
	move(ycurs,2*xcurs);
	refresh();
}

/*
 * canbeseen - Check if specific map coordinates are visible to current player
 *
 * Determines whether a given map coordinate is within the current player's
 * line of sight and has been marked as visible in the current display cycle.
 * This function serves as a simple interface to the visibility system,
 * combining map boundary checking with the hasseen visibility tracking array.
 *
 * The function first verifies that the coordinates are within valid map
 * boundaries using the ONMAP macro, then checks the corresponding position
 * in the hasseen array (adjusted for current screen offset) to determine
 * if the location has been marked as visible by the whatcansee() function.
 *
 * Parameters:
 *   x - Map X coordinate to check for visibility
 *   y - Map Y coordinate to check for visibility
 *
 * Returns:
 *   int - Boolean value (TRUE/FALSE)
 *         FALSE if coordinates are outside map boundaries
 *         TRUE/FALSE based on hasseen array visibility status
 *
 * Side Effects:
 *   - Accesses global hasseen array for visibility checking
 *   - Uses global offset variables (xoffset, yoffset) for coordinate translation
 *   - No modifications to game state (read-only function)
 *
 * Testing Notes:
 *   Category: A (Unit) - Testable with mock coordinates and visibility state
 *   Approach: Unit testing with various coordinate combinations and visibility states
 *   Key Tests: Boundary conditions, coordinate translation, visibility array access
 *   Dependencies: ONMAP macro, hasseen array, global coordinate offsets
 *   Mock Requirements: Map boundaries, visibility array state, coordinate system
 *   Complexity: Simple - Straightforward boundary checking and array access
 *
 * Notes:
 *   - Simple interface function for visibility system queries
 *   - Used by other functions to check visibility before rendering or processing
 *   - Coordinates are in map space, not screen space
 *   - Depends on proper initialization of hasseen array by whatcansee()
 *   - Performance optimized - minimal overhead for frequent visibility checks
 */
int canbeseen (int x, int y) {
	if(!ONMAP(x,y)) return(FALSE);
	return( (int) HAS_SEEN(x-xoffset,y-yoffset) );
}

/*
 * whatcansee - Calculate and populate complete visibility map for current player
 *
 * Performs comprehensive visibility calculations to determine which map sectors
 * are visible to the current player, populating the hasseen array with the
 * results. This function implements the core line-of-sight system that combines
 * land ownership, army positions, navy positions, and magic powers to create
 * the complete visibility picture for map rendering.
 *
 * The visibility calculation uses three main sources: owned land provides
 * visibility in a radius (LANDSEE), armies provide visibility in their
 * immediate area (ARMYSEE), and navies provide coastal/sea visibility
 * (NAVYSEE). Special magic powers can override these rules: KNOWALL provides
 * complete map visibility, while god mode (country==0) also sees everything.
 *
 * The function operates in screen-relative coordinates, calculating visibility
 * for the current screen view and storing results in the hasseen array for
 * subsequent rendering operations.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   None (void function)
 *
 * Side Effects:
 *   - Completely repopulates the hasseen visibility array
 *   - Initially sets all positions to FALSE (not visible)
 *   - Sets all positions to TRUE for KNOWALL magic or god mode
 *   - Iterates through owned land to calculate land-based visibility
 *   - Iterates through all navies to calculate naval visibility
 *   - Iterates through all armies to calculate army-based visibility
 *   - Uses global variables (country, xoffset, yoffset) for calculations
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires complete game state and magic system
 *   Approach: Integration testing with various game states and unit configurations
 *   Key Tests: Magic powers, land ownership, army/navy positions, visibility radii
 *   Dependencies: Complete game state, army/navy data, magic system, coordinate system
 *   Mock Requirements: Map data, unit positions, ownership, magic powers, coordinate offsets
 *   Complexity: Complex - Multi-source visibility with complex radius calculations
 *
 * Notes:
 *   - Core visibility engine - determines what players can see and interact with
 *   - Performance critical - called whenever visibility needs recalculation
 *   - Complex nested loops for radius calculations around each visibility source
 *   - Visibility constants (LANDSEE, ARMYSEE, NAVYSEE) control visibility ranges
 *   - Screen-relative calculations require careful coordinate translation
 *   - Must be called before map rendering to ensure accurate visibility state
 */
void whatcansee (void) {
	register int x,y;
	int	i,j;
	short	armynum,nvynum;

	if((magic(country,KNOWALL)==TRUE)||(country==0)) {
		for(x=0;x<SCREEN_X_SIZE;x++) for(y=0;y<SCREEN_Y_SIZE;y++)
			HAS_SEEN(x,y)=TRUE;
		return;
	}
	for(x=0;x<SCREEN_X_SIZE;x++) for(y=0;y<SCREEN_Y_SIZE;y++)
		HAS_SEEN(x,y)=FALSE;

	for(x=(-LANDSEE);(x<SCREEN_X_SIZE+LANDSEE);x++)
	for(y=(-LANDSEE);(y<SCREEN_Y_SIZE+LANDSEE);y++)
	if((ONMAP(x+xoffset,y+yoffset))
	&&(sct[x+xoffset][y+yoffset].owner==country)){
		for(i=x-LANDSEE;i<=x+LANDSEE;i++)
		for(j=y-LANDSEE;j<=y+LANDSEE;j++)
		if(i>=0 && j>=0 && i<SCREEN_X_SIZE && j<SCREEN_Y_SIZE)
			if (ONMAP(i+xoffset,j+yoffset))
				HAS_SEEN(i,j)=TRUE;
	}

	for(nvynum=0;nvynum<MAXNAVY;nvynum++)
	if((P_NMSHP!=0)||(P_NWSHP!=0)||(P_NGSHP!=0))
		for(i=(int)P_NXLOC-xoffset-NAVYSEE;i!=1+(int)P_NXLOC-xoffset+NAVYSEE;i++)
		for(j=(int)P_NYLOC-yoffset-NAVYSEE;j!=1+(int)P_NYLOC-yoffset+NAVYSEE;j++)
			if(ONMAP(i+xoffset,j+yoffset) && i>=0 && j>=0
			&& i<SCREEN_X_SIZE && j<SCREEN_Y_SIZE)
				HAS_SEEN(i,j)=TRUE;

	for(armynum=0;armynum<MAXARM;armynum++)
		if(P_ASOLD>0)
		for(i=(int)P_AXLOC-xoffset-ARMYSEE;i!=1+(int)P_AXLOC-xoffset+ARMYSEE;i++)
		for(j=(int)P_AYLOC-yoffset-ARMYSEE;j!=1+(int)P_AYLOC-yoffset+ARMYSEE;j++)
		if(ONMAP(i+xoffset,j+yoffset) && i>=0 && j>=0
		&& i<SCREEN_X_SIZE && j<SCREEN_Y_SIZE)
			HAS_SEEN(i,j)=TRUE;

	return;
}
