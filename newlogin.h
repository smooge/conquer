/*
 * newlogin.h - Nation Creation Configuration System
 *
 * This header file contains the complete configuration system for nation creation
 * in the newlogin system. It defines the point allocation framework, racial
 * characteristics, cost structures, and gameplay balance parameters that govern
 * how new nations are created and configured.
 *
 * The configuration system implements a point-based nation creation model where
 * players allocate points across various attributes (population, military, magic,
 * etc.) with costs and constraints determined by the tables and constants defined
 * in this file.
 *
 * Configuration Components:
 * - Allocation Categories: Defines the CH_* categories for nation attributes
 * - Racial Defaults: Race-specific starting values for balanced gameplay
 * - Cost/Value Systems: Point costs, unit conversions, and scaling factors
 * - Legacy Compatibility: Support for both new and legacy cost calculation methods
 * - Placement Quality: Location-based bonuses and penalties
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
 * =============================================================================
 * NATION ATTRIBUTE ALLOCATION CATEGORIES
 * =============================================================================
 *
 * These constants define the indices for the point allocation system used in
 * nation creation. Each category represents a different aspect of nation
 * development that players can invest points into during character creation.
 *
 * The allocation system is used by:
 * - newlogin() interactive point allocation interface
 * - nstartcst() modern cost calculation method
 * - Mcost[], Munits[], Mvalues[] configuration arrays
 * - spent[] array for tracking current allocations
 */
#define CH_PEOPLE	0	/* Civilian population - affects food production and economy */
#define CH_TREASURY	1	/* Starting gold reserves - immediate purchasing power */
#define CH_LOCATE	2	/* Placement quality - affects starting location and territories */
#define CH_SOLDIERS	3	/* Military units - standing army size and strength */
#define CH_ATTACK	4	/* Attack bonus - offensive combat effectiveness */
#define CH_DEFEND	5	/* Defense bonus - defensive combat effectiveness */
#define CH_REPRO	6	/* Reproduction rate - population growth and recovery */
#define CH_MOVEMENT	7	/* Movement range - army mobility and strategic reach */
#define CH_MAGIC	8	/* Magical power - spellcasting abilities and mana */
#define CH_LEADERS	9	/* Leadership capacity - army command and organization */
#define CH_RAWGOODS	10	/* Raw materials - jewels and metal starting resources */

/* Total number of allocation categories in the system */
#define CH_NUMBER	11

/*
 * =============================================================================
 * PLACEMENT QUALITY DEFINITIONS
 * =============================================================================
 *
 * These constants define the quality levels for nation placement on the game map.
 * Higher quality placements provide better starting territories but cost more points.
 * Used by the placement system in place() function and cost calculations.
 */
#define NLRANDOM	0	/* Random placement - basic location, no point cost */
#define NLFAIR		1	/* Fair placement - decent location, moderate point cost */
#define NLGOOD		2	/* Good placement - premium location, higher point cost */

/*
 * =============================================================================
 * ALLOCATION DIRECTION CONSTANTS
 * =============================================================================
 *
 * These constants define the direction of point allocation changes in the
 * interactive nation creation interface. Used by the allocation adjustment
 * system to determine whether points are being added or subtracted.
 */
#define ADDITION	0	/* Adding points to an allocation category */
#define SUBTRACTION	1	/* Removing points from an allocation category */

/*
 * =============================================================================
 * RACIAL DEFAULT STARTING VALUES
 * =============================================================================
 *
 * These constants define the racial characteristics and starting bonuses for each
 * playable race. Each race has different strengths and weaknesses reflected in
 * their base attribute values, creating strategic diversity in gameplay.
 *
 * The values represent the default/bonus amounts granted to each race in specific
 * categories, implementing the racial balance system that makes each race unique.
 * Used during nation initialization to apply race-specific bonuses.
 */

/*
 * DWARF NATION RACIAL CHARACTERISTICS
 * Dwarves are renowned for their defensive capabilities and balanced military approach.
 * Strong in both attack and defense with moderate mobility and population.
 */
