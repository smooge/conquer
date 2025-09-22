/*
 * update.c - Game state update and turn processing
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
#ifndef XENIX
#include <sys/types.h>
#include <sys/file.h>
#endif
#include <unistd.h>
#include "header.h"
#include "data.h"

extern FILE *fnews;

extern short country;
int	disarray;		/* TRUE if nation in disarray */
int	**attr;			/* sector attractiveness */
long	**newpop;		/* storage for old population */

/*
 * dtol - Convert double to long with precision control
 *
 * This utility function converts a double-precision floating point value
 * to a long integer using string formatting as an intermediate step.
 * The conversion uses sprintf with "%-60.0lf" format to eliminate fractional
 * parts and handle precision issues that might occur with direct casting.
 *
 * This approach was likely chosen to handle potential floating-point
 * precision issues in older systems or to ensure consistent rounding
 * behavior across different platforms.
 *
 * Parameters:
 *   d - Double value to convert to long integer
 *
 * Returns:
 *   Long integer representation of the input double (fractional part discarded)
 *
 * Side Effects:
 *   - Uses local buffer BIGLTH in size for string conversion
 *   - No global state modifications
 *
 * Testing Notes:
 *   Category: A (Unit Testable)
 *   Approach: Unit tests with various floating-point values
 *   Key Tests: Normal values, large values, negative values, edge cases near limits
 *   Dependencies: None - isolated utility function
 *   Mock Requirements: None
 *   Complexity: Simple - straightforward conversion utility
 *
 * Notes:
 *   - Thread safe - uses only local variables
 *   - Consider replacing with direct casting in modern C implementations
 *   - String formatting approach may be less efficient than direct conversion
 *   - BIGLTH buffer size should be sufficient for largest double representations
 */
long
dtol (double d)
{
	char tempstr[BIGLTH];
	long l;
	sprintf(tempstr,"%-60.0lf",d);
	sscanf(tempstr,"%ld",&l);
	return(l);
}

/*
 * update - Master turn processing coordinator for the entire game world
 *
 * This is the central orchestrator function that executes a complete game turn
 * by coordinating all major game subsystems in a carefully designed sequence.
 * It represents the core game loop that processes one complete turn cycle,
 * updating all nations, resolving conflicts, and advancing the world state.
 *
 * The function follows a strict execution order to ensure game consistency:
 * 1. Initialize turn infrastructure (news file, integrity checks)
 * 2. Execute all nations in random order (diplomatic, economic, military actions)
 * 3. Process special nation types (monsters, if enabled)
 * 4. Resolve combat between all military units
 * 5. Handle sector capture and territorial changes
 * 6. Process trade agreements (if trade system enabled)
 * 7. Update military units and movement capabilities
 * 8. Execute random events (if random event system enabled)
 * 9. Update all world sectors (population, resources, development)
 * 10. Process economic systems (food consumption, trade goods, famine)
 * 11. Handle leader birth and development
 * 12. Check for nation destruction due to low population/military
 * 13. Finalize news reporting and file management
 * 14. Apply NPC difficulty balancing (if cheat system enabled)
 * 15. Calculate nation scores and rankings
 * 16. Process mercenary system updates
 * 17. Clean up temporary files and sort news
 * 18. Advance turn counter and recalculate nation attributes
 *
 * Parameters:
 *   None - operates on global game state
 *
 * Returns:
 *   void - does not return values, modifies global game state
 *
 * Side Effects:
 *   - Modifies all global game data structures
 *   - Creates and writes news files for the current turn
 *   - Executes system commands for file management and sorting
 *   - Increments global TURN counter
 *   - Calls check() multiple times for integrity validation
 *   - May destroy nations that fall below survival thresholds
 *   - Updates nation attributes, scores, and military capabilities
 *   - Processes file I/O for news management and cleanup
 *
 * Testing Notes:
 *   Category: C (System Level Only)
 *   Approach: Full system integration testing with complete game state
 *   Key Tests: Turn sequence integrity, file operations, nation lifecycle
 *   Dependencies: All major game subsystems, file system, global game state
 *   Mock Requirements: File system, system command execution, all subsystem functions
 *   Complexity: Complex - orchestrates entire game turn with multiple interdependent systems
 *
 * Notes:
 *   - This function represents the core game loop execution
 *   - Order of operations is critical for game balance and consistency
 *   - Multiple conditional compilation blocks for optional features
 *   - Heavy reliance on global state makes this difficult to unit test
 *   - File operations and system commands require careful error handling
 *   - News file management ensures player communication and game history
 *   - Integrity checks (check()) provide critical error detection throughout turn processing
 */
void
update (void)
{
	char command[BIGLTH],filename[FILELTH];

	sprintf(filename,"%s%d",newsfile,TURN);
	if ((fnews=fopen(filename,"w"))==NULL) {
		printf("error opening news file\n");
		exit(FAIL);
	}
	check();

	updexecs();	/*run each nation in a random order*/
	check();

#ifdef MONSTER
	check();
	monster();	/* update monster nations */
	check();
#endif

	check();
	combat();	/* run combat */
	check();
	updcapture();	/* capture unoccupied sectors */

#ifdef TRADE
	uptrade();	/* update trade */
#endif

	updmil();	/* reset military stuff for whole world */

#ifdef RANEVENT
	randomevent();	/*run random events after setting movements */
#endif /* RANEVENT */

	updsectors();	/* for whole map, update one sector at a time*/
	updcomodities();/* commodities & food, metal, jewels */
	updleader();	/* new leaders are born, old leaders become wiser */

	/* check for destroyed nations */
	for(country=1;country<NTOTAL;country++)
	if (isntn(ntn[country].active)) {
		if (ntn[country].tciv < 100 &&
		    ntn[country].tmil < TAKESECTOR)
			destroy(country);
	}

	fprintf(fnews,"1\tIMPORTANT WORLD NEWS\n");
	fprintf(fnews,"5\tGLOBAL ANNOUNCEMENTS (see mail)\n");
	fclose(fnews);

#ifdef CHEAT
	cheat();
#endif /* CHEAT */

	score();	/* score all nations */

	/* check for mercenary increase 5% chance*/
	if (rand()%20==0) {
		printf("increasing mercenary bonuses\n");
		MERCATT++;
		MERCDEF++;
	}
	sprintf(command,"/bin/rm -f %s*",exefile);
	printf("%s\n",command);
	system(command);

	sprintf( command,"%s/%s %s %s", EXEDIR, sortname, filename, filename );
	printf("%s\n",command);
	system(command);

	/* remove old news files */
	if (TURN>MAXNEWS) {
		sprintf(filename,"%s%d",newsfile,TURN-MAXNEWS);
		unlink(filename);
	}

	/* increase turn number by one */
	TURN++;
	att_base();	/* calculate base for nation attributes */
	att_bonus();	/* calculate tradegood bonus for nation attributes */
}

/*
 * attract - Calculate sector attractiveness for civilian population movement
 *
 * Calculates how attractive a specific sector is to civilian populations
 * for migration purposes. The calculation considers sector designation,
 * natural resources, trade goods, and race-specific preferences.
 * This is a core component of the population movement algorithm that
 * drives demographic changes across the game world.
 *
 * Algorithm:
 * 1. Base attractiveness from trade goods and their values
 * 2. Designation-specific bonuses (cities, farms, mines, etc.)
 * 3. Race-specific modifiers based on cultural preferences
 *   - Dwarves prefer mountains and mining sectors
 *   - Elves prefer forests and natural environments
 *   - Humans are adaptable to most environments
 *   - Orcs have their own environmental preferences
 * 4. Special case handling for food-starved nations (farm attractiveness)
 * 5. Devastated or inaccessible sectors have zero attractiveness
 *
 * Parameters:
 *   x - X coordinate of the sector (0-MAPX)
 *   y - Y coordinate of the sector (0-MAPY)
 *   race - Race type (DWARF, ELF, HUMAN, ORC) determining preferences
 *
 * Returns:
 *   Integer attractiveness value (0 or positive)
 *   0 for devastated sectors or inaccessible terrain
 *   Higher values indicate more attractive sectors for migration
 *
 * Side Effects:
 *   - None - pure calculation function with no state changes
 *   - Reads from global sector array sct[x][y]
 *   - Reads from global nation array ntn[] for food calculations
 *
 * Testing Notes:
 *   Category: A (Unit Testable)
 *   Approach: Unit tests with mock sector data and race parameters
 *   Key Tests: Race preferences, trade good bonuses, devastation handling,
 *              food scarcity farm attractiveness, resource-based bonuses
 *   Dependencies: Global sct[][] array, ntn[] array, trade good constants
 *   Mock Requirements: Sector data structures, nation food statistics
 *   Complexity: Moderate - multiple conditional branches with race logic
 *
 * Notes:
 *   - Central to population migration mechanics in move_people()
 *   - Race-specific constants (DWOODATTR, EGOLDATTR, etc.) drive preferences
 *   - Food scarcity calculation affects farm attractiveness dynamically
 *   - Devastated sectors always return 0 regardless of other factors
 *   - Negative movement costs indicate inaccessible terrain
 */
int
attract(int x,int y,int race)
{
	register struct s_sector	*sptr = &sct[x][y];
	int	designation;
	int	Attr = 0;

	designation=sptr->designation;
	if(sptr->tradegood != TG_none
	&& *(tg_stype+sptr->tradegood)==designation ) {
		if((designation!=DMINE)
		&& (designation!=DGOLDMINE))
		Attr += ( tg_value[sptr->tradegood] - '0' )*TGATTR;
	}

	if(designation==DGOLDMINE){
		if(sptr->jewels>=6) Attr+=GOLDATTR*sptr->jewels*2;
		else	Attr+=GOLDATTR*sptr->jewels;
	} else if(designation==DFARM){
		if(ntn[sptr->owner].tfood*250 <= ntn[sptr->owner].eatrate*(ntn[sptr->owner].tciv*11))
			Attr+=50*FARMATTR;
		else Attr+=tofood(sptr,sptr->owner)*FARMATTR;
	}
	else if(designation==DCITY) Attr+=CITYATTR;
	else if(designation==DCAPITOL) Attr+=CITYATTR;
	else if(designation==DTOWN) Attr+=TOWNATTR;
	else if(designation==DMINE) {
		if(sptr->metal>6) Attr+=MINEATTR*sptr->metal*2;
		else Attr+=MINEATTR*sptr->metal;
	} else if((designation!=DROAD)&&(designation!=DNODESIG)
	&&(designation!=DDEVASTATED)&& is_habitable(x,y) ) Attr+= OTHRATTR;

	switch(race){
	case DWARF:
		if((designation==DGOLDMINE)&&(sptr->jewels>3))
			Attr += DGOLDATTR;
		else if((designation==DMINE)&&(sptr->metal>3))
			Attr += DMINEATTR;
		else if(designation==DTOWN) Attr += DTOWNATTR;
		else if(designation==DCITY) Attr += DCITYATTR;
		else if(designation==DCAPITOL) Attr += DCITYATTR;

		if(sptr->vegetation==WOOD) Attr += DWOODATTR;
		else if(sptr->vegetation==FOREST) Attr += DFOREATTR;

		if(sptr->altitude==MOUNTAIN) Attr += DMNTNATTR;
		else if(sptr->altitude==HILL) Attr += DHILLATTR;
		else if(sptr->altitude==CLEAR) Attr += DCLERATTR;
		else Attr=0;
		break;
	case ELF:
		if((designation==DGOLDMINE)&&(sptr->jewels>3))
			Attr += EGOLDATTR;
		else if((designation==DMINE)&&(sptr->metal>3))
			Attr += EMINEATTR;
		else if(designation==DTOWN) Attr += ECITYATTR;
		else if(designation==DCITY) Attr += ECITYATTR;
		else if(designation==DCAPITOL) Attr += ECITYATTR;

		if(sptr->vegetation==WOOD) Attr += EWOODATTR;
		else if(sptr->vegetation==FOREST) Attr += EFOREATTR;

		if(sptr->altitude==MOUNTAIN) Attr += EMNTNATTR;
		else if(sptr->altitude==HILL) Attr += EHILLATTR;
		else if(sptr->altitude==CLEAR) Attr += ECLERATTR;
		else Attr=0;
		break;
	case HUMAN:
		if((designation==DGOLDMINE)&&(sptr->jewels>3))
			Attr += HGOLDATTR;
		else if((designation==DMINE)&&(sptr->metal>3))
			Attr += HMINEATTR;
		else if(designation==DTOWN) Attr += HCITYATTR;
		else if(designation==DCITY) Attr += HCITYATTR;
		else if(designation==DCAPITOL) Attr += HCITYATTR;

		if(sptr->vegetation==WOOD) Attr += HWOODATTR;
		else if(sptr->vegetation==FOREST) Attr += HFOREATTR;

		if(sptr->altitude==MOUNTAIN) Attr += HMNTNATTR;
		else if(sptr->altitude==HILL) Attr += HHILLATTR;
		else if(sptr->altitude==CLEAR) Attr += HCLERATTR;
		else Attr=0;
		break;
	case ORC:
		if((designation==DGOLDMINE)&&(sptr->jewels>3))
			Attr += OGOLDATTR;
		else if((designation==DMINE)&&(sptr->metal>3))
			Attr += OMINEATTR;
		else if(designation==DTOWN) Attr += OCITYATTR;
		else if(designation==DCITY) Attr += OCITYATTR;
		else if(designation==DCAPITOL) Attr += OCITYATTR;

		if(sptr->vegetation==WOOD) Attr += OWOODATTR;
		else if(sptr->vegetation==FOREST) Attr += OFOREATTR;

		if(sptr->altitude==MOUNTAIN) Attr += OMNTNATTR;
		else if(sptr->altitude==HILL) Attr += OHILLATTR;
		else if(sptr->altitude==CLEAR) Attr += OCLERATTR;
		else Attr=0;
		break;
	default:
		break;
	}
	if((designation==DDEVASTATED)||(Attr<0)||(movecost[x][y]<0)) Attr=0;
	return(Attr);
}
/*
 * armymove - AI system for army movement and sector capture
 *
 * Complex AI function that handles automated army movement for NPC nations.
 * Implements sophisticated movement algorithms based on attractiveness,
 * leader behavior, group formation, and strategic sector capture.
 * This is a core component of the NPC AI system that drives computer-controlled
 * expansion and military activity across the game world.
 *
 * Algorithm:
 * 1. Validate army status and movement capability
 * 2. Handle special leader behavior (kings stay in capitol)
 * 3. Calculate movement targets based on army type:
 *    - Leaders without groups: find soldiers to command
 *    - Leaders with groups: coordinate group movement
 *    - Regular units: move based on sector attractiveness
 * 4. Execute probabilistic movement using weighted random selection
 * 5. Handle sector capture and ownership changes
 * 6. Update group member positions for coordinated movement
 * 7. Fallback movement algorithms for difficult terrain
 *
 * Movement Logic:
 * - Strong armies (>TAKESECTOR) have smaller search radius (2) but can go anywhere
 * - Weak armies have larger search radius (4) but limited to cities
 * - Leaders without groups seek out unattached soldiers to command
 * - Movement costs and pathfinding determine accessibility
 * - Attractiveness values drive target selection probability
 *
 * Parameters:
 *   armynum - Army identifier/index for the moving unit
 *
 * Returns:
 *   Integer count of sectors captured during this movement (0 or positive)
 *   0 if no movement occurred or no captures made
 *   Positive value indicates successful expansion
 *
 * Side Effects:
 *   - Modifies army position (P_AXLOC, P_AYLOC)
 *   - Changes army status (P_ASTAT) for leadership changes
 *   - Updates sector ownership (sct[x][y].owner)
 *   - Modifies attractiveness values (attr[x][y]) after movement
 *   - Increases nation popularity for new conquests
 *   - Updates positions of grouped units under leader command
 *
 * Testing Notes:
 *   Category: B (Integration Required)
 *   Approach: Integration tests with full world state and pathfinding
 *   Key Tests: Leader behavior, group coordination, sector capture,
 *              pathfinding integration, attractiveness-based movement,
 *              fallback algorithms, army strength thresholds
 *   Dependencies: Global army arrays, sector data, pathfinding, attractiveness
 *   Mock Requirements: World state, nation data, movement costs, pathfinding
 *   Complexity: Complex - multiple AI behaviors and world state interactions
 *
 * Notes:
 *   - XENIX platform has special handling for integer arithmetic
 *   - Kings always return to capitol when set to RULE status
 *   - Two-pass movement algorithm handles difficult terrain situations
 *   - Attractiveness is reduced after army visits to prevent clustering
 *   - Group leaders coordinate movement of all assigned units
 *   - Complex conditional logic for different army types and situations
 */
