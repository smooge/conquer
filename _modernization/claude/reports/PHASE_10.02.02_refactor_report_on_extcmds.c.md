# Phase 10.2.2 - Deep Refactoring Analysis: extcmds.c

**Analysis Date**: 2025-10-11
**Analyst**: Claude (claude-sonnet-4-5)
**File**: extcmds.c (773 lines, 9 functions)
**Phase**: 10.2.2 - Deep Refactoring Assessment

---

## Executive Summary

**File Purpose**: Extended army command system providing complex tactical operations beyond basic movement. Handles army splitting, combining, status changes, group formations, and specialized tactical commands.

**Overall Assessment**: ⭐⭐⭐ GOOD - Core command subsystem with moderate architectural debt

**Key Characteristics**:
- **Type**: Type 2+ (Core Subsystem with Misplaced Utilities)
- **Extraction Potential**: ⭐ VERY LOW (11% extractable, 1 pure function)
- **Architectural Placement**: ⭐⭐⭐ MODERATE DEBT (33% misplaced)
- **Configuration Coupling**: ⭐⭐⭐ MODERATE (6 magic numbers)

**Primary Recommendation**: Extract utility functions, externalize game balance constants

---

## Part 1: Unit Testable Extraction Potential

### Function Inventory (9 functions)

#### 1. Pure Functions (High Extraction Value) - 11%

**nocomb_stat(int astat)** - Status compatibility checker (19 lines)
- **Extractable**: ✅ YES - Pure validation function
- **Dependencies**: Status constants only (no global state)
- **Test Category**: A (Unit) - Perfect for pure unit testing
- **Extraction Target**: `army_validation.c` or `status_utils.c`
- **Unit Test Potential**: 10-15 tests (all status codes, boundary conditions)
- **Effort**: 1-2 hours (trivial extraction)
- **Value**: ⭐⭐⭐⭐ HIGH - Pure function, high reusability, zero coupling

```c
// PURE FUNCTION - Zero dependencies
int nocomb_stat(int astat) {
    switch (astat) {
        case FLIGHT:
        case MAGATT:
        case MAGDEF:
        case SCOUT:
        case ONBOARD:
            return TRUE;
        default:
            return FALSE;
    }
}
```

**Why Extract**: This is a textbook example of a pure function that should be in a validation utility module. Zero side effects, high testability, reusable across command subsystems.

#### 2. UI-Coupled Functions (Low Extraction Value) - 33%

**ext_cmd(int armie)** - Command menu dispatcher (77 lines)
- **Extractable**: ❌ NO - Heavy curses UI coupling
- **Dependencies**: Screen display, keyboard input, multiple army operations
- **Test Category**: B (Integration) - Requires UI mocking
- **Reason**: Command dispatcher tightly integrated with interactive menu system
- **Extraction ROI**: Very low - Would require extensive UI abstraction

**splitarmy(int armynum)** - Interactive army splitting UI (11 lines)
- **Extractable**: ❌ NO - UI wrapper
- **Dependencies**: Screen display, input system, reducearmy()
- **Test Category**: B (Integration)
- **Reason**: Simple UI wrapper around reducearmy()

**errormsg(char *str)** - Error display with user acknowledgment (8 lines)
- **Extractable**: ❌ NO - But should be RELOCATED
- **Dependencies**: Curses display, keyboard input
- **Test Category**: B (Integration)
- **Reason**: Generic UI utility, not specific to extended commands
- **Note**: This is misplaced (see Part 2)

**clear_bottom(int i)** - Screen clearing utility (8 lines)
- **Extractable**: ❌ NO - But should be RELOCATED
- **Dependencies**: Curses screen management
- **Test Category**: A (Unit) - With mock screen ops
- **Reason**: Generic UI utility, not specific to extended commands
- **Note**: This is misplaced (see Part 2)

#### 3. Business Logic Functions (Moderate Coupling) - 56%

**combinearmies(int armynum, int army2)** - Army merging logic (33 lines)
- **Extractable**: ⚠️ PARTIAL - Heavy coupling to army data structures
- **Dependencies**: Army arrays, nation data, execution file system
- **Test Category**: B (Integration)
- **Coupling**: Uses global army access macros (P_ASOLD, P_ASTAT, etc.)
- **Extraction Effort**: High (4-6 hours) - Requires data structure abstraction
- **ROI**: Low - Core army operation, extraction benefits minimal

