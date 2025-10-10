# Phase 10.2.2 - Pure Function Extraction Analysis Report

## File: forms.c

**Analysis Date**: 2025-10-10
**Analyzer**: Claude (claude-sonnet-4-5@20250929)
**Phase**: 10.2.2 - Pure Function Extraction
**File Size**: 1,786 lines
**Total Functions**: 6 major functions

---

## Executive Summary

**Overall Assessment**: ⭐⭐ **LOW extraction potential**

`forms.c` is a user interface module heavily focused on screen display, user input, and I/O operations. The file contains minimal pure calculation logic suitable for extraction. Most functions are presentation layer code with curses library integration, making them inherently stateful and I/O-bound.

**Key Findings**:
- **2-4 pure functions** can be extracted (very limited)
- **6-12 unit tests** potential (minimal)
- **High I/O coupling** throughout all major functions
- **Presentation logic dominates** over business logic
- **Not a priority target** for Phase 10.2.2

**Strategic Recommendation**: **DEFER** this file - Focus extraction efforts on business logic files (update.c, combat.c, navy.c) first. Forms.c provides minimal ROI for refactoring effort.

---

## File Overview

### Purpose
User interface framework providing:
- Nation score display and statistics screens
- Interactive diplomacy management interface
- Nation statistics editor
- Help system with file-based documentation
- News display and newspaper reading system
- String highlighting and text formatting

### Current Architecture
**Pattern**: Presentation layer with heavy curses integration
- Large UI functions (200-500+ lines)
- Direct screen manipulation throughout
- User input processing loops
- File I/O for help and news content
- Minimal business logic or calculations

### Documentation Status
✅ **Excellent** - All functions comprehensively documented in Phase 3
- Testing categories assigned (mostly B - Integration)
- Clear interface descriptions
- Comprehensive parameter documentation

---

## Extraction Candidates (Very Limited)

### 🥇 Priority 1: Revolt Risk Calculations

**Location**: `change()` function, lines 827-835
**Current State**: Embedded in massive UI loop (470 lines)
**Category**: Economic/political calculation

#### Extractable Functions

##### 1. `calculate_peasant_revolt_risk()`
```c
/*
 * calculate_peasant_revolt_risk - Calculate peasant revolt probability
 *
 * Computes revolt risk based on tax burden, popularity, terror, and charity.
 * Higher tax rates and terror increase risk; popularity and charity reduce it.
 *
 * Parameters:
 *   tax_rate - National tax rate percentage (0-20)
 *   popularity - National popularity score (0-100)
 *   terror - Terror level (0-100)
 *   charity - Charity percentage (0-25)
 *
 * Returns:
 *   Revolt risk percentage (0-100)
 *
 * Formula: max(0, (10*tax - popularity - terror - 3*charity) * PREVOLT / 100)
 *
 * Testing: 5+ test cases for factor combinations
 */
int calculate_peasant_revolt_risk(int tax_rate, unsigned char popularity,
                                  unsigned char terror, unsigned char charity);
```

**Current Code** (lines 827-831):
```c
i = 10 * curntn->tax_rate - curntn->popularity - curntn->terror
    - 3 * curntn->charity;
if (i < 0)
    i = 0;
mvprintw(8, 0, "peasant revolt %2d%%", (i * PREVOLT) / 100);
```

**Test Cases**:
1. Zero tax, high popularity → 0% revolt risk
2. High tax, low popularity → high revolt risk
3. High charity reduces tax burden effect
4. Terror contribution to revolt risk
5. Negative intermediate value handling

**Extraction Value**: ⭐⭐⭐ (Small but clean calculation)

---

##### 2. `calculate_other_revolt_risk()`
```c
/*
 * calculate_other_revolt_risk - Calculate non-peasant revolt probability
 *
 * Computes revolt risk from nobility/military based on tax burden and prestige.
 * Simpler formula than peasant revolt (no terror/charity factors).
 *
 * Parameters:
 *   tax_rate - National tax rate percentage (0-20)
 *   prestige - National prestige score (0-100)
 *
 * Returns:
 *   Revolt risk percentage (0-100)
 *
 * Formula: max(0, (5*tax - prestige) * PREVOLT / 100)
 *
 * Testing: 3+ test cases for tax/prestige combinations
 */
int calculate_other_revolt_risk(int tax_rate, unsigned char prestige);
```

