#!/bin/bash

#
# run_all_compiler_tests.sh - Unified cross-compiler testing runner for Phase 6 Testing Infrastructure
#
# This script orchestrates comprehensive compilation testing across multiple compilers
# (GCC and Clang) with automated warning detection, reporting, and verification.
#
# Usage:
#   ./run_all_compiler_tests.sh [options]
#
# Options:
#   -h, --help      Show this help message
#   -v, --verbose   Enable verbose output
#   -s, --silent    Suppress all output except errors
#   -g, --gcc-only  Run only GCC tests
#   -c, --clang-only Run only Clang tests
#   -o, --output    Specify output directory for reports (default: ../../_modernization/claude/scratch)
#
# Features:
#   - Orchestrates both GCC and Clang testing scripts
#   - Unified reporting and summary generation
#   - Cross-compiler compatibility verification
#   - Integration with Phase 6 testing framework
#   - Automated warning aggregation across compilers
#

set -euo pipefail

# Script configuration
SCRIPT_NAME="run_all_compiler_tests.sh"
SCRIPT_VERSION="2.0.0"
SCRIPT_DATE="2025-09-27"

# Default configuration
VERBOSE=false
SILENT=false
GCC_ONLY=false
CLANG_ONLY=false
WARN_LEVEL=8
OUTPUT_DIR="/projects/conquer-4.x/_modernization/claude/scratch"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
UNIFIED_REPORT="${OUTPUT_DIR}/unified_compiler_test_report_${TIMESTAMP}.md"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Script paths
SCRIPT_DIR="$(dirname "$0")"
GCC_SCRIPT="${SCRIPT_DIR}/test_gcc.sh"
CLANG_SCRIPT="${SCRIPT_DIR}/test_clang.sh"

# Function to display usage information
show_help() {
    cat << EOF
${SCRIPT_NAME} v${SCRIPT_VERSION} - Unified Cross-Compiler Testing Runner

USAGE:
    ./${SCRIPT_NAME} [OPTIONS]

DESCRIPTION:
    Orchestrates comprehensive compilation testing across multiple compilers
    with unified reporting and cross-compiler compatibility verification.

OPTIONS:
    -h, --help        Show this help message and exit
    -v, --verbose     Enable verbose output during compilation
    -s, --silent      Suppress all output except critical errors
    -g, --gcc-only    Run only GCC compilation tests
    -c, --clang-only  Run only Clang compilation tests
    -w, --warnings    Warning level for both compilers (0-10, default: 8)
    -o, --output      Specify output directory for reports
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
    10: Level 9 + analysis tools

FEATURES:
    • Unified cross-compiler testing orchestration
    • Automated warning aggregation across compilers
    • Comprehensive compatibility verification
    • Timestamped unified reports saved to ${OUTPUT_DIR}
    • Integration with Phase 6 testing framework
    • Fallback handling for missing compilers

EXAMPLES:
    ./${SCRIPT_NAME}                      # Test both GCC and Clang (default warning level 8)
    ./${SCRIPT_NAME} -g                   # Test only GCC
    ./${SCRIPT_NAME} -c                   # Test only Clang
    ./${SCRIPT_NAME} -w 10                # Maximum warning level with analysis
    ./${SCRIPT_NAME} -w 1 -v              # Basic warnings with verbose output
    ./${SCRIPT_NAME} -v -o /tmp/reports   # Verbose with custom output

PHASE 6 INTEGRATION:
    This script is part of Phase 6.2: Cross-Compiler Test Infrastructure
    Results are automatically integrated with the Phase 6 testing framework.

COMPILER DEPENDENCIES:
    • GCC: Required for standard compilation testing
    • Clang: Optional but recommended for additional verification
    • CMake: Used for build system integration testing

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
            "HEADER")
                echo -e "${BOLD}${CYAN}[${level}]${NC} ${timestamp}: $message" ;;
        esac
    fi
}

# Function to create output directory if it doesn't exist
ensure_output_dir() {
    if [[ ! -d "$OUTPUT_DIR" ]]; then
        log_message "INFO" "Creating output directory: $OUTPUT_DIR"
        mkdir -p "$OUTPUT_DIR"
    fi
}

