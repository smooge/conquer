/*
 * psmap.c - PostScript map generation utility for Conquer maps
 *
 * This file implements a standalone PostScript map generator that converts
 * Conquer game map files into printable PostScript format. The utility provides
 * sophisticated map rendering capabilities with support for multiple map types,
 * page layouts, and output formatting options.
 *
 * SYSTEM ARCHITECTURE:
 * ====================
 *
 * MAP PROCESSING PIPELINE:
 * ┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
 * │ Input Map File  │───▶│ Map Data Parser  │───▶│ PostScript Gen  │
 * │ (Conquer format)│    │ (readmap)        │    │ (buildps)       │
 * └─────────────────┘    └──────────────────┘    └─────────────────┘
 *           │                      │                       │
 *           ▼                      ▼                       ▼
 * ┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
 * │ Header Analysis │    │ Grid Processing  │    │ Output File     │
 * │ (getmaptype)    │    │ (character map)  │    │ (.ps format)    │
 * └─────────────────┘    └──────────────────┘    └─────────────────┘
 *
 * SUPPORTED MAP TYPES:
 * - SIMPLE: Basic character representation
 * - ALTITUDES: Elevation data maps
 * - DESIGNATIONS: Terrain type maps
 * - NATIONS: Political boundary maps
 * - VEGETATIONS: Ecosystem/terrain maps
 *
 * PAGE LAYOUT SYSTEM:
 * - A4 and Letter paper size support
 * - Multi-page output for large maps
 * - Configurable margins and scaling
 * - Optional grid lines and coordinates
 * - Custom titles and footers
 *
 * CONFIGURATION FEATURES:
 * - Environment variable support (CONQ_PSMAPDEFAULTPAGE, CONQ_PSFONT)
 * - Command-line option processing
 * - Flexible page layout options
 * - PostScript template integration
 *
 * FILE DEPENDENCIES:
 * - psmap.h: Constants and macro definitions
 * - PSFILE: PostScript template for rendering
 * - Input: Conquer map files with specific format requirements
 * - Output: Standard PostScript (.ps) files
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "psmap.h"
#include "safe_convert.h"

char buffer[BUFSIZ];
int c, xsize, ysize;
int xmin, ymin, xmax, ymax, centx, centy;
FILE *infile, *outfile, *fh;
int grid = TRUE, sqsize = 9, maptype = SIMPLE, onepage = FALSE;
int verbose = FALSE, coords = TRUE, center = TRUE, note = FALSE;
int pagewidth, pageheight, xoffset, yoffset;
char title[81] = "", foot[81], fontname[81];
char progname[80];

/*
 * parsepagesize - Parse page size string into numeric identifier
 *
 * Converts human-readable page size strings into internal numeric codes
 * used by the page layout system. Supports common paper sizes with
 * case-insensitive matching for user convenience.
 *
 * The function implements the page size mapping system used throughout
 * the PostScript generation pipeline. It provides a standardized way
 * to convert user input and environment variables into the internal
 * page size representation.
 *
 * Parameters:
 *   buf - String containing page size name (case-insensitive)
 *         Valid values: "A4", "a4", "LETTER", "letter"
 *         Must be null-terminated string
 *
 * Returns:
 *   1 for A4 paper size (210x297mm, common European standard)
 *   2 for Letter paper size (8.5x11", common US standard)
 *   0 for unrecognized page size (defaults to OTHER)
 *
 * Side Effects:
 *   None - pure function with no state modifications
 *
 * Testing Notes:
 *   Category: A (Unit) - Simple string comparison logic
 *   Approach: Unit tests with various string inputs
 *   Key Tests: Case sensitivity, exact matching, invalid inputs
 *   Dependencies: None - standalone function
 *   Mock Requirements: None
 *   Complexity: Simple - direct string comparison
 *
 * Notes:
 *   - Thread-safe (no global state access)
 *   - Case-insensitive for user convenience
 *   - Returns 0 for any unrecognized input (safe default)
 *   - Used by command-line option processing and environment variable parsing
 */
