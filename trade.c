/*
 * trade.c - Trade and commerce system
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
#include "trade.h"

#ifdef TRADE		

/* possible commodities */
#define TDGOLD   0
#define TDFOOD   1
#define TDMETAL  2
#define TDJEWL   3
#define TDLAND   4
#define TDARMY   5
#define TDSHIP   6

/* constants and indicators */
#define	NUMPRODUCTS	7
#define	MAXITM	30
#define	SELL	0
#define	BUY	1
#define	NODEAL	2
#define	NOSALE	3
#define TRADECOST(cost)	(100-cost)/100	/* twenty percent cost is normal*/

extern short country;

char *commodities[NUMPRODUCTS] = { "Gold", "Food", "Metal", "Jewels",
		"Land", "Soldiers", "Ships"};
#ifdef ADMIN
char *tradefail[NUMPRODUCTS] = { "lack of gold", "lack of food",
	"lack of metal", "lack of jewels", "land not owned",
	"unavailable or destroyed armies",
	"unavailable or destoryed navies"};
#endif /* ADMIN */
#ifdef CONQUER
/* Use this when you wish to bid something */
char *buylist[NUMPRODUCTS] = { "Bid how much gold? ", "Bid how much food? ",
	"Bid how much metal? ", "Bid how many jewels? ", "What X location? ",
	"Bid what army? ", "Bid what navy? "};

/*
 * trade - Main interactive trading interface for commodity exchange
 *
 * Implements the complete commodities exchange interface where players can buy,
 * sell, and unsell various commodities including gold, food, metal, jewels, land,
 * armies, and ships. The function provides a full-screen interface displaying
 * available commodities from all nations and enables real-time trading operations.
 *
 * The interface displays:
 * - God/NPC merchant offerings (food, metal, jewels) at fixed prices
 * - Player-offered commodities with minimum bid requirements
 * - Interactive menu for buy/sell/unsell operations
 * - Comprehensive validation and error handling
 *
 * Trading Features:
 * - Buy from god merchants using gold at fixed exchange rates
 * - Buy from other players by meeting minimum bid requirements
 * - Sell commodities to marketplace with minimum price requirements
 * - Unsell (remove) own commodities from marketplace
 * - Real-time display updates showing market changes
 * - File-based persistent trade data storage
 *
 * Parameters: None (uses global state)
 *
 * Returns: void
 *
 * Side Effects:
 *   - Modifies nation resources (gold, food, metal, jewels, armies, ships)
 *   - Updates trade file with new transactions
 *   - Modifies army/navy status for traded units
 *   - Updates screen display with commodity listings
 *   - May block for user input during interactive operations
 *
 * Testing Notes:
 *   Category: C (System) - Requires full game initialization and file system
 *   Approach: System testing with mock nations and trade file
 *   Key Tests: Buy/sell operations, validation, error handling, file I/O
 *   Dependencies: Game state, nation data, trade file, display system
 *   Mock Requirements: File system, display functions, nation initialization
 *   Complexity: Complex - Multi-branch UI with extensive validation and file operations
 *
 * Notes:
 *   - Interactive function requiring full curses display system
 *   - File operations may fail requiring error handling
 *   - Complex state management with trade reservations
 *   - Uses goto-style control flow with while loops and switch statements
 */
