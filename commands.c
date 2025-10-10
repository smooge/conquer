/*
 * commands.c - Player command processing
 *
 * This file is part of Conquer.
 * Originally Copyright (C) 1988-1989 by Edward M. Barlow and Adam Bryant
 * Copyright (C) 2025 Juan Manuel Méndez Rey (Vejeta) - Licensed under GPL v3 with permission
 * from original authors
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
#ifdef CONQUER
#include <sys/types.h>
#include <sys/stat.h>
#endif /*CONQUER*/
#include <time.h>
#include <unistd.h>
#include "header.h"
#include "data.h"
#include "safe_convert.h"

int roads_this_turn = 0;

/*
 * desg_ok - Validate sector designation change requests with comprehensive rules
 *
 * Performs complete validation of proposed sector designation changes, checking
 * all game rules including resource requirements, population constraints, magic
 * prerequisites, and logical transition restrictions. Used by redesignate()
 * and other construction commands to ensure valid sector development.
 *
 * Algorithm:
 * 1. Vegetation Check: Verify sufficient food production for most designations
 * 2. Duplicate Prevention: Reject attempts to designate same type
 * 3. City Protection: Require explicit ruin designation before city changes
 * 4. Population Requirements: Enforce minimum population for urban development
 * 5. Access Control: Block player creation of restricted designations
 * 6. Display Filtering: Handle special visibility rules for UI
 * 7. Ruin Logic: Validate ruin creation from cities/capitols only
 * 8. Urban Progression: Enforce city->capitol upgrade path requirements
 * 9. Resource Matching: Validate specialized buildings against trade goods
 * 10. Mining Viability: Check resource availability for mines
 * 11. Magic Prerequisites: Verify magic powers for special constructions
 *
 * Designation Rules Enforced:
 * - Most designations require DESFOOD minimum vegetation
 * - Cities/towns/capitols need 500+ population
 * - Capitols must upgrade from cities/towns/ruins
 * - Universities/lumberyards must match sector trade goods
 * - Mines require corresponding metal/jewel resources
 * - Special stone sites require SUMMON magic power
 * - Pirate coves restricted to NPC/admin creation
 * - Cities/capitols must be ruined before redesignation
 *
 * Error Reporting Modes:
 * - prtflag=TRUE: Display error messages to user
 * - prtflag=FALSE: Silent validation for UI filtering
 *
 * Parameters:
 *   prtflag - Display error messages if TRUE, silent validation if FALSE
 *   desg - Proposed new designation character (DCITY, DMINE, etc.)
 *   sptr - Pointer to sector being evaluated for designation change
 *
 * Returns:
 *   int - TRUE if designation change is valid, FALSE if invalid/blocked
 *
 * Side Effects:
 *   - May display error messages via errormsg() if prtflag is TRUE
 *   - Read-only validation, no modification of game state
 *   - No persistent side effects beyond user notification
 *
 * Global Variables Used:
 *   - country: Current player nation for magic/resource checks
 *   - DESFOOD: Minimum food requirement constant
 *   - tg_stype: Trade good to sector type mapping array
 *
 * Functions Called:
 *   - tofood(): Calculate food production potential
 *   - errormsg(): Display error message to user
 *   - magic(): Check player magic power availability
 *   - tg_ok(): Validate trade good employment rules
 *
 * Testing Notes:
 *   Category: A (Unit) - Pure validation logic with clear inputs/outputs
 *   Approach: Unit testing with comprehensive designation/sector combinations
 *   Key Tests: All designation types, edge cases, resource constraints, magic
 *   Dependencies: Sector data, magic system, trade good system
 *   Mock Requirements: Sector structures, magic checks, resource calculations
 *   Complexity: Moderate - Multiple validation rules with clear logic paths
 *
 * Notes:
 *   - Thread safety: Read-only operation, safe for concurrent access
 *   - Performance: O(1) - constant time validation checks
 *   - Historical context: Core game balance enforcement mechanism
 *   - Game balance: Prevents resource exploitation and maintains progression
 *   - UI integration: Dual mode supports both command validation and display
 *   - Error UX: Provides helpful error messages explaining restrictions
 *   - Extensibility: Clear structure allows easy addition of new rules
 */
/*
 * desg_ok - Validate sector designation change with comprehensive rule checking
 *
 * Determines if a given sector designation is permissible based on multiple
 * constraints including vegetation requirements, current designation, population
 * thresholds, trade good compatibility, and resource availability. Functions in
 * dual mode: validation mode (prtflag=TRUE) provides error messages, while query
 * mode (prtflag=FALSE) supports UI filtering of valid designation options.
 *
 * Validation Rules:
 * - Vegetation: Most designations require minimum DESFOOD vegetation (except roads/forts)
 * - No duplicate: Cannot redesignate to same type
 * - Cities/Capitols: Must be ruins before changing to other types
 * - Population: Towns/Cities/Capitols require minimum 500 people
 * - Trade goods: Universities/Lumberyards must match sector trade good
 * - Mines: Require metal (mines) or jewels (gold mines) and trade good compatibility
 * - Special: Requires SUMMON magic power and stone trade goods
 * - Ruins: Only valid from cities or capitols
 * - City/Capitol progression: Must upgrade through town → city → capitol chain
 * - Pirate coves: Reserved for god mode (players cannot create)
 *
 * Parameters:
 *   prtflag - TRUE: validation mode with error messages, FALSE: query mode for UI
 *   desg - Designation character to validate (DCITY, DTOWN, DMINE, etc.)
 *   sptr - Pointer to sector being evaluated (must not be NULL)
 *
 * Returns:
 *   TRUE if designation change is valid and permitted
 *   FALSE if designation violates constraints or is incompatible
 *
 * Side Effects:
 *   - Displays error messages via errormsg() when prtflag is TRUE
 *   - No modifications to game state (pure validation function)
 *
 * Testing Notes:
 *   Category: A - Unit tested
 *   Approach: Test each validation rule independently (vegetation, population,
 *            trade goods, resource requirements) with valid/invalid inputs
 *   Dependencies: Requires tofood(), magic(), tg_ok() helpers and designation constants
 *   Complexity: Moderate - 12 distinct validation rules with state dependencies
 *
 * Notes:
 *   - Dual mode design: prtflag controls both error output and filtering behavior
 *   - Query mode (prtflag=FALSE) hides invalid options from UI designation menus
 *   - Special handling for DNODESIG and DSPECIAL in query mode
 *   - Uses global 'country' variable to check magic powers and ownership
 *   - tg_stype array maps trade goods to required sector types
 *
 * @last_documented: 2025-10-08
 */
int desg_ok(int prtflag, char desg, struct s_sector *sptr) {
    /* check vegetation */
    if ((desg != DNODESIG) && (desg != DROAD) && (desg != DFORT) && (desg != DSTOCKADE)
        && (tofood(sptr, country) < DESFOOD)) {
        if (prtflag)
            errormsg("vegetation too sparse");
        return (FALSE);
    }

    /* don't allow the same designation */
    if (desg == sptr->designation) {
        if (prtflag)
            errormsg("Hey, get your act together!  There is already one there.");
        return (FALSE);
    }

    /* check for city/capitol being made into something else */
    if ((desg != DRUIN)
        && ((desg != DCAPITOL && sptr->designation == DCITY)
            || sptr->designation == DCAPITOL)) {
        if (prtflag) {
            char buf[LINELTH + 1];
            snprintf(buf, sizeof(buf), "Must first burn down city/capitol (designate as '%c')",
                     DRUIN);
            errormsg(buf);
        }
        return (FALSE);
    }

    /* check for proper population */
    if ((sptr->people < 500) && (desg == DCAPITOL || desg == DCITY || desg == DTOWN)) {
        if (prtflag)
            errormsg("Need 500 people to build a city or town");
        return (FALSE);
    }

    /* only god may create pirate  */
    if (desg == DBASECAMP) {
        if (prtflag)
            errormsg("A Pirate Cove?? Are you serious?!");
        return (FALSE);
    }

    /* should not appear in display list */
    if (prtflag == FALSE
        && ((desg == DSPECIAL && sptr->tradegood != TG_stones) || desg == DNODESIG))
        return (FALSE);

    if (desg == DRUIN) {
        if (sptr->designation != DCITY && sptr->designation != DCAPITOL) {
            if (prtflag)
                errormsg("Ruins may only come from cities or capitols");
            return (FALSE);
        }
    }

    if (desg == DCAPITOL || desg == DCITY) {
        if ((desg == DCAPITOL && sptr->designation != DCITY) && sptr->designation != DTOWN
            && sptr->designation != DRUIN) {
            if (prtflag)
                errormsg("You can't build a city from that!");
            return (FALSE);
        }
    }

    if (desg == DUNIVERSITY || desg == DLUMBERYD) {
        if ((sptr->tradegood != TG_none) && (*(tg_stype + sptr->tradegood) != desg)
            && (*(tg_stype + sptr->tradegood) != 'x')) {
            if (prtflag)
                errormsg("You can't have one of those here!");
            return (FALSE);
        }
    }

    if (desg == DMINE || desg == DGOLDMINE) {
        if (!tg_ok(country, sptr)) {
            if (prtflag)
                errormsg("Your people refuse to be unemployed");
            return (FALSE);
        }
        if ((desg == DMINE && sptr->metal == 0) || (desg == DGOLDMINE && sptr->jewels == 0)) {
            if (prtflag)
                errormsg("Your people refuse to be unemployed");
            return (FALSE);
        }
    }

    if ((desg == DSPECIAL) && (magic(country, SUMMON) != TRUE)) {
        if (prtflag)
            errormsg("You are gonna need SUMMON power to use those stones!");
        return (FALSE);
    }

    return (TRUE);
}

/*
 * redesignate - Change current sector designation with validation and cost processing
 *
 * Provides comprehensive sector designation change functionality supporting both
 * normal player operations and god-mode world editing. Handles validation through
 * desg_ok(), processes costs, manages special cases like capitol relocation,
 * and provides interactive UI for designation selection.
 *
 * The function operates in two distinct modes:
 * 1. God Mode (country==0): Allows editing of elevation, vegetation, ownership,
 *    population, and trade goods with unrestricted access
 * 2. Player Mode: Standard designation changes with full validation and costs
 *
 * Algorithm:
 * 1. Check for god mode and provide special editing interface if applicable
 * 2. Validate sector ownership (player mode only)
 * 3. Display available designations using desg_ok() filtering
 * 4. Process user input and validate designation choice
 * 5. Handle special cases (roads, special designations)
 * 6. Calculate and deduct costs (gold, metal) based on designation type
 * 7. Update sector designation and handle side effects (capitol moves)
 * 8. Update adjacent sector influences via SADJDES macro
 *
 * Parameters:
 *   None (operates on global XREAL/YREAL coordinates)
 *
 * Returns:
 *   void (no return value)
 *
 * Side Effects:
 *   - Modifies sector designation at current coordinates
 *   - Deducts gold and metal from nation treasury
 *   - May relocate nation capitol for DCAPITOL designation
 *   - Updates fortress values for ruin operations
 *   - Increments roads_this_turn counter for road building
 *   - Updates adjacent sector influences
 *   - Clears and redraws bottom screen area
 *
 * God Mode Features:
 *   - 'e': Change elevation (water, peak, mountain, hill, clear)
 *   - 'v': Change vegetation (volcano, desert, tundra, etc.)
 *   - 'o': Change sector owner
 *   - 'p': Change population count
 *   - 't': Change trade good type and value
 *   - 'd': Normal designation change (falls through to player mode)
 *
 * Cost Structure:
 *   - Basic designations: DESCOST gold
 *   - Stockades: STOCKCOST gold
 *   - Towns/Forts: 10*DESCOST gold + DESCOST metal
 *   - Cities: 20*DESCOST gold + 5*DESCOST metal
 *   - Capitols: 20*DESCOST gold + 5*DESCOST metal + capitol relocation
 *   - Ruins from cities: Fortress reduction by 4, rebuild cost applies
 *   - Roads: Limited to 2 per turn, requires 100+ population
 *
 * Special Validations:
 *   - Ownership check (non-god mode)
 *   - desg_ok() validation for all designations
 *   - Road building limitations (2 per turn, 100+ people)
 *   - Metal requirements for cities and forts
 *   - Capitol uniqueness enforcement
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires sector data, nation state, UI interaction
 *   Approach: Integration testing with mock sectors and nation data
 *   Key Tests: God mode operations, cost calculations, capitol relocation,
 *             road limits, validation failures, designation filtering
 *   Dependencies: sct[][] array, ntn[] array, country/curntn globals,
 *                XREAL/YREAL coordinates, desg_ok() function, UI functions
 *   Mock Requirements: Screen I/O (mvaddstr, getch), sector ownership,
 *                     nation treasury values, designation validation
 *   Complexity: Complex - Multiple operation modes, extensive state changes
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to global state modifications
 *   - UI Integration: Heavy curses dependency for interactive designation selection
 *   - God mode provides world editing capabilities for game administration
 *   - Capitol relocation automatically demotes previous capitol to city
 *   - SADJDES macro updates influence on adjacent sectors after changes
 *
 * @last_documented: 2025-10-08
 */