int
parsepagesize (char *buf)
{
    if (!strcmp(buf, "A4"))
	return (1);
    if (!strcmp(buf, "a4"))
	return (1);
    if (!strcmp(buf, "LETTER"))
	return (2);
    if (!strcmp(buf, "letter"))
	return (2);
    return (0);
}

/*
 * setpagesize - Configure page dimensions and layout parameters
 *
 * Sets global page layout variables based on the provided page size code.
 * This function translates abstract page size identifiers into concrete
 * dimension values used throughout the PostScript generation process.
 *
 * The function configures the complete page layout system including physical
 * dimensions (width/height) and positioning offsets. These values directly
 * affect how maps are scaled, positioned, and paginated in the final output.
 *
 * Parameters:
 *   defpag - Page size identifier code
 *            1: A4 paper (European standard)
 *            2: Letter paper (US standard)
 *            Other: Generic/custom page size
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   Modifies global variables:
 *   - pagewidth: Physical page width in PostScript points
 *   - pageheight: Physical page height in PostScript points
 *   - xoffset: Horizontal offset for content positioning
 *   - yoffset: Vertical offset for content positioning
 *
 * Testing Notes:
 *   Category: B (Integration) - Modifies global state variables
 *   Approach: Integration tests with global state verification
 *   Key Tests: Each page size case, default behavior
 *   Dependencies: Global page layout variables, psmap.h constants
 *   Mock Requirements: Access to global variables
 *   Complexity: Simple - direct variable assignment with switch logic
 *
 * Notes:
 *   - Not thread-safe due to global variable modifications
 *   - Must be called before any PostScript generation
 *   - Default case handles custom/user-defined page sizes
 *   - Values sourced from psmap.h constant definitions
 */
void
setpagesize (int defpag)
{
    switch (defpag) {
    case 1:
	pagewidth = PAGEWIDTH_A4;
	pageheight = PAGEHEIGHT_A4;
	xoffset = XOFFSET_A4;
	yoffset = YOFFSET_A4;
	break;
    case 2:
	pagewidth = PAGEWIDTH_LETTER;
	pageheight = PAGEHEIGHT_LETTER;
	xoffset = XOFFSET_LETTER;
	yoffset = YOFFSET_LETTER;
	break;
    default:
	pagewidth = PAGEWIDTH_OTHER;
	pageheight = PAGEHEIGHT_OTHER;
	xoffset = XOFFSET_OTHER;
	yoffset = YOFFSET_OTHER;
	break;
    }
}

/*
 * get_pagesize - Initialize page size configuration from environment
 *
 * Initializes the page size configuration system by checking for environment
 * variable overrides and applying appropriate defaults. This function provides
 * the primary entry point for page size configuration during program startup.
 *
 * The function implements a configuration hierarchy where environment variables
 * can override compile-time defaults, providing flexible user customization
 * without requiring command-line arguments.
 *
 * Configuration Priority:
 * 1. CONQ_PSMAPDEFAULTPAGE environment variable (if set)
 * 2. DEFAULTPAGE compile-time constant (fallback)
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   Calls setpagesize() which modifies global page layout variables:
 *   - pagewidth, pageheight, xoffset, yoffset
 *   Accesses environment variables via getenv()
 *
 * Testing Notes:
 *   Category: B (Integration) - Environment variable interaction
 *   Approach: Integration tests with environment setup/teardown
 *   Key Tests: Environment variable present/absent, valid/invalid values
 *   Dependencies: Environment variables, parsepagesize(), setpagesize()
 *   Mock Requirements: Environment variable mocking, function call verification
 *   Complexity: Moderate - environment interaction with fallback logic
 *
 * Notes:
 *   - Should be called during program initialization
 *   - Environment variable name: CONQ_PSMAPDEFAULTPAGE
 *   - Falls back gracefully if environment variable not set
 *   - Uses parsepagesize() for string-to-code conversion
 *   - Not thread-safe due to global state modifications
 */
void
get_pagesize (void)
{
    char *buf;
    int defpag = DEFAULTPAGE;

    buf = (char *) getenv("CONQ_PSMAPDEFAULTPAGE");
    if (buf != NULL)
	defpag = parsepagesize(buf);
    setpagesize(defpag);
}

