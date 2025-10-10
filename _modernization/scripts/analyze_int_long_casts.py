#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["pathlib"]
# ///
"""
Analyze (int) and (long) casts for potential portability issues.

This script categorizes integer casts by risk level and provides recommendations
for safe conversions using safe_convert.h functions.

Usage:
    python3 analyze_int_long_casts.py [--long|--int|--all]
"""

import re
import sys
from pathlib import Path
from collections import defaultdict

# Risk categories
RISK_HIGH = "HIGH"      # Array indexing, pointer arithmetic
RISK_MED = "MEDIUM"     # Function arguments, assignments with size mismatch
RISK_LOW = "LOW"        # Same-size conversions, explicit promotions

class CastAnalyzer:
    def __init__(self):
        self.long_casts = defaultdict(list)
        self.int_casts = defaultdict(list)

    def analyze_file(self, filepath):
        """Analyze a single C file for casts."""
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()

        for line_num, line in enumerate(lines, 1):
            # Analyze (long) casts
            if '(long)' in line:
                risk, reason = self._classify_long_cast(line)
                self.long_casts[risk].append({
                    'file': filepath.name,
                    'line': line_num,
                    'code': line.strip(),
                    'reason': reason
                })

            # Analyze (int) casts
            if '(int)' in line:
                risk, reason = self._classify_int_cast(line)
                self.int_casts[risk].append({
                    'file': filepath.name,
                    'line': line_num,
                    'code': line.strip(),
                    'reason': reason
                })

    def _classify_long_cast(self, line):
        """Classify (long) cast by risk level."""
        # HIGH RISK: Array indexing
        if re.search(r'\(long\)[^;]*\[', line):
            return RISK_HIGH, "Cast used for array indexing"

        # HIGH RISK: Pointer arithmetic
        if re.search(r'\(long\)[^;]*(\+\+|--|\+=|-=)', line) and '*' in line:
            return RISK_HIGH, "Cast in pointer arithmetic"

        # MEDIUM RISK: Assignment to smaller type
        if re.search(r'(short|char|unsigned char|unsigned short)\s+\w+\s*=.*\(long\)', line):
            return RISK_MED, "Cast to long then assigned to smaller type (potential truncation)"

        # MEDIUM RISK: Widening for multiplication (overflow prevention)
        if re.search(r'\(long\)[^;]*\*', line):
            return RISK_LOW, "Widening cast for multiplication (likely safe, prevents overflow)"

        # LOW RISK: Function argument
        if '(' in line and ')' in line:
            return RISK_LOW, "Function argument cast (verify function signature)"

        return RISK_LOW, "Generic long cast (review context)"

    def _classify_int_cast(self, line):
        """Classify (int) cast by risk level."""
        # HIGH RISK: Array indexing from long/size_t
        if re.search(r'\(int\)[^;]*\[', line):
            return RISK_HIGH, "Cast to int for array indexing (use safe_long_to_int)"

        # HIGH RISK: Unsigned char to int for array indexing
        if re.search(r'\(int\)(P_[AN]XLOC|P_[AN]YLOC|curntn->(arm|nvy).*\.(xloc|yloc))', line) and '[' in line:
            return RISK_HIGH, "Unsigned char to int for indexing (likely safe but verify bounds)"

        # MEDIUM RISK: long to int conversion
        if re.search(r'(int)\s+\w+\s*=.*\(int\)', line):
            return RISK_MED, "Long to int assignment (check for truncation, use safe_long_to_int)"

        # MEDIUM RISK: Return statement
        if re.search(r'return\s+\(int\)', line):
            return RISK_MED, "Return value cast (verify function return type matches)"

        # LOW RISK: Arithmetic expressions
        if re.search(r'\(int\)[^;]*(\+|\-|\*|/|%)', line):
            return RISK_LOW, "Arithmetic cast (likely type promotion)"

        # LOW RISK: Comparison
        if re.search(r'\(int\)[^;]*(==|!=|<|>|<=|>=)', line):
            return RISK_LOW, "Comparison cast (verify signedness)"

        return RISK_LOW, "Generic int cast (review context)"

    def print_report(self, cast_type='all'):
        """Print analysis report."""
        print("=" * 80)
        print("INTEGER CAST ANALYSIS REPORT")
        print("=" * 80)
        print()

        if cast_type in ['long', 'all']:
            self._print_cast_category("LONG", self.long_casts)

        if cast_type in ['int', 'all']:
            self._print_cast_category("INT", self.int_casts)

        self._print_recommendations()

    def _print_cast_category(self, cast_name, casts_dict):
        """Print casts for a specific category."""
        print(f"\n{'=' * 80}")
        print(f"({cast_name}) CASTS ANALYSIS")
        print(f"{'=' * 80}\n")

        total = sum(len(items) for items in casts_dict.values())
        print(f"Total ({cast_name}) casts found: {total}\n")

        for risk in [RISK_HIGH, RISK_MED, RISK_LOW]:
            items = casts_dict.get(risk, [])
            if not items:
                continue

            print(f"\n{risk} RISK ({len(items)} casts):")
            print("-" * 80)

            # Group by reason
            by_reason = defaultdict(list)
            for item in items:
                by_reason[item['reason']].append(item)

            for reason, reason_items in sorted(by_reason.items()):
                print(f"\n  • {reason} ({len(reason_items)} occurrences)")

                # Show first 5 examples
                for item in reason_items[:5]:
                    print(f"    {item['file']}:{item['line']}: {item['code'][:70]}")

                if len(reason_items) > 5:
                    print(f"    ... and {len(reason_items) - 5} more")

    def _print_recommendations(self):
        """Print remediation recommendations."""
        print("\n" + "=" * 80)
        print("RECOMMENDATIONS")
        print("=" * 80)
        print()
        print("1. HIGH RISK CASTS - Address First:")
        print("   - Array indexing with (int): Use safe_long_to_int() or safe_size_t_to_int()")
        print("   - Verify array bounds are within INT_MAX")
        print("   - Consider using size_t for array indices if appropriate")
        print()
        print("2. MEDIUM RISK CASTS - Review Carefully:")
        print("   - Function arguments: Verify function signature matches")
        print("   - Assignments: Check for truncation with safe_convert functions")
        print("   - Return values: Ensure return type matches function declaration")
        print()
        print("3. LOW RISK CASTS - Review During Cleanup:")
        print("   - Arithmetic: Usually safe, but verify no overflow")
        print("   - Comparisons: Check signedness matches")
        print("   - Widening casts (int→long): Generally safe")
        print()
        print("4. Available safe_convert.h Functions:")
        print("   - safe_long_to_int(long val)")
        print("   - safe_int_to_short(int val)")
        print("   - safe_int_to_uchar(int val)")
        print("   - safe_size_t_to_int(size_t val)")
        print("   - safe_double_to_long(double val)")
        print()

def main():
    analyzer = CastAnalyzer()

    # Determine what to analyze
    cast_type = 'all'
    if len(sys.argv) > 1:
        arg = sys.argv[1].lower()
        if arg in ['--long', '-l']:
            cast_type = 'long'
        elif arg in ['--int', '-i']:
            cast_type = 'int'
        elif arg in ['--all', '-a']:
            cast_type = 'all'
        else:
            print(f"Usage: {sys.argv[0]} [--long|--int|--all]")
            sys.exit(1)

    # Analyze all C files in project root
    project_root = Path(__file__).parent.parent.parent
    c_files = list(project_root.glob("*.c"))

    print(f"Analyzing {len(c_files)} C files...")
    for c_file in sorted(c_files):
        analyzer.analyze_file(c_file)

    analyzer.print_report(cast_type)

if __name__ == '__main__':
    main()
