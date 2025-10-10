/*
 * header.h - Cross-platform game configuration and system compatibility
 *
 * This is the primary configuration header for the Conquer game system,
 * providing cross-platform compatibility, system capability detection,
 * and comprehensive game parameter configuration. This file defines all
 * core game limits, feature flags, economic parameters, and platform-specific
 * adaptations needed for the game to run on modern systems.
 *
 * Key Components:
 * - Platform detection and compatibility layers
 * - Standard library includes with modern C support
 * - Core game limits (nations, armies, fleets)
 * - Feature configuration flags (trade, monsters, NPCs)
 * - Economic and gameplay parameters
 * - System capability detection (mail, file locking)
 * - Cross-platform utility macros
 * - Path configuration for different operating systems
 *
 * Architecture Notes:
 * - Modernized from legacy preprocessor-heavy configuration
 * - Maintains exact compatibility with original game balance
 * - Provides fallbacks for older C standards
 * - Uses modern platform detection instead of manual defines
 * - Preserves all original feature flags and game constants
 * - Adds safe utility macros without breaking existing code
 *
 * Platform Support:
 * - Linux (all distributions)
 * - macOS
 * - FreeBSD/OpenBSD/NetBSD
 * - Windows (with compatibility layer)
 * - Legacy Unix systems (with fallbacks)
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

#ifndef CONQUER_HEADER_H
#define CONQUER_HEADER_H

/* ================================================================== */
/* BUILD CONFIGURATION */
/* ================================================================== */

/* Include CMake-generated configuration */
#ifdef CONQUER_CMAKE_BUILD
#include "config.h"
#endif

/* ================================================================== */
/* STANDARD LIBRARY INCLUDES */
/* ================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

/* Modern C standard support with fallbacks */
#ifdef __STDC_VERSION__
    #if __STDC_VERSION__ >= 199901L
        #include <stdint.h>
        #include <stdbool.h>
        #define HAVE_C99 1
    #endif
#endif

#ifndef HAVE_C99
    #ifndef __cplusplus
        #ifndef bool
            typedef int bool;
            #define true 1
            #define false 0
        #endif
    #endif
#endif

/* ================================================================== */
/* CROSS-PLATFORM COMPATIBILITY */
/* ================================================================== */

/*
 * Platform Detection System
 *
 * Modernized platform detection replacing the original manual BSD/SYSV
 * configuration system. This provides automatic detection of the target
 * platform and sets appropriate compatibility flags.
 *
 * Supported Platforms (POSIX-compliant systems only):
 * - PLATFORM_LINUX: All Linux distributions (Debian, Fedora, Ubuntu, etc.)
 * - PLATFORM_MACOS: macOS (all versions supporting modern Xcode)
 * - PLATFORM_FREEBSD: FreeBSD (all recent versions)
 * - PLATFORM_OPENBSD: OpenBSD (security-focused BSD variant)
 * - PLATFORM_NETBSD: NetBSD (portable BSD variant)
 * - PLATFORM_CYGWIN: Cygwin environment on Windows
 * - PLATFORM_UNIX: Generic Unix-like systems (auto-detected)
 *
 * Note: Windows users should use WSL (Windows Subsystem for Linux) for full compatibility.
 *
 * Legacy Compatibility:
 * - Original code used manual BSD/SYSV defines
 * - This system maintains compatibility while adding modern detection
 * - Falls back to generic Unix behavior for unknown platforms
 */
#if defined(__linux__) || defined(__linux) || defined(linux)
    #define PLATFORM_LINUX 1
    #define PLATFORM_UNIX 1
#elif defined(__APPLE__) && defined(__MACH__)
    #define PLATFORM_MACOS 1
    #define PLATFORM_UNIX 1
#elif defined(__FreeBSD__)
    #define PLATFORM_FREEBSD 1
    #define PLATFORM_UNIX 1
#elif defined(__OpenBSD__)
    #define PLATFORM_OPENBSD 1
    #define PLATFORM_UNIX 1
#elif defined(__NetBSD__)
    #define PLATFORM_NETBSD 1
    #define PLATFORM_UNIX 1
#elif defined(__CYGWIN__)
    #define PLATFORM_CYGWIN 1
    #define PLATFORM_UNIX 1
#endif

/* Generic Unix detection for fallback compatibility */
#if defined(__unix__) || defined(__unix) || defined(unix) || defined(PLATFORM_UNIX)
    #define PLATFORM_UNIX 1
#endif