void redesignate(void) {
    char newdes;
    char tgtype[NAMELTH + 1];
    struct s_sector *sptr = &sct[XREAL][YREAL];
    short x = 0, y = 0;
    long metal = 0;
    int isgod = FALSE;

    if (country == 0) {
        isgod = TRUE;
        country = sptr->owner;
        curntn = &ntn[country];

        clear_bottom(0);
        mvaddstr(LINES - 4, 0,
                 "SUPER USER: CHANGE (v)eg, (e)lev, (d)esig, (o)wner, (p)op, (t)radegood");
        refresh();
        switch (getch()) {
            case 'd':
                /* fall into normal redesignation command */
                break;
            case 'e':
                /*simple contour map definitions*/
                mvprintw(LINES - 3, 7, "ELEVATIONS: change to %c, %c, %c, %c or %c?", WATER,
                         PEAK, MOUNTAIN, HILL, CLEAR);
                refresh();
                newdes = safe_int_to_char(getch());
                if (newdes != WATER && newdes != PEAK && newdes != MOUNTAIN && newdes != HILL
                    && newdes != CLEAR) {
                    reset_god();
                    return;
                }
                sptr->altitude = newdes;
                if ((newdes == PEAK) || (newdes == WATER)) {
                    sptr->owner = 0;
                    sptr->people = 0;
                    sptr->fortress = 0;
                }
                /*will fall through as must change vegetation*/
                [[fallthrough]];
            case 'v':
                /*vegetation types*/
                mvprintw(
                    LINES - 3, 7,
                    "VEGETATIONS: change to %c, %c, %c, %c, %c, %c, %c, %c, %c, %c, %c or %c?",
                    VOLCANO, DESERT, TUNDRA, BARREN, LT_VEG, GOOD, WOOD, FOREST, JUNGLE, SWAMP,
                    ICE, NONE);
                refresh();
                newdes = safe_int_to_char(getch());
                if (newdes != VOLCANO && newdes != DESERT && newdes != TUNDRA
                    && newdes != BARREN && newdes != LT_VEG && newdes != NONE && newdes != GOOD
                    && newdes != WOOD && newdes != FOREST && newdes != JUNGLE && newdes != SWAMP
                    && newdes != ICE) {
                    reset_god();
                    return;
                }
                sptr->vegetation = newdes;
                if (tofood(sptr, 0) < DESFOOD)
                    sptr->designation = newdes;
                else
                    sptr->designation = DNODESIG;
                reset_god();
                return;
            case 'o':
                mvaddstr(LINES - 3, 7, "What nation owner:");
                refresh();
                x = safe_int_to_short(get_country());
                if (x > 0 && x < NTOTAL)
                    sptr->owner = safe_short_to_uchar(x);
                reset_god();
                return;
            case 'p':
                if (sptr->altitude == WATER) {
                    errormsg("Trying to build a colony of mermen?");
                    reset_god();
                    return;
                }
                mvaddstr(LINES - 3, 7, "new population for sector: ");
                refresh();
                metal = get_number();
                if (metal <= (-1))
                    return;
                sptr->people = metal;
                reset_god();
                return;
            case 't':
                x = TRUE;
                while (x == TRUE) {
                    mvaddstr(LINES - 2, 7, "new sector tradegood type:");
                    clrtoeol();
                    refresh();
                    get_nname(tgtype);
                    if (strlen(tgtype) == 0) {
                        reset_god();
                        return;
                    }
                    for (y = 0; x == TRUE && y <= TG_none; y++)
                        if (strcmp(tg_name[y], tgtype) == 0)
                            x = FALSE;
                }
                y--;
                if ((y != TG_none) && (y > END_NORMAL)) {
                    mvaddstr(LINES - 1, 7, "new sector value: ");
                    refresh();
                    x = safe_long_to_short(get_number());
                    if (x < 100 && x > 0) {
                        if (y > END_MINE) {
				sptr->jewels = safe_short_to_uchar(x);
                            sptr->metal = 0;
                        } else {
				sptr->metal = safe_short_to_uchar(x);
                            sptr->jewels = 0;
                        }
                    } else {
                        sptr->jewels = 0;
                        sptr->metal = 0;
                    }
                } else {
                    sptr->jewels = 0;
                    sptr->metal = 0;
                }
                sptr->tradegood = safe_short_to_uchar(y);
                reset_god();
                return;
            default:
                reset_god();
                return;
        }
    }

    clear_bottom(0);
    if ((SOWN != country) && (isgod == FALSE)) {
        errormsg("Hey!  You don't own that sector!");
        return;
    }

    mvaddstr(LINES - 4, 0, "Possible sector designations: ");
    x = 30;
    y = safe_int_to_short(LINES - 4);
    for (newdes = 0; *(des + newdes) != '0'; newdes++) {
        if ((isgod == TRUE) || desg_ok(FALSE, *(des + newdes), sptr)) {
            mvprintw(y, x, "(%c)", *(des + newdes));
            x += 4;
            if (x > COLS - 15) {
                x = 5;
                y++;
            }
        }
    }

    if ((sptr->tradegood != TG_none) && (*(tg_stype + sptr->tradegood) != 'x')
        && (isgod == TRUE || desg_ok(FALSE, *(tg_stype + sptr->tradegood), sptr)))
        mvprintw(y, x, "(%c special=>%c)", DSPECIAL, *(tg_stype + sptr->tradegood));

    mvaddstr(++y, 0, "<Any other key to return>  What new designation:");
    refresh();

    /*read answer*/
    if ((newdes = safe_int_to_char(getch())) == DSPECIAL) {
        if ((sptr->tradegood == TG_none) || (*(tg_stype + sptr->tradegood) == 'x')
            || (isgod == FALSE && !desg_ok(FALSE, *(tg_stype + sptr->tradegood), sptr))) {
            errormsg("no special designation possible");
            if (isgod == TRUE)
                reset_god();
            return;
        } else
            newdes = *(tg_stype + sptr->tradegood);
    }

    for (x = 0; *(des + x) != '0'; x++)
        if (newdes == *(des + x))
            break;

    if (*(des + x) == '0') {
        if (isgod == TRUE)
            reset_god();
        return;
    }

    /* validate designation */
    if ((isgod == FALSE) && !desg_ok(TRUE, newdes, sptr)) {
        return;
    }

    if (newdes == DROAD && isgod == FALSE) {
        if (sptr->people < 100) {
            errormsg("Need 100+ people to build a road!");
            return;
        }
        roads_this_turn++;
        if (roads_this_turn > 2) {
            roads_this_turn = 2;
            errormsg("Only two roads a turn!  Let the road crews sleep!!");
            return;
        }
    }

    /* do not need metal to build a capitol from a city */
    if ((newdes == DFORT) || (newdes == DTOWN))
        metal = DESCOST;
    else if ((newdes == DCITY) || (newdes == DCAPITOL && sptr->designation != DCITY))
        metal = 5 * DESCOST;

    if ((newdes != DTOWN) && (newdes != DFORT) && (newdes != DCITY) && (newdes != DCAPITOL)) {
        /*decrement treasury*/
        if (newdes == DRUIN) {
            if (sptr->fortress > 4) {
                sptr->fortress -= 4;
            } else {
                sptr->fortress = 0;
            }
        } else if (sptr->designation == DRUIN) {
            curntn->tgold -= REBUILDCOST;
        }
        sptr->designation = newdes;

        SADJDES;
        if (newdes == DSTOCKADE)
            curntn->tgold -= STOCKCOST * (1 - isgod);
        else
            curntn->tgold -= DESCOST * (1 - isgod);

    } else if ((isgod == FALSE) && (curntn->metals < metal)) {
        errormsg("Not enough metal for city, town, or fort");
    } else if ((newdes == DCITY) || (newdes == DCAPITOL)) {
        if (sptr->designation == DRUIN) {
            curntn->tgold -= 10 * DESCOST * (1 - isgod);
            curntn->metals -= metal / 2 * (1 - isgod);
        } else {
            curntn->tgold -= 20 * DESCOST * (1 - isgod);
            if (newdes == DCITY || sptr->designation != DCITY)
                curntn->metals -= metal * (1 - isgod);
        }
        x = curntn->capx; /* need this for SADJDES2 */
        y = curntn->capy;
        /* can only have one capitol */
        if (newdes == DCAPITOL) {
            if (sct[x][y].owner == country) {
                sct[x][y].designation = DCITY;
                SADJDES2;
            }
            curntn->capx = safe_int_to_uchar(XREAL);
            curntn->capy = safe_int_to_uchar(YREAL);
        }
        sptr->designation = newdes;
        SADJDES;
    } else if ((newdes == DFORT) || (newdes == DTOWN)) {
        curntn->tgold -= 10 * DESCOST * (1 - isgod);
        curntn->metals -= metal * (1 - isgod);
        sptr->designation = newdes;
        SADJDES;
    } else
        errormsg("Serious error: What designation are you?");
    if (isgod == TRUE)
        reset_god();
}