void
trade()
{
	FILE *tfile;
	int count, done=FALSE, notopen=FALSE;
	int buysell, holdint, holdint2, extint, inloop;
	int type1[MAXITM], type2[MAXITM], deal[MAXITM], extra[MAXITM];
	int natn[MAXITM], itemnum, getland(), gettrade(), checkland();
	int tradable();
	long lvar1[MAXITM], lvar2[MAXITM], holdlong, holdlong2, armyvalue();
	void tradeerr(), setaside(), takeback();
	
	clear();
	while (done==FALSE) {
		itemnum=0;
		done=TRUE;
		/* open trading file */
		if ((tfile=fopen(tradefile,"r")) == NULL ) {
			notopen=TRUE;
		}
		/* read in all of the data */
		while (notopen==FALSE && !feof(tfile)) {
			holdint = fscanf(tfile,"%d %d %d %d %ld %ld %d\n",
				&deal[itemnum],&natn[itemnum],&type1[itemnum],
				&type2[itemnum],&lvar1[itemnum],
				&lvar2[itemnum],&extra[itemnum]);
			if (holdint == 7) {
				if (deal[itemnum]==NOSALE) {
					/* remove item from sales list */
					deal[type1[itemnum]]=NOSALE;
				} else if (deal[itemnum]==SELL) {
					itemnum++;
				}
			}
		}
		if (notopen==FALSE) fclose(tfile);
		move(0,0);
		clrtobot();
		/* display header */
		standout();
		mvaddstr(0,27,"COMMODITIES EXCHANGE");
		mvaddstr(1,20,"No Guarantees... All Trades Final");
		standend();
		count=3;
		mvaddstr(count,0,"    Nation                       Item            Price");
		/* give some minor items for purchase */
		mvprintw(++count,0,"%2d) %-20s",GETFOOD,"Merchants Guild");
		mvprintw(count,30,"%9ld %s",GODFOOD,commodities[TDFOOD]);
		mvprintw(count,50,"%9ld %s",GODPRICE,commodities[TDGOLD]);
		mvprintw(++count,0,"%2d) %-20s",GETMETAL,"Merchants Guild");
		mvprintw(count,30,"%9ld %s",GODMETAL,commodities[TDMETAL]);
		mvprintw(count,50,"%9ld %s",GODPRICE,commodities[TDGOLD]);
		mvprintw(++count,0,"%2d) %-20s",GETJEWL,"Merchants Guild");
		mvprintw(count,30,"%9ld %s",GODJEWL,commodities[TDJEWL]);
		mvprintw(count,50,"%9ld %s",GODPRICE,commodities[TDGOLD]);

		/* go through list of commodities */
		for (holdint=0;holdint<itemnum;holdint++) {
			if ((deal[holdint]==SELL) && ((country==0) ||
			(ntn[natn[holdint]].dstatus[country] != UNMET)
			|| (natn[holdint] == country))) {
				count++;
				mvprintw(count,0,"%2d) %-20s",
					holdint+1,
					ntn[natn[holdint]].name);

				if (type1[holdint]==TDLAND) {
					holdlong = (long) tofood( &sct[(int)lvar1[holdint]][extra[holdint]],0);
					mvprintw(count,30,"(food=%2ld) %s",
						holdlong,
						commodities[type1[holdint]]);
				} else {
					holdlong = lvar1[holdint];
					mvprintw(count,30,"%9ld %s",
						holdlong,
						commodities[type1[holdint]]);
				}
				if (type2[holdint]==TDLAND) {
					mvprintw(count,50,"(food=%2ld) %s",
						lvar2[holdint],
						commodities[type2[holdint]]);
				} else {
				mvprintw(count,50,"%9ld %s",
					lvar2[holdint],
					commodities[type2[holdint]]);
				}
				if (count>LINES-8) {
					standout();
					mvaddstr(LINES-2,30,"Hit Any Key to Continue");
					standend();
					refresh();
					getch();
					clear();
					standout();
					mvaddstr(0,27,"COMMODITIES EXCHANGE");
					standend();
			    		mvaddstr(2,0,"    Nation                       Item            Minimum Price");
					count=2;
				}
			}
		}
		standout();
		count++;
		count++;
		mvaddstr(count++,0,"Do you wish to (B)uy, (S)ell, or (U)nsell?");
		standend();
		refresh();
		inloop=TRUE;
		while (inloop==TRUE) switch(getch()) {
		case 'b':
		case 'B':
			buysell=BUY;
			mvaddstr(count++,0,"What item number do you want to purchase? ");
			refresh();
			holdint = get_number();
			if (holdint<0) break;
			/* check for minor sales */
			if (holdint==GETFOOD || holdint==GETMETAL || holdint==GETJEWL) {
				/* strange flow but less control needed */
				mvaddstr(count++,0,"Spend how much gold? ");
				refresh();
				holdlong = get_number();
				curntn->tgold-=holdlong;
				if (curntn->tgold<0L) {
					curntn->tgold+=holdlong;
					tradeerr("You do not have enough gold");
				}
				else switch (holdint) {
				case GETFOOD:
					curntn->tfood+= (long)(GODFOOD * ((double)holdlong / GODPRICE));
					if ( (tfile = fopen(tradefile,"a+"))==NULL) {
						tradeerr("Error opening file for trading");
						abrt();
					}
					fprintf(tfile, "%d %d %d %d %ld %ld %d\n",BUY, country, GETFOOD, 0, curntn->tfood, 0, 0);
					fclose(tfile);
					break;
				case GETMETAL:
					curntn->metals+=(long)(GODMETAL * ((double)holdlong / GODPRICE));
					break;
				case GETJEWL:
					curntn->jewels+=(long)(GODMETAL * ((double)holdlong / GODPRICE));
					break;
				}
				inloop=FALSE;
				done=FALSE;
				break;
			} else if (holdint<1 || holdint>itemnum
				|| ntn[natn[holdint - 1]].dstatus[country]==UNMET) {
				tradeerr("Invalid Item Number");
				return;
			}
			holdint--;
			if (deal[holdint]!=SELL) {
				tradeerr("Sorry, that item is not on the market.");
				return;
			}
			if (ntn[natn[holdint]].dstatus[country]==UNMET) {
				tradeerr("That nation has not been met by you");
				return;
			}
			if (ntn[natn[holdint]].dstatus[country]>HOSTILE) {
				tradeerr("That nation is not doing business with you");
				return;
			}
			/* obtain bid */
			mvprintw(count++,0,"%s",buylist[type2[holdint]]);
			refresh();
			holdlong2 = 0L;
			holdlong = (long) get_number();
			if (holdlong< 0L) break;
			/* check for valid bid */
			switch(type2[holdint]) {
			case TDGOLD:
				if (holdlong < lvar2[holdint]) {
					tradeerr("You underbid the minimum.");
					buysell=NODEAL;
				} else if (holdlong > curntn->tgold) {
					tradeerr("Not Enough Gold");
					buysell=NODEAL;
				}
				break;
			case TDFOOD:
				if (holdlong < lvar2[holdint]) {
					tradeerr("You underbid the minimum.");
					buysell=NODEAL;
				} else if (holdlong > curntn->tfood) {
					tradeerr("Not Enough Food");
					buysell=NODEAL;
				}
				break;
			case TDMETAL:
				if (holdlong < lvar2[holdint]) {
					tradeerr("You underbid the minimum.");
					buysell=NODEAL;
				} else if (holdlong > curntn->metals) {
					tradeerr("Not Enough Iron");
					buysell=NODEAL;
				}
				break;
			case TDJEWL:
				if (holdlong < lvar2[holdint]) {
					tradeerr("You underbid the minimum.");
					buysell=NODEAL;
				} else if (holdlong > curntn->jewels) {
					tradeerr("Not Enough Jewels");
					buysell=NODEAL;
				}
				break;
			case TDLAND:
				mvaddstr(count++,0,"What Y position? ");
				refresh();
				holdlong2 = (long) get_number();
				if (holdlong2 < 0L) break;
				if (checkland(BUY,(int)(holdlong),(int)(holdlong2))==NODEAL) {
					buysell=NODEAL;
				} else if (tofood( &sct[(int)holdlong][(int)holdlong2],natn[holdint]) < lvar2[holdint]) {
					tradeerr("You underbid the minimum");
					buysell=NODEAL;
				}
				break;
			case TDARMY:
				if ((int)holdlong > MAXARM) {
					tradeerr("Invalid Unit");
					buysell=NODEAL;
				} else if (tradable(country,(int)holdlong)==FALSE) {
					tradeerr("That unit type is non-tradable.");
					buysell=NODEAL;
				} else if (armyvalue(country,(int)holdlong) < lvar2[holdint]) {
					tradeerr("You underbid the minimum.");
					buysell=NODEAL;
				}
				holdlong2=holdlong;
				break;
			case TDSHIP:
				if ((int)holdlong >= MAXNAVY) {
					tradeerr("Invalid Navy");
					buysell=NODEAL;
				} else if (flthold((int)holdlong) < (int)lvar2[holdint]) {
					tradeerr("You underbid the minimum.");
					buysell=NODEAL;
				} else if((curntn->nvy[(int)holdlong].armynum!=MAXARM)||(curntn->nvy[(int)holdlong].people!=0)) {
					tradeerr("Navy must be unloaded first.");
					buysell=NODEAL;
				}
				holdlong2=holdlong;
				break;
			default:
				tradeerr("Invalid Commodity");
				buysell=NODEAL;
				break;
			}
			if (buysell==BUY) {
				if ( (tfile = fopen(tradefile,"a+"))==NULL) {
					tradeerr("Error opening file for trading");
					abrt();
				}
				setaside(country,type2[holdint],holdlong,(int)holdlong,FALSE);
				fprintf(tfile, "%d %d %d %d %ld %ld %d\n",BUY, country, holdint, 0, holdlong, holdlong2, 0);
				fclose(tfile);
			}
			return;
		case 's':
		case 'S':
			/* sell an item */
			/* only allow MAXITM on market */
			if (itemnum>=MAXITM) {
				standout();
				if (itemnum==0) mvaddstr(count++,0,"Market Congested.  Hit any key to continue");
				else mvaddstr(count++,0,"Market Congested.  (B)uy or any key to continue");
				standend();
				refresh();
				break;
			}
			buysell=SELL;
			holdint = gettrade("Selling",&count);
			if (holdint==(-1)) {
				tradeerr("Invalid Option");
				return;
			}

			mvprintw(count++,0,"%s",buylist[holdint]);
			refresh();
			/* find out how much commodities */
			holdlong = (long) get_number();
			if (holdlong < 0) return;
			extint = 0;
			if (holdint< TDLAND && holdlong==0L)
				return;

			/* check for valid items */
			switch(holdint) {
			case TDGOLD:
				if (holdlong > curntn->tgold) {
					tradeerr("Not Enough Gold");
					buysell=NODEAL;
				}
				break;
			case TDFOOD:
				if (holdlong > curntn->tfood) {
					tradeerr("Not Enough Food");
					buysell=NODEAL;
				}
				break;
			case TDMETAL:
				if (holdlong > curntn->metals) {
					tradeerr("Not Enough Iron");
					buysell=NODEAL;
				}
				break;
			case TDJEWL:
				if (holdlong > curntn->jewels) {
					tradeerr("Not Enough Jewels");
					buysell=NODEAL;
				}
				break;
			case TDLAND:
				mvaddstr(count++,0,"What Y position? ");
				refresh();
				extint = get_number();
				if (extint < 0) {
					buysell=NODEAL;
					break;
				}
				buysell = checkland(SELL,(int)holdlong,extint);
				break;
			case TDARMY:
				if (holdlong>=MAXARM || curntn->arm[(int)holdlong].sold <= 0) {
					tradeerr("Invalid Army");
					buysell=NODEAL;
				} else if (tradable(country,(int)holdlong)==FALSE) {
					tradeerr("That unit is non-tradable.");
					buysell=NODEAL;
				}
				break;
			case TDSHIP:
				if (holdlong>=MAXNAVY || flthold((int)holdlong) <= 0) {
					tradeerr("Invalid Navy");
					buysell=NODEAL;
				} else if((curntn->nvy[(int)holdlong].armynum!=MAXARM)||(curntn->nvy[(int)holdlong].people!=0)) {
					tradeerr("Navy must be unloaded first.");
					buysell=NODEAL;
				}
				break;
			default:
				tradeerr("Invalid Commodity");
				buysell=NODEAL;
				break;
			}
			/* invalid commodity */
			if (buysell==NODEAL) return;

			/* find out what they want in trade */
			holdint2 = gettrade("In Trade For",&count);
			if (holdint2==(-1)) {
				tradeerr("Invalid Option");
				return;
			}
			if (holdint2==TDLAND) {
				holdlong2 = (long) getland(&count);
				if (holdlong2==(-1L)) {
					tradeerr("Invalid Vegetation");
					return;
				}
			} else {
				/* find out for what value */
				mvprintw(count++,0,"Minimum Amount of %s? "
					,commodities[holdint2]);
				refresh();
				holdlong2 = (long) get_number();
				if (holdlong2 <= 0L) return;
			}

			/* make sure what was bid is unusable */
			setaside(country,holdint,holdlong,(int)holdlong,FALSE);

			/* set up output properly */
			if (holdint==TDARMY) {
				extint = (int) holdlong;
				holdlong = armyvalue(country,(int)holdlong);
			}
			else if (holdint==TDSHIP) {
				extint = (int)holdlong;
				holdlong = (long)flthold(extint);
			}

			/* send it out */
			if ( (tfile = fopen(tradefile,"a+"))==NULL) {
				tradeerr("Error opening file for trading");
				abrt();
			}
			fprintf(tfile, "%d %d %d %d %ld %ld %d\n", SELL, country, holdint, holdint2, holdlong, holdlong2, extint);
			fclose(tfile);
			inloop=FALSE;
			done=FALSE;
			break;
		case 'u':
		case 'U':
			/* unsell an item */
			if (itemnum==0) break;
			mvaddstr(count++,0,"What item number to remove? ");
			refresh();
			holdint = get_number();
			if (holdint < 0) return;
			if (holdint==0 || holdint>itemnum) {
				tradeerr("Invalid Item Number");
				return;
			}
			holdint--;
#ifdef OGOD
			/* allow god to remove commodities */
			if (country!=0 && country!=natn[holdint]) 
#else 
			if (country != natn[holdint]) 
#endif /* OGOD */
			{
				tradeerr("That is not your item");
				return;
			}

			if (deal[holdint]!=SELL) {
				tradeerr("That item is not up for sale");
				return;
			}
			
			/* remove it from market */
			if ( (tfile = fopen(tradefile,"a+"))==NULL) {
				tradeerr("Error opening file for trading");
				abrt();
			}
			fprintf(tfile, "%d %d %d %d %ld %ld %d\n", NOSALE, natn[holdint], holdint, 0, 0L, 0L, 0);
			fclose(tfile);
			takeback(natn[holdint],type1[holdint],lvar1[holdint],extra[holdint],FALSE);
			/*redraw the commodities board so removal is seen*/
			inloop=FALSE;
			done=FALSE;
			break;
		default:
			/* return on no choice */
			return;
		}
	}
}

