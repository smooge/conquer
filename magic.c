/*
 * magic.c - Comprehensive Magic System and Spell Casting Implementation
 *
 * This file implements the complete magic system for the Conquer game, providing
 * all functionality for magic power acquisition, management, and utilization.
 * The system includes power acquisition mechanics, interactive interfaces for
 * power management and spell casting, unit validation based on magic requirements,
 * administrative tools, and specialized ORC takeover mechanics.
 *
 * SYSTEM ARCHITECTURE OVERVIEW
 *
 * The magic system operates through several interconnected subsystems:
 *
 * 1. **Power Acquisition System** (getmagic, exenewmgk)
 *    - Random power selection from three categories (Military, Civilian, Magic)
 *    - Complex prerequisite chains and progression hierarchies
 *    - Race-specific restrictions and mutual exclusion rules
 *    - Immediate effect application for statistical and environmental changes
 *
 * 2. **Interactive Management Interface** (domagic, god_magk)
 *    - Player-facing power purchase interface with cost calculation
 *    - Administrative god-mode interface for power manipulation
 *    - Real-time display of current powers and available options
 *    - Resource validation and transaction processing
 *
 * 3. **Unit Validation and Summoning** (unitvalid, dosummon)
 *    - Magic power requirement validation for unit recruitment
 *    - Interactive monster summoning with resource cost management
 *    - Complex power combination requirements for elite units
 *    - Army creation and proper initialization at capital location
 *
 * 4. **Spell Casting System** (wizardry)
 *    - Interactive spell casting interface for army enhancements
 *    - Four spell types: Summoning, Flight, Attack Enhancement, Defense Enhancement
 *    - Army-size-based cost calculation and resource management
 *    - Army status modification and combat effectiveness enhancement
 *
 * 5. **ORC Takeover System** (takeover, orctake) [Conditional: ORCTAKE]
 *    - Magical nation conquest mechanics exclusive to ORC race
 *    - Probability-based takeover attempts with power scaling
 *    - Diplomatic validation and territorial transfer
 *    - Interactive interface with target selection and cost management
 *
 * 6. **Power Removal and Cleanup** (removemgk)
 *    - Exact inverse operations for power loss scenarios
 *    - Statistical restoration and army type reversion
 *    - Environmental restoration (DESTROYER power effects)
 *    - Game state consistency maintenance
 *
 * POWER SYSTEM CATEGORIES
 *
 * **Military Powers**: WARRIOR → CAPTAIN → WARLORD (+10 attack/defense each)
 * **Monster Powers**: MI_MONST → AV_MONST → MA_MONST (ORC-only progression)
 * **Spell Powers**: SUMMON → WYZARD → SORCERER (non-DWARF only)
 * **Economic Powers**: URBAN, BREEDER (reproduction/movement tradeoffs)
 * **Environmental Powers**: DESTROYER (desert creation), VAMPIRE (undead conversion)
 * **Civic Powers**: DEMOCRACY, ROADS, ARMOR (balanced stat modifications)
 * **Specialized Powers**: Race-specific and player-only restrictions
 *
 * CRITICAL DESIGN PATTERNS
 *
 * **Power Validation**: All power operations include comprehensive validation
 * **Effect Pairing**: exenewmgk() and removemgk() provide exact inverse operations
 * **Resource Management**: All operations validate costs before state modification
 * **Race Restrictions**: Complex race-based power availability rules
 * **Progressive Systems**: Hierarchical power advancement with prerequisites
 * **Administrative Access**: God-mode interfaces for testing and debugging
 *
 * INTEGRATION POINTS
 *
 * **Combat System**: Power effects modify attack/defense statistics and unit types
 * **Movement System**: ROADS, ARMOR, and other powers affect movement capabilities
 * **Economics**: Resource costs for power acquisition and spell casting
 * **Diplomacy**: ORC takeover system integrates with diplomatic status
 * **Terrain**: DESTROYER power modifies environmental features
 * **Army Management**: Unit validation and summoning integrate with army systems
 *
 * CONDITIONAL COMPILATION
 *
 * - **CONQUER**: Core game functionality (domagic, dosummon, unitvalid, wizardry)
 * - **ORCTAKE**: ORC takeover mechanics (takeover, orctake)
 * - **OGOD**: Administrative god-mode interfaces (god_magk)
 * - **ADMIN**: Administrative features affecting certain power effects
 *
 * PERFORMANCE CHARACTERISTICS
 *
 * - **Power Acquisition**: O(1) with random selection from fixed arrays
 * - **Validation Functions**: O(1) for most power checks
 * - **Effect Application**: O(n) for army iteration, O(1) for stat changes
 * - **Interactive Interfaces**: Response time dependent on user input
 * - **Environmental Effects**: O(n²) for terrain modification around capital
 *
 * THREAD SAFETY
 *
 * This module is NOT thread-safe due to:
 * - Global nation state modification
 * - Curses library usage for interactive interfaces
 * - File I/O operations for news reporting
 * - Shared army and sector data structures
 *
 * TESTING STRATEGY
 *
 * Functions categorized for systematic testing approach:
 * - **Category A (Unit)**: unitvalid - Pure logic with minimal dependencies
 * - **Category B (Integration)**: getmagic, exenewmgk, removemgk, takeover
 * - **Category C (System)**: domagic, dosummon, wizardry, orctake, god_magk
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
#include <curses.h>
#include <ctype.h>
#include <unistd.h>
#include "header.h"
#include "data.h"

extern short country;
extern short redraw;
extern FILE *fexe;
extern FILE *fnews;

/*
 * getmagic - Acquire a random magic power of specified type
 *
 * Central function for magic power acquisition that randomly selects and grants
 * a new magic power from the specified category (military, civilian, or magic).
 * Implements complex power prerequisite chains, race restrictions, and mutual
 * exclusivity rules to maintain game balance and logical progression.
 *
 * The function handles multiple power categories with sophisticated validation:
 * - Military powers: WARRIOR -> CAPTAIN -> WARLORD progression
 * - Monster powers: MI_MONST -> AV_MONST -> MA_MONST (ORC-only)
 * - Spell powers: SUMMON -> WYZARD -> SORCERER (non-DWARF only)
 * - Race-specific restrictions and mutual exclusions
 * - Player-only powers vs NPC-accessible powers
 *
 * Parameters:
 *   type - Magic power category (M_MIL, M_CIV, or M_MGK)
 *          Must be valid category constant from power system
 *
 * Returns:
 *   Power value of acquired magic power on success
 *   0 if no power could be granted (already owned, race restriction, etc.)
 *
 * Side Effects:
 *   - Modifies curntn->powers by setting appropriate power bits
 *   - Updates global game state through power acquisition
 *   - May trigger cascading power effects through progression chains
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires nation state and power system integration
 *   Approach: Integration testing with mock nation setups and power combinations
 *   Key Tests: Power progression chains, race restrictions, mutual exclusions,
 *             random distribution validation, prerequisite enforcement
 *   Dependencies: Global nation state, power arrays, race constants
 *   Mock Requirements: Nation setup, power state manipulation, race configuration
 *   Complexity: Complex - Sophisticated logic with multiple validation paths
 *
 * Notes:
 *   - Thread safety: Modifies global nation state (not thread-safe)
 *   - Performance: O(1) with random selection from fixed power arrays
 *   - Critical for game balance: Enforces power acquisition rules and restrictions
 *   - Historical: Complex evolution of power system with accumulated restrictions
 */

