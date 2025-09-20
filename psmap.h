/*
 * psmap.h - Configuration constants and definitions for PostScript map generation
 *
 * This header file provides the complete configuration system for the PostScript
 * map generation utility (psmap.c). It defines all constants, page layouts,
 * map types, and configuration parameters used throughout the PostScript
 * generation pipeline.
 *
 * CONFIGURATION ARCHITECTURE:
 * ===========================
 *
 * PAGE SIZE CONFIGURATION SYSTEM:
 * ┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
 * │ Compile-time    │───▶│ Default Page     │───▶│ Runtime Page    │
 * │ Defines         │    │ Selection        │    │ Configuration   │
 * │ (A4/LETTER/OTHER)│   │ (DEFAULTPAGE)    │    │ (setpagesize)   │
 * └─────────────────┘    └──────────────────┘    └─────────────────┘
 *
 * MAP TYPE CLASSIFICATION SYSTEM:
 * - SIMPLE: Basic character maps (default fallback)
 * - ALTITUDES: Elevation/topographical data representation
 * - DESIGNATIONS: Terrain type and classification maps
 * - NATIONS: Political boundaries and ownership maps
 * - VEGETATIONS: Ecosystem and land use visualization
 * - FORCED: User-specified simple output override
 *
 * PAPER SIZE SUPPORT MATRIX:
 * ┌────────────┬─────────────┬──────────────┬─────────────┬─────────────┐
 * │ Paper Type │ Width (pts) │ Height (pts) │ X Offset    │ Y Offset    │
 * ├────────────┼─────────────┼──────────────┼─────────────┼─────────────┤
 * │ A4         │ 540         │ 820          │ 30          │ 10          │
 * │ Letter     │ 575         │ 760          │ 15          │ 10          │
 * │ Other      │ 450         │ 700          │ 40          │ 30          │
 * └────────────┴─────────────┴──────────────┴─────────────┴─────────────┘
 *
 * MARGINS AND LAYOUT:
 * - XMARGINS: 30 pts horizontal margin reservation
 * - YMARGINS: 50 pts vertical margin reservation
 * - Margins ensure content fits within printable area
 * - Layout calculations use (page_size - margins) for content area
 *
 * VERSION AND COMPATIBILITY:
 * - VERSION: Program version string for PostScript output
 * - USAGE: Complete command-line help text
 * - MATCHSTRING: Conquer map file format identifier
 *
 * COMPILATION CONFIGURATION:
 * The header supports compile-time page size selection:
 * - Define A4: Sets A4 as default page size
 * - Define LETTER: Sets Letter as default page size
 * - Define OTHER: Sets custom page size as default
 * - No define: Falls back to runtime detection
 *
 * This file is part of Conquer.
 * Originally Copyright (C) 1989 by Martin Forssen (MaF)
 * Copyright (C) 2025 Juan Manuel Méndez Rey (Vejeta) - Licensed under GPL v3 with permission from original author
 *
 * Original author: Martin Forssen <d8forma@dtek.chalmers.se> (historical)
 * Permission granted by: Martin Forssen <maf@recordedfuture.com>
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
 * COMPILE-TIME PAGE SIZE CONFIGURATION
 * ====================================
 *
 * Conditional compilation system for default page size selection.
 * These defines allow the program to be compiled with a specific
 * default page size, which can still be overridden at runtime
 * through environment variables or command-line options.
 *
 * Usage during compilation:
 *   gcc -DA4 psmap.c      # Compiles with A4 as default
 *   gcc -DLETTER psmap.c  # Compiles with Letter as default
 *   gcc -DOTHER psmap.c   # Compiles with custom size as default
 *
 * The DEFAULTPAGE value is used by get_pagesize() as the fallback
 * when no environment variable override is provided.
 */
#ifdef OTHER
#define DEFAULTPAGE 0  /* Custom/generic page size */
#endif

#ifdef A4
#define DEFAULTPAGE 1  /* European A4 standard (210x297mm) */
#endif

#ifdef LETTER
#define DEFAULTPAGE 2  /* US Letter standard (8.5x11") */
#endif

/*
 * PAGE LAYOUT DIMENSIONS (PostScript Points)
 * ==========================================
 *
 * Physical page dimensions and positioning offsets for supported paper sizes.
 * All measurements are in PostScript points (1/72 inch). These values define
 * the printable area and content positioning for optimal output quality.
 *
 * DIMENSION CALCULATIONS:
 * - Page dimensions account for printer limitations and paper size standards
 * - Offsets position content to avoid unprintable margins near paper edges
 * - Values optimized for common PostScript printers and page requirements
 *
 * Used by setpagesize() to configure global layout variables based on
 * the selected page size identifier from parsepagesize().
 */

/* A4 Paper Layout (210mm x 297mm - European Standard) */
#define PAGEWIDTH_A4       540  /* Usable width in PostScript points */
#define PAGEHEIGHT_A4      820  /* Usable height in PostScript points */
#define XOFFSET_A4         30   /* Left margin offset from paper edge */
#define YOFFSET_A4         10   /* Bottom margin offset from paper edge */

