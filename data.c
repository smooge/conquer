/*
 * data.c - Global Game Data Definitions and Configuration Tables
 *
 * This file serves as the central repository for all game configuration data,
 * unit statistics, terrain properties, and lookup tables used throughout the
 * Conquer game system. Unlike other source files, data.c contains no functions -
 * it is purely a data definition file that establishes the game's core parameters
 * and provides lookup tables for game mechanics.
 *
 * ARCHITECTURAL ROLE:
 * - Central data repository for all game systems
 * - Configuration parameters for terrain, movement, and combat
 * - Unit type definitions with complete statistical profiles
 * - Economic and trade system lookup tables
 * - String literals for user interface and game display
 * - File system and external program configuration
 *
 * DATA ORGANIZATION:
 * 1. Terrain and Movement Configuration (lines 29-47)
 * 2. Display and Interface String Arrays (lines 49-82)
 * 3. Unit Type Statistics and Combat Data (lines 99-192)
 * 4. File System and External Program Paths (lines 194-209)
 * 5. Game Configuration and Powers System (lines 211-257)
 * 6. Trade and Economic System Data (lines 274-295)
 *
 * MODERNIZATION CONSIDERATIONS:
 * - All arrays use legacy C89 static initialization
 * - String literals lack const qualifiers (should be const char *)
 * - Hardcoded array sizes without symbolic constants
 * - Magic numbers embedded in data arrays need documentation
 * - Some arrays have alignment/formatting issues for readability
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
#include <curses.h>
#include "header.h"
#include "data.h"

/*
 * ============================================================================
 * SECTION 1: TERRAIN AND MOVEMENT CONFIGURATION
 * ============================================================================
 * This section defines the core terrain system, movement costs, and display
 * characters used throughout the game world. The system uses character-based
 * lookup tables for efficient terrain processing and movement calculation.
 */

/*
 * ele - Terrain elevation display characters
 * Maps terrain elevation types to single-character display symbols:
 * '~' = WATER (0), '#' = PEAK (1), '^' = MOUNTAIN (2),
 * '%' = HILL (3), '-' = FLAT (4), '0' = ERROR (5)
 * Used by display routines for map rendering and user interface.
 *
 * MUST be unsigned char for safe array indexing (Phase 10.1).
 * On x86_64, char is signed (-128 to 127). Using plain char for array
 * indices can produce negative indices when values exceed 127, causing
 * undefined behavior. Changed from 'char *' to 'unsigned char *' to
 * ensure safe array indexing on all platforms.
 *
 * @last_documented: 2025-10-09
 */
unsigned char *ele = (unsigned char *)"~#^%-0";

/*
 * Elevation Movement Cost Tables by Race
 * Each string maps to terrain types in 'ele' array (WATER,PEAK,MOUNTAIN,HILL,FLAT,ERROR)
 * Values 0-9 represent movement cost multipliers, '/' indicates impassable terrain
 * Higher numbers = more movement points required to traverse
 */
char *HElecost = "//521/"; /* Human movement costs */
char *OElecost = "//222/"; /* Orc movement costs */
char *EElecost = "//631/"; /* Elf movement costs */
char *DElecost = "//311/"; /* Dwarf movement costs */
char *FElecost = "16211/"; /* Flight movement costs - can cross water */

/*
 * Vegetation System Configuration
 * Defines vegetation types, movement costs by race, and food production values
 */

/*
 * veg - Vegetation type identifiers
 * Character codes for different vegetation/climate types:
 * 'v'=VOLCANO, 'd'=DESERT, 't'=TUNDRA, 'b'=BARREN, 'l'=LIGHT_VEG,
 * 'g'=GOOD, 'w'=WOOD, 'f'=FOREST, 'j'=JUNGLE, 's'=SWAMP,
 * 'i'=ICE, '~'=NONE, '0'=ERROR
 *
 * MUST be unsigned char for safe array indexing (Phase 10.1).
 * On x86_64, char is signed (-128 to 127). Using plain char for array
 * indices can produce negative indices when values exceed 127, causing
 * undefined behavior. Changed from 'char *' to 'unsigned char *' to
 * ensure safe array indexing on all platforms.
 *
 * @last_documented: 2025-10-09
 */
unsigned char *veg = (unsigned char *)"vdtblgwfjsi~0";

/*
 * Vegetation Movement Cost Tables by Race
 * Each position corresponds to vegetation types in 'veg' array
 * Values 0-9 = movement cost, '/' = impassable terrain
 */
char *HVegcost = "63210001332//"; /* Human vegetation movement costs */
char *OVegcost = "43100022527//"; /* Orc vegetation movement costs */
char *EVegcost = "86221000027//"; /* Elf vegetation movement costs */
char *DVegcost = "47100013577//"; /* Dwarf vegetation movement costs */
char *FVegcost = "410000001000/"; /* Flight vegetation movement costs */

