/*
 * npc.c - Non-player character AI and behavior management system
 *
 * This file implements the comprehensive artificial intelligence system for non-player
 * characters in Conquer. It provides sophisticated AI behavior including strategic
 * decision making, diplomatic relations, military planning, economic management,
 * and tactical movement algorithms.
 *
 * Core AI Systems:
 * - Diplomatic relationship management with dynamic status changes
 * - Strategic sector attractiveness calculation for movement planning
 * - Intelligent military reorganization and force allocation
 * - Economic sector redesignation based on resource needs
 * - Monster AI behavior (nomads, savages, pirates, lizards)
 * - Automated nation management for NPC civilizations
 *
 * Key Features:
 * - Multi-layered attractiveness scoring system for tactical decisions
 * - Visibility-based information warfare (magic powers affect AI knowledge)
 * - Adaptive AI personality types with different aggression levels
 * - Context-aware military and economic planning
 * - Sophisticated diplomacy system with alliance/war state management
 *
 * AI Architecture:
 * The AI operates through a main nationrun() function that coordinates multiple
 * subsystems including diplomacy, military management, economic planning, and
 * movement. The attractiveness calculation system uses weighted scoring to
 * determine optimal AI actions based on current game state and AI personality.
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
#include "header.h"
#include "data.h"
#include "safe_convert.h"

static	int **attr;     	/*sector attactiveness*/
extern	int	dissarray;	/* has nation lost its leader */

static  int stx, sty, endx, endy;	/* npc range of operations */
static  int peace;	/*is 8 if at peace, 12 if at war*/
static	int Avg_food;
static	int Avg_soldiers[NTOTAL];
static	int Avg_tradegood;

/* macros that indicate what the country can see */
#ifdef	NPC_SEE_SECTORS
#define		SEE_SECTOR(x,y,country)			TRUE
#else	/* NPC_SEE_SECTORS */
#define     SEE_SECTOR(x,y,country) \
                ( (magic(sct[x][y].owner,THE_VOID)!=TRUE) \
                  || (magic(country,NINJA)==TRUE) )
#endif	/* NPC_SEE_SECTORS */

#ifdef	NPC_SEE_CITIES
#define		SEE_CITIES(city_nation,country)		TRUE
#else	/* NPC_SEE_CITIES */
#define     SEE_CITIES(city_nation,country) \
                ( (magic(city_nation,THE_VOID)!=TRUE) \
                  || (magic(country,NINJA)==TRUE) )
#endif	/* NPC_SEE_CITIES */

#ifdef	NPC_COUNT_ARMIES
#define		COUNT_ARMIES(army_nation,country)	TRUE
#else	/* NPC_COUNT_ARMIES */
#define     COUNT_ARMIES(army_nation,country) \
                ( ( (magic(army_nation,THE_VOID)!=TRUE) \
                    && (magic(army_nation,HIDDEN)!=TRUE) \
                   ) || (magic(country,NINJA)==TRUE) )
#endif	/* NPC_COUNT_ARMIES */

/* When a sector cannot be seen by an npc nation because of
 * THE_VOID, a value must still be given to the
 * sector, because there MIGHT be something there.
 * The following unseen (UNS_) value represents this.
 */
#define	UNS_CITY_VALUE	10	/* If there is an under-defended City
				 * the value is 500
				 */

/*
 * prtattr - Debug output function for sector attractiveness visualization
 *
 * Displays a formatted grid showing the attractiveness values calculated for
 * sectors around the current nation's capital. This function is essential for
 * debugging and tuning the AI attractiveness calculation algorithms.
 *
 * Parameters:
 *   None (uses global curntn for current nation and attr for attractiveness)
 *
 * Returns:
 *   void (debug output only)
 *
 * Testing Notes:
 *   Category: D (Mock) - Debug-only function requiring specific compile flags
 *   Complexity: Simple - Straightforward debug output with conditional compilation
  * @last_documented: 2025-09-18
 */
void prtattr(void){
#ifdef DEBUG
	int x,y;
/*	FOR TESTING OF WHAT THE ATTRACTIVENESS ENDS UP LIKE  */
	printf("Attractiveness for sectors around CAPITAL at %d %d veg alt des\n",curntn->capx,curntn->capy);
	for(y=(int)curntn->capy-3;y<(int)curntn->capy+4;y++) {
		printf("\n");
		for(x=(int)curntn->capx-3;x<(int)curntn->capx+4;x++) if(ONMAP(x,y)) {
			if( sct[x][y].altitude != WATER ) {
				printf("[%2d,%2d]   ",x,y);
			}
		}
		printf("\n");
		for(x=(int)curntn->capx-3;x<(int)curntn->capx+4;x++) if(ONMAP(x,y)) {
			if( sct[x][y].altitude != WATER ) {
				printf("%5d %c%c%c "
					,attr[x][y]
					,sct[x][y].vegetation
					,sct[x][y].altitude
					,sct[x][y].designation);
			} else	printf("0      ~  ");
		}
		printf("\n");
	}
#endif /* DEBUG */
}

/*
 * newdip - Initialize diplomatic status between nations upon first contact
 *
 * Establishes the initial diplomatic relationship when two nations first
 * encounter each other. Uses race-based compatibility, nation types, and
 * probabilistic factors to determine starting diplomatic status. This
 * function is crucial for dynamic diplomacy where relationships evolve
 * based on first contact circumstances rather than predetermined tables.
 *
 * Parameters:
 *   ntn1 - Nation index being updated (the nation whose diplomacy is set)
 *   ntn2 - Target nation index (the nation being encountered)
 *
 * Returns:
 *   void (modifies ntn[ntn1].dstatus[ntn2] directly)
 *
 * Side Effects:
 *   - Modifies global diplomatic status array ntn[].dstatus[][]
 *   - May establish HOSTILE, WAR, NEUTRAL, or FRIENDLY status
 *   - Uses random number generation for probabilistic outcomes
 *
 * Diplomatic Logic:
 *   - PC nations: Generally peaceful (NEUTRAL), hostile only to orcs
 *   - Orc involvement: High probability of HOSTILE or WAR status
 *   - Monster nations: Always WAR status with everyone
 *   - Same race: 50% chance of FRIENDLY, otherwise NEUTRAL
 *   - Different races: Default to NEUTRAL status
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires nation data structures and race definitions
 *   Approach: Integration testing with mock nation setups
 *   Key Tests: PC-orc hostility, same-race friendship, monster warfare
 *   Dependencies: Global ntn[] array, race constants, status constants
 *   Mock Requirements: Nation structures with race and active fields
 *   Complexity: Moderate - Multiple conditional paths with probabilistic outcomes
 *
 * Notes:
 *   - Only updates ntn1's view of ntn2, not bidirectional
 *   - Random elements make relationships unpredictable but balanced
 *   - Special rules for PC (player character) nations
 *   - Orc racial hostility hardcoded for gameplay balance
  * @last_documented: 2025-09-18
 */
void
newdip (int ntn1, int ntn2)
{
	if( ispc( ntn[ntn1].active ) ) {
		if( ntn[ntn2].race==ORC )
			ntn[ntn1].dstatus[ntn2]=HOSTILE;
		else	ntn[ntn1].dstatus[ntn2]=NEUTRAL;
		return;
	}
	if((ntn[ntn1].race==ORC)
	||( ntn[ntn2].race==ORC)) {
		if(ntn[ntn1].dstatus[ntn2]==UNMET) {
			if((rand()%2==0)|| ispc(ntn[ntn1].active))
				ntn[ntn1].dstatus[ntn2]=HOSTILE;
			else
				ntn[ntn1].dstatus[ntn2]=WAR;
		}
	} else if(ismonst(ntn[ntn2].active)) {
		ntn[ntn1].dstatus[ntn2]=WAR;
	} else if(ispc(ntn[ntn1].active)) {
		if(ntn[ntn1].dstatus[ntn2]==UNMET)
			ntn[ntn1].dstatus[ntn2]=NEUTRAL;
	} else if(ntn[ntn1].race==ntn[ntn2].race){
		if(rand()%2<1) ntn[ntn1].dstatus[ntn2]=FRIENDLY;
		else ntn[ntn1].dstatus[ntn2]=NEUTRAL;
	} else ntn[ntn1].dstatus[ntn2]=NEUTRAL;
}

#ifdef MONSTER
/*
 * monster - Master monster controller and dynamic spawning system
 *
 * Orchestrates all monster nation AI behavior and manages dynamic monster
 * spawning to maintain game balance. This function coordinates different
 * monster types (nomads, pirates, savages, lizards) and implements a
 * sophisticated army spawning algorithm that scales with map size and
 * current monster population to prevent gameplay stagnation.
 *
 * Parameters:
 *   None (operates on global nation and map data structures)
 *
 * Returns:
 *   void (modifies global game state)
 *
 * Side Effects:
 *   - Executes AI behavior for all active monster nations
 *   - Dynamically spawns new monster armies when population is low
 *   - Modifies army positions, sizes, and unit types
 *   - Updates sector ownership through monster actions
 *   - Uses random number generation for spawning and placement
 *
 * Monster AI Coordination:
 *   - Nomads: Roving cavalry that devastate and capture territory
 *   - Pirates: Naval raiders attacking coastal areas and ships
 *   - Savages: Tribal warriors expanding through wilderness areas
 *   - Lizards: Specialized monster type (delegated to update.c)
 *
 * Dynamic Spawning Algorithm:
 *   - Calculates needed troops based on map size formula: (NUMSECTS/MONSTER)
 *   - Nomad proportion: 5/12 of total with 450 soldiers per army
 *   - Savage proportion: 1/4 of total with 250 soldiers per army
 *   - Spawns armies until quota reached or space exhausted
 *   - 5/8 probability favors nomad spawning over savage spawning
 *
 * Spawning Constraints:
 *   - Nomads: Must spawn on habitable terrain (not water/peaks)
 *   - Savages: Avoid peaks, water, and heavily populated enemy sectors
 *   - Army sizes: Nomads 100-600 soldiers, Savages 100-300 soldiers
 *   - Unit types: Nomads use light cavalry, Savages use racial defaults
 *
 * Testing Notes:
 *   Category: C (System) - Requires full game state and monster subsystems
 *   Approach: System testing with complete world simulation
 *   Key Tests: Monster spawning balance, AI coordination, map scaling
 *   Dependencies: Global ntn[], sct[][], MAXARM, map dimensions
 *   Mock Requirements: Complete world state with nations and sectors
 *   Complexity: Complex - Coordinates multiple AI subsystems and dynamic content
 *
 * Notes:
 *   - Requires MONSTER compilation flag to be active
 *   - MORE_MONST flag enables advanced dynamic spawning
 *   - Critical for maintaining game challenge and preventing stagnation
 *   - Spawning formula maintains balance across different map sizes
 *   - Uses goto statements for army slot allocation (legacy pattern)
  * @last_documented: 2025-09-18
 */
void
monster (void)
{
	for(country=1;country<NTOTAL;country++) {
		curntn = &ntn[country];
		if( curntn->active==NPC_NOMAD ) do_nomad();
		else if( curntn->active==NPC_PIRATE ) do_pirate();
		else if( curntn->active==NPC_SAVAGE ) do_savage();
		else if( curntn->active==NPC_LIZARD ) do_lizard(); /* update.c */
	}
#ifdef MORE_MONST
	{
	int neededtroops;
	int actualtroops;
	int savages = -1, nomads = -1;
	int i,x,y;
	int nomad_space=1, savage_space=1;

	for(i=1;i<NTOTAL;i++)
	    switch(ntn[i].active)
		{
	    case NPC_SAVAGE:
		savages=i;
		break;
	    case NPC_NOMAD:
		nomads=i;
		break;
		}

				/* neededarmies is an estimete of
				 * the desired military strength of savages
				 * and nomads. The proportions are based on
				 * npc.c routines. yeuch.
				 */

	{
		long temp_troops = safe_double_to_long(((NUMSECTS)/MONSTER)*( /* number of armies */
					    (5.0/12)*450 + /* nomads */
					    (1.0/4)*250 )); /* savages */
		neededtroops = safe_long_to_int(temp_troops);
	}


	actualtroops=0;
	for(i=0;i<MAXARM;i++)
	    {
	    if (ntn[nomads].arm[i].sold > 0)
		actualtroops += safe_long_to_int(ntn[nomads].arm[i].sold);
	    if (ntn[savages].arm[i].sold > 0)
		actualtroops += safe_long_to_int(ntn[savages].arm[i].sold);
	    }

#ifdef DEBUG
	printf("monsters: need %d have %d\n",neededtroops,actualtroops);
#endif /* DEBUG */

	neededtroops= neededtroops-actualtroops;

	while(neededtroops>0&& (nomad_space||savage_space))
	    {
#ifdef DEBUG
	    printf("\t need %d monster troops\n",neededtroops);
#endif /*  */
	    if ( (rand()%8)<5 && nomad_space)
		{
		while(!is_habitable((x=(rand()%(MAPX-8))+4),
				    (y=(rand()%(MAPY-8))+4)))
		    ;
		for(i=0;i<MAXARM;i++)
		    if ( ntn[nomads].arm[i].sold <=0 )
			goto found_free_nomad_army;

#ifdef DEBUG
		printf("\t\t Out of nomad space\n");
#endif
		nomad_space=0;
		continue;

	    found_free_nomad_army:

		ntn[nomads].arm[i].xloc=safe_int_to_uchar(x);
		ntn[nomads].arm[i].yloc=safe_int_to_uchar(y);
		ntn[nomads].arm[i].sold=100+100*(rand()%6);
		ntn[nomads].arm[i].unittyp=A_LT_CAV;
		ntn[nomads].arm[i].stat=ATTACK;
		neededtroops -= safe_long_to_int(ntn[nomads].arm[i].sold);
#ifdef DEBUG
		printf("\t\tAdding nomad army %d size %ld at (%d,%d)\n",i,
		       ntn[nomads].arm[i].sold,x,y);
#endif
		}
	    else
		{
		do {
		x=(rand()%(MAPX-8))+4;
		y=(rand()%(MAPY-8))+4;
		} while ( sct[x][y].altitude == PEAK ||
			  sct[x][y].altitude == WATER ||
			 ( sct[x][y].owner !=0 &&
			   sct[x][y].owner != savages &&
			   sct[x][y].people >= 50 ) );

		for(i=0;i<MAXARM;i++)
		    if ( ntn[savages].arm[i].sold <=0 )
			goto found_free_savage_army;

#ifdef DEBUG
		printf("\t\t Out of savage space\n");
#endif
		savage_space=0;
		continue;

	    found_free_savage_army:

		ntn[savages].arm[i].xloc=safe_int_to_uchar(x);
		ntn[savages].arm[i].yloc=safe_int_to_uchar(y);
		ntn[savages].arm[i].sold=100+100*(rand()%3);
		ntn[savages].arm[i].unittyp=safe_long_to_uchar(defaultunit(savages));
		ntn[savages].arm[i].stat=ATTACK;
		neededtroops -= safe_long_to_int(ntn[savages].arm[i].sold);
#ifdef DEBUG
		printf("\t\tAdding savage army %d size %ld at (%d,%d)\n",i,
		       ntn[savages].arm[i].sold,x,y);
#endif
		}
	    }
	}
#endif /* MORE_MONST */

}

/*
 * do_nomad - Execute nomad AI behavior for roving cavalry devastation
 *
 * Implements the artificial intelligence for nomad nations, which are
 * characterized by constant movement, territorial capture, and systematic
 * devastation of conquered lands. Nomads represent roving cavalry forces
 * that grow over time, never stay in one place, and leave devastation
 * in their wake while spreading across the map.
 *
 * Parameters:
 *   None (uses global country and curntn for current nomad nation)
 *
 * Returns:
 *   void (modifies global game state)
 *
 * Side Effects:
 *   - Moves all nomad armies to adjacent sectors
 *   - Captures undefended or unowned territory
 *   - Devastates captured sectors (reduces fertility/resources)
 *   - Causes population to flee from captured areas
 *   - Increases army sizes by 2% per turn (growth mechanic)
 *   - Generates news reports for territorial captures
 *   - Destroys armies that become trapped (>100 failed moves)
 *
 * Nomad AI Behavior:
 *   - Constant Movement: Cannot remain in the same sector
 *   - Random Direction: Moves to adjacent sectors (3x3 grid around current)
 *   - Habitat Restrictions: Only moves to habitable land (no water/peaks)
 *   - Reachability: Respects movement constraints and terrain
 *   - Territory Capture: Takes undefended sectors from other nations
 *   - Devastation: Systematically destroys captured sector resources
 *   - Growth: Armies increase in size by 2% each turn
 *
 * Capture Logic:
 *   - Targets unowned sectors or sectors without defending soldiers
 *   - Avoids capturing from other nomad nations
 *   - Forces civilian population to flee before capture
 *   - Immediately devastates captured territory
 *   - Updates sector ownership to nomad nation
 *
 * Movement Algorithm:
 *   - Attempts random movement within 3x3 grid around current position
 *   - Validates movement constraints (map bounds, terrain, reachability)
 *   - Destroys army if unable to find valid move after 100 attempts
 *   - Uses land_reachp() to verify movement feasibility
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires army, sector, and movement systems
 *   Approach: Integration testing with mock world state
 *   Key Tests: Movement validation, capture mechanics, devastation effects
 *   Dependencies: Global sct[][], ntn[], movement functions, news system
 *   Mock Requirements: Map sectors, armies, movement calculations
 *   Complexity: Moderate - Movement AI with capture and growth mechanics
 *
 * Notes:
 *   - Critical for nomad challenge and map dynamics
 *   - Growth mechanic prevents nomads from being eliminated early
 *   - Devastation creates permanent map changes affecting late game
 *   - Movement failure protection prevents infinite loops
 *   - Uses P_A* macros for army data access (legacy convenience macros)
  * @last_documented: 2025-09-18
 */