**change_status(int armynum, int new_stat)** - Status modification (92 lines)
- **Extractable**: ⚠️ PARTIAL - Complex game state coupling
- **Dependencies**: Army data, sector data, fort validation, execution system
- **Test Category**: B (Integration)
- **Coupling**: Heavy - validates locations, modifies army state, updates files
- **Extraction Effort**: Very High (8-10 hours)
- **ROI**: Very Low - Core engine function with necessary coupling

**reducearmy(int armynum, int men)** - Army splitting logic (58 lines)
- **Extractable**: ⚠️ PARTIAL - Army management coupling
- **Dependencies**: Army data, execution system, army slot management
- **Test Category**: B (Integration)
- **Coupling**: Creates new armies, modifies existing armies
- **Extraction Effort**: High (4-6 hours)
- **ROI**: Low - Core army operation

**addgroup(int armynum)** - Group formation management (54 lines)
- **Extractable**: ⚠️ PARTIAL - Leadership hierarchy coupling
- **Dependencies**: Army data, leader validation, input system
- **Test Category**: B (Integration)
- **Coupling**: Complex group coordination and movement rate calculation
- **Extraction Effort**: High (5-7 hours)
- **ROI**: Low - Core tactical feature

### Extraction Summary

| Category | Functions | Percentage | Extraction ROI |
|----------|-----------|------------|----------------|
| **Pure (High Value)** | 1 | 11% | ⭐⭐⭐⭐ High |
| **UI-Coupled (Low Value)** | 3 | 33% | ⭐ Very Low |
| **Business Logic (Low Value)** | 4 | 45% | ⭐ Very Low |
| **Misplaced Utilities** | 2 | 22% | N/A (relocate) |

**Total Extractable**: 1 function (11%)
**Realistic Unit Test Potential**: 10-15 tests from nocomb_stat()
**Extraction Effort**: 1-2 hours for pure function
**Overall Extraction Priority**: ⭐ VERY LOW

### Extraction Recommendation

**Action**: Extract 1 pure function only

1. **Extract nocomb_stat()** → `army_validation.c` (1-2 hours)
   - Create new validation utility module
   - Move pure status checking logic
   - Add comprehensive unit tests (10-15 tests)
   - Update all callers

**Do NOT Extract**:
- Business logic functions (combinearmies, change_status, reducearmy, addgroup)
- UI-coupled functions (ext_cmd, splitarmy)
- Misplaced utilities (errormsg, clear_bottom) - These should be RELOCATED, not extracted

**Rationale**: Only one truly pure function exists. Business logic functions have necessary coupling to army management system. UI functions require extensive mocking. The extraction ROI is very low except for the one pure validation function.

---

## Part 2: Architectural Placement Analysis

### Current File Organization

**File Purpose**: Extended army command system and tactical unit management

**Primary Responsibility**: Interactive command menu for complex army operations (splitting, combining, status changes, grouping)

### Function Placement Audit

#### ✅ Correctly Placed Functions (6 of 9 = 67%)

1. **ext_cmd()** - Command dispatcher
   - **Belongs**: ✅ YES - Core extended command menu
   - **Reason**: Primary entry point for extended commands
   - **Single Responsibility**: Command menu orchestration

2. **combinearmies()** - Army merging
   - **Belongs**: ✅ YES - Extended army operation
   - **Reason**: Complex tactical operation beyond basic commands
   - **Single Responsibility**: Army combination logic

3. **change_status()** - Status modification
   - **Belongs**: ✅ YES - Extended army operation
   - **Reason**: Advanced status changes with validation
   - **Single Responsibility**: Army status management

4. **reducearmy()** - Army splitting
   - **Belongs**: ✅ YES - Extended army operation
   - **Reason**: Creates new armies from existing ones
   - **Single Responsibility**: Army division logic

5. **splitarmy()** - Interactive splitting UI
   - **Belongs**: ✅ YES - Extended command UI wrapper
   - **Reason**: UI for reducearmy operation
   - **Single Responsibility**: User interface for splitting

