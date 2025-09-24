/*
 * reports.c - Game reports and statistics
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

/*
 * REPORTS SYSTEM - Game Statistics and Unit Management Interface
 *
 * This file implements the comprehensive reporting system for Conquer, providing
 * interactive displays and management interfaces for armies, navies, finances,
 * and production. The reporting system serves as the primary information and
 * control center for players to monitor and manage their empire.
 *
 * KEY REPORTING SYSTEMS:
 *
 * 1. ARMY REPORTS (armyrpt):
 *    - Comprehensive army statistics and status display
 *    - Interactive army modification (disband, merge, split, group)
 *    - Support for both full army reports and group-specific reports
 *    - God mode capabilities for administrative army management
 *    - Handles special unit types (spies, mercenaries, zombies, monsters)
 *
 * 2. FINANCIAL REPORTS (budget):
 *    - Complete treasury and economic status overview
 *    - Income/expense analysis with detailed breakdowns
 *    - Population distribution and revenue calculations
 *    - Military maintenance cost projections
 *    - Charity and inflation impact modeling
 *    - Integration with production and nation management screens
 *
 * 3. PRODUCTION REPORTS (produce):
 *    - Resource production estimates and planning
 *    - Food production vs consumption analysis
 *    - Jewel and metal ore production tracking
 *    - Population allocation efficiency assessment
 *    - Monster maintenance cost calculations
 *    - Cross-linked navigation to budget and nation screens
 *
 * 4. FLEET REPORTS (fleetrpt):
 *    - Naval unit inventory and status monitoring
 *    - Ship management (warships, merchants, galleys by size class)
 *    - Fleet operations (merge, split, disband navies)
 *    - Cargo and crew capacity management
 *    - Army transport status and coordination
 *    - God mode naval administrative controls
 *
 * ARCHITECTURAL FEATURES:
 *
 * Screen Layout Management:
 * - Multi-column display system with configurable grid layouts
 * - Dynamic screen pagination for large datasets
 * - Consistent header/footer navigation interface
 * - Responsive formatting based on terminal dimensions
 *
 * Interactive Navigation:
 * - Cross-linked report navigation (budget <-> production <-> nation)
 * - Context-sensitive option menus based on unit types and permissions
 * - God mode administrative interfaces with extended capabilities
 * - Error handling and validation for all user inputs
 *
 * Data Integration:
 * - Real-time calculation of maintenance costs and resource consumption
 * - Integration with spreadsheet() economic calculations
 * - Coordination with army/navy management systems
 * - Support for trade system constraints and validations
 *
 * Permission System:
 * - Country-specific data filtering and access control
 * - God mode privilege escalation with comprehensive administrative tools
 * - Trade system integration preventing modification of traded units
 * - Validation of ownership and location constraints
 *
 * DISPLAY CONSTANTS:
 * - RPT_LINES/RPT_COLS: Column spacing for army reports (11x10)
 * - Fleet reports use 13x11 grid for naval unit display
 * - BUF_LINES/BUF_COLS: Buffer zones for screen formatting (10x15)
 * - MAXINROW/MAXINSCR: Dynamic calculation of display capacity
 *
 * CROSS-SYSTEM DEPENDENCIES:
 * - Integrates with spreadsheet() for economic calculations
 * - Uses ext_cmd() for army command dispatch
 * - Coordinates with army/navy management functions
 * - Supports trade system constraints and notifications
 * - Leverages god mode functions for administrative capabilities
 *
 * The reporting system is essential for strategic decision-making, providing
 * players with comprehensive visibility into their empire's status while
 * offering direct management capabilities for military and economic assets.
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
extern short country;
extern long startgold;
extern short xcurs,ycurs;
extern short xoffset,yoffset;
extern short redraw;

#define RPT_LINES 11
#define RPT_COLS 10
#define BUF_LINES 10
#define BUF_COLS 15
#define MAXINROW ((COLS-BUF_COLS)/RPT_COLS)
#define MAXINSCR (((LINES-BUF_LINES)/RPT_LINES)*MAXINROW)

/*
 * armyrpt - Interactive army statistics report and modification system
 *
 * Provides a comprehensive interface for viewing and managing army units,
 * displaying detailed statistics in a multi-column grid layout with full
 * interactive modification capabilities. Supports both complete army
 * inventories and location-filtered group reports.
 *
 * Parameters:
 *   repnum - Report type selector:
 *            0 = Full army report (all armies regardless of location)
 *            1 = Group report (only armies at current cursor location)
 *
 * Returns:
 *   void - Function controls its own screen display and user interaction
 *
 * Side Effects:
 *   - Completely takes over screen display with curses interface
 *   - May modify army data through interactive commands
 *   - Updates army statistics, locations, groupings, and compositions
 *   - Can disband armies, affecting sector populations and treasury
 *   - May invoke ext_cmd() for extended army command processing
 *   - In god mode, provides unrestricted army modification capabilities
 *
 * Display Architecture:
 *   - Multi-column grid layout with configurable RPT_LINES/RPT_COLS spacing
 *   - Dynamic pagination supporting large army inventories
 *   - Column headers: soldiers, location, movement, status, type, costs
 *   - Interactive navigation with space/return/other key controls
 *   - Context-sensitive option menus based on unit types and permissions
 *
 * Army Management Operations:
 *   1. Command Execution - Dispatches to ext_cmd() for tactical operations
 *   2. Army Merging - Combines armies at same location with validation
 *   3. Army Splitting - Divides armies with interactive soldier allocation
 *   4. Army Disbanding - Complex dissolution with type-specific handling:
 *      - Regular troops: 15% become mercenaries, rest return to population
 *      - Spies: Require "shut up fee" payment to prevent exposure
 *      - Mercenaries: Demand severance pay for peaceful dissolution
 *      - Zombies: Cannot be disbanded (permanent units)
 *      - Monsters: Standard disbanding without population effects
 *   5. Group Management - Assigns armies to leader-based formations
 *
 * God Mode Administrative Features:
 *   - Unrestricted location modification (teleportation)
 *   - Direct soldier count adjustment
 *   - Movement point manipulation
 *   - Unit type transformation (normal/leader/monster)
 *   - Status modification with group assignment validation
 *   - Bypasses ownership and trade restrictions
 *
 * Permission and Validation Systems:
 *   - Country ownership verification for non-god users
 *   - Trade system integration (prevents modification of traded armies)
 *   - Location-based constraints (sector ownership for disbanding)
 *   - Unit type restrictions (no disbanding zombies, no merging monsters)
 *   - Army status validation (prevents modifying onboard units)
 *
 * Economic Integration:
 *   - Calculates and displays maintenance costs per turn
 *   - Handles enlistment cost display (talons vs points for monsters)
 *   - Processes disbanding payments (spy fees, mercenary severance)
 *   - Updates treasury and population statistics
 *   - Integrates with sector adjustment macros (AADJMEN, SADJCIV2)
 *
 * Testing Notes:
 *   Category: C (System) - Requires full game state initialization
 *   Approach: System testing with mock armies and god mode validation
 *   Key Tests: Army creation/modification, disbanding economics, god privileges
 *   Dependencies: Curses display, army data structures, economic systems
 *   Mock Requirements: Game state, army inventories, sector ownership
 *   Complexity: Complex - Full-featured management interface with economics
 *
 * Notes:
 *   - Function manages its own screen lifecycle (clear/refresh/restore)
 *   - Extensive use of curses positioning and formatting
 *   - Complex state machine for interactive command processing
 *   - Critical for empire management and military strategy
 *   - Integration point for multiple game systems (economy, military, trade)
 */