/*
 * tradeerr - Display trade error message with user acknowledgment
 *
 * Displays error messages during trading operations in a standardized format
 * at the bottom of the screen. Clears the bottom area, displays the error message
 * in standout (reverse video) mode, and waits for user acknowledgment before
 * returning control to the calling function.
 *
 * Error Display Format:
 * - Clears bottom portion of screen for clean message area
 * - Displays error message in standout mode (line 21)
 * - Shows "Hit any key to continue" prompt (line 22)
 * - Waits for single keypress before continuing
 *
 * Parameters:
 *   mesg - Error message string to display (must not be NULL)
 *
 * Returns: void
 *
 * Side Effects:
 *   - Modifies screen display (clears bottom, shows error message)
 *   - Blocks execution waiting for user keypress
 *   - Uses curses standout mode for emphasis
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires curses display system
 *   Approach: Integration testing with mock display functions
 *   Key Tests: Message display, screen formatting, user input handling
 *   Dependencies: Curses library, display state
 *   Mock Requirements: Curses functions (mvaddstr, standout, getch)
 *   Complexity: Simple - Basic display function with minimal logic
 *
 * Notes:
 *   - Used by all trade functions for consistent error reporting
 *   - Hardcoded screen positions (lines 21-22) assume standard terminal size
 *   - Blocking function that pauses game until user acknowledgment
 */
void
tradeerr(mesg)
char *mesg;
{
	clear_bottom(0);
	standout();
	mvaddstr(21,0,mesg);
	standend();
	mvaddstr(22,0,"Hit any key to continue");
	refresh();
	getch();
}

/*
 * checkland - Validate land sector for trading operations
 *
 * Performs comprehensive validation of a land sector to determine if it can
 * be legally traded. Checks map boundaries, ownership, special designations,
 * and capital restrictions to ensure only valid land can be bought or sold
 * in the commodities exchange.
 *
 * Validation Checks:
 * - Map boundaries: Coordinates must be within valid map area
 * - Ownership: Current player must own the sector for selling
 * - Capital protection: Capital sector cannot be traded
 * - Town/city protection: Towns and cities cannot be sold
 * - All restrictions applied regardless of trade direction
 *
 * Error Handling:
 * - Each validation failure displays specific error message
 * - Returns NODEAL status for any validation failure
 * - Preserves original status if all validations pass
 *
 * Parameters:
 *   tradestat - Current trade operation status (BUY, SELL, etc.)
 *   xspot - X coordinate of land sector to validate
 *   yspot - Y coordinate of land sector to validate
 *
 * Returns:
 *   Original tradestat if sector is valid for trading
 *   NODEAL if sector fails any validation check
 *
 * Side Effects:
 *   - May display error messages via tradeerr() for validation failures
 *   - No game state modifications
 *
 * Testing Notes:
 *   Category: A (Unit) - Testable with mock sector data
 *   Approach: Unit testing with various sector configurations
 *   Key Tests: Boundary checks, ownership validation, designation restrictions
 *   Dependencies: Global sector array, nation data, current player
 *   Mock Requirements: Sector data, nation capital coordinates
 *   Complexity: Simple - Sequential validation with clear error conditions
 *
 * Notes:
 *   - Core validation function for land trading system
 *   - Prevents exploitation of capital and city trading
 *   - Used by both buy and sell operations
 */
int
checkland(tradestat,xspot,yspot)
int tradestat,xspot,yspot;
{
	int newstat=tradestat;
	if (!ONMAP(xspot,yspot)) {
		tradeerr("That is off the map");
		newstat=NODEAL;
	} 
	else if (sct[xspot][yspot].owner != country) {
		tradeerr("You don't own it");
		newstat=NODEAL;
	}
	else if (curntn->capx==xspot && curntn->capy==yspot) {
		tradeerr("That is your capitol");
		newstat=NODEAL;
	}
	else if (sct[xspot][yspot].designation == DTOWN) {
		tradeerr("Towns may not be sold");
		newstat=NODEAL;
	}
	else if (sct[xspot][yspot].designation == DCITY) {
		tradeerr("Cities may not be sold");
		newstat=NODEAL;
	}
	return(newstat);
}

/*
 * getland - Interactive land vegetation selection for minimum trade value
 *
 * Provides interactive interface for players to specify minimum vegetation
 * requirements when trading land. Displays all available vegetation types,
 * accepts user selection, and calculates the corresponding food value for
 * that vegetation type to establish minimum trade requirements.
 *
 * Vegetation Types Supported:
 * - Natural: Volcano, Desert, Tundra, Barren, Light Vegetation
 * - Productive: Good land, Wood, Forest, Jungle, Swamp
 * - Special: Ice, None (empty land)
 * - Each type has different food production values
 *
 * Selection Process:
 * - Displays prompt with all valid vegetation characters
 * - Accepts single character input from user
 * - Validates input against known vegetation types
 * - Finds first sector with matching vegetation
 * - Calculates food value for that vegetation/nation combination
 * - Displays selected vegetation and calculated food value
 *
 * Parameters:
 *   count - Pointer to current line counter for display positioning
 *
 * Returns:
 *   Food value for selected vegetation type, or -1 if invalid selection
 *
 * Side Effects:
 *   - Increments count for display line positioning
 *   - Displays vegetation selection prompt and confirmation
 *   - Scans entire map to find vegetation example
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires map data and display system
 *   Approach: Integration testing with mock map and vegetation data
 *   Key Tests: Vegetation validation, food calculation, input handling
 *   Dependencies: Global map array, tofood() function, curses display
 *   Mock Requirements: Map data, vegetation constants, display functions
 *   Complexity: Moderate - Input validation with map scanning algorithm
 *
 * Notes:
 *   - Used for land trading to establish minimum food value requirements
 *   - Breaks out of nested loops using i=MAPX+1, j=MAPY+1 technique
 *   - Food value depends on both vegetation type and nation context
 */
int
getland(count)
int *count;
{
	int	temp;
	int	i,j;
	char	entered;

	mvprintw((*count)++,0,"MINIMUM VEGETATION: %c, %c, %c, %c, %c, %c, %c, %c, %c, %c, %c or %c: ",
		VOLCANO,DESERT,TUNDRA,BARREN,LT_VEG,
		GOOD,WOOD,FOREST,JUNGLE,SWAMP,ICE,NONE);

	refresh();
	entered=getch();
	if(entered!=VOLCANO       &&entered!=JUNGLE
		&&entered!=DESERT &&entered!=TUNDRA
		&&entered!=BARREN &&entered!=LT_VEG
		&&entered!=NONE   &&entered!=GOOD
		&&entered!=WOOD   &&entered!=FOREST
		&&entered!=SWAMP  &&entered!=ICE) temp=(-1);
	else {
		for(i=0;i<MAPX;i++)
		for(j=0;j<MAPY;j++) if(sct[i][j].vegetation == entered) {
			temp = tofood( &sct[i][j],country );
			i=MAPX+1;
			j=MAPY+1;
		}
	}
	mvprintw((*count)++,0," JUST ENTERED %c so food value is %d", entered,temp);
	refresh();
	return(temp);
}