/*
 * construct - Build fortifications, ships, or naval units with comprehensive validation
 *
 * Provides complete construction functionality for military infrastructure including
 * fortification improvements and naval fleet construction/repair. Supports both
 * god mode operations and standard player construction with resource validation,
 * cost processing, and capacity management.
 *
 * The function handles three main construction types:
 * 1. Fortification: Increase sector fortress level (+defensive bonus)
 * 2. Ship Building: Construct new warships, merchants, or galleys
 * 3. Ship Repair: Add crew to existing damaged fleets
 *
 * Algorithm:
 * 1. Validate ownership and sector requirements (town/city/capitol)
 * 2. Check population (500+ people) and treasury (positive gold)
 * 3. Determine available options based on proximity to water
 * 4. Process user choice (fortify, build ships, or repair ships)
 * 5. For ships: Validate harbor access and fleet management
 * 6. Calculate costs with magic/god mode modifiers
 * 7. Verify resource availability (gold, population)
 * 8. Execute construction and update all relevant data structures
 * 9. Display results and update adjacent influences
 *
 * Construction Requirements:
 * - Ownership: Must own the sector (unless god mode)
 * - Population: 500+ people minimum for construction
 * - Location: Must be in town, city, or capitol designation
 * - Treasury: Positive gold balance required
 * - Harbor: Ships require adjacent water tiles
 * - Heavy Ships: Require city/capitol (not available in towns)
 *
 * Parameters:
 *   None (operates on global XREAL/YREAL coordinates)
 *
 * Returns:
 *   void (no return value)
 *
 * Side Effects:
 *   - Modifies sector fortress level for fortification
 *   - Creates or modifies naval fleets in ntn[].navies array
 *   - Deducts gold from nation treasury
 *   - Reduces civilian population for crew recruitment
 *   - Updates fleet composition (warships, merchants, galleys)
 *   - Adjusts crew levels and cargo capacity
 *   - Updates adjacent sector influences
 *   - Clears and redraws bottom screen area
 *
 * Fortification System:
 *   - Cost: FORTCOST * (2^current_fortress_level)
 *   - Bonus: Varies by designation (TOWNSTR/FORTSTR/CITYSTR)
 *   - Magic: ARCHITECT doubles defensive bonus
 *   - Limit: Maximum 11 fortress levels per sector
 *   - Debt: Can go into debt up to 10 * nation's jewels
 *
 * Ship Construction System:
 *   - Types: Warship (combat), Merchant (cargo), Galley (balanced)
 *   - Classes: Light, Medium, Heavy (increasing cost/capacity)
 *   - Cost: Base cost * (class+1) * ship type multiplier
 *   - Magic: SAILOR reduces ship costs by 50%
 *   - Crew: SHIPCREW per ship, recruited from sector population
 *   - Fleet: Manages up to MAXNAVY fleets per nation
 *
 * Ship Repair System:
 *   - Target: Existing fleets with damaged crew levels
 *   - Cost: Proportional to fleet size and repair amount
 *   - Crew: Add crew up to SHIPCREW maximum per ship
 *   - Population: Requires civilians for crew recruitment
 *   - Validation: Prevents over-crewing beyond ship capacity
 *
 * Fleet Management:
 *   - Allocation: Automatic assignment to available fleet slots
 *   - Composition: Tracks light/medium/heavy for each ship type
 *   - Capacity: Calculates total holding and crew requirements
 *   - Location: Sets fleet position to construction sector
 *   - Movement: Resets movement points for new/modified fleets
 *
 * Cost Modifiers:
 *   - God Mode: All construction costs set to 0
 *   - SAILOR Magic: 50% reduction on ship costs
 *   - ARCHITECT Magic: Double fortress defensive bonus
 *   - Fortress Scaling: Exponential cost increase (2^level)
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires sector data, fleet arrays, UI interaction
 *   Approach: Integration testing with mock naval and sector systems
 *   Key Tests: Cost calculations, fleet allocation, capacity limits,
 *             fortress scaling, magic modifiers, harbor validation,
 *             crew management, resource validation
 *   Dependencies: sct[][] array, ntn[].navies array, magic() system,
 *                UI functions (mvaddstr, getch), getselunit() function
 *   Mock Requirements: Fleet data structures, sector ownership,
 *                     nation treasury/population, magic abilities
 *   Complexity: Complex - Multiple construction modes, fleet management
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to global state modifications
 *   - Magic Integration: Multiple spells affect construction costs/bonuses
 *   - Fleet Limits: Hard-coded ship type limits via N_MASK validation
 *   - Harbor Detection: Adjacent water tile scanning for ship construction
 *   - Debt Management: Fortification allows controlled debt up to jewel limit
 *   - Heavy Ship Restriction: Towns cannot construct heavy class ships
 *
 * @last_documented: 2025-10-08
 */
void construct(void) {
    int tmpvar, tmpvar2, onboard;
    long cost;
    int armbonus = 0;
    int x, y;
    short nvynum = 0;
    short shipsize, amount;
    short isgod = FALSE;
    char type;

    clear_bottom(0);
    if (country == 0) {
        isgod = TRUE;
        country = sct[XREAL][YREAL].owner;
        curntn = &ntn[country];
    }

    if (sct[XREAL][YREAL].owner != country) {
        errormsg("You do not own");
        if (isgod == TRUE)
            reset_god();
        return;
    }
    if ((isgod == FALSE) && (sct[XREAL][YREAL].people <= 500)) {
        errormsg("You need over 500 people to construct");
        return;
    }

    if ((isgod == FALSE) && (curntn->tgold < 0)) {
        errormsg("You are broke");
        return;
    }
    tmpvar = FALSE;
    for (x = XREAL - 1; x <= XREAL + 1; x++)
        for (y = YREAL - 1; y <= YREAL + 1; y++)
            if (ONMAP(x, y) && sct[x][y].altitude == WATER)
                tmpvar = TRUE;

    if ((sct[XREAL][YREAL].designation == DTOWN) || (sct[XREAL][YREAL].designation == DFORT)
        || (sct[XREAL][YREAL].designation == DCAPITOL)
        || (sct[XREAL][YREAL].designation == DCITY)) {
        /*calculate cost for fort*/
        cost = FORTCOST;
        if (isgod == TRUE)
            cost = 0;
        else
            for (x = 1; x <= sct[XREAL][YREAL].fortress; x++)
                cost *= 2;

        if (sct[XREAL][YREAL].designation == DTOWN)
            armbonus = TOWNSTR;
        else if (sct[XREAL][YREAL].designation == DFORT)
            armbonus = FORTSTR;
        else if (sct[XREAL][YREAL].designation == DCAPITOL)
            armbonus = CITYSTR;
        else if (sct[XREAL][YREAL].designation == DCITY)
            armbonus = CITYSTR;
        if (magic(country, ARCHITECT) == TRUE)
            armbonus *= 2;

        if (tmpvar)
            mvprintw(LINES - 4, 0,
                     "<f>ortify sector (+%d%%/%ld talons); <b>uild or <r>epair ships?: ",
                     armbonus, cost);
        else
            mvprintw(LINES - 4, 0, "<f>ortify sector (+%d%% - %ld talons):", armbonus, cost);
        refresh();
        type = safe_int_to_char(getch());
    } else {
        errormsg("Must construct in town, city, or fortress");
        if (isgod == TRUE)
            reset_god();
        return;
    }

    /* construct ships*/
    if ((type == 'b') || (type == 'r')) {
        /*check if next to sea*/
        if ((tmpvar == FALSE) || (sct[XREAL][YREAL].designation == DFORT)) {
            errormsg("not in a harbor");
            if (isgod == TRUE)
                reset_god();
            return;
        }

        nvynum = safe_int_to_short(getselunit() - MAXARM);
        if (type == 'r') {
            if ((nvynum >= MAXNAVY) || (nvynum < 0)) {
                errormsg("INVALID NAVY");
                if (isgod == TRUE)
                    reset_god();
                return;
            }

            if (P_NCREW == SHIPCREW) {
                errormsg("You may only &^#$! repair damaged fleets!!!");
                return;
            }
            clear_bottom(0);
            shipsize = safe_int_to_short(flthold(nvynum));
            mvprintw(LINES - 4, 0, "Repairing Fleet (%d)", nvynum);
            mvprintw(LINES - 4, 30, "Storage Units [%d]", shipsize);
            mvprintw(LINES - 3, 0, "crew per unit = %d", (int)P_NCREW);
            mvprintw(LINES - 3, 30, "max crew per unit = %d", SHIPCREW);
            mvaddstr(LINES - 2, 0, "How many crew per unit do you wish to add:");
            clrtoeol();
            refresh();
            amount = safe_long_to_short(get_number());
            if (amount < 0) {
                if (isgod == TRUE)
                    reset_god();
                return;
            }


            /* find cost of repairs on all ships */
            cost = 0;
            for (tmpvar = N_LIGHT; tmpvar <= N_HEAVY; tmpvar++) {
                cost += (tmpvar + 1) * WARSHPCOST * P_NWAR(tmpvar);
                cost += (tmpvar + 1) * MERSHPCOST * P_NMER(tmpvar);
                cost += (tmpvar + 1) * GALSHPCOST * P_NGAL(tmpvar);
            }
            cost *= amount / SHIPCREW;

            if (isgod == TRUE)
                cost = 0;
            else if (magic(country, SAILOR) == TRUE)
                cost /= 2L;

            if (curntn->tgold < cost) {
                errormsg("NOT ENOUGH GOLD");
                if (isgod == TRUE)
                    reset_god();
                return;
            }
            if (sct[XREAL][YREAL].people < amount * shipsize) {
                errormsg("NOT ENOUGH CIVILIANS IN SECTOR");
                if (isgod == TRUE)
                    reset_god();
                return;
            }
            if (SHIPCREW < P_NCREW + amount) {
                errormsg("THAT WOULD EXCEED MAXIMUM SHIP CREW");
                if (isgod == TRUE)
                    reset_god();
                return;
            }

            curntn->tgold -= cost;
            sct[XREAL][YREAL].people -= amount * shipsize;
            P_NCREW += safe_long_to_uchar(amount);
            NADJCRW;
            SADJCIV;

            if (isgod == TRUE)
                reset_god();
            return;
        }

        mvaddstr(LINES - 3, 0, "Do you wish to raise a new fleet? (y or n)");
        clrtoeol();
        refresh();
        if (getch() == 'y')
            nvynum = (-1);

        clear_bottom(0);
        if (nvynum < 0) {
            nvynum = 0;
            x = (-1);
            while ((x == (-1)) && (nvynum < MAXNAVY)) {
                if (P_NWSHP == 0 && P_NMSHP == 0 && P_NGSHP == 0) {
                    x = nvynum;
                    P_NWSHP = 0;
                    P_NMSHP = 0;
                    P_NGSHP = 0;
                    P_NCREW = 0;
                    P_NARMY = MAXARM;
                    P_NPEOP = 0;
                    NADJHLD;
                    NADJCRW;
                    NADJWAR;
                    NADJMER;
                    NADJGAL;
                }
                nvynum++;
            }
            nvynum = safe_int_to_short(x);
            if (nvynum < 0) {
                errormsg("NO FREE NAVIES");
                if (isgod == TRUE)
                    reset_god();
                return;
            } else
                mvprintw(LINES - 1, 0, "Raising New Fleet (%d)", nvynum);
        } else
            mvprintw(LINES - 1, 0, "Adding to Fleet (%d)", nvynum);

        /* process type first to be consistant with drafting */
        mvaddstr(LINES - 4, 0, "What ship type to construct: (W)arship (M)erchant (G)alley?");
        refresh();
        /*
         *  Note:  3 and 6 hard-coded for ship sizes based on
         *   0 = light   1 = medium  2 = heavy
         *         Bad technique, but it is not worth saying
         *   (N_HEAVY-N_LIGHT+1) everywhere.
         */
        switch (getch()) {
            case 'w':
            case 'W':
                shipsize = 0;
                break;
            case 'g':
            case 'G':
                shipsize = 3;
                break;
            case 'm':
            case 'M':
                shipsize = 6;
                break;
            case ' ':
                if (isgod == TRUE)
                    reset_god();
                return;
            default:
                errormsg("Invalid Ship Type");
                if (isgod == TRUE)
                    reset_god();
                return;
        }

        mvaddstr(LINES - 3, 0, "What ship class to construct: (L)ight  (M)edium");
        if (sct[XREAL][YREAL].designation != DTOWN)
            addstr("  (H)eavy?");
        else
            addstr("?");
        refresh();
        switch (getch()) {
            case 'l':
            case 'L':
                shipsize += N_LIGHT;
                break;
            case 'm':
            case 'M':
                shipsize += N_MEDIUM;
                break;
            case 'h':
            case 'H':
                if (sct[XREAL][YREAL].designation == DTOWN) {
                    errormsg("Towns cannot construct heavy ships");
                    if (isgod == TRUE)
                        reset_god();
                    return;
                }
                shipsize += N_HEAVY;
                break;
            case ' ':
                if (isgod == TRUE)
                    reset_god();
                return;
            default:
                errormsg("Invalid Ship Class");
                if (isgod == TRUE)
                    reset_god();
                return;
        }

        mvaddstr(LINES - 2, 0, "How many ships to construct?");
        refresh();
        amount = safe_long_to_short(get_number());

        /*sanity checks*/
        if ((amount > N_MASK))
            amount = 0;
        if (amount <= 0) {
            if (isgod == TRUE)
                reset_god();
            return;
        }

        /* 6 and 3 hard coded ... see above comment */
        cost = (long)amount * (shipsize % 3 + 1);
        if (shipsize >= 6) {
            cost *= MERSHPCOST;
        } else if (shipsize >= 3) {
            cost *= GALSHPCOST;
        } else {
            cost *= WARSHPCOST;
        }

        if (isgod == TRUE)
            cost = 0;
        else if (magic(country, SAILOR) == TRUE)
            cost /= 2L;

        if ((cost > curntn->tgold) && (cost > 0)) {
            errormsg("sorry - not enough talons");
            return;
        }

        if (sct[XREAL][YREAL].people < amount * (shipsize + 1) * SHIPCREW) {
            errormsg("NOT ENOUGH CIVILIANS IN SECTOR");
            if (isgod == TRUE)
                reset_god();
            return;
        }

        if ((nvynum >= 0) && (nvynum < MAXNAVY)) {
            clear_bottom(0);
            tmpvar = amount * SHIPCREW * (shipsize % 3 + 1) + flthold(nvynum) * P_NCREW;
            onboard = P_NPEOP * fltmhold(nvynum);

            if (shipsize >= 6) {
                shipsize %= 3;
                tmpvar2 = NADD_MER(amount)
            } else if (shipsize >= 3) {
                shipsize %= 3;
                tmpvar2 = NADD_GAL(amount)
            } else {
                shipsize %= 3;
                tmpvar2 = NADD_WAR(amount)
            }

            /* check for bad build */
            if (tmpvar2 == FALSE) {
                errormsg("Too many such ships in fleet.");
                if (isgod == TRUE)
                    reset_god();
                return;
            }

            /* crew average based on number of holding units */
            P_NCREW = safe_long_to_uchar(tmpvar / flthold(nvynum));

            sct[XREAL][YREAL].people -= amount * (shipsize + 1) * SHIPCREW;
            curntn->tgold -= cost;

            P_NXLOC = safe_int_to_uchar(XREAL);
            P_NYLOC = safe_int_to_uchar(YREAL);
            P_NMOVE = 0;

            mvprintw(LINES - 4, 0,
                     "Fleet (%2d):     Warships  = [Light %2hd/Medium %2hd/Heavy %2hd]", nvynum,
                     P_NWAR(N_LIGHT), P_NWAR(N_MEDIUM), P_NWAR(N_HEAVY));
            mvprintw(LINES - 3, 0,
                     "                Merchants = [Light %2hd/Medium %2hd/Heavy %2hd]",
                     P_NMER(N_LIGHT), P_NMER(N_MEDIUM), P_NMER(N_HEAVY));
            mvprintw(LINES - 2, 0,
                     "Avg Crew[%3d]   Galleys   = [Light %2hd/Medium %2hd/Heavy %2hd]",
                     (int)P_NCREW, P_NGAL(N_LIGHT), P_NGAL(N_MEDIUM), P_NGAL(N_HEAVY));

            if (fltmhold(nvynum) > 0)
                P_NPEOP = safe_long_to_uchar(onboard / fltmhold(nvynum));
            else
                P_NPEOP = 0;
            SADJCIV;
            NADJCRW;
            NADJWAR;
            NADJMER;
            NADJGAL;
            NADJHLD;
            NADJLOC;
            NADJMOV;
            mvaddstr(LINES - 2, 65, "HIT ANY KEY");
            refresh();
            getch();

        } else
            errormsg("ERROR!!!!!!!!!!!!!");
    }
    /* construct fortification points*/
    else if (type == 'f') {
        /* can only go into debt as much as the nation has jewels */
        if (sct[XREAL][YREAL].fortress > 11) {
            errormsg("That sector is as impregnable as you can make it");
        } else if ((curntn->tgold - cost) >= ((-1) * 10 * curntn->jewels)) {
            mvprintw(LINES - 2, 5, "you build +%d%% fort points for %ld gold", armbonus, cost);
            curntn->tgold -= cost;
            sct[XREAL][YREAL].fortress++;
            INCFORT;
            errormsg("");
        } else
            errormsg("you may not spend that much");
    } else
        errormsg("invalid input error");

    if (isgod == TRUE)
        reset_god();
    refresh();
}

