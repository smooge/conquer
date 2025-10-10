# Phase 10.2.2 - Pure Function Extraction Analysis Report

## File: combat.c

**Analysis Date**: 2025-10-10
**Analyzer**: Claude (claude-sonnet-4-5@20250929)
**Phase**: 10.2.2 - Pure Function Extraction
**File Size**: 2,092 lines
**Total Functions**: 7 major functions

---

## Executive Summary

**Overall Assessment**: ⭐⭐⭐⭐ **GOOD extraction potential**

`combat.c` is the core combat resolution system with **significant pure calculation opportunities** embedded within complex battle orchestration. The file contains sophisticated combat mechanics with extractable mathematical formulas for loss calculation, odds adjustment, and battle outcomes. Most extraction candidates are pure mathematical functions currently buried in large orchestration functions.

**Key Findings**:
- **8-12 pure functions** can be extracted
- **24-36 unit tests** potential
- **Complex mathematical formulas** ideal for isolation and testing
- **Battle mechanics** - critical calculations need verification
- **Integration complexity** - orchestration functions appropriately complex

**Strategic Insight**: This file has **high-value extraction opportunities** in combat calculation logic, but the main orchestration functions (`combat()`, `fight()`, `navalcbt()`) are appropriately designed as complex integrators. Focus extraction on **embedded mathematical formulas** rather than refactoring the orchestrators.

---

## File Overview

### Purpose
Combat resolution system providing:
- Army battle mechanics and resolution
- Naval combat with ship capture/sinking
- Loss calculation with bell curve dice mechanics
- Retreat destination calculation
- Unit combat bonus determination

### Current Architecture
**Pattern**: Large orchestration functions with embedded pure calculations
- **3 orchestration functions** (combat, fight, navalcbt - appropriately complex)
- **4 utility functions** (cbonus, fdxyretreat, retreat, capture)
- **1 formatting function** (show_ships)
- Mix of pure calculations and state modification

### Documentation Status
✅ **Excellent** - All functions comprehensively documented in Phase 3
- Testing categories assigned (A/B for most functions)
- Algorithm descriptions complete
- Combat mechanics well explained

---

## Extraction Candidates (High Quality)

### 🥇 Priority 1: Battle Loss Calculation Formulas

#### 1. `calculate_battle_losses()` - Combat Loss Percentage Calculation

**Location**: `fight()` function, lines 634-655
**Status**: ⭐⭐⭐⭐⭐ **PERFECT** extraction candidate
**Category**: Pure mathematical calculation

**Current Implementation**:
```c
/* calculate loss for an even battle */
PDloss = MAXLOSS * roll / 100;
PAloss = MAXLOSS * (100 - roll) / 100;

/* adjust for odds */
if (odds == 1) {
    PDloss = 0;
    PAloss = 200;
} else if (odds == 10000) {
    PAloss = 0;
    PDloss = 200;
} else if (odds > 100) {
    PDloss += (odds / 12 - 8);     /* 8.33% for higher odds */
    PAloss -= (odds / 16 - 6);     /* 6.25% for lower odds */
    if (PAloss < (100 - roll) / 20)
        PAloss = (100 - roll) / 20; /* can't get too small */
} else {
    PAloss += (800 / odds - 8);    /* 8% for higher odds */
    PDloss -= (600 / odds - 6);    /* 6% for lower odds */
    if (PDloss < roll / 20)
        PDloss = roll / 20;        /* can't get too small */
}
```

**Extracted Function Signature**:
```c
/*
 * calculate_battle_losses - Calculate percentage losses for both sides
 *
 * Implements the core combat loss calculation using bell curve dice roll
 * and odds-based adjustments. Base losses determined by random roll, then
 * modified by relative strength ratios with minimum loss floors.
 *
 * Parameters:
 *   roll - Bell curve dice roll (5d21-5) range 0-100
 *   odds - Combat odds (attacker_strength * 100 / defender_strength)
 *   max_loss - Maximum loss percentage constant (MAXLOSS)
 *
 * Returns:
 *   Structure containing attacker_loss_pct and defender_loss_pct
 *
 * Formula Logic:
 *   - Base: Symmetric losses from roll (50 roll = 50/50 split)
 *   - Overwhelming odds (1 or 10000): 200% loss for weaker side
 *   - Favorable odds (>100): Increased enemy losses, reduced own losses
 *   - Unfavorable odds (<100): Increased own losses, reduced enemy losses
 *   - Loss floors prevent unrealistically low casualties
 *
 * Testing: 10+ tests covering all odds ranges and edge cases
 */
struct battle_losses {
    int attacker_loss_pct;
    int defender_loss_pct;
};
struct battle_losses calculate_battle_losses(int roll, int odds, int max_loss);
```

