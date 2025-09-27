#!/bin/bash

#
# test_clang.sh - Clang-specific compilation testing script for Phase 6 Testing Infrastructure
#
# This script performs comprehensive Clang compilation testing with automated
# warning detection, reporting, and verification for the modernized codebase.
# Based on the sophisticated file-type detection from test_warnings.sh.
#
# Usage:
#   ./test_clang.sh [options]
#
# Options:
#   -h, --help      Show this help message
#   -v, --verbose   Enable verbose output
#   -s, --silent    Suppress all output except errors
#   -o, --output    Specify output directory for reports (default: ../../_modernization/claude/scratch)
#   -w, --warnings  Warning level (0-10, default: 8)
#
# Features:
#   - File-type aware compilation (ADMIN/GAME/DUAL/PSMAP)
#   - Automated warning detection and counting
#   - Timestamped compilation reports
#   - Integration with Phase 6 testing framework
#   - Cross-platform Clang flag compatibility
#

## Determine where this is running and fail if not in project homedir
## TODO: FIX THIS TO WORK ON CHECKOUT.
TOPDIR=/projects/conquer-4.x/
## TODO: FIND A SCRATCH PLACE TO PUT THESE FOR TESTS
EXEDIR=/projects/conquer/bin/
SHARDIR=/projects/conquer/share/

set -euo pipefail

# Script configuration
SCRIPT_NAME="test_clang.sh"
SCRIPT_VERSION="2.0.0"
SCRIPT_DATE="2025-09-27"

# Default configuration
VERBOSE=false
SILENT=false
WARN_LEVEL=8
OUTPUT_DIR="${TOPDIR}/_modernization/claude/scratch"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
REPORT_FILE="${OUTPUT_DIR}/clang_test_report_${TIMESTAMP}.md"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Base compilation configuration
CLANG_BASE_FLAGS="-std=c2x -D_POSIX_C_SOURCE=200809L"
CLANG_OPTIMIZATION_FLAGS="-O2 -g"
DFLAGS="-DDEFAULTDIR=\"/projects/conquer/lib\" -DEXEDIR=\"/projects/conquer/bin\" -DLOGIN=\"ssmoogen\""

# Warning levels (adapted from test_warnings.sh)
declare -A WARNING_LEVELS
WARNING_LEVELS[0]=""
WARNING_LEVELS[1]="-Wall"
WARNING_LEVELS[2]="-Wall -Wextra"
WARNING_LEVELS[3]="-Wall -Wextra -Wpedantic"
WARNING_LEVELS[4]="-Wall -Wextra -Wpedantic -Wformat=2"
WARNING_LEVELS[5]="-Wall -Wextra -Wpedantic -Wformat=2 -Wconversion"
WARNING_LEVELS[6]="-Wall -Wextra -Wpedantic -Wformat=2 -Wconversion -Wsign-conversion"
WARNING_LEVELS[7]="-Wall -Wextra -Wpedantic -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough"
WARNING_LEVELS[8]="-Wall -Wextra -Wpedantic -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -Wstrict-prototypes"
WARNING_LEVELS[9]="-Wall -Wextra -Wpedantic -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -Wstrict-prototypes -Wold-style-declaration -Wshadow -Wmissing-prototypes -Wcast-qual"
WARNING_LEVELS[10]="-Wall -Wextra -Wpedantic -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -Wstrict-prototypes -Wold-style-declaration -Wshadow -Wmissing-prototypes -Wcast-qual --analyze"

# File compilation flags (based on test_warnings.sh file categorization)
ADMIN_FLAGS="${CLANG_OPTIMIZATION_FLAGS} ${CLANG_BASE_FLAGS} ${DFLAGS} -DADMIN -DCONQUER"
GAME_FLAGS="${CLANG_OPTIMIZATION_FLAGS} ${CLANG_BASE_FLAGS} ${DFLAGS} -DCONQUER"
PSMAP_FLAGS="${CLANG_OPTIMIZATION_FLAGS} ${CLANG_BASE_FLAGS} ${DFLAGS} -DPSFILE=\"/projects/conquer/bin/psmap.ps\" -DLETTER"

# Function to determine file compilation type (adapted from test_warnings.sh)
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
        cexecute.c|io.c|misc.c|navy.c|magic.c|data.c|trade.c)
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