/*
 * vegfood - Food production by vegetation type
 * Maps vegetation types to food production values (0-9)
 * Higher values indicate better agricultural potential
 * Used by economic system for population support calculations
 */
char *vegfood = "0004697400000";

/*
 * des - Designation type identifiers
 * Character codes for sector improvements and structures:
 * 't'=TOWN, 'c'=CITY, 'm'=MINE, 'f'=FARM, 'x'=DEVASTATED, '$'=GOLDMINE,
 * '!'=FORT, '&'=RUIN, 's'=STOCKADE, 'C'=CAPITOL, '?'=SPECIAL, 'l'=LUMBERYARD,
 * 'b'=BLACKSMITH, '+'=ROAD, '*'=MILL, 'g'=GRANARY, '='=CHURCH, 'u'=UNIVERSITY,
 * '-'=NODESIG, 'P'=BASE_CAMP, '0'=ERROR
 */
char *des = "tcmfx$!&sC?lb+*g=u-P0";

/*
 * ============================================================================
 * SECTION 2: DISPLAY AND INTERFACE STRING ARRAYS
 * ============================================================================
 * This section contains human-readable string arrays for game display,
 * user interface elements, and lookup tables. These arrays provide the
 * textual representation of game concepts for player interaction.
 */

#ifdef CONQUER
/*
 * elename - Elevation terrain type names
 * Full descriptive names corresponding to 'ele' character codes
 * Used for detailed terrain display and game reports
 * Index matches terrain type: 0=WATER, 1=PEAK, 2=MOUNTAIN, 3=HILL, 4=FLAT, 5=ERROR
 */
char *elename[] = {"WATER", "PEAK", "MOUNTAIN", "HILL", "FLAT", "ERROR"};

/*
 * vegname - Vegetation type descriptive names
 * Full names corresponding to 'veg' character codes
 * Used for climate/vegetation display in game interface
 * Index: 0=VOLCANO, 1=DESERT, 2=TUNDRA, 3=BARREN, 4=LT_VEG, 5=GOOD,
 *        6=WOOD, 7=FOREST, 8=JUNGLE, 9=SWAMP, 10=ICE, 11=NONE
 */
char *vegname[] = {"VOLCANO", "DESERT", "TUNDRA", "BARREN", "LT VEG", "GOOD",
                   "WOOD",    "FOREST", "JUNGLE", "SWAMP",  "ICE",    "NONE"};

/*
 * desname - Sector designation descriptive names
 * Full names corresponding to 'des' character codes
 * Used for displaying sector improvements and structures to players
 * Index: 0=TOWN, 1=CITY, 2=MINE, 3=FARM, 4=DEVASTATED, 5=GOLDMINE,
 *        6=FORT, 7=RUIN, 8=STOCKADE, 9=CAPITOL, 10=SPECIAL, 11=LUMBERYARD,
 *        12=BLACKSMITH, 13=ROAD, 14=MILL, 15=GRANARY, 16=CHURCH,
 *        17=UNIVERSITY, 18=NODESIG, 19=BASE_CAMP, 20=ERROR
 */
char *desname[] = {"TOWN",     "CITY",      "MINE",     "FARM",    "DEVASTATED", "GOLDMINE",
                   "FORT",     "RUIN",      "STOCKADE", "CAPITOL", "SPECIAL",    "LUMBERYD",
                   "BLKSMITH", "ROAD",      "MILL",     "GRANARY", "CHURCH",     "UNIVERSITY",
                   "NODESIG",  "BASE CAMP", "ERROR"};

/*
 * races - Player race/nation type names
 * Defines the different races/factions available in the game
 * Index: 0=GOD, 1=ORC, 2=ELF, 3=DWARF, 4=LIZARD, 5=HUMAN,
 *        6=PIRATE, 7=SAVAGE, 8=NOMAD, 9=UNKNOWN
 * Used for diplomatic displays and player identification
 */
char *races[] = {"GOD",   "ORC",    "ELF",    "DWARF", "LIZARD",
                 "HUMAN", "PIRATE", "SAVAGE", "NOMAD", "UNKNOWN"};

/*
 * diploname - Diplomatic relationship status names
 * Defines the diplomatic states between nations
 * Index: 0=UNMET, 1=TREATY, 2=ALLIED, 3=FRIENDLY, 4=NEUTRAL,
 *        5=HOSTILE, 6=WAR, 7=JIHAD
 * Used by diplomatic system and player interface
 */
char *diploname[] = {"UNMET",   "TREATY",  "ALLIED", "FRIENDLY",
                     "NEUTRAL", "HOSTILE", "WAR",    "JIHAD"};

