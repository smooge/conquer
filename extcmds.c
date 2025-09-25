/*
 * extcmds.c - Extended army command system and tactical unit management
 *
 * Advanced army management system providing complex tactical operations
 * beyond basic movement. Handles army splitting, combining, status changes,
 * group formations, and specialized tactical commands. Critical component
 * for strategic gameplay and army coordination.
 *
 * KEY SYSTEMS:
 * - Extended command menu and user interface (ext_cmd)
 * - Army combination and splitting operations
 * - Status modification with comprehensive validation
 * - Group formation and leadership hierarchy
 * - Error handling and user feedback
 * - Screen management for interactive operations
 *
 * ARMY OPERATIONS:
 * The system supports complex army management including:
 * - Splitting armies into smaller units (minimum 25 soldiers)
 * - Combining compatible armies into larger forces
 * - Changing army statuses with tactical implications
 * - Creating group formations under leader command
 * - Context-sensitive commands based on location and unit type
 *
 * TACTICAL FEATURES:
 * - Fortress operations (siege, sortie, garrison, rule)
 * - Leadership hierarchy with general/follower relationships
 * - Movement coordination for grouped units
 * - Special unit handling (leaders, monsters, militia, zombies)
 * - Status-based restrictions to maintain game balance
 *
 * USER INTERFACE:
 * Interactive curses-based interface with context menus, error messages,
 * input validation, and visual feedback. Provides consistent user experience
 * across all extended command operations.
 *
 * VALIDATION SYSTEM:
 * Comprehensive validation ensures game rule compliance:
 * - Unit type compatibility checks
 * - Location-based requirement validation
 * - Status transition rule enforcement
 * - Resource and capacity limit checking
 * - Error reporting with specific feedback
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curses.h>
#include "header.h"
#include "data.h"
#include "safe_convert.h"

extern FILE *fexe;
extern short selector;
extern short country;
extern short xcurs, ycurs, xoffset, yoffset;

/*
 * ext_cmd - Display and process extended army command menu
 *
 * Interactive command dispatcher that presents an extended menu of army
 * management options and processes user input. Handles complex army
 * operations like splitting, combining, status changes, and grouping
 * that go beyond basic movement commands.
 *
 * Parameters:
 *   armie - Army index to operate on (-1 to prompt for selection)
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Displays interactive command menu on screen
 *   - Modifies army data based on user selections
 *   - Updates display with command prompts and feedback
 *   - May create, destroy, or modify army units
 *   - Changes army statuses, positions, and groupings
 *   - Handles screen clearing and cursor management
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires UI system and army data
 *   Approach: Integration testing with mock input and display systems
 *   Key Tests: Menu display, command processing, army operations, error handling
 *   Dependencies: Curses display, army data structures, input system
 *   Mock Requirements: Screen display, keyboard input, army state
 *   Complexity: Moderate - Interactive UI with multiple command branches
 *
 * Notes:
 *   - Context-sensitive menu based on army location and type
 *   - Supports fortress-specific commands (siege, sortie, rule)
 *   - Validates army capabilities before allowing operations
 *   - Uses curses for interactive display and input
 */
void ext_cmd(int armie) {
	int stat=0,armynum,army2,men;

	clear_bottom(0);

	if( armie == -1 ) armynum = getselunit();
	else armynum=armie;

	mvaddstr(LINES-2, 0,
	" (+) combine, (-) split n men, (/) divide by 2, (G)roup");
	mvaddstr(LINES-3, 0,
	" (a)ttack, (d)efend, (s)cout, (m)arch, (g)arrison");
	if(fort_val(&sct[P_AXLOC][P_AYLOC]) > 0) {
		if (sct[P_AXLOC][P_AYLOC].owner==country) addstr(", (S)ortie");
		else addstr(", (S)iege");
		if (P_ATYPE>=MINLEADER && P_ATYPE<MINMONSTER &&
		ISCITY(sct[P_AXLOC][P_AYLOC].designation)) addstr(", (R)ule");
	}
	mvaddstr(LINES-4, 0, "Extended command:");
	refresh();

	switch(getch()) {
	case '-':		/* split army */
		splitarmy(armynum);
		break;
	case '/':		/* divide army */
		men = safe_long_to_int(P_ASOLD/2);
		reducearmy(armynum,men);
		break;
	case '+':		/* combine armies */
		selector +=2;
		army2 = getselunit();
		selector -=2;
		combinearmies(armynum,army2);
		break;
	case 'G':
		addgroup(armynum);
		break;
	case 'a': if(stat==0) stat=ATTACK;
		/* FALLTHROUGH */
	case 'd': if(stat==0) stat=DEFEND;
		/* FALLTHROUGH */
	case 'm': if(stat==0) stat=MARCH;
		/* FALLTHROUGH */
	case 's': if(stat==0) stat=SCOUT;
		/* FALLTHROUGH */
	case 'g': if(stat==0) stat=GARRISON;
		/* FALLTHROUGH */
	case 'R': if(stat==0) stat=RULE;
		/* FALLTHROUGH */
	case 'S':
		if(stat==0) {
			if(sct[P_AXLOC][P_AYLOC].owner==country) stat=SORTIE;
			else stat=SIEGE;
		}
		change_status(armynum,stat);
		break;
	default:		/* unimplemented? */
		errormsg("Unimplemented extended command");
	}
}

