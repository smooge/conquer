/*
 * newlogin.c - New player registration and login system
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
 * =============================================================================
 * NEW PLAYER REGISTRATION AND LOGIN SYSTEM
 * =============================================================================
 *
 * This module implements the complete new player registration system for
 * Conquer, providing interactive nation creation, placement, and initialization.
 * It handles the entire workflow from initial nation setup through world
 * placement and army allocation.
 *
 * SYSTEM ARCHITECTURE:
 *
 * 1. INTERACTIVE NATION BUILDER
 *    - Curses-based user interface for nation creation
 *    - Point-based resource allocation system (MAXPTS total points)
 *    - Comprehensive input validation and error handling
 *    - Support for multiple races (Human, Elf, Dwarf, Orc) with unique abilities
 *    - Class selection system with race-specific restrictions
 *    - Password encryption and validation
 *
 * 2. WORLD PLACEMENT SYSTEM
 *    - Intelligent nation placement with multiple location quality levels
 *    - Terrain modification around capitals based on racial preferences
 *    - Collision avoidance with existing nations
 *    - Automatic fallback placement algorithms
 *
 * 3. MILITARY ALLOCATION
 *    - Automatic army structure creation and leader assignment
 *    - Capital fortification and sector ownership assignment
 *    - Resource distribution across starting territories
 *
 * CORE COMPONENTS:
 *
 * Nation Creation Flow:
 * newlogin() -> Nation Builder Interface -> convert() -> place() -> Army Setup
 *
 * Placement System:
 * place() -> Location Selection -> teraform() -> Territory Assignment
 *
 * UI Management:
 * newinit() -> Interactive Menus -> newreset() / newbye()
 *
 * DATA STRUCTURES:
 *
 * - Nation configuration arrays (Mlabels[], Mitems[], Mhelp[])
 * - Race/class power mappings (Classpow[], Classcost[], CPowlist[])
 * - Resource allocation tracking (spent[] array)
 * - Location quality types (LType[]: Random, Fair, Great)
 *
 * POINT ALLOCATION SYSTEM:
 *
 * Players start with MAXPTS points to distribute across:
 * - CH_PEOPLE: Population size
 * - CH_TREASURY: Starting gold
 * - CH_LOCATE: Location quality (Random/Fair/Great)
 * - CH_SOLDIERS: Military strength
 * - CH_ATTACK/CH_DEFEND: Combat bonuses
 * - CH_REPRO: Population growth rate
 * - CH_MOVEMENT: Army mobility
 * - CH_MAGIC: Magical power acquisition
 * - CH_LEADERS: Officer count
 * - CH_RAWGOODS: Food, jewels, and metal resources
 *
 * RACE-SPECIFIC FEATURES:
 *
 * Each race has unique starting bonuses and terrain preferences:
 * - DWARF: Mining abilities, mountain/hill terrain, metal resources
 * - ELF: Void cloaking magic, forest terrain, jewel resources
 * - ORC: Monster leadership, mountain terrain, mixed resources, always evil
 * - HUMAN: Warrior skills, clear/farmland terrain, balanced resources
 *
 * LOCATION QUALITY SYSTEM:
 *
 * - RANDOM: Basic placement with minimal collision avoidance
 * - FAIR: Enhanced placement with better food production requirements
 * - GREAT: Premium placement with maximum spacing and resource quality
 * Each level provides increasing terrain modification chances and territory bonuses.
 *
 * ERROR HANDLING AND VALIDATION:
 *
 * - Comprehensive input validation for names, passwords, choices
 * - Automatic fallback placement if preferred location fails
 * - Resource constraint enforcement during point allocation
 * - Terminal size validation and compatibility checking
 *
 * INTEGRATION POINTS:
 *
 * - World data structures (sct[][], ntn[])
 * - Mail system integration (mailtopc() for notifications)
 * - Army and leader management systems
 * - Magic power assignment and class abilities
 * - Terrain generation and resource allocation
 *
 * SECURITY FEATURES:
 *
 * - Password encryption using crypt() with salt
 * - User ID validation (CHECKUSER compilation flag)
 * - Nation limit enforcement per user
 * - Input sanitization and bounds checking
 *
 * This system provides a complete player onboarding experience, from initial
 * registration through world integration, ensuring balanced gameplay and
 * proper resource distribution while maintaining security and data integrity.
 */

#define _GNU_SOURCE  /* For crypt() function */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <ctype.h>
#include <pwd.h>
#include <unistd.h>
#include <crypt.h>
#include "patchlevel.h"
#include "header.h"
#include "data.h"
#include "newlogin.h"
#include "safe_convert.h"

/* information about national classes */
static char *Classwho[]= { "DEHO", "DEH", "DEH", "EH", "H", "DHO", "HE", "DHO",
	"O", "O", "O" };
static char *CPowlist[]= { "None", "None", "None", "SUMMON", "RELIGION", "SAILOR",
	"URBAN", "WARLORD", "DESTROYER", "MA_MONST", "THE_VOID"};
static int  Classcost[]= { 0, 0, 0, 4, 2, 2, 2, 6, 4, 4, 2 };
static long Classpow[]= { 0x0L, 0x0L, 0x0L, SUMMON, RELIGION, SAILOR,
	URBAN, 0x000000007L, DESTROYER, 0x00000700L, THE_VOID };

static char *Mprompt[]= { "<ADD", "SUB>" };
static char *LType[]={ "Random", "Fair", "Great" };

static char *Mlabels[]= { "Population", "Treasury", "Location",
	"Military", "Attack Bonus", "Defense Bonus", "Reproduction",
	"Movement", "Magic Powers", "Leaders", "Raw Materials" };

static char *Mitems[]= { "people", "gold talons", "location", "soldiers",
	"percent", "percent", "percent", "move points", "powers",
	"nation leaders", "units of food" };

static char *Mhelp[]= { "Population: Amount of citizens in your nation",
	"Treasury: Amount of monetary wealth in your nation",
	"Location: Relative value of nation placement in world",
	"Soldiers: Number of men in the national army, not counting leaders",
	"Attack Bonus: Skill level of your troops when attacking",
	"Defense Bonus: Skill level of your troops when defending",
	"Reproduction: Yearly rate of civilian population increase",
	"Movement: Number of movement points per normal army unit",
	"Magic Powers: Randomly obtain a new magical power",
	"Leaders: Number of leader units, including national ruler",
	"Raw Materials: Starting values for jewels, metal, and food" };

int nstartcst(void);
extern int pwater;		/* percent water in world (0-100) */
int	numleaders;
static int spent[CH_NUMBER];

/*
 * teraform - Modify terrain around a nation's capital based on racial preferences
 *
 * Creates race-appropriate terrain within a specified range of a nation's capital
 * to give new players favorable starting conditions. Each race modifies terrain
 * to match their cultural and strategic preferences, while adding appropriate
 * resource deposits based on probability.
 *
 * Parameters:
 *   x - X coordinate of the nation's capital (center of terraforming)
 *   y - Y coordinate of the nation's capital (center of terraforming)
 *   range - Radius of terrain modification around the capital
 *   chance - Percentage probability (0-100) of resource deposit placement
 *
 * Returns:
 *   None (void function)
 *
 * Side Effects:
 *   - Modifies world terrain (sct[][] altitude and vegetation)
 *   - Adds resource deposits (metal/jewels) based on racial preferences
 *   - Sets capital sector to race-appropriate terrain type
 *   - Uses random number generation for terrain variation and resource placement
 *
 * Racial Terrain Modifications:
 *   DWARF: Creates mountain capitals with surrounding hills/mountains + metal deposits
 *   ELF: Creates forest capitals with surrounding woods/forests + jewel deposits
 *   ORC: Creates mountain capitals with hills/mountains + mixed metal/jewel deposits
 *   HUMAN: Creates clear farmland with woods/good vegetation + balanced resources
 *
 * Algorithm:
 *   1. Set capital sector to race-specific terrain type
 *   2. Iterate through range × range area around capital
 *   3. Skip water sectors and the capital itself
 *   4. Apply race-specific terrain patterns with randomization
 *   5. Add resource deposits based on chance percentage and racial preferences
 *
 * Resource Distribution:
 *   - DWARF: 100% metal deposits (mining specialization)
 *   - ELF: 100% jewel deposits (magical affinity)
 *   - ORC: 50% metal, 50% jewels (aggressive expansion)
 *   - HUMAN: 50% metal, 50% jewels (balanced development)
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires world data structures and race definitions
 *   Approach: Integration testing with mock world state and race configurations
 *   Key Tests: [Terrain modification per race, resource placement probability, range boundaries]
 *   Dependencies: [Global world array sct[][], race constants, resource functions]
 *   Mock Requirements: [World sectors, race configuration, random number generator]
 *   Complexity: Moderate - race-specific logic with probabilistic resource placement
 *
 * Notes:
 *   - Uses global curntn->race to determine modification pattern
 *   - Relies on getmetal() and getjewel() functions for resource placement
 *   - Terrain modification is permanent and affects starting nation advantages
 *   - Critical for game balance as it determines starting resource availability
 */
void
teraform (int x, int y, int range, int chance)
{
	int i,j;
	switch(curntn->race) {
	case DWARF:
		sct[x][y].altitude=MOUNTAIN;
		for(i=x-range;i<=x+range;i++)
		for(j=y-range;j<=y+range;j++)
		if((i!=x)&&(j!=y)&&(ONMAP(i,j))
		&&(sct[i][j].altitude!=WATER)){
			if (rand()%3==0)
				sct[i][j].altitude=MOUNTAIN;
			else	sct[i][j].altitude=HILL;

			if( rand()%100 < chance ) getmetal( &sct[i][j] );
		}
		return;
	case ELF:
		sct[x][y].vegetation = FOREST;
		for(i=x-range;i<=x+range;i++)
		for(j=y-range;j<=y+range;j++)
		if((i!=x)&&(j!=y)&&(ONMAP(i,j))
		&&(sct[i][j].altitude!=WATER)) {
			if (rand()%3==0)
				sct[i][j].vegetation=FOREST;
			else	sct[i][j].vegetation=WOOD;
			if( rand()%100 < chance ) getjewel( &sct[i][j] );
		}
		return;
	case ORC:
		sct[x][y].altitude=MOUNTAIN;
		for(i=x-range;i<=x+range;i++)
		for(j=y-range;j<=y+range;j++)
		if((i!=x)&&(j!=y)&&(ONMAP(i,j))
		&&(sct[i][j].altitude!=WATER)) {
			if (rand()%3==0)
				sct[i][j].altitude=MOUNTAIN;
			else	sct[i][j].altitude=HILL;

			if( rand()%100 < chance ) {
				if(rand()%2==0)
					getmetal( &sct[i][j] );
				else	getjewel( &sct[i][j] );
			}
		}
		return;
	case HUMAN:
		sct[x][y].altitude = CLEAR;
		for(i=x-range;i<=x+range;i++)
		for(j=y-range;j<=y+range;j++)
		if((i!=x)&&(j!=y)&&(ONMAP(i,j))
		&&(sct[i][j].altitude!=WATER)) {
			if (rand()%2==0)
			sct[i][j].altitude = CLEAR;

			if (rand()%2==0)
				sct[i][j].vegetation=WOOD;
			else	sct[i][j].vegetation=GOOD;

			if( rand()%100 < chance ) {
				if (rand()%2==0)
					getmetal( &sct[i][j] );
				else	getjewel( &sct[i][j] );
			}
		}
		return;
	}
}

/*
 * mailtopc - Send notification message to all player character nations
 *
 * Broadcasts a system notification to all active player-controlled nations
 * in the game world. Used primarily to announce significant events such as
 * new player arrivals, system messages, or world updates that affect all
 * players.
 *
 * Parameters:
 *   string - Message text to send to all PC nations (null-terminated string)
 *
 * Returns:
 *   None (void function)
 *
 * Side Effects:
 *   - Opens mail files for all active PC nations
 *   - Writes standardized message format to each nation's mailbox
 *   - Closes mail files after writing
 *   - May generate file I/O operations for each active nation
 *
 * Message Format:
 *   "Message from Conquer\n\n[user-provided string]\n"
 *
 * Algorithm:
 *   1. Iterate through all nation slots (0 to NTOTAL-1)
 *   2. Check if nation 0 (special) or any PC-controlled nation (ispc())
 *   3. Attempt to open mail file for each qualifying nation
 *   4. Write standardized header and user message
 *   5. Close mail file and continue to next nation
 *   6. Silently skip nations where mail file cannot be opened
 *
 * Error Handling:
 *   - Gracefully handles mail file open failures (continues to next nation)
 *   - No error reporting for failed mail operations
 *   - Does not interrupt processing if individual mail operations fail
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires mail system and nation data structures
 *   Approach: Integration testing with mock mail system and nation configurations
 *   Key Tests: [PC nation filtering, mail file operations, message formatting]
 *   Dependencies: [Nation array ntn[], mail functions mailopen/mailclose, file pointer fm]
 *   Mock Requirements: [Nation status data, mail file system, file I/O operations]
 *   Complexity: Simple - straightforward iteration with mail system integration
 *
 * Notes:
 *   - Uses global mail file pointer fm for writing
 *   - Relies on mailopen() and mailclose() for file management
 *   - Nation 0 always receives messages regardless of PC status
 *   - Critical for multiplayer communication and event notification
 *   - Message delivery is best-effort (no delivery confirmation)
 */
void
mailtopc (char *string)
{
	int	ctry;
	for( ctry=0; ctry<NTOTAL; ctry++){
		if((ctry==0)||(ispc(ntn[ctry].active))){
			if (mailopen(ctry)!=(-1)) {
				fprintf(fm,"Message from Conquer\n\n");
				fprintf(fm,"%s\n",string);
				mailclose(ctry);
			}
		}
	}
}

/*
 * newinit - Initialize curses display system for interactive nation building
 *
 * Sets up the curses-based terminal interface for the new player registration
 * system. Validates terminal capabilities and enforces minimum size requirements
 * to ensure proper display of the nation builder interface.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   None (void function, but may exit program on terminal size failure)
 *
 * Side Effects:
 *   - Initializes curses display system (initscr())
 *   - Enables character-at-a-time input mode (crmode())
 *   - Disables automatic echo of typed characters (noecho())
 *   - May terminate program if terminal size is insufficient
 *   - Displays error messages and sounds alert for terminal size issues
 *
 * Terminal Requirements:
 *   - Minimum 80 columns by 24 lines
 *   - Supports curses/ncurses terminal capabilities
 *   - Compatible with standard terminal control sequences
 *
 * Error Handling:
 *   - Validates terminal size after initialization
 *   - Displays helpful error message if terminal too small
 *   - Provides specific size requirements in error display
 *   - Sounds audible alert (beep()) for user attention
 *   - Waits for user acknowledgment before clean exit
 *   - Calls newbye(SUCCESS) for graceful program termination
 *
 * Display Configuration:
 *   - crmode(): Enables immediate character input without buffering
 *   - noecho(): Prevents automatic display of typed characters (for password input)
 *   - Both settings essential for interactive menu navigation and security
 *
 * Testing Notes:
 *   Category: C (System) - Requires actual terminal/curses environment
 *   Approach: System testing with various terminal sizes and configurations
 *   Key Tests: [Terminal size validation, curses initialization, input mode setup]
 *   Dependencies: [Curses library, terminal environment, COLS/LINES globals]
 *   Mock Requirements: [Terminal emulator, curses system, display hardware]
 *   Complexity: Simple - standard curses initialization with size validation
 *
 * Notes:
 *   - Must be called before any curses display operations
 *   - Paired with newreset() for proper curses cleanup
 *   - Critical for proper interactive interface functionality
 *   - Terminal size check prevents interface corruption on small displays
 *   - Required for secure password input handling in registration
 */
void
newinit (void)
{
	initscr();
	/* check terminal size */
	if (COLS<80 || LINES<24) {
		mvaddstr(LINES-2,0,"conqrun: terminal should be at least 80x24");
		mvaddstr(LINES-1,0,"Please try again with a different setup");
		beep();
		getch();
		newbye(SUCCESS);
	}
	crmode();
	noecho();
}

/*
 * newreset - Clean up and terminate curses display system
 *
 * Properly shuts down the curses display interface and restores the terminal
 * to its normal state. This function ensures clean terminal cleanup before
 * program termination or when switching between curses and non-curses modes.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   None (void function)
 *
 * Side Effects:
 *   - Clears the entire screen display
 *   - Forces refresh to ensure clean visual state
 *   - Disables character-at-a-time input mode (nocrmode())
 *   - Terminates curses session and restores normal terminal mode (endwin())
 *   - Returns terminal to pre-curses state
 *
 * Cleanup Sequence:
 *   1. clear(): Erases all content from the screen
 *   2. refresh(): Forces immediate display update to show cleared screen
 *   3. nocrmode(): Restores normal line-buffered input mode
 *   4. endwin(): Properly terminates curses and restores terminal settings
 *
 * Terminal Restoration:
 *   - Restores original terminal attributes (echo, buffering, special chars)
 *   - Releases curses resources and memory
 *   - Ensures cursor is visible and positioned appropriately
 *   - Returns terminal to shell-compatible state
 *
 * Testing Notes:
 *   Category: C (System) - Requires actual terminal/curses environment
 *   Approach: System testing with terminal state verification before/after
 *   Key Tests: [Screen clearing, terminal restoration, input mode reset]
 *   Dependencies: [Active curses session, terminal environment, curses library]
 *   Mock Requirements: [Terminal emulator, curses system, display hardware]
 *   Complexity: Simple - standard curses cleanup sequence
 *
 * Notes:
 *   - Must be paired with newinit() for proper curses lifecycle management
 *   - Essential for clean program termination to prevent terminal corruption
 *   - Can be called multiple times safely (endwin() handles redundant calls)
 *   - Critical for proper shell prompt restoration after program exit
 *   - Used by newbye() for complete program termination sequence
 */
void
newreset (void)
{
	clear();
	refresh();
	nocrmode();
	endwin();
}

/*
 * newbye - Gracefully terminate the new player registration program
 *
 * Performs complete program shutdown including proper curses cleanup and
 * system exit with specified status code. Ensures clean terminal restoration
 * and proper resource cleanup before program termination.
 *
 * Parameters:
 *   status - Exit status code to return to the operating system
 *            SUCCESS (0) for normal termination
 *            FAIL (non-zero) for error conditions
 *
 * Returns:
 *   Does not return (calls exit() which terminates the program)
 *
 * Side Effects:
 *   - Calls newreset() to clean up curses display system
 *   - Restores terminal to normal state
 *   - Terminates program execution with specified exit code
 *   - Returns control to operating system/shell
 *
 * Exit Sequence:
 *   1. Call newreset() for complete curses cleanup
 *   2. Call exit(status) to terminate program with given status
 *
 * Status Code Usage:
 *   - SUCCESS: Normal program completion (successful registration or clean exit)
 *   - FAIL: Error conditions (file errors, system problems, user cancellation)
 *   - Other codes: Specific error conditions as defined by system constants
 *
 * Testing Notes:
 *   Category: C (System) - Requires actual program execution environment
 *   Approach: System testing with exit status verification and terminal state
 *   Key Tests: [Proper exit status codes, terminal cleanup verification, resource release]
 *   Dependencies: [Curses system, terminal environment, system exit() function]
 *   Mock Requirements: [Process execution environment, terminal emulator, exit monitoring]
 *   Complexity: Simple - standard cleanup and exit sequence
 *
 * Notes:
 *   - This function never returns to caller (exit() terminates process)
 *   - Essential for proper terminal restoration in error conditions
 *   - Used throughout newlogin system for both normal and error exits
 *   - Prevents terminal corruption that could occur with abrupt termination
 *   - Critical for clean integration with shell environment
 */
void
newbye (int status)
{
	newreset();
	exit(status);
}

/*
 * newmsg - Display status message without waiting for user input
 *
 * Shows a status message on the bottom line of the screen and immediately
 * refreshes the display. Used for providing real-time feedback during nation
 * creation without interrupting the user's workflow or requiring acknowledgment.
 *
 * Parameters:
 *   str - Message string to display (null-terminated)
 *
 * Returns:
 *   None (void function)
 *
 * Side Effects:
 *   - Displays message on the last line of the screen (LINES-1)
 *   - Clears any existing content on the message line
 *   - Forces immediate screen refresh to show the message
 *   - Does not wait for user input (non-blocking)
 *
 * Display Behavior:
 *   - Message appears at screen position (LINES-1, 0)
 *   - clrtoeol() removes any previous message content
 *   - refresh() ensures immediate visual update
 *   - Message remains visible until next screen update or message
 *
 * Usage Context:
 *   - Status updates during nation building process
 *   - Confirmation messages for user actions
 *   - Progress indicators during point allocation
 *   - Non-critical informational messages
 *   - Temporary feedback that doesn't require user response
 *
 * Comparison with newerror():
 *   - newmsg(): Non-blocking, immediate feedback, temporary display
 *   - newerror(): Blocking, requires keystroke, persistent until acknowledged
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires curses environment and display testing
 *   Approach: Integration testing with curses display verification
 *   Key Tests: [Message display positioning, line clearing, immediate refresh]
 *   Dependencies: [Active curses session, LINES global, screen display]
 *   Mock Requirements: [Curses display system, terminal emulator, screen buffer]
 *   Complexity: Simple - basic curses message display without blocking
 *
 * Notes:
 *   - Requires active curses session (newinit() must be called first)
 *   - Message positioning depends on LINES global variable
 *   - Essential for providing responsive user interface feedback
 *   - Complements blocking error display functions for complete UI messaging
 *   - Critical for user experience during interactive nation building
 */
void
newmsg (char *str)
{
	mvaddstr(LINES-1,0,str);
	clrtoeol();
	refresh();
}

/* message with wait for keystroke */
void
newerror (char *str)
{
	mvaddstr(LINES-1, 0, str);
	clrtoeol();
	mvaddstr(LINES-1, COLS-16, "PRESS ANY KEY");
	beep();
	refresh();
	getch();
	move(LINES-1,0);
	clrtoeol();
	refresh();
}

/*
 * in_str - Check if a character exists in a string
 *
 * Searches through a string to determine if a specific character is present.
 * This utility function provides case-sensitive character matching for input
 * validation and menu choice processing in the registration system.
 *
 * Parameters:
 *   ch  - Character to search for
 *   str - Null-terminated string to search in (must not be NULL)
 *
 * Returns:
 *   TRUE if character is found in the string
 *   FALSE if character is not found or string is empty
 *
 * Side Effects:
 *   None - read-only operation
 *
 * Testing Notes:
 *   Category: A (Unit) - Simple string search with minimal dependencies
 *   Approach: Unit tests with various character and string combinations
 *   Key Tests: [Character found, character not found, empty string, single character, NULL handling]
 *   Dependencies: [strlen() function from string.h, TRUE/FALSE constants]
 *   Mock Requirements: None - pure function with standard library dependencies
 *   Complexity: Simple - straightforward linear search algorithm
 *
 * Notes:
 *   - Case-sensitive comparison using direct character equality
 *   - Linear time complexity O(n) where n is string length
 *   - Uses strlen() to determine search bounds
 *   - Essential for validating user menu choices and input characters
 *   - Could be optimized with strchr() but current implementation is clear
 */
int
in_str (int ch, char *str)
{
	int i,l=safe_size_to_int(strlen(str));

	for(i=0;i<l;i++)
		if (ch == str[i]) return(TRUE);
	return(FALSE);
}

/*
 * errorbar - Display formatted comment and error window
 *
 * Creates a visually distinctive window at the bottom of the screen to display
 * informational or error messages. Uses inverse video highlighting and a
 * horizontal separator line to draw attention to important messages during
 * the registration process.
 *
 * Parameters:
 *   str1 - Primary message content (displayed after version info)
 *   str2 - Secondary message (right-aligned on the information line)
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Creates highlighted bar on line LINES-4 using standout/standend
 *   - Displays "Conquer [VERSION].[PATCHLEVEL]: [str1]" on line LINES-3
 *   - Right-aligns str2 on the same line as version info
 *   - Draws horizontal separator line of dashes on line LINES-2
 *   - Modifies cursor position to end of separator line
 *
 * Testing Notes:
 *   Category: C (System) - Requires curses initialization and terminal dimensions
 *   Approach: System testing with mock terminal and display verification
 *   Key Tests: [Highlighting display, text positioning, line drawing, version formatting]
 *   Dependencies: [Curses library, LINES/COLS globals, VERSION/PATCHLEVEL constants]
 *   Mock Requirements: [Terminal screen, version constants, curses standout capability]
 *   Complexity: Moderate - multi-line formatting with alignment and highlighting
 *
 * Notes:
 *   - Uses curses standout mode for visual emphasis (reverse video)
 *   - Calculates right-alignment based on string length and COLS width
 *   - Creates professional-looking information display consistent with UI
 *   - Essential for user feedback during registration validation
 *   - Assumes VERSION and PATCHLEVEL are defined string constants
 */
void
errorbar (char *str1, char *str2)
{
	int i;
	move(LINES-4,0);
	standout();
	for(i=0;i<COLS-1;i++)
		addch(' ');
	standend();
	mvprintw(LINES-3,0," Conquer %s.%s: %s",VERSION,PATCHLEVEL,str1);
	mvaddstr(LINES-3,COLS-safe_size_to_int(strlen(str2))-2,str2);
	move(LINES-2,0);
	for(i=0;i<COLS-1;i++)
		addch('-');
}

/*
 * dispitem - Display formatted item amount string at current cursor location
 *
 * Displays a formatted string showing the amount and type of a specific item
 * at the current cursor position. Handles special formatting for location items
 * and provides detailed breakdown for raw materials including derived jewel
 * and metal amounts.
 *
 * Parameters:
 *   item   - Item type identifier (CH_LOCATE, CH_RAWGOODS, or other item types)
 *   amount - Quantity of the item to display
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Prints formatted text at current cursor position using printw()
 *   - For location items: displays location type and item name
 *   - For other items: displays numeric amount and item name
 *   - For raw materials: adds calculated jewel and metal amounts
 *   - Adds period terminator for non-raw-goods items
 *
 * Testing Notes:
 *   Category: C (System) - Requires curses initialization and global arrays
 *   Approach: System testing with mock item arrays and cursor positioning
 *   Key Tests: [Location formatting, numeric formatting, raw materials calculation, period termination]
 *   Dependencies: [Curses library, LType[] array, Mitems[] array, NLJEWELS/NLMETAL/Mvalues constants]
 *   Mock Requirements: [Terminal screen, global item arrays, calculation constants]
 *   Complexity: Moderate - conditional formatting with calculation logic
 *
 * Notes:
 *   - Uses global arrays LType[] and Mitems[] for item name lookup
 *   - Special handling for CH_LOCATE uses amount as index into LType[]
 *   - Raw materials calculation uses ratio formulas with floating-point conversion
 *   - NLJEWELS/Mvalues[CH_RAWGOODS] ratio determines jewel conversion
 *   - NLMETAL/Mvalues[CH_RAWGOODS] ratio determines metal conversion
 *   - Essential for displaying resource allocation in registration interface
 */
void
dispitem (int item, long amount)
{
	if (item == CH_LOCATE) {
		printw("%s %s", LType[amount], Mitems[item]);
	} else {
		printw("%ld %s", amount, Mitems[item]);
	}

	if (item != CH_RAWGOODS) {
		addch('.');
		return;
	}

	/* now show the extras for the Raw Materials */
	printw(", %ld jewels", (long) (safe_long_to_float(amount) *
		safe_long_to_float(NLJEWELS)/safe_long_to_float(Mvalues[CH_RAWGOODS])));
	printw(", and %ld metal.", (long) (safe_long_to_float(amount) *
		safe_long_to_float(NLMETAL)/safe_long_to_float(Mvalues[CH_RAWGOODS])));
}

/*
 * showitem - Display current allocated amount for a country item
 *
 * Shows the currently allocated amount for a specific country item at a
 * specified line position. Handles special formatting for location items
 * and provides detailed breakdown for raw materials showing derived jewel
 * and metal amounts. Uses right-aligned formatting for consistent display.
 *
 * Parameters:
 *   line - Screen line number where to display the information
 *   item - Item type identifier (CH_LOCATE, CH_RAWGOODS, or other item types)
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Moves cursor to specified line, column 15
 *   - Displays right-aligned (23 characters) formatted item information
 *   - For location items: shows location type and item name from spent[] array
 *   - For other items: shows calculated amount (spent[item] * Mvalues[item])
 *   - For raw materials: adds additional line showing jewel/metal breakdown
 *   - Uses conditional compilation for equal vs. separate jewel/metal display
 *
 * Testing Notes:
 *   Category: C (System) - Requires curses initialization and global arrays
 *   Approach: System testing with mock spent[] array and screen positioning
 *   Key Tests: [Location formatting, amount calculation, raw materials display, alignment]
 *   Dependencies: [Curses library, spent[] array, LType/Mitems arrays, Mvalues constants, NLJEWELS/NLMETAL]
 *   Mock Requirements: [Terminal screen, global allocation arrays, calculation constants]
 *   Complexity: Moderate - conditional formatting with preprocessor compilation paths
 *
 * Notes:
 *   - Uses global spent[] array to track current resource allocations
 *   - Mvalues[item] provides cost multiplier for amount calculation
 *   - LINELTH constant defines temporary string buffer size
 *   - Conditional compilation (#if NLJEWELS==NLMETAL) handles display variants
 *   - Right-aligned formatting ensures consistent column alignment
 *   - Essential for displaying current allocation state during registration
 *   - Note: nsprintf on line 851 appears to be typo for sprintf
 */
void
showitem (int line, int item)
{
	char tempc[LINELTH];

	move(line,15);
	if (item == CH_LOCATE) {
		sprintf(tempc,"%s %s", LType[spent[item]], Mitems[item]);
		printw("%23s",tempc);
	} else {
		sprintf(tempc,"%ld %s", spent[item]*Mvalues[item], Mitems[item]);
		printw("%23s",tempc);
	}

	if (item != CH_RAWGOODS) return;
	line++;

	/* now show the extras for the Raw Materials */
#if NLJEWELS==NLMETAL
	sprintf(tempc,"%ld jewels & metal",
		   spent[CH_RAWGOODS]*NLJEWELS);
	mvprintw(line,0,"%38s",tempc);
	mvprintw(line,COLS/2+13,"%ld jewels & metal",NLJEWELS);
#else
	sprintf(tempc,"%ld jewels",
		   spent[CH_RAWGOODS]*NLJEWELS);
	mvprintw(line,0,"%38s",tempc);
	mvprintw(line++,COLS/2+10,"%ld jewels",NLJEWELS);
	nsprintf(tempc,"%ld metal",
		    spent[CH_RAWGOODS]*NLMETAL);
	mvprintw(line,0,"%38s",tempc);
	mvprintw(line,COLS/2+13,"%ld metals",NLMETAL);
#endif
}

/*
 * convert - Convert stored allocation information into nation statistics
 *
 * Transforms the temporary spent[] array values into actual nation
 * statistics by applying appropriate multipliers and conversions.
 * Handles location type conversion, magic acquisition, and resource
 * calculations to finalize the new nation's starting attributes.
 *
 * Parameters:
 *   None (operates on global spent[] array and curntn structure)
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Sets curntn->tciv (total civilians) from people allocation
 *   - Sets curntn->tgold (treasury) from treasury allocation
 *   - Converts location preference to actual location type (GREAT/FAIR/RANDOM)
 *   - Sets curntn->tmil (total military) from soldier allocation
 *   - Sets combat bonuses (aplus/dplus) from attack/defense allocations
 *   - Sets curntn->repro (reproduction rate) from reproduction allocation
 *   - Sets curntn->maxmove (movement points) from movement allocation
 *   - Purchases random magic spells based on magic allocation
 *   - Sets numleaders global from leaders allocation
 *   - Calculates raw resources (tfood, jewels, metals) from raw goods
 *   - Uses random number generation for magic spell acquisition
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires nation structure and magic system integration
 *   Approach: Integration testing with mock spent[] array and nation structure
 *   Key Tests: [Multiplier calculations, location conversion, magic acquisition, resource allocation]
 *   Dependencies: [Global spent[] array, curntn nation pointer, Mvalues[] constants, magic system]
 *   Mock Requirements: [Nation structure, magic functions, random number generator]
 *   Complexity: Moderate - multiple conversions with magic system integration
 *
 * Notes:
 *   - Uses Mvalues[] array as cost-to-benefit multipliers for each resource type
 *   - Location conversion: 2=GREAT, 1=FAIR, 0/default=RANDOM placement
 *   - Magic acquisition uses random selection from military magic range (M_MIL to M_MGK)
 *   - getmagic() function returns magic type, 0 indicates failure/retry needed
 *   - CHGMGK macro likely updates magic-related nation statistics
 *   - Raw goods generate both food and derived jewel/metal resources
 *   - Critical function that finalizes nation creation from user choices
 */
void
convert (void)
{
	int i,loop;
	long x;

	curntn->tciv = Mvalues[CH_PEOPLE] * spent[CH_PEOPLE];
	curntn->tgold = Mvalues[CH_TREASURY] * spent[CH_TREASURY];
	switch(spent[CH_LOCATE]) {
	case 2:
		curntn->location = GREAT;
		break;
	case 1:
		curntn->location = FAIR;
		break;
	case 0:
	default:
		curntn->location = RANDOM;
		break;
	}
	curntn->tmil = Mvalues[CH_SOLDIERS] * spent[CH_SOLDIERS];
	curntn->aplus = safe_long_to_short(Mvalues[CH_ATTACK] * spent[CH_ATTACK]);
	curntn->dplus = safe_long_to_short(Mvalues[CH_DEFEND] * spent[CH_DEFEND]);
	curntn->repro = safe_long_to_char(Mvalues[CH_REPRO] * spent[CH_REPRO]);
	curntn->maxmove = safe_long_to_uchar(Mvalues[CH_MOVEMENT]
		* spent[CH_MOVEMENT]);
	for(i = 0; i < spent[CH_MAGIC]; i++) {
		/* purchase magic */
		loop = TRUE;
		while (loop == TRUE) {
			if((x=getmagic((rand()%M_MGK+M_MIL)))!=0L){
				CHGMGK;
				loop = FALSE;
			}
		}
	}
	numleaders = (int) Mvalues[CH_LEADERS] * spent[CH_LEADERS];
	curntn->tfood = Mvalues[CH_RAWGOODS] * spent[CH_RAWGOODS];
	curntn->jewels = NLJEWELS * spent[CH_RAWGOODS];
	curntn->metals = NLMETAL * spent[CH_RAWGOODS];
}

/*
 * newlogin - Interactive nation registration and creation system
 *
 * The master function that provides a complete curses-based interactive interface
 * for new player registration in the Conquer game system. Handles the entire
 * nation creation workflow from initial player input through final map placement,
 * including nation name/password setup, race/alignment selection, point allocation
 * for resources and military, and optional multi-nation registration.
 *
 * This is the largest and most complex function in the registration system,
 * implementing a complete interactive state machine for nation building with
 * comprehensive input validation, resource management, and error handling.
 *
 * Parameters:
 *   realuser - Real user ID from system for user validation and tracking
 *              Used with CHECKUSER compilation flag to enforce one-nation-per-user limit
 *              Set to appropriate uid when called from main registration program
 *
 * Returns:
 *   void (function manages complete registration workflow internally)
 *
 * Side Effects:
 *   - Initializes curses display system for interactive interface
 *   - Scans and allocates available nation slots (country numbers)
 *   - Creates and validates nation names, passwords, and leader names
 *   - Sets up race-specific starting bonuses and restrictions
 *   - Manages interactive point allocation system for resource spending
 *   - Converts allocation choices to final nation statistics
 *   - Places nations on game map with location preference handling
 *   - Updates global nation arrays and game state
 *   - Creates executable file output for nation data
 *   - Sends mail notifications to existing players about new nations
 *   - Supports multi-nation registration in single session (optional)
 *   - Performs complete curses cleanup before return
 *
 * Workflow Overview:
 *   1. **System Initialization**: Sets up curses, finds available nation slots
 *   2. **Nation Identification**: Name, password, leader name input with validation
 *   3. **Race Selection**: Interactive race choice (Dwarf/Elf/Human/Orc) with bonuses
 *   4. **Alignment Selection**: Good/Neutral/Evil alignment (Orcs forced Evil)
 *   5. **National Mark**: Single character map symbol selection
 *   6. **Resource Allocation**: Interactive point-spending system for nation building
 *   7. **Final Confirmation**: Save/delete choice with conversion to game statistics
 *   8. **Map Placement**: Automatic placement based on location preference
 *   9. **Cleanup**: File closure, notifications, optional additional nations
 *
 * Interactive Point Allocation System:
 *   - **Starting Points**: MAXPTS total points to spend on nation attributes
 *   - **Resource Categories**: People, Treasury, Soldiers, Attack/Defense bonuses,
 *     Reproduction rate, Movement points, Magic spells, Leaders, Raw materials,
 *     Location preference (better starting positions cost more)
 *   - **Race-Specific Modifiers**: Each race has different costs and restrictions
 *   - **Real-time Interface**: Curses-based menu with immediate feedback
 *   - **Validation**: Prevents overspending, enforces minimums/maximums
 *   - **Key Bindings**: Standard vi-like navigation (hjkl) plus space/escape
 *
 * Race-Specific Features:
 *   - **Dwarfs**: MINER power, starting treasury/raw materials bonus
 *   - **Elves**: THE_VOID power (magical cloaking), FAIR starting location
 *   - **Humans**: WARRIOR power (combat bonus), standard balanced stats
 *   - **Orcs**: MI_MONST power, forced Evil alignment, no movement purchases,
 *     modified reproduction costs, different attack/defense calculations
 *
 * Input Validation and Security:
 *   - **Name Length Limits**: Nation names 2-NAMELTH chars, leader names 2-LEADERLTH
 *   - **Reserved Names**: Prevents "god", "unowned", duplicate nation names
 *   - **Password Security**: 2-PASSLTH character passwords with confirmation
 *   - **Cryptographic Storage**: Uses crypt() with SALT for password hashing
 *   - **User Limits**: Optional CHECKUSER enforcement of one-nation-per-user
 *   - **Input Sanitization**: All user input validated for length and content
 *
 * Error Handling and Recovery:
 *   - **File Operation Errors**: Graceful handling of executable file creation failures
 *   - **Nation Slot Exhaustion**: Clear error messages when no slots available
 *   - **Invalid Input**: Comprehensive validation with user-friendly error messages
 *   - **Point Allocation Errors**: Real-time validation prevents impossible purchases
 *   - **Memory Management**: Proper cleanup of curses resources on all exit paths
 *   - **User Cancellation**: Allows cancellation at multiple points with cleanup
 *
 * Display and User Interface:
 *   - **Screen Layout**: Multi-line display with headers, menus, and status information
 *   - **Real-time Updates**: Immediate feedback for point allocation and resource changes
 *   - **Visual Highlighting**: Standout mode for current selection and important info
 *   - **Help System**: Context-sensitive help for each allocation category
 *   - **Status Display**: Points remaining, current allocations, costs clearly shown
 *   - **Professional Layout**: Consistent formatting with game branding
 *
 * Integration with Game System:
 *   - **Nation Structure**: Populates complete ntn[] array entry for new nation
 *   - **Map Integration**: Calls place() for automatic map placement
 *   - **Attribute Calculation**: Calls att_setup() for derived nation statistics
 *   - **Global Updates**: Updates nation counts, writes data files
 *   - **Notification System**: Sends mail to existing players about new nation
 *   - **Turn Integration**: Records turn number for nation creation tracking
 *
 * Multi-Nation Support:
 *   - **Session Continuation**: Allows multiple nations in single registration session
 *   - **Slot Management**: Tracks available slots across multiple registrations
 *   - **User Choice**: Optional continuation prompt after each nation creation
 *   - **Resource Cleanup**: Proper cleanup between nation creation cycles
 *
 * Testing Notes:
 *   Category: C (System) - Requires full game environment with curses, file system, magic system
 *   Approach: System testing with complete game world setup and user interaction simulation
 *   Key Tests: [Complete registration workflow, race-specific features, point allocation system,
 *              input validation, error handling, multi-nation registration, curses interface]
 *   Dependencies: [Curses library, nation arrays, magic system, map system, file I/O,
 *                 password encryption, mail system, global game state, constants/arrays]
 *   Mock Requirements: [Complete game world, terminal interface, file system, user input simulation,
 *                      random number generation, cryptographic functions, mail delivery system]
 *   Complexity: Complex - Large interactive state machine with multiple subsystems integration
 *
 * Performance Considerations:
 *   - **Map Placement**: Uses iterative random placement with 2000-attempt limit
 *   - **Input Validation**: Real-time validation prevents expensive rollback operations
 *   - **Memory Usage**: Minimal dynamic allocation, uses global arrays efficiently
 *   - **Display Updates**: Selective screen refreshing for responsive interface
 *   - **File I/O**: Single executable file per nation, closed promptly
 *
 * Security Considerations:
 *   - **Password Hashing**: Strong cryptographic hashing with salt
 *   - **Input Sanitization**: All user input validated and bounded
 *   - **File Security**: Proper file handle management prevents resource leaks
 *   - **User Authentication**: Optional real user ID tracking and enforcement
 *   - **Session Management**: Clean state management prevents information leakage
 *
 * Notes:
 *   - This function is the core of the entire player registration system
 *   - Requires newinit() to be effective but calls it internally
 *   - Must be called with valid nation arrays and game world initialized
 *   - Uses extensive global state but manages it carefully for multiple nations
 *   - Critical for game population growth and new player onboarding
 *   - Complex control flow with multiple nested loops and state validation
 *   - Integration point for multiple game subsystems (magic, combat, economics)
 *   - Essential for maintaining game balance through controlled nation creation
 */
void
newlogin (int realuser)
{
	/* use points to create empire, add if late starter*/
	int points, clr;
	int pccount;
	int choice, direct;
	int valid=TRUE;  /*valid==FALSE means continue loop*/
	int temp,ypos,xpos;
	int more=TRUE;	/*0 if add another player*/
	long x;
	char tempc[LINELTH];
	char passwd[PASSLTH+1];
	register int i;

	/* setup curses display */
	newinit();

	/*find valid nation number type*/
	country=0;
        pccount=0;
	for(i=1;i<NTOTAL;i++)
		if(ntn[i].active==INACTIVE)
		{
			country=safe_int_to_short(i);
			curntn = &ntn[country];
			break;
		}
		else
		{
#ifdef CHECKUSER
			if ((ntn[i].uid == realuser) &&
			    (ntn[0].uid != realuser))
			{
			   newerror ("You may only have one nation in this world.");
			   newreset();
			   return;
			}
#endif
			pccount++;
		}

#ifdef MONSTER
	pccount = pccount + 4;	/* space taken by monster nations */
#endif
	while(more==TRUE) {
		clear();

		sprintf(tempc,"Country #%d", country);
		errorbar("Nation Builder",tempc);
		if((country==0)||(pccount+1>=NTOTAL-REVSPACE)) {
			newerror("No more nations available");
			newreset();
			return;
		}

		/* open output for future printing*/
		mvprintw(0,0,"Building Country Number %d",country);
		sprintf(tempc,"%s%d",exefile,i);
		if ((fexe=fopen(tempc,"w"))==NULL) {
			char errmsg[LINELTH*2];
			snprintf(errmsg,sizeof(errmsg),"Error opening <%s>",tempc);
			newerror(errmsg);
			newbye(FAIL);
		}

		/* begin purchasing */
		points=MAXPTS;
		for (i=0; i<CH_NUMBER; i++) {
			spent[i] = 0;
		}

		valid=FALSE;
		/*get name*/
		while(valid==FALSE) {
			valid=TRUE;
			mvaddstr(1,0,"Enter a Name for your Country: ");
			clrtoeol();
			refresh();
			get_nname(tempc);

			if((strlen(tempc)<=1)
			 ||(strlen(tempc)>NAMELTH)){
				newerror("Invalid Name Length");
				valid=FALSE;
			}

			/*check if already used*/
			if((strcmp(tempc,"god")==0)
			||(strcmp(tempc,"unowned")==0)){
				newerror("Name already used");
				valid=FALSE;
			}

			for(i=1;i<NTOTAL;i++)
			if((i!=country)&&(strcmp(ntn[i].name,tempc)==0)&&(isntn(ntn[i].active))) {
				newerror("Name already used");
				valid=FALSE;
			}
		}
		strcpy(curntn->name,tempc);
		move(0,0);
		clrtoeol();
		move(1,0);
		clrtoeol();
		standout();
		mvprintw(0,COLS/2-15-safe_size_to_int(strlen(curntn->name))/2,
			"< Building Country %s >",curntn->name);
		standend();
		clrtoeol();

		valid=FALSE;
		while(valid==FALSE) {			/* password routine */
			mvaddstr(2,0,"Enter National Password: ");
			clrtoeol();
			refresh();
			i = get_pass(tempc);
			if (i < 2) {
				newerror("Password Too Short");
				continue;
			} else if (i > PASSLTH) {
				newerror("Password Too Long");
				continue;
			}
			mvaddstr(2,0,"Reenter National Password: ");
			clrtoeol();
			refresh();
			i = get_pass(passwd);

			if((i<2)||(i>PASSLTH)||(strncmp(passwd,tempc,PASSLTH)!=0)){
				newerror("Invalid Password Match");
			} else valid=TRUE;
		}
		snprintf(curntn->passwd, PASSLTH+1, "%s", crypt(tempc,SALT));

		/*get your name*/
		valid=FALSE;
		while(valid==FALSE) {
			valid=TRUE;
			mvaddstr(2,0,"Enter the name of your country's leader (Ex. The Ed, Gandalf, Conan)");
			clrtoeol();
			mvprintw(3,0,"    [maximum %d characters]: ",LEADERLTH);
			refresh();
			get_nname(tempc);
			if((strlen(tempc)>LEADERLTH)||(strlen(tempc)<2)) {
				newerror("Invalid Name Length");
				valid=FALSE;
			}
			else strcpy(curntn->leader,tempc);
		}

		mvprintw(2,0,"Leader Name: %s", curntn->leader);
		clrtoeol();
		mvaddstr(3,0,"Enter your Race [ (D)warf (E)lf (H)uman (O)rc ]:");
		clrtoeol();
		refresh();
		valid=FALSE;
		while(valid==FALSE) {
			valid=TRUE;
			switch(getch()) {
			case 'D':
			case 'd':
				/*MINER POWER INATE TO DWARVES*/
				newmsg("Dwarves have MINING skills");
				mvaddstr(3,0,"National Race: Dwarf");
				clrtoeol();
				curntn->powers=MINER;
				x=MINER;
				CHGMGK;
				points -= getclass(DWARF);
				curntn->race=DWARF;
				spent[CH_TREASURY]=NLDGOLD;
				spent[CH_RAWGOODS]=NLDRAW;
				spent[CH_PEOPLE]= NLDCIVIL;
				spent[CH_SOLDIERS]= NLDMILIT;
				spent[CH_REPRO]= NLDREPRO;
				spent[CH_MOVEMENT]= NLDMMOVE;
				spent[CH_ATTACK]= NLDAPLUS;
				spent[CH_DEFEND]= NLDDPLUS;
				spent[CH_LOCATE]= NLRANDOM;
				points-=nstartcst();
				break;
			case 'E':
			case 'e':
				newmsg("Elves are magically cloaked (VOID power)");
				mvaddstr(3,0,"National Race: Elf");
				clrtoeol();
				curntn->powers=THE_VOID;
				x=THE_VOID;
				CHGMGK;
				points -= getclass(ELF);
				curntn->race=ELF;
				spent[CH_TREASURY]=NLEGOLD;
				spent[CH_RAWGOODS]=NLERAW;
				spent[CH_PEOPLE]= NLECIVIL;
				spent[CH_SOLDIERS]= NLEMILIT;
				spent[CH_REPRO]= NLEREPRO;
				spent[CH_MOVEMENT]= NLEMMOVE;
				spent[CH_ATTACK]= NLEAPLUS;
				spent[CH_DEFEND]= NLEDPLUS;
				spent[CH_LOCATE]= NLFAIR;
				points-=nstartcst();
				break;
			case 'O':
			case 'o':
				/*MINOR MONSTER POWER INATE TO ORCS*/
				newmsg("Your leader is a Monster!");
				mvaddstr(3,0,"National Race: Orc");
				clrtoeol();
				curntn->powers=MI_MONST;
				x=MI_MONST;
				CHGMGK;
				points -= getclass(ORC);
				curntn->race=ORC;
				spent[CH_TREASURY]=NLOGOLD;
				spent[CH_RAWGOODS]=NLORAW;
				spent[CH_PEOPLE]= NLOCIVIL;
				spent[CH_SOLDIERS]= NLOMILIT;
				spent[CH_REPRO]= NLOREPRO;
				spent[CH_MOVEMENT]= NLOMMOVE;
				spent[CH_ATTACK]= NLOAPLUS;
				spent[CH_DEFEND]= NLODPLUS;
				spent[CH_LOCATE]= NLRANDOM;
				points-=nstartcst();
				break;
			case 'H':
			case 'h':
				curntn->race=HUMAN;
				newmsg("Humans have the combat skill of a WARRIOR");
				mvaddstr(3,0,"National Race: Human");
				clrtoeol();
				curntn->powers = WARRIOR;
				x=WARRIOR;
				CHGMGK;
				points -= getclass(HUMAN);
				spent[CH_TREASURY]=NLHGOLD;
				spent[CH_RAWGOODS]=NLHRAW;
				spent[CH_PEOPLE]= NLHCIVIL;
				spent[CH_SOLDIERS]= NLHMILIT;
				spent[CH_REPRO]= NLHREPRO;
				spent[CH_MOVEMENT]= NLHMMOVE;
				spent[CH_ATTACK]= NLHAPLUS;
				spent[CH_DEFEND]= NLHDPLUS;
				spent[CH_LOCATE]= NLRANDOM;
				points-=nstartcst();
				break;
			default:
				valid=FALSE;
			}
		}
		mvprintw(4,0,"Nation Class: %s",Class[curntn->class]);
		clrtoeol();

		valid=FALSE;
		if( curntn->race == ORC ) {	/* orcs are always evil */
			valid=TRUE;
			curntn->active=PC_EVIL;
		} else {
			mvaddstr(5,0,"Please Enter Alignment [ (G)ood, (N)eutral, (E)vil ]");
			refresh();
		}
		while (valid==FALSE) {
			valid=TRUE;
			switch(getch()) {
			case 'G':
			case 'g':
				curntn->active=PC_GOOD;
				break;
			case 'N':
			case 'n':
				curntn->active=PC_NEUTRAL;
				break;
			case 'E':
			case 'e':
				curntn->active=PC_EVIL;
				break;
			default:
				valid=FALSE;
				break;
			}
		}
		mvprintw(2,COLS/2,"Alignment: %s", alignment[curntn->active]);
		clrtoeol();


		/* get new nation mark */
		curntn->mark = ' ';
		while(TRUE) {
			temp = 30;
			mvaddstr(6,0,"This can be any of the following:");
			for (tempc[0]='!';tempc[0]<='~';tempc[0]++) {
				if( markok( tempc[0], FALSE ) ) {
					temp += 2;
					if (temp>COLS-20) {
						printw("\n    ");
						temp = 8;
					}
					printw(" %c",tempc[0]);
				}
			}
			mvaddstr(5,0,"Enter National Mark (for maps): ");
			clrtoeol();
			refresh();
			tempc[0] = safe_int_to_char(getch());
			if( markok( tempc[0], TRUE ) ){
				curntn->mark=(*tempc);
				break;
			}
		}

		mvprintw(3,COLS/2,"National Mark [%c]",curntn->mark);
		clrtoeol();
		move(5,0);
		clrtoeol();
		move(6,0);
		clrtoeol();
		move(7,0);
		clrtoeol();
		refresh();

		ypos = 6;
		mvprintw(ypos,0,"  %-13s       %s", "ITEM", "CURRENTLY HAVE" );
		mvprintw(ypos++,COLS/2+5,"%4s    %s", "COST", "AMOUNT" );
		for(i=0; i<CH_NUMBER; i++) {
			mvprintw(ypos,0,"%-15s", Mlabels[i]);
			showitem(ypos,i);
			if (i==CH_LOCATE) {
				mvprintw(ypos,COLS/2+5,"%3d     %s", Mcost[i],
					    "Better Location");
			} else {
				if (curntn->race==ORC) {
					switch(i) {
					case CH_MOVEMENT:
						mvaddstr(ypos++,COLS/2+5,"  -     --------");
						continue;
					case CH_REPRO:
						x = 2*Munits[i]*Mvalues[i];
						break;
					case CH_ATTACK:
					case CH_DEFEND:
						x = Munits[i]*Mvalues[i]/2;
						break;
					default:
						x = Munits[i]*Mvalues[i];
						break;
					}
					mvprintw(ypos,COLS/2+5,"%3d for",Mcost[i]);
					printw(" %ld %s", x, Mitems[i]);
				} else {
					mvprintw(ypos,COLS/2+5,"%3d for",Mcost[i]);
					printw(" %ld %s", Munits[i]*Mvalues[i], Mitems[i]);
				}
			}
			ypos++;
		}

		/* show everything before menu */
		direct = ADDITION;
		choice = CH_PEOPLE;
		xpos = COLS/2;
		ypos = 7;
		valid = FALSE;
		clr = 1;
		standout();
		mvaddstr(LINES-4,2,"DONE=ESC  EXEC=SPACE  INFO=\"?\"  ADD=\"<+h\"  SUBtract=\">+l\"  UP=\"k\"  DOWN=\"j\"");
		standend();

		while(valid==FALSE) {
			if (clr==1) {
				standout();
				mvprintw(4,COLS/2,"Points Left: %d", points);
				standend();
				clrtoeol();
				clr++;
			} else if (clr==2) {
				newmsg("");
				clr = 0;
			}
			standout();
			mvaddstr(ypos+choice,xpos,Mprompt[direct]);
			standend();
			refresh();
			switch(getch()) {
			case '':
				/* redraw */
				wrefresh(stdscr);
				break;
			case '?':
				/* help on topic */
				newerror(Mhelp[choice]);
				break;
			case '\033':
				/* exit option */
				if (points > 0) {
					newmsg("Use remaining points for population? [ny]");
					if (getch()!='y') {
						newerror("All points must be spent prior to exiting");
						break;
					}
					temp = points * Munits[CH_PEOPLE] / Mcost[CH_PEOPLE];
					x = temp * Mvalues[CH_PEOPLE];
					spent[CH_PEOPLE] += temp;
					showitem(ypos+CH_PEOPLE,CH_PEOPLE);
					points = 0;
					sprintf(tempc,"Buying %ld more civilians", x);
					newerror(tempc);
				}
				newmsg("Is the modification complete? (y or n)");
				while (((temp=getch())!='y')&&(temp!='n')) ;
				if (temp == 'y') {
					valid = TRUE;
				}
				clr = 1;
				break;
			case '-':
			case '>':
			case 'l':
			case 'L':
				/* subtraction */
				direct = SUBTRACTION;
				break;
			case '+':
			case '<':
			case 'h':
			case 'H':
				/* addition */
				direct = ADDITION;
				break;
			case '\b':
			case '\177':
				/* decrease choice -- with wrap */
				mvaddstr(ypos+choice,xpos,"    ");
				if (choice==CH_PEOPLE) {
					choice = CH_RAWGOODS;
				} else {
					choice--;
					if (choice==CH_MOVEMENT && curntn->race==ORC) {
						choice--;
					}
				}
				break;
			case 'k':
			case 'K':
				/* move choice up one */
				if (choice > CH_PEOPLE) {
					mvaddstr(ypos+choice,xpos,"    ");
					choice--;
					if (choice==CH_MOVEMENT && curntn->race==ORC) {
						choice--;
					}
				}
				break;
			case '\r':
			case '\n':
				/* increase choice -- with wrap */
				mvaddstr(ypos+choice,xpos,"    ");
				if (choice==CH_RAWGOODS) {
					choice = CH_PEOPLE;
				} else {
					choice++;
					if (choice==CH_MOVEMENT && curntn->race==ORC) {
						choice++;
					}
				}
				break;
			case 'j':
			case 'J':
				/* move choice down one */
				if (choice < CH_RAWGOODS) {
					mvaddstr(ypos+choice,xpos,"    ");
					choice++;
					if (choice==CH_MOVEMENT && curntn->race==ORC) {
						choice++;
					}
				}
				break;
			case ' ':
			case '.':
				/* make the selection */
				if (curntn->race == ORC) {
					switch(choice) {
					case CH_REPRO:
						temp = 2*Munits[choice];
						break;
					case CH_ATTACK:
					case CH_DEFEND:
						temp = Munits[choice]/2;
						break;
					default:
						temp = Munits[choice];
						break;
					}
				} else temp = Munits[choice];
				if (direct == ADDITION) {
					if (Mcost[choice] > points) {
						sprintf(tempc, "You do not have %d points to spend",
							Mcost[choice]);
						newerror(tempc);
					} else if ((choice == CH_REPRO)&&(curntn->race==ORC)
						&&(spent[choice] + temp > 12)) {
						newerror("You may not purchase any more of that item");
					} else if ((spent[choice] + temp > Maxvalues[choice])
						&&((curntn->race!=ORC)||(choice!=CH_REPRO))) {
						newerror("You may not purchase any more of that item");
					} else {
						spent[choice] += temp;
						newmsg("You now have ");
						dispitem(choice,spent[choice]*Mvalues[choice]);
						showitem(ypos+choice,choice);
						points -= Mcost[choice];
						clr = 1;
					}
				} else if (direct == SUBTRACTION) {
					if (spent[choice] - temp < Minvalues[choice]) {
						newerror("You may not sell back any more of that item");
					} else {
						spent[choice] -= temp;
						newmsg("You now have ");
						dispitem(choice,spent[choice]*Mvalues[choice]);
						showitem(ypos+choice,choice);
						points += Mcost[choice];
						clr = 1;
					}
				}
				break;
			default:
				break;
			}
		}

		/* check for save */
		newmsg("Save this nation? [ny]");
		if(getch()!='y'){
			curntn->active=INACTIVE;
			curntn->powers=0;
			newerror("Ok, Nation Deleted");
			fclose(fexe);
		} else {
			convert();
			place(-1,-1);
			newerror("Ok, Your Nation has been Added to the World");
			att_setup(country);	/* setup values ntn attributes */
#ifdef CHECKUSER
			curntn->uid = safe_int_to_short(realuser);
#endif
			fclose(fexe);
			pccount++;
			sprintf(tempc,"NOTICE: Nation %s added to world on turn %d\n",curntn->name,TURN);
			mailtopc(tempc);
			/* cannot clear until after placement and initializing */
			curntn->powers=0;
		}
#ifndef CHECKUSER
		country=0;
		for(i=1;i<NTOTAL;i++) if (ntn[i].active==INACTIVE) {
			country = i;
			curntn = &ntn[country];
			break;
		}
		if ((country!=0)&&(pccount+1>=NTOTAL-REVSPACE)) {
			newmsg("Do you wish to Add another Nation? [ny]");
			if (getch()!='y') more = FALSE;
			else more = TRUE;
		} else {
			more = FALSE;
			newerror("No More Available Nations");
		}
#else
		more = FALSE;
#endif
	}
	newreset();
	att_base();	/* calculate base nation attributes */
	writedata();
}