**Current Code** (lines 832-835):
```c
i = 5 * curntn->tax_rate - curntn->prestige;
if (i < 0)
    i = 0;
mvprintw(9, 0, "other revolt.. %2d%%", (i * PREVOLT) / 100);
```

**Test Cases**:
1. Zero tax → 0% revolt risk
2. High tax, low prestige → high revolt risk
3. High prestige negates tax burden

**Extraction Value**: ⭐⭐⭐ (Simple but testable)

---

### 🥈 Priority 2: Combat Bonus Cost Calculation

**Location**: `change()` function, lines 1101-1109, 1122-1125
**Current State**: Embedded in combat bonus purchase logic
**Category**: Military economics

##### 1. `calculate_combat_bonus_cost()`
```c
/*
 * calculate_combat_bonus_cost - Calculate metal cost for combat bonus increase
 *
 * Implements quadratic cost scaling for combat improvements with race penalties.
 * Warlord/Captain/Warrior classes receive base bonus discounts.
 *
 * Parameters:
 *   current_bonus - Current attack or defense bonus (0-100+)
 *   base_discount - Leadership bonus discount (0 for none, 10/20/30 for ranks)
 *   military_count - Total military units (minimum 1500)
 *   race - Nation race (ORC gets 3x penalty)
 *
 * Returns:
 *   Metal cost for +1% bonus increase
 *
 * Formula: METALORE * max(men,1500) * ((bonus-discount)/10)^2 * (race==ORC ? 3 : 1)
 *
 * Testing: 6+ test cases for cost scaling
 */
long calculate_combat_bonus_cost(short current_bonus, int base_discount,
                                 long military_count, char race);
```

**Current Code** (lines 1101-1109):
```c
men = 0;
for (armynum = 0; armynum < MAXARM; armynum++)
    if ((P_ASOLD > 0) && (P_ATYPE < MINLEADER))
        men += P_ASOLD;
men = max(men, 1500);
armynum = safe_int_to_short(max(curntn->aplus - intval, 10) / 10);
cost = METALORE * men * armynum * armynum;
if (curntn->race == ORC)
    cost *= 3;
```

**Test Cases**:
1. Small army (< 1500) → minimum cost (1500 baseline)
2. Large army → proportional cost increase
3. Low bonus → low cost
4. High bonus → quadratic cost growth
5. Orc race → 3x cost multiplier
6. Leadership bonuses → cost reduction

**Extraction Value**: ⭐⭐⭐⭐ (Complex formula, testable)

---

### 🥉 Priority 3: Bribery Cost Calculation

**Location**: `diploscrn()` function, lines 518-523
**Current State**: Embedded in diplomacy UI loop
**Category**: Diplomatic economics

##### 1. `calculate_bribery_cost()`
```c
/*
 * calculate_bribery_cost - Calculate cost to bribe NPC nation
 *
 * Scales bribery cost based on target nation's military strength.
 * Prevents cheap bribery of powerful nations.
 *
 * Parameters:
 *   target_military - Target nation's total military (0+)
 *   is_god - God mode (bypasses cost)
 *
 * Returns:
 *   Gold cost for bribery attempt
 *   0 if god mode, BRIBE minimum, or BRIBE * (military/1000)
 *
 * Testing: 4 test cases for scaling tiers
 */
long calculate_bribery_cost(long target_military, int is_god);
```

**Current Code** (lines 518-523):
```c
if (isgod == TRUE) {
    bribecost = 0;
} else if (ntn[nation].tmil > 1000) {
    bribecost = BRIBE * ntn[nation].tmil / 1000;
} else
    bribecost = BRIBE;
```

**Test Cases**:
1. God mode → cost = 0
2. Small military (<= 1000) → base BRIBE cost
3. Medium military (1000-5000) → scaled cost
4. Large military (> 5000) → high cost

**Extraction Value**: ⭐⭐⭐ (Simple but important)

