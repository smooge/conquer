#!/usr/bin/env -S uv run --script
# /// script
# dependencies = []
# ///
"""
find_kr_functions.py - Identify K&R style function definitions

Scans C source files to find old-style K&R function definitions where
parameter declarations appear after the function header.

Usage:
    python3 find_kr_functions.py [directory]

Example:
    python3 find_kr_functions.py .
    python3 find_kr_functions.py /path/to/source
"""

import re
import os
import sys
from pathlib import Path


def find_kr_functions(directory='.'):
    """Scan directory for K&R style function definitions."""

    # Find all .c files
    if os.path.isdir(directory):
        c_files = [f for f in os.listdir(directory) if f.endswith('.c')]
        base_path = directory
    else:
        print(f"Error: {directory} is not a directory")
        return []

    kr_functions = []

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

            # Look for function definition pattern
            # K&R style: function_name(param1, param2)
            #            type1 param1;
            #            type2 param2;
            #            {
            if re.match(r'^[a-zA-Z_][a-zA-Z0-9_]*\s*\([^)]*\)\s*$', line):
                # Check next line for parameter declarations (K&R style)
                if i + 1 < len(lines):
                    next_line = lines[i + 1].strip()
                    # K&R functions have parameter type declarations before brace
                    if next_line and not next_line.startswith('{'):
                        if not next_line.startswith('//') and not next_line.startswith('/*'):
                            # Check if it's a type declaration
                            if re.match(r'^(int|char|short|long|unsigned|struct|const|void|static|register|extern)\s+', next_line):
                                kr_functions.append({
                                    'file': filename,
                                    'line': i + 1,
                                    'signature': line.strip(),
                                    'next_line': next_line
                                })
            i += 1

    return kr_functions


def main():
    """Main entry point."""
    directory = sys.argv[1] if len(sys.argv) > 1 else '.'

    print(f"Scanning for K&R style functions in: {directory}")
    print("=" * 70)

    kr_funcs = find_kr_functions(directory)

    if not kr_funcs:
        print("\n✅ No K&R style functions found!")
        print("All functions appear to use modern ANSI C prototypes.")
        return 0

    print(f"\n⚠️  Found {len(kr_funcs)} potential K&R style functions:\n")

    for func in kr_funcs:
        print(f"  {func['file']}:{func['line']}")
        print(f"    {func['signature']}")
        print(f"    {func['next_line']}")
        print()

    print(f"\nTotal K&R functions found: {len(kr_funcs)}")
    return len(kr_funcs)


if __name__ == '__main__':
    sys.exit(main())