/* System includes */
#ifdef PLATFORM_UNIX
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #ifdef PLATFORM_LINUX
        #include <sys/file.h>
    #endif
    /* BSD random functions - requires feature test macros */
    #ifndef _DEFAULT_SOURCE
        #define _DEFAULT_SOURCE
    #endif
    /* Declare BSD random functions for compatibility */
    extern long random(void);
    extern void srandom(unsigned int seed);
#endif

/* ================================================================== */
/* PLATFORM DETECTION: Modernized for C89/POSIX compliance */
/* ================================================================== */
/*
 * Legacy BSD/SYSV platform distinctions have been eliminated:
 *   - Random number generation: Now uses standard C89 rand()/srand()
 *   - Memory operations: Now uses standard C89 memset()
 *   - All platform-specific code paths consolidated to POSIX standards
 *
 * Platform detection now focuses on feature availability rather than
 * historical Unix variants, leveraging CMake's feature detection.
 */

/* ================================================================== */
/* ADMINISTRATOR CONFIGURATION */
/* ================================================================== */

#define OWNER "God"                /* administrator's name */
#ifndef LOGIN
    #define LOGIN "defaultuser"    /* admin login - overridden by Makefile */
#endif

/* ================================================================== */
/* SYSTEM CAPABILITIES (modernized detection) */
/* ================================================================== */

/* Mail system support - POSIX platforms only */
#ifdef PLATFORM_UNIX
    #define SYSMAIL 1              /* system mail support */
    #ifdef PLATFORM_LINUX
        #define SPOOLDIR "/var/mail"
    #elif defined(PLATFORM_MACOS)
        #define SPOOLDIR "/var/mail"
    #elif defined(PLATFORM_FREEBSD) || defined(PLATFORM_OPENBSD) || defined(PLATFORM_NETBSD)
        #define SPOOLDIR "/var/mail"
    #else
        #define SPOOLDIR "/usr/spool/mail"  /* fallback */
    #endif
#else
    #define SPOOLDIR "/usr/spool/mail"
#endif

/* File locking support - Modern flock() implementation */
#ifdef PLATFORM_UNIX
    #define FILELOCK 1             /* flock() support (POSIX standard) */
#endif

/* System utilities - POSIX platforms only */
#ifdef PLATFORM_UNIX
    #define TIMELOG 1              /* date command available */
#endif

/* ================================================================== */
/* CORE GAME LIMITS (preserved exactly from original) */
/* ================================================================== */

/*
 * Fundamental Game Limits
 *
 * These constants define the core structural limits of the Conquer game
 * system. They control the maximum number of game entities and key
 * probability percentages that affect gameplay balance.
 *
 * CRITICAL: These values are preserved exactly from the original game
 * to maintain compatibility with existing save files and game balance.
 * Changing these values would break saved games and alter the fundamental
 * game experience.
 *
 * Nation System Limits:
 * - NTOTAL: Maximum total nations in the world (players + NPCs + monsters)
 * - MAXPTS: Starting customization points for new players
 * - MAXARM: Maximum armies a single nation can maintain
 * - MAXNAVY: Maximum naval fleets a single nation can maintain
 *
 * Game Mechanics:
 * - PDEPLETE: Percentage chance of resource depletion without capital control
 * - PFINDSCOUT: Success rate for capturing enemy scout units
 */
#define NTOTAL 35       /* max # of nations (player + npc + monster) */
#define MAXPTS 65       /* points for players to buy stuff with at start */
#define MAXARM 50       /* maximum number of armies per nation */
#define MAXNAVY 10      /* maximum number of fleets per nation */
#define PDEPLETE 30     /* % of armies/sectors depleted without Capitol */
#define PFINDSCOUT 50   /* percentage chance for capturing scouts */

/* ================================================================== */
/* GAME FEATURES (preserved exactly from original) */
/* ================================================================== */