void
armyrpt(int repnum)
	/* 0 is for full report 'a' and 1 is for group report 'g' */
{
	int i,j;
	char	inpkey;
	int men,attset,defset,bemerc;
	short oldarmy;
	int done=FALSE;
	int xpos,ypos;
	int isgod=FALSE;
	int count,count2;    /*number of armies on current screen */
	short armynum=0;

	if(country==0) {
		isgod=TRUE;
		if (get_god()) return;
	} else {
		/* if not god, check there are armies to display */
		men=FALSE;
		for(armynum=0;men==FALSE && armynum<MAXARM;armynum++)
			if(P_ASOLD>0 && (!repnum ||
				(P_AXLOC==XREAL && P_AYLOC==YREAL)))
				men=TRUE;
		if (!men) {
			redraw=DONE;
			clear_bottom(0);
			errormsg("no armies to display");
			makebottom();
			return;
		}
	}
	/*new army screen*/
	clear();
	count2=0;
	while(done==FALSE) {
		move(0,0);
		clrtobot();
		armynum=safe_int_to_short(count2);
		xpos=BUF_COLS;
		ypos=2;
		count=0;
		while((armynum<MAXARM)&&(count<MAXINSCR)){
			if(P_ASOLD>0 && (!repnum || (P_AXLOC==XREAL && P_AYLOC==YREAL))) {
				if (count%MAXINROW==0) {
					/* diplay header information */
					mvaddstr(ypos+1,0,"soldiers  :");
					mvaddstr(ypos+2,0,"x location:");
					mvaddstr(ypos+3,0,"y location:");
					mvaddstr(ypos+4,0,"movement  :");
					mvaddstr(ypos+5,0,"status    :");
					mvaddstr(ypos+6,0,"unit type :");
					mvaddstr(ypos+7,0,"cost/turn :");
					mvaddstr(ypos+8,0,"enlist    :");
				}
				standout();
				mvprintw(ypos,xpos,"%d:",armynum);
				standend();
				mvprintw(ypos+1,xpos,"%ld",P_ASOLD);
				mvprintw(ypos+2,xpos,"%d",(int)P_AXLOC);
				mvprintw(ypos+3,xpos,"%d",(int)P_AYLOC);
				mvprintw(ypos+4,xpos,"%d",P_AMOVE);
				if(P_ASTAT>=NUMSTATUS)
				mvprintw(ypos+5,xpos,"group %d",P_ASTAT-NUMSTATUS);
				else
				mvprintw(ypos+5,xpos,"%s",*(soldname+P_ASTAT));
				mvprintw(ypos+6,xpos,"%s",*(unittype+(P_ATYPE%UTYPE)));
				if (P_ATYPE<MINMONSTER)
				mvprintw(ypos+7,xpos,"$%ld",P_ASOLD * *(unitmaint+(P_ATYPE%UTYPE)));
				else
				mvprintw(ypos+7,xpos,"$%ld", 5L * *(unitmaint+(P_ATYPE%UTYPE)));
				if (P_ATYPE<MINMONSTER)
				mvprintw(ypos+8,xpos,"$%ld",P_ASOLD * *(u_encost+(P_ATYPE%UTYPE)));
				else
				mvprintw(ypos+8,xpos,"%d pts", (int)*(u_encost+(P_ATYPE%UTYPE)));
				count++;
				if(count<MAXINSCR && count%MAXINROW==0) {
					ypos+=RPT_LINES;
					xpos=BUF_COLS;
				} else {
					xpos+=RPT_COLS;
				}
			}
			armynum++;
		}

		/*Operate on any armies that you wish*/
		standout();
		mvprintw(0,(COLS/2)-15-safe_size_to_int(strlen(curntn->name))/2,"ARMY STATS SUMMARY FOR %s",curntn->name);

		ypos+=10;
		mvaddstr(ypos++,(COLS/2)-14,"HIT SPACE KEY IF DONE");
		mvaddstr(ypos++,(COLS/2)-25,"HIT RETURN TO CHANGE ANY ARMY STATISTICS");
		mvaddstr(ypos++,(COLS/2)-17,"HIT ANY OTHER KEY FOR MORE");
		standend();
		refresh();

		ypos++;
		if ((inpkey = safe_int_to_char(getch()))==' ') done=TRUE;
		else if((inpkey=='\n')||(inpkey=='\r')) {
			done=0;
			mvaddstr(ypos++,0,"WHAT ARMY DO YOU WANT TO CHANGE:");
			ypos++;
			clrtoeol();
			refresh();
			armynum = safe_long_to_short(get_number());
			if((armynum<0)||(armynum>MAXARM)) {
				continue;
			}
#ifdef TRADE
			if(isgod==FALSE && P_ASTAT==TRADED) {
				errormsg("May not change traded army");
				continue;
			}
#endif /* TRADE */
			mvaddstr(ypos,0,"OPTIONS: 1) COMMAND 2) DISBAND 3) CHANGE GROUP");
			if(P_ATYPE<MINLEADER)
			mvaddstr(ypos,47,"4) MERGE 5) SPLIT ARMY");
			ypos++;
			clrtoeol();
#ifdef OGOD
			if(isgod==TRUE) mvaddstr(ypos++,0,"GOD OPTIONS: 6) LOCATION 7) SOLDIERS 8) MOVE 9) UNITTYPE 0) STATUS ");
			clrtoeol();
#endif /* OGOD */
			ypos++;
			refresh();
			switch(getch()){
			case '1':
				ext_cmd(armynum);
				break;
			case '4':
				clear_bottom(0);
				if(P_ATYPE>=MINLEADER){
					errormsg("SORRY -- army is monster");
					break;
				}
				oldarmy=armynum;
				mvaddstr(ypos++,0,"TO WHAT ARMY: ");
				refresh();
				armynum = safe_long_to_short(get_number());
				if (armynum < 0) break;
				combinearmies(armynum,oldarmy);
				break;
			case '5':
				splitarmy(armynum);
				break;
			case '2':	/* disband army */
				clear_bottom(0);
				if(P_ATYPE<MINLEADER) 
					mvprintw(ypos++,0,"Disband your %s army %d?",*(unittype+(P_ATYPE%UTYPE)),armynum);
				else 	mvprintw(ypos++,0,"Disband your %s %d?",*(unittype+(P_ATYPE%UTYPE)),armynum);
				refresh();
				if(getch()!='y') break;
				if(sct[P_AXLOC][P_AYLOC].owner!=country && P_ATYPE!=A_SPY){
					errormsg("You don't own the sector");
					break;
				} else if(P_ASTAT==ONBOARD) {
					errormsg("That unit must be first be unloaded");
					break;
				} else if(P_ATYPE==A_SPY) {
					/*spys are given a shut up fee */
					mvprintw(LINES-4,0,"Your spy demands %d talons to remain quiet",
					*(u_encost+(P_ATYPE%UTYPE)) * 2);
					mvaddstr(LINES-3,0,"Pay him off? (y or n)");
					clrtoeol();
					refresh();
					if(getch()=='y'){
					/* spys do not add to population */
					curntn->tgold -= *(u_encost+(P_ATYPE%UTYPE)) * 2;
					P_ASOLD=0;
					AADJMEN;
					}
				} else if(P_ATYPE==A_MERCENARY){
					/*mercs must be payed off*/
					mvprintw(LINES-4,0,"Your mercenaries demand %ld talons to disband",
					*(u_encost+(P_ATYPE%UTYPE)) * P_ASOLD);
	
					mvaddstr(LINES-3,0,"Give them severance pay? (y or n)");
					clrtoeol();
					refresh();
					if(getch()=='y'){
					curntn->tgold -= *(u_encost+(P_ATYPE%UTYPE)) * P_ASOLD;
					/*mercs do not add to local populace*/
					attset = MERCATT;
					defset = MERCDEF;
					bemerc = safe_long_to_int(P_ASOLD);
					P_ASOLD=0;
					AADJMEN;
					AADJDISB;
					}
				} else if(P_ATYPE==A_ZOMBIE){
					errormsg("Zombie units cannot disband");
					break;
				} else {
					if (P_ATYPE < MINLEADER) {
					bemerc = safe_long_to_int((P_ASOLD*15)/100);
					/*15% become mercs*/
					attset = curntn->aplus +
						*(unitattack+(P_ATYPE%UTYPE));
					defset = curntn->dplus +
						*(unitdefend+(P_ATYPE%UTYPE));
					P_ASOLD -= bemerc;
					sct[P_AXLOC][P_AYLOC].people+=P_ASOLD;
					AADJDISB;
					}
					P_ASOLD=0;
					AADJMEN;
					i=P_AXLOC;
					j=P_AYLOC;
					SADJCIV2;
				}
				break;
			case '3':
				addgroup(armynum);
				break;
#ifdef OGOD
			case '6':
				if(isgod==TRUE){
					/*X LOCATION*/
					mvaddstr(ypos++,0,"What is the New X Loc: ");
					refresh();
					men = safe_long_to_int(get_number());
					if (men>=0 && men<MAPX)
					P_AXLOC=safe_int_to_uchar(men);
					/*Y LOCATION*/
					mvaddstr(ypos++,0,"What is the New Y Loc: ");
					refresh();
					men = safe_long_to_int(get_number());
					if (men>=0 && men<MAPY)
					P_AYLOC=safe_int_to_uchar(men);
					AADJLOC;
				}
				break;
			case '7':
				if(isgod==TRUE){
					/*SOLDIERS*/
					mvaddstr(ypos++,0,"What is the New Total Soldiers: ");
					refresh();
					men = safe_long_to_int(get_number());
					if (men>=0) {
						P_ASOLD=men;
						AADJMEN;
					}
				}
				break;
			case '8':
				if (isgod == TRUE) {
					mvaddstr(ypos, 0, "What is the new movement value?");
					refresh();
					men = safe_long_to_int(get_number());

					if (men >=0 && men <= 100) {
						P_AMOVE = safe_int_to_uchar(men);
					}
				}
				break;
			case '9':
				if (isgod == TRUE) {
					mvaddstr(ypos++, 0, "Change unit type; Normal 0-26, Leader 27-44, Monster 45-59");
					mvaddstr(ypos++, 0, "[MajorHackEH?] New Type is? ");
					refresh();
					men = safe_long_to_int(get_number());

					if (men < 0 || men > MAXMONSTER) break;
					if (men > 44) men += TWOUTYPE;
					else if (men > NOUNITTYPES) men += UTYPE;
					
					P_ATYPE = safe_int_to_uchar(men);
				}
				break;
			case '0':
				if (isgod == TRUE) {
					mvprintw(ypos++, 0, "1) Mar 2) Sct 3) Garr 4) Trade 5) Mil 6) Fly 7) Def 8) MDef 9) Att 10) MAtt");
					mvprintw(ypos++, 0, "11) Genrl 12) Sort 13) Sieg 14) Sgd 15) Onb 16) Rule 17+) Group (leader-=17)");
					mvaddstr(ypos++, 0, "Set what status? ");
					refresh();
					men = safe_long_to_int(get_number());

					if (men < 0 || men > NUMSTATUS+MAXARM)
					  break;
					if (men >= NUMSTATUS) {
					  i = curntn->arm[men-NUMSTATUS].unittyp;
					  if (i < MINLEADER || i >= MINMONSTER) {
					    errormsg("There is no such leader to group under");
					    break;
					  }
					}
					P_ASTAT = safe_int_to_uchar(men);
				}
				break;
#endif /* OGOD */
			default:
				errormsg("Invalid Input");
			}
			armynum=0;
		} else {
			/* go to start of next page in army list */
			while(armynum<MAXARM && (P_ASOLD==0 || !(!repnum ||
			(P_AXLOC==XREAL && P_AYLOC==YREAL)))) armynum++;
			if(armynum==MAXARM) count2=0;
			else count2=armynum;
		}
	}
	if(isgod==TRUE) reset_god();
}