/*
 * nocomb_stat - Check if army status prevents combination with other armies
 *
 * Determines whether an army's current status allows it to be combined
 * with other armies. Certain special statuses (flight, magic, onboard, etc.)
 * make armies temporarily uncombinable to preserve game mechanics.
 *
 * Parameters:
 *   astat - Army status code to check for combination eligibility
 *
 * Returns:
 *   TRUE (1) if army status prevents combination
 *   FALSE (0) if army can be combined with others
 *
 * Side Effects:
 *   None - pure function with no state modifications
 *
 * Testing Notes:
 *   Category: A (Unit) - Pure function with no dependencies
 *   Approach: Unit testing with all possible status values
 *   Key Tests: All status codes, boundary conditions, invalid values
 *   Dependencies: None - status constants only
 *   Mock Requirements: None
 *   Complexity: Simple - Status lookup with clear logic
 *
 * Notes:
 *   - Critical for maintaining game balance and mechanics
 *   - Prevents exploitation of special statuses
 *   - TRADE compilation flag affects TRADED status handling
 *   - Pure function suitable for optimization
 */
int nocomb_stat (int astat) {
	int hold;

	switch(astat) {
#ifdef TRADE
	case TRADED:
#endif /* TRADE */
	case FLIGHT:
	case MAGATT:
	case MAGDEF:
	case SCOUT:
	case ONBOARD:
		hold = TRUE;
		break;
	default:
		hold = FALSE;
		break;
	}
	return(hold);
}

/*
 * combinearmies - Merge two compatible armies into a single unit
 *
 * Combines two armies by transferring all soldiers from the second army
 * into the first, removing the second army from active service. Validates
 * compatibility of unit types, statuses, and locations before merging.
 * Updates movement points to reflect the combined unit's capabilities.
 *
 * Parameters:
 *   armynum - Index of primary army to receive combined forces
 *   army2 - Index of secondary army to be absorbed and deactivated
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Increases primary army's soldier count by secondary army's soldiers
 *   - Sets secondary army's soldier count to zero (deactivates it)
 *   - Adjusts primary army's movement points to minimum of both armies
 *   - Updates execution file with army modifications
 *   - Handles special cases for general/grouped army leadership
 *   - May modify leader army movement if groups are involved
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires army data structures and validation
 *   Approach: Integration testing with various army combinations and edge cases
 *   Key Tests: Compatible armies, incompatible types, location mismatches, special statuses
 *   Dependencies: Army data structures, status checking, execution file system
 *   Mock Requirements: Army arrays, nation data, file operations
 *   Complexity: Moderate - Multiple validation steps and state updates
 *
 * Notes:
 *   - Enforces strict compatibility rules for game balance
 *   - Leaders cannot be combined due to their special nature
 *   - Movement calculation uses minimum to prevent exploitation
 *   - Handles complex group leadership scenarios
 *   - Critical for army management efficiency
 */