/*
 * draft - Military unit recruitment system with comprehensive validation and army management
 *
 * Provides complete military recruitment functionality supporting multiple unit types,
 * cost management, army allocation, and special unit handling (spies, scouts).
 * Handles both creation of new armies and reinforcement of existing armies with
 * extensive validation for recruitment limits, resource requirements, and unit
 * placement rules.
 *
 * Unit recruitment system supports multiple categories:
 * 1. Regular Military: Infantry, cavalry, archers, siege weapons
 * 2. Naval Units: Marines and sailors (harbor-restricted)
 * 3. Special Units: Spies (target selection) and scouts
 * 4. Mercenaries: Hired soldiers with availability limits
 *
 * Algorithm:
 * 1. Validate ownership and sector requirements (towns/cities/capitols)
 * 2. Check treasury (positive gold) and population constraints
 * 3. Display available unit types and process user selection
 * 4. Validate special unit restrictions (harbor for naval, etc.)
 * 5. Calculate recruitment numbers and costs with magic modifiers
 * 6. Verify resource availability (gold, metal, population)
 * 7. Determine army allocation (new army vs. existing army)
 * 8. Handle special unit setup (spy targets, scout placement)
 * 9. Deduct costs and population, update army data structures
 * 10. Update adjacent influences and refresh display
 *
 * Recruitment Requirements:
 * - Ownership: Must own the sector (unless god mode)
 * - Location: Must be in town, city, or capitol designation
 * - Treasury: Positive gold balance required
 * - Population: Sufficient civilian population for recruitment
 * - Harbor: Marines/sailors require adjacent water tiles
 * - Army Slots: Available army slots (up to MAXARM per nation)
 *
 * Parameters:
 *   None (operates on global XREAL/YREAL coordinates)
 *
 * Returns:
 *   void (no return value)
 *
 * Side Effects:
 *   - Creates new armies or reinforces existing armies
 *   - Deducts gold and metal from nation treasury
 *   - Reduces civilian population for recruited soldiers
 *   - Increments mercenary counter for mercenary recruitment
 *   - Sets army location, status, and movement points
 *   - Updates adjacent sector influences
 *   - Refreshes map display for new army visibility
 *   - Clears and redraws bottom screen area
 *
 * Unit Type Categories:
 *   - Regular Units: Use standard enlistment costs and metal requirements
 *   - Spies: Single unit, target nation selection, special placement rules
 *   - Scouts: Single unit, automatic SCOUT status assignment
 *   - Marines/Sailors: Harbor requirement, standard costs
 *   - Mercenaries: Special availability limits, no population cost
 *   - Siege Units: SAPPER magic provides cost reduction
 *
 * Cost Structure:
 *   - Gold Cost: u_encost[unit_type] * number_of_men
 *   - Metal Cost: u_enmetal[unit_type] * number_of_men
 *   - Magic Modifiers: WARRIOR/WARLORD/CAPTAIN = 50% gold reduction
 *   - Siege Magic: SAPPER = 50% gold and metal reduction for siege units
 *   - God Mode: All costs bypassed
 *
 * Population Management:
 *   - Draft Limit: Based on initial population (i_people) vs current population
 *   - Formula: max_draft = people - (i_people * 192/256)
 *   - Mercenaries: No population cost, but global availability limit
 *   - City Requirements: Population distribution rules for large cities
 *
 * Army Management:
 *   - New Army: Automatically allocated to first available slot
 *   - Existing Army: Must be same unit type and not ONBOARD
 *   - Status Assignment: New armies default to DEFEND, militia to MILITIA
 *   - Special Units: Spies and scouts get SCOUT status
 *   - Location: Set to recruitment sector coordinates
 *
 * Mercenary System:
 *   - Availability: Global limit of MERCMEN/NTOTAL per nation
 *   - Ratio Limit: Cannot exceed 50% of total army composition
 *   - Tracking: Global mercgot counter tracks total mercenaries recruited
 *   - Cost: Standard gold cost, no metal or population requirements
 *
 * Spy System:
 *   - Target Selection: Choose enemy nation for espionage
 *   - Placement: Known capitols vs. unknown nation handling
 *   - Intelligence: Provides directional hints for unknown nations
 *   - Status: Automatic SCOUT status for stealth operations
 *   - Restrictions: Cannot spy on own nation or inactive nations
 *
 * Harbor Requirements:
 *   - Marines/Sailors: Must have adjacent water tile (WATER altitude)
 *   - Detection: Scans 3x3 grid around recruitment sector
 *   - Validation: Error messages for land-locked recruitment attempts
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires army arrays, nation data, UI interaction
 *   Approach: Integration testing with mock army and nation systems
 *   Key Tests: Cost calculations, population limits, army allocation,
 *             mercenary ratios, harbor validation, spy targeting,
 *             magic modifiers, resource validation
 *   Dependencies: ntn[].armies array, sct[][] sector data, magic() system,
 *                UI functions (mvaddstr, getch), get_country() function
 *   Mock Requirements: Army data structures, nation treasury/population,
 *                     unit type definitions, magic abilities
 *   Complexity: Complex - Multiple unit types, extensive validation rules
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to global state modifications
 *   - Magic Integration: Multiple spells affect recruitment costs and capabilities
 *   - Unit Limits: Hard-coded army limits via MAXARM validation
 *   - Population Tracking: Complex draft limit calculation based on turn start
 *   - Map Integration: Updates display to show new army positions
 *   - Resource Management: Careful validation prevents over-recruitment
 *
 * @last_documented: 2025-10-08
 */