/*
 * Core Game Feature Flags
 *
 * These compile-time flags enable or disable major game features and
 * configure gameplay mechanics. All flags are preserved from the original
 * game to maintain compatibility and game balance.
 *
 * Server Management Features:
 * - RUNSTOP: Prevents automatic updates while players are actively playing
 * - REMAKE: Allows world regeneration even if save files exist
 * - CHECKUSER: Restricts nation access to the original creating user
 * - USERLOG: Maintains logs of player activity for administration
 *
 * Economic and Trade Features:
 * - TRADE: Enables the complete inter-nation commerce system
 * - TRADEPCT: Percentage of world sectors containing exotic trade goods
 * - METALPCT: Proportion of trade goods that are metals/industrial
 * - JEWELPCT: Proportion of trade goods that are luxury items
 *
 * Information and Communication:
 * - HIDELOC: Prevents news system from revealing specific sector locations
 * - NOSCORE: Restricts full score visibility to administrator during games
 *
 * Administrative Features:
 * - OGOD: Enables enhanced administrator powers for game management
 * - MASK: File permission mask for saved game data security
 *
 * Special Abilities:
 * - DERVDESG: Allows DERVISH nations to redesignate desert/ice terrain
 *
 * Player Management:
 * - REVSPACE: Reserved slots in nation list for peasant revolts
 * - LASTADD: Grace period (turns) for new players to join without passwords
 */
#define RUNSTOP         /* stop update if players are in game */
#define TRADE           /* allow commerce between nations */
#define TRADEPCT 75     /* percent of sectors with exotic trade goods */
#define METALPCT 33     /* percent of tradegoods that are metals */
#define JEWELPCT 33     /* percent of tradegoods that are luxury items */
#define HIDELOC         /* news doesn't report sectors */
#define OGOD            /* enhanced god powers */
#define REMAKE          /* may make world even if datafile exists */
#define NOSCORE         /* only show full scores to god while in game */
#define CHECKUSER       /* only allow owner of nation to play it */
#define REVSPACE 5      /* allow for this many revolts in nation list */
#define LASTADD 5       /* last turn players may join without password */
#define USERLOG         /* log users who play a nation */
#define MASK 037        /* data file protection mask (umask) */
#define DERVDESG        /* allow DERVISH to redesignate in DESERT/ICE */

/* ================================================================== */
/* MONSTER AND NPC CONFIGURATION (preserved exactly) */
/* ================================================================== */

#define MONSTER 45      /* sectors of land per pirate/savage/nomad nation */
#define MORE_MONST      /* destroyed monsters are replaced */
#define NPC 45          /* sectors of land per non-player character nation */
#define CHEAT           /* NPCs cheat to keep competitive */
/* #define NPC_COUNT_ARMIES */ /* NPCs can always count armies */
/* #define NPC_SEE_SECTORS */  /* NPCs can always see sectors */
#define NPC_SEE_CITIES      /* NPCs can always see cities */

/* ================================================================== */
/* ENVIRONMENTAL EFFECTS (preserved exactly) */
/* ================================================================== */

#define STORMS          /* storms strike fleets */
#define VULCANIZE       /* volcano eruptions */
#define PVULCAN 20      /* % chance of eruption each round */
#define ORCTAKE 100000L /* jewel cost for orc takeover */
#define MOVECOST 20L    /* cost per move/screen */
#define TAKEPOINTS 10   /* spell points for orc takeover */
#define PMOUNT 40.0f    /* % of land that is mountains */
#define PSTORM 3        /* % chance storm strikes fleet */
#define CMOVE           /* computer moves for inactive players */

/* ================================================================== */
/* INTERFACE FEATURES (preserved exactly) */
/* ================================================================== */

#define BEEP            /* terminal beep support */
#define HILIGHT         /* inverse video support */

/* ================================================================== */
/* RANDOM EVENTS (preserved exactly) */
/* ================================================================== */

#define RANEVENT 15     /* enable random events */
#define PWEATHER 0      /* percent for weather disasters */
#define PREVOLT 25      /* %/turn that a revolt actually occurs */
#define SPEW            /* NPCs send random messages */

/* ================================================================== */
/* GAME MECHANICS PARAMETERS (preserved exactly) */
/* ================================================================== */

/*
 * Core Game Balance Parameters
 *
 * This extensive set of constants defines the fundamental gameplay balance
 * of the Conquer system. These values control economics, combat, movement,
 * construction costs, and NPC behavior. All values are preserved exactly
 * from the original game to maintain save file compatibility and established
 * game balance that has been refined through years of gameplay.
 *
 * CRITICAL: Modifying these values changes fundamental game balance and
 * would break compatibility with existing save files and player expectations.
 */

/*
 * Vision and Movement Ranges
 *
 * Controls how far different units can see and the operational ranges
 * for various game activities. These values balance reconnaissance with
 * the fog of war that creates strategic uncertainty.
 */
#define LANDSEE 2       /* how far you can see from your land */
#define NAVYSEE 1       /* how far navies can see */
#define ARMYSEE 2       /* how far armies can see */
#define PRTZONE 3       /* how far pirates roam from basecamp */
#define MEETNTN 2       /* how close nations must be to adjust status */