/* Letter Paper Layout (8.5" x 11" - US Standard) */
#define PAGEWIDTH_LETTER   575  /* Usable width in PostScript points */
#define PAGEHEIGHT_LETTER  760  /* Usable height in PostScript points */
#define XOFFSET_LETTER     15   /* Left margin offset from paper edge */
#define YOFFSET_LETTER     10   /* Bottom margin offset from paper edge */

/* Custom/Other Paper Layout (Generic/Custom Sizes) */
#define PAGEWIDTH_OTHER    450  /* Conservative usable width */
#define PAGEHEIGHT_OTHER   700  /* Conservative usable height */
#define XOFFSET_OTHER      40   /* Larger offset for unknown printer margins */
#define YOFFSET_OTHER      30   /* Larger offset for unknown printer margins */

/*
 * CONTENT LAYOUT MARGINS
 * ======================
 *
 * Additional margin reservations within the usable page area to ensure
 * content fits comfortably and provides space for headers, footers,
 * and coordinate labels.
 *
 * These margins are subtracted from the page dimensions during layout
 * calculations in buildps() to determine the actual content area:
 *   content_area = (page_size - margins)
 *
 * Used in multi-page calculations to determine how many map squares
 * fit on each page and for centering calculations.
 */
#define XMARGINS        30  /* Horizontal margin reservation (left/right) */
#define YMARGINS        50  /* Vertical margin reservation (top/bottom) */

/*
 * PROGRAM METADATA AND INTERFACE STRINGS
 * ======================================
 *
 * Version information, command-line interface specification, and file
 * format identification strings used throughout the program.
 */
#define VERSION         "1.0"  /* Program version embedded in PostScript output */

/* Complete command-line usage specification for help display */
#define USAGE           "Usage: %s [cghlnvu] [p pagesize] [f font] [o x,y] [s size]\n\t[W n] [L n] [X n] [Y n] [t title] [infile [outfile]]\n"

/* File format validation string for Conquer map identification */
#define MATCHSTRING     "Conquer Version"  /* Required header in input files */

/*
 * BOOLEAN CONSTANTS
 * =================
 *
 * Standard boolean value definitions for logical operations and flag
 * variables throughout the codebase. Used for grid display, centering,
 * coordinate display, and other binary configuration options.
 */
#define TRUE            1  /* Boolean true value */
#define FALSE           0  /* Boolean false value */

/*
 * MAP TYPE CLASSIFICATION CONSTANTS
 * =================================
 *
 * Enumeration of supported map rendering types for PostScript output.
 * These constants determine the visual style, color scheme, and legend
 * information applied during PostScript generation.
 *
 * TYPE DETECTION:
 * Map types are automatically detected by getmaptype() through keyword
 * analysis of the map file header, or can be forced to SIMPLE through
 * the -u command-line option.
 *
 * RENDERING IMPLICATIONS:
 * Each map type may trigger different:
 * - Color schemes in PostScript template
 * - Legend generation
 * - Symbol interpretation
 * - Visual formatting rules
 */
#define SIMPLE          0  /* Basic character-based maps (default fallback) */
#define ALTITUDES       1  /* Elevation/topographical data maps */
#define DESIGNATIONS    2  /* Terrain type classification maps */
#define NATIONS         3  /* Political boundary and ownership maps */
#define VEGETATIONS     4  /* Ecosystem and land use visualization maps */
#define FORCED          5  /* User-forced simple output (via -u option) */

/*
 * TESTING NOTES FOR CONFIGURATION SYSTEM
 * =======================================
 *
 * Category: Configuration System (Header-only definitions)
 * Approach: Compile-time verification and integration testing
 * Key Tests: Conditional compilation, constant value verification, integration
 * Dependencies: Compilation system, psmap.c integration
 * Mock Requirements: None - compile-time constants
 * Complexity: Simple - static configuration definitions
 *
 * CRITICAL TEST SCENARIOS:
 * 1. Compile-time page size selection (-DA4, -DLETTER, -DOTHER)
 * 2. Page dimension constant verification (correct PostScript points)
 * 3. Map type constant uniqueness and range validation
 * 4. Integration with psmap.c function usage
 * 5. USAGE string format validation for help display
 * 6. MATCHSTRING validation against actual Conquer map files
 *
 * INTEGRATION DEPENDENCIES:
 * - parsepagesize() must handle all DEFAULTPAGE values
 * - setpagesize() must use all page dimension constants correctly
 * - getmaptype() must return only defined map type constants
 * - main() must display USAGE string correctly
 * - Map file validation must match MATCHSTRING exactly
 *
 * MODERNIZATION CONSIDERATIONS:
 * - Boolean constants could be replaced with <stdbool.h>
 * - Magic numbers could benefit from enum definitions
 * - String constants should be verified for buffer safety
 * - Page dimensions may need updates for modern printer margins
 */
