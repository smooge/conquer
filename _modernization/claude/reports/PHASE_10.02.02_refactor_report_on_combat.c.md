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


---

**Phase**: 10.2.2 - Deep Refactoring and Architectural Debt Analysis
**File**: combat.c (2092 lines, 11 functions)
**Purpose**: Complete 3-part architectural assessment for refactoring prioritization

---

## Document Purpose and Methodology

This document provides a **comprehensive architectural debt analysis** for `combat.c`, examining three critical dimensions:

1. **Architectural Placement** - Does this code belong in this file?
2. **Configuration Coupling** - How many magic numbers need externalization?
3. **Extraction Potential** - Which functions should be extracted for testing?

**Analysis Approach**:
- Complete file read (2092 lines)
- Function-by-function architectural review
- Magic number inventory and categorization
- Pure function identification for extraction
- Effort estimation with test coverage planning
- Priority ranking based on risk and ROI

---

## PART 1: ARCHITECTURAL PLACEMENT ANALYSIS

### 1.1 File Overview

**File**: `combat.c` (2092 lines)
**Primary Responsibility**: Combat system and battle resolution
**Functions**: 11 total

| Function | Lines | Purpose |
|----------|-------|---------|
| `combat()` | 132 | Global combat orchestration and battle detection |
| `fight()` | 516 | Army battle resolution engine |
| `cbonus()` | 110 | Individual unit combat bonus calculation |
| `fdxyretreat()` | 36 | Calculate retreat destination |
| `retreat()` | 23 | Execute unit retreats |
| `navalcbt()` | 551 | Naval battle resolution engine |
| `capture()` | 40 | Distribute captured ships |
| `show_ships()` | 12 | Format naval battle results |

### 1.2 Architectural Cohesion Assessment

**Question**: Does all code in combat.c belong in a combat system file?

#### ✅ CORRECTLY PLACED CODE (100% - All Functions)

**Combat Orchestration**: 1 function
- `combat()` - Global combat detection and battle initiation
  - **Rationale**: Core combat system responsibility
  - **Placement**: PERFECT - Top-level combat orchestration

**Army Combat System**: 4 functions
- `fight()` - Army battle resolution with complex mechanics
- `cbonus()` - Unit combat effectiveness calculation
- `fdxyretreat()` - Retreat destination calculation
- `retreat()` - Retreat execution
  - **Rationale**: All army combat mechanics belong together
  - **Placement**: PERFECT - Complete army combat subsystem

**Naval Combat System**: 3 functions
- `navalcbt()` - Naval battle resolution
- `capture()` - Ship capture distribution
- `show_ships()` - Battle report formatting
  - **Rationale**: All naval combat mechanics belong together
  - **Placement**: PERFECT - Complete naval combat subsystem

**Analysis**: **0% misplaced code** - combat.c demonstrates **PERFECT architectural cohesion**

#### Summary Statistics

| Category | Functions | Lines | Percentage |
|----------|-----------|-------|------------|
| **Correctly Placed** | **11** | **~2092** | **100%** |
| **Misplaced Code** | **0** | **0** | **0%** |

### 1.3 Architectural Quality Rating

**Architectural Placement**: ⭐⭐⭐⭐⭐ **PERFECT** (0% misplaced code)

**Rationale**:
- Combat.c is a **textbook example of single-responsibility design**
- All 11 functions directly support combat resolution
- Clear separation: Army combat vs Naval combat
- Shared utilities (retreat, bonus calculation) properly scoped
- No business logic leakage into other domains
- No UI/presentation code mixed with combat mechanics

**Comparison with Other Files**:

| File | Misplaced Code | Rating | Assessment |
|------|----------------|--------|------------|
| combat.c | 0% | ⭐⭐⭐⭐⭐ | PERFECT |
| update.c | 0% | ⭐⭐⭐⭐⭐ | PERFECT |
| forms.c | 0% | ⭐⭐⭐⭐⭐ | PERFECT |
| navy.c | 0% | ⭐⭐⭐⭐⭐ | PERFECT |
| admin.c | 29% | ⭐⭐ | POOR |

**Conclusion**: combat.c demonstrates **excellent architectural discipline** with zero architectural debt from misplaced code.

---

## PART 2: CONFIGURATION COUPLING ANALYSIS

### 2.1 Magic Number Inventory

**Methodology**: Scan all 11 functions for hardcoded numeric constants, formula coefficients, and game balance parameters.

#### 2.1.1 Combat Mechanics Constants

**Battle Resolution Formulas** (15+ constants)

1. **Dice Roll System**:
   - `5` - Number of dice rolls for bell curve
   - `21` - Die size (d21)
   - `-5` - Die roll offset for centering

2. **Loss Calculation Odds Modifiers**:
   - `12` - Divisor for defender loss odds adjustment (line 646)
   - `8` - Defender loss constant adjustment (line 646)
   - `16` - Divisor for attacker loss odds adjustment (line 647)
   - `6` - Attacker loss constant adjustment (line 647)
   - `20` - Minimum loss divisor for attacker (line 648-649)
   - `800` - Attacker loss odds multiplier (line 651)
   - `600` - Defender loss odds multiplier (line 652)

3. **Extreme Odds Handling**:
   - `1` - Minimum odds (complete defender victory)
   - `10000` - Maximum odds (complete attacker victory)
   - `100` - Odds threshold for "even battle"
   - `200` - Attacker percentage loss on total defeat (line 641)
   - `0` - Defender percentage loss on total victory (line 640)

4. **Fortification Damage Multiplier**:
   - `120` - Fortification combat damage multiplier (20% increase)
   - `100` - Baseline damage percentage (line 660-661)

5. **Retreat Triggers**:
   - `2` - Retreat threshold ratio (2x loss differential)
   - `150` - Odds threshold for retreat consideration
   - `50` - High percentage loss threshold for forced retreat
   - `15` - Additional loss penalty for blocked retreat (line 679, 681)
   - `4` - Defender retreat probability divisor (rand % 4)
   - `8` - Attacker retreat probability divisor (rand % 8)
   - `2` - Alternative retreat probability (rand % 2)
   - `6` - Alternative retreat probability (rand % 6)

**Naval Combat Mechanics** (20+ constants)

6. **Naval Crew Strength Multipliers**:
   - `1` - Warship crew base strength (implicit 1x)
   - `2` - Galley crew strength multiplier (2x base)
   - `4` - Merchant crew strength multiplier (4x base)
   - `3/2` - Archer/Sailor embarked strength (1.5x)
   - `3` - Marine embarked strength (3x)
   - `3/4` - Default embarked strength (0.75x)

7. **Naval Magic Bonuses**:
   - `5/4` - Sailor magic crew bonus (25% increase)

8. **Naval Capture Percentages** (complex formula):
   - `6000` - Maximum capture odds threshold
   - `100` - Baseline capture percentage
   - `1000` - Mid-range odds threshold
   - `60` - High odds base capture rate (line 1527)
   - `125` - High odds capture divisor (line 1527)
   - `2500` - Defender capture divisor (line 1526)
   - `69` - Mid-range defender capture divisor (line 1529)
   - `2` - Low-mid defender capture constant (line 1529)
   - `20` - Mid-range attacker capture divisor (line 1530)
   - `15` - Mid-range attacker capture base (line 1530)
   - `7` - Low odds capture divisor (line 1533)
   - `14` - Very low odds defender multiplier (line 1535)
   - `6` - Minimum odds threshold (line 1534)
   - `10` - Very low odds threshold (line 1531)