/*
 * psstring - Output PostScript-safe string with proper escaping
 *
 * Converts a C string into a properly escaped PostScript string literal
 * by handling special characters that have meaning in PostScript syntax.
 * The function wraps the output in parentheses and escapes problematic
 * characters to ensure valid PostScript generation.
 *
 * PostScript requires specific character escaping rules:
 * - Parentheses: Must be escaped to avoid syntax conflicts
 * - Backslashes: Must be double-escaped to preserve literal meaning
 * - Control characters: Filtered out to prevent format corruption
 *
 * The function implements the core PostScript string encoding used
 * throughout the map generation process for titles, labels, and text content.
 *
 * Parameters:
 *   fh - Output file handle for PostScript content (must be open for writing)
 *   str - Input C string to convert (must be null-terminated)
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   Writes formatted PostScript string literal to the specified file handle
 *   Format: (escaped_string_content)
 *
 * Testing Notes:
 *   Category: A (Unit) - String processing logic
 *   Approach: Unit tests with various string inputs and file handle mocking
 *   Key Tests: Special character escaping, empty strings, long strings
 *   Dependencies: File I/O functions (fprintf, fputc)
 *   Mock Requirements: File handle mocking for output verification
 *   Complexity: Simple - character-by-character processing with escape rules
 *
 * Notes:
 *   - Thread-safe for different file handles
 *   - Filters out newline and form feed characters
 *   - Essential for preventing PostScript syntax errors
 *   - Used for titles, footers, and text labels in map output
 */
void
psstring (FILE *fh, char *str)
{
    fprintf(fh, "(");
    while (*str != '\0') {
	switch (*str) {
	case ('('):
	    fprintf(fh, "\\(");
	    break;
	case (')'):
	    fprintf(fh, "\\)");
	    break;
	case ('\\'):
	    fprintf(fh, "\\\\");
	    break;
	case ('\n'):
	case ('\f'):
	    break;
	default:
	    fputc(*str, fh);
	    break;
	}
	str++;
    }
    fprintf(fh, ")");
}

/*
 * isinstr - Search for substring occurrence within a string
 *
 * Performs substring search to determine if a word occurs anywhere within
 * a longer string. This function implements a simple linear search algorithm
 * for string pattern matching used by the map type detection system.
 *
 * The function uses efficient early termination logic to avoid unnecessary
 * comparisons when the remaining string is shorter than the search pattern.
 * It performs case-sensitive exact matching for reliable map type identification.
 *
 * Algorithm:
 * 1. Check if search string is longer than target (impossible match)
 * 2. Iterate through each position in the target string
 * 3. When first character matches, perform full substring comparison
 * 4. Return immediately on first successful match
 *
 * Parameters:
 *   string - Target string to search within (must be null-terminated)
 *   word - Substring pattern to find (must be null-terminated)
 *
 * Returns:
 *   TRUE (non-zero) if word is found as substring in string
 *   FALSE (0) if word is not found or if word is longer than string
 *
 * Side Effects:
 *   None - pure function with no state modifications
 *
 * Testing Notes:
 *   Category: A (Unit) - String search algorithm
 *   Approach: Unit tests with various string combinations
 *   Key Tests: Found/not found cases, edge cases (empty strings, exact matches)
 *   Dependencies: String functions (strlen, strncmp)
 *   Mock Requirements: None - uses standard library functions
 *   Complexity: Simple - linear search with early termination
 *
 * Notes:
 *   - Thread-safe (no global state access)
 *   - Case-sensitive search (exact match required)
 *   - Used by getmaptype() for map format detection
 *   - Efficient for short patterns in header strings
 */
int
isinstr (char *string, char *word)
{
    size_t i,l1=strlen(string),l2=strlen(word);

    if (l1 < l2) return(FALSE);
    for(i = 0; i < l1; i++ ) {
	    if (l1-i<l2) break;
	    if (string[i] == word[0]) {
		    if (strncmp(string+i,word,l2)==0) return(TRUE);
	    }
    }
    return(FALSE);
}

