/*
 * randeven.c - Random event system for dynamic gameplay events
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
 * RANDOM EVENT SYSTEM ARCHITECTURE
 * =============================================================================
 *
 * This module implements a comprehensive random event system that adds dynamic,
 * unpredictable elements to gameplay. Events range from natural disasters and
 * political upheavals to economic windfalls and magical discoveries.
 *
 * CORE FUNCTIONALITY:
 * ------------------
 * 1. **Political Events**: Revolts, rebellions, diplomatic changes
 * 2. **Natural Disasters**: Earthquakes, volcanoes, hurricanes, tornadoes
 * 3. **Economic Events**: Resource discoveries, mine depletion, prosperity changes
 * 4. **Military Events**: Army attacks, disease outbreaks, general paralysis
 * 5. **Magical Events**: Power gains/losses, curse effects, magical discoveries
 *
 * EVENT PROBABILITY SYSTEM:
 * ------------------------
 * Events are triggered based on weighted probability calculations that consider:
 * - Nation size and score (larger nations more likely to experience events)
 * - Tax rate and popularity (affects revolt probability)
 * - Prestige levels (influences political stability)
 * - Terror and charity levels (social stability factors)
 * - Overall world score and nation count (global scaling)
 *
 * REVOLT MECHANICS:
 * ----------------
 * The system implements sophisticated revolt mechanics with multiple types:
 * - **Peasant Revolts**: Triggered by high taxes and low popularity
 * - **Military Revolts**: Based on prestige and tax burden
 * - **Regional Rebellions**: Province-level independence movements
 * - **Religious Schisms**: Faith-based separatist movements
 *
 * Each revolt type creates new nations with appropriate territories, armies,
 * and diplomatic relationships, providing realistic political fragmentation.
 *
 * NATURAL DISASTER SYSTEM:
 * ------------------------
 * Natural disasters affect specific geographic areas with varying severity:
 * - **Localized Events**: Single sector impacts (tornadoes, town fires)
 * - **Regional Effects**: Multi-sector damage patterns (earthquakes, hurricanes)
 * - **Volcanic Eruptions**: Specialized high-impact events with devastation zones
 * - **Population Reduction**: Realistic casualty modeling for disaster events
 *
 * NATION CREATION ENGINE:
 * ----------------------
 * When revolts occur, the system dynamically creates new nations by:
 * - Finding unused nation slots and generating unique names/marks
 * - Selecting appropriate capital locations based on population centers
 * - Distributing territories using distance-based allocation algorithms
 * - Transferring proportional resources and military units
 * - Establishing realistic diplomatic relationships with existing nations
 *
 * RESOURCE AND ECONOMIC EVENTS:
 * -----------------------------
 * Economic events provide balance through both positive and negative impacts:
 * - **Resource Discovery**: New gold/jewel strikes and metal ore deposits
 * - **Resource Depletion**: Mine exhaustion and vein collapse
 * - **Economic Prosperity**: Percentage-based wealth increases
 * - **Infrastructure Improvements**: Fortress strengthening, architectural advances
 *
 * CONDITIONAL COMPILATION:
 * -----------------------
 * - **RANEVENT**: Master feature flag - entire system disabled if undefined
 * - **VULCANIZE**: Enables volcanic eruption mechanics with devastation effects
 * - **MONSTER**: Enables nomad raid events with dynamic army creation
 * - **HIDELOC**: Controls location information disclosure in event reporting
 * - **CHECKUSER**: Enables user ID verification for new nations
 * - **DEBUG**: Provides detailed probability calculation logging
 *
 * EVENT CATEGORIES AND WEIGHTS:
 * -----------------------------
 * Events are categorized by type with different probability weights:
 * - **Disaster Events** (9-29): Natural and man-made catastrophes
 * - **Beneficial Events** (30-35): Positive discoveries and improvements
 * - **Political Events** (36-40): Power shifts and diplomatic changes
 * - **Military Events** (41-43): Army-affecting incidents
 *
 * NOTIFICATION SYSTEM:
 * -------------------
 * Events are reported through multiple channels:
 * - **News System**: Global event announcements for all players
 * - **Mail System**: Direct notification to affected nation rulers
 * - **Console Output**: Administrative logging and debugging information
 * - **Location Privacy**: Optional location hiding for strategic events
 *
 * INTEGRATION POINTS:
 * ------------------
 * - **Magic System**: Power gain/loss events interface with magic.c
 * - **Combat System**: Military events coordinate with combat resolution
 * - **Economy**: Resource events integrate with economic calculations
 * - **Diplomacy**: Political events modify inter-nation relationships
 * - **Geography**: Natural disasters interact with terrain and altitude systems
 *
 * MODERNIZATION CONSIDERATIONS:
 * ----------------------------
 * - Global variables (country, xpos, ypos) should be encapsulated
 * - Event probability calculations could benefit from floating-point precision
 * - Nation creation logic could be modularized for reusability
 * - Error handling could be improved with proper return codes
 * - Random number generation could use modern PRNG with better distribution
 *
 * TESTING STRATEGY:
 * ----------------
 * Testing requires careful balance between deterministic verification and
 * random event simulation. Key testing approaches:
 * - **Probability Testing**: Verify event distribution over large iterations
 * - **Nation Creation**: Test edge cases in nation slot allocation
 * - **Geographic Logic**: Validate sector selection and distance calculations
 * - **Resource Handling**: Ensure proper resource transfer and accounting
 * - **Integration Testing**: Verify correct interaction with other systems
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <pwd.h>
#include <ctype.h>
#include "header.h"
#include "data.h"

#ifdef RANEVENT
char	*names[] = {		/* must end in single character name */
	"groo","brok","vul","poin","srop","hoga","nobi","bonz","gail",
	"lynn","zorb","theed","urda","anima","bedlam","delos","quin",
	"xynd","putz","erde","clym","fanz","ilth","X"
};

extern FILE *fnews;
extern short country;

char eventstr[LINELTH+1];
int xpos,ypos;		/* saved x and y position */

char *randevents[] = {
/* 0 */  "a military rebellion",
/* 1 */  "a cult breaks with you",
/* 2 */  "a province rebels",
/* 3 */  "an evil wizard sets up",
/* 4 */  "a tax revolt occurs",
/* 5 */  "open rebellion flares",
/* 6 */  "an army revolts",
/* 7 */  "religions schism",
/* 8 */  "peasants revolt",
/* 9 */  "dragons raid",
/* 10 */ "a famine rages",
/* 11 */ "a hurricane blows over",
/* 12 */ "a tornado hits",
/* 13 */ "a volcano erupts",
/* 14 */ "a royal wedding occurs",
/* 15 */ "new alloy discovered",
/* 16 */ "royal advisor discovered to be spy",
/* 17 */ "gold strike in one sector",
/* 18 */ "gold strike in one sector",
/* 19 */ "gold vein runs out in one goldmine sector",
/* 20 */ "gold vein runs out in one goldmine sector",
/* 21 */ "a flood ravishes",
/* 22 */ "an earthquake quakes",
/* 23 */ "severe frost destroys crops",
/* 24 */ "feared dragon killed",
/* 25 */ "several nomad armies raid",
/* 26 */ "fire ravishes town",
/* 27 */ "black plague rages",
/* 28 */ "pirates raid",
/* 29 */ "savages raid",
/* 30 */ "wizard grants power",
/* 31 */ "magic item grants magic power",
/* 32 */ "ores in one mine run out",
/* 33 */ "new architect strengthens castle walls",
/* 34 */ "new ores discovered + 4-10 metal one sector",
/* 35 */ "skilled diplomacy obtains peace",
/* 36 */ "powerful magi curses nation",
/* 37 */ "severe winter hits",
/* 38 */ "tidal wave -- abandon all coastlands ",
/* 39 */ "ninja destroy general staff",
/* 40 */ "general found to be spy",
/* 41 */ "general prosperity +20% gold",
/* 42 */ "disease kills 20% of soldiers",
/* 43 */ "poor conditions kill 20% of soldiers"
};
#define MAXRANEVENT 43

/*
 * findnew - Find and initialize an unused nation slot for revolt/event creation
 *
 * Searches the nation array for an available slot (INACTIVE status) and performs
 * basic initialization for a new nation created during random events or revolts.
 * Uses reverse iteration to prefer higher-numbered nation slots, which helps
 * maintain stability of lower-numbered player nations.
 *
 * The function sets up minimal nation properties required for a functional
 * nation state, including default leadership, security credentials, and
 * initial status values. The new nation starts as NEUTRAL_6FREE, indicating
 * a newly created neutral state eligible for diplomatic contact.
 *
 * Parameters:
 *   None - operates on global nation array (ntn[])
 *
 * Returns:
 *   Nation index (1 to NTOTAL-1) on success
 *   0 if no unused nation slots are available
 *
 * Side Effects:
 *   - Modifies global ntn[] array for the allocated nation slot
 *   - Sets leader name to "rebel" as default identifier
 *   - Copies password from nation 0 (administrative/default password)
 *   - Initializes score to 0 and sector count to 0
 *   - Sets nation status to NEUTRAL_6FREE for diplomatic availability
 *
 * Testing Notes:
 *   Category: A (Unit) - Function with minimal dependencies, easily testable
 *   Approach: Unit tests with mock nation array states
 *   Key Tests: [Full nation array, empty array, single slot available]
 *   Dependencies: Global ntn[] array, NTOTAL constant, nation status constants
 *   Mock Requirements: Mock ntn[] array with various fill states
 *   Complexity: Simple - straightforward search and initialization logic
 *
 * Notes:
 *   - Reverse iteration (NTOTAL-1 to 1) preserves player nation stability
 *   - Nation 0 is reserved and never allocated to new nations
 *   - Password copying from nation 0 provides default authentication
 *   - NEUTRAL_6FREE status enables immediate diplomatic interaction
 *   - Function assumes nation array is properly initialized
 *   - No validation of nation state consistency after initialization
 */