9. **Naval Loss Calculation Modifiers**:
   - `10` - Naval defender loss percentage increase (line 1566)
   - `25` - Naval attacker loss percentage decrease (line 1567)
   - `5` - Naval minimum loss divisor (line 1568-1569)
   - `1000` - Naval attacker loss multiplier (line 1571)
   - `400` - Naval defender loss multiplier (line 1572)

10. **Ship Combat Probabilities**:
    - `2` - Warship capture divisor (50% capture rate) (line 1609)
    - `3` - Galley/Merchant destruction divisor (33% chance) (line 1703, 1791)
    - `90` - High loss forced destruction threshold (line 1632, 1704, 1791)
    - `100` - Full loss percentage ceiling

#### 2.1.2 Unit Combat Bonus Constants

**Terrain Bonuses** (7 constants)
- `20` - Mountain defender bonus (line 1011)
- `10` - Hill defender bonus (line 1013)
- `20` - Jungle defender bonus (line 1016)
- `15` - Forest defender bonus (line 1018)
- `10` - Wood defender bonus (line 1020)
- `30` - Ice/Desert magic terrain bonus (line 1005)

**Unit Status Modifiers** (7 constants)
- `20` - Army group bonus (NUMSTATUS+) (line 1008)
- `30` - Magical defense bonus (line 1028)
- `30` - Magical attack bonus (line 1050)
- `-40` - March status penalty (line 1073)
- `-30` - Sortie defense penalty (line 1030)
- `-20` - Sieged status penalty (line 1032)
- `-20` - Cavalry/Knight in fortress penalty (line 1079)

**Fortification Effects** (6 constants)
- `2` - Zombie fort utilization divisor (50% effectiveness) (line 1037)
- `10` - Sapper attack bonus vs fortifications (line 1043)
- `10` - Sortie base attack bonus (line 1053)
- `15` - Archer fort bonus (owned) (line 1082)
- `5` - Archer fort bonus (enemy) (line 1084)
- **Multiple sortie bonuses**:
  - `5` - Dragoon/Legion/Phalanx organization bonus (line 1056)
  - `10` - Light Cavalry/Cavalry mounted bonus (line 1059)
  - `15` - Avian/Elephant/Knight elite mounted bonus (line 1062)
  - `5` - Monster fear bonus (line 1066)

**Formation Size Bonuses** (3 constants)
- `1000` - Phalanx/Legion large formation threshold (line 1094)
- `20` - Large formation bonus (line 1095)
- `500` - Medium formation threshold (line 1096)
- `10` - Medium formation bonus (line 1097)

#### 2.1.3 Retreat and Casualty Constants

**Mercenary/Orc/Goblin Mechanics** (6 constants)
- `15` - Pre-battle refusal probability (line 435)
- `30` - Mid-battle desertion probability (line 499)
- `7/10` - Refusal retreat casualty rate (30% loss) (line 450-451)
- `8/10` - Refusal retreat casualty rate (20% loss) (line 454-455)
- `1/4` - Desertion capitol retreat casualty (75% loss) (line 518)
- `1/2` - Desertion normal retreat casualty (50% loss) (line 521)

**Unit Survival Thresholds** (2 constants)
- `25` - Minimum army size to survive battle (line 721, 748)
- `80` - Leader death threshold when ruling (line 737)

**Naval Retreat Casualties** (1 constant)
- `85/100` - Marine/Sailor retreat casualties (15% loss) (line 1269-1270)

#### 2.1.4 Fortification Siege Mechanics

**Siege Weapon Bonuses** (8 constants)
- `20` - Catapult crew per 1% defending bonus (line 579)
- `10` - Maximum catapult defending bonus (line 579)
- `40` - Catapult crew per 1% attacking bonus (line 583)
- `10` - Maximum catapult attacking bonus (line 583, 608)
- `20` - Siege engine crew per 1% attack bonus (line 595)
- `30` - Maximum siege engine bonus (line 595)
- `2` - Catapult fortress damage probability multiplier (line 586)
- `15` - Siege engine damage base probability (implicit /2) (line 598)

#### 2.1.5 Data Structure Constants

**Combat Participant Limits** (2 constants)
- `MGKNUM` (32) - Maximum units in battle (line 31)
- `MAXARM` - Maximum armies per nation
- `MAXNAVY` - Maximum navies per nation

**Combat Type Flags** (5 constants)
- `COMBAT_X` (0) - No combat occurred
- `COMBAT_A` (1) - Army combat possible
- `COMBAT_N` (2) - Naval combat possible
- `FOUGHT_A` (4) - Army combat completed
- `FOUGHT_N` (8) - Naval combat completed

**Combat Side Identifiers** (4 constants)
- `ATKR` (2) - Attacker side
- `DFND` (1) - Defender side
- `NTRL` (0) - Neutral side
- `WIMP` (3) - Retreating side identifier

**Ship Type Identifiers** (3 constants)
- `QWAR` (1) - Warship type
- `QGAL` (2) - Galley type
- `QMER` (3) - Merchant type

### 2.2 Configuration Categories Summary

| Category | Constants | Extraction Priority |
|----------|-----------|---------------------|
| **Battle Resolution Formulas** | 15+ | P0 CRITICAL |
| **Naval Combat Mechanics** | 20+ | P0 CRITICAL |
| **Unit Combat Bonuses** | 20+ | P1 HIGH |
| **Retreat Mechanics** | 9 | P2 MEDIUM |
| **Siege Mechanics** | 8 | P2 MEDIUM |
| **Data Structure Constants** | 14 | P3 LOW (header.h) |

**Total Magic Numbers**: **86+ game balance constants** (excluding data structure constants)

### 2.3 Configuration Coupling Rating

**Configuration Coupling**: ⭐⭐⭐⭐⭐ **CATASTROPHIC** (86+ game balance constants)

**Severity Assessment**:

| File | Magic Numbers | Rating | Severity |
|------|---------------|--------|----------|
| **combat.c** | **86+** | **⭐⭐⭐⭐⭐** | **CATASTROPHIC** |
| update.c | 61+ | ⭐⭐⭐⭐⭐ | SEVERE |
| admin.c | 50+ | ⭐⭐⭐⭐ | HIGH |
| forms.c | 22 | ⭐⭐ | LOW |
| navy.c | 4 | ⭐ | VERY LOW |

**Critical Discovery**: **combat.c has THE HIGHEST configuration coupling in the entire codebase**

**Why This Is Critical**:

1. **Game Balance Tuning**:
   - 86+ constants control core combat mechanics
   - Changing any constant requires rebuilding entire project
   - No ability to tune combat balance without recompilation
   - Impossible to create combat variants or mods

2. **Testing Challenges**:
   - Cannot test combat formulas in isolation
   - Hardcoded constants make unit testing extremely difficult
   - No way to verify formula correctness without integration tests
   - Combat balance bugs are nearly impossible to debug

3. **Historical Context**:
   - Original game designed for single static balance
   - No consideration for moddability or experimentation
   - Formula tuning required source code access
   - Balance changes = full rebuild + distribution

4. **Maintainability Issues**:
   - Formula documentation scattered throughout code
   - No central reference for combat mechanics
   - Difficult to understand combat balance holistically
   - Changes in one area may have cascading effects

### 2.4 Recommended Configuration Externalization

**Priority 1: Create `game_balance.h` - Combat Mechanics Section**