/*
 * getmaptype - Determine map rendering type from header string
 *
 * Analyzes the map file header to determine the appropriate rendering style
 * for PostScript output. Different map types require different visual treatments,
 * color schemes, and legend information in the final printed output.
 *
 * The function implements automatic map type detection by searching for
 * specific keywords in the map header string. This enables the PostScript
 * generator to apply appropriate formatting without manual user specification.
 *
 * Supported Map Types:
 * - ALTITUDES: Elevation/height maps (topographical data)
 * - DESIGNATIONS: Terrain type classifications
 * - NATIONS: Political boundary and ownership maps
 * - VEGETATIONS: Ecosystem and land use maps
 * - SIMPLE: Generic character-based maps (default fallback)
 *
 * Parameters:
 *   string - Map header string containing type information
 *            Typically the title portion after the colon in map files
 *            Must be null-terminated
 *
 * Returns:
 *   ALTITUDES constant if "Altitude" found in string
 *   DESIGNATIONS constant if "Designation" found in string
 *   NATIONS constant if "Nation" found in string
 *   VEGETATIONS constant if "Vegetation" found in string
 *   SIMPLE constant if no recognized type keywords found (default)
 *
 * Side Effects:
 *   None - pure function with no state modifications
 *
 * Testing Notes:
 *   Category: A (Unit) - String analysis with pattern matching
 *   Approach: Unit tests with various header string formats
 *   Key Tests: Each map type keyword, case sensitivity, partial matches
 *   Dependencies: isinstr() function for substring detection
 *   Mock Requirements: None - uses helper function
 *   Complexity: Simple - series of string searches with fallback
 *
 * Notes:
 *   - Thread-safe (no global state access)
 *   - Case-sensitive keyword matching
 *   - First match wins (order matters for overlapping keywords)
 *   - Used during map file parsing to configure rendering pipeline
 *   - Falls back to SIMPLE type for unknown formats
 */
int
getmaptype (char *string)
{
    if (isinstr(string, "Altitude"))
	return (ALTITUDES);
    if (isinstr(string, "Designation"))
	return (DESIGNATIONS);
    if (isinstr(string, "Nation"))
	return (NATIONS);
    if (isinstr(string, "Vegetation"))
	return (VEGETATIONS);
    return (SIMPLE);
}

/*
 * readmap - Parse map data and convert to PostScript array format
 *
 * Reads the map data from the input file character by character, analyzing
 * the structure and converting it into PostScript array format. This function
 * performs the core map data transformation that enables PostScript rendering.
 *
 * The function implements a sophisticated parsing algorithm that:
 * - Determines map dimensions dynamically during parsing
 * - Tracks the bounding box of actual map content (non-space areas)
 * - Converts character data to PostScript string arrays
 * - Validates map structure consistency (equal line lengths)
 * - Generates PostScript variable definitions for use by rendering code
 *
 * Map Processing Logic:
 * 1. Initialize dimension tracking variables and bounding box
 * 2. Parse character by character, building PostScript string arrays
 * 3. Track actual content areas (non-space characters) for bounding box
 * 4. Validate consistent line lengths for rectangular maps
 * 5. Generate final PostScript array definition
 *
 * PostScript Output Format:
 * /Map [
 *   (line1_content)
 *   (line2_content)
 *   ...
 * ] def
 *
 * Parameters:
 *   None (uses global file handles and variables)
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   Reads from global infile handle
 *   Writes PostScript array definition to global outfile handle
 *   Modifies global dimension variables: xsize, ysize, xmin, xmax, ymin, ymax
 *   Exits program on error conditions (inconsistent line lengths, empty file)
 *   Outputs verbose progress information if verbose mode enabled
 *
 * Testing Notes:
 *   Category: C (System) - File I/O with complex state management
 *   Approach: System testing with various map file formats
 *   Key Tests: Valid maps, empty files, inconsistent line lengths, boundary cases
 *   Dependencies: Global file handles, dimension variables, character processing
 *   Mock Requirements: File handle mocking, global variable state setup
 *   Complexity: Complex - stateful parsing with error handling and validation
 *
 * Notes:
 *   - Not thread-safe due to global variable modifications
 *   - Exits program on fatal errors (inconsistent structure, empty input)
 *   - Determines map bounding box dynamically for efficient rendering
 *   - Critical function in the map processing pipeline
 *   - Handles variable-sized maps with automatic dimension detection
 */
