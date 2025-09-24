# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with legacy code modernization projects.

## CRITICAL PATH REQUIREMENTS

**ALWAYS USE CORRECT PROJECT PATH:**
- **Correct path**: `/home/ssmoogen/conquer-project/conquer-4.x/`
- **NEVER use**: `/home/ssmoogan/` (common typo - incorrect spelling)
- **ALWAYS verify**: All file paths start with `/home/ssmoogen/` (double-o, not single-o)
- **Apply to**: All file operations, session memory files, documentation paths

## Project Overview

This is a legacy code modernization project focused on upgrading legacy codebases to modern standards. The primary goals are to improve **correctness, security, portability, and maintainability** for use on today's systems while preserving functionality.

## Target Platforms

**Supported Operating Systems:**
- Debian Linux
- Fedora Linux
- macOS
- FreeBSD

**Compliance Requirements:**
- Code must be POSIX-compliant where applicable
- Use portable system calls and library functions
- Avoid platform-specific extensions unless absolutely necessary

## Build and Testing Commands

**⚠️ CRITICAL**: Always use the standardized testing script first. Only fall back to manual gcc commands if the script is unavailable.

### **PRIMARY METHOD: Standardized Warning Analysis Script**

```bash
# PREFERRED: Use standardized warning analysis script for all testing
# This ensures consistent flags and proper reporting across all sessions

# Phase 4 intensive warning analysis (Level 8 warnings - recommended for modernization)
_modernization/scripts/test_warnings.sh -w 8 -x c2x -p 4 -s 8 -n c2x -t BASELINE filename.c

# Admin-only file testing
_modernization/scripts/test_warnings.sh -w 8 -x c2x -p 4 -s 8 -n c2x -t ADMIN filename.c

# Game-mode file testing (for dual-compiled files)
_modernization/scripts/test_warnings.sh -w 8 -x c2x -p 4 -s 8 -n c2x -t GAME filename.c

# Progress update testing
_modernization/scripts/test_warnings.sh -w 8 -x c2x -p 4 -s 8 -n c2x -t UPDATE filename.c

# Final verification testing
_modernization/scripts/test_warnings.sh -w 8 -x c2x -p 4 -s 8 -n c2x -t FINAL filename.c

# Test all files in project (comprehensive analysis)
_modernization/scripts/test_warnings.sh -w 8 -x c2x -p 4 -s 8 -n c2x -t PROJECT
```

**Script Benefits:**
- **Consistent Flags**: Standardized warning levels and C standard enforcement
- **Automated Reporting**: Results saved to `_modernization/claude/scratch/` for tracking
- **Mode Testing**: Supports admin-only, game-mode, and dual-compiled file testing
- **Progress Tracking**: Different test types for different phases of work

### **FALLBACK METHOD: Manual GCC Commands** (Use only if script unavailable)

```bash
# Basic compilation with strict warnings
gcc -std=c2x -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -g -O2 *.c -o program

# Build with additional safety flags
gcc -std=c2x -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -Werror -g -O2 -fsanitize=address -fsanitize=undefined *.c -o program

# INTENSIVE ANALYSIS - Comprehensive warning detection (Level 8 equivalent)
gcc -O2 -g -Wall -Wextra -Wformat -Wformat=2 -Wstrict-prototypes -Wold-style-definition -Wold-style-declaration -Wconversion -Wimplicit-fallthrough -Wsign-conversion -fanalyzer -std=c2x -D_POSIX_C_SOURCE=200809L *.c -o program

# INTENSIVE ANALYSIS - Single file testing (admin mode)
gcc -O2 -g -Wall -Wextra -Wformat -Wformat=2 -Wstrict-prototypes -Wold-style-definition -Wold-style-declaration -Wconversion -Wimplicit-fallthrough -Wsign-conversion -fanalyzer -std=c2x -D_POSIX_C_SOURCE=200809L -DADMIN -c filename.c

# INTENSIVE ANALYSIS - Single file testing (game mode)
gcc -O2 -g -Wall -Wextra -Wformat -Wformat=2 -Wstrict-prototypes -Wold-style-definition -Wold-style-declaration -Wconversion -Wimplicit-fallthrough -Wsign-conversion -fanalyzer -std=c2x -D_POSIX_C_SOURCE=200809L -c filename.c
```

### **Other Analysis Tools**