int
armymove (int armynum)
{
	long		sum, where;
#ifdef XENIX
	register int z;
#endif /*XENIX*/
	register int	x, y;
	int	i;
	long	menok;			/* enough men in the army? */
	int	leadflag=FALSE;		/* leader w/o group */
	int	takesctr=FALSE; 	/* takesctr is # unowned sctrs*/

	if(P_ASTAT>=NUMSTATUS || P_AMOVE==0) return(takesctr);

	/* if leader w/o a group, set leadflag */
	if((P_ATYPE>=MINLEADER)&&(P_ATYPE<MINMONSTER)&&(P_ASTAT!=GENERAL)) {
		leadflag=TRUE;
		/* the king stays in capitol on RULE */
		if(P_ATYPE == getleader(curntn->class)-1 ){
			P_AXLOC=curntn->capx;
			P_AYLOC=curntn->capy;
			P_ASTAT=RULE;
			return(takesctr);
		}
	}

	sum=0;
	if(leadflag) {		/* Move based on unattached soldiers */
		for(i=0;i<MAXARM;i++)
		if(( curntn->arm[i].unittyp<MINLEADER )
		&&( curntn->arm[i].stat!=MILITIA )
		&&( curntn->arm[i].stat!=ONBOARD )
		&&( curntn->arm[i].stat!=GARRISON )
		&&( curntn->arm[i].stat!=TRADED )
		&&( curntn->arm[i].stat<NUMSTATUS ))
			sum += curntn->arm[i].sold;
	} else	{		/* not leader w/o group */
		/* use menok as a temp vbl now == men in army */
		menok=0;
		if((P_ATYPE>=MINLEADER)
		&&(P_ATYPE<MINMONSTER)
		&&(P_ASTAT==GENERAL)) {
			for(x=0;x<MAXARM;x++)
				if((curntn->arm[x].stat==(NUMSTATUS+armynum))
				&& (curntn->arm[x].unittyp<MINLEADER))
					menok+=P_ASOLD;
		} else	menok=P_ASOLD;
		if((menok > TAKESECTOR )
		||( P_ATYPE>=MINLEADER)) menok=TRUE;
		else menok=FALSE;
		/* range of 4 if menok is FALSE else 2 */
		for(x=(int)P_AXLOC-4+menok*2;x<=(int)P_AXLOC+4-menok*2;x++)
		for(y=(int)P_AYLOC-4+menok*2;y<=(int)P_AYLOC+4-menok*2;y++) {
			if(ONMAP(x,y)) {
				if( menok==TRUE || ISCITY(sct[x][y].designation) )
					sum+=attr[x][y];
			}
		}
	}

	if(sum==0) {		/* nowhere to go */
		P_AXLOC=curntn->capx;
		P_AYLOC=curntn->capy;
		P_ASTAT=DEFEND;
	} else if(leadflag) {	/* find leader a group! */
		where=rand()%sum;
		for(x=0;x<MAXARM;x++)
		if((curntn->arm[x].unittyp<MINLEADER )
		&&( curntn->arm[x].stat!=MILITIA )
		&&( curntn->arm[x].stat!=GARRISON )
		&&( curntn->arm[x].stat!=ONBOARD )
		&&( curntn->arm[x].stat!=TRADED )
		&&( curntn->arm[x].stat<NUMSTATUS )){
			if ((where-=curntn->arm[x].sold) <= 0) {
				P_AXLOC=curntn->arm[x].xloc;
				P_AYLOC=curntn->arm[x].yloc;
				break;
			}
		}
		if(x!=MAXARM) for(x=0;x<MAXARM;x++) {
			if((curntn->arm[x].unittyp<MINLEADER )
			&&( curntn->arm[x].stat<NUMSTATUS )
			&&( curntn->arm[x].sold>=0 )
			&&( curntn->arm[x].stat!=MILITIA )
			&&( curntn->arm[x].stat!=GARRISON )
			&&( curntn->arm[x].stat!=SIEGED )
			&&( curntn->arm[x].stat!=SCOUT )
			&&( curntn->arm[x].stat!=ONBOARD )
			&&( curntn->arm[x].stat!=TRADED )
			&&( curntn->arm[x].unittyp!=A_ZOMBIE )
			&&( P_AXLOC==curntn->arm[x].xloc )
			&&( P_AYLOC==curntn->arm[x].yloc )){
				curntn->arm[x].stat=NUMSTATUS+armynum;
				P_ASTAT=GENERAL;
				break;
			}
		}
	} else {	/* move a normal unit */
		where=rand()%sum;
		/* range of 4 if menok is FALSE else 2 */
		for(x=(int)P_AXLOC-4+menok*2;x<=(int)P_AXLOC+4-menok*2;x++)
		for(y=(int)P_AYLOC-4+menok*2;y<=(int)P_AYLOC+4-menok*2;y++)
		if(ONMAP(x,y)){
			if( menok==TRUE || ISCITY(sct[x][y].designation) )
				where -= attr[x][y];
			if( (where < 0 )
			&& movecost[x][y]>=1
			&& movecost[x][y]<=P_AMOVE
			&&(land_reachp((int)P_AXLOC,(int)P_AYLOC,x,y,P_AMOVE,country))){
				P_AXLOC=x;
				P_AYLOC=y;
				if(P_ATYPE == getleader(curntn->class)-1 ){
					P_AXLOC=curntn->capx;
					P_AYLOC=curntn->capy;
				}

				/* ARMIES MOVE PSEUDO INDEPENDANTLY */
				if((sct[x][y].designation != DCITY)
				&&(sct[x][y].designation != DCAPITOL)
				&&(sct[x][y].designation != DTOWN)
				&&(sct[x][y].owner==country)) {
#ifdef XENIX
					z = attr[x][y];
					z /= 8;
					attr[x][y] = z;
#else
					attr[x][y] /= 8;
#endif /*XENIX*/
				}
				if(sct[x][y].owner==0){
					sct[x][y].owner=country;
					if (curntn->popularity<MAXTGVAL) curntn->popularity++;
#ifdef XENIX
					z = attr[x][y];
					z /= 8;
					attr[x][y] = z;
#else
					attr[x][y]/=8;
#endif /*XENIX*/
					takesctr++;
				}

				if((P_ATYPE>=MINLEADER)&&(P_ASTAT==GENERAL))
				for(x=0;x<MAXARM;x++)
				if((curntn->arm[x].sold>0 )
				&&( curntn->arm[x].stat==armynum+NUMSTATUS)){
					curntn->arm[x].xloc=P_AXLOC;
					curntn->arm[x].yloc=P_AYLOC;
				}
				return(takesctr);
			} /* if */
		} /* for for */

		/*do again - have this block if lots of bad terrain*/
		/*what could happen is that it won't find a move first time*/
		for(x=(int)P_AXLOC-2;x<=(int)P_AXLOC+2;x++)
		for(y=(int)P_AYLOC-2;y<=(int)P_AYLOC+2;y++) {
			if(!ONMAP(x,y))
				continue;

			if(leadflag) where -= solds_in_sector(x,y,country);
			else where -= attr[x][y];
			if( (where < 0 )
			&& movecost[x][y]>=1
			&& movecost[x][y]<=P_AMOVE
			&&(land_reachp(P_AXLOC,P_AYLOC,x,y,P_AMOVE,country))){
				P_AXLOC=x;
				P_AYLOC=y;
				if(sct[x][y].owner==0){
					if (curntn->popularity<MAXTGVAL) curntn->popularity++;
					sct[x][y].owner=country;
					attr[x][y] = 1;
					takesctr++;
				}
				if((P_ATYPE>=MINLEADER)&&(P_ASTAT==GENERAL))
				for(i=0;i<MAXARM;i++)
				if((curntn->arm[i].sold>0 )
				&&( curntn->arm[i].stat==armynum+NUMSTATUS)){
					curntn->arm[i].xloc=P_AXLOC;
					curntn->arm[i].yloc=P_AYLOC;
				}
				return(takesctr);
			} /* if */
		} /* for for */
	} /* if */
	return(takesctr);
}

/*
 * score - Update scoring for all active nations
 *
 * Iterates through all nations and updates their cumulative scores
 * using the score_one() function. This is called once per turn
 * during the main update cycle to track nation performance and
 * ranking over time. Provides the scoring system that measures
 * player and NPC progress throughout the game.
 *
 * Algorithm:
 * 1. Print status message for score updating
 * 2. Iterate through all possible nations (1 to NTOTAL-1)
 * 3. Check if each nation is active using isntn()
 * 4. Add current turn score to cumulative nation score
 * 5. Continue until all active nations processed
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   void - No return value
 *
 * Side Effects:
 *   - Modifies ntn[x].score for all active nations
 *   - Prints status message to stdout
 *   - Calls score_one() which may perform additional scoring calculations
 *
 * Testing Notes:
 *   Category: A (Unit Testable)
 *   Approach: Unit tests with mock nation data and score_one() function
 *   Key Tests: Active nation detection, score accumulation, boundary conditions,
 *              score_one() integration, NTOTAL boundary handling
 *   Dependencies: Global ntn[] array, score_one() function, isntn() macro
 *   Mock Requirements: Nation status data, score calculation functions
 *   Complexity: Simple - straightforward iteration with function calls
 *
 * Notes:
 *   - Called once per turn during main update cycle
 *   - Depends on score_one() for actual scoring algorithm
 *   - Only processes active nations to avoid updating dead/inactive nations
 *   - Nation 0 is skipped (nations start at index 1)
 *   - Cumulative scoring allows tracking long-term nation performance
 */