void
do_nomad (void)
{
	int	count;
	short	armynum;
	int	x, y;

	/*move nomads */
	printf("updating nomad (nation %d)\n",country);
	for(armynum=0;armynum<MAXARM;armynum++) if(P_ASOLD>0){
		P_ASTAT=ATTACK;
		P_AMOVE=safe_int_to_uchar((curntn->maxmove * *(unitmove+P_ATYPE%UTYPE))/10);
		if(P_ATYPE<MINLEADER) {
			P_ASOLD *= 102;
			P_ASOLD /= 100;
		}
		count=0;
		while( TRUE ) {
			x=(int)P_AXLOC+rand()%3-1;
			y=(int)P_AYLOC+rand()%3-1;

			if( count++ > 100 ) {
				P_ASOLD=0;
				break;
			}
			/* nomads cannot stay in the same spot */
			if(( x==P_AXLOC && y==P_AYLOC )
			||(!ONMAP(x,y)
			||(!is_habitable(x,y)))
			||(!land_reachp((int)P_AXLOC,(int)P_AYLOC,
				x,y,curntn->maxmove,country))) continue;

			P_AXLOC=safe_int_to_uchar(x);
			P_AYLOC=safe_int_to_uchar(y);
			/*if owned & unoccupied you take & people flee*/
			if( ((sct[x][y].owner) == 0
			|| solds_in_sector( x, y, sct[x][y].owner) == 0 )
			&& (ntn[sct[x][y].owner].active!=NPC_NOMAD) ) {
				fprintf(fnews,"3:\tnomads capture sector %d,%d\n",x,y);
				if(sct[x][y].owner!=0) flee(x,y,1,FALSE);
				sct[x][y].owner=safe_short_to_uchar(country);
				DEVASTATE(x,y)
			}
			break;
		}
	}
}

/*
 * do_savage - Execute savage AI behavior for tribal warrior expansion
 *
 * Implements the artificial intelligence for savage nations, which represent
 * tribal warrior societies that expand through wilderness areas with
 * aggressive territorial acquisition. Savages are more selective than nomads
 * in their movement but equally devastating in their conquest, representing
 * organized tribal expansion rather than pure nomadic wandering.
 *
 * Parameters:
 *   None (uses global country and curntn for current savage nation)
 *
 * Returns:
 *   void (modifies global game state)
 *
 * Side Effects:
 *   - Moves savage armies to adjacent suitable territories
 *   - Captures undefended or unowned territory selectively
 *   - Devastates captured sectors (reduces fertility/resources)
 *   - Causes population to flee from captured areas
 *   - Increases army sizes by 2% per turn (growth mechanic)
 *   - Generates news reports for territorial captures
 *   - More conservative movement than nomads (single attempt per turn)
 *
 * Savage AI Behavior:
 *   - Selective Movement: Single movement attempt per turn (vs nomad persistence)
 *   - Random Direction: Moves to adjacent sectors (3x3 grid around current)
 *   - Habitat Restrictions: Only moves to habitable land (no water/peaks)
 *   - Reachability: Respects movement constraints and terrain limitations
 *   - Territory Capture: Takes undefended sectors from other nations
 *   - Devastation: Systematically destroys captured sector resources
 *   - Growth: Armies increase in size by 2% each turn
 *
 * Capture Logic:
 *   - Targets unowned sectors or sectors without defending soldiers
 *   - Avoids capturing from other savage nations (tribal solidarity)
 *   - Only leaders can actually claim territory ownership
 *   - All units can devastate regardless of capture ability
 *   - Forces civilian population to flee before capture
 *
 * Movement vs Nomad Differences:
 *   - Single Move: One movement attempt per turn (nomads retry until success)
 *   - Stay Put: Can remain in same location if movement fails
 *   - Selective: More deliberate expansion pattern
 *   - Leader Rules: Only leaders can claim sectors, all can devastate
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires army, sector, and movement systems
 *   Approach: Integration testing with mock world state
 *   Key Tests: Movement patterns, capture mechanics, leader-based claiming
 *   Dependencies: Global sct[][], ntn[], movement functions, news system
 *   Mock Requirements: Map sectors, armies, leader units, movement calculations
 *   Complexity: Moderate - Movement AI with selective capture mechanics
 *
 * Notes:
 *   - More conservative expansion pattern than nomads
 *   - Leader-based territorial claiming creates strategic depth
 *   - Devastation occurs regardless of successful territorial claim
 *   - Growth mechanic maintains savage threat throughout game
 *   - Uses P_A* macros for army data access (legacy convenience macros)
  * @last_documented: 2025-09-18
 */
void
do_savage (void)
{
	short armynum;
	int x, y;

	printf("updating savage (nation %d)\n",country);
	for(armynum=0;armynum<MAXARM;armynum++) if(P_ASOLD>0){
		P_ASTAT=ATTACK;
		if(P_ATYPE<MINLEADER) {
			P_ASOLD *= 102;
			P_ASOLD /= 100;
		}
		P_AMOVE=safe_int_to_uchar((curntn->maxmove * *(unitmove+P_ATYPE%UTYPE))/10);
		x=(int)P_AXLOC+rand()%3-1;
		y=(int)P_AYLOC+rand()%3-1;
		if(ONMAP(x,y)&&(is_habitable(x,y))
		&&(land_reachp((int)P_AXLOC,(int)P_AYLOC,x,y,P_AMOVE,country))){
			P_AXLOC=safe_int_to_uchar(x);
			P_AYLOC=safe_int_to_uchar(y);
			/*if owned & unoccupied you take & people flee*/
			if( ((sct[x][y].owner == 0)
			|| (solds_in_sector( x, y, sct[x][y].owner) == 0))
			&& (ntn[sct[x][y].owner].active != NPC_SAVAGE)) {
				fprintf(fnews,"3:\tsavages capture sector %d,%d\n",x,y);
				if(P_ATYPE<MINLEADER) {
					if(sct[x][y].owner!=0) flee(x,y,1,FALSE);
					sct[x][y].owner=safe_short_to_uchar(country);
				}
				DEVASTATE(x,y)
			}
		}
	}
}

/*
 * do_pirate - Execute pirate AI behavior for naval raiding and fleet expansion
 *
 * Implements the artificial intelligence for pirate nations, which operate
 * naval fleets from base camps to raid other nations' shipping and coastal
 * areas. Pirates represent maritime threats that hunt for enemy fleets
 * within their operational zones and dynamically expand their naval power
 * through successful raids and random fleet augmentation.
 *
 * Parameters:
 *   None (uses global country and curntn for current pirate nation)
 *
 * Returns:
 *   void (modifies global game state)
 *
 * Side Effects:
 *   - Repositions pirate fleets to hunt enemy vessels
 *   - Maintains proximity to base camps for operational support
 *   - Randomly expands fleet size with new warships (6.67% chance per turn)
 *   - Tracks and pursues enemy naval forces within operational zones
 *   - Generates debug output for fleet movements and base operations
 *
 * Pirate AI Behavior:
 *   - Base Operations: Maintains connection to base camps (DBASECAMP sectors)
 *   - Fleet Hunting: Actively seeks enemy fleets within PRTZONE radius
 *   - Target Priority: Hunts warships, merchant vessels, and galleys equally
 *   - Zone Control: Operates within PRTZONE distance from base camps
 *   - Fleet Growth: Randomly adds warships to maintain pressure
 *
 * Base Camp Logic:
 *   - Searches PRTZONE radius around fleet for base camps
 *   - Returns fleets to base if found within operational area
 *   - Reports fleets that have strayed too far from bases
 *   - Validates base camp existence and repositions fleets accordingly
 *
 * Fleet Hunting Algorithm:
 *   - Scans all nations for active naval forces
 *   - Identifies enemy fleets within PRTZONE operational radius
 *   - Moves pirate fleet to intercept enemy positions
 *   - Prioritizes any vessel type (warships, merchants, galleys)
 *   - Maintains aggressive pursuit within operational constraints
 *
 * Fleet Expansion Mechanics:
 *   - 6.67% chance per fleet per turn to add new warship (rand()%15==0)
 *   - Random warship size selection (light to heavy warship range)
 *   - Uses NADD_WAR macro for standardized ship addition
 *   - Expansion only occurs when MORE_MONST compilation flag enabled
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires naval, sector, and nation systems
 *   Approach: Integration testing with mock naval state and base camps
 *   Key Tests: Base proximity, fleet hunting, expansion mechanics
 *   Dependencies: Global ntn[], sct[][], naval arrays, base camp sectors
 *   Mock Requirements: Naval units, base camps, enemy fleets, map coordinates
 *   Complexity: Moderate - Naval AI with base operations and target acquisition
 *
 * Notes:
 *   - Most complex monster AI due to naval operations and base coordination
 *   - PRTZONE defines operational radius for pirate activities
 *   - Fleet expansion maintains pirate threat throughout game progression
 *   - Base camp dependency creates strategic vulnerabilities for pirates
 *   - Uses P_N* macros for naval data access (legacy convenience macros)
 *   - Requires both MONSTER and MORE_MONST flags for full functionality
  * @last_documented: 2025-09-18
 */
void
do_pirate (void)
{
	short nvynum,shipsize;
	int x, y, campx, campy;

	printf("updating pirate (nation %d)\n",country);

	/* if pirate fleet within 3 attack if outnumber any fleets */
	/* automatically find their base first */
	for(nvynum=0;nvynum<MAXNAVY;nvynum++) if(P_NWSHP!=0) {
		int temp=TRUE;

		campx=P_NXLOC; campy=P_NYLOC;
		for(x=P_NXLOC-PRTZONE;x<=P_NXLOC+PRTZONE;x++)
		for(y=P_NYLOC-PRTZONE;y<=P_NYLOC+PRTZONE;y++) {
			if((ONMAP(x,y)) && sct[x][y].designation==DBASECAMP ) {
				temp=FALSE;
				campx=x;
				campy=y;
			}
		}
		if(temp==TRUE) {
			fprintf(stderr,"Pirate fleet %d away from base\n",nvynum);
		} else {
			if (sct[campx][campy].designation!=DBASECAMP) {
				fprintf(stderr,"BASECAMP NOT FOUND!!!\n");
			} else {
				P_NXLOC=safe_int_to_uchar(campx);
				P_NYLOC=safe_int_to_uchar(campy);
			}
		}
	}

	for(nvynum=0;nvynum<MAXNAVY;nvynum++) if (P_NWSHP!=0) {
		for(x=1;x<NTOTAL;x++) if(isntn(ntn[x].active))
		for(y=0;y<MAXNAVY;y++)
		if(ntn[x].nvy[y].warships!=0 || ntn[x].nvy[y].merchant!=0
		  || ntn[x].nvy[y].galleys!=0) {
			if((abs((int)ntn[x].nvy[y].xloc-(int)P_NXLOC)<=PRTZONE)
			&&(abs((int)ntn[x].nvy[y].yloc-(int)P_NYLOC)<=PRTZONE)) {
				P_NXLOC= ntn[x].nvy[y].xloc;
				P_NYLOC= ntn[x].nvy[y].yloc;
			}
		}
#ifdef MORE_MONST
		if(rand()%15==0) {
			/*randomly add one warship to pirate fleet*/
			shipsize = safe_rand_short(N_HEAVY-N_LIGHT+1);
			(void) NADD_WAR(1)
		}
#endif /* MORE_MONST */
	}
}
#endif /* MONSTER */

#ifdef NPC
void n_redes (int x, int y, int goldthresh, int metalthresh, int citythresh, double hunger) {
	register struct s_sector	*sptr = &sct[x][y];

	if((sptr->designation == DCAPITOL)
	||(sptr->designation == DCITY)) return;

	/*large enough for a city now?*/
	if(((sptr->people > (spread.civilians/CITYLIMIT))
	||((spread.civilians<30000)&&(sptr->people>1000)))
	&&( hunger > P_EATRATE*1.5 )
	&&( spread.incity+spread.incap < spread.civilians * CITYPERCENT / 100)
	&&( spread.sectors > 10)
	&&( sptr->tradegood == TG_none )){
		sptr->designation=DTOWN;
		spread.incity+=sptr->people;
		spread.infarm-=sptr->people;
	}

	/* large enough for city and not enough food*/
	if((sptr->designation==DTOWN)
	&&( hunger < P_EATRATE)
	&&( tofood(sptr,sptr->owner) > citythresh )){
		sptr->designation=DFARM;
		spread.incity-=sptr->people;
		spread.infarm+=sptr->people;
	}

	if((sptr->designation==DTOWN)
	&&( spread.incity+spread.incap > spread.civilians * CITYPERCENT / 66)){
		sptr->designation=DFARM;
		spread.incity-=sptr->people;
		spread.infarm+=sptr->people;
	}

	/*what if it is not a city*/
	if((sptr->designation!=DTOWN)
	&&(sptr->designation!=DCITY)
	&&(sptr->designation!=DCAPITOL)){
		if(( sptr->tradegood != TG_none )
		&&( tg_ok( sptr->owner, sptr ))) {
			if(( metalthresh+goldthresh > 8 )
			||(( sptr->metal < metalthresh )
			  &&( sptr->metal != 0 ))
			||(( sptr->jewels < goldthresh )
			  &&( sptr->jewels != 0 ))) {
				sptr->designation = DFARM;
			} else	sptr->designation= *(tg_stype+sptr->tradegood);

			if(( sptr->metal < metalthresh )
			  &&( sptr->metal != 0 ))
				sptr->designation=DBLKSMITH;

			if(( sptr->designation== 'x' )
			||(( sptr->designation== DCITY )&&(sptr->people<1000)))
				sptr->designation=DFARM;
		} else if( tofood(sptr,sptr->owner) >= 4 ){
			sptr->designation=DFARM;
		} else	sptr->designation=DSTOCKADE;
	}
	if(( sptr->designation==DFARM)
	&&( hunger > P_EATRATE*1.5 )
	&&(  tofood(sptr,sptr->owner) <= 6 )){
		if(( rand()%2 == 0 )&&( curntn->mine_ability<30 ))
			sptr->designation=DBLKSMITH;
		else if(( rand()%2 == 0 )
		&&( sptr->people<100 )
		&&( curntn->spoilrate >15))
			sptr->designation=DGRANARY;
		else if(( rand()%2 == 0 )&&( curntn->popularity<50 ))
			sptr->designation=DCHURCH;
		else if( sptr->people>1000 )
			sptr->designation=DTOWN;
	}
}

/*
 * redomil - Military reorganization and force allocation algorithms
 *
 * Implements comprehensive military management system that handles all aspects
 * of NPC military organization including garrison management, army sizing,
 * resource allocation, unit creation/disbanding, and strategic force distribution.
 * This function is the core of NPC military intelligence, maintaining optimal
 * military forces based on economic conditions, threat levels, and strategic needs.
 *
 * The algorithm manages multiple military subsystems:
 * 1. Naval crew allocation with randomized assignments
 * 2. Army positioning validation and capitol garrison management
 * 3. Militia validation and elimination of invalid units
 * 4. Garrison sizing based on ideal peace/war ratios
 * 5. Army creation, disbanding, splitting, and merging operations
 * 6. City militia deployment for urban defense
 * 7. Unit type standardization and equipment management
 *
 * Parameters:
 *   void (operates on global curntn and military data structures)
 *
 * Returns:
 *   void (modifies military arrays and nation statistics directly)
 *
 * Side Effects:
 *   - Modifies army structures: P_ASOLD, P_ATYPE, P_ASTAT, P_AXLOC, P_AYLOC
 *   - Updates nation totals: curntn->tmil, curntn->tciv
 *   - Adjusts economic resources: curntn->tgold, curntn->metals
 *   - Changes sector populations for recruitment/disbanding
 *   - Repositions armies and validates unit placement
 *   - Creates/destroys military units based on strategic needs
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires nation data, army arrays, sector information
 *   Approach: Integration testing with mock military and economic scenarios
 *   Key Tests: Garrison sizing, army creation/destruction, resource management
 *   Dependencies: Global nation state, army arrays, sector data, unit cost tables
 *   Mock Requirements: Mock nation with various economic/military conditions
 *   Complexity: Complex - Multi-system military management with resource constraints
 *
 * Military Management Algorithm Details:
 *   - Naval Crews: 50% chance to assign SHIPCREW to ships with randomization
 *   - Garrison Ratio: ideal = tmil * peace / (10*MILINCAP) for capitol defense
 *   - Army Thresholds: TAKESECTOR minimum, 2*TAKESECTOR maximum for splitting
 *   - Economic Limits: Resource availability constrains recruitment and equipment
 *   - Unit Validation: Eliminates invalid militia, repositions distant armies
 *   - City Defense: Automatic militia deployment in all cities and towns
 *
 * Strategic Resource Management:
 *   - Gold Cost: Unit recruitment costs modified by WARRIOR magic power (50% discount)
 *   - Metal Requirements: Unit equipment costs based on unit type and metal tables
 *   - Population Limits: Recruitment limited by available civilian population
 *   - Economic Constraints: Negative resources reduce ideal military sizes
 *   - Leader Management: Automatic leader placement in capitol with RULE status
 *
 * Notes:
 *   - Called after economic redesignation to balance military with resources
 *   - Uses complex peace/war calculations for optimal force sizing
 *   - Integrates with magic system for cost modifications
 *   - Critical for NPC military competitiveness and strategic effectiveness
 *   - Extensive debug output available for military analysis
  * @last_documented: 2025-09-19
 */
