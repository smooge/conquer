# Phase 9.2 - Pilot Formatting TODO

**Phase**: 9.2 - Pilot Formatting (3-5 source files)
**Status**: In Progress
**Created**: 2025-10-09
**Branch**: phase_9_code_formatting
**Objective**: Format pilot source files to validate workflow before mass formatting

---

## Context from Phase 9.1

**Infrastructure Status**: ✅ All validated
- clang-format configuration working
- CMake targets functional (format-fix-single FIXED)
- Backup mechanism tested
- All 8 header files already formatted in Phase 8

**Critical Watch Item**: ⚠️ Include reordering (alphabetical sorting may break compilation)

---

## Pilot File Selection (5 files)

Based on Phase 9.1 recommendations and Phase 9 strategy:

### 1. safe_system.c (System Utilities)
- **Priority**: HIGH
- **Rationale**: Recently modernized in Phase 8, good baseline test
- **Size**: Small-medium
- **Category**: System and Utilities
- **Status**: ⏳ NEXT

### 2. newhelp.c (UI and Interaction)
- **Priority**: HIGH
- **Rationale**: Already tested in Phase 9.1 backup mechanism
- **Size**: Medium
- **Category**: UI and Interaction
- **Status**: ⏳ Pending

### 3. combat.c (Core Game Logic)
- **Priority**: HIGH
- **Rationale**: Large file with many indentation warnings, critical system
- **Size**: Large
- **Category**: Core Game Logic
- **Status**: ⏳ Pending

### 4. misc.c (System Utilities)
- **Priority**: MEDIUM
- **Rationale**: Diverse code patterns, many utility functions
- **Size**: Medium-large
- **Category**: System and Utilities
- **Status**: ⏳ Pending

### 5. check.c (System Utilities)
- **Priority**: LOW
- **Rationale**: Small file for quick validation
- **Size**: Small
- **Category**: System and Utilities
- **Status**: ⏳ Pending

---

## Workflow Per File

### Standard Process (Repeat for Each File)

```bash
# 1. Format single file with CMake target
FORMAT_FILE=<filename>.c cmake --build build --target format-fix-single

# 2. Review changes (check first 100 lines)
git diff <filename>.c | head -100

# 3. Check for include order issues (CRITICAL)
grep -A 5 "^#include" <filename>.c

# 4. Compile clean build
cmake --build build --clean-first --target conqrun 2>&1 | tee build.log

# 5. Run full test suite
ctest --test-dir build --output-on-failure

# 6. Verify zero warnings at Level 9
_modernization/scripts/test_warnings.sh -w 9 -x c2x -p 4 -s 8 -n c2x -t FINAL <filename>.c

# 7. Commit if successful
git add <filename>.c
git commit -m "[PHASE-9.2] Format <filename> with clang-format

Applied automated code formatting:
- Function braces on same line
- 4-space indentation
- 96-column line limit

Testing: Build successful, 175+ tests passing

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>"

# 8. Clean backup file
rm <filename>.c.orig
```

### Edge Case Handling

**If Include Order Breaks Compilation**:
```bash
# 1. Restore from backup
git checkout <filename>.c

# 2. Add clang-format control comments
# Edit file to add:
# // clang-format off
# #include "header.h"
# #include "data.h"
# // clang-format on

# 3. Re-run formatting
FORMAT_FILE=<filename>.c cmake --build build --target format-fix-single

# 4. Re-test compilation
```

**If Unexpected Formatting Issues**:
1. Document issue in this TODO file
2. Review .clang-format configuration
3. Consider manual intervention
4. Continue with next file

---

## Success Criteria Per File

- ✅ File formatted successfully (clang-format completes without errors)
- ✅ Build succeeds (cmake --build returns 0)
- ✅ All tests passing (175+ tests, ideally 197/197)
- ✅ Zero warnings at Level 9 (test_warnings.sh reports clean)
- ✅ Git diff reviewable (whitespace-only changes)
- ✅ No include order breakage (compilation successful)

---

