#!/usr/bin/env python3
"""
Consolidate Phase 10.2.2 split reports into comprehensive single reports.

This script merges extraction-only reports with their architectural debt supplements
into comprehensive 3-part analysis reports.

Usage:
    python3 consolidate_phase_10_reports.py [--dry-run]
"""

import argparse
import sys
from pathlib import Path
from datetime import datetime

# Report directory
REPORTS_DIR = Path(__file__).parent.parent / "claude" / "reports"

# Files to consolidate (standard report + architectural debt supplement)
FILES_TO_CONSOLIDATE = [
    ("update.c", True),   # Already done, skip
    ("forms.c", False),
    ("navy.c", False),
    ("combat.c", False),
    ("admin.c", False),
]

def read_file(filepath):
    """Read file contents."""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            return f.read()
    except FileNotFoundError:
        print(f"ERROR: File not found: {filepath}", file=sys.stderr)
        return None

def extract_parts_from_architectural_debt(content):
    """Extract Part 2 and Part 3 from architectural debt report."""
    # Look for Part 1 (Architectural Placement) and Part 2 (Configuration Coupling)
    lines = content.split('\n')

    # Find the start of Part 1
    part1_start = None
    part2_start = None

    for i, line in enumerate(lines):
        if 'Part 1:' in line or 'Part 1 :' in line or '## Part 1:' in line:
            part1_start = i
        elif 'Part 2:' in line or 'Part 2 :' in line or '## Part 2:' in line:
            part2_start = i

    if part1_start is None:
        print("WARNING: Could not find Part 1 marker in architectural debt report")
        # Try to find architectural placement section
        for i, line in enumerate(lines):
            if 'Architectural Placement' in line or 'Architectural' in line and 'Analysis' in line:
                part1_start = i
                break

    if part1_start is not None:
        # Return from Part 1 onwards
        return '\n'.join(lines[part1_start:])
    else:
        print("WARNING: Returning full architectural debt content")
        return content

def consolidate_report(filename, dry_run=False):
    """Consolidate a single file's reports."""
    print(f"\n{'[DRY RUN] ' if dry_run else ''}Processing {filename}...")

    standard_file = REPORTS_DIR / f"PHASE_10.02.02_refactor_report_on_{filename}.md"
    arch_debt_file = REPORTS_DIR / f"PHASE_10.02.02_ARCHITECTURAL_DEBT_{filename}.md"

    # Read both files
    standard_content = read_file(standard_file)
    arch_debt_content = read_file(arch_debt_file)

    if standard_content is None or arch_debt_content is None:
        print(f"SKIP: Missing files for {filename}")
        return False

    # Extract parts from architectural debt report
    arch_parts = extract_parts_from_architectural_debt(arch_debt_content)

    # Create consolidated report
    # Find where to insert architectural parts (after the extraction analysis)
    standard_lines = standard_content.split('\n')

    # Find a good insertion point (before conclusion or at end)
    insert_idx = len(standard_lines)
    for i, line in enumerate(standard_lines):
        if '## Conclusion' in line or '## Final' in line or '## Summary' in line:
            insert_idx = i
            break

    # Build consolidated content
    consolidated_lines = standard_lines[:insert_idx]
    consolidated_lines.append('')
    consolidated_lines.append('---')
    consolidated_lines.append('')
    consolidated_lines.extend(arch_parts.split('\n'))
    consolidated_lines.append('')
    consolidated_lines.append('---')
    consolidated_lines.append('')
    consolidated_lines.extend(standard_lines[insert_idx:])

    consolidated_content = '\n'.join(consolidated_lines)

    if not dry_run:
        # Write consolidated report
        try:
            with open(standard_file, 'w', encoding='utf-8') as f:
                f.write(consolidated_content)
            print(f"✅ CONSOLIDATED: {standard_file.name}")
            return True
        except Exception as e:
            print(f"ERROR writing consolidated report: {e}", file=sys.stderr)
            return False
    else:
        print(f"✅ WOULD CONSOLIDATE: {standard_file.name}")
        print(f"   Standard report: {len(standard_lines)} lines")
        print(f"   Architectural debt: {len(arch_parts.split(chr(10)))} lines")
        print(f"   Consolidated: {len(consolidated_lines)} lines")
        return True

def main():
    parser = argparse.ArgumentParser(description='Consolidate Phase 10.2.2 split reports')
    parser.add_argument('--dry-run', action='store_true', help='Show what would be done without making changes')
    args = parser.parse_args()

    print(f"Phase 10.2.2 Report Consolidation Tool")
    print(f"{'='*60}")
    print(f"Mode: {'DRY RUN' if args.dry_run else 'LIVE CONSOLIDATION'}")
    print(f"Reports directory: {REPORTS_DIR}")

    success_count = 0
    skip_count = 0

    for filename, already_done in FILES_TO_CONSOLIDATE:
        if already_done:
            print(f"\n✓ SKIP: {filename} (already consolidated)")
            skip_count += 1
            continue

        if consolidate_report(filename, args.dry_run):
            success_count += 1

    print(f"\n{'='*60}")
    print(f"Summary:")
    print(f"  Consolidated: {success_count}")
    print(f"  Skipped: {skip_count}")
    print(f"  Total: {len(FILES_TO_CONSOLIDATE)}")

    if not args.dry_run and success_count > 0:
        print(f"\n✅ Consolidation complete!")
        print(f"Next steps:")
        print(f"  1. Review consolidated reports")
        print(f"  2. Delete ARCHITECTURAL_DEBT_*.md files")
        print(f"  3. Update TODO list")

    return 0 if success_count == len(FILES_TO_CONSOLIDATE) - skip_count else 1

if __name__ == '__main__':
    sys.exit(main())