void
redomil (void)
{
	short x,y,armynum,nvynum;
	int i, free, done;
	long ideal;
	long diff=0l;
	int ok;

	check()
	/* check out any ship crews */
	for(nvynum=1;nvynum<MAXNAVY;nvynum++) {
		/* definite cheat -- add some random */
		if((P_NMSHP!=0)||(P_NWSHP!=0)||(P_NGSHP != 0))
			if(rand()%2==0) P_NCREW = SHIPCREW;
	}
	check()
	curntn->tmil = 0L;
	for(armynum=1;armynum<MAXARM;armynum++) if(P_ASOLD>0){
		/* move army back if too far out */
		ok = 0;
		if (P_ATYPE < MINLEADER && P_ASOLD!=A_MILITIA) {
			curntn->tmil += P_ASOLD;
		}
		for(x=safe_int_to_short((int)P_AXLOC-3);x<=safe_int_to_short((int)P_AXLOC+3);x++)
			for(y=safe_int_to_short((int)P_AYLOC-3);y<=safe_int_to_short((int)P_AYLOC+3);y++)
				if((ONMAP(x,y))&&(sct[x][y].owner==country)) ok=1;
		if(ok==0){
			P_AXLOC=curntn->capx;
			P_AYLOC=curntn->capy;
		}

		/* verify militia */
		if(P_ATYPE==A_MILITIA) {
			/* eliminate invalid militia */
			if(((sct[P_AXLOC][P_AYLOC].designation!=DTOWN)
			&&(sct[P_AXLOC][P_AYLOC].designation!=DCAPITOL)
			&&(sct[P_AXLOC][P_AYLOC].designation!=DCITY))
			||(sct[P_AXLOC][P_AYLOC].owner!=country)){
#ifdef DEBUG
				printf("\teliminating %s army %d as %d %d is des:%c alt:%c own:%d\n",unittype[P_ATYPE],armynum,P_AXLOC,P_AYLOC,sct[P_AXLOC][P_AYLOC].designation,sct[P_AXLOC][P_AYLOC].altitude,sct[P_AXLOC][P_AYLOC].owner);
#endif /* DEBUG */
				if(sct[P_AXLOC][P_AYLOC].owner == country)
					sct[P_AXLOC][P_AYLOC].people+=P_ASOLD;
				else sct[curntn->capx][curntn->capy].people+=P_ASOLD;
				P_ASOLD=0;
			}
		}

		/* set default status */
		if(P_ASTAT<NUMSTATUS) switch(P_ASTAT) {
		case MILITIA:
		case SIEGED:
		case TRADED:
		case ONBOARD:
		case GENERAL:
		case SCOUT:
			break;
		default:
			P_ASTAT=DEFEND;
			break;
		}
	}

	/*make sure enough men in army 0 -- garrison duty in capitol*/
	armynum=0;
	/* find lowest army of non-leader type */
	while (armynum < MAXARM && (P_ATYPE>=MINLEADER && P_ASOLD>0)) {
		armynum++;
	}
	/* oh well... wipe out army zero; c'est la vie */
	if (armynum == MAXARM) armynum = 0;
	P_ATYPE=safe_long_to_uchar(defaultunit(country));
	P_ASTAT=GARRISON;
	P_AXLOC=curntn->capx;
	P_AYLOC=curntn->capy;

	/*Ideally P_ASOLD[0]*MILINCAP=tmil*peace/10*/
	ideal = curntn->tmil * peace / (10L*MILINCAP);
	if(curntn->tgold < 0) ideal/=2L;
#ifdef DEBUG
	if(peace==8)
		printf("\t%s IS AT PEACE - garrison in cap is %ld, ideal is %ld\n",curntn->name,P_ASOLD,ideal);
	else if(peace==12)
		printf("\t%s IS AT WAR - garrison in cap is %ld, ideal is %ld\n",curntn->name,P_ASOLD,ideal);
	else printf("error - incap is %ld ideal is %ld\n",P_ASOLD,ideal);
#endif /* DEBUG */

	/*MILRATIO ratio mil:civ for non player countries*/
	/*MILINCAP ratio (mil in cap):mil for NPCs*/
	check()

	if((P_ASOLD*10) < (9*ideal)){

	/*too few soldiers on garrison*/
	/*diff is number to change mil in cap (>0)*/
	if (*(u_enmetal + (P_ATYPE%UTYPE)) > 0)
	diff = (long) min(ideal-P_ASOLD,(int) (curntn->metals / *(u_enmetal + (P_ATYPE%UTYPE))));

	diff=(long) min((int) diff, sct[curntn->capx][curntn->capy].people/2L);

	if ((curntn->tgold<0L || curntn->metals<0L) && (diff > 0L)) diff=0L;
	if (sct[curntn->capx][curntn->capy].owner != country) diff=0L;

#ifdef DEBUG
	printf("\tadding %ld men to garrison (too few men on garrison)\n",diff);
#endif /* DEBUG */

	sct[curntn->capx][curntn->capy].people-=diff;
	P_ASOLD+=diff;
	curntn->tciv-=diff;
	curntn->tmil+=diff;
	if(magic(country,WARRIOR)==1)	/* take WARRIOR power into account */
		curntn->tgold -= (diff * *(u_encost + (P_ATYPE%UTYPE))) / 2;
	else curntn->tgold-=diff* *(u_encost + (P_ATYPE%UTYPE));
	curntn->metals-=(diff* *(u_enmetal + (P_ATYPE%UTYPE)));
#ifdef DEBUG
	if(P_ASOLD < 0L) printf("error 2... P_ASOLD=%ld <0\n",P_ASOLD);
#endif /* DEBUG */
	}
	/*else split garrison army if 1.25* needed number*/
	else if(P_ASOLD *4L > 5L*ideal){
		/*diff here is a negative number*/
		diff=((4L*P_ASOLD)-(5L*ideal))/4L;
#ifdef DEBUG
		printf("\tsplit garrison of %ld men\n",diff);
#endif /* DEBUG */
		free=FALSE;
		P_ASOLD-=diff;
#ifdef DEBUG
		if(P_ASOLD < 0) printf("error... subtracting %ld from %ld\n",diff,P_ASOLD);
#endif /* DEBUG */
		curntn->tmil-=diff;
		curntn->tciv+=diff;
		sct[curntn->capx][curntn->capy].people+=diff;
		/*I add back gold as armies get redone anyway*/
		curntn->metals += (diff* *(u_enmetal + (P_ATYPE%UTYPE)));
		if(magic(country,WARRIOR)==TRUE) { /* WARRIOR power */
			curntn->tgold+=(diff* *(u_encost + (P_ATYPE%UTYPE))) / 2;
		} else curntn->tgold+=diff* *(u_encost + (P_ATYPE%UTYPE));
	}
#ifdef DEBUG
	else printf("\tno action - P_ASOLD (%ld) ~= ideal (%ld)\n",P_ASOLD,ideal);
	printf("\tFinal Garrison Army %d (%s) type is %s men is %ld\n",armynum,curntn->name,*(unittype+(P_ATYPE)),P_ASOLD);
#endif /* DEBUG */

	/*build ships and/or armies*/
	done=FALSE;
	ideal = curntn->tciv * peace / (10 * MILRATIO);
	if(curntn->tgold<0) { ideal*=4; ideal/=5; }
#ifdef DEBUG
	printf("\t%s total military is %ld -> ideal is %ld\n",curntn->name,curntn->tmil,ideal);
#endif /* DEBUG */
	check()

	/* find leader and place on RULE in capitol */
	for(armynum=0;armynum<MAXARM;armynum++)
		if (P_ATYPE==getleader(curntn->class)-1) {
			P_ASTAT=RULE;
			P_AXLOC=curntn->capx;
			P_AYLOC=curntn->capy;
			break;
		}

	/* add to partial armies */
	for(armynum=1;armynum<MAXARM;armynum++)
	if((P_ASOLD>0)
	&&( P_ATYPE!=A_MILITIA )
	&&( P_ATYPE<MINLEADER )
	&&( P_ASOLD < TAKESECTOR )
	&&( curntn->tgold > 0 )
	&&(curntn->metals >= (TAKESECTOR+20-P_ASOLD)* *(u_enmetal + (P_ATYPE%UTYPE)))
	&&( fort_val(&sct[P_AXLOC][P_AYLOC]) > 0)
	&&( sct[P_AXLOC][P_AYLOC].owner == country )) {
#ifdef DEBUG
		printf("\tadding %ld men to weakened army %d\n",TAKESECTOR+20-P_ASOLD,armynum);
#endif /* DEBUG */
		if(magic(country,WARRIOR)==TRUE) /* WARRIOR power */
		curntn->tgold-=((TAKESECTOR+20-P_ASOLD)*
			*(u_encost + (P_ATYPE%UTYPE))) / 2;
		else curntn->tgold-=(TAKESECTOR+20-P_ASOLD)*
			*(u_encost + (P_ATYPE%UTYPE));
		curntn->tmil += TAKESECTOR+20-P_ASOLD;
		curntn->metals -= ((TAKESECTOR+20-P_ASOLD)* *(u_enmetal + (P_ATYPE%UTYPE)));
		P_ASOLD = TAKESECTOR+20;
	}

	/*if < ideal build new army in the capitol - if possible*/
	if(curntn->tmil < ((4*ideal)/5)) {
		for(armynum=1;armynum<MAXARM;armynum++)
		if((done==FALSE)&&(P_ASOLD==0)) {
			done=TRUE;
			P_ATYPE = safe_long_to_uchar(defaultunit(country));
			P_ASOLD = min ((int) (ideal-curntn->tmil), (int) (curntn->metals/ (*(u_enmetal + (P_ATYPE%UTYPE)))));

			P_ASOLD = min (P_ASOLD,sct[curntn->capx][curntn->capy].people/2);
			P_ASOLD = min (P_ASOLD, (int) (curntn->tgold/ *(u_encost+(P_ATYPE%UTYPE))));
			if(P_ASOLD>0){
#ifdef DEBUG
				printf("\tnot enough soldiers - build new army %d with %ld men\n",armynum,P_ASOLD);
#endif /* DEBUG */
				curntn->metals-=(P_ASOLD* *(u_enmetal + (P_ATYPE%UTYPE)));
				P_AXLOC= curntn->capx;
				P_AYLOC= curntn->capy;
				curntn->tmil += P_ASOLD;
				curntn->tciv -= P_ASOLD;
				if(magic(country,WARRIOR)==TRUE) /* WARRIOR power */
					curntn->tgold-=(P_ASOLD* *(u_encost + (P_ATYPE%UTYPE))) / 2;
				else curntn->tgold-=P_ASOLD* *(u_encost + (P_ATYPE%UTYPE));
				sct[P_AXLOC][P_AYLOC].people-=P_ASOLD;
				P_ASTAT= DEFEND;
				P_AMOVE=0;
			}
			else P_ASOLD=0;
		}
		check()
	} else if(curntn->tmil > (6*ideal/5)){
		check()
		/*disband a pseudo-random army*/
		done=FALSE;
		diff=curntn->tmil-(6*ideal/5);
		for(armynum=1;done==FALSE && armynum<MAXARM;armynum++){
			if((P_ASOLD<=0)
			||(P_ATYPE==A_ZOMBIE)
			||(P_ATYPE==A_MILITIA)
			||(P_ATYPE>=MINLEADER)
			||(P_ASTAT==ONBOARD)
			||(P_ASTAT==TRADED)) continue;

			if((sct[P_AXLOC][P_AYLOC].owner==country)
			&&((sct[P_AXLOC][P_AYLOC].jewels>4)
			  ||(sct[P_AXLOC][P_AYLOC].metal>4)
			  ||(ISCITY(sct[P_AXLOC][P_AYLOC].designation)))){
#ifdef DEBUG
				printf("\ttoo many soldiers eliminate army %d (%ld men)\n",armynum,P_ASOLD);
#endif /* DEBUG */
				diff-=P_ASOLD;
				sct[P_AXLOC][P_AYLOC].people+=P_ASOLD;
				curntn->tmil -= P_ASOLD;
				curntn->tciv += P_ASOLD;
				P_ASOLD=0;
				if(diff<=50) done=TRUE;
			}
		}
	}
	check()
#ifdef DEBUG
	printf("\twhew... new tmil is %ld\n",curntn->tmil);
#endif /* DEBUG */

	/*resize armies */
	for(armynum=1;armynum<MAXARM;armynum++) if(P_ATYPE < MINLEADER) {
		/*maximum npc army is 3*TAKESECTOR or 3*tmil/MAXARM */
		/* also let militia get big */
		if((P_ASOLD>(2*TAKESECTOR))
		&&(P_ATYPE!=A_MILITIA && P_ASTAT!=ONBOARD && P_ASTAT!=TRADED)) {
			free=FALSE;
			for(i=1;free==FALSE && i<MAXARM;i++){
				if(curntn->arm[i].sold==0){
					free=TRUE;
					P_ASOLD/=2;
#ifdef DEBUG
	printf("\tSplitting %ld troops from army %d forming %s army %d \n"
		,P_ASOLD,armynum,unittype[P_ATYPE],i);
#endif /* DEBUG */
					curntn->arm[i].sold  = P_ASOLD;
					curntn->arm[i].unittyp = P_ATYPE;
					curntn->arm[i].smove = P_AMOVE;
					curntn->arm[i].stat  = P_ASTAT;
					curntn->arm[i].xloc  = P_AXLOC;
					curntn->arm[i].yloc  = P_AYLOC;
				}
			}
		}
	/*minimum npc army is TAKESECTOR, merge them*/
		else if(P_ASOLD>0 && (P_ASOLD<TAKESECTOR || P_ASTAT==MILITIA)
		&& P_ASTAT!=ONBOARD && P_ASTAT!=TRADED) {
			free=FALSE;
			for(i=1;free==FALSE && i<MAXARM;i++){
				if((curntn->arm[i].sold>0)
				&&(curntn->arm[i].stat!=ONBOARD)
				&&(i!=armynum) 	/* don't use same army */
						/* or it will be deleted */
				&&(curntn->arm[i].stat!=TRADED)
				&&(curntn->arm[i].xloc==P_AXLOC)
				&&(curntn->arm[i].yloc==P_AYLOC)
				&&(curntn->arm[i].unittyp==P_ATYPE)) {
					free=TRUE;
					curntn->arm[i].sold += P_ASOLD;
#ifdef DEBUG
	printf("\tMerge %ld men from army %d to make %ld troops in %s army %d \n"
		,P_ASOLD,armynum,curntn->arm[i].sold,unittype[P_ATYPE],i);
#endif /* DEBUG */
					P_ASOLD=0;
				}

			}
		}
	}
	check()

	/* assure that a militia unit resides in each city */
	if(curntn->tgold > 0)
	for(x=safe_int_to_short(stx);x<endx;x++) for(y=safe_int_to_short(sty);y<endy;y++)
	if((sct[x][y].owner==country)
	&&((sct[x][y].designation==DTOWN)||(sct[x][y].designation==DCITY)||(sct[x][y].designation==DCAPITOL))){
		free=FALSE;
		for(armynum=0;armynum<MAXARM;armynum++){
			if((P_ASOLD>0)
			&&(P_AXLOC==x)&&(P_AYLOC==y)
			&&(P_ATYPE==A_MILITIA)) {
				free=TRUE;
				break;
			}
		}
		if(free==FALSE) {	 /* draft new militia army */
			for(armynum=0;armynum<MAXARM;armynum++) if(P_ASOLD==0){
				P_AXLOC=safe_int_to_uchar(x);
				P_AYLOC=safe_int_to_uchar(y);
				P_ATYPE=A_MILITIA;
				free=TRUE;
				break;
			}
		}
		if(free==TRUE) {
			/* want to have ideal troops */
			ideal = sct[x][y].people/MILINCITY;

			if(ideal < 50)	/* make the militia at least 50 */
				ideal = 50;

			if(ideal>0){
			if(magic(country,WARRIOR)==TRUE){ /* WARRIOR power */
				curntn->tgold-=
					((ideal-P_ASOLD)* *(u_encost+P_ATYPE))/2;
			} else {
				curntn->tgold-=
					(ideal-P_ASOLD)* *(u_encost + P_ATYPE);
			}
			}
#ifdef DEBUG
		printf("\tnow in sector %d,%d\n",x,y);
		printf("\tadding %ld troops to %s army %d (now %ld men - populace %ld)\n",ideal-P_ASOLD,unittype[P_ATYPE%UTYPE],armynum,ideal,sct[x][y].people);
#endif /* DEBUG */
			P_ASOLD=ideal;
			P_ASTAT=MILITIA;
		}
	}
	check()
#ifdef DEBUG
	printf("\tnow setting all units to default type of %s\n",unittype[defaultunit(country)]);
#endif /*DEBUG*/

	/* setup default units */
	for(armynum=1;armynum<MAXARM;armynum++)
	if((P_ASOLD>0)&&(P_ATYPE!=A_MILITIA)&&(P_ATYPE<MINLEADER))
		P_ATYPE=safe_long_to_uchar(defaultunit(country));
}

