/*
 * combat.c - Combat system and battle resolution
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
#include <math.h>
#include "header.h"
#include "data.h"

#define	MGKNUM	32		/* number of units possible in battle */
#define	ATKR	2
#define	DFND	1
#define	NTRL	0
#define	WIMP	3

/* ================= EXTERNAL DEPENDENCIES ================= */

extern	FILE	*fpmsg;		/* Message file for game communications */
extern	FILE	*fnews;		/* News file for battle reports */
extern	short	country;	/* Current nation context for operations */

/* ================= GLOBAL COMBAT STATE VARIABLES ================= */
/*
 * MODERNIZATION NOTE: These global variables should be encapsulated
 * in a combat context structure for better thread safety and clarity
 */

/*
 * Retreat Coordination Variables
 * Manage unit withdrawal from battle when conditions warrant retreat
 */
short	retreatside;	/* Retreating side: ATKR, DFND, or none (0) */
short	retreatx;	/* Retreat destination x coordinate */
short	retreaty;	/* Retreat destination y coordinate */

/*
 * Battle Participant Arrays
 * Track all units involved in current battle for resolution calculations
 */
int	unit[MGKNUM];		/* Army/navy unit numbers participating */
int	owner[MGKNUM];		/* Nation ownership of each unit */
int	side[MGKNUM];		/* Combat side assignment (ATKR/DFND/NTRL) */
long	troops[MGKNUM];		/* Starting troop strength for each unit */

/*
 * Battle Location and Primary Combatants
 * Define the geographic and political context of current battle
 */
int	xspot,yspot;		/* Map coordinates where battle occurs */
int	anation;		/* Primary attacking nation in this battle */
int	dnation;		/* Primary defending nation in this battle */
int	count=0;                /* Total number of units in battle sector */

/* ================= COMBAT TYPE AND STATUS INDICATORS ================= */
/*
 * Combat Type Tracking Flags
 * Used to prevent multiple battles in same sector and track combat history
 */
#define COMBAT_X	0	/* No combat occurred in sector */
#define COMBAT_A	1	/* Army combat possible/initiated */
#define COMBAT_N	2	/* Naval combat possible/initiated */
#define FOUGHT_A	4	/* Army combat already completed */
#define FOUGHT_N	8	/* Naval combat already completed */

/*
 * combat - Global Combat Orchestration and Battle Management
 *
 * Scans the entire world map to identify and resolve all potential battles
 * between nations. Handles both army and naval combat in a systematic manner,
 * ensuring each sector is processed only once per turn to prevent conflicts.
 *
 * ALGORITHM:
 * 1. Initialize combat tracking matrix for all map sectors
 * 2. For each active nation in reverse order (highest ID first):
 *    a. Check all armies for attack status and valid targets
 *    b. Check all navies for combat opportunities
 *    c. Resolve battles immediately when valid combatants found
 * 3. Clean up allocated memory and report completion
 *
 * COMBAT RESOLUTION ORDER:
 * - Nations processed in reverse ID order for turn balance
 * - Army combat checked before naval combat for each nation
 * - Battles resolved immediately upon detection to handle cascading effects
 * - Sectors marked as "fought" to prevent duplicate battles
 *
 * BATTLE PARTICIPATION CRITERIA:
 * Army Combat:
 * - Units must be in attack mode (ATTACK, SORTIE, etc.)
 * - Units must have hostile diplomatic relationships
 * - Minimum soldier count requirements
 * - Sector not already fought in this turn
 *
 * Naval Combat:
 * - Fleets must have warships present
 * - 2-sector engagement range for naval battles
 * - Water terrain requirements for fleet positioning
 * - Hostile diplomatic relationships required
 *
 * MEMORY MANAGEMENT:
 * - Allocates 2D matrix for combat tracking (freed at end)
 * - Combat participant arrays reused between battles
 * - Proper cleanup ensures no memory leaks
 *
 * DIPLOMATIC CONSIDERATIONS:
 * - JIHAD relationships always create combat
 * - WAR status enables automatic engagement
 * - TREATY and ALLIED nations may join battles
 * - Nation ownership of sectors affects targeting
 *
 * Parameters: None (operates on global game state)
 * Returns: None (modifies global army/navy state)
 *
 * Side Effects:
 * - Modifies army and navy unit positions and strengths
 * - Updates sector ownership through battle outcomes
 * - Generates battle reports in news and mail systems
 * - May trigger cascading diplomatic relationship changes
 *
 * Testing Notes:
 * Category: B (Integration) - Requires full game state and multiple nations
 * Approach: Integration testing with mock diplomatic relationships
 * Key Tests:
 *   - Multiple nation combat scenarios
 *   - Army vs naval combat differentiation
 *   - Retreat and capture mechanics
 *   - Diplomatic side assignment correctness
 * Dependencies: Global nation array, sector map, diplomatic relationships
 * Mock Requirements: Nation data, army/navy arrays, sector ownership
 * Complexity: Complex - Full game system integration with diplomatic logic
 *
 * Notes:
 * - Critical for turn processing and game balance
 * - Performance scales with map size and nation count
 * - Thread safety issues due to global variable usage
 * - Consider refactoring to use context structure
 */
void
combat()
{
	register int i,j;
	char	**fought; 		/* SET: if already fought in sctr */
	int	temp,ctry;
	int	initialized=FALSE;	/* TRUE if arrays initialized */
	short	armynum,nvynum;
	int	valid;
	struct  s_nation *nptr;
	struct  army	 *aptr;

	fought = (char **) m2alloc(MAPX,MAPY,sizeof(char));

	printf("Run Combat Routines\n");
	fprintf(fnews,"4\tBATTLE SUMMARY STATISTICS\n");
	/*for each nation, if in attack mode run a check*/

	/* no sectors have been fought in yet */
	for(i=0;i<MAPX;i++) for(j=0;j<MAPY;j++) fought[i][j]=COMBAT_X;

	for(ctry=NTOTAL-1;ctry>0;ctry--) if(isactive(ntn[ctry].active)) {

		nptr = &ntn[ctry];

		/*army combat*/
		for(j=0;j<MAXARM;j++) {
			aptr = &nptr->arm[j];
			if((aptr->sold>0)
			&&(aptr->stat>=ATTACK)
			&&(aptr->stat<=SORTIE||aptr->stat>=NUMSTATUS)
			&&(!(fought[aptr->xloc][aptr->yloc]&COMBAT_A))){

			/* someone can initiate combat in xspot,yspot */
			xspot=aptr->xloc;
			yspot=aptr->yloc;
			fought[xspot][yspot]|=COMBAT_A;

			/*initialize matrix*/
			if( !initialized ) {
				for(temp=0;temp<MGKNUM;temp++){
					unit[temp]=owner[temp]=(-1);
					side[temp]=NTRL;
					troops[temp]=0;
				}
				initialized=TRUE;
			}

			/*check all armies in sector and add to matrix*/
			count=0;
			valid=FALSE;
			/*is valid,set matrix*/
			for(country=0;country<NTOTAL;country++)
			if(isactive(ntn[country].active))
			for(armynum=0;armynum<MAXARM;armynum++)
			if((ASOLD>0)
			&&(ASTAT!=SCOUT)
			&&(AXLOC==xspot)
			&&(AYLOC==yspot)
			&&(count<MGKNUM)) {
				if((country!=ctry)
				&&(nptr->dstatus[country]>HOSTILE)) {
					valid=TRUE;
					if( sct[xspot][yspot].owner==ctry ) {
						dnation=ctry;
						anation=country;
					} else if(( rand()%2==0 )
					||( sct[xspot][yspot].owner==country )){
						anation=ctry;
						dnation=country;
					} else {
						dnation=ctry;
						anation=country;
					}
				}
				unit[count]=armynum;
				owner[count]=country;
				count++;
			}

			if(valid==TRUE) {
				fight();
				initialized=FALSE;
			}
		}
		}

		/*navy combat*/
		for(j=0;j<MAXNAVY;j++)
		if((nptr->nvy[j].warships!=0)
		&&(!(fought[nptr->nvy[j].xloc][nptr->nvy[j].yloc]&COMBAT_N))) {

			xspot=nptr->nvy[j].xloc;
			yspot=nptr->nvy[j].yloc;
			fought[xspot][yspot]|=COMBAT_N;

			/*initialize matrix*/
			if( !initialized ){
				for(temp=0;temp<MGKNUM;temp++){
					unit[temp]= owner[temp]=(-1);
					side[temp]=NTRL;
					troops[temp]=0;
				}
				initialized=TRUE;
			}

			/*check all fleets in 2 sector range and add to matrix*/
			count=0;
			valid=FALSE;
			/*is valid,set matrix*/
			for(country=0;country<NTOTAL;country++)
			if(isactive(ntn[country].active))
			for(nvynum=0;nvynum<MAXNAVY;nvynum++)
			if((NWSHP+NMSHP+NGSHP!=0)
			&&(((NXLOC==xspot) && (NYLOC==yspot)) ||
			   (sct[NXLOC][NYLOC].altitude==WATER
			    &&(abs(NXLOC-xspot)<=2)
			    &&(abs(NYLOC-yspot)<=2)))
			&&(count<MGKNUM)) {
				fought[NXLOC][NYLOC]|=COMBAT_N;
				if((country!=ctry)
				&&(nptr->dstatus[country]>HOSTILE)){
					valid=TRUE;
					anation=ctry;
					dnation=country;
				}
				unit[count]=nvynum;
				owner[count]=country;
				count++;
			}
			if(valid==TRUE) {
				navalcbt();
				initialized=FALSE;
			}
		}
	}
	free(fought);
	printf("\nall army and navy attacks completed\n");
}