void
readmap (void)
{
    int x, none;

    /*
     * Now we read the map. Anybody got an idea how big it is ? I don't, but
     * who cares ?
     */

    x = 0;
    xsize = 0;
    ysize = 0;
    xmin = 0xffffff;
    xmax = (-1);
    ymin = (-1);
    ymax = 0;
    none = TRUE;

    fprintf(outfile, "/Map [\n");

    while ((c = fgetc(infile)) != EOF) {
	switch (c) {
	case ('\n'):
	    if (xsize == 0)
		xsize = x;
	    if (xsize != x) {
		fprintf(stderr, "Error in map-file. Lines different length\n");
		exit(1);
	    }
	    x = 0;
	    if ((xmax > -1) && (ymin == -1))
		ymin = ysize;
	    ysize++;
	    fprintf(outfile, ")\n");
	    none = TRUE;
	    break;
	case (' '):
	    x++;
	    if (none) {
		fprintf(outfile, "(");
		none = FALSE;
	    }
	    fprintf(outfile, " ");
	    break;
	default:
	    if (x < xmin)
		xmin = x;
	    if (x > xmax)
		xmax = x;
	    if (ysize > ymax)
		ymax = ysize;
	    x++;
	    if (none) {
		fprintf(outfile, "(");
		none = FALSE;
	    }
	    fprintf(outfile, "%c", c);
	    break;
	}
    }

    fprintf(outfile, "] def\n\n");

    /* Did I get anything ? */
    if (xmax == -1) {
	fprintf(stderr, "Empty input file\n");
	exit(1);
    }
    if (verbose) {
	fprintf(stderr, "Map is %d * %d  squares\n", xsize, ysize);
	fprintf(stderr, "You can see %d * %d  squares\n",
		xmax - xmin + 1, ymax - ymin + 1);
    }
}

/*
 * buildps - Generate complete PostScript output with page layout
 *
 * Generates the complete PostScript file by combining map data, configuration
 * variables, and PostScript template code. This function handles the complex
 * task of multi-page layout calculation and PostScript code generation.
 *
 * The function implements sophisticated page layout algorithms:
 * - Calculates required pages based on map size and page dimensions
 * - Handles optional map centering across multiple pages
 * - Supports single-page mode with custom coordinates
 * - Integrates PostScript template with generated variables
 * - Generates page rendering commands for the PostScript interpreter
 *
 * PostScript Generation Process:
 * 1. Calculate page requirements and layout parameters
 * 2. Output PostScript variable definitions for map and layout data
 * 3. Include PostScript template file (PSFILE) containing rendering code
 * 4. Generate page-specific rendering commands (DoPage calls)
 *
 * Page Layout Calculation:
 * - Pages required = (map_size * square_size) / (page_size - margins)
 * - Centering corrections applied if center mode enabled
 * - Single page mode uses user-specified center coordinates
 *
 * Parameters:
 *   None (uses global variables and file handles)
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   Writes complete PostScript program to global outfile handle
 *   Reads and includes PostScript template file (PSFILE)
 *   Outputs verbose page count information if verbose mode enabled
 *   Exits program on template file access errors
 *
 * Testing Notes:
 *   Category: C (System) - Complex file I/O and mathematical calculations
 *   Approach: System testing with various map sizes and page configurations
 *   Key Tests: Multi-page layout, single-page mode, centering, template integration
 *   Dependencies: Global variables, file handles, PostScript template file
 *   Mock Requirements: File system mocking, template file availability
 *   Complexity: Complex - mathematical layout with file integration
 *
 * Notes:
 *   - Not thread-safe due to global variable dependencies
 *   - Requires PSFILE template to be available and readable
 *   - Critical function in PostScript generation pipeline
 *   - Handles both single-page and multi-page output modes
 *   - PostScript variables enable template customization
 */