/*
 * getdstatus - Dynamic diplomatic status management system
 *
 * Implements sophisticated AI diplomacy system that manages international
 * relations between nations based on alignment, military strength, proximity,
 * and historical relationships. This function calculates and updates diplomatic
 * status changes using probabilistic models that simulate realistic diplomatic
 * behavior patterns including alliance formation, hostility escalation, and
 * peace negotiations.
 *
 * The algorithm evaluates multiple diplomatic factors:
 * 1. Alignment compatibility (good/neutral/evil relationships)
 * 2. Military strength ratios and threat assessment
 * 3. Geographic proximity and territorial pressure
 * 4. Racial affinity and cultural relationships
 * 5. Historical diplomatic status and relationship momentum
 * 6. Activity level and diplomatic engagement capacity
 *
 * Parameters:
 *   void (operates on global curntn and diplomatic status arrays)
 *
 * Returns:
 *   void (modifies diplomatic status arrays and generates news/messages)
 *
 * Side Effects:
 *   - Modifies curntn->dstatus[] arrays for all nations
 *   - Generates news reports for diplomatic changes
 *   - Sends mail messages to PC nations about status changes
 *   - May trigger reciprocal diplomatic responses
 *   - Affects strategic AI behavior in subsequent turns
 *   - Influences trade, movement, and military targeting decisions
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires nation data, diplomatic arrays, mail system
 *   Approach: Integration testing with mock diplomatic scenarios and nation configurations
 *   Key Tests: Alignment interactions, military threat responses, proximity effects
 *   Dependencies: Global nation arrays, diplomatic status tracking, mail/news systems
 *   Mock Requirements: Mock nations with various alignments, military strengths, positions
 *   Complexity: Complex - Multi-factor probabilistic diplomatic decision making
 *
 * Diplomatic Algorithm Details:
 *   - Hostility Base: Activity level determines base hostility (5% to 35% range)
 *   - Alignment Modifier: +20% hostility for different alignments
 *   - Racial Bonus: Same race gets +10% friendly, -10% hostile
 *   - Proximity Penalty: Adjacent to capitol increases tensions
 *   - Military Imbalance: 4x military/score advantage triggers hostility
 *   - Treaty Protection: TREATY and JIHAD statuses have special handling
 *   - Ceasefire Negotiation: 20% chance for mutual WAR to become HOSTILE
 *
 * News and Communication System:
 *   - War Declarations: Automatic news announcements and PC mail notifications
 *   - Jihad Announcements: Special religious war declarations with notifications
 *   - Ceasefire Negotiations: Mutual war status can automatically de-escalate
 *   - Diplomatic Momentum: Status changes influenced by opponent's reciprocal status
 *
 * Notes:
 *   - Only processes NPC nations (returns early for PC nations)
 *   - Uses sophisticated probability calculations for realistic diplomatic evolution
 *   - Integrates with mail system for player notification of major changes
 *   - Critical for creating dynamic political landscape and strategic challenges
 *   - Prevents diplomatic status changes for certain protected relationships
  * @last_documented: 2025-09-19
 */
void
getdstatus (void)
{
	int x,oldstat[NTOTAL];
	int X,Y;
	int svhostile,hostile;	/* chance nation will react hostile */
	int friendly;		/* chance nation will react favorably */

	if(!isnpc(curntn->active)) return;

	if(( curntn->active==GOOD_6FREE )
	     ||( curntn->active==ISOLATIONIST )
	     ||( curntn->active==NEUTRAL_6FREE )
	     ||( curntn->active==EVIL_6FREE )) svhostile=5;
	else if(( curntn->active==GOOD_4FREE )
	     ||( curntn->active==NEUTRAL_4FREE )
	     ||( curntn->active==EVIL_4FREE )) svhostile=10;
	else if(( curntn->active==GOOD_2FREE )
	     ||( curntn->active==NEUTRAL_2FREE )
	     ||( curntn->active==EVIL_2FREE )) svhostile=20;
	else if(( curntn->active==GOOD_0FREE )
	     ||( curntn->active==NEUTRAL_0FREE )
	     ||( curntn->active==EVIL_0FREE )) svhostile=35;
	else svhostile=5;

	for(x=1;x<NTOTAL;x++) if( isntn(ntn[x].active) ){
		hostile = svhostile;
		if(npctype(curntn->active) != npctype(ntn[x].active))
			hostile+=20;	/* not same alignment */
		friendly = 60-hostile;
	     	if( curntn->active==ISOLATIONIST ) friendly -= 20;
		/* negate impact of above line on neutrals */
		if(isneutral(ntn[x].active)) {
			friendly-=10;
			hostile-=10;
		}
		if(ntn[x].race==curntn->race){
			friendly+=10;
			hostile-=10;
		}
		if(isneutral(curntn->active)) {
			friendly-=20;
			hostile-=20;
		}
		/* if next to capitol, they dont like you */
		for(X=(int)curntn->capx-1;X<=(int)curntn->capx+1;X++)
		for(Y=(int)curntn->capy-1;Y<=(int)curntn->capy+1;Y++)
		if(ONMAP(X,Y)) {
			if(sct[X][Y].owner == x) {
				friendly-=10;
				hostile +=10;
			}
		}

		if( friendly < 0 )	friendly=0;
		if( hostile < 0 )	hostile=0;

		oldstat[x] = curntn->dstatus[x];

		/* break bad treaties */
		if(curntn->dstatus[x] == TREATY) {
			if(ntn[x].dstatus[country]>=WAR)
				curntn->dstatus[x] = JIHAD;
			continue;
		}

		if((curntn->dstatus[x] == JIHAD)
		||(curntn->dstatus[x]==UNMET)
		||(ispc(curntn->active)))
			continue;

		/*if 4* mil and 4* score then not like them*/
		if((ntn[x].tmil>4*curntn->tmil)
		&&(ntn[x].score>4*curntn->score)){
			if(curntn->dstatus[x]<WAR){
				if(rand()%100<=hostile)
					curntn->dstatus[x]++;
			}
		}
		/*adjust based on your status with them*/
		if((curntn->dstatus[x]==WAR)
		&&(ntn[x].dstatus[country]<WAR))
			if(rand()%100<=friendly) curntn->dstatus[x]--;

		if((curntn->dstatus[x]<WAR)
		&&(curntn->dstatus[x]>ALLIED)){
			if(ntn[x].dstatus[country]>1+curntn->dstatus[x]){
				if(rand()%100<=hostile)
					curntn->dstatus[x]++;
			} else if(ntn[x].dstatus[country]+1<curntn->dstatus[x]){
				if(rand()%100<=friendly)
					curntn->dstatus[x]--;
			}
		}
		if(rand()%100<= hostile) {
			if((curntn->dstatus[x]!=JIHAD)
			&&(curntn->dstatus[x]!=TREATY))
				curntn->dstatus[x]++;
		}
		if((rand()%100<= friendly)
		&&(curntn->dstatus[x]!=TREATY)
		&&(curntn->dstatus[x]!=JIHAD)
		&&(curntn->dstatus[x]!=WAR)) curntn->dstatus[x]--;
	}

	for(x=1;x<NTOTAL;x++) if(isntn( ntn[x].active ) ){
		if((rand()%5==0)
		&&(ntn[x].dstatus[country]==WAR)
		&&(curntn->dstatus[x]==WAR)) {
			ntn[x].dstatus[country]=HOSTILE;
			curntn->dstatus[x]=HOSTILE;
			fprintf(fnews,"2.\tnation %s and %s announce ceasefire\n",curntn->name,ntn[x].name);
			if( isnotpc(ntn[x].active) ) continue;
			if (mailopen(x)!=(-1)) {
				fprintf(fm,"Message from Conquer\n\n");
				fprintf(fm,"Nation %s and you negotiate a ceasefire\n",curntn->name);
				mailclose(x);
			}
		} else if((oldstat[x]==WAR)&&(curntn->dstatus[x]==WAR)){
			fprintf(fnews,"2.\tnation %s stays at war with %s\n",curntn->name,ntn[x].name);
		} else if((oldstat[x]<WAR)&&(curntn->dstatus[x]==WAR)){
			fprintf(fnews,"2.\tnation %s goes to war with %s\n",curntn->name,ntn[x].name);
			if( isnotpc(ntn[x].active) ) continue;
			if (mailopen(x)!=(-1)) {
				fprintf(fm,"Message from Conquer\n\n");
				fprintf(fm,"Nation %s goes to war with you\n",curntn->name);
				mailclose(x);
			}
		} else if((oldstat[x]!=JIHAD)&&(curntn->dstatus[x]==JIHAD)){
			fprintf(fnews,"2.\tnation %s announces a jihad with %s\n",curntn->name,ntn[x].name);
			if( isnotpc(ntn[x].active) ) continue;
			if(mailopen(x)!=(-1)) {
				fprintf(fm,"Message from Conquer\n\n");
				fprintf(fm,"nation %s announces a jihad with you\n",curntn->name);
				mailclose(x);
			}
		}
	}
}


/*
 * find_avg_sector - Calculate world averages for AI decision support
 *
 * Computes critical world statistics that support AI strategic decision making
 * by calculating average food production, tradegood values, and military density
 * across the game world. These baseline statistics are used by AI systems to
 * evaluate unknown sectors, estimate enemy capabilities, and make informed
 * strategic decisions about expansion, defense, and resource prioritization.
 *
 * The function performs two major statistical calculations:
 * 1. World Resource Averages: Average food and tradegood values per useable sector
 * 2. Military Density Analysis: Average soldiers per sector for each nation
 *
 * These calculations enable AI systems to:
 * - Estimate value of unexplored or unseen sectors
 * - Gauge relative military strength of other nations
 * - Make informed decisions about territorial expansion priorities
 * - Assess strategic value of different world regions
 *
 * Parameters:
 *   void (operates on global map and nation data structures)
 *
 * Returns:
 *   void (updates global Avg_food, Avg_tradegood, and Avg_soldiers arrays)
 *
 * Side Effects:
 *   - Sets global Avg_food (average food production per useable sector)
 *   - Sets global Avg_tradegood (average tradegood value per useable sector)
 *   - Updates Avg_soldiers[nation] for all nations (soldiers per occupied sector)
 *   - Provides baseline statistics for AI strategic decision algorithms
 *   - Supports unseen sector evaluation and military assessment systems
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires complete world map and nation data
 *   Approach: Integration testing with various world configurations and nation states
 *   Key Tests: Empty world handling, resource distribution calculations, military density
 *   Dependencies: Global map data, nation military arrays, sector ownership information
 *   Mock Requirements: Mock world map with sectors, nations with armies and territories
 *   Complexity: Moderate - Statistical calculation with map traversal and army analysis
 *
 * Statistical Calculation Details:
 *   - Useable Land: Excludes WATER and PEAK altitude sectors
 *   - Food Calculation: Uses tofood() function for current nation perspective
 *   - Tradegood Valuation: Metal/jewels=500, other tradegoods=300 value points
 *   - Military Density: Total military divided by unique sectors occupied
 *   - Duplicate Prevention: Avoids counting multiple armies in same sector twice
 *
 * World Analysis Algorithm:
 *   - Complete Map Scan: Evaluates every sector for food and tradegood potential
 *   - Resource Classification: Different tradegood types receive appropriate valuations
 *   - Military Assessment: Calculates force distribution efficiency for each nation
 *   - Statistical Safety: Handles edge cases like empty worlds or no armies
 *
 * Notes:
 *   - Static function - only called internally by AI systems
 *   - Critical foundation for AI strategic intelligence and decision making
 *   - Provides objective baseline for comparing relative sector and nation values
 *   - Performance optimized with register variables for map traversal
 *   - Results used throughout AI subsystems for strategic evaluation
 */
static void
find_avg_sector (void)
{
	int armynum, i, nation, repeat, total_sectors, total_food = 0;
	struct s_sector *sptr;	/* used to speed up this function */
	register int  x,y;
	register long total_tg = 0;
	register long useable_land = 0;

	for(x=0;x<MAPX;x++) for(y=0;y<MAPY;y++) {
		sptr = &sct[x][y];
		if(( sptr->altitude!=WATER )&&( sptr->altitude!=PEAK )) {
			useable_land++;
			total_food += tofood(sptr,country);
			if(sptr->tradegood != TG_none) {
				if(sptr->metal != 0) total_tg +=500;
				else if(sptr->jewels != 0) total_tg +=500;
				else total_tg +=300;
			}
		}
	}
	if (useable_land>0) {
		Avg_food = safe_long_to_int(total_food / useable_land);
		Avg_tradegood = safe_long_to_int(total_tg / useable_land);
	} else {
		/* for stupidities sake */
		Avg_food = 0;
		Avg_tradegood = 0;
	}

	for(nation=1;nation<NTOTAL;nation++) {
		if(isntn(ntn[nation].active)
		&& !COUNT_ARMIES(nation,country)) {
			/* Count the number of sectors which are occupied by each
			 * nation's armies.
			 */
			total_sectors = 0;
			for(armynum=1;armynum<MAXARM;armynum++)
				if(ntn[nation].arm[armynum].sold > 0) {
					x = ntn[nation].arm[armynum].xloc;
					y = ntn[nation].arm[armynum].yloc;
					repeat = FALSE;
					for (i=1; (i<armynum) && !repeat; ++i)
						if((ntn[nation].arm[i].xloc == x)
						&& (ntn[nation].arm[i].yloc == y))
						repeat = TRUE;
					if (!repeat)
						total_sectors++;
				}
			if (total_sectors > 0)
				Avg_soldiers[nation] = safe_long_to_int(ntn[nation].tmil / total_sectors);
			else Avg_soldiers[nation] = 0;
		}
	}
}

/*
 * nationrun - Master AI Coordination Controller and Primary Turn Processor
 *
 * The central command and control function that orchestrates all AI subsystems
 * for NPC nations during their turn processing. This function coordinates
 * economic management, military operations, diplomatic relations, infrastructure
 * development, magical research, and unit movement in an integrated decision-making
 * framework.
 *
 * This is the master AI controller that calls and coordinates all the previously
 * documented AI subsystems (economic via n_redes(), military via redomil(),
 * diplomatic via getdstatus(), and strategic intelligence via find_avg_sector()).
 *
 * Algorithm Overview:
 * 1. System Initialization: Validate nation state and initialize global variables
 * 2. Scope Determination: Set operational boundaries based on nation type (PC vs NPC)
 * 3. Intelligence Gathering: Update diplomatic status and world statistics
 * 4. Communication: Send diplomatic messages to hostile players (debug mode)
 * 5. Strategic Decision: Determine war/peace status and set military posture
 * 6. Unit Movement: Execute tactical movement for infantry and leaders/monsters
 * 7. Status Management: Update NPC activity levels based on military engagement
 * 8. Economic Policy: Set taxation and charity rates based on nation conditions
 * 9. Sector Management: Execute intelligent sector redesignation for optimization
 * 10. Infrastructure: Build fortress improvements in urban areas
 * 11. Military Reorganization: Optimize force allocation and unit deployment
 * 12. Magical Research: Purchase new powers and magical capabilities
 * 13. Military Enhancement: Buy attack/defense bonuses with metal resources
 * 14. Final Validation: Prevent invalid army statuses and ensure consistency
 *
 * Strategic Coordination Patterns:
 * - Economic decisions inform military capability assessments
 * - Diplomatic status drives military posture (attack vs defend vs peace)
 * - World intelligence guides expansion and defensive priorities
 * - Resource availability constrains magical research and military upgrades
 * - Military activity levels influence NPC engagement status updates
 *
 * Decision-Making Framework:
 * - Peace Operations: Focus on expansion and resource development (pceattr)
 * - War Operations: Balance attack/defense based on relative military strength
 * - Defensive Posture: Prioritize territory protection when outmatched
 * - Offensive Posture: Pursue territorial expansion when militarily superior
 *
 * Resource Management Integration:
 * - Economic: Taxation policy balances revenue with popularity
 * - Military: Force allocation optimized through redomil() coordination
 * - Magical: Jewel expenditure prioritizes military vs civilian powers
 * - Infrastructure: Gold investment in fortress construction when wealthy
 *
 * Parameters:
 *   None - Operates on global nation state (curntn, country) and world data
 *
 * Returns:
 *   void - Function modifies global game state through integrated AI operations
 *
 * Side Effects:
 *   - Modifies nation economic policy (tax rates, charity levels)
 *   - Updates military force allocation and army positioning
 *   - Changes sector designations through intelligent redesignation
 *   - Builds fortress infrastructure in urban centers
 *   - Purchases magical powers and military enhancements
 *   - Sends diplomatic messages to player nations (debug mode)
 *   - Updates global attractiveness map for movement decisions
 *   - Modifies army statuses and deployment patterns
 *   - Adjusts NPC activity levels based on military engagement
 *
 * Global Dependencies:
 *   - curntn: Current nation being processed (economic, military, diplomatic state)
 *   - country: Current nation index for array access and coordination
 *   - attr[][]: Global attractiveness map for movement and expansion decisions
 *   - sct[][]: World sector map for territorial and resource management
 *   - ntn[]: All nations array for diplomatic and military assessments
 *   - peace: Global peace/war status variable for strategic coordination
 *   - stx,sty,endx,endy: Operational boundaries for NPC scope limitation
 *
 * AI Subsystem Coordination:
 *   - getdstatus(): Updates diplomatic relations with all nations
 *   - find_avg_sector(): Calculates world statistics for strategic intelligence
 *   - pceattr(): Peaceful expansion attractiveness calculation
 *   - atkattr(): Offensive targeting attractiveness calculation
 *   - defattr(): Defensive positioning attractiveness calculation
 *   - n_redes(): Intelligent economic sector redesignation
 *   - redomil(): Military force reorganization and optimization
 *   - n_people(): Population attractiveness modification for movement
 *   - armymove(): Individual army movement execution
 *
 * Testing Notes:
 *   Category: C (System) - Requires complete game state initialization
 *   Approach: System testing with full nation, world, and diplomatic state
 *   Key Tests:
 *     - Peace mode economic optimization and expansion behavior
 *     - War mode attack/defense strategic decision making
 *     - Resource management across economic/military/magical systems
 *     - AI subsystem coordination and information flow
 *     - NPC activity level adjustments based on military engagement
 *     - Boundary condition handling (no resources, no military, isolated nations)
 *   Dependencies: Complete world map, all nations, diplomatic arrays, magic system
 *   Mock Requirements: Full game simulation environment with turn processing
 *   Complexity: Complex - Master AI controller with multi-system coordination
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to extensive global state modification
 *   - Performance: Computationally intensive due to complete world analysis
 *   - Memory: Modifies large global arrays and data structures
 *   - Historical: Core AI design dating to original game architecture
 *   - Integration: Central coordination point for all AI subsystem interactions
 *   - Magic Number Usage: Contains numerous hardcoded thresholds for AI decisions
 *   - Debug Support: Includes comprehensive debug output for AI behavior analysis
 *   - Error Handling: Multiple check() calls ensure data integrity throughout
  * @last_documented: 2025-09-19
 */
