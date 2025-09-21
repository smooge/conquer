/*
 * data.h - Core data structures, constants, and function prototypes for Conquer
 *
 * This header file contains the complete data model for the Conquer game system,
 * including all major data structures (world, nations, sectors, armies, navies),
 * game constants, macros for data access, and function prototypes. This is the
 * central data definition file that defines the game's architecture.
 *
 * Key Components:
 * - Game constants and magic numbers
 * - Core data structures (s_world, s_nation, s_sector, army, navy)
 * - Nation strategy and diplomacy definitions
 * - Unit types, leaders, and monsters
 * - Magic powers and nation classes
 * - Trade goods and economic systems
 * - Screen and map display definitions
 * - Function prototypes for all major game systems
 *
 * Architecture Notes:
 * - Uses legacy K&R function prototypes (requires modernization)
 * - Extensive use of macros for data access and game calculations
 * - Complex bit manipulation for naval ship storage
 * - Global variables extensively used throughout system
 * - File I/O operations for game state persistence
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

/*--------I DO NOT BELIEVE IT IS NECESSARY TO ALTER THIS FILE----------------*/

/*
 * =============================================================================
 * FUNDAMENTAL SYSTEM CONSTANTS
 * =============================================================================
 * Basic return codes, limits, and system-wide constants used throughout the
 * game engine. These values control core system behavior and resource limits.
 */

/*
 * Shell return codes - Standard exit status values for process termination
 */
#define	FAIL	1		/* fail return to shell			*/
#define	SUCCESS	0		/* successful return to shell		*/

/*
 * System timing and limits
 */
#define TIME_DEAD	3600	/* number of seconds for file aging	*/
#define	BIG	500000000L	/* BIGGER THAN ANYTHING SHOULD BE	*/
				/* this is used to protect against overflow */

/*
 * Screen display calculations
 */
#define	SCRARM	((LINES-14)/2)	/* number of armies to fit on screen	*/

/*
 * Boolean constants - Standard true/false values with curses compatibility
 */
/* sometimes curses.h defines TRUE	*/
#ifndef TRUE
#define	TRUE		1
#define	FALSE		0
#endif

/*
 * Mail system constants - Control values for in-game messaging system
 */
#define	DONEMAIL	(-3)	/* Mail composition completed */
#define	NEWSMAIL	(-2)	/* News/bulletin mail type */
#define	ABORTMAIL	(-1)	/* Mail composition aborted */

/*
 * Screen refresh modes - Control levels for display updating
 */
#define	DONE	0		/* No screen update needed */
#define	PART	1		/* Partial screen refresh */
#define	FULL	2		/* Full screen redraw required */

/*
 * Screen geometry and visibility calculations
 */
#define	SCREEN_X_SIZE	(( COLS - 21) / 2)	/* divide by two as only 1/2
						                       sectors will be shown */
#define	SCREEN_Y_SIZE	( LINES - 5 )
#define	HAS_SEEN(x,y)	hasseen[(x)+((y)*((COLS-10)/2))]	/* sector visibility array access */

/*
 * String length limits - Maximum sizes for various text fields
 */
#define	PASSLTH		7	/* the number of characters in the passwd*/
#define	NAMELTH		9	/* the number of characters in the name*/
#define	LEADERLTH	9	/* the number of characters in the leader*/
#define	FILELTH		80	/*length for filename holders*/
#define	LINELTH		80	/*length for input string lines*/
#define	BIGLTH		256	/*length for large storage strings*/

/*
 * Game system limits
 */
#define	NUMCLASS	11	/*number of nation classes */

/*
 * Environment configuration
 */
#define	ENVIRON_OPTS	"CONQ_OPTS"	/* environment variable strings to check */

/*
 * =============================================================================
 * TERRAIN AND WORLD MAP DEFINITIONS
 * =============================================================================
 * Constants defining the physical geography and racial characteristics of the
 * game world. These include elevation levels, racial identifiers, and terrain
 * features that affect gameplay mechanics.
 */

/*
 * Elevation/contour map definitions - Terrain altitude levels
 * Uses dynamic array access through 'ele' pointer for configurable terrain
 */
#define	WATER		(*(ele+0))	/* Water/ocean sectors */
#define	PEAK		(*(ele+1))	/* Mountain peaks */
#define	MOUNTAIN	(*(ele+2))	/* Mountain terrain */
#define	HILL		(*(ele+3))	/* Hilly terrain */
#define	CLEAR		(*(ele+4))	/* Clear/flat terrain */

/*
 * Racial type identifiers - Character codes for different nation races
 * These single-character codes identify the racial type of nations and NPCs
 */
#define	GOD		'-'		/* God/divine entities */
#define	ORC		'O'		/* Orc nations */
#define	ELF		'E'		/* Elf nations */
#define	DWARF		'D'		/* Dwarf nations */
#define	LIZARD		'L'		/* Lizardmen */
#define	HUMAN		'H'		/* Human nations */
#define	PIRATE		'P'		/* Pirate factions */
#define	SAVAGE		'S'		/* Savage tribes */
#define	NOMAD		'N'		/* Nomadic peoples */
#define	TUNKNOWN	'?'		/* Unknown/unidentified race */

/*
 * Sector designations - Buildings and infrastructure types
 * Uses dynamic array access through 'des' pointer for configurable designations
 * These define what type of structure or development exists in a sector
 */
#define	DTOWN		(*(des+0))	/* Town settlement */
#define	DCITY		(*(des+1))	/* City settlement */
#define	DMINE		(*(des+2))	/* Mining operation */
#define	DFARM		(*(des+3))	/* Agricultural farm */
#define	DDEVASTATED	(*(des+4))	/* Devastated/destroyed area */
#define	DGOLDMINE	(*(des+5))	/* Gold mining operation */
#define	DFORT		(*(des+6))	/* Military fortress */
#define	DRUIN		(*(des+7))	/* Ruined structure */
#define	DSTOCKADE	(*(des+8))	/* Wooden fortification */
#define	DCAPITOL	(*(des+9))	/* Nation capital city */
#define	DSPECIAL	(*(des+10))	/* Special/unique designation */
#define	DLUMBERYD	(*(des+11))	/* Lumber yard */
#define	DBLKSMITH	(*(des+12))	/* Blacksmith shop */
#define	DROAD		(*(des+13))	/* Road infrastructure */
#define	DMILL		(*(des+14))	/* Mill facility */
#define	DGRANARY	(*(des+15))	/* Food storage granary */
#define	DCHURCH		(*(des+16))	/* Religious structure */
#define	DUNIVERSITY	(*(des+17))	/* Educational institution */
#define	DNODESIG	(*(des+18))	/* No designation (wilderness) */
#define	DBASECAMP	(*(des+19))	/* Military base camp */

/*
 * Nation placement variables - Control nation starting positions in world generation
 */
#define	GREAT		'G'		/* Great starting position */
#define	FAIR		'F'		/* Fair starting position */
#define	RANDOM		'R'		/* Random placement */
#define	OOPS		'X'		/* Error/invalid placement */

/*
 * Vegetation types - Natural terrain cover affecting sector productivity
 * Uses dynamic array access through 'veg' pointer for configurable vegetation
 * These are also legal in designations for mixed terrain types
 */
