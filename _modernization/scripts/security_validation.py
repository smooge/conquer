#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["pathlib", "subprocess", "re", "argparse", "sys"]
# ///

"""
Security Validation Script for Conquer 4.x Modernization

This script validates that security improvements from Phase 7 are properly
implemented and tests for potential security regressions.

Usage:
    python3 security_validation.py [options]
    python3 security_validation.py --test-system-calls
    python3 security_validation.py --test-user-safety --verbose
    python3 security_validation.py --all --report security_report.md

Security Tests:
    - system-calls: Verify all system() calls have been eliminated
    - user-safety: Test user information null checking and safety
    - memory-safety: Validate memory operations and bounds checking
    - string-safety: Check for unsafe string operations
    - all: Run comprehensive security validation
"""

import re
import sys
import subprocess
import argparse
from pathlib import Path
from typing import Dict, List, Tuple, Optional
import json

class SecurityValidator:
    """Validates security improvements and detects potential regressions."""

    def __init__(self, base_path: Path = Path(".")):
        self.base_path = base_path
        self.source_extensions = {'.c', '.h'}
        self.exclude_dirs = {'historical', 'build', '.git', '__pycache__'}
        self.test_results = {}

    def find_source_files(self) -> List[Path]:
        """Find all C source and header files, excluding historical directories."""
        source_files = []

        for file_path in self.base_path.rglob('*'):
            if (file_path.is_file() and
                file_path.suffix in self.source_extensions and
                not any(exclude in file_path.parts for exclude in self.exclude_dirs)):
                source_files.append(file_path)

        return sorted(source_files)

    def test_system_call_elimination(self) -> Dict[str, any]:
        """Test that all unsafe system() calls have been eliminated."""
        print("Testing system() call elimination...")

        results = {
            'test_name': 'System Call Elimination',
            'status': 'PASS',
            'findings': [],
            'summary': {}
        }

        source_files = self.find_source_files()
        system_call_pattern = re.compile(r'\bsystem\s*\(')
        popen_pattern = re.compile(r'\bpopen\s*\(')
        fork_pattern = re.compile(r'\bfork\s*\(')

        active_system_calls = []
        documented_references = []

        for file_path in source_files:
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    lines = f.readlines()

                for line_num, line in enumerate(lines, 1):
                    line_stripped = line.strip()

                    # Check for actual system calls (not in comments)
                    if not line_stripped.startswith('*') and not line_stripped.startswith('//'):
                        if system_call_pattern.search(line_stripped):
                            active_system_calls.append({
                                'file': str(file_path.relative_to(self.base_path)),
                                'line': line_num,
                                'content': line_stripped,
                                'type': 'system()'
                            })

                        if popen_pattern.search(line_stripped):
                            active_system_calls.append({
                                'file': str(file_path.relative_to(self.base_path)),
                                'line': line_num,
                                'content': line_stripped,
                                'type': 'popen()'
                            })

                        if fork_pattern.search(line_stripped):
                            active_system_calls.append({
                                'file': str(file_path.relative_to(self.base_path)),
                                'line': line_num,
                                'content': line_stripped,
                                'type': 'fork()'
                            })

                    # Track documented references (acceptable)
                    if ('system(' in line_stripped and
                        ('*' in line_stripped or '//' in line_stripped or 'replaces' in line_stripped.lower())):
                        documented_references.append({
                            'file': str(file_path.relative_to(self.base_path)),
                            'line': line_num,
                            'content': line_stripped
                        })

            except (IOError, OSError) as e:
                results['findings'].append(f"⚠️  File access error {file_path}: {e}")
            except UnicodeDecodeError as e:
                results['findings'].append(f"⚠️  Encoding error {file_path}: {e}")
            except Exception as e:
                results['findings'].append(f"💥 Unexpected error {file_path}: {e}")

        # Evaluate results
        if active_system_calls:
            results['status'] = 'FAIL'
            results['findings'].append(f"Found {len(active_system_calls)} active unsafe system calls:")
            for call in active_system_calls:
                results['findings'].append(f"  {call['file']}:{call['line']} - {call['type']}")
        else:
            results['findings'].append("✅ No active system() calls found")

        results['summary'] = {
            'active_calls': len(active_system_calls),
            'documented_references': len(documented_references),
            'files_scanned': len(source_files)
        }

        return results

    def test_user_information_safety(self) -> Dict[str, any]:
        """Test that user information access includes proper null checking."""
        print("Testing user information safety...")

        results = {
            'test_name': 'User Information Safety',
            'status': 'PASS',
            'findings': [],
            'summary': {}
        }

        # Patterns that indicate user information access
        user_access_patterns = [
            (r'getuid\s*\(', 'getuid() call'),
            (r'getpwuid\s*\(', 'getpwuid() call'),
            (r'getpwnam\s*\(', 'getpwnam() call'),
            (r'getlogin\s*\(', 'getlogin() call'),
            (r'pw->pw_name', 'passwd struct access'),
            (r'pw->pw_dir', 'passwd struct access'),
        ]

        null_check_patterns = [
            r'if\s*\([^)]*!=\s*NULL\)',
            r'if\s*\([^)]*==\s*NULL\)',
            r'if\s*\(\s*NULL\s*!=',
            r'if\s*\(\s*NULL\s*==',
        ]

        source_files = self.find_source_files()
        user_access_locations = []
        proper_null_checks = 0

        for file_path in source_files:
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    lines = content.split('\n')

                for line_num, line in enumerate(lines, 1):
                    line_stripped = line.strip()

                    # Check for user information access
                    for pattern, description in user_access_patterns:
                        if re.search(pattern, line_stripped):
                            user_access_locations.append({
                                'file': str(file_path.relative_to(self.base_path)),
                                'line': line_num,
                                'content': line_stripped,
                                'access_type': description
                            })

                    # Check for null checking patterns
                    for null_pattern in null_check_patterns:
                        if re.search(null_pattern, line_stripped, re.IGNORECASE):
                            proper_null_checks += 1

            except (IOError, OSError) as e:
                results['findings'].append(f"⚠️  File access error {file_path}: {e}")
            except UnicodeDecodeError as e:
                results['findings'].append(f"⚠️  Encoding error {file_path}: {e}")
            except Exception as e:
                results['findings'].append(f"💥 Unexpected error {file_path}: {e}")

        # Evaluate results
        if user_access_locations:
            results['findings'].append(f"Found {len(user_access_locations)} user information access points:")
            for access in user_access_locations:
                results['findings'].append(f"  {access['file']}:{access['line']} - {access['access_type']}")

            results['findings'].append(f"Found {proper_null_checks} null checking patterns")

            if proper_null_checks >= len(user_access_locations):
                results['findings'].append("✅ Adequate null checking appears to be present")
            else:
                results['status'] = 'WARNING'
                results['findings'].append("⚠️  May need additional null checking validation")
        else:
            results['findings'].append("ℹ️  No direct user information access patterns found")

        results['summary'] = {
            'user_access_points': len(user_access_locations),
            'null_check_patterns': proper_null_checks,
            'files_scanned': len(source_files)
        }

        return results

    def test_memory_safety(self) -> Dict[str, any]:
        """Test for memory safety improvements and potential issues."""
        print("Testing memory safety...")

        results = {
            'test_name': 'Memory Safety',
            'status': 'PASS',
            'findings': [],
            'summary': {}
        }

        # Memory allocation patterns
        malloc_pattern = re.compile(r'\bmalloc\s*\(')
        calloc_pattern = re.compile(r'\bcalloc\s*\(')
        free_pattern = re.compile(r'\bfree\s*\(')

        # Safety checking patterns
        null_check_after_malloc = re.compile(r'if\s*\([^)]*==\s*NULL\)|if\s*\([^)]*!=\s*NULL\)')

        source_files = self.find_source_files()
        malloc_calls = []
        free_calls = []
        null_checks = []

        for file_path in source_files:
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    lines = f.readlines()

                for line_num, line in enumerate(lines, 1):
                    line_stripped = line.strip()

                    if malloc_pattern.search(line_stripped) or calloc_pattern.search(line_stripped):
                        malloc_calls.append({
                            'file': str(file_path.relative_to(self.base_path)),
                            'line': line_num,
                            'content': line_stripped
                        })

                    if free_pattern.search(line_stripped):
                        free_calls.append({
                            'file': str(file_path.relative_to(self.base_path)),
                            'line': line_num,
                            'content': line_stripped
                        })

                    if null_check_after_malloc.search(line_stripped):
                        null_checks.append({
                            'file': str(file_path.relative_to(self.base_path)),
                            'line': line_num,
                            'content': line_stripped
                        })

            except (IOError, OSError) as e:
                results['findings'].append(f"⚠️  File access error {file_path}: {e}")
            except UnicodeDecodeError as e:
                results['findings'].append(f"⚠️  Encoding error {file_path}: {e}")
            except Exception as e:
                results['findings'].append(f"💥 Unexpected error {file_path}: {e}")

        # Evaluate memory safety
        results['findings'].append(f"Found {len(malloc_calls)} memory allocation calls")
        results['findings'].append(f"Found {len(free_calls)} memory deallocation calls")
        results['findings'].append(f"Found {len(null_checks)} null check patterns")

        # Basic analysis
        if len(malloc_calls) > 0:
            null_check_ratio = len(null_checks) / len(malloc_calls)
            if null_check_ratio >= 0.8:  # 80% or more null checks
                results['findings'].append("✅ Good null checking coverage for memory allocations")
            else:
                results['status'] = 'WARNING'
                results['findings'].append(f"⚠️  Null checking ratio: {null_check_ratio:.1%} (consider improvement)")

        results['summary'] = {
            'malloc_calls': len(malloc_calls),
            'free_calls': len(free_calls),
            'null_checks': len(null_checks),
            'files_scanned': len(source_files)
        }

        return results

    def test_string_safety(self) -> Dict[str, any]:
        """Test for string operation safety improvements."""
        print("Testing string operation safety...")

        results = {
            'test_name': 'String Operation Safety',
            'status': 'PASS',
            'findings': [],
            'summary': {}
        }

        # Unsafe string operations
        unsafe_patterns = [
            (r'\bstrcpy\s*\(', 'strcpy() - no bounds checking'),
            (r'\bstrcat\s*\(', 'strcat() - no bounds checking'),
            (r'\bsprintf\s*\(', 'sprintf() - no bounds checking'),
            (r'\bgets\s*\(', 'gets() - extremely unsafe'),
        ]

        # Safer alternatives
        safe_patterns = [
            (r'\bstrncpy\s*\(', 'strncpy() - bounds checking'),
            (r'\bstrncat\s*\(', 'strncat() - bounds checking'),
            (r'\bsnprintf\s*\(', 'snprintf() - bounds checking'),
            (r'\bfgets\s*\(', 'fgets() - safer input'),
        ]

        source_files = self.find_source_files()
        unsafe_operations = []
        safe_operations = []

        for file_path in source_files:
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    lines = f.readlines()

                for line_num, line in enumerate(lines, 1):
                    line_stripped = line.strip()

                    # Check for unsafe patterns
                    for pattern, description in unsafe_patterns:
                        if re.search(pattern, line_stripped):
                            unsafe_operations.append({
                                'file': str(file_path.relative_to(self.base_path)),
                                'line': line_num,
                                'content': line_stripped,
                                'issue': description
                            })

                    # Check for safe patterns
                    for pattern, description in safe_patterns:
                        if re.search(pattern, line_stripped):
                            safe_operations.append({
                                'file': str(file_path.relative_to(self.base_path)),
                                'line': line_num,
                                'content': line_stripped,
                                'safety': description
                            })

            except (IOError, OSError) as e:
                results['findings'].append(f"⚠️  File access error {file_path}: {e}")
            except UnicodeDecodeError as e:
                results['findings'].append(f"⚠️  Encoding error {file_path}: {e}")
            except Exception as e:
                results['findings'].append(f"💥 Unexpected error {file_path}: {e}")

        # Evaluate string safety
        if unsafe_operations:
            results['status'] = 'WARNING'
            results['findings'].append(f"⚠️  Found {len(unsafe_operations)} unsafe string operations:")
            for op in unsafe_operations[:10]:  # Show first 10
                results['findings'].append(f"  {op['file']}:{op['line']} - {op['issue']}")
            if len(unsafe_operations) > 10:
                results['findings'].append(f"  ... and {len(unsafe_operations) - 10} more")
        else:
            results['findings'].append("✅ No unsafe string operations found")

        if safe_operations:
            results['findings'].append(f"✅ Found {len(safe_operations)} safe string operations")

        results['summary'] = {
            'unsafe_operations': len(unsafe_operations),
            'safe_operations': len(safe_operations),
            'files_scanned': len(source_files)
        }

        return results

    def run_comprehensive_validation(self, verbose: bool = False) -> Dict[str, any]:
        """Run all security validation tests."""
        print("Running comprehensive security validation...")
        if verbose:
            print("📊 Security validation will test 4 categories across all source files")

        all_results = {
            'validation_date': self._get_timestamp(),
            'overall_status': 'PASS',
            'test_results': {}
        }

        # Run all tests
        tests = [
            ('system_calls', self.test_system_call_elimination),
            ('user_safety', self.test_user_information_safety),
            ('memory_safety', self.test_memory_safety),
            ('string_safety', self.test_string_safety),
        ]

        failed_tests = []
        warning_tests = []

        for test_name, test_function in tests:
            try:
                result = test_function()
                all_results['test_results'][test_name] = result

                if result['status'] == 'FAIL':
                    failed_tests.append(test_name)
                elif result['status'] == 'WARNING':
                    warning_tests.append(test_name)

            except Exception as e:
                all_results['test_results'][test_name] = {
                    'test_name': test_name,
                    'status': 'ERROR',
                    'findings': [f"Test execution error: {e}"],
                    'summary': {}
                }
                failed_tests.append(test_name)

        # Determine overall status
        if failed_tests:
            all_results['overall_status'] = 'FAIL'
        elif warning_tests:
            all_results['overall_status'] = 'WARNING'

        all_results['summary'] = {
            'total_tests': len(tests),
            'passed_tests': len(tests) - len(failed_tests) - len(warning_tests),
            'warning_tests': len(warning_tests),
            'failed_tests': len(failed_tests)
        }

        return all_results

    def generate_report(self, results: Dict, output_format: str = 'markdown') -> str:
        """Generate a comprehensive security validation report."""

        if output_format == 'markdown':
            return self._generate_markdown_report(results)
        elif output_format == 'json':
            return json.dumps(results, indent=2)
        else:
            raise ValueError(f"Unsupported output format: {output_format}")

    def _generate_markdown_report(self, results: Dict) -> str:
        """Generate Markdown format security validation report."""

        overall_status = results.get('overall_status', 'UNKNOWN')
        status_emoji = {'PASS': '✅', 'WARNING': '⚠️', 'FAIL': '❌', 'ERROR': '💥'}.get(overall_status, '❓')

        report_lines = [
            "# Security Validation Report",
            "",
            f"**Generated**: {results.get('validation_date', self._get_timestamp())}",
            f"**Overall Status**: {status_emoji} {overall_status}",
            "",
            "## Executive Summary",
            ""
        ]

        summary = results.get('summary', {})
        if summary:
            report_lines.extend([
                f"- **Total Tests**: {summary.get('total_tests', 0)}",
                f"- **Passed Tests**: {summary.get('passed_tests', 0)}",
                f"- **Warning Tests**: {summary.get('warning_tests', 0)}",
                f"- **Failed Tests**: {summary.get('failed_tests', 0)}",
                ""
            ])

        # Generate detailed test results
        report_lines.extend([
            "## Test Results",
            ""
        ])

        test_results = results.get('test_results', {})
        for test_name, test_result in test_results.items():
            status_emoji = {
                'PASS': '✅',
                'WARNING': '⚠️',
                'FAIL': '❌',
                'ERROR': '💥'
            }.get(test_result.get('status', 'UNKNOWN'), '❓')

            report_lines.extend([
                f"### {test_result.get('test_name', test_name)} {status_emoji}",
                "",
                f"**Status**: {test_result.get('status', 'UNKNOWN')}",
                ""
            ])

            findings = test_result.get('findings', [])
            if findings:
                report_lines.append("**Findings**:")
                for finding in findings:
                    report_lines.append(f"- {finding}")
                report_lines.append("")

            test_summary = test_result.get('summary', {})
            if test_summary:
                report_lines.append("**Statistics**:")
                for key, value in test_summary.items():
                    report_lines.append(f"- {key.replace('_', ' ').title()}: {value}")
                report_lines.append("")

        return "\n".join(report_lines)

    def _get_timestamp(self) -> str:
        """Get current timestamp for report generation."""
        from datetime import datetime
        return datetime.now().strftime("%Y-%m-%d %H:%M:%S")

