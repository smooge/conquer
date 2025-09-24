/*
 * navy.c - Naval units and fleet management
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
#include "header.h"
#include "data.h"
#include "safe_convert.h"

/*
 * addwships - Add warships to naval fleet using bit manipulation
 *
 * Increases the warship count for a specified fleet by adding ships of a given
 * size class. Uses sophisticated bit manipulation to encode multiple ship counts
 * in a single integer variable, with each ship size occupying a specific bit field.
 *
 * The function employs a pack/unpack strategy where ship counts are stored in
 * discrete bit fields within the fleet's warship variable (P_NWSHP). Each ship
 * size class (light, medium, heavy) occupies N_BITSIZE bits, allowing efficient
 * storage of multiple ship type counts in a single integer.
 *
 * Parameters:
 *   nvynum - Fleet number identifier (0 to MAXNAVY-1)
 *   shipsize - Ship size class (N_LIGHT, N_MEDIUM, or N_HEAVY)
 *   nships - Number of ships to add to the fleet
 *
 * Returns:
 *   TRUE (1) on successful addition
 *   FALSE (0) if operation fails due to invalid parameters or overflow
 *
 * Side Effects:
 *   - Modifies P_NWSHP global variable for the current nation's fleet
 *   - Updates bit field corresponding to specified ship size
 *   - Preserves other ship size counts through bit masking
 *
 * Algorithm:
 *   1. Validate input parameters (fleet number and ship size bounds)
 *   2. Calculate new ship count and check for overflow (N_MASK limit)
 *   3. Position new count in appropriate bit field using left shift
 *   4. Use bitwise OR to set new value in target bit field
 *   5. Create preservation mask to protect other bit fields
 *   6. Apply mask using bitwise AND to finalize update
 *
 * Testing Notes:
 *   Category: A (Unit) - Isolated bit manipulation with deterministic behavior
 *   Approach: Unit tests with various ship counts, sizes, and boundary conditions
 *   Key Tests: Overflow detection, bit field isolation, mask preservation
 *   Dependencies: P_NWSHP global variable, N_MASK and N_BITSIZE constants
 *   Mock Requirements: Global fleet state setup
 *   Complexity: Moderate - Bit manipulation requires careful boundary testing
 */
int
addwships(short nvynum, short shipsize, short nships)
{
	int hold=nships;

	/* return FALSE if fails, TRUE otherwise */

	/* check for valid input */
	if (nvynum<0 || nvynum>=MAXNAVY ||
	shipsize<N_LIGHT || shipsize>N_HEAVY)
		return(FALSE);

	hold += P_NWAR(shipsize);
	if (hold > N_MASK) {
		return(FALSE);
	}

	/* place new value in proper place */
	hold <<= (N_BITSIZE*shipsize);

	/* set it in variable */
	P_NWSHP |= safe_int_to_ushort(hold);

	/* form mask so other values won't be changed */
	hold |= ~(N_MASK<<(N_BITSIZE*shipsize));

	/* now change the variable */
	P_NWSHP &= safe_int_to_ushort(hold);
	return(TRUE);
}

/*
 * addmships - Add merchant ships to naval fleet using bit manipulation
 *
 * Increases the merchant ship count for a specified fleet by adding ships of a
 * given size class. Uses identical bit manipulation strategy as addwships() but
 * operates on the merchant ship variable (P_NMSHP) instead of warships.
 *
 * Merchant ships serve economic and transport functions, with storage capacity
 * varying by size class. The function maintains the same sophisticated bit field
 * encoding where each ship size class occupies N_BITSIZE bits within P_NMSHP.
 *
 * Parameters:
 *   nvynum - Fleet number identifier (0 to MAXNAVY-1)
 *   shipsize - Ship size class (N_LIGHT, N_MEDIUM, or N_HEAVY)
 *   nships - Number of merchant ships to add to the fleet
 *
 * Returns:
 *   TRUE (1) on successful addition
 *   FALSE (0) if operation fails due to invalid parameters or overflow
 *
 * Side Effects:
 *   - Modifies P_NMSHP global variable for the current nation's fleet
 *   - Updates bit field corresponding to specified ship size
 *   - Preserves other ship size counts through bit masking
 *
 * Algorithm:
 *   1. Validate input parameters (fleet number and ship size bounds)
 *   2. Calculate new ship count and check for overflow (N_MASK limit)
 *   3. Position new count in appropriate bit field using left shift
 *   4. Use bitwise OR to set new value in target bit field
 *   5. Create preservation mask to protect other bit fields
 *   6. Apply mask using bitwise AND to finalize update
 *
 * Testing Notes:
 *   Category: A (Unit) - Isolated bit manipulation with deterministic behavior
 *   Approach: Unit tests with various ship counts, sizes, and boundary conditions
 *   Key Tests: Overflow detection, bit field isolation, mask preservation
 *   Dependencies: P_NMSHP global variable, N_MASK and N_BITSIZE constants
 *   Mock Requirements: Global fleet state setup
 *   Complexity: Moderate - Bit manipulation requires careful boundary testing
 */
int 
addmships (int nvynum, int shipsize, int nships)
{
	int hold=nships;

	/* return FALSE if fails, TRUE otherwise */

	/* check for valid input */
	if (nvynum<0 || nvynum>=MAXNAVY ||
	shipsize<N_LIGHT || shipsize>N_HEAVY)
		return(FALSE);

	hold += P_NMER(shipsize);
	if (hold > N_MASK) {
		return(FALSE);
	}

	/* place new value in proper place */
	hold <<= (N_BITSIZE*shipsize);

	/* set it in variable */
	P_NMSHP |= safe_int_to_ushort(hold);

	/* form mask so other values won't be changed */
	hold |= ~(N_MASK<<(N_BITSIZE*shipsize));

	/* now change the variable */
	P_NMSHP &= safe_int_to_ushort(hold);
	return(TRUE);
}