```c
/* ========== COMBAT MECHANICS CONFIGURATION ========== */

/* Battle Resolution - Dice System */
#define COMBAT_DICE_COUNT 5
#define COMBAT_DIE_SIZE 21
#define COMBAT_DIE_OFFSET -5

/* Battle Resolution - Loss Calculation */
#define COMBAT_DEFENDER_LOSS_DIVISOR 12
#define COMBAT_DEFENDER_LOSS_CONSTANT 8
#define COMBAT_ATTACKER_LOSS_DIVISOR 16
#define COMBAT_ATTACKER_LOSS_CONSTANT 6
#define COMBAT_MIN_LOSS_DIVISOR 20

/* Battle Resolution - Extreme Odds */
#define COMBAT_MIN_ODDS 1
#define COMBAT_MAX_ODDS 10000
#define COMBAT_EVEN_ODDS_THRESHOLD 100
#define COMBAT_TOTAL_DEFEAT_LOSS 200
#define COMBAT_TOTAL_VICTORY_LOSS 0

/* Fortification Effects */
#define COMBAT_FORT_DAMAGE_MULTIPLIER 120
#define COMBAT_FORT_BASELINE_DAMAGE 100

/* Retreat Mechanics */
#define COMBAT_RETREAT_THRESHOLD_RATIO 2
#define COMBAT_RETREAT_ODDS_THRESHOLD 150
#define COMBAT_RETREAT_HIGH_LOSS_THRESHOLD 50
#define COMBAT_BLOCKED_RETREAT_PENALTY 15

/* Naval Combat - Crew Strength */
#define NAVAL_WARSHIP_CREW_MULTIPLIER 1
#define NAVAL_GALLEY_CREW_MULTIPLIER 2
#define NAVAL_MERCHANT_CREW_MULTIPLIER 4
#define NAVAL_ARCHER_SOLDIER_MULTIPLIER 3/2
#define NAVAL_MARINE_SOLDIER_MULTIPLIER 3
#define NAVAL_DEFAULT_SOLDIER_MULTIPLIER 3/4

/* Naval Combat - Magic Bonuses */
#define NAVAL_SAILOR_MAGIC_BONUS 5/4

/* Naval Combat - Capture Percentages */
#define NAVAL_MAX_CAPTURE_ODDS 6000
#define NAVAL_HIGH_CAPTURE_BASE 60
#define NAVAL_MID_CAPTURE_BASE 15
// ... (20+ more naval constants)

/* Unit Combat Bonuses - Terrain */
#define TERRAIN_MOUNTAIN_BONUS 20
#define TERRAIN_HILL_BONUS 10
#define TERRAIN_JUNGLE_BONUS 20
#define TERRAIN_FOREST_BONUS 15
#define TERRAIN_WOOD_BONUS 10
#define TERRAIN_ICE_DESERT_MAGIC_BONUS 30

/* Unit Combat Bonuses - Status */
#define STATUS_ARMY_GROUP_BONUS 20
#define STATUS_MAGDEF_BONUS 30
#define STATUS_MAGATT_BONUS 30
#define STATUS_MARCH_PENALTY -40
#define STATUS_SORTIE_DEFENSE_PENALTY -30
#define STATUS_SIEGED_PENALTY -20

/* Fortification Bonuses */
#define FORT_ZOMBIE_DIVISOR 2
#define FORT_SAPPER_BONUS 10
#define FORT_SORTIE_BASE_BONUS 10
#define FORT_ARCHER_OWNED_BONUS 15
#define FORT_ARCHER_ENEMY_BONUS 5
#define FORT_CAVALRY_PENALTY -20

/* Sortie Bonuses by Unit Type */
#define SORTIE_ORGANIZATION_BONUS 5
#define SORTIE_MOUNTED_BONUS 10
#define SORTIE_ELITE_MOUNTED_BONUS 15
#define SORTIE_MONSTER_BONUS 5

/* Formation Size Bonuses */
#define FORMATION_LARGE_THRESHOLD 1000
#define FORMATION_LARGE_BONUS 20
#define FORMATION_MEDIUM_THRESHOLD 500
#define FORMATION_MEDIUM_BONUS 10

/* Mercenary Mechanics */
#define MERC_REFUSAL_PROBABILITY 15
#define MERC_DESERTION_PROBABILITY 30
#define MERC_REFUSAL_CASUALTY_SEVERE 30
#define MERC_REFUSAL_CASUALTY_MODERATE 20
#define MERC_DESERTION_CASUALTY_CAPITOL 75
#define MERC_DESERTION_CASUALTY_NORMAL 50

/* Unit Survival */
#define COMBAT_MIN_ARMY_SIZE 25
#define COMBAT_LEADER_DEATH_THRESHOLD 80

/* Siege Mechanics */
#define SIEGE_CATAPULT_DEFEND_CREW_PER_BONUS 20
#define SIEGE_CATAPULT_DEFEND_MAX_BONUS 10
#define SIEGE_CATAPULT_ATTACK_CREW_PER_BONUS 40
#define SIEGE_CATAPULT_ATTACK_MAX_BONUS 10
#define SIEGE_ENGINE_CREW_PER_BONUS 20
#define SIEGE_ENGINE_MAX_BONUS 30
#define SIEGE_CATAPULT_DAMAGE_MULTIPLIER 2
#define SIEGE_ENGINE_DAMAGE_BASE_PROBABILITY 15
```

**Impact**: Single source of truth for all 86+ combat mechanics constants

---

## PART 3: EXTRACTION POTENTIAL ANALYSIS

### 3.1 Pure Function Identification

**Methodology**: Identify functions that can be extracted as pure, testable calculation functions.

#### 3.1.1 CRITICAL PRIORITY - Battle Resolution Formulas (P0)

**Function 1: `calculate_combat_dice_roll()`**
```c
/*
 * Pure function: Generate bell curve combat dice roll
 * Returns: Roll value (0-100 range)
 */
int calculate_combat_dice_roll(void);
```

**Extraction Benefits**:
- Test bell curve distribution properties
- Verify statistical properties of combat randomness
- Validate expected value and variance
- Ensure fair combat outcomes

**Testing Requirements**:
- **Statistical distribution tests** (10 tests)
  - Verify 5d21-5 produces correct range (0-100)
  - Test mean value converges to ~50
  - Validate standard deviation
  - Check for uniform die distribution
  - Verify bell curve shape
- **Edge case tests** (4 tests)
  - Minimum possible roll (5*1-5 = 0)
  - Maximum possible roll (5*21-5 = 100)
  - Repeated roll consistency
  - Random seed independence
- **Integration tests** (2 tests)
  - Integration with combat loss calculation
  - Historical roll distribution analysis

**Total Tests**: 16 tests
**Estimated Effort**: 1.5-2 hours (straightforward statistical function)

---

**Function 2: `calculate_combat_odds(long attacker_strength, long defender_strength)`**
```c
/*
 * Pure function: Calculate combat odds ratio
 * Returns: Odds value (1 to 10000 range)
 */
int calculate_combat_odds(long attacker_strength, long defender_strength);
```

**Extraction Benefits**:
- Isolate odds calculation formula
- Test extreme strength ratios
- Validate 100:1 clamping logic
- Ensure no division by zero

**Testing Requirements**:
- **Odds calculation tests** (12 tests)
  - Even strength (1:1 = 100 odds)
  - Attacker advantage (10:1 = 1000 odds)
  - Defender advantage (1:10 = 10 odds)
  - Extreme attacker (100:1 = 10000 max)
  - Extreme defender (1:100 = 1 min)
  - Zero defender (max odds)
  - Zero attacker (should error or min odds)
  - Large numbers (overflow protection)
  - Quality vs quantity scenarios
  - 2:1, 3:1, 5:1 standard ratios
  - Fractional advantages (3:2, 4:3)
  - Historical odds distribution