/*
 * budget - Next season's budget estimates and financial overview
 *
 * Displays comprehensive financial projections showing current treasury status,
 * population distribution, income sources, military expenses, and projected
 * treasury after inflation and charity. Provides cross-navigation to production
 * and nation management screens for complete economic planning.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   void - Function manages its own screen display and navigation
 *
 * Side Effects:
 *   - Takes over full screen display with financial dashboard
 *   - Calls spreadsheet() to calculate economic data and population distribution
 *   - May launch produce() or change() functions based on user navigation
 *   - In god mode, temporarily adjusts startgold to current treasury for calculations
 *
 * Financial Calculations and Display:
 *   - Starting treasury vs current treasury comparison
 *   - Sector count and population distribution analysis
 *   - Food storage with starvation warning (red highlight if insufficient)
 *   - Resource holdings (jewels, metals) with current inventory
 *   - Detailed income breakdown by population sector:
 *     * Gold mines (people + revenue)
 *     * Metal mines (people + revenue)
 *     * Farms (people + food production)
 *     * Capitals (people + revenue)
 *     * Cities (people + revenue)
 *     * Other sectors (people + revenue)
 *   - Total population and net income calculation
 *
 * Military Cost Analysis:
 *   - Army maintenance: Counts soldiers and calculates per-turn costs
 *   - Monster maintenance: Special 5x multiplier for monster unit costs
 *   - Naval maintenance: Ship holds × SHIPMAINT constant
 *   - Other expenses: Difference between starting and current treasury
 *   - Total expense summary with highlighted totals
 *
 * Economic Projections:
 *   - Charity calculation: Based on nation's charity percentage setting
 *   - Net income: Total income minus all expenses and charity
 *   - Inflation impact: Complex calculation based on treasury size:
 *     * Large treasuries (>1M): Divided by inflation factor, then scaled
 *     * Smaller treasuries: Inverse scaling to reduce inflation damage
 *   - Next season's projected treasury: Final economic forecast
 *
 * Interactive Navigation:
 *   - 'P': Launch production screen (produce()) for resource planning
 *   - 'C': Launch change nation screen (change()) for policy adjustments
 *   - Other keys: Return to calling context
 *
 * God Mode Features:
 *   - Bypasses country restrictions for administrative budget analysis
 *   - Temporarily sets startgold to current treasury for accurate projections
 *   - Provides economic oversight capabilities for any nation
 *
 * Screen Layout:
 *   - Left side: Nation status, treasury, resources, sector information
 *   - Right side: Population distribution with corresponding revenue streams
 *   - Bottom section: Military costs, projections, navigation options
 *   - Strategic highlighting for critical information (food shortages, totals)
 *
 * Integration Points:
 *   - spreadsheet(): Core economic calculation engine
 *   - produce(): Resource production planning
 *   - change(): Nation policy and charity adjustment
 *   - Army/navy data structures for maintenance cost calculations
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires economic system and army/navy data
 *   Approach: Integration testing with mock nations and military units
 *   Key Tests: Economic calculations, inflation modeling, cross-navigation
 *   Dependencies: Spreadsheet calculations, army/navy inventories, display system
 *   Mock Requirements: Nation data, military units, economic parameters
 *   Complexity: Moderate - Economic calculations with cross-system integration
 *
 * Notes:
 *   - Critical for strategic economic planning and empire management
 *   - Complex inflation modeling affects long-term economic strategy
 *   - Integration hub connecting economic, military, and policy systems
 *   - Red highlighting warns of potential food shortages
 *   - Supports both player and administrative (god mode) perspectives
 */