/*
 * addgships - Add galley ships to naval fleet using bit manipulation
 *
 * Increases the galley ship count for a specified fleet by adding ships of a
 * given size class. Uses the same bit manipulation strategy as other ship
 * addition functions but operates on the galley ship variable (P_NGSHP).
 *
 * Galleys represent classical oared vessels with different capabilities from
 * warships and merchants. They provide both military and transport capacity,
 * making them versatile naval units. The function maintains the established
 * bit field encoding pattern for efficient storage.
 *
 * Parameters:
 *   nvynum - Fleet number identifier (0 to MAXNAVY-1)
 *   shipsize - Ship size class (N_LIGHT, N_MEDIUM, or N_HEAVY)
 *   nships - Number of galley ships to add to the fleet
 *
 * Returns:
 *   TRUE (1) on successful addition
 *   FALSE (0) if operation fails due to invalid parameters or overflow
 *
 * Side Effects:
 *   - Modifies P_NGSHP global variable for the current nation's fleet
 *   - Updates bit field corresponding to specified ship size
 *   - Preserves other ship size counts through bit masking
 *
 * Algorithm:
 *   1. Validate input parameters (fleet number and ship size bounds)
 *   2. Calculate new ship count and check for overflow (N_MASK limit)
 *   3. Position new count in appropriate bit field using left shift
 *   4. Use bitwise OR to set new value in target bit field
 *   5. Create preservation mask to protect other bit fields
 *   6. Apply mask using bitwise AND to finalize update
 *
 * Testing Notes:
 *   Category: A (Unit) - Isolated bit manipulation with deterministic behavior
 *   Approach: Unit tests with various ship counts, sizes, and boundary conditions
 *   Key Tests: Overflow detection, bit field isolation, mask preservation
 *   Dependencies: P_NGSHP global variable, N_MASK and N_BITSIZE constants
 *   Mock Requirements: Global fleet state setup
 *   Complexity: Moderate - Bit manipulation requires careful boundary testing
 */
int 
addgships (int nvynum, int shipsize, int nships)
{
	int hold=nships;

	/* return FALSE if fails, TRUE otherwise */

	/* check for valid input */
	if (nvynum<0 || nvynum>=MAXNAVY ||
	shipsize<N_LIGHT || shipsize>N_HEAVY)
		return(FALSE);

	hold += P_NGAL(shipsize);
	if (hold > N_MASK) {
		return(FALSE);
	}

	/* place new value in proper place */
	hold <<= (N_BITSIZE*shipsize);

	/* set it in variable */
	P_NGSHP |= safe_int_to_ushort(hold);

	/* form mask so other values won't be changed */
	hold |= ~(N_MASK<<(N_BITSIZE*shipsize));

	/* now change the variable */
	P_NGSHP &= safe_int_to_ushort(hold);
	return(TRUE);
}

/*
 * subwships - Remove warships from naval fleet using bit manipulation
 *
 * Decreases the warship count for a specified fleet by removing ships of a
 * given size class. Uses the inverse of addwships() algorithm, performing
 * subtraction before applying the same bit manipulation techniques to update
 * the fleet's warship variable (P_NWSHP).
 *
 * The function performs underflow protection by checking that sufficient ships
 * exist before removal. If removal would result in negative ship count, the
 * operation is silently ignored to prevent fleet corruption.
 *
 * Parameters:
 *   nvynum - Fleet number identifier (0 to MAXNAVY-1)
 *   shipsize - Ship size class (N_LIGHT, N_MEDIUM, or N_HEAVY)
 *   nships - Number of warships to remove from the fleet
 *
 * Returns:
 *   void - No return value (silent failure on invalid operations)
 *
 * Side Effects:
 *   - Modifies P_NWSHP global variable for the current nation's fleet
 *   - Updates bit field corresponding to specified ship size
 *   - Preserves other ship size counts through bit masking
 *   - Silently ignores operations that would cause underflow
 *
 * Algorithm:
 *   1. Validate input parameters (fleet number and ship size bounds)
 *   2. Calculate new ship count and check for underflow (negative result)
 *   3. Position new count in appropriate bit field using left shift
 *   4. Use bitwise OR to set new value in target bit field
 *   5. Create preservation mask to protect other bit fields
 *   6. Apply mask using bitwise AND to finalize update
 *
 * Testing Notes:
 *   Category: A (Unit) - Isolated bit manipulation with deterministic behavior
 *   Approach: Unit tests with various ship counts, underflow conditions
 *   Key Tests: Underflow protection, bit field isolation, mask preservation
 *   Dependencies: P_NWSHP global variable, N_MASK and N_BITSIZE constants
 *   Mock Requirements: Global fleet state setup with existing ships
 *   Complexity: Moderate - Bit manipulation with underflow protection logic
 */
void 
subwships (int nvynum, int shipsize, int nships)
{
	int hold;

	/* return FALSE if fails, TRUE otherwise */

	/* check for valid input */
	if (nvynum<0 || nvynum>=MAXNAVY ||
	shipsize<N_LIGHT || shipsize>N_HEAVY)
		return;

	hold = P_NWAR(shipsize) - nships;
	if (hold < 0) {
		return;
	}

	/* place new value in proper place */
	hold <<= (N_BITSIZE*shipsize);

	/* set it in variable */
	P_NWSHP |= safe_int_to_ushort(hold);

	/* form mask so other values won't be changed */
	hold |= ~(N_MASK<<(N_BITSIZE*shipsize));

	/* now change the variable */
	P_NWSHP &= safe_int_to_ushort(hold);
}