void combinearmies (int armynum, int army2) {

	if (armynum < 0 || armynum >= MAXARM
	    || army2 < 0 || army2 >= MAXARM || P_ASOLD == 0
	    || curntn->arm[army2].sold == 0) {
		errormsg("Selected unit doesn't exist");
		return;
	}
	if (P_ASTAT != curntn->arm[army2].stat
	    || P_ATYPE != curntn->arm[army2].unittyp
	    || P_ATYPE >= MINLEADER
	    || P_ASTAT == ONBOARD || P_ASTAT == TRADED) {
	  if ((nocomb_stat(P_ASTAT) == TRUE) ||
		(nocomb_stat(curntn->arm[army2].stat) == TRUE) ||
	        curntn->arm[army2].stat == MARCH ||
		curntn->arm[army2].stat == SIEGE ||    /* may not jump out  */
		curntn->arm[army2].stat == SORTIE ||   /* of these statuses */
		P_ATYPE >= MINLEADER ||
		P_ATYPE!=curntn->arm[army2].unittyp) {
			errormsg("Selected armies not legal");
			return;
		}
	}
	if((curntn->arm[army2].xloc!=P_AXLOC)
	||(curntn->arm[army2].yloc!=P_AYLOC)) {
		errormsg("Selected armies not together");
		return;
	}
	P_ASOLD += curntn->arm[army2].sold;
	curntn->arm[army2].sold = 0;
	P_AMOVE=min(P_AMOVE,curntn->arm[army2].smove);
	AADJMEN;
	AADJMOV;
	if (P_ASTAT>NUMSTATUS) {
		curntn->arm[P_ASTAT-NUMSTATUS].smove=P_AMOVE;
		armynum = P_ASTAT-NUMSTATUS;
		AADJMOV;
	}
	armynum = army2;
	AADJMEN;
}

/*
 * change_status - Modify army status with comprehensive validation and effects
 *
 * Changes an army's operational status while enforcing game rules and
 * constraints. Handles complex status transitions, movement penalties,
 * and special requirements for different army types and locations.
 * Critical function for tactical army management.
 *
 * Parameters:
 *   armynum - Index of army to modify
 *   new_stat - New status code to apply to the army
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Changes army status and updates execution file
 *   - May modify army type (scout conversion)
 *   - Adjusts movement points based on status transitions
 *   - Handles group leadership changes for generals
 *   - Validates location requirements for specific statuses
 *   - Applies movement penalties for status changes from MARCH
 *   - Sets movement to zero for location-dependent statuses
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires army data and game state validation
 *   Approach: Integration testing with various status combinations and army types
 *   Key Tests: All status transitions, special unit types, location validation, movement penalties
 *   Dependencies: Army data, sector information, fort validation, execution system
 *   Mock Requirements: Army arrays, sector data, fort calculations
 *   Complexity: Complex - Extensive validation and multiple side effects
 *
 * Notes:
 *   - Enforces strict game rules for status transitions
 *   - Special handling for militia, zombies, and leaders
 *   - Location-dependent statuses require specific sector conditions
 *   - Movement penalty system prevents status change exploitation
 *   - Critical for maintaining game balance and realism
 */
