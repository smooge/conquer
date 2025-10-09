# Phase 10.00 - Planning and Strategy TODO

**Phase**: 10.00 - Planning and Strategy
**Status**: 📋 READY TO START
**Created**: 2025-10-09
**Branch**: phase_10_deep_refactoring
**Previous Phase**: 9.6 - Phase Retrospective (✅ COMPLETE)
**Critical Priority**: 🔴 Char Signedness Portability (GitHub Issue #10)

---

## Phase Overview

**Objective**: Complete Phase 10 planning with focus on char signedness portability critical path

**Scope**:
- Review GitHub Issue #10 (char signedness)
- Create char audit strategy
- Establish x86_64 validation approach
- Plan Phase 10.1-10.3 critical path
- Update issue tracker

**Estimated Duration**: 1-2 hours

**Critical Context**:
- Current platform: aarch64 (unsigned char default)
- Target platforms: x86_64 (signed char default)
- **34 `(unsigned char)` casts** work on aarch64 but **BROKEN on x86_64**
- This is a **portability blocker** for Debian, Fedora, FreeBSD deployment

---

## Task Checklist

### 1. GitHub Issue Management

- [ ] **1.1** Review GitHub Issue #10 details
  - Understand char signedness problem
  - Review platform research findings
  - Note high-risk file locations

- [ ] **1.2** Update Issue #10 labels and priority
  - Verify labels: `memory-safety`, `phase10`, `technical-debt`, `portability`
  - Confirm priority: HIGH/CRITICAL
  - Add milestone: Phase 10

- [ ] **1.3** Comment on Issue #10 with Phase 10 plan
  - Link to PHASE_10_STRATEGY.md
  - Outline critical path: 10.1 → 10.2 → 10.3
  - Set expectations for completion timeline

### 2. Char Audit Preparation

- [ ] **2.1** Create char audit tooling
  - Write grep script to find all `char` declarations
  - Script to find `(unsigned char)` casts
  - Script to find ctype.h function usage
  - Output to `_modernization/claude/reports/char_audit_raw.txt`

- [ ] **2.2** Identify high-risk files
  - `misc.c` - Array indexing (lines 1805-1838)
  - `spew.c` - ctype.h usage (line 1207)
  - `update.c` - Calculations (lines 1628-1638)
  - `forms.c` - User input (lines 837, 894-896)
  - `commands.c` - Navy calculations (lines 778, 944, 958)
  - `reports.c` - Navy calculations (lines 1210-1212)

- [ ] **2.3** Create classification template
  - Spreadsheet format: File:Line | Current | Category | New | Risk
  - Categories: A (text), B (unsigned int), C (signed int), D (ctype.h), E (array index)
  - Risk levels: HIGH, MEDIUM, LOW

### 3. x86_64 Validation Strategy

- [ ] **3.1** Determine x86_64 testing approach
  - **Option A**: Access to x86_64 machine (VM, cloud, physical)
  - **Option B**: Compiler flag simulation (`-fsigned-char`)
  - **Option C**: Docker/container with qemu
  - Document chosen approach and rationale

- [ ] **3.2** Create x86_64 validation test plan
  - Test cases for array indexing safety
  - Test cases for ctype.h safety
  - Test cases for calculation safety
  - Pre-fix validation (confirm problem exists)
  - Post-fix validation (confirm problem solved)

- [ ] **3.3** Establish baseline testing
  - Run current tests on aarch64 (baseline)
  - Document current pass rate (10/10 expected)
  - Identify tests needing x86_64-specific variants

### 4. Critical Path Planning

- [ ] **4.1** Create Phase 10.1 TODO (Char Audit & Refactoring)
  - Detailed task breakdown
  - File-by-file refactoring plan
  - Test requirements per change
  - Estimated duration: 6-8 hours

- [ ] **4.2** Create Phase 10.2 TODO (Safe Conversion Implementation)
  - safe_char_to_uchar implementation details
  - Test suite requirements (15-20 tests)
  - Deployment strategy (HIGH → MEDIUM → LOW risk)
  - Estimated duration: 2-3 hours

- [ ] **4.3** Create Phase 10.3 TODO (x86_64 Validation)
  - Platform-specific test cases
  - Validation checklist
  - Sanitizer testing requirements
  - Estimated duration: 2-3 hours

### 5. Secondary Refactoring Planning (Lower Priority)

- [ ] **5.1** Identify utility extraction candidates
  - String manipulation utilities
  - Path utilities
  - Number parsing utilities
  - Target: 15-20 functions

- [ ] **5.2** Identify error handling candidates
  - Void functions needing return values
  - Functions using exit() that shouldn't
  - Target: 30-50 functions

- [ ] **5.3** Identify size_t conversion candidates
  - Array indices using int
  - Memory sizes using int
  - File operations using int
  - Target: Focus on high-impact areas

### 6. Documentation and Communication

- [ ] **6.1** Update CLAUDE.md
  - Add Phase 10 section
  - Document char signedness guidelines
  - Add x86_64 testing requirements

- [ ] **6.2** Create char type guidelines document
  - When to use `char` vs `unsigned char` vs `signed char`
  - Safe conversion function usage guide
  - ctype.h safety requirements
  - Array indexing safety requirements

- [ ] **6.3** Update project README
  - Document platform requirements
  - Add char signedness notes
  - Link to char type guidelines

### 7. Risk Mitigation

- [ ] **7.1** Identify potential breaking changes
  - Functions with char parameters
  - Struct members using char
  - APIs exposed to external code

- [ ] **7.2** Create rollback strategy
  - Backup branch before major changes
  - Incremental commits for easy revert
  - Testing checkpoints between changes

- [ ] **7.3** Establish testing checkpoints
  - Test after each high-risk file
  - Full test suite after each sub-phase
  - x86_64 validation at end of 10.3

---

## Success Criteria

Phase 10.00 is complete when:

- ✅ **GitHub Issue #10 reviewed**: Fully understood and updated
- ✅ **Char audit tooling created**: Scripts ready to find all char declarations
- ✅ **High-risk files identified**: Know where to focus first
- ✅ **x86_64 validation strategy established**: Know how to test on signed char platform
- ✅ **Critical path TODOs created**: Phase 10.1, 10.2, 10.3 ready to execute
- ✅ **Char type guidelines documented**: Clear rules for future development
- ✅ **Risk mitigation planned**: Know how to handle breaking changes

---

## Key Deliverables

### Planning Documents
1. **PHASE_10.00_TODO.md** (this file) - Planning checklist
2. **PHASE_10.1_TODO.md** - Char audit and refactoring tasks
3. **PHASE_10.2_TODO.md** - Safe conversion implementation tasks
4. **PHASE_10.3_TODO.md** - x86_64 validation tasks

### Audit Tools
1. **char_audit_script.sh** - Find all char declarations
2. **char_audit_raw.txt** - Raw output from audit
3. **char_classification.md** - Classified char declarations

### Guidelines
1. **CHAR_TYPE_GUIDELINES.md** - When to use which char type
2. **X86_64_VALIDATION.md** - Testing strategy for signed char platforms

### Issue Tracking
1. GitHub Issue #10 updated with Phase 10 plan
2. Milestone set for Phase 10 completion

---

## Critical Path Summary

**Phase 10 Critical Path** (MUST complete for x86_64 deployment):
```
10.0 Planning (this phase)
  ↓
10.1 Char Type Audit & Refactoring (6-8 hours)
  ↓
10.2 Safe Conversion Implementation (2-3 hours)
  ↓
10.3 x86_64 Validation Testing (2-3 hours)
  ↓
x86_64 Deployment Unblocked ✅
```

**Total Critical Path Time**: 11-16 hours

**Secondary Work** (can defer if needed):
- 10.4: Utility Extraction (4-5 hours)
- 10.5: Error Handling (3-4 hours)
- 10.6: Integer Portability (2-3 hours)

---

## High-Risk Files Reference (from Issue #10)

### Immediate Attention Required

**misc.c (Array Indexing - CRITICAL)**
```c
// Lines 1805-1838
veg_cost[ (unsigned char)veg[j] ] = EVegcost[j] - '0';   // Line 1805
ele_cost[ (unsigned char)ele[j] ] = DElecost[j] - '0';   // Line 1828
```
**Risk**: Negative array index on x86_64 → undefined behavior → potential crash/exploit

**spew.c (ctype.h - CRITICAL)**
```c
// Line 1207
isspace((unsigned char)*end)
```
**Risk**: C standard violation on x86_64 → undefined behavior

**update.c (Calculation - HIGH)**
```c
// Lines 1628-1638
curntn->poverty = (unsigned char)(95L - curntn->tgold/curntn->tciv);  // Line 1630
```
**Risk**: Negative calculation result → incorrect unsigned char value

**forms.c, commands.c, reports.c (User Input/Calculations - MEDIUM)**
- forms.c: lines 837, 894-896
- commands.c: lines 778, 944, 958
- reports.c: lines 1210-1212
**Risk**: User input or calculations may produce negative values

---

## Timeline Estimate

| Task Category | Estimated Time | Priority |
|---------------|----------------|----------|
| GitHub Issue Management | 20-30 minutes | 🔴 CRITICAL |
| Char Audit Preparation | 30-45 minutes | 🔴 CRITICAL |
| x86_64 Validation Strategy | 20-30 minutes | 🔴 CRITICAL |
| Critical Path Planning | 30-45 minutes | 🔴 CRITICAL |
| Secondary Planning | 20-30 minutes | 🟡 HIGH |
| Documentation | 20-30 minutes | 🟡 HIGH |
| Risk Mitigation | 15-20 minutes | 🟡 HIGH |
| **Total** | **2.5-3.5 hours** | |

**Actual Target**: 1-2 hours (focus on critical items, defer secondary if needed)

---

## Important Reminders

### Platform Difference (CRITICAL)
- **aarch64** (current): `char` is UNSIGNED (`CHAR_MIN = 0`)
- **x86_64** (target): `char` is SIGNED (`CHAR_MIN = -128`)

### Security Implications
- Negative char → array index = **UNDEFINED BEHAVIOR**
- Negative char → ctype.h = **UNDEFINED BEHAVIOR** (C standard violation)
- Undefined behavior = **SECURITY VULNERABILITY** (exploitable)

### Testing Requirements
- Tests passing on aarch64 ≠ tests passing on x86_64
- MUST validate on x86_64 or simulate with `-fsigned-char`
- Cannot deploy to x86_64 without validation

---

**Document Version**: 1.0
**Created**: 2025-10-09
**Last Updated**: 2025-10-09
**Status**: Ready for Execution
**Next Session**: Begin Phase 10.00 tasks, starting with GitHub Issue #10 review