---

## Extraction Summary

### Total Extraction Potential

| Priority | Functions | Tests | Effort | Target File |
|----------|-----------|-------|--------|-------------|
| 1. Revolt Risk | 2 | 6-8 | 30min | `political_calculations.c` |
| 2. Combat Cost | 1 | 6 | 30min | `military_economics.c` |
| 3. Bribery Cost | 1 | 4 | 20min | `diplomatic_calculations.c` |
| **TOTAL** | **4** | **16-18** | **1.5h** | **3 new files** |

### Phase 10.2.2 Contribution

**Goal**: 15-25 pure functions, 30-75 tests
**This File**: 4 functions, 16-18 tests
**Contribution**: ~20% of goal (minimal impact)

---

## Functions NOT Suitable for Extraction

### `showscore()` - Nation Score Display (lines 193-301)
**Why Not**: Pure presentation layer
- Screen layout calculations (xpos, ypos positioning)
- Curses display operations (mvprintw, standout)
- Multi-column formatting logic
- Pagination state management
- **No business logic** - only display formatting

**Pattern**: UI orchestration (acceptable architecture)

---

### `diploscrn()` - Diplomacy Interface (lines 414-676)
**Why Not**: Heavy I/O and state modification
- Diplomatic status changes (core purpose)
- Mail system integration
- File I/O for bribery logging
- Complex user input loops
- Cascade effects (treaty alliances)
- **Side effects are primary purpose**

**Extractable**: Only bribery cost (already identified)

---

### `change()` - Nation Statistics Editor (lines 793-1263)
**Why Not**: Massive UI function (470 lines)
- Continuous display/input loop
- Password validation and encryption
- God mode authentication
- File I/O for commodity changes
- Screen rendering throughout
- **Primarily state modification interface**

**Extractable**: Revolt risk and combat cost (already identified)

**Refactoring Note**: This function is too large (470 lines) and could benefit from decomposition into smaller UI handler functions, but that's **architectural refactoring**, not pure function extraction.

---

### `help()` - Help System (lines 1352-1429)
**Why Not**: File I/O and display orchestration
- Help file reading and parsing
- Screen pagination control
- User navigation state machine
- Curses display formatting
- **No calculations** - only text display

**Pattern**: Acceptable for UI layer

---

### `mvaddstrnahil()` - String Highlighting (lines 1493-1507)
**Why Not**: Already a pure display function
- Simple string processing
- Direct curses integration
- **Already minimal and focused**
- No business logic to extract

**Status**: Appropriately sized utility function

---

### `newspaper()` - Newspaper Reading System (lines 1593-1785)
**Why Not**: Complex navigation state machine
- File I/O for news content
- Multi-page navigation with goto statements
- Screen rendering and formatting
- User input processing loops
- **No business calculations**

**Pattern**: Acceptable UI complexity for feature richness

---

## Strategic Analysis

### Why forms.c Has Low Extraction Potential

#### 1. **Presentation Layer Architecture**
- Primary purpose: Display game data to users
- Secondary purpose: Collect user input
- Tertiary purpose: Update game state based on input
- **Minimal calculation logic** involved

#### 2. **Appropriate Separation Already Exists**
- Business logic lives in other files (update.c, combat.c, etc.)
- forms.c correctly focuses on presentation concerns
- **Extraction would violate layer separation**

#### 3. **I/O Coupling is Intentional**
- Curses library integration is core functionality
- File reading for help/news is primary feature
- User input loops are necessary for interaction
- **Cannot separate UI from I/O in this context**

#### 4. **Low Business Logic Density**
- ~1,800 lines of code
- Only ~30 lines of extractable calculations (1.6%)
- **98.4% of code is presentation/I/O**
- ROI for extraction is very low

---

## Comparison with update.c

| Metric | update.c | forms.c |
|--------|----------|---------|
| **File Size** | 2,632 lines | 1,786 lines |
| **Extractable Functions** | 18 functions | 4 functions |
| **Extractable Tests** | 54-74 tests | 16-18 tests |
| **Business Logic %** | ~30% | ~1.6% |
| **Extraction Effort** | 6-7 hours | 1.5 hours |
| **ROI Rating** | ⭐⭐⭐⭐⭐ | ⭐⭐ |
| **Priority** | High | Low |

