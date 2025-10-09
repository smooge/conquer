/*
 * cexecute.c - Command execution engine and emergency cleanup system
 *
 * Core command processing system that reads and executes nation command files,
 * applying state changes to armies, navies, sectors, and nation data. Provides
 * the central execution engine for processing player commands and updating
 * game state during both regular gameplay and turn updates.
 *
 * KEY SYSTEMS:
 * - Command file parsing and execution (execute function)
 * - Game state modification and validation
 * - Resource management (gold, metals, jewels)
 * - Emergency cleanup and termination handling
 * - File locking and temporary file management
 *
 * EXECUTION MODEL:
 * The execute() function processes nation-specific command files containing
 * serialized game commands. Each command modifies specific aspects of game
 * state (army positions, sector ownership, resource levels, etc.). The system
 * supports both regular integer and long integer commands with L_ prefix.
 *
 * COMMAND TYPES SUPPORTED:
 * - Army adjustments (position, status, movement)
 * - Navy modifications (ships, crew, cargo)
 * - Sector changes (ownership, designation, population)
 * - Nation updates (resources, attributes, diplomacy)
 * - Magic system modifications
 * - Administrative functions
 *
 * ERROR HANDLING:
 * The system includes comprehensive error checking for ownership violations,
 * invalid commands, and resource conflicts. Emergency cleanup via hangup()
 * ensures data integrity during unexpected termination.
 *
 * CRITICAL DEPENDENCIES:
 * - Global game state (sectors, nations, armies, navies)
 * - File system access for execution files
 * - Mail system for administrative notifications
 * - Lock file management for concurrency control
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
#include <unistd.h>
#include "header.h"
#include "data.h"
#include "safe_convert.h"

/*
 * execute - Process nation command execution file and apply game state changes
 *
 * Core command execution engine that reads and processes nation command files,
 * applying requested state changes to armies, navies, sectors, and nation data.
 * Handles parsing of execution commands and updates game state accordingly.
 * Used both during regular gameplay and update processing.
 *
 * Parameters:
 *   isupdate - Execution mode flag (0 = regular play, 1 = update processing)
 *
 * Returns:
 *   1 if commands were executed and state was modified
 *   0 if no execution file exists or no commands processed
 *
 * Side Effects:
 *   - Modifies global game state (armies, navies, sectors, nations)
 *   - Updates nation resources (gold, metals, jewels)
 *   - Changes sector ownership and designation
 *   - Modifies army and navy positions and attributes
 *   - Updates nation attributes (popularity, terror, reputation)
 *   - May trigger destruction of nations
 *   - Initializes startgold for non-update executions
 *   - Scales i_people for sectors to handle values >= 32K
 *
 * Testing Notes:
 *   Category: C (System) - Requires full game state initialization
 *   Approach: System testing with mock execution files and game state
 *   Key Tests: Command parsing, state updates, file handling, error cases
 *   Dependencies: Global game state, file system, nation data structures
 *   Mock Requirements: Execution files, game state, nation structures
 *   Complexity: Complex - Central game logic with extensive state management
 *
 * Notes:
 *   - Processes commands from nation-specific execution files
 *   - Supports both regular integers and long integers (L_ prefix)
 *   - Contains extensive switch statement for ~30 command types
 *   - Critical for maintaining game state consistency
 *   - Handles edge cases for sector ownership conflicts
 *   - Debug output available for bribe operations
  * @last_documented: 2025-09-20
 */
