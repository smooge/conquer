/*
 * sort.c - Data sorting utilities
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
#include "safe_convert.h"

/* system definitions just in case */
#ifndef FALSE
#define FALSE (0)
#define TRUE (1)
#endif /* FALSE */

/* system exit definitions from sysexits.h on BSD machines */
#ifndef EX_OK
#define EX_OK 0 /* successful termination */
#define EX_USAGE 64 /* invalid command line format */
#define EX_NOINPUT 66 /* could not open input file */
#define EX_SOFTWARE 70 /* software error; couldn't malloc */
#define EX_CANTCREAT 73 /* could not create output file */
#endif /* EX_OK */

/* number of characters to compare by default */
#define DEFAULT_COMP 2
#define MAX_STR 200

/* structure for holding a line of information */
typedef struct holder {
    char *line;
    struct holder *next;
} L_DATA, *L_PTR;

/* Function prototypes */
static int get_line(char data[]);
static void send_out(void);
static void place(char data[]);
static int comp_line(char *a, char *b);
static L_PTR build_node(char data[], L_PTR nptr);

/* pointer to head of list of lines */
L_PTR head;

/* usage string */
static char usage[] = "Usage: %s [-num -h] [infile] [outfile]\n\
	-h		display this screen\n\
	-i		treat upper and lower case characters as equivalent\n\
	-num		Sort on first num characters\n";

/* number of characters for comparison */
int compnum = DEFAULT_COMP;

/* flag for folding upper and lowercase characters */
int iflag = FALSE;

/* file pointers to infile and outfile */
FILE *infile, *outfile;

/* macros */
#define UPPER(x) ((islower(x)) ? (toupper(x)) : (x))

/*
 * main - Text sorting utility with configurable options
 *
 * Command-line text sorting program that reads input lines and outputs them
 * in sorted order based on the first N characters. Supports case-insensitive
 * sorting and flexible input/output file handling. Uses insertion sort with
 * linked list data structure for in-memory sorting.
 *
 * Parameters:
 *   argc - Number of command-line arguments
 *   argv - Array of command-line argument strings
 *          Format: program [-num -i -h] [infile] [outfile]
 *          -num: Sort on first num characters (default 2)
 *          -i: Case-insensitive comparison
 *          -h: Display help and exit
 *
 * Returns:
 *   EX_OK (0) - Successful completion
 *   EX_USAGE (64) - Invalid command line arguments
 *   EX_NOINPUT (66) - Cannot open input file
 *   EX_CANTCREAT (73) - Cannot create output file
 *   EX_SOFTWARE (70) - Memory allocation failure
 *
 * Side Effects:
 *   - Reads from stdin or specified input file
 *   - Writes to stdout or specified output file
 *   - Allocates memory for linked list of input lines
 *   - Sets global variables: compnum, iflag, infile, outfile
 *   - May exit program with error codes on failure
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires file I/O and command-line processing
 *   Approach: Integration testing with mock files and argument arrays
 *   Key Tests: Valid/invalid arguments, file operations, sorting accuracy
 *   Dependencies: File system access, memory allocation, global variables
 *   Mock Requirements: File I/O operations, memory allocation functions
 *   Complexity: Complex - Multiple responsibilities and error paths
 *
 * Notes:
 *   - Main orchestrator function handling argument parsing and program flow
 *   - Uses global variables for configuration (not thread-safe)
 *   - Memory allocated by build_node() is never freed (acceptable for utility)
 *   - Processes input incrementally to handle large files efficiently
 * @last_documented: 2025-09-20
 */