```bash
# Static analysis with clang
clang --analyze -std=c2x -D_POSIX_C_SOURCE=200809L -Wall -Wextra *.c

# Static analysis with clang-tidy (modernization and security focus)
clang-tidy filename.c -checks=clang-analyzer-security*,clang-analyzer-core* -- -std=c2x -D_POSIX_C_SOURCE=200809L

# Format code (if clang-format is available)
clang-format -i *.c *.h

# Python usage note: Always use python3 explicitly
python3 script.py  # Correct
python script.py   # May fail - don't use
```

## Modernization Workflow

**Phase Sequence:**
1. **Phase 1**: Triage and Environment Setup
2. **Phase 2**: Initial Assessment and Planning
3. **Phase 3**: Comprehensive Documentation
4. **Phase 4**: Warning Elimination and Compilation Health
5. **Phase 5**: Modern Build System
6. **Phase 6**: Testing Infrastructure Setup
7. **Phase 7**: Configuration Modernization
8. **Phase 8**: Syntactic and Mechanical Modernization
9. **Phase 9**: Deep Refactoring and Portability
10. **Phase 10**: Advanced Analysis and Maintenance

### Phase 1: Triage and Environment Setup 🛡️

Before changing a single line of code, establishing a modern, strict, and controlled environment is critical.

1. **Version Control**: Place codebase under Git, create `.gitignore` for build artifacts
2. **Modern Compiler**: Use strict warnings (`-Wall -Wextra -Wpedantic`)
3. **Testing Baseline**: Create basic regression tests before refactoring

### Phase 2: Initial Assessment and Planning (Complete Before Starting Code Changes)

1. **System Analysis**:
   - Scan the entire codebase to understand what the initial system is and does
   - Document system architecture, data flow, and key functionality
   - Save findings to `_modernization/claude/reports/SYSTEM_ANALYSIS.md` for future Claude sessions

2. **Compliance Assessment**:
   - Scan code for all modernizations needed to make it standards compliant
   - Identify deprecated functions, implicit declarations, unsafe practices
   - Save detailed findings and modernization tasks to `_modernization/claude/reports/MODERNIZATION_TASKS.md`

3. **Security Analysis**:
   - Scan code for security problems that need fixing
   - Identify buffer overflows, unsafe string operations, memory leaks
   - Save security issues and remediation plans to `_modernization/claude/reports/SECURITY_FIXES.md`

4. **Compilation Health Assessment** (CRITICAL):
   - Test compilation with strict warning flags
   - Count and categorize all compilation warnings
   - Identify blocking compilation errors that prevent testing
   - Save findings to `_modernization/claude/reports/COMPILATION_HEALTH.md`

5. **Documentation Assessment**:
   - Analyze current state of code documentation across all source files
   - Evaluate documentation quality, coverage, and consistency
   - Create file prioritization strategy based on complexity and importance
   - Save comprehensive findings and strategy to `_modernization/claude/reports/DOCUMENTATION_ASSESSMENT.md`

6. **Testing Infrastructure Analysis**:
   - Analyze existing tests and testing frameworks in the codebase
   - Identify test coverage gaps and recommend appropriate testing infrastructure
   - Design test directory structure to keep tests separate from source code
   - Save detailed findings and testing strategy to `_modernization/claude/reports/TESTING_INFRASTRUCTURE.md`

7. **Project Planning**:
   - Create a comprehensive modernization plan with estimated time to complete
   - Prioritize tasks based on risk and complexity
   - Save plan to `_modernization/claude/reports/MODERNIZATION_PLAN.md`

### Phase 3: Comprehensive Documentation 📝

**CRITICAL: Document Functions Before Modernization**

**⚠️ DOCUMENTATION ONLY - NO CODE CHANGES ALLOWED ⚠️**
- **Phase 3 is STRICTLY documentation only**
- **NO code modifications, fixes, or improvements**
- **NO changing function signatures, logic, or structure**
- **ONLY add new documentation comments above functions**
- **ONLY clarify or improve existing comments if unclear**
- **All code changes deferred to Phase 4 and later**

**SYSTEMATIC DOCUMENTATION PROCESS** - Follow this exact workflow for large files to prevent context loss:

