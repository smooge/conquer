#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["regex", "pathlib", "argparse"]
# ///

"""
analyze_compilation.py - Parse GCC output and categorize warnings by type

This script analyzes compilation output from GCC to:
1. Parse warnings and errors by type and file
2. Generate categorized reports for session planning
3. Track warning reduction progress across subphases
4. Support the Phase 4 systematic methodology

Usage:
    python3 analyze_compilation.py input.log --output-dir reports/
    python3 analyze_compilation.py --test-mode  # Test with current directory compilation
"""

import argparse
import pathlib
import re
import subprocess
import sys
from collections import defaultdict
from datetime import datetime

# Standard compilation configuration from Phase 4 Strategy
FEATURE_TEST_MACROS = [
    '-D_POSIX_C_SOURCE=200809L',
    '-D_XOPEN_SOURCE=700',
    '-D_DEFAULT_SOURCE'
]

ESSENTIAL_FLAGS = [
    '-DDEFAULTDIR="/home/ssmoogen/conquer/lib"',
    '-DEXEDIR="/home/ssmoogen/conquer/bin"',
    '-DVERSION="4"',
    '-DPATCHLEVEL="12"',
    '-DLOGIN="ssmoogen"'
]

# Subphase flag progression from strategy
SUBPHASE_FLAGS = {
    'subphase_1': ['-std=c99'] + FEATURE_TEST_MACROS,  # Basic C99 compilation
    'subphase_2': ['-std=c99'] + FEATURE_TEST_MACROS,  # C99 error-free
    'subphase_3': ['-std=c99', '-Wall'] + FEATURE_TEST_MACROS,  # Add -Wall
    'subphase_4': ['-std=c99', '-Wall', '-Wextra'] + FEATURE_TEST_MACROS,  # Add -Wextra
    'subphase_5': ['-std=c99', '-Wall', '-Wextra', '-Wpedantic'] + FEATURE_TEST_MACROS,  # Add -Wpedantic
    'subphase_6': ['-std=c99', '-Wall', '-Wextra', '-Wpedantic', '-Wformat=2'] + FEATURE_TEST_MACROS,  # Add format warnings
    'subphase_7': ['-std=c2x', '-Wall', '-Wextra', '-Wpedantic', '-Wformat=2', '-Wconversion', '-Wsign-conversion', '-Wimplicit-fallthrough', '-fanalyzer', '-Wstrict-prototypes', '-Wold-style-declaration'] + FEATURE_TEST_MACROS  # Full C2x strictness
}