/* macro for owner, accounts for runaway indicator */
#define UOWNER(x) ((owner[(x)]<(-1))?(-owner[(x)]-1):(owner[(x)]))
/*
 * fight - Individual Army Battle Resolution Engine
 *
 * Resolves a single army battle using the participant matrices populated
 * by the combat() function. Implements complex combat mechanics including
 * unit types, terrain effects, fortifications, magic bonuses, and retreat.
 *
 * BATTLE RESOLUTION ALGORITHM:
 * 1. Assign combat sides based on diplomatic relationships
 * 2. Handle pre-battle unit defections (mercenaries, orcs, goblins)
 * 3. Calculate total attacking and defending strength
 * 4. Apply unit-specific combat bonuses and terrain modifiers
 * 5. Generate random battle outcome using bell curve dice roll
 * 6. Calculate percentage losses for both sides based on odds
 * 7. Apply losses to individual units with special case handling
 * 8. Process retreats and unit movement
 * 9. Generate comprehensive battle reports for all participants
 *
 * COMBAT MECHANICS:
 * Strength Calculation:
 * - Base unit soldier counts with unit type modifiers
 * - Terrain bonuses for defenders (mountains, forests, etc.)
 * - Fortification bonuses for defending units
 * - Magic power effects and special abilities
 * - Leadership bonuses for army groups
 *
 * Loss Calculation:
 * - Bell curve random roll (5d21-5) for base outcome
 * - Odds-based adjustment favoring stronger side
 * - Fortification effects increase casualties
 * - Unit type considerations (leaders vs regular troops)
 * - Minimum unit survival thresholds
 *
 * SPECIAL MECHANICS:
 * Unit Defection:
 * - Mercenaries/orcs/goblins may refuse to fight or flee
 * - Probability based on odds and unit morale
 * - Defecting units retreat with casualty penalties
 *
 * Fortification Combat:
 * - Catapults and siege engines gain attack bonuses
 * - Possible fortress damage during siege warfare
 * - Archer bonuses when defending fortifications
 * - Cavalry penalties in fortress combat
 *
 * Leader Mortality:
 * - Leaders (heroes, etc.) face death probability
 * - Death releases any units under their command
 * - Higher casualty rates increase leader death chance
 *
 * Retreat Mechanics:
 * - Automatic retreat triggers based on loss ratios
 * - Retreat destination calculated by fdxyretreat()
 * - Units unable to retreat suffer additional casualties
 * - Militia units disband to civilian population
 *
 * Vampire System:
 * - Non-vampire casualties feed vampire units
 * - Zombie armies gain strength from battle deaths
 * - Magic system integration for undead mechanics
 *
 * Parameters: None (uses global battle participant arrays)
 * Returns: None (modifies global unit state and generates reports)
 *
 * Side Effects:
 * - Modifies army unit strengths and positions
 * - Updates sector ownership and fortification levels
 * - Generates mail messages to all battle participants
 * - Writes battle summary to news file
 * - May trigger leader death and unit release
 * - Updates vampire unit strengths from casualties
 *
 * Testing Notes:
 * Category: B (Integration) - Requires army data and diplomatic systems
 * Approach: Integration testing with various unit compositions
 * Key Tests:
 *   - Even odds battles with random variation
 *   - Overwhelming advantage scenarios
 *   - Fortification siege mechanics
 *   - Magic system interactions
 *   - Retreat and capture outcomes
 *   - Leader death and unit release
 * Dependencies: Nation arrays, diplomatic status, magic system
 * Mock Requirements: Multiple nations with armies, diplomatic relationships
 * Complexity: Complex - Intricate combat calculations with many variables
 *
 * Notes:
 * - Core gameplay mechanic determining territorial control
 * - Complex interaction between multiple game systems
 * - Performance critical for large battles
 * - Historical combat balance maintained for game compatibility
 */