void draft(void) {
    short armynum, x, y, i;
    long men = 0, mercs;
    short army = (-1), isgod = FALSE, newtype = 0;
    long i_cost, e_cost;
    char ch;

    clear_bottom(0);
    if (country == 0) {
        isgod = TRUE;
        country = sct[XREAL][YREAL].owner;
        curntn = &ntn[country];
    } else if (sct[XREAL][YREAL].owner != country) {
        errormsg("You do not own");
        return;
    }

    if ((sct[XREAL][YREAL].designation != DTOWN) && (sct[XREAL][YREAL].designation != DCAPITOL)
        && (sct[XREAL][YREAL].designation != DCITY)) {
        errormsg("must raise in towns/cities/capitols");
        if (isgod == TRUE)
            reset_god();
        return;
    }
    if (curntn->tgold <= 0) {
        errormsg("You are broke");
        if (isgod == TRUE)
            reset_god();
        return;
    }

    if (ISCITY(sct[XREAL][YREAL].designation)
        && (sct[XREAL][YREAL].people * (3 * CITYLIMIT + (curntn->tsctrs / 2)) < curntn->tciv)) {
        mvprintw(LINES - 1, 0, "Need %ld people in sector: hit any key",
                 curntn->tciv / (3 * CITYLIMIT + (curntn->tsctrs / 2)));
        refresh();
        getch();
        if (isgod == TRUE)
            reset_god();
        return;
    }

    /*ask what type of unit*/
    y = safe_int_to_short(LINES - 2);
    mvaddstr(y, 0, "options: 1) spy 2) scout");
    clrtoeol();
    x = 25;
    for (i = 0; i <= NOUNITTYPES; i++) {
        if (unitvalid(i) == TRUE) {
            mvprintw(y, x + 2, "%s", *(shunittype + i));
            mvprintw(y, x, "(%c)", *(shunittype + i)[0]);
            x = safe_int_to_short(x + safe_size_to_short(strlen(*(shunittype + i))) + 3);
            if (x > COLS - 10) {
                x = 0;
                y++;
            }
        }
    }
    move(y, x);
    clrtoeol();
    if ((magic(country, WARRIOR) == TRUE) || (magic(country, WARLORD) == TRUE)
        || (magic(country, CAPTAIN) == TRUE))
        mvaddstr(LINES - 3, 0, "(Warrior = 1/2 enlist cost) what type of unit do you want:");
    else
        mvaddstr(LINES - 3, 0, "what type of unit do you want to raise:");

    clrtoeol();
    refresh();
    ch = safe_int_to_char(getch());
    for (newtype = 0; newtype <= NOUNITTYPES; newtype++)
        if (ch == *(shunittype + newtype)[0])
            break;

    if ((newtype == NOUNITTYPES + 1) || (unitvalid(newtype) == FALSE)) {
        if (ch == '1')
            newtype = A_SPY;
        else if (ch == '2')
            newtype = A_SCOUT;
        else {
            errormsg("Invalid type");
            if (isgod == TRUE)
                reset_god();
            return;
        }
    }
    clear_bottom(0);

    /* marines and sailors may only be drafted in harbors */
    if (newtype == A_MARINES || newtype == A_SAILOR) {
        i = FALSE;
        for (x = safe_int_to_short(XREAL - 1); x <= XREAL + 1; x++)
            for (y = safe_int_to_short(YREAL - 1); y <= YREAL + 1; y++)
                if (sct[x][y].altitude == WATER)
                    i = TRUE;

        /* not a harbor */
        if (i == FALSE) {
            if (newtype == A_MARINES)
                errormsg("Huh?  What would marines do without the water?");
            else
                errormsg("You gotta be kinding!?  Sailors on land?");
            if (isgod == TRUE)
                reset_god();
            return;
        }
    }

    /*raise an untrained army */
    i = FALSE;
    if (newtype == A_SPY || newtype == A_SCOUT) {
        men = 1;
    } else {
        mvprintw(LINES - 3, 0, "how many %s do you wish to raise:", unittype[newtype]);
        clrtoeol();
        refresh();
        men = get_number();
        if (men <= 0) {
            if (isgod == TRUE)
                reset_god();
            return;
        }
    }

    /* i_people*256 is initial people -> can  draft up to following */
    /*	draftable = max_draft - already drafted */
    /*		  = imen/4 - ( imen - people)   */
    /*		  = -3/4 * imen + people)  	*/
    /*	192 comes from 3*256/4 			*/
    if ((newtype != A_MERCENARY
         && (men > sct[XREAL][YREAL].people - (sct[XREAL][YREAL].i_people * 192)))
        || (sct[XREAL][YREAL].i_people < 0)) {
        if (sct[XREAL][YREAL].i_people < 0)
            errormsg("error: sector wasn't city at beginning of turn");
        else
            errormsg("error: raising too many soldiers");
        if (isgod == TRUE)
            reset_god();
        return;
    }

    /* check that you dont have too many mercenaries */
    mercs = 0;
    if (newtype == A_MERCENARY) {
        int totalsolds = 0;
        for (armynum = 0; armynum < MAXARM; armynum++) {
            if (P_ATYPE < MINLEADER) {
                if (P_ATYPE == A_MERCENARY)
                    mercs += P_ASOLD;
                totalsolds = safe_long_to_int(totalsolds + P_ASOLD);
            }
        }
        if (men + mercs > (totalsolds + men) / 2) {
            errormsg("you would then have more than 50%% mercenaries");
            if (isgod == TRUE)
                reset_god();
            return;
        }
        if (mercgot + men > MERCMEN / NTOTAL) {
            errormsg("there are not that many mercanaries available");
            if (isgod == TRUE)
                reset_god();
            return;
        }
    }

    e_cost = (long)*(u_encost + newtype) * men;
    i_cost = (long)*(u_enmetal + newtype) * men;

    /*magiced get 1/2 enlistment costs*/
    if ((magic(country, WARRIOR) == TRUE) || (magic(country, WARLORD) == TRUE)
        || (magic(country, CAPTAIN) == TRUE))
        e_cost /= 2;

    if ((magic(country, SAPPER) == TRUE) && ((newtype == A_SIEGE) || (newtype == A_CATAPULT))) {
        e_cost /= 2;
        i_cost /= 2;
    }

    /*check to see if enough gold*/
    if (e_cost > curntn->tgold) {
        errormsg("You don't have enough talons");
        if (isgod == TRUE)
            reset_god();
        return;
    } else if (i_cost > curntn->metals) {
        mvprintw(LINES - 1, 0, "You don't have %ld metal", i_cost);
        mvaddstr(LINES - 1, COLS - 20, "PRESS ANY KEY");
        clrtoeol();
        refresh();
        getch();
        if (isgod == TRUE)
            reset_god();
        return;
    } else {
        move(LINES - 2, 0);
        clrtoeol();
    }

    /*count is order of that army in sector*/
    /*armynum is number of that army*/
    if ((armynum = safe_int_to_short(getselunit())) >= 0) {
        if (armynum >= MAXARM || newtype == A_SPY || newtype == A_SCOUT) {
            army = -1;
        } else {
            /*if different types, must raise new army*/
            if ((newtype == P_ATYPE) && (P_ASTAT != ONBOARD)) {
                mvaddstr(LINES - 1, 0, "Do you wish to raise a new army:");
                clrtoeol();
                refresh();
                if (getch() != 'y')
                    army = armynum;
                else
                    army = -1;
            } else
                army = (-1);
        }
    }
    if (army == (-1)) {
        mvprintw(LINES - 2, 0, "(%s, gold talons=%ld, metal=%ld) raising a new army",
                 *(unittype + newtype), e_cost, i_cost);
        clrtoeol();
        refresh();
        sleep(1);

        armynum = 0;
        while ((army == (-1)) && (armynum < MAXARM)) {
            if (P_ASOLD <= 0) {
                army = armynum;
                P_ASOLD = 0;
                if (newtype == A_MILITIA)
                    P_ASTAT = MILITIA; /* new militia units=MILITIA */
                else
                    P_ASTAT = DEFEND; /* set new armies to DEFEND */
                AADJSTAT;
                AADJMEN;
            }
            armynum++;
        }
        if (army == (-1)) {
            errormsg("NO FREE ARMIES");
            if (isgod == TRUE)
                reset_god();
            return;
        }
        armynum = army;
    } else {
        mvprintw(LINES - 2, 0, "(%s, gold talons=%ld, metal=%ld) adding to existing army",
                 *(unittype + newtype), e_cost, i_cost);
        clrtoeol();
        refresh();
        sleep(2);
    }

    if (newtype == A_SPY) {
        while (TRUE) {
            clear_bottom(0);
            mvaddstr(LINES - 3, 0, "Spy Against What Nation: ");
            refresh();
            if ((i = safe_int_to_short(get_country())) == (-1)) {
                if (isgod == TRUE)
                    reset_god();
                return;
            }
            if (i == country) {
                errormsg("What?  You don't even trust yourself?");
                i = NTOTAL;
            }
            if (!(isntn(ntn[i].active))) {
                errormsg("You can't spy against them");
                i = NTOTAL;
            }
            if (i < NTOTAL && isactive(i))
                break;
        }
        if (curntn->dstatus[i] != UNMET) {
            P_AYLOC = ntn[i].capy;
            P_AXLOC = ntn[i].capx;
            mvprintw(LINES - 2, 0, "The Spy Starts in %s's Capitol (%d,%d)", ntn[i].name,
                     (int)P_AXLOC, (int)P_AYLOC);
            clrtoeol();
        } else {
            clear_bottom(0);
            mvprintw(LINES - 4, 0, "You do not yet know where %s is", ntn[i].name);
            mvaddstr(LINES - 3, 0, "Have the Spy start from this sector? [y or n]");
            refresh();
            if (getch() != 'y') {
                if (isgod == TRUE)
                    reset_god();
                return;
            }
            P_AYLOC = safe_int_to_uchar(YREAL);
            P_AXLOC = safe_int_to_uchar(XREAL);
            /* Safe widening casts: unsigned char (0-255) coordinates to int for compass() parameters */
            mvprintw(
                LINES - 2, 0, "Intelligence indicates that %s lies to the %s", ntn[i].name,
                *(directions
                  + compass((int)P_AXLOC, (int)P_AYLOC, (int)ntn[i].capx, (int)ntn[i].capy)));
            clrtoeol();
        }
        errormsg("");
        redraw = PART;
    } else {
        P_AYLOC = safe_int_to_uchar(YREAL);
        P_AXLOC = safe_int_to_uchar(XREAL);
    }
    if (newtype == A_SPY || newtype == A_SCOUT) {
        P_ASTAT = SCOUT;
        AADJSTAT;
    }
    P_ATYPE = safe_short_to_uchar(newtype);
    if (P_ATYPE != A_MERCENARY) {
        sct[XREAL][YREAL].people -= men;
        SADJCIV;
    } else {
        mercgot += men;
        AADJMERC;
    }
    AADJLOC;
    P_AMOVE = 0;
    AADJMOV;
    P_ASOLD += men;
    AADJMEN;
    if (P_ASTAT >= NUMSTATUS) {
        P_ASTAT = ATTACK;
        AADJSTAT;
    }
    curntn->metals -= i_cost;
    curntn->tgold -= e_cost;
    makemap(); /* if display 'y' is set, this will show new army */
    if (isgod == TRUE)
        reset_god();
}