void change_status (int armynum, int new_stat) {
	int	i;
	if(armynum<0 || armynum >= MAXARM || P_ASTAT==SCOUT ||
		P_ASTAT==TRADED || P_ASTAT==ONBOARD || P_ASTAT==SORTIE){
		errormsg("Can't change status on that army");
		return;
	}
	if( P_ATYPE == A_MILITIA && new_stat!=MILITIA ) {
		errormsg("Militia is all you get");
		return;
	}
	if( P_ATYPE == A_ZOMBIE && new_stat==MARCH ) {
		errormsg("Duh?  Your Zombies just lumber about.");
		return;
	}
	if(P_ASTAT == SIEGED && new_stat != SORTIE && new_stat != RULE) {
		errormsg("Sieged troops may only switch to sortie");
		return;
	}
	if(P_ASTAT == new_stat) {
		errormsg("Uh, sir.  Aren't we doing that already?");
		return;
	}
	if(new_stat == SCOUT) {
		if(( P_ATYPE != A_SCOUT )&&( P_ASOLD >= 25)) {
			errormsg("Need less than 25 men to scout");
			return;
		} else {
			P_ATYPE=A_SCOUT;
			AADJMEN;
		}
	}
	if(P_ASTAT == GENERAL ){
		i=armynum;
		for(armynum=0;armynum<MAXARM;armynum++){
			if(( P_ASOLD>0) && (P_ASTAT==i+NUMSTATUS)){
				P_ASTAT = ATTACK;
				AADJSTAT;
			}
		}
		armynum=i;
	}
	if(new_stat == GARRISON) {
		if((sct[P_AXLOC][P_AYLOC].owner!=country)
		||(fort_val(&sct[P_AXLOC][P_AYLOC])==0)) {
			errormsg("Just how do you want to garrison that?");
			return;
		}
	} else if(new_stat == SORTIE) {
		if((sct[P_AXLOC][P_AYLOC].owner!=country)
		||(fort_val(&sct[P_AXLOC][P_AYLOC])==0)) {
			errormsg("Hmmm... How do you figure to sortie from there?");
			return;
		}
	} else if(new_stat == SIEGE) {
		if((sct[P_AXLOC][P_AYLOC].owner==country)
		||(fort_val(&sct[P_AXLOC][P_AYLOC])==0)) {
			errormsg("You want to lay seige to this??");
			return;
		}
	} else if(new_stat == RULE) {
		if((sct[P_AXLOC][P_AYLOC].owner!=country)
		||(!ISCITY(sct[P_AXLOC][P_AYLOC].designation))
		||(P_ATYPE<MINLEADER)
		||(P_ATYPE>=MINMONSTER)) {
			errormsg("Sorry, but you can't rule in that sector");
			return;
		}
	}

	/* eliminate half starting movement if start out on march */
	if( P_ASTAT==MARCH && new_stat!=MARCH ) {
		if(P_AMOVE<(curntn->maxmove * *(unitmove+(P_ATYPE%UTYPE)))/50){
			errormsg("That troop has gone too far to stop marching");
			return;
		}
		P_AMOVE=safe_int_to_uchar(P_AMOVE-(curntn->maxmove * *(unitmove+(P_ATYPE%UTYPE)))/50);
		AADJMOV;
	}

	/* location dependent statuses make armies stay in place */
	if((new_stat==RULE)||(new_stat==SORTIE)
	||(new_stat==SIEGE)||(new_stat==GARRISON)) {
		P_AMOVE=0;
		AADJMOV;
	}
	P_ASTAT = safe_int_to_uchar(new_stat);
	AADJSTAT;
	if( P_AMOVE != 0 ) P_AMOVE--;
	AADJMOV;
}

/*
 * reducearmy - Split specified number of soldiers from army into new unit
 *
 * Creates a new army unit by splitting a specified number of soldiers
 * from an existing army. The new army inherits the same properties
 * (type, status, location, movement) as the source army. Validates
 * army size constraints and available army slots before splitting.
 *
 * Parameters:
 *   armynum - Index of source army to split soldiers from
 *   men - Number of soldiers to split into new army (minimum 25)
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Creates new army with specified soldier count
 *   - Reduces source army's soldier count by split amount
 *   - Updates execution file with army modifications
 *   - May fail if no army slots available (TOO MANY ARMIES error)
 *   - New army inherits all properties from source army
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires army management and validation
 *   Approach: Integration testing with various army sizes and split amounts
 *   Key Tests: Valid splits, boundary conditions, army slot limits, special unit types
 *   Dependencies: Army data structures, execution file system, error messaging
 *   Mock Requirements: Army arrays, nation data, army slot management
 *   Complexity: Moderate - Army creation with validation and state management
 *
 * Notes:
 *   - Minimum split size of 25 soldiers enforced
 *   - Cannot split monsters or leaders (special unit types)
 *   - Source army must retain at least 25 soldiers after split
 *   - New army gets same movement points as source army
 *   - Critical for tactical army management and positioning
 */