6. **addgroup()** - Group formation
   - **Belongs**: ✅ YES - Extended army operation
   - **Reason**: Advanced tactical feature (leadership hierarchy)
   - **Single Responsibility**: Group management

#### ❌ Misplaced Functions (3 of 9 = 33%)

**1. nocomb_stat(int astat)** - Status compatibility checker
- **Current Location**: extcmds.c
- **Should Be**: `army_validation.c` or `status_utils.c`
- **Reason**: Pure validation utility, reusable across command subsystems
- **Impact**: ⭐⭐⭐ MODERATE - Limits reusability, should be shared
- **Relocation Effort**: 1 hour (create validation module)
- **Dependencies**: None (pure function)

**Key Issue**: This pure function provides status validation that could be used by other command systems (move.c, combat.c, etc.). Should be centralized in a validation utility module.

**2. errormsg(char *str)** - Error display utility
- **Current Location**: extcmds.c
- **Should Be**: `ui_utils.c` or `display_utils.c`
- **Reason**: Generic UI utility, not specific to extended commands
- **Impact**: ⭐⭐⭐⭐ HIGH - Likely duplicated across multiple files
- **Relocation Effort**: 2-3 hours (check for duplicates, consolidate)
- **Dependencies**: Curses display system

**Key Issue**: Error message display is a common UI pattern that should be shared across all command and display modules. If duplicated in other files, this represents significant technical debt.

**3. clear_bottom(int i)** - Screen clearing utility
- **Current Location**: extcmds.c
- **Should Be**: `ui_utils.c` or `display_utils.c`
- **Reason**: Generic screen management, not specific to extended commands
- **Impact**: ⭐⭐⭐⭐ HIGH - Common UI operation, should be shared
- **Relocation Effort**: 2-3 hours (check for duplicates, consolidate)
- **Dependencies**: Curses screen management

**Key Issue**: Screen clearing is a fundamental UI operation used throughout the interface. Should be centralized with other screen management utilities.

### Architectural Debt Summary

**Misplaced Functions**: 3 of 9 (33%)
**Architectural Debt Level**: ⭐⭐⭐ MODERATE
**Primary Issues**:
1. Pure validation function not in shared utility module
2. UI utilities not in shared display module
3. Potential code duplication across command files

### Single Responsibility Analysis

**Current File Responsibility**: Extended army command system

**Violations**:
- Contains generic UI utilities (errormsg, clear_bottom)
- Contains shared validation logic (nocomb_stat)

**Ideal Responsibility**: Should focus only on extended army operations and their specific UI, delegating to shared utilities for common operations.

### Recommended Architectural Changes

#### Priority 1: Create Shared Modules (4-6 hours)

**1. Create `army_validation.c`** (1-2 hours)
```
Relocate: nocomb_stat()
Purpose: Centralized army and status validation
Additional Candidates: Other validation functions from commands.c, move.c
```

**2. Create `ui_utils.c`** (3-4 hours)
```
Relocate: errormsg(), clear_bottom()
Purpose: Shared UI utilities across all command systems
Check For Duplicates In: commands.c, move.c, display.c, magic.c
Consolidate: All duplicate UI helper functions
```

#### Priority 2: File Reorganization Impact

**After Reorganization**:
- extcmds.c: 6 functions (pure extended command operations)
- army_validation.c: Shared validation utilities
- ui_utils.c: Shared display utilities

**Benefits**:
1. Eliminates code duplication (if duplicates exist)
2. Improves reusability of validation and UI utilities
3. Clearer separation of concerns
4. Better testability of pure functions
5. Reduces coupling between command subsystems

### Comparison to Other Files

| File | Misplaced % | Type | Architectural Debt |
|------|-------------|------|-------------------|
| m2alloc.c | 0% | Type 1 | ⭐⭐⭐⭐⭐ ZERO |
| newhelp.c | 0% | Type 4 | ⭐⭐⭐⭐⭐ ZERO |
| sort.c | 0% | Type 4 | ⭐⭐⭐⭐⭐ ZERO |
| cexecute.c | 0% | Type 2 | ⭐⭐⭐⭐⭐ ZERO |
| check.c | 20% | Type 2+ | ⭐⭐⭐⭐ MINOR |
| **extcmds.c** | **33%** | **Type 2+** | **⭐⭐⭐ MODERATE** |
| admin.c | 29% | Type 3 | ⭐⭐ HIGH |