int execute(int isupdate) {	/* 0 if not update, 1 if update */
	FILE *fp;
	int cmd,savectry;
	char comment[LINELTH];
	char temp[LINELTH];
	long longvar = 0,long2var = 0;
	int armynum = 0;  /* Initialize to prevent undefined behavior with L_ commands */
	short int x,y;
	int execed=0;
	int done=FALSE;
	char line[BIGLTH];

	/* initialize startgold */
	if( isupdate==0) startgold = curntn->tgold;

	/* initialize i_people */
	for(x=0;x<MAPX;x++)
		for(y=0;y<MAPY;y++)
			if(( sct[x][y].owner == country)&&
			((sct[x][y].designation == DTOWN)
			||( sct[x][y].designation == DCAPITOL)
			||( sct[x][y].designation == DCITY)))
/* note: i_people is a short, so we must scale to allow for people >= 32K */
				sct[x][y].i_people = safe_long_to_short(sct[x][y].people/256);
			else
				sct[x][y].i_people = -1;

	/*open exefile file*/
	snprintf(line, sizeof(line), "%s%d", exefile, country);
	if ((fp=fopen(line,"r"))==NULL) {
		/*THIS MEANS THAT THE NATION HAS NOT MOVED YET*/
		return(0);
	}
	savectry=country;

	/*read in file*/
	if(fgets(line,80,fp)==NULL) done=TRUE;
	while(done==FALSE) {
		/*read and parse a new line*/
		/*CODE IF YOU USE LONG VAR IS L_*/
		if( line[0] == 'L' && line[1] == '_' ) {
			int result = sscanf(line,"%79s %d %hd %ld %ld %hd %79s",
				temp,&cmd,&country,&longvar,&long2var,&y,comment);
			if (result != 7) {
				/* Parse error - skip malformed command line */
				temp[0] = '\0';
				comment[0] = '\0';
				if(fgets(line,80,fp)==NULL) done=TRUE;
				continue;
			}
		} else {
			int result = sscanf(line,"%79s %d %hd %d %hd %hd %79s",
				temp,&cmd,&country,&armynum,&x,&y,comment);
			if (result != 7) {
				/* Parse error - skip malformed command line */
				temp[0] = '\0';
				comment[0] = '\0';
				if(fgets(line,80,fp)==NULL) done=TRUE;
				continue;
			}
		}
		curntn = &ntn[country];

		execed=1;
		switch(cmd){
		case XASTAT:		/*Aadjstat*/
			if(x>0)  P_ASTAT=safe_short_to_uchar(x);
			break;
		case XAMEN:	/*Aadjmen*/
			armynum= (int) longvar;
			P_ASOLD= (int) long2var;
			P_ATYPE= safe_short_to_uchar(y);
			break;
		case XBRIBE:	/* nation has been bribed */
			if(!isupdate) break;	/* only work on update */
			ntn[y].tgold += longvar;
#ifdef DEBUG
	fprintf(stderr,"DEBUG: BRIBE BY %s of %s\n",ntn[country].name,ntn[y].name);
#endif /* DEBUG */
			/* x represents chance of successful bribe */
			if(npctype(curntn->active)==npctype(ntn[y].active))
				x = 50;
			else if(isneutral(ntn[y].active)) x=30;
			else if(npctype(ntn[y].active)==ISOLATIONIST) x=15;
			else	x = 20;
			if(curntn->race==ntn[y].race) x+= 20;
			if( rand()%100 < x){
#ifdef DEBUG
	fprintf(stderr,"DEBUG: BRIBE IS SUCCESS\n");
#endif /* DEBUG */
				ntn[y].dstatus[country]--;
			}
			break;
		case XALOC:	/*Aadjloc*/
			P_AXLOC=safe_short_to_uchar(x);
			P_AYLOC=safe_short_to_uchar(y);
			break;
		case MSETA:	/*Aadjmerc*/
#ifdef CONQUER
			mercgot+=armynum;
#endif
#ifdef ADMIN
			MERCMEN-=armynum;
#endif
			break;
		case MSETB:	/*Aadjdisb*/
			/* only allow raising of merc bonus */
			if (x>MERCATT)
			MERCATT=safe_long_to_short((MERCMEN*MERCATT+armynum*x)/(MERCMEN+armynum));
			if (y>MERCDEF)
			MERCDEF=safe_long_to_short((MERCMEN*MERCDEF+armynum*y)/(MERCMEN+armynum));
			MERCMEN+=armynum;
			break;
		case XNLOC: /*nadjloc*/
			curntn->nvy[armynum].xloc=safe_short_to_uchar(x);
			curntn->nvy[armynum].yloc=safe_short_to_uchar(y);
			break;
		case XNACREW:
			curntn->nvy[armynum].crew=safe_short_to_uchar(x);
			curntn->nvy[armynum].armynum=safe_short_to_uchar(y);
			break;
		case XNAMER: /*nadjmer*/
			curntn->nvy[armynum].merchant=safe_int_to_ushort(x);
			break;
		case XNAWAR: /*nadjwar*/
			curntn->nvy[armynum].warships=safe_int_to_ushort(x);
			break;
		case XNAGAL: /*nadjgal*/
			curntn->nvy[armynum].galleys=safe_int_to_ushort(x);
			break;
		case XNAHOLD: /*nadjhld*/
			curntn->nvy[armynum].people=(unsigned char)y;
			curntn->nvy[armynum].armynum=(unsigned char)x;
			break;
		case XECNAME:	/*Nadjname*/
			snprintf(curntn->name, sizeof(curntn->name), "%.*s", (int)(sizeof(curntn->name) - 1), comment);
			break;
		case XECPAS:	/*Nadjpas*/
			snprintf(curntn->passwd, PASSLTH+1, "%.*s", PASSLTH, comment);
			break;
		case NPOP:	/* set various nation attributes */
#ifdef CONQUER
			terror_adj++;
#endif /* CONQUER */
			curntn->popularity = safe_int_to_uchar(armynum);
			curntn->terror = safe_short_to_uchar(x);
			curntn->reputation = safe_short_to_uchar(y);
			break;
		case NTAX:	/* set nations tax rate */
#ifdef ADMIN
			if (ispc(curntn->active) && !ispc(x)) {
			  fprintf(fnews, "1.\tNation %s is now being run by the computer.\n", curntn->name);
			}
			if (ispc(x) && !ispc(curntn->active)) {
			  fprintf(fnews, "1.\tNation %s is no longer being run by the computer.\n", curntn->name);
			}
#endif /*ADMIN*/
			curntn->tax_rate = safe_int_to_uchar(armynum);
			curntn->active = safe_short_to_uchar(x);
			curntn->charity = safe_short_to_uchar(y);
			break;
		case EDSPL:	/*Edecspl*/
			curntn->spellpts-=safe_int_to_short(armynum);
			break;
		case XSADES:	/*Sadjdes*/
			if((sct[x][y].owner!=country)&&(country!=0)) {
				fprintf(stderr,"ERROR: <%s> redesignate sector %d,%d that is not owned\n",curntn->name,x,y);
				break;
			}

			sct[x][y].designation=comment[0];
			if(sct[x][y].designation==DCAPITOL){
				curntn->capx=safe_short_to_uchar(x);
				curntn->capy=safe_short_to_uchar(y);
			}
#ifdef CONQUER
			if (sct[x][y].designation==DROAD)
				roads_this_turn++;
#endif
			break;
		case XSACIV:	/*Sadjciv*/
			/* if for some reason you dont own it, put people
			back into your capitol */
			if((sct[x][y].owner!=country)&&(country!=0)) {
				sct[curntn->capx][curntn->capy].people+=armynum;
				break;
			}
			sct[x][y].people=armynum;
			break;
		case XSACIV3:	/*Sadjciv3 - incremental people adjust */
			/* if you dont own it, put people in your capitol */
			if((sct[x][y].owner!=country)&&(country!=0)) {
				sct[curntn->capx][curntn->capy].people+=armynum;
			}
			else
			{
				sct[x][y].people+=armynum;
			}
			break;
		case XSIFORT:	/*Sincfort*/
			sct[x][y].fortress++;
			break;
		case XNAGOLD:	/*Nadjgold:*/
			curntn->tgold = longvar;
			break;
		case XAMOV:
			P_AMOVE=safe_short_to_uchar(x);
			break;
		case XNMOV:
			curntn->nvy[armynum].smove=safe_short_to_uchar(x);
			break;
		case XSAOWN:
			/* if not own it, and if people there, problem */
			if((sct[x][y].owner!=country)
			&&( country!=0)
			&&( sct[x][y].owner!=0 )
			&&( sct[x][y].people>0 )
			&&( magic(country,SLAVER)==FALSE )
			&&( ntn[sct[x][y].owner].race != curntn->race)){
				sct[ntn[sct[x][y].owner].capx][ntn[sct[x][y].owner].capy].people+= sct[x][y].people;
				sct[x][y].people=0;
				fprintf(stderr,"ERROR: <%s> taking sector %d %d but civilians exist of other race - puting them in their capitol\n",curntn->name,x,y);
			}
			if(curntn->popularity<MAXTGVAL) curntn->popularity++;
			sct[x][y].owner=safe_short_to_uchar(country);
			break;
		case EDADJ:
			curntn->dstatus[armynum]=safe_short_to_char(x);
			break;
		case XNARGOLD:
			curntn->jewels = longvar;
			break;
		case XNAMETAL:
			curntn->metals = longvar;
			break;
		case INCAPLUS:
			curntn->aplus++;
			break;
		case INCDPLUS:
			curntn->dplus++;
			break;
		case DESTRY:
			sct[ntn[armynum].capx][ntn[armynum].capy].owner=safe_int_to_uchar(savectry);
			country=safe_int_to_short(armynum);
			if (isupdate) destroy(country);
			country=safe_int_to_short(savectry);
			break;
		case CHG_MGK:
			curntn->powers|=long2var;
			if(curntn->powers!=longvar){
			printf("\nERROR ON MAGIC READ country=%d %ld != %ld (or of %ld)",country,longvar,curntn->powers,long2var);
			getchar();
			}
			for(armynum=0;armynum<=MAXPOWER;armynum++) {
				longvar = 1L << armynum;
				if (longvar & long2var)
					exenewmgk(longvar);
			}
			long2var=0;
			break;
		}
		if(fgets(line,80,fp)==NULL) done=TRUE;
	}
	fclose(fp);
	/*return 1 if it did something*/
	country=safe_int_to_short(savectry);
	curntn = &ntn[country];
	if(execed==1) return(1);
	else return(0);
}
#ifdef CONQUER
/*
 * hangup - Emergency cleanup and program termination handler
 *
 * Performs critical cleanup operations when the program terminates unexpectedly
 * or when a player disconnects. Ensures game state consistency by saving current
 * nation resources, cleaning up temporary files, and notifying administrators.
 * Called on signal interrupts or emergency shutdowns.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   Does not return - calls exit(FAIL) to terminate program
 *
 * Side Effects:
 *   - Writes current nation resources to execution file (gold, metals, jewels)
 *   - Closes execution file to ensure data persistence
 *   - Removes temporary mail files to clean up resources
 *   - Removes lock files to allow other processes access
 *   - Removes temporary message files
 *   - Sends notification message to game administrator (God)
 *   - Terminates program with failure exit code
 *
 * Testing Notes:
 *   Category: D (Mock) - Emergency handler requiring signal simulation
 *   Approach: Unit testing with mocked file operations and signal handling
 *   Key Tests: Resource saving, file cleanup, notification sending, exit handling
 *   Dependencies: File system, global variables, mail system
 *   Mock Requirements: File operations, signal handling, mail system
 *   Complexity: Moderate - Critical cleanup with multiple file operations
 *
 * Notes:
 *   - Only compiled when CONQUER is defined
 *   - Critical for preventing data loss during unexpected termination
 *   - Ensures proper cleanup of system resources
 *   - Maintains file locking integrity
 *   - Thread safety not required (single-threaded emergency handler)
  * @last_documented: 2025-09-20
 */