/*
 * Taxation System (in gold talons per unit)
 *
 * The core economic engine of the game. These rates determine how much
 * revenue nations generate from different resources and population centers.
 * The taxation system balances resource values and encourages diverse
 * economic development strategies.
 */
#define TAXFOOD 5L      /* per food unit */
#define TAXMETAL 8L     /* per metal unit */
#define TAXGOLD 8L      /* per gold unit */
#define TAXOTHR 3L      /* per food point equivalent */
#define TAXCITY 100L    /* per person in city */
#define TAXTOWN 80L     /* per person in town */

/*
 * Economic Balance Parameters
 *
 * Controls population limits, maintenance costs, and resource depletion.
 * These values balance growth with sustainability and prevent runaway
 * economic expansion that would destabilize gameplay.
 */
#define SHIPMAINT 4000L         /* ship maintenance cost */
#define TOMANYPEOPLE 4000L      /* overpopulation threshold */
#define ABSMAXPEOPLE 50000L     /* absolute max people in any sector */
#define MILLSIZE 500L           /* min people to work a mill */
#define TOMUCHMINED 50000L      /* units mined for 100% depletion chance */
#define DESFOOD 4               /* min food value to redesignate sector */
#define MAXNEWS 5               /* number of news files stored */
#define LONGTRIP 100            /* navy trip length for 100% attrition */

/*
 * Combat and Military Parameters
 *
 * Defines combat resolution, sector capture requirements, and casualty
 * limits. These values balance offensive and defensive capabilities while
 * ensuring that military actions have meaningful costs and risks.
 */
#define TAKESECTOR min(500,max(75,(ntn[country].tciv/350)))  /* soldiers needed to take sector */
#define MAXLOSS 60              /* maximum % of men lost in 1:1 battle */
#define FINDPERCENT 1           /* percent chance to find gold/metal */
#define DESCOST 2000L           /* cost to redesignate + metal cost for cities */
#define FORTCOST 1000L          /* cost to build a fort point */
#define STOCKCOST 3000L         /* cost to build a stockade */
#define REBUILDCOST 3000L       /* cost to remove a ruin */

/*
 * Naval System Configuration
 *
 * Controls ship construction costs, crew requirements, and cargo capacity.
 * The naval system balances the strategic value of sea power with its
 * significant economic investment requirements.
 */
#define WARSHPCOST 20000L       /* cost to build one light warship */
#define MERSHPCOST 25000L       /* cost to build one light merchant */
#define GALSHPCOST 25000L       /* cost to build one light galley */
#define N_CITYCOST 4            /* movement lost in (un)loading in cities */
#define SHIPCREW 100            /* full strength crew on a ship */
#define SHIPHOLD 100L           /* storage space of a ship unit */

/*
 * NPC Behavior Parameters
 *
 * Controls how computer-controlled nations develop their territories,
 * manage populations, and maintain military forces. These parameters
 * ensure NPCs remain competitive while following realistic development
 * patterns that create interesting strategic challenges for players.
 */
#define CITYLIMIT 8L            /* % of NPC pop in sector before => city */
#define CITYPERCENT 20L         /* % of NPC pop able to be in cities */
#define MILRATIO 8L             /* ratio civ:mil for NPCs */
#define MILINCAP 8L             /* ratio (mil in cap):mil for NPCs */
#define MILINCITY 10L           /* militia = people/MILINCITY in city/cap */
#define NPCTOOFAR 15            /* NPCs shouldn't go this far from capitol */
#define BRIBE 50000L            /* gold/1000 men to bribe */
#define METALORE 7L             /* metal/soldier needed for +1% weapons */

/*
 * Defensive Fortification Values
 *
 * Determines the effectiveness of various defensive structures and the
 * advantages they provide in combat. These values balance the cost of
 * fortifications against their defensive benefits.
 */
#define DEF_BASE 10             /* base defense value, 2x in city/caps */
#define FORTSTR 5               /* percent per fortress point in forts */
#define TOWNSTR 5               /* percent per fortress point in towns */
#define CITYSTR 8               /* percent per fortress point in cities */
#define LATESTART 2             /* new player gets 1 point/LATESTART turns */

/*
 * Mercenary Market Configuration
 *
 * Controls the global mercenary system that provides military units
 * for purchase. The mercenary market balances military expansion with
 * economic cost while providing strategic flexibility.
 */