/*
 * rmessage - Read and manage messages with selective deletion and file locking
 *
 * Provides comprehensive message reading functionality with interactive message
 * display, selective deletion capabilities, and file locking to prevent
 * concurrent access conflicts. Handles message pagination, user interaction,
 * and atomic file operations to maintain message integrity during reading.
 *
 * The function implements a safe message reading system:
 * 1. Creates temporary file for modified message storage
 * 2. Checks for concurrent mail operations via lock files
 * 3. Displays messages with pagination and user interaction
 * 4. Allows selective message deletion via user input
 * 5. Atomically replaces original file with filtered messages
 *
 * Algorithm:
 * 1. Create temporary file for storing kept messages
 * 2. Open and validate message file existence
 * 3. Check for send lock file to prevent concurrent access
 * 4. Read messages one at a time with full-screen display
 * 5. Handle pagination for long messages (more than screen height)
 * 6. Present user options: keep message or delete message
 * 7. Write kept messages to temporary file
 * 8. Atomically move temporary file to replace original
 * 9. Update mail system size counter for external integration
 *
 * File Operation Safety:
 * - Temporary File: Creates nation-specific .tmp file for atomic operations
 * - Lock Detection: Checks for send.msgfileN files indicating active senders
 * - Atomic Replacement: Uses move_file() for safe file replacement
 * - Error Handling: Cleanup temporary files on errors
 *
 * Parameters:
 *   None (operates on global country variable for file selection)
 *
 * Returns:
 *   void (no return value)
 *
 * Side Effects:
 *   - Modifies message file by removing deleted messages
 *   - Creates and removes temporary files during operation
 *   - Clears and redraws entire screen for message display
 *   - Updates global redraw flag for screen refresh
 *   - Increments conq_mail_size counter (if SYSMAIL enabled)
 *   - May block if concurrent mail operations detected
 *
 * Message Display System:
 *   - Full Screen: Clears screen for each message display
 *   - Pagination: Automatic "more" prompts for long messages
 *   - Highlighting: Uses standout mode for prompts and controls
 *   - Interactive: User controls message deletion via return key
 *   - End Detection: Messages terminated by "END" marker lines
 *
 * User Interface:
 *   - Display: Full-screen message rendering with line-by-line output
 *   - Controls: ANY KEY continues, RETURN deletes current message
 *   - Pagination: Automatic pause at screen boundaries with "more" prompt
 *   - Status: Clear prompts for user actions and message counts
 *
 * File Format:
 *   - Messages: Line-based format with "END" terminators
 *   - Storage: Country-specific files (msgfileN where N=country number)
 *   - Temporary: Uses .tmp extension for atomic operations
 *   - Lock Files: send.msgfileN indicates active mail sending
 *
 * Concurrency Control:
 *   - Send Detection: Checks for active mail sending via lock files
 *   - Time Validation: Removes stale lock files older than TIME_DEAD
 *   - Wait Handling: Returns with message if sender detected
 *   - File Safety: Atomic operations prevent corruption
 *
 * Error Conditions:
 *   - No Messages: Returns early with appropriate user message
 *   - File Errors: Cleanup temporary files and report errors
 *   - Concurrent Access: Detect and defer to active mail operations
 *   - Lock Files: Handle stale locks and active sender detection
 *
 * Integration Points:
 *   - SYSMAIL: Updates mail size counter for external mail integration
 *   - Screen: Uses curses for full-screen message display
 *   - Files: Integrates with move_file() for atomic operations
 *   - Timing: Uses TIME_DEAD constant for lock file aging
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires file system, screen I/O, timing
 *   Approach: Integration testing with mock files and screen simulation
 *   Key Tests: Message pagination, selective deletion, concurrent access,
 *             file operations, lock file handling, error recovery
 *   Dependencies: File system operations, curses screen I/O, timing functions,
 *                move_file() function, message file format
 *   Mock Requirements: File I/O operations, screen functions, timing,
 *                     message file content, lock file simulation
 *   Complexity: Moderate - File operations with concurrency control
 *
 * Notes:
 *   - Thread safety: File locking prevents concurrent access conflicts
 *   - Screen management: Full-screen interface requires careful state management
 *   - File integrity: Atomic operations ensure no message loss during reading
 *   - Performance: Pagination prevents memory issues with large messages
 *   - User experience: Clear prompts and immediate feedback for message management
 *
 * @last_documented: 2025-10-08
 */
void rmessage(void) {
    FILE *mesgfp;
    FILE *fptemp;
    int i;
    int count, msglen;
    int contd;
    int done = FALSE;
    char tempfile[FILELTH];
    char mesgfile[FILELTH];
    char line[LINELTH + 1], inpch;
    char save[LINELTH][LINELTH + 1];
    struct stat fst;

    /*open file; used in mailopen() as well */
    snprintf(tempfile, FILELTH, "%s%hd.tmp", msgfile, country);
    if ((fptemp = fopen(tempfile, "w")) == NULL) {
        clear_bottom(0);
        snprintf(mesgfile, FILELTH, "error: %.67s open", tempfile);
        errormsg(mesgfile);
        redraw = DONE;
        makebottom();
        return;
    }

    snprintf(mesgfile, FILELTH, "%s%d", msgfile, country);
    if ((mesgfp = fopen(mesgfile, "r")) == NULL) {
        fclose(fptemp);
        (void)unlink(tempfile);
        clear_bottom(0);
        errormsg("No Messages");
        makebottom();
        redraw = DONE;
        return;
    }

    /* check for people sending mail */
    snprintf(line, LINELTH + 1, "send.%s%hd", msgfile, country);
    if (stat(line, &fst) == 0) {
        long now;
        now = time(0);
        if (now - fst.st_mtime < TIME_DEAD) {
            /* someone is sending mail to the country */
            fclose(mesgfp);
            fclose(fptemp);
            (void)unlink(tempfile);
            clear_bottom(0);
            errormsg("Someone is sending you mail... please wait.");
            makebottom();
            redraw = DONE;
            return;
        } else {
            /* remove useless file */
            (void)unlink(line);
        }
    }

    /*read in file a line at at time*/
    if (fgets(line, LINELTH, mesgfp) == NULL) {
        done = TRUE;
        redraw = DONE;
        clear_bottom(0);
        errormsg("No messages");
        makebottom();
    }
    if (done == FALSE)
        clear();
    while (done == FALSE) {
        contd = FALSE;
        msglen = 0;
        count = 3;
        clear();
        standout();
        /*print to end of message*/
        while (contd == FALSE) {
            if (msglen < LINELTH) {
                size_t len = strlen(line);
                if (len >= LINELTH)
                    len = LINELTH - 1;
                memcpy(save[msglen], line, len);
                save[msglen][len] = '\0';
            }
            if (count == LINES - 3) {
                standout();
                mvaddstr(LINES - 3, (COLS / 2) - 8, "--- more ---");
                standend();
                refresh();
                getch();
                clear();
                count = 3;
            }
            mvprintw(count, 0, "%s", line);
            standend();
            count++;
            msglen++;
            if (fgets(line, LINELTH, mesgfp) == NULL)
                contd = TRUE;
            if (strncmp(line, "END", 3) == 0)
                contd = TRUE;
        }
        standout();
        mvaddstr(LINES - 3, (COLS / 2) - 13, "HIT ANY KEY TO CONTINUE");
        mvaddstr(LINES - 2, (COLS / 2) - 16, "HIT RETURN TO DELETE MESSAGE");
        standend();
        refresh();
        inpch = safe_int_to_char(getch());
        if ((inpch != '\n' && inpch != '\r')) {
            for (i = 0; i < msglen; i++)
                fputs(save[i], fptemp);
            strncpy(line, "END\n", LINELTH);
            line[LINELTH] = '\0';
            fputs(line, fptemp);
        }
        if (fgets(line, LINELTH, mesgfp) == NULL)
            done = TRUE;
    }
    fclose(mesgfp);
    fclose(fptemp);

    /*IMPLEMENT A MOVE BETWEEN TMP FILE AND REAL FILE HERE*/
    move_file(tempfile, mesgfile);
#ifdef SYSMAIL
    /* increase size to ensure mail check works */
    conq_mail_size++;
#endif /* SYSMAIL */
}

/*
 * wmessage - Interactive message composition and sending system
 *
 * Provides comprehensive message writing functionality with interactive text
 * editing, recipient selection, and multiple message formats. Supports sending
 * to individual nations, news broadcasts, and administrative messages with
 * full-screen text editing capabilities and real-time character input handling.
 *
 * The function implements a sophisticated message composition system:
 * 1. Interactive recipient selection (nations, news, administrator)
 * 2. Full-screen text editor with line editing capabilities
 * 3. Multiple message formats (personal, news, administrative)
 * 4. Real-time character processing with editing features
 * 5. Safe file operations with proper locking mechanisms
 *
 * Algorithm:
 * 1. Prompt for and validate message recipient selection
 * 2. Open mail file with appropriate locking mechanisms
 * 3. Set up full-screen editing interface with instructions
 * 4. Process character input with editing commands (backspace, delete)
 * 5. Handle special commands (Control-D end, ESC abort, line breaks)
 * 6. Format and write message content to appropriate mail file
 * 7. Handle pagination for long messages with screen management
 * 8. Close mail file and complete message sending process
 *
 * Recipient Types:
 * - Individual Nations: Direct messages to specific player nations
 * - News System: Broadcast messages to all players via news
 * - Administrator: Messages to game administrator (god account)
 * - Validation: Checks for active nations and valid recipients
 *
 * Parameters:
 *   None (interactive recipient selection and message composition)
 *
 * Returns:
 *   void (no return value)
 *
 * Side Effects:
 *   - Creates mail files for specified recipients
 *   - Writes formatted message content to mail system
 *   - Clears and manages full-screen editing interface
 *   - Updates global redraw flag for screen refresh
 *   - May create lock files during mail operations
 *   - Handles file system operations for message storage
 *
 * Text Editor Features:
 *   - Character Input: Real-time processing of printable characters
 *   - Backspace/Delete: Line editing with character removal
 *   - Line Management: Automatic line breaks and continuation
 *   - Control Commands: Control-D (end), ESC (abort), period (end)
 *   - Screen Management: Automatic pagination for long messages
 *   - Visual Feedback: Immediate character display and cursor management
 *
 * Message Formats:
 *   - Personal Messages: "Message to [Nation] from [Sender] (Season Year)"
 *   - News Messages: Special news format with sender identification
 *   - Administrative: Different formatting for god/administrator messages
 *   - Timestamping: Automatic date/time stamps with season and year
 *
 * Editing Controls:
 *   - Printable Characters: Direct input up to 65 characters per line
 *   - Backspace (\b, \177): Remove previous character with visual feedback
 *   - Return (\n, \r): Complete current line and advance to next
 *   - Control-D (\004): End message composition (must be at line start)
 *   - ESC (\033): Abort message with confirmation prompt
 *   - Form Feed: Screen refresh and continuation
 *   - Single Period: Alternative message termination
 *
 * Screen Management:
 *   - Full Screen: Complete screen control for editing interface
 *   - Title Display: Recipient information and instructions
 *   - Status Lines: Instructions for ending and aborting messages
 *   - Cursor Control: Real-time cursor positioning during editing
 *   - Pagination: Automatic screen clearing for continued editing
 *   - Visual Feedback: Standout mode for prompts and instructions
 *
 * File Operations:
 *   - Mail Opening: Uses mailopen() with proper locking
 *   - Content Writing: Formatted output to mail files
 *   - File Closing: Uses mailclose() for safe completion
 *   - Error Handling: Proper cleanup on file operation errors
 *   - Format Control: Different output formats based on message type
 *
 * Input Validation:
 *   - Recipient Validation: Checks for valid and active nations
 *   - Character Limits: Line length restrictions (65 characters)
 *   - Command Recognition: Proper handling of special key sequences
 *   - Abort Confirmation: User confirmation for message abortion
 *   - End Validation: Control-D must be at beginning of line
 *
 * Error Handling:
 *   - Invalid Recipients: Early return with appropriate messaging
 *   - File Errors: Proper cleanup and user notification
 *   - Abort Operations: Clean abort with confirmation prompts
 *   - Input Errors: Graceful handling of unexpected input
 *
 * Integration Points:
 *   - Mail System: Integrates with mailopen()/mailclose() functions
 *   - Nation System: Validates against active nation list
 *   - Screen System: Full curses integration for editing interface
 *   - Time System: Uses PSEASON() and YEAR() for timestamping
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires mail system, screen I/O, file operations
 *   Approach: Integration testing with mock mail system and screen simulation
 *   Key Tests: Text editing, recipient validation, message formatting,
 *             file operations, screen management, error handling
 *   Dependencies: Mail system functions, curses screen I/O, nation data,
 *                file system operations, time/date functions
 *   Mock Requirements: Mail system, screen functions, nation validation,
 *                     file operations, user input simulation
 *   Complexity: Moderate - Interactive editing with file operations
 *
 * Notes:
 *   - Thread safety: Mail system locking handles concurrent access
 *   - User experience: Full-screen editing with immediate feedback
 *   - File safety: Proper locking and error handling for mail operations
 *   - Input handling: Comprehensive character processing for editing
 *   - Screen management: Complex screen state management for editing interface
 * @last_documented: 2025-09-18
 */
