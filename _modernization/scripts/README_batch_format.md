# Batch Code Formatting Script

**Script**: `batch_format.py`
**Purpose**: Automated batch code formatting with clang-format for C modernization projects
**Phase**: Phase 9 - Code Formatting and Style Standardization

---

## Overview

The `batch_format.py` script automates the entire Phase 9 code formatting workflow:

1. ✅ Formats files using clang-format via CMake target
2. ✅ Creates `.orig` backups for validation
3. ✅ Verifies compilation after formatting
4. ✅ Optionally commits changes to git with standardized messages

This script eliminates manual context switching, makes the work discoverable, and is reusable for other C modernization projects.

---

## Quick Start

### Format Single File
```bash
_modernization/scripts/batch_format.py format commands.c
```

### Format Multiple Files
```bash
_modernization/scripts/batch_format.py format commands.c cexecute.c move.c
```

### Format Category with Git Commit
```bash
_modernization/scripts/batch_format.py category \
    --name "Core Game Logic" \
    --commit \
    commands.c cexecute.c move.c navy.c npc.c randeven.c update.c
```

### Dry Run (Preview Changes)
```bash
_modernization/scripts/batch_format.py format --dry-run commands.c
```

### Clean Up Backup Files
```bash
_modernization/scripts/batch_format.py cleanup
```

---

## Commands

### `format` - Format Individual Files

Formats one or more files without automatic git commit.

**Usage**:
```bash
batch_format.py format [OPTIONS] FILE [FILE ...]
```

**Options**:
- `--dry-run`: Show what would be done without making changes
- `--no-verify`: Skip build verification (faster, but risky)

**Examples**:
```bash
# Format single file with verification
batch_format.py format commands.c

# Format multiple files
batch_format.py format commands.c cexecute.c move.c

# Dry run to preview changes
batch_format.py format --dry-run commands.c

# Skip build verification (fast mode)
batch_format.py format --no-verify commands.c
```

---

### `category` - Format Files by Category and Commit

Formats multiple files in a logical category and creates a git commit.

**Usage**:
```bash
batch_format.py category --name CATEGORY_NAME [OPTIONS] FILE [FILE ...]
```

**Options**:
- `--name NAME` (required): Category name for commit message
- `--commit`: Create git commit after successful formatting
- `--dry-run`: Show what would be done without making changes
- `--no-verify`: Skip build verification

**Examples**:
```bash
# Format Core Game Logic category and commit
batch_format.py category \
    --name "Core Game Logic" \
    --commit \
    commands.c cexecute.c move.c navy.c npc.c randeven.c update.c

# Format I/O and Data category without commit
batch_format.py category \
    --name "I/O and Data Management" \
    io.c data.c display.c forms.c reports.c trade.c sort.c

# Dry run for System Utilities category
batch_format.py category \
    --name "System and Utilities" \
    --dry-run \
    admin.c m2alloc.c magic.c makeworl.c newlogin.c
```

---

### `cleanup` - Remove Backup Files

Removes all `.orig` backup files created during formatting.

**Usage**:
```bash
batch_format.py cleanup [OPTIONS]
```

**Options**:
- `--dry-run`: Show what would be removed without deleting

**Examples**:
```bash
# Remove all .orig backup files
batch_format.py cleanup

# Preview what would be removed
batch_format.py cleanup --dry-run
```

---

## Phase 9.3 Workflow

### Category 1: Core Game Logic (7 files)

```bash
_modernization/scripts/batch_format.py category \
    --name "Core Game Logic" \
    --commit \
    commands.c cexecute.c move.c navy.c npc.c randeven.c update.c
```

**Expected Output**:
- Formats 7 files
- Creates `.orig` backups
- Verifies build with zero warnings
- Creates git commit: `[PHASE-9.3] Format Core Game Logic files with clang-format`

---

### Category 2: I/O and Data Management (7 files)

```bash
_modernization/scripts/batch_format.py category \
    --name "I/O and Data Management" \
    --commit \
    io.c data.c display.c forms.c reports.c trade.c sort.c
```

---

### Category 3: System and Utilities (4 files)

```bash
_modernization/scripts/batch_format.py category \
    --name "System and Utilities" \
    --commit \
    admin.c m2alloc.c magic.c makeworl.c newlogin.c
```

---

### Category 4: UI and Interaction (4 files)

```bash
_modernization/scripts/batch_format.py category \
    --name "UI and Interaction" \
    --commit \
    extcmds.c main.c psmap.c spew.c
```

---

## Features

### ✅ Automated Workflow
- Handles entire formatting process automatically
- No manual cmake commands needed
- Consistent execution every time

### ✅ Safety Features
- Creates `.orig` backups before formatting
- Verifies build after formatting
- Dry run mode for preview
- Clear error messages and rollback guidance

### ✅ Git Integration
- Standardized commit messages
- Automatic file staging
- Co-authorship attribution
- Category-based organization