class CompilationAnalyzer:
    def __init__(self):
        self.warnings_by_file = defaultdict(list)
        self.errors_by_file = defaultdict(list)
        self.warning_categories = defaultdict(int)
        self.file_stats = defaultdict(dict)

    def parse_gcc_output(self, output_text):
        """Parse GCC warning/error output into structured data"""
        lines = output_text.strip().split('\n')

        for line in lines:
            # Parse GCC output format: filename:line:column: type: message
            match = re.match(r'^([^:]+):(\d+):(\d+):\s+(warning|error):\s+(.+?)(?:\s+\[-W([^\]]+)\])?$', line)
            if match:
                filename, line_num, col, msg_type, message, warning_flag = match.groups()

                entry = {
                    'file': filename,
                    'line': int(line_num),
                    'column': int(col),
                    'type': msg_type,
                    'message': message,
                    'warning_flag': warning_flag or 'unknown'
                }

                if msg_type == 'warning':
                    self.warnings_by_file[filename].append(entry)
                    self.warning_categories[warning_flag or 'unknown'] += 1
                else:  # error
                    self.errors_by_file[filename].append(entry)

    def test_file_compilation(self, filename, subphase='subphase_3', config='CONQUER'):
        """Test compilation of a single file with specified subphase flags"""
        flags = SUBPHASE_FLAGS.get(subphase, SUBPHASE_FLAGS['subphase_3'])

        # Configuration-specific flags
        config_flags = ESSENTIAL_FLAGS.copy()
        if config == 'CONQUER':
            config_flags.append('-DCONQUER')
        elif config == 'ADMIN':
            config_flags.extend(['-DADMIN', '-DCONQUER'])

        cmd = ['gcc', '-O2', '-g'] + flags + config_flags + ['-c', filename, '-o', '/tmp/foo.o']

        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            return {
                'returncode': result.returncode,
                'stdout': result.stdout,
                'stderr': result.stderr,
                'cmd': ' '.join(cmd)
            }
        except subprocess.TimeoutExpired:
            return {
                'returncode': -1,
                'stdout': '',
                'stderr': 'Compilation timeout',
                'cmd': ' '.join(cmd)
            }

    def analyze_all_files(self, subphase='subphase_3'):
        """Test compilation of all C files in current directory"""
        c_files = list(pathlib.Path('.').glob('*.c'))
        h_files = list(pathlib.Path('.').glob('*.h'))

        results = {}

        # Test header files with test harness
        if h_files:
            test_content = []
            test_content.append('#include "patchlevel.h"  /* needs to go first */')
            test_content.append('#include "header.h"     /* needed for other headers */')

            for h_file in h_files:
                test_content.append(f'#include "{h_file.name}"')

            test_content.extend([
                '',
                'int main() {',
                '    return 0;',
                '}'
            ])

            # Write test file
            test_file = pathlib.Path('test_headers.c')
            test_file.write_text('\n'.join(test_content))

            try:
                header_result = self.test_file_compilation('test_headers.c', subphase)
                results['test_headers.c'] = header_result
                if header_result['stderr']:
                    self.parse_gcc_output(header_result['stderr'])
            finally:
                test_file.unlink(missing_ok=True)

        # Test individual C files
        for c_file in c_files:
            result = self.test_file_compilation(str(c_file), subphase)
            results[str(c_file)] = result

            if result['stderr']:
                self.parse_gcc_output(result['stderr'])

            # For shared files, also test ADMIN configuration
            if c_file.stem in ['cexecute', 'io', 'misc', 'navy', 'magic', 'data', 'trade']:
                admin_result = self.test_file_compilation(str(c_file), subphase, 'ADMIN')
                results[f'{c_file}_ADMIN'] = admin_result
                if admin_result['stderr']:
                    self.parse_gcc_output(admin_result['stderr'])

        return results

    def generate_report(self, output_dir=None):
        """Generate comprehensive analysis report"""
        if output_dir:
            output_path = pathlib.Path(output_dir)
            output_path.mkdir(parents=True, exist_ok=True)
        else:
            output_path = pathlib.Path('.')

        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        report_file = output_path / f'compilation_analysis_{timestamp}.md'

        report = []
        report.append('# Compilation Analysis Report')
        report.append(f'**Generated**: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}')
        report.append('')

        # Summary statistics
        total_warnings = sum(len(warnings) for warnings in self.warnings_by_file.values())
        total_errors = sum(len(errors) for errors in self.errors_by_file.values())
        files_with_warnings = len(self.warnings_by_file)
        files_with_errors = len(self.errors_by_file)

        report.append('## Summary Statistics')
        report.append(f'- **Total Warnings**: {total_warnings}')
        report.append(f'- **Total Errors**: {total_errors}')
        report.append(f'- **Files with Warnings**: {files_with_warnings}')
        report.append(f'- **Files with Errors**: {files_with_errors}')
        report.append('')

        # Warning categories
        if self.warning_categories:
            report.append('## Warning Categories')
            for category, count in sorted(self.warning_categories.items(), key=lambda x: x[1], reverse=True):
                report.append(f'- **{category}**: {count}')
            report.append('')

        # Files with errors (blockers)
        if self.errors_by_file:
            report.append('## Files with Compilation Errors (Blockers)')
            for filename, errors in sorted(self.errors_by_file.items()):
                report.append(f'### {filename} ({len(errors)} errors)')
                for error in errors[:5]:  # Show first 5 errors
                    report.append(f'- Line {error["line"]}: {error["message"]}')
                if len(errors) > 5:
                    report.append(f'- ... and {len(errors) - 5} more errors')
                report.append('')

        # Files with warnings
        if self.warnings_by_file:
            report.append('## Files with Warnings')
            for filename, warnings in sorted(self.warnings_by_file.items(), key=lambda x: len(x[1]), reverse=True):
                report.append(f'### {filename} ({len(warnings)} warnings)')

                # Group warnings by category
                warning_groups = defaultdict(list)
                for warning in warnings:
                    warning_groups[warning['warning_flag']].append(warning)

                for category, cat_warnings in sorted(warning_groups.items()):
                    report.append(f'- **{category}**: {len(cat_warnings)} warnings')
                    for warning in cat_warnings[:3]:  # Show first 3 of each type
                        report.append(f'  - Line {warning["line"]}: {warning["message"]}')
                    if len(cat_warnings) > 3:
                        report.append(f'  - ... and {len(cat_warnings) - 3} more')
                report.append('')

        # File classification for session planning
        report.append('## File Classification for Session Planning')
        for filename in sorted(set(list(self.warnings_by_file.keys()) + list(self.errors_by_file.keys()))):
            warning_count = len(self.warnings_by_file.get(filename, []))
            error_count = len(self.errors_by_file.get(filename, []))

            # Simple classification based on warning count
            if error_count > 0:
                classification = "🔴 BLOCKED"
            elif warning_count == 0:
                classification = "✅ CLEAN"
            elif warning_count <= 5:
                classification = "🟢 SMALL"
            elif warning_count <= 15:
                classification = "🟡 MEDIUM"
            elif warning_count <= 30:
                classification = "🟠 LARGE"
            else:
                classification = "🔴 COMPLEX"

            report.append(f'- **{filename}**: {classification} ({error_count} errors, {warning_count} warnings)')

        report.append('')
        report.append('## Recommendations')

        if self.errors_by_file:
            report.append('### Priority 1: Fix Compilation Errors')
            for filename in sorted(self.errors_by_file.keys()):
                report.append(f'- Fix {filename} compilation errors before proceeding')

        if self.warning_categories:
            top_categories = sorted(self.warning_categories.items(), key=lambda x: x[1], reverse=True)[:3]
            report.append('### Priority 2: Address Top Warning Categories')
            for category, count in top_categories:
                report.append(f'- **{category}**: {count} instances - consider category-focused session')

        # Write report
        report_content = '\n'.join(report)
        report_file.write_text(report_content)

        return str(report_file)