/*
 * submships - Remove merchant ships from naval fleet using bit manipulation
 *
 * Decreases the merchant ship count for a specified fleet by removing ships of
 * a given size class. Uses the inverse of addmships() algorithm, performing
 * subtraction before applying bit manipulation techniques to update the fleet's
 * merchant ship variable (P_NMSHP).
 *
 * The function performs underflow protection by checking that sufficient ships
 * exist before removal. Merchant ship removal affects cargo capacity and
 * economic transport capabilities of the fleet.
 *
 * Parameters:
 *   nvynum - Fleet number identifier (0 to MAXNAVY-1)
 *   shipsize - Ship size class (N_LIGHT, N_MEDIUM, or N_HEAVY)
 *   nships - Number of merchant ships to remove from the fleet
 *
 * Returns:
 *   void - No return value (silent failure on invalid operations)
 *
 * Side Effects:
 *   - Modifies P_NMSHP global variable for the current nation's fleet
 *   - Updates bit field corresponding to specified ship size
 *   - Preserves other ship size counts through bit masking
 *   - Silently ignores operations that would cause underflow
 *   - Reduces fleet cargo capacity proportional to ships removed
 *
 * Algorithm:
 *   1. Validate input parameters (fleet number and ship size bounds)
 *   2. Calculate new ship count and check for underflow (negative result)
 *   3. Position new count in appropriate bit field using left shift
 *   4. Use bitwise OR to set new value in target bit field
 *   5. Create preservation mask to protect other bit fields
 *   6. Apply mask using bitwise AND to finalize update
 *
 * Testing Notes:
 *   Category: A (Unit) - Isolated bit manipulation with deterministic behavior
 *   Approach: Unit tests with various ship counts, underflow conditions
 *   Key Tests: Underflow protection, bit field isolation, capacity reduction
 *   Dependencies: P_NMSHP global variable, N_MASK and N_BITSIZE constants
 *   Mock Requirements: Global fleet state setup with existing merchant ships
 *   Complexity: Moderate - Bit manipulation with underflow protection logic
 */
void 
submships (int nvynum, int shipsize, int nships)
{
	int hold;

	/* return FALSE if fails, TRUE otherwise */

	/* check for valid input */
	if (nvynum<0 || nvynum>=MAXNAVY ||
	shipsize<N_LIGHT || shipsize>N_HEAVY)
		return;

	hold = P_NMER(shipsize) - nships;
	if (hold < 0) {
		return;
	}
	/* place new value in proper place */
	hold <<= (N_BITSIZE*shipsize);

	/* set it in variable */
	P_NMSHP |= safe_int_to_ushort(hold);

	/* form mask so other values won't be changed */
	hold |= ~(N_MASK<<(N_BITSIZE*shipsize));

	/* now change the variable */
	P_NMSHP &= safe_int_to_ushort(hold);
}

/*
 * subgships - Remove galley ships from naval fleet using bit manipulation
 *
 * Decreases the galley ship count for a specified fleet by removing ships of
 * a given size class. Uses the inverse of addgships() algorithm, performing
 * subtraction before applying bit manipulation techniques to update the fleet's
 * galley ship variable (P_NGSHP).
 *
 * The function performs underflow protection by checking that sufficient ships
 * exist before removal. Galley removal affects both military capability and
 * transport capacity, as galleys serve dual-purpose roles in the naval system.
 *
 * Parameters:
 *   nvynum - Fleet number identifier (0 to MAXNAVY-1)
 *   shipsize - Ship size class (N_LIGHT, N_MEDIUM, or N_HEAVY)
 *   nships - Number of galley ships to remove from the fleet
 *
 * Returns:
 *   void - No return value (silent failure on invalid operations)
 *
 * Side Effects:
 *   - Modifies P_NGSHP global variable for the current nation's fleet
 *   - Updates bit field corresponding to specified ship size
 *   - Preserves other ship size counts through bit masking
 *   - Silently ignores operations that would cause underflow
 *   - Reduces fleet military and transport capacity
 *
 * Algorithm:
 *   1. Validate input parameters (fleet number and ship size bounds)
 *   2. Calculate new ship count and check for underflow (negative result)
 *   3. Position new count in appropriate bit field using left shift
 *   4. Use bitwise OR to set new value in target bit field
 *   5. Create preservation mask to protect other bit fields
 *   6. Apply mask using bitwise AND to finalize update
 *
 * Testing Notes:
 *   Category: A (Unit) - Isolated bit manipulation with deterministic behavior
 *   Approach: Unit tests with various ship counts, underflow conditions
 *   Key Tests: Underflow protection, bit field isolation, capacity reduction
 *   Dependencies: P_NGSHP global variable, N_MASK and N_BITSIZE constants
 *   Mock Requirements: Global fleet state setup with existing galley ships
 *   Complexity: Moderate - Bit manipulation with underflow protection logic
 */
void 
subgships (int nvynum, int shipsize, int nships)
{
	int hold;

	/* return FALSE if fails, TRUE otherwise */

	/* check for valid input */
	if (nvynum<0 || nvynum>=MAXNAVY ||
	shipsize<N_LIGHT || shipsize>N_HEAVY)
		return;

	hold = P_NGAL(shipsize) - nships;
	if (hold < 0) {
		return;
	}
	/* place new value in proper place */
	hold <<= (N_BITSIZE*shipsize);

	/* set it in variable */
	P_NGSHP |= safe_int_to_ushort(hold);

	/* form mask so other values won't be changed */
	hold |= ~(N_MASK<<(N_BITSIZE*shipsize));

	/* now change the variable */
	P_NGSHP &= safe_int_to_ushort(hold);
	return;
}

