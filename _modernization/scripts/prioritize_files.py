#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["pathlib", "argparse", "subprocess"]
# ///

"""
prioritize_files.py - Sort files by dependency and error count for Phase 4 workflow

This script analyzes file dependencies and compilation health to determine
optimal processing order for Phase 4 systematic warning elimination.

Key features:
1. Dependency analysis - headers before source files
2. Compilation health assessment
3. File complexity classification (Small/Medium/Large/Complex)
4. Session planning recommendations

Usage:
    python3 prioritize_files.py --analyze
    python3 prioritize_files.py --output priorities.md
"""

import argparse
import pathlib
import re
import subprocess
import sys
from collections import defaultdict, deque
from datetime import datetime

class FilePrioritizer:
    def __init__(self):
        self.dependencies = defaultdict(set)  # file -> set of files it includes
        self.reverse_deps = defaultdict(set)  # file -> set of files that include it
        self.file_stats = {}
        self.c_files = []
        self.h_files = []

    def discover_files(self):
        """Find all C and header files in current directory"""
        self.c_files = sorted(list(pathlib.Path('.').glob('*.c')))
        self.h_files = sorted(list(pathlib.Path('.').glob('*.h')))
        print(f"Found {len(self.c_files)} C files and {len(self.h_files)} header files")

    def analyze_dependencies(self):
        """Analyze #include dependencies between files"""
        all_files = self.c_files + self.h_files

        for file_path in all_files:
            try:
                content = file_path.read_text(encoding='utf-8', errors='ignore')
                includes = re.findall(r'#include\s*["\<]([^">]+)[">]', content)

                for include in includes:
                    # Focus on local includes (quoted includes)
                    if include.endswith('.h'):
                        # Check if this header exists locally
                        local_header = pathlib.Path(include)
                        if local_header.exists():
                            self.dependencies[str(file_path)].add(include)
                            self.reverse_deps[include].add(str(file_path))

            except Exception as e:
                print(f"Warning: Could not analyze {file_path}: {e}")

    def classify_file_complexity(self, file_path):
        """Classify file as Small/Medium/Large/Complex based on content analysis"""
        try:
            content = file_path.read_text(encoding='utf-8', errors='ignore')
            lines = len(content.split('\n'))

            # Count functions (approximate)
            function_count = len(re.findall(r'^\s*\w+\s+\w+\s*\([^)]*\)\s*\{', content, re.MULTILINE))

            # Adjust for K&R style functions
            kr_functions = len(re.findall(r'^\w+\s*\([^)]*\)\s*$', content, re.MULTILINE))
            function_count += kr_functions

            # Classification logic from Phase 4 Strategy
            if function_count <= 5 and lines <= 200:
                return "SMALL", function_count, lines
            elif function_count <= 10 and lines <= 400:
                return "MEDIUM", function_count, lines
            elif function_count <= 20 and lines <= 800:
                return "LARGE", function_count, lines
            else:
                return "COMPLEX", function_count, lines

        except Exception as e:
            print(f"Warning: Could not classify {file_path}: {e}")
            return "UNKNOWN", 0, 0

    def test_compilation_health(self, file_path):
        """Test basic compilation health of a file"""
        # Basic C99 compilation test (Subphase 1 equivalent)
        base_flags = [
            'gcc', '-O2', '-g', '-std=c99',
            '-D_POSIX_C_SOURCE=200809L', '-D_XOPEN_SOURCE=700', '-D_DEFAULT_SOURCE',
            '-DDEFAULTDIR="/home/ssmoogen/conquer/lib"',
            '-DEXEDIR="/home/ssmoogen/conquer/bin"',
            '-DVERSION="4"', '-DPATCHLEVEL="12"', '-DLOGIN="ssmoogen"',
            '-DCONQUER'
        ]

        try:
            if str(file_path).endswith('.h'):
                # For headers, create a simple test
                test_content = f'#include "{file_path.name}"\nint main(){{return 0;}}'
                test_file = pathlib.Path('temp_header_test.c')
                test_file.write_text(test_content)

                result = subprocess.run(
                    base_flags + ['-c', 'temp_header_test.c', '-o', '/tmp/foo.o'],
                    capture_output=True, text=True, timeout=30
                )

                test_file.unlink(missing_ok=True)
            else:
                # For C files, compile directly
                result = subprocess.run(
                    base_flags + ['-c', str(file_path), '-o', '/tmp/foo.o'],
                    capture_output=True, text=True, timeout=30
                )

            # Count errors and warnings
            stderr = result.stderr
            error_count = len(re.findall(r': error:', stderr))
            warning_count = len(re.findall(r': warning:', stderr))

            return {
                'compiles': result.returncode == 0,
                'errors': error_count,
                'warnings': warning_count,
                'stderr': stderr
            }

        except subprocess.TimeoutExpired:
            return {'compiles': False, 'errors': 999, 'warnings': 0, 'stderr': 'Timeout'}
        except Exception as e:
            return {'compiles': False, 'errors': 999, 'warnings': 0, 'stderr': str(e)}

    def calculate_dependency_depth(self, filename):
        """Calculate how many files depend on this file (directly or indirectly)"""
        visited = set()
        to_visit = deque([filename])
        depth = 0

        while to_visit:
            current = to_visit.popleft()
            if current in visited:
                continue

            visited.add(current)
            depth += 1

            # Add files that depend on current file
            for dependent in self.reverse_deps.get(current, []):
                if dependent not in visited:
                    to_visit.append(dependent)

        return depth - 1  # Don't count the file itself

    def analyze_all_files(self):
        """Perform comprehensive analysis of all files"""
        self.discover_files()
        self.analyze_dependencies()

        all_files = self.c_files + self.h_files

        for file_path in all_files:
            filename = str(file_path)
            complexity, func_count, line_count = self.classify_file_complexity(file_path)
            health = self.test_compilation_health(file_path)
            dep_depth = self.calculate_dependency_depth(filename)

            self.file_stats[filename] = {
                'path': file_path,
                'type': 'header' if filename.endswith('.h') else 'source',
                'complexity': complexity,
                'function_count': func_count,
                'line_count': line_count,
                'compiles': health['compiles'],
                'errors': health['errors'],
                'warnings': health['warnings'],
                'dependency_depth': dep_depth,
                'depends_on': list(self.dependencies[filename]),
                'dependents': list(self.reverse_deps[filename])
            }

    def generate_priority_list(self):
        """Generate prioritized file processing order"""
        files = list(self.file_stats.keys())

        # Priority calculation:
        # 1. Headers before source files (type weight)
        # 2. Files with more dependents first (dependency depth)
        # 3. Files that compile without errors first
        # 4. Simpler files first (when all else equal)

        def priority_key(filename):
            stats = self.file_stats[filename]

            # Type priority: headers first
            type_priority = 0 if stats['type'] == 'header' else 1

            # Dependency priority: more dependents = higher priority
            dep_priority = -stats['dependency_depth']

            # Compilation health: working files first
            health_priority = 0 if stats['compiles'] else 1
            error_priority = stats['errors']

            # Complexity: simpler first
            complexity_map = {'SMALL': 0, 'MEDIUM': 1, 'LARGE': 2, 'COMPLEX': 3, 'UNKNOWN': 4}
            complexity_priority = complexity_map.get(stats['complexity'], 4)

            return (type_priority, health_priority, error_priority, dep_priority, complexity_priority, filename)

        return sorted(files, key=priority_key)

    def generate_report(self, output_file=None):
        """Generate comprehensive priority and analysis report"""
        if not output_file:
            output_file = f'_modernization/claude/reports/file_priorities_{datetime.now().strftime("%Y%m%d_%H%M%S")}.md'

        priority_list = self.generate_priority_list()

        report = []
        report.append('# File Priority Analysis for Phase 4')
        report.append(f'**Generated**: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}')
        report.append('')

        # Summary statistics
        total_files = len(self.file_stats)
        compilable_files = sum(1 for stats in self.file_stats.values() if stats['compiles'])
        total_errors = sum(stats['errors'] for stats in self.file_stats.values())
        total_warnings = sum(stats['warnings'] for stats in self.file_stats.values())

        report.append('## Summary Statistics')
        report.append(f'- **Total Files**: {total_files}')
        report.append(f'- **Compilable Files**: {compilable_files}/{total_files}')
        report.append(f'- **Total Compilation Errors**: {total_errors}')
        report.append(f'- **Total Warnings**: {total_warnings}')
        report.append('')

        # Recommended processing order
        report.append('## Recommended Processing Order')
        report.append('')
        report.append('### Critical Headers (Process First)')

        header_files = [f for f in priority_list if self.file_stats[f]['type'] == 'header']
        for i, filename in enumerate(header_files[:5], 1):
            stats = self.file_stats[filename]
            status = "✅" if stats['compiles'] else "🔴"
            report.append(f'{i}. **{filename}** {status} ({stats["dependency_depth"]} dependents, {stats["errors"]} errors)')

        report.append('')
        report.append('### Source Files by Priority')

        source_files = [f for f in priority_list if self.file_stats[f]['type'] == 'source']
        for i, filename in enumerate(source_files, 1):
            stats = self.file_stats[filename]
            status = "✅" if stats['compiles'] else "🔴"
            complexity_emoji = {'SMALL': '🟢', 'MEDIUM': '🟡', 'LARGE': '🟠', 'COMPLEX': '🔴'}.get(stats['complexity'], '⚪')
            report.append(f'{i}. **{filename}** {status} {complexity_emoji} {stats["complexity"]} ({stats["function_count"]} funcs, {stats["errors"]} errors, {stats["warnings"]} warnings)')

        # Detailed file analysis
        report.append('')
        report.append('## Detailed File Analysis')

        for filename in priority_list:
            stats = self.file_stats[filename]
            report.append(f'### {filename}')
            report.append(f'- **Type**: {stats["type"].title()}')
            report.append(f'- **Complexity**: {stats["complexity"]} ({stats["function_count"]} functions, {stats["line_count"]} lines)')
            report.append(f'- **Compilation**: {"✅ Success" if stats["compiles"] else "🔴 Failed"} ({stats["errors"]} errors, {stats["warnings"]} warnings)')
            report.append(f'- **Dependencies**: {stats["dependency_depth"]} files depend on this')

            if stats['depends_on']:
                report.append(f'- **Includes**: {", ".join(stats["depends_on"])}')

            if stats['dependents']:
                dependents = stats['dependents'][:5]  # Show first 5
                dep_list = ", ".join(dependents)
                if len(stats['dependents']) > 5:
                    dep_list += f" (and {len(stats['dependents']) - 5} more)"
                report.append(f'- **Used by**: {dep_list}')

            report.append('')

        # Session planning recommendations
        report.append('## Session Planning Recommendations')
        report.append('')

        blocked_files = [f for f, stats in self.file_stats.items() if not stats['compiles']]
        if blocked_files:
            report.append('### 🔴 Blocked Files (Fix First)')
            for filename in blocked_files:
                stats = self.file_stats[filename]
                report.append(f'- **{filename}**: {stats["errors"]} compilation errors')
            report.append('')

        # Group by complexity for session planning
        by_complexity = defaultdict(list)
        for filename, stats in self.file_stats.items():
            if stats['compiles']:  # Only include compilable files
                by_complexity[stats['complexity']].append(filename)

        complexity_order = ['SMALL', 'MEDIUM', 'LARGE', 'COMPLEX']
        for complexity in complexity_order:
            files = by_complexity[complexity]
            if files:
                emoji = {'SMALL': '🟢', 'MEDIUM': '🟡', 'LARGE': '🟠', 'COMPLEX': '🔴'}[complexity]
                report.append(f'### {emoji} {complexity} Files')

                if complexity == 'SMALL':
                    report.append('**Session Strategy**: Can use accelerated subphase progression, may combine 2-3 warning flags')
                elif complexity == 'MEDIUM':
                    report.append('**Session Strategy**: Standard progression, single flag addition per session')
                elif complexity == 'LARGE':
                    report.append('**Session Strategy**: Function-by-function approach, multiple sessions required')
                else:  # COMPLEX
                    report.append('**Session Strategy**: Maximum caution, 3-5 function chunks per session, automation assistance')

                for filename in files:
                    stats = self.file_stats[filename]
                    report.append(f'- **{filename}**: {stats["function_count"]} functions, {stats["warnings"]} warnings')
                report.append('')

        # Write report
        output_path = pathlib.Path(output_file)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text('\n'.join(report))

        return str(output_path)

def main():
    parser = argparse.ArgumentParser(description='Prioritize files for Phase 4 systematic warning elimination')
    parser.add_argument('--analyze', action='store_true', help='Perform analysis of current directory')
    parser.add_argument('--output', help='Output file for priority report')
    parser.add_argument('--dry-run', action='store_true', help='Show analysis without writing files')

    args = parser.parse_args()

    if not args.analyze:
        print("Use --analyze to analyze files in current directory")
        sys.exit(1)

    prioritizer = FilePrioritizer()

    print("Analyzing file dependencies and compilation health...")
    prioritizer.analyze_all_files()

    if not args.dry_run:
        report_file = prioritizer.generate_report(args.output)
        print(f"Generated priority analysis: {report_file}")
    else:
        print("Dry run - analysis complete, report generation skipped")

    # Print quick summary
    priority_list = prioritizer.generate_priority_list()
    print(f"\nTop 5 priority files:")
    for i, filename in enumerate(priority_list[:5], 1):
        stats = prioritizer.file_stats[filename]
        status = "✅" if stats['compiles'] else "🔴"
        print(f"{i}. {filename} {status} ({stats['complexity']})")

if __name__ == '__main__':
    main()