void
score (void)
{
	int x;
	printf("\nUpdating nation's scores\n");
	for(x=1;x<NTOTAL;x++) if(isntn(ntn[x].active))
		ntn[x].score += score_one(x);
}

#ifdef CHEAT
/*
 * cheat - NPC difficulty balancing and fairness adjustments (conditional compilation)
 *
 * Optional balancing system that provides minor assistance to NPC nations
 * when they fall behind human players. This function is only compiled
 * when CHEAT is defined and represents the only "cheating" assistance
 * the game provides to computer-controlled opponents.
 *
 * Algorithm:
 * 1. Identify active NPC nations vs human players
 * 2. Calculate average combat bonuses for human vs NPC nations
 * 3. Provide economic assistance (gold) to struggling NPCs
 * 4. Balance combat effectiveness based on player averages
 * 5. Improve diplomatic relations between same-race NPCs
 * 6. Slightly worsen relations between different races
 *
 * Balancing Mechanisms:
 * - Gold injection for NPCs with low gold/civilian ratios
 * - Combat skill adjustments based on human player averages
 * - Diplomatic bias toward same-race cooperation
 * - Score-based assistance targeting (only help underperforming NPCs)
 * - Race-based exclusions (orcs don't get combat bonuses)
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   void - No return value
 *
 * Side Effects:
 *   - May increase NPC nation gold (ntn[x].tgold)
 *   - May increase combat bonuses (ntn[x].aplus, ntn[x].dplus)
 *   - Modifies diplomatic relations (ntn[x].dstatus[y])
 *   - Prints assistance messages to stdout for transparency
 *
 * Testing Notes:
 *   Category: D (Mock Intensive)
 *   Approach: Unit tests with extensive mocking of nation data and file access
 *   Key Tests: NPC identification, average calculations, threshold behaviors,
 *              race-based logic, diplomatic modifications, boundary conditions
 *   Dependencies: Global ntn[] array, file system access, exefile paths
 *   Mock Requirements: Nation data, file access, randomization functions
 *   Complexity: Moderate - multiple calculation phases with conditional logic
 *
 * Notes:
 *   - Only compiled when CHEAT preprocessor flag is defined
 *   - Designed as minimal assistance to maintain game balance
 *   - Transparency through console output of all assistance provided
 *   - Author's pride comment indicates philosophy of minimal AI assistance
 *   - File access checks determine which nations are actively played
 *   - Score comparison ensures only struggling NPCs receive help
 *   - Diplomatic changes are probabilistic and gradual
 */
void
cheat (void)
{
	int x,y;
	int bonus=0, count=0, npcavg, pcavg, avgscore=0;
	char realnpc[NTOTAL],tempc[LINELTH];

	/* take inventory of countries */
	for(x=1;x<NTOTAL;x++) {
		sprintf(tempc,"%s%d", exefile, x);
		if (isnpc(ntn[x].active) && access(tempc,00)==0) {
			realnpc[x]=TRUE;
		} else {
			realnpc[x]=FALSE;
		}
	}

	/* add gold */
	for(x=1;x<NTOTAL;x++) if(realnpc[x]==TRUE) {
		if((ntn[x].tgold<ntn[x].tciv)
		&&( rand()%5==0)){
			ntn[x].tgold+=10000;
			printf("npc cheat routine - add $10000 to nation %s\n",ntn[x].name);
		}
	}

	for(x=1;x<NTOTAL;x++)
		if(realnpc[x]==FALSE)  {
			bonus+=ntn[x].aplus+ntn[x].dplus;
			avgscore+=ntn[x].score;
			count++;
		}

	if(count==0) return;
	pcavg = bonus / count;
	avgscore /= count;
	printf("pc average score is %d count is %d\n",avgscore,count);

	bonus=0;
	count=0;
	for(x=1;x<NTOTAL;x++)
		if(realnpc[x]==TRUE)  {
			bonus+=ntn[x].aplus+ntn[x].dplus;
			count++;
		}
	if(count==0) return;
	npcavg = bonus / count;
	for(x=1;x<NTOTAL;x++)
	if((realnpc[x]==TRUE)
	&&(ntn[x].score < avgscore)
	&&(ntn[x].race != ORC )
	&&(rand()%100 < (pcavg-npcavg))) {
		if(ntn[x].aplus>ntn[x].dplus) ntn[x].dplus+=1;
		else ntn[x].aplus+=1;
		printf("npc cheat routine - add 1%% to nation %s combat skill\n",ntn[x].name);
	}

	/* cheat by making npc's frendlier to each other if they are */
	/* of the same race */
	for(x=1;x<NTOTAL;x++) if(realnpc[x]==TRUE)
		for(y=1;y<NTOTAL;y++) if(realnpc[x]==TRUE)
			if((ntn[x].dstatus[y]!=TREATY)
			&&(ntn[x].dstatus[y]!=UNMET)){
				if(ntn[x].race == ntn[y].race){
					ntn[x].dstatus[y]--;
				} else {
					if(ntn[x].dstatus[y]!=JIHAD)
						if(rand()%4==0)
							ntn[x].dstatus[y]--;
				}
			}
}
#endif /* CHEAT */


/*
 * updexecs - Execute all nations in randomized order with comprehensive nation processing
 *
 * Central nation execution system that processes all active nations in
 * random order during each game turn. Handles player command execution,
 * NPC AI processing, leadership management, civilian movement, and
 * magical system updates. This is the heart of the turn-based
 * nation management system.
 *
 * Algorithm:
 * 1. Allocate attractiveness and population movement matrices
 * 2. Create randomized execution order for all active nations
 * 3. For each nation in random order:
 *    a. Execute player commands or run NPC AI
 *    b. Handle computer assistance for inactive players (CMOVE)
 *    c. Process NPC magic and takeover attempts (ORCTAKE)
 *    d. Check and manage national leadership status
 *    e. Handle leadership transitions and disarray states
 *    f. Calculate sector attractiveness for civilian movement
 *    g. Execute civilian population movement
 * 4. Reset nation statistics and spell point calculations
 * 5. Process magical regeneration and bonuses
 * 6. Cleanup allocated memory
 *
 * Nation Processing Features:
 * - Random execution order prevents turn order advantages
 * - Leadership crisis management with automatic succession
 * - Computer assistance for inactive human players
 * - NPC magical takeover attempts based on monster availability
 * - Civilian movement based on sector attractiveness
 * - Capital proximity bonuses for population movement
 * - Spell point regeneration and magical bonuses
 *
 * Parameters:
 *   None - processes all nations globally
 *
 * Returns:
 *   void - No return value
 *
 * Side Effects:
 *   - Allocates and frees attr[][] and newpop[][] matrices
 *   - Executes player commands and NPC AI systems
 *   - Modifies nation leadership and army positions
 *   - Updates sector attractiveness and civilian populations
 *   - Resets nation statistics (ships, military totals)
 *   - Modifies spell points based on magical abilities
 *   - Sends mail messages to players about leadership changes
 *   - Writes news entries for significant events
 *
 * Testing Notes:
 *   Category: C (System Level Only)
 *   Approach: System testing with full game state and all subsystems
 *   Key Tests: Random nation ordering, leadership management, NPC execution,
 *              memory allocation/cleanup, civilian movement, spell calculations
 *   Dependencies: Complete game state, file systems, mail/news systems,
 *                execute(), nationrun(), move_people(), magic() systems
 *   Mock Requirements: Full world simulation with all game subsystems
 *   Complexity: Complex - orchestrates multiple major game systems
 *
 * Notes:
 *   - Uses multiple conditional compilation blocks (TRADE, NPC, CMOVE, ORCTAKE)
 *   - Random nation execution prevents predictable advantages
 *   - Memory management critical - allocates large matrices
 *   - Leadership system prevents nations from becoming unplayable
 *   - XENIX platform requires special integer arithmetic handling
 *   - Nation disarray occurs when primary leader is killed
 *   - Civilian movement driven by sector attractiveness calculations
 */
void
updexecs (void)
{
	register struct s_sector	*sptr;
	register int x, y;
#ifdef XENIX
	register int z;
#endif /*XENIX*/
	int	armynum;
	int done, loop=0, number=0;
	void move_people();
	int execed[NTOTAL];

	check();
	attr = (int **) m2alloc(MAPX,MAPY,sizeof(int));
	check();
	newpop = (long **) m2alloc(5,MAPY,sizeof(long));
	check();

	for(country=0;country<NTOTAL;country++)
		if( isntn(ntn[country].active) ) execed[country]=FALSE;
		else {
			execed[country]=TRUE;
			loop++;
		}

	for(;loop<NTOTAL;loop++){
		number=(rand()%(NTOTAL-loop))+1; /*get random active nation*/

		done=FALSE;
		/*Find the appropiate nation*/
		for(country=0;done==FALSE && country<NTOTAL;country++) {
			if (execed[country]==FALSE) number--;
			if (number==0) {
				execed[country]=TRUE;
				done=TRUE;
				country--;	/* compensate for inc */
			}
		}

		curntn = &ntn[country];
		if(curntn->active == INACTIVE) continue;

		printf("updating nation number %d -> %s\n",country,curntn->name);
	check();

		disarray=FALSE;
#ifdef TRADE
		if(isntn(curntn->active)) checktrade();
#endif /* TRADE */

		/*if execute is 0 and PC nation then they did not move*/
		if((execute(TRUE)==0)&&(ispc(curntn->active))){
			printf("\tnation %s did not move\n",curntn->name);
#ifdef NPC
#ifdef CMOVE
			printf("\tthe computer will move for %s\n",curntn->name);
			fprintf(fnews,"1.\tthe computer will move for %s\n",curntn->name);
			if (mailopen( country )!=(-1)) {
				fprintf(fm,"Message to %s from CONQUER\n\n",curntn->name);
				fprintf(fm,"The computer moved for you in the %s of Year %d\n",PSEASON(TURN),YEAR(TURN));
				mailclose(country);
			}
			check();
			nationrun();
			check();
#endif /*CMOVE*/
		}
		/* run npc nations */
		if(isnpc(curntn->active)) {
			check();
			nationrun();
			check();
#ifdef ORCTAKE
			/*do npc nation magic*/
			if(magic(country,MA_MONST)==TRUE) {
				if((x=takeover(5,0))==1)
				printf("SUCCESSFUL TAKEOVER OF %d by %s",x,curntn->name);
			} else if(magic(country,AV_MONST)==TRUE) {
				if((x=takeover(3,0))==1)
				printf("SUCCESSFUL TAKEOVER OF %d by %s",x,curntn->name);
			} else if(magic(country,MI_MONST)==TRUE){
				if((x=takeover(1,0))==1)
				printf("SUCCESSFUL TAKEOVER OF %d by %s",x,curntn->name);
			}
#endif /* ORCTAKE */
#endif /*NPC*/
		}

		/* is leader killed - put nation into disarray */
		disarray=TRUE;
		x = getleader((int)curntn->class) - 1;
		for(armynum=0;armynum<MAXARM;armynum++)
		if(P_ATYPE == x && P_ASOLD>0) {
			disarray=FALSE;
			break;
		}
#ifdef DEBUG
printf("checking for leader in nation %s: armynum=%d\n",curntn->name,armynum);
#endif /* DEBUG */

		if(disarray == TRUE) {
			if(rand()%100 < 30) {	/* new leader takes over */
				x++;
				for(armynum=0;armynum<MAXARM;armynum++)
					if(P_ATYPE == x && P_ASOLD > 0) break;
				if( armynum<MAXARM) {
					P_ATYPE=x-1;
					P_ASOLD= *(unitminsth+(x-1)%UTYPE);
					disarray=FALSE;
					fprintf(stderr,"new leader in nation %s\n",curntn->name);
					fprintf(fnews,"1.\tnation %s has a new leader\n",curntn->name);
					if(ispc(curntn->active)){
						if (mailopen(country)!=(-1)) {
							fprintf(fm,"MESSAGE FROM CONQUER:\n\n");
							fprintf(fm,"YOU HAVE A NEW NATIONAL LEADER.\n");
							fprintf(fm,"YOUR TROOPS MAY NOW MOVE NORMALLY.\n");
							mailclose(country);
						}
					}
				}
			}
		}

		if( disarray ==  TRUE) {
			fprintf(stderr,"no leader in nation %s\n",curntn->name);
			fprintf(fnews,"1.\tnation %s still has no national leader\n",curntn->name);
			if(ispc(curntn->active)){
				if (mailopen(country)!=(-1)) {
					fprintf(fm,"MESSAGE FROM CONQUER\n\n");
					fprintf(fm,"YOU DON'T HAVE A COUNTRY LEADER;\n");
					fprintf(fm,"YOUR TROOPS MAY NOT MOVE\n");
					fprintf(fm,"THERE IS A 30%% CHANCE/TURN OF GETTING A NEW ONE\n");
					mailclose(country);
				}
			}
		}

		updmove(curntn->race,country);	/*update movement array*/

		/* Recalculate ATTR MATRIX for civilians */
		/*calculate sector attractiveness*/
		for(x=0;x<MAPX;x++) for(y=0;y<MAPY;y++) {
			sptr = &sct[x][y];
			if((sptr->owner==country)
			&&(tofood(sptr,sptr->owner)!=0)){
				attr[x][y]=attract(x,y,curntn->race);
			} else attr[x][y]=0;
		}

		/*if near capitol add to attr*/
		for(x=(int)curntn->capx-2;x<=(int)curntn->capx+2;x++)
			for(y=(int)curntn->capy-2;y<=(int)curntn->capy+2;y++)
				if((ONMAP(x,y))&&(attr[x][y]>0)) attr[x][y]+=20;
		move_people();

	} /* for */

	/*zero out all recalculated values; do not clear god */
	for(country=1;country<NTOTAL;country++) if(isntn(ntn[country].active)){
		ntn[country].tships=0;
		ntn[country].tmil=0;
#ifdef XENIX
		if (rand()%4 == 0) {
			z = ntn[country].spellpts;
			z /= 2;
			ntn[country].spellpts = z;
		}
#else
		if(rand()%4==0) ntn[country].spellpts/=2;
#endif /*XENIX*/
		if(magic(country,SUMMON)==TRUE) {
			ntn[country].spellpts+=4;
			if(magic(country,WYZARD)==TRUE)
				ntn[country].spellpts+=3;
			if(magic(country,SORCERER)==TRUE)
				ntn[country].spellpts+=3;
		}
		if(magic(country,MA_MONST)==TRUE) ntn[country].spellpts+=2;
		if(magic(country,AV_MONST)==TRUE) ntn[country].spellpts+=1;
		if((magic(country,MI_MONST)==TRUE)
			&&( rand()%2==0)) ntn[country].spellpts+=1;
	}
	free(attr);
	free(newpop);
}

