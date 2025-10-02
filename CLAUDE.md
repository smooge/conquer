# CLAUDE.md

Legacy code modernization project guidance for `/projects/conquer-4.x/`.

**Goals**: Improve correctness, security, portability, and maintainability while preserving functionality.

**Target Platforms**: Debian Linux, Fedora Linux, macOS, FreeBSD (POSIX-compliant)

## Build and Testing Commands

**PRIMARY**: Use standardized testing script: `_modernization/scripts/test_warnings.sh -w 8 -x c2x -p 4 -s 8 -n c2x -t [BASELINE|ADMIN|GAME|UPDATE|FINAL|PROJECT] filename.c`

**FALLBACK**: Manual GCC commands if script unavailable:
```bash
# Basic strict compilation
gcc -std=c2x -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -g -O2 *.c -o program

# Intensive analysis (Level 8 equivalent)
gcc -O2 -g -Wall -Wextra -Wformat -Wformat=2 -Wstrict-prototypes -Wold-style-definition -Wold-style-declaration -Wconversion -Wimplicit-fallthrough -Wsign-conversion -fanalyzer -std=c2x -D_POSIX_C_SOURCE=200809L *.c -o program
```

**CMake**: Always use `--clean-first` for accurate warning analysis:
```bash
cmake --build build --clean-first --target conqrun 2>&1 | grep "warning:"
```

## Modernization Workflow

**Phases**: 1) Triage/Setup 2) Assessment/Planning 3) Documentation 4) Warning Elimination 5) Modern Build 6) Testing Infrastructure 7) Configuration 8) Syntactic Modernization 9) Deep Refactoring 10) Advanced Analysis

### Phase 1: Triage and Environment Setup
- Version Control: Git setup, `.gitignore` for build artifacts
- Modern Compiler: Strict warnings (`-Wall -Wextra -Wpedantic`)
- Testing Baseline: Basic regression tests

### Phase 2: Initial Assessment and Planning
**Complete before any code changes:**
1. **System Analysis** → `_modernization/claude/reports/SYSTEM_ANALYSIS.md`
2. **Compliance Assessment** → `_modernization/claude/reports/MODERNIZATION_TASKS.md`
3. **Security Analysis** → `_modernization/claude/reports/SECURITY_FIXES.md`
4. **Compilation Health** → `_modernization/claude/reports/COMPILATION_HEALTH.md`
5. **⭐ Create standardized testing script** → `_modernization/scripts/test_warnings.sh`
6. **Documentation Assessment** → `_modernization/claude/reports/DOCUMENTATION_ASSESSMENT.md`
7. **Testing Infrastructure** → `_modernization/claude/reports/TESTING_INFRASTRUCTURE.md`
8. **Project Planning** → `_modernization/claude/reports/MODERNIZATION_PLAN.md`

### Phase 3: Comprehensive Documentation
**⚠️ DOCUMENTATION ONLY - NO CODE CHANGES ⚠️**

**Workflow Steps**: 1) Check `_modernization/claude/reports/PHASE_3_FILE_STATUS.md` 2) Count functions 3) Create TODO list 4) Document one function 5) Checkpoint decision 6) Loop/save 7) File completion 8) Next file

**Priority Order**: Core System → I/O/Data → User Interface → Content/Features → Utilities

**Rules**: ONLY add documentation comments, NO code changes, fixes, or improvements until Phase 4+

### Phase 4: Warning Elimination and Compilation Health
**Reference**: `_modernization/claude/reports/PHASE_4_STRATEGY.md`

**Priority**: 1) Compilation errors 2) Missing braces 3) Format warnings 4) Implicit declarations 5) Multiple definitions

**Strategy**: Incremental progression (C99→C2x), dependency-first (headers before sources), create automation scripts

**Testing Integration**: Expand test suite by 15-25 functions during warning fixes, add regression protection

**Subphases**: 0) Infrastructure 1-8) Progressive elimination 9) Retrospective

### Phase 5: Modern Build System
**Goal**: Enhance build system with testing integration, coverage reporting, sanitizer support

**Implementation**: Analyze current structure, implement library detection, configure feature detection