### ✅ User Experience
- Color-coded output (green ✓, red ✗, blue ℹ, yellow ⚠)
- Progress indicators
- Detailed summaries
- Clear headers and sections

---

## Output Format

### Successful Formatting
```
======================================================================
Formatting: commands.c
======================================================================

✓ Created backup: commands.c.orig
ℹ Formatting commands.c...
✓ Formatting commands.c - Done
✓ Formatting changed ~2894 lines

======================================================================
Verifying Build
======================================================================

ℹ Building project...
✓ Build succeeded with ZERO warnings

======================================================================
✓ Complete
======================================================================
```

### Build Warnings
```
⚠ Build succeeded with 3 warnings:
  file.c:123: warning: unused variable 'x'
  file.c:456: warning: implicit conversion
  file.c:789: warning: format specifier mismatch
```

### Errors
```
✗ File not found: nonexistent.c
✗ Build FAILED after formatting
✗ Git commit failed
```

---

## Requirements

- **Project Structure**: Must have `.clang-format` in project root
- **Build System**: CMake with `format-fix-single` target
- **Dependencies**: Python 3 with `uv` (handled by shebang)
- **Git**: Optional (only for `--commit` option)

---

## Troubleshooting

### Error: "Could not find project root"
**Cause**: Script cannot locate `.clang-format` file
**Solution**: Run script from project root or subdirectory

### Error: "Build directory not found"
**Cause**: CMake build directory doesn't exist
**Solution**: Run `cmake -B build` to create build directory

### Error: "Build FAILED after formatting"
**Cause**: Formatting introduced compilation errors (rare)
**Solution**:
1. Review `.orig` backup files
2. Check formatting changes with `diff file.c.orig file.c`
3. Restore from backup if needed: `cp file.c.orig file.c`

### Warning: "Build succeeded with warnings"
**Info**: Script reports warnings but continues (warnings may be pre-existing)
**Action**: Review warning output to ensure they're not formatting-related

---

## File Naming Convention

### Backup Files
- Format: `filename.c.orig`
- Location: Same directory as source file
- Cleanup: Use `batch_format.py cleanup` command

### Git Commits
- Format: `[PHASE-9.3] Format <Category> files with clang-format`
- Co-authored: Claude Code attribution
- Details: List of files and formatting changes applied

---

## Validation Workflow

1. **Format Files**: Script formats using clang-format
2. **Create Backups**: `.orig` files created automatically
3. **Verify Diff**: Script shows lines changed
4. **Build Test**: Clean build verification
5. **Warning Check**: Reports any compilation warnings
6. **Git Commit**: Optional automatic commit with standard message

---

## Performance

### Time Estimates

| Operation | Time per File | Notes |
|-----------|--------------|-------|
| Format single file | 10-15 seconds | Includes backup creation |
| Verify build | 30-60 seconds | Full clean build |
| Git commit | 5-10 seconds | File staging + commit |
| **Category total** | 30-45 minutes | 7 files average |

### Optimization Options

- **Skip verification**: Use `--no-verify` for faster execution (not recommended)
- **Batch processing**: Format multiple files in one command
- **Incremental builds**: Build system caches reduce verification time

---

## Integration with Other Tools

### CMake Targets
```bash
# Check formatting status
make format-check

# Generate formatting report
make format-report

# Manual format all files
make format-fix
```

### Manual Diff Check
```bash
# Compare formatted file with backup
diff -u file.c.orig file.c

# Show statistics
diff -u file.c.orig file.c | diffstat
```

### Git Operations
```bash
# View commit
git show HEAD

# Review changes
git diff HEAD~1

# Amend commit if needed
git commit --amend
```

---

## Reusability

This script is designed to be **reusable for other C modernization projects**:

### Adaptation Steps
1. Ensure project has `.clang-format` configuration
2. Configure CMake with `format-fix-single` target (see Phase 9.1)
3. Adjust category names for your project structure
4. Run script from project root or subdirectory

### Customization Points
- **Category names**: Modify `--name` parameter for your categories
- **Commit messages**: Edit `git_commit_changes()` function
- **Build command**: Adjust `verify_build()` for different targets
- **File patterns**: Extend to support `.h` files or other extensions

---

## Credits

**Created**: Phase 9.2-9.3 (2025-10-09)
**Author**: Claude Code automation
**License**: GPL v3 (matches project license)
**Dependencies**: Python 3, uv, clang-format, CMake, git (optional)

---

## See Also

- **Phase 9 Strategy**: `_modernization/claude/reports/PHASE_09_STRATEGY.md`
- **CMake Configuration**: `cmake/CodeFormatting.cmake`
- **Clang-Format Config**: `.clang-format` (project root)
- **Phase 9.2 TODO**: `_modernization/claude/reports/PHASE_09.02_TODO.md`