void
nationrun (void)
{
	int goldthresh,metalthresh,citythresh,useful;
	int armynum,loop;
	int x,y,i,p;
	float	hunger;
	long zz;
	check()
	prep(country,FALSE);

	for(x=0;x<MAPX;x++) for(y=0;y<MAPY;y++) attr[x][y]=0;

	/* is there an error*/
	if((sct[curntn->capx][curntn->capy].owner==country)
	&&(sct[curntn->capx][curntn->capy].designation!=DCAPITOL)){
		sct[curntn->capx][curntn->capy].designation=DCAPITOL;
	}

	if( ispc( curntn->active )) {
		stx=sty=0;
		endx=MAPX;
		endy=MAPY;
	} else {
		if( curntn->capx > NPCTOOFAR )
			stx=(int)curntn->capx-NPCTOOFAR;
		else	stx=0;
		if( curntn->capy > NPCTOOFAR )
			sty=(int)curntn->capy-NPCTOOFAR;
		else	sty=0;
		if( (int)curntn->capx + NPCTOOFAR < MAPX )
			endx=(int)curntn->capx+NPCTOOFAR;
		else	endx=MAPX;
		if( (int)curntn->capy + NPCTOOFAR < MAPY )
			endy=(int)curntn->capy+NPCTOOFAR;
		else	endy=MAPY;
	}

	getdstatus();

	find_avg_sector ();

#ifdef SPEW
	for(x=1;x<NTOTAL;x++) if(isntn( ntn[x].active )) {
		/* here is the bit which will occasionally send a randomly
		   generated message from a hostile NPC to a PC (25% chance) */
		if((curntn->dstatus[x] >= HOSTILE)
		&& (ispc(ntn[x].active)))
		if (rand()%4 == 0) {	/* send the message!! */
			if (mailopen(x)!=(-1)) {
				fprintf(fm,"Message to %s from %s (%s of year %d)\n\n"
					   ,ntn[x].name,curntn->name,PSEASON(TURN),YEAR(TURN));
				makemess(rand()%5 +1,fm);
				mailclose(x);
				printf("Sent message to %s\n",ntn[x].name);
			}
		}
	}
#endif /* SPEW */
	/*move units */
	/*are they at war with any normal countries*/
	peace=0;
	for(i=1;i<NTOTAL;i++)
	if(isntn(ntn[i].active)&&(curntn->dstatus[i]>peace)) {
		peace=curntn->dstatus[i];
		if( peace>= WAR) break;
	}

	if(peace<WAR){
		peace=8;
		pceattr();
	} else {
	/*if war then attack &/or expand */
		peace=12;
		/*are they attacking or defending */
		if(curntn->tmil==0) defattr();
		else for(x=0;x<NTOTAL;x++)
		if(isntn( ntn[x].active ) && (curntn->dstatus[x]>HOSTILE)){
			if(100*(curntn->tmil*(curntn->aplus+100))/((curntn->tmil*(curntn->aplus+100))+(ntn[x].tmil*(ntn[x].dplus+100)))>rand()%100){
				/*attacker*/
				for(armynum=1;armynum<MAXARM;armynum++)
					if((P_ASOLD>0)&&(P_ATYPE!=A_MILITIA)
					&&(P_ASTAT!=ONBOARD)&&(P_ASTAT!=TRADED)
					&&(P_ASTAT<NUMSTATUS)&&(P_ASTAT!=GENERAL))
						P_ASTAT=ATTACK;
				atkattr();
			} else {	/*defender*/
				for(armynum=1;armynum<MAXARM;armynum++)
					if((P_ASOLD>0)&&(P_ATYPE!=A_MILITIA)
					&&(P_ASTAT!=ONBOARD)&&(P_ASTAT!=TRADED)
					&&(P_ASTAT<NUMSTATUS)&&(P_ASTAT!=GENERAL)){
						if(P_ASOLD<350) P_ASTAT=DEFEND;
						else P_ASTAT=ATTACK;
					}
				defattr();
			}
		}
	}
	check()

	/* move infantry then leader/monsters */
	n_people(TRUE);			/* add to attr for people */

	if( country < 5 ) prtattr();

	loop=0;
	for(armynum=1;armynum<MAXARM;armynum++)
		if((P_ASOLD!=0)&&(P_ATYPE<MINLEADER)) loop+=armymove(armynum);
	n_people(FALSE);		/* subtract to attr for people */
	for(armynum=1;armynum<MAXARM;armynum++)
		if((P_ASOLD!=0)&&(P_ATYPE>=MINLEADER)) loop+=armymove(armynum);

	/* NPC ACTIVE STATUS CHANGE */
	if(isnpc(curntn->active)
	&&(curntn->active != ISOLATIONIST)) {
		if(isgood(curntn->active)) {
			if(loop<=1)	curntn->active=GOOD_0FREE;
			else if(loop>=6) curntn->active=GOOD_6FREE;
			else if(loop>=4) curntn->active=GOOD_4FREE;
			else	curntn->active=GOOD_2FREE;
		} else if(isneutral(curntn->active)) {
			if(loop<=1)	curntn->active=NEUTRAL_0FREE;
			else if(loop>=6) curntn->active=NEUTRAL_6FREE;
			else if(loop>=4) curntn->active=NEUTRAL_4FREE;
			else	curntn->active=NEUTRAL_2FREE;
		} else if(isevil(curntn->active)) {
			if(loop<=1)	curntn->active=EVIL_0FREE;
			else if(loop>=6) curntn->active=EVIL_6FREE;
			else if(loop>=4) curntn->active=EVIL_4FREE;
			else	curntn->active=EVIL_2FREE;
		}
	}

	if( curntn->tgold > curntn->tciv ) curntn->charity=10;
	else curntn->charity=0;
	if( (curntn->tsctrs < 20) || (curntn->score < 20) ) {
		if( curntn->tax_rate < 10 )
			curntn->tax_rate = 10;
	} else {
		curntn->tax_rate = safe_int_to_uchar((int)min((int)(curntn->prestige/5),(int)((curntn->popularity+curntn->terror+3*curntn->charity)/10)));
		curntn->tax_rate = safe_int_to_uchar((int)min(curntn->tax_rate,20));
		if(curntn->tax_rate < 4)
			curntn->tax_rate = 4;
	}

	/* INTELLIGENT SECTOR REDESIGNATION */
	/* note that only redesignate pc's if not designated yet */
	goldthresh=4;
	metalthresh=4;
	citythresh=10;
	hunger = 5.0;
	for(loop=1;loop<5;loop++) {
#ifdef DEBUG
		printf("\tnpcredes(): country %s gold=%d metal=%d, city=%d hunger=%f\n",curntn->name,goldthresh,metalthresh,citythresh,hunger);
#endif /* DEBUG */

		useful=FALSE;
		for(x=stx;x<endx;x++) for(y=sty;y<endy;y++)
		if((sct[x][y].owner==country)
		&&(is_habitable(x,y))
		&&((isnotpc(curntn->active))
		  ||(sct[x][y].designation==DNODESIG))) {
			n_redes(x,y,goldthresh,metalthresh,citythresh,hunger);
			useful=TRUE;
		}
		if(useful==FALSE) break;

		spreadsheet(country);

		if (spread.civilians+2*curntn->tmil > 0) {
			hunger = safe_long_to_float(spread.food)/((safe_long_to_float(spread.civilians+2*curntn->tmil)));
		} else hunger = 0.0;
		if(hunger < P_EATRATE ) {
			goldthresh++;
			metalthresh++;
			citythresh--;
		} else if(hunger > 2*P_EATRATE) {
			if( goldthresh==1 && metalthresh==1 ) break;
			goldthresh-=2;
			metalthresh-=2;
			citythresh+=2;
		} else {
			if( goldthresh==1 && metalthresh==1 ) break;
			if(rand()%2==0) goldthresh--;
			else metalthresh--;
			if(goldthresh==0) goldthresh=1;
			if(metalthresh==0) metalthresh=1;
			citythresh++;
		}
		if(goldthresh<=0) goldthresh=1;
		if(metalthresh<=0) metalthresh=1;
	}

	/*build forts in any cities*/
	if(isnotpc(curntn->active)) for(x=stx;x<endx;x++) for(y=sty;y<endy;y++)
		if((sct[x][y].owner==country)&&
			((sct[x][y].designation==DTOWN)
			||(sct[x][y].designation==DCITY)
			||(sct[x][y].designation==DCAPITOL)
			||(sct[x][y].designation==DFORT))
			&&(sct[x][y].fortress<10)
			&&(curntn->tgold>10000)
			&&(rand()%5==0)
			&&(sct[x][y].fortress<(sct[x][y].people%1000)))
				sct[x][y].fortress++;
	check()
	/*redo mil*/
	if(isnotpc(curntn->active)) redomil();
	check()

	/*buy new powers and/or new weapons*/
	if(getmgkcost(M_MIL,country) < getmgkcost(M_CIV,country)){
		if(curntn->jewels > getmgkcost(M_MIL,country)) {
			curntn->jewels-=getmgkcost(M_MIL,country);
			if((zz=getmagic(M_MIL))!=0){
				for(p=S_MIL;p<E_MIL;p++) if(powers[p]==zz){
					fprintf(fnews,"1.\tnation %s gets combat power %s\n",curntn->name,pwrname[p]);
					printf("\tnation %s gets combat power %s\n",curntn->name,pwrname[p]);
					break;
				}
				exenewmgk(zz);
			} else if((zz=getmagic(M_MIL))!=0){
				for(p=S_MIL;p<E_MIL;p++) if(powers[p]==zz){
					fprintf(fnews,"1.\tnation %s gets combat power %s\n",curntn->name,pwrname[p]);
					printf("\tnation %s gets combat power %s\n",curntn->name,pwrname[p]);
					break;
				}
				exenewmgk(zz);
			}
			else	curntn->jewels+=getmgkcost(M_MIL,country);
		}
	} else {
		if(curntn->jewels > getmgkcost(M_CIV,country)) {
			curntn->jewels-=getmgkcost(M_CIV,country);
			if((zz=getmagic(M_CIV))!=0){
				for(p=S_CIV;p<S_CIV+E_CIV;p++) if(powers[p]==zz){
					fprintf(fnews,"1.\tnation %s gets civilian power %s\n",curntn->name,pwrname[p]);
					printf("\tnation %s gets civilian power %s\n",curntn->name,pwrname[p]);
					break;
				}
				exenewmgk(zz);
			}
			else if((zz=getmagic(M_CIV))!=0){
				for(p=S_CIV;p<S_CIV+E_CIV;p++) if(powers[p]==zz){
					fprintf(fnews,"1.\tnation %s gets civilian power %s\n",curntn->name,pwrname[p]);
					printf("\tnation %s gets civilian power %s\n",curntn->name,pwrname[p]);
					break;
				}
				exenewmgk(zz);
			}
			else	curntn->jewels+=getmgkcost(M_CIV,country);
		}
	}

	check()
	if(magic(country,VAMPIRE)!=TRUE) {
		i=0;
		if(magic(country,WARLORD)==TRUE) i=30;
		else if(magic(country,CAPTAIN)==TRUE) i=20;
		else if(magic(country,WARRIOR)==TRUE) i=10;
		x = max ( curntn->aplus-i, 10 ) / 10;
		x *= x;
		if( curntn->race==ORC) x*=2;
		/* SHOULD USE spread.metal but it didnt work right */
		if(rand()%2==0)
		if(curntn->metals >  3 * METALORE * curntn->tmil*x){
			curntn->aplus+=1;
			curntn->metals-=METALORE*curntn->tmil*x;
			printf("\tnation %s buys +1 percent attack\n",curntn->name);
		}
		x = max ( curntn->dplus-i, 10 ) / 10;
		x *= x;
		if( curntn->race==ORC) x*=2;
		if(curntn->metals >  3 * METALORE * curntn->tmil*x){
			curntn->dplus+=1;
			curntn->metals-=METALORE*curntn->tmil*x;
			printf("\tnation %s buys +1 percent defense\n",curntn->name);
		}
	}
	/* don't allow status ATTACK from own city */
	for(armynum=0;armynum<MAXARM;armynum++) {
		if (P_ASOLD<=0 || P_ASTAT!=ATTACK) continue;
		if ((sct[P_AXLOC][P_AYLOC].owner==country)
		&& (fort_val(&sct[P_AXLOC][P_AYLOC]) > 0)) {
			if(rand()%2==0) P_ASTAT=DEFEND;
			else P_ASTAT=GARRISON;
		}
	}
}

/*
 * n_trespass - Diplomatic Trespass Prevention for AI Movement
 *
 * Marks sectors as undesirable (attr[x][y]=1) to prevent NPCs from moving
 * into territories owned by other nations when not at war. This function
 * implements diplomatic respect for territorial sovereignty, ensuring NPCs
 * avoid violating neutral or friendly territory unless in active warfare.
 *
 * The function enforces diplomatic movement restrictions by identifying
 * sectors that belong to other nations and are not appropriate for entry
 * based on current diplomatic relationships and territorial proximity rules.
 *
 * Algorithm:
 * 1. Scan all sectors within NPC operational boundaries (stx-endx, sty-endy)
 * 2. For each sector, check multiple diplomatic and territorial conditions
 * 3. Mark sectors as undesirable (attr=1) if they violate diplomatic rules
 * 4. Allow movement only into unowned sectors, own territory, or war targets
 *
 * Diplomatic Conditions for Trespass Prevention:
 * - Sector must be owned by another nation (not country, not unowned)
 * - Sector must be beyond immediate capitol vicinity (>2 sectors away)
 * - Current nation must not be at war with sector owner
 * - Sector owner must not be at war with current nation
 * - Diplomatic relationship must be worse than ALLIED (not allied nations)
 *
 * Territorial Proximity Rules:
 * - Sectors within 2 squares of capitol are exempt (diplomatic immunity zone)
 * - This allows passage near enemy capitols without being blocked by trespass
 * - Prevents NPCs from being completely unable to approach enemy territory
 *
 * Diplomatic Relationship Matrix:
 * - ALLIED: Movement allowed (relationship <= ALLIED exempted)
 * - NEUTRAL/HOSTILE: Movement blocked (ALLIED < relationship < WAR)
 * - WAR: Movement allowed (relationship >= WAR exempted)
 * - This creates proper diplomatic behavior respecting neutrality
 *
 * Parameters:
 *   None - Uses global state for current nation and diplomatic relationships
 *
 * Returns:
 *   void - Modifies global attractiveness map attr[x][y]
 *
 * Side Effects:
 *   - Sets attr[x][y]=1 for diplomatically inappropriate sectors
 *   - Prevents movement into neutral/hostile but non-war territory
 *   - Preserves diplomatic relationships by avoiding territorial violations
 *   - Creates movement corridors around enemy capitols for strategic access
 *
 * Global Dependencies:
 *   - stx,sty,endx,endy: NPC operational boundaries for sector scanning
 *   - sct[][]: World sector map for ownership and territorial information
 *   - attr[][]: Global attractiveness map modified to prevent movement
 *   - country: Current nation index for diplomatic relationship lookup
 *   - curntn: Current nation structure for capitol location access
 *   - ntn[]: All nations array for bilateral diplomatic status checking
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires diplomatic arrays and world map
 *   Approach: Integration testing with nations, diplomatic status, and sectors
 *   Key Tests:
 *     - Allied territory movement (should be allowed)
 *     - Neutral territory avoidance (should be blocked)
 *     - War target accessibility (should be allowed)
 *     - Capitol proximity exemption (movement near enemy capitols)
 *     - Bilateral diplomatic status handling (both directions checked)
 *     - Unowned sector accessibility (should remain unrestricted)
 *   Dependencies: Complete diplomatic arrays, nation structures, world sectors
 *   Mock Requirements: Nations with various diplomatic relationships and territories
 *   Complexity: Moderate - Diplomatic relationship matrix with territorial rules
 *
 * Notes:
 *   - Thread safety: Read-only access to diplomatic data, modifies only attr array
 *   - Performance: O(sectors) scan within NPC operational boundaries
 *   - Diplomatic logic: Implements proper diplomatic territorial respect
 *   - Strategic balance: Capitol proximity rule prevents diplomatic deadlocks
 *   - Bilateral checking: Ensures both nations agree on diplomatic status
 *   - Integration: Called by defattr(), atkattr(), and pceattr() coordinators
  * @last_documented: 2025-09-19
 */
