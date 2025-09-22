#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["pathlib", "argparse", "json", "datetime"]
# ///

"""
track_progress.py - Update and maintain Phase 4 progress tracking files

This script maintains systematic progress tracking across Phase 4 sessions:
1. Updates PHASE_4_STATUS.md with current file states
2. Tracks subphase progression per file
3. Maintains session completion logs
4. Generates progress reports for planning

Usage:
    python3 track_progress.py --update-status filename.c subphase_3 completed
    python3 track_progress.py --file-status filename.c
    python3 track_progress.py --generate-report
"""

import argparse
import json
import pathlib
import sys
from datetime import datetime
from typing import Dict, List, Optional

class ProgressTracker:
    def __init__(self, base_dir='.'):
        self.base_dir = pathlib.Path(base_dir)
        self.reports_dir = self.base_dir / '_modernization' / 'claude' / 'reports'
        self.status_file = self.reports_dir / 'PHASE_4_STATUS.md'
        self.progress_db = self.reports_dir / 'phase_4_progress.json'

        # Ensure directories exist
        self.reports_dir.mkdir(parents=True, exist_ok=True)

        # Load existing progress data
        self.progress_data = self.load_progress_data()

    def load_progress_data(self) -> Dict:
        """Load progress tracking database"""
        if self.progress_db.exists():
            try:
                return json.loads(self.progress_db.read_text())
            except (json.JSONDecodeError, Exception) as e:
                print(f"Warning: Could not load progress database: {e}")

        # Initialize default structure
        return {
            'files': {},
            'sessions': [],
            'last_updated': None,
            'phase_4_start': datetime.now().isoformat()
        }

    def save_progress_data(self):
        """Save progress tracking database"""
        self.progress_data['last_updated'] = datetime.now().isoformat()
        self.progress_db.write_text(json.dumps(self.progress_data, indent=2))

    def update_file_status(self, filename: str, subphase: str, status: str, notes: str = ""):
        """Update status of a specific file"""
        if filename not in self.progress_data['files']:
            self.progress_data['files'][filename] = {
                'history': [],
                'current_subphase': 'subphase_0',
                'current_status': 'pending',
                'classification': 'unknown',
                'errors': 0,
                'warnings': 0
            }

        file_data = self.progress_data['files'][filename]

        # Add history entry
        history_entry = {
            'timestamp': datetime.now().isoformat(),
            'subphase': subphase,
            'status': status,
            'notes': notes
        }
        file_data['history'].append(history_entry)

        # Update current state
        file_data['current_subphase'] = subphase
        file_data['current_status'] = status

        self.save_progress_data()
        print(f"Updated {filename}: {subphase} -> {status}")

    def get_file_status(self, filename: str) -> Optional[Dict]:
        """Get current status of a file"""
        return self.progress_data['files'].get(filename)

    def classify_file(self, filename: str, complexity: str, function_count: int, errors: int, warnings: int):
        """Update file classification data"""
        if filename not in self.progress_data['files']:
            self.progress_data['files'][filename] = {
                'history': [],
                'current_subphase': 'subphase_0',
                'current_status': 'pending'
            }

        file_data = self.progress_data['files'][filename]
        file_data.update({
            'classification': complexity,
            'function_count': function_count,
            'errors': errors,
            'warnings': warnings,
            'last_classified': datetime.now().isoformat()
        })

        self.save_progress_data()

    def record_session(self, session_type: str, files_worked: List[str], summary: str):
        """Record a completed session"""
        session_entry = {
            'timestamp': datetime.now().isoformat(),
            'type': session_type,
            'files': files_worked,
            'summary': summary
        }
        self.progress_data['sessions'].append(session_entry)
        self.save_progress_data()

    def generate_status_markdown(self) -> str:
        """Generate updated PHASE_4_STATUS.md content"""
        content = []
        content.append("# Phase 4: Warning Elimination Status")
        content.append("")
        content.append(f"**Last Updated**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        content.append("**Strategy Reference**: `_modernization/claude/reports/PHASE_4_STRATEGY.md`")
        content.append("")

        # Overall progress summary
        files = self.progress_data['files']
        total_files = len(files)
        completed_files = sum(1 for f in files.values() if f.get('current_status') == 'strategy_complete')
        in_progress = sum(1 for f in files.values() if f.get('current_status') == 'in_progress')
        blocked_files = sum(1 for f in files.values() if f.get('current_status') == 'blocked')

        content.append("## Overall Progress")
        content.append(f"- **Total Files**: {total_files}")
        content.append(f"- **Strategy Complete**: {completed_files}")
        content.append(f"- **In Progress**: {in_progress}")
        content.append(f"- **Blocked**: {blocked_files}")
        content.append(f"- **Remaining**: {total_files - completed_files - in_progress - blocked_files}")
        content.append("")

        # Status definitions
        content.append("## Status Definitions")
        content.append("")
        content.append("- ✅ **STRATEGY-COMPLETE**: Following new methodology, zero warnings with current flags")
        content.append("- 🔄 **IN-PROGRESS**: Partially modernized, following strategy subphases")
        content.append("- 📋 **READY**: Ready for next strategy subphase")
        content.append("- ⚠️ **BLOCKED**: Has compilation errors preventing progress")
        content.append("- ❌ **NEEDS-REEVAL**: Previous completion claims need strategy verification")
        content.append("")

        # Current subphase targets
        content.append("## Current Subphase Targets")
        content.append("")
        content.append("Based on Phase 4 Strategy systematic progression:")
        content.append("")
        content.append("### Subphase 1: Critical Dependencies (C99 Basic Compilation)")
        content.append("**Target**: All .h files compile without errors under C99")
        content.append("**Command**: `gcc -O2 -g -std=c99 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE -c test_headers.c`")
        content.append("")
        content.append("### Subphase 2: Basic Source Compilation (C99 Error-Free)")
        content.append("**Target**: All .c files compile without errors under C99 (warnings allowed)")
        content.append("**Command**: `gcc -O2 -g -std=c99 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE -c filename.c`")
        content.append("")
        content.append("### Subphase 3: First Warning Flag (-Wall Only)")
        content.append("**Target**: All files compile with -Wall and zero warnings")
        content.append("**Command**: `gcc -O2 -g -std=c99 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE -Wall -c filename.c`")
        content.append("")

        # File status by category
        by_status = {}
        for filename, file_data in files.items():
            status = file_data.get('current_status', 'unknown')
            if status not in by_status:
                by_status[status] = []
            by_status[status].append((filename, file_data))

        status_order = ['strategy_complete', 'in_progress', 'ready', 'blocked', 'needs_reeval', 'unknown']
        status_icons = {
            'strategy_complete': '✅',
            'in_progress': '🔄',
            'ready': '📋',
            'blocked': '⚠️',
            'needs_reeval': '❌',
            'unknown': '❓'
        }

        for status in status_order:
            if status in by_status:
                files_list = by_status[status]
                icon = status_icons.get(status, '❓')
                content.append(f"## {icon} {status.upper().replace('_', ' ')} ({len(files_list)} files)")
                content.append("")

                for filename, file_data in sorted(files_list):
                    subphase = file_data.get('current_subphase', 'unknown')
                    classification = file_data.get('classification', 'unknown')
                    errors = file_data.get('errors', 0)
                    warnings = file_data.get('warnings', 0)

                    content.append(f"### {filename}")
                    content.append(f"- **Current Subphase**: {subphase}")
                    content.append(f"- **File Classification**: {classification}")
                    content.append(f"- **Compilation Status**: {errors} errors, {warnings} warnings")

                    # Show recent history
                    history = file_data.get('history', [])
                    if history:
                        recent = history[-3:]  # Last 3 entries
                        content.append("- **Recent History**:")
                        for entry in recent:
                            timestamp = entry['timestamp'][:16]  # YYYY-MM-DD HH:MM
                            content.append(f"  - {timestamp}: {entry['subphase']} -> {entry['status']}")
                            if entry.get('notes'):
                                content.append(f"    Note: {entry['notes']}")

                    content.append("")

        # Next steps section
        content.append("## Next Steps")
        content.append("")

        # Prioritize based on current status
        if 'blocked' in by_status:
            content.append("### Priority 1: Fix Blocked Files")
            for filename, _ in by_status['blocked']:
                content.append(f"- Fix compilation errors in {filename}")

        if 'ready' in by_status:
            content.append("### Priority 2: Process Ready Files")
            for filename, file_data in by_status['ready']:
                classification = file_data.get('classification', 'unknown')
                subphase = file_data.get('current_subphase', 'subphase_1')
                content.append(f"- Continue {filename} ({classification} file) from {subphase}")

        content.append("")
        content.append("### Session Planning Guidelines")
        content.append("- **SMALL files**: Can use accelerated progression")
        content.append("- **MEDIUM files**: Standard subphase progression")
        content.append("- **LARGE files**: Function-by-function approach, multiple sessions")
        content.append("- **COMPLEX files**: Maximum caution, 3-5 function chunks per session")
        content.append("")

        return '\n'.join(content)

    def update_status_file(self):
        """Update the main PHASE_4_STATUS.md file"""
        content = self.generate_status_markdown()
        self.status_file.write_text(content)
        print(f"Updated {self.status_file}")

    def generate_progress_report(self) -> str:
        """Generate detailed progress report"""
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        report_file = self.reports_dir / f'phase_4_progress_report_{timestamp}.md'

        content = []
        content.append("# Phase 4 Progress Report")
        content.append("")
        content.append(f"**Generated**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        content.append("")

        # Statistics
        files = self.progress_data['files']
        sessions = self.progress_data['sessions']

        content.append("## Progress Statistics")
        content.append(f"- **Total Files Tracked**: {len(files)}")
        content.append(f"- **Total Sessions**: {len(sessions)}")

        # Calculate time since start
        if self.progress_data.get('phase_4_start'):
            start_date = datetime.fromisoformat(self.progress_data['phase_4_start'])
            duration = datetime.now() - start_date
            content.append(f"- **Phase 4 Duration**: {duration.days} days")

        content.append("")

        # Progress by file classification
        by_classification = {}
        for filename, file_data in files.items():
            classification = file_data.get('classification', 'unknown')
            if classification not in by_classification:
                by_classification[classification] = []
            by_classification[classification].append((filename, file_data))

        content.append("## Progress by File Complexity")
        for classification in ['SMALL', 'MEDIUM', 'LARGE', 'COMPLEX', 'unknown']:
            if classification in by_classification:
                files_list = by_classification[classification]
                completed = sum(1 for _, f in files_list if f.get('current_status') == 'strategy_complete')
                content.append(f"- **{classification}**: {completed}/{len(files_list)} complete")

        content.append("")

        # Recent activity
        content.append("## Recent Sessions")
        recent_sessions = sessions[-5:] if len(sessions) > 5 else sessions
        for session in reversed(recent_sessions):
            timestamp = session['timestamp'][:16]
            content.append(f"### {timestamp} - {session['type']}")
            content.append(f"**Files**: {', '.join(session['files'])}")
            content.append(f"**Summary**: {session['summary']}")
            content.append("")

        # Detailed file progress
        content.append("## Detailed File Progress")
        for filename, file_data in sorted(files.items()):
            content.append(f"### {filename}")
            content.append(f"- **Status**: {file_data.get('current_status', 'unknown')}")
            content.append(f"- **Subphase**: {file_data.get('current_subphase', 'unknown')}")
            content.append(f"- **Classification**: {file_data.get('classification', 'unknown')}")

            history = file_data.get('history', [])
            if history:
                content.append(f"- **Progress History**: {len(history)} updates")
                latest = history[-1]
                content.append(f"  - Latest: {latest['timestamp'][:16]} - {latest['subphase']} -> {latest['status']}")

            content.append("")

        report_content = '\n'.join(content)
        report_file.write_text(report_content)

        return str(report_file)

def main():
    parser = argparse.ArgumentParser(description='Track Phase 4 progress systematically')

    # Action commands
    parser.add_argument('--update-status', nargs=4, metavar=('FILENAME', 'SUBPHASE', 'STATUS', 'NOTES'),
                       help='Update file status: filename subphase status notes')
    parser.add_argument('--classify-file', nargs=5, metavar=('FILENAME', 'COMPLEXITY', 'FUNCS', 'ERRORS', 'WARNINGS'),
                       help='Classify file: filename complexity function_count errors warnings')
    parser.add_argument('--file-status', metavar='FILENAME',
                       help='Show current status of specific file')
    parser.add_argument('--record-session', nargs=3, metavar=('TYPE', 'FILES', 'SUMMARY'),
                       help='Record completed session: type "file1,file2" summary')
    parser.add_argument('--generate-report', action='store_true',
                       help='Generate detailed progress report')
    parser.add_argument('--update-main-status', action='store_true',
                       help='Update main PHASE_4_STATUS.md file')

    # Options
    parser.add_argument('--base-dir', default='.',
                       help='Base directory for project (default: current)')

    args = parser.parse_args()

    tracker = ProgressTracker(args.base_dir)

    if args.update_status:
        filename, subphase, status, notes = args.update_status
        tracker.update_file_status(filename, subphase, status, notes)
        tracker.update_status_file()

    elif args.classify_file:
        filename, complexity, func_count, errors, warnings = args.classify_file
        tracker.classify_file(filename, complexity, int(func_count), int(errors), int(warnings))

    elif args.file_status:
        status = tracker.get_file_status(args.file_status)
        if status:
            print(f"Status for {args.file_status}:")
            print(f"  Current Subphase: {status.get('current_subphase')}")
            print(f"  Current Status: {status.get('current_status')}")
            print(f"  Classification: {status.get('classification')}")
            print(f"  Errors/Warnings: {status.get('errors', 0)}/{status.get('warnings', 0)}")

            history = status.get('history', [])
            if history:
                print(f"  Recent Activity:")
                for entry in history[-3:]:
                    print(f"    {entry['timestamp'][:16]}: {entry['subphase']} -> {entry['status']}")
        else:
            print(f"No status found for {args.file_status}")

    elif args.record_session:
        session_type, files_str, summary = args.record_session
        files_list = [f.strip() for f in files_str.split(',')]
        tracker.record_session(session_type, files_list, summary)

    elif args.generate_report:
        report_file = tracker.generate_progress_report()
        print(f"Generated progress report: {report_file}")

    elif args.update_main_status:
        tracker.update_status_file()

    else:
        print("Specify an action. Use --help for options.")
        sys.exit(1)

if __name__ == '__main__':
    main()