void 
fight (void)
{
	int	roll,strength,fortdam=FALSE;
	int	odds;			/* odds (asold/dsold) times 100 */
	int	done;
	int	i,j,k;
	long	asold=0,dsold=0;	/*a's and d's total soldiers*/
	float astr=0,dstr=0;		/*a's and d's relative strength*/
	long	Aloss,Dloss;    	/*a's and d's total losses*/
	int	PAloss,PDloss;		/*percent a and d loss*/
	long	loss;
	int	abonus=0,dbonus=0;	/* bonus aggregate */
	long	vampire=0;		/* # non vamps deaded */
	short	nvamps=0;		/* number of vampire armies */

	/* determine who is attacker & who is on defenders side?*/
	for(j=0;j<count;j++) if(owner[j]!=(-1)){
		if(owner[j]==anation) side[j]=ATKR;
		else if(owner[j]==dnation) side[j]=DFND;
		else if(ntn[anation].dstatus[owner[j]]==JIHAD) side[j]=DFND;
		else if(ntn[owner[j]].dstatus[anation]==JIHAD) side[j]=DFND;
		else if(ntn[anation].dstatus[owner[j]]==WAR)   side[j]=DFND;
		else if(ntn[owner[j]].dstatus[anation]==WAR)   side[j]=DFND;
		else if((ntn[owner[j]].dstatus[anation]==TREATY)&&(ntn[owner[j]].dstatus[dnation]>HOSTILE)) side[j]=ATKR;
		else if((ntn[owner[j]].dstatus[anation]==ALLIED)&&(ntn[owner[j]].dstatus[dnation]>HOSTILE)) side[j]=ATKR;
	}

	/*calculate number of troops and assign statuses */
	asold=0;
	dsold=0;
	for(i=0;i<count;i++) if(owner[i]>(-1)) {
		/* record troops for all units in sector */
		troops[i]=ntn[owner[i]].arm[unit[i]].sold;

		if(((ntn[owner[i]].arm[unit[i]].unittyp == A_MERCENARY)
		||(ntn[owner[i]].arm[unit[i]].unittyp == A_ORC)
		||(ntn[owner[i]].arm[unit[i]].unittyp == A_GOBLIN))
		&&( ntn[owner[i]].arm[unit[i]].stat < NUMSTATUS )
		&&( rand()%100<15 )) {
			if( ispc(ntn[owner[i]].active)) {
				if (mailopen( owner[i] )!=(-1)) {
				fprintf(fm,"Message to %s from Conquer\n\n",ntn[owner[i]].name);
				fprintf(fm,"  Your %s Army %d Refuses to Fight\n",
				  unittype[ntn[owner[i]].arm[unit[i]].unittyp],
				  unit[i]);
				mailclose(owner[i]);
				}
			}
			retreatside = side[i];
			fdxyretreat();
			if((retreatx==xspot)&&(retreaty==yspot)){
				/* move to capitol & kill 30% */
				ntn[owner[i]].arm[unit[i]].xloc=ntn[owner[i]].capx;
				ntn[owner[i]].arm[unit[i]].yloc=ntn[owner[i]].capy;
				ntn[owner[i]].arm[unit[i]].sold*=7;
				ntn[owner[i]].arm[unit[i]].sold/=10;
			} else {
				/* retreat normally and kill 20% */
				ntn[owner[i]].arm[unit[i]].sold*=8;
				ntn[owner[i]].arm[unit[i]].sold/=10;
				retreat( i );
			}
			owner[i]=(-1-owner[i]);
			continue;
		}
		else if(side[i]==ATKR){
			if ((ntn[owner[i]].arm[unit[i]].stat >= ATTACK)
			 &&(ntn[owner[i]].arm[unit[i]].stat <= SORTIE
			 ||ntn[owner[i]].arm[unit[i]].stat >= NUMSTATUS)) {
				asold += ntn[owner[i]].arm[unit[i]].sold;
			} else {
				side[i]=NTRL;
			}
			/* sortie 20% bonus in odds */
			if(ntn[owner[i]].arm[unit[i]].stat==SORTIE)
				asold += ntn[owner[i]].arm[unit[i]].sold/5;
		} else if(side[i]==DFND){
			if(ntn[owner[i]].arm[unit[i]].stat!=RULE) {
				dsold += ntn[owner[i]].arm[unit[i]].sold;
			}
		}
		if((magic(owner[i],VAMPIRE)==TRUE) &&
		(ntn[owner[i]].arm[unit[i]].unittyp==A_ZOMBIE)) nvamps++;
	}

	if(asold<=0) {
		printf("\nCombat aborted due to lack of attackers.\n");
		return;
	}
	if( asold > dsold*100)		odds=10000;
	else if( dsold > asold*100 )	odds=1;
	else	odds = (asold*100)/dsold;

	/* mercenaries/orcs/goblins might run away */
	for(i=0;i<count;i++) if(owner[i]>(-1)) {
		if(((( odds > 200 )&&(side[i]==DFND))
		||(( odds < 100 )&&(side[i]==ATKR)))
		&&((ntn[owner[i]].arm[unit[i]].unittyp == A_MERCENARY)
		  ||(ntn[owner[i]].arm[unit[i]].unittyp == A_ORC)
		  ||(ntn[owner[i]].arm[unit[i]].unittyp == A_GOBLIN))
		&&(ntn[owner[i]].arm[unit[i]].stat < NUMSTATUS)
		&&( rand()%100<30 )) {
			if( ispc(ntn[owner[i]].active)) {
				if(mailopen( owner[i] )!=(-1)) {
				fprintf(fm,"Message to %s from Conquer\n\n",ntn[owner[i]].name);
				fprintf(fm,"  Your %s Army %d Runs Away\n",
				  unittype[ntn[owner[i]].arm[unit[i]].unittyp],
				  unit[i]);
				mailclose(owner[i]);
				}
			}
			retreatside = side[i];
			if( side[i] == ATKR ) asold-= troops[i];
			if( side[i] == DFND ) dsold-= troops[i];
			fdxyretreat();
			if((retreatx==xspot)&&(retreaty==yspot)){
				/* move to capitol & kill 75% */
				ntn[owner[i]].arm[unit[i]].xloc=ntn[owner[i]].capx;
				ntn[owner[i]].arm[unit[i]].yloc=ntn[owner[i]].capy;
				ntn[owner[i]].arm[unit[i]].sold/=4;
			} else {
				/* retreat normally and kill 50% */
				ntn[owner[i]].arm[unit[i]].sold/=2;
				retreat( i );
			}
			owner[i]=(-1-owner[i]);
			continue;
		}
	}
	retreatside=0;
	if( asold<=0 ) {
		printf("Exit from battle due to lack of attackers\n");
		return;
	}

	/* CALCULATE AVERAGE COMBAT BONUS */
	abonus=0;
	dbonus=0;
	for(i=0;i<count;i++) if(owner[i]>(-1)) {
		if(side[i]==ATKR)
			abonus += cbonus(i)*troops[i];
		else if(side[i]==DFND && ntn[owner[i]].arm[unit[i]].stat!=RULE)
			dbonus += cbonus(i)*troops[i];
	}

	/*archer bonus if not in fort vs knights/cavalry*/
	j=0;
	k=0;
	for(i=0;i<count;i++) if(owner[i]>(-1)) {
		if(ISCITY(sct[xspot][yspot].designation)){
			if((ntn[owner[i]].arm[unit[i]].unittyp == A_CAVALRY)
			||(ntn[owner[i]].arm[unit[i]].unittyp == A_KNIGHT)) {
				if(side[i]==ATKR) j+=troops[i];
				else if(side[i]==DFND) k+=troops[i];
			}
		}
	}

	for(i=0;i<count;i++) if(owner[i]>(-1)) {
		if(j>0) abonus += (15 * j * troops[i]) / asold;
		if(k>0 && dsold>0) dbonus += (15 * k * troops[i]) / dsold;
	}

	abonus/=asold;
	if (dsold>0) dbonus/=dsold;

	/*CALCULATED BONUSES TO WHOLE COMBAT*/
	for(i=0;i<count;i++) if(owner[i]>(-1)) {
		if(fort_val(&sct[xspot][yspot]) != 0){
			/*Catapults add +1%/20 men defending castle (max +10%)*/
			if((ntn[owner[i]].arm[unit[i]].unittyp == A_CATAPULT)
			&&(side[i]==DFND))
				dbonus += max((troops[i]/20),10);
			/*Catapults add +1%/40 men attacking castle (max +10%)*/
			else if((ntn[owner[i]].arm[unit[i]].unittyp == A_CATAPULT)
			&&(side[i]==ATKR)) {
				strength = max((troops[i]/40),10);
				abonus += strength;
				/* possible damage 20% chance */
				if(rand()%100<2*strength) {
					fortdam=TRUE;
					sct[xspot][yspot].fortress--;
					if(sct[xspot][yspot].fortress == 0)
					sct[xspot][yspot].designation = DRUIN;
				}
			}
			/*Siege_engines add +1%/20 men when attacking fortress*/
			else if((ntn[owner[i]].arm[unit[i]].unittyp == A_SIEGE)
			&&(side[i]==ATKR)) {
				strength = max((troops[i]/20),30);
				abonus += strength;
				/* possible damage 15% chance */
				if(rand()%100<strength/2) {
					fortdam=TRUE;
					sct[xspot][yspot].fortress--;
					if(sct[xspot][yspot].fortress == 0)
						sct[xspot][yspot].designation = DRUIN;
				}
			}
		} else {
			/*Catapults add +1%/40 men normal combat (max +10%)*/
			if(ntn[owner[i]].arm[unit[i]].unittyp == A_CATAPULT)
				abonus+=max((troops[i]/40),10);
		}
	}

	/*RUN COMBAT */
	/*FIRST GIVE RANDOM ROLL FROM 0 to 100 */
	/*WITH A PROBABILITY BELL CURVE */
	/* high roll favors attacker [ 5 d21 - 5 ] */
	roll = 0;
	for(i=0;i<5;i++) {
		roll += rand()%21+1;
	}
	roll -= 5;

	/*find relative strength of troops*/
	astr = asold * (100 + abonus);
	dstr = dsold * (100 + dbonus);

	/*Recalculate odds based on quality of troops*/
	if( astr > dstr*100)		odds=10000;
	else if( dstr > astr*100 )	odds=1;
	else	odds = (astr*100)/dstr;

	/* calculate loss for an even battle */
	PDloss = MAXLOSS * roll / 100;
	PAloss = MAXLOSS * (100 - roll) / 100;

	/* adjust for odds */
	if( odds == 1 ) {
		PDloss=0;
		PAloss=200;
	} else if( odds == 10000 ) {
		PAloss=0;
		PDloss=200;
	} else if(odds > 100) {
		PDloss += (odds / 12 - 8);	/* 8.33% for higher odds */
		PAloss -= (odds / 16 - 6);	/* 6.25% for lower odds */
		if(PAloss<(100-roll)/20)
			PAloss=(100-roll)/20;	/* can't get too small */
	} else {
		PAloss += ( 800 / odds - 8);	/* 8% for higher odds */
		PDloss -= ( 600 / odds - 6);	/* 6% for lower odds */
		if(PDloss<roll/20)
			PDloss = roll/20;	/* can't get too small */
	}

	if( fort_val(&sct[xspot][yspot]) > 0 ){
		PDloss *= 120;
		PAloss *= 120;
		PDloss /= 100;
		PAloss /= 100;
	}

	retreatside = 0;

	if((PDloss > 2* PAloss)
	&&(odds>150)
	&&(((PDloss>=50)&&(rand()%4==0))
	  ||(rand()%8)))	retreatside=DFND;

	if((PAloss > 2* PDloss)
	&&(odds<150)
	&&(((PAloss>=50)&&(rand()%2==0))
	  ||(rand()%6)))	retreatside=ATKR;

	if(retreatside!=0) {
		fdxyretreat();
		/* no legal retreat route */
		if((retreatside!=0) && (retreatx== xspot)
		&& (retreaty== yspot)){
			if(retreatside==ATKR) PAloss+=15;
			else if(retreatside==DFND) PDloss+=15;
#ifdef DEBUG
			printf("side %d (%d %d) can't retreat...+15%% loss\n",retreatside,retreatx,retreaty);
#endif /* DEBUG */
			retreatside = 0;
		}
#ifdef DEBUG
		else printf("retreat side %d to %d %d\n",retreatside,retreatx,retreaty);
#endif /* DEBUG */
	}

	if(PAloss>100) PAloss = 100;
	if(PDloss>100) PDloss = 100;

	Aloss = Dloss = 0;
	for(i=0;i<count;i++) if(owner[i]>(-1)){
		if(side[i]==ATKR){
			if( ntn[owner[i]].arm[unit[i]].unittyp >= MINLEADER) {
				if((rand()%100) < PAloss){ /* kill it */
					for(j=0;j<MAXARM;j++)
					if(ntn[owner[i]].arm[j].stat==unit[i]+NUMSTATUS)
					ntn[owner[i]].arm[j].stat=ATTACK;
					Aloss += troops[i];
					ntn[owner[i]].arm[unit[i]].sold=0;
				}
			} else {
				loss=(troops[i]*PAloss)/100;
				/*archers/catapults on sortie take 1/4 damage*/
				if((ntn[owner[i]].arm[unit[i]].stat==SORTIE)
				&&(fort_val(&sct[xspot][yspot]) > 0)
				&&(sct[xspot][yspot].owner==country)
				&&((ntn[owner[i]].arm[unit[i]].unittyp==A_ARCHER)
				||(ntn[owner[i]].arm[unit[i]].unittyp==A_CATAPULT)))
					loss /= 4;
				/*army can't have less than 25 men in it*/
				if(troops[i]-loss<25)
					loss=troops[i];
if( loss>troops[i] ) {
printf("I AM VERY CONFUSED - PLEASE HELP... combat.c\n");
}
				Aloss+=loss;
				ntn[owner[i]].arm[unit[i]].sold-=loss;
				if((ntn[owner[i]].arm[unit[i]].unittyp==A_MILITIA)&&(retreatside==ATKR)) {
					sct[ntn[owner[i]].arm[unit[i]].xloc][ntn[owner[i]].arm[unit[i]].yloc].people += ntn[owner[i]].arm[unit[i]].sold;
					ntn[owner[i]].arm[unit[i]].sold=0;
				}
			}
		} else if(side[i]==DFND){
			if( ntn[owner[i]].arm[unit[i]].unittyp >= MINLEADER) {
				if((ntn[owner[i]].arm[unit[i]].stat!=RULE
				||PDloss>=80)&&((rand()%100) < PDloss)){ /* kill it */
					for(j=0;j<MAXARM;j++)
					if(ntn[owner[i]].arm[j].stat==unit[i]+NUMSTATUS)
					ntn[owner[i]].arm[j].stat=ATTACK;
					Dloss +=troops[i];
					ntn[owner[i]].arm[unit[i]].sold=0;
				}
			} else {
				loss=(troops[i]*PDloss)/100;
				/*destroy army if < 25 men*/
				if(troops[i]-loss<25)
					loss=troops[i];
				Dloss+=loss;
				ntn[owner[i]].arm[unit[i]].sold-=loss;
				if((ntn[owner[i]].arm[unit[i]].unittyp==A_MILITIA)&&(retreatside==DFND)) {
					sct[ntn[owner[i]].arm[unit[i]].xloc][ntn[owner[i]].arm[unit[i]].yloc].people += ntn[owner[i]].arm[unit[i]].sold;
					ntn[owner[i]].arm[unit[i]].sold=0;
				}
			}
		}
		/* non-vampire troops are sucked in by vampires */
		if((nvamps>0)&&(magic(owner[i],VAMPIRE)==FALSE)
		&&(ntn[owner[i]].arm[unit[i]].unittyp!=A_ZOMBIE)
		&&(ntn[owner[i]].arm[unit[i]].unittyp<MINLEADER))
			vampire+= loss / 3;
	}
	/* use k variable to hold length */
#ifdef HIDELOC
	if( isntn( ntn[sct[xspot][yspot].owner].active )) {
	fprintf(fnews,"4.\tBattle occurs in %s", ntn[sct[xspot][yspot].owner].name);
	k = 27+strlen(ntn[sct[xspot][yspot].owner].name);
	} else {
	fprintf(fnews,"4.\tBattle on unowned land");
	k = 30;
	}
#else
	fprintf(fnews,"4.\tBattle in %d,%d",xspot,yspot);
	k = 25;
#endif
	for(j=0;j<count;j++) if(UOWNER(j)>(-1)){
		done=FALSE;
		for(i=0;i<j;i++) if(UOWNER(j)==UOWNER(i)) done=TRUE;
		if(done==FALSE) {
			loss=NTRL;
			for(i=j;(loss==NTRL||loss==WIMP) && i<count;i++)
				if(UOWNER(i)==UOWNER(j)) {
					if(owner[i]<(-1)) loss=WIMP;
					else loss=side[i];
				}
			if(loss!=NTRL) {
				k += 11 + strlen(ntn[UOWNER(j)].name);
				if(loss==WIMP) k++;
				if(k>79) {
					k = 30;
					fprintf(fnews,",\n4.\t  ");
				} else fprintf(fnews,", ");
				if(loss==ATKR)
					fprintf(fnews,"attacker %s",ntn[UOWNER(j)].name);
				else if(loss==DFND)
					fprintf(fnews,"defender %s",ntn[UOWNER(j)].name);
				else if(loss==WIMP)
					fprintf(fnews,"retreater %s",ntn[UOWNER(j)].name);
			}
		}
	}
	fprintf(fnews,"\n");
	if(nvamps>0){
		for(i=0;i<count;i++) if(owner[i]>(-1)){
			if((magic(owner[i],VAMPIRE)==TRUE)
			&&(ntn[owner[i]].arm[unit[i]].unittyp==A_ZOMBIE)
			&&(ntn[owner[i]].arm[unit[i]].sold > 0))
				ntn[owner[i]].arm[unit[i]].sold+=vampire/nvamps;
		}
	}

	/*who is in the battle; but don't send to scared armies */
	for(j=0;j<count;j++) if(owner[j]>(-1)){
		done=FALSE;

		/*first time your nation appears done=FALSE*/
		for(i=0;i<j;i++) if(owner[j]==owner[i]) done=TRUE;

		if((done==FALSE)&&(ispc(ntn[owner[j]].active))) {

			loss=NTRL;
			for(i=j;loss==NTRL && i<count;i++)
				loss=side[i];
			if (mailopen( owner[j] )==(-1)) continue;

			fprintf(fm,"BATTLE SUMMARY for sector %d, %d\n",xspot,yspot);
			fprintf(fm,"Battle occured during %s of Year %d\n",PSEASON(TURN),YEAR(TURN));

			if(loss==ATKR)
				fprintf(fm,"You are on the Attacking Side\n");
			else if(loss==DFND)
				fprintf(fm,"You are on the Defending Side\n");
			else	fprintf(fm,"You are Neutral\n");

			/*detail all participants in battle*/
			for(k=0;k<count;k++) if(owner[k]!=(-1)){
				fprintf(fm," %s ",ntn[UOWNER(k)].name);
				if(owner[k]<(-1))
					fprintf(fm,"chickens out: ");
				else if(side[k]==DFND
				&& ntn[owner[k]].arm[unit[k]].stat!=RULE)
					fprintf(fm,"defending: ");
				else if(side[k]==ATKR)
					fprintf(fm,"attacking: ");
				else if(side[k]==NTRL
				|| (side[k]==DFND
				    && ntn[owner[k]].arm[unit[k]].stat==RULE))
					fprintf(fm,"neutral: ");
				else
					fprintf(fm,"in limbo: ");
			
				fprintf(fm,"army %d (%s, men %ld, bonus=%d, loss=%ld)",
					unit[k],
					unittype[ntn[UOWNER(k)].arm[unit[k]].unittyp%UTYPE],
					troops[k],
					cbonus(k),
					troops[k]-ntn[UOWNER(k)].arm[unit[k]].sold);
				if((ntn[UOWNER(k)].arm[unit[k]].unittyp >= MINLEADER)
				&&( ntn[UOWNER(k)].arm[unit[k]].sold == 0))
					fprintf(fm," (killed)\n");
				else if((ntn[UOWNER(k)].arm[unit[k]].unittyp == A_MILITIA)
				&&( ntn[UOWNER(k)].arm[unit[k]].sold == 0))
				fprintf(fm," (disbanded)\n");
				else fputc('\n',fm);
			}

			fprintf(fm,"attacking soldiers=%ld -> percent loss %d%%\n",asold,PAloss);
			fprintf(fm,"defending soldiers=%ld -> percent loss %d%%\n",dsold,PDloss);
			fprintf(fm,"ODDS=%d => adjusted to %d to 100; Die Roll is %d\n",odds*(100+dbonus)/(100+abonus),odds,roll);
			fprintf(fm,"RESULT: Attackers lose %ld men, Defenders lose %ld men\n",Aloss, Dloss);
			if(fortdam==TRUE) fprintf(fm,"Fortifications damaged during the attack\n");
			if(retreatside==ATKR){
			if(Aloss<asold)
			fprintf(fm,"Additionally, All attackers retreat to %d %d\n",retreatx,retreaty);
			} else if(retreatside==DFND){
			if(Dloss<dsold)
			fprintf(fm,"Additionally, All defenders retreat to %d %d\n",retreatx,retreaty);
			}
			mailclose(owner[j]);
		}
	}
	retreat( -1 );
}