/*****************************************************************/
/* PLACE(): put nation on the map.  Fill out army structures too */
/*****************************************************************/
/*
 * place - Place a new nation on the game map with location strategy handling
 *
 * This is a comprehensive nation placement system that handles multiple placement
 * strategies (OOPS, RANDOM, FAIR, GREAT) with increasing quality requirements.
 * The function not only finds a suitable map location but also initializes the
 * complete nation state including armies, territories, and diplomatic relations.
 *
 * The placement algorithm implements a multi-tier system:
 * - OOPS: Emergency placement with minimal constraints
 * - RANDOM: Basic placement avoiding immediate conflicts
 * - FAIR: Quality placement with food and spacing requirements
 * - GREAT: Premium placement with extensive buffer zones and terrain quality
 *
 * Parameters:
 *   xloc - Specific X coordinate for placement, or -1 for algorithm-determined
 *   yloc - Specific Y coordinate for placement, or -1 for algorithm-determined
 *        If both are not -1 and location is habitable, forces placement there
 *
 * Returns:
 *   void - Modifies global game state (curntn, sct, ntn arrays)
 *
 * Side Effects:
 *   - Sets nation capital coordinates (curntn->capx, curntn->capy)
 *   - Establishes initial territory ownership and designation
 *   - Creates and positions all starting armies and leaders
 *   - Distributes population across acquired territories
 *   - Initializes diplomatic status with all other nations
 *   - Calls teraform() to improve terrain around capital
 *   - May recursively call itself on placement failure with degraded strategy
 *   - Modifies map sectors (sct array) for ownership, designation, resources
 *
 * Placement Strategy Details:
 *   OOPS Strategy:
 *   - Last resort placement with minimal constraints
 *   - PC nations: 4-sector border from map edge
 *   - NPC nations: 1-sector border from map edge
 *   - Requires habitable terrain and no adjacent owners
 *   - Rejects locations with 7+ water sectors in 3x3 area
 *   - Applies 25% terraform improvement on success
 *
 *   RANDOM Strategy:
 *   - Basic quality placement for standard nations
 *   - PC nations: 6-12 sector border depending on map size
 *   - NPC nations: 3-sector border from map edge
 *   - Enforces 2-sector buffer from existing nations
 *   - Rejects locations with 7+ water sectors in 3x3 area
 *   - Applies 40% terraform improvement on success
 *
 *   FAIR Strategy:
 *   - Quality placement requiring food production capability
 *   - PC nations: 7-24 sector border depending on map size
 *   - NPC nations: 5-sector border from map edge
 *   - Requires minimum DESFOOD production at capital location
 *   - Enforces 3-sector buffer from existing nations
 *   - Water tolerance varies by world water percentage (>50%: 7+ water rejected, ≤50%: 5+ water rejected)
 *   - Applies 65% terraform improvement on success
 *
 *   GREAT Strategy:
 *   - Premium placement with extensive quality requirements
 *   - PC nations: 9-40 sector border depending on map size
 *   - NPC nations: 6-24 sector border depending on map size
 *   - Enforces 4-sector buffer from existing nations
 *   - Requires no water within 2 sectors of capital
 *   - Complex food production requirements in 5x5 area around capital
 *   - Rejection thresholds: >50% water worlds: 18+ poor sectors, ≤50% water: 15+ poor sectors
 *   - Applies 100% terraform improvement on success
 *
 * Army Initialization System:
 *   - Creates garrison army at capital with portion of total military
 *   - Generates national leader with enhanced movement and defensive positioning
 *   - Creates additional leaders up to class-determined limit (5-7 leaders)
 *   - Distributes remaining military across standard armies with optimal sizing
 *   - All armies initially positioned at capital with appropriate combat status
 *   - Leader armies get 2x normal movement, regular armies get standard movement
 *
 * Territory Expansion System:
 *   - Capital automatically designated and assigned trade good
 *   - Additional territories granted based on placement quality:
 *     * OOPS/RANDOM: No additional territories
 *     * FAIR: 1-sector radius expansion with population distribution
 *     * GREAT: 2-sector radius expansion with population distribution
 *   - Territory acquisition requires: sufficient food production, habitable, unowned, unpopulated
 *   - Acquired territories automatically designated as farms with distributed population
 *
 * Failure Recovery System:
 *   - Each strategy attempts placement up to 2000 times before failure
 *   - On failure, system degrades to next lower strategy with compensation:
 *     * GREAT → FAIR: Grants population compensation for lost quality
 *     * FAIR → RANDOM: Grants population compensation for lost quality
 *     * RANDOM → OOPS: Recursive placement attempt with emergency parameters
 *     * OOPS: Reports major error - indicates severe map constraints
 *
 * Testing Notes:
 *   Category: C (System) - Requires full game state initialization
 *   Approach: System testing with various map configurations and nation settings
 *   Key Tests: [Placement success rates, territory allocation, army distribution, failure recovery]
 *   Dependencies: [Global game state, map data, terrain functions, curses display, nation arrays]
 *   Mock Requirements: [Complete game world with various terrain types and existing nations]
 *   Complexity: Complex - Full system integration with multiple subsystems and failure handling
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to global state modifications
 *   - Performance: O(n) where n is placement attempts, can be expensive on crowded maps
 *   - Memory safety: Uses global arrays, assumes proper bounds in supporting functions
 *   - Historical context: Core game mechanic determining nation starting conditions
 *   - Critical for game balance: Placement quality directly affects nation viability
 */
void
place (
    int xloc,
    int yloc	/* if not -1,-1 should place in this spot */
)
{
	int	placed=0,armysize=100;
	short	armynum=0;
	long	people=0;
	char tempo[LINELTH+1];
	int	x=0,y=0,i=0,j=0,temp=0,t=0;
	int	n=0, leadtype=0;
	long	soldsleft=0;	/* soldiers left to place */

	if( xloc != -1 && yloc != -1 && is_habitable(xloc,yloc)) {
		placed=1;
		x = xloc;
		y = yloc;
	}

	switch(curntn->location) {
	case OOPS:
		while((placed == 0)&&(n++<2000)){
			if(ispc(curntn->active)){
				x = (rand()%(MAPX-8))+4;
				y = (rand()%(MAPY-8))+4;
			} else {
				x = (rand()%(MAPX-2))+1;
				y = (rand()%(MAPY-2))+1;
			}
			if(is_habitable(x,y)) placed=1;

			for(i=x-1;i<=x+1;i++) for(j=y-1;j<=y+1;j++)
				if(sct[i][j].owner!=0) placed=0;
			temp=0;
			for(i=x-1;i<=x+1;i++) for(j=y-1;j<=y+1;j++)
				if(sct[i][j].altitude==WATER) temp++;
			if(temp>=7) placed=0;
		}
		if(placed) teraform( x,y,1,25 );
		break;
	case RANDOM:
		while ((placed == 0)&&(n++<2000)){
			if(ispc(curntn->active)){
				if(MAPX>12){
					x = rand()%(MAPX-12)+6;
					y = rand()%(MAPY-12)+6;
				} else {
					x = rand()%(MAPX-8)+4;
					y = rand()%(MAPY-8)+4;
				}
				if(is_habitable(x,y)) placed=1;
				/*important that no countries near*/
				for(i=x-2;i<=x+2;i++) for(j=y-2;j<=y+2;j++)
					if((isntn(ntn[sct[i][j].owner].active))
					&&(sct[i][j].owner!=0)) placed=0;
			} else {
				x = (rand()%(MAPX-6))+3;
				y = (rand()%(MAPY-6))+3;
				if(is_habitable(x,y)) placed=1;
				/*important that no countries near*/
				for(i=x-2;i<=x+2;i++) for(j=y-2;j<=y+2;j++)
					if((isntn(ntn[sct[i][j].owner].active))
					&&(sct[i][j].owner!=0)) placed=0;
			}
			temp=0;
			for(i=x-1;i<=x+1;i++) for(j=y-1;j<=y+1;j++)
				if(sct[i][j].altitude==WATER) temp++;
			if(temp>=7) placed=0;
			for(i=x-1;i<=x+1;i++) for(j=y-1;j<=y+1;j++)
				if(sct[i][j].owner!=0) placed=0;
		}
		if(placed) teraform( x,y,1,40 );
		break;
	case FAIR:
		while ((placed == 0)&&(n++<2000)) {
			if(ispc(curntn->active)){
				if(MAPX>24) {
					x = rand()%(MAPX-24)+12;
				} else {
					x = rand()%(MAPX-14)+7;
				}
				if(MAPY>24) {
					y = rand()%(MAPY-24)+12;
				} else {
					y = rand()%(MAPY-14)+7;
				}
			} else {
				x = rand()%(MAPX-10)+5;
				y = rand()%(MAPY-10)+5;
			}

			if(!is_habitable(x,y)) continue;
			if(tofood( &sct[x][y],country)<DESFOOD) continue;

			placed=1;
			for(i=x-1;i<=x+1;i++) for(j=y-1;j<=y+1;j++)
				if(sct[i][j].owner!=0) placed=0;

			if(pwater>50) {
				temp=0;
				for(i=x-1;i<=x+1;i++) for(j=y-1;j<=y+1;j++)
				if(sct[i][j].altitude==WATER) temp++;
				if(temp>=7) placed=0;

				/*important that no countries near*/
				for(i=x-3;i<=x+3;i++) for(j=y-3;j<=y+3;j++){
				if((isntn(ntn[sct[i][j].owner].active))
					&&(sct[i][j].owner!=0)) placed=0;
				}
			} else {
				temp=0;
				for(i=x-1;i<=x+1;i++) for(j=y-1;j<=y+1;j++)
					if(sct[i][j].altitude==WATER) temp++;
				if(temp>=5) placed=0;

				/*important that no countries near*/
				for(i=x-3;i<=x+3;i++) for(j=y-3;j<=y+3;j++){
				if((isntn(ntn[sct[i][j].owner].active))
				&&(sct[i][j].owner!=0)) placed=0;
				}
			}
		}

		if(placed) teraform( x,y,1,65 );
		break;
	case GREAT:
		placed = 0;
		while ((placed == 0) && (n++<2000)){
			if(ispc(curntn->active)){
				if (MAPX>40){
					x = rand()%(MAPX-40)+20;
				}else{
					x = rand()%(MAPX-18)+9;
				}
				if (MAPY>40){
					y = rand()%(MAPY-40)+20;
				}else{
					y = rand()%(MAPY-18)+9;
				}

				if(is_habitable(x,y)) placed=1;
				/*important that no countries near*/
				for(i=x-4;i<=x+4;i++) for(j=y-4;j<=y+4;j++){
				if((isntn(ntn[sct[i][j].owner].active))
				&&( sct[i][j].owner!=0)) placed=0;
				}
			} else {
				if(MAPX>24){
					x = rand()%(MAPX-24)+12;
				}else {
					x = rand()%(MAPX-12)+6;
				}
				if(MAPY>24){
					y = rand()%(MAPY-24)+12;
				}else {
					y = rand()%(MAPY-12)+6;
				}
				if(is_habitable(x,y)) placed=1;
				/*important that no countries near*/
				for(i=x-4;i<=x+4;i++) for(j=y-4;j<=y+4;j++){
				if((isntn(ntn[sct[i][j].owner].active))
					&&(sct[i][j].owner!=0)) placed=0;
				}
			}

			for(i=x-1;i<=x+1;i++) for(j=y-1;j<=y+1;j++)
				if(sct[i][j].owner!=0) placed=0;

			temp=0;
			/*if any water within 2 sectors placed = 0*/
			for(i=x-2;i<=x+2;i++) for(j=y-2;j<=y+2;j++)
				if(tofood( &sct[x][y],country)<=0)
					temp++;

			if( pwater>50 ) {
				if(temp>=18) placed=0;
			} else {
				if(temp>=15) placed=0;
			}
		}
		if(placed) teraform( x,y,1,100 );
	}

	/*done with one try*/
	if(placed==1) {
		curntn->capx = safe_int_to_uchar(x);
		curntn->capy = safe_int_to_uchar(y);
		sct[x][y].designation=DCAPITOL;
		sct[x][y].tradegood=safe_int_to_uchar(rand()%(END_KNOWLEDGE-END_SPOILRATE)+END_SPOILRATE+1);
		sct[x][y].jewels=0;
		sct[x][y].metal=0;
		sct[x][y].owner=safe_int_to_uchar(country);
		sct[x][y].people=curntn->tciv;
		sct[x][y].fortress=5;

		/* put all military into armies of armysize */
		armysize = safe_long_to_int((TAKESECTOR*12)/10);
		if(armysize<100) armysize=100;
		/* cant have more than 50% leaders */
		if( MAXARM < numleaders * 2 ) numleaders = MAXARM / 2;
		armynum=0;
		soldsleft = curntn->tmil;
		P_ASOLD = curntn->tmil/MILINCAP;
		soldsleft-=P_ASOLD;
		P_ATYPE=safe_long_to_uchar(defaultunit(country));
		P_ASTAT=GARRISON;
		P_AMOVE=0;
		P_AXLOC=curntn->capx;
		P_AYLOC=curntn->capy;
		armynum++;

		armysize = safe_long_to_int(max( armysize, soldsleft  / (MAXARM-numleaders-1)));

		/* give you your leaders */
		leadtype = getleader(curntn->class);
		P_ATYPE = safe_int_to_uchar(leadtype-1);	/* This is the national leader */
		P_ASOLD = *(unitminsth+((leadtype-1)%UTYPE));
		P_AXLOC = curntn->capx;
		P_AYLOC = curntn->capy;
		P_ASTAT = DEFEND;
		P_AMOVE = 2*curntn->maxmove;
		armynum++;
		numleaders--;
		while ((armynum < MAXARM)&&(numleaders>0)) {
			P_ATYPE=safe_int_to_uchar(leadtype);
			P_ASOLD= *(unitminsth+(leadtype%UTYPE));
			P_AXLOC=curntn->capx;
			P_AYLOC=curntn->capy;
			P_ASTAT=DEFEND;
			P_AMOVE=2*curntn->maxmove;
			armynum++;
			numleaders--;
		}

		/* give you the rest of your armies */
		while((armynum < MAXARM)&&(soldsleft >0)) {
			P_ATYPE=safe_long_to_uchar(defaultunit(country));
			if(soldsleft >= armysize){
				P_ASOLD=armysize;
				soldsleft -=armysize;
			} else {
				P_ASOLD=soldsleft ;
				soldsleft=0;
			}
			P_AXLOC=curntn->capx;
			P_AYLOC=curntn->capy;
			P_ASTAT=DEFEND;
			P_AMOVE=curntn->maxmove;
			armynum++;
		}

		if(soldsleft >0) {
			curntn->arm[0].sold += soldsleft;
			curntn->arm[0].unittyp = A_INFANTRY;
		}

		/* give you some terain to start with: pc nations get more*/
		if (isnotpc(curntn->active)) t=1;
		else if (curntn->location==OOPS) t=0;
		else if (curntn->location==RANDOM) t=0;
		else if (curntn->location==FAIR) t=1;
		else if (curntn->location==GREAT) t=2;
		else {
			if (ispc(curntn->active))
			newerror("Error in finding placement");
			t=0;
		}
		if( t==1 )
			people = sct[x][y].people / 12;
		else if( t==2 )
			people = sct[x][y].people / 30;

		curntn->tsctrs=1;
		for(i=x-t;i<=x+t;i++) for(j=y-t;j<=y+t;j++)
			if((tofood( &sct[i][j],country)>=DESFOOD)
			&&(sct[i][j].owner==0)
			&&(is_habitable(i,j)==TRUE)
			&&(sct[i][j].people==0)) {
				curntn->tsctrs++;
				sct[i][j].owner=safe_int_to_uchar(country);
				sct[i][j].designation=DFARM;
				sct[i][j].people=people;
				sct[x][y].people-=people;
			}
		/* make sure status is set properly */
		for(i=0;i<NTOTAL;i++) {
			if (ntn[i].active < NPC_PEASANT) {
				ntn[i].dstatus[country]=UNMET;
				ntn[country].dstatus[i]=UNMET;
			} else {
				ntn[i].dstatus[country]=WAR;
				ntn[country].dstatus[i]=WAR;
			}
		}
	}
	else {
		if(curntn->location==OOPS) {
			newerror("MAJOR ERROR IN PLACEMENT");
		} else if(curntn->location==RANDOM) {
			newerror("Random Place Failed, trying to place again");
			curntn->location=OOPS;
			place(-1,-1);
		} else if(curntn->location==FAIR) {
			sprintf(tempo,"Fair Place Failed, trying again - Adding %ld people to nation",Munits[CH_PEOPLE]*Mvalues[CH_PEOPLE]/Mcost[CH_PEOPLE]);
			newerror(tempo);
			/*give back one point -> NLPOP people*/
			curntn->tciv += Munits[CH_PEOPLE] * Mvalues[CH_PEOPLE]
				/ Mcost[CH_PEOPLE];
			curntn->location=RANDOM;
			place(-1,-1);
		} else if(curntn->location==GREAT) {
			sprintf(tempo,"Great Place Failed, trying again - Adding %ld people to nation",Munits[CH_PEOPLE]*Mvalues[CH_PEOPLE]/Mcost[CH_PEOPLE]);
			newerror(tempo);
			/*give back one point -> NLPOP people*/
			curntn->tciv+= Munits[CH_PEOPLE] * Mvalues[CH_PEOPLE]
				/ Mcost[CH_PEOPLE];
			curntn->location=FAIR;
			place(-1,-1);
		}
	}
}