void wmessage(void) {
    int x = 0, y = 0;
    int done = FALSE;
    char ch;
    char name[NAMELTH + 1];
    int temp = (-1);
    int linedone, dotitles = TRUE;
    char line[BIGLTH];

    /*what nation to send to*/
    clear_bottom(0);
    mvaddstr(LINES - 4, 0,
             "The Conquer Administrator is 'god'; To send to the News use 'news';");
    mvaddstr(LINES - 3, 0, "Send mail to what nation? ");
    refresh();
    temp = get_country();

    if (temp == NEWSMAIL) {
        strncpy(name, "news", NAMELTH);
        name[NAMELTH] = '\0';
    } else {
        /* quick return on bad input */
        if (temp == (-1) || temp >= NTOTAL || (!isntn(ntn[temp].active) && temp != 0)) {
            makebottom();
            return;
        }
        size_t len = strlen(ntn[temp].name);
        if (len >= NAMELTH)
            len = NAMELTH - 1;
        memcpy(name, ntn[temp].name, len);
        name[len] = '\0';
    }

    if (mailopen(temp) == (-1)) {
        makebottom();
        return;
    }
    redraw = FULL;

    if (temp != -2) {
        if (country == 0)
            fprintf(fm, "Message to %s from GOD (%s of year %d)\n\n", name, PSEASON(TURN),
                    YEAR(TURN));
        else
            fprintf(fm, "Message to %s from %s (%s of year %d)\n\n", name, curntn->name,
                    PSEASON(TURN), YEAR(TURN));
    } else
        fprintf(fm, "5.----------\n");
    line[0] = '\0';

    while (done == FALSE) {
        if (dotitles == TRUE) {
            move(0, 0);
            clrtobot();
            standout();
            if (temp != -2)
                mvprintw(3, (COLS - 25) / 2, "Message to Nation %s", name);
            else
                mvaddstr(3, (COLS - 25) / 2, "Message to All Players");
            mvaddstr(LINES - 2, (COLS - 37) / 2, "End with a <Control-D> on a New Line");
            mvaddstr(LINES - 1, (COLS - 28) / 2, "Hit ESC to Abort the Message");
            standend();
            mvaddstr(5, 0, line);
            y = 6;
            x = 0;
            refresh();
            dotitles = FALSE;
        }
        linedone = FALSE;
        ch = ' ';
        /*read line*/
        while (linedone == FALSE) {
            /* check for delete or backspace */
            switch (ch) {
                case '\b':
                case '\177':
                    /* backspace or delete */
                    if (x > 1)
                        x--;
                    mvaddch(y, x, ' ');
                    move(y, x);
                    line[x] = ' ';
                    refresh();
                    ch = safe_int_to_char(getch());
                    break;
                case '\n':
                case '\r':
                    /* newline or carriage return */
                    linedone = TRUE;
                    break;
                case '\004':
                    /* a control-d was hit */
                    if (x == 1) {
                        linedone = TRUE;
                        done = TRUE;
                    } else {
                        standout();
                        mvaddstr(LINES - 3, (COLS - 37) / 2,
                                 "Hit [RETURN] Control-D to End Message");
                        standend();
                        move(y, x);
                        refresh();
                        ch = safe_int_to_char(getch());
                        move(LINES - 3, 0);
                        clrtoeol();
                        refresh();
                    }
                    break;
                case '\033':
                    /* escape key was hit */
                    mvaddstr(LINES - 3, 0, "Abort Message? ");
                    refresh();
                    if (getch() == 'y') {
                        linedone = TRUE;
                        done = TRUE;
                        temp = ABORTMAIL;
                    } else {
                        move(LINES - 3, 0);
                        clrtoeol();
                        move(y, x);
                        refresh();
                        ch = safe_int_to_char(getch());
                    }
                    break;
                case '':
                    /* new page -- end of form */
                    wrefresh(stdscr);
                    ch = safe_int_to_char(getch());
                    break;
                default:
                    /* any remaining possibilities */
                    if (isprint(ch) && (x < 65)) {
                        /*concatonate to end*/
                        line[x] = ch;
                        mvaddch(y, x, ch);
                        x++;
                        refresh();
                    }
                    ch = safe_int_to_char(getch());
                    break;
            }
        }
        if ((ch != '\n') && (ch != '\r') && (ch != '\033')) {
            mvaddch(y, x, ch);
            line[x] = ch;
            x++;
        }
        line[x] = '\0';

        /*check for single period */
        if (strcmp(line, " .") == 0)
            done = TRUE;

        /*write to file*/
        if (done == FALSE) {
            if (temp != -2)
                fprintf(fm, "%s\n", line);
            else {
                if (country != 0)
                    fprintf(fm, "5.%-9s:%s\n", curntn->name, line);
                else
                    fprintf(fm, "5.God      :%s\n", line);
            }
            x = 0;
            y++;
            if (y == LINES - 3) {
                standout();
                mvaddstr(LINES - 3, 0, "Continuing...");
                standend();
                refresh();
                sleep(2);
                dotitles = TRUE;
            }
        }
    }
    mailclose(temp);
}

/*
 * moveciv - Strategic civilian population movement with cost and validation
 *
 * Provides civilian population movement functionality allowing players to
 * relocate civilians between owned sectors within movement distance limits.
 * Handles cost calculations, ownership validation, movement restrictions,
 * and sector accessibility checks with comprehensive error handling.
 *
 * The function implements a controlled civilian migration system:
 * 1. Validates sector ownership and population availability
 * 2. Interactive selection of population count and destination
 * 3. Enforces movement distance limits (maximum 2 sectors)
 * 4. Validates destination ownership and accessibility
 * 5. Processes movement cost and updates population distributions
 *
 * Algorithm:
 * 1. Validate source sector ownership and population existence
 * 2. Prompt for number of people to move with cost display
 * 3. Validate population availability and gold sufficiency
 * 4. Interactive destination coordinate selection (X, Y)
 * 5. Validate movement distance (maximum 2 sectors in each direction)
 * 6. Check destination ownership and sector accessibility
 * 7. Process movement cost deduction and population transfer
 * 8. Update adjacent sector influences for both locations
 *
 * Movement Restrictions:
 * - Ownership: Must own both source and destination sectors
 * - Distance: Maximum 2 sectors in X or Y direction (not diagonal distance)
 * - Population: Cannot move more people than available in source
 * - Cost: 50 gold per civilian moved (must have sufficient treasury)
 * - Accessibility: Destination must have positive move cost (movecost[i][j] >= 0)
 *
 * Parameters:
 *   None (operates on global XREAL/YREAL coordinates for source sector)
 *
 * Returns:
 *   void (no return value)
 *
 * Side Effects:
 *   - Reduces population in source sector
 *   - Increases population in destination sector
 *   - Deducts movement cost from nation treasury (50 gold per person)
 *   - Updates adjacent sector influences for both sectors
 *   - Clears and redraws bottom screen area
 *
 * Cost Structure:
 *   - Fixed Rate: 50 gold talons per civilian moved
 *   - No Magic Modifiers: Movement cost is not affected by magic abilities
 *   - Treasury Check: Validates sufficient gold before allowing movement
 *   - Immediate Deduction: Cost is deducted when movement is executed
 *
 * Distance Validation:
 *   - X-Axis Limit: |destination_x - source_x| <= 2
 *   - Y-Axis Limit: |destination_y - source_y| <= 2
 *   - Independent Axes: Limits apply separately to X and Y coordinates
 *   - No Diagonal Distance: Uses Manhattan-style distance checking
 *
 * Sector Validation:
 *   - Source Ownership: Must own the sector containing civilians to move
 *   - Destination Ownership: Must own the destination sector
 *   - Population Check: Source sector must have civilians present
 *   - Accessibility: Destination sector must allow civilian entry (movecost >= 0)
 *   - God Mode: OGOD compilation flag allows god to bypass ownership checks
 *
 * Error Conditions:
 *   - Ownership Errors: "Sorry, you don't own that sector"
 *   - Population Errors: "Nobody lives there" or "not that many people"
 *   - Cost Errors: "you do not have enough gold talons"
 *   - Distance Errors: "refuse to move more than two sectors"
 *   - Accessibility Errors: "refuse to enter that sector"
 *
 * User Interface:
 *   - Information Display: Shows current population and movement cost
 *   - Coordinate Input: Interactive X and Y coordinate selection
 *   - Error Feedback: Clear error messages for various failure conditions
 *   - Cost Preview: Displays cost per civilian before commitment
 *
 * Integration Points:
 *   - Treasury System: Integrates with nation gold management
 *   - Sector System: Updates population and influence calculations
 *   - Movement System: Uses movecost array for accessibility validation
 *   - God Mode: Conditional compilation for administrative access
 *
 * Testing Notes:
 *   Category: A (Unit) - Isolated function with clear input/output behavior
 *   Approach: Unit testing with mock sector data and user input
 *   Key Tests: Distance validation, ownership checks, cost calculations,
 *             population limits, accessibility validation, error handling
 *   Dependencies: sct[][] sector array, curntn nation data, movecost array,
 *                XREAL/YREAL coordinates, user input functions
 *   Mock Requirements: Sector ownership, population data, treasury values,
 *                     movecost accessibility, user input simulation
 *   Complexity: Simple - Straightforward validation and population transfer
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to global state modifications
 *   - Strategic Purpose: Allows population redistribution for economic planning
 *   - One-time Use: Comment suggests this may be limited use per turn
 *   - Distance Logic: Uses absolute difference checking for movement limits
 *   - God Mode: Conditional ownership bypass for administrative functions
 *
 * @last_documented: 2025-10-08
 */
void moveciv(void) {
    long people;
    short i, j;

    clear_bottom(0);
#ifdef OGOD
    if (sct[XREAL][YREAL].owner != country && country != 0)
#else
    if (sct[XREAL][YREAL].owner != country)
#endif
    {
        errormsg("Sorry, you don't own that sector.");
        return;
    } else if (sct[XREAL][YREAL].people == 0) {
        errormsg("Nobody lives there!!!");
        return;
    }

    mvprintw(LINES - 4, 0, "Sector contains %ld people [cost 50 per civilian]",
             sct[XREAL][YREAL].people);
    mvaddstr(LINES - 3, 0, "How many people to move?");
    clrtoeol();
    refresh();
    people = get_number();
    if (people <= 0) {
        return;
    }
    if (people > sct[XREAL][YREAL].people) {
        errormsg("Sorry, not that many people live there.");
        return;
    }
    if (people * 50 > curntn->tgold) {
        errormsg("Sorry, you do not have enough gold talons.");
        return;
    }

    mvprintw(LINES - 4, 0, "Sector location is x=%d, y=%d", XREAL, YREAL);
    clrtoeol();
    mvaddstr(LINES - 3, 0, "What X location to move to?");
    clrtoeol();
    refresh();
    i = safe_long_to_short(get_number());
    if (i < 0) {
        return;
    }

    if ((i - (XREAL)) > 2 || (i - (XREAL)) < -2) {
        errormsg("Sorry, your people refuse to move more than two sectors.");
        return;
    }

    mvaddstr(LINES - 2, 0, "What Y location to move to?");
    clrtoeol();
    refresh();
    j = safe_long_to_short(get_number());
    if (j < 0) {
        return;
    }
    if ((j - (YREAL) > 2) || ((YREAL)-j > 2)) {
        errormsg("Sorry, your people refuse to move more than two sectors.");
    }
#ifdef OGOD
    else if (sct[i][j].owner != country && country != 0)
#else
    else if (sct[i][j].owner != country)
#endif /*OGOD*/
    {
        errormsg("Sorry, you don't own that sector.");
    } else if (movecost[i][j] < 0) {
        /*need to check move cost > 0 for sector*/
        errormsg("Sorry, your people refuse to enter that sector.");
    } else if ((i != XREAL) || (j != YREAL)) {
        curntn->tgold -= 50 * people;
        sct[XREAL][YREAL].people -= people;
        SADJCIV;
        sct[i][j].people += people;
        SADJCIV2;
    }
}