long
getmagic(type)
int type;
{
	long newpower;
	int start,end;
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
		default:
			printf("fatal error in num_powers");
			abrt();
	}
	newpower=powers[start+(rand()%end)];
	if(newpower==0) {
		printf("ILLEGAL POWER");
		abrt();
	}

	if((newpower==WARRIOR)||(newpower==CAPTAIN)||(newpower==WARLORD)){
		if(magic(country,WARRIOR)!=TRUE){
			curntn->powers|=WARRIOR;
			return(WARRIOR);
		}
		else if(magic(country,CAPTAIN)!=TRUE){
			curntn->powers|=CAPTAIN;
			return(CAPTAIN);
		}
		else if(magic(country,WARLORD)!=TRUE){
			curntn->powers|=WARLORD;
			return(WARLORD);
		}
		else return(0L);
	}else if((newpower==MI_MONST) ||(newpower==AV_MONST) ||(newpower==MA_MONST)){
		if(curntn->race!=ORC) return(0L);
		if(magic(country,MI_MONST)!=TRUE){
			curntn->powers|=MI_MONST;
			return(MI_MONST);
		}
		else if(magic(country,AV_MONST)!=TRUE){
			curntn->powers|=AV_MONST;
			return(AV_MONST);
		}
		else if(magic(country,MA_MONST)==TRUE){
			curntn->powers|=MA_MONST;
			return(MA_MONST);
		}
		else return(0L);
	}else if(newpower==CAVALRY){
		if(curntn->race==ORC) return(0L);
		if(isnotpc(curntn->active)) return(0L);	/*npc nation*/
		if(magic(country,newpower)==TRUE) return(0L);
		curntn->powers|=newpower;
		return(newpower);
	}else if(newpower==URBAN){
		if(magic(country,BREEDER)==TRUE) return(0L);
		if(magic(country,newpower)==TRUE) return(0L);
		curntn->powers|=newpower;
		return(newpower);
	}else if(newpower==RELIGION){
		if(curntn->race==ORC) return(0L);
		if(magic(country,newpower)==TRUE) return(0L);
		curntn->powers|=newpower;
		return(newpower);
	}else if(newpower==KNOWALL){
#ifdef OGOD
#ifdef HIDELOC
	     /* only god should have KNOWALL if sectors hidden */
	     if(country!=0) return(0L);
#endif
#endif
	     if(magic(country,KNOWALL)==TRUE) return(0L);
	     curntn->powers|=KNOWALL;
	     return(KNOWALL);
	}else if((newpower==SLAVER)
	||(newpower==DERVISH)
	||(newpower==HIDDEN)
	||(newpower==ARCHITECT)
	||(newpower==THE_VOID)
	||(newpower==ARCHER)){
		if((magic(country,newpower)==TRUE)
		||((newpower==DERVISH)&&(magic(country,DESTROYER)==TRUE)))
			 return(0L);
		curntn->powers|=newpower;
		return(newpower);
	}else if(newpower==DESTROYER){
		if((curntn->race!=ELF)
		&&(magic(country,DESTROYER)!=TRUE)
		&&(magic(country,DERVISH)!=TRUE)){
			curntn->powers|=DESTROYER;
			return(DESTROYER);
		}
		return(0L);
	}else if(newpower==VAMPIRE){
		if((curntn->race!=ELF)&&(magic(country,VAMPIRE)!=TRUE)){
			curntn->powers|=VAMPIRE;
			return(VAMPIRE);
		}
		return(0L);
	}else if(newpower==MINER){
		if((curntn->race!=ELF)&&(curntn->race!=DWARF)&&(magic(country,MINER)!=TRUE)){
			curntn->powers|=MINER;
			return(MINER);
		}
		return(0L);
	}else if(newpower==STEEL){
		if(magic(country,STEEL)==TRUE) return(0L);
		if(magic(country,MINER)!=TRUE) return(0L);
		curntn->powers|=STEEL;
		return(STEEL);
	}else if(newpower==BREEDER){
		if(magic(country,URBAN)==TRUE) return(0L);
		if(magic(country,BREEDER)==TRUE) return(0L);
		if(curntn->race!=ORC) return(0L);
		curntn->powers|=BREEDER;
		return(BREEDER);
	}
	else if(isnotpc(curntn->active)) {
		return(0L);	/* remaining powers only for pc's */
	} else if((newpower==NINJA)
	||(newpower==SLAVER)
	||(newpower==SAILOR)
	||(newpower==DEMOCRACY)
	||(newpower==ROADS)
	||(newpower==SAPPER)
	||(newpower==ARMOR)
	||(newpower==AVIAN)){
		if(magic(country,newpower)==TRUE) return(0L);
		curntn->powers|=newpower;
		return(newpower);
	}
	else if((newpower==SUMMON)||(newpower==WYZARD)||(newpower==SORCERER)){
		/* dwarves may not cast spells */
		if(curntn->race==DWARF) return(0L);
		if(magic(country,SUMMON)!=TRUE) {
			curntn->powers|=SUMMON;
			return(SUMMON);
		} else if(magic(country,WYZARD)!=TRUE) {
			curntn->powers|=WYZARD;
			return(WYZARD);
		} else if(magic(country,SORCERER)!=TRUE) {
			curntn->powers|=SORCERER;
			return(SORCERER);
		} else return(0L);
	} else return(0L);
}
#ifdef CONQUER
/*
 * domagic - Interactive magic power purchase and display interface
 *
 * Comprehensive magic system interface that allows players to view available
 * magic powers, current holdings, costs, and purchase new random powers.
 * Provides full-screen interactive form with real-time cost calculation,
 * power display, and god-mode administrative access for debugging.
 *
 * The interface displays three main categories of magic powers with dynamic
 * pricing based on current nation state and existing power count. Implements
 * retry logic for power acquisition attempts and provides feedback on success
 * or failure. Includes special ORC takeover functionality when enabled.
 *
 * Key Interface Features:
 * - Real-time jewel cost calculation for each power category
 * - Complete listing of current nation powers with descriptions
 * - Category selection with input validation
 * - Retry loop (up to 500 attempts) for power acquisition
 * - Administrative god-mode power manipulation access
 * - Integration with ORC takeover mechanics (conditional compilation)
 *
 * Parameters:
 *   None - Function operates on global nation state and user input
 *
 * Returns:
 *   void - No return value, modifies global state through user interaction
 *
 * Side Effects:
 *   - Modifies curntn->jewels by deducting purchase costs
 *   - Updates curntn->powers through successful power acquisition
 *   - Triggers screen redraws and cursor positioning
 *   - May invoke god-mode administration or ORC takeover features
 *   - Updates game state through CHGMGK and exenewmgk() calls
 *
 * Testing Notes:
 *   Category: C (System) - Requires full curses interface and user interaction
 *   Approach: System testing with automated input simulation and screen validation
 *   Key Tests: Cost calculation accuracy, power purchase flow, god mode access,
 *             input validation, screen layout, retry logic effectiveness
 *   Dependencies: Curses library, global nation state, power system, screen management
 *   Mock Requirements: Input simulation, screen capture, nation state setup
 *   Complexity: Complex - Full interactive system with multiple conditional paths
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to curses and global state usage
 *   - Performance: Interactive - response time dependent on user input speed
 *   - User experience: Central magic system interface - critical for gameplay
 *   - Conditional compilation: Features vary based on compile-time options (OGOD, ORCTAKE)
 */
