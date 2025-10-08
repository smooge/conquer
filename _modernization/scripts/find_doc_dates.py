#!/usr/bin/env -S uv run --script
# /// script
# dependencies = []
# ///
"""
find_doc_dates.py - Find when function documentation was added using git history

Uses git blame to determine when documentation blocks were added to functions.
Reports the date each function's documentation was committed.

Usage:
    python3 find_doc_dates.py <file.c>
    python3 find_doc_dates.py misc.c

Output:
    Lists each function with:
    - Function name
    - Line number
    - Date documentation was added (from git blame)
    - Commit hash

Examples:
    python3 find_doc_dates.py misc.c
    python3 find_doc_dates.py misc.c > doc_dates.txt
"""

import subprocess
import re
import sys
import os


def has_documentation_block(lines, func_line_idx):
    """Check if function has documentation and return the block range."""
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
                # Found comment block - check if it's substantial
                comment_start = i

                # Count non-empty comment lines
                comment_lines = 0
                for j in range(comment_start, comment_end + 1):
                    line = lines[j].strip()
                    if line and line not in ['/*', '*/', '*']:
                        comment_lines += 1

                # Consider it documented if it has at least 3 lines of content
                if comment_lines >= 3:
                    return (comment_start, comment_end)
                else:
                    return None
            i -= 1

    return None


def find_functions(filepath):
    """Find all functions in a C file with their line numbers."""
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

                    functions.append({
                        'name': func_name,
                        'line': i + 1,
                        'has_docs': doc_range is not None,
                        'doc_range': doc_range
                    })

        # Pattern 2: return type and function on same line
        func_match = re.match(r'^(void|int|char|short|long|unsigned|static|struct)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\(([^)]*)\)', line)
        if func_match:
            func_name = func_match.group(2)
            doc_range = has_documentation_block(lines, i)

            functions.append({
                'name': func_name,
                'line': i + 1,
                'has_docs': doc_range is not None,
                'doc_range': doc_range
            })

        i += 1

    return functions


def get_git_blame_date(filepath, line_num):
    """Get the date when a specific line was last modified using git blame."""
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
                import datetime
                date = datetime.datetime.fromtimestamp(timestamp)
                return date.strftime('%Y-%m-%d')
            if line.startswith('committer-time '):
                # Fallback to committer time
                timestamp = int(line.split()[1])
                import datetime
                date = datetime.datetime.fromtimestamp(timestamp)
                return date.strftime('%Y-%m-%d')

        return "unknown"
    except subprocess.CalledProcessError:
        return "unknown"
    except Exception as e:
        return f"error: {e}"


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 find_doc_dates.py <file.c>", file=sys.stderr)
        return 1

    filepath = sys.argv[1]

    if not os.path.exists(filepath):
        print(f"Error: File {filepath} not found", file=sys.stderr)
        return 1

    if not filepath.endswith('.c'):
        print(f"Warning: {filepath} is not a .c file", file=sys.stderr)

    print(f"Analyzing function documentation dates in: {filepath}")
    print("=" * 80)

    functions = find_functions(filepath)

    if not functions:
        print("\nNo functions found.")
        return 0

    documented_funcs = [f for f in functions if f['has_docs']]

    if not documented_funcs:
        print("\nNo documented functions found.")
        return 0

    print(f"\nDocumented functions ({len(documented_funcs)}):\n")

    for func in documented_funcs:
        # Get date from the start of the documentation block
        doc_start_line = func['doc_range'][0] + 1  # Convert to 1-indexed
        date = get_git_blame_date(filepath, doc_start_line)

        print(f"  {func['name']:30s} line {func['line']:5d}  @last_documented: {date}")

    return 0


if __name__ == '__main__':
    sys.exit(main())
