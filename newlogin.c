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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <ctype.h>
#include <pwd.h>
#include "patchlevel.h"
#include "header.h"
#include "data.h"
#include "newlogin.h"

/* information about national classes */
char *Classwho[]= { "DEHO", "DEH", "DEH", "EH", "H", "DHO", "HE", "DHO",
	"O", "O", "O" };
char *CPowlist[]= { "None", "None", "None", "SUMMON", "RELIGION", "SAILOR",
	"URBAN", "WARLORD", "DESTROYER", "MA_MONST", "THE_VOID"};
int  Classcost[]= { 0, 0, 0, 4, 2, 2, 2, 6, 4, 4, 2 };
long Classpow[]= { 0x0L, 0x0L, 0x0L, SUMMON, RELIGION, SAILOR,
	URBAN, 0x000000007L, DESTROYER, 0x00000700L, THE_VOID };

char *Mprompt[]= { "<ADD", "SUB>" };
char *LType[]={ "Random", "Fair", "Great" };

char *Mlabels[]= { "Population", "Treasury", "Location",
	"Military", "Attack Bonus", "Defense Bonus", "Reproduction",
	"Movement", "Magic Powers", "Leaders", "Raw Materials" };

char *Mitems[]= { "people", "gold talons", "location", "soldiers",
	"percent", "percent", "percent", "move points", "powers",
	"nation leaders", "units of food" };

char *Mhelp[]= { "Population: Amount of citizens in your nation",
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
extern FILE *fexe, *fopen();
extern short country;
int	numleaders;
int spent[CH_NUMBER];

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
teraform( x,y,range, chance )
int x,y;
int range,chance;
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
mailtopc(string)
char	*string;
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
newinit()
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
newreset()
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
newbye(status)
	int status;
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
newmsg(str)
	char *str;
{
	mvaddstr(LINES-1,0,str);
	clrtoeol();
	refresh();
}

/* message with wait for keystroke */
void
newerror(str)
	char *str;
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

/* function to check if a character is in a character array */
int
in_str(ch,str)
	char ch, *str;
{
	int i,l=strlen(str);

	for(i=0;i<l;i++)
		if (ch == str[i]) return(TRUE);
	return(FALSE);
}

/* function to display the comment and error window */
void
errorbar(str1,str2)
	char *str1,*str2;
{
	int i;
	move(LINES-4,0);
	standout();
	for(i=0;i<COLS-1;i++)
		addch(' ');
	standend();
	mvprintw(LINES-3,0," Conquer %s.%s: %s",VERSION,PATCHLEVEL,str1);
	mvaddstr(LINES-3,COLS-strlen(str2)-2,str2);
	move(LINES-2,0);
	for(i=0;i<COLS-1;i++)
		addch('-');
}

/* display amount string at current location */
void
dispitem(item, amount)
	int item;
	long amount;
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
	printw(", %ld jewels", (long) (amount *
		((float)NLJEWELS/Mvalues[CH_RAWGOODS])));
	printw(", and %ld metal.", (long) (amount *
		((float)NLMETAL/Mvalues[CH_RAWGOODS])));
}

/* show the current amount for country item */
void
showitem(line,item)
	int line, item;
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

/* convert the stored information into the nation statistics */
void
convert()
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
	curntn->aplus = (short) Mvalues[CH_ATTACK] * spent[CH_ATTACK];
	curntn->dplus = (short) Mvalues[CH_DEFEND] * spent[CH_DEFEND];
	curntn->repro = (char) Mvalues[CH_REPRO] * spent[CH_REPRO];
	curntn->maxmove = (unsigned char) Mvalues[CH_MOVEMENT]
		* spent[CH_MOVEMENT];
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

void
newlogin(realuser)
  int realuser;
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
			country=i;
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
			sprintf(tempc,"Error opening <%s>",tempc);
			newerror(tempc);
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
		mvprintw(0,COLS/2-15-strlen(curntn->name)/2,
			"< Building Country %s >",curntn->name,country);
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
		strncpy(curntn->passwd,crypt(tempc,SALT),PASSLTH+1);

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
			tempc[0] = getch();
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
			curntn->uid = realuser;
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
void
place(xloc,yloc)
int	xloc,yloc;	/* if not -1,-1 should place in this spot */
{
	int	placed=0,armysize=100;
	short	armynum=0;
	long	people;
	char tempo[LINELTH+1];
	int	x,y,i,j,temp,t;
	int	n=0, leadtype;
	long	soldsleft;	/* soldiers left to place */

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
		curntn->capx = x;
		curntn->capy = y;
		sct[x][y].designation=DCAPITOL;
		sct[x][y].tradegood=rand()%(END_KNOWLEDGE-END_SPOILRATE)+END_SPOILRATE+1;
		sct[x][y].jewels=0;
		sct[x][y].metal=0;
		sct[x][y].owner=country;
		sct[x][y].people=curntn->tciv;
		sct[x][y].fortress=5;

		/* put all military into armies of armysize */
		armysize = (TAKESECTOR*12)/10;
		if(armysize<100) armysize=100;
		/* cant have more than 50% leaders */
		if( MAXARM < numleaders * 2 ) numleaders = MAXARM / 2;
		armynum=0;
		soldsleft = curntn->tmil;
		P_ASOLD = curntn->tmil/MILINCAP;
		soldsleft-=P_ASOLD;
		P_ATYPE=defaultunit(country);
		P_ASTAT=GARRISON;
		P_AMOVE=0;
		P_AXLOC=curntn->capx;
		P_AYLOC=curntn->capy;
		armynum++;

		armysize = max( armysize, soldsleft  / (MAXARM-numleaders-1));

		/* give you your leaders */
		leadtype = getleader(curntn->class);
		P_ATYPE = leadtype-1;	/* This is the national leader */
		P_ASOLD = *(unitminsth+((leadtype-1)%UTYPE));
		P_AXLOC = curntn->capx;
		P_AYLOC = curntn->capy;
		P_ASTAT = DEFEND;
		P_AMOVE = 2*curntn->maxmove;
		armynum++;
		numleaders--;
		while ((armynum < MAXARM)&&(numleaders>0)) {
			P_ATYPE=leadtype;
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
			P_ATYPE=defaultunit(country);
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
				sct[i][j].owner=country;
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

/*get class routine*/
/* return the number of points needed */
int
getclass(race)
	int race;
{
	short chk=FALSE;
	short tmp;
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
			tmp = strlen(CPowlist[i]);
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
		tmp = get_number();
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

int
doclass( tmp, isupd )
short	tmp;
int	isupd;	/* true if update, false if interactive */
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

int
nstartcst()	/* to be used for new method */
{
	float points=0.0;
	char temp[LINELTH];
	int i;

	/* calculate cost for all so far */
	for (i=0; i<CH_NUMBER; i++) {
		points += Mcost[i] * (float) spent[i] / Munits[i];
	}

	/* extra points for starting late */
	points -= (float) (TURN-1) / LATESTART;
	if( (float)(TURN-1)/LATESTART > 0.0 ) {
		sprintf(temp,"%.1f points added for starting late",
			   (float) (TURN-1) / LATESTART);
		newerror(temp);
	}
	points += 1.0;	/* round up */	
	return((int)points);
}

int
startcost()	/* cant be used for npc nations yet!!! see below */
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
	points -= (TURN-1) / LATESTART;	/* extra points if you start late */
	if( TURN > 1 )
	printf("point cost for nation %d is %.2f (bonus for latestart is %f)\n",country,points,(float) (TURN-1)/LATESTART);

	points += 1.0;	/* round up */
	return((int) points);
}