/*
 * gettrade - Interactive commodity type selection for trading operations
 *
 * Provides a standardized interface for players to select commodity types
 * during trading operations. Displays available commodity options and accepts
 * single-character input to determine which type of resource the player wants
 * to trade (buy, sell, or specify as payment).
 *
 * Supported Commodities:
 * - (G)old: Primary currency for most transactions
 * - (F)ood: Basic resource for population sustenance
 * - (I)ron: Metal resource for equipment and construction
 * - (J)ewels: Luxury commodity for high-value trades
 * - (L)and: Territory sectors with varying productivity
 * - (A)rmy: Military units (mercenaries, siege engines, etc.)
 * - (S)hips: Naval units (merchants, warships, galleys)
 *
 * Interface Operation:
 * - Displays customizable prompt with commodity options
 * - Accepts case-insensitive single character input
 * - Maps character input to internal commodity constants
 * - Returns appropriate TD* constant for selected commodity
 *
 * Parameters:
 *   saletype - Context string describing the operation ("Selling", "In Trade For", etc.)
 *   count - Pointer to current line counter for display positioning
 *
 * Returns:
 *   Commodity type constant (TDGOLD, TDFOOD, TDMETAL, TDJEWL, TDLAND, TDARMY, TDSHIP)
 *   -1 if invalid or unrecognized input
 *
 * Side Effects:
 *   - Increments count for display line positioning
 *   - Displays commodity selection prompt
 *   - Waits for user input
 *
 * Testing Notes:
 *   Category: A (Unit) - Simple input mapping function
 *   Approach: Unit testing with various input characters
 *   Key Tests: Valid inputs, invalid inputs, case sensitivity
 *   Dependencies: Display system for prompts
 *   Mock Requirements: getch() function for input
 *   Complexity: Simple - Direct character to constant mapping
 *
 * Notes:
 *   - Used by multiple trading functions for consistent commodity selection
 *   - Case-insensitive input handling for user convenience
 *   - Returns -1 for any unrecognized input to signal error condition
 */
int
gettrade(saletype,count)
char *saletype;
int *count;
{
	int hold=(-1);

	mvprintw((*count)++,0,"%s: (G)old, (F)ood, (I)ron, (J)ewels, (L)and, (A)rmy, (S)hips?",saletype);
	refresh();
	switch(getch()) {
	case 'g':
	case 'G':
		hold=TDGOLD;
		break;
	case 'f':
	case 'F':
		hold=TDFOOD;
		break;
	case 'i':
	case 'I':
		hold=TDMETAL;
		break;
	case 'j':
	case 'J':
		hold=TDJEWL;
		break;
	case 'l':
	case 'L':
		hold=TDLAND;
		break;
	case 'a':
	case 'A':
		hold=TDARMY;
		break;
	case 's':
	case 'S':
		hold=TDSHIP;
		break;
	default:
		break;
	}
	return(hold);
}
#endif /* CONQUER */

/*
 * setaside - Reserve commodities for active trading operations
 *
 * Temporarily removes commodities from a nation's available resources when
 * they are placed up for sale or offered as bids in trading operations.
 * This prevents double-spending and ensures committed resources cannot be
 * used elsewhere while trade is pending.
 *
 * Commodity Handling by Type:
 * - Gold/Metal/Jewels: Deducted from nation totals (conditional on isup flag)
 * - Food: Always deducted regardless of isup flag
 * - Land: No reservation needed (ownership transfer handled elsewhere)
 * - Army: Sets movement to 0 and status to TRADED
 * - Ships: Sets movement to 0 and commodity flag to TRADED
 *
 * Status Management:
 * - Armies marked as TRADED cannot move or be used in combat
 * - Ships marked as TRADED cannot move or participate in naval operations
 * - Resources are held in "escrow" until trade completes or is cancelled
 *
 * Parameters:
 *   cntry - Nation index whose commodities are being reserved
 *   item - Commodity type constant (TDGOLD, TDFOOD, etc.)
 *   longval - Quantity/amount of commodity to reserve
 *   extint - Unit index for armies/ships, unused for other commodities
 *   isup - Flag indicating if this is an "up for trade" operation (affects gold/metal/jewels)
 *
 * Returns: void
 *
 * Side Effects:
 *   - Modifies nation resource totals (gold, food, metal, jewels)
 *   - Changes army/ship status and movement allowances
 *   - Affects game state until commodity is returned via takeback()
 *
 * Testing Notes:
 *   Category: A (Unit) - Testable with mock nation data
 *   Approach: Unit testing with various commodity types and amounts
 *   Key Tests: Resource deduction, unit status changes, isup flag behavior
 *   Dependencies: Nation array structure, commodity constants
 *   Mock Requirements: Nation data structure, army/ship arrays
 *   Complexity: Simple - Switch statement with direct state modifications
 *
 * Notes:
 *   - Paired with takeback() to implement trade reservation system
 *   - isup flag creates conditional behavior for some commodity types
 *   - TRADED status prevents units from being used while reserved
 */
void
setaside(cntry,item,longval,extint,isup)
int cntry,item,isup,extint;
long longval;
{
	switch(item)
	{
	case TDGOLD:
		if (isup==FALSE) ntn[cntry].tgold -= longval;
		break;
	case TDFOOD:
		ntn[cntry].tfood -= longval;
		break;
	case TDMETAL:
		if (isup==FALSE) ntn[cntry].metals -= longval;
		break;
	case TDJEWL:
		if (isup==FALSE) ntn[cntry].jewels -= longval;
		break;
	case TDLAND:
		break;
	case TDARMY:
		ntn[cntry].arm[extint].smove = 0;
		ntn[cntry].arm[extint].stat = TRADED;
		break;
	case TDSHIP:
		/* use commodity to hold indicator */
		ntn[cntry].nvy[extint].smove = 0;
		ntn[cntry].nvy[extint].commodity = TRADED;
		break;
	}
}

/*
 * takeback - Return reserved commodities to nation's available resources
 *
 * Reverses the effects of setaside() by returning previously reserved commodities
 * back to a nation's available resources when trades are cancelled, fail, or
 * when unselling items from the marketplace. This restores normal access to
 * resources and units that were temporarily locked for trading.
 *
 * Commodity Restoration by Type:
 * - Gold/Metal/Jewels: Added back to nation totals (conditional on isup flag)
 * - Food: Always restored regardless of isup flag
 * - Land: No restoration needed (handled by ownership transfer)
 * - Army: Status reset to DEFEND, remains immobilized
 * - Ships: Commodity flag cleared to 0, remains immobilized
 *
 * Error Handling:
 * - Early return if cntry == -1 (invalid nation)
 * - Graceful handling of restoration for all commodity types
 * - Maintains consistency with setaside() parameter patterns
 *
 * Parameters:
 *   cntry - Nation index receiving back the commodities (-1 for none)
 *   item - Commodity type constant (TDGOLD, TDFOOD, etc.)
 *   longval - Quantity/amount of commodity to restore
 *   extint - Unit index for armies/ships, unused for other commodities
 *   isup - Flag indicating operation type (affects gold/metal/jewels restoration)
 *
 * Returns: void
 *
 * Side Effects:
 *   - Restores nation resource totals (gold, food, metal, jewels)
 *   - Resets army status from TRADED to DEFEND
 *   - Clears ship commodity trading flag
 *   - Makes resources available for other operations
 *
 * Testing Notes:
 *   Category: A (Unit) - Testable with mock nation data
 *   Approach: Unit testing with various commodity types and restoration scenarios
 *   Key Tests: Resource restoration, unit status reset, isup flag behavior, error handling
 *   Dependencies: Nation array structure, commodity constants
 *   Mock Requirements: Nation data structure, army/ship arrays
 *   Complexity: Simple - Mirror logic of setaside() with inverse operations
 *
 * Notes:
 *   - Paired with setaside() to implement complete trade reservation system
 *   - Essential for trade cancellation and marketplace unselling operations
 *   - Units remain immobilized until next turn despite status restoration
 */
void
takeback(cntry,item,longval,extint,isup)
int cntry,item,isup,extint;
long longval;
{
	if (cntry == -1) return;
	switch(item)
	{
	case TDGOLD:
		if (isup==FALSE) ntn[cntry].tgold += longval;
		break;
	case TDFOOD:
		ntn[cntry].tfood += longval;
		break;
	case TDMETAL:
		if (isup==FALSE) ntn[cntry].metals += longval;
		break;
	case TDJEWL:
		if (isup==FALSE) ntn[cntry].jewels += longval;
		break;
	case TDLAND:
		break;
	case TDARMY:
		ntn[cntry].arm[extint].stat = DEFEND;
		break;
	case TDSHIP:
		/* use commodity to hold indicator */
		ntn[cntry].nvy[extint].commodity = 0;
		break;
	}
}