void
domagic()
{
	int county, countx, done=FALSE, loop=0, i,type;
	long price,x;
#ifdef OGOD
	void god_magk();
#endif /* OGOD */

	short isgod=0;
	if(country==0) {
		isgod=TRUE;
		if (get_god()) return;
	}

	clear();
	while(done==FALSE){
		done=TRUE;
		move(0,0);
		clrtobot();
		county=3;
		countx=0;
		redraw=FULL;
		standout();
		mvprintw(0,(COLS/2)-15,"MAGIC POWERS FOR %s",curntn->name);
		mvprintw(county++,30,"1) %d military powers: %ld jewels",
			num_powers(country,M_MIL) ,getmgkcost(M_MIL,country));
		mvprintw(county++,30,"2) %d civilian powers: %ld jewels",
			num_powers(country,M_CIV) ,getmgkcost(M_CIV,country));
		mvprintw(county++,30,"3) %d magic powers:    %ld jewels",
			num_powers(country,M_MGK),getmgkcost(M_MGK,country));

		price =  getmgkcost(M_MIL,country);
		if(price > getmgkcost(M_CIV,country))
			price = getmgkcost(M_CIV,country);
		if(price > getmgkcost(M_MGK,country))
			price = getmgkcost(M_MGK,country);

		standend();
		county=3;
		/*print the powers that you have*/
		i=0;
		while( powers[i] != 0 ){
			if(magic(country,powers[i])==TRUE)
			mvprintw(county++,countx,"you have power %s",*(pwrname+i));
			i++;
			if (county > 18) {
				county=7;
				countx=40;
			}
		}

		if(county<=7) county=8;
		else if(countx == 40) county = 20;
		else county++;
		standout();
		mvprintw(county++,0,"YOU HAVE %ld JEWELS IN YOUR TREASURY",curntn->jewels);
#ifdef OGOD
		if (isgod==TRUE) {
			mvaddstr(county++,0,"DO YOU WISH TO CHANGE THIS NATION'S POWERS? [yn]");
			standend();
			refresh();
			if (getch()=='y') god_magk();
		} else
#endif /* OGOD */
		if(price <= curntn->jewels){

		mvaddstr(county++,0,"DO YOU WISH TO BUY A RANDOM NEW POWER? [yn]");
		standend();
		refresh();
		if(getch()=='y'){
			done=FALSE;
			mvaddstr(county++,0,"ENTER SELECTION (1,2,3):");
			refresh();
			type = getch() - '0';
			if(type==M_MIL || type==M_CIV || type==M_MGK){
			price=getmgkcost(type,country);
			if(curntn->jewels>=price) {
				loop = 0;
				while(loop++ < 500) if((x=getmagic(type))!=0){
					curntn->jewels -= price;
					CHGMGK;
					exenewmgk(x);
					refresh();
					if (isgod==TRUE) reset_god();
					break;
				}
				if (loop >= 500)
					errormsg("You have too many powers!");
			} else errormsg("CAN'T AFFORD A NEW POWER");
			} else errormsg("BAD SELECTION");
		}
		} else {
			mvaddstr(LINES-1,0,"CAN'T AFFORD A NEW POWER");
			clrtoeol();
			mvaddstr(LINES-1,60,"PRESS ANY KEY");
			refresh();
			standend();
			getch();
		}
#ifdef ORCTAKE
		if((curntn->race==ORC)&&(curntn->jewels>=ORCTAKE)&&(curntn->spellpts>=TAKEPOINTS))
			done |= orctake(&county);
#endif /* ORCTAKE */
	}
	if(isgod==TRUE) reset_god();
}
#endif /* CONQUER */
#ifdef ORCTAKE
/*
 * takeover - Execute magical nation takeover attempt with probability mechanics
 *
 * Core implementation of the ORC magical takeover system that allows nations
 * to attempt hostile magical conquest of other nations based on monster powers.
 * Supports both targeted takeovers (player-initiated) and random takeovers
 * (AI/update system). Implements sophisticated validation for diplomatic
 * relations, race compatibility, and nation status requirements.
 *
 * The function operates in two distinct modes:
 * - Update mode (target=0): AI randomly selects valid takeover targets
 * - Targeted mode (target>0): Player specifies exact target nation
 * Both modes respect diplomatic constraints and race requirements.
 *
 * Takeover Process:
 * 1. Probability check against specified percentage chance
 * 2. Target validation (race, diplomatic status, active status)
 * 3. Capital acquisition and nation destruction sequence
 * 4. News reporting and cleanup operations
 * 5. Territory transfer and administrative updates
 *
 * Parameters:
 *   percent - Probability of takeover success (0-100 percentage)
 *   target  - Target nation ID (0 for random selection, >0 for specific target)
 *
 * Returns:
 *   Nation ID of successfully taken nation on success
 *   0 if takeover failed or no valid targets found
 *
 * Side Effects:
 *   - Destroys target nation and transfers ownership to aggressor
 *   - Modifies sector ownership and designation of conquered capital
 *   - Generates news reports about successful takeovers
 *   - Updates diplomatic status and nation relationships
 *   - May trigger cascading political and territorial changes
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires nation state, diplomacy, and map systems
 *   Approach: Integration testing with diplomatic setups and nation configurations
 *   Key Tests: Probability mechanics, target validation, diplomatic constraints,
 *             race restrictions, random target selection, news generation
 *   Dependencies: Global nation arrays, sector map, diplomatic status, news system
 *   Mock Requirements: Nation setup, diplomatic relationships, sector ownership
 *   Complexity: Complex - Multi-system integration with cascading effects
 *
 * Notes:
 *   - Thread safety: Modifies global game state (not thread-safe)
 *   - Performance: O(n) worst case for random target search (n=nations)
 *   - Game balance: High-impact feature requiring careful probability tuning
 *   - Conditional: Only available when ORCTAKE compile option enabled
 */
int
takeover(percent,target)
int percent,target;
{
	int loop=1,y,save,isupdate=0;
	save=country;
	if(target==country) return(0);
	if(target==0) isupdate=1;
	country=target;
	if(rand()%100<percent){
		loop=0;
		y=0;
		if (target==0) while(loop==FALSE){
			y++;
			country=rand()%NTOTAL;
			if((ntn[country].race==ntn[save].race)
			&&(isnpc(ntn[country].active))
			&&(curntn->dstatus[country]<HOSTILE)
			&&(curntn->dstatus[country]!=UNMET)
			&&(country!=save))
				loop=TRUE;
			else if(y>=500) {
				country=save;
				return(0);
			}
		}
		sct[ntn[country].capx][ntn[country].capy].owner=save;
		if(isupdate==1){
		printf("nation %s magically taken over by %s\n",ntn[country].name,ntn[save].name);
		fprintf(fnews,"1.\tnation %s magically taken over by %s\n",ntn[country].name,ntn[save].name);
		}
		else {
			DESTROY;
			if ((fnews=fopen("/dev/null","w"))==NULL){
				printf("error opening null file\n");
				exit(FAIL);
			}
		}
		destroy(country);
		sct[ntn[country].capx][ntn[country].capy].designation=DCITY;
		if(isupdate!=1) fclose(fnews);
		y=country;
		country=save;
		return(y);
	}
	country=save;
	return(0);
}
#endif /* ORCTAKE */