/*
 * soldname - Military unit status names
 * Defines various military unit states and orders
 * Index: 0=?, 1=MARCH, 2=SCOUT, 3=GARRISON, 4=TRADED, 5=MILITIA,
 *        6=FLYING, 7=DEFEND, 8=MAG_DEF, 9=ATTACK, 10=MAG_ATT, 11=GENERAL,
 *        12=SORTIE, 13=SIEGE, 14=BESIEGED, 15=ON_BOARD, 16=RULE
 * Used for displaying unit orders and status to players
 */
char *soldname[] = {"?",      "MARCH",  "SCOUT",    "GARRISON", "TRADED",  "MILITIA",
                    "FLYING", "DEFEND", "MAG_DEF",  "ATTACK",   "MAG_ATT", "GENERAL",
                    "SORTIE", "SIEGE",  "BESIEGED", "ON_BOARD", "RULE"};
#endif /* CONQUER */

/*
 * directions - Directional movement names
 * Standard 8-direction compass with center position
 * Index: 0=here, 1=north, 2=northeast, 3=east, 4=southeast,
 *        5=south, 6=southwest, 7=west, 8=northwest
 * Used for movement commands and directional references
 */
char *directions[] = {"here",  "north",     "northeast", "east",     "southeast",
                      "south", "southwest", "west",      "northwest"};

/*
 * Class - Leader character class names
 * Defines the different types of special characters and leaders
 * Index: 0=monster, 1=king, 2=emperor, 3=wizard, 4=priest, 5=pirate,
 *        6=trader, 7=warlord, 8=demon, 9=dragon, 10=shadow
 * Used for character generation and display
 */
char *Class[] = {"monster", "king",    "emperor", "wizard", "priest", "pirate",
                 "trader",  "warlord", "demon",   "dragon", "shadow"};

/*
 * alignment - Character alignment system
 * Defines moral alignment categories for characters
 * Index: 0=Other, 1=Good, 2=Neutral, 3=Evil, 4=Other
 * Used for character behavior and interaction mechanics
 */
char *alignment[] = {"Other", "Good", "Neutral", "Evil", "Other"};

/*
 * ============================================================================
 * SECTION 3: UNIT TYPE DEFINITIONS AND COMBAT STATISTICS
 * ============================================================================
 * This section defines all military unit types, their combat capabilities,
 * costs, and movement characteristics. The game features 55 different unit
 * types ranging from basic militia to powerful magical creatures.
 *
 * UNIT ORGANIZATION:
 * - Basic Military Units (0-25): Standard troops, specialists, and scouts
 * - Leaders and Nobles (26-37): Command units with special abilities
 * - Monsters and Magical (38-54): Powerful magical creatures and demons
 *
 * Each unit type has corresponding entries in multiple parallel arrays:
 * - unittype[]: Display names
 * - shunittype[]: Short names for UI display
 * - unitminsth[]: Minimum strength values
 * - unitattack[]: Attack bonuses/penalties
 * - unitdefend[]: Defense bonuses/penalties
 * - unitmove[]: Movement rates (multiplied by 10)
 * - u_enmetal[]: Metal costs for construction
 * - u_encost[]: Gold costs for enlistment
 * - unitmaint[]: Maintenance costs per turn
 */

/*
 * unittype - Full descriptive names for all unit types
 * Index 0-54: Complete roster of military units, leaders, and monsters
 * Used for detailed unit displays and game reports
 *
 * UNIT CATEGORIES:
 * 0-25: Basic Military Units (Militia through Scout)
 * 26-37: Leaders and Nobles (King through Lord)
 * 38-49: Demons and Magical Creatures (Demon through Nazgul)
 * 50-54: Summoned and Elemental Creatures (Spirit through Dragon)
 */
char *unittype[] = {
    /* Basic Military Units (0-25) */
    "Militia", "Goblins", "Orcs", "Infantry", "Sailors", "Marines", "Archers", "Uruk-Hai",
    "Ninjas", "Phalanx", "Olog-Hai", "Legionaries", "Dragoons", "Mercenaries", "Trolls",
    "Elite", "Lt_Cavalry", "Hv_Cavalry", "Catapults", "Siege", "Rocs", "Knights", "Gryfins",
    "Elephants", "Zombies", "Spy", "Scout",

    /* Leaders and Nobles (26-37) */
    "King", "Baron", "Emperor", "Prince", "Wizard", "Mage", "Pope", "Bishop", "Admiral",
    "Captain", "Warlord", "Lord",

    /* Demons and Magical Creatures (38-49) */
    "Demon", "Devil", "Dragyn", "Wyrm", "Shadow", "Nazgul",

    /* Summoned and Elemental Creatures (50-54) */
    "Spirit", "Assasin", "Efreet", "Gargoyl", "Wraith", "Hero", "Centaur", "Giant", "Suphero",
    "Mummy", "Elmentl", "Mintaur", "Daemon", "Balrog", "Dragon"};