/*
 * do_lizard - Special update routine for lizard nation AI behavior
 *
 * Implements specialized AI behavior for the lizard race, featuring
 * automatic population growth, aggressive movement patterns, and
 * strategic positioning around friendly armies. This function provides
 * unique racial characteristics that differentiate lizard nations from
 * standard NPC behavior.
 *
 * Algorithm:
 * 1. Iterate through all army slots for the current lizard nation
 * 2. For active armies, grant movement points and apply 2% population growth
 * 3. Even-numbered armies: Set to garrison status for defensive positioning
 * 4. Odd-numbered armies: Follow and support the previous (even) army
 * 5. Implement siege relief by randomly moving to adjacent enemy sectors
 * 6. Apply garrison bonuses when positioned in owned forts
 * 7. Maintain aggressive attack stance when not garrisoned
 *
 * Special Behaviors:
 * - Population Growth: 2% per turn for active lizard armies
 * - Paired Army Strategy: Even armies garrison, odd armies provide mobile support
 * - Siege Relief: Automatic attempts to relieve besieged friendly forces
 * - Fort Utilization: Intelligent use of fort defensive bonuses
 * - Aggressive Expansion: Tendency to move into enemy territory
 *
 * Race-Specific Features:
 * - Higher reproduction rate than other races
 * - Coordinated army movement patterns
 * - Aggressive territorial expansion behavior
 * - Strategic fort usage for defense
 *
 * Parameters:
 *   None - Uses global 'country' variable for current nation
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Modifies army positions, populations, and status for lizard nation
 *   - Updates movement points for all lizard armies
 *   - Changes army status (GARRISON, ATTACK, SIEGED)
 *   - May relocate armies for strategic positioning
 *   - Uses global variables: country, curntn, ntn array, sct array
 *
 * Testing Notes:
 *   Category: B (Integration Required)
 *   Approach: Integration testing with full world state
 *   Key Tests: Population growth calculation, army pairing behavior, siege relief
 *   Dependencies: Global nation/army data, sector map, random number generator
 *   Mock Requirements: Nation setup, army configuration, sector ownership
 *   Complexity: Moderate - race-specific AI with multiple behavior patterns
 *
 * Platform Notes:
 *   - Uses XENIX-specific integer arithmetic to prevent overflow
 *   - Includes debug output for army position validation
 *   - Handles edge cases for army positioning and water detection
 *
 * Notes:
 *   - Only called for lizard race nations during update cycle
 *   - Implements unique racial characteristics not found in other NPC types
 *   - Population growth gives lizards significant long-term advantage
 *   - Army coordination provides tactical military benefits
 *   - Thread safety: Not thread-safe due to global variable dependencies
 */

/****************************************************************/
/*	DO_LIZARD() 						*/
/* update lizards	 					*/
/****************************************************************/
void
do_lizard (void)
{
#ifdef XENIX
	register int x;
#endif /*XENIX*/
	register int i, j;
	int armynum;

	printf("updating lizard (nation %d)\n",country);
	curntn = &ntn[country];
	for(armynum=0;armynum<MAXARM;armynum++)
	if((P_ASOLD>0)) {
		P_AMOVE =20;	/* just in case god wants to move them */
		/* increase population */
#ifdef XENIX
		x = P_ASOLD * 102;
		x /= 100;
		P_ASOLD = x;
#else
		P_ASOLD*=102;
		P_ASOLD/=100;
#endif /*XENIX*/
		if(armynum%2==0) {
			if(P_ASTAT!=SIEGED) P_ASTAT=GARRISON;
		} else {
			if(ntn[country].arm[armynum-1].sold<=0) {
				P_ASOLD=0;
				continue;
			}
			P_AXLOC = ntn[country].arm[armynum-1].xloc;
			P_AYLOC = ntn[country].arm[armynum-1].yloc;
			/* try to relieve sieges */
			if(P_ASTAT!=SIEGED
			&& ntn[country].arm[armynum-1].stat!=SIEGED) {
			for(i=(int)ntn[country].arm[armynum-1].xloc-1;i<=ntn[country].arm[armynum-1].xloc+1;i++) {
				for(j=(int)ntn[country].arm[armynum-1].yloc-1;j<=ntn[country].arm[armynum-1].yloc+1;j++) {
					if(ONMAP(i,j)
					&&(sct[i][j].altitude!=WATER)
					&&(sct[i][j].altitude!=PEAK)
					&&(sct[i][j].owner != country)
					&&(rand()%3==0)){
						P_AXLOC = i;
						P_AYLOC = j;
					}
				}
			}
			}
			/* this cheats by giving garrison bonus with movement */
			if((sct[P_AXLOC][P_AYLOC].designation==DFORT)
			&&(sct[P_AXLOC][P_AYLOC].owner==country)) {
				if(P_ASTAT!=SIEGED) P_ASTAT=GARRISON;
			} else P_ASTAT=ATTACK;
		}
	}
#ifdef DEBUG
	for(armynum=0;armynum<MAXARM;armynum++) {
		if((P_ASOLD>0)&&(sct[P_AXLOC][P_AYLOC].altitude==WATER))
			printf("ERROR line %d... %s army %d in water (army %d: x: %d y: %d)\n",__LINE__,ntn[country].name,armynum,armynum-1, ntn[country].arm[armynum-1].xloc, ntn[country].arm[armynum-1].yloc);
	}
#endif /* DEBUG */
}

/*
 * updcapture - Process sector capture and scout detection for all nations
 *
 * Handles the critical end-of-turn logic for territorial expansion through
 * sector capture and the dangerous game of scouting enemy territory. This
 * function processes two main systems: army-based sector capture and
 * scout detection/capture mechanics.
 *
 * Algorithm:
 * 1. Initialize occupation matrix using prep(0,-1) to track army positions
 * 2. For each active nation and army:
 *    a. Check army size requirements (players: TAKESECTOR, NPCs: >75)
 *    b. Validate capture conditions (not on fleet, not in water)
 *    c. Process sector ownership changes based on war status
 *    d. Handle population displacement with slavery/massacre mechanics
 * 3. Process scout capture mechanics:
 *    a. Calculate detection probability based on diplomatic status
 *    b. Handle scout capture with appropriate notifications
 * 4. Process capital capture and nation destruction via sackem()
 *
 * Sector Capture Rules:
 * - Players require TAKESECTOR soldiers minimum for capture
 * - NPCs require >75 soldiers (balancing for AI army generation)
 * - Armies on ships cannot capture territory
 * - Water sectors cannot be captured
 * - Only sectors occupied solely by attacking nation can be captured
 * - War status required for hostile takeovers
 *
 * Scout Detection System:
 * - Scouts detected with PFINDSCOUT% chance in hostile territory
 * - Reduced detection (PFINDSCOUT/5%) in neutral owned territory
 * - Detection varies by diplomatic status (hostile vs neutral)
 * - Captured scouts are eliminated from the game
 *
 * Population Displacement:
 * - Different race captures trigger flee() with potential slavery
 * - SLAVER magic enhances population capture efficiency
 * - Same race captures are less devastating to civilians
 *
 * Parameters:
 *   None - Uses global variables for world state
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Modifies sector ownership across the entire map
 *   - Updates nation popularity scores for territorial gains
 *   - Removes captured scouts from armies
 *   - Triggers capital destruction via sackem() for lost capitals
 *   - Sends mail notifications to affected players
 *   - Writes news reports for sector captures
 *   - Uses global variables: country, curntn, sct array, ntn array, occ array
 *
 * Testing Notes:
 *   Category: B (Integration Required)
 *   Approach: Integration testing with full world state and army setup
 *   Key Tests: Sector capture rules, scout detection rates, capital loss
 *   Dependencies: Army positions, diplomatic status, sector ownership, mail system
 *   Mock Requirements: Multiple nations, armies, diplomatic relationships
 *   Complexity: Complex - multiple interacting systems and edge cases
 *
 * Balance Considerations:
 * - NPC army size advantage compensates for AI limitations
 * - Scout detection prevents excessive intelligence gathering
 * - Popularity system rewards territorial expansion
 * - Capital vulnerability creates strategic focal points
 *
 * Historical Context:
 * - HIDELOC compilation option hides coordinates in news reports
 * - Different detection rates balance scouting risk vs reward
 * - Population displacement reflects medieval conquest mechanics
 *
 * Notes:
 *   - Critical for territorial control and expansion mechanics
 *   - Balances aggressive expansion with scouting risks
 *   - Capital capture triggers catastrophic nation destruction
 *   - Scout mechanics encourage careful intelligence operations
 *   - Thread safety: Not thread-safe due to global variable dependencies
 */

/****************************************************************/
/*	UPDCAPTURE() 						*/
/* capture unoccupied sectors					*/
/****************************************************************/
void
updcapture (void)
{
	register struct s_sector	*sptr;
	int armynum, occval;

	fprintf(fnews,"3\tNEWS ON WHAT SECTORS HAVE BEEN CAPTURED\n");
	printf("distributing captured sectors\n");

	/*look for any areas where armies alone in sector*/
	prep(0,-1);

	for(country=1;country<NTOTAL;country++)
	if(ntn[country].active!=INACTIVE){
		curntn = &ntn[country];
		for(armynum=0;armynum<MAXARM;armynum++)
/* cheat in favor of npcs as the create army routines assume 75 man armies */
		if(P_ATYPE<MINLEADER) {
			if((ispc(curntn->active)&&(P_ASOLD>=TAKESECTOR))
			||((isnotpc(curntn->active))&&(P_ASOLD>75))){
				/* may not capture land while on a fleet */
				if(P_ASTAT==ONBOARD) continue;
				/* may not capture water */
				if(sct[P_AXLOC][P_AYLOC].altitude==WATER) {
					fprintf(stderr,"Nation %s Army %d in Water\n",curntn->name,armynum);
					continue;
				}
				if(occ[P_AXLOC][P_AYLOC] != country) continue;
				sptr = &sct[P_AXLOC][P_AYLOC];
				if(sptr->owner==0){
					sptr->owner=country;
					if (curntn->popularity<MAXTGVAL) curntn->popularity++;
				} else if((sptr->owner!=country)
				&&(curntn->dstatus[sptr->owner]>=WAR)) {
					if(ntn[sptr->owner].race!=curntn->race) {
						if(magic(country,SLAVER)==TRUE){
							flee(P_AXLOC,P_AYLOC,1,TRUE);
						} else {
							flee(P_AXLOC,P_AYLOC,1,FALSE);
						}
					}

					if((isntn( curntn->active ))
					   &&(isntn( ntn[sptr->owner].active))) {
#ifdef HIDELOC
						fprintf(fnews,"3.\tarea captured by %s from %s\n",curntn->name,ntn[sptr->owner].name);
#else
						fprintf(fnews,"3.\tarea %d,%d captured by %s from %s\n",P_AXLOC,P_AYLOC,curntn->name,ntn[sptr->owner].name);
#endif /* HIDELOC */
					}
					sptr->owner=country;
					curntn->popularity++;
				}
			}
		} else if(P_ASTAT==A_SCOUT && P_ATYPE!=A_SPY && P_ASOLD>0) {
			occval=occ[P_AXLOC][P_AYLOC];
			/* capture situations:
			 *   - alone with a hostile army     [PFINDSCOUT% chance]
			 *   - alone in someone else's territory with one of
			 *      their armies (non-allied)    [(PFINDSCOUT/5)% chance]
			 * NOTE: do not remove chance to capture in unmet territory.
			 */
			if (occval!=0 && occval!=country && occval<NTOTAL) {
				if(((ntn[occval].dstatus[country]>=HOSTILE)
				  &&(rand()%100<PFINDSCOUT))
				||((sct[P_AXLOC][P_AYLOC].owner==occval)
				  &&(ntn[occval].dstatus[country]!=TREATY)
				  &&(ntn[occval].dstatus[country]!=ALLIED)
				  &&(rand()%100<PFINDSCOUT/5))) {
					/* capture the scout */
					P_ASOLD=0;
					if (ispc(curntn->active)) {
						if(mailopen(country)!=(-1)) {
							fprintf(fm,"Message from Conquer\n\n");
							fprintf(fm,"\tYour Scouting Unit %d was captured\n",armynum);
							fprintf(fm,"\t  by %s military in sector %d,%d\n",
								   ntn[occval].name,(int)P_AXLOC,(int)P_AYLOC);
							mailclose(country);
						}
					}
					if (ispc(ntn[occval].active)) {
						if(mailopen(occval)!=(-1)) {
							fprintf(fm,"Message from Conquer\n\n");
							fprintf(fm,"\tA Scout from nation %s was captured\n",curntn->name);
							fprintf(fm,"\t  in sector %d,%d.\n",(int)P_AXLOC,(int)P_AYLOC);
							mailclose(occval);
						}
					}
				}
			}
		}
	}

	/* capture countries */
	for(country=1;country<NTOTAL;country++)
	if (isntn(ntn[country].active)) {

		/* check for capitols being sacked */
		if(sct[ntn[country].capx][ntn[country].capy].owner != country)
			sackem(country);
	}
}