/*
 * fltships - Calculate total number of ships in a fleet across all types
 *
 * Computes the complete ship count for a specified fleet by iterating through
 * all ship size classes (light, medium, heavy) and all ship types (warships,
 * merchants, galleys). Uses nation switching to access fleet data for any
 * country, then restores the original nation context.
 *
 * The function performs bit field extraction for each ship type and size
 * combination using P_NWAR(), P_NMER(), and P_NGAL() macros. This provides
 * a comprehensive fleet size assessment for strategic planning and display.
 *
 * Parameters:
 *   country - Nation identifier (0 to MAXNTN-1) whose fleet to examine
 *   nvynum - Fleet number identifier (0 to MAXNAVY-1)
 *
 * Returns:
 *   Total number of ships in the fleet across all types and sizes
 *   0 if fleet is empty or invalid parameters
 *
 * Side Effects:
 *   - Temporarily switches curntn to access target country's fleet data
 *   - Restores original curntn before returning
 *   - No permanent modification of fleet or nation state
 *
 * Algorithm:
 *   1. Save current nation context for restoration
 *   2. Switch to target country's nation structure
 *   3. Iterate through ship size classes (N_LIGHT to N_HEAVY)
 *   4. For each size, accumulate warship, merchant, and galley counts
 *   5. Restore original nation context
 *   6. Return total accumulated ship count
 *
 * Testing Notes:
 *   Category: A (Unit) - Isolated calculation with deterministic behavior
 *   Approach: Unit tests with various fleet configurations and country switching
 *   Key Tests: Nation context handling, comprehensive ship counting, empty fleets
 *   Dependencies: P_NWAR/P_NMER/P_NGAL macros, curntn global variable
 *   Mock Requirements: Multi-nation setup with populated fleets
 *   Complexity: Simple - Mathematical accumulation with context management
 */
int 
fltships (int country, int nvynum)
{
	struct s_nation *savntn=curntn;
	int i, hold=0;

	curntn= &ntn[country];
	for(i=N_LIGHT;i<=N_HEAVY;i++) {
		hold += (int) P_NWAR(i);
		hold += (int) P_NMER(i);
		hold += (int) P_NGAL(i);
	}
	curntn= savntn;
	return(hold);
}

#ifdef ADMIN
/*
 * fltspeed - Calculate fleet movement speed based on slowest ship (ADMIN only)
 *
 * Determines the maximum movement speed for a fleet by finding the slowest
 * ship type and size within the fleet composition. Fleet speed is constrained
 * by the least maneuverable vessel, following realistic naval movement principles.
 *
 * The function iterates through all ship types (warships, merchants, galleys)
 * and size classes (light, medium, heavy), calculating speed for each based on
 * base speed constants and size modifiers. Lighter ships move faster than
 * heavier ships within each ship type category.
 *
 * Speed calculation uses type-specific base speeds (N_WSPD, N_MSPD, N_GSPD)
 * modified by size-dependent speed adjustments (N_SIZESPD). The formula
 * applies: base_speed + (2-size_class) * N_SIZESPD.
 *
 * Parameters:
 *   nvynum - Fleet number identifier (0 to MAXNAVY-1)
 *
 * Returns:
 *   Fleet movement speed (lowest among all ships in fleet)
 *   N_NOSPD if fleet contains no ships
 *
 * Side Effects:
 *   - No modification of fleet or game state
 *   - Read-only access to current nation's fleet data
 *
 * Algorithm:
 *   1. Initialize speed to maximum value (99)
 *   2. Check warships by size, update speed if slower ships found
 *   3. Check merchants by size, update speed if slower ships found
 *   4. Check galleys by size, update speed if slower ships found
 *   5. Return N_NOSPD if no ships found, otherwise return calculated speed
 *
 * Notes:
 *   - ADMIN compilation flag required - administrative tool only
 *   - Used for strategic planning and fleet movement calculation
 *   - Speed decreases as ship size increases (heavy < medium < light)
 *   - Each ship type has different base movement characteristics
 *
 * Testing Notes:
 *   Category: A (Unit) - Isolated calculation with deterministic behavior
 *   Approach: Unit tests with various fleet compositions and speed calculations
 *   Key Tests: Empty fleet handling, mixed ship types, size speed modifiers
 *   Dependencies: P_NWAR/P_NMER/P_NGAL macros, speed constants
 *   Mock Requirements: Fleet setup with various ship types and sizes
 *   Complexity: Simple - Mathematical minimum calculation with iteration
 */
unsigned short 
fltspeed (int nvynum)
{
	int i,hold=99;

	/* light ships faster than heavy ships */
	for (i=N_LIGHT;i<=N_HEAVY && hold>N_WSPD;i++)
		if (P_NWAR(i)>0) hold=N_WSPD+(2-i)*N_SIZESPD;

	for (i=N_LIGHT;i<=N_HEAVY && hold>N_MSPD;i++)
		if (P_NMER(i)>0) hold=N_MSPD+(2-i)*N_SIZESPD;

	for (i=N_LIGHT;i<=N_HEAVY && hold>N_GSPD;i++)
		if (P_NGAL(i)>0) hold=N_GSPD+(2-i)*N_SIZESPD;

	if (hold==99) hold=N_NOSPD;
	return(safe_int_to_ushort(hold));
}
#endif /* ADMIN */