#ifdef CONQUER
/*
 * shunittype - Short unit type names for UI display
 * Abbreviated 4-character names corresponding to unittype[] array
 * Used for compact display in upper right corner and limited space areas
 * Parallel array to unittype[] with same indexing
 */
char *shunittype[] = {
    /* Basic Military Units (0-25) */
    "mlta", "Gob", "Orc", "Inf", "Sail", "XMar", "Arch", "Uruk", "Ninj", "Phax", "olog", "Legn",
    "Drag", "Merc", "Trol", "Elt", "lCav", "hCav", "cat", "sge", "Roc", "Kni", "grif", "ele",
    "zom", "Spy", "Scout",

    /* Leaders and Nobles (26-37) */
    "King", "Bar", "Emp", "Prin", "Wizd", "Magi", "Apos", "Bish", "Admi", "Capt", "Warl",
    "Lord",

    /* Demons and Magical Creatures (38-49) */
    "Demn", "Devl", "Drag", "Wyrm", "Shad", "Nazg",

    /* Summoned and Elemental Creatures (50-54) */
    "spir", "Assn", "efr", "Garg", "Wra", "Hero", "Cent", "gt", "Shro", "Mumm", "Elem", "mino",
    "daem", "Bal", "Drgn"};
#endif /* CONQUER */

/*
 * unitminsth - Minimum strength values for each unit type
 * Defines the base strength/power level of each unit when created
 * Higher values indicate more powerful units with greater combat effectiveness
 *
 * VALUE RANGES:
 * - Basic units (0-25): 1 (standard troops)
 * - Leaders (26-37): 50-250 (command units with varying power)
 * - Monsters (38-54): 50-1000 (magical creatures, dragons = 1000)
 *
 * Used by combat system to determine unit effectiveness and survival
 */
int unitminsth[] = {
    /* Basic Military Units (0-25): All have strength 1 */
    1, 1, 1, 1, 1, 1, 1, /* Militia through Archers */
    1, 1, 1, 1, 1, 1, 1, /* Uruk-Hai through Mercenaries */
    1, 1, 1, 1, 1, 1, 1, /* Trolls through Rocs */
    1, 1, 1, 1, 1, 1, /* Knights through Scout */

    /* Leaders and Nobles (26-37): Varying leadership strength */
    100, 50, 100, 50, 250, 50, /* King, Baron, Emperor, Prince, Wizard, Mage */
    100, 50, 100, 50, 250, 125, /* Pope, Bishop, Admiral, Captain, Warlord, Lord */

    /* Demons and Magical Creatures (38-49): High power levels */
    250, 50, 500, 100, 250, 125, /* Demon, Devil, Dragyn, Wyrm, Shadow, Nazgul */

    /* Summoned and Elemental Creatures (50-54): Extremely powerful */
    50, 50, 50, 75, 75, /* Spirit, Assassin, Efreet, Gargoyle, Wraith */
    75, 50, 150, 150, 150, /* Hero, Centaur, Giant, Superhero, Mummy */
    175, 150, 500, 500, 1000 /* Elemental, Minotaur, Daemon, Balrog, Dragon */
};

/*
 * unitattack - Attack bonus/penalty values for each unit type
 * Positive values = attack bonus, negative values = attack penalty
 * These modifiers are applied during combat resolution
 *
 * NOTABLE PATTERNS:
 * - Weak units (Militia, Goblins): Large negative penalties (-40, -15)
 * - Siege weapons (Catapults, Siege): Large negative penalties (-20) - not for direct combat
 * - Scouts and Spies: Large negative penalties (-30) - not combat units
 * - Elite combat units (Knights, Gryfins, Elephants): High bonuses (+40, +40, +50)
 * - Powerful monsters (Demons, Dragons): Very high bonuses (+50)
 */
int unitattack[] = {
    /* Basic Military Units (0-25) */
    -40, -15, 0, 0, 0, 5, 0, /* Militia through Archers */
    5, 20, 10, 15, 20, 10, 0, /* Uruk-Hai through Mercenaries */
    25, 20, 20, 30, -20, -20, 20, /* Trolls through Rocs */
    40, 40, 50, -15, -30, -30, /* Knights through Scout */

    /* Leaders and Nobles (26-37) */
    30, 20, 30, 20, 30, 20, /* King through Mage */
    30, 20, 30, 20, 30, 30, /* Pope through Lord */

    /* Demons and Magical Creatures (38-49) */
    50, 20, 50, 40, 50, 40, /* Demon through Nazgul */

    /* Summoned and Elemental Creatures (50-54) */
    0, 20, 10, 10, 10, /* Spirit through Wraith */
    0, 10, 0, 15, 15, /* Hero through Mummy */
    5, 20, 50, 40, 50 /* Elemental through Dragon */
};