/*
 * getclass - Interactive class selection interface for nation creation
 *
 * Presents a curses-based menu system allowing players to select their nation's
 * class from available options based on their chosen race. Each class provides
 * different magical abilities, leader counts, and costs, fundamentally affecting
 * gameplay strategy and nation capabilities.
 *
 * The function displays a formatted table showing class names, racial restrictions,
 * magical powers, and costs. It validates user selection against race compatibility
 * and handles special pricing for race/class combinations (e.g., Human Warlords
 * receive a cost discount).
 *
 * Parameters:
 *   race - The player's chosen race (DWARF, ELF, HUMAN, ORC)
 *          Used to filter available classes and apply race-specific bonuses
 *
 * Returns:
 *   int - The cost in points required for the selected class
 *         This value is returned from doclass() after configuration
 *
 * Side Effects:
 *   - Displays interactive curses menu with class options and details
 *   - Modifies current nation's class (curntn->class)
 *   - Calls doclass() which configures class powers and leader counts
 *   - Updates screen with formatted class table and selection prompt
 *   - Clears display area after selection is made
 *   - May display error messages for invalid selections
 *
 * Display Format:
 *   Shows formatted table with columns:
 *   - class: Class name (e.g., "TRADER", "WIZARD", "WARLORD")
 *   - who: Racial eligibility codes (D=Dwarf, E=Elf, H=Human, O=Orc)
 *   - magic: Magical abilities granted (displayed with dotted formatting)
 *   - cost: Point cost (with Human Warlord discount: 2/3 normal cost)
 *
 * Class System Integration:
 *   - Filters available classes using in_str() for race compatibility
 *   - Applies special Human Warlord pricing (2/3 cost reduction)
 *   - Displays magical powers from CPowlist array with formatted alignment
 *   - Validates selection range (1 to NUMCLASS) and race eligibility
 *   - Integrates with doclass() for power assignment and leader configuration
 *
 * Input Validation:
 *   - Checks selection is within valid class number range (1-NUMCLASS)
 *   - Verifies selected class is available for player's race
 *   - Continues prompt loop until valid selection is made
 *   - Displays appropriate error messages for invalid choices
 *
 * Screen Management:
 *   - Uses curses positioning starting at line 4
 *   - Dynamically tracks ypos for proper line spacing
 *   - Clears input area after each attempt
 *   - Cleans up display area after successful selection
 *   - Refreshes screen for user input visibility
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires curses display and class configuration systems
 *   Approach: Integration testing with various race/class combinations
 *   Key Tests: [Race eligibility filtering, special pricing, input validation, display formatting]
 *   Dependencies: [Curses display system, class configuration arrays, doclass() function]
 *   Mock Requirements: [Curses environment, class data arrays, user input simulation]
 *   Complexity: Moderate - Interactive menu with validation and formatting requirements
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to curses display and global state access
 *   - Performance: Interactive function, speed not critical
 *   - Memory safety: Uses global arrays, string length calculations for formatting
 *   - Historical context: Core character creation mechanic affecting gameplay balance
 *   - User experience: Critical for player understanding of class capabilities and costs
 */
/*get class routine*/
/* return the number of points needed */
int
getclass (int race)
{
	short chk=FALSE;
	short tmp=0;
	short ypos=4;
	int i,j;

	mvaddstr(ypos,0,"The List of Possible Nation Classes:");
	ypos+=2;
	mvprintw(ypos++,0,"     %-8s %4s   %15s %8s %4s", "class", "who",
		"", "magic", "cost");
	mvprintw(ypos++,0,"     %-8s %4s   %15s %8s %4s", "--------", "----",
		"", "-------", "----");
	for(i=1;i<NUMCLASS;i++) {
		if (in_str(race,Classwho[i])==TRUE) {
			mvprintw(ypos++,0," %2d) %-8s %4s %15s", i, Class[i],
				Classwho[i], "...............");
			tmp = safe_int_to_short(safe_size_to_int(strlen(CPowlist[i])));
			for(j=0; j < 10-tmp ; j++) {
				addch('.');
			}
			printw(" %s", CPowlist[i]);
			if (i == C_WARLORD && race == HUMAN)
				printw(" %4d", Classcost[i]*2/3);
			else printw(" %4d", Classcost[i]);
		}
	}
	ypos++;
	while(chk==FALSE){
		mvaddstr(ypos,0,"Enter the number of your choice: ");
		clrtoeol();
		refresh();
		tmp = safe_long_to_short(get_number());
		if (tmp < 1 || tmp > NUMCLASS) {
			newerror("Invalid Choice");
		} else if (in_str(race,Classwho[tmp])==TRUE) {
			chk = TRUE;
		} else {
			newerror("That Class is Invalid for your Race");
		}
	}
	curntn->class=tmp;
	for(tmp=ypos; tmp>4; tmp--) {
		move(tmp,0);
		clrtoeol();
	}

	return( doclass( curntn->class, TRUE ) );
}

