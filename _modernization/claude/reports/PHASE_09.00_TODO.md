# Phase 9.00 - Code Formatting Standardization: Planning TODO

**Phase**: 9.00 - Planning and Strategy
**Status**: ✅ COMPLETE
**Created**: 2025-10-07
**Objective**: Establish comprehensive formatting strategy and infrastructure

---

## Phase Overview

**Main Goal**: Prepare infrastructure and strategy for formatting entire codebase with clang-format

**Scope**: 28 files requiring formatting (out of 45 .c + 14 .h files)

**Expected Impact**:
- Eliminate 69 indentation warnings (39% of total clang-tidy warnings)
- Improve code readability and maintainability
- Reduce 177 total warnings → ~108 warnings
- Establish formatting standard for future development

---

## Planning Tasks

### 1. Analysis and Assessment ✅ COMPLETE

- [x] Analyze existing `.indent.pro` configuration
- [x] Understand current formatting rules and limitations
- [x] Identify GNU indent limitations (function braces)
- [x] Count files needing formatting (Result: 28 files)
- [x] Categorize clang-tidy warnings (Result: 69 indentation, 177 total)
- [x] Assess impact on codebase (Result: 39% warning reduction possible)

**Completion Date**: 2025-10-07

---

### 2. Tool Selection and Configuration ✅ COMPLETE

- [x] Evaluate formatting tool options (GNU indent vs clang-format)
- [x] **Decision**: Use clang-format as primary tool
- [x] Update `.indent.pro` configuration for modern standards
  - [x] Remove column 40 alignment
  - [x] Remove column 64 comment pushing
  - [x] Document GNU indent limitations
- [x] Create `.clang-format` configuration file
  - [x] Function braces on same line: `int foo(){`
  - [x] Control flow braces on same line: `if(){`, `for(){`
  - [x] Cuddle else/while: `} else {`, `} while()`
  - [x] 4-space indentation, no tabs
  - [x] 96-column line limit
  - [x] Minimal alignment (no forced columns)
- [x] Test clang-format on sample file (combat.c)
- [x] Verify clang-format configuration works correctly
- [x] Fix duplicate key error (AlignTrailingComments)

**Completion Date**: 2025-10-07

---

### 3. CMake Integration ✅ COMPLETE

- [x] Create `cmake/CodeFormatting.cmake` module
- [x] Implement formatting targets:
  - [x] `format-check` - Audit without changes
  - [x] `format-check-verbose` - Detailed diffs
  - [x] `format-fix` - Apply formatting with backups
  - [x] `format-fix-single` - Format single file
  - [x] `format-clean-backups` - Remove .orig files
  - [x] `format-report` - Statistics report
- [x] Integrate module into `CMakeLists.txt`
- [x] Test CMake reconfiguration
- [x] Verify all targets are available

**Completion Date**: 2025-10-07

---

### 4. File Categorization ✅ COMPLETE

**Files Needing Formatting (28 total)**:

**Category 1: Core Game Logic** (8 files)
- [x] Identify: combat.c, commands.c, cexecute.c, move.c
- [x] Identify: navy.c, npc.c, randeven.c, update.c

**Category 2: I/O and Data Management** (7 files)
- [x] Identify: io.c, data.c, display.c, forms.c
- [x] Identify: reports.c, trade.c, sort.c

**Category 3: System and Utilities** (8 files)
- [x] Identify: admin.c, misc.c, safe_system.c, check.c
- [x] Identify: m2alloc.c, magic.c, makeworl.c, newlogin.c

**Category 4: UI and Interaction** (5 files)
- [x] Identify: extcmds.c, main.c, newhelp.c, psmap.c, spew.c

**Completion Date**: 2025-10-07

---

### 5. Strategy Documentation ✅ COMPLETE

- [x] Create `PHASE_09_STRATEGY.md` comprehensive plan
  - [x] Executive summary
  - [x] Sub-phase breakdown (9.1 - 9.6)
  - [x] Detailed tasks for each sub-phase
  - [x] Risk assessment
  - [x] Success metrics
  - [x] Timeline and milestones
  - [x] Rollback strategy
- [x] Create `PHASE_09.00_TODO.md` (this file)
- [x] Document key decisions made
- [x] Document questions for user

**Completion Date**: 2025-10-07

---

### 6. Phase Renumbering Planning ✅ COMPLETE

- [x] Plan new phase structure:
  - Phase 9: Code Formatting (NEW)
  - Phase 10: Deep Refactoring (was Phase 9)
  - Phase 11: Advanced Analysis (was Phase 10)