/*
 * updsectors - Comprehensive sector-by-sector world update and economic simulation
 *
 * Handles the complex economic engine that drives population growth, resource
 * depletion, trade good discovery, diplomatic contact, and national economics.
 * This function processes every owned sector on the map and calculates national
 * statistics including wealth distribution, poverty levels, and inflation rates.
 *
 * Algorithm:
 * Phase 1 - Sector Processing (for each sector):
 * 1. Random trade good discovery (FINDPERCENT chance per turn)
 * 2. Calculate seasonal reproduction rates (varies by nation and season)
 * 3. Validate capitol designations (only one per nation)
 * 4. Apply population growth based on current population and food availability
 * 5. Handle resource depletion for mines (population vs resource balance)
 * 6. Process mine exhaustion (converts to devastated sectors)
 * 7. Handle diplomatic contact between adjacent nations (MEETNTN range)
 * 8. Desert reversion for sectors with insufficient food production
 *
 * Phase 2 - National Economics (for each nation):
 * 1. Check for capitol loss and trigger nation depletion
 * 2. Calculate national totals via spreadsheet() function
 * 3. Apply charity system (redistributes wealth to reduce poverty)
 * 4. Calculate poverty index based on wealth per civilian ratio
 * 5. Process inflation mechanics (taxes, military burden, poverty factors)
 * 6. Apply inflation effects to national treasury
 * 7. Update final resource totals (metals, jewels, food)
 *
 * Population Growth Mechanics:
 * - Seasonal variation: Higher growth in spring/summer, lower in winter
 * - Size-based rates: Small populations grow faster (bootstrap effect)
 * - Urban areas: Cities/capitols/towns have reduced growth (urbanization)
 * - Food limitation: Desert conditions prevent normal growth
 * - Maximum limits: Population caps prevent infinite growth
 *
 * Economic Systems:
 * - Wealth Distribution: Charity system redistributes gold to civilians
 * - Poverty Calculation: Complex formula based on gold-per-civilian ratios
 * - Inflation Engine: Multi-factor system (taxes, military, poverty)
 * - Resource Depletion: Mining efficiency decreases with overpopulation
 * - Treasury Management: Large treasuries suffer inflation penalties
 *
 * Diplomatic Contact:
 * - Automatic meeting when nations expand into adjacent sectors
 * - Bidirectional contact establishment (both nations meet each other)
 * - Range-based detection (MEETNTN sector radius)
 * - Triggers diplomatic relationship initialization
 *
 * Resource Discovery:
 * - Random chance for new trade goods in unexploited sectors
 * - Equal probability between metals and jewels discovery
 * - Only occurs in sectors without existing trade goods
 * - Provides ongoing economic development opportunities
 *
 * Parameters:
 *   None - Uses global world state variables
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Modifies population levels across all owned sectors
 *   - Updates trade good availability and mine resources
 *   - Establishes diplomatic contact between expanding nations
 *   - Recalculates all national economic statistics
 *   - Triggers nation depletion for capitol-less countries
 *   - Uses global variables: sct array, ntn array, country, curntn, spread structure
 *
 * Testing Notes:
 *   Category: C (System Level Only)
 *   Approach: Full system testing with complete world state
 *   Key Tests: Population growth rates, economic calculations, diplomatic contact
 *   Dependencies: Complete world map, multiple nations, economic data structures
 *   Mock Requirements: Extensive - world state, multiple nations, sector ownership
 *   Complexity: Complex - multiple interacting economic and demographic systems
 *
 * Economic Balance Features:
 * - Progressive poverty calculation (wealth inequality handling)
 * - Inflation as economic regulation mechanism
 * - Charity system for wealth redistribution
 * - Military burden on civilian economy
 * - Resource scarcity driving expansion pressure
 *
 * Performance Considerations:
 * - Processes entire world map every turn (O(MAPX*MAPY))
 * - Heavy computational load for large worlds
 * - Economic calculations for all active nations
 * - Critical path function affecting game performance
 *
 * Notes:
 *   - Central economic engine for the entire game world
 *   - Balances population growth against resource constraints
 *   - Implements sophisticated economic modeling for strategy game
 *   - Creates pressure for territorial expansion and resource control
 *   - Thread safety: Not thread-safe due to extensive global variable usage
 */

/**************************************************************/
/*	UPDSECTORS() 						*/
/* update sectors one at a time				*/
/**************************************************************/
void
updsectors (void)
{
	register struct s_sector	*sptr;
	register struct s_nation		*nptr;
	long	charity;	/* talons to the poor */
	register int i, j;
	register int x,y;

	printf("\nupdating all sectors\n");
	for(x=0;x<MAPX;x++) for(y=0;y<MAPY;y++) {
		int rephold;

		sptr = &sct[x][y];
		if(sptr->owner == 0) continue;
		nptr = &ntn[sptr->owner];

		/* add to contents of sector */
		if(rand()%100<FINDPERCENT) {
			if( sct[x][y].tradegood == TG_none ) {
				if(rand()%2==0) getmetal( &sct[x][y] );
				else getjewel( &sct[x][y] );
			}
		}

		/* calculate reproduction per season */
		rephold = nptr->repro/4;
		if ((SEASON(TURN)!=WINTER) &&
		(SEASON(TURN)<=(nptr->repro%4)))
			rephold++;

		/* only one capitol per nation */
		if (sptr->designation==DCAPITOL) {
			if (nptr->capx!=x || nptr->capy!=y) {
				sptr->designation=DCITY;
			}
		}

		/* if huge number dont reproduce */
		if(sptr->people >= ABSMAXPEOPLE) {
			sptr->people = ABSMAXPEOPLE;
			if(sptr->people * sptr->metal > 2*(rand()%100)*TOMUCHMINED)
				if(sptr->designation==DMINE) sptr->metal--;
			if(sptr->people * sptr->jewels > 2*(rand()%100)*TOMUCHMINED)
				if(sptr->designation==DGOLDMINE) sptr->jewels--;
		} else if((sptr->people > TOMANYPEOPLE)
		&&(sptr->designation!=DTOWN)
		&&(sptr->designation!=DCAPITOL)
		&&(sptr->designation!=DCITY)){
			sptr->people += (rephold * sptr->people)/200;
			if(sptr->people > ABSMAXPEOPLE)
				sptr->people = ABSMAXPEOPLE;
			if(sptr->people * sptr->metal > 2*(rand()%100)*TOMUCHMINED)
				if(sptr->designation==DMINE) sptr->metal--;
			if(sptr->people * sptr->jewels > 2*(rand()%100)*TOMUCHMINED)
				if(sptr->designation==DGOLDMINE) sptr->jewels--;
		} else if(sptr->people<100) {
			sptr->people+=sptr->people/10;
		} else {
			sptr->people += (rephold * sptr->people)/100;
			if(sptr->people * sptr->metal > (rand()%100)*TOMUCHMINED)
				if(sptr->designation==DMINE) sptr->metal--;
			if(sptr->people * sptr->jewels > (rand()%100)*TOMUCHMINED)
				if(sptr->designation==DGOLDMINE) sptr->jewels--;
		}
		/* if no metal/gold left, remove tradegood */
		if(((sptr->designation==DGOLDMINE)&&(sptr->jewels==0))
		||((sptr->designation==DMINE)&&(sptr->metal==0))){
			sptr->tradegood = TG_none;
			sptr->designation = DDEVASTATED;
		}

		/*check all adjacent sectors and decide if met */
		for(i=x-MEETNTN;i<=x+MEETNTN;i++)
		for(j=y-MEETNTN;j<=y+MEETNTN;j++)
		if(ONMAP(i,j)&&(sct[i][j].owner!=0)) {
			if(sptr->owner!=sct[i][j].owner) {
				if(nptr->dstatus[sct[i][j].owner]==UNMET)
					newdip(sptr->owner,sct[i][j].owner);
				if(ntn[sct[i][j].owner].dstatus[sptr->owner]==UNMET)
					newdip(sct[i][j].owner,sptr->owner);
			}
		}

		/* if desert sector... reverts to desert */
		if(tofood(sptr,sptr->owner)<DESFOOD){
			if((sptr->designation != DSTOCKADE)
			&&(sptr->designation != DFORT)
			&&(sptr->designation != DROAD))
			sptr->designation=DNODESIG;
		}
	}

	for(country=1;country<NTOTAL;country++) {
		curntn = &ntn[country];
		if(isntn(curntn->active)){

			/* check for depletion of country through */
			/* lack of a capitol                      */
			if((sct[curntn->capx][curntn->capy].designation!=DCAPITOL)
			  ||(sct[curntn->capx][curntn->capy].owner!=country)) {

				printf("depleting nation %s\n",curntn->name);
				deplete(country);
			}

			spreadsheet(country);
			if ((int)curntn->popularity-2*curntn->inflation < (int)MAXTGVAL) {
				curntn->popularity = max(0,(int)(curntn->popularity-2*curntn->inflation));
			} else curntn->popularity = (char) MAXTGVAL;
			curntn->tsctrs = spread.sectors;
			curntn->tciv=spread.civilians;
			curntn->tfood=spread.food;

			/* take out for charity */
			charity=((spread.gold-curntn->tgold)*curntn->charity)/100;

			if(charity < 0) charity = 0;
			curntn->tgold = spread.gold - charity;

			if(curntn->tciv > 0) charity /= curntn->tciv;
			else charity = 0;

			/* calculate poverty base */
			if (curntn->tgold < 0L) {
				curntn->poverty = 95;
			} else if (curntn->tciv < 100L) {
				/* give some check on civilians */
				curntn->poverty = (unsigned char)20;
			} else if (curntn->tgold/curntn->tciv < 30L) {
				curntn->poverty = (unsigned char)(95L - curntn->tgold/curntn->tciv);
			} else if (curntn->tgold/curntn->tciv < 80L) {
				curntn->poverty = (unsigned char)(65L - (curntn->tgold/curntn->tciv-30L)/2L);
			} else if (curntn->tgold/curntn->tciv < 120L) {
				curntn->poverty = (unsigned char)(40L - (curntn->tgold/curntn->tciv-80L)/4L);
			} else if (curntn->tgold/curntn->tciv < 200L) {
				curntn->poverty = (unsigned char)(30L - (curntn->tgold/curntn->tciv-120L)/8L);
			} else {
				curntn->poverty = (unsigned char)20;
			}

			/* charity increase to popularity */
			curntn->popularity = min(curntn->popularity+5*charity,MAXTGVAL);

			/* charity adjustment to poverty; rounding upward */
			if(curntn->poverty < (charity+1)/2 )
				curntn->poverty = 0;
			else	curntn->poverty -= (charity+1)/2;

			/* Calculate inflation base */
			if(curntn->inflation > 0)
				curntn->inflation = rand()%(curntn->inflation/2+1);
			else curntn->inflation = 0;
			curntn->inflation += (curntn->tax_rate/4 + (rand()%(curntn->tax_rate*3/4+1)));

			/* adjustment for military */
			if (spread.civilians>0)
				curntn->inflation += ((curntn->tmil*100/spread.civilians - 15)/5);
			/* adjustment for poverty */
			curntn->inflation += (curntn->poverty-50)/2;

			/* plus maybe an adjustment for jewel production as a ratio */
			/* for whatever is produced by the country.                 */

			/* now find new total gold talons in nations*/
			if (curntn->tgold > 1000000L) {
				curntn->tgold = (curntn->tgold /
					(400L + curntn->inflation)) * 400L;
			} else {
				curntn->tgold = (curntn->tgold * 400L) /
					(400L + curntn->inflation);
			}

			/* provide goods production */
			curntn->metals=spread.metal;
			curntn->jewels=spread.jewels;
		}
	}
}