**Position**: extcmds.c has higher architectural debt than check.c but lower than admin.c. The 33% misplacement is significant but consists of small utility functions rather than major subsystems.

---

## Part 3: Configuration Coupling Identification

### Magic Number Audit

#### Game Balance Magic Numbers (Should Be Externalized)

**1. Army Division Ratio** - Line 139
```c
men = safe_long_to_int(P_ASOLD / 2);  // Magic: 2 (divide army in half)
```
- **Context**: Divide army command (/) splits army into two equal parts
- **Impact**: Game balance - affects army management strategy
- **Recommendation**: `#define DEFAULT_ARMY_DIVISION_RATIO 2` → `game_balance.h`
- **Rationale**: Fixed ratio affects tactical decisions
- **Priority**: ⭐⭐⭐ MODERATE

**2. Minimum Soldier Thresholds** - Lines 380, 501, 504
```c
if ((P_ATYPE != A_SCOUT) && (P_ASOLD >= 25)) {  // Magic: 25
    errormsg("Need less than 25 men to scout");
}

if (men < 25 || ... || P_ASOLD < men + 25 ...) {  // Magic: 25 (twice)
    errormsg("Selected army too small or illegal");
}
```
- **Context**: Minimum army sizes for scouting and splitting
- **Impact**: Core game balance - affects army composition strategy
- **Recommendation**: Externalize to `game_balance.h`:
  ```c
  #define MIN_ARMY_SCOUT_SIZE 25        /* Maximum soldiers for scout conversion */
  #define MIN_ARMY_SPLIT_SIZE 25        /* Minimum soldiers to split from army */
  #define MIN_ARMY_REMAIN_SIZE 25       /* Minimum soldiers remaining after split */
  ```
- **Rationale**: Critical thresholds affecting strategic gameplay
- **Priority**: ⭐⭐⭐⭐ HIGH
- **Occurrences**: 3 (lines 380, 501, 504)

**3. Movement Penalty Percentage** - Lines 427, 432
```c
if (P_AMOVE < (curntn->maxmove * *(unitmove + (P_ATYPE % UTYPE))) / 50) {
    // Magic: 50 (represents 50% = half movement)
}
P_AMOVE = safe_int_to_uchar(P_AMOVE
    - (curntn->maxmove * *(unitmove + (P_ATYPE % UTYPE))) / 50);
```
- **Context**: Status change penalty when stopping march
- **Impact**: Tactical balance - prevents rapid status switching
- **Recommendation**: `#define MARCH_STOP_PENALTY_DIVISOR 50` → `game_balance.h`
- **Comment**: "50 = half of 100% (represents 50% movement penalty)"
- **Rationale**: Complex formula using percentage-based calculation
- **Priority**: ⭐⭐⭐⭐ HIGH
- **Occurrences**: 2 (lines 427, 432)

#### UI Layout Magic Numbers (Lower Priority)

**4. Default Clear Lines** - Line 675
```c
if (i == 0)
    i = 4;  // Magic: 4 (default number of lines to clear)
```
- **Context**: Screen clearing utility default parameter
- **Impact**: UI layout consistency
- **Recommendation**: `#define DEFAULT_UI_CLEAR_LINES 4` → `ui_config.h`
- **Rationale**: UI constant, less critical but should be defined
- **Priority**: ⭐⭐ LOW

**5. UI Column Offset** - Line 634
```c
mvaddstr(LINES - 1, COLS - 16, "PRESS ANY KEY");  // Magic: 16
```
- **Context**: Right-aligned prompt positioning
- **Impact**: Visual layout only
- **Recommendation**: `#define PROMPT_KEY_WIDTH 16` → `ui_config.h`
- **Comment**: "Width of 'PRESS ANY KEY' prompt"
- **Rationale**: UI positioning constant
- **Priority**: ⭐ VERY LOW

**6. Selector Offset** - Lines 143, 145
```c
selector += 2;  // Magic: 2 (UI offset for army selection)
army2 = getselunit();
selector -= 2;  // Magic: 2 (restore selector offset)
```
- **Context**: Temporary offset for secondary army selection
- **Impact**: UI interaction behavior
- **Recommendation**: `#define SELECTOR_SECONDARY_OFFSET 2` → `ui_config.h`
- **Rationale**: UI behavior constant
- **Priority**: ⭐ VERY LOW

### Configuration Coupling Summary

| Category | Count | Occurrences | Priority | Target File |
|----------|-------|-------------|----------|-------------|
| **Game Balance** | 3 | 6 | ⭐⭐⭐⭐ HIGH | game_balance.h |
| **UI Layout** | 3 | 5 | ⭐⭐ LOW | ui_config.h |
| **Total** | 6 | 11 | | |

### Constants Already Well-Defined ✅

**Excellent Constant Usage**:
- Status codes: ATTACK, DEFEND, MARCH, SCOUT, GARRISON, RULE, SORTIE, SIEGE, SIEGED, GENERAL, FLIGHT, MAGATT, MAGDEF, ONBOARD, TRADED
- Army types: A_SCOUT, A_MILITIA, A_ZOMBIE, MINLEADER, MINMONSTER
- Array limits: MAXARM, NUMSTATUS
- Boolean values: TRUE, FALSE
- Special values: UTYPE

**No Magic Numbers Found For**:
- Status codes (all properly defined)
- Army type checks (all using constants)
- Array bounds (all using MAXARM)

### Recommended Externalizations

#### Create `game_balance.h` (if not exists) or add to existing file:

```c
/* Army Management Thresholds */
#define MIN_ARMY_SCOUT_SIZE        25  /* Maximum soldiers for scout conversion */
#define MIN_ARMY_SPLIT_SIZE        25  /* Minimum soldiers to split from army */
#define MIN_ARMY_REMAIN_SIZE       25  /* Minimum soldiers remaining after split */
#define DEFAULT_ARMY_DIVISION_RATIO 2  /* Ratio for divide-by-2 army split command */

/* Movement Penalties */
#define MARCH_STOP_PENALTY_DIVISOR 50  /* Movement penalty when stopping march (50% = half) */
```

**Impact**: 3 game balance constants affecting tactical gameplay
**Priority**: ⭐⭐⭐⭐ HIGH - Should be done in Phase 10.3+
**Effort**: 1-2 hours (externalization + testing)

#### Create `ui_config.h` (if not exists) or add to existing file:

```c
/* UI Layout Constants */
#define DEFAULT_UI_CLEAR_LINES    4   /* Default number of screen lines to clear */
#define PROMPT_KEY_WIDTH         16   /* Width of 'PRESS ANY KEY' prompt */
#define SELECTOR_SECONDARY_OFFSET 2   /* Offset for secondary unit selection */
```

**Impact**: UI consistency and maintainability
**Priority**: ⭐⭐ LOW - Can be deferred
**Effort**: 30 minutes

### Configuration Coupling Level

**Overall Rating**: ⭐⭐⭐ MODERATE (6 magic numbers)

**Comparison to Other Files**:
- m2alloc.c: 0 magic numbers (⭐⭐⭐⭐⭐ Perfect)
- newhelp.c: 0 magic numbers (⭐⭐⭐⭐⭐ Perfect)
- sort.c: 0 magic numbers (⭐⭐⭐⭐⭐ Perfect)
- check.c: 3 magic numbers (⭐⭐⭐⭐ Good)
- **extcmds.c: 6 magic numbers (⭐⭐⭐ Moderate)**
- cexecute.c: 8 magic numbers (⭐⭐⭐ Moderate)
- admin.c: 50+ magic numbers (⭐ Poor)

**Assessment**: Moderate coupling with clear separation between game balance constants (high priority) and UI layout constants (low priority). Better than admin.c and cexecute.c, but room for improvement.

---

## Overall File Assessment

### File Classification

**Type**: Type 2+ (Core Command Subsystem with Misplaced Utilities)

