#!/usr/bin/env -S uv run --script
# /// script
# dependencies = []
# ///

"""
Legacy Pattern Detection Script for Conquer 4.x Modernization

This script automatically detects remaining legacy patterns in the codebase
and generates comprehensive reports for modernization planning.

Usage:
    python3 detect_legacy_patterns.py [options]
    python3 detect_legacy_patterns.py --category string --report-file legacy_strings.md
    python3 detect_legacy_patterns.py --all --output-dir reports/

Categories:
    - platform: Platform-specific code (BSD/SYSV)
    - system: Unsafe system calls (system(), popen(), fork())
    - string: Unsafe string operations (strcpy, strcat, sprintf)
    - memory: Memory management patterns
    - functions: Function declaration patterns
    - types: Legacy type usage
"""

import re
import sys
import argparse
from pathlib import Path
from typing import Dict, List, Tuple, Optional
import json

class LegacyPatternDetector:
    """Detects and categorizes legacy patterns in C source code."""

    def __init__(self, base_path: Path = Path(".")):
        self.base_path = base_path
        self.patterns = {
            'platform': [
                (r'#ifdef\s+(BSD|SYSV)', 'Platform-specific conditional compilation'),
                (r'#ifndef\s+(BSD|SYSV)', 'Platform-specific conditional compilation'),
                (r'#if\s+defined\s*\(\s*(BSD|SYSV)', 'Platform-specific conditional compilation'),
            ],
            'system': [
                (r'\bsystem\s*\(', 'Unsafe system() call'),
                (r'\bpopen\s*\(', 'Potentially unsafe popen() call'),
                (r'\bfork\s*\(', 'Platform-specific fork() call'),
                (r'\bexec[lv]p?\s*\(', 'Direct exec family calls'),
            ],
            'string': [
                (r'\bstrcpy\s*\(', 'Unsafe strcpy() - no bounds checking'),
                (r'\bstrcat\s*\(', 'Unsafe strcat() - no bounds checking'),
                (r'\bsprintf\s*\(', 'Unsafe sprintf() - no bounds checking'),
                (r'\bgets\s*\(', 'Extremely unsafe gets() function'),
            ],
            'memory': [
                (r'\bmalloc\s*\([^)]*\)', 'Dynamic memory allocation'),
                (r'\bcalloc\s*\([^)]*\)', 'Dynamic memory allocation'),
                (r'\brealloc\s*\([^)]*\)', 'Dynamic memory reallocation'),
                (r'\bfree\s*\(', 'Memory deallocation'),
            ],
            'functions': [
                (r'^[a-zA-Z_][a-zA-Z0-9_]*\s*\([^)]*\)\s*$', 'Function declaration'),
                (r'^\s*register\s+', 'Legacy register keyword'),
                (r'^\s*auto\s+', 'Explicit auto keyword'),
            ],
            'types': [
                (r'\bPARM_[A-Z0-9_]+', 'Legacy parameter macro'),
                (r'\bUCHAR\b', 'Legacy UCHAR type'),
                (r'\bSHORT\b', 'Legacy SHORT type'),
                (r'\bUSHORT\b', 'Legacy USHORT type'),
            ]
        }

        self.source_extensions = {'.c', '.h'}
        self.exclude_dirs = {'historical', 'build', '.git', '__pycache__'}

    def _is_in_comment(self, line: str) -> bool:
        """Check if a line is within a comment (C-style /* */ or // comments)."""
        line_stripped = line.strip()

        # Check for obvious comment indicators
        if (line_stripped.startswith('//') or
            line_stripped.startswith('*') or
            line_stripped.startswith('/*')):
            return True

        # Check for inline comments that contain the pattern
        # Look for /* comment */ or // comment patterns
        if '//' in line_stripped:
            # Find position of // and see if pattern is after it
            comment_pos = line_stripped.find('//')
            # This is a more sophisticated check we'll implement
            # For now, if line contains // assume any system() after it is in comment
            pass

        if '/*' in line_stripped and '*/' in line_stripped:
            # Single line /* comment */ - need to check if pattern is inside
            pass

        # For now, use simple heuristics
        # Check for common comment patterns that mention legacy operations
        comment_indicators = [
            'replaces strcpy(',
            'instead of strcpy(',
            'use strcpy(',
            'strcpy() is',
            'strcpy() call',
            'strcpy() usage',
            'strcpy() example',
            '// strcpy(',
            '/* strcpy(',
            '* strcpy(',
            'eliminates strcpy(',
            'avoids strcpy(',
            'removed strcpy(',
            'replaces sprintf(',
            'instead of sprintf(',
            'use sprintf(',
            'sprintf() is',
            'sprintf() call',
            'sprintf() usage',
            'sprintf() example',
            '// sprintf(',
            '/* sprintf(',
            '* sprintf(',
            'eliminates sprintf(',
            'avoids sprintf(',
            'removed sprintf(',
            'replaces strcat(',
            'instead of strcat(',
            'use strcat(',
            'strcat() is',
            'strcat() call',
            'strcat() usage',
            'strcat() example',
            '// strcat(',
            '/* strcat(',
            '* strcat(',
            'eliminates strcat(',
            'avoids strcat(',
            'removed strcat(',
            'replaces system(',
            'instead of system(',
            'use system(',
            'system() is',
            'system() call',
            'system() usage',
            'system() example',
            '// system(',
            '/* system(',
            '* system(',
            'eliminates system(',
            'avoids system(',
            'removed system(',
            'before (unsafe)',
            'after (safe)',
            'unsafe)',
            'safe)',
        ]

        line_lower = line_stripped.lower()
        for indicator in comment_indicators:
            if indicator in line_lower:
                return True

        return False

    def find_source_files(self) -> List[Path]:
        """Find all C source and header files, excluding historical directories."""
        source_files = []

        for file_path in self.base_path.rglob('*'):
            if (file_path.is_file() and
                file_path.suffix in self.source_extensions and
                not any(exclude in file_path.parts for exclude in self.exclude_dirs)):
                source_files.append(file_path)

        return sorted(source_files)

    def detect_patterns_in_file(self, file_path: Path, categories: List[str] = None) -> Dict[str, List[Tuple[int, str, str]]]:
        """
        Detect legacy patterns in a single file.

        Returns:
            Dict mapping category -> [(line_number, line_content, description), ...]
        """
        if categories is None:
            categories = list(self.patterns.keys())

        results = {category: [] for category in categories}

        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except Exception as e:
            print(f"Warning: Could not read {file_path}: {e}", file=sys.stderr)
            return results

        for category in categories:
            if category not in self.patterns:
                continue

            for pattern, description in self.patterns[category]:
                regex = re.compile(pattern, re.MULTILINE)

                for line_num, line in enumerate(lines, 1):
                    line_stripped = line.strip()
                    # Skip if in comment
                    if not self._is_in_comment(line_stripped):
                        if regex.search(line_stripped):
                            results[category].append((line_num, line_stripped, description))

        return results

    def scan_codebase(self, categories: List[str] = None) -> Dict[str, Dict[str, List[Tuple[int, str, str]]]]:
        """
        Scan entire codebase for legacy patterns.

        Returns:
            Dict mapping file_path -> category -> [(line_number, line_content, description), ...]
        """
        if categories is None:
            categories = list(self.patterns.keys())

        source_files = self.find_source_files()
        results = {}

        print(f"Scanning {len(source_files)} source files for legacy patterns...")

        for file_path in source_files:
            file_results = self.detect_patterns_in_file(file_path, categories)

            # Only include files that have matches
            if any(matches for matches in file_results.values()):
                rel_path = str(file_path.relative_to(self.base_path))
                results[rel_path] = file_results

        return results

    def generate_report(self, results: Dict, output_format: str = 'markdown', summary_only: bool = False, baseline: Optional[Path] = None) -> str:
        """Generate a comprehensive report of legacy patterns found."""

        if output_format == 'markdown':
            return self._generate_markdown_report(results, summary_only=summary_only, baseline=baseline)
        elif output_format == 'json':
            return json.dumps(results, indent=2)
        else:
            raise ValueError(f"Unsupported output format: {output_format}")

    def _generate_markdown_report(self, results: Dict, summary_only: bool = False, baseline: Optional[Path] = None) -> str:
        """Generate management-friendly Markdown format report."""

        # Calculate statistics
        total_files_scanned = len(self.find_source_files())
        files_with_patterns = len(results)

        category_stats = {}
        total_patterns = 0

        for file_path, file_results in results.items():
            for category, matches in file_results.items():
                if matches:
                    if category not in category_stats:
                        category_stats[category] = {'files': set(), 'total_matches': 0}
                    category_stats[category]['files'].add(file_path)
                    category_stats[category]['total_matches'] += len(matches)
                    total_patterns += len(matches)

        # Define priority levels
        priority_mapping = {
            'string': ('🔴 HIGH', 'Security risk - buffer overflows'),
            'system': ('🔴 HIGH', 'Security risk - command injection'),
            'memory': ('🟡 MEDIUM', 'Memory management modernization'),
            'functions': ('🟡 MEDIUM', 'Code quality and maintainability'),
            'platform': ('🟢 LOW', 'Cross-platform compatibility'),
            'types': ('🟢 LOW', 'Type system modernization')
        }

        report_lines = [
            "# 📊 Legacy Pattern Detection Report",
            "",
            f"**Generated**: {self._get_timestamp()}",
            "",
            "## 🎯 Executive Summary",
            "",
            f"| Metric | Value |",
            f"|--------|-------|",
            f"| **Total Files Scanned** | {total_files_scanned} |",
            f"| **Files Requiring Modernization** | {files_with_patterns} ({files_with_patterns/total_files_scanned*100:.1f}%) |",
            f"| **Total Legacy Patterns Found** | {total_patterns} |",
            f"| **Categories with Issues** | {len(category_stats)} |",
            "",
            "## ⚡ Priority Dashboard",
            "",
            "| Priority | Category | Files | Instances | Description |",
            "|----------|----------|-------|-----------|-------------|"
        ]

        # Sort categories by priority (high to low)
        priority_order = ['string', 'system', 'memory', 'functions', 'platform', 'types']
        for category in priority_order:
            if category in category_stats:
                stats = category_stats[category]
                priority, description = priority_mapping.get(category, ('🟢 LOW', 'General modernization'))
                report_lines.append(
                    f"| {priority} | **{category.title()}** | {len(stats['files'])} | {stats['total_matches']} | {description} |"
                )

        report_lines.extend([
            "",
            "## 📈 Modernization Effort Estimate",
            "",
        ])

        # Calculate effort estimates
        effort_estimates = {
            'string': 2,    # 2 hours per file (security critical)
            'system': 1,    # 1 hour per file
            'memory': 1.5,  # 1.5 hours per file
            'functions': 1, # 1 hour per file
            'platform': 0.5, # 30 minutes per file
            'types': 0.5    # 30 minutes per file
        }

        total_effort = 0
        for category, stats in category_stats.items():
            file_count = len(stats['files'])
            effort_per_file = effort_estimates.get(category, 1)
            category_effort = file_count * effort_per_file
            total_effort += category_effort

            priority, _ = priority_mapping.get(category, ('🟢 LOW', ''))
            report_lines.append(
                f"- **{category.title()}**: {file_count} files × {effort_per_file}h = {category_effort:.1f}h {priority}"
            )

        report_lines.extend([
            "",
            f"**Total Estimated Effort**: {total_effort:.1f} hours ({total_effort/8:.1f} days)",
            ""
        ])

        # Add baseline comparison if provided
        if baseline and baseline.exists():
            baseline_comparison = self._compare_with_baseline(results, baseline)
            if baseline_comparison:
                report_lines.extend([
                    "## 📊 Progress Since Baseline",
                    "",
                    baseline_comparison,
                    ""
                ])

        # If summary-only mode, stop here
        if summary_only:
            report_lines.extend([
                "---",
                "",
                f"*Management Summary Report generated by Legacy Pattern Detection Script v2.0*",
                f"*For detailed technical analysis, run without --summary-only flag*"
            ])
            return "\n".join(report_lines)

        report_lines.extend([
            "## 📁 Files by Priority",
            ""
        ])

        # Group files by highest priority issue
        file_priorities = {}
        for file_path, file_results in results.items():
            highest_priority = 'types'  # Default to lowest priority
            for category in priority_order:
                if category in file_results and file_results[category]:
                    highest_priority = category
                    break

            if highest_priority not in file_priorities:
                file_priorities[highest_priority] = []
            file_priorities[highest_priority].append(file_path)

        for category in priority_order:
            if category in file_priorities:
                priority, description = priority_mapping.get(category, ('🟢 LOW', ''))
                files = sorted(file_priorities[category])
                report_lines.extend([
                    f"### {priority} {category.title()} ({len(files)} files)",
                    f"*{description}*",
                    ""
                ])

                for file_path in files:
                    file_patterns = sum(len(matches) for matches in results[file_path].values() if matches)
                    report_lines.append(f"- `{file_path}` ({file_patterns} patterns)")

                report_lines.append("")

        # Generate detailed findings (condensed format)
        report_lines.extend([
            "## 🔍 Detailed Pattern Analysis",
            "",
            "*Note: This section provides technical details for developers. Management can focus on the summary above.*",
            ""
        ])

        for file_path in sorted(results.keys()):
            file_results = results[file_path]
            total_file_patterns = sum(len(matches) for matches in file_results.values() if matches)

            report_lines.extend([
                f"### 📄 `{file_path}` ({total_file_patterns} patterns)",
                ""
            ])

            for category in priority_order:
                if category in file_results and file_results[category]:
                    matches = file_results[category]
                    priority, _ = priority_mapping.get(category, ('🟢 LOW', ''))

                    report_lines.extend([
                        f"#### {priority} {category.title()} ({len(matches)} instances)",
                        ""
                    ])

                    # Group similar patterns to reduce noise
                    pattern_groups = {}
                    for line_num, line_content, description in matches:
                        if description not in pattern_groups:
                            pattern_groups[description] = []
                        pattern_groups[description].append((line_num, line_content))

                    for description, occurrences in pattern_groups.items():
                        if len(occurrences) == 1:
                            line_num, line_content = occurrences[0]
                            report_lines.extend([
                                f"**Line {line_num}**: {description}",
                                f"```c",
                                f"{line_content}",
                                f"```",
                                ""
                            ])
                        else:
                            lines = ", ".join(str(line_num) for line_num, _ in occurrences)
                            report_lines.extend([
                                f"**Lines {lines}**: {description}",
                                f"```c",
                                f"// {len(occurrences)} occurrences found",
                                f"// Example: {occurrences[0][1]}",
                                f"```",
                                ""
                            ])

            report_lines.append("")

        # Add footer with next steps
        report_lines.extend([
            "---",
            "",
            "## 🚀 Recommended Next Steps",
            "",
            "1. **Phase 8.1**: Address HIGH priority security issues (string/system patterns)",
            "2. **Phase 8.2**: Modernize memory management patterns",
            "3. **Phase 8.3**: Update function declarations and improve code quality",
            "4. **Phase 8.4**: Complete remaining low-priority modernizations",
            "",
            f"*Report generated by Legacy Pattern Detection Script v2.0*"
        ])

        return "\n".join(report_lines)

    def _get_timestamp(self) -> str:
        """Get current timestamp for report generation."""
        from datetime import datetime
        return datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    def _compare_with_baseline(self, current_results: Dict, baseline_path: Path) -> str:
        """Compare current results with baseline report for progress tracking."""
        try:
            with open(baseline_path, 'r', encoding='utf-8') as f:
                baseline_content = f.read()

            # Extract baseline statistics from previous report
            import re

            # Extract key metrics from baseline report
            total_patterns_match = re.search(r'\| \*\*Total Legacy Patterns Found\*\* \| (\d+)', baseline_content)
            files_match = re.search(r'\| \*\*Files Requiring Modernization\*\* \| (\d+)', baseline_content)

            if not total_patterns_match or not files_match:
                return "*Baseline comparison unavailable - incompatible format*"

            baseline_patterns = int(total_patterns_match.group(1))
            baseline_files = int(files_match.group(1))

            # Calculate current statistics
            current_patterns = sum(
                len(matches) for file_results in current_results.values()
                for matches in file_results.values() if matches
            )
            current_files = len(current_results)

            # Calculate progress
            patterns_fixed = baseline_patterns - current_patterns
            files_fixed = baseline_files - current_files

            progress_lines = [
                f"| Metric | Baseline | Current | Progress |",
                f"|--------|----------|---------|----------|",
                f"| **Legacy Patterns** | {baseline_patterns} | {current_patterns} | {patterns_fixed:+d} ({patterns_fixed/baseline_patterns*100:+.1f}%) |",
                f"| **Files with Issues** | {baseline_files} | {current_files} | {files_fixed:+d} ({files_fixed/baseline_files*100:+.1f}%) |"
            ]

            if patterns_fixed > 0:
                progress_lines.append("")
                progress_lines.append("✅ **Progress detected** - legacy patterns have been successfully modernized!")
            elif patterns_fixed == 0:
                progress_lines.append("")
                progress_lines.append("⏸️ **No change** - no new modernization work completed since baseline.")
            else:
                progress_lines.append("")
                progress_lines.append("⚠️ **Regression detected** - new legacy patterns have been introduced.")

            return "\n".join(progress_lines)

        except Exception as e:
            return f"*Baseline comparison failed: {e}*"