/*
 * updmil - Comprehensive military system update and movement calculation
 *
 * Handles the complex military engine that manages army movement points,
 * siege warfare mechanics, naval fleet operations, unit maintenance costs,
 * leadership coordination, and combat status updates. This function is
 * the core of the military simulation system.
 *
 * Algorithm:
 * Phase 1 - Army Processing (for each nation and army):
 * 1. Check for national disarray (lack of proper leadership)
 * 2. Determine default unit types for PC vs NPC nations
 * 3. Calculate movement points based on unit type and army status
 * 4. Handle special status validation (siege, garrison, rule)
 * 5. Process magical effects (flight, roads, sapper abilities)
 * 6. Apply unit maintenance costs (gold for regulars, jewels for monsters)
 * 7. Coordinate group movement under generals (slowest unit + 2)
 *
 * Phase 2 - Naval Operations (for each fleet):
 * 1. Process storm damage and ship destruction (STORMS feature)
 * 2. Calculate fleet movement based on crew efficiency and ship speed
 * 3. Apply magical sailing bonuses (SAILOR magic doubles speed)
 * 4. Update national ship totals and maintenance costs
 * 5. Handle crewless ship abandonment
 *
 * Phase 3 - Siege Resolution:
 * 1. Validate ongoing sieges with proper force ratios
 * 2. Compare attacker strength vs defender strength (2:1 ratio required)
 * 3. Apply SIEGED status to defending units in besieged sectors
 * 4. Generate news reports and mail notifications for sieges
 * 5. Restrict movement for units under siege
 *
 * Movement Calculation System:
 * - Base movement from unit type and national maximum movement
 * - Status modifiers: MARCH (+50%), GARRISON/SIEGE (0), DEFEND (base)
 * - Leadership effects: Disarray reduces movement to 0
 * - Terrain penalties: Roads magic reduces enemy movement in foreign territory
 * - Group coordination: Generals coordinate multiple units at slowest speed +2
 * - Player restrictions: PC units move at half speed without leader presence
 *
 * Siege Warfare Mechanics:
 * - Attackers need 2:1 advantage over defenders to maintain siege
 * - Siege engines count as 3x regular troops in siege calculations
 * - Militia defend at 50% effectiveness when besieged
 * - Successful sieges immobilize all defending units except flying units
 * - Fort value determines siege viability (no siege on unfortified positions)
 *
 * Naval System Features:
 * - Storm damage with percentage-based ship destruction (PSTORM)
 * - Crew dependency: Fleets without crew are automatically abandoned
 * - Speed calculation based on crew efficiency vs optimal crew levels
 * - Magical enhancement: SAILOR magic doubles fleet movement
 * - Maintenance costs scale with fleet holding capacity
 *
 * Economic Integration:
 * - Unit maintenance deducted from national treasury
 * - Different cost structures: Regular units (gold), Monsters (gold + jewels)
 * - SAPPER magic reduces siege engine maintenance by 50%
 * - Fleet maintenance based on total carrying capacity
 * - Monster dismissal when jewel payments cannot be made
 *
 * Parameters:
 *   None - Uses global military and nation state variables
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Updates movement points for all armies and fleets
 *   - Modifies army status based on tactical situations
 *   - Deducts maintenance costs from national treasuries
 *   - Establishes or removes siege conditions
 *   - Dismisses unpaid monster units
 *   - Sends mail notifications to affected players
 *   - Generates news reports for military events
 *   - Uses global variables: country, curntn, ntn array, sct array, occ array
 *
 * Testing Notes:
 *   Category: B (Integration Required)
 *   Approach: Integration testing with full military and world state
 *   Key Tests: Movement calculation, siege mechanics, naval operations
 *   Dependencies: Complete military setup, economic system, magic system
 *   Mock Requirements: Multiple nations, armies, fleets, fortifications
 *   Complexity: Complex - multiple interacting military subsystems
 *
 * Military Balance Features:
 * - Leadership dependency prevents leaderless military effectiveness
 * - Economic constraints limit military expansion through maintenance costs
 * - Siege mechanics create tactical focal points and defensive advantages
 * - Group movement coordination enables combined arms tactics
 * - Storm risks balance naval power projection
 *
 * Performance Considerations:
 * - Processes all armies and fleets every turn
 * - Siege validation requires checking all armies in contested sectors
 * - Complex movement calculations for each unit
 * - Critical path function for military gameplay
 *
 * Historical Context:
 * - STORMS feature adds environmental hazards to naval operations
 * - HIDELOC option provides operational security in news reports
 * - Multiple magic systems integrate with military mechanics
 * - Different unit types have distinct tactical roles and costs
 *
 * Notes:
 *   - Core military simulation engine for strategic gameplay
 *   - Balances tactical complexity with economic constraints
 *   - Integrates leadership, magic, and economic systems
 *   - Creates meaningful strategic choices between unit types
 *   - Thread safety: Not thread-safe due to extensive global variable usage
 */

/****************************************************************/
/*	UPDMIL() 						*/
/* reset military stuff 					*/
/****************************************************************/
#define MAXSIEGE (NTOTAL)
void
updmil (void)
{
	struct	army	*A;
	int	AX, AY, AT;	/* armies x,y locations, type : for speed */
	int armynum,nvynum,flag,dfltunit;
	int army2,asmen,dsmen,nation=0,sieges=0;
	unsigned char siegex[MAXSIEGE],siegey[MAXSIEGE],siegok[MAXSIEGE];

	fprintf(stderr,"updating armies and navies\n");
	for(country=1;country<NTOTAL;country++)
	if(isntn(ntn[country].active)){
		curntn = &ntn[country];

		disarray=TRUE;
		dfltunit=(getleader(curntn->class)-1);
		for(armynum=0;armynum<MAXARM;armynum++)
			if (P_ATYPE==dfltunit && P_ASOLD>0) {
				disarray=FALSE;
				break;
			}

		if(ispc(curntn->active)) {
		prep( country, TRUE );	/* occ[][] now >0 if leader near */
		dfltunit = defaultunit(country);
		} else dfltunit = A_INFANTRY;

		for(armynum=0;armynum<MAXARM;armynum++) if(P_ASOLD>0) {

			A = &curntn->arm[armynum];
			AX = A->xloc;
			AY = A->yloc;
			if(A->unittyp==A_INFANTRY)
				A->unittyp = dfltunit;
			AT=A->unittyp;

			if( AT< MINLEADER ) {
				curntn->tmil+=A->sold;
				if( AT==A_MILITIA ) A->stat=MILITIA;
			}

			/* if group does not have a leader anymore */
			if((A->stat >= NUMSTATUS)
			&&((curntn->arm[A->stat-NUMSTATUS].unittyp<MINLEADER)
			||(curntn->arm[A->stat-NUMSTATUS].sold==0))) {
				A->stat=ATTACK;
			}
			flag=TRUE;

			/*add movement to all armies */
			/*unitmove is 10 times movement rate*/
			if(disarray) A->smove=0;
			else switch(A->stat) {
			case MARCH:
				A->smove=(curntn->maxmove * *(unitmove+(AT%UTYPE)))/5;
				break;
			case MILITIA:
			case ONBOARD:
				A->smove=0;
				break;
			case SIEGE:
				if((sct[AX][AY].owner!=country)
				&&(fort_val(&sct[AX][AY]) > 0)) {
					A->smove=0;
					flag=FALSE;
					for (army2=0;flag==FALSE && army2<sieges;army2++)
						if ((AX==siegex[army2])
						&&(AY==siegey[army2])) flag=TRUE;
					/* if this is a new SIEGE... check it */
					if (flag==FALSE && sieges<MAXSIEGE) {
						siegex[sieges]=AX;
						siegey[sieges]=AY;
						siegok[sieges]=FALSE;
						asmen=0;
						dsmen=0;
						for(nation=0;nation<NTOTAL;nation++){
							for(army2=0;army2<MAXARM;army2++)
							if((ntn[nation].arm[army2].xloc==AX)
							&&(ntn[nation].arm[army2].yloc==AY)
							&&(ntn[nation].arm[army2].stat==SIEGE)){
								if (ntn[nation].arm[army2].unittyp==A_SIEGE)
								asmen+=3*ntn[nation].arm[army2].sold;
								else asmen+=ntn[nation].arm[army2].sold;
							}
						}
						nation=sct[siegex[sieges]][siegey[sieges]].owner;
						for(army2=0;army2<MAXARM;army2++)
						if((ntn[nation].arm[army2].xloc==AX)
						&&(ntn[nation].arm[army2].yloc==AY)){
							if (ntn[nation].arm[army2].unittyp==A_MILITIA)
							dsmen+=ntn[nation].arm[army2].sold/2;
							else dsmen+=ntn[nation].arm[army2].sold;
						}
						if(asmen > 2*dsmen) {
							siegok[sieges]=TRUE;
							sieges++;
							/* keep SIEGE status */
							break;
						}
					} else {
						/* keep SIEGE status */
						if (siegok[army2-1]==TRUE) break;
					}
				}
				flag=FALSE;
				/* should drop through to defend reset */
			case GARRISON:
				if((flag==TRUE)
				&&(fort_val(&sct[AX][AY]) > 0)
				&&(sct[AX][AY].owner==country)) {
					A->smove=0;
					P_AMOVE=0;
					break;
				}
				flag=FALSE;
				/* reset to defend for improper garrison */
			case RULE:
				if((flag==TRUE)
				&&(ISCITY(sct[AX][AY].designation))
				&&(AT>=MINLEADER)&&(AT<MINMONSTER)
				&&(sct[AX][AY].owner==country)) {
					A->smove=0;
					break;
				}
				/* reset to defend for improper Rule */
			case SIEGED:
			case SORTIE:
				/* reset besieged or sortie troops to DEFEND */
			case FLIGHT:
			case MAGDEF:
			case MAGATT:
				/* reset magical stats to DEFEND */
				A->stat=DEFEND;
			default:
				A->smove=(curntn->maxmove * *(unitmove+(AT%UTYPE)))/10;
				break;
			}

			/* empower flight */
			if((avian(AT)==TRUE)
 			&&( A->stat!=ONBOARD )
			&&( A->stat<NUMSTATUS ))
				A->stat=FLIGHT;

			if((magic(country,ROADS)==TRUE)
			&&(sct[AX][AY].owner!=country)){
				if(A->smove>7) A->smove-=4;
				else if (A->smove>4) A->smove=4;
			}

			if((magic(country,SAPPER)==1)
			&&((AT==A_CATAPULT)||(AT==A_SIEGE))){
				curntn->tgold -= A->sold * (*(unitmaint+(AT))) / 2;
			} else if (AT<MINLEADER) {
				curntn->tgold -= A->sold * (*(unitmaint+(AT%UTYPE)));
				if((ispc(ntn[country].active))
				&&(occ[AX][AY] == 0))
					A->smove /= 2;
			} else if (AT>=MINMONSTER) {
				curntn->tgold -= 5L * (*(unitmaint+(AT%UTYPE)));
				if(curntn->jewels > (*(unitmaint+(AT%UTYPE))))
				curntn->jewels -= (long) (*(unitmaint+(AT%UTYPE)));
				else {
					if(ispc(curntn->active)) {
					if(mailopen(country)!=(-1)) {
						fprintf(fm,"Message to %s from Conquer\n\n",curntn->name);
						fprintf(fm,"Your %s (unit %d) leaves due to lack of jewels\n",
							   *(unittype+(AT%UTYPE)),armynum);
						mailclose(country);
						A->sold=0;
					}
					}
				}
			}
		}
		/* group moves at rate of slowest +2 */
		for(armynum=0;armynum<MAXARM;armynum++)
		if(( P_ASTAT == GENERAL )&&(P_ASOLD>0)){
			flag=FALSE;
			for(nvynum=0;nvynum<MAXARM;nvynum++) {
				if((curntn->arm[nvynum].sold>0)
				&&(curntn->arm[nvynum].stat==armynum+NUMSTATUS)){
				flag=TRUE;
				if(P_AMOVE > curntn->arm[nvynum].smove)
					P_AMOVE = curntn->arm[nvynum].smove;
				}
			}
			if(flag==FALSE) P_ASTAT=DEFEND;
			else P_AMOVE+=2;
		}
		/*add to movement of fleets*/
		for(nvynum=0;nvynum<MAXNAVY;nvynum++) {
			/*update sea sectors*/
			if(P_NWSHP!=0 || P_NMSHP!=0 || P_NGSHP!=0) {
#ifdef STORMS
				if(sct[P_NXLOC][P_NYLOC].altitude==WATER) {
/*
 *	Storms should stay around and slowly move
 *	around the world.
 */
				/*all ships sunk on percentage PSTORM*/
				/*pirates never are sunk (implicitly)*/
				if((ntn[country].active != NPC_PIRATE )
				&&(magic(country,SAILOR)==FALSE)
				&&( rand()%100 < PSTORM) ) {
#ifdef HIDELOC
					fprintf(fnews,"3.\tstorm sinks %s fleet at sea\n",curntn->name);
#else
					fprintf(fnews,"3.\tstorm sinks %s fleet in %d,%d\n",curntn->name,P_NXLOC,P_NYLOC);
#endif /* HIDELOC */
					P_NWSHP=0;
					P_NMSHP=0;
					P_NGSHP=0;
					armynum=P_NARMY;
					if(armynum>=0&&armynum<MAXARM) {
						P_ASOLD=0;
					}
					P_NARMY=MAXARM;
					P_NPEOP=0;
					P_NCREW=0;
				}
				/* destroy ships without crew */
				if(P_NCREW==0) {
					P_NWSHP=0;
					P_NMSHP=0;
					P_NGSHP=0;
					armynum=P_NARMY;
					if(armynum>=0&&armynum<MAXARM) {
						P_ASOLD=0;
					}
					P_NARMY=0;
				}
				}
#endif
				if(disarray) P_NMOVE=0;
				else P_NMOVE = (fltspeed(nvynum)*P_NCREW)/SHIPCREW;
				if(magic(country,SAILOR)==TRUE) P_NMOVE*=2;

				curntn->tships += fltships(country,nvynum);
				curntn->tgold -= flthold(nvynum)*SHIPMAINT;
			} else {
				P_NWSHP=0;
				P_NMSHP=0;
				P_NGSHP=0;
			}
		} /* for */
	}
	fprintf(stderr,"doing sieges\n");

	/* kill movement on SIEGED troops */
	for(army2=0;army2<sieges;army2++) {
		if (siegok[army2]==FALSE) continue;
		country= sct[siegex[army2]][siegey[army2]].owner;
		curntn = &ntn[country];
#ifdef HIDELOC
		fprintf(fnews,"2.\tSector in nation %s is under siege\n",
			curntn->name);
#else
		fprintf(fnews,"2.\tNation %s under siege in sector %d,%d\n",
			curntn->name,siegex[army2],siegey[army2]);
#endif /* HIDELOC */
		if(ispc(curntn->active)) {
			if (mailopen( country )!=(-1)) {
				fprintf(fm, "Message to %s from Conquer\n\n",ntn[nation].name);
				fprintf(fm, "\tYou are under siege in sector %d,%d.\n",
					   siegex[army2],siegey[army2]);
				mailclose(country);
			}
		}
		for(armynum=0;armynum<MAXARM;armynum++) if(P_ASOLD>0){
			if(P_ASTAT!=FLIGHT&&(P_AXLOC==siegex[army2])
			&&(P_AYLOC==siegey[army2])) {
				P_AMOVE=0;
				if((P_ASTAT!=ONBOARD)&&(P_ASTAT!=RULE)&&(P_ASTAT!=TRADED))
				P_ASTAT=SIEGED;
			}
		}
	}
	printf("done with military\n");
}