/*
 * cbonus - Calculate Combat Bonus for Individual Unit
 *
 * Computes the total combat effectiveness modifier for a specific unit
 * based on terrain, unit type, status, fortifications, and magical effects.
 * This bonus is applied to the unit's base strength during battle resolution.
 *
 * BONUS CALCULATION FACTORS:
 *
 * Terrain Effects (Defenders):
 * - Mountain terrain: +20% bonus
 * - Hill terrain: +10% bonus
 * - Jungle vegetation: +20% bonus
 * - Forest vegetation: +15% bonus
 * - Wood vegetation: +10% bonus
 *
 * Unit Status Modifiers:
 * - MARCH status: -40% penalty (units in movement)
 * - MAGDEF status: +30% bonus (magical defense)
 * - MAGATT status: +30% bonus (magical attack)
 * - SORTIE status: Variable based on fortification and unit type
 * - SIEGED status: -20% penalty
 * - Army groups (>=NUMSTATUS): +20% bonus
 *
 * Fortification Effects:
 * - Defending in owned fortification: Full fort value bonus
 * - Zombie units: Half fort value (poor wall utilization)
 * - Cavalry/Knights in forts: -20% penalty
 * - Archers in owned forts: +15% bonus
 * - Archers in enemy forts: +5% bonus
 * - Sapper magic vs fortifications: +10% attack bonus
 *
 * Unit Type Specializations:
 * - Base attack/defense values from unit type tables
 * - Phalanx/Legion formations: Size-based bonuses
 * - Mercenary units: Special attack/defense modifiers
 * - Monster units: Fear bonuses in sorties
 * - Mounted units: Sortie bonuses for mobility
 *
 * Magic System Integration:
 * - DESTROYER/DERVISH powers in ice/desert: +30% bonus
 * - VAMPIRE effects handled in main battle resolution
 * - SAPPER abilities for siege warfare
 * - SAILOR bonuses for naval unit effectiveness
 *
 * SORTIE SPECIAL MECHANICS:
 * When attacking from owned fortification (sortie):
 * - Base +10% bonus for organized attack
 * - Dragoon/Legion/Phalanx: +5% organization bonus
 * - Light Cavalry/Cavalry: +10% mounted bonus
 * - Avian/Elephant/Knight: +15% elite mounted bonus
 * - Monsters: +5% fear factor bonus
 *
 * Parameters:
 *   num - Index into global unit arrays for target unit
 *
 * Returns:
 *   Integer percentage bonus to apply to unit strength
 *   Positive values increase effectiveness, negative reduce it
 *
 * Side Effects:
 * - Accesses global combat state arrays (unit, owner, side)
 * - Reads terrain and fortification data from sector
 * - Queries magic system for special abilities
 * - May access nation statistics for mercenary modifiers
 *
 * Testing Notes:
 * Category: A (Unit) - Pure calculation function with clear inputs/outputs
 * Approach: Unit tests with various terrain/unit/status combinations
 * Key Tests:
 *   - Terrain bonus calculations for all terrain types
 *   - Unit type attack/defense value lookups
 *   - Fortification bonus calculations
 *   - Magic system integration
 *   - Status modifier applications
 *   - Sortie bonus calculations
 * Dependencies: Global arrays, terrain data, magic system
 * Mock Requirements: Unit data, terrain sectors, magic abilities
 * Complexity: Moderate - Multiple conditional calculations with table lookups
 *
 * Notes:
 * - Critical for combat balance and tactical depth
 * - Historical balance values preserved for compatibility
 * - Consider caching results for performance in large battles
 * - Unit type arrays must be properly initialized
 */