**Characteristics**:
- Runtime game code (no main())
- Core tactical command system
- 33% misplaced utility functions
- Moderate configuration coupling
- Heavy UI integration (curses)

**Comparison to Type Framework**:
- Not Type 1 (has UI coupling)
- Not Type 2 (has misplaced functions)
- **Type 2+** (core subsystem with utility debt)
- Not Type 3 (not mixed responsibility - utilities are small)
- Not Type 4 (not standalone program)

### Quality Rating

**Overall**: ⭐⭐⭐ GOOD (3 of 5 stars)

**Breakdown**:
- **Code Quality**: ⭐⭐⭐⭐ Good - Well-documented, modern C2x syntax
- **Architecture**: ⭐⭐⭐ Moderate - 33% misplaced utilities
- **Testability**: ⭐⭐ Low - Heavy UI coupling, integration testing required
- **Modularity**: ⭐⭐⭐ Moderate - Core operations well-separated
- **Configuration**: ⭐⭐⭐ Moderate - 6 magic numbers needing externalization

### Strengths

1. **Excellent Documentation**: Comprehensive function headers with testing notes
2. **Modern C2x**: Uses C2x features ([[fallthrough]], proper types)
3. **Clear Responsibility**: Extended commands well-separated from basic commands
4. **Good Validation**: Extensive error checking and user feedback
5. **Tactical Depth**: Supports complex army management features

### Weaknesses

1. **UI Coupling**: Heavy reliance on curses makes testing difficult
2. **Misplaced Utilities**: 3 functions should be in shared modules
3. **Magic Numbers**: 6 hardcoded values need externalization
4. **Global State**: Uses macro-based global access (P_ASOLD, AADJMEN, etc.)
5. **Low Testability**: Only 11% of code is pure and unit testable

---

## Recommendations

### Priority 1: Architectural Cleanup (4-6 hours)

**Action Items**:
1. Create `army_validation.c` and relocate nocomb_stat() (1-2 hours)
2. Create `ui_utils.c` and relocate errormsg(), clear_bottom() (2-3 hours)
3. Search for and consolidate any duplicate UI utilities (1 hour)

**Benefits**:
- Eliminates code duplication
- Improves reusability
- Better testability of pure functions
- Clearer separation of concerns

### Priority 2: Configuration Externalization (1-2 hours)

**Action Items**:
1. Create or extend `game_balance.h` with army thresholds (30 min)
2. Create or extend `game_balance.h` with movement penalties (30 min)
3. Update all references to use constants (30 min)
4. Test changes (30 min)

**Benefits**:
- Game balance tuning without code changes
- Clear documentation of tactical parameters
- Easier testing of different configurations

### Priority 3: Testing Infrastructure (Deferred to Phase 11)

**Action Items**:
1. Create unit tests for nocomb_stat() after extraction (15 tests)
2. Create integration tests for army operations with mocked data
3. Create UI tests with mocked curses functions

**Benefits**:
- Regression protection
- Confidence in refactoring
- Documentation of expected behavior

### Do NOT Do (Low ROI)

❌ **Do Not Extract Business Logic Functions**
- combinearmies(), change_status(), reducearmy(), addgroup()
- These have necessary coupling to army management
- Extraction would create complex data abstraction with minimal benefit

❌ **Do Not Extract UI Functions**
- ext_cmd(), splitarmy()
- These are tightly integrated with interactive menu system
- Would require extensive UI abstraction layer

---

## File Type Analysis

### Comparison to Previously Analyzed Files

| Metric | m2alloc.c | check.c | extcmds.c | cexecute.c | admin.c |
|--------|-----------|---------|-----------|------------|---------|
| **Lines** | 269 | 478 | 773 | 478 | ~500+ |
| **Functions** | 10 | 5 | 9 | 2 | 7 |
| **Has main()** | ❌ | ❌ | ❌ | ❌ | ❌ |
| **Game Deps** | ❌ | ✅ | ✅ | ✅ | ✅ |
| **Extractable** | 100% | 20% | 11% | 0% | ~14% |
| **Misplaced** | 0% | 20% | 33% | 0% | 29% |
| **Magic #s** | 0 | 3 | 6 | 8 | 50+ |
| **File Type** | Type 1 | Type 2+ | Type 2+ | Type 2 | Type 3 |
| **Context** | Runtime | Runtime | Runtime | Runtime | Runtime |
| **Rating** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐ |
| **Action** | None | Extract+Config | Relocate+Config | Config | Major refactor |