### Phase 6: Testing Infrastructure Setup ✅ **COMPLETE**
**Status**: 158 tests passing (100% success rate), Unity framework operational, strategic analysis complete

**Deferred to Phase 10+**: Integration testing (6.6), Game world testing (6.7)

### Phase 7: Configuration Modernization
**Tasks**: Audit configuration files, replace hardcoded configs with automated feature detection, add configuration testing

### Phase 8: Syntactic and Mechanical Modernization
**Approach**: Test-driven modernization with utility extraction

**Tasks**: Convert legacy functions to modern prototypes, add explicit types, replace deprecated functions, add safety checks, extract 15-20 utility functions

**Automation Scripts**: Function modernization, header updates, safety checks, type modernization (use uv shebang, `--dry-run`, `--backup`)

**Goal**: 50-75 additional tests, 200+ total tests passing

### Phase 9: Deep Refactoring and Portability
**Focus**: Architecture evolution with dependency injection, pure function extraction, interface abstraction

**Improvements**: 1) Abstract system dependencies 2) Separate business logic from I/O 3) Externalize configuration 4) Add return values for error handling 5) Standardize APIs

**Goal**: 75-100 additional tests, 300+ total tests, cross-platform compatibility

### Phase 10: Advanced Analysis and Maintenance
**Tasks**: Implement deferred Phase 6 components (integration/game world testing), static/dynamic analysis, security analysis

**Analysis Tools**: AddressSanitizer, UndefinedBehaviorSanitizer, Valgrind, security scanners

**Final Goal**: 350-400 total tests, production readiness

## Common Legacy Patterns to Modernize

**Before**: K&R style, `strcpy`, implicit types
**After**: ANSI prototypes, `strncpy`, explicit types, bounds checking, error handling, comprehensive documentation

## Code Quality Standards

### Function Documentation Format
```c
/*
 * function_name - Brief description
 * Parameters: [param descriptions]
 * Returns: [return value descriptions]
 * Side Effects: [global state changes, memory allocation]
 * Testing Notes: Category A-E, approach, dependencies, complexity
 * Notes: [thread safety, performance, historical context]
 */
```

**Testing Categories**: A) Unit B) Integration C) System D) Mock E) Skip

### General Standards
- Proper prototypes in headers
- Comments explaining function purpose
- Consistent indentation (4 spaces)
- Comprehensive error checking
- Meaningful names, explicit type conversions

## Testing Strategy

**Structure**: `tests/unit/`, `tests/integration/`, `tests/regression/`, `tests/fixtures/`, `tests/scripts/`

**Naming**: `test_<module>.c`, `test_integration_<feature>.c`, `test_regression_<id>.c`

**Framework**: Unity (recommended for C), CMocka, Check, Criterion

**Requirements**:
- Preserve functionality during modernization
- Use standardized testing script for compilation
- Cross-platform testing on all target platforms
- Memory safety testing with sanitizers
- >90% coverage on critical paths

## Git Workflow

**.gitignore**: Exclude `tests/test_*` (executables), `*.o`, `*/build/`, `*.gcov`, `*.tmp`, `*~`

**Commit Format**:
```
[PHASE.SECTION] [ACTION]: [WHAT] in [WHERE]

Why: [user request/modernization goal]
Who: Claude + User
Files: [specific files changed]

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>
```

## Session Management

**Directory Structure**: `_modernization/scripts/`, `_modernization/claude/reports/`, `_modernization/memory/`

## Sub-Phase Protocol (MANDATORY)

**Process**: 1) Read session memory/determine phase 2) Review/create strategy guide 3) Identify sub-phase 4) Create TODO file 5) Execute systematically

**Files**:
- Strategy: `PHASE_NN_STRATEGY.md` (complete breakdown, objectives)
- TODOs: `PHASE_XX.YY_TODO.md` (specific tasks, completion criteria)
- Memory: `SESSION_MEMORY_[PHASE].[SECTION]_[YYYY-MM-DD]_[HHMMSS].md` (progress, decisions, next steps)

**Why Essential**: Prevents documentation gaps, ensures systematic progress, maintains continuity

