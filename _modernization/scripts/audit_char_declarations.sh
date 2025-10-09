#!/bin/bash

# audit_char_declarations.sh - Comprehensive Char Declaration Audit
#
# Purpose: Find all plain 'char' declarations that are ambiguous (not unsigned/signed)
#
# Context:
#   - aarch64: 'char' is UNSIGNED (CHAR_MIN = 0)
#   - x86_64:  'char' is SIGNED (CHAR_MIN = -128)
#   - Plain 'char' without explicit signedness is platform-dependent
#
# This script finds all uses of plain 'char' to help classify them as:
#   Category A: Text/strings (keep as char)
#   Category B: Small unsigned integers (change to unsigned char)
#   Category C: Small signed integers (change to signed char)
#   Category D: ctype.h parameters (MUST be unsigned char)
#   Category E: Array indices (MUST be unsigned char or size_t)
#
# Phase: 10.1 - Char Type Audit and Declaration Refactoring
# GitHub Issue: #10 (Char Signedness Portability)

OUTDIR="_modernization/claude/reports"
OUTFILE="${OUTDIR}/char_audit_raw.txt"

# Ensure output directory exists
mkdir -p "${OUTDIR}"

# Clear output file
> "${OUTFILE}"

echo "=== COMPREHENSIVE CHAR DECLARATION AUDIT ===" > "${OUTFILE}"
echo "Purpose: Find all plain 'char' declarations (not unsigned/signed)" >> "${OUTFILE}"
echo "Date: $(date)" >> "${OUTFILE}"
echo "" >> "${OUTFILE}"

# Function to search for pattern and append to output
search_pattern() {
    local title="$1"
    local pattern="$2"
    local files="$3"

    echo "=== $title ===" >> "${OUTFILE}"
    echo "" >> "${OUTFILE}"

    # Use grep to find matches, exclude unsigned/signed char
    # -n: line numbers, -H: filename
    grep -nH "$pattern" $files 2>/dev/null | \
        grep -v "unsigned char" | \
        grep -v "signed char" | \
        grep -v "^[[:space:]]*\*" | \
        grep -v "^[[:space:]]*//" | \
        grep -v "^[[:space:]]*/\*" >> "${OUTFILE}" || \
        echo "(none found)" >> "${OUTFILE}"

    echo "" >> "${OUTFILE}"

    # Count matches
    local count=$(grep -nH "$pattern" $files 2>/dev/null | \
        grep -v "unsigned char" | \
        grep -v "signed char" | \
        grep -v "^[[:space:]]*\*" | \
        grep -v "^[[:space:]]*//" | \
        grep -v "^[[:space:]]*/\*" | wc -l)

    echo "Count: $count" >> "${OUTFILE}"
    echo "" >> "${OUTFILE}"
}

# Search in both .c and .h files
SOURCE_FILES="*.c *.h"

echo "Searching for plain 'char' declarations in: $SOURCE_FILES"
echo ""

# 1. Simple variable declarations: char varname;
search_pattern "1. CHAR VARIABLE DECLARATIONS (char varname;)" \
    "char [a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*;" \
    "$SOURCE_FILES"

# 2. Char pointer declarations: char *ptr;
search_pattern "2. CHAR POINTER DECLARATIONS (char *ptr;)" \
    "char[[:space:]]*\*" \
    "$SOURCE_FILES"

# 3. Char array declarations: char array[SIZE];
search_pattern "3. CHAR ARRAY DECLARATIONS (char array[SIZE];)" \
    "char[[:space:]]\+[a-zA-Z_][a-zA-Z0-9_]*\[" \
    "$SOURCE_FILES"

# 4. Function parameters: (char param)
search_pattern "4. CHAR FUNCTION PARAMETERS (char param)" \
    "([[:space:]]*char[[:space:]]\+[a-zA-Z_]" \
    "$SOURCE_FILES"

# 5. Function return types: char function_name(
search_pattern "5. CHAR RETURN TYPES (char function_name()" \
    "^char[[:space:]]\+[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*(" \
    "$SOURCE_FILES"