def main():
    """Main entry point for the legacy pattern detection script."""

    parser = argparse.ArgumentParser(
        description="Detect legacy patterns in C source code for modernization planning",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    # Scan all categories and output to stdout
    python3 detect_legacy_patterns.py

    # Scan only string operations and save to file
    python3 detect_legacy_patterns.py --category string --output legacy_strings.md

    # Scan multiple categories
    python3 detect_legacy_patterns.py --category string --category memory

    # Generate JSON output
    python3 detect_legacy_patterns.py --format json --output results.json
        """
    )

    parser.add_argument(
        '--category',
        action='append',
        choices=['platform', 'system', 'string', 'memory', 'functions', 'types'],
        help='Pattern category to scan (can be specified multiple times)'
    )

    parser.add_argument(
        '--all',
        action='store_true',
        help='Scan all pattern categories'
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

    parser.add_argument(
        '--summary-only',
        action='store_true',
        help='Generate only executive summary (management report)'
    )

    parser.add_argument(
        '--baseline',
        type=Path,
        help='Compare against previous baseline report for progress tracking'
    )

    args = parser.parse_args()

    # Determine which categories to scan
    if args.all:
        categories = None  # Scan all categories
    elif args.category:
        categories = args.category
    else:
        categories = None  # Default to all categories

    # Initialize detector
    detector = LegacyPatternDetector(args.base_path)

    # Perform scan
    if args.verbose:
        print(f"Scanning for legacy patterns in: {args.base_path}", file=sys.stderr)
        if categories:
            print(f"Categories: {', '.join(categories)}", file=sys.stderr)
        else:
            print("Categories: all", file=sys.stderr)

    results = detector.scan_codebase(categories)

    # Generate report
    report = detector.generate_report(results, args.format, summary_only=args.summary_only, baseline=args.baseline)

    # Output report
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        with open(args.output, 'w', encoding='utf-8') as f:
            f.write(report)

        if args.verbose:
            print(f"Report written to: {args.output}", file=sys.stderr)
    else:
        print(report)

    # Exit with appropriate code
    if results:
        if args.verbose:
            print(f"Found legacy patterns in {len(results)} files", file=sys.stderr)
        sys.exit(1)  # Indicate patterns found
    else:
        if args.verbose:
            print("No legacy patterns found", file=sys.stderr)
        sys.exit(0)

if __name__ == '__main__':
    main()