## Phase 9.2 Success Criteria

- ✅ All 5 pilot files formatted and committed
- ✅ Build succeeds after all formatting
- ✅ All tests pass (175+ tests minimum)
- ✅ No functionality regressions discovered
- ✅ Git commits are clean and follow standard format
- ✅ Include order issues identified and documented (if any)
- ✅ Workflow validated for Phase 9.3

---

## File Progress Tracking

### File 1: safe_system.c
- [ ] Format with clang-format
- [ ] Review diff
- [ ] Check include order
- [ ] Build clean
- [ ] Run tests
- [ ] Verify zero warnings
- [ ] Commit
- [ ] Clean backup

### File 2: newhelp.c
- [ ] Format with clang-format
- [ ] Review diff
- [ ] Check include order
- [ ] Build clean
- [ ] Run tests
- [ ] Verify zero warnings
- [ ] Commit
- [ ] Clean backup

### File 3: combat.c
- [ ] Format with clang-format
- [ ] Review diff
- [ ] Check include order
- [ ] Build clean
- [ ] Run tests
- [ ] Verify zero warnings
- [ ] Commit
- [ ] Clean backup

### File 4: misc.c
- [ ] Format with clang-format
- [ ] Review diff
- [ ] Check include order
- [ ] Build clean
- [ ] Run tests
- [ ] Verify zero warnings
- [ ] Commit
- [ ] Clean backup

### File 5: check.c
- [ ] Format with clang-format
- [ ] Review diff
- [ ] Check include order
- [ ] Build clean
- [ ] Run tests
- [ ] Verify zero warnings
- [ ] Commit
- [ ] Clean backup

---

## Issues Discovered

### Issue Log

*Record any issues discovered during pilot formatting here*

**Example Format**:
```
Issue #1: Include order breaks compilation in <file>
- Symptom: Compilation error after formatting
- Cause: clang-format alphabetized includes
- Solution: Added clang-format off/on directives
- Status: RESOLVED
```

*No issues discovered yet - Phase 9.2 just starting*

---

## Timing Estimates

**Per File Estimate**: 15-20 minutes
- Format: 2 minutes
- Review: 5 minutes
- Build/Test: 5-8 minutes
- Commit: 2-3 minutes
- Cleanup: 1 minute

**Total Phase 9.2 Estimate**: 75-100 minutes (1.5-2 hours)

**Actual Time Tracking**: *Will update as we progress*

---

## Deliverables

1. ✅ PHASE_09.02_TODO.md (this file)
2. ⏳ 5 formatted source files committed to git
3. ⏳ Phase 9.2 pilot report (brief summary in commit message or session memory)
4. ⏳ Updated workflow documentation (if issues discovered)

---

## Next Steps After Phase 9.2

**If Successful**:
- Proceed to Phase 9.3 (Core Files Formatting)
- Apply validated workflow to remaining ~23 files
- Use category-based commits (4 categories)

**If Issues Discovered**:
- Document issues thoroughly
- Adjust .clang-format configuration if needed
- Update workflow for edge cases
- Re-validate with additional pilot files if necessary

---

## Quick Reference Commands

### Format Single File
```bash
FORMAT_FILE=filename.c cmake --build build --target format-fix-single
```

### Check Includes
```bash
grep -A 5 "^#include" filename.c
```

### Build Clean
```bash
cmake --build build --clean-first --target conqrun
```

### Run Tests
```bash
ctest --test-dir build --output-on-failure
```

### Verify Warnings
```bash
_modernization/scripts/test_warnings.sh -w 9 -x c2x -p 4 -s 8 -n c2x -t FINAL filename.c
```

### Commit Template
```bash
git add filename.c
git commit -m "[PHASE-9.2] Format filename.c with clang-format

Applied automated code formatting:
- Function braces on same line
- 4-space indentation
- 96-column line limit

Testing: Build successful, tests passing

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

**Status**: Phase 9.2 started 2025-10-09
**Next Action**: Format safe_system.c (pilot file #1)