#define	VOLCANO		(*(veg+0))	/* Volcanic terrain */
#define	DESERT		(*(veg+1))	/* Desert terrain */
#define	TUNDRA		(*(veg+2))	/* Frozen tundra */
#define	BARREN		(*(veg+3))	/* Barren wasteland */
#define	LT_VEG		(*(veg+4))	/* Light vegetation */
#define	GOOD		(*(veg+5))	/* Good farmland */
#define	WOOD		(*(veg+6))	/* Wooded terrain */
#define	FOREST		(*(veg+7))	/* Dense forest */
#define	JUNGLE		(*(veg+8))	/* Tropical jungle */
#define	SWAMP		(*(veg+9))	/* Swamp/marsh */
#define	ICE		(*(veg+10))	/* Ice/frozen terrain */
#define	NONE		(*(veg+11))	/* No vegetation */

/*
 * =============================================================================
 * DIPLOMACY AND MILITARY SYSTEMS
 * =============================================================================
 */

/*
 * Diplomacy Variables - Relationship levels between nations
 * Ordered from most hostile (7) to unmet (0)
 */
#define	JIHAD		7	/* Holy war - highest hostility */
#define	WAR		6	/* Open warfare */
#define	HOSTILE		5	/* Hostile relations */
#define	NEUTRAL		4	/* Neutral/no formal relations */
#define	FRIENDLY	3	/* Friendly relations */
#define	ALLIED		2	/* Military alliance */
#define	TREATY		1	/* Peace treaty */
#define	UNMET		0	/* Nations have not met */

#define	BREAKJIHAD	200000L /* $ cost to break confederacy or jihad	*/

/*
 * Army status definitions - Combat and positioning modes for military units
 * These control army behavior, combat engagement, and special abilities
 */
#define	MARCH		1	/* March - standard movement */
#define	SCOUT		2	/* Scouting - will not engage enemy if possible */
#define	GARRISON	3	/* Garrison - defending a town or Capitol */
#define	TRADED		4	/* Indicates an army that has been traded */
#define	MILITIA		5	/* Unit is a militia unit */
#define	FLIGHT		6	/* Indicates army is flying */
#define	DEFEND		7	/* Defensive stance */
#define	MAGDEF		8	/* Magically enhanced defend mode */
#define	ATTACK		9	/* Attack anybody (Hostile+) within 2 sectors */
#define	MAGATT		10	/* Magically enhanced attack mode */
#define	GENERAL		11	/* Group leader !!! */
#define	SORTIE		12	/* Quick attack from a city */
#define	SIEGE		13	/* Set siege on a city */
#define	SIEGED		14	/* Unit under siege */
#define	ONBOARD		15	/* On board a fleet */
#define	RULE		16	/* Nation leader status for capitol */
#define	NUMSTATUS	17	/* number of possible stats */
				/* Army groups are implemented in the army
				status vbl.  if >= NUMSTATUS, you belong to
				army group x-NUMSTATUS - army groups always
				of status attack and may not be magicked */

/*
 * Time and seasonal system - Game calendar and turn progression
 */
#define	TURN		world.turn	/* current game turn */
#define	SEASON(x)	((x)%4)		/* calculate season from turn number */
#define	PSEASON(x)	seasonstr[((x)%4)]	/* season name string */
#define	YEAR(x)		((int)((x+3)/4))	/* calculate year from turn */
#define	WINTER		0		/* Winter season */
#define	SPRING		1		/* Spring season */
#define	SUMMER		2		/* Summer season */
#define	FALL		3		/* Fall/Autumn season */

/*
 * Compass directions - Movement and positioning system
 * Used for army movement, map navigation, and spatial calculations
 */
#define	CENTERED	0		/* Current position/no movement */
#define	NORTH		1		/* North direction */
#define	NORTHEAST	2		/* Northeast direction */
#define	EAST		3		/* East direction */
#define	SOUTHEAST	4		/* Southeast direction */
#define	SOUTH		5		/* South direction */
#define	SOUTHWEST	6		/* Southwest direction */
#define	WEST		7		/* West direction */
#define	NORTHWEST	8		/* Northwest direction */

/*
 * =============================================================================
 * CORE DATA STRUCTURES
 * =============================================================================
 */

/*
 * s_world - Global world state and statistics
 *
 * Central data structure containing world-wide game state including map
 * dimensions, turn counter, global resource totals, and mercenary market.
 * This structure tracks aggregate statistics across all nations and sectors.
 *
 * Fields:
 *   mapx, mapy - World map dimensions (width x height in sectors)
 *   nations - Number of active player nations
 *   othrntns - Number of NPC nations (gods, lizards, etc.)
 *   turn - Current game turn number (used for seasonal calculations)
 *   m_mil - Available mercenaries in the global mercenary market
 *   m_aplus, m_dplus - Mercenary combat bonuses (attack/defense)
 *   w_jewels - Total jewels existing in the world economy
 *   w_gold - Total gold talons in circulation
 *   w_food - Total food resources across all sectors
 *   w_metal - Total metal resources in the world
 *   w_civ - Total civilian population across all sectors
 *   w_mil - Total military units across all armies
 *   w_sctrs - Total number of owned/developed sectors
 *   score - Aggregate world score (sum of all nation scores)
 */
struct	s_world
{
	short	mapx,mapy;	/* size of world		*/
	short	nations;	/* number of actual nations	*/
	short	othrntns;	/* god, lizard...		*/
	short	turn;		/* game turn			*/
	long	m_mil;		/* number of mercs available	*/
	short	m_aplus;	/* mercenary attack bonus	*/
	short	m_dplus;	/* mercenary defense bonus	*/
	long	w_jewels;	/* jewels in world		*/
	long	w_gold;		/* gold talons in world		*/
	long	w_food;		/* food in world		*/
	long	w_metal;	/* metal in world		*/
	long	w_civ;		/* world population		*/
	long	w_mil;		/* world military		*/
	long	w_sctrs;	/* owned sectors in world	*/
	long	score;		/* world score total		*/
};

/*
 * World data accessor macros - Convenient access to global world state
 * These macros provide easy access to world structure fields throughout the codebase
 */
#define MAPX		world.mapx	/* World map width */
#define MAPY		world.mapy	/* World map height */
#define	MERCMEN		world.m_mil	/* Available mercenaries */
#define	MERCATT		world.m_aplus	/* Mercenary attack bonus */
#define	MERCDEF		world.m_dplus	/* Mercenary defense bonus */
#define	WORLDJEWELS	world.w_jewels	/* Total world jewels */
#define	WORLDGOLD	world.w_gold	/* Total world gold */
#define	WORLDMETAL	world.w_metal	/* Total world metal */
#define	WORLDFOOD	world.w_food	/* Total world food */
#define	WORLDSCORE	world.score	/* Total world score */
#define	WORLDCIV	world.w_civ	/* Total world civilians */
#define	WORLDSCT	world.w_sctrs	/* Total owned sectors */
#define	WORLDMIL	world.w_mil	/* Total world military */
#define	WORLDNTN	world.nations	/* Number of nations */

/*
 * s_sector - Individual map sector data
 *
 * Represents a single hexagonal sector on the world map containing all
 * sector-specific information including terrain, ownership, population,
 * resources, and infrastructure. This is the fundamental unit of the
 * game world that players interact with and develop.
 *
 * Fields:
 *   designation - Type of development/building in sector (see DTOWN, DCITY, etc.)
 *   altitude - Terrain elevation level (affects movement, combat)
 *   vegetation - Natural vegetation type (affects productivity)
 *   owner - Nation ID that owns this sector (0 = unowned)
 *   people - Current civilian population in sector
 *   i_people - Initial civilian population at world generation
 *   jewels - Jewel production capacity of this sector
 *   fortress - Fortification level (0-12, affects defense)
 *   metal - Metal production capacity
 *   tradegood - Type of exotic trade good produced (affects economy)
 *   region - (Commented out) Region index for future regional system
 */