def main():
    """Main entry point for the security validation script."""

    parser = argparse.ArgumentParser(
        description="Validate security improvements in C source code",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    # Run all security validation tests
    python3 security_validation.py --all

    # Test only system call elimination
    python3 security_validation.py --test-system-calls

    # Run comprehensive validation and save report
    python3 security_validation.py --all --output security_report.md --verbose
        """
    )

    parser.add_argument(
        '--test-system-calls',
        action='store_true',
        help='Test that unsafe system() calls have been eliminated'
    )

    parser.add_argument(
        '--test-user-safety',
        action='store_true',
        help='Test user information access safety'
    )

    parser.add_argument(
        '--test-memory-safety',
        action='store_true',
        help='Test memory safety improvements'
    )

    parser.add_argument(
        '--test-string-safety',
        action='store_true',
        help='Test string operation safety'
    )

    parser.add_argument(
        '--all',
        action='store_true',
        help='Run all security validation tests'
    )

    parser.add_argument(
        '--format',
        choices=['markdown', 'json'],
        default='markdown',
        help='Output format (default: markdown)'
    )

    parser.add_argument(
        '--output', '-o',
        type=Path,
        help='Output file (default: stdout)'
    )

    parser.add_argument(
        '--base-path',
        type=Path,
        default=Path('.'),
        help='Base directory to scan (default: current directory)'
    )

    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Enable verbose output'
    )

    args = parser.parse_args()

    # Initialize validator
    validator = SecurityValidator(args.base_path)

    # Determine which tests to run
    if args.all:
        results = validator.run_comprehensive_validation(verbose=args.verbose)
    else:
        # Run individual tests
        results = {
            'validation_date': validator._get_timestamp(),
            'overall_status': 'PASS',
            'test_results': {}
        }

        if args.test_system_calls:
            results['test_results']['system_calls'] = validator.test_system_call_elimination()

        if args.test_user_safety:
            results['test_results']['user_safety'] = validator.test_user_information_safety()

        if args.test_memory_safety:
            results['test_results']['memory_safety'] = validator.test_memory_safety()

        if args.test_string_safety:
            results['test_results']['string_safety'] = validator.test_string_safety()

        # If no specific tests requested, run all
        if not any([args.test_system_calls, args.test_user_safety,
                   args.test_memory_safety, args.test_string_safety]):
            results = validator.run_comprehensive_validation(verbose=args.verbose)

    # Generate report
    report = validator.generate_report(results, args.format)

    # Output report
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        with open(args.output, 'w', encoding='utf-8') as f:
            f.write(report)

        if args.verbose:
            print(f"Security validation report written to: {args.output}", file=sys.stderr)
    else:
        print(report)

    # Exit with appropriate code based on overall status
    overall_status = results.get('overall_status', 'UNKNOWN')
    if overall_status == 'FAIL':
        sys.exit(2)  # Critical security issues
    elif overall_status == 'WARNING':
        sys.exit(1)  # Security warnings
    else:
        sys.exit(0)  # All tests passed

if __name__ == '__main__':
    main()