/*
 * unitdefend - Defense bonus/penalty values for each unit type
 * Positive values = defense bonus, negative values = defense penalty
 * These modifiers are applied when unit is defending in combat
 *
 * NOTABLE PATTERNS:
 * - Weak units (Militia, Goblins): Negative defense penalties
 * - Archers: Positive defense bonus (+10) when defending
 * - Heavy units (Phalanx, Legionaries): Good defense bonuses (+10, +20)
 * - Fortified units (Knights, Gryfins): High defense bonuses (+40, +50)
 * - Siege weapons: Positive defense (+20) when stationary
 */
int unitdefend[] = {
    /* Basic Military Units (0-25) */
    -25, -15, 0, 0, 0, 0, 10, /* Militia through Archers */
    5, 0, 10, 15, 20, 10, 0, /* Uruk-Hai through Mercenaries */
    15, 20, 20, 30, 20, 20, 30, /* Trolls through Rocs */
    40, 50, 50, -15, -30, -30, /* Knights through Scout */

    /* Leaders and Nobles (26-37) */
    30, 20, 30, 20, 30, 20, /* King through Mage */
    30, 20, 30, 20, 30, 30, /* Pope through Lord */

    /* Demons and Magical Creatures (38-49) */
    50, 20, 50, 40, 50, 40, /* Demon through Nazgul */

    /* Summoned and Elemental Creatures (50-54) */
    0, 20, 10, 10, 10, /* Spirit through Wraith */
    0, 10, 0, 15, 15, /* Hero through Mummy */
    5, 20, 50, 40, 50 /* Elemental through Dragon */
};

/*
 * unitmove - Movement rates for each unit type (values multiplied by 10)
 * Higher values = faster movement, 0 = immobile units
 * Actual movement rate = value/10 (e.g., 20 = 2.0 movement rate)
 *
 * MOVEMENT PATTERNS:
 * - Immobile units (Militia, Sailors, Marines): 0 movement - defensive only
 * - Standard infantry: 10 (1.0 movement rate)
 * - Fast cavalry (Dragoons, Lt_Cavalry, Hv_Cavalry): 20 (2.0 movement rate)
 * - Elite units (Knights): 20 (fast heavy cavalry)
 * - Flying units (Rocs, Gryfins): 15-20 (aerial movement advantage)
 * - Siege engines (Catapults, Siege): 5 (very slow, heavy equipment)
 * - Leaders: Generally 20 (fast command units)
 * - Monsters: Variable 10-20 based on creature type
 */
int unitmove[] = {
    /* Basic Military Units (0-25) */
    0, 10, 10, 10, 0, 0, 10, /* Militia through Archers */
    10, 10, 10, 10, 10, 20, 10, /* Uruk-Hai through Mercenaries */
    10, 13, 20, 20, 5, 5, 10, /* Trolls through Rocs */
    20, 15, 5, 10, 10, 20, /* Knights through Scout */

    /* Leaders and Nobles (26-37) */
    20, 20, 20, 20, 20, 20, /* King through Mage */
    20, 20, 20, 20, 20, 20, /* Pope through Lord */

    /* Demons and Magical Creatures (38-49) */
    20, 20, 20, 20, 20, 20, /* Demon through Nazgul */

    /* Summoned and Elemental Creatures (50-54) */
    10, 10, 15, 10, 10, /* Spirit through Wraith */
    10, 15, 10, 10, 10, /* Hero through Mummy */
    15, 10, 10, 15, 20 /* Elemental through Dragon */
};

/*
 * u_enmetal - Metal costs for unit construction
 * Metal resources required to create each unit type
 * 0 = no metal cost (leaders, monsters don't require metal)
 *
 * COST PATTERNS:
 * - Basic infantry: 80-100 metal
 * - Advanced units: 150-200 metal
 * - Elite/Heavy units: 300-600 metal
 * - Siege weapons: 1000 metal (very expensive)
 * - Leaders and monsters: 0 (no metal requirement)
 */
int u_enmetal[] = {
    /* Basic Military Units (0-25) */
    0, 80, 80, 100, 100, 100, 100, /* Militia through Archers */
    150, 150, 150, 150, 150, 100, 0, /* Uruk-Hai through Mercenaries */
    200, 200, 100, 300, 1000, 1000, 300, /* Trolls through Rocs */
    600, 400, 600, 100, 0, 0, /* Knights through Scout */

    /* Leaders and Nobles (26-37) - No metal costs */
    0, 0, 0, 0, 0, 0, /* King through Mage */
    0, 0, 0, 0, 0, 0, /* Pope through Lord */

    /* Demons and Magical Creatures (38-49) - No metal costs */
    0, 0, 0, 0, 0, 0, /* Demon through Nazgul */

    /* Summoned and Elemental Creatures (50-54) - No metal costs */
    0, 0, 0, 0, 0, /* Spirit through Wraith */
    0, 0, 0, 0, 0, /* Hero through Mummy */
    0, 0, 0, 0, 0 /* Elemental through Dragon */
};

