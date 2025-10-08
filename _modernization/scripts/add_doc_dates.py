#!/usr/bin/env -S uv run --script
# /// script
# dependencies = []
# ///
"""
add_doc_dates.py - Add @last_documented dates to function documentation

Scans C source files for functions with comprehensive documentation, uses git blame
to determine when the documentation was added, and inserts @last_documented field
for documentation added during modernization (2025-09-01 or later).

ONLY adds @last_documented to functions with Phase 8.5.4+ documentation.
Legacy documentation (pre-2025-09-01) is skipped.

Usage:
    python3 add_doc_dates.py <file.c> [options]

Options:
    --dry-run       Preview changes without modifying files
    --backup        Create .orig backup before modifying
    --cutoff-date   Date cutoff for modernization (default: 2025-09-01)

Examples:
    python3 add_doc_dates.py misc.c --dry-run
    python3 add_doc_dates.py misc.c --backup
    python3 add_doc_dates.py commands.c --cutoff-date 2025-10-01
"""

import subprocess
import re
import sys
import os
import shutil
from datetime import datetime


def has_documentation_block(lines, func_line_idx):
    """Check if function has comprehensive Phase 8.5.4 documentation.

    Returns the block range only if the documentation includes "Testing Notes:"
    which is the marker for comprehensive modernization documentation vs. legacy comments.
    """
    i = func_line_idx - 1

    # Skip blank lines
    while i >= 0 and not lines[i].strip():
        i -= 1

    if i < 0:
        return None

    # Check if we hit a comment closing */
    if lines[i].strip().endswith('*/'):
        # Find the opening /*
        comment_end = i
        while i >= 0:
            if lines[i].strip().startswith('/*'):
                # Found comment block
                comment_start = i

                # Check if this is comprehensive Phase 8.5.4 documentation
                # by looking for "Testing Notes:" marker
                has_testing_notes = False
                for j in range(comment_start, comment_end + 1):
                    if 'Testing Notes:' in lines[j]:
                        has_testing_notes = True
                        break

                # Only return range if it has comprehensive documentation
                if has_testing_notes:
                    return (comment_start, comment_end)
                else:
                    return None
            i -= 1

    return None


def has_last_documented_tag(lines, doc_range):
    """Check if documentation block already has @last_documented tag."""
    start, end = doc_range
    for i in range(start, end + 1):
        if '@last_documented:' in lines[i]:
            return True
    return False


def find_functions(filepath):
    """Find all functions in a C file with their line numbers and doc blocks."""
    functions = []

    try:
        with open(filepath, 'r', encoding='latin-1') as f:
            lines = f.readlines()
    except Exception as e:
        print(f"Error reading {filepath}: {e}", file=sys.stderr)
        return functions

    i = 0
    while i < len(lines):
        line = lines[i].rstrip()

        # Look for function definitions
        # Pattern 1: return type on separate line
        if re.match(r'^(void|int|char|short|long|unsigned|static|struct)\s*$', line):
            if i + 1 < len(lines):
                next_line = lines[i + 1].strip()
                func_match = re.match(r'^([a-zA-Z_][a-zA-Z0-9_]*)\s*\(([^)]*)\)', next_line)
                if func_match:
                    func_name = func_match.group(1)
                    doc_range = has_documentation_block(lines, i)

                    if doc_range:
                        functions.append({
                            'name': func_name,
                            'line': i + 1,
                            'doc_range': doc_range
                        })

        # Pattern 2: return type and function on same line
        func_match = re.match(r'^(void|int|char|short|long|unsigned|static|struct)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\(([^)]*)\)', line)
        if func_match:
            func_name = func_match.group(2)
            doc_range = has_documentation_block(lines, i)

            if doc_range:
                functions.append({
                    'name': func_name,
                    'line': i + 1,
                    'doc_range': doc_range
                })

        i += 1

    return functions


def get_git_blame_date(filepath, line_num):
    """Get the date when a specific line was last modified using git blame.

    Note: For documentation blocks, we check a line INSIDE the block (not the
    opening /* line) because git blame may attribute the /* to an old commit
    even if the content was rewritten.
    """
    try:
        # Use git blame with porcelain format for easy parsing
        result = subprocess.run(
            ['git', 'blame', '--porcelain', '-L', f'{line_num},{line_num}', filepath],
            capture_output=True,
            text=True,
            check=True
        )

        # Parse porcelain output
        for line in result.stdout.split('\n'):
            if line.startswith('author-time '):
                # Unix timestamp
                timestamp = int(line.split()[1])
                # Convert to date
                date = datetime.fromtimestamp(timestamp)
                return date.strftime('%Y-%m-%d')

        return None
    except subprocess.CalledProcessError:
        return None
    except Exception as e:
        print(f"Warning: git blame error for {filepath}:{line_num}: {e}", file=sys.stderr)
        return None