**Total Tests**: 12 tests
**Estimated Effort**: 1-1.5 hours

---

**Function 3: `calculate_percentage_loss(int base_roll, int odds, int is_attacker, int in_fortification)`**
```c
/*
 * Pure function: Calculate percentage casualties for combat side
 * Returns: Percentage loss (0-100)
 */
int calculate_percentage_loss(int base_roll, int odds, int is_attacker, int in_fortification);
```

**Extraction Benefits**:
- **CRITICAL**: Most complex combat formula in the system
- Test all odds adjustment branches
- Validate fortification damage multiplier
- Ensure loss caps at 100%
- Verify minimum loss calculations

**Testing Requirements**:
- **Even battle tests** (8 tests)
  - Roll 50, odds 100 (baseline)
  - Roll 0 (minimum roll)
  - Roll 100 (maximum roll)
  - Fortification effect (20% increase)
  - Attacker vs defender loss asymmetry
  - MAXLOSS scaling verification
  - Loss percentage bounds (0-100%)
  - Statistical fairness across rolls

- **Advantage scenarios** (10 tests)
  - Odds > 100 (attacker advantage)
  - Odds < 100 (defender advantage)
  - Odds = 1 (extreme defender victory)
  - Odds = 10000 (extreme attacker victory)
  - Odds 150, 200, 500 (various advantages)
  - Minimum loss floor enforcement
  - Maximum loss ceiling enforcement
  - Odds adjustment formula correctness
  - Loss differential calculation
  - Asymmetric adjustment verification

- **Fortification tests** (4 tests)
  - 120% damage multiplier application
  - Fortification in even battle
  - Fortification with attacker advantage
  - Fortification with defender advantage

- **Edge cases** (6 tests)
  - Zero odds handling
  - Negative roll protection
  - Roll > 100 clamping
  - Overflow protection for large numbers
  - Loss never exceeds 100%
  - Loss never goes negative

**Total Tests**: 28 tests
**Estimated Effort**: 3-4 hours (COMPLEX - most important combat formula)

**Priority Justification**: This is the **CORE COMBAT FORMULA** that determines all battle outcomes. Testing this function is **CRITICAL** for:
- Verifying game balance correctness
- Preventing combat bugs that break gameplay
- Enabling future balance tuning with confidence
- Documenting intended combat behavior

---

**Function 4: `calculate_naval_capture_percentage(int odds, int is_attacker)`**
```c
/*
 * Pure function: Calculate ship capture probability based on combat odds
 * Returns: Capture percentage (0-100)
 */
int calculate_naval_capture_percentage(int odds, int is_attacker);
```

**Extraction Benefits**:
- Isolate complex piecewise linear capture formula
- Test all odds thresholds (6000, 1000, 100, 10, 6)
- Validate capture rate progression
- Ensure symmetric attacker/defender formulas

**Testing Requirements**:
- **Threshold tests** (14 tests)
  - Odds > 6000 (100% attacker, 0% defender)
  - Odds = 6000 (boundary case)
  - Odds 1000-6000 range (linear progression)
  - Odds = 1000 (boundary case)
  - Odds 100-1000 range
  - Odds = 100 (15% capture baseline)
  - Odds 10-100 range
  - Odds = 10 (boundary case)
  - Odds 6-10 range
  - Odds = 6 (boundary case)
  - Odds < 6 (0% attacker, 100% defender)
  - Attacker capture progression
  - Defender capture progression
  - Symmetry verification

- **Formula verification** (6 tests)
  - Divisor correctness (2500, 125, 69, 20, 7, 14)
  - Constant correctness (60, 15, 2)
  - Piecewise continuity at boundaries
  - Linear interpolation correctness
  - Percentage bounds (0-100%)
  - Historical balance validation

**Total Tests**: 20 tests
**Estimated Effort**: 2-3 hours (complex piecewise formula)

---

**Function 5: `calculate_naval_loss_percentage(int base_roll, int odds, int is_attacker)`**
```c
/*
 * Pure function: Calculate naval combat loss percentage
 * Returns: Percentage loss (0-100)
 */
int calculate_naval_loss_percentage(int base_roll, int odds, int is_attacker);
```

**Extraction Benefits**:
- Test naval-specific loss calculation
- Validate different divisors (10, 25, 5) vs army combat
- Ensure proper odds adjustment
- Verify loss caps

**Testing Requirements**:
- **Naval formula tests** (10 tests)
  - Even odds baseline
  - Odds > 100 (naval-specific adjustments)
  - Odds < 100 (naval-specific adjustments)
  - Extreme odds (1, 10000)
  - Minimum loss floor (roll/5)
  - Maximum loss ceiling (100%)
  - Attacker vs defender loss differential
  - Roll variation (0, 50, 100)
  - Divisor verification (10, 25, 400, 1000)
  - Comparison with army combat formula

**Total Tests**: 10 tests
**Estimated Effort**: 1.5-2 hours

---

#### 3.1.2 HIGH PRIORITY - Combat Bonus Calculations (P1)

**Function 6: `calculate_terrain_bonus(int altitude, int vegetation, int is_defender)`**
```c
/*
 * Pure function: Calculate terrain combat bonus
 * Returns: Percentage bonus (0-40)
 */
int calculate_terrain_bonus(int altitude, int vegetation, int is_defender);
```

**Extraction Benefits**:
- Test all terrain type bonuses
- Validate defender-only terrain advantages
- Ensure proper stacking (altitude + vegetation)
- Document terrain combat mechanics

**Testing Requirements**:
- **Altitude tests** (4 tests)
  - Mountain (+20% defender, 0% attacker)
  - Hill (+10% defender, 0% attacker)
  - Plain (0% all)
  - Water (0% all - not applicable to land combat)

- **Vegetation tests** (6 tests)
  - Jungle (+20% defender, 0% attacker)
  - Forest (+15% defender, 0% attacker)
  - Wood (+10% defender, 0% attacker)
  - Desert (0% base, magic bonus separate)
  - Ice (0% base, magic bonus separate)
  - Plain (0% all)

- **Stacking tests** (4 tests)
  - Mountain + Jungle (20+20 = 40% max defender bonus)
  - Hill + Forest (10+15 = 25%)
  - Mountain + Plain (20+0 = 20%)
  - Attacker always gets 0% terrain bonus

**Total Tests**: 14 tests
**Estimated Effort**: 1-1.5 hours

---

**Function 7: `calculate_status_bonus(int status, int is_attacker)`**
```c
/*
 * Pure function: Calculate unit status combat modifier
 * Returns: Percentage bonus (can be negative)
 */
int calculate_status_bonus(int status, int is_attacker);
```

**Extraction Benefits**:
- Test all status types (MARCH, MAGDEF, MAGATT, SORTIE, SIEGED, etc.)
- Validate attacker vs defender status effects
- Ensure negative penalties apply correctly
- Document status modifier mechanics

**Testing Requirements**:
- **Status modifier tests** (12 tests)
  - MARCH (-40% all)
  - MAGDEF (+30% defender, 0% attacker)
  - MAGATT (+30% attacker, 0% defender)
  - SORTIE (-30% defender, varies for attacker)
  - SIEGED (-20% defender)
  - GARRISON (0% base, fort bonus separate)
  - MILITIA (0% base, fort bonus separate)
  - RULE (0% base)
  - Army group (>=NUMSTATUS, +20%)
  - ATTACK (0% base)
  - DEFEND (0% base)
  - SCOUT (0% base)