**Key Files**:
- System analysis, modernization tasks, security fixes, compilation health reports
- `_modernization/claude/reports/PHASE_4_STRATEGY.md` - Phase 4 implementation guide

**Session End**: Save memory file, create session log with completed tasks, decisions, next steps

**Session Log Format**: `SESSION_LOG_[PHASE].[SECTION]_[YYYY-MM-DD]_[HHMMSS].md`

## End of Phase Protocol (MANDATORY)

**When completing any major phase (Phase 1-10), Claude must execute this standardized protocol to ensure proper project management, version control, and continuity.**

### Phase Completion Sequence

#### 1. Content Check-in ✅
- **Verify all work committed**: Ensure all modified files are committed to git
- **Check working directory**: Confirm `git status` shows clean working tree
- **Validate commits**: Review recent commits for proper formatting and completeness
- **Session documentation**: Ensure session log and memory files are saved

#### 2. Remote Repository Sync 🚀
```bash
# Push current phase branch to GitHub
git push origin [current_phase_branch]

# Verify push successful
git status
```

#### 3. GitHub Issues Review 🎫
```bash
# Check for phase-related issues
gh issue list --label "phase-[N]" --state open

# Review and update issue status
gh issue view [issue_number]
gh issue comment [issue_number] --body "Phase [N] completed. [Status update]"
```

#### 4. Branch Management Setup 🌿
```bash
# Switch to main integration branch
git checkout ai_modernization

# Verify current branch
git branch --show-current
```

#### 5. Merge Decision Point 🤔
**Claude must ask the user for merge strategy:**

*"Phase [N] is complete. Current phase branch: `[phase_branch_name]`*

*Merge options:*
*A) **Merge completed phase** into `ai_modernization` (recommended for stable phases)*
*B) **Keep separate** for additional review/testing*
*C) **Custom strategy** (please specify)*

*Which option would you prefer?"*

#### 6. Execute Merge (if approved) 🔀
```bash
# If user chooses option A - standard merge
git merge [completed_phase_branch] --no-ff

# Add merge commit message
git commit --amend -m "[PHASE-MERGE] Phase [N]: [Phase Name] → ai_modernization

Completed Phase [N] ([Phase Name]) with:
- [Key achievements]
- [Files modified/created]
- [Test coverage changes]

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>"

# Push merged changes
git push origin ai_modernization
```

#### 7. Next Phase Branch Creation 🆕
```bash
# Create new branch for next phase
git checkout -b phase_[N+1]_[next_phase_name]

# Verify new branch created
git branch --show-current

# Push new branch to establish remote tracking
git push -u origin phase_[N+1]_[next_phase_name]
```

#### 8. Next Phase Initialization (.00) 📋
**Create Phase [N+1].00 TODO planning session:**

1. **Create strategy document**: `_modernization/claude/reports/PHASE_[N+1]_STRATEGY.md`
2. **Generate TODO list**: `_modernization/claude/reports/PHASE_[N+1].00_TODO.md`
3. **Plan sub-phases**: Break down phase into manageable sub-phases
4. **Estimate effort**: Time/complexity assessment for each sub-phase
5. **Define success criteria**: Clear completion metrics for the phase

**TODO List Template Structure:**
```markdown
# Phase [N+1].00 - [Phase Name] Planning TODO

## Phase Overview
- **Objective**: [Main goal of this phase]
- **Duration Estimate**: [Expected time to complete]
- **Dependencies**: [Previous phase completions required]

## Sub-Phase Breakdown
### [N+1].1 - [Sub-phase name]
- [ ] Task 1
- [ ] Task 2
- **Estimated Effort**: [hours/complexity]

### [N+1].2 - [Sub-phase name]
- [ ] Task 1
- [ ] Task 2
- **Estimated Effort**: [hours/complexity]

## Success Criteria
- [ ] Criterion 1
- [ ] Criterion 2
- [ ] All tests passing (current count → target count)

## Risk Assessment
- **High Risk**: [potential blockers]
- **Medium Risk**: [challenges to monitor]
- **Mitigation**: [strategies to address risks]
```

### Phase Transition Verification ✓