int
findnew()
{
	int newntn=0,nationis;
	for ( nationis=NTOTAL-1; nationis >= 1; nationis--)
		if(ntn[nationis].active == INACTIVE) newntn=nationis;
	if (newntn == 0) return (0);
	strcpy(ntn[newntn].leader,"rebel");
	strcpy(ntn[newntn].passwd,ntn[0].passwd);
	ntn[newntn].score=0L;
	ntn[newntn].tsctrs=0;
	ntn[newntn].active=NEUTRAL_6FREE;
	return(newntn);
}

/*
 * getnewmark - Generate an unused single-character nation mark identifier
 *
 * Searches for an available character to serve as a nation's map marker,
 * which is used for visual identification on the game map display. The
 * function systematically checks uppercase letters first (A-Z), then
 * lowercase letters (a-z) to find the first unused character.
 *
 * The nation mark system provides quick visual identification of territories
 * on the map display, with each nation having a unique character identifier.
 * This is essential for map readability when multiple nations control
 * adjacent territories.
 *
 * The algorithm prioritizes uppercase letters, which are typically more
 * visible and distinguishable on map displays. If all uppercase letters
 * are exhausted, it falls back to lowercase letters, providing up to 52
 * unique nation identifiers total.
 *
 * Parameters:
 *   None - uses global nation state for mark validation
 *
 * Returns:
 *   Unused character (A-Z, then a-z) suitable for nation marking
 *   Last checked character ('z') if all marks are exhausted
 *
 * Side Effects:
 *   - Calls markok() function to validate character availability
 *   - No direct modifications to global state
 *   - Relies on existing nation mark assignments for conflict detection
 *
 * Testing Notes:
 *   Category: A (Unit) - Simple character generation with external validation
 *   Approach: Unit tests with mock markok() function responses
 *   Key Tests: [Empty nation list, full uppercase, all marks exhausted]
 *   Dependencies: markok() function for mark availability checking
 *   Mock Requirements: Mock markok() with various availability patterns
 *   Complexity: Simple - straightforward character iteration algorithm
 *
 * Notes:
 *   - Infinite loop protection: stops at 'z' if all characters are used
 *   - Uppercase precedence ensures better visibility for primary nations
 *   - Character range covers 52 possible nation marks (26+26)
 *   - Returns 'z' as fallback if system is completely saturated
 *   - Function assumes markok() provides accurate availability information
 *   - No validation of returned character's actual uniqueness
 */
char
getnewmark()
{
	char tmpchr='A'-1;			/* cap letters first */
	while (TRUE) {
		tmpchr++;
		if( markok( tmpchr, FALSE )) break;
		if(tmpchr=='Z') tmpchr='a'-1;	/* then small letters */
		if(tmpchr=='z') break;	/* otherwise it will loop forever */
	}
	return(tmpchr);
}

/*
 * disolve - Split a nation by dissolving a percentage into a new independent nation
 *
 * Creates a new nation by splitting off a specified percentage of territories,
 * resources, and population from an existing target nation. This is the core
 * mechanism for implementing political fragmentation events such as revolts,
 * rebellions, and secessions that occur during random events.
 *
 * The function handles two distinct types of nation splitting:
 * 1. **Peasant Revolts** (ispsnt=TRUE): Creates peasant nations with minimal
 *    resources, centered on high-population sectors, and at war with all
 * 2. **Political Revolts** (ispsnt=FALSE): Creates full nations with proportional
 *    resource sharing, centered on cities/towns, with realistic diplomacy
 *
 * The algorithm implements sophisticated territory allocation using distance-based
 * selection from a chosen capital location, ensuring geographic coherence of the
 * new nation. Resource and military unit transfers maintain game balance while
 * providing the new nation with sufficient assets for survival.
 *
 * Parameters:
 *   percent - Percentage of target nation to split off (1-100)
 *   target  - Nation index to split (source nation)
 *   ispsnt  - Boolean flag: TRUE for peasant revolt, FALSE for political revolt
 *
 * Returns:
 *   Index of newly created nation on success
 *   0 on failure (insufficient size, no available slots, etc.)
 *
 * Side Effects:
 *   - Creates new nation entry in global ntn[] array
 *   - Transfers sectors from target to new nation based on distance algorithm
 *   - Splits resources (gold, food, jewels, metals) proportionally (non-peasant)
 *   - Divides civilian and military populations between nations
 *   - Transfers military units located in split territories
 *   - Establishes diplomatic relationships for new nation
 *   - Updates global eventstr with descriptive outcome message
 *   - Modifies global xpos/ypos coordinates for location tracking
 *
 * Testing Notes:
 *   Category: C (System) - Complex function requiring full game state initialization
 *   Approach: System testing with complete nation and map setup
 *   Key Tests: [Small nations, large percentages, peasant vs political splits]
 *   Dependencies: Complete game state, nation array, sector map, army arrays
 *   Mock Requirements: Full game world simulation with nations and territories
 *   Complexity: Complex - sophisticated territory allocation and resource management
 *
 * Notes:
 *   - Minimum 7 sectors required for non-peasant splits to prevent micro-nations
 *   - Capital selection prioritizes cities/towns for political stability
 *   - Distance-based allocation (1-10 sector radius) ensures geographic coherence
 *   - Peasant revolts join existing peasant nations when possible
 *   - Military units automatically transfer with their territories
 *   - Diplomatic status reflects revolt type (peasant=war with all, political=neutral)
 *   - Uses global country variable temporarily during sector selection
 *   - HIDELOC compilation flag controls location disclosure in event messages
 */