**Test Cases**:
1. Even odds (100): Symmetric losses based on roll
2. Overwhelming attacker (10000): 0% attacker, 200% defender
3. Overwhelming defender (1): 200% attacker, 0% defender
4. Favorable odds (200): Reduced attacker losses, increased defender
5. Unfavorable odds (50): Increased attacker losses, reduced defender
6. High roll + high odds: Verify loss floor mechanics
7. Low roll + low odds: Verify loss floor mechanics
8. Boundary: odds = 100 (transition point)
9. Boundary: odds = 101 (first favorable)
10. Boundary: odds = 99 (first unfavorable)

**Extraction Value**: ⭐⭐⭐⭐⭐ (Core combat balance mechanic)

---

#### 2. `calculate_naval_losses()` - Naval Combat Loss Calculation

**Location**: `navalcbt()` function, lines 1552-1579
**Status**: ⭐⭐⭐⭐⭐ **PERFECT** extraction candidate
**Category**: Pure mathematical calculation (identical pattern to army combat)

**Current Implementation**: Nearly identical to army battle losses but with slightly different adjustment factors

**Extracted Function Signature**:
```c
/*
 * calculate_naval_losses - Calculate naval battle percentage losses
 *
 * Similar to calculate_battle_losses but with naval-specific adjustment factors.
 * Naval combat has different loss scaling due to ship capture mechanics.
 *
 * Parameters:
 *   roll - Bell curve dice roll (5d21-5) range 0-100
 *   odds - Combat odds (attacker_crew * 100 / defender_crew)
 *
 * Returns:
 *   Structure containing attacker_loss_pct and defender_loss_pct
 *
 * Differences from Land Combat:
 *   - Odds > 100: PDloss += (odds/10 - 10), PAloss -= (odds/25 - 4)
 *   - Odds < 100: PAloss += (1000/odds - 10), PDloss -= (400/odds - 4)
 *   - Naval combat: Higher loss increments (10% vs 8.33%)
 *   - Naval combat: Different floor divisors (5 vs 20)
 *
 * Testing: 10+ tests mirroring army combat tests with naval factors
 */
struct battle_losses calculate_naval_losses(int roll, int odds);
```

**Test Cases**: Mirror army combat tests with naval-specific parameters

**Extraction Value**: ⭐⭐⭐⭐⭐ (Naval balance critical)

---

### 🥈 Priority 2: Odds Calculation Functions

#### 3. `calculate_battle_odds()` - Combat Odds from Strength

**Location**: `fight()` function, lines 485-490, 627-632 (used twice with different inputs)
**Status**: ⭐⭐⭐⭐ **EXCELLENT** extraction candidate

**Current Implementation**:
```c
if (asold > dsold * 100)
    odds = 10000;
else if (dsold > asold * 100)
    odds = 1;
else
    odds = safe_long_to_int((asold * 100) / dsold);
```

**Extracted Function Signature**:
```c
/*
 * calculate_battle_odds - Calculate combat odds ratio from strengths
 *
 * Computes odds ratio (attacker * 100 / defender) with overflow protection.
 * Caps extreme odds at 10000:100 (100:1) and 1:100 to prevent divide-by-zero
 * and maintain reasonable calculation ranges.
 *
 * Parameters:
 *   attacker_strength - Total attacking force strength (soldiers or crew)
 *   defender_strength - Total defending force strength (soldiers or crew)
 *
 * Returns:
 *   Odds ratio (attacker * 100 / defender)
 *   10000 if attacker > defender * 100 (overwhelming attack)
 *   1 if defender > attacker * 100 (overwhelming defense)
 *
 * Testing: 5 tests for edge cases and typical ratios
 */
int calculate_battle_odds(long attacker_strength, long defender_strength);
```

**Test Cases**:
1. Even strength (1000 vs 1000): odds = 100
2. Overwhelming attack (10000 vs 50): odds = 10000 (capped)
3. Overwhelming defense (50 vs 10000): odds = 1 (capped)
4. Moderate advantage (2000 vs 1000): odds = 200
5. Slight disadvantage (900 vs 1000): odds = 90

**Extraction Value**: ⭐⭐⭐⭐

---

#### 4. `calculate_naval_capture_percentage()` - Ship Capture Probability

**Location**: `navalcbt()` function, lines 1522-1540
**Status**: ⭐⭐⭐⭐⭐ **PERFECT** extraction candidate
**Category**: Complex piecewise formula - **CRITICAL** for testing

**Current Implementation**:
```c
/* calculate capture percentages */
/*
 *  This formula produces:
 *        0% capture for   1:100  odds
 *        2% capture for   1:10   odds
 *       15% capture for   1:1    odds
 *       60% capture for  10:1    odds
 *      100% capture for >60:1    odds
 *      with linear progression between each.
 */
if (odds > 6000) {
    dcptpct = 0;
    acptpct = 100;
} else if (odds > 1000) {
    dcptpct = (6000 - odds) / 2500;
    acptpct = (odds - 1000) / 125 + 60;
} else if (odds > 100) {
    dcptpct = (1000 - odds) / 69 + 2;
    acptpct = (odds - 100) / 20 + 15;
} else if (odds > 10) {
    dcptpct = (100 - odds) / 2 + 15;
    acptpct = (odds - 10) / 7 + 2;
} else if (odds > 6) {
    dcptpct = (10 - odds) * 14 + 60;
    acptpct = (odds - 6) / 2;
} else {
    dcptpct = 100;
    acptpct = 0;
}
```

**Extracted Function Signature**:
```c
/*
 * calculate_naval_capture_percentage - Determine ship capture probabilities
 *
 * Implements a piecewise linear function mapping combat odds to capture
 * probability for both sides. Higher odds increase victor's capture rate
 * while reducing enemy capture chances.
 *
 * Parameters:
 *   odds - Combat odds ratio (attacker * 100 / defender)
 *
 * Returns:
 *   Structure with attacker_capture_pct and defender_capture_pct
 *
 * Capture Probability Curve:
 *   - odds > 6000: attacker 100%, defender 0%
 *   - odds 1000-6000: attacker 60-100%, defender 0-2%
 *   - odds 100-1000: attacker 15-60%, defender 2-15%
 *   - odds 10-100: attacker 2-15%, defender 15-60%
 *   - odds 6-10: attacker 0-2%, defender 60-100%
 *   - odds <= 6: attacker 0%, defender 100%
 *
 * Testing: 12+ tests covering all 6 piecewise regions + boundaries
 */
struct capture_percentages {
    int attacker_capture_pct;
    int defender_capture_pct;
};
struct capture_percentages calculate_naval_capture_percentage(int odds);
```

**Test Cases**:
1. Odds = 10000: 100% attacker, 0% defender
2. Odds = 6000: Transition point upper bound
3. Odds = 5000: Mid-range attacker advantage
4. Odds = 1000: Transition to moderate advantage
5. Odds = 500: Mid-range moderate advantage
6. Odds = 100: Even odds (15% each)
7. Odds = 50: Mid-range disadvantage
8. Odds = 10: Transition to severe disadvantage
9. Odds = 8: Mid-range severe disadvantage
10. Odds = 6: Lower transition point
11. Odds = 4: Overwhelming defender advantage
12. Odds = 1: 0% attacker, 100% defender

**Extraction Value**: ⭐⭐⭐⭐⭐ (Complex formula - **MUST** be tested!)

---

### 🥉 Priority 3: Combat Bonus Calculation

#### 5. `calculate_terrain_bonus()` - Terrain Combat Modifier

**Location**: `cbonus()` function, lines 1009-1020
**Status**: ⭐⭐⭐⭐ **EXCELLENT** extraction candidate

**Current Implementation**:
```c
if (side[num] == DFND) {
    if (sct[xspot][yspot].altitude == MOUNTAIN)
        armbonus += 20;
    else if (sct[xspot][yspot].altitude == HILL)
        armbonus += 10;

    if (sct[xspot][yspot].vegetation == JUNGLE)
        armbonus += 20;
    else if (sct[xspot][yspot].vegetation == FOREST)
        armbonus += 15;
    else if (sct[xspot][yspot].vegetation == WOOD)
        armbonus += 10;
}
```

**Extracted Function Signature**:
```c
/*
 * calculate_terrain_bonus - Compute defensive terrain bonuses
 *
 * Defenders gain combat bonuses from favorable terrain. Mountains, hills,
 * and dense vegetation provide tactical advantages for defensive positions.
 *
 * Parameters:
 *   altitude - Terrain altitude type (MOUNTAIN, HILL, CLEAR, etc.)
 *   vegetation - Vegetation type (JUNGLE, FOREST, WOOD, etc.)
 *   is_defender - Boolean (1=defender gets bonus, 0=no terrain bonus)
 *
 * Returns:
 *   Terrain combat bonus percentage (0 if attacker, 0-40 if defender)
 *
 * Altitude Bonuses (defenders only):
 *   - MOUNTAIN: +20%
 *   - HILL: +10%
 *   - Other: 0%
 *
 * Vegetation Bonuses (defenders only):
 *   - JUNGLE: +20%
 *   - FOREST: +15%
 *   - WOOD: +10%
 *   - Other: 0%
 *
 * Bonuses stack (max +40% for mountain jungle)
 *
 * Testing: 8+ tests for terrain combinations
 */
int calculate_terrain_bonus(int altitude, int vegetation, int is_defender);
```

**Test Cases**:
1. Attacker (any terrain): 0% bonus
2. Defender mountain clear: +20%
3. Defender hill clear: +10%
4. Defender clear jungle: +20%
5. Defender clear forest: +15%
6. Defender clear wood: +10%
7. Defender mountain jungle: +40% (stacked max)
8. Defender hill forest: +25% (stacked)

**Extraction Value**: ⭐⭐⭐⭐

---

#### 6. `calculate_fortification_bonus()` - Fort Defense Modifier

**Location**: `cbonus()` function, lines 1034-1085
**Status**: ⭐⭐⭐ **GOOD** extraction candidate

**Current Implementation**: Complex conditional logic for fort bonuses based on unit type, ownership, and combat status

**Extracted Function Signature**:
```c
/*
 * calculate_fortification_bonus - Fort combat bonuses and penalties
 *
 * Fortifications provide complex bonuses/penalties based on unit type,
 * ownership, combat role, and status. Defenders benefit more than attackers,
 * with unit-specific modifiers (cavalry penalty, archer bonus, etc.).
 *
 * Parameters:
 *   fort_value - Fortification strength (0 = none, 1-N = strength)
 *   is_defender - Combat role (1=defender, 0=attacker)
 *   owns_sector - Sector ownership (1=unit's nation, 0=enemy/neutral)
 *   unit_type - Army unit type (A_CAVALRY, A_ARCHER, A_ZOMBIE, etc.)
 *   unit_status - Unit status (GARRISON, MILITIA, SIEGED, SORTIE, etc.)
 *   has_sapper_magic - Sapper magic ability (1=yes, 0=no)
 *
 * Returns:
 *   Fortification bonus/penalty percentage
 *
 * Complex Rules:
 *   - Defenders in owned forts: +fort_value (full bonus)
 *   - Zombies in owned forts: +fort_value/2 (half bonus)
 *   - Cavalry/Knights in any fort: -20% penalty
 *   - Archers in owned forts: +15% bonus
 *   - Archers in enemy forts: +5% bonus
 *   - Sappers attacking forts: +10% bonus
 *   - Status modifiers: GARRISON/MILITIA/SIEGED enable fort bonus
 *
 * Testing: 10+ tests for unit type and ownership combinations
 */
int calculate_fortification_bonus(int fort_value, int is_defender, int owns_sector,
                                   int unit_type, int unit_status, int has_sapper_magic);
```

**Test Cases**:
1. No fort: 0% bonus
2. Defender in owned fort (normal unit): +fort_value
3. Defender in owned fort (zombie): +fort_value/2
4. Defender cavalry in fort: base + (-20%)
5. Defender archer in owned fort: base + 15%
6. Defender archer in enemy fort: +5%
7. Attacker with sapper vs fort: +10%
8. Defender garrison status: enables fort bonus
9. Defender militia status: enables fort bonus
10. Defender sieged status: enables fort bonus

**Extraction Value**: ⭐⭐⭐

---

### ⚠️ Priority 4: Retreat Mechanics

#### 7. `calculate_retreat_probability()` - Should Unit Retreat?

**Location**: `fight()` function, lines 664-672
**Status**: ⭐⭐⭐ **GOOD** extraction candidate

**Current Implementation**:
```c
retreatside = 0;

if ((PDloss > 2 * PAloss) && (odds > 150)
    && (((PDloss >= 50) && (rand() % 4 == 0)) || (rand() % 8)))
    retreatside = DFND;

if ((PAloss > 2 * PDloss) && (odds < 150)
    && (((PAloss >= 50) && (rand() % 2 == 0)) || (rand() % 6)))
    retreatside = ATKR;
```

**Extracted Function Signature**:
```c
/*
 * calculate_retreat_probability - Determine if side should retreat
 *
 * Complex probabilistic retreat decision based on loss differential,
 * odds ratio, and absolute loss magnitude. Higher losses and worse odds
 * increase retreat probability.
 *
 * Parameters:
 *   attacker_loss_pct - Attacker percentage losses
 *   defender_loss_pct - Defender percentage losses
 *   odds - Combat odds ratio (attacker * 100 / defender)
 *   random_factor_1 - Random value 0-7 (for general retreat chance)
 *   random_factor_2 - Random value 0-3 or 0-5 (for high loss retreat)
 *
 * Returns:
 *   Retreat side indicator:
 *   0 = no retreat
 *   ATKR = attackers retreat
 *   DFND = defenders retreat
 *
 * Retreat Logic:
 *   Defenders retreat if:
 *     - PDloss > 2 * PAloss AND odds > 150 AND
 *     - (PDloss >= 50 AND random < 25%) OR random < 12.5%
 *
 *   Attackers retreat if:
 *     - PAloss > 2 * PDloss AND odds < 150 AND
 *     - (PAloss >= 50 AND random < 50%) OR random < 16.7%
 *
 * Testing: 6+ tests for retreat conditions
 */
int calculate_retreat_probability(int attacker_loss_pct, int defender_loss_pct,
                                   int odds, int random_factor_1, int random_factor_2);
```

**Test Cases**:
1. No retreat: losses balanced
2. Defender retreat: high PDloss, high odds, favorable random
3. Defender no retreat: high PDloss, high odds, unfavorable random
4. Attacker retreat: high PAloss, low odds, favorable random
5. Attacker no retreat: high PAloss, low odds, unfavorable random
6. High losses (>=50%): increased retreat probability

**Extraction Value**: ⭐⭐⭐

---

### 🔧 Priority 5: Utility Calculations

#### 8. `calculate_bell_curve_roll()` - Dice Roll Generator

**Location**: `fight()` line 616-620, `navalcbt()` lines 1547-1551
**Status**: ⭐⭐⭐ **GOOD** extraction candidate (used in both army and naval)

**Current Implementation**:
```c
/* high roll favors attacker [ 5 d21 - 5 ] */
roll = 0;
for (i = 0; i < 5; i++) {
    roll += safe_long_to_int(rand() % 21 + 1);
}
roll -= 5;
```

**Extracted Function Signature**:
```c
/*
 * calculate_bell_curve_roll - Generate bell curve battle outcome roll
 *
 * Produces a bell curve random value by summing 5 twenty-sided dice and
 * subtracting 5 to create range 0-100 with peak at 50. This creates
 * realistic battle outcome distribution favoring moderate results over
 * extreme outcomes.
 *
 * Parameters:
 *   random_values - Array of 5 random values (0-20 each) for deterministic testing
 *                   If NULL, uses rand() for actual gameplay
 *
 * Returns:
 *   Battle roll value (range 0-100, bell curve distribution)
 *   Mean ≈ 50, higher rolls favor attackers
 *
 * Distribution:
 *   - 0-20: Very rare (crushing defeat for attacker)
 *   - 21-40: Uncommon (defender victory)
 *   - 41-60: Common (close battle)
 *   - 61-80: Uncommon (attacker victory)
 *   - 81-100: Very rare (crushing victory for attacker)
 *
 * Testing: 5+ tests including distribution verification
 */
int calculate_bell_curve_roll(const int *random_values);
```

**Test Cases**:
1. All minimum rolls (1,1,1,1,1): result = 0
2. All maximum rolls (21,21,21,21,21): result = 100
3. All average rolls (11,11,11,11,11): result = 50
4. Mixed low rolls: verify correct summation
5. Mixed high rolls: verify correct summation

**Extraction Value**: ⭐⭐⭐

---

## Functions NOT Suitable for Extraction

### `combat()` - Global Combat Orchestrator (lines 157-289)
**Why Not**: Appropriate orchestration function
- Scans entire world map for combat opportunities
- Manages fought tracking matrix
- Calls fight()/navalcbt() for individual battles
- **Primary purpose is coordination**, not calculation
- Heavy I/O (file operations, news reporting)

**Pattern**: Orchestrator function (acceptable architecture)

---

### `fight()` - Army Battle Resolution (lines 386-902)
**Why Not**: Appropriate orchestration function (517 lines)
- Coordinates entire battle flow
- Manages diplomatic side assignment
- Handles unit defection, retreat, casualties
- Generates comprehensive battle reports
- **Primary purpose is battle orchestration**
- Contains extractable calculations (already identified)

**Extractable**: Loss calculation, odds calculation, retreat probability (already identified)
**Recommendation**: Extract embedded calculations, leave orchestration as-is

---

### `navalcbt()` - Naval Battle Resolution (lines 1365-1916)
**Why Not**: Appropriate orchestration function (551 lines)
- Coordinates naval battle flow
- Manages ship-by-ship combat resolution
- Handles capture vs sinking mechanics
- Generates detailed battle reports
- **Primary purpose is naval orchestration**
- Contains extractable calculations (already identified)

**Extractable**: Naval losses, capture percentage (already identified)
**Recommendation**: Extract embedded calculations, leave orchestration as-is

---

### `cbonus()` - Combat Bonus Calculator (lines 991-1101)
**Why Not**: Partial extraction only
- 110-line function with some pure components
- Heavy global state dependencies (unit arrays, terrain, magic system)
- Multiple subsystems integration (terrain, fort, magic, unit type)
- **Some extractable subfunctions** (terrain bonus, fort bonus - already identified)

**Extractable**: Terrain bonus, fortification bonus (already identified)
**Recommendation**: Extract subfunctions, leave main function for integration

---

### `fdxyretreat()` - Retreat Destination Calculator (lines 1161-1196)
**Why Not**: Appropriate map scanning function
- Scans adjacent sectors for retreat options
- Requires full map state and diplomatic relationships
- Integration function (coordinates multiple systems)
- **Primary purpose is pathfinding**, not calculation

**Pattern**: Acceptable integration function

---

### `retreat()` - Execute Retreat Movement (lines 1256-1279)
**Why Not**: Appropriate state modification function
- Moves units to retreat coordinates
- Applies naval retreat casualties
- **Primary purpose is state modification**
- Simple logic, well-structured

**Pattern**: Acceptable state transformation

---

### `capture()` - Ship Capture Distribution (lines 1983-2022)
**Why Not**: Appropriate distribution function
- Weighted ship assignment to victorious fleets
- Requires fleet state access
- **Primary purpose is prize allocation**
- Simple algorithm, not worth extracting

**Pattern**: Acceptable distribution logic

---

### `show_ships()` - Battle Report Formatting (lines 2080-2091)
**Why Not**: Appropriate formatting function
- Simple output formatting
- **Already minimal** (11 lines)
- No business logic to extract

**Pattern**: Acceptable utility function

---

## Extraction Summary

### Total Extraction Potential

| Priority | Functions | Tests | Effort | Target File |
|----------|-----------|-------|--------|-------------|
| 1. Battle Loss Formulas | 2 | 20 | 1.5h | `combat_calculations.c` |
| 2. Odds Calculations | 2 | 17 | 1h | `combat_calculations.c` |
| 3. Combat Bonuses | 2 | 18 | 1.5h | `combat_modifiers.c` |
| 4. Retreat Mechanics | 1 | 6 | 30min | `combat_calculations.c` |
| 5. Utility Functions | 1 | 5 | 20min | `combat_calculations.c` |
| **TOTAL** | **8** | **66** | **4.5h** | **2 new files** |

### Phase 10.2.2 Contribution

**Goal**: 15-25 pure functions, 30-75 tests
**This File**: 8 functions, 66 tests
**Contribution**: Exceeds lower bound alone, strong toward upper bound

---

## Detailed Extraction Plan

### Session 1: Core Battle Calculations (2h)

**Target**: `combat_calculations.c` / `combat_calculations.h`

**Functions to Extract**:
1. `calculate_battle_losses()` - Army loss percentages from roll/odds
2. `calculate_naval_losses()` - Naval loss percentages from roll/odds
3. `calculate_battle_odds()` - Odds ratio from strengths with overflow protection
4. `calculate_naval_capture_percentage()` - Ship capture probabilities (piecewise)
5. `calculate_retreat_probability()` - Should unit retreat? (probabilistic)
6. `calculate_bell_curve_roll()` - Generate 5d21-5 battle roll

**Approach**:
- Create pure functions taking explicit parameters (no global state)
- Return structures for multi-value results (battle_losses, capture_percentages)
- Accept random values as parameters for deterministic testing
- Document formulas with mathematical notation

**Example Signature**:
```c
/*
 * calculate_battle_losses - Core combat loss calculation
 *
 * Parameters:
 *   roll - Bell curve battle outcome (0-100, peak at 50)
 *   odds - Attacker strength * 100 / defender strength (1-10000)
 *   max_loss - Maximum loss constant (MAXLOSS, typically 60-80)
 *
 * Returns:
 *   Structure with attacker_loss_pct and defender_loss_pct (0-200)
 */
struct battle_losses calculate_battle_losses(int roll, int odds, int max_loss);
```

**Tests**: 43 tests (10 + 10 + 5 + 12 + 6)

---

### Session 2: Combat Modifiers (1.5h)

**Target**: `combat_modifiers.c` / `combat_modifiers.h`

**Functions to Extract**:
1. `calculate_terrain_bonus()` - Terrain defense bonuses (altitude + vegetation)
2. `calculate_fortification_bonus()` - Fort bonuses/penalties by unit type

**Approach**:
- Extract complex conditional logic into testable functions
- Pass terrain/unit data as explicit parameters
- Document bonus stacking rules clearly
- Create comprehensive test coverage for all unit type combinations

**Example Signature**:
```c
/*
 * calculate_terrain_bonus - Defensive terrain advantages
 *
 * Parameters:
 *   altitude - MOUNTAIN, HILL, CLEAR, etc.
 *   vegetation - JUNGLE, FOREST, WOOD, etc.
 *   is_defender - 1 if defending, 0 if attacking
 *
 * Returns:
 *   Terrain bonus percentage (0-40, bonuses stack)
 */
int calculate_terrain_bonus(int altitude, int vegetation, int is_defender);
```

**Tests**: 18 tests (8 + 10)

---

### Session 3: Integration & Validation (1h)

**Tasks**:
1. Refactor `fight()` to use extracted battle loss calculations
2. Refactor `navalcbt()` to use extracted naval calculations
3. Refactor `cbonus()` to use extracted modifier calculations
4. Verify zero behavioral change (compare battle outcomes)
5. Run full test suite (verify 66 tests passing)
6. Compilation at warning level 9
7. Git commits

---

## Strategic Analysis

### Why combat.c is Good for Extraction

#### 1. **Complex Mathematical Formulas**
- Loss calculation formulas have multiple branches
- Piecewise linear functions (capture percentage)
- Bell curve probability distributions
- **High bug risk** without comprehensive tests

#### 2. **Critical Game Balance Mechanics**
- Combat outcomes determine territorial control
- Loss calculations affect gameplay balance
- Capture mechanics impact naval warfare economics
- **MUST be verified** for correctness

#### 3. **Embedded Pure Calculations**
- Most calculations are pure math (roll, odds → losses)
- No side effects in calculation logic
- Clear input → output semantics
- **Ready for extraction** with minimal refactoring

#### 4. **High Testing Value**
- Mathematical formulas can be tested exhaustively
- Edge cases are clearly defined (odds boundaries)
- Regression testing prevents balance changes
- **Dramatic improvement** in confidence

---

## Comparison with Other Files

| Metric | update.c | forms.c | navy.c | combat.c |
|--------|----------|---------|--------|----------|
| **File Size** | 2,632 lines | 1,786 lines | 1,223 lines | 2,092 lines |
| **Extractable Functions** | 18 | 4 | 13 | 8 |
| **Extractable Tests** | 54-74 | 16-18 | 104-130 | 66 |
| **Business Logic %** | ~30% | ~1.6% | ~60% | ~15% |
| **Extraction Effort** | 6-7h | 1.5h | 6h | 4.5h |
| **ROI Rating** | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| **Priority** | High | Low | Very High | High |
| **Formula Complexity** | Moderate | Low | High (bit fields) | **High (piecewise)** |

**Key Insight**: combat.c has **fewer extractable functions** than update.c or navy.c, but the functions are **mathematically complex** with high testing value. The piecewise capture percentage formula alone justifies extraction.

---

## Risk Assessment

### Extraction Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Formula transcription errors | Medium | **CRITICAL** | Comprehensive unit tests (66 tests) |
| Behavioral change | Low | High | Verification tests before/after |
| Global state coupling | Low | Medium | Pass parameters explicitly |
| Test maintenance | Medium | Low | Clear test organization, good naming |

### Success Factors

✅ **High Success Probability**:
1. ✅ Formulas are pure calculations (no I/O)
2. ✅ Clear input/output semantics
3. ✅ Well-documented algorithms
4. ✅ **Critical need** for formula verification
5. ✅ Existing documentation provides test guidance
6. ✅ Clean extraction path (pass parameters)

---

## Recommendations

### For This File (combat.c)

**Option A: Full Extraction (STRONGLY RECOMMENDED)**
- Extract all 8 calculation functions
- Create 66 comprehensive unit tests
- **Effort**: 4.5 hours
- **Benefit**: Dramatic improvement in confidence for combat balance
- **Priority**: **High** - complex formulas NEED testing
- **When**: Phase 10.2.2 - Session 3 (after update.c and navy.c)

**Why Prioritize**:
1. **Complex mathematical formulas** - prone to subtle bugs
2. **Critical game balance** - affects core gameplay
3. **High test value** - formulas can be exhaustively tested
4. **Exceeds phase contribution** - strong toward upper bound

---

### Extraction Priority Within File

**Must Extract (P0 - Critical)**:
1. ⭐⭐⭐⭐⭐ **Loss calculation formulas** (2 functions, 20 tests)
   - Core combat balance mechanics
   - Complex piecewise logic
   - **Cannot defer** - critical for correctness

2. ⭐⭐⭐⭐⭐ **Naval capture percentage** (1 function, 12 tests)
   - Extremely complex piecewise formula
   - High bug risk without tests
   - **Cannot defer** - too complex to verify manually

**Should Extract (P1 - High Value)**:
3. ⭐⭐⭐⭐ **Odds calculations** (2 functions, 17 tests)
   - Used in multiple places
   - Overflow protection logic
   - Important for balance

4. ⭐⭐⭐⭐ **Terrain/fortification bonuses** (2 functions, 18 tests)
   - Complex conditional logic
   - Multiple unit type interactions
   - Testability improvement

**Could Extract (P2 - Nice to Have)**:
5. ⭐⭐⭐ **Retreat probability** (1 function, 6 tests)
   - Probabilistic logic
   - Lower priority (simpler)

6. ⭐⭐⭐ **Bell curve roll** (1 function, 5 tests)
   - Simple utility
   - Extract if time permits

---

## Implementation Roadmap

### Phase 10.2.2 Execution for combat.c

#### Pre-work (15 min):
- Create file structure (combat_calculations.c/h, combat_modifiers.c/h)
- Update CMakeLists.txt
- Create test file stubs

#### Session 1: Core Calculations (2h):
1. Extract 6 core calculation functions
2. Write 43 unit tests
3. Verify zero behavioral change
4. Git commit: `[PHASE-10.2.2] EXTRACT: Combat battle calculations with comprehensive tests`

#### Session 2: Modifiers (1.5h):
1. Extract 2 modifier calculation functions
2. Write 18 unit tests
3. Refactor cbonus() to use extracted functions
4. Git commit: `[PHASE-10.2.2] EXTRACT: Combat terrain and fortification modifiers`

#### Session 3: Validation (1h):
1. Run full test suite (verify 66 tests passing)
2. Compilation check (level 9 warnings)
3. Manual smoke testing (run sample battles)
4. Create session memory

---

## Testing Strategy

### Formula Testing Pattern

**Critical Test Categories**:

1. **Boundary Tests** - Verify edge case handling:
```c
TEST(combat_calculations, battle_losses_even_odds) {
    struct battle_losses result = calculate_battle_losses(50, 100, 60);

    ASSERT_EQ(30, result.attacker_loss_pct);  // 60 * 50 / 100
    ASSERT_EQ(30, result.defender_loss_pct);  // 60 * 50 / 100
}
```

2. **Overflow Tests** - Verify extreme odds handling:
```c
TEST(combat_calculations, battle_losses_overwhelming_attacker) {
    struct battle_losses result = calculate_battle_losses(50, 10000, 60);

    ASSERT_EQ(0, result.attacker_loss_pct);    // Capped at 0
    ASSERT_EQ(200, result.defender_loss_pct);  // Maximum destruction
}
```

3. **Piecewise Tests** - Verify all formula regions:
```c
TEST(combat_calculations, naval_capture_high_odds) {
    struct capture_percentages result = calculate_naval_capture_percentage(5000);

    // In range 1000-6000: specific formula applies
    int expected_attacker = (5000 - 1000) / 125 + 60;  // 92%
    int expected_defender = (6000 - 5000) / 2500;      // 0%

    ASSERT_EQ(expected_attacker, result.attacker_capture_pct);
    ASSERT_EQ(expected_defender, result.defender_capture_pct);
}
```

4. **Deterministic Tests** - Use explicit random values:
```c
TEST(combat_calculations, bell_curve_roll_minimum) {
    int dice[5] = {1, 1, 1, 1, 1};
    int result = calculate_bell_curve_roll(dice);

    ASSERT_EQ(0, result);  // 5*1 - 5 = 0
}
```

---

## Conclusion

### Overall Assessment: ⭐⭐⭐⭐ **EXCELLENT**

`combat.c` is a **high-value extraction target** with:
- Complex mathematical formulas requiring comprehensive testing
- Critical game balance calculations needing verification
- Pure calculation logic ready for extraction
- High ROI despite lower function count than other files
- **Exceeds Phase 10.2.2 test goal** with 66 comprehensive tests

### Key Decision Points:

1. **Formula extraction is HIGH PRIORITY** - too complex to verify manually
2. **Focus on mathematical functions** - leave orchestrators as-is
3. **This file should be extracted** in Phase 10.2.2 (after update.c and navy.c)
4. **66 tests are JUSTIFIED** - complex formulas require exhaustive testing

### Strategic Priority:

**Rank**: #3 overall (after update.c and navy.c, before move.c)

**Rationale**:
- Complex piecewise formulas need testing
- Critical combat balance mechanics
- High test value provides maximum confidence
- Clean extraction path with clear benefits

---

**Report Generated**: 2025-10-10
**Recommendation**: **PRIORITIZE** combat.c extraction in Phase 10.2.2 (Session 3)
**Expected Outcome**: 8 extracted functions, 66 tests, dramatically improved confidence in combat formulas

---

**Analysis Completed By**: Claude (claude-sonnet-4-5@20250929)
**Session**: Phase 10.2.2 Planning - Pure Function Extraction Assessment