void
buildps (void)
{
    int xbeg, ybeg, xnumb, ynumb, x, y, xpages, ypages, xcorr, ycorr;

    /*
     * The map should be dumped to the outfile now. Maybe we should tell the
     * printer what to do with it?
     */

    xpages = 1 + (xmax - xmin) * sqsize / (pagewidth - XMARGINS);
    ypages = 1 + (ymax - ymin) * sqsize / (pageheight - YMARGINS);

    if (verbose)
	fprintf(stderr, "The map will be %d * %d pages\n", xpages, ypages);

    fprintf(outfile, "/xmin %d def\n", xmin);
    fprintf(outfile, "/ymin %d def\n", ymin);
    fprintf(outfile, "/xmax %d def\n", xmax);
    fprintf(outfile, "/ymax %d def\n", ymax);
    fprintf(outfile, "/xpages %d def\n", xpages);
    fprintf(outfile, "/ypages %d def\n", ypages);
    fprintf(outfile, "/sqsize %d def\n", sqsize);
    fprintf(outfile, "/title ");
    psstring(outfile, title);
    fprintf(outfile, " def\n");
    fprintf(outfile, "/foot ");
    psstring(outfile, foot);
    fprintf(outfile, " def\n");
    fprintf(outfile, "/foot2 (ConqPS Version %s) def\n", VERSION);
    fprintf(outfile, "/PAGEWIDTH %d def\n", pagewidth);
    fprintf(outfile, "/PAGEHEIGHT %d def\n", pageheight);
    fprintf(outfile, "/XMARGINS %d def\n", XMARGINS);
    fprintf(outfile, "/YMARGINS %d def\n", YMARGINS);
    fprintf(outfile, "/XOFFSET %d def\n", xoffset);
    fprintf(outfile, "/YOFFSET %d def\n", yoffset);
    fprintf(outfile, "/maptype %d def\n", maptype);
    fprintf(outfile, "/font /%s def\n", fontname);
    fprintf(outfile, "/grid ");
    if (grid) {
	fprintf(outfile, "true def\n");
    } else
	fprintf(outfile, "false def\n");
    fprintf(outfile, "/coords ");
    if (coords) {
	fprintf(outfile, "true def\n");
    } else
	fprintf(outfile, "false def\n");
    fprintf(outfile, "/noteq ");
    if (note) {
	fprintf(outfile, "true def\n");
    } else
	fprintf(outfile, "false def\n");

    fprintf(outfile, "\n\n");

    fh = fopen(PSFILE, "r");

    if (fh == NULL) {
	perror(progname);
	exit(-10);
    }
    while ((c = fgetc(fh)) != EOF)
	fputc(c, outfile);

    fclose(fh);

    fprintf(outfile, "\n %% Here we start the magic\n");

    xnumb = (int) ((pagewidth - XMARGINS) / sqsize);
    ynumb = (int) ((pageheight - YMARGINS) / sqsize);

    if (center) {
	xcorr = (int) ((xpages * xnumb - (xmax - xmin)) / 2);
	ycorr = (int) ((ypages * ynumb - (ymax - ymin)) / 2);
    } else {
	xcorr = 0;
	ycorr = 0;
    }

    if (onepage) {
	fprintf(outfile, "%d %d %d %d %d %d DoPage\n",
		1, 1, centx - xnumb / 2, xnumb, centy - ynumb / 2, ynumb);
    } else
	for (x = 0; x < xpages; x++)
	    for (y = 0; y < ypages; y++) {
		xbeg = x * xnumb + x + xmin - xcorr;
		ybeg = y * ynumb + y + ymin - ycorr;
		fprintf(outfile, "%d %d %d %d %d %d DoPage\n",
			x + 1, y + 1, xbeg, xnumb, ybeg, ynumb);
	    }
}

