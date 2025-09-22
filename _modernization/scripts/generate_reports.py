#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["pathlib", "argparse", "subprocess", "json"]
# ///

"""
generate_reports.py - Create session summary reports for Phase 4

This script generates comprehensive reports for Phase 4 session planning:
1. Compilation status summaries
2. Warning category breakdowns
3. Session planning recommendations
4. Integration with other automation scripts

Usage:
    python3 generate_reports.py --session-summary
    python3 generate_reports.py --compilation-report subphase_3
    python3 generate_reports.py --planning-report next-session
"""

import argparse
import json
import pathlib
import subprocess
import sys
from datetime import datetime
from typing import Dict, List

class ReportGenerator:
    def __init__(self, base_dir='.'):
        self.base_dir = pathlib.Path(base_dir)
        self.reports_dir = self.base_dir / '_modernization' / 'claude' / 'reports'
        self.scripts_dir = self.base_dir / '_modernization' / 'scripts'

        # Ensure directories exist
        self.reports_dir.mkdir(parents=True, exist_ok=True)

    def run_analysis_script(self, script_name: str, args: List[str] = None) -> Dict:
        """Run one of the other automation scripts and capture results"""
        script_path = self.scripts_dir / script_name
        if not script_path.exists():
            print(f"Warning: Script {script_name} not found")
            return {}

        cmd = ['python3', str(script_path)]
        if args:
            cmd.extend(args)

        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
            return {
                'returncode': result.returncode,
                'stdout': result.stdout,
                'stderr': result.stderr,
                'success': result.returncode == 0
            }
        except subprocess.TimeoutExpired:
            return {'success': False, 'error': 'Script timeout'}
        except Exception as e:
            return {'success': False, 'error': str(e)}

    def load_progress_data(self) -> Dict:
        """Load current progress tracking data"""
        progress_file = self.reports_dir / 'phase_4_progress.json'
        if progress_file.exists():
            try:
                return json.loads(progress_file.read_text())
            except Exception as e:
                print(f"Warning: Could not load progress data: {e}")
        return {}

    def generate_session_summary(self, session_focus: str = None) -> str:
        """Generate comprehensive session summary report"""
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        output_file = self.reports_dir / f'session_summary_{timestamp}.md'

        content = []
        content.append('# Phase 4 Session Summary Report')
        content.append('')
        content.append(f'**Generated**: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}')
        if session_focus:
            content.append(f'**Session Focus**: {session_focus}')
        content.append('')

        # Run compilation analysis
        print("Running compilation analysis...")
        comp_result = self.run_analysis_script('analyze_compilation.py', ['--test-mode', '--dry-run'])

        if comp_result.get('success'):
            content.append('## Compilation Status')
            content.append('✅ Compilation analysis completed successfully')
            # Parse key statistics from stdout
            stdout = comp_result.get('stdout', '')
            for line in stdout.split('\n'):
                if 'Tested' in line and 'files' in line:
                    content.append(f'- {line.strip()}')
        else:
            content.append('## Compilation Status')
            content.append('❌ Compilation analysis failed')
            content.append(f'Error: {comp_result.get("error", "Unknown error")}')

        content.append('')

        # Run file prioritization
        print("Running file prioritization...")
        priority_result = self.run_analysis_script('prioritize_files.py', ['--analyze', '--dry-run'])

        if priority_result.get('success'):
            content.append('## File Priority Analysis')
            content.append('✅ Priority analysis completed successfully')
            # Parse top priorities from stdout
            stdout = priority_result.get('stdout', '')
            if 'Top 5 priority files:' in stdout:
                lines = stdout.split('\n')
                capturing = False
                for line in lines:
                    if 'Top 5 priority files:' in line:
                        capturing = True
                        content.append('### Top Priority Files')
                        continue
                    elif capturing and line.strip() and line[0].isdigit():
                        content.append(f'- {line.strip()}')
                    elif capturing and not line.strip():
                        break
        else:
            content.append('## File Priority Analysis')
            content.append('❌ Priority analysis failed')

        content.append('')

        # Load and summarize progress data
        progress_data = self.load_progress_data()
        if progress_data:
            content.append('## Current Progress Summary')
            files = progress_data.get('files', {})
            if files:
                by_status = {}
                for filename, file_data in files.items():
                    status = file_data.get('current_status', 'unknown')
                    by_status.setdefault(status, []).append(filename)

                for status, files_list in by_status.items():
                    icon = {'strategy_complete': '✅', 'in_progress': '🔄', 'ready': '📋', 'blocked': '⚠️'}.get(status, '❓')
                    content.append(f'- **{status.replace("_", " ").title()}** {icon}: {len(files_list)} files')

            # Recent sessions
            sessions = progress_data.get('sessions', [])
            if sessions:
                content.append('')
                content.append('### Recent Activity')
                recent = sessions[-3:] if len(sessions) > 3 else sessions
                for session in reversed(recent):
                    timestamp = session['timestamp'][:16]
                    content.append(f'- **{timestamp}**: {session["type"]} - {", ".join(session["files"])}')

        content.append('')

        # Recommendations section
        content.append('## Session Recommendations')
        content.append('')

        # Analyze current state and provide recommendations
        if progress_data:
            files = progress_data.get('files', {})
            blocked_files = [f for f, data in files.items() if data.get('current_status') == 'blocked']
            ready_files = [f for f, data in files.items() if data.get('current_status') == 'ready']
            in_progress = [f for f, data in files.items() if data.get('current_status') == 'in_progress']

            if blocked_files:
                content.append('### 🔴 Priority 1: Fix Blocked Files')
                for filename in blocked_files[:5]:  # Show first 5
                    content.append(f'- **{filename}**: Fix compilation errors before proceeding')
                if len(blocked_files) > 5:
                    content.append(f'- ... and {len(blocked_files) - 5} more blocked files')
                content.append('')

            if in_progress:
                content.append('### 🔄 Priority 2: Continue In-Progress Files')
                for filename in in_progress:
                    file_data = files[filename]
                    subphase = file_data.get('current_subphase', 'unknown')
                    classification = file_data.get('classification', 'unknown')
                    content.append(f'- **{filename}**: Continue from {subphase} ({classification} file)')
                content.append('')

            if ready_files:
                content.append('### 📋 Priority 3: Start Ready Files')
                for filename in ready_files[:3]:  # Show first 3
                    file_data = files[filename]
                    classification = file_data.get('classification', 'unknown')
                    content.append(f'- **{filename}**: Begin systematic progression ({classification} file)')
                if len(ready_files) > 3:
                    content.append(f'- ... and {len(ready_files) - 3} more ready files')

        content.append('')
        content.append('### Session Planning Guidelines')
        content.append('- **Focus on one file at a time** for better context management')
        content.append('- **Follow file classification strategy** (Small/Medium/Large/Complex)')
        content.append('- **Use single warning flag progression** from Phase 4 Strategy')
        content.append('- **Update progress tracking** after each significant milestone')
        content.append('')

        # Write report
        report_content = '\n'.join(content)
        output_file.write_text(report_content)

        return str(output_file)

    def generate_compilation_report(self, subphase: str = 'subphase_3') -> str:
        """Generate detailed compilation status report for specific subphase"""
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        output_file = self.reports_dir / f'compilation_report_{subphase}_{timestamp}.md'

        content = []
        content.append(f'# Compilation Report - {subphase.upper()}')
        content.append('')
        content.append(f'**Generated**: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}')
        content.append(f'**Subphase**: {subphase}')
        content.append('')

        # Run detailed compilation analysis
        print(f"Running compilation analysis for {subphase}...")
        result = self.run_analysis_script('analyze_compilation.py', [
            '--test-mode', '--subphase', subphase,
            '--output-dir', str(self.reports_dir)
        ])

        if result.get('success'):
            content.append('## Compilation Analysis Results')
            content.append('✅ Analysis completed successfully')
            content.append('')

            # Look for generated analysis report
            analysis_files = list(self.reports_dir.glob('compilation_analysis_*.md'))
            if analysis_files:
                latest_analysis = max(analysis_files, key=lambda f: f.stat().st_mtime)
                content.append(f'**Detailed Report**: {latest_analysis.name}')
                content.append('')

                # Extract key findings from the analysis
                try:
                    analysis_content = latest_analysis.read_text()
                    # Extract summary statistics
                    lines = analysis_content.split('\n')
                    in_summary = False
                    for line in lines:
                        if '## Summary Statistics' in line:
                            in_summary = True
                            content.append('## Key Statistics')
                            continue
                        elif in_summary and line.startswith('- **'):
                            content.append(line)
                        elif in_summary and line.startswith('##'):
                            break
                except Exception as e:
                    content.append(f'Note: Could not extract summary from analysis: {e}')

        else:
            content.append('## Compilation Analysis Results')
            content.append('❌ Analysis failed')
            content.append(f'Error: {result.get("error", "Unknown error")}')

        content.append('')

        # Add subphase-specific recommendations
        content.append(f'## {subphase.upper()} Specific Guidance')
        content.append('')

        subphase_guidance = {
            'subphase_1': [
                'Focus on header file compilation first',
                'Fix critical dependencies before source files',
                'Use test_headers.c approach for header testing',
                'Address compilation errors only, ignore warnings'
            ],
            'subphase_2': [
                'Ensure all C files compile without errors',
                'Still ignore warnings at this stage',
                'Test both CONQUER and ADMIN configurations',
                'Create GitHub issues for persistent compilation errors'
            ],
            'subphase_3': [
                'Add -Wall flag and address all warnings',
                'Stay in C99 standard for stability',
                'Group similar warnings for efficient fixing',
                'Use file classification for session planning'
            ],
            'subphase_4': [
                'Add -Wextra flag to existing -Wall',
                'Focus only on new warnings from -Wextra',
                'Continue with C99 standard',
                'Break large files into multiple sessions'
            ],
            'subphase_5': [
                'Add -Wpedantic flag to existing flags',
                'Address strict standard compliance issues',
                'Still maintain C99 for compatibility',
                'Document any behavioral changes needed'
            ]
        }

        guidance = subphase_guidance.get(subphase, ['Follow Phase 4 Strategy guidelines'])
        for item in guidance:
            content.append(f'- {item}')

        content.append('')

        # Next steps
        content.append('## Recommended Next Steps')
        content.append('')

        next_subphases = {
            'subphase_1': 'subphase_2 (Basic Source Compilation)',
            'subphase_2': 'subphase_3 (Add -Wall flag)',
            'subphase_3': 'subphase_4 (Add -Wextra flag)',
            'subphase_4': 'subphase_5 (Add -Wpedantic flag)',
            'subphase_5': 'subphase_6 (Add -Wformat=2 flag)'
        }

        if subphase in next_subphases:
            content.append(f'1. Complete all files in {subphase}')
            content.append(f'2. Verify zero warnings/errors before proceeding')
            content.append(f'3. Move to {next_subphases[subphase]}')
        else:
            content.append('1. Continue following Phase 4 Strategy progression')
            content.append('2. Refer to strategy document for next subphase')

        content.append('')
        content.append('---')
        content.append('')
        content.append(f'**Strategy Reference**: `_modernization/claude/reports/PHASE_4_STRATEGY.md`')
        content.append(f'**Progress Tracking**: `_modernization/claude/reports/PHASE_4_STATUS.md`')

        # Write report
        report_content = '\n'.join(content)
        output_file.write_text(report_content)

        return str(output_file)

    def generate_planning_report(self, focus: str = 'next-session') -> str:
        """Generate planning report for upcoming session"""
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        output_file = self.reports_dir / f'planning_report_{focus}_{timestamp}.md'

        content = []
        content.append('# Phase 4 Planning Report')
        content.append('')
        content.append(f'**Generated**: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}')
        content.append(f'**Focus**: {focus}')
        content.append('')

        # Load current state
        progress_data = self.load_progress_data()

        if progress_data:
            files = progress_data.get('files', {})

            # Analyze what needs attention
            blocked = [(f, d) for f, d in files.items() if d.get('current_status') == 'blocked']
            in_progress = [(f, d) for f, d in files.items() if d.get('current_status') == 'in_progress']
            ready = [(f, d) for f, d in files.items() if d.get('current_status') == 'ready']

            content.append('## Current State Analysis')
            content.append(f'- **Blocked Files**: {len(blocked)} (compilation errors)')
            content.append(f'- **In Progress**: {len(in_progress)} (partial completion)')
            content.append(f'- **Ready to Start**: {len(ready)} (awaiting systematic processing)')
            content.append('')

            # Session recommendations based on current state
            content.append('## Session Recommendations')
            content.append('')

            if blocked:
                content.append('### Immediate Priority: Fix Blocked Files')
                content.append('')
                content.append('These files have compilation errors preventing progress:')
                for filename, file_data in blocked[:3]:  # Top 3 blocked
                    errors = file_data.get('errors', 0)
                    content.append(f'- **{filename}**: {errors} compilation errors')
                    content.append(f'  - Classification: {file_data.get("classification", "unknown")}')
                    content.append(f'  - Last attempted: {file_data.get("current_subphase", "unknown")}')
                content.append('')
                content.append('**Recommended action**: Focus next session on fixing compilation errors in priority order')
                content.append('')

            elif in_progress:
                content.append('### Continue In-Progress Files')
                content.append('')
                for filename, file_data in in_progress:
                    subphase = file_data.get('current_subphase', 'unknown')
                    classification = file_data.get('classification', 'unknown')
                    warnings = file_data.get('warnings', 0)

                    content.append(f'- **{filename}** ({classification})')
                    content.append(f'  - Current subphase: {subphase}')
                    content.append(f'  - Warnings remaining: {warnings}')

                    # Session strategy based on classification
                    if classification == 'SMALL':
                        content.append(f'  - **Session strategy**: Accelerated progression, may complete multiple subphases')
                    elif classification == 'MEDIUM':
                        content.append(f'  - **Session strategy**: Standard progression, one subphase per session')
                    elif classification == 'LARGE':
                        content.append(f'  - **Session strategy**: Function-by-function approach, multiple sessions needed')
                    elif classification == 'COMPLEX':
                        content.append(f'  - **Session strategy**: Maximum caution, 3-5 functions per session')

                    content.append('')

            elif ready:
                content.append('### Start New Files')
                content.append('')
                content.append('Recommended files to begin systematic processing:')

                # Sort ready files by complexity (easier first)
                complexity_order = ['SMALL', 'MEDIUM', 'LARGE', 'COMPLEX']
                ready_sorted = sorted(ready, key=lambda x: complexity_order.index(x[1].get('classification', 'COMPLEX')))

                for filename, file_data in ready_sorted[:3]:  # Top 3 candidates
                    classification = file_data.get('classification', 'unknown')
                    warnings = file_data.get('warnings', 0)
                    content.append(f'- **{filename}** ({classification})')
                    content.append(f'  - Estimated warnings: {warnings}')

                    if classification == 'SMALL':
                        content.append(f'  - **Time estimate**: 1 session for multiple subphases')
                    elif classification == 'MEDIUM':
                        content.append(f'  - **Time estimate**: 2-3 sessions for full progression')
                    else:
                        content.append(f'  - **Time estimate**: Multiple sessions, careful progression needed')

                    content.append('')

        # Add context management advice
        content.append('## Context Management Guidelines')
        content.append('')
        content.append('### Session Planning')
        content.append('- **SMALL files**: Single session, multiple subphases possible')
        content.append('- **MEDIUM files**: One subphase per session, 2-3 sessions total')
        content.append('- **LARGE files**: Break by function groups, 5-8 functions per session')
        content.append('- **COMPLEX files**: Minimal chunks, 3-5 functions per session')
        content.append('')
        content.append('### Checkpoint Strategy')
        content.append('- Save progress after each significant milestone')
        content.append('- Update tracking files with current status')
        content.append('- Stop at 80% context for large files, 90% for small files')
        content.append('- Create git commits for completed subphases')
        content.append('')

        # Write report
        report_content = '\n'.join(content)
        output_file.write_text(report_content)

        return str(output_file)

def main():
    parser = argparse.ArgumentParser(description='Generate Phase 4 session reports')

    # Report types
    parser.add_argument('--session-summary', action='store_true',
                       help='Generate comprehensive session summary')
    parser.add_argument('--compilation-report', metavar='SUBPHASE',
                       help='Generate compilation report for specific subphase')
    parser.add_argument('--planning-report', metavar='FOCUS',
                       help='Generate planning report (e.g., "next-session")')

    # Options
    parser.add_argument('--base-dir', default='.',
                       help='Base directory for project')

    args = parser.parse_args()

    generator = ReportGenerator(args.base_dir)

    if args.session_summary:
        report_file = generator.generate_session_summary()
        print(f"Generated session summary: {report_file}")

    elif args.compilation_report:
        report_file = generator.generate_compilation_report(args.compilation_report)
        print(f"Generated compilation report: {report_file}")

    elif args.planning_report:
        report_file = generator.generate_planning_report(args.planning_report)
        print(f"Generated planning report: {report_file}")

    else:
        print("Specify a report type. Use --help for options.")
        sys.exit(1)

if __name__ == '__main__':
    main()