/*
 * updcomodities - Economic commodities management and famine resolution system
 *
 * Handles the critical economic systems that manage food consumption, famine
 * catastrophes, commodity trading balance, and overflow protection. This
 * function implements the harsh realities of medieval economics where food
 * shortages can devastate populations and unbalanced economies self-regulate
 * through automatic commodity trading.
 *
 * Algorithm:
 * Phase 1 - Food Consumption and Famine Processing:
 * 1. Calculate total food consumption (military eats 2x civilian rate)
 * 2. Process famine when food deficit occurs:
 *    a. Target urban populations (towns, cities, capitals) first
 *    b. Kill up to 1/3 of urban population per food deficit
 *    c. Generate famine news reports and player notifications
 * 3. Apply food spoilage rates to remaining food stores
 *
 * Phase 2 - Economic Balance and Overflow Protection:
 * 1. Detect gold/jewel imbalance using GOLDTHRESH ratio
 * 2. Force automatic jewel purchases to maintain economic stability
 * 3. Apply overflow protection to prevent integer overflow crashes
 * 4. Generate news reports for economic anomalies
 *
 * Famine Mechanics:
 * - Military units consume food at 2x P_EATRATE (double civilian consumption)
 * - Civilians consume food at P_EATRATE per person
 * - Food shortages trigger urban famine (rural areas more resilient)
 * - Urban deaths scale with food deficit (1 death per 3 food shortage)
 * - Maximum 33% urban population loss per famine event
 * - Famine targets cities/towns/capitals preferentially over rural areas
 *
 * Economic Regulation:
 * - Automatic commodity trading when gold exceeds GOLDTHRESH*jewels ratio
 * - Forced jewel purchases at GODJEWL/GODPRICE exchange rate
 * - Prevents runaway gold accumulation without jewel backing
 * - Maintains economic balance through mandatory diversification
 *
 * Food Spoilage System:
 * - Remaining food reduced by nation's spoilage rate percentage
 * - Represents natural decay, storage losses, and distribution inefficiency
 * - Creates pressure for regular food production and trade
 * - Prevents infinite food stockpiling strategies
 *
 * Overflow Protection:
 * - Monitors all major economic variables for overflow conditions
 * - Clamps values to BIG constant to prevent integer wraparound
 * - Reports overflow events in news for debugging and balance
 * - Protects game stability from extreme economic conditions
 *
 * Parameters:
 *   None - Uses global nation and world state variables
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Reduces national food supplies based on consumption
 *   - Kills urban populations during famine events
 *   - Forces automatic commodity purchases for economic balance
 *   - Applies food spoilage to remaining stores
 *   - Clamps economic variables to prevent overflow
 *   - Sends mail notifications to affected players
 *   - Generates news reports for famines and economic events
 *   - Uses global variables: country, curntn, ntn array, sct array
 *
 * Testing Notes:
 *   Category: B (Integration Required)
 *   Approach: Integration testing with economic and population systems
 *   Key Tests: Famine calculations, commodity trading, overflow protection
 *   Dependencies: Nation economic data, population systems, mail system
 *   Mock Requirements: Nations with food shortages, economic imbalances
 *   Complexity: Moderate - economic simulation with multiple interacting systems
 *
 * Economic Balance Features:
 * - Harsh famine penalties discourage military over-expansion
 * - Automatic trading prevents economic exploitation
 * - Spoilage creates ongoing resource pressure
 * - Urban targeting makes city development risky during food crises
 * - Overflow protection ensures game stability
 *
 * Historical Context:
 * - HIDELOC option provides operational security for famine locations
 * - Seasonal messaging provides immersive timeline context
 * - Military food burden reflects historical army logistics challenges
 * - Urban famine vulnerability models medieval city dependencies
 *
 * Mathematical Formulas:
 * - Food Consumption: (tmil * P_EATRATE * 2) + (tciv * P_EATRATE)
 * - Famine Deaths: min(people/3, food_deficit/3)
 * - Spoilage: tfood * (100 - spoilrate) / 100
 * - Commodity Trading: jewels += (excess_gold * GODJEWL / GODPRICE)
 *
 * Notes:
 *   - Critical economic regulation system preventing game-breaking accumulation
 *   - Balances military expansion against economic sustainability
 *   - Creates meaningful resource management decisions
 *   - Famine system adds realistic consequences to poor planning
 *   - Thread safety: Not thread-safe due to global variable dependencies
 */

/****************************************************************/
/*	UPDCOMODITIES()						*/
/* update commodities						*/
/****************************************************************/
void
updcomodities (void)
{
	register struct s_sector	*sptr;
	register int x,y;
	long xx;
	float tempflt;
	long dead = 0;

	fprintf(fnews,"2\tWORLD ECONOMY & DECLARATIONS OF WAR\n");
	printf("working on world economy\n");
	for(country=1;country<NTOTAL;country++)
	if(isntn(ntn[country].active)){
		curntn = &ntn[country];
		/*soldiers eat  2 times as much */
		curntn->tfood-=curntn->tmil*P_EATRATE*2;
		curntn->tfood-=curntn->tciv*P_EATRATE;

		/*starve people*/
		if(curntn->tfood<0) for(x=0;x<MAPX;x++) for(y=0;y<MAPY;y++) {
			sptr = &sct[x][y];
			if((sptr->owner==country)
			&&((sptr->designation==DTOWN)
			||(sptr->designation==DCAPITOL)
			||(sptr->designation==DCITY))
			&&(curntn->tfood<0)){
				/*lose one person in city per three food*/
				/*maximum of 1/3 people in city lost*/
				if(sptr->people < curntn->tfood){
					sptr->people+=curntn->tfood/3;
					curntn->tfood=0;
				} else {
					curntn->tfood+=sptr->people;
					dead = sptr->people/3;
					sptr->people -= dead;
				}
#ifdef HIDELOC
				fprintf(fnews,"2.\tfamine hits town in %s.\n",curntn->name);
#else
				fprintf(fnews,"2.\tfamine hits town at %d,%d in %s.\n",x,y,curntn->name);
#endif /* HIDELOC */
				printf("famine hits town at %d,%d in %s.\n",x,y,curntn->name);
				if(ispc(curntn->active)){
				if (mailopen( country )!=(-1)) {
					fprintf(fm,"Message to %s from CONQUER\n\n",curntn->name);
					fprintf(fm,"During the %s of Year %d,\n",PSEASON(TURN),YEAR(TURN));
					fprintf(fm,"a famine hit your town at %d,%d.\n",x,y);
					fprintf(fm,"%ld out of %ld people died.\n",dead,sptr->people);
					mailclose(country);
				}
				}
			}
		}
		/*this state can occur if few people live in cities*/
		if(curntn->tfood<0) curntn->tfood=0L;
		tempflt = (float) curntn->tfood * (100-curntn->spoilrate);
		curntn->tfood = (long) (tempflt / 100.0);

		if((0.0+curntn->tgold) - GOLDTHRESH*(0.0+curntn->jewels) > 0.0){
			/* buy jewels off commodities board */
			xx=curntn->tgold-GOLDTHRESH*curntn->jewels;
			if (ispc(curntn->active)) {
				if (mailopen(country)!=(-1)) {
					fprintf(fm,"Message from Conquer\n\n");
					fprintf(fm,"Gold imbalance forced your treasury to purchase\n");
					fprintf(fm,"%ld jewels for %ld gold talons to compensate.\n",
						dtol((double)xx*GODJEWL/GODPRICE)
						,xx);
					mailclose(country);
				}
			}
			curntn->jewels += dtol((double)xx*GODJEWL/GODPRICE);
			curntn->tgold  -= xx;
		}

		/* fix overflow problems */
		if(curntn->tgold < -1*BIG)  {
			fprintf(fnews,"2.\tVariable Overflow - gold in nation %s\n",curntn->name);
			curntn->tgold=BIG;
		}
		if(curntn->tfood < -1*BIG)  {
			fprintf(fnews,"2.\tVariable Overflow - food in nation %s\n",curntn->name);
			curntn->tfood=BIG;
		}
		if(curntn->jewels < -1*BIG) {
			fprintf(fnews,"2.\tVariable Overflow - jewels in nation %s\n",curntn->name);
			curntn->jewels=BIG;
		}
		if(curntn->metals < -1*BIG)  {
			fprintf(fnews,"2.\tVariable Overflow - metal in nation %s\n",curntn->name);
			curntn->metals=BIG;
		}
	}
}