void 
budget (void)
{
	short armynum,nvynum;
	long numship=0L,costsold=0L,numsold=0L;
	long nummonst=0L,costmonst=0L,money,chty,infmoney;
	int isgod=FALSE;

	if(country==0) {
		isgod=TRUE;
		if (get_god()) return;
		/* make budget report work for god */
		startgold=curntn->tgold;
	}

	clear();
	standout();
	mvaddstr(0,(COLS/2)-15,"NEXT SEASON'S BUDGET ESTIMATES");
	standend();
	spreadsheet(country);
  
	for(armynum=0;armynum<MAXARM;armynum++)
	if(P_ASOLD>0) {
		if(P_ATYPE<MINLEADER) {
			numsold += P_ASOLD;
			costsold += P_ASOLD * ((long) *(unitmaint+(P_ATYPE%UTYPE)));
		} else if (P_ATYPE>=MINMONSTER) {
			nummonst++;
			costmonst += 5L * ((long) *(unitmaint+(P_ATYPE%UTYPE)));
		}
	}
	for(nvynum=0;nvynum<MAXNAVY;nvynum++)
		if(P_NWSHP!=0||P_NMSHP!=0||P_NGSHP!=0)
			numship+=flthold(nvynum);

	standout();
	mvprintw(2,0,  "nation name is......%s",curntn->name);
	standend();
	mvprintw(3,0,  "starting treasury..$%ld",startgold);
	mvprintw(4,0,  "number of sectors...%d",spread.sectors);
	if(curntn->tfood<2*curntn->tciv) standout();
	mvprintw(6,0,  "granary holds.....%8ld",curntn->tfood);
	standend();
	mvprintw(7,0, "jewels owned......%8ld",curntn->jewels);
	mvprintw(8,0, "metal ore owned...%8ld",curntn->metals);
	mvprintw(2,COLS-50, "%8ld people in gold mines.%8ld",spread.ingold,spread.revjewels);
	mvprintw(3,COLS-50, "%8ld people in mines......%8ld",spread.inmetal,spread.revmetal);
	mvprintw(4,COLS-50, "%8ld people in farms......%8ld",spread.infarm,spread.revfood);
	mvprintw(5,COLS-50,"%8ld people in cities.....%8ld",spread.incap,spread.revcap);
	mvprintw(6,COLS-50,"%8ld people in towns......%8ld",spread.incity,spread.revcity);
	mvprintw(7,COLS-50, "%8ld people elsewhere.....%8ld",spread.inothr,spread.revothr);
	standout();
	mvprintw(8,COLS-50,"%8ld people INCOME........%8ld",spread.civilians,spread.gold - curntn->tgold);
	standend();
	mvprintw(10,COLS-50,"%8ld troops...............%8ld",numsold,costsold);
	mvprintw(11,COLS-50,"%8ld monsters.............%8ld",nummonst,costmonst);

	mvprintw(12,COLS-50,"%8ld ship holds @ %4ld....%8ld",numship,SHIPMAINT,numship*SHIPMAINT);
	mvprintw(13,COLS-50,"other expenses this turn......%8ld",startgold-curntn->tgold);
	standout();
	money=costmonst+costsold+(numship*SHIPMAINT)+startgold-curntn->tgold;
	mvprintw(14,COLS-50,"TOTAL EXPENSES................%8ld",money);
	standend();
	money=spread.gold - curntn->tgold - money;	/* net income */
	chty = max((money*(long)curntn->charity)/100L,0L);
	mvprintw(15,COLS-50,"CHARITY.......................%8ld",chty);
	mvprintw(16,COLS-50,"NET INCOME....................%8ld",money);

	infmoney = startgold + money - chty;
	if (infmoney > 1000000L) {
		infmoney = (infmoney / (400L + curntn->inflation)) * 400L;
	} else {
		infmoney = (infmoney * 400L) / (400L + curntn->inflation);
	}

	mvprintw(17,COLS-50,"LOSS DUE TO INFLATION.........%8ld",(startgold + money-chty)-infmoney);
	standout();
	mvprintw(18,COLS-50,"NEXT SEASON'S TREASURY........%8ld",infmoney);

	mvaddstr(LINES-3,(COLS/2)-15,"HIT 'P' TO SEE PRODUCTION SCREEN");
	mvaddstr(LINES-2,(COLS/2)-15,"HIT 'C' FOR CHANGE NATION SCREEN");
	mvaddstr(LINES-1,(COLS/2)-14,"HIT ANY OTHER KEY TO CONTINUE");
	standend();
	refresh();
	switch( getch() ) {
	case 'p':
	case 'P': produce(); break;
	case 'c':
	case 'C': change(); break;
	}
	if(isgod==TRUE) reset_god();
}