# Function to display usage information
show_help() {
    cat << EOF
${SCRIPT_NAME} v${SCRIPT_VERSION} - Clang Compilation Testing Script

USAGE:
    ./${SCRIPT_NAME} [OPTIONS]

DESCRIPTION:
    Performs comprehensive Clang compilation testing with automated warning
    detection and reporting for the Phase 6 testing infrastructure.
    Uses sophisticated file-type detection for proper compilation flags.

OPTIONS:
    -h, --help      Show this help message and exit
    -v, --verbose   Enable verbose output during compilation
    -s, --silent    Suppress all output except critical errors
    -w, --warnings  Warning level (0-10, default: 8)
    -o, --output    Specify output directory for reports
                    Default: ${OUTPUT_DIR}

WARNING LEVELS:
    0: No warnings
    1: -Wall
    2: -Wall -Wextra
    3: -Wall -Wextra -Wpedantic
    4: Level 3 + -Wformat=2
    5: Level 4 + -Wconversion
    6: Level 5 + -Wsign-conversion
    7: Level 6 + -Wimplicit-fallthrough
    8: Level 7 + -Wstrict-prototypes (default)
    9: Level 8 + advanced warnings
    10: Level 9 + --analyze

FILE TYPES:
    • ADMIN files: Compiled with -DADMIN -DCONQUER
    • GAME files: Compiled with -DCONQUER only
    • DUAL files: Compiled in both ADMIN and GAME modes
    • PSMAP files: Compiled with PostScript flags
    • SHARED files: Compiled with ADMIN flags

EXAMPLES:
    ./${SCRIPT_NAME}                    # Standard compilation test
    ./${SCRIPT_NAME} -v                 # Verbose compilation test
    ./${SCRIPT_NAME} -w 10              # Maximum warning level with analysis
    ./${SCRIPT_NAME} -o /tmp/reports    # Custom output directory

PHASE 6 INTEGRATION:
    This script is part of Phase 6.2: Cross-Compiler Test Infrastructure
    Results are automatically integrated with the Phase 6 testing framework.

EOF
}

# Function to log messages with timestamps
log_message() {
    local level="$1"
    local message="$2"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')

    if [[ "$SILENT" == false ]]; then
        case "$level" in
            "INFO")
                echo -e "${BLUE}[INFO]${NC} ${timestamp}: $message" ;;
            "WARN")
                echo -e "${YELLOW}[WARN]${NC} ${timestamp}: $message" ;;
            "ERROR")
                echo -e "${RED}[ERROR]${NC} ${timestamp}: $message" >&2 ;;
            "SUCCESS")
                echo -e "${GREEN}[SUCCESS]${NC} ${timestamp}: $message" ;;
        esac
    fi
}

# Function to create output directory if it doesn't exist
ensure_output_dir() {
    if [[ ! -d "$OUTPUT_DIR" ]]; then
        log_message "INFO" "Creating output directory: $OUTPUT_DIR"
        mkdir -p "$OUTPUT_DIR"
        if [[ $? -ne 0 ]]; then
            log_message "ERROR" "Unable to make ${OUTPUT_DIR}"
            exit 1
        fi
    fi
}

