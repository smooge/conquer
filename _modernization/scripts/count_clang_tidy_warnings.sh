#!/bin/bash
# Count real clang-tidy warnings from report files
# Filters out the misleading "N warnings generated" count

REPORT_DIR="/projects/conquer-4.x/build/reports/clang-tidy"

echo "=== Clang-Tidy Real Warning Counts ==="
echo ""
echo "Note: Console output shows warnings generated during parsing (including headers)."
echo "      These counts show REAL warnings after HeaderFilterRegex filtering."
echo ""

if [ ! -d "$REPORT_DIR" ]; then
    echo "Error: Report directory not found: $REPORT_DIR"
    echo "Run a clang-tidy target first (e.g., make clang-tidy-security)"
    exit 1
fi

total=0
found_reports=0

for report in "$REPORT_DIR"/*.txt; do
    if [ -f "$report" ]; then
        found_reports=$((found_reports + 1))
        basename=$(basename "$report")
        count=$(grep -c "warning:" "$report" 2>/dev/null)
        if [ -z "$count" ]; then
            count=0
        fi
        total=$((total + count))
        printf "%-30s %5d warnings\n" "$basename" "$count"
    fi
done

if [ $found_reports -eq 0 ]; then
    echo "No report files found in $REPORT_DIR"
    echo "Run a clang-tidy target first (e.g., make clang-tidy-security)"
    exit 1
fi

echo ""
echo "----------------------------------------"
printf "%-30s %5d warnings\n" "TOTAL" "$total"
echo ""