#define NLDGOLD		1	/* Starting gold reserves */
#define NLDRAW		1	/* Raw materials bonus */
#define NLDCIVIL	6	/* Civilian population base */
#define NLDMILIT	2	/* Military unit allocation */
#define NLDREPRO	4	/* Reproduction rate */
#define NLDMMOVE	6	/* Movement range capability */
#define NLDAPLUS	20	/* Attack bonus - strong offensive capability */
#define NLDDPLUS	20	/* Defense bonus - excellent defensive capability */

/*
 * ELF NATION RACIAL CHARACTERISTICS
 * Elves excel in mobility and defense with strong population but weaker military numbers.
 * Emphasizes quality over quantity with superior movement and defensive abilities.
 */
#define NLEGOLD		1	/* Starting gold reserves */
#define NLERAW		1	/* Raw materials bonus */
#define NLECIVIL	7	/* Civilian population base - highest among races */
#define NLEMILIT	1	/* Military unit allocation - smallest standing army */
#define NLEREPRO	5	/* Reproduction rate */
#define NLEMMOVE	8	/* Movement range capability - excellent mobility */
#define NLEAPLUS	10	/* Attack bonus - moderate offensive capability */
#define NLEDPLUS	40	/* Defense bonus - superior defensive capability */

/*
 * ORC NATION RACIAL CHARACTERISTICS
 * Orcs prioritize population growth and reproduction with no economic or combat bonuses.
 * Strategy focuses on overwhelming numbers and rapid expansion rather than individual strength.
 */
#define NLOGOLD		0	/* Starting gold reserves - no economic bonus */
#define NLORAW		1	/* Raw materials bonus */
#define NLOCIVIL	8	/* Civilian population base - largest among races */
#define NLOMILIT	2	/* Military unit allocation */
#define NLOREPRO	8	/* Reproduction rate - highest among races */
#define NLOMMOVE	6	/* Movement range capability */
#define NLOAPLUS	0	/* Attack bonus - no combat bonuses */
#define NLODPLUS	0	/* Defense bonus - no combat bonuses */

/*
 * HUMAN NATION RACIAL CHARACTERISTICS
 * Humans balance military prowess with superior mobility, representing versatile civilization.
 * Strong attack capabilities with excellent movement range but moderate defense.
 */
#define NLHGOLD		0	/* Starting gold reserves - no economic bonus */
#define NLHRAW		1	/* Raw materials bonus */
#define NLHCIVIL	6	/* Civilian population base */
#define NLHMILIT	2	/* Military unit allocation */
#define NLHREPRO	5	/* Reproduction rate */
#define NLHMMOVE	10	/* Movement range capability - best among races */
#define NLHAPLUS	20	/* Attack bonus - strong offensive capability */
#define NLHDPLUS	10	/* Defense bonus - moderate defensive capability */

/*
 * =============================================================================
 * MODERN NATION CREATION CONFIGURATION SYSTEM
 * =============================================================================
 *
 * The following configuration arrays implement the modern point allocation system
 * used by nstartcst() and the interactive newlogin() interface. Each array has
 * CH_NUMBER elements corresponding to the allocation categories defined above.
 *
 * Note: These arrays are placed in the header for configuration clarity since
 * newlogin.c is the only file to include this header. This design choice aids
 * in game balance tuning and configuration management.
 */

/*
 * ALLOCATION CONSTRAINTS SYSTEM
 * Defines the valid range of point allocation for each category to maintain
 * game balance and prevent degenerate nation configurations.
 */
/* Minimum allocation values for each category (prevents zero allocations in critical areas) */
static int Minvalues[]= { 6, 0, 0, 1, 0, 0, 4, 6, 0, 4, 1 };
/*                 P  T  L  S  A  D  R  M  G  L  W */
/*                 E  R  O  O  T  E  E  O  I  E  R */
/*                 O  E  C  L  T  F  P  V  C  A  A */
/*                 P  A  A  D  A  E  R  E  K  D  W */
/*                 L  S  T  I  C  N  O     E  E     */
/*                 E  U  E  E  K  S     G  R  R     */
/*                    R     R     E     S  S  S     */
/*                    Y                             */