struct s_sector
{
	unsigned char	designation;	/* designation of sector	*/
	unsigned char	altitude;	/* sector altitude		*/
	unsigned char	vegetation;	/* sector vegetation		*/
	unsigned char	owner;		/* nation id of owner		*/
	long	people;			/* civilians in sector		*/
	short	i_people;		/* initial civilians in sector	*/
	unsigned char	jewels;		/* jewel production ability	*/
	unsigned char	fortress;	/* fortification level; 0 to 12	*/
	unsigned char	metal;		/* metal produced by sector	*/
	unsigned char	tradegood;	/* exotic trade goods in sector	*/
	/* unsigned char	region; */		/* index of region	*/
};
/*
 * Sector accessor macros - Quick access to current sector data
 */
#define	SOWN	sct[xcurs+xoffset][ycurs+yoffset].owner	/* Current sector owner */
#define	XREAL	(xcurs+xoffset)		/* Real X coordinate on map */
#define	YREAL	(ycurs+yoffset)		/* Real Y coordinate on map */

/*
 * navy - Naval fleet structure
 *
 * Represents a naval fleet containing different ship types and their
 * cargo/crew. Ships use bit-packed storage to efficiently store multiple
 * ship sizes within the warships/merchant/galleys fields.
 *
 * Fields:
 *   warships - Bit-packed warship counts by size (light/medium/heavy)
 *   merchant - Bit-packed merchant ship counts by size
 *   galleys - Bit-packed galley counts by size
 *   xloc, yloc - Fleet location coordinates on map
 *   smove - Movement points remaining this turn
 *   crew - Number of crew aboard the fleet
 *   people - Civilian passengers carried
 *   commodity - Reserved for future trade commodity system
 *   armynum - Army index if fleet is carrying troops
 *
 * Note: Ship storage uses complex bit manipulation (see naval macros below)
 * to pack multiple ship size categories into single fields.
 */
struct navy
{
	unsigned short warships;
	unsigned short merchant;
	unsigned short galleys;
	unsigned char xloc;
	unsigned char yloc;
	unsigned char smove;		/* movement ability of ship */
	unsigned char crew;		/* crew on ship */
	unsigned char people;		/* people carried */
	unsigned char commodity;	/* future commodities */
	unsigned char armynum;		/* army carried */
};


/*
 * =============================================================================
 * NATION STRATEGY AND AI BEHAVIOR DEFINITIONS
 * =============================================================================
 *
 * NATION STRATEGY: A nation's strategy is part of the ntn[].active variable
 * which determines if it is a PC or NPC, its moral alignment, and behavioral
 * constraints. Alignments can be good, neutral, or evil.
 *
 * NPC Nations can be:
 * - Expansionist (with 0,2,4,6 sectors available to expand into)
 * - Isolationist (non-expansionist behavior)
 * - Special monster types (peasants, pirates, lizards, nomads, savages)
 */
#define	INACTIVE	0
#define	PC_GOOD		1		/* PC NATIONS	*/
#define	PC_NEUTRAL	2
#define	PC_EVIL		3
#define	GOOD_0FREE	4		/* NPC NATIONS	*/
#define	GOOD_2FREE	5
#define	GOOD_4FREE	6
#define	GOOD_6FREE	7
#define	NEUTRAL_0FREE	8
#define	NEUTRAL_2FREE	9
#define	NEUTRAL_4FREE	10
#define	NEUTRAL_6FREE	11
#define	EVIL_0FREE	12
#define	EVIL_2FREE	13
#define	EVIL_4FREE	14
#define	EVIL_6FREE	15
#define	ISOLATIONIST	16		/* ISOLATIONIST NATIONS	*/
#define	NPC_PEASANT	17		/* PEASANT REVOLT TYPE NATIONS	*/
#define NPC_PIRATE	18
#define NPC_LIZARD	19
#define NPC_NOMAD	20
#define NPC_SAVAGE	21

#define	ispc(x)		(((x)==PC_GOOD)||((x)==PC_EVIL)||((x)==PC_NEUTRAL))
#define	npctype(x)	(ispc(x) ? (x) : (ismonst(x)?0:(x)/4))
#define	isgood(x)	(npctype(x) == 1)
#define	isneutral(x)	(npctype(x) == 2)
#define	isevil(x)	(npctype(x) == 3)
#define	isnpc(x)	(((x)>=GOOD_0FREE)&&((x)<=ISOLATIONIST))
#define	isnotpc(x)	(((x)>=GOOD_0FREE)&&((x)!=INACTIVE))
#define	ismonst(x)	((x)>=NPC_PEASANT)
#define	ispeasant(x)	((x)==NPC_PEASANT)
#define	isntn(x)	(((x)!=INACTIVE)&&((x)<=ISOLATIONIST))
#define	isntnorp(x)	(((x)!=INACTIVE)&&((x)<=NPC_PEASANT))
#define	isactive(x)	((x)!=INACTIVE)

/* definitions for mail checker */
#define	NO_MAIL		0
#define	NEW_MAIL	1

/* definitions for naval types	*/
#define	N_LIGHT		0
#define	N_MEDIUM	1
#define	N_HEAVY		2
#define	N_BITSIZE	5
#define	N_MASK		((unsigned short) 0x001f)

/* definitions for naval calculations */
#define	N_WSPD		20	/* speed of warships */
#define	N_GSPD		18	/* speed of galleys */
#define	N_MSPD		15	/* speed of merchants */
#define	N_NOSPD		0	/* no ships no speed */
#define	N_SIZESPD	3	/* bonus speed for lighter ships */

#define	UTYPE		75
#define	TWOUTYPE	150	/* two times value of UTYPE above	*/

#define	A_MILITIA	0
#define	A_GOBLIN	1
#define	A_ORC		2
#define	A_INFANTRY	3
#define	A_SAILOR	4
#define	A_MARINES	5
#define	A_ARCHER	6
#define	A_URUK		7
#define	A_NINJA		8
#define	A_PHALANX	9
#define	A_OLOG		10
#define	A_LEGION	11
#define	A_DRAGOON	12
#define	A_MERCENARY	13
#define	A_TROLL		14
#define	A_ELITE		15
#define	A_LT_CAV	16
#define	A_CAVALRY	17
#define	A_CATAPULT	18
#define	A_SIEGE		19
#define	A_ROC		20
#define	A_KNIGHT	21
#define	A_GRIFFON	22
#define	A_ELEPHANT	23
#define	A_ZOMBIE	24
#define A_SPY		25
#define A_SCOUT		26
#define	NOUNITTYPES	26	/*number of unit types*/

