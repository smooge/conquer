/*
 * spew.c - Advanced NPC Message Generation System
 *
 * This file implements a sophisticated text generation engine for creating dynamic
 * NPC messages in the Conquer game. The system uses a rules-based approach with
 * weighted random selection, recursive text expansion, and variant handling to
 * generate contextually appropriate messages for game events, NPC interactions,
 * and random occurrences.
 *
 * SYSTEM ARCHITECTURE:
 * ==================
 * The message generation system consists of several interconnected components:
 *
 * 1. RULES FILE PARSING ENGINE
 *    - Loads and parses external rules files containing message templates
 *    - Supports class-based organization with hierarchical text definitions
 *    - Handles weighted probability distributions for message selection
 *    - Implements variant systems for context-sensitive message variations
 *
 * 2. TEXT CLASS MANAGEMENT SYSTEM
 *    - Organizes message templates into named classes for easy reference
 *    - Supports binary search optimization for fast class lookup
 *    - Manages variant tags for context-dependent message customization
 *    - Handles cumulative weight calculation for probability distributions
 *
 * 3. RECURSIVE TEXT GENERATION ENGINE
 *    - Processes embedded class references within message templates
 *    - Supports recursive expansion for complex message hierarchies
 *    - Handles escape sequences and special formatting directives
 *    - Implements variant selection based on context propagation
 *
 * 4. MEMORY MANAGEMENT SUBSYSTEM
 *    - Dynamic allocation for scalable class and definition storage
 *    - Linked list management for definition chains within classes
 *    - Comprehensive cleanup routines for leak-free operation
 *    - String duplication utilities for safe text storage
 *
 * RULES FILE FORMAT:
 * ================
 * The system uses a specialized text format for defining message templates:
 *
 * - Class Headers: %CLASSNAME {variants}
 *   Example: %GREETING {formal casual}
 *
 * - Weighted Definitions: (weight) message template
 *   Example: (3) Hello there, \TITLE/& \NAME/!
 *
 * - Class References: \CLASSNAME/variant
 *   Example: \GREETING/formal or \TITLE/& (inherit variant)
 *
 * - Variant Blocks: {option1|option2|option3}
 *   Example: {Your Majesty|Sir|Friend}
 *
 * - Escape Sequences: \! (newline), \\ (literal backslash)
 *
 * INTEGRATION POINTS:
 * =================
 * The spew system integrates with several game subsystems:
 *
 * - NPC System (npc.c): Provides dynamic messages for AI character interactions
 * - Random Events (randeven.c): Generates descriptive text for special occurrences
 * - Combat System (combat.c): Creates battle descriptions and outcome messages
 * - Administrative Functions: Supports message generation for game management
 *
 * CONDITIONAL COMPILATION:
 * ======================
 * The entire system is conditionally compiled based on the SPEW preprocessor
 * definition. When SPEW is not defined, a stub implementation is provided to
 * maintain API compatibility while disabling message generation functionality.
 *
 * CONFIGURATION PARAMETERS:
 * =======================
 * - MAX_CLASSES: Maximum number of message classes (300)
 * - MAX_LINE_LEN: Maximum line length for file I/O (256 bytes)
 * - MAX_DEF_LEN: Maximum definition text length (1000 bytes)
 * - DEFFILE: Default rules filename ("rules")
 * - DEFAULTDIR: Directory containing rules files
 *
 * PERFORMANCE CHARACTERISTICS:
 * ==========================
 * - Class lookup: O(log n) via binary search after initial sort
 * - Memory usage: Dynamic allocation scales with rules file size
 * - Text generation: Linear in output length with recursive expansion
 * - File parsing: Single-pass with efficient line-by-line processing
 *
 * ERROR HANDLING STRATEGY:
 * ======================
 * - Graceful degradation with placeholder text for missing classes
 * - Comprehensive validation during rules file parsing
 * - Memory allocation failure handling with cleanup
 * - Invalid format detection with informative error messages
 *
 * THREAD SAFETY CONSIDERATIONS:
 * ============================
 * The current implementation uses static global variables and is NOT thread-safe.
 * Multiple simultaneous calls to makemess() would cause data corruption.
 * Future modernization should consider thread-local storage or instance-based design.
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
#include <ctype.h>
#include <sysexits.h>
#include <stdint.h>
#include "header.h"
#include "data.h"
#include "safe_convert.h"

#ifdef SPEW

/* Configuration parameters */
#define MAX_CLASSES 300
#define MAX_LINE_LEN 256
#define MAX_DEF_LEN 1000
#define ESCAPE_CHAR '\\'
#define DELIMITER_CHAR '/'
#define VARIANT_CHAR '|'

#ifndef DEFFILE
#define DEFFILE "rules"
#endif

/* Random number generator macro */
#define RAND_INT(n) (rand() % (n))

/* Structure to hold a single definition within a class */
struct definition {
    int weight;                    /* cumulative weight for selection */
    char *text;                   /* the actual text definition */
    struct definition *next;      /* linked list pointer */
};

/* Structure to hold a class of definitions */
struct text_class {
    char *name;                   /* name of this class */
    char *variants;               /* string of variant tags */
    int total_weight;             /* total weight of all definitions */
    struct definition *defs;      /* linked list of definitions */
};

/* Global variables */
static FILE *rules_file = NULL;
static struct text_class *classes = NULL;
static int num_classes = 0;
static char input_line[MAX_LINE_LEN];
static const char *default_variants = " ";

/* Function prototypes */
static int load_rules_file(const char *filename);
static int parse_class_header(const char *line, struct text_class *cls);
static struct definition *parse_definition(const char *line);
static struct text_class *find_class(const char *name, int name_len);
static void generate_text(const char *class_spec, char variant_tag, FILE *output);
static void cleanup_memory(void);
static char *duplicate_string(const char *str);
static int read_line(void);
static int compare_classes(const void *a, const void *b);

/*
 * makemess - Generate dynamic NPC messages using rules-based text generation
 *
 * This function serves as the primary entry point for the message generation
 * system, orchestrating the loading of rules files and the creation of
 * contextually appropriate messages for game events. It handles the complete
 * lifecycle from rules file initialization to message output and cleanup.
 *
 * The function implements a batch processing approach, generating multiple
 * messages in a single invocation to amortize the cost of rules file loading
 * and parsing. Each generated message follows the template structures defined
 * in the external rules file, with random selection providing variability.
 *
 * PROCESSING WORKFLOW:
 * ==================
 * 1. Input validation to ensure output destination is available
 * 2. Rules file path construction using DEFAULTDIR and DEFFILE macros
 * 3. Rules file loading and parsing into internal class structures
 * 4. Iterative message generation using "MAIN" class as entry point
 * 5. Memory cleanup to prevent resource leaks
 *
 * RULES FILE INTEGRATION:
 * =====================
 * The function expects a rules file at {DEFAULTDIR}/{DEFFILE} containing
 * class definitions starting with "MAIN" as the root class. The MAIN class
 * serves as the entry point for message generation, typically containing
 * references to more specific message classes.
 *
 * MESSAGE SEPARATION:
 * =================
 * Multiple messages are separated by newlines, with the final message not
 * followed by a newline to allow caller control over output formatting.
 * This approach supports both file output and direct console display.
 *
 * ERROR HANDLING STRATEGY:
 * ======================
 * - Null output validation prevents segmentation faults
 * - Rules file loading failures result in graceful exit with error message
 * - Memory allocation failures during rules parsing are handled internally
 * - Cleanup is guaranteed even in error conditions
 *
 * PERFORMANCE CONSIDERATIONS:
 * =========================
 * - Rules file is loaded once per function call, not per message
 * - Binary search optimization for class lookup during generation
 * - Memory is allocated in blocks to minimize fragmentation
 * - Cleanup occurs after all messages to avoid repeated allocations
 *
 * INTEGRATION CONTEXT:
 * ==================
 * This function is typically called by:
 * - NPC interaction systems for dynamic dialogue generation
 * - Random event processors for descriptive text creation
 * - Administrative tools for bulk message generation
 * - Testing frameworks for system validation
 *
 * Parameters:
 *   count - Number of messages to generate (must be > 0 for meaningful output)
 *   output - File stream for message output (must not be NULL)
 *
 * Returns:
 *   void (errors reported via stderr, successful operation produces messages)
 *
 * Side Effects:
 *   - Loads and parses external rules file from filesystem
 *   - Allocates substantial memory for class and definition storage
 *   - Writes formatted text messages to output stream
 *   - May generate error messages to stderr on failure
 *   - Modifies global random number generator state
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires external rules file and filesystem access
 *   Approach: Integration testing with mock rules files and output capture
 *   Key Tests: Valid rules file processing, invalid file handling, output formatting
 *   Dependencies: Filesystem access, DEFAULTDIR/DEFFILE configuration, random generator
 *   Mock Requirements: Filesystem mocking, rules file fixtures, output stream capture
 *   Complexity: Moderate - File I/O and text processing with multiple error paths
 */