#### Documentation Workflow Steps (MANDATORY)
1. **Check Status File**: Read `_modernization/claude/reports/PHASE_3_FILE_STATUS.md` for current progress
2. **Count Functions**: Determine total number of routines in the target file
3. **Create Todo List**: Generate function-by-function todo list for session tracking
4. **Document One Function**: Work on one function at a time with full documentation
5. **Checkpoint Decision**: After each function, ask user: continue or save session?
6. **Loop Until Complete**: Continue until all functions documented or session saved
7. **File Completion**: When file complete, update status file and create git commit
8. **Next File Decision**: Ask user to proceed to next file or save session

#### Context Management for Large Files
- **Todo List**: Essential for tracking progress across functions and sessions
- **Session Checkpoints**: Stop after each function to prevent context overflow
- **Status Tracking**: Keep `PHASE_3_FILE_STATUS.md` updated continuously
- **Git Commits**: Immediate commit when file documentation complete
- **Memory Files**: Save session state if stopping mid-file

#### Error Recovery
- **Session Crashes**: Todo list preserves progress if session history lost
- **Context Limits**: Checkpointing prevents running out of context
- **Resume Capability**: Status file enables seamless session continuation

**One File Per Session Approach** - document one complete file per session, commit immediately

**File Priority Order:**
- **Priority 1**: Core System (main files, data structures, core algorithms)
- **Priority 2**: I/O and Data Management (input/output, data handling)
- **Priority 3**: User Interface (display, user interaction)
- **Priority 4**: Content/Features (business logic, feature implementations)
- **Priority 5**: Utilities and Support Files

**Documentation Requirements**: Analyze each function's purpose, parameters, returns, side effects. Use standard format from [Code Quality Standards](#code-quality-standards). Document before modernization to preserve knowledge and enable safe refactoring.

**⚠️ PHASE 3 STRICT RULES ⚠️**
- **DOCUMENTATION ONLY**: Add only function documentation comments
- **NO CODE CHANGES**: Do not modify any existing code, logic, or structure
- **NO BUG FIXES**: Do not fix obvious bugs or issues found
- **NO IMPROVEMENTS**: Do not optimize or modernize any code
- **NO SIGNATURE CHANGES**: Do not modify function parameters or return types
- **PRESERVE EVERYTHING**: All code must remain exactly as written
- **DEFER ALL CHANGES**: All code modifications wait until Phase 4+

### Phase 4: Warning Elimination and Compilation Health 🚨

**CRITICAL: This phase is required before any testing or build system work can proceed.**

**Essential for clean compilation needed by testing frameworks.**

**⚠️ MANDATORY: Use Comprehensive Phase 4 Strategy Guide ⚠️**
- **PRIMARY REFERENCE**: `_modernization/claude/reports/PHASE_4_STRATEGY.md`
- **ALWAYS consult this detailed strategy guide before beginning Phase 4 work**
- **Contains complete subphase breakdown, session management, and automation scripts**
- **Includes retrospective methodology for multi-codebase knowledge capture**

**Warning Elimination Priority**:
1. Compilation errors
2. Missing braces (data structure initialization)
3. Format warnings (sprintf/printf mismatches)
4. Implicit declarations
5. Multiple definitions

**Safety Improvements**: Replace sprintf with snprintf, add missing includes

**Key Strategy Elements** (see full strategy guide for details):
- **Incremental Progression**: C99 first, then C2x to manage warning explosion
- **Dependency-First**: Fix header.h before source files
- **Context Management**: Break large files into manageable chunks per session
- **Tool Synergy**: GCC primary, Clang for clarification, clang-tidy for automation
- **Session Management**: Warning-type sessions for high warning count files
- **Automation Required**: Create analysis and tracking scripts in Subphase 0
- **Bug Tracking Strategy**: GitHub Issues for compilation errors ONLY, local tracking for warnings
- **Retrospective**: Capture lessons learned for future codebase modernizations

**Phase 4 Subphases** (see strategy guide for full details):
- **Subphase 0**: Baseline Assessment & Infrastructure (MANDATORY FIRST)
- **Subphase 1-8**: Progressive warning elimination with increasing strictness
- **Subphase 9**: Retrospective and Knowledge Capture for future codebases

**Completion Criteria**: All source files compile with zero warnings using strict flags

### Phase 5: Modern Build System 🛠️

**CRITICAL: Establish modern build system before testing infrastructure**

Replace legacy build files with modern build system to enable proper testing integration, cross-platform compatibility, and modern development workflows.