**Total Tests**: 12 tests
**Estimated Effort**: 1-1.5 hours

---

**Function 8: `calculate_fortification_bonus(int fort_value, int owner_match, int unit_type, int status, int is_attacker)`**
```c
/*
 * Pure function: Calculate fortification combat bonus
 * Returns: Percentage bonus (can be negative for cavalry)
 */
int calculate_fortification_bonus(int fort_value, int owner_match, int unit_type, int status, int is_attacker);
```

**Extraction Benefits**:
- Test fortress combat mechanics
- Validate unit type interactions (cavalry penalty, archer bonus)
- Ensure ownership requirements for bonuses
- Test zombie half-effectiveness

**Testing Requirements**:
- **Defender fortification tests** (10 tests)
  - Owned fort, regular unit (full fort bonus)
  - Owned fort, zombie unit (half fort bonus)
  - Owned fort, cavalry/knight (-20% penalty + fort)
  - Owned fort, archer (+15% bonus + fort)
  - Enemy fort, archer (+5% bonus)
  - Enemy fort, regular unit (0% bonus)
  - No fort, all units (0% bonus)
  - GARRISON status in owned fort
  - MILITIA status in owned fort
  - SIEGED status in owned fort

- **Attacker fortification tests** (4 tests)
  - Attacking fort, regular unit (0% base)
  - Attacking fort, cavalry/knight (-20% penalty)
  - Attacking fort, sapper magic (+10% bonus)
  - No fort, all units (0% bonus)

**Total Tests**: 14 tests
**Estimated Effort**: 2-2.5 hours

---

**Function 9: `calculate_sortie_bonus(int unit_type, int fort_owned)`**
```c
/*
 * Pure function: Calculate sortie attack bonus by unit type
 * Returns: Percentage bonus (0-25)
 */
int calculate_sortie_bonus(int unit_type, int fort_owned);
```

**Extraction Benefits**:
- Test sortie mechanics for all unit types
- Validate stacking bonuses (base + unit type)
- Ensure owned fortress requirement
- Document sortie tactical advantages

**Testing Requirements**:
- **Sortie bonus tests** (10 tests)
  - No fort owned (0% all units)
  - Dragoon/Legion/Phalanx (+5% organization)
  - Light Cavalry/Cavalry (+10% mounted)
  - Avian/Elephant/Knight (+15% elite mounted)
  - Monster units (+5% fear)
  - Regular infantry (0% extra)
  - Base sortie bonus (+10% all)
  - Stacking verification (10% + 15% = 25% max)
  - Archer/Catapult sortie interactions
  - Invalid unit types (error handling)

**Total Tests**: 10 tests
**Estimated Effort**: 1-1.5 hours

---

**Function 10: `calculate_siege_weapon_bonus(int unit_type, int troop_count, int fort_value, int is_attacker)`**
```c
/*
 * Pure function: Calculate catapult/siege engine combat bonus
 * Returns: Percentage bonus (0-30)
 */
int calculate_siege_weapon_bonus(int unit_type, int troop_count, int fort_value, int is_attacker);
```

**Extraction Benefits**:
- Test siege weapon mechanics
- Validate crew size thresholds
- Ensure max bonus caps
- Document siege warfare balance

**Testing Requirements**:
- **Catapult tests** (8 tests)
  - Defending fort: 1%/20 men (max 10%)
  - Attacking fort: 1%/40 men (max 10%)
  - No fort: 1%/40 men (max 10%)
  - Zero troops (0% bonus)
  - 200 troops defending fort (10% max bonus)
  - 400 troops attacking fort (10% max bonus)
  - 1000 troops (ensure cap at 10%)
  - Crew threshold verification

- **Siege Engine tests** (4 tests)
  - Attacking fort: 1%/20 men (max 30%)
  - Defending/no fort: 0% (attacker only)
  - 600 troops (30% max bonus)
  - Zero troops (0% bonus)

**Total Tests**: 12 tests
**Estimated Effort**: 1.5-2 hours

---

#### 3.1.3 MEDIUM PRIORITY - Formation and Magic Bonuses (P2)

**Function 11: `calculate_formation_size_bonus(int unit_type, long troop_count)`**
```c
/*
 * Pure function: Calculate phalanx/legion size bonus
 * Returns: Percentage bonus (0-20)
 */
int calculate_formation_size_bonus(int unit_type, long troop_count);
```

**Extraction Benefits**:
- Test formation size thresholds
- Validate unit type restrictions
- Ensure proper scaling

**Testing Requirements**:
- **Formation tests** (8 tests)
  - Phalanx > 1000 troops (+20%)
  - Phalanx 500-1000 troops (+10%)
  - Phalanx < 500 troops (0%)
  - Legion > 1000 troops (+20%)
  - Legion 500-1000 troops (+10%)
  - Legion < 500 troops (0%)
  - Non-formation unit (0% all)
  - Boundary cases (exactly 500, 1000)

**Total Tests**: 8 tests
**Estimated Effort**: 0.5-1 hour

---

**Function 12: `calculate_magic_terrain_bonus(int nation_id, int altitude, int vegetation)`**
```c
/*
 * Pure function: Calculate DESTROYER/DERVISH terrain bonus
 * Returns: Percentage bonus (0 or 30)
 */
int calculate_magic_terrain_bonus(int nation_id, int altitude, int vegetation);
```

**Extraction Benefits**:
- Test magic system integration
- Validate terrain requirements (ice/desert)
- Ensure proper magic power checks

**Testing Requirements**:
- **Magic bonus tests** (8 tests)
  - DESTROYER in ice (+30%)
  - DESTROYER in desert (+30%)
  - DERVISH in ice (+30%)
  - DERVISH in desert (+30%)
  - DESTROYER in forest (0%)
  - No magic in ice/desert (0%)
  - Multiple magic powers (ensure +30% not doubled)
  - Invalid nation (0%)

**Total Tests**: 8 tests
**Estimated Effort**: 0.5-1 hour

---

#### 3.1.4 LOW PRIORITY - Retreat and Utility Functions (P3)

**Function 13: `determine_retreat_side(int attacker_loss_pct, int defender_loss_pct, int odds)`**
```c
/*
 * Pure function: Determine if retreat occurs and which side retreats
 * Returns: Retreat side (ATKR, DFND, or 0 for no retreat)
 */
int determine_retreat_side(int attacker_loss_pct, int defender_loss_pct, int odds);
```

**Extraction Benefits**:
- Test retreat trigger logic
- Validate loss differential threshold (2x)
- Ensure probabilistic retreat mechanics
- Document retreat conditions

**Testing Requirements**:
- **Retreat trigger tests** (12 tests)
  - Defender loss > 2x attacker + odds > 150
  - Attacker loss > 2x defender + odds < 150
  - Even losses (no retreat)
  - High loss threshold (>= 50%)
  - Random retreat probability (rand % 4, % 8, % 2, % 6)
  - Blocked retreat handling
  - Extreme loss scenarios (100% vs 0%)
  - Marginal retreat conditions
  - Odds boundary cases (exactly 150)
  - Loss boundary cases (exactly 50%)
  - Statistical retreat frequency
  - Symmetry verification

**Total Tests**: 12 tests
**Estimated Effort**: 1.5-2 hours