#ifdef ADMIN
/*
 * tradeit - Execute actual commodity transfer between nations (ADMIN only)
 *
 * Implements the core commodity transfer mechanics for completed trades by
 * moving resources, units, or land ownership from seller (cntry1) to buyer
 * (cntry2). Applies trading costs/fees and handles all commodity types with
 * appropriate validation and unit management.
 *
 * Transfer Operations by Commodity:
 * - Gold/Food/Metal/Jewels: Direct transfer with 20% trading cost applied
 * - Land: Ownership transfer with validation checks
 * - Army: Unit transfer with capital placement and validation
 * - Ships: Fleet transfer with location preservation and validation
 *
 * Trading Economics:
 * - TRADECOST(20) macro applies 80% efficiency (20% trading fee)
 * - Fee represents market transaction costs and intermediary charges
 * - Buyer receives reduced quantity due to trading overhead
 *
 * Unit Transfer Management:
 * - Armies: Transferred to buyer's capital, status reset to DEFEND
 * - Ships: Transferred with original location, commodity flag cleared
 * - Finds first available unit slot for buyer
 * - Clears original unit from seller's roster
 *
 * Parameters:
 *   cntry1 - Selling nation index
 *   cntry2 - Buying nation index
 *   item - Commodity type constant (TDGOLD, TDFOOD, etc.)
 *   longval - Quantity/amount being transferred
 *   extra - Additional data (unit index for armies/ships, Y coord for land)
 *
 * Returns:
 *   Assigned unit number for armies/ships transfers, or longval for other commodities
 *   -1 if transfer fails due to validation or availability issues
 *
 * Side Effects:
 *   - Modifies nation resource totals with trading costs applied
 *   - Transfers land ownership between nations
 *   - Moves military/naval units between nation rosters
 *   - Updates unit status and location data
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires nation data and complex state management
 *   Approach: Integration testing with mock nation and unit data
 *   Key Tests: All commodity transfers, cost calculations, unit management, error conditions
 *   Dependencies: Nation arrays, map data, unit management system
 *   Mock Requirements: Complete nation state, army/ship data structures
 *   Complexity: Complex - Multi-branch logic with extensive state modifications
 *
 * Notes:
 *   - ADMIN-only function for turn processing and administrative operations
 *   - Critical for maintaining game economy balance with trading costs
 *   - Handles complex unit roster management for military transfers
 */
long
tradeit(cntry1,cntry2,item,longval,extra)
int cntry1,cntry2,item,extra;
long longval;
{
	int unitnum=(-1),unitcount=0;
	/* error for -1 returned */
	long returnval=(-1);
	switch(item)
	{
	case TDGOLD:
		returnval = longval;
		ntn[cntry2].tgold += longval * TRADECOST(20);
		break;
	case TDFOOD:
		returnval = longval;
		ntn[cntry2].tfood += longval * TRADECOST(20);
		break;
	case TDMETAL:
		ntn[cntry2].metals += longval * TRADECOST(20);
		returnval = longval;
		break;
	case TDJEWL:
		ntn[cntry2].jewels += longval * TRADECOST(20);
		returnval = longval;
		break;
	case TDLAND:
		if (sct[(int)longval][extra].owner==cntry1) {
			sct[(int)longval][extra].owner = cntry2;
			returnval = longval;
		}
		break;
	case TDARMY:
		/* find army number for cntry2 */
		/* give army to cntry2 */
		if(ntn[cntry1].arm[extra].sold <= 0) return -1;
		while(unitnum==(-1)&&unitcount<MAXARM) {
			if (ntn[cntry2].arm[unitcount].sold<=0) {
				/* give army to cntry2 */
				ntn[cntry2].arm[unitcount].sold = ntn[cntry1].arm[extra].sold;
				ntn[cntry2].arm[unitcount].unittyp = ntn[cntry1].arm[extra].unittyp;
				ntn[cntry2].arm[unitcount].xloc = ntn[cntry2].capx;
				ntn[cntry2].arm[unitcount].yloc = ntn[cntry2].capy;
				ntn[cntry2].arm[unitcount].stat = DEFEND;
				ntn[cntry2].arm[unitcount].smove = 0;
				/* remove army from cntry1 */
				ntn[cntry1].arm[extra].sold = 0;
				ntn[cntry1].arm[extra].smove = 0;
				ntn[cntry1].arm[extra].stat = DEFEND;
				unitnum=unitcount;
			}
			unitcount++;
		}
		returnval=(long)unitnum;
		break;
	case TDSHIP:
		/* give navy to cntry1 */
		if(ntn[cntry1].nvy[extra].merchant==0
		   && ntn[cntry1].nvy[extra].warships==0
		   && ntn[cntry1].nvy[extra].galleys==0) return -1;
		while(unitnum==(-1)&&unitcount<MAXARM){
			if ((int)ntn[cntry2].nvy[unitcount].merchant+ntn[cntry2].nvy[unitcount].warships+ntn[cntry2].nvy[unitcount].galleys == 0) {
				/* give navy to cntry2 */
				ntn[cntry2].nvy[unitcount].warships = ntn[cntry1].nvy[extra].warships;
				ntn[cntry2].nvy[unitcount].merchant = ntn[cntry1].nvy[extra].merchant;
				ntn[cntry2].nvy[unitcount].galleys = ntn[cntry1].nvy[extra].galleys;
				ntn[cntry2].nvy[unitcount].crew = ntn[cntry1].nvy[extra].crew;
				ntn[cntry2].nvy[unitcount].xloc = ntn[cntry1].nvy[extra].xloc;
				ntn[cntry2].nvy[unitcount].yloc = ntn[cntry1].nvy[extra].yloc;
				ntn[cntry2].nvy[unitcount].commodity = 0;
				ntn[cntry2].nvy[unitcount].smove = 0;
				/* remove navy from cntry1 */
				ntn[cntry1].nvy[extra].smove = 0;
				ntn[cntry1].nvy[extra].merchant = 0;
				ntn[cntry1].nvy[extra].warships = 0;
				ntn[cntry1].nvy[extra].galleys = 0;
				ntn[cntry1].nvy[extra].crew = 0;
				ntn[cntry1].nvy[extra].commodity = 0;
				unitnum=unitcount;
			}
			unitcount++;
		}
		returnval=(long)unitnum;
		break;
	}
	return(returnval);
}

/*
 * gettval - Calculate actual trade value for commodities in transactions
 *
 * Evaluates the real trade value of commodities during trade processing,
 * taking into account current ownership, unit status, and contextual factors.
 * Used to determine if bids meet minimum requirements and to establish
 * actual transfer values for completed trades.
 *
 * Value Calculation by Commodity Type:
 * - Gold/Food/Metal/Jewels: Direct value passthrough (longval)
 * - Land: Food production value based on current ownership and nation context
 * - Army: Military value calculation if unit exists and has positive value
 * - Ships: Fleet cargo capacity if unit exists and has positive capacity
 *
 * Ownership Validation:
 * - Land: Must be owned by cntry2 to have valid trade value
 * - Army: Must belong to cntry2 and have positive armyvalue()
 * - Ships: Must belong to cntry2 and have positive flthold() capacity
 * - Prevents trading of invalid or non-existent assets
 *
 * Context Switching:
 * - Temporarily switches curntn for ship capacity calculations
 * - Ensures calculations use correct nation context
 * - Restores original context after calculation
 *
 * Parameters:
 *   cntry1 - Source nation (seller) for context
 *   cntry2 - Target nation (owner) for ownership validation
 *   type - Commodity type constant (TDGOLD, TDFOOD, etc.)
 *   longval - Base value or coordinate for calculation
 *   extint - Unit index or additional coordinate data
 *
 * Returns:
 *   Calculated trade value for the commodity, or -1 if invalid/unavailable
 *
 * Side Effects:
 *   - Temporarily modifies curntn global for ship calculations
 *   - No permanent state changes
 *
 * Testing Notes:
 *   Category: A (Unit) - Testable with mock nation and unit data
 *   Approach: Unit testing with various commodity types and ownership scenarios
 *   Key Tests: Value calculations, ownership validation, context switching
 *   Dependencies: Nation data, unit arrays, tofood(), armyvalue(), flthold()
 *   Mock Requirements: Nation state, army/ship data, map data for land
 *   Complexity: Moderate - Multi-branch logic with context management
 *
 * Notes:
 *   - Essential for trade validation and bid comparison
 *   - Handles edge cases where units/land may not exist or be invalid
 *   - Used by both trade processing and marketplace bid evaluation
 */