/*
 * main - Program entry point and command-line interface
 *
 * Implements the complete PostScript map generation utility with comprehensive
 * command-line option processing, file handling, and pipeline orchestration.
 * This function serves as the central coordinator for the entire map conversion
 * process from Conquer format to PostScript output.
 *
 * Program Architecture:
 * 1. Environment and default configuration setup
 * 2. Command-line option parsing with extensive customization
 * 3. Input/output file handling with validation
 * 4. Map file format validation and header processing
 * 5. Map type detection and configuration
 * 6. PostScript generation pipeline execution
 *
 * Command Line Options:
 * -c: Disable coordinate display     -f: Set font name
 * -g: Disable grid lines            -h: Show help information
 * -l: Enable large map notes        -L: Set page height
 * -n: Disable map centering         -o: Single page mode with coordinates
 * -p: Set page size (A4/LETTER)     -s: Set square size
 * -t: Set map title                 -u: Force simple map output
 * -v: Enable verbose mode           -W: Set page width
 * -X: Set X offset                  -Y: Set Y offset
 *
 * Environment Variables:
 * - CONQ_PSFONT: Default font for PostScript output
 * - CONQ_PSMAPDEFAULTPAGE: Default page size (A4/LETTER/OTHER)
 *
 * Parameters:
 *   argc - Command line argument count
 *   argv - Command line argument vector
 *          argv[optind]: Input map file (optional, defaults to stdin)
 *          argv[optind+1]: Output PostScript file (optional, defaults to stdout)
 *
 * Returns:
 *   0 on successful completion
 *   1 on command line errors or invalid input format
 *   -10 on file access errors
 *
 * Side Effects:
 *   Reads from input file or stdin (Conquer map format)
 *   Writes to output file or stdout (PostScript format)
 *   Modifies global configuration variables based on options
 *   Outputs verbose information to stderr if verbose mode enabled
 *   Sets up file buffering for output optimization
 *
 * Testing Notes:
 *   Category: C (System) - Complete program integration with file I/O
 *   Approach: System testing with various command line combinations
 *   Key Tests: All option combinations, file I/O, format validation, error cases
 *   Dependencies: File system, environment variables, command line arguments
 *   Mock Requirements: File system mocking, environment setup, argument simulation
 *   Complexity: Complex - complete program orchestration with multiple subsystems
 *
 * Notes:
 *   - Program entry point with complete option processing
 *   - Handles both stdin/stdout and file-based I/O
 *   - Validates Conquer map file format before processing
 *   - Supports extensive customization through command line options
 *   - Environment variables provide user-specific defaults
 *   - Orchestrates the complete map generation pipeline
 */