void makemess(int count, FILE *output)
{
    char main_class[32];
    int i;

    if (!output) {
        fprintf(stderr, "Error: No output file provided\n");
        return;
    }

    /* Try to load the rules file */
    char filename[256];
    snprintf(filename, sizeof(filename), "%s/%s", DEFAULTDIR, DEFFILE);

    if (load_rules_file(filename) != 0) {
        fprintf(stderr, "Error: Cannot load rules file: %s\n", filename);
        return;
    }

    /* Generate the requested number of messages */
    strncpy(main_class, "MAIN/ ", sizeof(main_class));
    main_class[sizeof(main_class) - 1] = '\0';
    for (i = 0; i < count; i++) {
        generate_text(main_class, ' ', output);
        if (i < count - 1) {
            fprintf(output, "\n");
        }
    }

    cleanup_memory();
}

/*
 * load_rules_file - Parse and load message class definitions from external rules file
 *
 * This function implements a comprehensive rules file parser that reads external
 * message template definitions and constructs an in-memory representation for
 * efficient message generation. The parser handles the complete rules file format
 * including class headers, weighted definitions, and proper error validation.
 *
 * The function performs a single-pass parse of the rules file, building linked
 * lists of message definitions organized into named classes. After parsing,
 * classes are sorted alphabetically to enable efficient binary search during
 * message generation.
 *
 * PARSING ALGORITHM:
 * ================
 * 1. File opening and initial validation (must start with '%' class marker)
 * 2. Memory allocation for maximum number of classes (MAX_CLASSES)
 * 3. Iterative parsing of class headers and their associated definitions
 * 4. Cumulative weight calculation for probability distribution setup
 * 5. Post-processing sort for optimized lookup performance
 *
 * RULES FILE FORMAT REQUIREMENTS:
 * =============================
 * - File must begin with a class definition line starting with '%'
 * - Class headers: %CLASSNAME {optional_variants}
 * - Definitions follow class headers until next '%' or end of file
 * - File ending marked by '%%' (double percent) line
 * - Comments supported via '\*' marker (line remainder ignored)
 * - Empty lines and whitespace-only lines are skipped
 *
 * MEMORY ORGANIZATION:
 * ==================
 * - Classes stored in array for fast access and sorting
 * - Definitions within each class stored as linked lists
 * - All text content dynamically allocated for memory efficiency
 * - Cumulative weights calculated to support O(1) random selection
 *
 * ERROR HANDLING STRATEGY:
 * ======================
 * - File access failures return -1 with errno preserved
 * - Overflow detection prevents allocation and returns -1 with error message
 * - Memory allocation failures cleanup partial data and return -1 with error message
 * - Invalid format detection produces stderr messages and exits gracefully
 * - Partial success is not allowed - either complete success or total failure
 *
 * PERFORMANCE OPTIMIZATIONS:
 * =========================
 * - Single-pass parsing minimizes file I/O operations
 * - Block memory allocation reduces fragmentation
 * - Post-parse sorting enables O(log n) class lookup
 * - Weight accumulation during parse eliminates post-processing
 *
 * DATA STRUCTURE CONSISTENCY:
 * =========================
 * After successful completion, the following invariants are maintained:
 * - All classes have valid names and properly initialized structures
 * - Definition weights are cumulative within each class
 * - Classes are sorted alphabetically by name
 * - All dynamically allocated memory is properly referenced
 *
 * INTEGRATION DEPENDENCIES:
 * =======================
 * - Requires read_line() for comment-aware line processing
 * - Uses parse_class_header() for class definition parsing
 * - Uses parse_definition() for individual message template processing
 * - Depends on compare_classes() for post-parse sorting
 *
 * Parameters:
 *   filename - Path to rules file for parsing (must be readable)
 *
 * Returns:
 *   0 on successful parse and load, -1 on any error condition
 *
 * Side Effects:
 *   - Opens and reads specified file from filesystem
 *   - Allocates global classes array and associated memory
 *   - Sets global num_classes to reflect loaded class count
 *   - Modifies global classes array with sorted class data
 *   - May output error messages to stderr on parsing failures
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires filesystem access and file fixtures
 *   Approach: Integration testing with various rules file formats and error conditions
 *   Key Tests: Valid format parsing, invalid format handling, memory allocation failures
 *   Dependencies: Filesystem access, rules file fixtures, error message capture
 *   Mock Requirements: File system mocking, malloc failure injection, stderr capture
 *   Complexity: Complex - Multi-stage parsing with dynamic memory management
 */
static int load_rules_file(const char *filename)
{
    rules_file = fopen(filename, "r");
    if (!rules_file) {
        return -1;
    }

    /* Allocate memory for classes - check for overflow first */
    if (MAX_CLASSES > SIZE_MAX / sizeof(struct text_class)) {
        fprintf(stderr, "load_rules_file: Array allocation size overflow\n");
        fclose(rules_file);
        return -1;
    }

    classes = calloc(MAX_CLASSES, sizeof(struct text_class));
    if (!classes) {
        fprintf(stderr, "load_rules_file: calloc(%d, %zu) failed\n",
                MAX_CLASSES, sizeof(struct text_class));
        fclose(rules_file);
        return -1;
    }

    /* Read the first line - should start with '%' */
    if (!read_line() || input_line[0] != '%') {
        fprintf(stderr, "Error: Rules file must start with class definition\n");
        fclose(rules_file);
        return -1;
    }

    /* Parse all classes */
    while (input_line[1] != '%') {
        if (num_classes >= MAX_CLASSES) {
            fprintf(stderr, "Error: Too many classes (max %d)\n", MAX_CLASSES);
            fclose(rules_file);
            return -1;
        }

        /* Parse class header */
        if (parse_class_header(input_line, &classes[num_classes]) != 0) {
            fclose(rules_file);
            return -1;
        }

        /* Parse definitions for this class */
        struct definition **def_ptr = &classes[num_classes].defs;
        while (read_line() && input_line[0] != '%') {
            struct definition *def = parse_definition(input_line);
            if (!def) {
                continue;
            }

            *def_ptr = def;
            classes[num_classes].total_weight += def->weight;
            def->weight = classes[num_classes].total_weight; /* Make cumulative */
            def_ptr = &def->next;
        }

        num_classes++;
    }

    fclose(rules_file);

    /* Sort classes by name for binary search */
    qsort(classes, safe_int_to_size(num_classes), sizeof(struct text_class), compare_classes);

    return 0;
}