void
n_trespass (void)
{
	register int x,y;
	for(x=stx;x<endx;x++) for(y=sty;y<endy;y++)  {
		if((sct[x][y].owner != country )
		&&( sct[x][y].owner != 0 )
		&&( abs(x-(int)curntn->capx)>2 )
		&&( abs(y-(int)curntn->capy)>2 )
		&&( ntn[country].dstatus[sct[x][y].owner]<WAR)
		&&( ntn[sct[x][y].owner].dstatus[country]<WAR)
		&&( ntn[country].dstatus[sct[x][y].owner]>ALLIED))
			attr[x][y]=1;
	}
}

/*
 * n_toofar - Capitol Distance Limitation for NPC Operational Scope
 *
 * Marks sectors as undesirable (attr[x][y]=1) that fall outside the NPC's
 * operational boundaries, enforcing distance limitations from the nation's
 * capitol. This function implements strategic range constraints that prevent
 * NPCs from overextending their operations beyond manageable distances.
 *
 * The function creates a movement restriction zone by marking all sectors
 * outside the pre-calculated operational boundaries (stx-endx, sty-endy)
 * as undesirable, ensuring NPCs focus their activities within a reasonable
 * geographic scope around their capitol.
 *
 * Algorithm:
 * 1. Scan every sector on the entire world map (0-MAPX, 0-MAPY)
 * 2. Compare each sector against operational boundaries
 * 3. Mark sectors outside boundaries as undesirable (attr=1)
 * 4. Leave sectors within boundaries unmodified for other attractiveness functions
 *
 * Operational Boundary Logic:
 * - Boundaries are set in nationrun() based on NPCTOOFAR constant
 * - PC nations: Use entire world map (stx=sty=0, endx=MAPX, endy=MAPY)
 * - NPC nations: Limited scope around capitol (NPCTOOFAR distance)
 * - This creates focused AI behavior preventing scattered expansion
 *
 * Strategic Purpose:
 * - Concentrates NPC activity around their power base (capitol)
 * - Prevents NPCs from pursuing unrealistic long-distance operations
 * - Creates regional AI behavior rather than global scope
 * - Balances game by limiting NPC expansion range
 *
 * Performance Considerations:
 * - Scans entire world map (MAPX * MAPY sectors)
 * - Simple boundary check operation for each sector
 * - Sets attr=1 for majority of sectors (outside boundaries)
 * - Most expensive attractiveness function due to global scope
 *
 * Parameters:
 *   None - Uses global operational boundaries set by nationrun()
 *
 * Returns:
 *   void - Modifies global attractiveness map attr[x][y]
 *
 * Side Effects:
 *   - Sets attr[x][y]=1 for all sectors outside operational boundaries
 *   - Effectively restricts NPC movement and expansion to regional scope
 *   - Creates focused AI behavior around capitol region
 *   - Prevents NPCs from pursuing distant opportunities
 *
 * Global Dependencies:
 *   - stx,sty,endx,endy: Operational boundaries set in nationrun()
 *   - attr[][]: Global attractiveness map modified for distance restriction
 *   - MAPX,MAPY: World map dimensions for complete sector scanning
 *
 * Integration with Operational Boundaries:
 *   - nationrun() calculates boundaries based on capitol location and NPCTOOFAR
 *   - PC nations get unlimited scope (entire world)
 *   - NPC nations get limited scope (NPCTOOFAR squares from capitol)
 *   - Boundaries account for world edge conditions and map limits
 *
 * Testing Notes:
 *   Category: A (Unit) - Simple boundary checking with global variables
 *   Approach: Unit testing with various boundary configurations
 *   Key Tests:
 *     - PC nation unlimited scope (no sectors marked undesirable)
 *     - NPC nation limited scope (sectors outside range marked)
 *     - World edge boundary conditions (prevent array overflow)
 *     - Capitol-centered operational area validation
 *     - NPCTOOFAR distance calculation accuracy
 *   Dependencies: Global boundary variables and world map dimensions
 *   Mock Requirements: Operational boundaries (stx,sty,endx,endy)
 *   Complexity: Simple - Straightforward boundary checking algorithm
 *
 * Notes:
 *   - Thread safety: Read-only access to boundaries, modifies only attr array
 *   - Performance: O(MAPX*MAPY) global scan - most expensive attractiveness function
 *   - Strategic design: Regional focus prevents NPC overextension
 *   - Game balance: Limits NPC expansion range for manageable AI behavior
 *   - Implementation: Simple but globally comprehensive boundary enforcement
 *   - Integration: Called by all attractiveness coordinators (defattr, atkattr, pceattr)
  * @last_documented: 2025-09-19
 */
void
n_toofar (void)
{
	register int x,y;
	for(x=0;x<MAPX;x++) for(y=0;y<MAPY;y++) {
		if( x<stx || y<sty || x>=endx || y>=endy )
			attr[x][y]=1;
	}
}

/*
 * n_unowned - Territorial Expansion and Resource Acquisition Attractiveness
 *
 * Increases attractiveness (attr[x][y]) for sectors that represent valuable
 * expansion opportunities, focusing on unowned territory, resource-rich areas,
 * and food production potential. This function implements the core territorial
 * expansion logic that drives AI nation growth and resource acquisition.
 *
 * The function evaluates sectors based on ownership status, resource wealth,
 * trade good availability, food production capacity, and strategic value,
 * creating attractiveness gradients that guide AI expansion decisions toward
 * the most valuable territories.
 *
 * Algorithm:
 * 1. Capitol Region Priority: Massively boost unowned sectors near capitol (±4 squares)
 * 2. Resource Evaluation: Assess known sectors for trade goods and resource values
 * 3. Unknown Sector Estimation: Use world averages for unexplored territories
 * 4. Ownership Assessment: Prioritize unowned sectors and nomad territories
 * 5. Food Production: Add food-based attractiveness for economic sustainability
 * 6. Habitability Adjustment: Reduce attractiveness for uninhabitable terrain
 *
 * Capitol Region Expansion (Priority Zone):
 * - Scans 9x9 grid around capitol (capx±4, capy±4)
 * - Adds +450 attractiveness to unowned sectors near capitol
 * - Creates strong preference for consolidating territory around power base
 * - Ensures secure territorial foundation before distant expansion
 *
 * Resource-Based Attractiveness Calculation:
 * - Metal trade goods: +500 attractiveness (highest priority resources)
 * - Jewel trade goods: +500 attractiveness (magical/luxury resources)
 * - Other trade goods: +300 attractiveness (standard valuable resources)
 * - No trade goods: +0 from this category (basic territories)
 *
 * Visibility and Intelligence Handling:
 * - Known sectors: Use actual resource and food data for precise evaluation
 * - Unknown sectors: Use world averages (Avg_tradegood, Avg_food) for estimation
 * - This allows strategic planning even with incomplete reconnaissance
 *
 * Territorial Ownership Priorities:
 * - Unowned sectors (owner=0): +300 attractiveness (primary expansion targets)
 * - Nomad-controlled sectors: +100 attractiveness (weak opposition territories)
 * - Owned sectors: +0 from ownership (may have other attractiveness sources)
 *
 * Food Production Integration:
 * - Known sectors: +50 * actual food production value
 * - Unknown sectors: +50 * estimated average food production
 * - Ensures expansion considers economic sustainability and population support
 *
 * Habitability Penalty:
 * - Uninhabitable terrain: Divide total attractiveness by 5 (80% reduction)
 * - Habitable terrain: No penalty applied
 * - Balances resource value against settlement viability
 *
 * Parameters:
 *   None - Uses global nation state and world map for evaluation
 *
 * Returns:
 *   void - Modifies global attractiveness map attr[x][y]
 *
 * Side Effects:
 *   - Increases attr[x][y] for valuable expansion territories
 *   - Creates attractiveness gradients favoring resource-rich areas
 *   - Prioritizes capitol region consolidation over distant expansion
 *   - Balances immediate territorial gains with long-term economic value
 *
 * Global Dependencies:
 *   - curntn: Current nation for capitol location and visibility calculations
 *   - country: Nation index for sector visibility and food production queries
 *   - attr[][]: Global attractiveness map modified with expansion priorities
 *   - sct[][]: World sector map for ownership, resources, and terrain data
 *   - ntn[]: All nations array for nomad territory identification
 *   - stx,sty,endx,endy: Operational boundaries for expansion scope
 *   - Avg_tradegood, Avg_food: World statistics for unknown sector estimation
 *
 * Resource Evaluation Functions:
 *   - SEE_SECTOR(): Determines if sector is visible for accurate resource assessment
 *   - tofood(): Calculates food production value for economic planning
 *   - is_habitable(): Checks terrain suitability for settlement
 *   - ONMAP(): Validates coordinate bounds for capitol region scanning
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires world map, nations, and statistics
 *   Approach: Integration testing with various world configurations
 *   Key Tests:
 *     - Capitol region unowned sector prioritization (+450 bonus)
 *     - Resource-rich sector attractiveness (metal/jewels +500)
 *     - Unknown sector estimation using world averages
 *     - Unowned vs nomad vs owned territory preference ranking
 *     - Food production attractiveness calculation accuracy
 *     - Habitability penalty application (÷5 for uninhabitable)
 *   Dependencies: World map, nation structures, visibility system, world statistics
 *   Mock Requirements: Sectors with various resources, ownership, and terrain types
 *   Complexity: Moderate - Multi-factor territorial evaluation with resource analysis
 *
 * Notes:
 *   - Thread safety: Read-only world access, modifies only attr array
 *   - Performance: O(operational_area) scan with resource evaluation per sector
 *   - Strategic design: Balances immediate territorial gains with resource value
 *   - Economic integration: Food production ensures sustainable expansion
 *   - Intelligence handling: Graceful degradation for unexplored territories
  * @last_documented: 2025-09-19
 */
void
n_unowned (void)
{
	register int x,y;

	/* around capitol */
	for(x=(int)curntn->capx-4;x<=(int)curntn->capx+4;x++){
		for(y=(int)curntn->capy-4;y<=(int)curntn->capy+4;y++){
			if((ONMAP(x,y))&&(sct[x][y].owner==0)) {
				attr[x][y] += 450;
			}
		}
	}

	for(x=stx;x<endx;x++) {
		for(y=sty;y<endy;y++) {
			if (SEE_SECTOR(x,y,country)) {
				/* add if metal high */
				if(sct[x][y].tradegood != TG_none) {
					if(sct[x][y].metal != 0) attr[x][y]+=500;
					else if(sct[x][y].jewels != 0) attr[x][y]+=500;
					else attr[x][y]+=300;
				}
			}
			else
			/*give some value: tradegood MIGHT be there*/
				attr[x][y] += Avg_tradegood;

			/*add to attractiveness for unowned sectors*/
			if(sct[x][y].owner == 0) {
				attr[x][y]+=300;
			} else	{
			if(ntn[sct[x][y].owner].active == NPC_NOMAD)
				attr[x][y]+=100;
			}
			if (SEE_SECTOR(x,y,country))
				attr[x][y] += 50*tofood(&sct[x][y],country);
			else
			/* give some value: there is probably average food there	*/
				attr[x][y] += 50*Avg_food;

			if(!is_habitable(x,y)) {
				attr[x][y] /= 5;
			}
		}
	}
}

void
n_defend (int natn)
{
	register	int		i,j;
	int		repeat;
	register	int		x,y;

	if (COUNT_ARMIES(natn,country)) {
		/* add 1/10th of their soldiers in sector */
		for(x=1;x<MAXARM;x++) if(ntn[natn].arm[x].sold > 0)
			if(sct[ntn[natn].arm[x].xloc][ntn[natn].arm[x].yloc].owner==country)
				attr[ntn[natn].arm[x].xloc][ntn[natn].arm[x].yloc] +=
					safe_long_to_int(ntn[natn].arm[x].sold/10);
	}
	else {
		/* For every of this country's sectors that has a 'natn' army
		 * in it, add 1/10 of the average number of soldiers in that
		 * 'natn' army.
		 */
		for(j=1;j<MAXARM;j++) if(ntn[natn].arm[j].sold > 0) {
			x = ntn[natn].arm[j].xloc;
			y = ntn[natn].arm[j].yloc;
			if(sct[x][y].owner==country) {
				/* insure the average soldiers is only added
				 * once per sector								*/
				repeat = FALSE;
				for (i=1; (i<j) && !repeat; ++i)
					if((ntn[natn].arm[i].xloc == x)
					&& (ntn[natn].arm[i].yloc == y))
						repeat = TRUE;

				if (!repeat)
					attr[x][y] += Avg_soldiers[natn]/10;
			}
		}
	}
	/*plus 80 if near your capitol */
	for(x=(int)curntn->capx-1;x<=(int)curntn->capy+1;x++){
		for(y=(int)curntn->capy-1;y<=(int)curntn->capy+1;y++){
			if(ONMAP(x,y)) attr[x][y]+=80;
		}
	}

	/*plus based on defensive value and population */
	for(x=stx;x<endx;x++) for(y=sty;y<endy;y++) {
		if(movecost[x][y]==1) attr[x][y] += 50;
		else if(movecost[x][y]<=3) attr[x][y] += 20;
		else if(movecost[x][y]<=5) attr[x][y] += 10;

 		if(sct[x][y].owner==country){
			if(ISCITY(sct[x][y].designation))
				attr[x][y] += 50;
			/* should spread 3000 points over country */
			if (ntn[country].tciv>0)
			attr[x][y]+=safe_long_to_int(3000*sct[x][y].people/ntn[country].tciv);
		}
	}
}

/*
 * n_attack - Offensive Target Prioritization for Military Campaigns
 *
 * Calculates and applies offensive attractiveness modifiers to identify and
 * prioritize enemy targets based on their strategic value and defensive weakness.
 * This function implements intelligent target selection by evaluating enemy
 * cities, comparing defensive strength against available forces, and adjusting
 * for intelligence limitations in combat planning.
 *
 * The function creates offensive positioning incentives that guide AI armies
 * toward valuable but vulnerable enemy targets, balancing strategic value
 * against realistic chances of success in combat operations.
 *
 * Algorithm:
 * 1. Scan all sectors owned by target enemy nation within operational boundaries
 * 2. Identify high-value targets (cities, capitols, towns) using intelligence
 * 3. Calculate defender-to-attacker ratios for tactical feasibility assessment
 * 4. Apply attractiveness bonuses based on target value and combat odds
 * 5. Handle intelligence limitations with conservative estimates and fallback values
 *
 * Target Identification Logic:
 * - Requires successful intelligence: SEE_CITIES(nation,country) must be true
 * - High-value targets: DCITY, DCAPITOL, DTOWN designations prioritized
 * - Intelligence failure: All enemy sectors get UNS_CITY_VALUE as potential targets
 * - Strategic focus: Cities provide maximum economic and political value
 *
 * Combat Feasibility Assessment:
 * - Visible defenders: Use exact enemy soldier counts via solds_in_sector()
 * - Hidden defenders: Use intelligence estimates via Avg_soldiers[] array
 * - Attacker strength: Count friendly armies within 1 sector of target
 * - Success threshold: Attackers must have >1.5x defender strength (3*solds > defenders*2)
 *
 * Attractiveness Scaling:
 * - Perfect intelligence + favorable odds: +500 attractiveness (maximum priority)
 * - Limited intelligence + favorable odds: +400 attractiveness (high priority)
 * - Unknown targets (intelligence failure): +UNS_CITY_VALUE (exploration value)
 * - Unfavorable odds: No bonus applied (realistic tactical assessment)
 *
 * Intelligence Integration:
 * - COUNT_ARMIES(): Determines if exact enemy force counts are available
 * - SEE_CITIES(): Controls whether city locations and types are visible
 * - Avg_soldiers[]: Provides fallback estimates when exact counts unavailable
 * - Graceful degradation: Maintains functionality with incomplete information
 *
 * Parameters:
 *   nation - Target enemy nation index to attack (0-MAXNTN)
 *          Used to identify enemy territories and assess defensive capabilities
 *
 * Returns:
 *   void - Modifies global attractiveness map attr[x][y] with offensive bonuses
 *
 * Side Effects:
 *   - Increases attr[x][y] for valuable and vulnerable enemy targets
 *   - Prioritizes cities and towns over regular territory
 *   - Applies tactical realism through defender-to-attacker ratio analysis
 *   - Adjusts for intelligence quality in target assessment
 *   - Creates offensive movement patterns toward optimal targets
 *
 * Global Dependencies:
 *   - ntn[]: All nations array for enemy territory and army information
 *   - attr[][]: Global attractiveness map modified with offensive bonuses
 *   - sct[][]: World sector map for ownership and city designation data
 *   - country: Current nation index for friendly force calculations
 *   - stx,sty,endx,endy: NPC operational boundaries for target scanning
 *   - Avg_soldiers[]: Intelligence estimates for enemy force strength
 *   - SEE_CITIES(): Intelligence macro for city visibility assessment
 *   - COUNT_ARMIES(): Intelligence macro for force count availability
 *   - solds_in_sector(): Function for exact enemy defender counts
 *   - UNS_CITY_VALUE: Constant for unknown target value estimation
 *
 * Tactical Decision Framework:
 * - Strategic value: Cities provide maximum political and economic impact
 * - Tactical feasibility: Combat odds must favor attackers for target prioritization
 * - Intelligence adaptation: Adjusts strategy based on available reconnaissance
 * - Force concentration: Considers nearby friendly armies for realistic combat assessment
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires nations, armies, intelligence, and world map
 *   Approach: Integration testing with enemy territories and combat scenarios
 *   Key Tests:
 *     - City targeting prioritization (high attractiveness for valuable targets)
 *     - Combat odds calculation (favorable ratios increase attractiveness)
 *     - Intelligence handling (visible vs hidden enemy forces)
 *     - Force ratio thresholds (>1.5x attacker advantage required)
 *     - Fallback targeting (UNS_CITY_VALUE for unknown targets)
 *     - Multi-army coordination (friendly forces within attack range)
 *   Dependencies: Nations with cities, armies, intelligence systems, combat mechanics
 *   Mock Requirements: Enemy territories with varying defenses and intelligence levels
 *   Complexity: Moderate - Multi-factor offensive strategy with intelligence integration
 *
 * Notes:
 *   - Thread safety: Read-only access to enemy data, modifies only attr array
 *   - Performance: O(sectors + armies) for target analysis and force calculations
 *   - Strategic realism: Only targets achievable objectives based on available forces
 *   - Intelligence dependence: Effectiveness scales with reconnaissance quality
 *   - Tactical integration: Coordinates with other attractiveness functions for strategy
 *   - Combat doctrine: Implements combined-arms coordination with force concentration
  * @last_documented: 2025-09-19
 */