def insert_last_documented_tag(lines, doc_range, date_str):
    """Insert @last_documented tag before closing */ of documentation block."""
    start, end = doc_range

    # Find the closing */ line
    close_idx = end

    # Insert the tag before the closing */
    # Find the indentation of the comment block
    indent = ''
    for i in range(start, end + 1):
        if lines[i].strip().startswith('*') and not lines[i].strip().startswith('/*'):
            # Get indentation from existing comment line
            indent = lines[i][:lines[i].index('*')]
            break

    # Create the new tag line with proper indentation
    tag_line = f"{indent} * @last_documented: {date_str}\n"

    # Insert before the closing */
    lines.insert(close_idx, tag_line)

    return lines


def process_file(filepath, dry_run=False, backup=False, cutoff_date='2025-09-01'):
    """Process a file and add @last_documented tags where appropriate."""

    cutoff = datetime.strptime(cutoff_date, '%Y-%m-%d')

    print(f"\nAnalyzing: {filepath}")
    print("=" * 80)

    functions = find_functions(filepath)

    if not functions:
        print("No documented functions found.")
        return 0

    # Read file for modification
    try:
        with open(filepath, 'r', encoding='latin-1') as f:
            lines = f.readlines()
    except Exception as e:
        print(f"Error reading {filepath}: {e}", file=sys.stderr)
        return 1

    # Track changes
    added_count = 0
    skipped_legacy = 0
    skipped_existing = 0

    # Process functions in reverse order so line numbers remain valid
    for func in reversed(functions):
        # Check if already has @last_documented tag
        if has_last_documented_tag(lines, func['doc_range']):
            skipped_existing += 1
            if dry_run:
                print(f"  SKIP (has tag):   {func['name']:30s} line {func['line']}")
            continue

        # Get date from git blame - check 2 lines into the block to avoid
        # getting the date of the original /* opening from old commits
        doc_start_line = func['doc_range'][0] + 2  # Skip /* and check actual content
        date_str = get_git_blame_date(filepath, doc_start_line)

        if not date_str:
            print(f"  SKIP (no date):   {func['name']:30s} line {func['line']}", file=sys.stderr)
            continue

        # Check if documentation is from modernization period
        doc_date = datetime.strptime(date_str, '%Y-%m-%d')

        if doc_date < cutoff:
            skipped_legacy += 1
            if dry_run:
                print(f"  SKIP (legacy):    {func['name']:30s} line {func['line']:5d}  date: {date_str}")
            continue

        # Add the tag
        if dry_run:
            print(f"  WOULD ADD:        {func['name']:30s} line {func['line']:5d}  @last_documented: {date_str}")
        else:
            print(f"  ADDING:           {func['name']:30s} line {func['line']:5d}  @last_documented: {date_str}")
            lines = insert_last_documented_tag(lines, func['doc_range'], date_str)

        added_count += 1

    # Summary
    print("\n" + "=" * 80)
    print(f"Summary:")
    print(f"  Total documented functions: {len(functions)}")
    print(f"  Would add @last_documented: {added_count}" if dry_run else f"  Added @last_documented: {added_count}")
    print(f"  Skipped (legacy pre-{cutoff_date}): {skipped_legacy}")
    print(f"  Skipped (already tagged): {skipped_existing}")

    if not dry_run and added_count > 0:
        # Create backup if requested
        if backup:
            backup_path = filepath + '.orig'
            shutil.copy2(filepath, backup_path)
            print(f"\nBackup created: {backup_path}")

        # Write modified file
        try:
            with open(filepath, 'w', encoding='latin-1') as f:
                f.writelines(lines)
            print(f"\n✅ File updated: {filepath}")
        except Exception as e:
            print(f"\n❌ Error writing {filepath}: {e}", file=sys.stderr)
            return 1

    return 0


def main():
    """Main entry point."""
    if len(sys.argv) < 2:
        print("Usage: python3 add_doc_dates.py <file.c> [--dry-run] [--backup] [--cutoff-date YYYY-MM-DD]", file=sys.stderr)
        return 1

    filepath = sys.argv[1]
    dry_run = '--dry-run' in sys.argv
    backup = '--backup' in sys.argv

    # Parse cutoff date if provided
    cutoff_date = '2025-09-01'
    for i, arg in enumerate(sys.argv):
        if arg == '--cutoff-date' and i + 1 < len(sys.argv):
            cutoff_date = sys.argv[i + 1]

    if not os.path.exists(filepath):
        print(f"Error: File {filepath} not found", file=sys.stderr)
        return 1

    if not filepath.endswith('.c'):
        print(f"Warning: {filepath} is not a .c file", file=sys.stderr)

    if dry_run:
        print("🔍 DRY RUN MODE - No files will be modified")

    return process_file(filepath, dry_run=dry_run, backup=backup, cutoff_date=cutoff_date)


if __name__ == '__main__':
    sys.exit(main())