Before completing the protocol, verify:
- [ ] All phase work committed and pushed
- [ ] GitHub issues reviewed and updated
- [ ] Merge strategy executed (if applicable)
- [ ] Next phase branch created and pushed
- [ ] Phase [N+1].00 planning documents created
- [ ] TODO list ready for next session

### Protocol Benefits

**Version Control**: Clean phase boundaries with proper branching
**Project Management**: GitHub integration with issue tracking
**Continuity**: Systematic planning prevents gaps between phases
**Collaboration**: Clear merge points for team coordination
**Documentation**: Comprehensive phase transition records

## Bug Tracking

**Primary**: GitHub Issues using standardized template:
```bash
gh issue create --title "DOC-BUG: [Brief Description]" --body "
## Bug Type: DOC-[CATEGORY] - [Logic/Security/Memory/Compatibility/Undefined]
## Severity: [Critical/High/Medium/Low]
## Phase Discovered: Phase [X]: [Phase Name] - [file:line]
## Files Affected: [list with line references]
## Description: [detailed description]
## Impact Assessment: [modernization/security impact]
## Recommended Resolution: [fix approach and priority]
"
```

**Categories**: DOC-LOGIC, DOC-SECURITY, DOC-MEMORY, DOC-COMPATIBILITY, DOC-UNDEFINED, WARN-COMPILE, WARN-STATIC, MOD-REGRESSION, TEST-FAILURE

**Fallback**: `PERSISTENT_BUGS.md` when GitHub unavailable

## Script-Driven Modernization Benefits

**Automation**: Consistency, speed, auditability, reversibility, reusability, documentation patterns

**Quality**: `--dry-run` preview, `--backup` validation, logging, idempotent, built-in checks

**Efficiency**: Batch processing, 80-90% time savings, focus on analysis, reduced errors, faster iterations

## Critical Lessons Learned

### **⭐ Essential Infrastructure: Standardized Testing Script (Phase 2)**

**Lesson**: Creating a standardized warning analysis script during Phase 2 is **ESSENTIAL** for successful modernization.

**Problem Discovered**: Manual gcc compilation commands with complex flag sets lead to:
- **Flag Inconsistencies**: Different sessions using slightly different warning flags
- **Manual Errors**: Typos in complex gcc command lines causing compilation failures
- **Time Waste**: Debugging compilation issues instead of focusing on modernization
- **Progress Loss**: Inconsistent testing makes it hard to track actual warning reduction progress

**Solution Implemented**: Create `_modernization/scripts/test_warnings.sh` (or similar) during Phase 2 that:
- **Standardizes flags**: Consistent warning levels and compilation settings
- **Automates reporting**: Results saved to timestamped files for progress tracking
- **Supports multiple modes**: Admin-only, game-mode, and dual-compiled file testing
- **Enables progress tracking**: Baseline, update, and final verification testing
- **Prevents errors**: Eliminates manual flag entry and typos

**Implementation Requirements**:
```bash
# Example usage that should be established in Phase 2
_modernization/scripts/test_warnings.sh -w 8 -x c2x -p 4 -s 8 -n c2x -t BASELINE filename.c
_modernization/scripts/test_warnings.sh -w 8 -x c2x -p 4 -s 8 -n c2x -t FINAL filename.c
```

**Impact**: This single infrastructure investment in Phase 2 **eliminates entire categories of problems** in Phases 4-10 and dramatically improves modernization efficiency and reliability.

**Recommendation**: Every modernization project should create this infrastructure during initial assessment, not discover the need during active modernization work.

## Migration Notes

- Always backup original code before modernization
- **Complete the Initial Assessment and Planning phase before making any code changes**
- Modernize incrementally, not all at once
- Test frequently during the modernization process
- Document any behavioral changes from the original code
- Maintain compatibility with existing interfaces where possible
- **Create git commits after each completed work session using the format above**
- **Export conversation before compaction when context is below 4%**
- Refer to analysis files for guidance throughout the project

## Important Instruction Reminders

Do what has been asked; nothing more, nothing less.
NEVER create files unless they're absolutely necessary for achieving your goal.
ALWAYS prefer editing an existing file to creating a new one.
NEVER proactively create documentation files (*.md) or README files. Only create documentation files if explicitly requested by the User.
