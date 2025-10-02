#!/bin/bash

# Enhanced test_warnings.sh - Flexible compilation testing script
# Supports variable warning levels, C standards, and individual file testing

# Default values
PHASE=4
SUB=4
NAME=WEXTRA
TYPE=BASELINE
STD="c99"
WARN_LEVEL=2
SINGLE_FILE=""
VERBOSE=0

# TODO: Make this a flag
CC=gcc
#CC=clang

# Usage function
usage() {
    cat << EOF
Usage: $0 [OPTIONS] [FILENAME]

Enhanced compilation testing with flexible warning levels and standards.

OPTIONS:
    -w LEVEL    Warning level (0-10, default: 2)
                0: No warnings
                1: -Wall
                2: -Wall -Wextra
                3: -Wall -Wextra -Wpedantic
                4: Level 3 + -Wformat=2
                5: Level 4 + -Wconversion
                6: Level 5 + -Wsign-conversion
                7: Level 6 + -Wimplicit-fallthrough
                8: Level 7 + -Wstrict-prototypes
                9: Level 8 + -Wshadow -Wmissing-prototypes -Wcast-qual
                10: Level 9 + -fanalyzer -fsanitize=address,undefined (intensive analysis)

    -x STD    C standard (c89, c99, c11, c17, c2x, default: c99)

    -p PHASE    Phase number for output file naming (default: 4)
    -s SUB      Subphase number for output file naming (default: 4)
    -n NAME     Name component for output file (default: WEXTRA)
    -t TYPE     Type component for output file (default: BASELINE)
    -v          Verbose output
    -h          Show this help

FILENAME:
    Optional: Test single file instead of all files
    Example: $0 update.c
    Example: $0 -w 10 -x c2x main.c

Examples:
    $0                           # Test all files with default settings
    $0 -w 1 -x c99            # Test all files with -Wall and C99
    $0 -w 10 update.c           # Intensive analysis on update.c only
    $0 -w 2 -x c2x main.c     # Test main.c with -Wextra and C2x
EOF
}

# Parse command line arguments
while getopts "w:x:p:s:n:t:vh" opt; do
    case $opt in
        w)
            WARN_LEVEL=$OPTARG
            if [[ ! "$WARN_LEVEL" =~ ^[0-9]|10$ ]]; then
                echo "Error: Warning level must be 0-10" >&2
                exit 1
            fi
            ;;
        x)
            case $OPTARG in
                c89|c99|c11|c17|c2x)
                    STD=$OPTARG
                    ;;
                *)
                    echo "Error: Invalid C standard. Use: c89, c99, c11, c17, c2x" >&2
                    exit 1
                    ;;
            esac
            ;;
        p) PHASE=$OPTARG ;;
        s) SUB=$OPTARG ;;
        n) NAME=$OPTARG ;;
        t) TYPE=$OPTARG ;;
        v) VERBOSE=1 ;;
        h) usage; exit 0 ;;
        \?) echo "Invalid option -$OPTARG" >&2; usage; exit 1 ;;
    esac
done

shift $((OPTIND-1))

# Check for single file argument
if [[ $# -eq 1 ]]; then
    SINGLE_FILE=$1
    if [[ ! -f "$SINGLE_FILE" ]]; then
        echo "Error: File '$SINGLE_FILE' not found" >&2
        exit 1
    fi
elif [[ $# -gt 1 ]]; then
    echo "Error: Too many arguments" >&2
    usage
    exit 1
fi

# Build warning flags based on level
case $WARN_LEVEL in
    0) WARN="" ;;
    1) WARN="-Wall" ;;
    2) WARN="-Wall -Wextra" ;;
    3) WARN="-Wall -Wextra -Wpedantic" ;;
    4) WARN="-Wall -Wextra -Wpedantic -Wformat=2" ;;
    5) WARN="-Wall -Wextra -Wpedantic -Wformat=2 -Wconversion" ;;
    6) WARN="-Wall -Wextra -Wpedantic -Wformat=2 -Wconversion -Wsign-conversion" ;;
    7) WARN="-Wall -Wextra -Wpedantic -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough" ;;
    8) WARN="-Wall -Wextra -Wpedantic -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -Wstrict-prototypes" ;;
    9) WARN="-Wall -Wextra -Wpedantic -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -Wstrict-prototypes -Wshadow -Wmissing-prototypes -Wcast-qual" ;;
    10) WARN="-Wall -Wextra -Wpedantic -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -Wstrict-prototypes -Wshadow -Wmissing-prototypes -Wcast-qual -fanalyzer -fsanitize=address,undefined" ;;
    11) WARN="-Weverything"