/* Maximum allocation values for each category (prevents overpowered configurations) */
static int Maxvalues[]= { 50, 10, 2, 20, 80, 80, 10, 30, 5, MAXARM/2, 10 };

/*
 * POINT COST SYSTEM
 * Determines how expensive each type of allocation is in the point economy.
 * Higher costs make attributes more expensive and limit over-specialization.
 */
/* Point cost per unit for each allocation category */
static int Mcost[]= { 1, 1, 1, 1, 1, 1, 3, 1, 3, 1, 1 };
/*             P  T  L  S  A  D  R  M  G  L  W */
/*             Note: Reproduction (R) and Magic (G) cost 3x due to their power */

/*
 * UNIT SCALING SYSTEM
 * Defines how many game units are gained per point spent in each category.
 * Higher values mean more units per point (better efficiency).
 */
/* Number of game units gained per point spent */
static int Munits[]= { 1, 1, 1, 1, 8, 8, 1, 2, 1, 3, 1 };
/*              P  T  L  S  A  D  R  M  G  L  W */
/*              Note: Attack/Defense get 8x multiplier, Movement 2x, Leaders 3x */

/*
 * VALUE CONVERSION SYSTEM
 * Converts allocation points into actual game values (population numbers, gold amounts, etc.).
 * These values determine the real-world impact of point allocations.
 */
/* Game value per unit for each allocation category */
static long Mvalues[]= { 1000L, 100000L, 1L, 1000L,  1L,  1L,  1L,  1L,  1L,  1L, 30000L };
/*                      P=1000  T=100k  L=1  S=1000  A=1  D=1  R=1  M=1  G=1  L=1  W=30k */
/*                      people  gold    loc  troops  att  def  rep  mov  mag  lea  jewels/metal */

/*
 * =============================================================================
 * RESOURCE VALUE CONSTANTS
 * =============================================================================
 *
 * These constants define the standard values for precious resources that can be
 * allocated during nation creation. Used in conjunction with the raw goods
 * allocation category to provide starting wealth beyond gold reserves.
 */
/* Starting values for precious materials when allocated */
#define NLJEWELS	15000L	/* Jewels value per raw goods allocation unit */
#define NLMETAL		15000L	/* Metal value per raw goods allocation unit */

/*
 * =============================================================================
 * LEGACY COST CALCULATION SYSTEM
 * =============================================================================
 *
 * These constants implement the original "old method" cost calculation system
 * used by startcost() for backward compatibility and validation. These values
 * define the conversion rates between nation attributes and point costs in the
 * legacy system.
 *
 * The legacy system calculates costs directly from final nation attributes
 * rather than using the modern allocation arrays, making it suitable for
 * cost validation and NPC nation evaluation.
 */

/* Population and Economic Conversion Constants */
#define	ONLPOP		1000L	/* Civilian population per point */
#define	ONLGOLD		100000L	/* Gold pieces per point */
#define	ONLSOLD		900L	/* Military units per point */

/* Placement Quality Costs */
#define	ONLLOCCOST	1	/* Point cost for improved placement quality */

/* Combat Attribute Conversion Constants */
#define	ONLATTACK	10	/* Attack bonus points per point cost */
#define	ONLDEFENCE	10	/* Defense bonus points per point cost */

/* Reproduction System Constants (Race-Specific) */
#define	ONLREPRO_ORC	2	/* Reproduction points per ONLREPCOST for Orc nations */
#define	ONLREPRO	1	/* Reproduction points per ONLREPCOST for other races */
#define	ONLREPCOST	3	/* Point cost for reproduction improvements */

/* Movement and Special Abilities */
#define	ONLMOVE		2	/* Movement points per point cost */
#define	ONLMAGIC	3	/* Point cost for magical abilities */

/* Leadership System Constants */
#define	ONLEADER	3	/* Number of leaders gained per point investment */
#define	ONLEADPT	1	/* Point cost for leadership capacity */

/* Miscellaneous Legacy Constants */
#define	ONLDBLCOST	1	/* Double cost modifier for certain calculations */