**Implementation**:
- Analyze current build structure
- Create modern build configuration with current standards
- Implement library detection
- Configure feature detection to replace hardcoded configurations
- Prepare test integration

### Phase 6: Testing Infrastructure Setup 🧪

**CRITICAL: Establish comprehensive testing framework after build system modernization**

With modern build system in place, establish robust testing infrastructure to ensure that remaining modernization preserves all original functionality.

**Testing Framework Selection**: Choose appropriate testing framework for the language
- For C: Unity (lightweight, C89 compatible)
- For C++: Google Test or Catch2
- For Python: pytest
- For JavaScript: Jest or Mocha

**Test Structure**: `tests/` with subdirectories for unit, integration, regression, security, performance

**Implementation**: Baseline tests for current behavior, automated test runners, coverage reporting, build system integration

### Phase 7: Configuration Modernization 🧐

**Audit**: Review configuration files and build options, document dependencies
**Feature Detection**: Replace hardcoded configurations with automated feature detection

### Phase 8: Syntactic and Mechanical Modernization ⚙️

**IMPORTANT: Create Automation Scripts**

Create automation scripts for repetitive tasks to save time and ensure consistency across sessions.

**Automation Scripts** (language-specific):
- Function prototype modernization
- Header/import modernization
- Safety checks addition
- Type modernization
- Test template generation

**Script Guidelines**: Use uv shebang format for Python, make idempotent, include `--dry-run` and `--backup` options, log changes

**Modernization Tasks**:
- Convert legacy function styles to modern prototypes
- Add explicit types and appropriate qualifiers
- Replace deprecated functions with modern equivalents
- Add safety checks and error handling
- Use appropriate types for indices and sizes
- Consider modern language features where beneficial

### Phase 9: Deep Refactoring and Portability 🧠

**Portability Issues**: Address platform-specific assumptions and legacy practices that don't work on modern systems.

**Decision Framework for Types and APIs**:
1. **For counting or general arithmetic**: Use appropriate native types
2. **When exact specifications are essential**: Use standardized types
3. **For memory/object operations**: Use appropriate size types
4. **For platform interfaces**: Use proper interface types

**Updating I/O and Formatting**: Use modern format specifiers and safe alternatives

### Phase 10: Advanced Analysis and Maintenance 🔬

**Static Analysis**: Use multiple analysis tools for comprehensive code quality assessment

**Dynamic Analysis**: Use runtime analysis tools:
- **Memory Error Detection**: AddressSanitizer, Valgrind
- **Undefined Behavior Detection**: UndefinedBehaviorSanitizer
- **Security Analysis**: Security-focused static analysis tools

## Common Legacy Patterns to Modernize

### Before (Legacy):
```c
/* Old style function */
int process(data, size)
char *data;
int size;
{
    char buffer[100];
    strcpy(buffer, data);
    return size;
}
```

### After (Modern):
```c
/*
 * process - Copy data to internal buffer with bounds checking
 *
 * Copies the input data to a local buffer ensuring no buffer overflow.
 * Validates input parameters and ensures null termination.
 *
 * Parameters:
 *   data - Input string to process (must not be NULL)
 *   size - Size of data to process
 *
 * Returns:
 *   Size of processed data on success, -1 on error
 */
int process(const char *data, size_t size) {
    if (data == NULL || size == 0) {
        return -1;
    }

    char buffer[100];
    if (size >= sizeof(buffer)) {
        return -1;
    }

    strncpy(buffer, data, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    return (int)size;
}
```

## Code Quality Standards

### Function Documentation Requirements

All functions must be documented before modernization using this standard format:

```c
/*
 * function_name - Brief one-line description
 *
 * Detailed description explaining the function's purpose,
 * algorithm, and any important implementation details.
 *
 * Parameters:
 *   param1 - Description of first parameter (constraints, valid ranges)
 *   param2 - Description of second parameter (must not be NULL)
 *
 * Returns:
 *   Description of return value and meaning of different return codes
 *   NULL on error, valid pointer on success
 *   -1 on failure, 0 on success, positive value for count/size
 *
 * Side Effects:
 *   - Modifies global state if applicable
 *   - Allocates memory that caller must free
 *   - May block on I/O operations
 *
 * Testing Notes:
 *   Category: A (Unit) | B (Integration) | C (System) | D (Mock) | E (Skip)
 *   Approach: [Unit tests with mocks | Integration testing | System testing]
 *   Key Tests: [List of critical test scenarios]
 *   Dependencies: [Global variables, initialization requirements]
 *   Mock Requirements: [What needs to be mocked for testing]
 *   Complexity: [Simple | Moderate | Complex] - [suitability assessment]
 *
 * Notes:
 *   - Thread safety information
 *   - Performance considerations
 *   - Historical context if relevant
 */
```