/*
 * flthold - Calculate total cargo storage capacity across entire fleet
 *
 * Computes the complete cargo capacity for a fleet by calculating storage
 * contributions from all ship types (warships, merchants, galleys) and all
 * size classes (light, medium, heavy). Each ship contributes storage based
 * on its size class using a progressive capacity system.
 *
 * Storage capacity scales with ship size: light ships provide 1 unit,
 * medium ships provide 2 units, and heavy ships provide 3 units of cargo
 * space. This applies uniformly across all ship types, representing the
 * larger hull capacity of bigger vessels.
 *
 * The function aggregates storage from all ship types, making it the
 * primary capacity calculation for fleet logistics and cargo planning.
 * This total capacity determines how much cargo (people, armies, goods)
 * the fleet can transport.
 *
 * Parameters:
 *   nvynum - Fleet number identifier (0 to MAXNAVY-1)
 *
 * Returns:
 *   Total cargo storage capacity across all ships in fleet
 *   0 if fleet is empty or contains no ships
 *
 * Side Effects:
 *   - No modification of fleet or game state
 *   - Read-only access to current nation's fleet data
 *
 * Algorithm:
 *   1. Initialize total capacity to 0
 *   2. Iterate through ship size classes (N_LIGHT to N_HEAVY)
 *   3. For each size, calculate capacity: (size+1) * ship_count
 *   4. Add warship, merchant, and galley contributions
 *   5. Return accumulated total capacity
 *
 * Capacity Formula:
 *   - Light ships (size 0): (0+1) * count = 1 * count
 *   - Medium ships (size 1): (1+1) * count = 2 * count
 *   - Heavy ships (size 2): (2+1) * count = 3 * count
 *
 * Testing Notes:
 *   Category: A (Unit) - Isolated calculation with deterministic behavior
 *   Approach: Unit tests with various fleet compositions and capacity calculations
 *   Key Tests: Mixed ship types, size capacity scaling, empty fleet handling
 *   Dependencies: P_NWAR/P_NMER/P_NGAL macros, ship size constants
 *   Mock Requirements: Fleet setup with various ship types and sizes
 *   Complexity: Simple - Mathematical accumulation with size-based multipliers
 */
int 
flthold (int nvynum)
{
	int i,hold=0;

	/* light ships faster than heavy ships */
	for (i=N_LIGHT;i<=N_HEAVY;i++)
	{
		hold += (i+1) * (int)P_NWAR(i);
		hold += (i+1) * (int)P_NMER(i);
		hold += (i+1) * (int)P_NGAL(i);
	}
	return(hold);
}

#ifdef ADMIN
/*
 * fltwhold - Calculate cargo storage capacity for warships only (ADMIN only)
 *
 * Computes the cargo capacity contribution from warships exclusively within
 * a fleet, ignoring merchants and galleys. This specialized calculation
 * supports administrative analysis of military transport capability separate
 * from civilian cargo capacity.
 *
 * Uses the same capacity scaling as flthold() where ship size determines
 * storage contribution: light warships provide 1 unit, medium warships
 * provide 2 units, and heavy warships provide 3 units of cargo space.
 *
 * This function enables military logistics planning by showing how much
 * cargo capacity comes specifically from combat vessels, useful for
 * scenarios where military and civilian transport needs are analyzed
 * separately.
 *
 * Parameters:
 *   nvynum - Fleet number identifier (0 to MAXNAVY-1)
 *
 * Returns:
 *   Total cargo storage capacity from warships only
 *   0 if fleet contains no warships
 *
 * Side Effects:
 *   - No modification of fleet or game state
 *   - Read-only access to current nation's warship data
 *
 * Algorithm:
 *   1. Initialize warship capacity to 0
 *   2. Iterate through ship size classes (N_LIGHT to N_HEAVY)
 *   3. For each size, calculate: (size+1) * warship_count
 *   4. Accumulate warship contributions only
 *   5. Return total warship cargo capacity
 *
 * Notes:
 *   - ADMIN compilation flag required - administrative tool only
 *   - Used for military logistics and strategic planning
 *   - Excludes merchant and galley cargo contributions
 *   - Useful for analyzing pure military transport capability
 *
 * Testing Notes:
 *   Category: A (Unit) - Isolated calculation with deterministic behavior
 *   Approach: Unit tests with warship-only fleet configurations
 *   Key Tests: Mixed fleet filtering, warship capacity scaling, empty warship handling
 *   Dependencies: P_NWAR macro, ship size constants
 *   Mock Requirements: Fleet setup with various warship types and sizes
 *   Complexity: Simple - Mathematical accumulation with size-based multipliers
 */
int 
fltwhold (int nvynum)
{
	int i,hold=0;

	for (i=N_LIGHT;i<=N_HEAVY;i++)
	{
		hold += (i+1) * (int)P_NWAR(i);
	}
	return(hold);
}
#endif /* ADMIN */

/*
 * fltghold - Calculate cargo storage capacity for galleys only
 *
 * Computes the cargo capacity contribution from galley ships exclusively
 * within a fleet, ignoring warships and merchants. This specialized calculation
 * supports analysis of classical oared vessel transport capability, which
 * serves dual military and civilian purposes.
 *
 * Uses the same capacity scaling as other ship types where size determines
 * storage contribution: light galleys provide 1 unit, medium galleys provide
 * 2 units, and heavy galleys provide 3 units of cargo space.
 *
 * Galleys represent classical Mediterranean-style oared vessels with unique
 * operational characteristics. This function enables analysis of their
 * specific cargo contribution, useful for strategic planning in coastal
 * and riverine operations where galleys excel.
 *
 * Parameters:
 *   nvynum - Fleet number identifier (0 to MAXNAVY-1)
 *
 * Returns:
 *   Total cargo storage capacity from galleys only
 *   0 if fleet contains no galleys
 *
 * Side Effects:
 *   - No modification of fleet or game state
 *   - Read-only access to current nation's galley data
 *
 * Algorithm:
 *   1. Initialize galley capacity to 0
 *   2. Iterate through ship size classes (N_LIGHT to N_HEAVY)
 *   3. For each size, calculate: (size+1) * galley_count
 *   4. Accumulate galley contributions only
 *   5. Return total galley cargo capacity
 *
 * Notes:
 *   - Available in all compilation modes (not ADMIN-restricted)
 *   - Useful for analyzing classical naval operations
 *   - Excludes warship and merchant cargo contributions
 *   - Galleys provide dual military/transport capability
 *
 * Testing Notes:
 *   Category: A (Unit) - Isolated calculation with deterministic behavior
 *   Approach: Unit tests with galley-only fleet configurations
 *   Key Tests: Mixed fleet filtering, galley capacity scaling, empty galley handling
 *   Dependencies: P_NGAL macro, ship size constants
 *   Mock Requirements: Fleet setup with various galley types and sizes
 *   Complexity: Simple - Mathematical accumulation with size-based multipliers
 */