/*
 * doclass - Configure nation class powers and determine point cost
 *
 * This function applies the selected nation class configuration to the current
 * nation, assigning magical powers, determining leader counts, and calculating
 * the final point cost. It handles special cases like Human Warlord bonuses
 * and integrates with the magic system for power updates.
 *
 * The function implements the core class system logic, translating class
 * selections into concrete gameplay effects including magical abilities,
 * military leadership capacity, and resource costs.
 *
 * Parameters:
 *   tmp - The selected class ID (e.g., C_TRADER, C_WIZARD, C_WARLORD)
 *         Used to look up class-specific powers, costs, and leader counts
 *   isupd - Update flag: TRUE for interactive mode with magic system updates,
 *           FALSE for non-interactive calculation only
 *
 * Returns:
 *   int - The final point cost for the selected class after any modifiers
 *         (e.g., Human Warlord receives 2/3 cost reduction)
 *
 * Side Effects:
 *   - Sets global numleaders and spent[CH_LEADERS] based on class type
 *   - Modifies current nation's powers (curntn->powers) with class abilities
 *   - Calls CHGMGK macro if isupd is TRUE to update magic system
 *   - Updates global spent array to track leader allocation
 *
 * Class-Specific Logic:
 *   Leader Count Assignment:
 *   - Trader class and classes <= C_WIZARD: 5 leaders
 *   - All other classes (including military classes): 7 leaders
 *   - Leaders directly affect army command capacity and strategic options
 *
 *   Power Assignment:
 *   - Retrieves base powers from Classpow[tmp] lookup table
 *   - Powers are applied using bitwise OR to curntn->powers
 *   - Each power bit represents a different magical or special ability
 *
 *   Special Case Handling:
 *   - Human Warlord Bonus: Toggles WARRIOR power bit and reduces cost to 2/3
 *   - Cost reduction reflects Human affinity for military leadership
 *   - XOR operation (^=) toggles the WARRIOR power specifically
 *
 * Magic System Integration:
 *   - CHGMGK macro called when isupd is TRUE
 *   - Updates magic system to reflect new nation powers
 *   - Essential for maintaining consistency in interactive mode
 *   - Skipped in calculation-only mode for efficiency
 *
 * Cost Calculation:
 *   - Base cost retrieved from Classcost[tmp] array
 *   - Human Warlord special case: cost = (Classcost[tmp] * 2) / 3
 *   - Cost represents point expenditure in nation creation system
 *   - Returned value used by getclass() and newlogin() for budgeting
 *
 * Testing Notes:
 *   Category: A (Unit) - Isolated logic with clear inputs and outputs
 *   Approach: Unit testing with various class/race combinations
 *   Key Tests: [Leader count assignment, power configuration, cost calculation, Human Warlord special case]
 *   Dependencies: [Class configuration arrays, current nation structure, magic system macros]
 *   Mock Requirements: [Class data arrays, nation structure, CHGMGK macro]
 *   Complexity: Simple - Straightforward configuration logic with minimal branching
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to global variable modifications
 *   - Performance: Very fast, simple lookup and assignment operations
 *   - Memory safety: Uses array indexing, assumes valid tmp parameter
 *   - Historical context: Core RPG class system determining nation capabilities
 *   - Design pattern: Configuration function separating UI from logic
 */
int
doclass (
    int tmp,
    int isupd	/* true if update, false if interactive */
)
{
	int cost;
	long x;

	/* determine number of leaders you want */
	if((tmp == C_TRADER) || (tmp <= C_WIZARD)) {
		numleaders = spent[CH_LEADERS] = 5;
	} else {
		numleaders = spent[CH_LEADERS] = 7;
	}

	/* assign the powers */
	x=Classpow[tmp];

	/* check for special case */
	if (tmp==C_WARLORD && curntn->race==HUMAN) {
		x ^= WARRIOR;
		cost = Classcost[tmp]*2/3;
	} else {
		cost = Classcost[tmp];
	}
	curntn->powers |= x;

	if ( isupd ) CHGMGK;
	return(cost);
}

/*
 * nstartcst - Calculate total point cost for new nation creation method
 *
 * This function implements the "new method" for calculating nation creation
 * point costs based on the modern spending allocation system. It totals the
 * costs of all allocated resources using the current spending arrays and
 * applies late-start bonuses for players joining ongoing games.
 *
 * The function provides a more modular and flexible cost calculation compared
 * to the legacy startcost() method, using configurable cost tables and
 * standardized calculation formulas across all nation attributes.
 *
 * Parameters:
 *   None - Uses global spent[] array and configuration tables
 *
 * Returns:
 *   int - Total point cost for the nation configuration, rounded up
 *         Includes late-start bonus calculations and base adjustments
 *
 * Side Effects:
 *   - May display error message showing late-start bonus information
 *   - Uses newerror() to communicate bonus details to player
 *   - Accesses global configuration arrays and game state
 *
 * Cost Calculation Algorithm:
 *   Base Cost Calculation:
 *   - Iterates through all allocation categories (CH_NUMBER total)
 *   - For each category: cost += Mcost[i] * spent[i] / Munits[i]
 *   - Uses floating-point arithmetic for precise calculations
 *   - Handles fractional allocations and scaling factors
 *
 *   Late-Start Bonus System:
 *   - Bonus = (TURN - 1) / LATESTART
 *   - Reduces total cost for players joining mid-game
 *   - Compensates for missed early-game development opportunities
 *   - Displays bonus amount if applicable using formatted message
 *
 *   Final Processing:
 *   - Adds 1.0 for rounding up to ensure minimum cost coverage
 *   - Converts final floating-point result to integer
 *   - Ensures consistent point allocation across all players
 *
 * Configuration Dependencies:
 *   - spent[]: Current resource allocation array (CH_NUMBER elements)
 *   - Mcost[]: Point cost per unit for each resource category
 *   - Munits[]: Unit scaling factors for cost calculations
 *   - TURN: Current game turn number
 *   - LATESTART: Late-start bonus divisor constant
 *
 * Comparison with Legacy Method:
 *   - More modular: Uses configuration tables instead of hardcoded values
 *   - More flexible: Handles arbitrary allocation categories
 *   - More precise: Floating-point calculations with proper rounding
 *   - More maintainable: Centralized cost configuration
 *   - Same late-start logic: Maintains game balance compatibility
 *
 * Testing Notes:
 *   Category: A (Unit) - Isolated calculation with clear dependencies
 *   Approach: Unit testing with various allocation scenarios and game turns
 *   Key Tests: [Cost calculation accuracy, late-start bonus, rounding behavior, edge cases]
 *   Dependencies: [Global spent array, cost configuration tables, game turn state]
 *   Mock Requirements: [Allocation arrays, cost tables, turn counter]
 *   Complexity: Simple - Straightforward mathematical calculation with minimal branching
 *
 * Notes:
 *   - Thread safety: Read-only access to global state, generally safe
 *   - Performance: O(CH_NUMBER) iteration, very fast
 *   - Memory safety: Uses array bounds from CH_NUMBER constant
 *   - Historical context: Modern evolution of nation creation cost system
 *   - Design pattern: Data-driven calculation using configuration tables
 */
int nstartcst (void)	/* to be used for new method */
{
	float points=0.0f;
	char temp[LINELTH];
	int i;
	float tmpx = 0.0f;

	/* calculate cost for all so far */
	for (i=0; i<CH_NUMBER; i++) {
		points += safe_int_to_float(Mcost[i]) * safe_int_to_float(spent[i]) / safe_int_to_float(Munits[i]);
	}

	/* extra points for starting late */
	tmpx = safe_int_to_float(TURN-1) / LATESTART;
	points -= tmpx;
	if( tmpx > 0.0f ) {
		sprintf(temp,"%.1f points added for starting late", tmpx);
		newerror(temp);
	}
	points += 1.0f;	/* round up */
	return((int)points);
}

/*
 * startcost - Calculate total point cost using legacy nation creation method
 *
 * This function implements the original "legacy method" for calculating nation
 * creation point costs by directly examining final nation attributes. It uses
 * hardcoded cost constants and race-specific formulas to determine the total
 * point expenditure required for the current nation configuration.
 *
 * Unlike the modern nstartcst() method which uses allocation arrays, this
 * function calculates costs from the final nation state, making it suitable
 * for validation and legacy compatibility but not for NPC nation generation.
 *
 * Parameters:
 *   None - Uses current nation (curntn) attributes and global constants
 *
 * Returns:
 *   int - Total point cost for the nation configuration, rounded up
 *         Includes race-specific calculations and late-start bonuses
 *
 * Side Effects:
 *   - Prints detailed cost breakdown to stdout if TURN > 1
 *   - Shows point cost and late-start bonus for debugging/validation
 *   - Accesses global nation state and game turn information
 *
 * Cost Calculation Components:
 *   Population Cost:
 *   - points += curntn->tciv / ONLPOP
 *   - Direct conversion of civilian population to point cost
 *
 *   Economic Cost:
 *   - points += curntn->tgold / ONLGOLD
 *   - Treasury size converted to point equivalent
 *
 *   Military Cost:
 *   - points += curntn->tmil / ONLSOLD
 *   - Standing army size converted to point cost
 *
 *   Race-Specific Attribute Costs:
 *   ORC Nations:
 *   - Reproduction: curntn->repro * ONLREPCOST / ONLREPRO_ORC
 *   - Attack bonus: curntn->aplus * 2 / ONLATTACK (double weighting)
 *   - Defense bonus: curntn->dplus * 2 / ONLDEFENCE (double weighting)
 *
 *   Non-ORC Nations:
 *   - Attack bonus: curntn->aplus / ONLATTACK
 *   - Defense bonus: curntn->dplus / ONLDEFENCE
 *   - Reproduction: curntn->repro * ONLREPCOST / ONLREPRO
 *
 *   Movement Cost:
 *   - points += curntn->maxmove / ONLMOVE
 *   - Maximum movement range converted to point cost
 *
 *   Location Quality Cost:
 *   - FAIR placement: +ONLLOCCOST points
 *   - GREAT placement: +2*ONLLOCCOST points
 *   - RANDOM/OOPS: No additional cost
 *
 * Late-Start Bonus System:
 *   - Bonus = (TURN - 1) / LATESTART
 *   - Identical to nstartcst() for game balance consistency
 *   - Compensates players joining ongoing games
 *   - Shows detailed breakdown when TURN > 1
 *
 * Race-Specific Balance:
 *   - ORC nations get different reproduction and combat cost scaling
 *   - ORC combat bonuses weighted at 2x for balance purposes
 *   - Reflects racial strengths and weaknesses in point economy
 *   - Maintains strategic diversity between race choices
 *
 * NPC Limitation:
 *   - Comment indicates this cannot be used for NPC nations
 *   - Legacy method assumes player-controlled nation attributes
 *   - NPC generation requires different cost calculation approach
 *   - Modern method (nstartcst) preferred for new implementations
 *
 * Testing Notes:
 *   Category: A (Unit) - Direct calculation from nation attributes
 *   Approach: Unit testing with various nation configurations and races
 *   Key Tests: [Race-specific calculations, location costs, late-start bonus, ORC special cases]
 *   Dependencies: [Current nation structure, cost constants, game turn state]
 *   Mock Requirements: [Nation with various attribute combinations, cost constants]
 *   Complexity: Simple - Direct attribute-to-cost conversion with race branching
 *
 * Notes:
 *   - Thread safety: Read-only access to global state, generally safe
 *   - Performance: Very fast, direct calculation with minimal computation
 *   - Memory safety: Uses global nation structure, no array bounds issues
 *   - Historical context: Original nation creation cost system
 *   - Deprecation status: Legacy method, nstartcst() preferred for new code
 */
int
startcost (void)	/* cant be used for npc nations yet!!! see below */
{
	float	points;	/* points */

	points = ((float)curntn->tciv)/ONLPOP;
	points += ((float)curntn->tgold)/ONLGOLD;
	points += ((float)curntn->tmil)/ONLSOLD;
	if(curntn->race==ORC) {
		points += ((float)curntn->repro)*ONLREPCOST/(ONLREPRO_ORC);
		points += ((float)curntn->aplus*2)/ONLATTACK;
		points += ((float)curntn->dplus*2)/ONLDEFENCE;
	} else {
		points += ((float)curntn->aplus)/ONLATTACK;
		points += ((float)curntn->dplus)/ONLDEFENCE;
		points += ((float)curntn->repro)*ONLREPCOST/ONLREPRO;
	}
	points += ((float)curntn->maxmove)/ONLMOVE;
	if(curntn->location==FAIR)
		points += ONLLOCCOST;
	else if(curntn->location==GREAT)
		points += 2*ONLLOCCOST;
	/* points+=ONLDBLCOST*curntn->tfood/ONLHFOOD; */
	points -= safe_int_to_float((TURN-1) / LATESTART);	/* extra points if you start late */
	if( TURN > 1 )
	printf("point cost for nation %d is %.2f (bonus for latestart is %f)\n",country,points,(float) (TURN-1)/LATESTART);

	points += 1.0f;	/* round up */
	return((int) points);
}