---

**Function 14: `calculate_army_unit_casualties(long initial_troops, int loss_percentage, int is_leader, int unit_type, int status)`**
```c
/*
 * Pure function: Calculate individual unit casualties
 * Returns: Casualty count
 */
long calculate_army_unit_casualties(long initial_troops, int loss_percentage, int is_leader, int unit_type, int status);
```

**Extraction Benefits**:
- Test casualty distribution
- Validate minimum army size (25)
- Ensure leader death mechanics
- Test special cases (sortie archers, militia)

**Testing Requirements**:
- **Casualty tests** (14 tests)
  - Regular unit, normal casualties
  - Regular unit, casualties reduce below 25 (full loss)
  - Leader unit, death probability
  - Leader unit, survival
  - Sortie archer in owned fort (1/4 damage)
  - Militia retreat (disband to civilians)
  - Zero initial troops (0 casualties)
  - 100% loss (all troops)
  - 0% loss (0 casualties)
  - Boundary case (exactly 25 remaining)
  - Large army casualties (overflow protection)
  - RULE leader special protection
  - Vampire casualty feeding
  - Mercenary casualty handling

**Total Tests**: 14 tests
**Estimated Effort**: 2-2.5 hours

---

**Function 15: `calculate_naval_crew_strength(int ship_type, int ship_count, int crew_count, int has_sailor_magic, int army_type, long army_size)`**
```c
/*
 * Pure function: Calculate naval combat strength from ships and crew
 * Returns: Total combat strength
 */
int calculate_naval_crew_strength(int ship_type, int ship_count, int crew_count, int has_sailor_magic, int army_type, long army_size);
```

**Extraction Benefits**:
- Test crew strength multipliers
- Validate embarked army bonuses
- Ensure sailor magic effects
- Document naval combat math

**Testing Requirements**:
- **Ship crew tests** (12 tests)
  - Warship crew (1x multiplier)
  - Galley crew (2x multiplier)
  - Merchant crew (4x multiplier)
  - Sailor magic bonus (5/4 multiplier)
  - No magic bonus (baseline)
  - Zero crew (0 strength)
  - Multiple ships (scaling)
  - Crew calculation for each ship type
  - Mixed ship types (total strength)
  - Overflow protection
  - Historical crew ratios
  - Strength contribution verification

- **Embarked army tests** (6 tests)
  - Archer/Sailor embarked (3/2 multiplier)
  - Marine embarked (3x multiplier)
  - Default unit embarked (3/4 multiplier)
  - No army embarked (0 bonus)
  - Large army (scaling)
  - Zero soldiers (0 strength)

**Total Tests**: 18 tests
**Estimated Effort**: 2-3 hours

---

### 3.2 Extraction Summary and Effort Estimates

| Priority | Functions | Tests | Effort (hours) | Category |
|----------|-----------|-------|----------------|----------|
| **P0 CRITICAL** | 5 | 86 | 9.5-13.5 | Battle Resolution Formulas |
| **P1 HIGH** | 6 | 72 | 9.5-12 | Combat Bonus Calculations |
| **P2 MEDIUM** | 2 | 16 | 1-2 | Formation and Magic |
| **P3 LOW** | 3 | 44 | 6-7.5 | Retreat and Utilities |
| **TOTAL** | **16** | **218** | **26-35 hours** | **Complete Extraction** |

### 3.3 Extraction Priority Justification

**Why P0 (Battle Resolution Formulas) Is CRITICAL**:

1. **Core Gameplay Impact**:
   - These 5 functions determine ALL combat outcomes in the game
   - Bugs in these formulas break game balance completely
   - Current code has NO unit tests for these calculations
   - Silent formula errors are impossible to debug without extraction

2. **Testing Impossibility**:
   - Cannot test combat formulas without extracting them
   - Integration tests cannot isolate formula bugs
   - No way to verify formula correctness in current code
   - Historical bugs may exist undetected for years

3. **Formula Complexity**:
   - `calculate_percentage_loss()` has **7 different logic branches**
   - Naval capture formula has **5 piecewise linear segments**
   - Extreme odds handling uses different divisors per range
   - Impossible to verify correctness without comprehensive tests

4. **Risk Assessment**:
   - **Bug Impact**: CATASTROPHIC - breaks entire combat system
   - **Detection Difficulty**: EXTREME - formulas are black boxes
   - **Fix Complexity**: HIGH - changing formulas affects game balance
   - **Current Test Coverage**: 0% (no unit tests exist)

**Why P1 (Combat Bonuses) Is HIGH Priority**:

1. **Tactical Depth**:
   - 6 bonus functions determine combat effectiveness
   - Terrain, fortifications, status, and unit types all interact
   - Bugs create exploits and balance issues
   - Critical for strategic gameplay

2. **Complexity**:
   - Multiple stacking bonuses with order dependencies
   - Special cases (zombie forts, cavalry penalties, sortie bonuses)
   - Magic system integration
   - Siege weapon mechanics

**Why P2-P3 Are Lower Priority**:

- Formation bonuses: Simple threshold logic
- Magic terrain bonuses: Binary on/off checks
- Retreat mechanics: Already well-tested through integration
- Utilities: Lower complexity, easier to debug

### 3.4 Testing Infrastructure Requirements

**Test Framework**: Unity C testing framework (already integrated)

**Test Organization**:
```
tests/unit/combat/
├── test_battle_resolution.c        # P0 - 86 tests
├── test_combat_bonuses.c           # P1 - 72 tests
├── test_formation_magic.c          # P2 - 16 tests
└── test_retreat_utilities.c        # P3 - 44 tests
```

**Estimated Total Test Lines**: ~8,000-10,000 lines of comprehensive test code

**Test Execution Time**: ~2-3 seconds (all 218 tests)

**Coverage Target**: 100% line coverage for all 16 extracted functions

---

## PART 4: COMPARATIVE ANALYSIS AND PRIORITIZATION

### 4.1 Combat.c vs Other Files Comparison

| Metric | combat.c | update.c | forms.c | navy.c | admin.c |
|--------|----------|----------|---------|--------|---------|
| **Lines of Code** | 2092 | 2632 | 1786 | 1224 | 2400+ |
| **Functions** | 11 | 12 | 6 | 13 | 15+ |
| **Architectural Debt** | NONE | NONE | NONE | NONE | HIGH |
| **Misplaced Code** | 0% | 0% | 0% | 0% | 29% |
| **Magic Numbers** | **86+** | 61+ | 22 | 4 | 50+ |
| **Config Coupling** | ⭐⭐⭐⭐⭐ CATASTROPHIC | ⭐⭐⭐⭐⭐ SEVERE | ⭐⭐ LOW | ⭐ VERY LOW | ⭐⭐⭐⭐ HIGH |
| **Extractable Functions** | **16** | 18 | 4 | 12 | 10+ |
| **Test Coverage Potential** | **218 tests** | 60-120 tests | 22-26 tests | 102-128 tests | 50+ tests |
| **Extraction Effort** | **26-35h** | 21-30h | 5.5-7h | 9-11.5h | 12-16h |
| **Bug Severity** | ⭐⭐⭐⭐⭐ CATASTROPHIC | ⭐⭐⭐ MODERATE | ⭐⭐ LOW | ⭐⭐⭐⭐⭐ CATASTROPHIC | ⭐⭐⭐ MODERATE |
| **Overall Priority** | **P0 URGENT** | P1 HIGH | P4 LOW | P0 URGENT | P2 MEDIUM |