int
disolve(percent, target, ispsnt)
int target;
int percent;
int ispsnt;		/* true/false */
{
	int new;	/* new nation number */
	int split;	/* number of sectors split */
	int defaultx=(-1), defaulty=(-1), realx=(-1), realy=(-1), dist;
	int i=0,j,armynum,narmynum;

	split =  ntn[target].tsctrs * percent / 100;
	if((split<=7)&&(!ispsnt)) {
		strcpy(eventstr,"nation too small->no sectors will be split");
		return(0);
	}
	/* find starting town */
	if( ispsnt ) {
		for( new=0; new<NTOTAL; new++ ) {
			if((ntn[new].race==ntn[target].race )
			&&( ntn[new].active==NPC_PEASANT )) {
				printf("\tntn %s peasants of same type as target %d\n",ntn[new].name,target);
				sprintf(eventstr,"rebellion joins nation %s",ntn[new].name);
				return(new);
			}
		}
		new = country;
		country=target;
		xpos = ypos = (-1);
		while( i++ < 300 ) {
			rand_sector();
			if(sct[xpos][ypos].people>=300) break;
		} 
		if( i==300 ) {
			strcpy(eventstr,"no sectors available");
			return(0);
		}
		printf("TMP peasant centered on %d,%d\n",xpos,ypos);
		country = new;
		realx = xpos;
		realy = ypos;
	} else for(i=0; i<MAPX; i++) for(j=0; j<MAPY; j++) {
		if(sct[i][j].owner == target){
			if((sct[i][j].designation==DCITY)
		 	||(sct[i][j].designation==DTOWN)
			||((sct[i][j].people>=2000)&&(sct[i][j].designation!=DCAPITOL))){
				if( rand()%3 == 0 ) {
					realx = i;
					realy = j;
				} else {
					defaultx = i;
					defaulty = j;
				}
			} 
		}
	}
	if(realx == (-1)){
		realx = defaultx;
		realy = defaulty;
	}
	if(realx == (-1)) {
		strcpy(eventstr,"no cities available");
		return(0);
	}
	if((new=findnew()) == 0) {
		strcpy(eventstr,"no nations available");
		return(0);
	}
	if(getnewname(new) == 0) {
		strcpy(eventstr,"no names available");
		return(0);
	}

#ifdef HIDELOC
	sprintf(eventstr,"new nation %s created",ntn[new].name);
#else
	sprintf(eventstr,"new nation %s created at %d,%d",ntn[new].name,realx,realy);
#endif /* HIDELOC */
	printf("TMP new nation %s created at %d,%d",ntn[new].name,realx,realy);
	sct[realx][realy].owner=new;
#ifdef CHECKUSER
	ntn[new].uid = getpwnam(LOGIN)->pw_uid;
#endif /* CHECKUSER */
	ntn[new].capx=realx;
	ntn[new].capy=realy;
	sct[realx][realy].designation=DCAPITOL;
	ntn[new].class=ntn[target].class;
	ntn[new].race= ntn[target].race;
	if( !ispsnt ) {
		ntn[new].tgold= ntn[target].tgold* percent / 100;
		ntn[new].tfood= ntn[target].tfood* percent / 100;
		ntn[new].jewels= ntn[target].jewels* percent / 100;
		ntn[new].metals= ntn[target].metals* percent / 100;
		ntn[target].tgold -= ntn[new].tgold;
		ntn[target].tfood -= ntn[new].tfood;
		ntn[target].jewels -= ntn[new].jewels;
		ntn[target].metals -= ntn[new].metals;
	}
	ntn[new].tciv= ntn[target].tciv* percent / 100;
	ntn[new].tmil= ntn[target].tmil* percent / 100;
	ntn[target].tciv -= ntn[new].tciv;
	ntn[target].tmil -= ntn[new].tmil;
	ntn[new].repro= ntn[target].repro;
	ntn[new].maxmove= ntn[target].maxmove;
	if( ispsnt ) {
		ntn[new].aplus= ntn[new].dplus= 0;
	} else {
		ntn[new].aplus= ntn[target].aplus - 10;
		ntn[new].dplus= ntn[target].dplus - 10;
	}
	ntn[new].location= ntn[target].location;
	ntn[new].powers= ntn[target].powers;
	ntn[new].tships= 0;
	ntn[new].tsctrs = split;
	ntn[new].tax_rate = 10;

	/* first check first letter of name */
	if( markok(toupper(ntn[new].name[0]),FALSE) )
		ntn[new].mark = toupper(ntn[new].name[0]);
	else
		ntn[new].mark = getnewmark();

	for ( dist=1 ; dist < 10; dist++) if (split > 0)
	for (i=realx-dist; i<realx+dist; i++)
		for (j=realy-dist; j<realy+dist; j++){
			if(ONMAP(i,j)
			&&( split>0 )
			&&( sct[i][j].designation != DCAPITOL )
			&&( sct[i][j].owner == target)){
				split--;
				if( sct[i][j].people > 0 )
					sct[i][j].owner=new;
			}
		}

	narmynum=1;
	for (armynum=0;armynum<MAXNAVY;armynum++) {
		ntn[new].nvy[armynum].warships = 0;
		ntn[new].nvy[armynum].merchant = 0;
	}
	if(!ispsnt) for (armynum=narmynum; armynum<MAXARM; armynum++) {
		ntn[new].arm[armynum].sold = 0;
		if((ntn[target].arm[armynum].sold>0)
		&&(sct[ntn[target].arm[armynum].xloc][ntn[target].arm[armynum].yloc].owner==new)){
			ntn[new].arm[narmynum].sold
				=ntn[target].arm[armynum].sold;
			ntn[new].arm[narmynum].unittyp
				=ntn[target].arm[armynum].unittyp;
			ntn[new].arm[narmynum].xloc
				=ntn[target].arm[armynum].xloc;
			ntn[new].arm[narmynum].yloc
				=ntn[target].arm[armynum].yloc;
			ntn[new].arm[narmynum].stat
				=ntn[target].arm[armynum].stat;
			ntn[new].arm[narmynum].smove
				=ntn[target].arm[armynum].smove;
			ntn[target].arm[armynum].sold = 0;
			narmynum++;
		}
	}
	if( !ispsnt ) {
		armynum=0;
		ntn[new].arm[0].sold = 300;
		ntn[new].arm[0].unittyp = A_INFANTRY;
		ntn[new].arm[0].xloc = realx;
		ntn[new].arm[0].yloc = realy;
		ntn[new].arm[0].stat = GARRISON;
		ntn[new].arm[0].smove = 0;
	}
	for(dist=0;dist<NTOTAL;dist++) if(dist!=new) {
		/* create realistic diplomatic status */
		if( ispsnt || ismonst( ntn[dist].active ) ) {
			ntn[new].dstatus[dist]=WAR;
			ntn[dist].dstatus[new]=WAR;
		} else if(ntn[target].dstatus[dist]==UNMET) {
			ntn[new].dstatus[dist]=UNMET;
			ntn[dist].dstatus[new]=UNMET;
		} else {
			ntn[new].dstatus[dist]=NEUTRAL;
			ntn[dist].dstatus[new]=NEUTRAL;
		}
	}
	ntn[new].dstatus[target]=WAR;
	ntn[target].dstatus[new]=WAR;
	return(new);
}

/*
 * getnewname - Assign an unused nation name from the predefined name pool
 *
 * Selects an available name from the global names[] array for a newly created
 * nation. The function iterates through the predefined list of nation names,
 * checking each against existing nations to ensure uniqueness. When an unused
 * name is found, it assigns that name to the specified nation slot.
 *
 * The names[] array contains carefully chosen fantasy/fictional nation names
 * that fit the game's medieval/fantasy setting. The array is terminated by
 * a single-character entry ("X") which serves as a sentinel value to prevent
 * buffer overruns during iteration.
 *
 * This system ensures that newly created nations (from revolts, random events,
 * or other game mechanics) receive distinctive, thematically appropriate names
 * rather than generic identifiers. The name pool provides sufficient variety
 * for typical gameplay scenarios while maintaining setting consistency.
 *
 * Parameters:
 *   new - Nation index to assign a name to (must be valid nation slot)
 *
 * Returns:
 *   1 on successful name assignment
 *   0 if no unused names are available in the predefined pool
 *
 * Side Effects:
 *   - Modifies ntn[new].name field with selected name string
 *   - No other global state modifications
 *   - Assumes nation slot 'new' is properly allocated and initialized
 *
 * Testing Notes:
 *   Category: A (Unit) - Simple name assignment with string operations
 *   Approach: Unit tests with mock nation arrays and name pools
 *   Key Tests: [Empty nation list, full name usage, invalid nation slot]
 *   Dependencies: Global names[] array, ntn[] array, string functions
 *   Mock Requirements: Mock names[] array and ntn[] state
 *   Complexity: Simple - straightforward string comparison and assignment
 *
 * Notes:
 *   - Names array sentinel: stops iteration at single-character entries
 *   - String comparison uses exact match (case-sensitive)
 *   - No validation of target nation slot validity
 *   - Linear search algorithm - O(n*m) where n=names, m=nations
 *   - Name pool exhaustion returns failure without fallback generation
 *   - Function assumes names[] array is properly null-terminated
 *   - Does not handle duplicate names within the names[] array itself
 */
int
getnewname(new)
int	new;
{
	int count,i=0;

	for( i=0;strlen(*(names+i)) > 1;i++ ){
		for(count=0;count<NTOTAL;count++)
			if(strcmp(ntn[count].name, *(names+i))==0) break;
		if( count==NTOTAL ) {
			strcpy(ntn[new].name,*(names+i));
			return(1);
		}
	}
	return(0);
}

/*
 * randomevent - Main random event processing engine and game event dispatcher
 *
 * This is the central function that orchestrates all random events throughout
 * the game world. It implements a sophisticated probability-based event system
 * that triggers various events (disasters, political upheavals, economic changes,
 * magical discoveries) based on nation characteristics and global game state.
 *
 * The function operates in multiple phases:
 * 1. **Global Events**: Processes world-wide events like volcanic eruptions
 * 2. **Nation Screening**: Evaluates each nation for event eligibility
 * 3. **Probability Calculation**: Computes revolt and event probabilities
 * 4. **Event Execution**: Triggers and processes selected events
 * 5. **Notification**: Reports events through multiple channels
 *
 * PROBABILITY SYSTEM MECHANICS:
 * The function uses sophisticated probability calculations that consider:
 * - **Peasant Revolt Factor**: 10*tax_rate - popularity - terror - 3*charity
 * - **Political Revolt Factor**: 5*tax_rate - prestige
 * - **Event Scaling**: (rand%100 * WORLDSCORE) < (RANEVENT * WORLDNTN * nation_score)
 * - **Weather Events**: Fixed PWEATHER percentage chance
 *
 * EVENT CATEGORIES PROCESSED:
 * - **Political Events** (0-8): Various types of revolts and political upheavals
 * - **Natural Disasters** (9-29): Earthquakes, volcanoes, hurricanes, plagues
 * - **Beneficial Events** (30-35): Magic power gains, resource discoveries
 * - **Economic Events**: Resource depletion, prosperity changes, fortress improvements
 * - **Military Events** (39-43): Army paralysis, disease, general prosperity
 *
 * ELIGIBILITY REQUIREMENTS:
 * Nations must meet minimum thresholds to experience events:
 * - Must be active nation (not INACTIVE)
 * - Score must exceed 20 points
 * - Must control at least 20 sectors
 * This prevents events from affecting insignificant micro-nations.
 *
 * Parameters:
 *   None - operates on global game state and nation arrays
 *
 * Returns:
 *   void - side effects through global state modification
 *
 * Side Effects:
 *   - Modifies global nation properties (resources, populations, armies)
 *   - Updates sector ownership, designations, and characteristics
 *   - Creates new nations through revolt mechanics
 *   - Sends notifications through news, mail, and console systems
 *   - Updates global eventstr variable with event descriptions
 *   - Modifies global xpos/ypos coordinates for event location tracking
 *   - May trigger volcanic eruptions with VULCANIZE compilation flag
 *   - Can create nomad armies with MONSTER compilation flag
 *
 * Testing Notes:
 *   Category: C (System) - Complex function requiring full game world simulation
 *   Approach: System testing with complete game state and mock probability functions
 *   Key Tests: [Various nation configurations, probability edge cases, event chains]
 *   Dependencies: Complete game world, nation arrays, map state, magic system
 *   Mock Requirements: Full world simulation with nations, sectors, armies, resources
 *   Complexity: Complex - Central game engine with extensive state interactions
 *
 * Notes:
 *   - Uses global 'country' variable to iterate through nations
 *   - Temporary modification of curntn pointer during event processing
 *   - Event probability calculations designed to scale with game progression
 *   - Some events are disabled (done=FALSE) indicating incomplete implementation
 *   - DEBUG compilation flag provides detailed probability calculation logging
 *   - HIDELOC flag controls location information disclosure in event reports
 *   - Events 14, 16, 21, 23, 28, 29, 37, 38 are currently unimplemented
 *   - Magic system integration for power gain/loss events (cases 30, 31, 36)
 *   - Resource management events affect gold, jewels, metals, and food stores
 *   - Military events can paralyze armies or reduce soldier populations
 *   - Architectural events improve fortress defenses across all cities
 */