void
n_attack (int nation)
{
	register int x,y;
	int	armynum;
	long	solds;	/* solds within 1 of capitol or city */

	for(x=stx;x<endx;x++) for(y=sty;y<endy;y++){
		if(sct[x][y].owner==nation) {
			if(SEE_CITIES(nation,country)) {
				if (((sct[x][y].designation==DCITY)
				||(sct[x][y].designation==DCAPITOL)
				||(sct[x][y].designation==DTOWN))){
					solds=0;
					for(armynum=1;armynum<MAXARM;armynum++)
						if((ntn[country].arm[armynum].sold > 0)
						&&(abs(AXLOC-x)<=1)
						&&(abs(AYLOC-y)<=1)) solds+=ASOLD;

					if (COUNT_ARMIES(nation,country)) {
						if(solds_in_sector(x,y,nation)*2 < 3*solds)
							attr[x][y]+=500;
					}
					else {
						if((solds_in_sector(x,y,nation) != 0)
						&&(Avg_soldiers[nation]*2 < 3*solds))
							attr[x][y]+=400;
					}
				}
			}
			else
			/* Give some value: the sector MIGHT be an under-defended city	*/
				attr[x][y] += UNS_CITY_VALUE;
		}
	}
}

/*
 * n_undefended - Undefended Sector Targeting for Opportunistic Expansion
 *
 * Calculates and applies attractiveness modifiers to prioritize enemy sectors
 * based on their defensive vulnerability and strategic accessibility. This
 * function implements opportunistic target selection by identifying weakly
 * defended or completely undefended enemy territories suitable for rapid
 * conquest with minimal military resistance.
 *
 * The function creates tactical opportunities by evaluating enemy territorial
 * vulnerabilities, distinguishing between undefended sectors (easy targets),
 * lightly defended areas (moderate difficulty), and uninhabitable regions
 * (low strategic value but potential staging areas).
 *
 * Algorithm:
 * 1. Scan all sectors owned by target enemy nation within operational boundaries
 * 2. Evaluate habitability and defensive presence for each enemy sector
 * 3. Apply attractiveness bonuses based on vulnerability and strategic value
 * 4. Prioritize completely undefended sectors for immediate opportunity
 * 5. Provide moderate attractiveness for defended but accessible targets
 *
 * Vulnerability Assessment Logic:
 * - Uninhabitable sectors: +30 attractiveness (low value, staging areas)
 * - Undefended habitable sectors: +100 attractiveness (maximum opportunity)
 * - Defended habitable sectors: +60 attractiveness (moderate targets)
 * - Evaluation based on enemy occupation and territorial suitability
 *
 * Defensive Presence Evaluation:
 * - occ[x][y]==0: No enemy military presence (undefended)
 * - occ[x][y]>0: Enemy forces present (defended but potentially vulnerable)
 * - is_habitable(): Determines long-term strategic value of territory
 * - Focus on sectors with tactical and strategic acquisition potential
 *
 * Strategic Value Hierarchy:
 * - Undefended habitable: Highest priority (+100) - immediate conquest opportunity
 * - Defended habitable: Moderate priority (+60) - achievable with effort
 * - Uninhabitable areas: Low priority (+30) - staging or resource value only
 * - Systematic prioritization creates realistic expansion targeting
 *
 * Tactical Integration:
 * - Coordinates with n_attack() for high-value city targeting
 * - Complements n_defend() by identifying expansion vs consolidation balance
 * - Supports territorial acquisition strategies through vulnerability exploitation
 * - Creates opportunistic movement patterns for efficient conquest
 *
 * Parameters:
 *   nation - Target enemy nation index for vulnerability assessment (0-MAXNTN)
 *          Used to identify enemy territories and evaluate defensive weaknesses
 *
 * Returns:
 *   void - Modifies global attractiveness map attr[x][y] with opportunity bonuses
 *
 * Side Effects:
 *   - Increases attr[x][y] for vulnerable enemy territories
 *   - Prioritizes undefended sectors for immediate conquest opportunities
 *   - Creates expansion patterns toward accessible enemy holdings
 *   - Balances territorial acquisition with strategic value assessment
 *   - Guides opportunistic military movements toward weak points
 *
 * Global Dependencies:
 *   - sct[][]: World sector map for enemy territory ownership identification
 *   - attr[][]: Global attractiveness map modified with opportunity bonuses
 *   - occ[][]: Military occupation map for defensive presence assessment
 *   - stx,sty,endx,endy: NPC operational boundaries for target area scanning
 *   - is_habitable(): Function for territorial strategic value evaluation
 *   - nation: Target enemy nation parameter for territory identification
 *
 * Opportunistic Strategy Framework:
 * - Vulnerability exploitation: Targets sectors with minimal resistance
 * - Efficient expansion: Prioritizes low-cost territorial acquisition
 * - Strategic balance: Considers long-term value alongside immediate opportunity
 * - Tactical realism: Focuses on achievable objectives rather than costly targets
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires nations, territories, and occupation data
 *   Approach: Integration testing with enemy territories and defense scenarios
 *   Key Tests:
 *     - Undefended sector prioritization (+100 attractiveness verification)
 *     - Defended sector evaluation (+60 attractiveness for occupied areas)
 *     - Uninhabitable area handling (+30 for staging area potential)
 *     - Territory ownership validation (only enemy nation sectors targeted)
 *     - Habitability assessment (strategic value vs tactical opportunity)
 *     - Occupation status accuracy (defended vs undefended classification)
 *   Dependencies: Nations with territories, occupation data, habitability system
 *   Mock Requirements: Enemy territories with varying defense levels and habitability
 *   Complexity: Simple - Straightforward vulnerability assessment with clear priorities
 *
 * Notes:
 *   - Thread safety: Read-only access to territory data, modifies only attr array
 *   - Performance: O(sectors) scan within operational boundaries
 *   - Strategic focus: Opportunistic expansion through vulnerability exploitation
 *   - Tactical efficiency: Minimizes military costs through intelligent target selection
 *   - Integration: Coordinates with other attractiveness functions for balanced strategy
 *   - Expansion doctrine: Implements efficient territorial acquisition through weakness targeting
  * @last_documented: 2025-09-19
 */
void
n_undefended(int nation )
{
	register int x,y;
	for(x=stx;x<endx;x++) for(y=sty;y<endy;y++) if(sct[x][y].owner==nation){
		if(is_habitable(x,y) == 0) {
			attr[x][y] += 30;
		} else if(occ[x][y]==0) {
			attr[x][y]+=100;
		} else {
			attr[x][y]+=60;
		}
	}
}

/*
 * n_people - Population-Based Attractiveness Modification for Movement Decisions
 *
 * Modifies attractiveness values based on civilian population density within
 * the nation's own territory, implementing population-centric military strategy
 * that can either attract forces toward populated areas (for protection) or
 * away from them (to avoid civilian casualties or for strategic repositioning).
 *
 * This function provides flexible population-based movement influence that can
 * be applied in different strategic contexts - concentrating forces near
 * civilians during defensive operations or dispersing them during offensive
 * campaigns to protect non-combatants from retaliation.
 *
 * Algorithm:
 * 1. Scan all sectors within operational boundaries owned by current nation
 * 2. Evaluate habitability and civilian population for each owned sector
 * 3. Apply population-based attractiveness modification (addition or subtraction)
 * 4. Scale influence by population density (1/4 of civilian count)
 * 5. Operate only on habitable sectors suitable for sustained civilian presence
 *
 * Population Influence Logic:
 * - doadd=TRUE: +population/4 attractiveness (protection/support strategy)
 * - doadd=FALSE: -population/4 attractiveness (dispersal/avoidance strategy)
 * - Only habitable sectors: is_habitable() ensures realistic civilian presence
 * - Proportional scaling: Higher populations create stronger influence
 *
 * Strategic Application Contexts:
 * - Defensive Mode (doadd=TRUE): Concentrate forces near civilians for protection
 * - Offensive Mode (doadd=FALSE): Disperse forces to protect civilians from retaliation
 * - Garrison Strategy (doadd=TRUE): Position troops near population centers
 * - Expeditionary Strategy (doadd=FALSE): Minimize civilian exposure during campaigns
 *
 * Population Protection Framework:
 * - Civilian safety: Military positioning considers non-combatant welfare
 * - Strategic flexibility: Same function supports opposite tactical approaches
 * - Realistic constraints: Only applies to habitable sectors with actual populations
 * - Proportional response: Military influence scales with civilian density
 *
 * Tactical Integration:
 * - Coordinates with n_defend() for civilian protection during defensive operations
 * - Balances with n_attack() to minimize civilian exposure during offensive campaigns
 * - Supports garrison vs expeditionary force allocation decisions
 * - Enables population-centric military strategy implementation
 *
 * Parameters:
 *   doadd - Strategic mode flag controlling population influence direction
 *         TRUE: Increase attractiveness near populations (protection strategy)
 *         FALSE: Decrease attractiveness near populations (dispersal strategy)
 *
 * Returns:
 *   void - Modifies global attractiveness map attr[x][y] with population influence
 *
 * Side Effects:
 *   - Increases or decreases attr[x][y] based on civilian population density
 *   - Applies influence only to nation's own habitable territories
 *   - Creates population-centric movement patterns for military strategy
 *   - Balances military objectives with civilian protection considerations
 *   - Enables flexible strategic doctrine implementation through parameter control
 *
 * Global Dependencies:
 *   - sct[][]: World sector map for territory ownership and population data
 *   - attr[][]: Global attractiveness map modified with population influence
 *   - country: Current nation index for territorial ownership verification
 *   - stx,sty,endx,endy: NPC operational boundaries for area scanning
 *   - is_habitable(): Function for civilian habitability assessment
 *   - sct[x][y].people: Civilian population count for influence scaling
 *
 * Strategic Doctrine Framework:
 * - Protection Strategy: Military concentration near civilian populations
 * - Dispersal Strategy: Military dispersion to protect civilian areas
 * - Flexible Application: Same function supports different strategic approaches
 * - Civilian Consideration: Military strategy incorporates non-combatant welfare
 *
 * Testing Notes:
 *   Category: A (Unit) - Simple population-based calculation with clear inputs
 *   Approach: Unit testing with varying population densities and doadd modes
 *   Key Tests:
 *     - Population scaling accuracy (attractiveness = population/4)
 *     - Addition mode verification (doadd=TRUE increases attractiveness)
 *     - Subtraction mode verification (doadd=FALSE decreases attractiveness)
 *     - Habitability filtering (only habitable sectors affected)
 *     - Territory ownership validation (only own nation sectors modified)
 *     - Zero population handling (no effect on unpopulated areas)
 *   Dependencies: Sector data with population and habitability information
 *   Mock Requirements: Territories with varying population densities
 *   Complexity: Simple - Straightforward population-based scaling with mode control
 *
 * Notes:
 *   - Thread safety: Read-only access to population data, modifies only attr array
 *   - Performance: O(sectors) scan within operational boundaries
 *   - Strategic flexibility: Supports both concentration and dispersal doctrines
 *   - Civilian focus: Incorporates non-combatant considerations into military strategy
 *   - Tactical realism: Population influence reflects real-world military constraints
 *   - Integration: Coordinates with other attractiveness functions for balanced strategy
 */
void
n_people (
    int doadd	/* TRUE if adding, FALSE if subtracting */
)
{
	register int x,y;
	for(x=stx;x<endx;x++) for(y=sty;y<endy;y++)
	if(sct[x][y].owner==country){
		if(is_habitable(x,y)){
			if(doadd==TRUE) {
				attr[x][y] += safe_long_to_int(sct[x][y].people/4);
			} else	attr[x][y] -= safe_long_to_int(sct[x][y].people/4);
		}
	}
}

/*
 * n_between - Strategic Blocking Position Calculation for Territorial Control
 *
 * Calculates and applies attractiveness bonuses to sectors positioned between
 * the current nation's capitol and an enemy nation's capitol, implementing
 * strategic territorial control through geographic chokepoint identification
 * and military positioning along critical communication and supply lines.
 *
 * This function identifies and prioritizes positions that can disrupt enemy
 * operations, control strategic corridors, and establish territorial buffer
 * zones between competing national power centers, creating tactical advantages
 * through geographic positioning and strategic depth.
 *
 * Algorithm:
 * 1. Verify enemy capitol visibility through intelligence systems
 * 2. Calculate rectangular corridor between own and enemy capitol coordinates
 * 3. Determine coordinate boundaries for strategic corridor identification
 * 4. Apply uniform attractiveness bonus throughout the strategic corridor
 * 5. Create territorial control incentives along critical axis of competition
 *
 * Strategic Corridor Logic:
 * - Intelligence prerequisite: SEE_CITIES(nation,country) must be true
 * - Rectangular corridor: Defined by capitol coordinate boundaries
 * - Coordinate calculation: min/max values create strategic rectangle
 * - Uniform bonus: +60 attractiveness throughout entire corridor area
 *
 * Capitol Coordinate Analysis:
 * - X-axis boundaries: min(own_capx, enemy_capx) to max(own_capx, enemy_capx)
 * - Y-axis boundaries: min(own_capy, enemy_capy) to max(own_capy, enemy_capy)
 * - Rectangular corridor: All sectors within coordinate boundaries
 * - Strategic positioning: Controls communication lines between power centers
 *
 * Territorial Control Strategy:
 * - Chokepoint identification: Strategic positions between competing nations
 * - Communication disruption: Positions along critical supply and command routes
 * - Buffer zone creation: Territorial depth between national power centers
 * - Strategic initiative: Proactive positioning rather than reactive defense
 *
 * Intelligence Integration:
 * - Enemy capitol visibility: Required for strategic corridor calculation
 * - Intelligence failure: Function returns immediately without effect
 * - Graceful degradation: No strategic positioning when intelligence unavailable
 * - Realistic constraints: Strategy depends on reconnaissance capabilities
 *
 * Parameters:
 *   nation - Target enemy nation index for corridor calculation (0-MAXNTN)
 *          Used to locate enemy capitol and calculate strategic positioning
 *
 * Returns:
 *   void - Modifies global attractiveness map attr[x][y] with corridor bonuses
 *
 * Side Effects:
 *   - Increases attr[x][y] by +60 for all sectors in strategic corridor
 *   - Creates uniform attractiveness throughout capitol-to-capitol rectangle
 *   - Establishes territorial control priorities along strategic axis
 *   - Returns immediately if enemy capitol intelligence unavailable
 *   - Generates strategic positioning patterns for territorial competition
 *
 * Global Dependencies:
 *   - ntn[]: All nations array for enemy capitol coordinate access
 *   - curntn: Current nation structure for own capitol coordinates
 *   - attr[][]: Global attractiveness map modified with corridor bonuses
 *   - SEE_CITIES(): Intelligence macro for enemy capitol visibility assessment
 *   - ONMAP(): Boundary validation macro for coordinate safety
 *   - ntn[nation].capx/capy: Enemy capitol coordinates for corridor calculation
 *   - curntn->capx/capy: Own capitol coordinates for corridor calculation
 *
 * Strategic Positioning Framework:
 * - Geographic advantage: Utilizes terrain and positioning for strategic control
 * - Territorial competition: Establishes influence along critical national axes
 * - Proactive strategy: Positions forces before conflicts escalate
 * - Communication control: Disrupts enemy coordination and supply lines
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires nations, intelligence, and coordinate systems
 *   Approach: Integration testing with capitol positions and intelligence scenarios
 *   Key Tests:
 *     - Corridor calculation accuracy (rectangular boundary verification)
 *     - Intelligence prerequisite enforcement (SEE_CITIES requirement)
 *     - Coordinate boundary handling (min/max calculations correct)
 *     - Map boundary validation (ONMAP coordinate safety)
 *     - Attractiveness application (+60 bonus throughout corridor)
 *     - Early return on intelligence failure (no effect when capitol invisible)
 *   Dependencies: Nations with capitols, intelligence systems, coordinate validation
 *   Mock Requirements: Enemy nations with visible/invisible capitols at various positions
 *   Complexity: Simple - Straightforward geometric calculation with intelligence gates
 *
 * Notes:
 *   - Thread safety: Read-only access to capitol data, modifies only attr array
 *   - Performance: O(corridor_area) application within strategic rectangle
 *   - Strategic value: Creates territorial control through geographic positioning
 *   - Intelligence dependence: Effectiveness requires enemy capitol reconnaissance
 *   - Geometric approach: Simple rectangular corridor calculation
 *   - Tactical integration: Coordinates with other attractiveness functions for strategy
  * @last_documented: 2025-09-19
 */