#define	MINLEADER	(27+UTYPE)	/* min value of a leader -1*/
#define	L_KING		(27+UTYPE)
#define	L_BARON		(28+UTYPE)
#define	L_EMPEROR	(29+UTYPE)
#define	L_PRINCE	(30+UTYPE)
#define	L_WIZARD	(31+UTYPE)
#define	L_MAGI		(32+UTYPE)
#define	L_APOSTLE	(33+UTYPE)
#define	L_BISHOP	(34+UTYPE)
#define	L_ADMIRAL	(35+UTYPE)
#define	L_CAPTAIN	(36+UTYPE)
#define	L_WARLORD	(37+UTYPE)
#define	L_LORD		(38+UTYPE)
#define	L_DEMON		(39+UTYPE)
#define	L_DEVIL		(40+UTYPE)
#define	L_DRAGON	(41+UTYPE)
#define	L_WYRM		(42+UTYPE)
#define	L_SHADOW	(43+UTYPE)
#define	L_NAZGUL	(44+UTYPE)

#define	MINMONSTER	(45+TWOUTYPE)	/* min value of a monster	*/
#define	SPIRIT		(45+TWOUTYPE)
#define	ASSASSIN	(46+TWOUTYPE)
#define	DJINNI		(47+TWOUTYPE)
#define	GARGOYLE	(48+TWOUTYPE)
#define	WRAITH		(49+TWOUTYPE)
#define	HERO		(50+TWOUTYPE)
#define	CENTAUR		(51+TWOUTYPE)
#define	GIANT		(52+TWOUTYPE)
#define	SUPERHERO	(53+TWOUTYPE)
#define	MUMMY		(54+TWOUTYPE)
#define	ELEMENTAL	(55+TWOUTYPE)
#define	MINOTAUR	(56+TWOUTYPE)
#define	DEMON		(57+TWOUTYPE)
#define	BALROG		(58+TWOUTYPE)
#define	DRAGON		(59+TWOUTYPE)
#define	MAXMONSTER	(59+TWOUTYPE)

/*
 * army - Military unit structure
 *
 * Represents a single army unit with its type, location, movement,
 * troop count, and current status/orders.
 *
 * Fields:
 *   unittyp - Unit type ID (see A_MILITIA, A_INFANTRY, etc.)
 *   xloc, yloc - Army location coordinates on map
 *   smove - Movement points remaining this turn
 *   sold - Number of soldiers in this army unit
 *   stat - Army status/orders (see MARCH, DEFEND, ATTACK, etc.)
 */
struct army
{
	unsigned char unittyp;
	unsigned char xloc;
	unsigned char yloc;
	unsigned char smove;
	long sold;
	unsigned char stat;
};

/*
 * s_nation - Complete nation state structure
 *
 * This is the most complex and critical data structure containing all
 * information about a nation including identification, resources, military
 * forces, diplomatic relations, and national characteristics. Each nation
 * (PC and NPC) has one instance of this structure.
 *
 * Key sections:
 * - Identity: name, password, leader, race, location
 * - Resources: gold, jewels, food, metal, civilians, military
 * - Military: armies array, navies array, combat bonuses
 * - Diplomacy: diplomatic status with all other nations
 * - National Stats: prestige, popularity, wealth, knowledge, etc.
 * - Magic: powers bitmask, spell points
 * - Economics: tax rate, inflation, charity budget
 *
 * Note: Contains fixed-size arrays for armies (MAXARM) and navies (MAXNAVY)
 * and diplomatic relations (NTOTAL). This structure is extensively accessed
 * throughout the game via the global 'ntn' array and 'curntn' pointer.
 */
struct	s_nation		/* player nation stats	*/
{
	char	name[NAMELTH+1];	/* name			*/
	char	passwd[PASSLTH+1];	/* password		*/
	char	leader[LEADERLTH+1];	/* leader title		*/
	char	race;		/* national race (integer--see header.h)*/
	char	location;	/* location variable (gfr)	*/
	char	mark;		/* unique mark for nation	*/
#ifdef CHECKUSER
	short int uid;		/* for uid checking (CHECKUSER) */
#endif /* CHECKUSER */
	unsigned char	capx;	/* Capitol x coordinate		*/
	unsigned char	capy;	/* Capitol y coordinate		*/
	unsigned char	active;	/* nation type and strategy	*/
	unsigned char	maxmove;/* maximum movement of soldiers */
	char	repro;		/* reproduction rate of nation	*/
	long	score;		/* score			*/
	long	tgold;		/* gold in treasury		*/
	long	jewels;		/* raw amount of jewels in treasury	*/
	long	tmil;		/* total military		*/
	long	tciv;		/* total civilians		*/
	long	metals;		/* total real metal in nation	*/
	long	tfood;		/* total food in nation		*/
	long	powers;		/* magic powers bitmask		*/
	short	class;		/* national class		*/
	short	aplus;		/* attack plus of all soldiers	*/
	short	dplus;		/* defense plus of all soldiers	*/
	short	spellpts;	/* spell points			*/
	short	tsctrs;		/* total number sectors		*/
	short	tships;		/* number warships		*/
	short	inflation;	/* inflation rate		*/
	unsigned char charity;	/* charity budget (% of Taxes)	*/
	struct	army arm[MAXARM];	/* nation's armies	*/
	struct	navy nvy[MAXNAVY];	/* nation's navies	*/
	char	dstatus[NTOTAL];	/* diplomatic status with all nations */
	unsigned char	tax_rate;	/* taxrate populace	*/
	unsigned char	prestige;	/* nations prestige	*/
	unsigned char	popularity;	/* governments popularity	*/
	unsigned char	power;		/* nation power		*/
	unsigned char	communications;	/* leader communication	*/
	unsigned char	wealth;		/* per capita income	*/
	unsigned char	eatrate;	/* food eaten / 10 people*/
	unsigned char	spoilrate;	/* food spoilage rate	*/
	unsigned char	knowledge;	/* general knowledge	*/
	unsigned char	farm_ability;	/* farming ability	*/
	unsigned char	mine_ability;	/* mine ability		*/
	unsigned char	poverty;	/* % poor people	*/
	unsigned char	terror;		/* peoples terror of you*/
	unsigned char	reputation;	/* reputation of nation	*/
};
#define	P_NTNCOM	((((float) curntn->communications)/ 50.0 ))
#define	P_EATRATE	((((float) curntn->eatrate) / 25.0 ))
#define	P_REPRORATE	( ((float) (100.0 + 25.0*curntn->repro )/100.0))

#define	P_ATYPE	curntn->arm[armynum].unittyp
#define	P_AXLOC	curntn->arm[armynum].xloc
#define	P_AYLOC	curntn->arm[armynum].yloc
#define	P_AMOVE	curntn->arm[armynum].smove
#define	P_ASOLD	curntn->arm[armynum].sold
#define	P_ASTAT	curntn->arm[armynum].stat
#define	P_NWSHP	curntn->nvy[nvynum].warships
#define	P_NMSHP	curntn->nvy[nvynum].merchant
#define P_NGSHP	curntn->nvy[nvynum].galleys
#define	P_NCREW	curntn->nvy[nvynum].crew
#define	P_NARMY	curntn->nvy[nvynum].armynum
#define	P_NPEOP	curntn->nvy[nvynum].people
#define	P_NXLOC	curntn->nvy[nvynum].xloc
#define	P_NYLOC	curntn->nvy[nvynum].yloc
#define	P_NMOVE	curntn->nvy[nvynum].smove

/* naval routine macros */
#define	SHIPS(x,y)	(short)( ((x)&(N_MASK<<((y)*N_BITSIZE))) >> ((y)*N_BITSIZE) )
#define	P_NWAR(x)	SHIPS(P_NWSHP,x)
#define	P_NMER(x)	SHIPS(P_NMSHP,x)
#define	P_NGAL(x)	SHIPS(P_NGSHP,x)
#define	NADD_WAR(x)	addwships(nvynum,shipsize,(x));
#define	NADD_MER(x)	addmships(nvynum,shipsize,(x));
#define	NADD_GAL(x)	addgships(nvynum,shipsize,(x));
#define	NSUB_WAR(x)	subwships(nvynum,shipsize,(x));
#define	NSUB_MER(x)	submships(nvynum,shipsize,(x));
#define	NSUB_GAL(x)	subgships(nvynum,shipsize,(x));

#define	ATYPE	ntn[country].arm[armynum].unittyp
#define	AXLOC	ntn[country].arm[armynum].xloc
#define	AYLOC	ntn[country].arm[armynum].yloc
#define	AMOVE	ntn[country].arm[armynum].smove
#define	ASOLD	ntn[country].arm[armynum].sold
#define	ASTAT	ntn[country].arm[armynum].stat
#define	NWSHP	ntn[country].nvy[nvynum].warships
#define	NMSHP	ntn[country].nvy[nvynum].merchant
#define	NGSHP	ntn[country].nvy[nvynum].galleys
#define	NCREW	ntn[country].nvy[nvynum].crew
#define	NARMY	ntn[country].nvy[nvynum].armynum
#define	NPEOP	ntn[country].nvy[nvynum].people
#define	NXLOC	ntn[country].nvy[nvynum].xloc
#define	NYLOC	ntn[country].nvy[nvynum].yloc
#define	NMOVE	ntn[country].nvy[nvynum].smove

/* these defines are for types of magic	powers	*/
#define	M_MIL		1	/* military	*/
#define	M_CIV		2	/* civilian	*/
#define	M_MGK		3	/* magical	*/
#define M_TECH		4	/* technological	*/
#define	M_ALL		5

#define	AORN		0
#define	ARMY		1
#define	NAVY		2

#define	XASTAT		1
#define	XAMEN		2
#define	XBRIBE		3
#define	XALOC		4
#define	XNLOC		5
#define	XNAMER		6
#define	XNACREW		7
#define	XECNAME		8
#define	XECPAS		9
#define	EDSPL		10
#define	XSADES		11
#define	XSACIV		12
#define	XSIFORT		13
#define	XNAGOLD		14
#define	XAMOV		15
#define	XNMOV		16
#define	XSAOWN		17
#define	EDADJ		18
#define	XNARGOLD	19
#define	XNAMETAL	20
#define	INCAPLUS	22
#define	INCDPLUS	23
#define	CHG_MGK		24
#define	DESTRY		25
#define	MSETA		26
#define	MSETB		27
#define NTAX		28
#define XNAWAR		29
#define XNAGAL		30
#define XNAHOLD		31
#define NPOP		32
#define XSACIV3	33

#define	BRIBENATION fprintf(fm,"L_NGOLD\t%d\t%d\t%ld\t0\t%d\t%s\n",XBRIBE,country,bribecost,nation,"null");
#define	DESTROY fprintf(fexe,"DESTROY\t%d\t%d\t%hd\t0\t0\t%s\n",DESTRY,save,country,"null")
#define	CHGMGK fprintf(fexe,"L_MAGIC\t%d\t%hd\t%ld\t%ld\t0\t%s\n",CHG_MGK,country,ntn[country].powers,x,"null")
#define	I_APLUS	fprintf(fexe,"INC_APLUS\t%d\t%hd\t0\t0\t0\t%s\n",INCAPLUS,country,"null")
#define	I_DPLUS	fprintf(fexe,"INC_DPLUS\t%d\t%hd\t0\t0\t0\t%s\n",INCDPLUS,country,"null")
#define	AADJMERC fprintf(fexe,"A_MERC\t%d\t%d\t%ld\t0\t0\t%s\n",MSETA,country,men,"null")
#define	AADJDISB fprintf(fexe,"A_MERC\t%d\t%d\t%ld\t%d\t%d\t%s\n",MSETB,country,bemerc,attset,defset,"null")
#define	AADJSTAT fprintf(fexe,"A_STAT\t%d\t%hd\t%d\t%d\t0\t%s\n",XASTAT,country,armynum,ntn[country].arm[armynum].stat,"null")
#define	AADJMEN	fprintf(fexe,"L_ADJMEN\t%d\t%hd\t%hd\t%ld\t%d\t%s\n",XAMEN,country,armynum,P_ASOLD,P_ATYPE,"null")
#define	NADJCRW	fprintf(fexe,"N_ASHP\t%d\t%hd\t%hd\t%d\t%d\t%s\n",XNACREW,country,nvynum,NCREW,ntn[country].nvy[nvynum].armynum,"null")
#define	NADJMER	fprintf(fexe,"N_ASHP\t%d\t%hd\t%d\t%hd\t%d\t%s\n",XNAMER,country,nvynum,ntn[country].nvy[nvynum].merchant,0,"null")
#define	NADJWAR	fprintf(fexe,"N_ASHP\t%d\t%hd\t%d\t%hd\t%d\t%s\n",XNAWAR,country,nvynum,ntn[country].nvy[nvynum].warships,0,"null")
#define	NADJGAL	fprintf(fexe,"N_ASHP\t%d\t%hd\t%d\t%hd\t%d\t%s\n",XNAGAL,country,nvynum,ntn[country].nvy[nvynum].galleys,0,"null")
#define	NADJHLD	fprintf(fexe,"N_ASHP\t%d\t%hd\t%d\t%hd\t%d\t%s\n",XNAHOLD,country,nvynum,ntn[country].nvy[nvynum].armynum,ntn[country].nvy[nvynum].people,"null")
#define	AADJLOC	fprintf(fexe,"A_LOC\t%d\t%hd\t%d\t%d\t%d\t%s\n",XALOC,country,armynum,ntn[country].arm[armynum].xloc,ntn[country].arm[armynum].yloc,"null")
#define	NADJLOC	fprintf(fexe,"N_LOC\t%d\t%hd\t%d\t%d\t%d\t%s\n",XNLOC,country,nvynum,ntn[country].nvy[nvynum].xloc,ntn[country].nvy[nvynum].yloc,"null")
#define	AADJMOV	fprintf(fexe,"A_MOV\t%d\t%hd\t%d\t%d\t0\t%s\n",XAMOV,country,armynum,ntn[country].arm[armynum].smove,"null")
#define	NADJMOV	fprintf(fexe,"N_MOV\t%d\t%hd\t%d\t%d\t0\t%s\n",XNMOV,country,nvynum,ntn[country].nvy[nvynum].smove,"null")
#define	ECHGNAME fprintf(fexe,"E_CNAME\t%d\t%hd\t0\t0\t0\t%s\n",XECNAME,country,ntn[country].name)
#define	ECHGPAS	fprintf(fexe,"E_CPAS\t%d\t%hd\t0\t0\t0\t%s\n",XECPAS,country,curntn->passwd)
#define	SADJDES	fprintf(fexe,"S_ADES\t%d\t%hd\t0\t%d\t%d\t%c\n",XSADES,country,xcurs+xoffset,ycurs+yoffset,sct[xcurs+xoffset][ycurs+yoffset].designation)
#define	SADJDES2	fprintf(fexe,"S_ADES\t%d\t%hd\t0\t%d\t%d\t%c\n",XSADES,country,x,y,sct[x][y].designation)
#define	SADJCIV2 fprintf(fexe,"S_ACIV\t%d\t%hd\t%ld\t%d\t%d\t%s\n",XSACIV,country,sct[i][j].people,i,j,"null")
#define	SADJCIV	fprintf(fexe,"S_ACIV\t%d\t%hd\t%ld\t%d\t%d\t%s\n",XSACIV,country,sct[xcurs+xoffset][ycurs+yoffset].people,xcurs+xoffset,ycurs+yoffset,"null")
#define	SADJCIV3 fprintf(fexe,"S_ACIV3\t%d\t%hd\t%ld\t%d\t%d\t%s\n",XSACIV3,country,people_to_add,i,j,"null")
#define	INCFORT fprintf(fexe,"SIFORT\t%d\t%hd\t0\t%d\t%d\t%s\n",XSIFORT,country,xcurs+xoffset,ycurs+yoffset,"null")
#define	SADJOWN	fprintf(fexe,"S_AOWN\t%d\t%hd\t0\t%d\t%d\t%s\n",XSAOWN,country,xcurs+xoffset,ycurs+yoffset,"null")
#define	EADJDIP(a,b)	fprintf(fexe,"E_ADJ\t%d\t%hd\t%d\t%d\t0\t%s\n",EDADJ,a,b,ntn[a].dstatus[b],"null")
#define	EDECSPL	fprintf(fexe,"E_SPL\t%d\t%hd\t%d\t%d\t0\t%s\n",EDSPL,country,s_cost,0,"null")
#define	NADJNTN  fprintf(fexe,"N_TAX \t%d \t%hd \t%d \t%d \t%d \tnull\n",NTAX,country,(int) curntn->tax_rate,(int) curntn->active, (int) curntn->charity)
#define	NADJNTN2  fprintf(fexe,"N_POP \t%d \t%hd \t%d \t%d \t%d \tnull\n",NPOP,country,(int) curntn->popularity,(int) curntn->terror, (int) curntn->reputation)

#define	S_MIL		0		/* position in powers array	*/
#define	WARRIOR		0x00000001L
#define	CAPTAIN		0x00000002L
#define	WARLORD		0x00000004L
#define	ARCHER		0x00000008L
#define	CAVALRY		0x00000010L
#define	SAPPER		0x00000020L
#define	ARMOR		0x00000040L
#define	AVIAN		0x00000080L
#define	MI_MONST	0x00000100L
#define	AV_MONST	0x00000200L
#define	MA_MONST	0x00000400L
#define	E_MIL		11
		/*CIVILIAN POWERS	*/
#define	S_CIV		11
#define	SLAVER		0x00000800L
#define	DERVISH		0x00001000L
#define	HIDDEN		0x00002000L
#define	ARCHITECT	0x00004000L
#define	RELIGION	0x00008000L
#define	MINER		0x00010000L
#define	BREEDER		0x00020000L
#define	URBAN		0x00040000L
#define	STEEL		0x00080000L
#define	NINJA		0x00100000L
#define	SAILOR		0x00200000L
#define	DEMOCRACY	0x00400000L
#define	ROADS		0x00800000L
#define	E_CIV		13
			/* MAGICAL SKILLS	*/
#define	S_MGK		24
#define	THE_VOID	0x01000000L
#define	KNOWALL		0x02000000L
#define	DESTROYER	0x04000000L
#define	VAMPIRE		0x08000000L
#define	SUMMON		0x10000000L
#define	WYZARD		0x20000000L
#define	SORCERER	0x40000000L
#define	E_MGK		7
#define	MAXPOWER	31 /* number of powers	*/

/* magic macro: returns TRUE if the nation has that power*/
#define	magic(NATION,POWER)	((ntn[NATION].powers&(POWER))!=0)
#define	ONMAP(x,y)	((x)>=0 && (y)>=0 && (x)<MAPX && (y)<MAPY)

#ifndef HILIGHT
#define	standout()
#endif /* HILIGHT */

#ifdef BEEP
#define	beep()		putc('\007',stderr)
#else
#define	beep()
#endif

#ifdef SYSV
extern	long		lrand48(void);
#define	rand()		lrand48()
#define	srand(x)	srand48(x)
#endif

#ifdef	BSD
#define	rand()		random()
#define	srand(x)	srandom(x)
#endif

#ifndef DEBUG
#define	check()	;
#else
#define	check()	checkout(__FILE__,__LINE__)
#endif /* DEBUG */

#define	GOLDTHRESH	10L	/* min ratio of gold:jewels */

#define	SALT "aa"		/* seed for crypt() encryption	*/

/*
 * =============================================================================
 * FUNCTION PROTOTYPES - LEGACY K&R STYLE (REQUIRES MODERNIZATION)
 * =============================================================================
 *
 * This section contains function prototypes for the entire Conquer game system.
 * These are legacy K&R style prototypes that will need to be modernized to
 * ANSI C standards during Phase 8 (Syntactic and Mechanical Modernization).
 *
 * Function Categories:
 * - Game Core: main(), parse(), execute()
 * - World Management: createworld(), makeworld(), readmap()
 * - Nation Management: nationrun(), get_country(), verify_ntn()
 * - Military: combat(), armymove(), navygoto(), fight()
 * - Magic System: domagic(), getmagic(), removemgk()
 * - Economics: budget(), produce(), trade()
 * - Display: makeside(), makemap(), newdisplay()
 * - File I/O: readdata(), writedata(), mailopen()
 * - Utility: score(), random functions, mathematical calculations
 *
 * Modernization Notes:
 * - Many functions have inconsistent or missing parameter specifications
 * - Return types often omitted (implicitly int)
 * - Some prototypes duplicated (e.g., getmagic() appears twice)
 * - Conditional compilation affects some function availability
 * - Memory allocation functions use non-standard naming (**m2alloc)
 */

/* extern all subroutine calls	*/
extern long	getmagic(void), getmagic(void), getmgkcost(void), score_one(void);
extern long	get_number(void), solds_in_sector(void),defaultunit(void);