esac

# Update output file name components based on settings
if [[ -n "$SINGLE_FILE" ]]; then
    # For single files, create descriptive name
    BASENAME=$(basename "$SINGLE_FILE" .c)
    NAME="${BASENAME}_W${WARN_LEVEL}_${STD}"
    TYPE="SINGLE"
fi

OUTFILE=_modernization/claude/scratch/PHASE_${PHASE}.${SUB}_${NAME}_${TYPE}.txt
TEMPFILE=$(mktemp /tmp/my-app-data.XXXXXX)

# Phase 5 Clean Architecture: Configuration via headers, not compiler flags
# All DEFAULTDIR, EXEDIR, LOGIN now defined in config.h via header.h inclusion
DFLAGS="-D_POSIX_C_SOURCE=200809L"

# Define compilation flags for each file type
# Phase 5 Clean Architecture: Configuration via headers, not compiler flags
# All DEFAULTDIR, EXEDIR, LOGIN now defined in config.h via header.h inclusion
DFLAGS="-D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE -DDEFAULTDIR=\"/projects/conquer/lib\" -DEXEDIR=\"/projects/conquer/bin\" -DLOGIN=\"ssmoogen\""
 # Define compilation flags for each file type
ADMIN_FLAGS="-O2 -g -std=${STD} ${DFLAGS} -DADMIN -DCONQUER ${WARN}"

GAME_FLAGS="-O2 -g -std=${STD} ${DFLAGS} -DCONQUER ${WARN}"

PSMAP_FLAGS="-O2 -g -std=${STD} ${DFLAGS} -DPSFILE=\"/projects/conquer/bin/psmap.ps\" -DLETTER ${WARN}"


# Verbose output function
verbose_echo() {
    if [[ $VERBOSE -eq 1 ]]; then
        echo "$@"
    fi
}

# Function to determine file compilation type
get_file_type() {
    local file=$1
    case $file in
        admin.c|combat.c|spew.c|newlogin.c|update.c|npc.c|randeven.c|makeworl.c)
            echo "ADMIN"
            ;;
        check.c)
            echo "SHARED"
            ;;
        commands.c|main.c|forms.c|move.c|reports.c|display.c|extcmds.c)
            echo "GAME"
            ;;
        cexecute.c|io.c|misc.c|navy.c|magic.c|data.c|trade.c|safe_system.c)
            echo "DUAL"
            ;;
        psmap.c)
            echo "PSMAP"
            ;;
        *)
            echo "UNKNOWN"
            ;;
    esac
}