void
n_between(int nation)
{
	int x1,x2,y1,y2,x,y;

	if (SEE_CITIES(nation,country)==FALSE)
		return;

	/*plus if strategic blocking sector*/

	/*+60 if between the two capitol*/
	if (ntn[nation].capx < curntn->capx){
		x1=ntn[nation].capx;
		x2=curntn->capx;
	} else {
		x1=curntn->capx;
		x2=ntn[nation].capx;
	}
	if (ntn[nation].capy < curntn->capy){
		y1=ntn[nation].capy;
		y2=curntn->capy;
	}
	else {
		y1=curntn->capy;
		y2=ntn[nation].capy;
	}

	for(x=x1;x<=x2;x++) for(y=y1;y<=y2;y++) {
		if(ONMAP(x,y)) attr[x][y]+=60;
	}
}

/* if in jeopardy, move to survive
 *	if within two of cap add 1/5th of men
 *	if on cap and war and 2x your garrison go jihad and + 1/2 men
 */
void
n_survive (void)
{
	int i;
	int nation,armynum;
	int capx,capy;
	int	repeat;
	int x,y;

	capx=curntn->capx;
	capy=curntn->capy;

	if(sct[capx][capy].owner!=country){
		attr[capx][capy]=1000;
	}

	/*defend your capitol if occupied, +50 more if with their army*/
	for(nation=1;nation<NTOTAL;nation++)
	if((isntn(ntn[nation].active))
	&&((ntn[nation].dstatus[country]>=WAR)
	  ||(curntn->dstatus[nation]>=WAR))){
		if (COUNT_ARMIES(nation,country)) {
			for(armynum=1;armynum<MAXARM;armynum++)
			if((ntn[nation].arm[armynum].sold > 0)
			&&( ntn[nation].arm[armynum].xloc<=capx+2)
			&&( ntn[nation].arm[armynum].xloc>=capx-2)
			&&( ntn[nation].arm[armynum].yloc<=capy+2)
			&&( ntn[nation].arm[armynum].yloc>=capy-2)){
				if((ntn[nation].arm[armynum].xloc==capx)
				&&(ntn[nation].arm[armynum].yloc==capy)){
					attr[capx][capy]+=safe_long_to_int(2*ntn[nation].arm[armynum].sold);
				}
				else {
					attr[ntn[nation].arm[armynum].xloc][ntn[nation].arm[armynum].yloc]+=safe_long_to_int(ntn[nation].arm[armynum].sold);
				}
			}
		} else {
		/* The next line might be more accurate to take into account
		 * the actual number of armies that natn has, but I don't
		 * think that VOID or HIDDEN should allow that to be known.
		 */
			for(armynum=1;armynum<MAXARM;armynum++)
			if(ntn[nation].arm[armynum].sold > 0) {
				x = ntn[nation].arm[armynum].xloc;
				y = ntn[nation].arm[armynum].yloc;
				if((capx-2<=x && x<=capx+2)
				&&( capy-2<=y && y<=capy+2)){
				/* insure the average soldiers is only added
				 * once per sector
				 */
					repeat = FALSE;
					for (i=1; (i<armynum) && !repeat; ++i)
						if((ntn[nation].arm[i].xloc == x)
						&& (ntn[nation].arm[i].yloc == y))
						repeat = TRUE;
					if (!repeat) {
						if((ntn[nation].arm[armynum].xloc==capx)
						&&(ntn[nation].arm[armynum].yloc==capy)){
							attr[capx][capy]+=2* Avg_soldiers[nation];
						}
						else {
							attr[ntn[nation].arm[armynum].xloc][ntn[nation].arm[armynum].yloc]+= Avg_soldiers[nation];
						}
					}
				}
			}
		}
	}
}

/*
 * defattr - Defensive attractiveness coordinator for comprehensive defensive positioning
 *
 * Master coordinator function that orchestrates all defensive attractiveness calculations
 * to create comprehensive defensive strategy incorporating territorial expansion, threat
 * response, strategic positioning, and emergency survival protocols. Integrates multiple
 * defensive attractiveness functions to produce cohesive defensive movement priorities.
 *
 * Defensive Strategy Integration:
 * 1. Territorial Foundation: n_unowned() establishes base expansion attractiveness
 * 2. Multi-Nation Threat Response: Coordinates defenses against all hostile nations
 * 3. Enemy-Specific Defense: n_defend() for threat-specific defensive positioning
 * 4. Strategic Blocking: n_between() for territorial control and corridor blocking
 * 5. Opportunistic Defense: n_undefended() for exploiting enemy vulnerabilities
 * 6. Movement Constraints: n_trespass() and n_toofar() for operational limitations
 * 7. Crisis Management: n_survive() for emergency capitol defense situations
 *
 * Algorithm Sequence:
 * 1. Base Expansion: Calculate unowned territory attractiveness for defensive expansion
 * 2. War Status Assessment: Identify all nations with hostile diplomatic relations
 * 3. Multi-Threat Coordination: For each enemy nation simultaneously:
 *    - Defensive positioning against specific enemy forces (n_defend)
 *    - Strategic corridor blocking between capitols (n_between)
 *    - Vulnerability exploitation of enemy territories (n_undefended)
 * 4. Operational Constraints: Apply movement limitations and diplomatic restrictions
 * 5. Emergency Protocol: Overlay emergency capitol defense priorities
 *
 * Defensive Coordination Features:
 * - Multi-Enemy Integration: Coordinates defense against multiple simultaneous threats
 * - Strategic Layering: Combines territorial, tactical, and emergency defensive priorities
 * - Constraint Application: Ensures all defensive moves respect operational boundaries
 * - Crisis Override: Emergency survival takes precedence over standard defensive positioning
 *
 * Note: Debug print statement incorrectly shows "atkattr()" instead of "defattr()".
 * This appears to be a copy-paste error from offensive coordinator function.
 *
 * Parameters:
 *   (none) - Uses global curntn (current nation), country (nation ID), and world state
 *
 * Returns:
 *   (void) - Modifies global attr[][] attractiveness array with integrated defensive values
 *
 * Side Effects:
 *   - Modifies attr[][] with comprehensive defensive attractiveness calculations
 *   - Calls multiple attractiveness functions that modify global state
 *   - Reads ntn[] for diplomatic status and nation activity verification
 *   - Uses DEBUG compilation flag for diagnostic output
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires multiple nations, diplomatic system, full world state
 *   Approach: Integration testing with multiple enemy nations and complex defensive scenarios
 *   Key Tests: Multi-enemy coordination, defensive priority layering, constraint application
 *   Dependencies: Full diplomatic system, nation management, all attractiveness subsystems
 *   Mock Requirements: Multiple nations, complex diplomatic relations, world geography
 *   Complexity: Moderate - Orchestrates multiple defensive subsystems with proper sequencing
 *
 * Notes:
 *   - Orchestration Function: Primary coordinator for all defensive attractiveness systems
 *   - Thread Safety: Not thread-safe due to global variable dependencies
 *   - Performance: O(nations^2 * sectors) due to multiple attractiveness function calls
 *   - Debug Output: Contains incorrect debug message (shows "atkattr" instead of "defattr")
 *   - Legacy Pattern: K&R function declaration needs modernization to ANSI C
  * @last_documented: 2025-09-19
 */
void
defattr (void)
{
	int nation;

#ifdef DEBUG
	printf("atkattr()\n");
#endif /* DEBUG */

	n_unowned();

	for(nation=1;nation<NTOTAL;nation++)
	if((isntn(ntn[nation].active))&&(curntn->dstatus[nation]>=WAR)) {
		n_defend(nation);
		n_between(nation);
		n_undefended(nation);
	}

	n_trespass();
	n_toofar();
	n_survive();
}

/*
 * atkattr - Offensive attractiveness coordinator for comprehensive military campaigns
 *
 * Master coordinator function that orchestrates all offensive attractiveness calculations
 * to create comprehensive military strategy incorporating territorial expansion, target
 * prioritization, strategic positioning, and diplomatic escalation responses. Integrates
 * multiple offensive attractiveness functions with escalation intensity based on war status.
 *
 * Offensive Strategy Integration:
 * 1. Territorial Foundation: n_unowned() establishes base expansion attractiveness
 * 2. Diplomatic Escalation: Different offensive intensity based on war vs jihad status
 * 3. Standard War Operations: Strategic positioning, vulnerability exploitation, direct attack
 * 4. Jihad Intensification: Repeated offensive function calls for maximum aggression
 * 5. Operational Constraints: Movement limitations and diplomatic restrictions
 * 6. Survival Override: Emergency protocols maintain defensive capabilities
 *
 * Algorithm Sequence:
 * 1. Base Expansion: Calculate unowned territory attractiveness for offensive expansion
 * 2. Nation-by-Nation Assessment: Evaluate diplomatic status with each active nation
 * 3. War-Level Operations (WAR status): Single-intensity offensive coordination:
 *    - Strategic corridor blocking between capitols (n_between)
 *    - Vulnerability exploitation of enemy territories (n_undefended)
 *    - Direct military target prioritization (n_attack)
 * 4. Jihad-Level Operations (JIHAD status): Triple-intensity offensive escalation:
 *    - Repeated attack prioritization calls (4x n_attack for maximum aggression)
 *    - Doubled strategic positioning (2x n_between, 2x n_undefended)
 *    - Maximum offensive commitment with sustained pressure
 * 5. Operational Constraints: Apply movement limitations and diplomatic restrictions
 * 6. Survival Integration: Maintain emergency defensive capabilities
 *
 * Offensive Escalation Features:
 * - Graduated Response: Different offensive intensity based on diplomatic relations
 * - War Intensity: Standard offensive operations (1x each function)
 * - Jihad Intensity: Maximum aggression with repeated function calls (up to 4x attack)
 * - Strategic Integration: Combines direct attack with territorial positioning
 * - Constraint Respect: All offensive moves respect operational boundaries
 *
 * Jihad Escalation Pattern:
 * - 4x n_attack() calls: Maximum target prioritization and attack focus
 * - 2x n_between() calls: Doubled strategic corridor control
 * - 2x n_undefended() calls: Doubled vulnerability exploitation
 * - Represents total war commitment with maximum resource allocation
 *
 * Parameters:
 *   (none) - Uses global curntn (current nation), country (nation ID), and world state
 *
 * Returns:
 *   (void) - Modifies global attr[][] attractiveness array with integrated offensive values
 *
 * Side Effects:
 *   - Modifies attr[][] with comprehensive offensive attractiveness calculations
 *   - Calls multiple attractiveness functions that modify global state
 *   - Reads ntn[] for diplomatic status and nation activity verification
 *   - Uses DEBUG compilation flag for diagnostic output
 *   - Intensive computation during jihad operations due to repeated function calls
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires multiple nations, diplomatic system, full world state
 *   Approach: Integration testing with various diplomatic scenarios and escalation levels
 *   Key Tests: War vs jihad intensity differences, multi-enemy coordination, constraint application
 *   Dependencies: Full diplomatic system, nation management, all attractiveness subsystems
 *   Mock Requirements: Multiple nations, varied diplomatic relations (WAR/JIHAD), world geography
 *   Complexity: Moderate - Orchestrates multiple offensive subsystems with escalation scaling
 *
 * Notes:
 *   - Orchestration Function: Primary coordinator for all offensive attractiveness systems
 *   - Escalation Aware: Adapts offensive intensity based on diplomatic relations
 *   - Thread Safety: Not thread-safe due to global variable dependencies
 *   - Performance: O(nations^2 * sectors) for WAR, higher for JIHAD due to repeated calls
 *   - Diplomatic Integration: Demonstrates sophisticated graduated response capability
 *   - Legacy Pattern: K&R function declaration needs modernization to ANSI C
 */
/*calculate attractiveness of attacking sectors*/
void
atkattr (void)
{
	int nation;
#ifdef DEBUG
	printf("atkattr()\n");
#endif /* DEBUG */

	n_unowned();

	/*adjust for each nation that you are at war with*/
	for(nation=1;nation<NTOTAL;nation++) if( isntn(ntn[nation].active) ){
		if(curntn->dstatus[nation]==WAR) {
			n_between(nation);
			n_undefended(nation);
			n_attack(nation);
		} else if(curntn->dstatus[nation]==JIHAD) {
			n_attack(nation);
			n_attack(nation);
			n_between(nation);
			n_undefended(nation);
			n_attack(nation);
			n_between(nation);
			n_undefended(nation);
			n_attack(nation);
		}
	}

	n_toofar();
	n_trespass();
	n_survive();
}

/*
 * pceattr - Peaceful expansion attractiveness coordinator for non-hostile territorial growth
 *
 * Master coordinator function that orchestrates attractiveness calculations during peaceful
 * periods when the nation is not engaged in active warfare. Focuses on territorial expansion
 * through uncontested territory acquisition while maintaining operational constraints and
 * emergency preparedness. Provides peaceful growth strategy with emphasis on expansion.
 *
 * Peaceful Strategy Integration:
 * 1. Triple Expansion Focus: Three calls to n_unowned() for maximum expansion priority
 * 2. No Hostile Operations: Excludes all military targeting and offensive positioning
 * 3. Operational Constraints: Maintains movement limitations and diplomatic restrictions
 * 4. Emergency Preparedness: Retains survival protocols for unexpected threats
 *
 * Algorithm Sequence:
 * 1. Maximum Expansion Priority: Triple n_unowned() calls for intensive territorial growth
 * 2. Constraint Application: Apply movement limitations (n_toofar, n_trespass)
 * 3. Emergency Protocols: Maintain defensive capabilities (n_survive)
 *
 * Peaceful Expansion Features:
 * - Expansion Emphasis: Triple territorial acquisition attractiveness through repeated calls
 * - Non-Aggressive: Excludes all offensive and defensive military positioning functions
 * - Constraint Respect: All expansion moves respect operational boundaries and diplomacy
 * - Growth Focus: Optimized for rapid territorial expansion during peaceful periods
 * - Emergency Ready: Maintains crisis response capability for unexpected threats
 *
 * Strategic Design:
 * - 3x n_unowned(): Maximum emphasis on territorial expansion during peace
 * - No Military Positioning: Excludes n_attack, n_defend, n_between, n_undefended
 * - Peaceful Doctrine: Designed for growth-focused expansion without military considerations
 * - Resource Optimization: Concentrates all attractiveness on uncontested territory
 *
 * Comparison with Other Coordinators:
 * - vs defattr(): Excludes all defensive military positioning, focuses purely on expansion
 * - vs atkattr(): Excludes all offensive operations, no target prioritization
 * - Unique Approach: Only coordinator with repeated function calls for expansion emphasis
 *
 * Parameters:
 *   (none) - Uses global curntn (current nation), country (nation ID), and world state
 *
 * Returns:
 *   (void) - Modifies global attr[][] attractiveness array with peaceful expansion values
 *
 * Side Effects:
 *   - Modifies attr[][] with expansion-focused attractiveness calculations
 *   - Calls n_unowned() three times for maximum territorial acquisition priority
 *   - Applies operational constraints to prevent invalid movement
 *   - Maintains emergency survival protocols for unexpected threats
 *   - Uses DEBUG compilation flag for diagnostic output
 *
 * Testing Notes:
 *   Category: A (Unit) - Simpler coordination with fewer dependencies than military coordinators
 *   Approach: Unit testing with peaceful diplomatic scenarios and expansion opportunities
 *   Key Tests: Triple expansion priority, constraint application, no military interference
 *   Dependencies: Minimal - territorial data, diplomatic constraints, emergency protocols
 *   Mock Requirements: World geography, territorial ownership, basic diplomatic state
 *   Complexity: Simple - Straightforward expansion coordination without military complexity
 *
 * Notes:
 *   - Orchestration Function: Specialized coordinator for peaceful territorial growth
 *   - Growth Optimized: Designed for maximum expansion efficiency during peace
 *   - Thread Safety: Not thread-safe due to global variable dependencies
 *   - Performance: O(sectors) with 3x expansion calculation overhead
 *   - Peaceful Design: Demonstrates clear separation between military and peaceful strategies
 *   - Legacy Pattern: K&R function declaration needs modernization to ANSI C
 */
/*calculate attractiveness when at peace*/
void
pceattr (void)
{
#ifdef DEBUG
	printf("pceattr()\n");
#endif /* DEBUG */
	n_unowned();
	n_unowned();
	n_unowned();
	n_trespass();
	n_toofar();
	n_survive();
}
#endif /* NPC */