---

## Recommendations

### For This File (forms.c)

**Option A: Extract Minimal Calculations (Recommended)**
- Extract 4 identified calculation functions
- Create 16-18 unit tests
- Effort: 1.5 hours
- Benefit: Improved testability of revolt/cost formulas
- **When**: After higher-priority files completed

**Option B: Defer Entirely (Acceptable)**
- Skip extraction for this file
- Focus on business logic files first
- Return to forms.c only if time permits after Phase 10.2.2 complete
- **Rationale**: Minimal ROI, presentation layer appropriate as-is

**Option C: Architectural Refactoring (Future Work)**
- Break down `change()` function (470 lines) into smaller handlers
- Extract UI components for reusability
- Improve test isolation through dependency injection
- **Scope**: Beyond Phase 10.2.2 - requires architectural changes
- **Recommendation**: Defer to Phase 10.3 or later

---

### For Phase 10.2.2 Strategy

**Revised File Priority Order**:
1. ⭐⭐⭐⭐⭐ **update.c** - 18 functions, 54-74 tests (DONE - analyzed)
2. ⭐⭐⭐⭐⭐ **combat.c** - Likely high extraction potential (analyze next)
3. ⭐⭐⭐⭐ **navy.c** - Naval calculations likely extractable
4. ⭐⭐⭐⭐ **move.c** - Movement calculations likely extractable
5. ⭐⭐⭐ **randeven.c** - Random event probability calculations
6. ⭐⭐⭐ **extcmds.c** - Unknown (requires analysis)
7. ⭐⭐⭐ **cexecute.c** - Unknown (requires analysis)
8. ⭐⭐ **forms.c** - Minimal extraction value (THIS FILE)

**Strategic Insight**: Focus extraction efforts on **game engine files** (update, combat, navy, move) rather than **presentation layer files** (forms, display).

---

## Risk Assessment

### Extraction Risks for forms.c

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Breaking UI layout | Low | Medium | Calculations don't affect display logic |
| Test maintenance overhead | Medium | Low | Only 16-18 tests, manageable |
| Unclear benefit | High | Low | Extraction provides minimal value |
| Time waste | High | Medium | Defer to focus on higher-value files |

### Success Factors

✅ **If Extracted**:
1. Simple mathematical formulas (easy to test)
2. Clear input/output semantics
3. No display dependencies
4. Minimal risk of regression

❌ **Arguments Against Extraction**:
1. Low quantity of extractable logic
2. Minimal improvement to testability
3. Presentation layer is working well as-is
4. Better ROI available in other files

---

## Conclusion

### Overall Assessment: ⭐⭐ LOW PRIORITY

`forms.c` represents a **well-designed presentation layer** with appropriate separation of concerns. The file correctly focuses on UI responsibilities with minimal embedded business logic.

### Key Decision Point:

**Extraction is POSSIBLE but NOT VALUABLE** for this file. The 4 extractable functions provide minimal ROI compared to business logic files like update.c, combat.c, and navy.c.

### Recommendations:

1. **Defer forms.c extraction** until higher-priority files complete
2. **Focus Phase 10.2.2 efforts** on game engine files (combat, navy, move)
3. **Consider extraction only if** Phase 10.2.2 has remaining capacity after core files
4. **Treat as optional** - not critical to phase success

### If Time Permits After Core Files:

Extract revolt risk and combat cost calculations (~1.5 hours) for:
- Improved testability of political/economic formulas
- Educational value as examples of clean calculation extraction
- Completeness of modernization effort

**But this is LOW PRIORITY and should be done LAST.**

---

**Report Generated**: 2025-10-10
**Next Steps**: Analyze combat.c, navy.c, move.c (high-priority game engine files)
**Recommendation**: **SKIP forms.c extraction** unless all higher-priority files exhausted

---

**Analysis Completed By**: Claude (claude-sonnet-4-5@20250929)
**Session**: Phase 10.2.2 Planning - Pure Function Extraction Assessment