### Pattern Recognition

**extcmds.c Patterns**:
- Core command subsystem (like check.c validation subsystem)
- Similar misplacement level to admin.c (utilities not in shared modules)
- Moderate magic numbers (better than admin.c, worse than check.c)
- Heavy UI coupling (unique to command files)

**Key Insight**: Command subsystems (extcmds.c, likely commands.c) form their own category with:
- High UI coupling (curses integration)
- Low extraction potential (business logic tied to data structures)
- Utility function misplacement (UI helpers should be shared)
- Moderate configuration coupling (game balance constants)

---

## Effort Estimates

### Immediate Refactoring (Phase 10.3+)

**Total Effort**: 5-8 hours

1. **Relocate nocomb_stat() to army_validation.c** (1-2 hours)
   - Create validation module
   - Move function and update callers
   - Add unit tests

2. **Consolidate UI utilities to ui_utils.c** (3-4 hours)
   - Check for duplicates across codebase
   - Create shared UI module
   - Relocate errormsg() and clear_bottom()
   - Update all callers

3. **Externalize game balance constants** (1-2 hours)
   - Add constants to game_balance.h
   - Update all references
   - Test changes

### Testing Infrastructure (Phase 11)

**Total Effort**: 4-6 hours

1. **Unit tests for nocomb_stat()** (1 hour) - 10-15 tests
2. **Integration tests for army operations** (3-5 hours) - Mock data structures
3. **UI tests with mocked curses** (not estimated - complex)

### Total Re-engineering Impact

**This File**: 9-14 hours (moderate impact)
**Dependencies**: Requires creation of shared modules that benefit entire codebase

---

## Critical Discoveries

### Discovery 1: UI Utility Duplication Pattern

**Finding**: errormsg() and clear_bottom() are generic UI utilities in a command-specific file.

**Hypothesis**: These utilities are likely duplicated in other command files (commands.c, move.c, magic.c, etc.).

**Investigation Needed**: Search codebase for similar UI utility functions and consolidate to shared module.

**Impact**: If duplicated, represents significant technical debt and opportunity for code reduction.

### Discovery 2: Pure Validation Functions

**Finding**: nocomb_stat() is a pure validation function with zero dependencies.

**Pattern**: Pure validation functions should be in shared validation modules, not scattered across command files.

**Recommendation**: Create systematic validation module architecture for army, status, location, and unit validations.

### Discovery 3: Command Subsystem Architecture

**Finding**: Extended commands form a distinct subsystem separate from basic commands.

**Architecture**: Good separation between basic and extended commands, but shared utilities not properly factored out.

**Recommendation**: Continue this separation pattern but extract common utilities.

---

## Next Steps

### For Phase 10.2.2 (Analysis)

- [x] Complete analysis of extcmds.c
- [ ] Continue with next file in user-specified order (header.h)
- [ ] Track patterns across command subsystems

### For Phase 10.3+ (Implementation)

**Priority Actions**:
1. Create shared utility modules (army_validation.c, ui_utils.c)
2. Search for and consolidate duplicate UI utilities
3. Externalize game balance constants
4. Add unit tests for pure functions

**Deferred Actions**:
- Integration testing infrastructure (Phase 11)
- UI abstraction layer (if needed)
- Comprehensive army operation testing

---

## Session Notes

**Analysis Duration**: ~45 minutes
**Complexity**: Moderate - Command subsystem with UI coupling
**Surprises**:
- Only 11% extractable (lower than expected)
- 33% misplaced (higher than check.c)
- Pure validation function ideal for extraction

**Key Takeaway**: Command subsystems have distinct patterns (high UI coupling, low extraction potential, utility misplacement) that differ from engine files (cexecute.c) and pure utilities (m2alloc.c).

---

**Report Complete**: 2025-10-11 16:45
**Next File**: header.h (architectural debt + configuration coupling only)
**Files Completed**: 7 of 30 (23% progress)