/*
 * produce - Next season's production estimates and resource planning
 *
 * Displays detailed production forecasts for food, jewels, and metals,
 * comparing current resource levels with projected production and consumption.
 * Provides critical planning information for resource management and includes
 * cross-navigation to budget and nation management screens.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   void - Function manages its own screen display and navigation
 *
 * Side Effects:
 *   - Takes over full screen display with production dashboard
 *   - Creates temporary debug file "temp" with line tracing information
 *   - Calls spreadsheet() to calculate production data and population allocation
 *   - May launch budget() or change() functions based on user navigation
 *   - In god mode, bypasses country restrictions for administrative analysis
 *
 * Food Production Analysis (Left Side):
 *   - Current granary holdings (tons of food stored)
 *   - Farm production: Population assigned to farms + tons produced
 *   - Consumption calculations with precise eat rate modeling:
 *     * Civilian consumption: population × P_EATRATE per person
 *     * Military consumption: soldiers × 2 × P_EATRATE (double military ration)
 *   - Net food projection: Production minus total consumption
 *   - Food supply estimate: Available food after consumption
 *   - Critical starvation warnings if consumption exceeds production
 *
 * Resource Production Analysis (Right Side):
 *   - Jewel Production System:
 *     * Current jewel inventory
 *     * Gold mine population assignment + jewel production
 *     * Monster jewel consumption (maintenance costs)
 *     * Net jewel supply after monster upkeep
 *   - Metal Production System:
 *     * Current metal ore inventory
 *     * Mining population assignment + ore production
 *     * Net metal supply projections
 *
 * Military Resource Impact:
 *   - Military food consumption: Doubled eat rate for soldiers
 *   - Monster maintenance: Jewel costs for magical creature upkeep
 *   - Comprehensive cost/benefit analysis for military vs production balance
 *
 * Production Debugging and Validation:
 *   - Writes line trace information to temporary debug file
 *   - Enables production calculation verification and troubleshooting
 *   - Tracks complex floating-point calculations for accuracy validation
 *
 * Interactive Navigation:
 *   - 'B': Launch budget screen (budget()) for financial analysis
 *   - 'C': Launch change nation screen (change()) for policy adjustments
 *   - Other keys: Return to calling context
 *   - Seamless integration between production, budget, and policy management
 *
 * God Mode Features:
 *   - Administrative production analysis for any nation
 *   - Bypasses country ownership restrictions
 *   - Enables economic oversight and game balance analysis
 *
 * Screen Layout and Formatting:
 *   - Split-screen design: Food production (left) + Resource production (right)
 *   - Strategic highlighting for critical projections and estimates
 *   - Detailed population allocation breakdown with corresponding output
 *   - Clear separation between current holdings and projected changes
 *
 * Economic Integration:
 *   - Coordinates with spreadsheet() for accurate production calculations
 *   - Integrates military data for consumption modeling
 *   - Supports strategic resource allocation decision-making
 *   - Links production planning with budgetary and policy systems
 *
 * Critical Planning Information:
 *   - Food shortage warnings for starvation prevention
 *   - Resource allocation efficiency analysis
 *   - Military sustainability assessment
 *   - Strategic balance between military strength and economic production
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires economic system and production data
 *   Approach: Integration testing with mock nations and resource scenarios
 *   Key Tests: Production calculations, consumption modeling, starvation scenarios
 *   Dependencies: Spreadsheet calculations, army inventories, economic data
 *   Mock Requirements: Nation data, population distribution, military units
 *   Complexity: Moderate - Resource planning with military integration
 *
 * Notes:
 *   - Essential for long-term empire sustainability and strategic planning
 *   - Complex eat rate calculations require careful floating-point handling
 *   - Debug file creation aids in production calculation verification
 *   - Critical for preventing empire collapse due to resource shortages
 *   - Integration hub connecting production, military, and economic systems
 */
void 
produce (void)
{
	FILE *fp;
	short armynum;
	long military=0;
	long nummonst=0L,costmonst=0L;
	int isgod=FALSE;

	if(country==0) {
		isgod=TRUE;
		if (get_god()) return;
	}
	fp=fopen("temp","w");
	clear();
	standout();
	mvaddstr(0,(COLS/2)-17,"NEXT SEASON'S PRODUCTION ESTIMATES");
	standend();

	spreadsheet(country);
	fprintf(fp,"LINE %d FILE %s\n",__LINE__,__FILE__);
	for(armynum=0;armynum<MAXARM;armynum++)
	if(P_ASOLD>0)
	{
		if (P_ATYPE<MINLEADER) {
			military+=P_ASOLD;
		} else if (P_ATYPE>=MINMONSTER) {
			nummonst++;
			costmonst += (long) *(unitmaint+(P_ATYPE%UTYPE));
		}
	}

	standout();
	mvprintw(4,0,  "nation name is....%s",curntn->name);
	standend();
	mvprintw(5,0,  "talons in treasury..$%ld",curntn->tgold);
	mvaddstr(7,0,  "FOOD PRODUCTION");
	mvprintw(9,0,  "granary now holds.........%8ld tons",curntn->tfood);
	mvprintw(10,0, "%8ld people in farms..%8ld tons",spread.infarm,spread.food - curntn->tfood);
	fprintf(fp,"LINE %d FILE %s\n",__LINE__,__FILE__);
	mvprintw(12,0, "%8ld people @ %3.1f eat.%8.0ld tons",spread.civilians,P_EATRATE,(long)(P_EATRATE*(float)spread.civilians));
	fprintf(fp,"LINE %d FILE %s\n",__LINE__,__FILE__);
	mvprintw(13,0, "%8ld soldiers eat.....%8.0ld tons",military,(long)(military*2*(double)P_EATRATE));
	military+= military+spread.civilians; /* military is amount eaten */
	fprintf(fp,"LINE %d FILE %s\n",__LINE__,__FILE__);
	standout();
	mvprintw(15,0, "ESTIMATE NET FOOD.........%8.0f tons",spread.food-curntn->tfood-(double)P_EATRATE*(double)military);
	mvprintw(16,0, "ESTIMATE FOOD SUPPLY......%8.0f tons",spread.food-(double)P_EATRATE*(double)military);
	standend();
	fprintf(fp,"LINE %d FILE %s\n",__LINE__,__FILE__);

	mvaddstr(7,COLS/2+1,  "OTHER PRODUCTION");
	mvprintw(9,COLS/2+1, "jewels owned is...............%8ld",curntn->jewels);
	mvprintw(10,COLS/2+1, "%8ld people in goldmines..%8ld",spread.ingold,spread.jewels-curntn->jewels);
	mvprintw(11,COLS/2+1, "%8ld monsters.............%8ld",nummonst,-costmonst);
	standout();
	mvprintw(12,COLS/2+1, "ESTIMATE JEWEL SUPPLY.........%8ld",spread.jewels-costmonst);
	standend();

	mvprintw(14,COLS/2+1, "metal ore owned is............%8ld",curntn->metals);
	mvprintw(15,COLS/2+1, "%8ld people in mines......%8ld",spread.inmetal,spread.metal-curntn->metals);
	standout();
	mvprintw(16,COLS/2+1, "ESTIMATE METAL SUPPLY.........%8ld",spread.metal);
	standend();

	standout();
	mvaddstr(LINES-3,(COLS/2)-15,"HIT 'B' TO SEE BUDGET SCREEN");
	mvaddstr(LINES-2,(COLS/2)-17,"HIT 'C' FOR CHANGE NATION SCREEN");
	mvaddstr(LINES-1,(COLS/2)-15,"HIT ANY OTHER KEY TO CONTINUE");
	standend();
	refresh();
	switch( getch() ) {
		case 'b':
		case 'B': budget(); break;
		case 'c':
		case 'C': change(); break;
	}

	if(isgod==TRUE) reset_god();
	fclose(fp);
}

#undef RPT_LINES
#undef RPT_COLS
#undef BUF_LINES
#undef BUF_COLS
#define RPT_LINES 13
#define RPT_COLS 11
#define BUF_LINES 10
#define BUF_COLS 15
char *fltstr[]= {"Light", "Medium", "Heavy"};