int 
fltghold (int nvynum)
{
	int i,hold=0;

	for (i=N_LIGHT;i<=N_HEAVY;i++)
	{
		hold += (i+1) * (int)P_NGAL(i);
	}
	return(hold);
}

/*
 * fltmhold - Calculate cargo storage capacity for merchant ships only
 *
 * Computes the cargo capacity contribution from merchant ships exclusively
 * within a fleet, ignoring warships and galleys. This specialized calculation
 * supports analysis of civilian transport capability, which is essential
 * for economic operations and non-military cargo movement.
 *
 * Uses the same capacity scaling as other ship types where size determines
 * storage contribution: light merchants provide 1 unit, medium merchants
 * provide 2 units, and heavy merchants provide 3 units of cargo space.
 *
 * Merchant ships are optimized for cargo transport rather than combat,
 * making them the backbone of economic naval operations. This function
 * enables analysis of pure civilian transport capability, useful for
 * trade route planning and economic logistics.
 *
 * Parameters:
 *   nvynum - Fleet number identifier (0 to MAXNAVY-1)
 *
 * Returns:
 *   Total cargo storage capacity from merchant ships only
 *   0 if fleet contains no merchant ships
 *
 * Side Effects:
 *   - No modification of fleet or game state
 *   - Read-only access to current nation's merchant ship data
 *
 * Algorithm:
 *   1. Initialize merchant capacity to 0
 *   2. Iterate through ship size classes (N_LIGHT to N_HEAVY)
 *   3. For each size, calculate: (size+1) * merchant_count
 *   4. Accumulate merchant contributions only
 *   5. Return total merchant cargo capacity
 *
 * Notes:
 *   - Available in all compilation modes (not ADMIN-restricted)
 *   - Essential for economic and trade operations
 *   - Excludes warship and galley cargo contributions
 *   - Optimized for civilian cargo transport efficiency
 *
 * Testing Notes:
 *   Category: A (Unit) - Isolated calculation with deterministic behavior
 *   Approach: Unit tests with merchant-only fleet configurations
 *   Key Tests: Mixed fleet filtering, merchant capacity scaling, empty merchant handling
 *   Dependencies: P_NMER macro, ship size constants
 *   Mock Requirements: Fleet setup with various merchant types and sizes
 *   Complexity: Simple - Mathematical accumulation with size-based multipliers
 */
int 
fltmhold (int nvynum)
{
	int i,hold=0;

	for (i=N_LIGHT;i<=N_HEAVY;i++)
	{
		hold += (i+1) * (int)P_NMER(i);
	}
	return(hold);
}

#ifdef CONQUER
extern short country;
extern short selector, pager;
extern short xcurs, ycurs, xoffset, yoffset;
extern FILE *fexe;

/*
 * get_cargo - Interactive cargo type selection interface (static helper)
 *
 * Presents user with choice between army or people cargo types during
 * fleet loading/unloading operations. Provides standardized interface
 * for cargo type selection with clear prompting and input validation.
 *
 * Displays formatted prompt with the provided action string ("Load" or
 * "Unload") and waits for user input to determine cargo type preference.
 * Supports both uppercase and lowercase input for user convenience.
 *
 * Parameters:
 *   str - Action string to display ("Load", "Unload", etc.)
 *
 * Returns:
 *   TRUE (1) if user selects army cargo
 *   FALSE (0) if user selects people cargo
 *   -1 if invalid input or user cancellation
 *
 * Side Effects:
 *   - Updates screen display with prompt message
 *   - Clears end of line for clean presentation
 *   - Refreshes screen and waits for user input
 *   - No modification of game state
 *
 * Interface:
 *   - Prompts: "[action] (A)rmy or (P)eople?"
 *   - Accepts: 'A', 'a' for army selection
 *   - Accepts: 'P', 'p' for people selection
 *   - Any other input returns invalid (-1)
 *
 * Testing Notes:
 *   Category: B (Integration) - UI component requiring user input simulation
 *   Approach: Integration tests with mock input and display validation
 *   Key Tests: Input validation, case handling, display formatting
 *   Dependencies: ncurses library, getch() function
 *   Mock Requirements: UI system setup and input simulation
 *   Complexity: Simple - Straightforward input handling with validation
 */
static int 
get_cargo (char *str)
{
	int choice=(-1);

	mvprintw(LINES-3,0,"%s (A)rmy or (P)eople?",str);
	clrtoeol();
	refresh();
	switch(getch()) {
	case 'a':
	case 'A':
	     choice=TRUE;
	     break;
	case 'p':
	case 'P':
	     choice=FALSE;
	     break;
	default:
	     break;
	}

	return(choice);
}

/*
 * loadstat - Validate army status for fleet loading operations
 *
 * Determines whether an army with a given status can be loaded onto a
 * fleet by checking against prohibited status conditions. Prevents
 * loading of armies that are already committed to other operations
 * or in states incompatible with naval transport.
 *
 * The function implements status-based restrictions to maintain game
 * logic consistency and prevent conflicts with other army operations.
 * Only armies in compatible states can be loaded onto fleets.
 *
 * Prohibited Status Conditions:
 * - TRADED: Army involved in trade operations
 * - GENERAL: Army under general command (special status)
 * - MILITIA: Army serving as local militia
 * - GARRISON: Army stationed as garrison troops
 * - ONBOARD: Army already aboard another fleet
 *
 * Parameters:
 *   status - Army status code to validate for loading eligibility
 *
 * Returns:
 *   FALSE (0) if army status prohibits fleet loading
 *   TRUE (1) if army status allows fleet loading
 *
 * Side Effects:
 *   - No modification of game state
 *   - Read-only status validation
 *
 * Algorithm:
 *   1. Check status against prohibited conditions
 *   2. Return FALSE for any prohibited status
 *   3. Return TRUE for all other statuses (default allow)
 *
 * Usage Context:
 *   - Called during army loading validation in loadfleet()
 *   - Prevents invalid army state transitions
 *   - Maintains consistency with army status system
 *
 * Testing Notes:
 *   Category: A (Unit) - Simple status validation with deterministic behavior
 *   Approach: Unit tests with all army status constants
 *   Key Tests: Prohibited status detection, valid status acceptance
 *   Dependencies: Army status constants (TRADED, GENERAL, etc.)
 *   Mock Requirements: Army status constant definitions
 *   Complexity: Simple - Switch statement with boolean logic
 */