/*
 * armygoto - Army navigation and selection automation for map interface
 *
 * Provides automated army navigation functionality for the map interface,
 * allowing rapid movement between armies and automatic selection cycling.
 * Handles army validation, screen positioning, cursor management, and
 * selection state updates for efficient army management during gameplay.
 *
 * The function implements intelligent army navigation:
 * 1. Determines next valid army from current selection
 * 2. Validates army existence and availability
 * 3. Updates map cursor position to army location
 * 4. Adjusts selection interface to highlight target army
 * 5. Handles wraparound and fallback for army cycling
 *
 * Algorithm:
 * 1. Get current army selection and increment to next army
 * 2. Search for next army with active soldiers and valid status
 * 3. Handle wraparound to beginning if end of army list reached
 * 4. Calculate screen position relative to map offsets
 * 5. Update cursor position and refresh map display
 * 6. Adjust selector and pager for proper army highlighting
 * 7. Iterate selection interface until target army is selected
 *
 * Army Validation Criteria:
 * - Soldier Count: Army must have P_ASOLD > 0 (active soldiers)
 * - Status Check: Army status must be < NUMSTATUS (not in special group)
 * - Range Check: Army number must be within valid range (0 to MAXARM)
 * - Existence: Army must be allocated and active
 *
 * Parameters:
 *   None (operates on global army and interface state)
 *
 * Returns:
 *   int - 1 if valid army found and selected, 0 if no armies available
 *
 * Side Effects:
 *   - Updates global xcurs and ycurs cursor position
 *   - Modifies global selector and pager for interface highlighting
 *   - Refreshes map display via coffmap() function
 *   - Changes current army selection in interface
 *   - May cycle through multiple interface selections
 *
 * Navigation Logic:
 *   - Sequential Search: Checks armies in ascending order
 *   - Wraparound: Returns to army 0 if no armies found after current
 *   - Fallback: Returns 0 if no valid armies exist at all
 *   - Position Update: Centers map view on selected army location
 *
 * Interface Integration:
 *   - Cursor Management: Updates xcurs/ycurs for map positioning
 *   - Selection System: Manages selector/pager for army highlighting
 *   - Screen Offsets: Accounts for xoffset/yoffset in position calculations
 *   - Map Refresh: Triggers coffmap() to update display
 *
 * Selection Interface:
 *   - Selector: 2-unit increments for selection highlighting
 *   - Pager: Tracks page changes when selector reaches limit (>=10)
 *   - Loop Protection: Maximum 500 iterations to prevent infinite loops
 *   - Target Matching: Continues until getselunit() returns target army
 *
 * Error Handling:
 *   - Invalid Range: Handles army numbers outside valid range
 *   - No Armies: Returns 0 when no valid armies are found
 *   - Loop Protection: Prevents infinite loops in selection interface
 *   - Graceful Fallback: Handles empty army lists appropriately
 *
 * Performance Considerations:
 *   - Sequential Search: O(n) search through army list
 *   - Interface Updates: Multiple screen updates during selection
 *   - Loop Limit: 500-iteration limit prevents excessive processing
 *   - Map Refresh: Single coffmap() call for screen update
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires army data, interface state, map system
 *   Approach: Integration testing with mock army data and interface
 *   Key Tests: Army validation, navigation wraparound, cursor positioning,
 *             selection interface, empty army handling, loop protection
 *   Dependencies: Army data structures, map coordinates, interface globals,
 *                getselunit() function, coffmap() function
 *   Mock Requirements: Army arrays, map positioning, selection state,
 *                     interface variables, screen functions
 *   Complexity: Simple - Linear search with interface updates
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to global state modifications
 *   - UI Integration: Tight coupling with map and selection interfaces
 *   - Army Management: Essential for efficient army navigation in game
 *   - Performance: Optimized for typical army counts in gameplay
 *   - User Experience: Provides smooth army cycling for player convenience
 * @last_documented: 2025-09-18
 */
int armygoto(void) {
    short armynum = 0, loop = 0;
    armynum = safe_int_to_short(getselunit());
    if ((armynum < 0) || (armynum > MAXARM))
        armynum = 0;
    else
        armynum++;
    /* move to next army with > 0 soldiers or army not in group */
    while ((armynum < MAXARM) && ((P_ASOLD <= 0) || (P_ASTAT >= NUMSTATUS)))
        armynum++;
    if (armynum >= MAXARM) {
        armynum = 0;
        if (P_ASOLD <= 0)
            return (0);
    }
    /*move to correct location*/
    xcurs = safe_int_to_short((int)P_AXLOC - xoffset);
    ycurs = safe_int_to_short((int)P_AYLOC - yoffset);
    coffmap();

    /*select correct unit*/
    selector = 0;
    pager = 0;
    while ((getselunit() != armynum) && (loop++ < 500)) {
        selector += 2;
        if (selector >= 10) {
            selector = 0;
            pager += 1;
        }
    }
    return (1);
}

/*
 * navygoto - Naval fleet navigation and selection automation for map interface
 *
 * Provides automated naval fleet navigation functionality for the map interface,
 * allowing rapid movement between fleets and automatic selection cycling.
 * Handles fleet validation, screen positioning, cursor management, and
 * selection state updates for efficient naval fleet management during gameplay.
 *
 * The function implements intelligent naval fleet navigation:
 * 1. Determines next valid fleet from current selection
 * 2. Validates fleet existence and ship availability
 * 3. Updates map cursor position to fleet location
 * 4. Adjusts selection interface to highlight target fleet
 * 5. Handles wraparound and fallback for fleet cycling
 *
 * Algorithm:
 * 1. Get current navy selection (offset by MAXARM) and increment to next fleet
 * 2. Search for next fleet with active ships (merchants, warships, or galleys)
 * 3. Handle wraparound to beginning if end of fleet list reached
 * 4. Calculate screen position relative to map offsets
 * 5. Update cursor position and refresh map display
 * 6. Adjust selector and pager for proper fleet highlighting
 * 7. Iterate selection interface until target fleet is selected
 *
 * Fleet Validation Criteria:
 * - Ship Count: Fleet must have ships (P_NMSHP > 0 OR P_NWSHP > 0 OR P_NGSHP > 0)
 * - Range Check: Fleet number must be within valid range (0 to MAXNAVY)
 * - Existence: Fleet must be allocated and contain at least one ship type
 * - Active Status: Fleet must be actively deployed and operational
 *
 * Parameters:
 *   None (operates on global fleet and interface state)
 *
 * Returns:
 *   int - 1 if valid fleet found and selected, 0 if no fleets available
 *
 * Side Effects:
 *   - Updates global xcurs and ycurs cursor position
 *   - Modifies global selector and pager for interface highlighting
 *   - Refreshes map display via coffmap() function
 *   - Changes current fleet selection in interface
 *   - May cycle through multiple interface selections
 *
 * Navigation Logic:
 *   - Sequential Search: Checks fleets in ascending order
 *   - Wraparound: Returns to fleet 0 if no fleets found after current
 *   - Fallback: Returns 0 if no valid fleets exist at all
 *   - Position Update: Centers map view on selected fleet location
 *   - Navy Offset: Accounts for MAXARM offset in navy numbering
 *
 * Ship Type Validation:
 *   - Merchant Ships: P_NMSHP count for cargo and trade vessels
 *   - War Ships: P_NWSHP count for combat vessels
 *   - Galleys: P_NGSHP count for versatile naval units
 *   - Any Type: Fleet is valid if any ship type count > 0
 *
 * Interface Integration:
 *   - Cursor Management: Updates xcurs/ycurs for map positioning
 *   - Selection System: Manages selector/pager for fleet highlighting
 *   - Screen Offsets: Accounts for xoffset/yoffset in position calculations
 *   - Map Refresh: Triggers coffmap() to update display
 *   - Navy Indexing: Adjusts for MAXARM offset in fleet numbering
 *
 * Selection Interface:
 *   - Selector: 2-unit increments for selection highlighting
 *   - Pager: Tracks page changes when selector reaches limit (>=10)
 *   - Loop Protection: Maximum 500 iterations to prevent infinite loops
 *   - Target Matching: Continues until (getselunit()-MAXARM) returns target fleet
 *   - Navy Offset: Handles MAXARM offset for proper fleet identification
 *
 * Fleet Position Management:
 *   - Location Access: Uses P_NXLOC and P_NYLOC for fleet coordinates
 *   - Screen Mapping: Converts world coordinates to screen coordinates
 *   - Offset Calculation: Accounts for current map view offsets
 *   - Cursor Update: Updates xcurs/ycurs for immediate visual feedback
 *
 * Error Handling:
 *   - Invalid Range: Handles fleet numbers outside valid range
 *   - No Fleets: Returns 0 when no valid fleets are found
 *   - Loop Protection: Prevents infinite loops in selection interface
 *   - Graceful Fallback: Handles empty fleet lists appropriately
 *   - Empty Ships: Handles fleets with zero ships in all categories
 *
 * Performance Considerations:
 *   - Sequential Search: O(n) search through fleet list
 *   - Interface Updates: Multiple screen updates during selection
 *   - Loop Limit: 500-iteration limit prevents excessive processing
 *   - Map Refresh: Single coffmap() call for screen update
 *   - Ship Counting: Multiple ship type checks per fleet
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires fleet data, interface state, map system
 *   Approach: Integration testing with mock fleet data and interface
 *   Key Tests: Fleet validation, ship type checking, navigation wraparound,
 *             cursor positioning, selection interface, empty fleet handling,
 *             loop protection, navy offset handling
 *   Dependencies: Fleet data structures, map coordinates, interface globals,
 *                getselunit() function, coffmap() function, MAXARM constant
 *   Mock Requirements: Fleet arrays, ship counts, map positioning, selection state,
 *                     interface variables, screen functions
 *   Complexity: Simple - Linear search with interface updates and ship validation
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to global state modifications
 *   - UI Integration: Tight coupling with map and selection interfaces
 *   - Fleet Management: Essential for efficient naval navigation in game
 *   - Navy Numbering: Uses MAXARM offset to distinguish from army units
 *   - Ship Types: Supports three distinct ship categories for tactical diversity
 *   - User Experience: Provides smooth fleet cycling for naval operations
 * @last_documented: 2025-09-18
 */
int navygoto(void) {
    short nvynum = 0, loop = 0;
    nvynum = safe_int_to_short(getselunit() - MAXARM);
    if ((nvynum < 0) || (nvynum > MAXNAVY))
        nvynum = 0;
    else
        nvynum++;
    /* move to next nvy with > 0 soldiers*/
    while ((nvynum < MAXNAVY) && (P_NMSHP == 0) && (P_NWSHP == 0) && (P_NGSHP == 0))
        nvynum++;
    if (nvynum >= MAXNAVY) {
        nvynum = 0;
        if ((P_NMSHP == 0) && (P_NWSHP == 0) && (P_NGSHP == 0))
            return (0);
    }
    /*move to correct location*/
    xcurs = P_NXLOC - xoffset;
    ycurs = P_NYLOC - yoffset;
    coffmap();

    /*select correct unit*/
    selector = 0;
    pager = 0;
    while (((getselunit() - MAXARM) != nvynum) && (loop++ < 500)) {
        selector += 2;
        if (selector >= 10) {
            selector = 0;
            pager += 1;
        }
    }
    return (1);
}
