#!/usr/bin/env -S uv run --script
# /// script
# dependencies = []
# ///
"""
find_void_functions.py - Identify void functions that might need return values

Scans C source files to find void functions that could benefit from
returning status codes for better error handling.

Usage:
    python3 find_void_functions.py [directory] [--detailed]

Options:
    --detailed    Show function signatures and context

Example:
    python3 find_void_functions.py .
    python3 find_void_functions.py /path/to/source --detailed
"""

import re
import os
import sys
from pathlib import Path


def find_void_functions(directory='.', detailed=False):
    """Scan directory for void function definitions."""

    # Find all .c files
    if os.path.isdir(directory):
        c_files = sorted([f for f in os.listdir(directory) if f.endswith('.c')])
        base_path = directory
    else:
        print(f"Error: {directory} is not a directory")
        return []

    void_functions = []

    for filename in c_files:
        filepath = os.path.join(base_path, filename)
        try:
            with open(filepath, 'r', encoding='latin-1') as f:
                lines = f.readlines()
        except Exception as e:
            print(f"Warning: Could not read {filename}: {e}")
            continue

        i = 0
        while i < len(lines):
            line = lines[i].rstrip()

            # Look for void return type on its own line
            if re.match(r'^void\s*$', line):
                # Next line should have function name and parameters
                if i + 1 < len(lines):
                    next_line = lines[i + 1].strip()
                    func_match = re.match(r'^([a-zA-Z_][a-zA-Z0-9_]*)\s*\(([^)]*)\)', next_line)
                    if func_match:
                        func_name = func_match.group(1)
                        params = func_match.group(2)

                        void_functions.append({
                            'file': filename,
                            'line': i + 1,
                            'name': func_name,
                            'params': params,
                            'full_sig': f"void {func_name}({params})"
                        })

            # Also check for inline void functions: void func_name(...)
            elif re.match(r'^void\s+[a-zA-Z_]', line):
                func_match = re.match(r'^void\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\(([^)]*)\)', line)
                if func_match:
                    func_name = func_match.group(1)
                    params = func_match.group(2)

                    void_functions.append({
                        'file': filename,
                        'line': i + 1,
                        'name': func_name,
                        'params': params,
                        'full_sig': f"void {func_name}({params})"
                    })

            i += 1

    return void_functions


def main():
    """Main entry point."""
    args = sys.argv[1:]
    directory = '.'
    detailed = False

    for arg in args:
        if arg == '--detailed':
            detailed = True
        elif not arg.startswith('--'):
            directory = arg

    print(f"Scanning for void functions in: {directory}")
    print("=" * 70)

    void_funcs = find_void_functions(directory, detailed)

    if not void_funcs:
        print("\nNo void functions found.")
        return 0

    print(f"\nFound {len(void_funcs)} void functions:\n")

    # Group by file
    by_file = {}
    for func in void_funcs:
        fname = func['file']
        if fname not in by_file:
            by_file[fname] = []
        by_file[fname].append(func)

    for filename in sorted(by_file.keys()):
        funcs = by_file[filename]
        print(f"\n{filename}: {len(funcs)} void functions")

        if detailed:
            for func in funcs:
                print(f"  Line {func['line']:4d}: {func['name']}({func['params']})")
        else:
            func_names = [f['name'] for f in funcs]
            print(f"  {', '.join(func_names)}")

    print(f"\n{'=' * 70}")
    print(f"Total void functions: {len(void_funcs)}")
    print(f"Files with void functions: {len(by_file)}")

    return 0


if __name__ == '__main__':
    sys.exit(main())
