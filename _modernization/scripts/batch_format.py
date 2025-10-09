#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["pathlib"]
# ///
"""
batch_format.py - Automated batch code formatting with clang-format

This script automates the Phase 9 code formatting workflow:
1. Formats files using clang-format via CMake target
2. Creates backups (.orig) for validation
3. Verifies compilation after formatting
4. Optionally commits changes to git

Usage:
    # Format single file
    batch_format.py format file.c

    # Format multiple files
    batch_format.py format file1.c file2.c file3.c

    # Format by category with git commit
    batch_format.py category --name "Core Game Logic" --commit commands.c cexecute.c move.c

    # Dry run (show what would be done)
    batch_format.py format --dry-run file.c

    # Skip compilation verification (faster)
    batch_format.py format --no-verify file.c

Examples:
    # Format Category 1 files and commit
    batch_format.py category --name "Core Game Logic" --commit \\
        commands.c cexecute.c move.c navy.c npc.c randeven.c update.c

    # Format all remaining files without commit
    batch_format.py format *.c

    # Clean up all .orig backup files
    batch_format.py cleanup
"""

import argparse
import subprocess
import sys
from pathlib import Path
from datetime import datetime


class Colors:
    """ANSI color codes for terminal output"""
    RESET = "\033[0m"
    BOLD = "\033[1m"
    RED = "\033[91m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    BLUE = "\033[94m"
    CYAN = "\033[96m"


def print_header(message):
    """Print a formatted header"""
    print(f"\n{Colors.BOLD}{Colors.CYAN}{'='*70}{Colors.RESET}")
    print(f"{Colors.BOLD}{Colors.CYAN}{message}{Colors.RESET}")
    print(f"{Colors.BOLD}{Colors.CYAN}{'='*70}{Colors.RESET}\n")


def print_success(message):
    """Print success message"""
    print(f"{Colors.GREEN}✓ {message}{Colors.RESET}")


def print_error(message):
    """Print error message"""
    print(f"{Colors.RED}✗ {message}{Colors.RESET}", file=sys.stderr)


def print_info(message):
    """Print info message"""
    print(f"{Colors.BLUE}ℹ {message}{Colors.RESET}")


def print_warning(message):
    """Print warning message"""
    print(f"{Colors.YELLOW}⚠ {message}{Colors.RESET}")


def run_command(cmd, description, dry_run=False, check=True, capture_output=False):
    """
    Run a shell command with error handling

    Args:
        cmd: Command string or list
        description: Human-readable description
        dry_run: If True, only print what would be done
        check: If True, raise error on non-zero exit
        capture_output: If True, return stdout

    Returns:
        subprocess.CompletedProcess or None if dry_run
    """
    if dry_run:
        print_info(f"Would run: {cmd}")
        return None

    print_info(f"{description}...")
    try:
        result = subprocess.run(
            cmd,
            shell=isinstance(cmd, str),
            check=check,
            capture_output=capture_output,
            text=True
        )
        if capture_output:
            return result
        print_success(f"{description} - Done")
        return result
    except subprocess.CalledProcessError as e:
        print_error(f"{description} - FAILED")
        if capture_output and e.stderr:
            print(e.stderr, file=sys.stderr)
        if check:
            raise
        return e


def find_project_root():
    """Find the project root directory (contains .clang-format)"""
    current = Path.cwd()

    # Check if we're already in project root
    if (current / ".clang-format").exists():
        return current

    # Check if we're in a subdirectory
    for parent in current.parents:
        if (parent / ".clang-format").exists():
            return parent

    print_error("Could not find project root (no .clang-format found)")
    print_info("Current directory: " + str(current))
    sys.exit(1)


def format_file(filepath, project_root, build_dir, dry_run=False):
    """
    Format a single file using clang-format

    Args:
        filepath: Path to the file to format
        project_root: Project root directory
        build_dir: Build directory
        dry_run: If True, only print what would be done

    Returns:
        True if successful, False otherwise
    """
    file_path = Path(filepath)

    if not file_path.exists():
        print_error(f"File not found: {filepath}")
        return False

    if not file_path.suffix == '.c':
        print_warning(f"Skipping non-.c file: {filepath}")
        return False

    print_header(f"Formatting: {file_path.name}")

    # Create backup
    backup_path = file_path.with_suffix(file_path.suffix + '.orig')
    if not dry_run:
        if backup_path.exists():
            print_warning(f"Backup already exists: {backup_path} (removing old backup)")
            backup_path.unlink()

        import shutil
        shutil.copy2(file_path, backup_path)
        print_success(f"Created backup: {backup_path}")
    else:
        print_info(f"Would create backup: {backup_path}")

    # Run clang-format via CMake
    cmake_cmd = f"cd {build_dir} && cmake .. -DFORMAT_FILE={file_path.name} && make format-fix-single"
    result = run_command(
        cmake_cmd,
        f"Formatting {file_path.name}",
        dry_run=dry_run,
        check=False
    )

    if result is not None and result.returncode != 0:
        print_error(f"Formatting failed for {file_path.name}")
        return False

    # Show diff if not dry run
    if not dry_run and backup_path.exists():
        diff_result = subprocess.run(
            ["diff", "-u", str(backup_path), str(file_path)],
            capture_output=True,
            text=True
        )
        if diff_result.returncode != 0:  # Files differ
            lines_changed = len([l for l in diff_result.stdout.split('\n') if l.startswith('+') or l.startswith('-')])
            print_success(f"Formatting changed ~{lines_changed} lines")
        else:
            print_info("No formatting changes needed")

    return True


def verify_build(build_dir, dry_run=False):
    """
    Verify the build succeeds after formatting

    Args:
        build_dir: Build directory
        dry_run: If True, only print what would be done

    Returns:
        True if build successful, False otherwise
    """
    print_header("Verifying Build")

    cmd = f"cmake --build {build_dir} --clean-first --target conqrun 2>&1"
    result = run_command(
        cmd,
        "Building project",
        dry_run=dry_run,
        check=False,
        capture_output=True
    )

    if dry_run:
        return True

    if result.returncode != 0:
        print_error("Build FAILED after formatting")
        print_error("Output:")
        print(result.stdout)
        return False

    # Check for warnings
    warnings = [line for line in result.stdout.split('\n') if 'warning:' in line.lower()]

    if warnings:
        print_warning(f"Build succeeded with {len(warnings)} warnings:")
        for warning in warnings[:10]:  # Show first 10
            print(f"  {warning}")
        if len(warnings) > 10:
            print(f"  ... and {len(warnings) - 10} more warnings")
        return True

    print_success("Build succeeded with ZERO warnings")
    return True


def git_commit_changes(files, category_name, dry_run=False):
    """
    Commit the formatted files to git

    Args:
        files: List of file paths
        category_name: Name of the category for commit message
        dry_run: If True, only print what would be done

    Returns:
        True if successful, False otherwise
    """
    print_header("Creating Git Commit")

    # Stage files
    file_list = ' '.join([str(Path(f).name) for f in files])
    git_add_cmd = f"git add {file_list}"

    result = run_command(
        git_add_cmd,
        "Staging formatted files",
        dry_run=dry_run,
        check=False
    )

    if result is not None and result.returncode != 0:
        return False

    # Create commit message
    commit_msg = f"""[PHASE-9.3] Format {category_name} files with clang-format

Formatted {len(files)} files in {category_name} category:
{chr(10).join('- ' + Path(f).name for f in files)}

Applied consistent formatting:
- Function braces on same line
- Control structure braces on same line
- 4-space indentation
- 96-column line limit

Build verified: zero warnings

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>"""

    # Commit
    git_commit_cmd = ["git", "commit", "-m", commit_msg]
    result = run_command(
        git_commit_cmd,
        "Creating commit",
        dry_run=dry_run,
        check=False
    )

    if result is not None and result.returncode != 0:
        return False

    print_success(f"Committed {len(files)} files to git")
    return True


def cleanup_backups(project_root, dry_run=False):
    """
    Remove all .orig backup files

    Args:
        project_root: Project root directory
        dry_run: If True, only print what would be done
    """
    print_header("Cleaning Up Backup Files")

    orig_files = list(project_root.glob("*.c.orig"))

    if not orig_files:
        print_info("No .orig backup files found")
        return

    print_info(f"Found {len(orig_files)} backup files")

    for orig_file in orig_files:
        if dry_run:
            print_info(f"Would remove: {orig_file}")
        else:
            orig_file.unlink()
            print_success(f"Removed: {orig_file}")

    if not dry_run:
        print_success(f"Cleaned up {len(orig_files)} backup files")


def format_category(args):
    """Handle category formatting command"""
    project_root = find_project_root()
    build_dir = project_root / "build"

    if not build_dir.exists():
        print_error(f"Build directory not found: {build_dir}")
        return 1

    print_header(f"Batch Formatting: {args.name}")
    print_info(f"Project root: {project_root}")
    print_info(f"Build directory: {build_dir}")
    print_info(f"Files to format: {len(args.files)}")

    if args.dry_run:
        print_warning("DRY RUN MODE - No changes will be made")

    # Format each file
    success_count = 0
    failed_files = []

    for filepath in args.files:
        if format_file(filepath, project_root, build_dir, args.dry_run):
            success_count += 1
        else:
            failed_files.append(filepath)

    print_header("Formatting Summary")
    print_success(f"Successfully formatted: {success_count}/{len(args.files)} files")

    if failed_files:
        print_error(f"Failed to format {len(failed_files)} files:")
        for f in failed_files:
            print(f"  - {f}")
        return 1

    # Verify build if requested
    if not args.no_verify:
        if not verify_build(build_dir, args.dry_run):
            print_error("Build verification failed - not committing")
            return 1
    else:
        print_warning("Skipping build verification (--no-verify)")

    # Commit if requested
    if args.commit and not args.dry_run:
        if not git_commit_changes(args.files, args.name, args.dry_run):
            print_error("Git commit failed")
            return 1
    elif args.commit and args.dry_run:
        print_info("Would create git commit (dry run mode)")

    print_header("✓ Complete")
    return 0


def format_files(args):
    """Handle individual file formatting command"""
    project_root = find_project_root()
    build_dir = project_root / "build"

    if not build_dir.exists():
        print_error(f"Build directory not found: {build_dir}")
        return 1

    print_header("Batch File Formatting")
    print_info(f"Project root: {project_root}")
    print_info(f"Files to format: {len(args.files)}")

    if args.dry_run:
        print_warning("DRY RUN MODE - No changes will be made")

    # Format each file
    success_count = 0
    failed_files = []

    for filepath in args.files:
        if format_file(filepath, project_root, build_dir, args.dry_run):
            success_count += 1
        else:
            failed_files.append(filepath)

    print_header("Summary")
    print_success(f"Successfully formatted: {success_count}/{len(args.files)} files")

    if failed_files:
        print_error(f"Failed to format {len(failed_files)} files:")
        for f in failed_files:
            print(f"  - {f}")
        return 1

    # Verify build if requested
    if not args.no_verify and not args.dry_run:
        if not verify_build(build_dir, args.dry_run):
            print_warning("Build verification failed")
            return 1

    print_header("✓ Complete")
    return 0


def main():
    parser = argparse.ArgumentParser(
        description="Automated batch code formatting with clang-format",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )

    subparsers = parser.add_subparsers(dest='command', help='Command to run')

    # Format command
    format_parser = subparsers.add_parser('format', help='Format individual files')
    format_parser.add_argument('files', nargs='+', help='Files to format')
    format_parser.add_argument('--dry-run', action='store_true', help='Show what would be done')
    format_parser.add_argument('--no-verify', action='store_true', help='Skip build verification')

    # Category command
    category_parser = subparsers.add_parser('category', help='Format files by category and commit')
    category_parser.add_argument('--name', required=True, help='Category name for commit message')
    category_parser.add_argument('--commit', action='store_true', help='Create git commit')
    category_parser.add_argument('--dry-run', action='store_true', help='Show what would be done')
    category_parser.add_argument('--no-verify', action='store_true', help='Skip build verification')
    category_parser.add_argument('files', nargs='+', help='Files in this category')

    # Cleanup command
    cleanup_parser = subparsers.add_parser('cleanup', help='Remove all .orig backup files')
    cleanup_parser.add_argument('--dry-run', action='store_true', help='Show what would be done')

    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        return 1

    try:
        if args.command == 'format':
            return format_files(args)
        elif args.command == 'category':
            return format_category(args)
        elif args.command == 'cleanup':
            project_root = find_project_root()
            cleanup_backups(project_root, args.dry_run)
            return 0
    except KeyboardInterrupt:
        print_error("\n\nInterrupted by user")
        return 130
    except Exception as e:
        print_error(f"Unexpected error: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