int 
loadstat (int status)
{
	switch(status) {
	case TRADED:
	case GENERAL:
	case MILITIA:
	case GARRISON:
	case ONBOARD:
		return(FALSE);
		break;
	default:
		break;
	}
	return(TRUE);
}

/*
 * loadfleet - Complex interactive fleet loading and unloading interface
 *
 * Comprehensive fleet cargo management system that handles loading and unloading
 * of both armies and civilian populations to/from naval fleets. Provides
 * interactive user interface with extensive validation for complex cargo
 * operations including capacity checking, ownership verification, and
 * movement cost calculations.
 *
 * The function supports dual cargo types: military units (armies) loaded onto
 * galley cargo space, and civilian populations loaded onto merchant cargo
 * space. Each operation includes comprehensive safety checks, ownership
 * verification, and movement cost adjustments.
 *
 * Key Operations:
 * - Load armies onto galleys (military transport)
 * - Load people onto merchants (civilian transport)
 * - Unload armies from galleys with location validation
 * - Unload people from merchants with ownership checks
 * - Interactive cargo type selection interface
 * - Real-time capacity and status display
 *
 * Compilation Dependencies:
 *   - Requires CONQUER compilation flag
 *   - Uses global variables for UI and game state
 *   - Depends on sector ownership and designation systems
 *
 * Parameters:
 *   None - Uses global game state and user input
 *
 * Returns:
 *   void - Operation status communicated through error messages
 *
 * Side Effects:
 *   - Modifies fleet cargo (P_NPEOP, P_NARMY)
 *   - Updates army status and location
 *   - Modifies sector population counts
 *   - Adjusts movement points for fleet and armies
 *   - Updates display through ncurses interface
 *   - Validates ownership and diplomatic status
 *
 * Error Conditions:
 *   - Invalid fleet selection or fleet not found
 *   - Fleet not landed (must be on land to load/unload)
 *   - No storage space available
 *   - Invalid army selection or army not in sector
 *   - Army too large for available fleet capacity
 *   - Ownership restrictions for people loading/unloading
 *   - Diplomatic restrictions for army disembarkation
 *
 * Interactive Interface:
 *   - Real-time cargo status display
 *   - Load/Unload operation selection
 *   - Army/People cargo type selection
 *   - Numeric input for quantities and army numbers
 *   - Error message display with user feedback
 *
 * Capacity Management:
 *   - Galley space for armies (calculated by fltghold())
 *   - Merchant space for people (calculated by fltmhold())
 *   - Special handling for leaders and monsters
 *   - SHIPHOLD constant defines per-ship capacity ratios
 *
 * Movement Cost System:
 *   - N_CITYCOST deducted for operations in cities/capitols
 *   - Full movement point loss for non-city operations
 *   - Separate cost tracking for fleets and armies
 *
 * Ownership and Diplomatic Validation:
 *   - Sailors/marines can disembark in unowned sectors
 *   - Only marines can disembark in enemy territory
 *   - People refuse to board in sectors not owned by their nation
 *   - Neutral/allied city access for reduced movement costs
 *
 * Testing Notes:
 *   Category: C (System) - Complex interactive system with extensive dependencies
 *   Approach: System testing with full game state initialization
 *   Key Tests: Fleet validation, capacity calculations, ownership checks, UI flow
 *   Dependencies: Global game state, sector system, army system, UI system
 *   Mock Requirements: Complete game state with fleets, armies, sectors, nations
 *   Complexity: Complex - Extensive branching logic with multiple subsystem interactions
 */