# 6. Struct members: Inside struct definitions
search_pattern "6. CHAR STRUCT MEMBERS (struct member definitions)" \
    "^[[:space:]]\+char[[:space:]]\+[a-zA-Z_]" \
    "*.h"

# 7. Typedef with char
search_pattern "7. CHAR TYPEDEFS (typedef ... char)" \
    "typedef.*char" \
    "$SOURCE_FILES"

# 8. Char in casts: (char)
search_pattern "8. CHAR CASTS ((char)expression)" \
    "([[:space:]]*char[[:space:]]*)" \
    "$SOURCE_FILES"

echo "=== SUMMARY ===" >> "${OUTFILE}"
echo "" >> "${OUTFILE}"

# Count total matches
TOTAL_VARS=$(grep -nH "char [a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*;" $SOURCE_FILES 2>/dev/null | \
    grep -v "unsigned char" | grep -v "signed char" | \
    grep -v "^[[:space:]]*\*" | grep -v "^[[:space:]]*//" | grep -v "^[[:space:]]*/\*" | wc -l)

TOTAL_PTRS=$(grep -nH "char[[:space:]]*\*" $SOURCE_FILES 2>/dev/null | \
    grep -v "unsigned char" | grep -v "signed char" | \
    grep -v "^[[:space:]]*\*" | grep -v "^[[:space:]]*//" | grep -v "^[[:space:]]*/\*" | wc -l)

TOTAL_ARRAYS=$(grep -nH "char[[:space:]]\+[a-zA-Z_][a-zA-Z0-9_]*\[" $SOURCE_FILES 2>/dev/null | \
    grep -v "unsigned char" | grep -v "signed char" | \
    grep -v "^[[:space:]]*\*" | grep -v "^[[:space:]]*//" | grep -v "^[[:space:]]*/\*" | wc -l)

TOTAL_PARAMS=$(grep -nH "([[:space:]]*char[[:space:]]\+[a-zA-Z_]" $SOURCE_FILES 2>/dev/null | \
    grep -v "unsigned char" | grep -v "signed char" | \
    grep -v "^[[:space:]]*\*" | grep -v "^[[:space:]]*//" | grep -v "^[[:space:]]*/\*" | wc -l)

echo "Total plain char variable declarations: $TOTAL_VARS" >> "${OUTFILE}"
echo "Total plain char pointer declarations: $TOTAL_PTRS" >> "${OUTFILE}"
echo "Total plain char array declarations: $TOTAL_ARRAYS" >> "${OUTFILE}"
echo "Total plain char function parameters: $TOTAL_PARAMS" >> "${OUTFILE}"
echo "" >> "${OUTFILE}"
echo "TOTAL APPROXIMATE COUNT: $(($TOTAL_VARS + $TOTAL_PTRS + $TOTAL_ARRAYS + $TOTAL_PARAMS))" >> "${OUTFILE}"
echo "" >> "${OUTFILE}"

echo "=== AUDIT COMPLETE ===" >> "${OUTFILE}"
echo "Output: ${OUTFILE}" >> "${OUTFILE}"
echo "" >> "${OUTFILE}"
echo "Next Steps:" >> "${OUTFILE}"
echo "1. Review audit results" >> "${OUTFILE}"
echo "2. Classify each char by category (A-E)" >> "${OUTFILE}"
echo "3. Assign risk levels (HIGH/MEDIUM/LOW)" >> "${OUTFILE}"
echo "4. Create char_classification.md spreadsheet" >> "${OUTFILE}"
echo "5. Prioritize HIGH RISK fixes (array indexing, ctype.h)" >> "${OUTFILE}"

# Display results
echo "Audit complete. Results written to: ${OUTFILE}"
echo ""
echo "Summary:"
echo "  Variable declarations: $TOTAL_VARS"
echo "  Pointer declarations: $TOTAL_PTRS"
echo "  Array declarations: $TOTAL_ARRAYS"
echo "  Function parameters: $TOTAL_PARAMS"
echo "  TOTAL (approximate): $(($TOTAL_VARS + $TOTAL_PTRS + $TOTAL_ARRAYS + $TOTAL_PARAMS))"
echo ""
echo "Review the output file for detailed line-by-line results."