extern int	move_file(void), land_2reachp(void), land_reachp(void), canbeseen(void);
extern int	water_reachp(void), markok(void), is_habitable(void), parse(void);
extern int	units_in_sector(void), num_powers(void), tofood(void), mailopen(void);
extern int	get_god(void), flightcost(void), todigit(void), getclass(void), startcost(void);
extern int	water_2reachp(void),tg_ok(void), readmap(void), avian(void);
extern int	cbonus(void), armymove(void),takeover(void),getnewname(void);
extern int getleader(void);
extern int execute(void);
extern void peasant_revolt(int *newnation);
extern int other_revolt(int *new);
extern int	aretheyon(void),armygoto(void),navygoto(void),getselunit(void);
extern int	unitvalid(void),orctake(void),fort_val(void);
extern int	conquer_access(void);  /* renamed to avoid conflict with system access() */
extern int	addgships(void),addmships(void),addwships(void),fltships(void);
extern int	fltghold(void),fltwhold(void),fltmhold(void),flthold(void),compass(void);
extern int	get_country(void),check_lock(void),doclass(void),get_pass(void);
extern unsigned short	fltspeed(void);
extern void	do_pirate(void), do_nomad(void), do_savage(void), do_lizard(void);
extern void	getjewel(void),getmetal(void),loadfleet(void),removemgk(void),exenewmgk(void);
extern struct	s_sector *rand_sector(void);
extern void	subgships(void),submships(void),subwships(void),getspace(void),sackem(void);
extern void	whatcansee(void), reset_god(void), get_nname(void), camp_info(void);
extern int	main(void);
extern void	makebottom(void), makeside(void), check_mail(void), centermap(void);
extern void	checkout(void),copyscreen(void),bye(void),credits(void),init_hasseen(void);
extern void	combinearmies(void),change_status(void),reducearmy(void),splitarmy(void);
extern void	errormsg(void), clear_bottom(void), addgroup(void),ext_cmd(void);
extern void	randomevent(void), wdisaster(void), weather(void), deplete(void);
extern void	verify_ntn(void), verify_sct(void), verifydata(void), prep(void);
extern void	errorbar(void), newbye(void), newreset(void), newmsg(void), newerror(void);
extern void	newinit(void), jump_to(void);
extern void	destroy(void), updmove(void), spreadsheet(void), mailclose(void);
extern void	updexecs(void), updcapture(void), updsectors(void);
extern void	updmil(void), updcomodities(void), updleader(void);
extern void	nationrun(void), n_atpeace(void), n_trespass(void), n_people(void);
extern void	n_toofar(void), n_unowned(void), pceattr(void), checkout(void);
extern void	fdxyretreat(void), retreat(void), rawmaterials(void), createworld(void);
extern void	att_setup(void), att_base(void), att_bonus(void);
extern void	adjarm(void),armyrpt(void),atkattr(void),blowup(void);
extern void	budget(void),change(void),cheat(void),coffmap(void),combat(void),construct(void);
extern void	defattr(void),diploscrn(void),domagic(void),draft(void),erupt(void);
extern void	fight(void);
extern void	fill_edge(void),flee(void),fleetrpt(void),hangup(void),help(void);
extern void	highlight(int x, int y, short hmode);
extern void	makemap(void);
extern void	makeside(void);
extern void	makeworld(void),monster(void),moveciv(void);
extern void	mymove(void),navalcbt(void),newdip(void),newdisplay(void),newlogin(void);
extern void	newspaper(void),npcredes(void),offmap(void),place(void),populate(void);
extern void	printele(void),printnat(void),printscore(void),printveg(void);
extern void	pr_ntns(void),pr_desg(void),produce(void);
extern void	readdata(void),redesignate(void),redomil(void),reduce(void),rmessage(void),score(void);
extern void	see(void),showscore(void),update(void);
extern void	wmessage(void),writedata(void),getdstatus(void);
/* exit() declaration removed - conflicts with system exit(int) */
extern void	wizardry(void);
extern	char	**m2alloc(void);
/* crypt() declaration removed - conflicts with system crypt(const char*, const char*) */
#ifdef SPEW
extern void makemess(int n, FILE *fp);
#endif
#ifdef TRADE
void trade(void),uptrade(void),checktrade(void);
#endif /* TRADE */

#define	HI_OWN		0	/* hilight modes	*/
#define	HI_ARMY		1
#define	HI_NONE		2
#define	HI_YARM		3
#define	HI_MOVE		4
#define HI_GOOD		5

#define	DI_VEGE		1	/* display modes	*/
#define	DI_DESI		2
#define	DI_CONT		3
#define	DI_FOOD		4
#define	DI_NATI		5
#define	DI_RACE		6
#define	DI_MOVE		7
#define	DI_DEFE		8
#define	DI_PEOP		9
#define	DI_GOLD		10
#define	DI_METAL	11
#define	DI_ITEMS	12

extern	struct sprd_sht
{
	long food,gold,jewels,metal; 		/* total @ end of turn	*/
	long revfood,revjewels,revmetal,
	     revcap,revcity,revothr; 		/* revenue in turn	*/
	long ingold,inmetal,infarm,incity,incap,inothr; 
						/* civilians in area	*/
	long civilians;				/* total civilians	*/
	int sectors;
} spread;

#ifndef max
#define	max(x,y)	((x)>(y) ? (x) : (y))
#define	min(x,y)	((x)<(y) ? (x) : (y))
#endif

/* attractiveness of sector to general populace	*/
#define	GOLDATTR	9	/* per jewel value of sector */
#define	FARMATTR	7	/* per point of food producable */
#define	MINEATTR	9	/* per metal value of sector */
#define	TOWNATTR	150
#define	CITYATTR 	300
#define TGATTR		10	/* bonus per point of value for tradegoods */
#define OTHRATTR	50	/* attractiveness of other sector types */


#define	DMNTNATTR	40	/* DWARF ATTRACTIVENESS	*/
#define	DHILLATTR	20
#define	DCLERATTR	0
#define	DCITYATTR	-20
#define	DTOWNATTR	-20
#define	DGOLDATTR	40
#define	DMINEATTR	40
#define	DFOREATTR	-20
#define	DWOODATTR	-10

#define	EMNTNATTR	-40	/* ELF	*/
#define	EHILLATTR	-20
#define	ECLERATTR	0
#define	ECITYATTR	-50
#define	ETOWNATTR	-50
#define	EGOLDATTR	0
#define	EMINEATTR	0
#define	EFOREATTR	40
#define	EWOODATTR	40

#define	OMNTNATTR	30	/* ORC	*/
#define	OHILLATTR	20
#define	OCLERATTR	0
#define	OCITYATTR	50
#define	OTOWNATTR	25
#define	OGOLDATTR	20
#define	OMINEATTR	20
#define	OFOREATTR	-40
#define	OWOODATTR	-20

#define	HMNTNATTR	-10	/* HUMAN	*/
#define	HHILLATTR	0
#define	HCLERATTR	30
#define	HCITYATTR	50
#define	HTOWNATTR	40
#define	HGOLDATTR	10
#define	HMINEATTR	10
#define	HFOREATTR	-20
#define	HWOODATTR	0

/* EXOTIC TRADE GOODS	*/
#define	TG_furs		0
#define	TG_wool		1
#define	TG_beer		2
#define	TG_cloth	3
#define	TG_wine		4
#define	END_POPULARITY	4

#define	TG_mules	5
#define	TG_horses	6
#define	TG_pigeons	7
#define	TG_griffons	8
#define	END_COMMUNICATION	8

#define	TG_corn		9
#define	TG_fish		10
#define	TG_sugar	11
#define	TG_honey	12
#define	TG_fruit	13
#define	TG_rice		14
#define	TG_wheat	15
#define	TG_dairy	16
#define	TG_peas		17
#define	TG_bread	18
#define	TG_cereal	19
#define	END_EATRATE	19

#define	TG_pottery	20
#define	TG_salt		21
#define	TG_timber	22
#define	TG_granite	23
#define	TG_pine		24
#define	TG_oak		25
#define	TG_nails	26
#define	END_SPOILRATE	26	/* also Knowledge start */

#define	TG_papyrus	27
#define	TG_math		28
#define	TG_library	29
#define	TG_drama	30
#define	TG_paper	31
#define	TG_literature	32
#define	TG_law		33
#define	TG_philosophy	34
#define	END_KNOWLEDGE	34

#define	TG_irregation	35
#define	TG_oxen		36
#define	TG_plows	37
#define	END_FARM	37	

#define	TG_stones	38
#define	END_SPELL	38

#define	TG_herbs	39
#define	TG_medecine	40
#define	END_HEALTH	40

