#!/bin/bash

# test_char_signed.sh - x86_64 Char Signedness Simulation Testing
#
# Purpose: Test code with -fsigned-char to simulate x86_64 behavior on aarch64
#
# Background:
#   - aarch64: 'char' is UNSIGNED by default (CHAR_MIN = 0)
#   - x86_64:  'char' is SIGNED by default (CHAR_MIN = -128)
#   - This script forces signed char to reveal portability issues
#
# Critical Issues Detected:
#   - Array indexing with char (negative index → undefined behavior)
#   - ctype.h functions with char (negative → undefined behavior)
#   - Calculations assuming unsigned range (0-255)
#
# Based on: test_warnings.sh (Phase 4 warning analysis)
# Created: Phase 10.1 (Char Type Audit and Declaration Refactoring)
# GitHub Issue: #10 (Char Signedness Portability)

# Default values
PHASE=10
SUB=1
NAME=CHAR_SIGNED
TYPE=BASELINE
STD="c2x"
WARN_LEVEL=9
SINGLE_FILE=""
VERBOSE=0
CC=gcc

# Usage function
usage() {
    cat << EOF
Usage: $0 [OPTIONS] [FILENAME]

x86_64 char signedness simulation testing with -fsigned-char flag.

This script forces 'char' to be signed (as on x86_64) to reveal portability
issues when running on aarch64 (where 'char' is unsigned by default).

OPTIONS:
    -w LEVEL    Warning level (0-11, default: 9)
                0: No warnings
                1: -Wall
                2: -Wall -Wextra
                3: -Wall -Wextra -Wpedantic
                4: Level 3 + -Wformat=2
                5: Level 4 + -Wconversion
                6: Level 5 + -Wsign-conversion
                7: Level 6 + -Wimplicit-fallthrough
                8: Level 7 + -Wstrict-prototypes -Wstringop-truncation
                9: Level 8 + -Wshadow -Wmissing-prototypes -Wcast-qual
                10: Level 9 + -fanalyzer (gcc) or extra warnings (clang)
                11: Level 10 + -Weverything (clang only)

    -c COMPILER Compiler to use (gcc or clang, default: gcc)

    -x STD      C standard (c89, c99, c11, c17, c2x, default: c2x)

    -p PHASE    Phase number for output file naming (default: 10)
    -s SUB      Subphase number for output file naming (default: 1)
    -n NAME     Name component for output file (default: CHAR_SIGNED)
    -t TYPE     Type component for output file (default: BASELINE)
    -v          Verbose output
    -h          Show this help

FILENAME:
    Optional: Test single file instead of all files
    Example: $0 misc.c          # Test array indexing issue
    Example: $0 spew.c          # Test ctype.h usage
    Example: $0 update.c        # Test calculation issues

Examples:
    $0                          # Baseline test (all files, c2x, warn 9)
    $0 -t FINAL                 # Final test after fixes
    $0 -w 10 misc.c             # Intensive analysis on misc.c
    $0 -x c99 -w 8              # Test with C99 standard

Common Testing Workflow:
    $0 -t BASELINE              # Before Phase 10.1 fixes
    # ... make char signedness fixes ...
    $0 -t FINAL                 # After Phase 10.1 fixes
    # Compare BASELINE vs FINAL for improvements

High-Risk Files (Priority Testing):
    misc.c      Array indexing with char (lines 1805-1838)
    spew.c      ctype.h usage with char (line 1207)
    update.c    Calculations with char (lines 1628-1638)
    forms.c     User input with char (lines 837, 894-896)
    commands.c  Navy calculations (lines 778, 944, 958)
    reports.c   Navy calculations (lines 1210-1212)
EOF
}

# Parse command line arguments
while getopts "w:c:x:p:s:n:t:vh" opt; do
    case $opt in
        w)
            WARN_LEVEL=$OPTARG
            if [[ ! "$WARN_LEVEL" =~ ^([0-9]|1[01])$ ]]; then
                echo "Error: Warning level must be 0-11" >&2
                exit 1
            fi
            ;;
        c)
            case $OPTARG in
                gcc|clang)
                    CC=$OPTARG
                    ;;
                *)
                    echo "Error: Invalid compiler. Use: gcc, clang" >&2
                    exit 1
                    ;;
            esac
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