long
gettval(int cntry1,int cntry2,int type,long longval,int extint)
{
	int returnval=(-1);
	long armyvalue();

	switch(type) {
	case TDGOLD:
	case TDFOOD:
	case TDMETAL:
	case TDJEWL:
		returnval=longval;
		break;
	case TDLAND:
		if (cntry2 == sct[(int)longval][extint].owner)
		returnval=(long)tofood( &sct[(int)longval][extint],cntry1);
		break;
	case TDARMY:
		if (armyvalue(cntry2,extint)>0)
		returnval=armyvalue(cntry2,extint);
		break;
	case TDSHIP:
		curntn = &ntn[cntry2];
		if (flthold(extint)>0)
		returnval = (long)flthold(extint);
		break;
	}
	return(returnval);
}

/*
 * trademail - Send detailed trade completion notifications to both parties (ADMIN only)
 *
 * Generates and sends comprehensive trade completion messages to both the seller
 * and buyer nations when a trade transaction is successfully completed. Creates
 * formal notification files with transaction details, timestamps, and complete
 * commodity transfer information for both parties.
 *
 * Message Content Structure:
 * - Official header from "Conquer Commerce Commission"
 * - Date stamp with current season and year
 * - Complete transaction details for both sides
 * - Commodity descriptions with quantities/locations
 * - Standardized formatting for all trade types
 *
 * File Management:
 * - Creates separate message files for each nation
 * - Appends to existing message files if present
 * - Uses nation-specific filenames (msgfile + nation number)
 * - Handles file creation errors with program termination
 *
 * Commodity Display Formatting:
 * - Basic resources: Shows quantity and commodity name
 * - Land: Shows sector coordinates (X, Y format)
 * - Military units: Shows assigned unit numbers
 * - Consistent formatting across all transaction types
 *
 * Parameters:
 *   cntry1 - Seller nation index
 *   cntry2 - Buyer nation index
 *   item1 - Commodity type sold by cntry1
 *   item2 - Commodity type received by cntry1 (payment)
 *   lvar1 - Amount/unit number received by cntry2
 *   lvar2 - Extra data for item1 (coordinates, etc.)
 *   lvar3 - Amount/unit number received by cntry1
 *   lvar4 - Extra data for item2 (coordinates, etc.)
 *
 * Returns: void
 *
 * Side Effects:
 *   - Creates/modifies nation message files
 *   - May terminate program on file I/O errors
 *   - Generates permanent trade records
 *
 * Testing Notes:
 *   Category: C (System) - Requires file system and message infrastructure
 *   Approach: System testing with mock file system and trade data
 *   Key Tests: File creation, message formatting, error handling, all commodity types
 *   Dependencies: File system, nation data, message file system
 *   Mock Requirements: File I/O functions, nation arrays, global constants
 *   Complexity: Moderate - File handling with formatted output generation
 *
 * Notes:
 *   - ADMIN-only function for turn processing
 *   - Provides audit trail for all completed trades
 *   - Critical for player communication and game transparency
 *   - Terminates program on file errors to prevent silent failures
 */
void
trademail(cntry1,cntry2,item1,item2,lvar1,lvar2,lvar3,lvar4)
int cntry1,cntry2,item1,item2;
long lvar1,lvar2,lvar3,lvar4;
{
	FILE *fp[2];
	int count;
	char cname[2][NAMELTH+1],filename[2][FILELTH];

	sprintf(filename[0],"%s%d",msgfile,cntry1);
	sprintf(filename[1],"%s%d",msgfile,cntry2);
	strcpy(cname[0],ntn[cntry1].name);
	strcpy(cname[1],ntn[cntry2].name);

	if ((fp[0]=fopen(filename[0],"a+"))==NULL) {
		printf("error opening <%s>\n",filename[0]);
		abrt();
	}
	if ((fp[1]=fopen(filename[1],"a+"))==NULL) {
		printf("error opening <%s>\n",filename[1]);
		abrt();
	}

	for (count=0;count<2;count++) {
		fprintf(fp[count],"Message to %s from Conquer Commerce Commision\n",cname[count]);
		fprintf(fp[count],"Dated: %s of Year %d\n",PSEASON(TURN),YEAR(TURN));
		fprintf(fp[count],"\n");
		fprintf(fp[count],"   Trade transaction between %s and %s completed.\n",cname[1],cname[0]);
		if (item1<=TDJEWL)
		fprintf(fp[count],"       Nation %s receives %ld %s\n",cname[1],lvar1,commodities[item1]);
		else if (item1==TDLAND)
		fprintf(fp[count],"       Nation %s receives sector %ld, %ld\n",cname[1],lvar1,lvar2);
		else if (item1==TDARMY)
		fprintf(fp[count],"       Nation %s receives army #%ld\n",cname[1],lvar1);
		else if (item1==TDSHIP)
		fprintf(fp[count],"       Nation %s receives navy #%ld\n",cname[1],lvar1);
		if (item2<=TDJEWL)
		fprintf(fp[count],"       Nation %s receives %ld %s\n",cname[0],lvar3,commodities[item2]);
		else if (item2==TDLAND)
		fprintf(fp[count],"       Nation %s receives sector %ld, %ld\n",cname[0],lvar3,lvar4);
		else if (item2==TDARMY)
		fprintf(fp[count],"       Nation %s receives army #%ld\n",cname[0],lvar3);
		else if (item2==TDSHIP)
		fprintf(fp[count],"       Nation %s receives navy #%ld\n",cname[0],lvar3);
		fprintf(fp[count],"END\n");
		fclose(fp[count]);
	}
}
#endif /* ADMIN */

#ifdef CONQUER
/*
 * tradable - Determine if an army unit is eligible for trading operations
 *
 * Evaluates whether a specific army unit can be legally traded in the
 * commodities exchange based on unit type, status, and trading restrictions.
 * Only certain unit types are allowed to be traded, and units must not be
 * in special states that prevent trading.
 *
 * Tradable Unit Types:
 * - A_MERCENARY: Professional soldiers available for hire
 * - A_SIEGE: Siege engines and equipment
 * - A_CATAPULT: Artillery and bombardment units
 * - A_ELEPHANT: War elephants and exotic units
 * - MINMONSTER and higher: Special creatures and monsters
 *
 * Trading Restrictions:
 * - Unit must not be in TRADED status (already reserved)
 * - Unit must not be ONBOARD ships (cannot trade loaded units)
 * - Regular troops and militia cannot be traded
 * - Special units and creatures are tradable
 *
 * Context Switching:
 * - Temporarily switches global country to specified nation
 * - Uses ASTAT and ATYPE macros for current context
 * - Restores original country context after evaluation
 * - Ensures evaluation uses correct nation's unit data
 *
 * Parameters:
 *   cntry - Nation index that owns the army unit
 *   armynum - Army unit index to evaluate for tradability
 *
 * Returns:
 *   TRUE if unit is eligible for trading, FALSE otherwise
 *
 * Side Effects:
 *   - Temporarily modifies global country variable
 *   - Restores original country context before returning
 *
 * Testing Notes:
 *   Category: A (Unit) - Testable with mock army data
 *   Approach: Unit testing with various unit types and statuses
 *   Key Tests: Tradable types, restricted statuses, context switching
 *   Dependencies: Global country variable, army data structures
 *   Mock Requirements: Army data, unit type constants, status constants
 *   Complexity: Simple - Status checks with context management
 *
 * Notes:
 *   - Used by trading interface to validate unit availability
 *   - Prevents trading of regular troops to maintain game balance
 *   - Context switching ensures accurate unit status evaluation
 */
int
tradable(cntry,armynum)
int cntry,armynum;
{
	int oldcntry=country,returnval=FALSE;
	country=cntry;
	if ( (ASTAT!=TRADED) && (ASTAT!=ONBOARD) && (ATYPE==A_MERCENARY
		|| ATYPE==A_SIEGE || ATYPE==A_CATAPULT
		|| ATYPE==A_ELEPHANT || ATYPE>=MINMONSTER) ) returnval=TRUE;
	country=oldcntry;
	return(returnval);
}
#endif /* CONQUER */