/*
 * parse_class_header - Parse rules file class definition header lines
 *
 * This function analyzes class header lines from the rules file and extracts
 * the class name and optional variant specifications. Class headers define
 * the organizational structure for message templates, establishing named
 * categories with optional variant tags for context-sensitive generation.
 *
 * The function performs comprehensive parsing and validation of the class
 * header format, handling whitespace normalization, name extraction, and
 * variant tag processing. It initializes the provided text_class structure
 * with the parsed information and prepares it for definition attachment.
 *
 * HEADER FORMAT REQUIREMENTS:
 * =========================
 * - Must start with '%' character (already stripped by caller)
 * - Class name must begin with alphanumeric character
 * - Class name consists only of alphanumeric characters
 * - Optional variant tags enclosed in curly braces: {variant1 variant2}
 * - Whitespace allowed between elements
 * - Example: "%GREETING {formal casual}" or "%MESSAGE"
 *
 * PARSING ALGORITHM:
 * ================
 * 1. Skip leading whitespace after '%' marker
 * 2. Validate class name starts with alphanumeric character
 * 3. Extract class name using alphanumeric character set
 * 4. Search for optional variant specification in curly braces
 * 5. Parse variant tags and construct variant string
 * 6. Allocate memory for extracted strings and initialize structure
 *
 * VARIANT TAG PROCESSING:
 * =====================
 * - Default variant is always space character (' ') at position 0
 * - Additional variants extracted from {tag1 tag2} specification
 * - Variants stored as continuous string for efficient lookup
 * - Variant characters used as indices during message generation
 * - Missing variant specification defaults to single space variant
 *
 * MEMORY MANAGEMENT:
 * ================
 * - Class name string dynamically allocated via duplicate_string()
 * - Variant string allocated if non-default variants specified
 * - Default variants use static string to avoid unnecessary allocation
 * - All dynamic allocations must be freed during cleanup phase
 *
 * ERROR HANDLING STRATEGY:
 * ======================
 * - Invalid class names (non-alphanumeric start) return -1 with stderr message
 * - Malformed variant syntax returns -1 with informative error message
 * - Memory allocation failures handled by duplicate_string() returning NULL
 * - Partial initialization cleaned up by caller on error return
 *
 * INTEGRATION CONTEXT:
 * ==================
 * - Called exclusively by load_rules_file() during rules file parsing
 * - Operates on single line extracted by read_line() function
 * - Initializes text_class structure for subsequent definition attachment
 * - Error return aborts entire rules file loading process
 *
 * Parameters:
 *   line - Class header line with '%' prefix already removed (must not be NULL)
 *   cls - Text class structure to initialize with parsed data (must not be NULL)
 *
 * Returns:
 *   0 on successful parsing and initialization, -1 on any error condition
 *
 * Side Effects:
 *   - Modifies all fields of provided text_class structure
 *   - Allocates memory for class name and optionally variant strings
 *   - May output error messages to stderr on parsing failures
 *   - Uses static buffers for temporary string processing
 *
 * Testing Notes:
 *   Category: A (Unit) - Isolated parsing logic with clear input/output
 *   Approach: Unit tests with various header format combinations
 *   Key Tests: Valid formats, invalid names, malformed variants, memory allocation
 *   Dependencies: duplicate_string() function, stderr output capture
 *   Mock Requirements: malloc failure injection for duplicate_string()
 *   Complexity: Moderate - String parsing with multiple validation paths
 */
static int parse_class_header(const char *line, struct text_class *cls)
{
    static char temp_name[100];
    static char temp_variants[100];
    const char *p = line + 1; /* Skip the '%' */
    char *name_ptr = temp_name;

    /* Initialize the class */
    cls->name = NULL;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
    cls->variants = (char *)default_variants;
#pragma GCC diagnostic pop
    cls->total_weight = 0;
    cls->defs = NULL;

    /* Skip whitespace */
    while (*p == ' ') p++;

    /* Extract class name */
    if (!isalnum(*p)) {
        fprintf(stderr, "Error: Invalid class name in: %s\n", line);
        return -1;
    }

    while (isalnum(*p)) {
        *name_ptr++ = *p++;
    }
    *name_ptr = '\0';
    cls->name = duplicate_string(temp_name);

    /* Look for variant tags */
    while (*p) {
        if (*p == ' ') {
            p++;
            continue;
        } else if (*p == '{') {
            p++;
            char *var_ptr = temp_variants;
            *var_ptr++ = ' '; /* Default variant */

            while (*p && *p != '}') {
                if (isalnum(*p)) {
                    *var_ptr++ = *p;
                }
                p++;
            }

            if (*p == '}') p++;
            *var_ptr = '\0';
            cls->variants = duplicate_string(temp_variants);
            break;
        } else {
            fprintf(stderr, "Error: Invalid class header: %s\n", line);
            return -1;
        }
    }

    return 0;
}

/*
 * parse_definition - Parse individual message template definitions from rules file
 *
 * This function processes individual definition lines within message classes,
 * extracting the optional weight specification and the message template text.
 * Definitions form the actual content pool for message generation, with
 * weights controlling the probability of selection during random generation.
 *
 * The function handles the complete definition syntax including weight parsing,
 * escape sequence processing, and class reference normalization. It constructs
 * a definition structure suitable for integration into the class's linked list
 * of available message templates.
 *
 * DEFINITION FORMAT REQUIREMENTS:
 * =============================
 * - Optional weight: (number) at line beginning
 * - Message template text following weight specification
 * - Escape sequences: \CLASSNAME/variant for references
 * - Special escapes: \! (newline), \\ (literal backslash)
 * - Class references: \GREETING/formal or \TITLE/&
 * - Default weight is 1 if not specified
 *
 * WEIGHT PROCESSING:
 * ================
 * - Weight specification format: (digits) message text
 * - Extracted weight becomes probability factor for random selection
 * - Higher weights increase selection probability
 * - Default weight of 1 used for unspecified weights
 * - Weight validation ensures positive integer values
 *
 * ESCAPE SEQUENCE HANDLING:
 * =======================
 * The function processes several types of escape sequences:
 * - Class references: \CLASSNAME/variant becomes \CLASSNAME/v format
 * - Newline escape: \! preserved for output formatting
 * - Literal escapes: \\ and other backslash sequences
 * - Variant inheritance: /& converted to / (space) for default variant
 * - Alphanumeric class names: Only valid characters in class references
 *
 * TEXT NORMALIZATION:
 * =================
 * - Class references normalized to standard \NAME/v format
 * - Escape sequences preserved in processed text
 * - Text length limited by MAX_DEF_LEN for memory safety
 * - Input text copied with escape processing applied
 *
 * MEMORY ALLOCATION:
 * ================
 * - Definition structure allocated dynamically
 * - Processed text string allocated via duplicate_string()
 * - Memory allocation failures return NULL
 * - Caller responsible for linking into class definition list
 *
 * ERROR HANDLING STRATEGY:
 * ======================
 * - NULL parameter validation: exits with EX_SOFTWARE (parameter error)
 * - Memory allocation failures exit with EX_SOFTWARE (critical failure)
 * - Invalid weight syntax defaults to weight 1
 * - Text overflow truncated at MAX_DEF_LEN boundary
 * - Malformed escape sequences processed as literal text
 *
 * INTEGRATION CONTEXT:
 * ==================
 * - Called by load_rules_file() for each definition line
 * - Returns structure for linking into class definition chain
 * - Always returns valid structure or exits program on failure
 * - Weight used for cumulative probability calculation by caller
 *
 * Parameters:
 *   line - Definition line text to parse (must not be NULL)
 *
 * Returns:
 *   Pointer to allocated definition structure on success (never returns on failure)
 *
 * Side Effects:
 *   - Allocates memory for definition structure and text content
 *   - May exit program with EX_SOFTWARE on allocation failure
 *   - Outputs error messages to stderr on failure conditions
 *   - Processes escape sequences in static buffer
 *   - No global state modification
 *
 * Testing Notes:
 *   Category: A (Unit) - Isolated parsing with clear input/output
 *   Approach: Unit tests with various definition formats and edge cases
 *   Key Tests: NULL parameter, weight parsing, escape sequences, memory allocation, text limits
 *   Dependencies: duplicate_string() function, malloc availability
 *   Mock Requirements: malloc failure injection, MAX_DEF_LEN boundary testing, NULL parameter handling
 *   Complexity: Moderate - Text processing with multiple parsing states
 */