#define	TG_torture	41
#define	TG_prison	42
#define	END_TERROR	42	
#define	END_NORMAL	42	/* also end of non mine/jewel goods */

#define	TG_bronze	43
#define	TG_copper	44
#define	TG_lead		45
#define	TG_tin		46
#define	TG_iron		47
#define	TG_steel	48
#define	TG_mithral	49
#define	TG_adamantine	50
#define	END_MINE	50

#define	TG_spice	51
#define	TG_silver	52
#define	TG_pearls	53
#define	TG_dye		54
#define	TG_silk		55
#define	TG_gold		56
#define	TG_rubys	57
#define	TG_ivory	58
#define	TG_diamonds	59
#define	TG_platinum	60
#define	END_WEALTH	60
#define	TG_none		61	/* no trade goods in sector	*/
#define MAXTGVAL	100	/* maximum value for stat */

/* defines for a nations class	*/
#define	C_NPC		0
#define	C_KING		1
#define	C_EMPEROR	2
#define	C_WIZARD	3
#define	C_PRIEST	4
#define	C_PIRATE	5
#define	C_TRADER	6
#define	C_WARLORD	7
#define	C_DEMON		8
#define	C_DRAGON	9
#define	C_SHADOW	10
#define	C_END		10

#define	ISCITY(desig)	((desig==DCITY)||(desig==DCAPITOL)||(desig==DFORT)||(desig==DTOWN))

#define	PWR_NA	10	/* national attributes gained from power	*/
#define	CLA_NA	30	/* national attributes gained from class	*/

/* MAGIC/CIVILIAN/MILITARY POWER COSTS BY RACE	*/
#define	BASEMAGIC	50000L	/* default for all not mentioned	*/
#define	DWFMAGIC	80000L
#define	HUMMAGIC	100000L
#define	ORCMAGIC	150000L
#define	DWFCIVIL	40000L
#define	ORCCIVIL	75000L
#define	HUMCIVIL	25000L
#define	DWFMILIT	40000L
#define	ORCMILIT	45000L

#define	abrt() { \
fprintf(stderr,"\nSerious Error (File %s, Line %d) - Aborting\n",__FILE__,__LINE__); \
abort(); \
}

#define	DEVASTATE(X,Y) { \
if(is_habitable(X,Y)) { \
if((sct[X][Y].designation == DCAPITOL) \
||(sct[X][Y].designation == DRUIN) \
||(sct[X][Y].designation == DCITY)) { \
	if(sct[X][Y].fortress >= 4) { \
		sct[X][Y].fortress -= 4; \
	} else sct[X][Y].fortress = 0; \
	sct[X][Y].designation = DRUIN; \
} else sct[X][Y].designation = DDEVASTATED; \
} \
}

extern	FILE	*fm;
extern	int	mailok;
extern	char	*seasonstr[];
extern	char	*alignment[];	
extern	struct	s_sector	**sct;
extern	struct	s_nation	ntn[NTOTAL];
extern	struct	s_nation	*curntn;	
extern	struct	s_world		world;

extern	short	**movecost;
extern	char	**occ;		/* sector occupied?, NTOTAL+1 if contested */
extern	char	*ele, *elename[], *veg,	*vegfood, *vegname[];
extern	char	*Class[],*races[],*diploname[];
extern	char	*soldname[], *unittype[], *shunittype[], *directions[];
extern	int	unitminsth[], u_enmetal[], u_encost[], unitmaint[];
extern	char	*des, *desname[], *pwrname[];
extern	long	powers[];
extern	char	*tg_value, *tg_name[], *tg_stype;	/* trade goods	*/

#ifdef CONQUER
extern	long	mercgot;
#endif /* CONQUER */

#ifdef ADMIN
extern	char	*npcsfile;
extern	char	scenario[];
#endif /* ADMIN */
extern	int	unitmove[], unitattack[], unitdefend[];

/*	file name definitions	*/
extern	char *sortname,*exefile,*datafile,*msgfile;
extern	char *helpfile,*newsfile,*isonfile,*timefile;

#ifdef CONQUER
extern	int	conq_mail_status;
extern	char	conqmail[];
#ifdef SYSMAIL
extern	int	sys_mail_status;
extern	char	sysmail[];
#endif /* SYSMAIL */
#endif /* CONQUER */
#ifdef TRADE
extern	char	*tradefile;
#endif /* TRADE */

#define EXT_CMD '\033'		/* ESC to start extended command */

#define MAXX		(MAPX / 8)	/* Number of X areas on map */
#define MAXY		(MAPY / 8)	/* Number of Y areas on map */
#define NUMAREAS	(MAXX * MAXY)	/* total # areas, MAXX*MAXY */
#define NUMSECTS	(MAPX * MAPY)	/* total # areas, MAXX*MAXY */
#define MAXHELP		6

#ifdef HPUX
#define SYSV
#endif /* HPUX */

/* minor market items */
#define GETFOOD		97		/* response needed to get food */
#define GETMETAL	98
#define GETJEWL		99
#define GODFOOD		8000L		/* food recieved for GODPRICE */
#define GODMETAL	2000L		/* " */
#define GODJEWL		3000L		/* " */
#define GODPRICE	25000L

#ifdef XYZ
/* THE FOLLOWING DEFINES ARE NOT IMPLEMENTED YET		*/
/* THEY DEFINE THE ATTRIBUTES OF A (TO BE IMPLEMENTED) REGION	*/

/* REGIONS REVOLT STATUS:	if larger than the value	*/
#define	R_OPEN_REVOLT	220
#define R_GUERILLA_WAR	170
#define R_UNREST	150
#define R_DISSENT	120
#define R_CALM		80
#define R_HAPPY		50
#define R_PROSPEROUS	0

/* REGIONS GOVERNMENTAL STRENGTH	*/
#define R_BARBARIAN	0
#define R_IND_VILAGES	20
#define R_TRIBAL	40
#define R_CITY_STATES	80
#define R_BALKANIZED	110
#define R_CONFEDERACY	140
#define R_NATION	160
#define R_MONARCHY	180
#define R_EMPIRE	200

/* REGION STRATEGY: region strategy by owning nation	*/
#define R_ISOLATION	37
#define R_BUILD_ECON	36
#define R_EXPAND_PCE	35
#define R_TAX		34
#define R_BUILD_MIL	33
#define R_EXPAND_MIL	32
#define R_AT_WAR	31
#define R_SUBDUE	20	/* 20-30: # of turns left in unrest	*/
#define R_CIVIL_WAR	0	/* 0-19: number of turns left in war	*/

#define	NUMREGIONS	250	/* number of regions in the world	*/
#define	STARTYEAR	-10000	/* year that game treats as 0.  In this	
					case - ten thousand years B.C	*/

struct s_region
{
	char	*name;			/* name of region	*/
	long	people;			/* civilians in sector*/
	long	tax;			/* tax production ability*/
	unsigned char owner;		/* nation id of owner	*/
	unsigned char color;		/* color to display region*/
	unsigned char tech_level;	/* technology level	*/
	unsigned char law_level;	/* law level of region	*/
	unsigned char social_level;	/* social level of region*/
	unsigned char govt_sth;	/* owners sth. in region	*/
	unsigned char strategy;	/* owners strategy in region	*/
	unsigned char revolt;	/* status of region	*/
	unsigned char prestige;	/* prestige for owning	*/
};
#endif /* 0 */