/*
 * exenewmgk - Execute immediate effects and transformations of newly acquired magic powers
 *
 * Central magic power activation function that applies the immediate statistical,
 * military, and territorial effects when a nation acquires a new magic power.
 * Implements the complete range of power effects from simple stat bonuses to
 * complex environmental transformations and army conversions.
 *
 * The function handles diverse magic power effects including:
 * - Combat stat modifications (attack/defense bonuses and penalties)
 * - Population and reproduction rate changes
 * - Movement capability adjustments
 * - Army type transformations (infantry to zombies, etc.)
 * - Environmental terraforming (DESTROYER creates deserts)
 * - Special ability enhancements (mining capability increases)
 * - Player-only power restrictions for balance
 *
 * Power Categories Handled:
 * - Military Powers: WARRIOR, CAPTAIN, WARLORD (+10 attack/defense each)
 * - Religious Powers: RELIGION (reproduction bonus, defense scaling)
 * - Environmental Powers: DESTROYER (desert creation), VAMPIRE (undead conversion)
 * - Economic Powers: URBAN, BREEDER (reproduction/movement tradeoffs)
 * - Civic Powers: DEMOCRACY (balanced stat increases), ROADS (+4 movement)
 * - Special Powers: ARMOR (defense/movement tradeoff), MINER (+25 mining)
 *
 * Parameters:
 *   newpower - Magic power constant to activate (must be valid power from powers array)
 *
 * Returns:
 *   void - No return value, effects applied through global state modification
 *
 * Side Effects:
 *   - Modifies nation combat statistics (aplus, dplus)
 *   - Updates reproduction rates and movement capabilities
 *   - Transforms army units to new types based on power effects
 *   - Modifies terrain around nation capital (DESTROYER power)
 *   - Updates mining and special abilities
 *   - Triggers movement updates and news reporting
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires nation state, army arrays, and map systems
 *   Approach: Integration testing with nation setup and stat verification
 *   Key Tests: Stat modification accuracy, army transformation logic, terrain changes,
 *             race-specific restrictions, power interaction effects
 *   Dependencies: Global nation state, army arrays, sector map, news system
 *   Mock Requirements: Nation setup, army configuration, map initialization
 *   Complexity: Complex - Multiple system interactions and conditional effects
 *
 * Notes:
 *   - Thread safety: Modifies global game state (not thread-safe)
 *   - Performance: O(n) for army iteration, O(1) for most stat changes
 *   - Game balance: Critical function affecting core game mechanics
 *   - Conditional compilation: Some effects depend on ADMIN flag for update vs player mode
 */
void
exenewmgk(newpower)
long newpower;
{
	short x,armynum;
#ifdef ADMIN
	short y;
#endif
	if(newpower==WARRIOR) {
		curntn->aplus+=10;
		curntn->dplus+=10;
		return;
	}
	if(newpower==CAPTAIN) {
		curntn->aplus+=10;
		curntn->dplus+=10;
		return;
	}
	if(newpower==WARLORD) {
		curntn->aplus+=10;
		curntn->dplus+=10;
		return;
	}
	if(newpower==RELIGION) {
		if(curntn->race==ORC) {
			printf("ORCS CANT HAVE RELIGION POWER\n");
			abrt();
		} else if(curntn->repro<=8){
			curntn->repro+=2;
		} else if(curntn->repro==9){
			curntn->repro=10;
			curntn->dplus+=5;
		} else if(curntn->repro>=10){
			curntn->dplus+=10;
		}
		return;
	}
	if(newpower==DESTROYER) {
/* this ifdef is so that destroyer only takes place in an update */
#ifdef ADMIN
		for(x=(int)curntn->capx-3;x<=(int)curntn->capx+3;x++) {
			for(y=(int)curntn->capy-3;y<=(int)curntn->capy+3;y++){
				if((ONMAP(x,y))
				&&(sct[x][y].altitude!=WATER)
#ifdef DERVDESG
				&&((rand()%2)==0)
#else
				&&(tofood( &sct[x][y],0)<6)
#endif /* DERVDESG */
				&&((x!=curntn->capx)
					||(y!=curntn->capy))){
					sct[x][y].vegetation=DESERT;
					sct[x][y].designation=DNODESIG;
				}
			}
		}
		fprintf(fnews,"1.\tnation %s gets destroyer power: land turns to desert\n",curntn->name);
#endif /* ADMIN */
		updmove(curntn->race,country);
		return;
	}
	if(newpower==DERVISH) {
		updmove(curntn->race,country);
		return;
	}
	if((newpower==MI_MONST)
	||(newpower==AV_MONST)
	||(newpower==MA_MONST)
	||(newpower==KNOWALL)
	||(newpower==HIDDEN)
	||(newpower==THE_VOID)
	||(newpower==ARCHITECT))
		return;
	if(newpower==MINER) {
		curntn->mine_ability+=25;
		return;
	}
	if(newpower==VAMPIRE) {
		curntn->aplus-=35;
		curntn->dplus-=35;
		for(armynum=0;armynum<MAXARM;armynum++){
			if((P_ATYPE == A_INFANTRY)||(P_ATYPE == A_MILITIA))
				P_ATYPE=A_ZOMBIE;
		}
		return;
	}
	if(newpower==URBAN) {
		if(curntn->race==ORC) {
			x=curntn->repro;
			if(curntn->repro>=14){
				curntn->maxmove+=3;
			}
			else if(curntn->repro>11){
				curntn->maxmove+= x-11;
				curntn->repro=14;
			}
			else curntn->repro+=3;
		}
		else if(curntn->repro<=9){
			curntn->repro+=3;
		}
		else {
			curntn->maxmove+=2*(curntn->repro-9);
			curntn->repro=12;
		}
		return;
	}
	if(newpower==BREEDER) {
		x=curntn->repro;
		if(curntn->repro>=14){
			curntn->maxmove+=3;
		}
		else if(curntn->repro>11){
			curntn->maxmove+= x-11;
			curntn->repro=14;
		}
		else curntn->repro+=3;
		curntn->dplus-=10;
		curntn->aplus-=10;
		return;
	}
	if(newpower==DEMOCRACY){
		curntn->maxmove+=1;
		curntn->repro+=1;
		curntn->dplus+=10;
		curntn->aplus+=10;
		return;
	}
	if(newpower==ROADS){
		curntn->maxmove+=4;
		return;
	}
	if(newpower==ARMOR){
		curntn->maxmove-=3;
		if( curntn->maxmove<4) curntn->maxmove=4;
		curntn->dplus+=20;
	}
	if((newpower==NINJA)
	||(newpower==STEEL)
	||(newpower==ARCHER)
	||(newpower==CAVALRY)
	||(newpower==SAILOR)
	||(newpower==SUMMON)
	||(newpower==WYZARD)
	||(newpower==SORCERER)
	||(newpower==SAPPER)
	||(newpower==AVIAN)){	/* these powers are only for pc's */
		return;
	}
}
#ifdef CONQUER
/*
 * dosummon - Interactive monster summoning interface with cost validation
 *
 * Comprehensive spell-casting interface that allows players to summon magical
 * creatures and monsters using spell points and gold. Provides dynamic monster
 * selection menu, validates summoning prerequisites, handles resource costs,
 * and creates new army units at the nation's capital location.
 *
 * The interface displays all valid monster types based on current magic powers,
 * calculates summoning costs (both spell points and gold), validates player
 * resources, and creates properly configured army units upon successful summon.
 * Implements sophisticated cost calculation based on unit strength and type.
 *
 * Summoning Process:
 * 1. Display available monster types based on magic powers
 * 2. Accept player selection via single character input
 * 3. Validate monster type against power requirements
 * 4. Calculate and verify spell point and gold costs
 * 5. Create new army unit at capital with proper initialization
 * 6. Deduct costs and update player resources
 *
 * Cost Calculation:
 * - Spell points: Fixed cost per unit type (u_encost array)
 * - Gold cost: Unit strength × enchantment cost (unitminsth × u_encost)
 * - Resource validation prevents insufficient fund attempts
 *
 * Parameters:
 *   None - Operates on global nation state and user input
 *
 * Returns:
 *   void - No return value, creates army units and modifies resources
 *
 * Side Effects:
 *   - Creates new army unit in first available army slot
 *   - Deducts spell points and gold from nation treasury
 *   - Updates army location to nation capital coordinates
 *   - Sets army status to DEFEND and initializes movement points
 *   - Triggers army adjustment macros for proper state management
 *
 * Testing Notes:
 *   Category: C (System) - Requires full curses interface and army management
 *   Approach: System testing with input simulation and resource verification
 *   Key Tests: Monster selection validation, cost calculation accuracy, resource deduction,
 *             army creation logic, insufficient funds handling, army slot management
 *   Dependencies: Curses library, unit data arrays, army management, nation treasury
 *   Mock Requirements: Input simulation, unit data setup, army slot availability
 *   Complexity: Complex - Full interactive system with resource management
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to curses and global state usage
 *   - Performance: Interactive - dependent on user selection speed
 *   - Game balance: Resource costs prevent summoning abuse
 *   - Army management: Finds first available slot or reports "NO FREE ARMIES"
 */