int
cbonus(int num)
{
	short	armynum;
	int	armbonus;

	armbonus=0;
	armynum=unit[num];
	country=UOWNER(num);

	/*Racial combat bonus due to terrain (the faster you move the better)*/
	armbonus+=5*(9-movecost[xspot][yspot]);	/* this line always has */
				/* the same result... must fix -- ADB */

	if(((magic(country,DESTROYER)==1)
	||(magic(country,DERVISH)==1))
	&&((sct[xspot][yspot].vegetation==ICE)
	||(sct[xspot][yspot].vegetation==DESERT)))
		armbonus+=30;

	if(ASTAT>=NUMSTATUS) armbonus+=20;	/* army group */
	if(side[num]==DFND){

		if(sct[xspot][yspot].altitude==MOUNTAIN) armbonus+=20;
		else if(sct[xspot][yspot].altitude==HILL) armbonus+=10;

		if(sct[xspot][yspot].vegetation==JUNGLE) armbonus+=20;
		else if(sct[xspot][yspot].vegetation==FOREST) armbonus+=15;
		else if(sct[xspot][yspot].vegetation==WOOD) armbonus+=10;

		if(ATYPE==A_MERCENARY) armbonus += MERCDEF;
		else armbonus += ntn[UOWNER(num)].dplus;

		if(ASTAT==MAGDEF) armbonus+=30;
		else if(ASTAT==SORTIE) armbonus-=30;
		else if(ASTAT==SIEGED) armbonus-=20;

		if((sct[xspot][yspot].owner==country)
		&&(ASTAT==GARRISON||ASTAT==MILITIA||ASTAT==SIEGED)){
			if(ATYPE == A_ZOMBIE) /* don't utilize walls well */
				armbonus += fort_val(&sct[xspot][yspot])/2;
			else armbonus += fort_val(&sct[xspot][yspot]);
		}
	}
	else if(side[num]==ATKR) {
		if( (fort_val(&sct[xspot][yspot]) > 0)
		&&( magic(country,SAPPER)==TRUE)) armbonus += 10;

		if(ATYPE == A_MERCENARY) armbonus += MERCATT;
		else armbonus += ntn[UOWNER(num)].aplus;
		if(ASTAT==MAGATT) armbonus += 30;
		if(ASTAT==SORTIE && (fort_val(&sct[xspot][yspot]) > 0)
		&& sct[xspot][yspot].owner==country) {
			armbonus += 10;
			if((ATYPE==A_DRAGOON)||(ATYPE==A_LEGION)
			||(ATYPE==A_PHALANX)) {
				/* bonus for organization or riding cavalry */
				armbonus += 5;
			} else if ((ATYPE==A_LT_CAV)||(ATYPE==A_CAVALRY)) {
				/* bonus for mounted sortie */
				armbonus += 10;
			} else if (avian(ATYPE)||ATYPE==A_ELEPHANT||ATYPE==A_KNIGHT) {
				/* bonus for mounted or flying sortie */
				armbonus += 15;
			}
			if ((ATYPE>=MINMONSTER)||(ATYPE<=MAXMONSTER)) {
				/* bonus for monsters (scare factor) */
				armbonus += 5;
			}
		}
	}

	/*army status is important*/
	if(ASTAT==MARCH) armbonus-=40;

	/*if a fortress*/
	if(fort_val(&sct[xspot][yspot]) > 0){
		/*Cavalry and Knights get -20%*/
		if((ATYPE == A_CAVALRY) ||(ATYPE == A_KNIGHT)) armbonus -= 20;
		/*Archers gain pluses*/
		else if((ATYPE == A_ARCHER)&&(sct[xspot][yspot].owner==country))
			armbonus += 15;
		else if(ATYPE == A_ARCHER) armbonus += 5;
	}

	if(side[num]==ATKR) armbonus+= *(unitattack+(ATYPE%UTYPE));
	else armbonus+= *(unitdefend+(ATYPE%UTYPE));

	/*Phalanx and Legionaires need certain numbers of troops*/
	if((ATYPE==A_PHALANX)||(ATYPE==A_LEGION)) {
 		if(ASOLD>1000){	armbonus+=20;
 		} else if(ASOLD>500) armbonus+=10;
	}

	return(armbonus);
}

/*
 * fdxyretreat - Calculate optimal retreat destination for units in combat
 *
 * Determines the safest adjacent sector for retreating units during combat.
 * Uses a priority system to find the best available retreat location:
 * 1. Friendly sectors owned by retreating nation
 * 2. Neutral sectors with friendly diplomatic status
 * 3. Unoccupied sectors with sufficient food capacity
 * 4. If no valid retreat found, sets retreat coordinates to current location
 *    (forcing retreat to nation's capital with heavy casualties)
 *
 * Algorithm:
 * - First checks if current sector is a town/city/capital (no retreat needed)
 * - Determines retreating nation based on retreat side (attacker/defender)
 * - Scans all 8 adjacent sectors in systematic grid pattern
 * - For each adjacent sector, validates:
 *   * Food capacity sufficient for retreating units
 *   * Diplomatic relations allow safe passage
 *   * No hostile forces present to block retreat
 * - Returns first valid retreat location found, or current location if none
 *
 * Parameters:
 *   None (uses global combat state variables)
 *
 * Returns:
 *   void (sets global retreatx, retreaty coordinates)
 *
 * Side Effects:
 *   - Sets retreatx, retreaty global variables to retreat destination
 *   - If retreatside is 0, clears retreat entirely and returns immediately
 *   - May set retreat destination to current location if no valid retreat
 *
 * Global Variables Used:
 *   - retreatside: Which side is retreating (ATKR/DFND/0=none)
 *   - xspot, yspot: Current battle location coordinates
 *   - retreatx, retreaty: Calculated retreat destination (output)
 *   - anation, dnation: Attacking and defending nation IDs
 *   - sct[][]: Global sector data for terrain and ownership
 *   - ntn[]: Global nation data for diplomatic status
 *   - country: Current player nation ID for food calculations
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires map state, diplomatic relations
 *   Approach: Integration testing with various map configurations
 *   Key Tests: Adjacent friendly sectors, blocked retreats, food capacity
 *   Dependencies: Global map state, nation diplomatic status, food system
 *   Mock Requirements: Map sectors, diplomatic relationships, food calculations
 *   Complexity: Moderate - Grid scanning with multiple validation criteria
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to global variable dependencies
 *   - Performance: O(1) complexity - always scans exactly 8 adjacent sectors
 *   - Historical context: Part of original retreat mechanics system
 *   - Retreat failure handling: Forces retreat to capital with 30-75% casualties
 *   - Diplomatic integration: Respects alliance/war status for safe passage
 *   - Town/city sectors block retreats (defensive advantage mechanic)
 */
void 
fdxyretreat (void)	/* finds retreat location */
{
	int	x,y,nation=(-1);
	int	xsctr= xspot;
	int	ysctr= yspot;

	retreatx=xsctr;
	retreaty=ysctr;

	if((sct[xsctr][ysctr].designation==DTOWN)
	||(sct[xsctr][ysctr].designation==DCAPITOL)
	||(sct[xsctr][ysctr].designation==DCITY)){
		retreatside=0;
		return;
	}

	if(retreatside == ATKR) nation=anation;
	else nation=dnation;

	for(x= xsctr-1; x<=xsctr+1; x++)
	for(y= ysctr-1; y<=ysctr+1; y++) if(ONMAP(x,y)){
		if(tofood( &sct[x][y],
			sct[x][y].owner == country ? country : 0)==0) continue;
		if(((sct[x][y].owner == nation)
		   ||(ntn[sct[x][y].owner].dstatus[nation] < NEUTRAL))
		||(solds_in_sector( x, y, sct[x][y].owner) == 0)){
			retreatx=x;
			retreaty=y;
#ifdef DEBUG
			printf("armies in %d %d retreat to %d %d\n",xsctr,ysctr,x,y);
#endif /* DEBUG */
			return;
		}
	}
}

/*
 * retreat - Execute unit retreats from combat to predetermined destination
 *
 * Moves retreating units from the battle sector to the destination calculated
 * by fdxyretreat(). Handles two retreat modes: mass retreat of all units on
 * retreating side, or selective retreat of a single unit (typically mercenaries
 * who refused to fight). Applies different casualty rates based on unit type.
 *
 * Algorithm:
 * - Early exit if no retreat is occurring (retreatside == 0)
 * - Iterate through all units in the battle
 * - For mass retreat (unitnum == -1): process all units on retreating side
 * - For selective retreat: process only the specified unit
 * - Apply unit-specific retreat handling:
 *   * Naval units (marines/sailors): Suffer 15% casualties, remain in place
 *   * Land units: Move to retreat coordinates without additional casualties
 * - For selective retreat, exit immediately after processing target unit
 *
 * Parameters:
 *   unitnum - Unit index for selective retreat, or -1 for mass retreat
 *            -1: Retreat all units on the retreating side (normal battle retreat)
 *            >=0: Retreat only the specified unit (mercenary refusal/rout)
 *
 * Returns:
 *   void (modifies unit locations and troop counts directly)
 *
 * Side Effects:
 *   - Moves land units to retreat coordinates (retreatx, retreaty)
 *   - Reduces naval unit troop strength by 15% (represents evacuation losses)
 *   - Naval units remain at battle location (cannot retreat overland)
 *   - No additional side effects for units not on retreating side
 *
 * Global Variables Used:
 *   - retreatside: Which side is retreating (ATKR/DFND/0=none)
 *   - retreatx, retreaty: Destination coordinates set by fdxyretreat()
 *   - count: Number of units in current battle
 *   - owner[]: Nation ownership of each unit in battle
 *   - unit[]: Unit array indices for each combatant
 *   - side[]: Combat side assignment for each unit
 *   - ntn[]: Global nation data containing army information
 *
 * Testing Notes:
 *   Category: A (Unit) - Clear input/output with predictable behavior
 *   Approach: Unit testing with mock army data and battle state
 *   Key Tests: Mass retreat, selective retreat, naval unit handling, no-retreat case
 *   Dependencies: Global army data, battle state variables
 *   Mock Requirements: Army locations, unit types, battle participants
 *   Complexity: Simple - Straightforward iteration with clear logic branches
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to global variable dependencies
 *   - Performance: O(n) where n = number of units in battle (typically small)
 *   - Historical context: Supports both voluntary and involuntary retreats
 *   - Naval retreat limitation: Represents inability to retreat ships overland
 *   - Casualty asymmetry: Naval units suffer retreat losses, land units don't
 *   - Used for: Battle retreats, mercenary desertion, diplomatic withdrawals
 */
void 
retreat (
    int unitnum	/* if -1 then normal, else retreat only unit ismerc */
)
{
	int cnum;

	if(retreatside == 0) return;

	for(cnum=0;cnum<count;cnum++) if(owner[cnum]>(-1)){
		if( unitnum != (-1) ) cnum=unitnum;
		if(side[cnum] == retreatside){
			if ((ntn[owner[cnum]].arm[unit[cnum]].unittyp==A_MARINES)||
			    (ntn[owner[cnum]].arm[unit[cnum]].unittyp==A_SAILOR)){
				ntn[owner[cnum]].arm[unit[cnum]].sold *= 85;
				ntn[owner[cnum]].arm[unit[cnum]].sold /= 100;
			} else {
				ntn[owner[cnum]].arm[unit[cnum]].xloc = retreatx;
				ntn[owner[cnum]].arm[unit[cnum]].yloc = retreaty;
			}
		}
		if( unitnum != (-1) ) return;
	}
}