/*
 * fleetrpt - Interactive naval fleet statistics report and management system
 *
 * Provides comprehensive interface for viewing and managing naval fleets,
 * displaying detailed ship inventories in multi-column grid layout with full
 * interactive modification capabilities. Handles complex naval operations
 * including fleet merging, splitting, and disbanding with sophisticated
 * ship type management across three size classes.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   void - Function controls its own screen display and user interaction
 *
 * Side Effects:
 *   - Completely takes over screen display with curses naval interface
 *   - May modify naval fleet data through interactive commands
 *   - Updates ship inventories, crew assignments, and cargo allocations
 *   - Can disband fleets, affecting sector populations and army deployments
 *   - Handles complex ship transfers and fleet reorganization
 *   - In god mode, provides unrestricted naval modification capabilities
 *
 * Naval Display Architecture:
 *   - Multi-column grid layout using 13x11 spacing (RPT_LINES/RPT_COLS)
 *   - Three ship types per size class: Warships, Merchants, Galleys
 *   - Size classes: Light, Medium, Heavy (N_LIGHT, N_MEDIUM, N_HEAVY)
 *   - Column headers: ship counts, location, crew, movement, cargo, civilians
 *   - Dynamic pagination supporting large fleet inventories
 *   - Interactive navigation with space/return/other key controls
 *
 * Ship Type Management System:
 *   - Warships: Combat vessels with attack capabilities
 *   - Merchants: Cargo vessels for resource and civilian transport
 *   - Galleys: Specialized vessels with unique movement characteristics
 *   - Size Classes: Light (fast, small), Medium (balanced), Heavy (large, slow)
 *   - Each navy can contain combinations across all types and sizes
 *
 * Fleet Management Operations:
 *   1. Fleet Transfer/Merge - Combines fleets at same location with validation:
 *      - Location verification (fleets must be co-located)
 *      - Ship type limits (maximum ships per type using N_MASK)
 *      - Crew and cargo redistribution calculations
 *      - Army transfer coordination (only one fleet can carry army)
 *      - Movement point synchronization (slower fleet determines new speed)
 *
 *   2. Fleet Splitting - Divides fleets with interactive ship allocation:
 *      - Finds available empty navy slot
 *      - Individual ship type/size allocation interface
 *      - Crew and cargo distribution between split fleets
 *      - Army handling (must be unloaded before splitting)
 *      - Movement point and location inheritance
 *
 *   3. Fleet Disbanding - Complex dissolution with constraints:
 *      - Location validation (must be on land or in harbor, not open water)
 *      - Ownership verification (must own the land for disbanding)
 *      - Crew return to sector population
 *      - Civilian passenger integration into local population
 *      - Army unloading and status reset to DEFEND
 *      - Complete fleet data structure cleanup
 *
 * God Mode Administrative Features:
 *   - Ship Adjustment: Direct modification of ship counts by type and size
 *   - Location Modification: Teleportation of fleets to any coordinates
 *   - Crew Manipulation: Direct crew/ship ratio adjustment
 *   - Movement Control: Movement point modification
 *   - Bypasses ownership, location, and trade restrictions
 *
 * Cargo and Crew Management:
 *   - Crew calculations: flthold() determines total ship holding capacity
 *   - Civilian transport: fltmhold() calculates merchant vessel capacity
 *   - Army transport: Single army per fleet with coordination validation
 *   - Capacity distribution: Automatic crew/cargo rebalancing during operations
 *   - Hold calculations: Total capacity across all ship types and sizes
 *
 * Permission and Validation Systems:
 *   - Country ownership verification for non-god users
 *   - Trade system integration (prevents modification of traded fleets)
 *   - Location-based constraints (harbor/land requirements for disbanding)
 *   - Ship capacity limits (N_MASK maximum per ship type)
 *   - Army coordination rules (single army per fleet, unloading requirements)
 *
 * Ship Calculation Functions:
 *   - P_NWAR(size): Warship count for specific size class
 *   - P_NMER(size): Merchant count for specific size class
 *   - P_NGAL(size): Galley count for specific size class
 *   - SHIPS(data, size): Extracts ship count from packed data structure
 *   - addwships(), addmships(), addgships(): Ship addition functions
 *   - NADD_WAR, NSUB_WAR: Warship modification macros
 *
 * Complex State Management:
 *   - Fleet location coordination (P_NXLOC, P_NYLOC)
 *   - Movement synchronization (P_NMOVE, slowest fleet determines speed)
 *   - Army transport status (P_NARMY, MAXARM for empty)
 *   - Crew and passenger counts (P_NCREW, P_NPEOP)
 *   - Ship type packed storage (warships, merchant, galleys data structures)
 *
 * Adjustment Macro Integration:
 *   - NADJWAR, NADJMER, NADJGAL: Ship count updates
 *   - NADJCRW, NADJHLD: Crew and cargo capacity updates
 *   - NADJMOV, NADJLOC: Movement and location updates
 *   - AADJSTAT: Army status coordination for transported units
 *
 * Testing Notes:
 *   Category: C (System) - Requires full naval system and game state
 *   Approach: System testing with mock fleets and god mode validation
 *   Key Tests: Fleet operations, ship transfers, capacity calculations, god privileges
 *   Dependencies: Naval data structures, army coordination, display system
 *   Mock Requirements: Fleet inventories, army units, sector ownership
 *   Complexity: Complex - Full naval management with cargo/crew coordination
 *
 * Notes:
 *   - Most complex naval interface in the game system
 *   - Handles intricate ship type and size class management
 *   - Critical for maritime strategy and logistics
 *   - Complex validation ensures game balance and logical constraints
 *   - Integration point for naval, military, and economic systems
 *   - Extensive use of packed data structures for efficient ship storage
 */