int main(int argc, char *argv[]) {
    /* declare temporary variables and functions */
    int i, j, num_args = 0, l, innum = 0, outnum = 0;
    int get_line(char data[]);
    void place(char data[]);
    void send_out(void);

    /* This needs to be done at run-time now */
    infile = stdin;
    outfile = stdout;

    /* input string */
    char data[MAX_STR];

    /* parse command line arguments */
    for (i = 1; i < argc; i++) {
        /* process non-switch statements */
        if (argv[i][0] != '-') {
            num_args++;
            switch (num_args) {
                case 1:
                    /* do not open unless arguments are valid */
                    innum = i;
                    break;
                case 2:
                    /* do not open unless arguments are valid */
                    outnum = i;
                    break;
                default:
                    fprintf(stderr, "%s: Too many parameters\n", argv[0]);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
                    fprintf(stderr, usage, argv[0]);
#pragma GCC diagnostic pop
                    exit(EX_USAGE);
                    break;
            }
            continue;
        }

        /* process switch statements */
        l = safe_size_to_int(strlen(argv[i]));
        for (j = 1; j < l; j++) {
            switch (argv[i][j]) {
                case 'h':
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
                    fprintf(stderr, usage, argv[0]);
#pragma GCC diagnostic pop
                    exit(EX_OK);
                    break;
                case 'i':
                    iflag = TRUE;
                    break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    /* assign new comparison number */
                    compnum = 0;
                    for (; j < l && argv[i][j] >= '0' && argv[i][j] <= '9'; j++) {
                        compnum *= 10;
                        compnum += argv[i][j] - '0';
                    }
                    /* compensate for increment on exit */
                    j--;
                    break;
                default:
                    fprintf(stderr, "%s: invalid option '%c' in <%s>\n", argv[0], argv[i][j],
                            argv[i]);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
                    fprintf(stderr, usage, argv[0]);
#pragma GCC diagnostic pop
                    exit(EX_USAGE);
                    break;
            }
        }
    }

    /* open input file now */
    if (innum != 0) {
        if ((infile = fopen(argv[innum], "r")) == (FILE *)NULL) {
            fprintf(stderr, "%s: can't open file <%s> for input\n", argv[0], argv[innum]);
            exit(EX_NOINPUT);
        }
    }

    /* ======= main processing loop ======== */

    while (!feof(infile)) {
        /* remove any lines less than sort characters */
        if (get_line(data) > compnum) {
            place(data);
        }
    }

    /* ==== end of main processing loop ==== */

    /* close input file if not stdin */
    if (infile != stdin) {
        (void)fclose(infile);
    }

    /* open output file now */
    if (outnum != 0) {
        if ((outfile = fopen(argv[outnum], "w")) == (FILE *)NULL) {
            fprintf(stderr, "%s: cannot open file <%s> for output\n", argv[0], argv[outnum]);
            exit(EX_CANTCREAT);
        }
    }

    send_out();

    /* close output file if not stdout */
    if (outfile != stdout) {
        (void)fclose(outfile);
    }

    exit(EX_OK);
}

/*
 * get_line - Read and filter input line from file stream
 *
 * Reads characters from the global input file stream until newline or EOF,
 * filtering out invalid characters and enforcing maximum line length.
 * Only accepts printable ASCII characters, spaces, and tabs while
 * discarding exceptionally long lines to prevent buffer overflow.
 *
 * Parameters:
 *   data - Character array buffer to store filtered line (size MAX_STR)
 *          Must be allocated by caller with sufficient space
 *
 * Returns:
 *   Number of valid characters read and stored in data buffer
 *   0 for empty lines or lines with only invalid characters
 *   Always null-terminates the output string
 *
 * Side Effects:
 *   - Reads from global infile stream until newline or EOF
 *   - Modifies data array with filtered input characters
 *   - Advances file pointer position in infile
 *   - Discards characters beyond MAX_STR-1 limit
 *
 * Testing Notes:
 *   Category: A (Unit) - Testable with mock file streams
 *   Approach: Unit tests with controlled input streams and buffers
 *   Key Tests: Valid chars, invalid chars, long lines, EOF conditions
 *   Dependencies: Global infile stream, MAX_STR constant
 *   Mock Requirements: FILE stream operations (getc, feof)
 *   Complexity: Moderate - Character filtering with boundary conditions
 *
 * Notes:
 *   - Silently truncates lines longer than MAX_STR-1 characters
 *   - Character filtering: printable ASCII (0x20-0x7E), tabs, spaces
 *   - Input validation prevents buffer overflows in downstream processing
 *   - Essential preprocessing step for reliable sorting operations
 */
static int get_line(char data[]) {
    int in, ch;

    /* discard exceptionally long lines */
    for (in = 0; !feof(infile) && (in < MAX_STR - 1) && ((ch = getc(infile)) != '\n');) {
        /* copy valid input into data */
        if ((ch == '\t') || (ch == ' ') || ((ch >= ' ') && (ch <= '~'))) {
            data[in] = safe_int_to_char(ch);
            in++;
        }
    }
    /* end string */
    data[in] = '\0';

    return (in);
}