void
randomevent()
{
	int percent,count, event, newnation, i, j, armynum,x,y;
	int done,holdval;	/*if 1 then event happened */
	long longval;
	long newpower;
	struct s_sector *sptr;

	printf("Random Events\n");

#ifdef VULCANIZE
/* have a volcano erupt on the map based on percent chance of PVULCAN */
	if(rand()%100<PVULCAN) erupt();
#endif

	/* decide what nations get random event */
	for(country=0;country<NTOTAL;country++) {
		curntn = &ntn[country];
		if(( !isntn(curntn->active))
		||(curntn->score<=20L)
		||(curntn->tsctrs<=20))
			continue;

		/* clear the event string */
		strcpy(eventstr,"");
		
		x = 10*curntn->tax_rate - curntn->popularity- curntn->terror - 3*curntn->charity;
#ifdef DEBUG
printf("TEMP: %s chance of peasant revolt is %d (tax=%d pop=%d terror=%d)\n",
		curntn->name, x, curntn->tax_rate, curntn->popularity,
		curntn->terror );
#endif /* DEBUG */

		if((rand()%100)<x) {
			if(rand()%100<PREVOLT){
				holdval=0;
				peasant_revolt( &holdval );
				if( holdval != 0 )
				wdisaster(country,ntn[holdval].capx,ntn[holdval].capy,0,"peasant revolt");
				else printf("revolt in %s fails because:\n\t%s\n",curntn->name,eventstr);
			}
		} 

		x = 5 * curntn->tax_rate - curntn->prestige;
#ifdef DEBUG
printf("TEMP: %s chance of revolt is %d (tax=%d prest=%d)\n",
		curntn->name, x, curntn->tax_rate, curntn->prestige );
#endif /* DEBUG */

		if(( rand()%100)< x ){
			if(rand()%100<PREVOLT){
				event = other_revolt( &holdval );
				if( event != -1 )
				wdisaster(country,ntn[holdval].capx,ntn[holdval].capy,0,randevents[event]);
				else printf("revolt in %s fails because:\n\t%s\n",curntn->name,eventstr);
			}
		}

		if( (rand()%100) < PWEATHER )	weather();

		/* clear the event string for other random events */
		strcpy(eventstr,"");

		/* do truely random events */
		if((rand()%100)* WORLDSCORE < RANEVENT * WORLDNTN * curntn->score){

		percent=0;
		event = rand()%(MAXRANEVENT-9) + 9;
		done=TRUE;
		xpos = ypos = -1;
		/* do the event */
		switch(event) {
		case 9: /*dragon raid -- lose 30% of food*/
			strcpy(eventstr,"lose 30% of food");
			curntn->tfood *= 7L;
			curntn->tfood /= 10L;
			break;
		case 10: /*famine -- food=0 10% starve*/
			curntn->tfood /= 4L;
			strcpy(eventstr,"lose 3/4ths of food & 10% starve");
			for (i=0; i<MAPX; i++) for (j=0; j<MAPY; j++)
			if(sct[i][j].owner==country)
				sct[i][j].people -= sct[i][j].people / 10;
			break;
		case 11: /*hurricane*/
			sptr = rand_sector();
			percent = 10 + rand()%20;
			/* one hex radius */
			for (x=xpos-1;x<=xpos+1;x++)
			for (y=ypos-1;y<=ypos+1;y++) if(ONMAP(x,y)) {
				sptr = &sct[x][y];
				if( !(ISCITY( sptr->designation ))
				&& (sptr->designation != DRUIN))
					sptr->designation = DNODESIG;
				if (sptr->fortress>=1) sptr->fortress--;
				reduce(x, y, percent);
			}
			break;
		case 12:
			/*tornado*/
			sptr = rand_sector();
			if( !(ISCITY( sptr->designation ))
			&& (sptr->designation != DRUIN))
				sptr->designation = DNODESIG;
			else {
				if (sptr->fortress < 2) sptr->fortress = 0;
				else sptr->fortress -= 2;
			}
			percent = 10 + rand()%25;
			reduce(xpos, ypos, percent);
			break;
		case 13:
			/*volcano -- all flee around one mountain -- 30% die*/
			holdval=0;		/* holdval is # of mountains */
			for (i=0; i<MAPX; i++) for (j=0; j<MAPY; j++)
			if(sct[i][j].owner==country && sct[i][j].altitude==MOUNTAIN)
				holdval++;

			if (holdval > 0) count = (rand()%holdval) + 1;
			else {
				done = FALSE;
				break;
			}

			sprintf(eventstr,"all flee, 30%% die in 1 sector range");
			for (xpos=0; count && (xpos<MAPX); xpos++)
			for (ypos=0;count && (ypos<MAPY); ypos++)
			if (( sct[xpos][ypos].owner == country)
			&& (sct[xpos][ypos].altitude == MOUNTAIN)) {
				count--;
				if (count == 0) blowup(xpos,ypos);
			}
#ifdef HIDELOC
			/* hide nation of eruption if HIDELOC */
			done = FALSE;
#endif /* HIDELOC */
			break;
		case 14:
			/*royal wedding (absorb neighbor nation)*/
			/*	takeover ( 100, 0 ); */  
			/* sprintf(eventstr,"absorb neighbor nation %s");*/
			/* something not right.... */
			done=FALSE;
			break;
		case 15:
			/*new alloy +10% combat (WARRIOR...)*/
			if(magic(country,WARRIOR)!=1){
				curntn->powers|=WARRIOR;
				exenewmgk(WARRIOR);
				strcpy(eventstr,"gives WARRIOR power");
			}
			else if(magic(country,CAPTAIN)!=1){
				curntn->powers|=CAPTAIN;
				exenewmgk(CAPTAIN);
				strcpy(eventstr,"gives CAPTAIN power");
			}
			else if(magic(country,WARLORD)!=1){
				curntn->powers|=WARLORD;
				exenewmgk(WARLORD);
				strcpy(eventstr,"gives WARLORD power");
			}
			else {	/* have all three powers... oh well */
				done=FALSE;
			}
			break;
		case 16:
			/*royal advisor is spy -- lose power*/
			/* sprintf(eventstr,"nation loses power %s");*/
			done=FALSE;
			break;
		case 17:
		case 18:
			/*gold/jewel strike one sector */
			holdval = 0;
			for (i=0; i<MAPX; i++) for (j=0; j<MAPY; j++)
				if((sct[i][j].owner == country)
				&& (sct[i][j].jewels == 0)
				&& (is_habitable(i,j))) {
					holdval++;
				}
			if (holdval > 0) count = (rand()%holdval) + 1;
			else {
				done = FALSE;
				break;
			}
			for (xpos=0; count && (xpos<MAPX); xpos++)
			for (ypos=0; count && (ypos<MAPY); ypos++)
			if(( sct[xpos][ypos].owner == country)
			&& (sct[xpos][ypos].jewels == 0)
			&& (is_habitable(xpos,ypos))) {
				if (count-- <=0) {
					getjewel( &sct[xpos][ypos] );
					done=TRUE;
				}
			}
			/* decrement to contain proper value */
			xpos--;
			ypos--;
			break;
		case 19:
		case 20:
			/*gold vein runs out one goldmine sector */
			holdval = 0;
			for (i=0; i<MAPX; i++) for (j=0; j<MAPY; j++)
			if(( sct[i][j].owner == country)
			&& (sct[i][j].designation == DGOLDMINE)){
				holdval+=sct[i][j].jewels;
			}
			if (holdval > 0) count = (rand()%holdval) + 1;
			else count = done = FALSE;
			if (count == FALSE ) break;
			for (xpos=0; count && (xpos<MAPX); xpos++)
			for (ypos=0; count && (ypos<MAPY); ypos++)
			if(( sct[xpos][ypos].owner == country)
			&& (sct[xpos][ypos].designation == DGOLDMINE)){
				count-=sct[xpos][ypos].jewels;
				if (count<=0) {
					count=0;
					done=TRUE;
					sct[xpos][ypos].designation = DNODESIG;
					sct[xpos][ypos].tradegood = TG_none;
					sct[xpos][ypos].jewels = 0;
				}
			}
			/* decrement to contain proper value */
			xpos--;
			ypos--;
			break;
		case 21:
			/*flood*/
			done=FALSE;
			break;
		case 22:	/*earthquake*/
			/* get epicenter */
			sptr = rand_sector();
			percent = 30+rand()%40;
			/* 10% damage in 3 sectors, 25 in 1, 50 in */
			for (x=xpos-3;x<=xpos+3;x++)
			for (y=ypos-3;y<=ypos+3;y++) if(ONMAP(x,y)) {
				reduce(x, y, percent/5 );
			}

			for (x=xpos-1;x<=xpos+1;x++)
			for (y=ypos-1;y<=ypos+1;y++) if(ONMAP(x,y)) {
				reduce(x,y,percent/5);	/* ADDITIONAL % */
				if((rand()%2) == 0)
					DEVASTATE(x,y);
			}

			if(sptr->fortress < 2) sptr->fortress = 0;
			else sptr->fortress -= 2;
			reduce(xpos, ypos, (percent*3)/5); /* ADDITIONAL % */
			break;
		case 23:
			/*frost -- crops ruined*/
			done=FALSE;
			break;
		case 24:
			/*dragon killed + 50000 jewels*/
			longval = rand()%10 * 10000;
			sprintf(eventstr,"you gain %ld jewels",longval);
			curntn->jewels+=longval;
			break;
		case 25:
#ifdef MONSTER
			/*nomad raid -- put large nomad army in area*/
			for( holdval=1;holdval<NTOTAL;holdval++ )
				if( ntn[holdval].active==NPC_NOMAD ) break;

			if( holdval==NTOTAL ) break;
			done=FALSE;
			if(holdval==NTOTAL) break;
			for(count=0; count < 100; count++) if(done <= 3){
			xpos=(rand()%(MAPX-8))+4;
			ypos=(rand()%(MAPY-8))+4;
			/* get army number */
			armynum = -1;
			for(newpower=0; newpower<MAXARM; newpower++)
				if (ntn[holdval].arm[newpower].sold == 0)
					armynum=newpower;
			if(armynum == -1) done=4;
			else if((is_habitable(xpos,ypos))
			&& ( sct[xpos][ypos].owner == country)) {
				ntn[holdval].arm[armynum].xloc =xpos;
				ntn[holdval].arm[armynum].yloc =ypos;
				if(curntn->tmil > 10000)	/* 800-4800 */
				ntn[holdval].arm[armynum].sold =800+50*(rand()%80);
				else if(curntn->tmil > 5000) /* 500-2500 */
				ntn[holdval].arm[armynum].sold =500+50*(rand()%40);
				else if(curntn->tmil > 1000) /* 400-1400 */
				ntn[holdval].arm[armynum].sold =400+20*(rand()%50);
				else	/* 200-600 */
				ntn[holdval].arm[armynum].sold =200+20*(rand()%20);
				ntn[holdval].arm[armynum].unittyp = A_LT_CAV;
				ntn[holdval].arm[armynum].stat =ATTACK;
				done++;
			}
			}
			done=TRUE;
			break;
#endif /* MONSTER */
		case 26:
			/*town burns -- reduce fort and redesignate*/
			holdval=0;
			for (xpos=0; xpos<MAPX; xpos++) 
			for (ypos=0; ypos<MAPY; ypos++)
			if(( sct[xpos][ypos].owner == country)
			&& ( sct[xpos][ypos].designation == DTOWN)){
				holdval++;
			}
			done = FALSE;
			if (holdval > 0) count = (rand()%holdval) + 1;
			else	break;

			percent = 25 + rand()%50;
			for (xpos=0; count && (xpos<MAPX); xpos++)
			for (ypos=0; count && (ypos<MAPY); ypos++)
			if(( sct[xpos][ypos].owner == country)
			&& ( done==FALSE )
			&& ( sct[xpos][ypos].designation == DTOWN)){
				count--;
				if (count<=0) {
					count=0;
					sct[xpos][ypos].designation = DNODESIG;
					reduce(xpos,ypos,percent);
					done=TRUE;
					break;
				}
			}
			/* decrement to contain proper value */
			xpos--;
			ypos--;
			break;
		case 27:
			/*plague -- 40% of populace in cities dies*/
			strcpy(eventstr,"40% of populace & armies in towns die");
			for (i=0; i<MAPX; i++) for (j=0; j<MAPY; j++)
			if( sct[i][j].owner == country ){
				sptr = &sct[i][j];
				if( ISCITY( sptr->designation )){
					sct[i][j].people *= 6;
					sct[i][j].people /= 10;
				}
			}
			for (armynum=0; armynum<MAXARM; armynum++)
			if((P_ASOLD > 0)&&(P_ATYPE<MINLEADER)){
				sptr = &sct[P_AXLOC][P_AYLOC];
				if( ISCITY( sptr->designation )){
					P_ASOLD *= (6);
					P_ASOLD /= (10);
				}
			}
			break;
		case 28: /*pirate raid on harbor*/
			done=FALSE;
			break;
		case 29: /*savages raid*/
			done=FALSE;
			break;
		case 30: /*new magician + RANDOM POWER*/
			/*buy new powers and/or new weapons*/
			if((newpower=getmagic(M_CIV))!=0L){
				for(i=S_CIV;i<S_CIV+E_CIV;i++) 
				if(powers[i]==newpower){
				sprintf(eventstr,"nation %s gets civilian power %s",curntn->name,pwrname[i]);
				}
				exenewmgk(newpower);
			}
			else done=FALSE;
			break;
		case 31: /*new magic item + RANDOM POWER*/
			/*buy new powers and/or new weapons*/
			if((newpower=getmagic(M_MIL))!=0){
				for(i=S_MIL;i<S_MIL+E_MIL;i++) 
				if(powers[i]==newpower)
				sprintf(eventstr,"nation %s gets military power %s",curntn->name,pwrname[i]);
				exenewmgk(newpower);
			}
			else done=FALSE;
			break;
		case 32:
			/* ores run out */
			holdval=0;
			for (i=0; i<MAPX; i++) for (j=0; j<MAPY; j++)
			if(( sct[i][j].owner == country)
			&& (sct[i][j].designation == DMINE)){
				holdval+= sct[i][j].metal;
			}

			done = FALSE;
			if (holdval > 0) count = (rand()%holdval) + 1;
			else break;

			for (xpos=0; count && (xpos<MAPX); xpos++)
			for (ypos=0; count && (ypos<MAPY); ypos++)
			if(( sct[xpos][ypos].owner == country)
			&& (done == FALSE )
			&& (sct[xpos][ypos].designation == DMINE)){
				count-=sct[xpos][ypos].metal;
				if (count<=0) {
					sct[xpos][ypos].metal =0;
					sct[xpos][ypos].tradegood =TG_none;
					sct[xpos][ypos].designation = DNODESIG;
					done=TRUE;
				}
			}
			/* decrement to contain proper value */
			xpos--;
			ypos--;
			break;
		case 33:
			/*new architect strengthens castle walls */
			for (i=0; i<MAPX; i++) for (j=0; j<MAPY; j++)
				if( sct[i][j].owner == country ){
					sptr = &sct[i][j];
					if( ISCITY( sptr->designation ))
						sct[i][j].fortress += 2;
				}
			break;
		case 34: /* new ores discovered */
			holdval=0;
			for (i=0; i<MAPX; i++) for (j=0; j<MAPY; j++)
			if(( sct[i][j].owner == country)
			&& (is_habitable(i,j))
			&&(sct[i][j].metal == 0)) holdval++;

			done = FALSE;
			if (holdval > 0) count = (rand()%holdval)+1;
			else break;

			for (xpos=0; count && (xpos<MAPX); xpos++)
			for (ypos=0; count && (ypos<MAPY); ypos++)
			if(( sct[xpos][ypos].owner == country)
			&& ( done == FALSE )
			&& (is_habitable(xpos,ypos))
			&& (sct[xpos][ypos].metal == 0)){
				if(count-- <= 0) {
					getmetal( &(sct[xpos][ypos]) );
					done=TRUE;
					break;
				}
			}
			/* decrement to contain proper value */
			xpos--;
			ypos--;
			break;
		case 35:
			/*diplomat sets up peace*/
			for(newnation=0;newnation<NTOTAL;newnation++) 
			if(( country!=newnation ) 
			&&( isntn( ntn[newnation].active ))
			&&( ntn[newnation].dstatus[country]>NEUTRAL )){
				ntn[newnation].dstatus[country]=NEUTRAL;
				curntn->dstatus[newnation]=NEUTRAL;
			}
			break;
		case 36:
			/*powerful magi curses nation; lose a power*/
			done=TRUE;
			i=holdval=0;
			while( powers[i] != 0 ){
				if(magic(country,powers[i])==TRUE) holdval++;
				i++;
			}
			if (holdval!=0) holdval = rand()%holdval+1;
			newpower=0L;
			for(i=0;powers[i]!=0 && holdval>0;i++) {
				if (magic(country,powers[i])==TRUE) holdval--;
				if (holdval==0) {
					newpower=powers[i];
					break;
				}
			}
			if(newpower==0L) {
				done=FALSE;
				break;
			}
			/* upgrade powers */
			switch(newpower) {
			case MI_MONST:
				if(magic(country,AV_MONST)==TRUE) {
					newpower=AV_MONST;
					i++;
				}
			case AV_MONST:
				if(magic(country,MA_MONST)==TRUE) {
					newpower=MA_MONST;
					i++;
				}
				break;
			case WARRIOR:
				if(magic(country,CAPTAIN)==TRUE) {
					newpower=CAPTAIN;
					i++;
				}
			case CAPTAIN:
				if(magic(country,WARLORD)==TRUE) {
					newpower=WARLORD;
					i++;
				}
				break;
			case SUMMON:
				if(magic(country,WYZARD)==TRUE) {
					newpower=WYZARD;
					i++;
				}
			case WYZARD:
				if(magic(country,SORCERER)==TRUE) {
					newpower=SORCERER;
					i++;
				}
				break;
			default:
				break;
			}
			/* may not remove racial magics */
			switch(curntn->race) {
			case ORC:
				if(newpower==MI_MONST) done=FALSE;
				break;
			case ELF:
				if(newpower==THE_VOID) done=FALSE;
				break;
			case DWARF:
				if(newpower==MINER) done=FALSE;
				break;
			case HUMAN:
				if(newpower==WARRIOR) done=FALSE;
				break;
			}
			switch(curntn->class) {
			case C_WIZARD:
				if(newpower==SUMMON) done=FALSE;
				if(newpower==WYZARD) done=FALSE;
				break;
			case C_PRIEST:
				if(newpower==RELIGION) done=FALSE;
				break;
			case C_PIRATE:
				if(newpower==SAILOR) done=FALSE;
				break;
			case C_TRADER:
				if(newpower==URBAN) done=FALSE;
				break;
			case C_WARLORD:
				if(newpower==WARLORD) done=FALSE;
				break;
			case C_DEMON:
				if(newpower==DESTROYER) done=FALSE;
				break;
			case C_DRAGON:
				if(newpower==MA_MONST) done=FALSE;
				break;
			case C_SHADOW:
				if(newpower==THE_VOID) done=FALSE;
				break;
			default:
				break;
			}
			/* remove the magic */
			if(done==TRUE) {
				if(magic(country,newpower)==TRUE) {
					curntn->powers ^= newpower;
					removemgk(newpower);
					sprintf(eventstr,"nation %s loses %s power",curntn->name,pwrname[i]);
				} else {
					done=FALSE;
					fprintf(stderr,"ERROR: removing no-magic\n");
				}
			}
			break;
		case 37:
			/*severe winter*/
			done=FALSE;
			break;
		case 38:
			/*tidal wave -- abandon all coastlands */
			done=FALSE;
			break;
		case 39: /*ninja attack paralyzes half your armys P_AMOVE=0*/
		case 40: /*general found to be spy P_AMOVE=0*/
 			strcpy(eventstr,"1/2 nations armies are paralyzed");
			for(armynum=0;armynum<MAXARM;armynum++) if(rand()%2==0)
				P_AMOVE = 0;
			break;
		case 41:
			/*general prosperity +20% gold*/
			if (curntn->tgold > 0l) {
				curntn->tgold += curntn->tgold / 5;
			}
			else 
				curntn->tgold += 50000L;
			break;
		case 42:
		case 43:
			/*kill 20% of armies*/
			for (armynum=0; armynum<MAXARM; armynum++)
				if((P_ASOLD > 0) && (P_ATYPE<MINLEADER)){
					P_ASOLD *= (8);
					P_ASOLD /= (10);
				}
			break;
		default:
			break;
		}
		if(done) wdisaster(country,xpos,ypos,percent,randevents[event]);
		}
	}
}