- [x] Identify documents needing updates:
  - CLAUDE.md (both /projects/ and /projects/conquer-4.x/)
  - Any existing Phase 9/10 documentation
  - Session memory files
- [x] Create update plan for CLAUDE.md

**Completion Date**: 2025-10-07

---

## Key Decisions Made

### Decision 1: Primary Formatting Tool
**Choice**: clang-format (not GNU indent)
**Rationale**:
- Modern industry standard
- Supports function braces on same line
- Better IDE/editor integration
- More precise control over formatting rules
- Same ecosystem as clang-tidy

### Decision 2: Brace Style
**Choice**: Same-line braces for functions and control flow
**Rationale**:
- Reduces line count (easier reading)
- Less scrolling for AI assistants
- Improves visual parsing of code structure
- Modern standard (K&R is legacy)

### Decision 3: Indentation
**Choice**: 4 spaces, no tabs
**Rationale**:
- Consistent with existing .indent.pro
- Modern standard
- Better cross-platform compatibility
- No editor-specific tab rendering issues

### Decision 4: Alignment
**Choice**: Minimal alignment (no forced columns)
**Rationale**:
- Eliminates excessive whitespace
- More compact code
- Easier to read and maintain
- Comments stay near code

### Decision 5: Commit Strategy
**Choice**: Category-based commits (4 commits recommended)
**Rationale**:
- Balance between granularity and reviewability
- Logical grouping by functional area
- Rollback possible at category level
- Not too many commits to overwhelm history

### Decision 6: Phase Creation
**Choice**: Create dedicated Phase 9 for formatting
**Rationale**:
- Scope too large for sub-phase (28 files, thousands of lines)
- Needs careful git review and commit strategy
- Formatting is foundational for future phases
- Deserves dedicated focus and documentation

---

## Next Steps

### Immediate Actions (User Decision Required)

**Questions for User**:
1. **Phase Execution**: Proceed with Phase 9.1 now or defer?
2. **Commit Strategy**: Approve category-based commits (4 commits)?
3. **Branch Strategy**: Format on current branch or create dedicated branch?
4. **Review Process**: Review after each category or all at once?
5. **CLAUDE.md Updates**: Update phase numbering now or after Phase 9 complete?

### Recommended Next Session

**Phase 9.1: Infrastructure Validation**
- Test formatting on sample files
- Verify CMake targets work correctly
- Validate backup/restore mechanism
- Confirm no compilation issues
- Document any edge cases

**Estimated Time**: 1-2 hours

---

## Success Criteria for Phase 9.00

- [x] ✅ Formatting tools evaluated and selected
- [x] ✅ `.clang-format` configuration created and tested
- [x] ✅ CMake integration complete
- [x] ✅ All 28 files identified and categorized
- [x] ✅ Comprehensive strategy document created
- [x] ✅ Risk assessment completed
- [x] ✅ Timeline established
- [x] ✅ User questions documented

**Status**: Phase 9.00 ✅ COMPLETE

---

## Files Created/Modified in Phase 9.00

### Created
- `_modernization/claude/reports/PHASE_09_STRATEGY.md` - Complete phase plan
- `_modernization/claude/reports/PHASE_09.00_TODO.md` - This file
- `cmake/CodeFormatting.cmake` - CMake formatting module
- `.clang-format` - Clang-format configuration

### Modified
- `.indent.pro` - Updated for modern standards
- `CMakeLists.txt` - Integrated CodeFormatting module

### To Be Created (Phase 9.6)
- `_modernization/claude/reports/FORMATTING_GUIDELINES.md` - Formatting standards doc
- Updated CLAUDE.md files with new phase numbering

---

## Metrics Summary

| Metric | Value |
|--------|-------|
| Files needing formatting | 28 |
| Total C source files | 45 |
| Total C header files | 14 |
| Total lines of code | 45,865 |
| Current indentation warnings | 69 |
| Total clang-tidy warnings | 177 |
| Target indentation warnings | 0 |
| Target total warnings | ~108 |
| Expected warning reduction | 39% |

---

## References

- **Main Strategy**: `_modernization/claude/reports/PHASE_09_STRATEGY.md`
- **Formatting Config**: `.clang-format`
- **CMake Module**: `cmake/CodeFormatting.cmake`
- **Legacy Config**: `.indent.pro`

---

**Phase Status**: ✅ COMPLETE
**Ready for**: Phase 9.1 - Infrastructure Validation
**Awaiting**: User decision on execution strategy
**Created**: 2025-10-07
**Completed**: 2025-10-07