/*
 * armyvalue - Calculate commercial trading value of army units
 *
 * Determines the market value of army units for trading purposes by
 * combining base unit count with combat effectiveness and special
 * bonuses. The calculation factors in unit size, attack strength,
 * and special creature bonuses to establish fair trading values.
 *
 * Value Calculation Formula:
 * 1. Base value: unit_count * 100 (size factor)
 * 2. Combat bonus: unit_count * attack_strength (effectiveness factor)
 * 3. Monster bonus: unit_count * 10 (if unit >= MINMONSTER)
 * 4. Final value: (base + combat + monster) / 100 (scaling factor)
 *
 * Unit Value Factors:
 * - sold: Number of soldiers/creatures in the unit
 * - unitattack[unittyp%UTYPE]: Attack strength from unit type table
 * - MINMONSTER threshold: Special creatures get additional value
 * - 100-based scaling: Prevents excessive inflation of unit values
 *
 * Monster Premium:
 * - Units with type >= MINMONSTER receive 10% bonus per soldier
 * - Reflects rarity and special abilities of monster units
 * - Dragons, demons, and other creatures command premium prices
 *
 * Parameters:
 *   cntry - Nation index that owns the army unit
 *   unit - Army unit index to evaluate for trading value
 *
 * Returns:
 *   Calculated commercial value of the army unit for trading
 *
 * Side Effects:
 *   - No game state modifications
 *   - Read-only evaluation of unit data
 *
 * Testing Notes:
 *   Category: A (Unit) - Testable with mock army data
 *   Approach: Unit testing with various unit types and sizes
 *   Key Tests: Value calculations, monster bonuses, scaling factors
 *   Dependencies: Nation army arrays, unitattack table, unit type constants
 *   Mock Requirements: Army data structures, unit attack values
 *   Complexity: Simple - Mathematical calculation with table lookup
 *
 * Notes:
 *   - Used for both bid validation and trade completion
 *   - Essential for establishing fair market prices for military units
 *   - Scaling factor prevents unrealistic unit values in trading
 */
long armyvalue(cntry,unit)
int cntry,unit;
{
	long returnval;
	
	returnval = ntn[cntry].arm[unit].sold*100 +
		ntn[cntry].arm[unit].sold * unitattack[ntn[cntry].arm[unit].unittyp%UTYPE];
	if (ntn[cntry].arm[unit].unittyp >= MINMONSTER) returnval+=ntn[cntry].arm[unit].sold*10;
	returnval/=100;
	return(returnval);
}

/*
 * checktrade - Update current player's trade status from trade file
 *
 * Reads the trade file and updates the current player's game state to reflect
 * their active trading operations. Processes trade file entries to restore
 * reserved commodities for items the player has for sale and to apply
 * purchases from god merchants. Essential for maintaining consistent trade
 * state across game sessions.
 *
 * Trade File Processing:
 * - NOSALE entries: Return reserved commodities to player if they own them
 * - SELL entries: Reserve commodities that player has listed for sale
 * - BUY entries: Apply god merchant purchases or reserve bid commodities
 * - Processes only entries belonging to current player (country)
 *
 * State Synchronization:
 * - Restores commodity reservations for active sales
 * - Updates food totals from god merchant purchases
 * - Maintains consistency between file and game state
 * - Handles missing or corrupted trade file gracefully
 *
 * File Format:
 * - Seven fields per line: deal, nation, type1, type2, lvar1, lvar2, extra
 * - Processes until EOF or format error encountered
 * - Initializes deal array to prevent processing garbage data
 *
 * Parameters: None (uses global country and game state)
 *
 * Returns: void
 *
 * Side Effects:
 *   - Modifies player resource totals and unit status
 *   - Updates commodity reservations via setaside()/takeback()
 *   - May update food totals from god purchases
 *   - No effect if trade file doesn't exist
 *
 * Testing Notes:
 *   Category: C (System) - Requires file system and full game state
 *   Approach: System testing with mock trade files and nation data
 *   Key Tests: File processing, state updates, error handling, all trade types
 *   Dependencies: Trade file, nation data, setaside()/takeback() functions
 *   Mock Requirements: File system, complete game state, trade file format
 *   Complexity: Moderate - File parsing with conditional state updates
 *
 * Notes:
 *   - Called during player login to restore trade state
 *   - Essential for persistent trading across game sessions
 *   - Gracefully handles missing trade file (no operations active)
 */
void
checktrade()
{
	FILE *tfile;
	int count, itemnum=0, natn[MAXITM];
	int type1[MAXITM], type2[MAXITM], deal[MAXITM], extra[MAXITM];
	long lvar1[MAXITM], lvar2[MAXITM];
	void takeback();
	void setaside();
	/* initialize purchase list */
	for (count=0; count<MAXITM; count++) {
		deal[count]=(-1);
	}
	/* open trading file */
	if ((tfile = fopen(tradefile,"r")) == NULL) {
		/* no commodities - no transactions */
		return;
	}
	/* read in all of the transactions */
	while(!feof(tfile)) {
		if (7 != fscanf(tfile,"%d %d %d %d %ld %ld %d\n",&deal[itemnum],
			&natn[itemnum],&type1[itemnum],&type2[itemnum],&lvar1[itemnum],&lvar2[itemnum],&extra[itemnum])) break;
		if (deal[itemnum]==NOSALE) {
			if (natn[itemnum]==country)
			takeback(country,type1[type1[itemnum]],lvar1[type1[itemnum]],extra[type1[itemnum]],TRUE);
		} else if (deal[itemnum]==SELL) {
			if (natn[itemnum]==country)
			setaside(country,type1[itemnum],lvar1[itemnum],extra[itemnum],TRUE);
			itemnum++;
		} else if (deal[itemnum]==BUY) {
			if (natn[itemnum]==country) {
				if (type1[itemnum]==GETFOOD) ntn[country].tfood=lvar1[itemnum];
				else setaside(country,type2[type1[itemnum]],lvar1[itemnum],(int)lvar1[itemnum],TRUE);
			}
		}
	}
	fclose(tfile);
}

#ifdef ADMIN
/*
 * uptrade - Process all trading transactions during turn update (ADMIN only)
 *
 * Core trade processing function that handles all pending trading operations
 * during turn update. Reads trade file, processes bids, executes trades,
 * handles failures, and updates marketplace. Implements complete auction
 * system with bid comparison, trade execution, and comprehensive error
 * handling and reporting.
 *
 * Trade Processing Phases:
 * 1. Read all trade file entries (sales, bids, cancellations)
 * 2. Process NOSALE entries to remove cancelled items
 * 3. Evaluate BUY entries against SELL items for highest bids
 * 4. Execute successful trades with commodity transfers
 * 5. Handle trade failures with appropriate reversals
 * 6. Update marketplace with unsold items for next turn
 * 7. Generate news reports and mail notifications
 *
 * Auction System Logic:
 * - Highest bid wins for each commodity
 * - Bids compared using gettval() for accurate valuation
 * - Failed bids returned to losing bidders
 * - Successful trades execute both sides of transaction
 * - Trade failures revert successful side and generate news
 *
 * Error Handling:
 * - Trade failures due to seller's commodity unavailability
 * - Trade failures due to buyer's bid commodity unavailability
 * - Automatic reversal of partial transactions
 * - News generation for failed trades with specific failure reasons
 * - Marketplace cleanup for next turn
 *
 * File Management:
 * - Removes old trade file and creates new one
 * - Preserves unsold items for continued marketplace listing
 * - Updates displayed values using gettval() for accuracy
 * - Handles file I/O errors with program termination
 *
 * Parameters: None (uses global game state and files)
 *
 * Returns: void
 *
 * Side Effects:
 *   - Executes all pending trades with resource transfers
 *   - Modifies trade file for next turn marketplace
 *   - Generates news reports for trade outcomes
 *   - Sends mail notifications to trading parties
 *   - Updates all nation resources and unit rosters
 *
 * Testing Notes:
 *   Category: C (System) - Requires complete game infrastructure
 *   Approach: System testing with full game state and file system
 *   Key Tests: Auction logic, trade execution, error handling, file management
 *   Dependencies: Complete trading system, file I/O, news system, mail system
 *   Mock Requirements: Full game state, file system, news/mail infrastructure
 *   Complexity: Complex - Multi-phase processing with extensive error handling
 *
 * Notes:
 *   - ADMIN-only function for automated turn processing
 *   - Central function for entire trading economy
 *   - Critical for maintaining game balance and fairness
 *   - Handles complex auction mechanics with multiple bidders
 */