/*
 * navalcbt - Execute complete naval combat resolution for all fleets in sector
 *
 * Implements the comprehensive naval battle system, handling multiple fleet types
 * (warships, galleys, merchants) with complex combat mechanics including capture,
 * sinking, and crew casualties. This is the naval equivalent of the land-based
 * fight() function, featuring detailed ship-by-ship resolution and diplomatic
 * side assignment.
 *
 * Algorithm Overview:
 * 1. Diplomatic Side Assignment: Determine attacker/defender/neutral based on
 *    nation relationships (war, jihad, treaty, alliance status)
 * 2. Combat Strength Calculation: Calculate crew effectiveness by ship type:
 *    - Warship crew: 1.0x base strength
 *    - Galley crew: 2.0x base strength
 *    - Merchant crew: 4.0x base strength
 *    - Embarked armies: Variable multipliers (Marines 3x, Sailors/Archers 1.5x)
 *    - Sailor magic bonus: +25% effectiveness for all crew types
 * 3. Combat Resolution: Bell curve dice mechanics with odds-based modifiers
 * 4. Loss Distribution: Ship-by-ship resolution with capture vs. sinking
 * 5. Crew Casualties: Proportional losses based on combat outcome
 * 6. Battle Reporting: Detailed mail reports to all participating nations
 *
 * Ship Combat Mechanics:
 * - Three ship classes: Light, Medium, Heavy (different combat values)
 * - Three ship types: Warships (combat), Galleys (hybrid), Merchants (transport)
 * - Capture probability based on relative fleet strength and ship type
 * - Warships have higher combat effectiveness but lower capture resistance
 * - Galleys can carry armies and provide combined arms bonuses
 * - Merchants are vulnerable but valuable capture targets
 *
 * Capture vs. Sinking System:
 * - Capture rates vary by ship type and relative strength
 * - Captured ships join victor's fleet under new ownership
 * - Sunk ships are completely destroyed with crew losses
 * - Capture probability: (enemy_strength / total_strength) * base_rate
 *
 * Parameters:
 *   None (uses global battle state from combat() setup)
 *
 * Returns:
 *   void (modifies fleet compositions and sends mail reports)
 *
 * Side Effects:
 *   - Modifies ship counts in participating nation fleets
 *   - Reduces crew numbers based on combat casualties
 *   - Transfers captured ships between nations
 *   - Sends detailed battle reports via mail system
 *   - Updates global combat statistics for session
 *   - May trigger army casualties for embarked forces
 *
 * Global Variables Used:
 *   - count: Number of participating fleets in battle
 *   - owner[], unit[], side[]: Fleet ownership, unit IDs, combat sides
 *   - anation, dnation: Primary attacking and defending nations
 *   - xspot, yspot: Battle location coordinates
 *   - ntn[]: Global nation data (fleets, armies, diplomatic status)
 *   - country, curntn: Current nation context for processing
 *   - TURN: Current game turn for battle timestamping
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires multiple fleets and diplomatic setup
 *   Approach: Integration testing with various fleet compositions
 *   Key Tests: Multi-nation battles, ship captures, crew calculations, magic effects
 *   Dependencies: Fleet data, diplomatic relationships, mail system, magic system
 *   Mock Requirements: Fleet compositions, nation relationships, battle scenarios
 *   Complexity: Complex - Multi-phase naval combat with extensive calculations
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to global variable dependencies
 *   - Performance: O(n*m) where n=fleets, m=average ships per fleet
 *   - Historical context: Sophisticated naval warfare simulation
 *   - Diplomatic integration: Respects complex alliance/war relationships
 *   - Magic system: Sailor magic provides significant combat advantages
 *   - Capture mechanics: Realistic ship capture vs. destruction ratios
 *   - Mail integration: Comprehensive battle reporting to all participants
 *   - Ship type balance: Each ship type has distinct tactical role
 *   - Combined arms: Army-navy cooperation through embarked forces
 */