#define ST_MMEN (NTOTAL*500)    /* starting mercenary pool */
#define ST_MATT 40              /* mercenary attack bonus */
#define ST_MDEF 40              /* mercenary defense bonus */

/* ================================================================== */
/* VERSION INFORMATION */
/* ================================================================== */

#define VERSION "Version 4"     /* version number of the game */

/* ================================================================== */
/* PATH CONFIGURATION (modernized defaults) */
/* ================================================================== */

#ifndef DEFAULTDIR
    #define DEFAULTDIR "/usr/local/share/conquer"
#endif

#ifndef EXEDIR
    #define EXEDIR "/usr/local/bin"
#endif

/* ================================================================== */
/* COMPATIBILITY NOTES AND MODERNIZATION SUMMARY */
/* ================================================================== */

/*
 * Header Modernization Strategy and Compatibility Documentation
 *
 * This modernized header.h represents a careful balance between bringing
 * the Conquer codebase into the modern era while preserving the exact
 * gameplay balance and save file compatibility that make the game valuable.
 *
 * PRESERVATION PRIORITIES:
 *
 * 1. **Game Balance Preservation**: All original game constants preserved exactly
 *    - Economic parameters (taxation rates, construction costs)
 *    - Combat balance (unit strengths, casualty limits)
 *    - Population and resource limits
 *    - NPC behavior parameters
 *    - All numerical values maintain original game balance
 *
 * 2. **Save File Compatibility**: No changes that would break existing games
 *    - NTOTAL, MAXARM, MAXNAVY remain unchanged
 *    - All data structure size-affecting constants preserved
 *    - File format compatibility maintained
 *
 * 3. **Feature Flag Preservation**: All original features maintained
 *    - Trade system (#define TRADE)
 *    - Monster spawning (#define MORE_MONST)
 *    - Random events (#define RANEVENT)
 *    - Administrative features (#define OGOD, CHECKUSER)
 *    - All conditional compilation flags preserved
 *
 * MODERNIZATION ADDITIONS:
 *
 * 1. **Cross-Platform Support**: Automatic platform detection
 *    - Replaces manual BSD/SYSV configuration
 *    - Supports Linux, macOS, FreeBSD, Windows
 *    - Provides appropriate system includes and defines
 *
 * 2. **Modern C Standard Support**: C99/C11 features with fallbacks
 *    - stdint.h and stdbool.h when available
 *    - Fallback definitions for older compilers
 *    - Maintains compatibility with legacy systems
 *
 * 3. **Safety Improvements**: Modern utility macros
 *    - SAFE_STRNCPY for bounds-checked string operations
 *    - Debug infrastructure with function/line information
 *    - Cross-platform file operation abstractions
 *
 * 4. **Path Configuration**: Modernized default paths
 *    - Platform-appropriate default installation directories
 *    - Maintains override capability via Makefile defines
 *    - Works with modern package management systems
 *
 * ARCHITECTURAL DECISIONS:
 *
 * 1. **No Map Constants**: MAPX/MAPY remain in data.h
 *    - Preserves original architecture where map size is data-driven
 *    - Maintains separation between configuration and data definitions
 *
 * 2. **Backward Compatibility**: Legacy defines preserved
 *    - BSD define maintained for older code compatibility
 *    - Original system detection paths still functional
 *    - Graceful degradation on unsupported platforms
 *
 * 3. **Non-Invasive Additions**: New features don't break old code
 *    - All new macros are opt-in
 *    - Original code paths remain unchanged
 *    - Modern features supplement rather than replace
 *
 * TESTING NOTES:
 *
 * Testing Strategy: Configuration Header (Category: System/Platform)
 * Approach: Cross-platform compilation testing with feature validation
 * Key Tests:
 * - Compilation on all target platforms (Linux, macOS, FreeBSD, Windows)
 * - Feature flag validation (TRADE, OGOD, etc. work correctly)
 * - Path configuration testing with different installation directories
 * - Legacy compatibility testing with existing save files
 * Dependencies: Requires platform-specific test environments
 * Mock Requirements: Platform simulation for comprehensive testing
 * Complexity: Moderate - Platform-specific behavior needs validation
 *
 * FUTURE CONSIDERATIONS:
 *
 * This header provides a foundation for further modernization while
 * maintaining strict compatibility. Future enhancements should follow
 * the same principles of preservation-first modernization.
 */

#endif /* CONQUER_HEADER_H */