### Testing Notes Guidelines

**MANDATORY**: All function documentation must include Testing Notes section to prevent retesting attempts.

**Category Classification**:
- **Category A (Unit Testable)**: Isolated functions, minimal dependencies
- **Category B (Integration Required)**: Requires system state or multiple modules
- **Category C (System Level Only)**: Requires full system initialization
- **Category D (Mock Intensive)**: Testable with extensive mocking
- **Category E (Deferred/Skip)**: Skip until post-modernization

### Special Documentation for Legacy Code
- **Unclear Logic**: Document confusing or non-obvious code sections
- **Magic Numbers**: Explain the meaning of hardcoded constants
- **Workarounds**: Document any platform-specific hacks or workarounds
- **Assumptions**: Note assumptions about input data, system state, etc.
- **Historical Context**: Preserve information about why code was written this way

### General Code Standards
- All functions must have proper prototypes in header files
- **All routines must have comments at the beginning explaining what the code does**
- Use consistent indentation (4 spaces recommended)
- Add comprehensive error checking
- Document complex algorithms and data structures
- Prefer explicit over implicit type conversions
- Use meaningful variable and function names

## Testing Strategy

### Test Directory Structure

**All tests must be organized in dedicated directories separate from source code:**

- `tests/` - Main test directory containing all test files
- `tests/unit/` - Unit tests for individual functions and modules
- `tests/integration/` - Integration tests for component interactions
- `tests/regression/` - Regression tests to ensure modernization preserves functionality
- `tests/fixtures/` - Test data files and mock inputs
- `tests/scripts/` - Test runner scripts and utilities

**Test File Naming Conventions:**
- Unit tests: `test_<module_name>.<ext>` (e.g., `test_string_utils.c`)
- Integration tests: `test_integration_<feature>.<ext>`
- Regression tests: `test_regression_<issue_id>.<ext>`

### Testing Framework Selection

**Choose appropriate frameworks based on language:**

**C Testing Frameworks:**
1. **Unity** - Lightweight, portable, C89 compatible
2. **CMocka** - Modern unit testing framework with mocking
3. **Check** - GNU Autotools compatible
4. **Criterion** - Modern C/C++ testing framework

**Other Languages:**
- **C++**: Google Test, Catch2
- **Python**: pytest, unittest
- **JavaScript**: Jest, Mocha
- **Java**: JUnit, TestNG
- **Rust**: Built-in test framework

### Testing Requirements

- **Preserve original functionality during modernization**
- **Test each modernized component thoroughly**
- **Create regression tests before making changes**
- **Use standardized warning analysis**: Always use `_modernization/scripts/test_warnings.sh` for compilation testing
- Use compiler warnings as early error detection
- Add unit tests for all critical functions
- Test with different compiler versions and flags
- **Cross-platform testing required**: Verify functionality on all target platforms
- Test compliance using portable system calls
- **Memory safety testing**: Use appropriate sanitizers and analysis tools
- **Code coverage analysis**: Aim for >90% coverage on critical paths

### Test Build Integration

**Build System Integration Example:**
```cmake
# Enable testing
enable_testing()

# Add test directory
add_subdirectory(tests)

# Create test executables
add_executable(test_module tests/unit/test_module.c src/module.c)
target_link_libraries(test_module testing_framework)

# Add tests to test runner
add_test(NAME module_test COMMAND test_module)
```

## Git Workflow

### Build Artifacts and .gitignore Management

**IMPORTANT**: When creating executable files during development (test binaries, compiled programs), always add them to `.gitignore` to prevent accidental commits.

**Common build artifacts to exclude:**
- Test executables: `tests/test_*` (without source extension)
- Compiled object files: `*.o`
- Build directories: `*/build/`, `*/target/`
- Temporary files: `*.tmp`, `*~`
- Coverage files: `*.gcov`, `*.gcda`, `*.gcno`

**Example .gitignore entries:**
```
# Compiled test executables
tests/test_*
!tests/test_*.c
!tests/test_*.h

# Build artifacts
*.o
*/build/
*.gcov
*.gcda
*.gcno
*.tmp
*~

# Platform specific
.DS_Store
Thumbs.db
```

### Commit Guidelines

When completing a set of work, Claude must create a git commit with a comprehensive commit message:

**Commit Message Format:**
```
Brief summary of changes

Prompt: [Description of what the user requested]

Work Completed:
- [List of specific changes made]
- [Include files modified, functions updated, etc.]
- [Note any modernization patterns applied]

Generated by Claude
Co-Authored-By: Claude <noreply@anthropic.com>
```

## Session Management

### Project Directory Structure
At the beginning of each project, Claude must create a `_modernization/` directory structure:
1. Create `_modernization/` directory in the project root
2. Create `_modernization/scripts/` subdirectory for automation scripts created during sessions
3. Create `_modernization/claude/reports/` subdirectory for analysis and planning documents
4. Create `_modernization/memory/` subdirectory for session progress snapshots
5. **All automation scripts created by Claude sessions must be stored in `_modernization/scripts/` and added to git**
6. **Scripts must be immediately useful and follow the standardized naming convention**

### Memory File Naming Convention

**Session Memory Files** (Saved at end of EVERY session):
- Format: `SESSION_MEMORY_YYYY-MM-DD_HHMMSS.md`
- Location: `_modernization/memory/`
- Content: Current progress, key decisions, next steps

**Analysis Documents** (Created during assessment phases):
- `_modernization/claude/reports/SYSTEM_ANALYSIS.md`
- `_modernization/claude/reports/MODERNIZATION_TASKS.md`
- `_modernization/claude/reports/SECURITY_FIXES.md`
- `_modernization/claude/reports/COMPILATION_HEALTH.md`
- `_modernization/claude/reports/DOCUMENTATION_ASSESSMENT.md`
- `_modernization/claude/reports/TESTING_INFRASTRUCTURE.md`
- `_modernization/claude/reports/MODERNIZATION_PLAN.md`

**Phase-Specific Strategy Documents** (MANDATORY references for each phase):
- `_modernization/claude/reports/PHASE_4_STRATEGY.md` - Comprehensive Phase 4 implementation guide
- `_modernization/claude/reports/PHASE_4_PLAN_REVIEW.md` - Phase 4 planning analysis and feedback

### Session End Management
When the user indicates it's time to end a session, Claude must:
1. Save current progress to a memory file in `_modernization/memory/` with an appropriate descriptive name
2. Include completed tasks, current status, and next steps
3. Document any important decisions or discoveries made during the session
4. Ensure the memory file provides sufficient context for future sessions

### Session Log Creation Process

**When the user formally quits a session**, Claude must automatically execute this sequence:

#### 1. Work Completion Summary
Create a comprehensive session summary including:
- **Tasks Completed**: List all completed work items
- **Files Created/Modified**: Document all file changes made
- **Git Commits**: List all commits made during session
- **Key Decisions**: Important technical decisions and rationale
- **Next Steps**: Recommendations for subsequent sessions
- **Blockers/Issues**: Any unresolved problems or concerns

#### 2. Session Log Export
Export the conversation to a structured file named `SESSION_LOG_[YYYYMMDD]_[HHMMSS].md` to the directory `_modernization/claude/reports/` with format:

```markdown
# Claude Code Session Log

**Session Date**: [YYYY-MM-DD]
**Session Duration**: [Wall time]
**Code Changes**: [Lines added/removed]

## Session Objectives
[What was the user trying to accomplish]

## Work Completed
### Files Created
- `filename.ext` - Description of purpose and content

### Files Modified
- `filename.ext` - Description of changes made

### Git Commits
- `commit_hash` - Commit message summary

## Key Technical Decisions
[Important architectural or implementation decisions made]

## Testing/Validation Performed
[Any testing, verification, or validation completed]

## Session Outcomes
### Successful Completions
[Tasks that were fully completed]

### Partial Progress
[Tasks that were started but not finished]

### Deferred Items
[Tasks identified but deferred to future sessions]

## Recommendations for Next Session
[Specific next steps and priorities]

## Issues/Blockers Identified
[Any problems that need resolution]

## Session Context Preservation
[Important context that future sessions should know]

---
Generated by Claude
Session Export Date: [YYYY-MM-DD HH:MM:SS]
```