# Function to test a single file with appropriate flags
test_single_file() {
    local file=$1
    local file_type=$(get_file_type "$file")
    local warnings=0
    local output=""
    local warn_flags="${WARNING_LEVELS[$WARN_LEVEL]}"

    log_message "INFO" "Testing $file (type: $file_type)"

    case $file_type in
        ADMIN|SHARED)
            local flags="$ADMIN_FLAGS $warn_flags"
            if output=$(clang $flags -c "$file" -o /tmp/test.o 2>&1); then
                warnings=$(echo "$output" | grep -c "warning:" || true)
                log_message "SUCCESS" "$file (Admin) - $warnings warnings"
            else
                warnings="FAILED"
                log_message "ERROR" "$file (Admin) - compilation failed"
            fi
            echo "### $file (Admin Mode)" >> "$REPORT_FILE"
            echo "**Flags**: \`$flags\`" >> "$REPORT_FILE"
            echo "**Status**: $([ "$warnings" = "FAILED" ] && echo "❌ FAILED" || echo "✅ PASSED - $warnings warnings")" >> "$REPORT_FILE"
            echo "" >> "$REPORT_FILE"
            ;;
        GAME)
            local flags="$GAME_FLAGS $warn_flags"
            if output=$(clang $flags -c "$file" -o /tmp/test.o 2>&1); then
                warnings=$(echo "$output" | grep -c "warning:" || true)
                log_message "SUCCESS" "$file (Game) - $warnings warnings"
            else
                warnings="FAILED"
                log_message "ERROR" "$file (Game) - compilation failed"
            fi
            echo "### $file (Game Mode)" >> "$REPORT_FILE"
            echo "**Flags**: \`$flags\`" >> "$REPORT_FILE"
            echo "**Status**: $([ "$warnings" = "FAILED" ] && echo "❌ FAILED" || echo "✅ PASSED - $warnings warnings")" >> "$REPORT_FILE"
            echo "" >> "$REPORT_FILE"
            ;;
        DUAL)
            # Test both admin and game modes
            local admin_flags="$ADMIN_FLAGS $warn_flags"
            local game_flags="$GAME_FLAGS $warn_flags"
            local admin_warnings=0
            local game_warnings=0

            # Admin mode test
            if output=$(clang $admin_flags -c "$file" -o /tmp/test.o 2>&1); then
                admin_warnings=$(echo "$output" | grep -c "warning:" || true)
                log_message "SUCCESS" "$file (Admin) - $admin_warnings warnings"
            else
                admin_warnings="FAILED"
                log_message "ERROR" "$file (Admin) - compilation failed"
            fi

            # Game mode test
            if output=$(clang $game_flags -c "$file" -o /tmp/test.o 2>&1); then
                game_warnings=$(echo "$output" | grep -c "warning:" || true)
                log_message "SUCCESS" "$file (Game) - $game_warnings warnings"
            else
                game_warnings="FAILED"
                log_message "ERROR" "$file (Game) - compilation failed"
            fi

            echo "### $file (Dual Mode - Admin)" >> "$REPORT_FILE"
            echo "**Flags**: \`$admin_flags\`" >> "$REPORT_FILE"
            echo "**Status**: $([ "$admin_warnings" = "FAILED" ] && echo "❌ FAILED" || echo "✅ PASSED - $admin_warnings warnings")" >> "$REPORT_FILE"
            echo "" >> "$REPORT_FILE"
            echo "### $file (Dual Mode - Game)" >> "$REPORT_FILE"
            echo "**Flags**: \`$game_flags\`" >> "$REPORT_FILE"
            echo "**Status**: $([ "$game_warnings" = "FAILED" ] && echo "❌ FAILED" || echo "✅ PASSED - $game_warnings warnings")" >> "$REPORT_FILE"
            echo "" >> "$REPORT_FILE"

            if [[ "$admin_warnings" != "FAILED" ]] && [[ "$game_warnings" != "FAILED" ]]; then
                if [[ "$admin_warnings" =~ ^[0-9]+$ ]] && [[ "$game_warnings" =~ ^[0-9]+$ ]]; then
                    warnings=$((admin_warnings + game_warnings))
                else
                    warnings="FAILED"
                fi
            else
                warnings="FAILED"
            fi
            ;;
        PSMAP)
            local flags="$PSMAP_FLAGS $warn_flags"
            if output=$(clang $flags -c "$file" -o /tmp/test.o 2>&1); then
                warnings=$(echo "$output" | grep -c "warning:" || true)
                log_message "SUCCESS" "$file (PostScript) - $warnings warnings"
            else
                warnings="FAILED"
                log_message "ERROR" "$file (PostScript) - compilation failed"
            fi
            echo "### $file (PostScript Mode)" >> "$REPORT_FILE"
            echo "**Flags**: \`$flags\`" >> "$REPORT_FILE"
            echo "**Status**: $([ "$warnings" = "FAILED" ] && echo "❌ FAILED" || echo "✅ PASSED - $warnings warnings")" >> "$REPORT_FILE"
            echo "" >> "$REPORT_FILE"
            ;;
        UNKNOWN)
            local flags="$ADMIN_FLAGS $warn_flags"
            if output=$(clang $flags -c "$file" -o /tmp/test.o 2>&1); then
                warnings=$(echo "$output" | grep -c "warning:" || true)
                log_message "SUCCESS" "$file (Unknown, using Admin) - $warnings warnings"
            else
                warnings="FAILED"
                log_message "ERROR" "$file (Unknown, using Admin) - compilation failed"
            fi
            echo "### $file (Unknown Type - Admin Flags)" >> "$REPORT_FILE"
            echo "**Flags**: \`$flags\`" >> "$REPORT_FILE"
            echo "**Status**: $([ "$warnings" = "FAILED" ] && echo "❌ FAILED" || echo "✅ PASSED - $warnings warnings")" >> "$REPORT_FILE"
            echo "" >> "$REPORT_FILE"
            ;;
    esac

    # Clean up
    rm -f /tmp/test.o *.plist

    echo "$warnings"
}