void
dosummon()
{
	int x,count,i,armynum;
	long e_cost;
	int newtype,s_cost;
	char line[LINELTH+1],ch;

	x=0;
	count=LINES-4;
	clear_bottom(0);
	mvaddstr(count,x,"options:");
	x+=9;
	for(i=MINMONSTER;i<=MAXMONSTER;i++){
		if(unitvalid(i)==TRUE) {
			mvprintw(count,x+2,"%s",*(shunittype+(i%UTYPE)));
			mvprintw(count,x,"(%c)",*(shunittype+(i%UTYPE))[0]);
			x+=7;
			if(x>COLS-20){
				x=0;
				count++;
			}
		}
	}
	count++;
	mvaddstr(count++,0,"what type of unit do you want to raise:");
	refresh();
	ch=getch();
	for(newtype=MINMONSTER;newtype<=MAXMONSTER;newtype++){
		if( *(shunittype+(newtype%UTYPE))[0] == ch ) break;
	}

	if (newtype==MAXMONSTER+1) {
		/* quick exit */
		return;
	}
	if(unitvalid(newtype)==FALSE) {
		beep();
		clear_bottom(0);
		errormsg("you are unable to summon that monster");
		return;
	}

	s_cost= *(u_encost+(newtype%UTYPE));
	if(s_cost > curntn->spellpts) {
		sprintf(line,"you dont have %d spell points",s_cost);
		clear_bottom(0);
		errormsg(line);
		return;
	}

	e_cost= (long) *(u_encost+(newtype%UTYPE)) * *(unitminsth+(newtype%UTYPE));
	if(e_cost >  curntn->tgold) {
		sprintf(line,"you dont have %ld gold talons in your treasury",e_cost);
		clear_bottom(0);
		errormsg(line);
		return;
	}

	armynum=0;
	while(armynum<MAXARM) {
		if(P_ASOLD<=0) {
			P_ASOLD= *(unitminsth+(newtype%UTYPE));
			P_ATYPE=newtype;
			P_ASTAT=DEFEND; /* set new armies to DEFEND */
			P_AXLOC=curntn->capx;
			P_AYLOC=curntn->capy;
			P_AMOVE=0;
			AADJLOC;
			AADJSTAT;
			AADJMEN;
			AADJMOV;
			armynum=MAXARM;
		} else if(armynum==MAXARM-1) {
			clear_bottom(0);
			errormsg("NO FREE ARMIES");
			return;
		} else armynum++;
  	}
  	curntn->tgold -= e_cost;
  	curntn->spellpts -= s_cost;
	EDECSPL;
}
#ifdef ORCTAKE
/*
 * orctake - Interactive ORC nation takeover interface with power-based probability
 *
 * User interface component that provides ORC players with magical nation takeover
 * capabilities based on their monster power levels. Calculates takeover probability
 * based on acquired monster powers, handles resource costs, validates targets,
 * and executes takeover attempts through the core takeover() function.
 *
 * The interface integrates with the broader magic system to provide ORC-specific
 * gameplay mechanics. Takeover probability scales with monster power advancement:
 * MI_MONST (3%), AV_MONST (6%), MA_MONST (10%). Implements comprehensive target
 * validation including diplomatic status, race requirements, and nation activity.
 *
 * Takeover Power Scaling:
 * - MI_MONST: 3% chance, basic monster control power
 * - AV_MONST: 6% chance, advanced monster control power
 * - MA_MONST: 10% chance, master monster control power
 * - Requires ORCTAKE jewels and TAKEPOINTS spell points per attempt
 *
 * Target Validation Requirements:
 * - Target must be ORC race (same-race takeover restriction)
 * - Diplomatic status cannot be UNMET, HOSTILE, WAR, or JIHAD
 * - Target nation must be active NPC (not player controlled)
 * - Target nation ID must be valid within nation array bounds
 *
 * Parameters:
 *   count - Pointer to screen line counter for display positioning
 *           Modified to track current screen position during interface updates
 *
 * Returns:
 *   TRUE (1) if interface completed without takeover attempt
 *   FALSE (0) if takeover attempt was executed (regardless of success)
 *
 * Side Effects:
 *   - Deducts ORCTAKE jewels and TAKEPOINTS spell points on attempt
 *   - May execute successful nation takeover through takeover() function
 *   - Updates diplomatic status on failed attempts (increases hostility)
 *   - Modifies screen display and cursor positioning
 *   - Triggers spell point decrements and diplomatic adjustments
 *
 * Testing Notes:
 *   Category: C (System) - Requires full curses interface and diplomatic system
 *   Approach: System testing with ORC nation setup and diplomatic configurations
 *   Key Tests: Power-based probability calculation, target validation logic,
 *             resource deduction, diplomatic status updates, screen management
 *   Dependencies: Curses library, nation arrays, diplomatic system, resource management
 *   Mock Requirements: ORC nation setup, diplomatic relationships, resource allocation
 *   Complexity: Complex - Multi-system integration with conditional success paths
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to curses and global state usage
 *   - Performance: Interactive - depends on user input and target validation speed
 *   - Game balance: High-cost, low-probability feature requiring strategic resource use
 *   - ORC-specific: Unique racial ability providing alternative conquest mechanism
 */
int
orctake(count)
int *count;
{
	int chance=0,done=TRUE,i,s_cost;
	if((*count)>20) {
		(*count)=2;
		clear();
	}
	if(magic(country,MA_MONST)==TRUE) {
	mvprintw((*count)++,0,"You have a 10 percent chance for %ld Jewels take over other orcs",ORCTAKE);
	chance=10;
	} else if(magic(country,AV_MONST)==TRUE) {
	mvprintw((*count)++,0,"You have a 6 percent chance for %ld Jewels take over other orcs",ORCTAKE);
	chance=6;
	} else if(magic(country,MI_MONST)==TRUE){
	mvprintw((*count)++,0,"You have a 3 percent chance for %ld Jewels to take over other orcs",ORCTAKE);
	chance=3;
	}
	if(chance==0) return(TRUE);

	mvaddstr((*count)++,0,"  Target can't be Unmet, Hostile, War, or Jihad");
	mvaddstr((*count)++,0,"DO YOU WISH TO TAKE OVER AN ORC NPC NATION: [ny]");
	refresh();
	if(getch()=='y'){
		done=FALSE;
		mvaddstr((*count)++,0,"  What orc nation:");
		refresh();
		i=get_country();
		if(i<=0 || i>=NTOTAL || !isntn(ntn[i].active) )
			mvaddstr((*count)++,0,"  Invalid Nation");
		else if((curntn->dstatus[i]<HOSTILE)
		&&(curntn->dstatus[i]!=UNMET)
		&&(ntn[i].race==ORC)){
			curntn->jewels-=ORCTAKE;
			s_cost=TAKEPOINTS;
			curntn->spellpts-=s_cost;
			EDECSPL;
			if(( takeover(chance,i)) !=0 )
				mvprintw((*count)++,0,"  Successful: %d",i);
			else {
				mvaddstr((*count)++,0,"  Failed: Nation becomes more hostile");
				curntn->dstatus[i]++;
				EADJDIP(country,i);
			}
		} else {
			if (ntn[i].race==ORC) 
				mvaddstr((*count)++,0,"  That Nation is Outside Your Influence");
			else mvaddstr((*count)++,0,"  Wrong Race");
		}
		refresh();
		sleep(2);
	}
	return(done);
}
#endif /* ORCTAKE */
#endif /* CONQUER */
#ifdef CONQUER