/*
 * send_out - Output sorted lines to destination file stream
 *
 * Traverses the sorted linked list of lines and outputs each line to the
 * global output file stream. Provides the final output phase of the sorting
 * process, writing all accumulated and sorted lines in order.
 *
 * Parameters:
 *   None (uses global variables)
 *
 * Returns:
 *   None (void function)
 *
 * Side Effects:
 *   - Writes all lines from sorted linked list to global outfile stream
 *   - Adds newline character after each line for proper formatting
 *   - Traverses linked list from head to end without modification
 *   - May cause I/O errors if outfile stream is invalid
 *
 * Testing Notes:
 *   Category: A (Unit) - Testable with mock linked list and output stream
 *   Approach: Unit tests with controlled linked list and mock FILE streams
 *   Key Tests: Empty list, single item, multiple items, I/O error handling
 *   Dependencies: Global head pointer, global outfile stream
 *   Mock Requirements: FILE stream operations (fprintf), linked list nodes
 *   Complexity: Simple - Straightforward linked list traversal and output
 *
 * Notes:
 *   - Assumes linked list is already properly sorted by place() function
 *   - No error checking on fprintf operations (relies on system buffering)
 *   - Memory allocated for linked list nodes is not freed (utility exits)
 *   - Essential final phase of sorting pipeline after all input processed
 */
static void send_out(void) {
    L_PTR temp = head;

    while (temp != (L_PTR)NULL) {
        fprintf(outfile, "%s\n", temp->line);
        temp = temp->next;
    }
}

/*
 * place - Insert line into sorted linked list using insertion sort
 *
 * Implements insertion sort algorithm by finding the correct position for
 * a new line in the existing sorted linked list and inserting it there.
 * Maintains sorted order throughout the input processing phase, enabling
 * efficient incremental sorting as lines are read.
 *
 * Parameters:
 *   data - Input line string to be inserted into sorted list
 *          Must be null-terminated string with valid content
 *
 * Returns:
 *   None (void function)
 *
 * Side Effects:
 *   - Modifies global linked list by inserting new node
 *   - Updates head pointer if inserting at beginning of list
 *   - Allocates memory for new node via build_node() function
 *   - Maintains sorted order of entire linked list
 *
 * Testing Notes:
 *   Category: A (Unit) - Testable with controlled input and list states
 *   Approach: Unit tests with various list states and input combinations
 *   Key Tests: Empty list, insert at start, middle, end, duplicate entries
 *   Dependencies: Global head pointer, comp_line(), build_node()
 *   Mock Requirements: String comparison, memory allocation, linked list
 *   Complexity: Moderate - Insertion sort logic with linked list manipulation
 *
 * Notes:
 *   - Uses comp_line() for configurable string comparison (case, length)
 *   - Insertion sort has O(n) average case for partially sorted data
 *   - Memory allocation handled by build_node() with error checking
 *   - Core sorting algorithm that maintains list order incrementally
 */
static void place(char data[]) {
    L_PTR temp;

    /* find location for placing input */
    if (head == (L_PTR)NULL) {
        /* begin list properly */
        head = build_node(data, (L_PTR)NULL);
    } else if (comp_line(head->line, data) == 1) {
        /* add to beginning of list */
        head = build_node(data, head);
    } else {
        /* otherwise add in proper position */
        temp = head;
        while ((temp->next != (L_PTR)NULL) && (comp_line(temp->next->line, data) != 1)) {
            temp = temp->next;
        }
        temp->next = build_node(data, temp->next);
    }
}

/*
 * comp_line - Compare two strings with configurable options
 *
 * Compares two strings character by character up to the specified number
 * of characters (compnum) with optional case-insensitive comparison.
 * Provides the core comparison logic for the sorting algorithm with
 * configurable behavior based on global flags.
 *
 * Parameters:
 *   a - First string to compare (null-terminated)
 *   b - Second string to compare (null-terminated)
 *
 * Returns:
 *   -1 if string a precedes string b in sort order
 *    0 if strings are equal within comparison parameters
 *    1 if string a follows string b in sort order
 *
 * Side Effects:
 *   - Reads global compnum variable for character count limit
 *   - Reads global iflag variable for case sensitivity mode
 *   - No modification of input strings or global state
 *
 * Testing Notes:
 *   Category: A (Unit) - Highly testable with controlled string inputs
 *   Approach: Unit tests with various string combinations and flag states
 *   Key Tests: Equal strings, different cases, length variations, edge cases
 *   Dependencies: Global compnum, iflag variables, UPPER macro
 *   Mock Requirements: None (pure function with global config)
 *   Complexity: Simple - Straightforward string comparison with options
 *
 * Notes:
 *   - Honors global compnum setting for partial string comparison
 *   - Case sensitivity controlled by global iflag (TRUE = ignore case)
 *   - Stops comparison at first null character in either string
 *   - Essential component for customizable sorting behavior
 */