void
uptrade()
{
	FILE *tfile;
	int count, itemnum=0, natn[MAXITM];
	int type1[MAXITM], type2[MAXITM], deal[MAXITM], extra[MAXITM];
	extern FILE *fnews;
	void trademail();
	int whobuy[MAXITM];
	long tradeit(), buy1[MAXITM], buy2[MAXITM];
	long price[MAXITM], gettval(), longval1, longval2;
	long lvar1[MAXITM], lvar2[MAXITM];
	void takeback();

	/* initialize purchase list */
	for (count=0; count<MAXITM; count++) {
		deal[count]=(-1);
		whobuy[count]=(-1);
		price[count]=(-1);
	}
	/* open trading file */
	if ((tfile = fopen(tradefile,"r")) == NULL) {
		/* no commodities - no transactions */
		return;
	}
	/* read in all of the transactions */
	while(!feof(tfile)) {
		if (7 != fscanf(tfile,"%d %d %d %d %ld %ld %d\n",&deal[itemnum],
			&natn[itemnum],&type1[itemnum],&type2[itemnum],&lvar1[itemnum],&lvar2[itemnum],&extra[itemnum])) break;
		if (deal[itemnum]==NOSALE) {
			/* remove item from sales list */
			deal[type1[itemnum]]=NOSALE;
		} else if (deal[itemnum]==SELL) {
			itemnum++;
		} else if (deal[itemnum]==BUY) {
			if (type1[itemnum]==GETFOOD) /* just ignore food */;
			else if (deal[type1[itemnum]]==SELL &&
			(price[type1[itemnum]] <
			gettval(natn[type1[itemnum]],natn[itemnum],
			type2[type1[itemnum]],lvar1[itemnum],(int)lvar2[itemnum]))) {
				deal[type1[itemnum]]=BUY;
				/* highest bid so far */
				price[type1[itemnum]]=gettval(natn[type1[itemnum]],natn[itemnum],type2[type1[itemnum]],lvar1[itemnum],(int)lvar2[itemnum]);
				/* return bid to loser */
				takeback(whobuy[type1[itemnum]],
					type2[type1[itemnum]],
					buy1[type1[itemnum]],(int)buy2[type1[itemnum]],FALSE);
				/* record details of trade */
				buy1[type1[itemnum]]=lvar1[itemnum];
				buy2[type1[itemnum]]=lvar2[itemnum];
				whobuy[type1[itemnum]]=natn[itemnum];
			} else {
				/* return bid */
				takeback(natn[itemnum],
					type2[type1[itemnum]],
					lvar1[itemnum],(int)lvar2[itemnum],FALSE);
			}
		}
	}
	fclose(tfile);
	unlink(tradefile);
	if (itemnum == 0) return;
	/* reopen the file for unsold commodities */
	if ((tfile=fopen(tradefile,"w")) == NULL) {
		/* error on opening file */
		printf("Error opening <%s> for trade update\n",tradefile);
		abrt();
	}

	/* compute the trading */
	for (count=0;count<itemnum;count++) {
		if (deal[count]==SELL) {
			/* adjust the displayed value */
			if (type1[count] != TDLAND)
			lvar1[count]=gettval(0,natn[count],type1[count],lvar1[count],extra[count]);
			/* keep unsold items up for sale */
			if(lvar1[count]>=0) fprintf(tfile,"%d %d %d %d %ld %ld %d\n", deal[count], natn[count],type1[count],type2[count],lvar1[count],lvar2[count],extra[count]);
		} else if (deal[count]==BUY) {
			/* do the trading */
			longval1 = tradeit(natn[count],whobuy[count],type1[count],lvar1[count],extra[count]);
			if (longval1!=(-1)) longval2 = tradeit(whobuy[count],natn[count],type2[count],buy1[count],(int)buy2[count]);
			if (longval1==(-1)) {
				/* abort due to seller */
				fprintf(fnews,"2.\tTrade between %s and %s breaks down due to %s.\n",
					ntn[natn[count]].name,ntn[whobuy[count]].name,tradefail[type1[count]]);
			} else if (longval2==(-1)) {
				/* abort due to buyer */
				tradeit(whobuy[count],natn[count],type1[count],longval1,extra[count]);
				fprintf(fnews,"2.\tTrade between %s and %s breaks down due to %s.\n",
					ntn[whobuy[count]].name,ntn[natn[count]].name,tradefail[type2[count]]);
				/* place it on the list for next turn */
				/* adjust the displayed value */
				/* let land fail during trade */
				if (type1[count] != TDLAND)
				lvar1[count]=gettval(0,natn[count],type1[count],lvar1[count],extra[count]);
				if(lvar1[count]>=0) fprintf(tfile,"%d %d %d %d %ld %ld %d\n", SELL, natn[count],type1[count],type2[count],lvar1[count],lvar2[count],extra[count]);
			} else {
				/* trade completed send mail */
				fprintf(fnews,"2.\tNation %s sells %s to %s for %s\n"
					,ntn[natn[count]].name,commodities[type1[count]],ntn[whobuy[count]].name,commodities[type2[count]]);
				trademail(natn[count],whobuy[count],type1[count],
					type2[count],longval1,(long)extra[count],
					longval2,buy2[count]);
			}
		}
	}
	fclose(tfile);
}

/*
 * fixtrade - Remove all trade items belonging to specified nation (ADMIN only)
 *
 * Administrative function to remove all commodities that a nation has listed
 * for sale from the trading marketplace. Used when a nation is eliminated,
 * goes inactive, or requires administrative cleanup of their trade presence.
 * Ensures marketplace integrity by removing invalid or unavailable items.
 *
 * Cleanup Operations:
 * - Reads current trade file to identify nation's active sales
 * - Processes only SELL entries belonging to specified nation
 * - Generates NOSALE entries to cancel each item
 * - Returns reserved commodities to nation via takeback()
 * - Maintains trade file consistency for other nations
 *
 * Trade File Processing:
 * - Reads all trade entries to build current marketplace state
 * - Identifies items owned by target nation (natn[holdint] == cntry)
 * - Appends NOSALE cancellation entries for each owned item
 * - Preserves other nations' trade entries unchanged
 * - Handles missing trade file gracefully (no operations needed)
 *
 * Commodity Restoration:
 * - Returns all reserved resources to nation's available totals
 * - Restores army/ship status from TRADED back to normal
 * - Ensures nation retains access to previously committed resources
 * - Prevents resource loss during administrative cleanup
 *
 * Parameters:
 *   cntry - Nation index whose trade items should be removed
 *
 * Returns: void
 *
 * Side Effects:
 *   - Modifies trade file with NOSALE entries for nation's items
 *   - Returns reserved commodities to nation via takeback()
 *   - Removes nation's presence from active marketplace
 *   - May terminate program on file I/O errors
 *
 * Testing Notes:
 *   Category: C (System) - Requires file system and complete game state
 *   Approach: System testing with mock trade files and nation data
 *   Key Tests: Item identification, file updates, commodity restoration
 *   Dependencies: Trade file, nation data, takeback() function
 *   Mock Requirements: File system, trade file format, complete nation state
 *   Complexity: Moderate - File processing with selective updates
 *
 * Notes:
 *   - ADMIN-only function for administrative and cleanup operations
 *   - Essential for maintaining marketplace integrity
 *   - Used during nation elimination or administrative intervention
 *   - Prevents orphaned trade entries from inactive nations
 */
void
fixtrade (int cntry)
{
	FILE *tfile;
	int holdint, notopen=FALSE;
	int type1[MAXITM], type2[MAXITM], deal[MAXITM], extra[MAXITM];
	int natn[MAXITM], itemnum, getland(), gettrade(), checkland();
	long lvar1[MAXITM], lvar2[MAXITM], armyvalue();
	void  setaside(), takeback();

	/* open trading file */
	if ((tfile=fopen(tradefile,"r")) == NULL ) {
		notopen=TRUE;
	}
	itemnum = 0;

	/* read in all of the data */
	while (notopen==FALSE && !feof(tfile)) 
	{
		if (fscanf(tfile,"%d %d %d %d %ld %ld %d\n",&deal[itemnum],
			&natn[itemnum],&type1[itemnum],&type2[itemnum],
			&lvar1[itemnum],&lvar2[itemnum],&extra[itemnum]) == 7){
			if (deal[itemnum]==NOSALE) {
				/* remove item from sales list */
				deal[type1[itemnum]]=NOSALE;
			} else if (deal[itemnum]==SELL) {
				itemnum++;
			}
		}
	}
	if (notopen==FALSE) fclose(tfile);

	/* go through list of commodities */

	for ( holdint=0; holdint<itemnum; holdint++) 
	{
		if ( deal[holdint]==SELL ) 
		{

			if ( natn[holdint] == cntry )
			{
				/* remove it from market */

				if ( (tfile = fopen(tradefile,"a+"))==NULL)
				{
					printf("Error opening file for trading");
					abrt();
				}

				fprintf(tfile, "%d %d %d %d %ld %ld %d\n", 
				 NOSALE, natn[holdint], holdint, 0, 0L, 0L, 0);

				fclose(tfile);

				takeback( natn[holdint], type1[holdint], lvar1[holdint],
					extra[holdint], FALSE);

			}  /* natn == cntry */

		} /* SELL order */

	} /* loop through commodities */
}
#endif /* ADMIN */
#endif /* TRADE */