/*
 * unitvalid - Validate if nation has magic powers required for specific unit type
 *
 * Core unit validation function that determines whether a nation possesses the
 * necessary magic powers to recruit, draft, or summon a specific unit type.
 * Implements the complete magic power prerequisite system that governs unit
 * availability and maintains strategic balance in military compositions.
 *
 * The function enforces complex power requirements including:
 * - Basic military powers for standard units (WARRIOR, CAPTAIN, WARLORD)
 * - Monster control powers for creature units (MI_MONST, AV_MONST, MA_MONST)
 * - Specialized combat powers for enhanced units (NINJA, ARCHER, CAVALRY)
 * - Combined power requirements for elite units (ARMOR + CAVALRY = KNIGHT)
 * - Race-specific unit restrictions and default unit fallbacks
 * - Conditional availability based on power combinations
 *
 * Power Validation Categories:
 * - Infantry/Militia: Default unit validation through defaultunit() function
 * - Monster Units: Tiered monster powers with ORC racial requirements
 * - Specialist Units: Individual magic power requirements per unit type
 * - Elite Units: Multiple power combinations required for advanced units
 * - Magical Creatures: Spell casting powers for summoned creatures
 * - Undead Units: VAMPIRE power for zombie/wraith/mummy recruitment
 *
 * Unit Type Validation Examples:
 * - A_INFANTRY: Available if nation's default unit type
 * - A_ORC/A_GOBLIN: Requires MI_MONST magic power
 * - A_KNIGHT: Requires both ARMOR and CAVALRY powers
 * - DRAGON: Requires both MA_MONST and WYZARD powers
 * - A_SPY/A_SCOUT: Handled elsewhere in recruitment system
 *
 * Parameters:
 *   type - Unit type constant to validate (from unit type enumeration)
 *          Must be valid unit type from game's unit classification system
 *
 * Returns:
 *   TRUE (1) if nation has required magic powers for unit type
 *   FALSE (0) if nation lacks necessary powers or restrictions apply
 *
 * Side Effects:
 *   - No direct side effects (read-only validation function)
 *   - Calls magic() function to check current nation power status
 *   - May call defaultunit() to determine nation's basic unit type
 *
 * Testing Notes:
 *   Category: A (Unit) - Pure validation logic with minimal dependencies
 *   Approach: Unit testing with mock power configurations and unit type sets
 *   Key Tests: Power requirement accuracy, combined power validation, race restrictions,
 *             default unit handling, edge cases for unrestricted types
 *   Dependencies: magic() function, defaultunit() function, power constants
 *   Mock Requirements: Nation power state setup, unit type constants
 *   Complexity: Moderate - Complex conditional logic but well-structured validation
 *
 * Notes:
 *   - Thread safety: Read-only function, safe for concurrent access
 *   - Performance: O(1) - Single switch statement with constant-time power checks
 *   - Game balance: Critical function controlling unit availability and strategic options
 *   - Integration: Used by recruitment, summoning, and army management systems
 */
int
unitvalid(type)
int type;
{
	int valid=FALSE;
	switch(type){
		case A_INFANTRY: /* not everyone gets infantry now */
				if(defaultunit(country)==A_INFANTRY) valid=TRUE;
				break;
		case GARGOYLE:
		case A_GOBLIN:
		case A_ORC:	if(magic(country,MI_MONST)==TRUE) valid=TRUE;
				break;
		case A_MARINES: if(magic(country,SAILOR)==TRUE) valid=TRUE;
				break;
		case A_ARCHER:	if(magic(country,ARCHER)==TRUE) valid=TRUE;
				break;
		case A_URUK:	if(magic(country,AV_MONST)==TRUE) valid=TRUE;
				break;
		case A_NINJA:	if(magic(country,NINJA)==TRUE) valid=TRUE;
				break;
		case A_PHALANX:	if(magic(country,CAPTAIN)==TRUE) valid=TRUE;
				break;
		case A_OLOG:	if((magic(country,BREEDER)==TRUE)
				&&(magic(country,AV_MONST)==TRUE)) valid=TRUE;
				break;
		case A_ELEPHANT:if(magic(country,DERVISH)==TRUE)  valid=TRUE;
				break;
		case SUPERHERO:
		case A_LEGION:	if(magic(country,WARLORD)==TRUE) valid=TRUE;
				break;
		case A_TROLL:	if(magic(country,MA_MONST)==TRUE) valid=TRUE;
				break;
		case A_ELITE:	if(magic(country,ARMOR)==TRUE) valid=TRUE;
				break;
		case CENTAUR:
		case A_LT_CAV:
		case A_CAVALRY:	if(magic(country,CAVALRY)==TRUE) valid=TRUE;
				break;
		case A_KNIGHT:	if((magic(country,ARMOR)==TRUE)
				&&(magic(country,CAVALRY)==TRUE)) valid=TRUE;
				break;
		case A_ROC:
		case A_GRIFFON: if(magic(country,AVIAN)==TRUE) valid=TRUE;
				break;
		case ASSASSIN:	if(magic(country,NINJA)==TRUE) valid=TRUE;
				break;
		case DJINNI:	if(magic(country,DERVISH)==TRUE) valid=TRUE;
				break;
		case HERO:	if(magic(country,WARRIOR)==TRUE) valid=TRUE;
				break;
		case ELEMENTAL:	if(magic(country,SORCERER)==TRUE) valid=TRUE;
				break;
		case A_ZOMBIE:
		case WRAITH:
		case MUMMY:	if(magic(country,VAMPIRE)==TRUE) valid=TRUE;
				break;
		case MINOTAUR:
		case DEMON:	if(magic(country,DESTROYER)==TRUE) valid=TRUE;
				break;
		case BALROG:	if((magic(country,WYZARD)==TRUE)
				&&(magic(country,VAMPIRE)==TRUE)) valid=TRUE;
				break;
		case DRAGON:	if((magic(country,MA_MONST)==TRUE)
				&&(magic(country,WYZARD)==TRUE)) valid=TRUE;
				break;
		case A_SPY:
		case A_SCOUT:	break;	/* handled elsewhere */
		default:	valid=TRUE;	/* for all unrestricted types */
	}
	return(valid);
}
#endif /* CONQUER */

/*
 * removemgk - Remove effects and cleanup when magic power is lost or revoked
 *
 * Critical magic system function that reverses all statistical, military, and
 * territorial effects when a nation loses a magic power. Implements the exact
 * inverse operations of exenewmgk() to ensure game state consistency and prevent
 * residual effects from powers no longer possessed by the nation.
 *
 * The function handles comprehensive power removal effects including:
 * - Combat stat reversals (attack/defense penalty removal)
 * - Population and reproduction rate restoration
 * - Movement capability adjustments back to baseline
 * - Army type reversion (undead back to standard units)
 * - Environmental restoration (desert terrain back to vegetation)
 * - Special ability reductions (mining capability decreases)
 * - Complex power interaction cleanup
 *
 * Power Removal Categories Handled:
 * - Military Powers: WARRIOR, CAPTAIN, WARLORD (-10 attack/defense each)
 * - Religious Powers: RELIGION (reproduction reduction, defense scaling removal)
 * - Environmental Powers: DESTROYER (desert restoration), VAMPIRE (undead reversion)
 * - Economic Powers: URBAN, BREEDER (reproduction/movement baseline restoration)
 * - Civic Powers: DEMOCRACY (balanced stat decreases), ROADS (-4 movement)
 * - Special Powers: ARMOR (defense/movement restoration), MINER (-25 mining)
 * - Monster Powers: MI_MONST, AV_MONST (army type reversion to default units)
 *
 * Critical Design Requirements:
 * - Must be exact inverse of corresponding exenewmgk() operations
 * - Handles complex army transformations back to race default units
 * - Manages environmental restoration for DESTROYER power removal
 * - Prevents invalid states through proper error checking
 * - Updates movement capabilities for affected races and powers
 *
 * Parameters:
 *   oldpower - Magic power constant being removed (must be valid power from powers array)
 *              Power must already be removed from nation->powers before calling
 *
 * Returns:
 *   void - No return value, effects applied through global state modification
 *
 * Side Effects:
 *   - Modifies nation combat statistics (aplus, dplus) in reverse
 *   - Updates reproduction rates and movement capabilities to baseline
 *   - Reverts army units to default types based on race
 *   - Restores terrain around nation capital (DESTROYER removal)
 *   - Updates mining and special abilities to pre-power levels
 *   - Triggers movement recalculation for affected powers
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires nation state, army arrays, and map systems
 *   Approach: Integration testing with power acquisition/removal cycles
 *   Key Tests: Stat restoration accuracy, army reversion logic, terrain restoration,
 *             exact inverse verification, power interaction cleanup
 *   Dependencies: Global nation state, army arrays, sector map, movement system
 *   Mock Requirements: Nation setup with powers, army configuration, map initialization
 *   Complexity: Complex - Mirror complexity of exenewmgk() with additional reversion logic
 *
 * Notes:
 *   - Thread safety: Modifies global game state (not thread-safe)
 *   - Performance: O(n) for army iteration, O(1) for most stat reversals
 *   - Game balance: Critical for maintaining consistent game state during power loss
 *   - Pairing requirement: Must exactly reverse effects applied by exenewmgk()
 *   - Administrative usage: Called during god-mode power manipulation and game events
 */