static int comp_line(char *a, char *b) {
    int i;

    for (i = 0; i < compnum; i++) {
        /* do checks */
        if (iflag) {
            if (UPPER(a[i]) < UPPER(b[i]))
                return (-1);
            if (UPPER(b[i]) < UPPER(a[i]))
                return (1);
        } else {
            if (a[i] < b[i])
                return (-1);
            if (b[i] < a[i])
                return (1);
        }
        if (a[i] == '\0')
            return (0);
    }
    return (0);
}

/*
 * build_node - Create new linked list node with string data
 *
 * Allocates memory for a new linked list node and copies the provided
 * string data into it. Handles both structure and string memory allocation
 * with comprehensive error checking. Sets up proper linkage for insertion
 * into the sorted linked list.
 *
 * Validates input parameters and enforces maximum string length limits to
 * prevent buffer overflows and integer overflow vulnerabilities.
 *
 * Parameters:
 *   data - String data to store in the new node (must not be NULL, max MAX_STR bytes)
 *   nptr - Pointer to the next node in the linked list (may be NULL)
 *
 * Returns:
 *   Pointer to newly allocated and initialized L_DATA node
 *   Program exits with EX_SOFTWARE if validation or allocation fails
 *
 * Side Effects:
 *   - Validates data parameter is not NULL
 *   - Checks string length against MAX_STR limit
 *   - Prevents integer overflow in allocation size calculation
 *   - Allocates memory for L_DATA structure using malloc()
 *   - Allocates memory for string copy using malloc()
 *   - Copies input string data to allocated memory
 *   - Sets next pointer to provided nptr value
 *   - Exits program on validation or allocation failure with error message
 *
 * Validation Checks:
 *   - Rejects NULL data parameter
 *   - Enforces maximum string length (MAX_STR = 200 bytes)
 *   - Prevents integer overflow in allocation size calculation
 *
 * Memory Management:
 *   - Allocates L_DATA structure using malloc() (sizeof(L_DATA) bytes)
 *   - Allocates string storage using malloc() (strlen(data) + 1 bytes)
 *   - **Memory is never freed** - acceptable for short-lived utility program
 *   - Program exits on allocation failure (no memory leak possible)
 *   - Total allocation per node: ~16 bytes + string length
 *
 * Testing Notes:
 *   Category: A (Unit) - Testable with controlled inputs and mock allocation
 *   Approach: Unit tests with various string lengths, NULL checks, and mock malloc
 *   Key Tests: Normal allocation, NULL parameter, oversized strings, allocation failures
 *   Dependencies: malloc(), strlen(), memcpy(), exit()
 *   Mock Requirements: Memory allocation functions, error handling
 *   Complexity: Simple - Straightforward allocation with validation and error checking
 *
 * Notes:
 *   - Memory allocated is never freed (acceptable for short-lived utility)
 *   - Uses exit() on validation or allocation failure rather than returning error code
 *   - Essential memory management component for linked list construction
 *   - Copies string data to prevent external modification issues
 *   - Bounds checking added in Phase 8.4.3.2 to prevent overflow vulnerabilities
 */
static L_PTR build_node(char data[], L_PTR nptr) {
    L_PTR temp;

    /* Validate input parameter */
    if (data == NULL) {
        fprintf(stderr, "build_node: NULL data parameter\n");
        exit(EX_SOFTWARE);
    }

    /* Check string length and bounds */
    size_t data_len = strlen(data);
    if (data_len > MAX_STR) {
        fprintf(stderr, "build_node: String too long (%zu > %d)\n", data_len, MAX_STR);
        exit(EX_SOFTWARE);
    }

    /* Check for overflow in allocation size calculation */
    if (data_len >= SIZE_MAX - 1) {
        fprintf(stderr, "build_node: String length overflow\n");
        exit(EX_SOFTWARE);
    }

    /* build the memory space */
    if ((temp = (L_PTR)malloc(sizeof(L_DATA))) == (L_PTR)NULL) {
        fprintf(stderr, "Error in creating structure memory!\n");
        exit(EX_SOFTWARE);
    }
    if ((temp->line = (char *)malloc((strlen(data) + 1) * sizeof(char))) == NULL) {
        fprintf(stderr, "Error in creating data memory!\n");
        exit(EX_SOFTWARE);
    }

    /* assign the values */
    memcpy(temp->line, data, data_len);
    temp->line[data_len] = '\0';
    temp->next = nptr;
    return (temp);
}