# Function to check script dependencies
check_dependencies() {
    local missing_deps=()

    # Check for GCC test script
    if [[ ! -x "$GCC_SCRIPT" ]]; then
        missing_deps+=("$GCC_SCRIPT (not executable or missing)")
    fi

    # Check for Clang test script
    if [[ ! -x "$CLANG_SCRIPT" ]]; then
        missing_deps+=("$CLANG_SCRIPT (not executable or missing)")
    fi

    # Check for compilers (informational)
    if ! command -v gcc &> /dev/null; then
        log_message "WARN" "GCC compiler not found - GCC tests will fail"
    fi

    if ! command -v clang &> /dev/null; then
        log_message "WARN" "Clang compiler not found - Clang tests may be skipped"
    fi

    # Report missing critical dependencies
    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        log_message "ERROR" "Missing dependencies:"
        for dep in "${missing_deps[@]}"; do
            echo "  - $dep"
        done
        return 1
    fi

    return 0
}

# Function to initialize the unified report
init_unified_report() {
    ensure_output_dir

    cat > "$UNIFIED_REPORT" << EOF
# Unified Cross-Compiler Test Report

**Generated**: $(date '+%Y-%m-%d %H:%M:%S')
**Script**: ${SCRIPT_NAME} v${SCRIPT_VERSION}
**Phase**: 6.2 - Cross-Compiler Test Infrastructure
**Test Scope**: $([ "$GCC_ONLY" = true ] && echo "GCC Only" || [ "$CLANG_ONLY" = true ] && echo "Clang Only" || echo "GCC + Clang")
**Warning Level**: $WARN_LEVEL (Progressive warning analysis)

## Environment Information

**Operating System**: $(uname -s) $(uname -r)
**Architecture**: $(uname -m)
**Available Compilers**:
- GCC: $(gcc --version 2>/dev/null | head -n1 || echo "Not available")
- Clang: $(clang --version 2>/dev/null | head -n1 || echo "Not available")

## Test Configuration

**Warning Level Details**:
- Level $WARN_LEVEL provides $([ $WARN_LEVEL -eq 0 ] && echo "no warnings" || [ $WARN_LEVEL -eq 1 ] && echo "basic warnings (-Wall)" || [ $WARN_LEVEL -le 3 ] && echo "standard warnings" || [ $WARN_LEVEL -le 6 ] && echo "enhanced warnings" || [ $WARN_LEVEL -le 8 ] && echo "comprehensive warnings" || [ $WARN_LEVEL -eq 9 ] && echo "advanced warnings" || echo "maximum analysis with static analyzers")
- Both compilers use identical warning configurations for consistent comparison

## Test Execution Summary

EOF

    log_message "INFO" "Initialized unified report: $UNIFIED_REPORT"
}