void 
fleetrpt (void)
{
	short navy,armynum;
	short newx,newy,newnavy;
	short done=FALSE;
	int i,j;
	int xpos,ypos,crew,people;
	int count,count2;       /*screen number */
	short nvynum=0;    /*current ship id */
	short shipsize;
	int isgod=FALSE;
	if(country==0) {
		isgod=TRUE;
		if (get_god()) return;
	}
	else {
		/* if not god, check if there are navies to display */
		for(count=nvynum=0;count==0 && nvynum<MAXNAVY;nvynum++)
			if (P_NWSHP!=0||P_NMSHP!=0||P_NGSHP!=0) count=1;
		if(count==0){
			clear_bottom(0);
			errormsg("no navies");
			redraw=DONE;
			makebottom();
			return;
		}
  	}

	clear();
	count2=0;
	while(done==FALSE) {
		move(0,0);
		clrtobot();
		ypos=2;
		xpos=BUF_COLS;
		count=0;
		nvynum=safe_int_to_short(count2);
		while((nvynum<MAXNAVY)&&(count<MAXINSCR)){
			if((P_NWSHP!=0)||(P_NMSHP!=0)||(P_NGSHP!=0)) {

				/* give a navy report */
				if (count%MAXINROW==0) {
					mvaddstr(ypos+2,0, "warships  :");
					mvaddstr(ypos+3,0, "merchants :");
					mvaddstr(ypos+4,0, "galleys   :");
					mvaddstr(ypos+5,0, "x location:");
					mvaddstr(ypos+6,0, "y location:");
					mvaddstr(ypos+7,0, "crew/unit :");
					mvaddstr(ypos+8,0, "move left :");
					mvaddstr(ypos+9,0, "carrying  :");
					mvaddstr(ypos+10,0, "civilians :");
				}

				/* give naval information */
				standout();
				mvprintw(ypos,xpos,"%d:",nvynum);
				standend();
				mvaddstr(ypos+1,xpos,"lt/md/hv");
				mvprintw(ypos+2,xpos,"%2hd/%2hd/%2hd",P_NWAR(N_LIGHT),P_NWAR(N_MEDIUM),P_NWAR(N_HEAVY));
				mvprintw(ypos+3,xpos,"%2hd/%2hd/%2hd",P_NMER(N_LIGHT),P_NMER(N_MEDIUM),P_NMER(N_HEAVY));
				mvprintw(ypos+4,xpos,"%2hd/%2hd/%2hd",P_NGAL(N_LIGHT),P_NGAL(N_MEDIUM),P_NGAL(N_HEAVY));
				mvprintw(ypos+5,xpos,"%d",(int)P_NXLOC);
				mvprintw(ypos+6,xpos,"%d",(int)P_NYLOC);
				mvprintw(ypos+7,xpos,"%d",(int)P_NCREW);
				mvprintw(ypos+8,xpos,"%d",(int)P_NMOVE);
				if(P_NARMY==MAXARM)
					mvaddstr(ypos+9,xpos,"(none)");
				else {
					armynum=P_NARMY;
					if(P_ATYPE<MINLEADER)
						mvprintw(ypos+9,xpos,"army %d",armynum);
					else
						mvprintw(ypos+9,xpos,"%s %d",*(unittype+(P_ATYPE%UTYPE)),armynum);
				}
				mvprintw(ypos+10,xpos,"%d",P_NPEOP*fltmhold(nvynum));

				count++;
				if(count<MAXINSCR && count%MAXINROW==0) {
					ypos+=RPT_LINES;
					xpos=BUF_COLS;
				} else {
					xpos+=RPT_COLS;
				}
			}
			nvynum++;
		}

		/*Operate on any navies that you wish*/
		standout();
		mvprintw(0,(COLS/2)-15-safe_size_to_int(strlen(curntn->name))/2,"NAVY STATS SUMMARY FOR %s",curntn->name);

		ypos+=12;
		mvaddstr(ypos++,(COLS/2)-14,"HIT SPACE KEY IF DONE");
		mvaddstr(ypos++,(COLS/2)-18,"HIT RETURN TO CHANGE A NAVY");
		mvaddstr(ypos++,(COLS/2)-17,"HIT ANY OTHER KEY FOR MORE");
		ypos++;
		standend();
		refresh();

		if ((navy=safe_int_to_short(getch()))==' ') done=TRUE;
		else if ((navy=='\n')||(navy=='\r')){
			mvaddstr(ypos++,0,"WHAT NAVY DO YOU WANT TO CHANGE:");
			clrtoeol();
			refresh();
			nvynum = safe_long_to_short(get_number());
			if(nvynum<0) continue;
#ifdef TRADE
			if (isgod == FALSE && curntn->nvy[nvynum].commodity==TRADED) {
				errormsg("Sorry - That Navy is up for trade");
				continue;
			}
#endif /* TRADE */
			if((nvynum<0)||(nvynum>=MAXNAVY)) {
				errormsg("Invalid Naval unit");
				continue;
			}
			mvaddstr(ypos++,0,"OPTIONS: 1) TRANSFER / MERGE, 2) SPLIT NAVY, 3) DISBAND NAVY");
  			clrtoeol();
#ifdef OGOD
			if(isgod==TRUE) mvaddstr(ypos++,0,"GOD OPTIONS:  4) ADJUST SHIPS, 5) LOCATION, 6) CREW 7) MOVE");
			ypos++;
			clrtoeol();
#endif /* OGOD */
			refresh();
			switch(getch()){
			case '1':
				mvaddstr(ypos++,0,"TO WHAT NAVY: ");
				clrtoeol();
				refresh();
				newnavy = safe_long_to_short(get_number());
				if(newnavy < 0) break;
				if(newnavy >= MAXNAVY) {
					errormsg("Sorry - Invalid Navy unit");
					break;
				}
				newx = curntn->nvy[newnavy].xloc;
				newy = curntn->nvy[newnavy].yloc;
#ifdef TRADE
				if (curntn->nvy[newnavy].commodity==TRADED) {
					errormsg("Sorry - That Navy is up for trade");
				} else
#endif /* TRADE */
				if(nvynum==newnavy) {
					errormsg("Sorry -- That is the same Navy");
				}
				else if((P_NARMY!=MAXARM)
				&&(curntn->nvy[newnavy].armynum!=MAXARM)) {
					errormsg("Sorry -- One of the two Armies must be unloaded");
				}
				else if((newx==P_NXLOC)&&(newy==P_NYLOC)) {
					/* first check sizes */
					for(i=N_LIGHT;i<=N_HEAVY;i++) {
						if (P_NGAL(i) + SHIPS(curntn->nvy[newnavy].galleys,i) > N_MASK) {
							i = (-1);
							break;
						}
						if (P_NMER(i) + SHIPS(curntn->nvy[newnavy].merchant,i) > N_MASK) {
							i = (-1);
							break;
						}
						if (P_NWAR(i) + SHIPS(curntn->nvy[newnavy].warships,i) > N_MASK) {
							i = (-1);
							break;
						}
					}
					if (i == (-1)) {
					  errormsg("That would cause you to have too many ships of one type");
					  break;
					}
					crew = flthold(nvynum)*P_NCREW;
					people = fltmhold(nvynum)*P_NPEOP;
					crew += flthold(newnavy)*curntn->nvy[newnavy].crew;
					people += fltmhold(newnavy)*curntn->nvy[newnavy].people;
					for(i=N_LIGHT;i<=N_HEAVY;i++) {
						(void) addwships(newnavy,safe_int_to_short(i),safe_int_to_short(P_NWAR(i)));
						(void) addmships(newnavy,i,P_NMER(i));
						(void) addgships(newnavy,i,P_NGAL(i));
					}
					P_NCREW=0;
					P_NWSHP=0;
					P_NMSHP=0;
					P_NGSHP=0;
					if (P_NARMY!=MAXARM) {
						curntn->nvy[newnavy].armynum=P_NARMY;
						P_NARMY=MAXARM;
					}
					P_NPEOP=0;
					NADJWAR;
					NADJMER;
					NADJGAL;
					NADJCRW;
					NADJHLD;
					if(P_NMOVE<curntn->nvy[newnavy].smove)
						curntn->nvy[newnavy].smove=P_NMOVE;
					P_NMOVE=0;
					NADJMOV;
					nvynum=newnavy;
					if (flthold(nvynum)>0)
					P_NCREW=(unsigned char)(crew/flthold(nvynum));
					if (fltmhold(nvynum)>0)
					P_NPEOP=(unsigned char)(people/fltmhold(nvynum));
					NADJMOV;
					NADJWAR;
					NADJMER;
					NADJGAL;
					NADJCRW;
					NADJHLD;
				}
				else {
					errormsg("Navies not together");
				}
				break;
			case '2':
				newnavy=nvynum;
				navy=(-1);
				for(nvynum=0;nvynum<MAXNAVY;nvynum++)
				if(((P_NWSHP==0)&&(P_NMSHP==0)&&(P_NGSHP==0))
				&&(navy==(-1)))
					navy=nvynum;
				if(navy==(-1)) {
					errormsg("Too many Navies");
					break;
				}
				nvynum=newnavy;
				curntn->nvy[navy].xloc=P_NXLOC;
				curntn->nvy[navy].yloc=P_NYLOC;
				curntn->nvy[navy].crew=P_NCREW;
				curntn->nvy[navy].people=P_NPEOP;
				curntn->nvy[navy].smove=P_NMOVE;
					
				if (P_NWSHP != 0)
				mvaddstr(LINES-2,0,"Do you wish to separate warships from remainder of fleet?");
				refresh();
				if(P_NWSHP!=0 && getch()=='y') {
					curntn->nvy[navy].warships=P_NWSHP;
					P_NWSHP=0;
					NADJWAR;
					nvynum=navy;
					P_NMSHP=0;
					P_NGSHP=0;
					P_NARMY=MAXARM;
					P_NPEOP=0;
					NADJMOV;
					NADJCRW;
					NADJWAR;
					NADJMER;
					NADJGAL;
					NADJHLD;
					NADJLOC;
					break;
				} else if(P_NARMY!=MAXARM) {
					errormsg("Army must be unloaded before Navy is divided");
					break;
				}
				for(shipsize=N_LIGHT;shipsize<=N_HEAVY;shipsize++) {
					if (P_NWAR(shipsize) == 0) continue;
					mvprintw(ypos,0,"How Many %s Warships To Split?",fltstr[shipsize]);
					clrtoeol();
					refresh();
					newnavy = safe_long_to_short(get_number());
					if(newnavy>P_NWAR(shipsize)||newnavy<0) newnavy=0;
					NSUB_WAR(newnavy);
					(void) addwships(navy,shipsize,newnavy);
				}
				for(shipsize=N_LIGHT;shipsize<=N_HEAVY;shipsize++) {
					if (P_NMER(shipsize) == 0) continue;
					mvprintw(ypos,0,"How Many %s Merchants To Split?",fltstr[shipsize]);
					clrtoeol();
					refresh();
					newnavy = safe_long_to_short(get_number());
					if(newnavy>P_NMER(shipsize)||newnavy<0) newnavy=0;
					NSUB_MER(newnavy);
					(void) addmships(navy,shipsize,newnavy);
				}
				for(shipsize=N_LIGHT;shipsize<=N_HEAVY;shipsize++) {
					if (P_NGAL(shipsize) == 0) continue;
					mvprintw(ypos,0,"How Many %s Galleys To Split?",fltstr[shipsize]);
					clrtoeol();
					refresh();
					newnavy = safe_long_to_short(get_number());
					if(newnavy>P_NGAL(shipsize)||newnavy<0) newnavy=0;
					NSUB_GAL(newnavy);
					(void) addgships(navy,shipsize,newnavy);
				}

				NADJWAR;
				NADJMER;
				NADJGAL;
				nvynum=navy;
				P_NARMY=MAXARM;
				NADJMOV;
				NADJCRW;
				NADJWAR;
				NADJMER;
				NADJGAL;
				NADJHLD;
				NADJLOC;
				break;
			case '3':
				/* DISBAND NAVY */
				i=P_NXLOC;
				j=P_NYLOC;
				if(sct[i][j].altitude == WATER && isgod==FALSE) {
					errormsg("Ships need to be on land or in harbor");
					break;
				}
				if(sct[i][j].owner != country && isgod==FALSE) {
					errormsg("You don't own the land");
					break;
				}
				sct[i][j].people+=P_NCREW*flthold(nvynum);
				sct[i][j].people+=P_NPEOP*fltmhold(nvynum);
				if(P_NARMY!=MAXARM) {
					armynum=P_NARMY;
					P_NARMY=MAXARM;
					P_ASTAT=DEFEND;
					AADJSTAT;
				}
				P_NWSHP=0;
				P_NMSHP=0;
				P_NGSHP=0;
				P_NCREW=0;
				P_NPEOP=0;
				SADJCIV2;
				NADJCRW;
				NADJWAR;
				NADJGAL;
				NADJMER;
				NADJHLD;
				break;
#ifdef OGOD
			case '4':
				if (isgod==TRUE) {
					/* ADJUST SHIPS */
					for(shipsize=N_LIGHT;shipsize<=N_HEAVY;shipsize++) {
						mvprintw(ypos,0,"How Many %s Warships? [%d] ",
							 fltstr[shipsize],P_NWAR(shipsize));
						clrtoeol();
						refresh();
						newnavy = safe_long_to_short(get_number());
						if (newnavy < 0 || newnavy > N_MASK) continue;
						newnavy -= safe_int_to_short(P_NWAR(shipsize));
						if (newnavy > 0) {
							(void) NADD_WAR(newnavy);
						} else if (newnavy < 0) {
							newnavy = -newnavy;
							(void) NSUB_WAR(newnavy);
						}
					}
					for(shipsize=N_LIGHT;shipsize<=N_HEAVY;shipsize++) {
						mvprintw(ypos,0,"How Many %s Merchants? [%d] ",
							 fltstr[shipsize], P_NMER(shipsize));
						clrtoeol();
						refresh();
						newnavy = safe_long_to_short(get_number());
						if(newnavy>N_MASK||newnavy<0) continue;
						newnavy -= safe_int_to_short(P_NMER(shipsize));
						if (newnavy > 0) {
						  (void) NADD_MER(newnavy);
						} else if (newnavy < 0) {
						  newnavy = -newnavy;
						  (void) NSUB_MER(newnavy);
						}
					}
					for(shipsize=N_LIGHT;shipsize<=N_HEAVY;shipsize++) {
						mvprintw(ypos,0,"How Many %s Galleys? [%d]",
							 fltstr[shipsize], P_NGAL(shipsize));
						clrtoeol();
						refresh();
						newnavy = safe_long_to_short(get_number());
						if(newnavy>N_MASK||newnavy<0) continue;
						newnavy -= safe_int_to_short(P_NGAL(shipsize));
						if (newnavy > 0) {
						  (void) NADD_GAL(newnavy);
						} else if (newnavy < 0) {
						  newnavy = -newnavy;
						  (void) NSUB_GAL(newnavy);
						}
					}
					NADJWAR;
					NADJMER;
					NADJGAL;
				}
				break;
			case '5':
				if (isgod==TRUE) {
					/*X LOCATION*/
					mvaddstr(ypos++,0,"What Is The New X Loc: ");
					refresh();
					crew = safe_long_to_int(get_number());
					if (crew>=0 && crew<MAPX)
						P_NXLOC=safe_int_to_uchar(crew);
					/*Y LOCATION*/
					mvaddstr(ypos,0,"What Is The New Y Loc: ");
					clrtoeol();
					refresh();
					crew = safe_long_to_int(get_number());
					if (crew>=0 && crew<MAPY)
						P_NYLOC=safe_int_to_uchar(crew);
					NADJLOC;
				}
				break;
			case '6':
				if (isgod==TRUE) {
					/* ADJUST CREWSIZE */
					mvaddstr(ypos,0,"What value for crew/ship unit: ");
					refresh();
					crew = safe_long_to_int(get_number());
					if (crew>=0 && crew<=SHIPCREW) {
						P_NCREW = safe_int_to_uchar(crew);
						NADJCRW;
					}
				}
				break;
			case '7':
				if (isgod == TRUE) {
					mvaddstr(ypos, 0, "Set what move value? ");
					refresh();
					newnavy = safe_long_to_short(get_number());
					if (newnavy <= 100 && newnavy >= 0) {
						P_NMOVE = safe_short_to_uchar(newnavy);
					}
				}
				break;
#endif /* OGOD */
			default:
				errormsg("Invalid Input");
			}
			noecho();
			nvynum=0;
		} else {
			/* go to start of next page in navy list */
			while((nvynum<MAXNAVY)
			&&((P_NWSHP==0)&&(P_NMSHP==0)&&(P_NGSHP==0)))
				nvynum++;
			if(nvynum==MAXNAVY) count2=0;
			else count2=nvynum;
		}
	}
	if(isgod==TRUE) reset_god();
}