/*
 * u_encost - Enlistment costs for each unit type
 * For basic units (0-25): Gold cost for recruitment
 * For monsters (46-54): Spell points required for summoning
 * For leaders (26-45): No enlistment cost (special recruitment)
 *
 * DUAL COST SYSTEM:
 * - Gold costs: 50-10000 gold pieces for basic military units
 * - Spell costs: 2-15 spell points for magical creatures
 * - Leaders: 0 cost (born, not recruited)
 */
int u_encost[] = {
    /* Basic Military Units (0-25) - Gold costs */
    50, 70, 85, 100, 100, 100, 100, /* Militia through Archers */
    125, 125, 150, 180, 180, 300, 225, /* Uruk-Hai through Mercenaries */
    225, 225, 300, 450, 600, 600, 600, /* Trolls through Rocs */
    600, 800, 600, 100, 10000, 100, /* Knights through Scout */

    /* Leaders and Nobles (26-37) - No enlistment costs */
    0, 0, 0, 0, 0, 0, /* King through Mage */
    0, 0, 0, 0, 0, 0, /* Pope through Lord */

    /* Demons and Magical Creatures (38-49) - No direct costs */
    0, 0, 0, 0, 0, 0, /* Demon through Nazgul */

    /* Summoned and Elemental Creatures (50-54) - Spell point costs */
    2, 2, 2, 2, 2, /* Spirit through Wraith: 2 SP each */
    2, 2, 5, 5, 5, /* Hero through Mummy: 2-5 SP */
    5, 5, 10, 10, 15 /* Elemental through Dragon: 5-15 SP */
};

/*
 * unitmaint - Maintenance costs per turn for each unit type
 * For basic units (0-25): Gold cost per turn per unit
 * For monsters (46-54): Jewel cost per turn (multiply by 5 for gold equivalent)
 * For leaders (26-45): No maintenance cost
 *
 * MAINTENANCE PATTERNS:
 * - Basic troops: 20-250 gold per turn
 * - Elite units: Higher maintenance (200+ gold)
 * - Scouts: Very high maintenance (2000 gold) - balance mechanism
 * - Monsters: Jewel costs 200-10000 per turn (extremely expensive)
 */
int unitmaint[] = {
    /* Basic Military Units (0-25) - Gold per turn */
    20, 20, 50, 50, 50, 50, 50, /* Militia through Archers */
    50, 50, 50, 75, 75, 200, 100, /* Uruk-Hai through Mercenaries */
    100, 100, 175, 225, 250, 250, 250, /* Trolls through Rocs */
    250, 250, 250, 0, 2000, 100, /* Knights through Scout */

    /* Leaders and Nobles (26-37) - No maintenance costs */
    0, 0, 0, 0, 0, 0, /* King through Mage */
    0, 0, 0, 0, 0, 0, /* Pope through Lord */

    /* Demons and Magical Creatures (38-49) - No maintenance */
    0, 0, 0, 0, 0, 0, /* Demon through Nazgul */

    /* Summoned Creatures (50-54) - Jewel costs per turn */
    1000, 400, 400, 450, 450, /* Spirit through Wraith */
    200, 200, 2100, 450, 1000, /* Hero through Mummy */
    1900, 2100, 6000, 6000, 10000 /* Elemental through Dragon */
};

/*
 * ============================================================================
 * SECTION 4: FILE SYSTEM AND EXTERNAL PROGRAM CONFIGURATION
 * ============================================================================
 * This section defines file names and paths used by the game system for
 * data storage, external program execution, and game state management.
 */

/*
 * Core Game Files
 * These files are essential for basic game operation
 */
char *exefile = "exec"; /* External command execution file */
char *datafile = "data"; /* Main game data storage file */
char *msgfile = "mesg"; /* Message passing and communication file */
char *timefile = "timelog"; /* Game timing and scheduling log */
char *sortname = "conqsort"; /* External sorting utility program name */

#ifdef ADMIN
/*
 * Administrative Files (enabled with ADMIN compilation flag)
 */
char scenario[NAMELTH + 1]; /* Current scenario name buffer */
char *npcsfile = "nations"; /* NPC nation configuration file */
#endif /* ADMIN */

/*
 * Game Content and Interface Files
 */
char *helpfile = "help"; /* Help system content file */
char *newsfile = "news"; /* Game news and announcements file */
char *isonfile = "lock"; /* Game session lock file for multi-user coordination */

#ifdef TRADE
/*
 * Economic System Files (enabled with TRADE compilation flag)
 */
char *tradefile = "commerce"; /* Trade and economic data file */
#endif /* TRADE */

/*
 * ============================================================================
 * SECTION 5: GAME CONFIGURATION AND POWERS SYSTEM
 * ============================================================================
 * This section defines seasonal cycles, power systems, and special abilities
 * that nations and leaders can acquire throughout the game.
 */

/*
 * seasonstr - Season name array for temporal game mechanics
 * Four-season cycle used for random events, agricultural cycles,
 * and time-dependent game mechanics
 */
char *seasonstr[] = {"Winter", "Spring", "Summer", "Fall"};

/*
 * POWERS SYSTEM - Nation and Leader Special Abilities
 * The game features a comprehensive powers system with 31 different abilities
 * organized into three categories: Military, Civilian, and Magical powers.
 * Leaders and nations can acquire these powers to gain strategic advantages.
 */

/*
 * pwrname - Power name strings for display and user interface
 * Human-readable names for all 31 powers in the game system
 * Organized by category: Military (0-10), Civilian (11-23), Magical (24-30)
 */
char *pwrname[] = {
    /* Military Powers (0-10): Combat and warfare enhancements */
    "WARRIOR", "CAPTAIN", "WARLORD", "ARCHER", "CAVALRY", "SAPPER", "ARMOR", "AVIAN",
    "MI_MONST", "AV_MONST", "MA_MONST",

    /* Civilian Powers (11-23): Economic and infrastructure improvements */
    "SLAVER", "DERVISH", "HIDDEN", "ARCHITECT", "RELIGION", "MINER", "BREEDER", "URBAN",
    "STEEL", "NINJA", "SAILOR", "DEMOCRACY", "ROADS",

    /* Magical Powers (24-30): Supernatural abilities and magic */
    "THE_VOID", "KNOWALL", "DESTROYER", "VAMPIRE", "SUMMON", "WYZARD", "SORCERER", "ERROR"};

/*
 * powers - Power identifier constants array
 * Maps power indices to their corresponding constant values defined in header.h
 * Used by game logic to check for specific powers and apply their effects
 *
 * POWER CATEGORIES:
 * - Military Powers (0-10): WARRIOR through MA_MONST
 * - Civilian Powers (11-23): SLAVER through ROADS
 * - Magical Powers (24-30): THE_VOID through SORCERER
 * - Terminator: 0 (marks end of array)
 *
 * Each power provides specific gameplay benefits:
 * - Military: Combat bonuses, unit creation abilities, tactical advantages
 * - Civilian: Economic benefits, population growth, infrastructure
 * - Magical: Supernatural abilities, summoning, divination, destruction
 */
long powers[] = {
    /* Military Powers (0-10) */
    WARRIOR, /* 0: Basic military enhancement */
    CAPTAIN, /* 1: Leadership and command bonuses */
    WARLORD, /* 2: Advanced military tactics */
    ARCHER, /* 3: Ranged combat specialization */
    CAVALRY, /* 4: Mounted unit advantages */
    SAPPER, /* 5: Siege warfare and fortification */
    ARMOR, /* 6: Defensive combat bonuses */
    AVIAN, /* 7: Flying unit creation and control */
    MI_MONST, /* 8: Minor monster summoning */
    AV_MONST, /* 9: Average monster summoning */
    MA_MONST, /* 10: Major monster summoning */

    /* Civilian Powers (11-23) */
    SLAVER, /* 11: Population manipulation */
    DERVISH, /* 12: Desert and nomadic bonuses */
    HIDDEN, /* 13: Stealth and espionage */
    ARCHITECT, /* 14: Advanced construction */
    RELIGION, /* 15: Religious and morale bonuses */
    MINER, /* 16: Resource extraction bonuses */
    BREEDER, /* 17: Population growth enhancement */
    URBAN, /* 18: City development bonuses */
    STEEL, /* 19: Advanced metallurgy */
    NINJA, /* 20: Stealth combat specialists */
    SAILOR, /* 21: Naval and maritime bonuses */
    DEMOCRACY, /* 22: Political and diplomatic bonuses */
    ROADS, /* 23: Transportation infrastructure */

    /* Magical Powers (24-30) */
    THE_VOID, /* 24: Dark magic and void manipulation */
    KNOWALL, /* 25: Divination and information gathering */
    DESTROYER, /* 26: Destructive magical abilities */
    VAMPIRE, /* 27: Undead and life-drain powers */
    SUMMON, /* 28: General summoning abilities */
    WYZARD, /* 29: Advanced magical knowledge */
    SORCERER, /* 30: Master magical abilities */

    0 /* Array terminator */
};

/*
 * ============================================================================
 * SECTION 6: RUNTIME VARIABLES AND COMMUNICATION SYSTEM
 * ============================================================================
 * This section defines runtime file handles, communication systems, and
 * dynamic variables used during game execution.
 */

/*
 * File Handles and I/O Systems
 * Global file pointers and I/O state variables for game operation
 */
FILE *fnews; /* News file handle */
struct sprd_sht spread; /* Spreadsheet/data structure for game calculations */

FILE *fm; /* Main communication file handle */
int mailok = DONEMAIL; /* Mail system status flag */

#ifdef CONQUER
#ifdef SYSMAIL
/*
 * System Mail Configuration (SYSMAIL enabled builds)
 */
char sysmail[FILELTH]; /* System mail file path buffer */
int sys_mail_status; /* System mail status flag */
#endif /* SYSMAIL */

/*
 * Conquer Mail System Variables
 */
char conqmail[FILELTH]; /* Conquer mail file path buffer */
int conq_mail_status; /* Conquer mail system status */
#endif /* CONQUER */

/*
 * ============================================================================
 * SECTION 7: TRADE AND ECONOMIC SYSTEM DATA
 * ============================================================================
 * This section defines the comprehensive trade system with 63 different trade
 * goods, their values, production requirements, and economic characteristics.
 * The trade system forms a core part of the game's economic simulation.
 */

/*
 * Trade Good Classification and Value System
 * The trade system uses character-based encoding for efficiency:
 * - tg_stype: Specifies sector types that can produce each good
 * - tg_value: Economic value/rarity of each trade good (0-9 scale)
 * - tg_name: Human-readable names for all trade goods
 *
 * ENCODING SYSTEM:
 * tg_stype characters map to sector designations:
 * 'f'=farm, 't'=town, 'c'=city, 'm'=mine, 'l'=lumber, 'u'=university,
 * 'x'=any sector, '$'=goldmine, '?'=special, '0'=none/error
 *
 * tg_value scale: 1=common, 9=extremely rare/valuable
 */

/* Trade good sector type requirements (63 goods) */
/*                0         1         2         3         4         5         6  */
/*                0123456789012345678901234567890123456789012345678901234567890123 */
char *tg_stype = "xffffttttffffffffffftxlxllttuuctcccfff?xtccmmmmmmmm$$$$$$$$$$0";

/* Trade good value ratings (63 goods) */
/*                0         1         2         3         4         5         6  */
/*                0123456789012345678901234567890123456789012345678901234567890123 */
char *tg_value = "13335157911433442331131135734567789123937571111111111111111110";

/*
 * tg_name - Trade good descriptive names
 * Complete list of all 63 trade goods organized by category:
 * 0-8: Textiles and Livestock (furs through griffons)
 * 9-19: Food and Agriculture (corn through cereal)
 * 20-26: Construction Materials (pottery through nails)
 * 27-34: Knowledge and Culture (papyrus through philosophy)
 * 35-37: Tools and Equipment (irrigation through plows)
 * 38: Building Materials (stones)
 * 39-40: Medicine and Healing (herbs, medicine)
 * 41-42: Control and Oppression (torture, prison)
 * 43-50: Metals and Alloys (bronze through adamantine)
 * 51-61: Luxury Goods and Precious Materials (spice through platinum)
 * 62: Terminator (none)
 */
char *tg_name[] = {
    /* Textiles and Livestock (0-8) */
    "furs", "wool", "beer", "cloth", "wine", "mules", "horses", "pigeons", "griffons",

    /* Food and Agriculture (9-19) */
    "corn", "fish", "sugar", "honey", "fruit", "rice", "wheat", "dairy", "peas", "bread",
    "cereal",

    /* Construction Materials (20-26) */
    "pottery", "salt", "timber", "granite", "pine", "oak", "nails",

    /* Knowledge and Culture (27-34) */
    "papyrus", "math", "library", "drama", "paper", "literature", "law", "philosophy",

    /* Tools and Equipment (35-37) */
    "irrigation", "oxen", "plows",

    /* Building Materials (38) */
    "stones",

    /* Medicine and Healing (39-40) */
    "herbs", "medicine",

    /* Control and Oppression (41-42) */
    "torture", "prison",

    /* Metals and Alloys (43-50) */
    "bronze", "copper", "lead", "tin", "iron", "steel", "mithral", "adamantine",

    /* Luxury Goods and Precious Materials (51-61) */
    "spice", "silver", "pearls", "dye", "silk", "gold", "rubys", "ivory", "diamonds",
    "platinum",

    /* Terminator (62) */
    "none"};