# Function to run GCC tests
run_gcc_tests() {
    log_message "HEADER" "Starting GCC Compilation Tests"

    local gcc_status=0
    local gcc_output=""
    local gcc_report=""

    if [[ -x "$GCC_SCRIPT" ]]; then
        # Run GCC tests with appropriate flags
        local gcc_args=()
        if [[ "$VERBOSE" = true ]]; then
            gcc_args+=("-v")
        fi
        if [[ "$SILENT" = true ]]; then
            gcc_args+=("-s")
        fi
        gcc_args+=("-w" "$WARN_LEVEL")
        gcc_args+=("-o" "$OUTPUT_DIR")

        if gcc_output=$("$GCC_SCRIPT" "${gcc_args[@]}" 2>&1); then
            gcc_status=0
            log_message "SUCCESS" "GCC tests completed successfully"
        else
            gcc_status=1
            log_message "ERROR" "GCC tests failed"
        fi

        # Find the most recent GCC report
        gcc_report=$(find "$OUTPUT_DIR" -name "gcc_test_report_*.md" -type f -printf '%T@ %p\n' 2>/dev/null | sort -n | tail -1 | cut -d' ' -f2- || echo "")
    else
        gcc_status=1
        log_message "ERROR" "GCC test script not found or not executable"
    fi

    # Add GCC results to unified report
    cat >> "$UNIFIED_REPORT" << EOF
### GCC Test Results
**Status**: $([ $gcc_status -eq 0 ] && echo "✅ PASSED" || echo "❌ FAILED")
**Test Script**: \`$GCC_SCRIPT\`
**Detailed Report**: $([ -n "$gcc_report" ] && echo "\`$(basename "$gcc_report")\`" || echo "Not generated")

EOF

    return $gcc_status
}

# Function to run Clang tests
run_clang_tests() {
    log_message "HEADER" "Starting Clang Compilation Tests"

    local clang_status=0
    local clang_output=""
    local clang_report=""

    if [[ -x "$CLANG_SCRIPT" ]]; then
        # Run Clang tests with appropriate flags
        local clang_args=()
        if [[ "$VERBOSE" = true ]]; then
            clang_args+=("-v")
        fi
        if [[ "$SILENT" = true ]]; then
            clang_args+=("-s")
        fi
        clang_args+=("-w" "$WARN_LEVEL")
        clang_args+=("-o" "$OUTPUT_DIR")

        if clang_output=$("$CLANG_SCRIPT" "${clang_args[@]}" 2>&1); then
            clang_status=0
            log_message "SUCCESS" "Clang tests completed successfully"
        else
            clang_status=1
            log_message "ERROR" "Clang tests failed"
        fi

        # Find the most recent Clang report
        clang_report=$(find "$OUTPUT_DIR" -name "clang_test_report_*.md" -type f -printf '%T@ %p\n' 2>/dev/null | sort -n | tail -1 | cut -d' ' -f2- || echo "")
    else
        clang_status=1
        log_message "ERROR" "Clang test script not found or not executable"
    fi

    # Add Clang results to unified report
    cat >> "$UNIFIED_REPORT" << EOF
### Clang Test Results
**Status**: $([ $clang_status -eq 0 ] && echo "✅ PASSED" || echo "❌ FAILED")
**Test Script**: \`$CLANG_SCRIPT\`
**Detailed Report**: $([ -n "$clang_report" ] && echo "\`$(basename "$clang_report")\`" || echo "Not generated")

EOF

    return $clang_status
}

# Function to generate cross-compiler analysis
generate_cross_compiler_analysis() {
    log_message "INFO" "Generating cross-compiler compatibility analysis"

    # Find recent GCC and Clang reports for analysis
    local gcc_report=$(find "$OUTPUT_DIR" -name "gcc_test_report_*.md" -type f -printf '%T@ %p\n' 2>/dev/null | sort -n | tail -1 | cut -d' ' -f2- || echo "")
    local clang_report=$(find "$OUTPUT_DIR" -name "clang_test_report_*.md" -type f -printf '%T@ %p\n' 2>/dev/null | sort -n | tail -1 | cut -d' ' -f2- || echo "")

    cat >> "$UNIFIED_REPORT" << EOF

## Cross-Compiler Compatibility Analysis

EOF

    # Extract warning counts and test results if reports exist
    local gcc_warnings="N/A"
    local clang_warnings="N/A"
    local gcc_tests="N/A"
    local clang_tests="N/A"

    if [[ -f "$gcc_report" ]]; then
        gcc_warnings=$(grep "\*\*Total Warnings\*\*:" "$gcc_report" | sed 's/.*Total Warnings.*: //' || echo "N/A")
        gcc_tests=$(grep "\*\*Tests Passed\*\*:" "$gcc_report" | sed 's/.*Tests Passed.*: //' || echo "N/A")
    fi

    if [[ -f "$clang_report" ]]; then
        clang_warnings=$(grep "\*\*Total Warnings\*\*:" "$clang_report" | sed 's/.*Total Warnings.*: //' || echo "N/A")
        clang_tests=$(grep "\*\*Tests Passed\*\*:" "$clang_report" | sed 's/.*Tests Passed.*: //' || echo "N/A")
    fi

    cat >> "$UNIFIED_REPORT" << EOF
### Detailed Comparison Summary
| Compiler | Tests Passed | Total Warnings | Warning Level | Status |
|----------|-------------|----------------|---------------|--------|
| GCC      | $gcc_tests  | $gcc_warnings  | $WARN_LEVEL   | $([ -f "$gcc_report" ] && echo "✅ Tested" || echo "❌ Not tested") |
| Clang    | $clang_tests | $clang_warnings | $WARN_LEVEL   | $([ -f "$clang_report" ] && echo "✅ Tested" || echo "❌ Not tested") |

### Compatibility Assessment
$(if [[ "$gcc_warnings" != "N/A" && "$clang_warnings" != "N/A" ]]; then
    if [[ "$gcc_warnings" = "$clang_warnings" ]]; then
        echo "✅ **Excellent**: Both compilers report identical warning counts"
    elif [[ "${gcc_warnings//[^0-9]/}" -eq 0 && "${clang_warnings//[^0-9]/}" -eq 0 ]]; then
        echo "✅ **Excellent**: Both compilers report zero warnings"
    else
        echo "⚠️ **Good**: Both compilers tested with different warning counts"
    fi
else
    echo "ℹ️ **Partial**: Not all compilers were successfully tested"
fi)

EOF
}

# Function to finalize the unified report
finalize_unified_report() {
    local overall_status="$1"

    cat >> "$UNIFIED_REPORT" << EOF

## Overall Test Summary

**Overall Status**: $([ $overall_status -eq 0 ] && echo "✅ ALL TESTS PASSED" || echo "❌ SOME TESTS FAILED")
**Total Test Duration**: $(date '+%Y-%m-%d %H:%M:%S')
**Report Generation**: Completed successfully

### Next Steps
- Review individual compiler reports for detailed analysis
- Address any warnings or compilation failures found
- Integrate results with Phase 6 testing framework
- Proceed with Phase 6.3 development when all tests pass

### Generated Reports
$(find "$OUTPUT_DIR" -name "*_test_report_${TIMESTAMP:0:8}*.md" -type f -printf '- `%f`\n' 2>/dev/null || echo "- Check $OUTPUT_DIR for individual reports")

---
**Generated by**: ${SCRIPT_NAME} v${SCRIPT_VERSION}
**Phase**: 6.2 - Cross-Compiler Test Infrastructure
**Date**: $(date '+%Y-%m-%d %H:%M:%S')
EOF
}

# Function to display final results
show_final_results() {
    local overall_status="$1"

    if [[ "$SILENT" == false ]]; then
        echo ""
        echo "========================================"
        echo "Unified Cross-Compiler Testing Complete"
        echo "========================================"
        echo ""
        if [[ $overall_status -eq 0 ]]; then
            echo -e "${GREEN}✅ Overall Status: ALL TESTS PASSED${NC}"
        else
            echo -e "${RED}❌ Overall Status: SOME TESTS FAILED${NC}"
        fi
        echo ""
        echo "Unified report: $UNIFIED_REPORT"
        echo ""
        echo "Individual reports in: $OUTPUT_DIR"
        echo ""
        echo "To view the unified report:"
        echo "  cat $UNIFIED_REPORT"
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
            -g|--gcc-only)
                GCC_ONLY=true
                shift
                ;;
            -c|--clang-only)
                CLANG_ONLY=true
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
                UNIFIED_REPORT="${OUTPUT_DIR}/unified_compiler_test_report_${TIMESTAMP}.md"
                shift 2
                ;;
            *)
                log_message "ERROR" "Unknown option: $1"
                echo "Use -h or --help for usage information"
                exit 1
                ;;
        esac
    done

    # Validate exclusive options
    if [[ "$GCC_ONLY" = true && "$CLANG_ONLY" = true ]]; then
        log_message "ERROR" "Cannot specify both --gcc-only and --clang-only"
        exit 1
    fi

    # Execute the test sequence
    log_message "HEADER" "Starting Unified Cross-Compiler Testing"

    # Check dependencies
    if ! check_dependencies; then
        exit 1
    fi

    # Initialize unified report
    init_unified_report

    # Run tests based on options
    local overall_status=0

    if [[ "$CLANG_ONLY" != true ]]; then
        if ! run_gcc_tests; then
            overall_status=1
        fi
    fi

    if [[ "$GCC_ONLY" != true ]]; then
        if ! run_clang_tests; then
            overall_status=1
        fi
    fi

    # Generate cross-compiler analysis
    generate_cross_compiler_analysis

    # Finalize report
    finalize_unified_report $overall_status

    # Display results
    show_final_results $overall_status

    exit $overall_status
}

# Execute main function if script is run directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi