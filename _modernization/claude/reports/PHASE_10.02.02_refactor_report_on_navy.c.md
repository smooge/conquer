# Phase 10.2.2 - Pure Function Extraction Analysis Report

## File: navy.c

**Analysis Date**: 2025-10-10
**Analyzer**: Claude (claude-sonnet-4-5@20250929)
**Phase**: 10.2.2 - Pure Function Extraction
**File Size**: 1,223 lines
**Total Functions**: 13 functions

---

## Executive Summary

**Overall Assessment**: ⭐⭐⭐⭐ **GOOD extraction potential**

`navy.c` is a naval fleet management module with **excellent extraction candidates**. The file contains pure calculation functions that are **already well-isolated** and **already documented as "Category A - Unit Testable"**. Most functions perform mathematical calculations without complex I/O dependencies.

**Key Findings**:
- **10-12 pure functions** can be extracted (majority already isolated!)
- **30-36 unit tests** potential
- **Functions already isolated** - minimal refactoring needed
- **Bit manipulation logic** - critical to extract for testability
- **Capacity calculations** - pure mathematical formulas

**Strategic Insight**: This file is **nearly ideal** for extraction - most functions are ALREADY pure calculations with minimal dependencies. Extraction effort is primarily **documentation and test creation** rather than complex refactoring.

---

## File Overview

### Purpose
Naval fleet management providing:
- Ship addition/removal with bit manipulation
- Fleet cargo capacity calculations
- Fleet speed calculations
- Interactive fleet loading/unloading (UI portion)

### Current Architecture
**Pattern**: Mostly pure calculation functions + one large UI function
- **12 calculation functions** (add/sub ships, capacity, speed)
- **1 UI function** (loadfleet - 190 lines of UI/I/O)
- Clean separation between calculations and UI

### Documentation Status
✅ **Excellent** - All functions comprehensively documented in Phase 3
- Testing categories assigned (mostly A - Unit Testable)
- Algorithm descriptions complete
- All marked as "Category A (Unit)" or "Category B (Integration)"

---

## Extraction Candidates (High Quality)

### 🥇 Priority 1: Ship Capacity Calculations (PURE MATH)

#### 1. `flthold()` - Total Fleet Capacity

**Location**: Lines 631-641
**Status**: ⭐⭐⭐⭐⭐ **PERFECT** extraction candidate
**Category**: Already documented as "Category A (Unit)"

```c
/*
 * flthold - Calculate total cargo storage capacity across entire fleet
 *
 * Formula: Σ(size+1) * ship_count for all ship types and sizes
 *   - Light ships (size=0): 1 * count
 *   - Medium ships (size=1): 2 * count
 *   - Heavy ships (size=2): 3 * count
 *
 * Parameters:
 *   nvynum - Fleet number (0 to MAXNAVY-1)
 *
 * Returns:
 *   Total cargo capacity (0 if fleet empty)
 */
int flthold(int nvynum);
```

**Current Implementation**:
```c
int flthold(int nvynum) {
    int i, hold = 0;
    for (i = N_LIGHT; i <= N_HEAVY; i++) {
        hold += (i + 1) * (int)P_NWAR(i);
        hold += (i + 1) * (int)P_NMER(i);
        hold += (i + 1) * (int)P_NGAL(i);
    }
    return (hold);
}
```

**Extraction Status**: **ALREADY PURE** - just needs fleet data as parameter instead of global
**Test Cases**: 6-8 tests
**Extraction Value**: ⭐⭐⭐⭐⭐

---

#### 2. `fltghold()` - Galley Capacity Only

**Location**: Lines 754-761
**Status**: ⭐⭐⭐⭐⭐ **PERFECT** extraction candidate

```c
int fltghold(int nvynum);
```

**Formula**: Same as flthold() but only galleys
**Test Cases**: 4-5 tests
**Extraction Value**: ⭐⭐⭐⭐⭐

---

#### 3. `fltmhold()` - Merchant Capacity Only

**Location**: Lines 813-820
**Status**: ⭐⭐⭐⭐⭐ **PERFECT** extraction candidate

```c
int fltmhold(int nvynum);
```

**Formula**: Same as flthold() but only merchants
**Test Cases**: 4-5 tests
**Extraction Value**: ⭐⭐⭐⭐⭐

---

#### 4. `fltwhold()` - Warship Capacity Only (ADMIN)

**Location**: Lines 694-701
**Status**: ⭐⭐⭐⭐⭐ **PERFECT** extraction candidate

```c
#ifdef ADMIN
int fltwhold(int nvynum);
#endif
```

**Formula**: Same as flthold() but only warships
**Test Cases**: 4-5 tests
**Extraction Value**: ⭐⭐⭐⭐⭐

---

### 🥈 Priority 2: Fleet Speed Calculation

#### 5. `fltspeed()` - Fleet Movement Speed (ADMIN)

**Location**: Lines 559-578
**Status**: ⭐⭐⭐⭐ **EXCELLENT** extraction candidate

```c
/*
 * fltspeed - Calculate fleet speed based on slowest ship
 *
 * Fleet constrained by least maneuverable vessel.
 * Formula: base_speed + (2-size) * N_SIZESPD
 *   - Warships: N_WSPD base
 *   - Merchants: N_MSPD base
 *   - Galleys: N_GSPD base
 *
 * Returns: N_NOSPD if no ships, else lowest speed
 */
unsigned short fltspeed(int nvynum);
```

**Current Implementation**:
```c
unsigned short fltspeed(int nvynum) {
    int i, hold = 99;

    for (i = N_LIGHT; i <= N_HEAVY && hold > N_WSPD; i++)
        if (P_NWAR(i) > 0)
            hold = N_WSPD + (2 - i) * N_SIZESPD;

    for (i = N_LIGHT; i <= N_HEAVY && hold > N_MSPD; i++)
        if (P_NMER(i) > 0)
            hold = N_MSPD + (2 - i) * N_SIZESPD;

    for (i = N_LIGHT; i <= N_HEAVY && hold > N_GSPD; i++)
        if (P_NGAL(i) > 0)
            hold = N_GSPD + (2 - i) * N_SIZESPD;

    if (hold == 99)
        hold = N_NOSPD;
    return (safe_int_to_ushort(hold));
}
```

**Extraction Status**: **ALREADY PURE** - uses only fleet data
**Test Cases**: 8-10 tests (various fleet compositions, empty fleet, mixed types)
**Extraction Value**: ⭐⭐⭐⭐⭐

---

### 🥉 Priority 3: Fleet Ship Count

#### 6. `fltships()` - Total Ship Count

**Location**: Lines 496-508
**Status**: ⭐⭐⭐⭐ **GOOD** extraction candidate

```c
/*
 * fltships - Calculate total number of ships in fleet
 *
 * Sums all ship types and sizes.
 */
int fltships(short nation, int nvynum);
```

**Current Implementation**:
```c
int fltships(short nation, int nvynum) {
    struct s_nation *savntn = curntn;
    int i, hold = 0;

    curntn = &ntn[nation];
    for (i = N_LIGHT; i <= N_HEAVY; i++) {
        hold += (int)P_NWAR(i);
        hold += (int)P_NMER(i);
        hold += (int)P_NGAL(i);
    }
    curntn = savntn;
    return (hold);
}
```

**Issue**: Nation context switching (curntn manipulation)
**Solution**: Extract pure calculation, pass fleet structure directly
**Test Cases**: 5-6 tests
**Extraction Value**: ⭐⭐⭐⭐

---

### ⚠️ Priority 4: Bit Manipulation Functions (CRITICAL for Testing)

These functions manipulate ship counts using bit fields. **CRITICAL** to extract for testing due to complex bit logic.

#### 7-9. Ship Addition Functions

**Functions**:
- `addwships()` - Add warships (lines 74-100)
- `addmships()` - Add merchants (lines 144-170)
- `addgships()` - Add galleys (lines 215-241)

**Status**: ⭐⭐⭐⭐⭐ **CRITICAL** for extraction

**Current Pattern**:
```c
int addwships(int nvynum, int shipsize, int nships) {
    int hold = nships;

    // Validation
    if (nvynum < 0 || nvynum >= MAXNAVY || shipsize < N_LIGHT || shipsize > N_HEAVY)
        return (FALSE);

    // Overflow check
    hold += P_NWAR(shipsize);
    if (hold > N_MASK)
        return (FALSE);

    // Bit manipulation
    hold <<= (N_BITSIZE * shipsize);
    P_NWSHP |= safe_int_to_ushort(hold);
    hold |= ~(N_MASK << (N_BITSIZE * shipsize));
    P_NWSHP &= safe_int_to_ushort(hold);

    return (TRUE);
}
```

**Why Extract**:
- **Complex bit manipulation** - easy to break, hard to verify manually
- **Boundary conditions** - overflow, underflow, bit field isolation
- **High bug risk** - bit errors are subtle and dangerous
- **Already documented** as "Category A (Unit)"

**Extraction Approach**:
Extract to pure functions that take/return bit field values:
```c
/*
 * calculate_ship_bitfield_add - Add ships to bit field encoding
 *
 * Parameters:
 *   current_bitfield - Current bit field value
 *   shipsize - Ship size class (0=light, 1=medium, 2=heavy)
 *   nships - Number of ships to add
 *   max_per_size - Maximum ships per size (N_MASK)
 *   bitsize - Bits per size class (N_BITSIZE)
 *
 * Returns:
 *   New bitfield value, or -1 on overflow
 */
int calculate_ship_bitfield_add(unsigned short current_bitfield,
                                int shipsize, int nships,
                                int max_per_size, int bitsize);
```

**Test Cases**: 12-15 tests (each function)
- Valid additions (various sizes)
- Overflow conditions
- Bit field isolation (verify other sizes unchanged)
- Boundary values
- Multiple sequential additions

**Extraction Value**: ⭐⭐⭐⭐⭐ (CRITICAL - complex bit logic)

---

#### 10-12. Ship Subtraction Functions

**Functions**:
- `subwships()` - Remove warships (lines 286-311)
- `submships()` - Remove merchants (lines 357-381)
- `subgships()` - Remove galleys (lines 427-452)

**Status**: ⭐⭐⭐⭐⭐ **CRITICAL** for extraction

**Same Pattern**: Bit manipulation with underflow protection

**Test Cases**: 12-15 tests (each function)
- Valid removals
- Underflow protection (prevent negative counts)
- Bit field isolation
- Sequential operations

**Extraction Value**: ⭐⭐⭐⭐⭐ (CRITICAL - complex bit logic)

---

## Functions NOT Suitable for Extraction

### `loadfleet()` - Fleet Loading UI (lines 1032-1222)

**Why Not**: Large interactive UI function (190 lines)
- Heavy I/O (user input, screen display)
- Complex state modifications (fleet cargo, army status, sector population)
- Extensive validation and error messaging
- **Primary purpose is interaction**, not calculation

**Pattern**: UI orchestration (acceptable for presentation layer)

**Possible Minor Extractions**:
Could extract small validation functions, but ROI is low:
- `loadstat()` (lines 934-947) - Already extracted as utility
- Capacity availability calculations - Already use fltghold()/fltmhold()

**Recommendation**: Leave as-is (UI function appropriate as-is)

---

### `get_cargo()` - Cargo Type Selection (lines 863-883)

**Why Not**: UI helper function
- User input processing
- Screen display
- **Already appropriately sized** (20 lines)

**Status**: Acceptable as-is

---

### `loadstat()` - Status Validation (lines 934-947)

**Status**: ⭐⭐⭐ **GOOD** extraction candidate (already isolated!)

**Already Pure**: Simple switch statement validation
```c
int loadstat(int status) {
    switch (status) {
        case TRADED:
        case GENERAL:
        case MILITIA:
        case GARRISON:
        case ONBOARD:
            return (FALSE);
        default:
            break;
    }
    return (TRUE);
}
```

**Test Cases**: 3-4 tests (prohibited statuses, valid statuses)
**Extraction Value**: ⭐⭐⭐ (Simple utility, low effort)

---

## Extraction Summary

### Total Extraction Potential

| Priority | Functions | Tests | Effort | Target File |
|----------|-----------|-------|--------|-------------|
| 1. Capacity Calculations | 4 | 16-20 | 1h | `naval_calculations.c` |
| 2. Fleet Speed | 1 | 8-10 | 30min | `naval_calculations.c` |
| 3. Fleet Ship Count | 1 | 5-6 | 20min | `naval_calculations.c` |
| 4. Bit Manipulation (Add) | 3 | 36-45 | 2h | `ship_bitfield.c` |
| 5. Bit Manipulation (Sub) | 3 | 36-45 | 2h | `ship_bitfield.c` |
| 6. Status Validation | 1 | 3-4 | 15min | `naval_validation.c` |
| **TOTAL** | **13** | **104-130** | **6h** | **3 new files** |

### Phase 10.2.2 Contribution

**Goal**: 15-25 pure functions, 30-75 tests
**This File**: 13 functions, 104-130 tests
**Contribution**: **Exceeds goal alone** (amazing!)

---

## Detailed Extraction Plan

### Session 1: Capacity & Speed Functions (1.5h)

**Target**: `naval_calculations.c` / `naval_calculations.h`

**Functions to Extract**:
1. `calculate_fleet_total_capacity()` - From flthold()
2. `calculate_fleet_warship_capacity()` - From fltwhold()
3. `calculate_fleet_merchant_capacity()` - From fltmhold()
4. `calculate_fleet_galley_capacity()` - From fltghold()
5. `calculate_fleet_speed()` - From fltspeed()
6. `calculate_fleet_ship_count()` - From fltships()

**Approach**:
- Create pure functions taking fleet ship arrays as parameters
- Remove global curntn dependencies
- Pass ship counts directly

**Example Signature**:
```c
/*
 * calculate_fleet_total_capacity - Compute total cargo capacity
 *
 * Parameters:
 *   warships - Array of warship counts [light, medium, heavy]
 *   merchants - Array of merchant counts [light, medium, heavy]
 *   galleys - Array of galley counts [light, medium, heavy]
 *
 * Returns:
 *   Total capacity (sum of (size+1) * count for all ships)
 */
int calculate_fleet_total_capacity(const int warships[3],
                                   const int merchants[3],
                                   const int galleys[3]);
```

**Tests**: 16-20 capacity tests, 8-10 speed tests, 5-6 count tests

---

### Session 2: Bit Manipulation Functions (4h)

**Target**: `ship_bitfield.c` / `ship_bitfield.h`

**Critical Importance**: These functions use complex bit manipulation that is **extremely error-prone** and **difficult to verify manually**. Unit tests are **essential** for confidence.

**Functions to Extract**:

##### Addition Operations:
1. `bitfield_add_ships()` - Generic bit field addition
2. `bitfield_get_ship_count()` - Extract count from bit field
3. `bitfield_validate_overflow()` - Check addition overflow

##### Subtraction Operations:
4. `bitfield_sub_ships()` - Generic bit field subtraction
5. `bitfield_validate_underflow()` - Check subtraction underflow

**Example Signatures**:
```c
/*
 * bitfield_add_ships - Add ships to bit field encoding
 *
 * Bit field layout: [heavy: N_BITSIZE bits][medium: N_BITSIZE bits][light: N_BITSIZE bits]
 *
 * Parameters:
 *   current_field - Current bit field value
 *   size_class - Ship size (0=light, 1=medium, 2=heavy)
 *   count_to_add - Ships to add
 *
 * Returns:
 *   New bit field value, or -1 on overflow
 *
 * Testing: 15+ tests for boundary conditions, bit isolation, overflow
 */
int bitfield_add_ships(unsigned short current_field, int size_class, int count_to_add);

/*
 * bitfield_get_ship_count - Extract ship count for specific size
 *
 * Parameters:
 *   bitfield - Encoded bit field
 *   size_class - Ship size to extract (0-2)
 *
 * Returns:
 *   Ship count for that size class
 *
 * Testing: 6+ tests for all sizes, edge cases
 */
int bitfield_get_ship_count(unsigned short bitfield, int size_class);

/*
 * bitfield_validate_overflow - Check if addition would overflow
 *
 * Parameters:
 *   current_count - Current ships in size class
 *   count_to_add - Ships to add
 *   max_ships - Maximum per size (N_MASK)
 *
 * Returns:
 *   TRUE if addition valid, FALSE if overflow
 */
int bitfield_validate_overflow(int current_count, int count_to_add, int max_ships);
```

**Tests**: 72-90 tests total
- **Bit field addition**: 15 tests × 3 ship types = 45 tests
  - Valid additions (each size class)
  - Overflow conditions
  - Bit field isolation (verify other fields unchanged)
  - Sequential additions
  - Maximum values

- **Bit field subtraction**: 15 tests × 3 ship types = 45 tests
  - Valid subtractions
  - Underflow protection
  - Bit field isolation
  - Sequential subtractions
  - Boundary values

**Why So Many Tests?**:
- Bit manipulation is **extremely fragile**
- Subtle bugs can corrupt entire fleet data
- Boundary conditions are critical
- Each ship type needs full test coverage
- **Cannot afford bugs in bit manipulation**

---

### Session 3: Integration & Validation (30min)

**Tasks**:
1. Refactor original functions to use extracted calculations
2. Verify zero behavioral change
3. Run full test suite
4. Compilation at warning level 9
5. Git commits

---

## Strategic Analysis

### Why navy.c is Excellent for Extraction

#### 1. **Functions Already Pure**
- Most calculation functions have no I/O
- Minimal global state dependencies
- Clean input → calculation → output pattern
- **Already documented as "Unit Testable"**

#### 2. **Critical Testing Value**
- **Bit manipulation** - extremely error-prone, MUST be tested
- **Capacity calculations** - critical for game balance
- **Speed calculations** - affects movement mechanics
- **High bug risk** without tests

#### 3. **Clean Separation Already Exists**
- Calculations isolated from UI (loadfleet)
- Each function has single responsibility
- Minimal coupling between functions
- **Ready for extraction** with minimal refactoring

#### 4. **High ROI**
- ~6 hours effort for 13 functions
- 104-130 comprehensive tests
- **Dramatically improves confidence** in bit manipulation
- Prevents future bugs in critical fleet logic

---

## Comparison with Other Files

| Metric | update.c | forms.c | navy.c |
|--------|----------|---------|---------|
| **File Size** | 2,632 lines | 1,786 lines | 1,223 lines |
| **Extractable Functions** | 18 | 4 | 13 |
| **Extractable Tests** | 54-74 | 16-18 | 104-130 |
| **Business Logic %** | ~30% | ~1.6% | ~60% |
| **Extraction Effort** | 6-7h | 1.5h | 6h |
| **ROI Rating** | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Priority** | High | Low | **Very High** |
| **Bit Manipulation** | No | No | **YES** (critical!) |

**Key Insight**: navy.c has **highest test-to-function ratio** (8-10 tests per function) due to bit manipulation complexity. This is **extremely valuable** for confidence.

---

## Risk Assessment

### Extraction Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Bit manipulation bugs | Low | **CRITICAL** | Comprehensive unit tests (72-90 tests) |
| Global state coupling | Low | Medium | Pass fleet data as parameters |
| Behavioral change | Very Low | Medium | Verification tests before/after |
| Test maintenance | Medium | Low | Clear test organization, good naming |

### Success Factors

✅ **Extremely High Success Probability**:
1. ✅ Functions already documented as "Unit Testable"
2. ✅ Clean calculation logic (no I/O)
3. ✅ Single responsibility per function
4. ✅ Minimal global state dependencies
5. ✅ **Critical need** for bit manipulation tests
6. ✅ Clear extraction path (pass fleet data directly)

---

## Recommendations

### For This File (navy.c)

**Option A: Full Extraction (STRONGLY RECOMMENDED)**
- Extract all 13 calculation functions
- Create 104-130 comprehensive unit tests
- **Effort**: 6 hours
- **Benefit**: Dramatic improvement in confidence for critical fleet logic
- **Priority**: **Very High** - bit manipulation NEEDS testing
- **When**: Phase 10.2.2 - Session 2 (after update.c)

**Why Prioritize**:
1. **Bit manipulation is critical** - cannot afford bugs
2. **Functions already isolated** - minimal refactoring needed
3. **High test value** - 8-10 tests per function average
4. **Exceeds phase goal alone** - 13 functions vs 15-25 goal

---

### Extraction Priority Within File

**Must Extract (P0 - Critical)**:
1. ⭐⭐⭐⭐⭐ **Bit manipulation functions** (6 functions, 72-90 tests)
   - Highest risk without tests
   - Complex logic prone to subtle bugs
   - **Cannot defer** - critical for confidence

**Should Extract (P1 - High Value)**:
2. ⭐⭐⭐⭐⭐ **Capacity calculations** (4 functions, 16-20 tests)
   - Pure math, easy to test
   - Critical for game balance
   - Already isolated

3. ⭐⭐⭐⭐ **Speed/Count calculations** (2 functions, 13-16 tests)
   - Straightforward formulas
   - Important for gameplay
   - Quick wins

**Could Extract (P2 - Nice to Have)**:
4. ⭐⭐⭐ **Status validation** (1 function, 3-4 tests)
   - Simple utility
   - Low effort, low value
   - Extract if time permits

---

## Implementation Roadmap

### Phase 10.2.2 Execution for navy.c

#### Pre-work (15 min):
- Create file structure (naval_calculations.c/h, ship_bitfield.c/h)
- Update CMakeLists.txt
- Create test file stubs

#### Session 1: Capacity & Speed (1.5h):
1. Extract 6 calculation functions
2. Write 29-36 unit tests
3. Verify zero behavioral change
4. Git commit: `[PHASE-10.2.2] EXTRACT: Naval capacity and speed calculations`

#### Session 2: Bit Manipulation (4h):
1. Extract bit field helper functions (5 functions)
2. Refactor ship add/sub functions to use helpers
3. Write 72-90 comprehensive unit tests
4. **Extensive verification** (bit manipulation is critical!)
5. Git commit: `[PHASE-10.2.2] EXTRACT: Ship bit field manipulation with comprehensive tests`

#### Session 3: Validation (30min):
1. Run full test suite (verify 104-130 tests passing)
2. Compilation check (level 9 warnings)
3. Manual smoke testing
4. Create session memory

---

## Testing Strategy

### Bit Manipulation Test Pattern

**Critical Test Categories**:

1. **Isolation Tests** - Verify changing one size doesn't affect others:
```c
TEST(ship_bitfield, add_light_preserves_medium_and_heavy) {
    unsigned short field = encode_ships(0, 5, 3); // 0 light, 5 medium, 3 heavy
    field = bitfield_add_ships(field, N_LIGHT, 2);

    ASSERT_EQ(2, bitfield_get_ship_count(field, N_LIGHT));
    ASSERT_EQ(5, bitfield_get_ship_count(field, N_MEDIUM)); // unchanged
    ASSERT_EQ(3, bitfield_get_ship_count(field, N_HEAVY));  // unchanged
}
```

2. **Overflow Tests** - Verify N_MASK limits enforced:
```c
TEST(ship_bitfield, add_ships_rejects_overflow) {
    unsigned short field = encode_ships(N_MASK, 0, 0);
    int result = bitfield_add_ships(field, N_LIGHT, 1);

    ASSERT_EQ(-1, result); // overflow rejected
}
```

3. **Underflow Tests** - Verify subtraction protection:
```c
TEST(ship_bitfield, sub_ships_prevents_negative) {
    unsigned short field = encode_ships(3, 0, 0);
    field = bitfield_sub_ships(field, N_LIGHT, 5);

    ASSERT_EQ(3, bitfield_get_ship_count(field, N_LIGHT)); // unchanged (underflow prevented)
}
```

4. **Sequential Operations** - Verify multiple operations:
```c
TEST(ship_bitfield, sequential_operations_maintain_integrity) {
    unsigned short field = 0;
    field = bitfield_add_ships(field, N_LIGHT, 5);
    field = bitfield_add_ships(field, N_MEDIUM, 3);
    field = bitfield_sub_ships(field, N_LIGHT, 2);

    ASSERT_EQ(3, bitfield_get_ship_count(field, N_LIGHT));
    ASSERT_EQ(3, bitfield_get_ship_count(field, N_MEDIUM));
    ASSERT_EQ(0, bitfield_get_ship_count(field, N_HEAVY));
}
```

---

## Conclusion

### Overall Assessment: ⭐⭐⭐⭐⭐ **EXCELLENT**

`navy.c` is an **ideal extraction target** with:
- Clean separation between calculations and UI
- Functions already isolated and pure
- **Critical need** for bit manipulation testing
- High test value (8-10 tests per function average)
- **Exceeds Phase 10.2.2 goal alone** (13 functions, 104-130 tests)

### Key Decision Points:

1. **Bit manipulation extraction is NON-NEGOTIABLE** - too risky without tests
2. **Capacity/speed extraction is high value** - pure math, easy wins
3. **This file should be extracted EARLY** in Phase 10.2.2 (right after update.c)
4. **High test count is JUSTIFIED** - bit manipulation requires comprehensive testing

### Strategic Priority:

**Rank**: #2 overall (after update.c, before combat.c)

**Rationale**:
- Functions already isolated (minimal refactoring)
- Critical bit manipulation needs testing
- High test count provides maximum confidence
- Clean extraction path with clear benefits

---

**Report Generated**: 2025-10-10
**Recommendation**: **PRIORITIZE** navy.c extraction in Phase 10.2.2 (Session 2)
**Expected Outcome**: 13 extracted functions, 104-130 tests, dramatically improved confidence in fleet logic

---

**Analysis Completed By**: Claude (claude-sonnet-4-5@20250929)
**Session**: Phase 10.2.2 Planning - Pure Function Extraction Assessment