# Build warning flags based on level and compiler
build_warning_flags() {
    case $WARN_LEVEL in
        0) WARN="" ;;
        1) WARN="-Wall" ;;
        2) WARN="-Wall -Wextra" ;;
        3) WARN="-Wall -Wextra -Wpedantic" ;;
        4) WARN="-Wall -Wextra -Wpedantic -Wformat=2" ;;
        5) WARN="-Wall -Wextra -Wpedantic -Wformat=2 -Wconversion" ;;
        6) WARN="-Wall -Wextra -Wpedantic -Wformat=2 -Wconversion -Wsign-conversion" ;;
        7) WARN="-Wall -Wextra -Wpedantic -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough" ;;
        8)
            WARN="-Wall -Wextra -Wpedantic -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough"
            if [[ "$CC" == "gcc" ]]; then
                WARN="$WARN -Wstrict-prototypes -Wstringop-truncation"
            else
                WARN="$WARN -Wstrict-prototypes"
            fi
            ;;
        9)
            WARN="-Wall -Wextra -Wpedantic -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -Wshadow -Wmissing-prototypes -Wcast-qual"
            if [[ "$CC" == "gcc" ]]; then
                WARN="$WARN -Wstrict-prototypes -Wstringop-truncation"
            else
                WARN="$WARN -Wstrict-prototypes"
            fi
            ;;
        10)
            WARN="-Wall -Wextra -Wpedantic -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -Wshadow -Wmissing-prototypes -Wcast-qual"
            if [[ "$CC" == "gcc" ]]; then
                WARN="$WARN -Wstrict-prototypes -Wstringop-truncation -fanalyzer -fsanitize=address,undefined"
            else
                WARN="$WARN -Wstrict-prototypes -Wunused-parameter -Wunused-variable -Warray-bounds"
            fi
            ;;
        11)
            if [[ "$CC" == "clang" ]]; then
                WARN="-Weverything"
            else
                echo "Error: Warning level 11 (-Weverything) is only available with clang" >&2
                exit 1
            fi
            ;;
    esac
}

# Build the warning flags
build_warning_flags

# Update output file name components based on settings
if [[ -n "$SINGLE_FILE" ]]; then
    # For single files, create descriptive name
    BASENAME=$(basename "$SINGLE_FILE" .c)
    NAME="CHAR_SIGNED_${BASENAME}_${CC}_W${WARN_LEVEL}_${STD}"
    TYPE="SINGLE"
else
    # For comprehensive tests, include compiler in name
    NAME="CHAR_SIGNED_${CC}_W${WARN_LEVEL}"
fi

OUTFILE=_modernization/claude/reports/CHAR_SIGNED_TEST_${TYPE}.txt
TEMPFILE=$(mktemp /tmp/char-signed-test.XXXXXX)

# POSIX and system feature flags
DFLAGS="-D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE -DDEFAULTDIR=\"/projects/conquer/lib\" -DEXEDIR=\"/projects/conquer/bin\" -DLOGIN=\"ssmoogen\""

# ⭐ CRITICAL: Force signed char to simulate x86_64 behavior
CHAR_FLAGS="-fsigned-char"

# Define compilation flags for each file type
# ⭐ All flags include ${CHAR_FLAGS} to force signed char
ADMIN_FLAGS="-O2 -g -std=${STD} ${DFLAGS} ${CHAR_FLAGS} -DADMIN -DCONQUER ${WARN}"

GAME_FLAGS="-O2 -g -std=${STD} ${DFLAGS} ${CHAR_FLAGS} -DCONQUER ${WARN}"

PSMAP_FLAGS="-O2 -g -std=${STD} ${DFLAGS} ${CHAR_FLAGS} -DPSFILE=\"/projects/conquer/bin/psmap.ps\" -DLETTER ${WARN}"


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

    verbose_echo "Testing $file (type: $file_type) with -fsigned-char"

    case $file_type in
        ADMIN|SHARED)
            echo "--- Testing $file (Admin mode, -fsigned-char) ---" >> ${OUTFILE}
            ${CC} $ADMIN_FLAGS -c "$file" -o /tmp/test.o 2>&1 | tee ${TEMPFILE}
            cat ${TEMPFILE} >> ${OUTFILE}
            ;;
        GAME)
            echo "--- Testing $file (Game mode, -fsigned-char) ---" >> ${OUTFILE}
            ${CC} $GAME_FLAGS -c "$file" -o /tmp/test.o 2>&1 | tee ${TEMPFILE}
            cat ${TEMPFILE} >> ${OUTFILE}
            ;;
        DUAL)
            echo "--- Testing $file (Admin mode, -fsigned-char) ---" >> ${OUTFILE}
            ${CC} $ADMIN_FLAGS -c "$file" -o /tmp/test.o 2>&1 | tee ${TEMPFILE}
            cat ${TEMPFILE} >> ${OUTFILE}
            echo "" >> ${OUTFILE}

            echo "--- Testing $file (Game mode, -fsigned-char) ---" >> ${OUTFILE}
            ${CC} $GAME_FLAGS -c "$file" -o /tmp/test.o 2>&1 | tee ${TEMPFILE}
            cat ${TEMPFILE} >> ${OUTFILE}
            ;;
        PSMAP)
            echo "--- Testing $file (PostScript mode, -fsigned-char) ---" >> ${OUTFILE}
            ${CC} $PSMAP_FLAGS -c "$file" -o /tmp/test.o 2>&1 | tee ${TEMPFILE}
            cat ${TEMPFILE} >> ${OUTFILE}
            ;;
        UNKNOWN)
            echo "--- Testing $file (Unknown type, using ADMIN flags, -fsigned-char) ---" >> ${OUTFILE}
            ${CC} $ADMIN_FLAGS -c "$file" -o /tmp/test.o 2>&1 | tee ${TEMPFILE}
            cat ${TEMPFILE} >> ${OUTFILE}
            ;;
    esac
    echo "" >> ${OUTFILE}
}