/*SUBROUTINE TO RUN NAVAL COMBAT ON ALL SHIPS */
/* quick define for easier reading */
#define QWAR 1
#define QGAL 2
#define QMER 3
/* just like fight, this takes array of owner,side,unit and calculates */
/* a random battle based on the strengths of the combatants.           */
void 
navalcbt (void)
{
	int acrew=0,dcrew=0;	/*a's and d's crew and soldier strength*/
	int ahold=0,dhold=0;	/*a's and d's warship strength*/
	int awsunk=0,dwsunk=0;	/*a's and d's warship losses for the round*/
	int agsunk=0,dgsunk=0;	/*a's and d's galley losses for the round*/
	int amsunk=0,dmsunk=0;	/*a's and d's merchent losses for the round*/
	int awcapt=0,dwcapt=0;	/*a's and d's warship captures for the round*/
	int agcapt=0,dgcapt=0;	/*a's and d's galley captures for the round*/
	int amcapt=0,dmcapt=0;	/*a's and d's merchant captures for the round*/
	int akcrew=0,dkcrew=0;	/*a's and d's crew losses for the round*/
	char wnum[MGKNUM],gnum[MGKNUM],mnum[MGKNUM];
	register int done,i,j,k;
	int roll,odds,savecntry=country;
	int PAloss, PDloss, Ploss, which, shipsize;
	int thold, ghold, nvynum, armynum;
	int dcptpct, acptpct, cptpct;
	struct s_nation *saventn=curntn;

	printf("In Naval Combat....\n");

	/* determine who is attacker & who is on defenders side?*/
	for(j=0;j<count;j++) if(owner[j]!=(-1)){
		if(owner[j]==anation) side[j]=ATKR;
		else if(ntn[anation].dstatus[owner[j]]==JIHAD) side[j]=DFND;
		else if(ntn[owner[j]].dstatus[anation]==JIHAD) side[j]=DFND;
		else if(ntn[anation].dstatus[owner[j]]==WAR)   side[j]=DFND;
		else if(ntn[owner[j]].dstatus[anation]==WAR)   side[j]=DFND;
		else if((ntn[owner[j]].dstatus[anation]==TREATY)
		&&(ntn[owner[j]].dstatus[dnation]>HOSTILE)) side[j]=ATKR;
		else if((ntn[owner[j]].dstatus[anation]==ALLIED)
		&&(ntn[owner[j]].dstatus[dnation]>HOSTILE)) side[j]=ATKR;
	}

	/* Loop through all competitors to determine
	 *  relative combat strengths:
	 *                              men/unit strength
	 * crew on ship:
	 *     warship crew                     1
	 *     galley crew                      2
	 *     merchant crew                    4
	 * soldiers onboard:
         *     SAILOR/ARCHER                   3/4
         *     MARINE                          1/3
	 *     others                          4/3
	 */
	for(j=0;j<count;j++) if(owner[j]!=(-1)){
		curntn= &ntn[owner[j]];
		country= owner[j];
		wnum[j]=SHIPS(ntn[country].nvy[unit[j]].warships,N_LIGHT)+
			SHIPS(ntn[country].nvy[unit[j]].warships,N_MEDIUM)+
			SHIPS(ntn[country].nvy[unit[j]].warships,N_HEAVY);
		mnum[j]=SHIPS(ntn[country].nvy[unit[j]].merchant,N_LIGHT)+
			SHIPS(ntn[country].nvy[unit[j]].merchant,N_MEDIUM)+
			SHIPS(ntn[country].nvy[unit[j]].merchant,N_HEAVY);
		gnum[j]=SHIPS(ntn[country].nvy[unit[j]].galleys,N_LIGHT)+
			SHIPS(ntn[country].nvy[unit[j]].galleys,N_MEDIUM)+
			SHIPS(ntn[country].nvy[unit[j]].galleys,N_HEAVY);
		if(side[j]==DFND) {
			if((k=fltwhold(unit[j]))>0) {
				dhold += k;
				if (magic(country,SAILOR)==TRUE) {
					dcrew += 5*k*curntn->nvy[unit[j]].crew/4;
				} else dcrew += k*curntn->nvy[unit[j]].crew;
			}
			if((k=fltmhold(unit[j]))>0) {
				if (magic(country,SAILOR)==TRUE) {
					dcrew += 5*k*curntn->nvy[unit[j]].crew/16;
				} else dcrew += k*curntn->nvy[unit[j]].crew/4;
			}
			if((k=fltghold(unit[j]))>0) {
				if (magic(country,SAILOR)==TRUE) {
					dcrew += 5*k*curntn->nvy[unit[j]].crew/8;
				} else dcrew += k*curntn->nvy[unit[j]].crew/2;
				if(curntn->nvy[unit[j]].armynum!=MAXARM) {
					k = curntn->nvy[unit[j]].armynum;
					switch (curntn->arm[k].unittyp) {
					case A_ARCHER:
					case A_SAILOR:
						dcrew += 3*curntn->arm[k].sold/2;
						break;
					case A_MARINES:
						dcrew += 3*curntn->arm[k].sold;
						break;
					default:
						dcrew += 3*curntn->arm[k].sold/4;
						break;
					}
				}
			}
		} else if(side[j]==ATKR) {
			if((k=fltwhold(unit[j]))>0) {
				ahold += k;
				if (magic(country,SAILOR)==TRUE) {
					acrew += 5*k*curntn->nvy[unit[j]].crew/4;
				} else acrew += k*curntn->nvy[unit[j]].crew;
			}
			if((k=fltmhold(unit[j]))>0) {
				if (magic(country,SAILOR)==TRUE) {
					acrew += 5*k*curntn->nvy[unit[j]].crew/16;
				} else acrew += k*curntn->nvy[unit[j]].crew/4;
			}
			if((k=fltghold(unit[j]))>0) {
				if (magic(country,SAILOR)==TRUE) {
					acrew += 5*k*curntn->nvy[unit[j]].crew/8;
				} else acrew += k*curntn->nvy[unit[j]].crew/2;
				if(curntn->nvy[unit[j]].armynum!=MAXARM) {
					k = curntn->nvy[unit[j]].armynum;
					switch (curntn->arm[k].unittyp) {
					case A_ARCHER:
					case A_SAILOR:
						acrew += 3*curntn->arm[k].sold/2;
						break;
					case A_MARINES:
						acrew += 3*curntn->arm[k].sold;
						break;
					default:
						acrew += 3*curntn->arm[k].sold/4;
						break;
					}
				}
			}
		}
	}

	/*find battle odds*/
	if( acrew > dcrew*100 ) odds=10000;
	else if ( dcrew > acrew*100 ) odds=1;
	else odds = (acrew*100)/dcrew;

	/* calculate capture percentages */
	/*
	 *  This formula produces:
         *        0% capture for   1:100  odds
	 *        2% capture for   1:10   odds
         *       15% capture for   1:1    odds
	 *       60% capture for  10:1    odds
	 *      100% capture for >60:1    odds
	 *      with linear progression between each.
	 */
	if (odds>6000) {
		dcptpct=0;
		acptpct=100;
	} else if (odds>1000) {
		dcptpct= (6000-odds)/2500;
		acptpct= (odds-1000)/125+60;
	} else if (odds>100) {
		dcptpct= (1000-odds)/69+2;
		acptpct= (odds-100)/20+15;
	} else if (odds>10) {
		dcptpct= (100-odds)/2+15;
		acptpct= (odds-10)/6.9+2;
	} else if (odds>6) {
		dcptpct= (10-odds)*14+60;
		acptpct= (odds-6)/2;
	} else {
		dcptpct= 100;
		acptpct= 0;
	}
#ifdef DEBUG
	printf("capture percentage: attack = %d  defend = %d\n",acptpct,dcptpct);
#endif /* DEBUG */

	/*figure combat on a one-to-one basis */
	/* use a bell curve roll */
	roll = 0;
	for(i=0;i<5;i++) {
		roll += rand()%21+1;
	}
	roll -= 5;
	PDloss = MAXLOSS * roll / 100;
	PAloss = MAXLOSS * (100 - roll) / 100;
#ifdef DEBUG
	printf("Pdloss = %d  PAloss = %d\n",PDloss,PAloss);
#endif /* DEBUG */

	/* adjust based on the odds */
	if( odds == 1 ) {
		PDloss = 0;
		PAloss = 100;
	} else if ( odds == 10000 ) {
		PAloss = 0;
		PDloss = 100;
	} else if ( odds > 100 ) {
		PDloss += (odds / 10 - 10);		/* 10% increase */
		PAloss -= (odds / 25 - 4);		/* 4% decrease */
		if(PAloss<(100-roll)/5)
			PAloss=(100-roll)/5;
	} else {
		PAloss += ( 1000 / odds - 10);	/* 10% increase */
		PDloss -= ( 400 / odds - 4);		/* 4% decrease  */
		if(PDloss<roll/5)
			PDloss = roll/5;
	}
	if (PAloss>100) PAloss=100;
	if (PDloss>100) PDloss=100;

	/* calculate actual losses */
	for(j=0;j<count;j++) if(owner[j]!=(-1)){
		curntn= &ntn[owner[j]];
		country= owner[j];

		/* determine side */
		which=side[j];
		/* capturing is by foe so use other percent */
		if(which==ATKR) {
			cptpct=dcptpct;
			Ploss= PAloss;
		} else if(which==DFND) {
			cptpct=acptpct;
			Ploss= PDloss;
		} else continue;

		/* find all weighting info */
		nvynum = unit[j];
		thold = flthold(nvynum);
		ghold = fltghold(nvynum);

		/* go through all ships in the navy */
		for(shipsize=N_LIGHT;shipsize<=N_HEAVY;shipsize++) {
			/* check warships */
			for(i=0;i<P_NWAR(shipsize);i++)
			if (rand()%100<Ploss) {
				if(rand()%100<cptpct/2) {
					/* capture a ship */
					if(which==ATKR){
						if (dhold) {
							awcapt++;
							ahold-=(shipsize+1);
							capture(QWAR,DFND,shipsize,rand()%dhold+1);
							NSUB_WAR(1);
						}
					} else {
						if (ahold) {
							dwcapt++;
							dhold-=(shipsize+1);
							capture(QWAR,ATKR,shipsize,rand()%ahold+1);
							NSUB_WAR(1);
						}
					}
					if(which==ATKR) akcrew += P_NCREW;
					else dkcrew += P_NCREW;
				} else if (rand()%2==0 || Ploss>90) {
					/* destroy a ship */
					NSUB_WAR(1);
					k = P_NCREW*(shipsize+1);
					thold -= (shipsize+1);
					if(which==ATKR) {
						awsunk++;
						ahold-=(shipsize+1);
						akcrew+=k;
					} else {
						dwsunk++;
						dhold-=(shipsize+1);
						dkcrew+=k;
					}
				} else {
					/* damage a ship */
					k = (shipsize+1)*Ploss*P_NCREW/100;
					if(which==ATKR) akcrew += k;
					else dkcrew += k;
					P_NCREW -= k/thold;
				}
			}
			/* check galleys */
			for(i=0;i<P_NGAL(shipsize);i++)
			if (rand()%100<Ploss) {
				if(rand()%100<cptpct) {
					/* capture a ship */
					if(which==ATKR){
						if (dhold) {
							agcapt++;
							ghold-=(shipsize+1);
							thold-=(shipsize+1);
							capture(QGAL,DFND,shipsize,rand()%dhold+1);
							NSUB_GAL(1);
						}
					} else if(which==DFND) {
						if (ahold) {
							dgcapt++;
							ghold-=(shipsize+1);
							thold-=(shipsize+1);
							capture(QGAL,ATKR,shipsize,rand()%ahold+1);
							NSUB_GAL(1);
						}
					}
					if(which==ATKR) akcrew += P_NCREW;
					else dkcrew += P_NCREW;
					if (P_NARMY!=MAXARM) {
						armynum = P_NARMY;
						k = P_ASOLD*(shipsize+1)/(ghold+shipsize+1);
						if(P_ATYPE<MINLEADER) {
						if(which==ATKR) akcrew += k;
						else dkcrew += k;
						P_ASOLD -= k;
						} else if(rand()%100<k*100/P_ASOLD){
						if(which==ATKR) akcrew += P_ASOLD;
						else dkcrew += P_ASOLD;
						P_ASOLD = 0;
						P_NARMY = MAXARM;
						}
					}
				} else if (rand()%3==0||Ploss>90) {
					/* ship destroyed */
					k = (shipsize+1)*SHIPCREW;
					if(which==ATKR) akcrew += k;
					else dkcrew += k;
					NSUB_GAL(1);
					ghold-=(shipsize+1);
					thold-=(shipsize+1);
					/* kill all soldiers onboard */
					if (P_NARMY!=MAXARM) {
						armynum = P_NARMY;
						k = P_ASOLD*(shipsize+1)/(ghold+shipsize+1);
						if(P_ATYPE<MINLEADER) {
						if(which==ATKR) akcrew += k;
						else dkcrew += k;
						P_ASOLD -= k;
						} else if(rand()%100<k*100/P_ASOLD){
						if(which==ATKR) akcrew += P_ASOLD;
						else dkcrew += P_ASOLD;
						P_ASOLD = 0;
						P_NARMY = MAXARM;
						}
					}
					if(which==ATKR) agsunk++;
					else dgsunk++;
				} else {
					/* damage a ship */
					k = (shipsize+1)*Ploss*P_NCREW/100;
					if(which==ATKR) akcrew += k;
					else dkcrew += k;
					P_NCREW -= k/thold;
					/* damage any soldiers onboard */
					if (P_NARMY!=MAXARM) {
						armynum = P_NARMY;
						k = P_ASOLD*(shipsize+1)*Ploss/(ghold*100);
						if(P_ATYPE<MINLEADER) {
						P_ASOLD -= k;
						if(which==ATKR) akcrew += k;
						else dkcrew += k;
						} else if(rand()%100<k*100/P_ASOLD){
						if(which==ATKR) akcrew += P_ASOLD;
						else dkcrew += P_ASOLD;
						P_ASOLD = 0;
						P_NARMY = MAXARM;
						}
					}
				}
			}
			/* check merchants */
			for(i=0;i<P_NMER(shipsize);i++)
			if (rand()%100<Ploss) {
				if(rand()%100<cptpct) {
					/* capture a ship */
					if(which==ATKR){
						if (dhold) {
							amcapt++;
							capture(QMER,DFND,shipsize,rand()%dhold+1);
							NSUB_MER(1);
						}
					} else if(which==DFND) {
						if (ahold) {
							dmcapt++;
							capture(QMER,ATKR,shipsize,rand()%ahold+1);
							NSUB_MER(1);
						}
					}
					if(which==ATKR) akcrew += P_NCREW;
					else dkcrew += P_NCREW;
				} else if (rand()%3==0 || Ploss>90) {
					/* ship destroyed */
					k = (shipsize+1)*P_NCREW;
					if(which==ATKR) akcrew += k;
					else dkcrew += k;
					P_NCREW -= k/thold;
					k = P_NCREW*thold;
					NSUB_MER(1);
					if((thold-=(shipsize+1))!=0) P_NCREW = k / thold;
					else P_NCREW = 0;
					/* kill all people onboard */
					k = (shipsize+1)*P_NPEOP;
					if(which==ATKR) akcrew += k;
					else dkcrew += k;
					P_NPEOP -= k/(thold+shipsize+1);
					k = P_NPEOP*(thold+shipsize+1);
					if (thold>0) P_NPEOP = k / thold;
					else P_NPEOP=0;
					if(which==ATKR) amsunk++;
					else dmsunk++;
				} else {
					/* damage a ship */
					k = (shipsize+1)*Ploss*P_NCREW/100;
					if(which==ATKR) akcrew += k;
					else dkcrew += k;
					P_NCREW -= k/thold;
					/* damage any people onboard */
					k = (shipsize+1)*Ploss*P_NPEOP/100;
					if(which==ATKR) akcrew += k;
					else dkcrew += k;
					P_NPEOP -= k/thold;
				}
			}
		}
	}
#ifdef HIDELOC
	fprintf(fnews,"4.\tNaval Battle occurs");
#else
	fprintf(fnews,"4.\t%d,%d: Naval Battle",xspot,yspot);
#endif
	for(j=0;j<count;j++) if(owner[j]!=(-1)){
		k=0;
		for(i=0;i<j;i++) if(owner[j]==owner[i]) k=1;
		if(k==0) {
		if(side[j]==ATKR)
			fprintf(fnews,", attacker %s",ntn[owner[j]].name);
		else if(side[j]==DFND)
			fprintf(fnews,", defender %s",ntn[owner[j]].name);
		else fprintf(fnews,", neutral %s",ntn[owner[j]].name);
		}
	}
	fprintf(fnews,"\n");

	/*mail results; who is in the battle*/
	for(j=0;j<count;j++) if(owner[j]!=(-1)){
		done=FALSE;

		/*first time your nation appears done=FALSE*/
		for(i=0;i<j;i++) if(owner[j]==owner[i]) done=TRUE;

		if((done==FALSE)&&(ispc(ntn[owner[j]].active))) {
			if (mailopen( owner[j] )==(-1)) continue;

			fprintf(fm,"NAVAL BATTLE in sector %d %d\n",xspot,yspot);
			fprintf(fm,"Battle occured during %s of Year %d\n",
				PSEASON(TURN),YEAR(TURN));

			if(side[j]==ATKR)
				fprintf(fm,"You are on the Attacking Side\n");
			else	if(side[j]==DFND)
				fprintf(fm,"You are on the Defending Side\n");
			else	fprintf(fm,"You are on the Neutral Side\n");

			/*detail all participants in battle*/
			for(k=0;k<count;k++) if(owner[k]!=(-1)){
				if(side[k]==DFND)
				fprintf(fm," %s is defender with navy ",ntn[owner[k]].name);
				else if(side[k]==ATKR)
				fprintf(fm," %s is attacker with navy ",ntn[owner[k]].name);
				else
				fprintf(fm," %s is neutral with navy ",ntn[owner[k]].name);
				fprintf(fm,"%d (%d warships %d galleys %d merchants)",
					unit[k],wnum[k],gnum[k],mnum[k]);
				putc('\n',fm);
 
			}

			fprintf(fm,"attacker strength (%d men) -> percent loss %d%%\n",acrew,PAloss);
			fprintf(fm,"defender strength (%d men) -> percent loss %d%%\n",dcrew,PDloss);
			fprintf(fm,"Odds are %d to 100; RANDOM ROLL is %d\n",odds,roll);
			fprintf(fm,"RESULT:  Attackers lose %d men  Defenders lose %d men\n",akcrew,dkcrew);
			/* display any other results */
			show_ships("Attacking","sunk",awsunk,agsunk,amsunk);
			show_ships("Defending","sunk",dwsunk,dgsunk,dmsunk);
			show_ships("Attacking","captured",awcapt,agcapt,amcapt);
			show_ships("Defending","captured",dwcapt,dgcapt,dmcapt);
			mailclose(owner[j]);
		}
	}
	curntn= saventn;
	country= savecntry;
	printf("Out Naval Combat....\n");
}