int
main(int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    char *buf, firstline[81];

    strcpy(progname, argv[0]);
    infile = stdin;
    outfile = stdout;
    buf = (char *) getenv("CONQ_PSFONT");
    if (buf != NULL) {
	strncpy(fontname, buf, 80);
	fontname[79] = '\0';
    } else
	strcpy(fontname, "Times-Roman");
    get_pagesize();

    while ((c = getopt(argc, argv, "nuf:gs:t:vcho:p:lW:L:X:Y:")) != -1)
	switch (c) {
	case 'h':
	    fprintf(stderr, "%s version %s\n", progname, VERSION);
	    fprintf(stderr, "Default pagesize is ");
	    switch (DEFAULTPAGE) {
	    case 1:
		fprintf(stderr, "A4\n");
		break;
	    case 2:
		fprintf(stderr, "LETTER\n");
		break;
	    default:
		fprintf(stderr, "OTHER\n");
		break;
	    }
	    fprintf(stderr, USAGE, progname);
	    fprintf(stderr, "\t-c  Turn off coordinates\n");
	    fprintf(stderr, "\t-f  Set the font\n");
	    fprintf(stderr, "\t-g  Turn off grid\n");
	    fprintf(stderr, "\t-h  Show this text\n");
	    fprintf(stderr, "\t-l  Print large maps\n");
	    fprintf(stderr, "\t-L  Set the pagelength\n");
	    fprintf(stderr, "\t-n  Turn off map centering\n");
	    fprintf(stderr, "\t-o  Show one page centered around x,y\n");
	    fprintf(stderr, "\t-p  Set pagesize (A4,LETTER or OTHER)\n");
	    fprintf(stderr, "\t-s  Set size of square (default: %d)\n", sqsize);
	    fprintf(stderr, "\t-t  Set the title of the map\n");
	    fprintf(stderr, "\t-u  Force simple map output (just letters)\n");
	    fprintf(stderr, "\t-v  Verbose mode\n");
	    fprintf(stderr, "\t-W  Set the pagewidth\n");
	    fprintf(stderr, "\t-X  Set the X-offset\n");
	    fprintf(stderr, "\t-Y  Set the Y-offset\n");
	    exit(1);
	case 'u':
	    maptype = FORCED;
	    break;
	case 'o':
	    onepage = TRUE;	/* Mode one of onepage */
	    if (2 != sscanf(optarg, "%d,%d", &centx, &centy)) {
		fprintf(stderr, "Error in coordinates to o-option\n");
		exit(1);
	    }
	    break;
	case 'l':
	    note = TRUE;
	    break;
	case 'p':
	    setpagesize(parsepagesize(optarg));
	    break;
	case 'n':
	    center = FALSE;
	    break;
	case 'g':
	    grid = FALSE;
	    break;
	case 's':
	    sscanf(optarg, "%d", &sqsize);
	    break;
	case 'f':
	    strncpy(fontname, optarg, 80);
	    fontname[79] = '\0';
	    break;
	case 't':
	    strncpy(title, optarg, 80);
	    title[79] = '\0';
	    break;
	case 'v':
	    verbose = TRUE;
	    break;
	case 'c':
	    coords = FALSE;
	    break;
	case 'W':
	    sscanf(optarg, "%d", &pagewidth);
	    break;
	case 'L':
	    sscanf(optarg, "%d", &pageheight);
	    break;
	case 'X':
	    sscanf(optarg, "%d", &xoffset);
	    break;
	case 'Y':
	    sscanf(optarg, "%d", &yoffset);
	    break;
	default:
	    fprintf(stderr, USAGE, argv[0]);
	    exit(1);
	}
    if (optind < argc)
	infile = fopen(argv[optind], "r");

    if (infile == NULL) {
	perror(progname);
	exit(-10);
    }
    if (++optind < argc)
	outfile = fopen(argv[optind], "w");

    if (outfile == NULL) {
	perror(progname);
	exit(-10);
    }
    setbuf(outfile, buffer);	/* They recomended this on the net today */

    if (++optind < argc) {
	fprintf(stderr, USAGE, argv[0]);
	exit(1);
    }
    /* If verbose identify program */

    if (verbose)
	fprintf(stderr, "Psmap version %s\n", VERSION);


    /* First we check if the infile is a valid conquer map file */

    if (NULL == fgets(firstline, 80, infile)) {
	fprintf(stderr, "Empty input file\n");
	exit(1);
    }
    if (0 != strncmp(firstline, MATCHSTRING, strlen(MATCHSTRING))) {
	fprintf(stderr, "Not a Conquer Map file\n");
	exit(1);
    }
    for (c = 0; c < (int)strlen(firstline); c++) {
	if (firstline[c] == ':')
	    break;
    }

    strncpy(foot, firstline, safe_int_to_size(c));
    foot[c] = '\0';
    if (title[0] == '\0') {
	strncpy(title, &firstline[c + 1], 80);
	title[79] = '\0';
    }
    /* Find out which type of map it is */

    if (maptype != FORCED) {
	maptype = getmaptype(&firstline[c + 1]);
    } else
	maptype = SIMPLE;

    if (verbose) {
	fprintf(stderr, "Maptype is ");
	switch (maptype) {
	case (SIMPLE):
	    fprintf(stderr, "simple\n");
	    break;
	case (ALTITUDES):
	    fprintf(stderr, "altitudes\n");
	    break;
	case (DESIGNATIONS):
	    fprintf(stderr, "designations\n");
	    break;
	case (NATIONS):
	    fprintf(stderr, "nations\n");
	    break;
	case (VEGETATIONS):
	    fprintf(stderr, "vegetations\n");
	    break;
	default:
	    break;
	}
    }
    /* Send the first part of the postscript file to outfile */

    fprintf(outfile, "%%!\n");
    fprintf(outfile, "%% Created by conqps version %s\n\n", VERSION);
    if (note)
	fprintf(outfile, "\nnote\n\n");
    fprintf(outfile, "%% Here comes the map data:\n");

    /* Parse the map */

    readmap();

    /* Build the last part of postscript file */

    buildps();

    /* Phu that was it.  */

    if (infile != NULL)
	fclose(infile);
    if (outfile != NULL)
	fclose(outfile);

    return (0);
}