### 4.2 Critical Discovery: Bug Severity Categorization

**CATASTROPHIC Bug Severity** (P0 URGENT):

1. **combat.c - Battle Resolution Formulas**:
   - ❌ **Silent gameplay breakage** - Wrong formulas produce unfair combat
   - ❌ **Zero test coverage** - No validation of correctness
   - ❌ **7 complex logic branches** - High probability of bugs
   - ❌ **86+ magic numbers** - Impossible to verify balance
   - ❌ **Core gameplay system** - Affects every battle in game
   - ❌ **Historical bugs unknown** - May have existed for decades
   - **Impact**: **GAME-BREAKING** if formulas are wrong

2. **navy.c - Bit Manipulation**:
   - ❌ **Silent data corruption** - Ships disappear without warning
   - ❌ **Bit field interference** - Adding ships corrupts counts
   - ❌ **Overflow bugs** - Invalid ship counts
   - ❌ **Nearly impossible to debug** without unit tests
   - **Impact**: **DATA INTEGRITY FAILURE**

**Moderate Bug Severity** (P1-P2):

3. **update.c - Economic Formulas**:
   - ✅ **Visible errors** - Players notice wrong inflation/population
   - ✅ **Easier to debug** - Integration tests can catch issues
   - ✅ **Less critical** - Economic bugs don't break core gameplay
   - **Impact**: **BALANCE ISSUES** but detectable

4. **admin.c - Administrative Functions**:
   - ✅ **Architectural debt** - 29% misplaced code
   - ✅ **Moderate complexity** - Config coupling but not critical
   - **Impact**: **CODE QUALITY** issues

**Low Bug Severity** (P3-P4):

5. **forms.c - UI Presentation**:
   - ✅ **UI-only errors** - Visible immediately to players
   - ✅ **Low complexity** - Simple display logic
   - ✅ **Easy to fix** - No complex formulas
   - **Impact**: **COSMETIC** issues only

### 4.3 Updated Priority Ranking (5 Files Analyzed)

Based on complete analyses of 4 files + admin.c:

| Rank | File | Effort | Impact | Bug Severity | Reason |
|------|------|--------|--------|--------------|--------|
| 🔥 **#1** | **combat.c** | **26-35h** | **⭐⭐⭐⭐⭐** | **CATASTROPHIC** | **HIGHEST PRIORITY: 86+ constants, 0% test coverage, game-breaking bugs** |
| 🔥 **#2** | **navy.c** | **9-11.5h** | **⭐⭐⭐⭐⭐** | **CATASTROPHIC** | **CRITICAL: Bit manipulation testing - silent data corruption** |
| 🔥 **#3** | **update.c** | **21-30h** | **⭐⭐⭐⭐⭐** | **MODERATE** | **Core game loop: 61+ constants, 18 extractable functions** |
| ⭐ **#4** | **admin.c** | **12-16h** | **⭐⭐⭐⭐** | **MODERATE** | **Architectural debt: 29% misplaced code, 50+ constants** |
| ⏸️ **#5** | **forms.c** | **5.5-7h** | **⭐⭐** | **LOW** | **LOW ROI: Defer to later phases** |

**Total Estimated Effort (Top 4 Files)**: **68-92.5 hours**

### 4.4 Combat.c Specific Priority Breakdown

**Within combat.c, prioritize extraction in this order**:

1. **Week 1: Battle Resolution Formulas** (P0 CRITICAL)
   - Extract 5 core combat functions
   - Create 86 comprehensive tests
   - Validate all formula logic
   - **Effort**: 9.5-13.5 hours
   - **ROI**: EXTREME - prevents game-breaking bugs

2. **Week 2: Combat Bonus Calculations** (P1 HIGH)
   - Extract 6 bonus calculation functions
   - Create 72 tactical tests
   - Document combat mechanics
   - **Effort**: 9.5-12 hours
   - **ROI**: HIGH - enables tactical depth validation

3. **Week 3: Retreat and Utilities** (P2-P3 MEDIUM)
   - Extract 5 utility functions
   - Create 60 integration tests
   - Complete combat testing suite
   - **Effort**: 7-9.5 hours
   - **ROI**: MEDIUM - completes test coverage

**Total Combat.c Effort**: 26-35 hours (3 weeks of focused work)

---

## PART 5: RECOMMENDED ACTIONS AND IMPLEMENTATION STRATEGY

### 5.1 Immediate Actions (Phase 10.3)

**Priority 1: Create `game_balance.h` Configuration File**

```c
/* game_balance.h - Centralized Game Balance Configuration */

#ifndef GAME_BALANCE_H
#define GAME_BALANCE_H

/* ========== COMBAT MECHANICS ========== */
/* (86+ constants from Section 2.4) */

/* ========== ECONOMIC MECHANICS ========== */
/* (61+ constants from update.c analysis) */

/* ========== NAVAL MECHANICS ========== */
/* (4 constants from navy.c analysis) */

/* ========== ADMINISTRATIVE LIMITS ========== */
/* (50+ constants from admin.c analysis) */

/* ========== UI POLICY CONSTANTS ========== */
/* (22 constants from forms.c analysis) */

#endif /* GAME_BALANCE_H */
```

**Estimated Effort**: 3-4 hours (consolidate all 223+ constants)

**Priority 2: Extract Combat Resolution Formulas** (P0 CRITICAL)

1. Create `combat_formulas.c` and `combat_formulas.h`
2. Extract 5 P0 functions with full documentation
3. Create `tests/unit/combat/test_battle_resolution.c`
4. Implement 86 comprehensive tests
5. Validate all formulas against historical game data

**Estimated Effort**: 9.5-13.5 hours
**Completion**: Week 1 of Phase 10.3

**Priority 3: Extract Navy.c Bit Manipulation** (P0 CRITICAL)

(Per navy.c analysis - separate workstream)

**Estimated Effort**: 6-8 hours
**Completion**: Week 1 of Phase 10.3 (parallel with combat)

### 5.2 Phase 10.3 Recommended Sequence

**Week 1: Critical Formula Extraction** (Parallel Workstreams)
- **Stream A**: Combat resolution formulas (9.5-13.5h)
- **Stream B**: Navy bit manipulation (6-8h)
- **Total**: 15.5-21.5 hours

**Week 2: Combat Bonus Calculations** (P1 HIGH)
- Extract 6 bonus functions (9.5-12h)
- Create 72 tactical tests
- Document combat mechanics

**Week 3: Update.c Economic Formulas** (P1 HIGH)
- Extract 10-15 calculation functions (14-20h)
- Create 60-120 tests
- Validate economic balance

**Week 4: Combat Utilities + Admin Refactoring** (P2-P3)
- Complete combat.c extraction (7-9.5h)
- Relocate admin.c misplaced code (12-16h)
- Create game_balance.h (3-4h)

**Week 5+: Lower Priority Work** (P4)
- Forms.c extraction (optional, 5.5-7h)
- Additional utility extraction
- Documentation and cleanup

**Total Estimated Timeline**: 4-5 weeks of focused modernization work

### 5.3 Success Criteria

**Phase 10.3 Completion Requires**:

1. ✅ **game_balance.h created** - All 223+ constants externalized
2. ✅ **Combat formulas extracted** - 5 P0 functions with 86 tests passing
3. ✅ **Navy bit manipulation extracted** - 6 functions with 72-90 tests passing
4. ✅ **Combat bonuses extracted** - 6 P1 functions with 72 tests passing
5. ✅ **Update.c economics extracted** - 10-15 functions with 60-120 tests passing
6. ✅ **Admin.c refactored** - 0% misplaced code remaining
7. ✅ **300+ total tests passing** - Comprehensive validation
8. ✅ **Zero regressions** - All existing functionality preserved