/*
 * wdisaster - Multi-channel event notification and reporting system
 *
 * Handles comprehensive event reporting through multiple communication channels
 * when random events, disasters, or political upheavals occur. This function
 * serves as the central notification hub, ensuring that all affected parties
 * and administrative systems receive appropriate event information.
 *
 * The function implements a three-tier notification system:
 * 1. **Global News System**: Public announcements visible to all players
 * 2. **Direct Mail System**: Private notifications to affected nation leaders
 * 3. **Administrative Console**: Debug and administrative event logging
 *
 * NOTIFICATION CHANNELS:
 * - **News File (fnews)**: Global event announcements for public consumption
 * - **Mail System (fm)**: Direct messages to affected player nation leaders
 * - **Console Output**: Administrative logging and debugging information
 * - **Location Reporting**: Optional coordinate disclosure based on HIDELOC flag
 *
 * PRIVACY AND SECURITY:
 * The function respects privacy settings through conditional compilation:
 * - HIDELOC flag controls whether event locations are publicly disclosed
 * - Volcanic eruption locations are specially protected from revelation
 * - Player nations receive detailed private notifications via mail system
 * - NPC nations only receive console logging without mail notifications
 *
 * MESSAGE FORMATTING:
 * - **News Format**: "event in nation_name" with optional details
 * - **Mail Format**: Formal administrative message with timestamp and location
 * - **Console Format**: Debug-friendly output with administrative details
 * - **Severity Reporting**: Percentage damage estimates when applicable
 *
 * Parameters:
 *   cntry - Nation index experiencing the event
 *   xloc  - X coordinate of event location (-1 if no specific location)
 *   yloc  - Y coordinate of event location (-1 if no specific location)
 *   prcnt - Percentage severity/damage of event (0 if not applicable)
 *   event - Descriptive string of the event that occurred
 *
 * Returns:
 *   void - performs notifications through side effects
 *
 * Side Effects:
 *   - Writes event announcements to global news file (fnews)
 *   - Sends private mail messages to affected player nation leaders
 *   - Outputs administrative information to console/stdout
 *   - Opens and closes mail system for affected players
 *   - Uses global eventstr variable for additional event details
 *   - Respects HIDELOC compilation flag for location privacy
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires mail system and file I/O infrastructure
 *   Approach: Integration testing with mock file systems and mail infrastructure
 *   Key Tests: [Player vs NPC nations, location privacy, mail system failures]
 *   Dependencies: Mail system (mailopen/mailclose), news file, console output
 *   Mock Requirements: Mock file I/O, mail system, and console output
 *   Complexity: Moderate - Multi-channel I/O coordination with conditional logic
 *
 * Notes:
 *   - Assumes fnews (news file pointer) is properly initialized and open
 *   - Mail system calls (mailopen/mailclose) must be available for player nations
 *   - Global eventstr variable provides additional context when length > 5
 *   - Location coordinates -1,-1 indicate events without specific geographic focus
 *   - HIDELOC compilation controls public disclosure of sensitive event locations
 *   - Special protection for volcanic eruption location disclosure
 *   - Function handles both localized events (with coordinates) and general events
 *   - Percentage reporting allows players to assess event severity/impact
 *   - Administrative console output aids in game monitoring and debugging
 */