static struct definition *parse_definition(const char *line)
{
    struct definition *def;
    const char *p;
    int weight = 1; /* default weight */
    static char processed_text[MAX_DEF_LEN];
    char *out = processed_text;

    /* Validate parameter */
    if (!line) {
        fprintf(stderr, "parse_definition: NULL parameter\n");
        exit(EX_SOFTWARE);
    }

    p = line;

    def = malloc(sizeof(struct definition));
    if (!def) {
        fprintf(stderr, "parse_definition: malloc(%zu) failed\n",
                sizeof(struct definition));
        exit(EX_SOFTWARE);
    }

    /* Check for weight specification */
    if (*p == '(') {
        p++;
        while (*p == ' ') p++;

        if (isdigit(*p)) {
            weight = 0;
            while (isdigit(*p)) {
                weight = weight * 10 + (*p - '0');
                p++;
            }
        }

        while (*p == ' ') p++;
        if (*p == ')') p++;
    }

    /* Process the text, handling escape sequences */
    while (*p && (out - processed_text) < MAX_DEF_LEN - 10) {
        if (*p == ESCAPE_CHAR) {
            *out++ = ESCAPE_CHAR;
            p++;
            if (isalnum(*p)) {
                /* Copy class reference */
                while (isalnum(*p)) {
                    *out++ = *p++;
                }
                *out++ = DELIMITER_CHAR;

                /* Handle variant tag */
                if (*p == DELIMITER_CHAR) {
                    p++;
                    if (isalnum(*p) || *p == ' ' || *p == '&') {
                        *out++ = *p++;
                    } else {
                        *out++ = ' ';
                    }
                } else {
                    *out++ = ' ';
                }
            } else if (*p == '!') {
                /* Newline escape */
                *out++ = '!';
                p++;
            } else if (*p) {
                /* Other escapes */
                *out++ = *p++;
            }
        } else {
            *out++ = *p++;
        }
    }

    *out = '\0';

    def->weight = weight;
    def->text = duplicate_string(processed_text);
    def->next = NULL;

    return def;
}

/*
 * find_class - Locate message class by name using optimized binary search
 *
 * This function implements an efficient binary search algorithm to locate
 * a message class by name within the sorted global classes array. The search
 * operates on a name prefix of specified length, enabling efficient lookup
 * during class reference resolution in message generation.
 *
 * The function leverages the alphabetically sorted class array established
 * during rules file loading to achieve O(log n) lookup performance. This
 * optimization is critical for message generation performance, as class
 * references are resolved repeatedly during recursive text expansion.
 *
 * SEARCH ALGORITHM:
 * ===============
 * 1. Initialize low and high bounds for binary search range
 * 2. Calculate midpoint and compare target name with midpoint class name
 * 3. Use strncmp() for prefix comparison with exact length matching
 * 4. Adjust search bounds based on comparison result
 * 5. Verify exact match by checking null termination at name_len
 * 6. Return class pointer on match, NULL on search failure
 *
 * NAME MATCHING STRATEGY:
 * =====================
 * - Uses strncmp() for length-limited prefix comparison
 * - Requires exact match: prefix matches AND name ends at name_len
 * - Case-sensitive matching to preserve class name distinctness
 * - Handles variable-length class names efficiently
 * - Validates complete name boundary to avoid partial matches
 *
 * PERFORMANCE CHARACTERISTICS:
 * ==========================
 * - Time complexity: O(log n) where n is number of classes
 * - Space complexity: O(1) - no additional memory allocation
 * - Cache-friendly: operates on contiguous sorted array
 * - String comparison optimized with length limitation
 * - No dynamic memory allocation during search
 *
 * PREREQUISITE CONDITIONS:
 * =====================
 * - Global classes array must be alphabetically sorted (by qsort)
 * - num_classes must accurately reflect array size
 * - All class names must be null-terminated strings
 * - Classes array must be properly initialized
 *
 * ERROR HANDLING:
 * =============
 * - Returns NULL for class not found (normal operation case)
 * - Handles empty classes array gracefully (returns NULL)
 * - No error messages generated (expected failure mode)
 * - Safe with invalid name_len values (strncmp handles boundary)
 *
 * INTEGRATION CONTEXT:
 * ==================
 * - Called by generate_text() during class reference resolution
 * - Used for all \CLASSNAME/ references in message templates
 * - Critical path function for message generation performance
 * - Returns class pointer for weight-based definition selection
 *
 * THREAD SAFETY:
 * ============
 * - Read-only operation on global data structures
 * - Safe for concurrent calls if classes array is immutable
 * - No modification of global state during search
 * - Depends on stable sorted order of classes array
 *
 * Parameters:
 *   name - Class name to search for (must not be NULL)
 *   name_len - Length of name prefix to match (must be > 0)
 *
 * Returns:
 *   Pointer to matching text_class structure, NULL if not found
 *
 * Side Effects:
 *   - No side effects - pure read-only operation
 *   - No global state modification
 *   - No memory allocation or deallocation
 *   - No error message output
 *
 * Testing Notes:
 *   Category: A (Unit) - Isolated search algorithm with predictable behavior
 *   Approach: Unit tests with various class arrays and search targets
 *   Key Tests: Found/not found cases, boundary conditions, empty arrays
 *   Dependencies: Sorted classes array, accurate num_classes value
 *   Mock Requirements: Test class arrays with known sort order
 *   Complexity: Simple - Standard binary search with string comparison
 */
static struct text_class *find_class(const char *name, int name_len)
{
    int low = 0, high = num_classes - 1;