### 5.4 Risk Mitigation Strategies

**Risk 1: Formula Extraction Breaking Gameplay**
- **Mitigation**: Extensive regression testing with saved game files
- **Validation**: Compare before/after combat outcomes for 100+ battles
- **Rollback**: Keep original functions parallel during transition

**Risk 2: Game Balance Changes**
- **Mitigation**: Extract formulas with NO changes to constants
- **Validation**: Bit-for-bit identical combat results
- **Testing**: Historical game replay verification

**Risk 3: Time Overruns**
- **Mitigation**: Prioritize P0 work first (combat + navy)
- **Fallback**: Can defer P2-P4 work to later phases
- **Minimum Viable**: Combat formulas + navy bit manipulation = success

**Risk 4: Test Infrastructure Gaps**
- **Mitigation**: Unity framework already integrated
- **Preparation**: Create test templates before extraction
- **Validation**: Incremental test development with each function

---

## PART 6: ARCHITECTURAL RECOMMENDATIONS

### 6.1 Long-Term Refactoring Strategy

**Current Architecture**: Monolithic combat system with global state

**Recommended Architecture**: Context-based combat engine

```c
/* Future combat system design */

typedef struct combat_context {
    /* Battle participants */
    combat_unit units[MGKNUM];
    int participant_count;

    /* Battle location */
    int x_coordinate;
    int y_coordinate;

    /* Combat state */
    int attacker_nation;
    int defender_nation;
    int retreat_side;
    int retreat_x;
    int retreat_y;

    /* Combat results */
    combat_result result;
} combat_context_t;

/* Pure function interface */
combat_result resolve_battle(combat_context_t *ctx, const game_balance_t *balance);
int calculate_unit_bonus(const combat_unit *unit, const sector_t *sector, const game_balance_t *balance);
```

**Benefits**:
- Thread-safe combat resolution
- Easier unit testing (no global state)
- Moddable combat system (balance parameter injection)
- Clearer data flow and dependencies

**Estimated Effort**: 40-60 hours (Phase 11+ work)

### 6.2 Combat System Documentation Needs

**Current State**: Functions documented but formulas not explained

**Required Documentation**:

1. **Combat Mechanics Guide** (`COMBAT_MECHANICS.md`)
   - Complete formula explanations
   - Worked examples for all calculations
   - Historical balance rationale
   - Moddability guide

2. **Combat Testing Guide** (`COMBAT_TESTING.md`)
   - Test scenario creation
   - Balance validation procedures
   - Regression test suite usage
   - Performance benchmarking

3. **Combat Balance Tuning Guide** (`COMBAT_TUNING.md`)
   - game_balance.h parameter reference
   - Impact analysis for each constant
   - Recommended tuning ranges
   - Playtesting procedures

**Estimated Effort**: 6-8 hours (Phase 10.4+ work)

---

## FINAL ASSESSMENT AND RECOMMENDATIONS

### Critical Findings Summary

1. **Architectural Placement**: ⭐⭐⭐⭐⭐ PERFECT (0% misplaced code)
2. **Configuration Coupling**: ⭐⭐⭐⭐⭐ CATASTROPHIC (86+ magic numbers)
3. **Extraction Potential**: ⭐⭐⭐⭐⭐ VERY HIGH (16 functions, 218 tests)
4. **Bug Severity**: ⭐⭐⭐⭐⭐ CATASTROPHIC (zero test coverage on core formulas)
5. **Overall Priority**: **#1 - HIGHEST PRIORITY FILE IN CODEBASE**

### Why Combat.c Is THE HIGHEST Priority

**Combat.c surpasses even navy.c for top priority because**:

1. **Scope Impact**:
   - Combat affects EVERY battle in the game (100% of military gameplay)
   - Navy bit manipulation only affects naval units (subset of gameplay)

2. **Formula Complexity**:
   - 86+ interacting constants with complex piecewise formulas
   - Navy has 4 constants with straightforward bit operations

3. **Bug Detection Difficulty**:
   - Combat formula bugs are **silent and subtle** (wrong percentages)
   - Navy bit bugs are **silent but catastrophic** (data corruption)
   - **Both are equally hard to detect, but combat affects more players**

4. **Testing Effort**:
   - Combat requires 218 comprehensive tests (26-35 hours)
   - Navy requires 102-128 tests (9-11.5 hours)
   - **Combat is 3x the effort but 10x the impact**

5. **Zero Current Test Coverage**:
   - NO unit tests exist for combat formulas
   - NO way to verify formula correctness
   - **Bugs may have existed for decades undetected**

### Recommended Implementation Order

**Phase 10.3 - Week 1** (P0 CRITICAL - Parallel Workstreams):
- **Stream A**: Extract combat resolution formulas (9.5-13.5h)
- **Stream B**: Extract navy bit manipulation (6-8h)
- **Create**: game_balance.h with 223+ constants (3-4h)

**Phase 10.3 - Week 2** (P1 HIGH):
- Extract combat bonus calculations (9.5-12h)
- Test tactical combat mechanics (72 tests)

**Phase 10.3 - Week 3** (P1 HIGH):
- Extract update.c economic formulas (14-20h)
- Test game balance mechanics (60-120 tests)

**Phase 10.3 - Week 4** (P2-P3):
- Complete combat.c extraction (7-9.5h)
- Refactor admin.c architectural debt (12-16h)

**Phase 10.4+** (P4 LOW):
- Forms.c extraction (optional, 5.5-7h)
- Long-term architectural refactoring (40-60h)

### Success Metrics

**Immediate Success** (Phase 10.3 Complete):
- ✅ 86+ combat formula constants externalized to game_balance.h
- ✅ 16 combat functions extracted with full documentation
- ✅ 218 comprehensive tests passing (100% coverage)
- ✅ Zero regressions in existing gameplay
- ✅ Combat formulas validated against historical data

**Long-Term Success** (Phase 11+):
- ✅ Context-based combat architecture implemented
- ✅ Thread-safe combat resolution
- ✅ Moddable combat system with parameter injection
- ✅ Complete combat mechanics documentation

### Final Recommendation

**URGENT: Prioritize combat.c extraction as #1 modernization task**

**Rationale**:
- **Highest configuration coupling** in entire codebase (86+ constants)
- **Zero test coverage** on mission-critical formulas
- **CATASTROPHIC bug potential** affecting core gameplay
- **Highest test ROI** (218 tests protecting most important code)
- **Enables all future combat work** (balance tuning, modding, variants)

**Next Steps**:
1. Create game_balance.h with combat constants (3-4h)
2. Extract battle resolution formulas (9.5-13.5h)
3. Create comprehensive test suite (86 tests)
4. Validate formulas against historical game data
5. Complete combat bonus extraction (9.5-12h)
6. Move to update.c economic formulas (14-20h)

**Total Phase 10.3 Effort**: 68-92.5 hours (4-5 weeks)

**Expected Outcome**: **Combat system fully tested, externally configurable, and validated** - eliminating decades of potential silent bugs and enabling future game balance innovation.

---

**Document Complete**
**Generated**: 2025-10-10
**Status**: Combat.c architectural debt assessment complete - READY FOR PHASE 10.3 IMPLEMENTATION


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