void hangup (int sig) {
	(void)sig;  /* Signal number not used in this handler */
	char line[LINELTH];

	if(country==0) writedata();
	else {
		fprintf(fexe,"L_NGOLD\t%d \t%d \t%ld \t0 \t0 \t%s\n",
		XNAGOLD ,country,curntn->tgold,"null");
		fprintf(fexe,"L_NMETAL\t%d \t%d \t%ld \t0 \t0 \t%s\n",
		XNAMETAL ,country,curntn->metals,"null");
		fprintf(fexe,"L_NJWLS\t%d \t%d \t%ld \t0 \t0 \t%s\n",
		XNARGOLD ,country,curntn->jewels,"null");
	}
	/*close file*/
	fclose(fexe);

	/* remove any existing mail reading/writing file */
	/* TODO why are we doing this here versus in misc? */
	if (mailok!=DONEMAIL) {
		unlink(tmp_mail_name);
	}

	/* remove the lock file */
	unlink(fison);
	snprintf(line, sizeof(line), "%s%hd.tmp", msgfile, country);
	unlink(line);

	/*send a message to God*/
	if(mailopen( 0 )!=(-1)) {
		fprintf(fm,"WARNING: Nation %s hungup on me.\n",curntn->name);
		mailclose(0);
	}

	/* exit program */
	exit(FAIL);
}
#endif /* CONQUER */