void 
loadfleet (void)
{
	short nvynum;
	/* merchant holding unused */
	int ghold, mhold, unload, doarmy;
	int gcargo, mcargo, amount, armynum;

	clear_bottom(0);
	if((nvynum=safe_int_to_short(getselunit()-MAXARM))>=0){
		if(nvynum>=MAXNAVY){
			errormsg("Invalid Navy");
			return;
		}
	} else {
		errormsg("Invalid Navy");
		return;
	}

	if(sct[XREAL][YREAL].altitude==WATER) {
		errormsg("Fleet must be landed");
		return;
	}

	/* process loading or unloading */
	ghold = fltghold(nvynum);
	mhold = fltmhold(nvynum);
	if((ghold==0)&&(mhold == 0)) {
		errormsg("No storage space available");
		return;
	} else if((P_NARMY==MAXARM)&&(P_NPEOP==0)) {
		unload=FALSE;
	} else {
		mvprintw(LINES-4,0,"Cargo:   People %d",P_NPEOP*mhold);
		if(P_NARMY==MAXARM) mvaddstr(LINES-4,25,"Army (none)");
		else mvprintw(LINES-4,25,"Army (%d)",P_NARMY);
		mvaddstr(LINES-3,0,"Do you wish to (L)oad or (U)nload?");
		refresh();
		switch(getch()) {
		case 'l':
		case 'L':
		     unload=FALSE;
		     break;
		case 'u':
		case 'U':
		     unload=TRUE;
		     break;
		default:
		     return;
		}
	}

	if(unload==TRUE) {
		if(P_NARMY==MAXARM) doarmy=FALSE;
		else if(P_NPEOP==0) doarmy=TRUE;
		else {
			doarmy=get_cargo("Unload");
		}
		if(doarmy==TRUE) {
			armynum=P_NARMY;
			if(sct[XREAL][YREAL].owner==0
			&& P_ATYPE!=A_MARINES
			&& P_ATYPE!=A_SAILOR) {
				errormsg("Only sailors or marines may disembark in unowned land");
				return;
			} else if (sct[XREAL][YREAL].owner!=country
			&& sct[XREAL][YREAL].owner!=0
			&& P_ATYPE!=A_MARINES) {
				errormsg("Only marines may disembark in someone else's land");
				return;
			}
			P_ASTAT=DEFEND;
			P_NARMY=MAXARM;
			if (!((sct[XREAL][YREAL].designation==DCITY
			|| sct[XREAL][YREAL].designation==DCAPITOL)
			&& (sct[XREAL][YREAL].owner==country
			|| (!ntn[sct[XREAL][YREAL].owner].dstatus[country]!=UNMET
			&& ntn[sct[XREAL][YREAL].owner].dstatus[country]<=NEUTRAL)))
			|| P_NMOVE < N_CITYCOST) {
				P_NMOVE=0;
			} else {
				P_NMOVE-= N_CITYCOST;
			}
			NADJMOV;
			NADJHLD;
			AADJSTAT;
		} else if(doarmy==FALSE){
			if (sct[XREAL][YREAL].owner!=country) {
				mvaddstr(LINES-3,0,"Unload in a sector you don't own? (y or n)");
				clrtoeol();
				refresh();
				if (getch()!='y') {
					return;
				}
			}
			mvaddstr(LINES-2,0,"Unload how many people?");
			refresh();
			amount=safe_long_to_int(get_number());
			if(amount > mhold*P_NPEOP) {
				errormsg("There are not that many on board");
			} else if (amount > 0) {
				sct[XREAL][YREAL].people += amount;
				P_NPEOP=(unsigned char)((mhold*P_NPEOP-amount)/mhold);
				NADJHLD;
				if ((sct[XREAL][YREAL].designation!=DCITY
				&& sct[XREAL][YREAL].designation!=DCAPITOL)
				|| P_NMOVE < N_CITYCOST) {
					P_NMOVE=0;
				} else {
					P_NMOVE-= N_CITYCOST;
				}
				NADJMOV;
				SADJCIV;
			}
		}
	} else {
		clear_bottom(0);
		mcargo = safe_long_to_int(mhold*(SHIPHOLD-P_NPEOP));
		if(P_NARMY==MAXARM) {
			gcargo = ghold*SHIPHOLD;
			mvprintw(LINES-4,0,"Available Space:  %d soldiers  %d people", gcargo, mcargo);
		} else {
			gcargo = 0;
			mvprintw(LINES-4,0,"Available Space:  0 soldiers  %d people", mcargo);
		}
		if(gcargo==0) doarmy=FALSE;
		else if(mcargo==0) doarmy=TRUE;
		else {
			doarmy=get_cargo("Load");
		}
		if(doarmy==TRUE) {
			mvaddstr(LINES-2,0,"Load what army?");
			refresh();
			armynum = safe_long_to_int(get_number());
			if(armynum<0) {
				;
			} else if((armynum>=MAXARM)||(P_ASOLD<=0)
			||(loadstat(P_ASTAT)==FALSE)) {
				errormsg("Invalid Army");
			} else if((P_AXLOC!=XREAL)||(P_AYLOC!=YREAL)) {
				errormsg("Army not in sector");
			} else if(P_ASOLD > gcargo &&
			(P_ATYPE<MINLEADER || P_ATYPE>=MINMONSTER)) {
				errormsg("Army too large for fleet");
			} else {
				P_ASTAT=ONBOARD;
				P_AMOVE=0;
				P_NARMY=safe_int_to_uchar(armynum);
				if (!((sct[XREAL][YREAL].designation==DCITY
				|| sct[XREAL][YREAL].designation==DCAPITOL)
				&& (sct[XREAL][YREAL].owner==country
				|| (!ntn[sct[XREAL][YREAL].owner].dstatus[country]!=UNMET
				&& ntn[sct[XREAL][YREAL].owner].dstatus[country]<=NEUTRAL)))
				|| P_NMOVE < N_CITYCOST) {
					P_NMOVE=0;
				} else {
					P_NMOVE-= N_CITYCOST;
				}
				NADJMOV;
				NADJHLD;
				AADJMOV;
				AADJSTAT;
			}
		} else if(doarmy==FALSE && mcargo!=0){
			mvaddstr(LINES-2,0,"Load how many people?");
			refresh();
			amount=safe_long_to_int(get_number());
			if(sct[XREAL][YREAL].owner!=country) {
				errormsg("The people refuse to board");
			} else if(amount > mcargo) {
				errormsg("Not enough room on fleet");
			} else if(sct[XREAL][YREAL].people < amount) {
				errormsg("Not enough people in sector");
			} else if (amount > 0) {
				sct[XREAL][YREAL].people -= amount;
				P_NPEOP += (unsigned char)(amount / mhold);
				SADJCIV;
				if ((sct[XREAL][YREAL].designation!=DCITY
				&& sct[XREAL][YREAL].designation!=DCAPITOL)
				|| P_NMOVE < N_CITYCOST) {
					P_NMOVE=0;
				} else {
					P_NMOVE-= N_CITYCOST;
				}
				NADJMOV;
				NADJHLD;
			}
		} else if (mcargo==0) {
			errormsg("No more room onboard fleet");
		}
	}
}
#endif /* CONQUER */