void
wdisaster( cntry,xloc,yloc,prcnt,event )
int cntry,xloc,yloc,prcnt;
char *event;
{
	fprintf(fnews,"1. \t%s in %s\n",event,ntn[cntry].name);
	printf("\t%s in %s\n",event,ntn[cntry].name);

	/*send a message to the country if it is a PC*/
	if(ispc(ntn[cntry].active)) {
		if(mailopen( cntry )!=(-1)) {
		fprintf(fm,"MESSAGE FROM CONQUER\n\n");
		fprintf(fm,"An event occurs within your nation (%s)\n",ntn[cntry].name);
		fprintf(fm,"%s during the %s of Year %d,\n",event,PSEASON(TURN),YEAR(TURN));
		if(xloc != -1)
		fprintf(fm," centered around location %d, %d.\n",xloc,yloc);
		if(prcnt>0) {
			fprintf(fm,"Damage was estimated at about %d%% in severity.\n",prcnt);
		}
		}
	}

	if(strlen(eventstr)>5) {
#ifdef HIDELOC
		/* make sure that volcano locations are not revealed */
		if(strcmp(eventstr,"all flee, 30%% die in 1 sector range")!=0)
#endif /* HIDELOC */
		fprintf(fnews,"1. \tevent in %s -->%s\n",ntn[cntry].name,eventstr);
#ifndef HIDELOC
		if(xloc != -1)
		fprintf(fnews,"1. \tevent in %s -->centered around location %d, %d.\n",ntn[cntry].name,xloc,yloc);
#endif /* HIDELOC */
		printf("\t\t->%s\n",eventstr);
		if(ispc(ntn[cntry].active))
			fprintf(fm,"\t%s\n",eventstr);
	}
	if(ispc(ntn[cntry].active)) mailclose(cntry);
}