def main():
    parser = argparse.ArgumentParser(description='Analyze compilation output for Phase 4 warning elimination')
    parser.add_argument('input', nargs='?', help='Input compilation log file')
    parser.add_argument('--output-dir', default='_modernization/claude/reports/',
                       help='Output directory for reports')
    parser.add_argument('--test-mode', action='store_true',
                       help='Test compilation of files in current directory')
    parser.add_argument('--subphase', default='subphase_3',
                       choices=list(SUBPHASE_FLAGS.keys()),
                       help='Subphase flag set to use for testing')
    parser.add_argument('--dry-run', action='store_true',
                       help='Show what would be done without writing files')

    args = parser.parse_args()

    analyzer = CompilationAnalyzer()

    if args.test_mode:
        print(f"Testing compilation with {args.subphase} flags...")
        results = analyzer.analyze_all_files(args.subphase)

        # Print summary
        total_files = len(results)
        failed_files = sum(1 for r in results.values() if r['returncode'] != 0)
        print(f"Tested {total_files} files, {failed_files} failed compilation")

    elif args.input:
        # Parse existing log file
        input_path = pathlib.Path(args.input)
        if not input_path.exists():
            print(f"Error: Input file {args.input} not found")
            sys.exit(1)

        log_content = input_path.read_text()
        analyzer.parse_gcc_output(log_content)
    else:
        print("Error: Must specify input file or use --test-mode")
        sys.exit(1)

    # Generate report
    if not args.dry_run:
        report_file = analyzer.generate_report(args.output_dir)
        print(f"Generated analysis report: {report_file}")
    else:
        print("Dry run - report generation skipped")

if __name__ == '__main__':
    main()