# Create header for output file
if [[ -n "$SINGLE_FILE" ]]; then
    echo "=== CHAR SIGNEDNESS TEST (SINGLE FILE): $SINGLE_FILE ===" > ${OUTFILE}
    echo "Purpose: Simulate x86_64 behavior with -fsigned-char" >> ${OUTFILE}
    echo "Platform: aarch64 (char is unsigned) → x86_64 simulation (char is signed)" >> ${OUTFILE}
    echo "Compiler: $CC" >> ${OUTFILE}
    echo "Char Flag: -fsigned-char (⭐ CRITICAL)" >> ${OUTFILE}
    echo "Warning Level: $WARN_LEVEL ($WARN)" >> ${OUTFILE}
    echo "C Standard: $STD" >> ${OUTFILE}
    echo "Date: $(date)" >> ${OUTFILE}
    echo "" >> ${OUTFILE}

    test_single_file "$SINGLE_FILE"

    echo "=== SINGLE FILE TEST COMPLETE ===" >> ${OUTFILE}
    echo "File: $SINGLE_FILE" >> ${OUTFILE}
    echo "Report: $OUTFILE" >> ${OUTFILE}
else
    echo "=== CHAR SIGNEDNESS COMPREHENSIVE TEST ===" > ${OUTFILE}
    echo "Purpose: Simulate x86_64 behavior with -fsigned-char" >> ${OUTFILE}
    echo "Platform: aarch64 (char is unsigned) → x86_64 simulation (char is signed)" >> ${OUTFILE}
    echo "Compiler: $CC" >> ${OUTFILE}
    echo "Char Flag: -fsigned-char (⭐ CRITICAL)" >> ${OUTFILE}
    echo "Warning Level: $WARN_LEVEL ($WARN)" >> ${OUTFILE}
    echo "C Standard: $STD" >> ${OUTFILE}
    echo "Date: $(date)" >> ${OUTFILE}
    echo "" >> ${OUTFILE}
    echo "High-Risk Files (array indexing, ctype.h, calculations):" >> ${OUTFILE}
    echo "  - misc.c (lines 1805-1838)" >> ${OUTFILE}
    echo "  - spew.c (line 1207)" >> ${OUTFILE}
    echo "  - update.c (lines 1628-1638)" >> ${OUTFILE}
    echo "  - forms.c (lines 837, 894-896)" >> ${OUTFILE}
    echo "  - commands.c (lines 778, 944, 958)" >> ${OUTFILE}
    echo "  - reports.c (lines 1210-1212)" >> ${OUTFILE}
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

    echo "=== DUAL-COMPILED FILES (8 files) ===" >> ${OUTFILE}
    for file in cexecute.c io.c misc.c navy.c magic.c data.c trade.c safe_system.c; do
        test_single_file "$file"
    done

    echo "=== POSTSCRIPT UTILITY FILES (1 file) ===" >> ${OUTFILE}
    test_single_file "psmap.c"

    echo "=== COMPREHENSIVE TEST COMPLETE ===" >> ${OUTFILE}
    echo "Check report for char signedness warnings and compilation status." >> ${OUTFILE}
    echo "" >> ${OUTFILE}
    echo "Compare with baseline (without -fsigned-char) to see portability issues." >> ${OUTFILE}
fi

# Display results
verbose_echo "Test complete. Results written to: $OUTFILE"
if [[ $VERBOSE -eq 1 ]] || [[ -n "$SINGLE_FILE" ]]; then
    echo ""
    echo "=== COMPILATION RESULTS ==="
    cat "$OUTFILE"
fi

rm -f ${TEMPFILE}