# Function to check if Clang is available
check_clang_availability() {
    if ! command -v clang &> /dev/null; then
        log_message "ERROR" "Clang compiler not found. Please install Clang to run this test."
        echo "## Clang Availability" >> "$REPORT_FILE"
        echo "**Status**: ❌ Clang not available" >> "$REPORT_FILE"
        echo "**Error**: Clang compiler not found in PATH" >> "$REPORT_FILE"
        return 1
    fi

    log_message "INFO" "Clang found: $(clang --version | head -n1)"
    return 0
}

# Function to initialize the test report
init_report() {
    ensure_output_dir

    cat > "$REPORT_FILE" << EOF
# Clang Compilation Test Report

**Generated**: $(date '+%Y-%m-%d %H:%M:%S')
**Script**: ${SCRIPT_NAME} v${SCRIPT_VERSION}
**Phase**: 6.2 - Cross-Compiler Test Infrastructure
**Compiler**: $(clang --version 2>/dev/null | head -n1 || echo "Clang not available")

## Test Configuration

**Warning Level**: $WARN_LEVEL (${WARNING_LEVELS[$WARN_LEVEL]})
**Clang Flags Used**:
- Base: \`${CLANG_BASE_FLAGS}\`
- Warning Flags: \`${WARNING_LEVELS[$WARN_LEVEL]}\`
- Optimization: \`${CLANG_OPTIMIZATION_FLAGS}\`
- Directory Flags: \`${DFLAGS}\`

**File Type Categories**:
- ADMIN: \`${ADMIN_FLAGS}\` + warning flags
- GAME: \`${GAME_FLAGS}\` + warning flags
- PSMAP: \`${PSMAP_FLAGS}\` + warning flags
- DUAL: Tested in both ADMIN and GAME modes

## Test Results

EOF

    log_message "INFO" "Initialized test report: $REPORT_FILE"
}

# Function to test Clang compilation with file-type awareness
test_clang_compilation() {
    log_message "INFO" "Starting Clang compilation test with file-type detection"

    # Check Clang availability first
    if ! check_clang_availability; then
        return 1
    fi

    # Change to project root
    cd "${TOPDIR}"

    local total_warnings=0
    local failed_files=0
    local total_tests=0

    # Test files by category (adapted from test_warnings.sh)
    echo "## File-by-File Compilation Results" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"

    # Admin-only files
    log_message "INFO" "Testing ADMIN-ONLY files"
    echo "### ADMIN-ONLY FILES (8 files)" >> "$REPORT_FILE"
    for file in admin.c combat.c spew.c newlogin.c update.c npc.c randeven.c makeworl.c; do
        if [[ -f "$file" ]]; then
            local result=$(test_single_file "$file")
            total_tests=$((total_tests + 1))
            if [[ "$result" == "FAILED" ]]; then
                failed_files=$((failed_files + 1))
            elif [[ "$result" =~ ^[0-9]+$ ]]; then
                total_warnings=$((total_warnings + result))
            fi
        fi
    done

    # Shared files
    log_message "INFO" "Testing SHARED files"
    echo "### SHARED FILES (1 file)" >> "$REPORT_FILE"
    if [[ -f "check.c" ]]; then
        local result=$(test_single_file "check.c")
        total_tests=$((total_tests + 1))
        if [[ "$result" == "FAILED" ]]; then
            failed_files=$((failed_files + 1))
        elif [[ "$result" =~ ^[0-9]+$ ]]; then
            total_warnings=$((total_warnings + result))
        fi
    fi

    # Game-only files
    log_message "INFO" "Testing GAME-ONLY files"
    echo "### GAME-ONLY FILES (7 files)" >> "$REPORT_FILE"
    for file in commands.c main.c forms.c move.c reports.c display.c extcmds.c; do
        if [[ -f "$file" ]]; then
            local result=$(test_single_file "$file")
            total_tests=$((total_tests + 1))
            if [[ "$result" == "FAILED" ]]; then
                failed_files=$((failed_files + 1))
            elif [[ "$result" =~ ^[0-9]+$ ]]; then
                total_warnings=$((total_warnings + result))
            fi
        fi
    done

    # Dual-compiled files (tested in both modes)
    log_message "INFO" "Testing DUAL-COMPILED files"
    echo "### DUAL-COMPILED FILES (7 files)" >> "$REPORT_FILE"
    for file in cexecute.c io.c misc.c navy.c magic.c data.c trade.c; do
        if [[ -f "$file" ]]; then
            local result=$(test_single_file "$file")
            total_tests=$((total_tests + 2))  # Counts as 2 tests (admin + game)
            if [[ "$result" == "FAILED" ]]; then
                failed_files=$((failed_files + 1))
            elif [[ "$result" =~ ^[0-9]+$ ]]; then
                total_warnings=$((total_warnings + result))
            fi
        fi
    done

    # PostScript utility files
    log_message "INFO" "Testing POSTSCRIPT files"
    echo "### POSTSCRIPT UTILITY FILES (1 file)" >> "$REPORT_FILE"
    if [[ -f "psmap.c" ]]; then
        local result=$(test_single_file "psmap.c")
        total_tests=$((total_tests + 1))
        if [[ "$result" == "FAILED" ]]; then
            failed_files=$((failed_files + 1))
        elif [[ "$result" =~ ^[0-9]+$ ]]; then
            total_warnings=$((total_warnings + result))
        fi
    fi

    # Test CMake build with Clang (if CC=clang is supported)
    log_message "INFO" "Testing CMake build with Clang"
    echo "### CMake Build with Clang Test" >> "$REPORT_FILE"
    local cmake_warnings=0
    local cmake_output=""
    local cmake_status="SKIPPED"

    # Set CC=clang for CMake build
    if cmake_output=$(CC=clang cmake --build build --clean-first 2>&1); then
        cmake_warnings=$(echo "$cmake_output" | grep -c "warning:" || true)
        cmake_status="PASSED"
        log_message "SUCCESS" "CMake build with Clang successful - $cmake_warnings warnings"
        echo "**Status**: ✅ PASSED - $cmake_warnings warnings" >> "$REPORT_FILE"
        total_warnings=$((total_warnings + cmake_warnings))
    else
        # If that fails, note it but don't fail the test
        cmake_status="FAILED"
        log_message "WARN" "CMake build with Clang failed or not configured"
        echo "**Status**: ⚠️ FAILED/NOT_CONFIGURED" >> "$REPORT_FILE"
    fi
    echo "**Command**: \`CC=clang cmake --build build --clean-first\`" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"

    # Clean up object files and analysis results
    rm -f *.o *.plist

    # Write summary to report
    local success_tests=$((total_tests - failed_files))
    cat >> "$REPORT_FILE" << EOF
## Summary

**Overall Status**: $([ $failed_files -eq 0 ] && echo "✅ ALL TESTS PASSED" || echo "❌ $failed_files TESTS FAILED")
**Tests Passed**: $success_tests / $total_tests
**Total Warnings**: $total_warnings
**Warning Level**: $WARN_LEVEL (${WARNING_LEVELS[$WARN_LEVEL]})
**Clang Version**: $(clang --version | head -n1)
**Test Date**: $(date '+%Y-%m-%d %H:%M:%S')
**CMake Status**: $cmake_status

EOF

    # Return overall status (don't fail on CMake issues since it may not be configured for Clang)
    if [[ $failed_files -eq 0 ]]; then
        log_message "SUCCESS" "All Clang tests passed - $success_tests/$total_tests tests, $total_warnings total warnings"
        return 0
    else
        log_message "ERROR" "$failed_files Clang tests failed - $success_tests/$total_tests passed"
        return 1
    fi
}

# Function to display final results
show_results() {
    if [[ "$SILENT" == false ]]; then
        echo ""
        echo "========================================"
        echo "Clang Compilation Test Complete"
        echo "========================================"
        echo "Report saved to: $REPORT_FILE"
        echo ""
        echo "To view the full report:"
        echo "  cat $REPORT_FILE"
        echo ""
    fi
}

# Main execution function
main() {
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -s|--silent)
                SILENT=true
                shift
                ;;
            -w|--warnings)
                WARN_LEVEL="$2"
                if [[ ! "$WARN_LEVEL" =~ ^([0-9]|10)$ ]]; then
                    log_message "ERROR" "Warning level must be 0-10"
                    exit 1
                fi
                shift 2
                ;;
            -o|--output)
                OUTPUT_DIR="$2"
                REPORT_FILE="${OUTPUT_DIR}/clang_test_report_${TIMESTAMP}.md"
                shift 2
                ;;
            *)
                log_message "ERROR" "Unknown option: $1"
                echo "Use -h or --help for usage information"
                exit 1
                ;;
        esac
    done

    # Execute the test sequence
    log_message "INFO" "Starting Clang compilation testing"
    init_report

    if test_clang_compilation; then
        show_results
        exit 0
    else
        show_results
        exit 1
    fi
}

# Execute main function if script is run directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