/*
 * peasant_revolt - Create and deploy peasant rebellion nation with militia armies
 *
 * Implements the specialized mechanics for peasant revolts, which differ significantly
 * from political revolts in their scope, organization, and military capabilities.
 * This function creates a new peasant nation through territory dissolution and
 * then deploys militia armies across undefended territories of the parent nation.
 *
 * PEASANT REVOLT CHARACTERISTICS:
 * - **Limited Initial Territory**: Only 10% of parent nation through disolve()
 * - **Militia-Based Forces**: Creates local militia from civilian populations
 * - **Opportunistic Expansion**: Spreads to undefended sectors with 50% probability
 * - **Population-Driven**: Militia strength based on local civilian populations
 * - **Anti-Government**: Automatically at war with parent nation and others
 *
 * EXPANSION MECHANICS:
 * After initial nation creation, the function implements territorial expansion
 * by examining all sectors of the parent nation and converting undefended
 * territories to the new peasant nation. Each conversion:
 * - Creates local militia army from 1/5th of sector population
 * - Requires absence of existing military forces in the sector
 * - Excludes capitol sectors from conversion (political protection)
 * - Only affects sectors with existing populations
 *
 * NPC BEHAVIOR DIFFERENCES:
 * The function includes "cheating" logic for NPC nations, giving them a 50%
 * chance to avoid peasant revolts entirely. This balances gameplay by preventing
 * excessive NPC fragmentation while maintaining player challenge.
 *
 * ARMY DEPLOYMENT SYSTEM:
 * - **Militia Type**: A_MILITIA units with MILITIA status for defensive operations
 * - **Population Source**: 1/5th of sector civilians become militia soldiers
 * - **Local Defense**: Armies stationed in their home sectors for local control
 * - **Army Slot Management**: Uses first available army slot for each new militia
 *
 * Parameters:
 *   newnation - Pointer to integer that receives the new peasant nation index
 *               Set to 0 if revolt fails, otherwise contains valid nation number
 *
 * Returns:
 *   void - success/failure indicated through newnation parameter
 *
 * Side Effects:
 *   - Creates new peasant nation through disolve() with 10% split
 *   - Sets nation status to NPC_PEASANT and class to 0 (civilian)
 *   - Transfers ownership of undefended sectors to new peasant nation
 *   - Creates militia armies in converted territories using available army slots
 *   - Reduces civilian populations by amount converted to militia
 *   - Sets peasant nation popularity to 99 to prevent cascading revolts
 *   - Modifies global curntn pointer temporarily during nation setup
 *   - Updates global eventstr through disolve() if revolt fails
 *
 * Testing Notes:
 *   Category: C (System) - Complex function requiring full nation and army simulation
 *   Approach: System testing with complete game state including armies and populations
 *   Key Tests: [Various army configurations, population distributions, NPC vs player]
 *   Dependencies: Complete nation array, army arrays, sector ownership, disolve function
 *   Mock Requirements: Full military simulation with army slot management
 *   Complexity: Complex - Multi-phase nation creation with army deployment
 *
 * Notes:
 *   - NPC nations have 50% chance to avoid peasant revolts ("cheating" mechanism)
 *   - Uses disolve() with ispsnt=TRUE flag for peasant-specific nation creation
 *   - Temporarily modifies global curntn pointer during new nation configuration
 *   - High peasant popularity (99) prevents immediate follow-up revolts
 *   - Militia armies are defensive units (MILITIA status) rather than mobile forces
 *   - Function assumes army slots are available for militia creation
 *   - Capitol sectors are protected from peasant conversion
 *   - Only sectors with existing populations can generate militia forces
 *   - solds_in_sector() check prevents conversion of militarily defended areas
 */
void
peasant_revolt(int *newnation)	/* peasant revolt */
{
	register int i,j;
	int	armynum;

	/* a little cheating for now */
	if(isnpc( ntn[country].active ) && (rand()%2==0)) {
		strcpy(eventstr,"npc cheating");
		return;
	}
	if((*newnation=disolve(10, country, TRUE)) == 0 ) return;

	curntn = &ntn[*newnation];	/* beware - curntn & country not same */
	curntn->active = NPC_PEASANT;
	curntn->class = 0;

	for (i=0; i<MAPX; i++) for (j=0; j<MAPY; j++) {
		if(( sct[i][j].owner == country)
		&&( rand()%2==0 ) 
		&&( sct[i][j].designation != DCAPITOL )
		&&( sct[i][j].people > 0 )
		&&( solds_in_sector(i,j,country)==0)){
			sct[i][j].owner = *newnation;
			for(armynum=0;armynum<MAXARM;armynum++)
			if(P_ASOLD == 0) {
				P_ASOLD = sct[i][j].people/5;
				P_ATYPE = A_MILITIA;
				P_ASTAT = MILITIA;
				P_AXLOC = i;
				P_AYLOC = j;
				sct[i][j].people -= P_ASOLD;
				break;
			}
		}
	}
	curntn->popularity=99;	/* so it dont happen too often */
	curntn = &ntn[country];	/* fix above beware comment */
	return;
}
/*
 * other_revolt - Generate political revolts with random causes and escalating severity
 *
 * Implements sophisticated political revolt mechanics that create new independent
 * nations through various political and military upheavals. Unlike peasant revolts,
 * these are organized political movements with proper resources, military units,
 * and diplomatic relationships.
 *
 * REVOLT TYPE SYSTEM:
 * The function uses random selection among 8 different revolt types, each with
 * distinct characteristics and territorial percentages:
 * - **Types 0-3**: Limited revolts (10% territory) - localized independence movements
 * - **Types 4-7**: Major revolts (20-30% territory) - significant political upheavals
 *
 * POLITICAL REVOLT CATEGORIES:
 * 0. **Military Rebellion**: General or officer corps revolt (10% split)
 * 1. **Religious Cult**: Faith-based separatist movement (10% split)
 * 2. **Provincial Rebellion**: Regional independence movement (10% split)
 * 3. **Evil Wizard**: Magical tyranny establishment (10% split)
 * 4. **Tax Revolt**: Anti-taxation independence movement (20% split)
 * 5. **Open Rebellion**: Large-scale organized uprising (30% split)
 * 6. **Military Takeover**: General seizes control of province (30% split)
 * 7. **Religious Schism**: Major religious split creating new nation (30% split)
 *
 * ESCALATING SEVERITY MODEL:
 * The revolt system implements escalating territorial loss based on revolt type:
 * - **Minor Political Issues** (0-3): 10% territorial loss
 * - **Economic/Social Issues** (4): 20% territorial loss
 * - **Major Upheavals** (5-7): 30% territorial loss
 * This reflects how different types of political instability affect nation cohesion.
 *
 * RESOURCE AND MILITARY INHERITANCE:
 * Unlike peasant revolts, political revolts create fully functional nations with:
 * - Proportional resource transfers (gold, food, jewels, metals)
 * - Professional military units rather than militia
 * - Diplomatic relationships with other nations
 * - Proper capital cities and governmental structures
 *
 * Parameters:
 *   new - Pointer to integer that receives the new nation index
 *         Set to 0 if revolt fails, otherwise contains valid nation number
 *
 * Returns:
 *   Revolt type index (0-7) corresponding to randevents[] array for event description
 *   -1 if revolt fails (no territory split, insufficient resources, etc.)
 *
 * Side Effects:
 *   - Creates new independent nation through disolve() with ispsnt=FALSE
 *   - Transfers proportional resources and military units to new nation
 *   - Establishes realistic diplomatic relationships for new nation
 *   - Sets up proper governmental and military structures
 *   - Updates global eventstr through disolve() if revolt fails
 *   - Modifies parent nation by reducing territory and resources
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires nation management and resource systems
 *   Approach: Integration testing with mock nation states and resource management
 *   Key Tests: [Various revolt types, resource distributions, territory sizes]
 *   Dependencies: disolve() function, nation resource management, diplomatic system
 *   Mock Requirements: Mock nation states with varying resource and territory configurations
 *   Complexity: Moderate - Coordinated resource and territory management with random selection
 *
 * Notes:
 *   - Uses disolve() with ispsnt=FALSE for full political nation creation
 *   - Random revolt type selection provides gameplay variety and unpredictability
 *   - Escalating severity model reflects realistic political instability patterns
 *   - Function returns event type for proper notification through randevents[] array
 *   - Failure cases handled gracefully with -1 return and error strings in eventstr
 *   - Political revolts create stronger, more viable nations than peasant revolts
 *   - Territorial percentages balance gameplay impact with historical realism
 *   - Each revolt type corresponds to specific event description in randevents[]
 */
int
other_revolt( new )	/* return reason and new nation number*/
int	*new;
{
	short	reason = rand()%8;
	switch( reason ) {
	case 0: /* general */
		if((*new=disolve(10, country, FALSE))!=0) return(reason);
		break;
	case 1: /* cult */
		if((*new=disolve(10, country, FALSE))!=0) return(reason);
		break;
	case 2: /* general/province defects*/
		if((*new=disolve(10, country, FALSE))!=0) return(reason);
		break;
	case 3: /* evil wizard sets up */
		if((*new=disolve(10, country, FALSE))!=0) return(reason);
		break;
	case 4: /* tax revolt */
		if((*new=disolve(20, country, FALSE))!=0) return(reason);
		break;
	case 5: /* open rebellion */
		if((*new=disolve(30, country, FALSE))!=0) return(reason);
		break;
	case 6: /* general takes over province*/
		if((*new=disolve(30, country, FALSE))!=0) return(reason);
		break;
	case 7: /*religious schism*/
		if((*new=disolve(30, country, FALSE))!=0) return(reason);
		break;
	}
	return(-1);
}