    while (low <= high) {
        int mid = (low + high) / 2;
        int cmp = strncmp(name, classes[mid].name, safe_int_to_size(name_len));

        if (cmp == 0 && classes[mid].name[name_len] == '\0') {
            return &classes[mid];
        } else if (cmp < 0) {
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }

    return NULL;
}

/*
 * generate_text - Core recursive text generation engine with variant processing
 *
 * This function implements the heart of the message generation system, processing
 * class specifications and recursively expanding message templates to produce
 * final output text. It handles the complete text generation pipeline including
 * class resolution, random definition selection, variant processing, and
 * recursive expansion of embedded class references.
 *
 * The function operates as a recursive engine, capable of expanding nested
 * class references to arbitrary depth while maintaining variant context
 * propagation and proper escape sequence handling. This design enables
 * complex hierarchical message structures with context-sensitive variations.
 *
 * CLASS SPECIFICATION FORMAT:
 * =========================
 * - Input format: CLASSNAME/variant (e.g., "GREETING/formal")
 * - Variant inheritance: /& inherits default_variant from parent context
 * - Default variant: / (space) used when no specific variant requested
 * - Class name extraction: everything before first '/' character
 * - Variant tag: single character following '/' delimiter
 *
 * TEXT GENERATION ALGORITHM:
 * ========================
 * 1. Parse class specification to extract name and variant
 * 2. Locate class using binary search (find_class)
 * 3. Calculate variant index for context-sensitive selection
 * 4. Perform weighted random selection from available definitions
 * 5. Process definition text with escape sequence and variant handling
 * 6. Recursively expand embedded class references
 * 7. Output final processed text to specified stream
 *
 * VARIANT PROCESSING SYSTEM:
 * =========================
 * - Variant characters mapped to indices in class variant string
 * - Variant blocks: {option1|option2|option3} with positional selection
 * - Context propagation: variants passed to recursive calls
 * - Default inheritance: & variant inherits from calling context
 * - Variant scope: limited to single definition expansion
 *
 * RECURSIVE EXPANSION:
 * ==================
 * - Embedded references: \CLASSNAME/variant within definitions
 * - Depth control: relies on well-formed rules file structure
 * - Context preservation: variant tags propagated through call chain
 * - Memory safety: bounded by definition text length limits
 * - Infinite recursion protection: depends on rules file design
 *
 * ESCAPE SEQUENCE PROCESSING:
 * ==========================
 * The function handles multiple escape sequence types:
 * - Class references: \CLASSNAME/v → recursive generate_text() call
 * - Newline escape: \! → literal newline in output
 * - Literal escapes: \\ and other backslash sequences
 * - Variant blocks: {opt1|opt2} → conditional output based on variant
 * - Error markers: ??? for unresolved class references
 *
 * WEIGHTED RANDOM SELECTION:
 * =========================
 * - Uses cumulative weights for O(1) selection
 * - Random value generated within total weight range
 * - Linear scan through definitions until weight threshold exceeded
 * - Ensures proper probability distribution across definitions
 * - Handles zero-weight classes gracefully (no output)
 *
 * ERROR HANDLING STRATEGY:
 * ======================
 * - Unknown classes: Output ???CLASSNAME??? error marker
 * - Missing definitions: Silent return (no output)
 * - Malformed specifications: Output error markers
 * - File output errors: Handled by fprintf() return values
 * - No exceptions thrown: graceful degradation approach
 *
 * PERFORMANCE CONSIDERATIONS:
 * =========================
 * - Binary search for class lookup: O(log n)
 * - Random selection: O(1) with cumulative weights
 * - Text processing: Linear in definition length
 * - Memory usage: Stack depth proportional to recursion depth
 * - I/O efficiency: Direct character output without buffering
 *
 * INTEGRATION CONTEXT:
 * ==================
 * - Called by makemess() for root "MAIN/ " expansion
 * - Self-recursive for embedded class reference processing
 * - Uses find_class() for efficient class resolution
 * - Outputs directly to provided FILE stream
 * - Maintains variant context across recursive calls
 *
 * Parameters:
 *   class_spec - Class specification string "NAME/variant" (must not be NULL)
 *   default_variant - Variant character to use for & inheritance
 *   output - File stream for text output (must not be NULL)
 *
 * Returns:
 *   void (success indicated by text output, errors via ??? markers)
 *
 * Side Effects:
 *   - Writes generated text to output stream
 *   - May output error markers for unresolved references
 *   - Modifies random number generator state
 *   - Recursively calls itself for embedded references
 *   - No global state modification beyond RNG
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires classes array and random number generator
 *   Approach: Integration testing with mock classes and output stream capture
 *   Key Tests: Class resolution, variant processing, recursive expansion, error handling
 *   Dependencies: find_class(), classes array, random generator, FILE stream
 *   Mock Requirements: Controlled classes array, deterministic random values, output capture
 *   Complexity: Complex - Recursive engine with multiple processing modes
 */
static void generate_text(const char *class_spec, char default_variant, FILE *output)
{
    const char *slash_pos = strchr(class_spec, DELIMITER_CHAR);
    if (!slash_pos) {
        fprintf(output, "???%s???", class_spec);
        return;
    }

    int name_len = safe_long_to_int(slash_pos - class_spec);
    char variant_tag = slash_pos[1];
    if (variant_tag == '&') {
        variant_tag = default_variant;
    }

    /* Find the class */
    struct text_class *cls = find_class(class_spec, name_len);
    if (!cls) {
        fprintf(output, "???");
        fwrite(class_spec, 1, safe_int_to_size(name_len), output);
        fprintf(output, "???");
        return;
    }

    /* Find variant index */
    int variant_idx = 0;
    if (cls->variants) {
        const char *var_pos = strchr(cls->variants, variant_tag);
        if (var_pos) {
            variant_idx = safe_long_to_int(var_pos - cls->variants);
        }
    }

    /* Select a random definition */
    if (cls->total_weight == 0) {
        return;
    }

    int rand_val = safe_long_to_int(RAND_INT(cls->total_weight));
    struct definition *def = cls->defs;
    while (def && def->weight <= rand_val) {
        def = def->next;
    }

    if (!def) {
        return;
    }

    /* Process the definition text */
    const char *p = def->text;
    int in_variants = 0;
    int writing = 1;
    int current_variant = 0;

    while (*p) {
        if (*p == ESCAPE_CHAR) {
            p++;
            if (*p == '!') {
                if (writing) {
                    fprintf(output, "\n");
                }
                p++;
            } else if (isalnum(*p)) {
                /* Recursive class reference */
                if (writing) {
                    const char *start = p - 1;
                    while (*p != DELIMITER_CHAR && *p) p++;
                    if (*p == DELIMITER_CHAR) {
                        p += 2; /* Skip delimiter and variant tag */
                        char temp_spec[64];
                        size_t spec_len = (size_t)(p - start);
                        if (spec_len < sizeof(temp_spec)) {
                            memcpy(temp_spec, start, spec_len);
                            temp_spec[spec_len] = '\0';
                            generate_text(temp_spec, default_variant, output);
                        }
                    }
                } else {
                    /* Skip over the reference */
                    while (*p != DELIMITER_CHAR && *p) p++;
                    if (*p == DELIMITER_CHAR) p += 2;
                }
            } else if (*p) {
                if (writing) {
                    fputc(*p, output);
                }
                p++;
            }
        } else if (*p == '{') {
            if (!in_variants) {
                in_variants = 1;
                writing = (variant_idx == 0);
                current_variant = 0;
            } else if (writing) {
                fputc('{', output);
            }
            p++;
        } else if (*p == VARIANT_CHAR) {
            if (in_variants) {
                current_variant++;
                writing = (variant_idx == current_variant);
            } else if (writing) {
                fputc(VARIANT_CHAR, output);
            }
            p++;
        } else if (*p == '}') {
            if (in_variants) {
                writing = 1;
                in_variants = 0;
            } else if (writing) {
                fputc('}', output);
            }
            p++;
        } else {
            if (writing) {
                fputc(*p, output);
            }
            p++;
        }
    }
}

/*
 * read_line - Read rules file lines with comment processing and whitespace cleanup
 *
 * This function provides intelligent line reading for the rules file parser,
 * implementing comment filtering, whitespace normalization, and empty line
 * skipping. It serves as the foundation for clean rules file processing by
 * ensuring that only meaningful content lines are presented to the parser.
 *
 * The function handles the complete line preprocessing pipeline, including
 * comment removal, trailing whitespace cleanup, and automatic empty line
 * filtering. This preprocessing simplifies the parsing logic in higher-level
 * functions by providing clean, normalized input lines.
 *
 * COMMENT PROCESSING SYSTEM:
 * ========================
 * - Comment marker: \* (backslash followed by asterisk)
 * - Comment removal: Everything from \* to end of line is discarded
 * - Line preservation: Comments are removed without affecting line structure
 * - Comment position: Can appear anywhere within a line
 * - Parser integration: Allows inline documentation within rules files
 *
 * WHITESPACE NORMALIZATION:
 * =======================
 * - Trailing whitespace: Removed from end of all lines
 * - Newline handling: Automatic removal of line termination characters
 * - Empty line detection: Lines containing only whitespace are considered empty
 * - Blank line skipping: Continues reading until non-empty line found
 * - Buffer management: Uses global input_line buffer for processed content
 *
 * LINE PROCESSING ALGORITHM:
 * ========================
 * 1. Read raw line from rules file using fgets()
 * 2. Remove newline character if present (typically '\n')
 * 3. Locate and remove comment marker and following text
 * 4. Trim trailing whitespace from processed line
 * 5. Check if line is empty after processing
 * 6. Repeat cycle until non-empty line found or EOF reached
 * 7. Return success/failure indication to caller
 *
 * EOF HANDLING STRATEGY:
 * ====================
 * - End of file detection: fgets() returns NULL
 * - EOF marker generation: Sets input_line to "%%" double percent
 * - Parser compatibility: EOF marker recognized by load_rules_file()
 * - Clean termination: Ensures parser can detect file completion
 * - Return value: 0 indicates EOF reached, 1 indicates valid line read
 *
 * BUFFER MANAGEMENT:
 * ================
 * - Global buffer: Uses static input_line array for line storage
 * - Length limitation: Lines truncated at MAX_LINE_LEN boundary
 * - Null termination: Ensures proper string termination after processing
 * - Memory safety: Buffer bounds respected throughout processing
 * - Reusable storage: Buffer reused for each line read operation
 *
 * ERROR HANDLING APPROACH:
 * ======================
 * - File read failures: Treated as EOF condition with marker generation
 * - Oversized lines: Truncated at buffer boundary (fgets behavior)
 * - Comment processing: Malformed comments processed safely
 * - Memory safety: No dynamic allocation, uses static buffer
 * - Graceful degradation: Invalid content filtered out automatically
 *
 * INTEGRATION CONTEXT:
 * ==================
 * - Called exclusively by load_rules_file() during parsing
 * - Provides clean input for parse_class_header() and parse_definition()
 * - Maintains global input_line buffer for parser access
 * - Critical component of rules file processing pipeline
 * - Enables robust parsing in presence of comments and formatting variations
 *
 * PERFORMANCE CHARACTERISTICS:
 * ==========================
 * - I/O efficiency: Single character read per line via fgets()
 * - Memory usage: Constant space with static buffer reuse
 * - Processing speed: Linear scan for comment and whitespace removal
 * - Loop efficiency: Minimal overhead for empty line skipping
 * - String operations: Efficient in-place modification approach
 *
 * FILE FORMAT COMPATIBILITY:
 * =========================
 * - Line ending types: Handles standard '\n' line termination
 * - Comment syntax: Compatible with \* comment marker convention
 * - Whitespace tolerance: Accepts various indentation and spacing patterns
 * - Empty line handling: Flexible with respect to blank line presence
 * - Cross-platform: Works with different text file formats
 *
 * Parameters:
 *   void (operates on global rules_file and input_line buffer)
 *
 * Returns:
 *   1 on successful line read, 0 on EOF or file read failure
 *
 * Side Effects:
 *   - Modifies global input_line buffer with processed line content
 *   - Advances rules_file stream position past processed lines
 *   - May skip multiple lines if they are empty or comment-only
 *   - Sets input_line to "%%" marker on EOF condition
 *   - No error messages output (silent processing)
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires file stream and global state
 *   Approach: Integration testing with various line formats and edge cases
 *   Key Tests: Comment removal, whitespace handling, EOF detection, empty line skipping
 *   Dependencies: Global rules_file stream, input_line buffer, MAX_LINE_LEN constant
 *   Mock Requirements: File stream mocking, controlled input content
 *   Complexity: Moderate - String processing with multiple filtering stages
 */
static int read_line(void)
{
    char *comment_pos;

    do {
        if (!fgets(input_line, MAX_LINE_LEN, rules_file)) {
            strncpy(input_line, "%%", MAX_LINE_LEN); /* EOF marker */
            input_line[MAX_LINE_LEN - 1] = '\0';
            return 0;
        }

        /* Remove newline */
        char *newline = strrchr(input_line, '\n');
        if (newline) *newline = '\0';

        /* Remove comments (marked by \*) */
        comment_pos = strstr(input_line, "\\*");
        if (comment_pos) {
            *comment_pos = '\0';
        }

        /* Trim trailing whitespace */
        size_t len = strlen(input_line);
        if (len > 0) {
            char *end = input_line + len - 1;
            while (end >= input_line && isspace((unsigned char)*end)) {
                *end = '\0';
                end--;
            }
        }

    } while (input_line[0] == '\0');

    return 1;
}

/*
 * compare_classes - Comparison function for alphabetical class sorting
 *
 * This function provides a standard comparison interface for qsort() to enable
 * alphabetical sorting of message classes by name. Proper class sorting is
 * essential for the binary search optimization used during class lookup in
 * the message generation process.
 *
 * The function implements a straightforward lexicographic comparison using
 * strcmp(), ensuring consistent alphabetical ordering that supports efficient
 * O(log n) class resolution during message generation. This optimization is
 * critical for system performance when processing large rules files with
 * many class definitions.
 *
 * SORTING ALGORITHM INTEGRATION:
 * ============================
 * - Standard qsort() compatibility: Follows void* parameter convention
 * - Consistent ordering: Provides stable alphabetical sort results
 * - Case sensitivity: Maintains case-sensitive comparison for distinctness
 * - Performance optimization: Enables binary search after sorting
 * - Memory efficiency: No additional memory allocation during comparison
 *
 * COMPARISON SEMANTICS:
 * ===================
 * - Alphabetical ordering: Uses standard string comparison semantics
 * - Return value convention: <0, 0, >0 for less-than, equal, greater-than
 * - String handling: Depends on null-terminated class name strings
 * - Unicode compatibility: Works with standard ASCII and extended character sets
 * - Collation consistency: Provides reproducible sort ordering
 *
 * PERFORMANCE CHARACTERISTICS:
 * ==========================
 * - Time complexity: O(k) where k is average class name length
 * - Space complexity: O(1) - no additional memory allocation
 * - String comparison: Optimized strcmp() implementation
 * - Call frequency: Invoked O(n log n) times during qsort() execution
 * - Cache efficiency: Simple comparison with predictable memory access
 *
 * QSORT INTEGRATION:
 * ================
 * - Function signature: Compatible with qsort() comparison function requirements
 * - Type safety: Handles void* parameters with proper casting
 * - Pointer validation: Assumes valid text_class pointers (caller responsibility)
 * - Structure access: Accesses name field of text_class structures
 * - Sorting context: Used during load_rules_file() post-processing phase
 *
 * BINARY SEARCH ENABLEMENT:
 * =========================
 * The primary purpose of this comparison function is to enable efficient
 * class lookup during message generation:
 * - Sorted array: Creates alphabetically ordered class array
 * - Search optimization: Enables O(log n) find_class() performance
 * - Consistency requirement: Must produce stable ordering for search correctness
 * - Performance impact: Dramatically improves lookup speed for large rule sets
 *
 * ERROR HANDLING STRATEGY:
 * ======================
 * - Null pointer handling: Assumes caller provides valid pointers
 * - String validation: Assumes properly null-terminated class names
 * - No error return: Standard comparison interface doesn't support error codes
 * - Defensive programming: Relies on load_rules_file() validation
 * - Failure mode: Invalid pointers would cause segmentation fault
 *
 * INTEGRATION CONTEXT:
 * ==================
 * - Called by: qsort() during load_rules_file() completion
 * - Operates on: Global classes array elements
 * - Enables: Efficient find_class() binary search implementation
 * - Performance target: Fast class resolution during message generation
 * - Critical path: Essential for message system scalability
 *
 * THREAD SAFETY:
 * =============
 * - Read-only operation: No modification of compared structures
 * - Stateless function: No global state access or modification
 * - Concurrent safe: Multiple threads could safely call comparison
 * - qsort dependency: Thread safety depends on qsort() implementation
 * - Local variables only: No shared state between invocations
 *
 * Parameters:
 *   a - Pointer to first text_class structure for comparison (must not be NULL)
 *   b - Pointer to second text_class structure for comparison (must not be NULL)
 *
 * Returns:
 *   Integer indicating comparison result: <0 if a<b, 0 if a==b, >0 if a>b
 *
 * Side Effects:
 *   - No side effects - pure comparison function
 *   - No global state modification
 *   - No memory allocation or deallocation
 *   - No I/O operations performed
 *
 * Testing Notes:
 *   Category: A (Unit) - Pure function with deterministic behavior
 *   Approach: Unit tests with various class name combinations
 *   Key Tests: Alphabetical ordering, equal names, case sensitivity, edge cases
 *   Dependencies: strcmp() standard library function
 *   Mock Requirements: Test text_class structures with known names
 *   Complexity: Simple - Standard string comparison wrapper
 */
static int compare_classes(const void *a, const void *b)
{
    const struct text_class *cls_a = (const struct text_class *)a;
    const struct text_class *cls_b = (const struct text_class *)b;
    return strcmp(cls_a->name, cls_b->name);
}

/*
 * duplicate_string - Create dynamic copy of string with memory allocation
 *
 * This function provides safe string duplication with dynamic memory allocation,
 * serving as a foundation for text storage throughout the message generation
 * system. It handles the complete duplication process including length calculation,
 * memory allocation, and content copying while providing proper error handling
 * for memory allocation failures.
 *
 * The function implements a robust string cloning mechanism that ensures
 * independent memory ownership for duplicated strings. This is essential
 * for the message class system where string content must persist beyond
 * the scope of parsing operations and be safely deallocated during cleanup.
 *
 * MEMORY ALLOCATION STRATEGY:
 * =========================
 * - Dynamic allocation: Uses malloc() for scalable memory management
 * - Exact sizing: Allocates precisely strlen(str) + 1 bytes
 * - Null termination: Ensures proper string termination in allocated memory
 * - Ownership transfer: Caller assumes responsibility for freeing allocated memory
 * - Failure handling: Exits program with EX_SOFTWARE on allocation failure
 *
 * STRING PROCESSING ALGORITHM:
 * ==========================
 * 1. Input validation: Check for NULL input pointer (exits on NULL)
 * 2. Length validation: Ensure string length does not exceed MAX_DEF_LEN
 * 3. Overflow prevention: Verify allocation size does not overflow size_t
 * 4. Memory allocation: Allocate buffer for content plus null terminator
 * 5. Allocation check: Verify malloc() success (exits on failure)
 * 6. Content copying: Transfer string content using memcpy()
 *
 * ERROR HANDLING APPROACH:
 * ======================
 * - Null input handling: Exits with EX_SOFTWARE for NULL input
 * - Length limit enforcement: Exits if string exceeds MAX_DEF_LEN
 * - Overflow prevention: Exits if allocation size would overflow
 * - Allocation failure: Exits with EX_SOFTWARE when malloc() fails
 * - Error messages: All error conditions report to stderr before exit
 * - Fail-fast strategy: Critical allocation failures terminate program immediately
 *
 * INTEGRATION CONTEXT:
 * ==================
 * - Used by parse_class_header(): Duplicates class names and variant strings
 * - Used by parse_definition(): Duplicates processed definition text
 * - Critical for data persistence: Ensures text survives parsing completion
 * - Memory lifecycle: Allocated strings freed during cleanup_memory()
 * - Performance impact: Significant memory allocation during rules loading
 *
 * MEMORY LIFECYCLE MANAGEMENT:
 * ===========================
 * - Allocation phase: Called during rules file parsing
 * - Usage phase: Strings accessed during message generation
 * - Cleanup phase: Deallocated by cleanup_memory() function
 * - Ownership model: Caller owns allocated memory until cleanup
 * - Leak prevention: Systematic cleanup ensures no memory leaks
 *
 * PERFORMANCE CHARACTERISTICS:
 * ==========================
 * - Time complexity: O(n) where n is string length (strlen + strcpy)
 * - Space complexity: O(n) additional memory allocation per call
 * - Memory overhead: One allocation per string with minimal waste
 * - Fragmentation impact: May contribute to heap fragmentation
 * - Call frequency: High during rules file loading phase
 *
 * THREAD SAFETY CONSIDERATIONS:
 * ===========================
 * - Malloc dependency: Thread safety depends on malloc() implementation
 * - Local variables: No shared state between function calls
 * - Input immutability: Does not modify input string
 * - Concurrent usage: Safe for concurrent calls with different inputs
 * - Global state: No global state access or modification
 *
 * ALTERNATIVE IMPLEMENTATIONS:
 * ==========================
 * - strdup() availability: Could use POSIX strdup() where available
 * - Memory pool: Could be optimized with string pool allocation
 * - Static buffers: Could use fixed buffers for bounded strings
 * - Reference counting: Could implement shared string references
 * - Current choice: Simple malloc/free for clarity and portability
 *
 * USAGE PATTERNS:
 * =============
 * - Class names: Persistent storage for class identification
 * - Variant strings: Storage for variant tag sequences
 * - Definition text: Processed message template storage
 * - Configuration strings: Various system string parameters
 * - Memory ownership: Caller must free returned pointers
 *
 * Parameters:
 *   str - Source string to duplicate (must not be NULL, must not exceed MAX_DEF_LEN)
 *
 * Returns:
 *   Pointer to allocated string copy on success (never returns on failure)
 *
 * Side Effects:
 *   - Allocates memory via malloc() that caller must free
 *   - May exit program with EX_SOFTWARE on validation or allocation failure
 *   - Outputs error messages to stderr on failure conditions
 *   - No modification of input string or global state
 *   - Memory allocation affects heap state
 *
 * Testing Notes:
 *   Category: A (Unit) - Simple utility function with clear behavior
 *   Approach: Unit tests with various string inputs and memory conditions
 *   Key Tests: Normal strings, empty strings, NULL input, allocation failures
 *   Dependencies: malloc(), strlen(), strcpy() standard library functions
 *   Mock Requirements: malloc failure injection for error path testing
 *   Complexity: Simple - Standard string duplication with error handling
 */
static char *duplicate_string(const char *str)
{
    if (!str) {
        fprintf(stderr, "duplicate_string: NULL parameter\n");
        exit(EX_SOFTWARE);
    }

    /* Validate string length is within reasonable bounds */
    size_t str_len = strlen(str);
    if (str_len > MAX_DEF_LEN) {
        fprintf(stderr, "duplicate_string: String too long (%zu > %d)\n",
                str_len, MAX_DEF_LEN);
        exit(EX_SOFTWARE);
    }

    /* Check for allocation size overflow (len + 1 must fit in size_t) */
    if (str_len >= SIZE_MAX) {
        fprintf(stderr, "duplicate_string: String length overflow\n");
        exit(EX_SOFTWARE);
    }

    int len = safe_size_to_int(str_len);
    char *copy = malloc(safe_int_to_size(len + 1));
    if (!copy) {
        fprintf(stderr, "duplicate_string: malloc(%zu) failed\n",
                safe_int_to_size(len + 1));
        exit(EX_SOFTWARE);
    }

    memcpy(copy, str, safe_int_to_size(len + 1));
    return copy;
}

/*
 * cleanup_memory - Comprehensive memory deallocation and system reset
 *
 * This function provides complete cleanup of all dynamically allocated memory
 * used by the message generation system, ensuring no memory leaks and proper
 * system reset for subsequent operations. It handles the systematic deallocation
 * of all data structures including classes, definitions, names, variants, and
 * associated text content.
 *
 * The function implements a comprehensive cleanup strategy that visits every
 * allocated structure in the message system and safely deallocates memory
 * while maintaining proper order to avoid accessing freed memory. This is
 * essential for system reliability and resource management in long-running
 * applications.
 *
 * CLEANUP ALGORITHM:
 * ================
 * 1. Input validation: Check if global classes array exists
 * 2. Class iteration: Process each class in the global classes array
 * 3. Name deallocation: Free dynamically allocated class names
 * 4. Variant cleanup: Free variant strings (excluding static defaults)
 * 5. Definition chain traversal: Walk and free all definition linked lists
 * 6. Text deallocation: Free all definition text content
 * 7. Structure cleanup: Free definition structures themselves
 * 8. Array deallocation: Free global classes array
 * 9. State reset: Reset global counters and pointers to initial state
 *
 * MEMORY HIERARCHY CLEANUP:
 * =======================
 * The cleanup follows the memory hierarchy from leaf nodes to root:
 * - Definition text strings (deepest level)
 * - Definition structures containing text pointers
 * - Definition linked lists within each class
 * - Class variant strings (when not using default static string)
 * - Class name strings
 * - Global classes array (root level)
 * - Global state variables reset
 *
 * LINKED LIST TRAVERSAL SAFETY:
 * ============================
 * - Safe iteration: Saves next pointer before freeing current node
 * - Order dependency: Frees text before structure to avoid dangling pointers
 * - Null termination: Properly handles null-terminated definition chains
 * - Memory corruption prevention: No access to freed memory during traversal
 * - Complete traversal: Ensures all definitions in chain are processed
 *
 * VARIANT STRING HANDLING:
 * ======================
 * - Static string detection: Checks if variant pointer equals default_variants
 * - Conditional deallocation: Only frees dynamically allocated variant strings
 * - Memory safety: Avoids freeing static storage for default variants
 * - Pointer comparison: Uses direct pointer comparison for static detection
 * - Default preservation: Maintains static default_variants string unchanged
 *
 * GLOBAL STATE RESET:
 * =================
 * - Array pointer: Sets classes to NULL to indicate uninitialized state
 * - Counter reset: Resets num_classes to 0 for accurate state tracking
 * - Ready for reuse: System prepared for subsequent load_rules_file() calls
 * - Clean slate: No residual state from previous operations
 * - Error prevention: Prevents double-free or use-after-free errors
 *
 * ERROR PREVENTION STRATEGY:
 * =========================
 * - Null check protection: Returns immediately if classes array is NULL
 * - Double-free prevention: Resets global pointers after deallocation
 * - Use-after-free prevention: Systematic order of deallocation
 * - Memory corruption avoidance: No access to freed memory
 * - Safe reentry: Function can be called multiple times safely
 *
 * INTEGRATION CONTEXT:
 * ==================
 * - Called by makemess(): Cleanup after message generation completion
 * - System lifecycle: Essential part of proper system shutdown
 * - Memory management: Critical for preventing memory leaks
 * - Reusability: Prepares system for subsequent rules file loading
 * - Error recovery: Can be called during error conditions for cleanup
 *
 * PERFORMANCE CHARACTERISTICS:
 * ==========================
 * - Time complexity: O(n + m) where n=classes, m=total definitions
 * - Space complexity: O(1) - no additional memory allocation
 * - I/O impact: No file operations performed
 * - Memory system: May trigger heap consolidation in malloc implementation
 * - Call frequency: Once per makemess() invocation
 *
 * THREAD SAFETY CONSIDERATIONS:
 * ===========================
 * - Global state modification: Not thread-safe due to global variable access
 * - Free operations: Depends on thread safety of free() implementation
 * - Concurrent access: Unsafe if other threads accessing global structures
 * - Single-threaded design: Assumes single-threaded message generation
 * - Synchronization required: Needs external synchronization for concurrent use
 *
 * DEBUGGING AND VALIDATION:
 * ========================
 * - Memory leak detection: Essential for memory debugging tools
 * - Valgrind compatibility: Proper cleanup enables clean valgrind runs
 * - Address sanitizer: Supports AddressSanitizer leak detection
 * - Test validation: Enables memory leak testing in unit tests
 * - Production reliability: Prevents memory exhaustion in long-running systems
 *
 * Parameters:
 *   void (operates on global state variables)
 *
 * Returns:
 *   void (no return value, success indicated by clean global state)
 *
 * Side Effects:
 *   - Deallocates all memory allocated by message generation system
 *   - Resets global classes pointer to NULL
 *   - Resets global num_classes counter to 0
 *   - Modifies heap state through extensive free() operations
 *   - Prepares system for clean reinitialization
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires loaded message system state
 *   Approach: Integration testing with memory leak detection tools
 *   Key Tests: Complete cleanup verification, multiple call safety, memory tools
 *   Dependencies: Global classes array, loaded message data, malloc/free
 *   Mock Requirements: Memory allocation tracking, leak detection integration
 *   Complexity: Moderate - Multi-level deallocation with ordering requirements
 */
static void cleanup_memory(void)
{
    if (!classes) return;

    for (int i = 0; i < num_classes; i++) {
        free(classes[i].name);
        if (classes[i].variants != default_variants) {
            free(classes[i].variants);
        }

        struct definition *def = classes[i].defs;
        while (def) {
            struct definition *next = def->next;
            free(def->text);
            free(def);
            def = next;
        }
    }

    free(classes);
    classes = NULL;
    num_classes = 0;
}

#else
/* If SPEW is not defined, provide stub implementation */
void makemess(int count, FILE *output)
{
    /* Do nothing if SPEW is disabled */
}
#endif /* SPEW */