/*
 * capture - Distribute captured ship to appropriate fleet during naval combat
 *
 * Assigns a captured enemy ship to a specific fleet on the victorious side,
 * using a weighted distribution system based on fleet sizes. The ship is
 * awarded to the fleet with the largest warship capacity, representing
 * the fleet most capable of securing and integrating the captured vessel.
 *
 * Algorithm:
 * - Scan all fleets on the victorious side (ATKR or DFND)
 * - Calculate cumulative warship capacity for weighted selection
 * - Use holdcount parameter to determine which fleet receives the prize
 * - Award ship to the fleet whose capacity bracket contains holdcount
 * - Add the captured ship to the winning fleet's inventory
 * - Restore nation context to maintain global state consistency
 *
 * Ship Distribution Logic:
 * - Larger fleets have higher probability of receiving captured ships
 * - Distribution proportional to warship hold capacity (combat effectiveness)
 * - Ensures ships go to fleets capable of utilizing them effectively
 * - Prevents captured ships from being assigned to non-existent fleets
 *
 * Parameters:
 *   type - Ship type being captured (QWAR=warship, QGAL=galley, QMER=merchant)
 *   to - Combat side receiving the captured ship (ATKR/DFND)
 *   shipsize - Size class of captured ship (N_LIGHT/N_MEDIUM/N_HEAVY)
 *   holdcount - Weighted distribution selector based on fleet capacities
 *
 * Returns:
 *   void (modifies fleet inventories directly)
 *
 * Side Effects:
 *   - Adds one ship of specified type/size to a victorious fleet
 *   - Temporarily changes curntn global pointer during processing
 *   - Restores original nation context after ship assignment
 *   - May fail silently if no valid recipient fleet found
 *
 * Global Variables Used:
 *   - count: Number of fleets participating in battle
 *   - owner[], unit[], side[]: Fleet ownership, unit IDs, combat sides
 *   - curntn: Current nation context pointer (saved/restored)
 *   - ntn[]: Global nation data for fleet modifications
 *
 * Macros Used:
 *   - fltwhold(): Calculate warship hold capacity for fleet
 *   - NADD_WAR(), NADD_GAL(), NADD_MER(): Add ships to fleet inventory
 *
 * Testing Notes:
 *   Category: A (Unit) - Clear input/output with predictable ship assignment
 *   Approach: Unit testing with mock fleet data and capture scenarios
 *   Key Tests: Ship type assignment, side validation, fleet selection logic
 *   Dependencies: Fleet data structures, ship inventory macros
 *   Mock Requirements: Battle participants, fleet capacities, ship inventories
 *   Complexity: Simple - Straightforward weighted distribution algorithm
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to global variable dependencies
 *   - Performance: O(n) where n = number of participating fleets
 *   - Historical context: Represents prize allocation in naval warfare
 *   - Fair distribution: Larger fleets receive more captures (realistic)
 *   - Error handling: Silent failure if no valid recipient found
 *   - Integration: Called from navalcbt() during combat resolution
 *   - Ship types: Supports all three naval vessel categories
 */
/* routine to distribute a captured ship */
void 
capture (int type, int to, int shipsize, int holdcount)
{
	int i,nvynum;
	struct s_nation *saventn=curntn;
#ifdef DEBUG
	printf("capture: hdcnt==%d typ==%d spsz==%d to==%d\n",holdcount,
		  type,shipsize,to);
#endif /* DEBUG */
	for (i=0;holdcount && i<count;i++) {
		if (owner[i]!=(-1) && side[i]==to) {
			curntn= &ntn[owner[i]];
			holdcount -= fltwhold(unit[i]);
		}
		if(holdcount<=0) {
			holdcount=0;
			i--;
		}
	}
#ifdef DEBUG
	printf("capture 2: holdcount==%d i==%d\n",holdcount,i);
#endif /* DEBUG */
	if (i==count) {
		curntn = saventn;
		return;
	}
	nvynum = unit[i];
#ifdef DEBUG
	printf("Should have successful capture of %d\n",type);
#endif /* DEBUG */
	switch(type) {
	case QWAR:
		(void) NADD_WAR(1);
		break;
	case QGAL:
		(void) NADD_GAL(1);
		break;
	case QMER:
		(void) NADD_MER(1);
		break;
	default:
		fprintf(stderr,"unknown type in function capture");
		break;
	}
	curntn = saventn;
}

/*
 * show_ships - Format and display naval battle results in mail reports
 *
 * Generates formatted output for naval combat casualties (sunk ships) and
 * prize captures in battle reports sent to participating nations. Provides
 * clear, consistent formatting for ship losses and gains across all three
 * vessel types (warships, galleys, merchants).
 *
 * Algorithm:
 * - Check if any ships of any type need reporting (total > 0)
 * - If ships to report, output descriptive header with side and action
 * - For each ship type with non-zero count, append count and type name
 * - Complete line with newline for clean report formatting
 * - Skip output entirely if no ships to report (avoids empty lines)
 *
 * Output Format Examples:
 * - "Attacking ships sunk: 3 Warships 2 Galleys"
 * - "Defending ships captured: 1 Merchants"
 * - "Attacking ships sunk: 5 Warships 1 Galleys 2 Merchants"
 *
 * Parameters:
 *   who - Side description string ("Attacking" or "Defending")
 *   what - Action description string ("sunk" or "captured")
 *   war - Number of warships affected (0 = none to report)
 *   gal - Number of galleys affected (0 = none to report)
 *   mer - Number of merchant ships affected (0 = none to report)
 *
 * Returns:
 *   void (outputs directly to mail file stream)
 *
 * Side Effects:
 *   - Writes formatted text to global mail file stream (fm)
 *   - Outputs newline character to complete the report line
 *   - No output if all ship counts are zero (clean report formatting)
 *
 * Global Variables Used:
 *   - fm: Global mail file stream for battle report output
 *
 * Testing Notes:
 *   Category: A (Unit) - Simple formatting function with clear inputs/outputs
 *   Approach: Unit testing with various ship count combinations
 *   Key Tests: All zero counts, single ship type, multiple ship types, formatting
 *   Dependencies: Mail file stream for output
 *   Mock Requirements: Mail system context, file stream handling
 *   Complexity: Simple - Straightforward conditional formatting
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to global file stream dependency
 *   - Performance: O(1) - constant time formatting operation
 *   - Historical context: Clean battle report presentation for players
 *   - Integration: Called from navalcbt() for result reporting
 *   - Output optimization: Skips empty reports to avoid clutter
 *   - Formatting consistency: Standardized naval battle report format
 *   - User experience: Clear, readable battle outcome presentation
 */
/* routine to display combat results */
void 
show_ships (char *who, char *what, int war, int gal, int mer)
{
	if (war+gal+mer>0) {
		fprintf(fm,"%s ships %s: ",who,what);
		if (war) fprintf(fm,"%d Warships ",war);
		if (gal) fprintf(fm,"%d Galleys ",gal);
		if (mer) fprintf(fm,"%d Merchants",mer);
		putc('\n',fm);
	}
}