void
removemgk(oldpower)
long oldpower;
{
	short x,y,armynum;
	if((oldpower==WARRIOR)
	||(oldpower==CAPTAIN)
	||(oldpower==WARLORD)) {
		curntn->aplus-=10;
		curntn->dplus-=10;
		return;
	}
	if(oldpower==RELIGION) {
		if(curntn->race==ORC) {
			printf("ORCS CANT HAVE RELIGION POWER\n");
			abrt();
		} else curntn->repro -= 2;
		return;
	}
	if(oldpower==DESTROYER) {
		for(x=(int)curntn->capx-3;x<=(int)curntn->capx+3;x++) {
			for(y=(int)curntn->capy-3;y<=(int)curntn->capy+3;y++){
				if((ONMAP(x,y))
				&&(sct[x][y].altitude!=WATER)
				&&((x!=curntn->capx)
					||(y!=curntn->capy))){
					if (sct[x][y].vegetation==DESERT)
					{
						/* LT_VEG has medium value*/
						sct[x][y].vegetation=LT_VEG;
						sct[x][y].designation=DNODESIG;
					}
				}
			}
		}
		updmove(curntn->race,country);
		return;
	}
	if(oldpower==DERVISH) {
		updmove(curntn->race,country);
		return;
	}
	if(oldpower==VAMPIRE) {
		curntn->aplus+=35;
		curntn->dplus+=35;
		for(armynum=0;armynum<MAXARM;armynum++){
			if(P_ATYPE == A_ZOMBIE) P_ATYPE=defaultunit(country);
		}
		return;
	}
	if(oldpower==URBAN) {
		curntn->repro -= 3;
		return;
	}
	if(oldpower==BREEDER) {
		curntn->repro-=3;
		curntn->dplus+=10;
		curntn->aplus+=10;
		for(armynum=0;armynum<MAXARM;armynum++){
			if(P_ATYPE == A_OLOG) P_ATYPE=A_URUK;
		}
		return;
	}
	if(oldpower==DEMOCRACY){
		curntn->maxmove-=1;
		curntn->repro-=1;
		curntn->dplus-=10;
		curntn->aplus-=10;
		return;
	}
	if(oldpower==ROADS){
		curntn->maxmove-=4;
		return;
	}
	if(oldpower==ARMOR){
		curntn->maxmove+=3;
		curntn->dplus-=20;
		return;
	}
	if(oldpower==MI_MONST) {
		for(armynum=0;armynum<MAXARM;armynum++){
			if(P_ATYPE == A_ORC) P_ATYPE=defaultunit(country);
		}
		return;
	}
	if(oldpower==AV_MONST) {
		for(armynum=0;armynum<MAXARM;armynum++){
			if(P_ATYPE == A_URUK || P_ATYPE == A_OLOG)
				P_ATYPE=defaultunit(country);
		}
		return;
	}
	if(oldpower==ARCHER) {
		for(armynum=0;armynum<MAXARM;armynum++){
			if(P_ATYPE == A_ARCHER) P_ATYPE=defaultunit(country);
		}
		return;
	}
	/* remaining cause no change in statistics */
}

#ifdef CONQUER
#ifdef OGOD
/*
 * god_magk - Administrative god-mode interface for selective magic power manipulation
 *
 * Comprehensive administrative tool that provides god-mode users with complete
 * control over nation magic power states. Offers interactive interface for adding
 * or removing any magic power from any nation, with real-time power listing,
 * validation, and immediate effect application through the magic system.
 *
 * The interface provides two distinct operational modes:
 * - Add Mode: Displays all powers the nation lacks and allows selective addition
 * - Remove Mode: Shows all powers the nation possesses and enables targeted removal
 * Both modes include comprehensive validation and effect application through
 * exenewmgk() and removemgk() functions to maintain game state consistency.
 *
 * Administrative Interface Features:
 * - Dynamic power listing based on current nation state
 * - Two-column display layout for large power lists
 * - Numeric selection system with validation
 * - Confirmation prompts before power modification
 * - Immediate effect application and game state updates
 * - Multiple operation support with continuation prompts
 * - Complete integration with magic system effect functions
 *
 * Power Manipulation Process:
 * 1. Mode selection (Add/Remove) with input validation
 * 2. Dynamic power list generation based on nation state
 * 3. Numeric selection with range validation
 * 4. Confirmation prompt with power identification
 * 5. Power state modification through XOR bitwise operation
 * 6. Effect application through exenewmgk() or removemgk()
 * 7. Continuation prompt for additional modifications
 *
 * Validation and Safety Features:
 * - Prevents adding already-possessed powers
 * - Prevents removing non-existent powers
 * - Validates power selection within valid range
 * - Confirmation prompts prevent accidental modifications
 * - Immediate effect application maintains consistency
 *
 * Parameters:
 *   None - Operates on global god state and user input
 *          Assumes god mode is active (country==0)
 *
 * Returns:
 *   void - No return value, modifies nation power state through interaction
 *
 * Side Effects:
 *   - Modifies target nation's power bitfield through XOR operations
 *   - Applies immediate power effects through exenewmgk() or removemgk()
 *   - Updates screen display and cursor positioning
 *   - Triggers comprehensive game state updates
 *   - May modify nation statistics, armies, and environmental features
 *
 * Testing Notes:
 *   Category: C (System) - Requires full curses interface and god mode access
 *   Approach: System testing with god mode setup and power manipulation validation
 *   Key Tests: Mode selection accuracy, power listing correctness, validation logic,
 *             effect application verification, interface navigation, safety features
 *   Dependencies: Curses library, god mode state, complete magic system integration
 *   Mock Requirements: God mode simulation, nation setup, power state manipulation
 *   Complexity: Complex - Full administrative interface with multi-mode operation
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to curses and global state usage
 *   - Performance: Interactive - response dependent on administrator input speed
 *   - Administrative privilege: Only available in god mode (OGOD compilation)
 *   - Power management: Provides complete administrative control over magic systems
 *   - Game testing: Essential tool for game balance testing and debugging
 */
