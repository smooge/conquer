#!/usr/bin/env -S uv run --script
# /// script
# dependencies = []
# ///
"""
analyze_function_docs.py - Analyze function documentation coverage

Scans C source files to identify functions with and without documentation,
helping prioritize documentation efforts.

Usage:
    python3 analyze_function_docs.py [path] [--summary|--missing|--all]

    path can be:
        - A directory (analyzes all .c files in that directory)
        - A single .c file (analyzes just that file)
        - Defaults to current directory if not specified

Options:
    --summary     Show summary statistics only (default)
    --missing     Show only functions missing documentation
    --all         Show all functions with documentation status

Examples:
    python3 analyze_function_docs.py .
    python3 analyze_function_docs.py misc.c --missing
    python3 analyze_function_docs.py /path/to/file.c --all
    python3 analyze_function_docs.py /path/to/directory --summary
"""

import re
import os
import sys


def has_documentation(lines, func_line_idx):
    """Check if function has comprehensive Phase 8.5.4 documentation.

    Returns True only if the documentation includes "Testing Notes:" marker,
    which distinguishes comprehensive modernization documentation from legacy comments.
    """

    # Look backwards from function line for comment block
    i = func_line_idx - 1

    # Skip blank lines
    while i >= 0 and not lines[i].strip():
        i -= 1

    if i < 0:
        return False

    # Check if we hit a comment closing */
    if lines[i].strip().endswith('*/'):
        # Find the opening /*
        comment_end = i
        while i >= 0:
            if lines[i].strip().startswith('/*'):
                # Found comment block - check if it's comprehensive Phase 8.5.4 documentation
                comment_start = i

                # Look for "Testing Notes:" marker
                for j in range(comment_start, comment_end + 1):
                    if 'Testing Notes:' in lines[j]:
                        return True

                # No Testing Notes found - not comprehensive documentation
                return False
            i -= 1

    return False


def analyze_file(filepath):
    """Analyze a single C file for function documentation."""
    filename = os.path.basename(filepath)
    functions = []

    try:
        with open(filepath, 'r', encoding='latin-1') as f:
            lines = f.readlines()
    except Exception as e:
        print(f"Warning: Could not read {filepath}: {e}")
        return functions

    i = 0
    while i < len(lines):
        line = lines[i].rstrip()

        # Look for function definitions (skip declarations ending with ;)
        # Pattern 1: return type on separate line
        if re.match(r'^(void|int|char|short|long|unsigned|static|struct)\s*$', line):
            if i + 1 < len(lines):
                next_line = lines[i + 1].strip()
                func_match = re.match(r'^([a-zA-Z_][a-zA-Z0-9_]*)\s*\(([^)]*)\)', next_line)
                if func_match:
                    # Skip function declarations (prototypes ending with ;)
                    if next_line.rstrip().endswith(';'):
                        i += 1
                        continue

                    func_name = func_match.group(1)
                    has_docs = has_documentation(lines, i)

                    functions.append({
                        'file': filename,
                        'line': i + 1,
                        'name': func_name,
                        'documented': has_docs
                    })

        # Pattern 2: return type and function on same line
        func_match = re.match(r'^(void|int|char|short|long|unsigned|static|struct)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\(([^)]*)\)', line)
        if func_match:
            # Skip function declarations (prototypes ending with ;)
            if line.rstrip().endswith(';'):
                i += 1
                continue

            func_name = func_match.group(2)
            has_docs = has_documentation(lines, i)

            functions.append({
                'file': filename,
                'line': i + 1,
                'name': func_name,
                'documented': has_docs
            })

        i += 1

    return functions


def find_functions(path='.'):
    """Find all functions and their documentation status.

    Args:
        path: Can be a directory (analyzes all .c files) or a single .c file
    """
    all_functions = []

    if os.path.isfile(path):
        # Analyze single file
        if path.endswith('.c'):
            all_functions = analyze_file(path)
        else:
            print(f"Error: {path} is not a C source file (.c)")
            return []
    elif os.path.isdir(path):
        # Analyze all .c files in directory
        c_files = sorted([f for f in os.listdir(path) if f.endswith('.c')])
        for filename in c_files:
            filepath = os.path.join(path, filename)
            all_functions.extend(analyze_file(filepath))
    else:
        print(f"Error: {path} does not exist")
        return []

    return all_functions


def main():
    """Main entry point."""
    args = sys.argv[1:]
    path = '.'
    mode = 'summary'

    for arg in args:
        if arg in ['--summary', '--missing', '--all']:
            mode = arg.replace('--', '')
        elif not arg.startswith('--'):
            path = arg

    print(f"Analyzing function documentation in: {path}")
    print("=" * 70)

    functions = find_functions(path)

    if not functions:
        print("\nNo functions found.")
        return 0

    documented = [f for f in functions if f['documented']]
    missing_docs = [f for f in functions if not f['documented']]

    coverage = (len(documented) / len(functions) * 100) if functions else 0

    if mode == 'summary':
        print(f"\nDocumentation Coverage Summary:")
        print(f"  Total functions: {len(functions)}")
        print(f"  Documented: {len(documented)} ({coverage:.1f}%)")
        print(f"  Missing docs: {len(missing_docs)} ({100-coverage:.1f}%)")

        # Group by file
        by_file = {}
        for func in functions:
            fname = func['file']
            if fname not in by_file:
                by_file[fname] = {'total': 0, 'documented': 0}
            by_file[fname]['total'] += 1
            if func['documented']:
                by_file[fname]['documented'] += 1

        print(f"\nPer-file coverage:")
        for filename in sorted(by_file.keys()):
            stats = by_file[filename]
            file_coverage = (stats['documented'] / stats['total'] * 100) if stats['total'] else 0
            print(f"  {filename:20s}: {stats['documented']:3d}/{stats['total']:3d} ({file_coverage:5.1f}%)")

    elif mode == 'missing':
        print(f"\nFunctions missing documentation ({len(missing_docs)}):\n")
        for func in missing_docs:
            print(f"  {func['file']:20s}:{func['line']:4d}  {func['name']}()")

    elif mode == 'all':
        print(f"\nAll functions ({len(functions)}):\n")
        for func in functions:
            status = "✓" if func['documented'] else "✗"
            print(f"  {status} {func['file']:20s}:{func['line']:4d}  {func['name']}()")

    return 0


if __name__ == '__main__':
    sys.exit(main())