#ifdef VULCANIZE
/*
 * erupt - Global volcanic eruption system with random volcano selection
 *
 * Implements a world-wide volcanic eruption system that randomly selects and
 * triggers volcanic activity across the game map. This function provides
 * dramatic natural disaster events that can significantly impact gameplay
 * through localized devastation and territorial disruption.
 *
 * VOLCANO DETECTION AND SELECTION:
 * The function operates through a two-phase process:
 * 1. **Census Phase**: Scans entire map to count all VOLCANO vegetation sectors
 * 2. **Selection Phase**: Uses random selection to choose one volcano for eruption
 *
 * RANDOM VOLCANO SELECTION ALGORITHM:
 * The selection process ensures equal probability for all volcanoes:
 * - Counts total volcanoes across entire map (nvolcanos)
 * - Generates random number from 0 to (nvolcanos-1)
 * - Performs second map scan, decrementing counter until target volcano reached
 * - Triggers eruption through blowup() function for selected coordinates
 *
 * WORLD-WIDE IMPACT POTENTIAL:
 * Unlike other natural disasters that affect single nations, volcanic eruptions:
 * - Can occur anywhere on the map regardless of nation ownership
 * - Affect multiple nations if eruption occurs near borders
 * - Provide unpredictable global events that no player can fully control
 * - Create permanent geographical changes through devastation effects
 *
 * COMPILATION DEPENDENCY:
 * This function is conditionally compiled under the VULCANIZE flag, allowing
 * game administrators to enable/disable volcanic activity at compile time.
 * This supports different gameplay preferences and world configurations.
 *
 * INTEGRATION WITH DEVASTATION SYSTEM:
 * The function delegates actual eruption effects to blowup() which handles:
 * - Population casualties and displacement
 * - Fortress destruction and infrastructure damage
 * - Resource depletion (jewels, metals) in affected areas
 * - Multi-sector damage patterns with diminishing effects by distance
 *
 * Parameters:
 *   None - operates on global map state
 *
 * Returns:
 *   void - effects achieved through blowup() function calls
 *
 * Side Effects:
 *   - Scans entire map twice for volcano detection and selection
 *   - Calls blowup() function for selected volcano coordinates
 *   - Outputs administrative messages to console for monitoring
 *   - No direct map modifications (delegated to blowup function)
 *   - May trigger multi-nation event notifications through blowup
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires map state and devastation system integration
 *   Approach: Integration testing with mock map configurations and volcano distributions
 *   Key Tests: [No volcanoes, single volcano, multiple volcanoes, edge cases]
 *   Dependencies: Map vegetation system, blowup() function, console output
 *   Mock Requirements: Mock map with various volcano configurations
 *   Complexity: Moderate - Map scanning with random selection and delegation
 *
 * Notes:
 *   - Function safely handles maps with no volcanoes (early return)
 *   - Random selection ensures fair distribution across all available volcanoes
 *   - Two-pass algorithm trades efficiency for simplicity and correctness
 *   - Console output provides administrative monitoring of volcanic activity
 *   - VULCANIZE compilation flag allows selective inclusion of volcanic events
 *   - Function designed for periodic calling from main event processing
 *   - Volcano selection algorithm has O(n) complexity where n = total map sectors
 *   - No validation of volcano sector validity before triggering eruption
 */
void
erupt()
{
	int i, j, nvolcanos=0, volhold;

	printf("checking for volcanic eruptions\n");
	/* count all of the volcanos */
	for (i=0; i<MAPX; i++) for (j=0; j<MAPY; j++)
		if(sct[i][j].vegetation == VOLCANO) nvolcanos++;

	if(nvolcanos==0) {
		printf("no volcano's found\n");
		return;
	}
	/* choose a random one to erupt */
	volhold = rand()%nvolcanos;
	for (i=0; i<MAPX; i++) for (j=0; j<MAPY; j++)
	if ( sct[i][j].vegetation == VOLCANO ) {
		volhold--;
		if (volhold == 0) blowup(i,j);
	}
}
#endif /* VULCANIZE */

/*
 * blowup - Execute volcanic eruption devastation with multi-sector damage patterns
 *
 * Implements the complete devastation effects of a volcanic eruption at specific
 * coordinates, including immediate destruction at ground zero and cascading
 * damage to surrounding areas. This function provides the most severe natural
 * disaster effect in the game, capable of permanently altering landscapes
 * and eliminating entire populations.
 *
 * DEVASTATION MECHANICS:
 * The function implements a two-tier damage system:
 * 1. **Ground Zero**: Complete devastation (100% casualties, total resource loss)
 * 2. **Adjacent Sectors**: Severe damage (30% casualties) in 1-sector radius
 *
 * GROUND ZERO EFFECTS:
 * The eruption epicenter experiences total devastation:
 * - **Population**: 100% casualties through reduce() function
 * - **Resources**: Complete depletion of jewels and metals
 * - **Infrastructure**: Total fortress destruction (set to 0)
 * - **Terrain**: Ensured VOLCANO vegetation designation
 * - **Geography**: Permanent devastation marking through DEVASTATE() macro
 *
 * ADJACENT SECTOR DAMAGE:
 * All non-water sectors within 1-sector radius experience severe effects:
 * - **Population**: 30% casualties through reduce() function
 * - **Infrastructure**: Complete fortress destruction
 * - **Terrain**: Devastation marking for long-term impact
 * - **Water Protection**: Water sectors immune to volcanic effects
 *
 * NOTIFICATION SYSTEM INTEGRATION:
 * The function generates comprehensive event notifications:
 * - **Administrative**: Console output with coordinates and scope
 * - **News System**: Global announcement through fnews file
 * - **Mail System**: Direct notification to affected nation through wdisaster()
 * - **Event Reporting**: 100% severity classification for maximum impact
 *
 * PERMANENT GEOGRAPHICAL CHANGES:
 * Volcanic eruptions create lasting environmental modifications:
 * - **Vegetation**: Ensures VOLCANO type at eruption center
 * - **Devastation**: Permanent marking of affected areas
 * - **Resource Depletion**: Permanent loss of extractable materials
 * - **Infrastructure**: Complete destruction requiring rebuilding
 *
 * Parameters:
 *   i - X coordinate of volcanic eruption epicenter
 *   j - Y coordinate of volcanic eruption epicenter
 *
 * Returns:
 *   void - effects achieved through direct map and population modification
 *
 * Side Effects:
 *   - Calls reduce() for population casualties at epicenter and adjacent sectors
 *   - Calls DEVASTATE() macro for permanent terrain marking
 *   - Calls wdisaster() for comprehensive event notification
 *   - Modifies sector vegetation, resources, and fortress values directly
 *   - Outputs administrative information to console and news systems
 *   - Creates permanent geographical and demographic changes
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires map state, population system, and notification integration
 *   Approach: Integration testing with mock map sectors and population distributions
 *   Key Tests: [Edge locations, water boundaries, populated vs unpopulated areas]
 *   Dependencies: Map system, reduce() function, DEVASTATE() macro, notification systems
 *   Mock Requirements: Mock map with various terrain types and population distributions
 *   Complexity: Moderate - Coordinated multi-system damage application with area effects
 *
 * Notes:
 *   - Water sectors (WATER altitude) are immune to volcanic damage effects
 *   - DEVASTATE() macro appears to be called incorrectly on line 1398 (should be x,y not i,j)
 *   - Function ensures VOLCANO vegetation at epicenter regardless of previous terrain
 *   - 100% damage classification makes this the most severe disaster event type
 *   - Administrative console output aids in monitoring major geological events
 *   - Function designed for integration with random event system and volcano selection
 *   - Fortress destruction affects defensive capabilities permanently
 *   - Resource depletion impacts economic recovery for affected nations
 *   - No bounds checking on coordinates - assumes valid map positions
 */
void
blowup(i,j)
register int i,j;
{
	register int x,y;
	wdisaster(sct[i][j].owner,i,j,100,"volcano erupted");
	printf("\tvolcano at %d, %d erupts; devastates surounding area\n",i,j);
	fprintf(fnews,"1. \tevent in sector %d, %d->volcanic eruption causes devastation\n",i,j);
	sct[i][j].vegetation = VOLCANO;
	sct[i][j].jewels = 0;
	sct[i][j].metal = 0;
	reduce(i,j,100);
	DEVASTATE(i,j);
	sct[i][j].fortress = 0;
	/* decrease neighboring population and armies 30% */
	for(x=i-1; x<=i+1; x++) for(y=j-1; y<=j+1; y++)
	if((ONMAP(x,y))&&(sct[x][y].altitude != WATER)) {
		reduce(x,y,30);
		DEVASTATE(i,j);
		sct[x][y].fortress = 0;
	}
}

/** reduce will drop armies & and civilians in sector by percent **/
void
reduce(x,y,percent)
int x,y,percent;
{
	long temp;	/* used to avoid overflow problems */
	int armynum,ctry;

	percent = 100 - percent;	/* invert percent so math works */

	/* work on people */
	temp = sct[x][y].people;
	temp *= percent;
	temp /= 100;
	sct[x][y].people = temp;

	/* work on armies */
	for(ctry=1;ctry<NTOTAL;ctry++) {
		for(armynum=0;armynum<MAXARM;armynum++)
		if((ntn[ctry].arm[armynum].xloc==x)
		&&(ntn[ctry].arm[armynum].unittyp<MINLEADER)
		&&(ntn[ctry].arm[armynum].yloc==y)) {
			temp = ntn[ctry].arm[armynum].sold;
			temp *= percent;
			temp /= 100;
			ntn[ctry].arm[armynum].sold = temp;
		}
	}
}

/* returns pointer to random sector in country */
struct s_sector
*rand_sector()
{
	int count=0;
	for(xpos=0;xpos<MAPX;xpos++) for(ypos=0;ypos<MAPY;ypos++)
		if(sct[xpos][ypos].owner == country) count++;
	count = rand()%count;
	for(xpos=0;xpos<MAPX;xpos++) for(ypos=0;ypos<MAPY;ypos++){
		if(sct[xpos][ypos].owner == country) count--;
		if(count==0) return(&sct[xpos][ypos]);
	}
	fprintf(stderr,"could find no location for country %d\n",country);
	abrt();
	return(NULL);	/* stop lint from complaining */
}

void
weather()
{
}
#endif /* RANEVENT */