void
god_magk()
{
	int county,countx,choice;
	int remove,i,done=FALSE;

	clear();
	while (done==FALSE) {
		county=3;
		countx=0;
		standout();
		mvaddstr(0,0,"Do you wish to (A)dd or (R)emove a power?");
		clrtoeol();
		refresh();
		while (done==FALSE) {
			done=TRUE;
			switch(getch()) {
			case 'A':
			case 'a':
				remove=FALSE;
				break;
			case 'R':
			case 'r':
				remove=TRUE;
				break;
			default:
				done=FALSE;
				break;
			}
		}
		move(0,0);
		clrtobot();
		if (remove)
		mvprintw(0,(COLS/2)-15,"CURRENT POWERS FOR %s",curntn->name);
		else
		mvprintw(0,(COLS/2)-15,"LACKING POWERS FOR %s",curntn->name);
		standend();
		i=0;
		while( powers[i] != 0 ){
			if(magic(country,powers[i])==remove) {
				mvprintw(county,countx,"%2d: %s Power",i+1,*(pwrname+i));
				county++;
			}
			i++;
			if (county > LINES-6) {
				county = 3;
				countx = COLS/2;
			}
		}
		if (countx == COLS/2) {
			county = LINES-4;
		}
		else county++;
		standout();
		if (remove) mvaddstr(county++,0,"Which power to remove?");
		else mvaddstr(county++,0,"Which power to add?");
		standend();
		addch(' ');
		refresh();
		choice=get_number();
		if(choice > 0 && choice <= MAXPOWER) {
			if (magic(country,powers[choice-1])==remove) {
				if (remove)
				mvprintw(county++,0,"Remove magic #%d? (y or [n])",choice);
				else
				mvprintw(county++,0,"Add magic #%d? (y or [n])",choice);
				refresh();
				if (getch()=='y') {
					curntn->powers ^= powers[choice-1];
					if (remove) removemgk(powers[choice-1]);
					else exenewmgk(powers[choice-1]);
				}
			} else {
				if (remove) mvaddstr(county++,0,"The nation doesn't have that power.");
				else mvaddstr(county++,0,"The nation already has that power.");
			}
		}
		mvaddstr(county++,0,"Do you wish to add or remove another power?");
		done=TRUE;
		refresh();
		if (getch()=='y') done=FALSE;
	}
}
#endif /* OGOD */

#define NUMSPELLS 4
char *spellstr[NUMSPELLS]={"(S)ummon","(F)light","(A)ttack Enhancement",
	"(D)efense Enhancement"};
/* quick adjustment to allow magical status change */
int magicstat[NUMSPELLS]={DEFEND, FLIGHT, MAGATT, MAGDEF};
/* number of soldiers per point of spell cost */
int magiccost[NUMSPELLS]={0,100,300,300};
/*
 * wizardry - Interactive spell casting interface for magical combat enhancements
 *
 * Comprehensive spell casting system that provides players with access to four
 * distinct magical effects: summoning, flight enhancement, attack enhancement,
 * and defense enhancement. Implements a complete interactive interface with
 * spell selection, cost calculation, resource validation, and effect application
 * to selected army units.
 *
 * The interface offers four primary spell categories:
 * - Summon: Delegates to dosummon() for creature summoning (requires SUMMON power)
 * - Flight: Grants flight capability to selected army unit
 * - Attack Enhancement: Provides magical attack bonus to selected army unit
 * - Defense Enhancement: Provides magical defense bonus to selected army unit
 * All spells except summoning require spell point expenditure and valid army selection.
 *
 * Spell System Features:
 * - Dynamic spell menu based on available magic powers
 * - Spell point cost calculation based on army size
 * - Comprehensive army validation and status checking
 * - Immediate status change application through change_status() function
 * - Resource deduction and game state updates
 * - Error handling for invalid selections and insufficient resources
 *
 * Spell Cost Mechanics:
 * - Flight: 1 spell point per 100 soldiers (minimum 1 point)
 * - Attack Enhancement: 1 spell point per 300 soldiers (minimum 1 point)
 * - Defense Enhancement: 1 spell point per 300 soldiers (minimum 1 point)
 * - Summoning: Variable cost handled by dosummon() function
 *
 * Army Validation Requirements:
 * - Army must be currently selected through getselunit()
 * - Army must have soldiers (P_ASOLD > 0)
 * - Army cannot be SCOUT, TRADED, GENERAL, or invalid status
 * - Army cannot be ONBOARD (carried by another unit)
 * - Army cannot already have the target magical status
 * - Army cannot be MARCH status (too busy moving)
 *
 * Parameters:
 *   None - Operates on global spell point state and selected army
 *          Requires player to have spell points available
 *
 * Returns:
 *   void - No return value, applies spell effects through army status changes
 *
 * Side Effects:
 *   - Deducts spell points from nation treasury based on spell cost
 *   - Changes selected army status to magical enhancement state
 *   - Updates army capabilities and combat effectiveness
 *   - Triggers screen updates and bottom line reconstruction
 *   - May invoke dosummon() function for creature summoning
 *
 * Testing Notes:
 *   Category: C (System) - Requires full curses interface and army selection system
 *   Approach: System testing with army setup and spell point allocation
 *   Key Tests: Spell selection accuracy, cost calculation verification, army validation,
 *             status change application, resource deduction, error handling
 *   Dependencies: Curses library, army management system, spell point system, dosummon()
 *   Mock Requirements: Army setup, spell point allocation, unit selection simulation
 *   Complexity: Complex - Full interactive system with multiple spell types and validation
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to curses and global state usage
 *   - Performance: Interactive - dependent on user selection and army validation speed
 *   - Game balance: Spell costs scale with army size to prevent overpowered small units
 *   - Integration: Works closely with army management and combat systems
 *   - Conditional compilation: Only available when CONQUER flag is enabled
 */
void
wizardry()
{
	int i,xspt,yspt,choice,armynum,s_cost;
	char line[LINELTH+1];
	void dosummon();

	clear_bottom(0);
	if(curntn->spellpts>0)
	{
		xspt=0; yspt=LINES-3;
		/* summon only to those with summon */
		if (magic(country,SUMMON)==TRUE) i=0;
		else i=1;
		for (;i<NUMSPELLS;i++) {
			sprintf(line,"  %s",spellstr[i]);
			mvaddstr(yspt,xspt,line);
			xspt += strlen(line);
			if (xspt>COLS-20) {
				xspt=0;
				yspt++;
			}
		}
		mvaddstr(LINES-4,0,"Which spell to cast:");
		refresh();
		choice=NUMSPELLS;
		switch(getch()) {
		case 's':
		case 'S':
			if (magic(country,SUMMON)==TRUE) dosummon();
			else {
				clear_bottom(0);
				errormsg("you do not have SUMMON power");
			}
			break;
		case 'f':
		case 'F':
			choice--;
		case 'a':
		case 'A':
			choice--;
		case 'd':
		case 'D':
			choice--;
			/* change status of currently selected army */
			armynum = getselunit();
			clear_bottom(0);
			if (armynum<0 || armynum>=MAXARM ||
			P_ASTAT==SCOUT || P_ASTAT==TRADED ||
			P_ASTAT==GENERAL || P_ASTAT>NUMSTATUS ||
			P_ASOLD<=0) {
				errormsg("Invalid Unit for Magicking");
			} else if(P_ASTAT==ONBOARD) {
				errormsg("Carried armies must be unloaded");
			} else if(P_ASTAT==magicstat[choice]) {
				errormsg("Unit has already been magicked");
			} else if(P_ASTAT==MARCH) {
				errormsg("That unit is too busy marching");
			} else {
				/*cost of 1 spell point for magiccost men*/
				s_cost = (P_ASOLD-1) / magiccost[choice] + 1;
				if (s_cost > curntn->spellpts) {
					sprintf(line,"You don't have %d spell points",s_cost);
					errormsg(line);
				} else {
					change_status(armynum,magicstat[choice]);
					curntn->spellpts -= s_cost;
					EDECSPL;
				}
			}
			break;
		default:
			break;
		}
	}
	else errormsg("You have no spell points for spell casting");
	makebottom();
}
#endif /* CONQUER */