void reducearmy (int armynum, int men) {
	int army2;
	int oldx, oldy, army;

	if(P_ATYPE>=MINMONSTER){
		errormsg("sorry -- army is monster");
		return;
	}
	if(P_ATYPE>=MINLEADER){
		errormsg("sorry -- army is leader");
		return;
	}
#ifdef TRADE
	if(men<25 || armynum < 0 || armynum >= MAXARM || P_ASOLD < men+25 || P_ASTAT==ONBOARD || P_ASTAT==TRADED) {
#else
	if(men<25 || armynum < 0 || armynum >= MAXARM || P_ASOLD < men+25 || P_ASTAT==ONBOARD) {
#endif /* TRADE */
		errormsg("Selected army too small or illegal");
		return;
	}

	if(P_ASOLD < men) {
		errormsg("Too many men to split from army");
		return;
	}

	oldx=(int)P_AXLOC;
	oldy=(int)P_AYLOC;
	army2=armynum;

	armynum=(-1);
	for(army=0;army<MAXARM;army++)
		if(curntn->arm[army].sold==0){
			if(armynum==(-1)) armynum=army;
		}
	/*overflow*/
	if(armynum==(-1)) {
		errormsg("TOO MANY ARMIES");
		return;
	} else {
		P_AMOVE=curntn->arm[army2].smove;
		P_ATYPE=curntn->arm[army2].unittyp;
		P_ASTAT=curntn->arm[army2].stat;
		P_AXLOC=safe_int_to_uchar(oldx);
		P_AYLOC=safe_int_to_uchar(oldy);
		P_ASOLD=men;
		curntn->arm[army2].sold-=P_ASOLD;
		AADJSTAT;
		AADJMEN;
		AADJLOC;
		AADJMOV;
		armynum=army2;
		AADJMEN;
	}
}

/*
 * splitarmy - Interactive interface for splitting armies
 *
 * Provides user interface for splitting armies by prompting for the
 * number of soldiers to split and then calling reducearmy to perform
 * the actual split operation. Handles user input validation and
 * provides clean interface for army splitting commands.
 *
 * Parameters:
 *   armynum - Index of army to split
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Displays prompt on screen for soldier count input
 *   - Clears screen areas for user interaction
 *   - Calls reducearmy to perform actual split if valid input
 *   - May create new army if split is successful
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires UI system and army operations
 *   Approach: Integration testing with mock input and display systems
 *   Key Tests: Valid input, invalid input, zero/negative values, user cancellation
 *   Dependencies: Screen display, input system, reducearmy function
 *   Mock Requirements: Screen operations, keyboard input, army data
 *   Complexity: Simple - UI wrapper around reducearmy function
 *
 * Notes:
 *   - Simple wrapper providing user interface for army splitting
 *   - Handles input validation by checking for non-positive values
 *   - User can cancel operation by entering zero or negative value
 *   - Delegates actual splitting logic to reducearmy function
 */
void splitarmy(int armynum) {
	int men;
	clear_bottom(2);
	mvaddstr(LINES-2, 0, "How many men to split? ");
	refresh();
	men = safe_long_to_int(get_number());
	if(men <= 0) {
		return;
	}
	reducearmy(armynum,men);
}

/*
 * errormsg - Display error message and wait for user acknowledgment
 *
 * Presents error messages to the user in a standardized format with
 * visual and audio feedback. Clears the message area, displays the
 * error text, and waits for user input before continuing. Provides
 * consistent error handling across the extended command system.
 *
 * Parameters:
 *   str - Error message string to display to user
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Displays error message at bottom of screen
 *   - Clears message line and adds "PRESS ANY KEY" prompt
 *   - Produces audible beep to alert user
 *   - Waits for keyboard input before returning
 *   - Updates screen display with message formatting
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires display system and user interaction
 *   Approach: Integration testing with mock display and input systems
 *   Key Tests: Message display, screen formatting, input handling, beep functionality
 *   Dependencies: Curses display system, keyboard input, screen management
 *   Mock Requirements: Screen operations, keyboard input, audio output
 *   Complexity: Simple - Standardized error display with user interaction
 *
 * Notes:
 *   - Provides consistent error handling across extended command system
 *   - Uses curses for screen management and positioning
 *   - Audible feedback ensures user notices error condition
 *   - Blocking input ensures user acknowledges error before continuing
 *   - Critical for user experience and error communication
 */
void errormsg(char *str) {
	mvaddstr(LINES-1, 0, str);
	clrtoeol();
	mvaddstr(LINES-1, COLS-16, "PRESS ANY KEY");
	beep();
	refresh();
	getch();
}

/*
 * clear_bottom - Clear bottom lines of screen for user interface
 *
 * Utility function that clears the bottom portion of the screen to
 * prepare for user interface elements like menus, prompts, and messages.
 * Provides flexible clearing with configurable number of lines to clear.
 *
 * Parameters:
 *   i - Number of lines from bottom to clear (0 defaults to 4 lines)
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Clears specified number of lines from bottom of screen
 *   - Moves cursor to each line and clears to end of line
 *   - Prepares screen real estate for user interface elements
 *
 * Testing Notes:
 *   Category: A (Unit) - Screen utility function with minimal dependencies
 *   Approach: Unit testing with mock screen operations
 *   Key Tests: Various line counts, zero parameter handling, boundary conditions
 *   Dependencies: Curses screen management functions
 *   Mock Requirements: Screen operations (move, clrtoeol)
 *   Complexity: Simple - Basic screen clearing utility
 *
 * Notes:
 *   - Essential utility for maintaining clean user interface
 *   - Default parameter handling (0 becomes 4) for convenience
 *   - Used extensively throughout extended command system
 *   - Simple but critical for user experience
 */
void clear_bottom(int i) {
	if(i==0) i=4;
	for (; i>0 ; i--) {
		move( LINES-i, 0);
		clrtoeol();
	}
}

/*
 * addgroup - Add army to a leader's command group
 *
 * Assigns an army to follow a leader unit, creating hierarchical command
 * structures for coordinated movement and tactics. The grouped army follows
 * the leader's movements and is constrained by the leader's movement rate.
 * Critical for advanced tactical formations and army management.
 *
 * Parameters:
 *   armynum - Index of army to add to a leader's group
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Changes army status to follow specified leader (group+NUMSTATUS)
 *   - Sets leader's status to GENERAL if not already leading
 *   - Adjusts movement rates to match slowest unit in group
 *   - Updates execution file with group assignments
 *   - Validates leader and follower are in same location
 *   - Prevents invalid groupings (zombies, militia, leaders, etc.)
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires army data and leader validation
 *   Approach: Integration testing with various army and leader combinations
 *   Key Tests: Valid groupings, invalid unit types, location mismatches, movement coordination
 *   Dependencies: Army data, leader validation, status management, execution system
 *   Mock Requirements: Army arrays, leader units, input system
 *   Complexity: Moderate - Complex validation and group coordination logic
 *
 * Notes:
 *   - Only leader units (MINLEADER to MINMONSTER) can lead groups
 *   - Grouped armies cannot be leaders, zombies, militia, or special statuses
 *   - Movement rate coordination ensures group moves together
 *   - Leader becomes GENERAL when first army joins group
 *   - Essential for advanced tactical gameplay and army coordination
 */
void addgroup(int armynum) {
	int	moverate,group;

	if((P_ATYPE>=MINLEADER)&&(P_ATYPE<MINMONSTER)){
		errormsg("Can't add leader to group");
		return;
	}
	if(P_ATYPE==A_ZOMBIE) {
		errormsg("Your Zombies just don't seem inspired");
		return;
	}
	if(P_ASTAT==MARCH) {
		errormsg("Your troops must stop marching before they can be lead");
		return;
	}
	if(armynum<0 || armynum >= MAXARM || P_ASTAT==SCOUT ||
		P_ASTAT==TRADED || P_ASTAT==ONBOARD || P_ATYPE==A_MILITIA ){
		errormsg("Selected army not groupable");
		return;
	}
	mvaddstr(LINES-4,0,"enter a valid leader unit/army group id: ");
	refresh();
	group = safe_long_to_int(get_number());
	if(group < 0) {
		return;
	}
	if(group > MAXARM || curntn->arm[group].sold <=0){
		errormsg("invalid unit number");
		return;
	}
	if((P_AXLOC != curntn->arm[group].xloc)
	||(P_AYLOC != curntn->arm[group].yloc)){
		errormsg("units are not in same sector");
		return;
	}
	if((curntn->arm[group].unittyp <MINLEADER)
	||( curntn->arm[group].unittyp >=MINMONSTER)){
		errormsg("Group leader not a leader unit");
		return;
	} else if(curntn->arm[group].stat==ONBOARD){
		errormsg("Cannot group onto a transport");
		return;
	}

	P_ASTAT=safe_int_to_uchar(group+NUMSTATUS);
	moverate = P_AMOVE;
	AADJSTAT;
	armynum = group;
	P_ASTAT=GENERAL;
	AADJSTAT;
	if( P_AMOVE > moverate ){
		P_AMOVE = safe_int_to_uchar(moverate);
		AADJMOV;
	}
}