# Function to test a single file
test_single_file() {
    local file=$1
    local file_type=$(get_file_type "$file")

    verbose_echo "Testing $file (type: $file_type)"

    case $file_type in
        ADMIN|SHARED)
            echo "--- Testing $file (Admin mode) ---" >> ${OUTFILE}
            ${CC} $ADMIN_FLAGS -c "$file" -o /tmp/test.o 2>&1 | tee ${TEMPFILE}
            cat ${TEMPFILE} >> ${OUTFILE}
            ;;
        GAME)
            echo "--- Testing $file (Game mode) ---" >> ${OUTFILE}
            ${CC} $GAME_FLAGS -c "$file" -o /tmp/test.o 2>&1 | tee ${TEMPFILE}
            cat ${TEMPFILE} >> ${OUTFILE}
            ;;
        DUAL)
            echo "--- Testing $file (Admin mode) ---" >> ${OUTFILE}
            ${CC} $ADMIN_FLAGS -c "$file" -o /tmp/test.o 2>&1 | tee ${TEMPFILE}
            cat ${TEMPFILE} >> ${OUTFILE}
            echo "" >> ${OUTFILE}

            echo "--- Testing $file (Game mode) ---" >> ${OUTFILE}
            ${CC} $GAME_FLAGS -c "$file" -o /tmp/test.o 2>&1 | tee ${TEMPFILE}
            cat ${TEMPFILE} >> ${OUTFILE}
            ;;
        PSMAP)
            echo "--- Testing $file (PostScript mode) ---" >> ${OUTFILE}
            ${CC} $PSMAP_FLAGS -c "$file" -o /tmp/test.o 2>&1 | tee ${TEMPFILE}
            cat ${TEMPFILE} >> ${OUTFILE}
            ;;
        UNKNOWN)
            echo "--- Testing $file (Unknown type, using ADMIN flags) ---" >> ${OUTFILE}
            ${CC} $ADMIN_FLAGS -c "$file" -o /tmp/test.o 2>&1 | tee ${TEMPFILE}
            cat ${TEMPFILE} >> ${OUTFILE}
            ;;
    esac
    echo "" >> ${OUTFILE}
}

# Create header for output file
if [[ -n "$SINGLE_FILE" ]]; then
    echo "=== SINGLE FILE TEST: $SINGLE_FILE ===" > ${OUTFILE}
    echo "Warning Level: $WARN_LEVEL ($WARN)" >> ${OUTFILE}
    echo "C Standard: $STD" >> ${OUTFILE}
    echo "Date: $(date)" >> ${OUTFILE}
    echo "" >> ${OUTFILE}

    test_single_file "$SINGLE_FILE"

    echo "=== SINGLE FILE TEST COMPLETE ===" >> ${OUTFILE}
    echo "File: $SINGLE_FILE" >> ${OUTFILE}
    echo "Report: $OUTFILE" >> ${OUTFILE}
else
    echo "=== COMPREHENSIVE COMPILATION TEST ===" > ${OUTFILE}
    echo "Warning Level: $WARN_LEVEL ($WARN)" >> ${OUTFILE}
    echo "C Standard: $STD" >> ${OUTFILE}
    echo "Date: $(date)" >> ${OUTFILE}
    echo "" >> ${OUTFILE}

    echo "=== ADMIN-ONLY FILES (8 files) ===" >> ${OUTFILE}
    for file in admin.c combat.c spew.c newlogin.c update.c npc.c randeven.c makeworl.c; do
        test_single_file "$file"
    done

    echo "=== SHARED FILES (1 file) ===" >> ${OUTFILE}
    test_single_file "check.c"

    echo "=== GAME-ONLY FILES (7 files) ===" >> ${OUTFILE}
    for file in commands.c main.c forms.c move.c reports.c display.c extcmds.c; do
        test_single_file "$file"
    done

    echo "=== DUAL-COMPILED FILES (7 files) ===" >> ${OUTFILE}
    for file in cexecute.c io.c misc.c navy.c magic.c data.c trade.c safe_system.c; do
        test_single_file "$file"
    done

    echo "=== POSTSCRIPT UTILITY FILES (1 file) ===" >> ${OUTFILE}
    test_single_file "psmap.c"

    echo "=== COMPREHENSIVE TEST COMPLETE ===" >> ${OUTFILE}
    echo "Check report for warning counts and compilation status." >> ${OUTFILE}
fi

# Display results
verbose_echo "Test complete. Results written to: $OUTFILE"
if [[ $VERBOSE -eq 1 ]] || [[ -n "$SINGLE_FILE" ]]; then
    echo ""
    echo "=== COMPILATION RESULTS ==="
    cat "$OUTFILE"
fi

rm -f ${TEMPFILE}