## Bug Tracking During Modernization

### GitHub Issues Integration (Primary Method)

**PRIORITY: Use GitHub Issues for all bug tracking when repository has issues enabled.**

When GitHub Issues are available, Claude must automatically file bugs discovered during modernization using the standardized template:

```bash
gh issue create --title "DOC-BUG: [Brief Description]" --body "
## Bug Type
DOC-[CATEGORY] - [Logic/Security/Memory/Compatibility/Undefined]

## Severity
[Critical/High/Medium/Low]

## Phase Discovered
Phase [X]: [Phase Name] - [file being documented/modernized]

## Files Affected
- [list of affected files with line references]

## Description
[Detailed description of the bug discovered]

## Code Location
[file:line_number references for easy navigation]

## Impact Assessment
[How this affects modernization/gameplay/security]

## Recommended Resolution
[Suggested fix approach and priority]

## Session History
- [Current session info and discovery context]
"
```

**Bug Categories by Phase:**
- **DOC-LOGIC**: Logic errors discovered during documentation
- **DOC-SECURITY**: Security vulnerabilities found during analysis
- **DOC-MEMORY**: Memory management issues identified
- **DOC-COMPATIBILITY**: Platform/portability problems
- **DOC-UNDEFINED**: Undefined behavior or unclear contracts
- **WARN-COMPILE**: Compilation warnings requiring fixes
- **WARN-STATIC**: Static analysis findings
- **MOD-REGRESSION**: Regressions introduced during modernization
- **TEST-FAILURE**: Test failures discovered during validation

**GitHub Issues Setup:**
```bash
# Enable issues if disabled
gh repo edit --enable-issues

# Test issue creation
gh issue list

# File bugs immediately when discovered
gh issue create --title "..." --body "..."
```

### PERSISTENT_BUGS.md Usage (Fallback Method)

**Use only when GitHub Issues are unavailable.** Create and maintain `_modernization/claude/reports/PERSISTENT_BUGS.md` throughout modernization to track issues that span multiple sessions.

**File Format:**
```markdown
# Persistent Bugs and Issues

## Active Bugs (Require Attention)

### BUG-001: Brief Description
- **Status**: Open/In Progress/Resolved
- **Severity**: Critical/High/Medium/Low
- **Phase Discovered**: [Phase number/name]
- **Files Affected**: [List of files]
- **Description**: [Detailed problem description]
- **Reproduction Steps**: [How to reproduce]
- **Workaround**: [Temporary solution if any]
- **Resolution Plan**: [Next steps to fix]
- **Session History**: [Which sessions worked on this]

## Resolved Bugs (For Reference)

### BUG-XXX: Brief Description
[Same format as active bugs]
- **Resolution**: [How it was fixed]
- **Date Resolved**: [YYYY-MM-DD]
```

**Integration with Session Management:**
- **Update during each session** when bugs are discovered, worked on, or resolved
- **Reference in session memory files** when bugs impact current work
- **Include in session logs** when significant bug work is performed
- **Track across phases** to ensure no issues are lost during modernization

## Script-Driven Modernization Benefits

**Automation Benefits:**
- **Consistency**: Ensures uniform modernization across large codebases
- **Speed**: Handles repetitive tasks much faster than manual edits
- **Auditability**: Scripts provide clear record of what changes were made
- **Reversibility**: Backup options allow validation and rollback if needed
- **Reusability**: Scripts can be applied to multiple similar projects
- **Documentation**: Scripts serve as documentation of modernization patterns

**Quality Assurance:**
- **--dry-run option**: Preview changes before applying them
- **--backup option**: Create .orig files for comparison and validation
- **Logging**: All changes logged to timestamped files for audit trail
- **Idempotent**: Safe to run multiple times without corruption
- **Validation**: Built-in checks to ensure changes are appropriate

**Session Efficiency:**
- **Batch Processing**: Handle multiple files in single script execution
- **Time Savings**: Reduces manual editing time by 80-90% for repetitive tasks
- **Focus on Analysis**: More time for complex decision-making and architecture
- **Reduced Errors**: Eliminates manual transcription errors
- **Faster Iterations**: Quick to test different modernization approaches

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