/*
 * updleader - Leader development and monster spawning system
 *
 * Manages the critical leadership generation system that spawns both natural
 * leaders and supernatural monsters based on national characteristics and
 * magical abilities. This function ensures nations maintain leadership
 * continuity while providing magical nations with monster reinforcements.
 *
 * Algorithm:
 * Phase 1 - Monster Spawning (Spring season only):
 * 1. Check for monster summoning magic (MI_MONST, AV_MONST, MA_MONST)
 * 2. Determine monster strength tier based on magical abilities
 * 3. Randomly select appropriate monster type within strength limits
 * 4. Spawn monster at national capital with default stats
 * 5. Notify player of monster birth via mail system
 *
 * Phase 2 - Natural Leader Birth:
 * 1. Calculate birth rate based on national class characteristics
 * 2. Apply random chance for leader generation (class-dependent rates)
 * 3. Spawn leader type appropriate to national class
 * 4. Place new leader at capital with standard starting attributes
 * 5. Send player notification of leadership addition
 *
 * Monster Spawning System:
 * - Seasonal Restriction: Only occurs during SPRING season
 * - Magical Requirement: Requires MI_MONST magic minimum
 * - Strength Tiers: MI_MONST (100), AV_MONST (200), MA_MONST (BIG)
 * - Type Selection: Random choice from MINMONSTER to MAXMONSTER range
 * - Strength Validation: Monster strength must not exceed magical capacity
 * - Capital Spawn: All monsters appear at national capital location
 *
 * Leader Birth Rates by Class:
 * - High Birth Rate (50/400 = 12.5%): King, Trader, Emperor, NPC
 * - Medium Birth Rate (25/400 = 6.25%): Wizard, Priest, Pirate, Warlord, Demon
 * - Low Birth Rate (2/400 = 0.5%): Dragon, Shadow (rare/powerful classes)
 * - Error Handling: Undefined classes trigger program termination
 *
 * Leadership Mechanics:
 * - Leader Type: Determined by getleader(class) function for class appropriateness
 * - Starting Strength: Based on unitminsth table for leader type
 * - Default Status: All new leaders start in DEFEND status
 * - Movement Bonus: New leaders get 2x national maximum movement
 * - Capital Placement: Leaders always spawn at national capital
 *
 * Spawn Limitations:
 * - Army Slot Dependency: Requires available army slot (MAXARM limit)
 * - Single Spawn: Only one leader/monster per turn maximum
 * - Capital Requirement: Spawning location tied to capital location
 * - Class Validation: Prevents spawning for undefined nation classes
 *
 * Parameters:
 *   None - Uses global nation state and magic system variables
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Creates new leader or monster units in available army slots
 *   - Places new units at national capitals
 *   - Sends mail notifications to human players
 *   - Generates console output for debugging/monitoring
 *   - Uses global variables: ntn array, curntn, nation
 *
 * Testing Notes:
 *   Category: B (Integration Required)
 *   Approach: Integration testing with magic system and nation setup
 *   Key Tests: Birth rate calculations, monster type selection, class handling
 *   Dependencies: Magic system, nation class data, army management
 *   Mock Requirements: Nations with different classes, magic abilities
 *   Complexity: Moderate - probabilistic system with class-dependent logic
 *
 * Balance Considerations:
 * - Birth rates balanced to prevent leader flooding while ensuring succession
 * - Monster spawning limited to magical nations to maintain fantasy balance
 * - Seasonal restriction on monsters prevents constant supernatural reinforcement
 * - Class-based rates reflect leadership rarity and power level
 * - Capital spawning creates predictable but defendable leadership source
 *
 * Strategic Implications:
 * - Provides leadership succession for long-term nation survival
 * - Rewards magical investment with monster reinforcements
 * - Creates incentive to protect national capitals (leadership source)
 * - Balances rare classes with lower birth rates
 * - Enables recovery from leadership losses
 *
 * Historical Context:
 * - Spring spawning reflects natural birth/awakening cycles
 * - Class-based rates model different leadership structures
 * - Monster summoning integrates fantasy elements
 * - Capital spawning models hereditary leadership traditions
 *
 * Notes:
 *   - Essential for long-term nation viability and leadership continuity
 *   - Integrates magic system with leadership mechanics
 *   - Provides both mundane and supernatural leadership sources
 *   - Creates strategic value for capital protection
 *   - Thread safety: Not thread-safe due to global variable dependencies
 */

/****************************************************************/
/* Conquer: Copyright (c) 1988 by Edward M Barlow              */
/*	UPDLEADER()						*/
/****************************************************************/
void
updleader (void)
{
	int	nation,armynum,born,type;
	printf("working on national leaders\n");
	for(nation=0;nation<NTOTAL;nation++) {
		curntn = &ntn[nation];
		if(!isntn(curntn->active)) continue;

		/* monster nations get monsters */
		if((SEASON(TURN) == SPRING)&&(magic(nation,MI_MONST)==TRUE)) {
			born=100;	/* born represents strength of monst */
			if(magic(nation,AV_MONST)==TRUE) born=200;
			if(magic(nation,MA_MONST)==TRUE) born=BIG;

			do type = MINMONSTER + rand()%(MAXMONSTER-MINMONSTER+1);
			while( *(unitminsth+(type%UTYPE)) > born);

			for(armynum=0;armynum < MAXARM;armynum++) {
				if(P_ASOLD != 0) continue;
				P_ATYPE = type;
				P_ASOLD = *(unitminsth+(type%UTYPE));
				P_AXLOC = curntn->capx;
				P_AYLOC = curntn->capy;
				P_ASTAT = DEFEND;
				P_AMOVE = 2*curntn->maxmove;
				if( ispc( ntn[nation].active ) ){
					if (mailopen( nation )!=(-1)) {
					fprintf(fm,"Message to %s from Conquer:\n\n",ntn[nation].name);
					fprintf(fm,"\t\tMonster born in your nation!\n");
					mailclose(nation);
					}
				}
				printf("\tmonster born in nation %s\n",curntn->name);
				break;
			}
		}

		switch(curntn->class){	/* get national born rate */
		case C_NPC:
		case C_KING:
		case C_TRADER:
		case C_EMPEROR:	born = 50; break;
		case C_WIZARD:
		case C_PRIEST:
		case C_PIRATE:
		case C_WARLORD:
		case C_DEMON:	born = 25; break;
		case C_DRAGON:
		case C_SHADOW:	born = 2; break;
		default:
			printf("ERROR - national class (%d) undefined\n",curntn->class);
			abrt();
		}
		/* born represents yearly birth rate */
		if( rand()%400 >= born ) continue;

		for(armynum=0;armynum < MAXARM;armynum++) { /* add one leader */
			if(P_ASOLD != 0) continue;
			P_ATYPE = getleader(curntn->class);
			P_ASOLD = *(unitminsth+(P_ATYPE%UTYPE));
			P_AXLOC = curntn->capx;
			P_AYLOC = curntn->capy;
			P_ASTAT = DEFEND;
			P_AMOVE = 2*curntn->maxmove;
			if( ispc( ntn[nation].active ) ){
				if(mailopen( nation )!=(-1)) {
					fprintf(fm,"Message to %s from Conquer:\n\n",ntn[nation].name);
					fprintf(fm,"\t\tLeader born in your nation!\n");
					mailclose(nation);
				}
			}
			printf("\tleader born in nation %s\n",curntn->name);
			break;
		}
	}
}

/*
 * move_people - Civilian population movement algorithm based on sector attractiveness
 *
 * Implements sophisticated population dynamics modeling where civilians migrate
 * between sectors based on relative attractiveness calculations. This function
 * creates realistic population flows that respond to economic opportunities,
 * safety conditions, and environmental factors, forming the demographic
 * foundation of the economic simulation.
 *
 * Algorithm:
 * 1. Initialize rolling population buffer for efficient map processing
 * 2. For each sector owned by the current nation:
 *    a. Calculate total attractiveness in 5x5 neighborhood around sector
 *    b. Distribute current population proportionally to surrounding attractions
 *    c. Move population fractions toward equilibrium (1/5 convergence rate)
 *    d. Update rolling buffer with new population distributions
 * 3. Apply final population changes to world map
 *
 * Mathematical Model:
 * EQUILIBRIUM(sector) = Attractiveness(sector) / Total_Area_Attractiveness * Total_Area_Population
 * MOVEMENT_DELTA = (EQUILIBRIUM - Current_Population) / 5
 * NEW_POPULATION = Current_Population + MOVEMENT_DELTA
 *
 * Attractiveness-Based Distribution:
 * - Each sector attracts population proportional to its attractiveness rating
 * - Population flows from less attractive to more attractive areas
 * - 5x5 neighborhood (2-sector radius) determines local migration area
 * - Gradual convergence (20% per turn) prevents violent population swings
 *
 * Buffer Management System:
 * - Uses rotating 5-column buffer (newpop[x%5][y]) for memory efficiency
 * - Processes map in column-by-column fashion to minimize memory usage
 * - Maintains consistency across map boundaries with careful buffer rotation
 * - Handles map edges correctly with boundary condition management
 *
 * Population Flow Mechanics:
 * - Migration occurs only within same-nation territories (no cross-border movement)
 * - Empty sectors (people=0) do not participate in population exchanges
 * - Attractiveness calculated by attract() function (food, trade, safety, race preferences)
 * - Movement preserves total population (conservation of people)
 * - Gradual equilibration prevents population shock and maintains stability
 *
 * Memory Management:
 * - Rolling buffer technique processes entire world with minimal memory overhead
 * - Column-wise processing enables efficient large-world handling
 * - Boundary management ensures correct edge cases without buffer overruns
 * - Modular arithmetic (x%5) provides efficient circular buffer indexing
 *
 * Performance Optimizations:
 * - Processes only owned sectors to reduce computation
 * - Uses integer arithmetic throughout for speed and precision
 * - Minimizes memory allocation with fixed-size circular buffer
 * - Batches population updates to reduce write operations
 *
 * Parameters:
 *   None - Uses global nation, sector, and attractiveness data
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Modifies population levels across all sectors owned by current nation
 *   - Uses and modifies global newpop buffer for intermediate calculations
 *   - Reads attractiveness matrix (attr) calculated by attract() function
 *   - Uses global variables: country, sct array, newpop array, attr array
 *
 * Testing Notes:
 *   Category: A (Unit Testable)
 *   Approach: Unit testing with mock attractiveness data and sector setup
 *   Key Tests: Population conservation, equilibrium convergence, boundary handling
 *   Dependencies: Attractiveness matrix, sector ownership data
 *   Mock Requirements: Nation with multiple sectors, attractiveness values
 *   Complexity: Moderate - algorithmic complexity with mathematical precision requirements
 *
 * Mathematical Properties:
 * - Population Conservation: Total population remains constant across moves
 * - Convergence: System approaches equilibrium state over multiple turns
 * - Stability: Gradual movement prevents oscillation and population shock
 * - Proportionality: Population distribution matches attractiveness ratios
 * - Locality: Migration limited to local neighborhoods (5x5 area)
 *
 * Demographic Modeling Features:
 * - Economic Migration: Population flows toward profitable trade centers
 * - Safety Migration: Civilians flee from dangerous or war-torn areas
 * - Racial Preferences: Different races prefer different terrain types
 * - Capital Attraction: Proximity to capital increases sector desirability
 * - Resource-Based Movement: Mining and farming opportunities drive migration
 *
 * Integration with Game Systems:
 * - Works with attract() function output for sector desirability
 * - Influences economic calculations through population distribution
 * - Affects military recruitment pools in population centers
 * - Creates strategic value for improving sector attractiveness
 * - Drives urban/rural population dynamics
 *
 * Historical Context:
 * - Models medieval population movements based on economic opportunity
 * - Reflects historical migration patterns toward trade centers and safety
 * - Implements gradual demographic change rather than sudden population shifts
 * - Balances individual choice with collective demographic trends
 *
 * Notes:
 *   - Core demographic engine driving economic and military population distribution
 *   - Creates dynamic population landscapes responding to player actions
 *   - Essential for realistic economic modeling and strategic depth
 *   - Mathematical precision ensures stable long-term demographic patterns
 *   - Thread safety: Not thread-safe due to global buffer and sector modifications
 */

/* MOVE CIVILIANS based on the ratio of attractivenesses
 *
 * EQUILIBRIUM(1) = A1 / (A1 + A2) * (P1 + P2)
 * EQUILIBRIUM(2) = A2 / (A1 + A2) * (P1 + P2)
 * MOVE 1/5 of way to equilibrium each turn
 * DELTA(1) = (EQUILIBRIUM(1) - P1) / 5 =(A1P2 - P1A2) / 5(A1 + A2)
 * DELTA(2) = (EQUILIBRIUM(2) - P2) / 5 =(A2P1 - P2A1) / 5(A1 + A2) = -DELTA(1)
 * (i, j) is refered to as 1, (x, y) as 2
 */
void
move_people (void)
{
	register int x, y, i, j;
	register struct s_sector *sptr;
	int t_attr;
	long *curpop, moved;

	/* store the first two rows for computation */
	for (x = 0; x < 3; x++) {
		for (y = 0; y < MAPY; y++) {
			if (sct[x][y].owner == country)
				newpop[x][y] = sct[x][y].people;
			else newpop[x][y] = 0L;
		}
	}

	/* go through entire map to move civilians */
	for (x = 0; x < MAPX; x++) {
		for (y = 0; y < MAPY; y++) {
			sptr = &sct[x][y];
			if ((sptr->owner == country) && (sptr->people != 0)) {

				for (t_attr = 0, i = x - 2; i < x + 3; i++)
				for (j = y - 2; j < y + 3; j++)
				if (ONMAP(i, j))
					t_attr += attr[i][j];

				if (t_attr > 0) {
					t_attr *= 5;
					curpop = &newpop[x%5][y];
					for (i = x - 2; i < x + 3; i++)
					for (j = y - 2; j < y + 3; j++)
					if (ONMAP(i, j)) {
						moved = sptr->people * attr[i][j];
						if (moved > 0) {
							moved /= t_attr;
							*curpop -= moved;
							newpop[i%5][j] += moved;
						}
					}
				}
			}
		}
		/* store old row, and read in new */
		for (y = 0; y < MAPY; y++) {
			if (x > 1) {
				if (sct[x-2][y].owner == country)
					sct[x-2][y].people = newpop[(x-2)%5][y];
			}
			if (x < MAPX - 3) {
				if (sct[x+3][y].owner == country)
					newpop[(x+3)%5][y] = sct[x+3][y].people;
				else newpop[(x+3)%5][y] = 0L;
			}
		}
	}

	/* assign values for final rows */
	for (x = MAPX - 2; x < MAPX; x++) {
		for (y = 0; y < MAPY; y++) {
			if (sct[x][y].owner == country)
				sct[x][y].people = newpop[x%5][y];
		}
	}
}
