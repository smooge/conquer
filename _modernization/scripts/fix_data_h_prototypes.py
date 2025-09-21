#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["re", "argparse", "pathlib", "shutil"]
# ///

"""
fix_data_h_prototypes.py - Fix K&R function prototypes in data.h

This script systematically converts old K&R style function prototypes
from 'functionname()' to 'functionname(void)' to eliminate
-Wstrict-prototypes warnings when compiling with intensive warning flags.

The script specifically targets function prototype declarations in data.h
that use the pattern:
  extern type functionname(), otherfunc(), ...;

And converts them to:
  extern type functionname(void), otherfunc(void), ...;

Usage:
  python3 fix_data_h_prototypes.py data.h [--dry-run] [--backup]

Options:
  --dry-run    Show changes without modifying file
  --backup     Create .orig backup file before changes
"""

import re
import argparse
import pathlib
import shutil
import sys

def fix_function_prototypes(content):
    """
    Fix K&R style function prototypes by adding (void) parameter lists.

    Converts patterns like:
    - extern int func();
    - extern int func(), other();
    - extern type func1(), func2(), func3();

    To:
    - extern int func(void);
    - extern int func(void), other(void);
    - extern type func1(void), func2(void), func3(void);
    """
    changes_made = []

    # Pattern to match function declarations with empty parentheses
    # Matches: functionname() but not functionname(parameters)
    pattern = r'(\w+)\(\s*\)'

    def replace_prototype(match):
        func_name = match.group(1)
        # Skip if this looks like a function call in a comment or string
        return f'{func_name}(void)'

    lines = content.split('\n')
    modified_lines = []

    for line_num, line in enumerate(lines, 1):
        original_line = line

        # Only process lines that contain extern function declarations
        if 'extern' in line and '()' in line and not '//' in line and not '/*' in line:
            # Apply the replacement
            modified_line = re.sub(pattern, replace_prototype, line)

            if modified_line != original_line:
                changes_made.append({
                    'line': line_num,
                    'before': original_line.strip(),
                    'after': modified_line.strip()
                })

            modified_lines.append(modified_line)
        else:
            modified_lines.append(line)

    return '\n'.join(modified_lines), changes_made

def main():
    parser = argparse.ArgumentParser(
        description='Fix K&R function prototypes in data.h',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument('file', help='data.h file to process')
    parser.add_argument('--dry-run', action='store_true',
                       help='Show changes without modifying file')
    parser.add_argument('--backup', action='store_true',
                       help='Create .orig backup before changes')

    args = parser.parse_args()

    file_path = pathlib.Path(args.file)

    if not file_path.exists():
        print(f"Error: File {file_path} does not exist")
        sys.exit(1)

    # Read original content
    try:
        content = file_path.read_text()
    except Exception as e:
        print(f"Error reading {file_path}: {e}")
        sys.exit(1)

    # Process the content
    modified_content, changes = fix_function_prototypes(content)

    # Report changes
    if changes:
        print(f"Found {len(changes)} function prototype fixes needed:")
        print("=" * 60)

        for change in changes:
            print(f"Line {change['line']:3d}: {change['before']}")
            print(f"       -> {change['after']}")
            print()

        if args.dry_run:
            print("DRY RUN: No changes made to file")
            return

        # Create backup if requested
        if args.backup:
            backup_path = file_path.with_suffix(file_path.suffix + '.orig')
            shutil.copy2(file_path, backup_path)
            print(f"Backup created: {backup_path}")

        # Write modified content
        try:
            file_path.write_text(modified_content)
            print(f"Successfully updated {file_path}")
            print(f"Fixed {len(changes)} function prototype declarations")
        except Exception as e:
            print(f"Error writing to {file_path}: {e}")
            sys.exit(1)
    else:
        print("No function prototype fixes needed")

if __name__ == '__main__':
    main()