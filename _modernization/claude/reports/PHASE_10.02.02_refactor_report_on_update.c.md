# Phase 10.2.2 - Pure Function Extraction Analysis Report

## File: update.c

**Analysis Date**: 2025-10-10
**Analyzer**: Claude (claude-sonnet-4-5@20250929)
**Phase**: 10.2.2 - Pure Function Extraction
**File Size**: 2,632 lines
**Total Functions**: 11 major functions

---

## Executive Summary

**Overall Assessment**: ⭐⭐⭐⭐ **EXCELLENT extraction candidate**

`update.c` is the core game engine turn processing file containing multiple high-value pure calculation functions embedded within I/O-heavy orchestration code. Extraction opportunities are **abundant and high-quality**, with clear separation points between business logic and infrastructure.

**Key Findings**:
- **17-21 pure functions** can be extracted
- **54-74 unit tests** can be created
- **5 high-priority extraction targets** identified
- **Zero anticipated behavioral changes** (pure refactor)
- **Critical game mechanics** (economics, population, military) are extractable

**Strategic Decision Point**: This file represents significant re-engineering opportunity. The volume and quality of extractable logic suggests this work may warrant:
- Separate feature branch (`feature/phase-10.2-pure-function-extraction`)
- Potential minor version increment (e.g., 4.x → 4.y) due to substantial architectural changes

---

## File Overview

### Purpose
Central orchestrator for game turn processing including:
- Economic simulation (poverty, inflation, famine)
- Population dynamics and migration
- Military movement and siege warfare
- Leader and monster spawning
- Diplomatic contact and trade processing

### Current Architecture
**Pattern**: God objects with embedded business logic
- Large orchestration functions (100-300+ lines)
- Pure calculations mixed with file I/O, mail, news reporting
- Global state dependencies throughout
- Complex conditional logic buried in loops

### Documentation Status
✅ **Excellent** - All functions comprehensively documented in Phase 3
- Testing categories assigned (A/B/C)
- Algorithm descriptions complete
- Parameter/return value documentation thorough

---

## Extraction Candidates (Priority Order)

### 🥇 Priority 1: Poverty & Inflation Calculations

**Location**: `updsectors()` function, lines 1584-1637
**Current State**: Embedded in massive sector processing loop
**Category**: Pure economic modeling

#### Extractable Functions

##### 1. `calculate_poverty_index()`
```c
/*
 * calculate_poverty_index - Determine poverty level based on wealth distribution
 *
 * Implements 6-tier progressive poverty calculation based on gold-per-civilian ratio.
 * Models wealth inequality with diminishing poverty as per-capita wealth increases.
 *
 * Parameters:
 *   gold - National treasury in gold talons (>= 0)
 *   civilians - Total civilian population (> 0)
 *
 * Returns:
 *   Poverty index (0-95):
 *     95: Negative treasury (bankrupt)
 *     20-95: Graduated scale based on gold/civilian ratio
 *     0-20: Wealthy nation with >200 gold per civilian
 *
 * Testing: 7+ test cases covering all 6 tiers and boundary conditions
 */
unsigned char calculate_poverty_index(long gold, long civilians);
```

**Current Code** (lines 1584-1602):
- 6-tier conditional structure
- Complex ratio calculations
- Edge case handling (negative gold, low population)
- **Zero I/O dependencies**

**Test Cases**:
1. Negative treasury → poverty = 95
2. Very low population (< 100) → poverty = 20
3. Gold/civilian < 30 → poverty = 65-95
4. Gold/civilian 30-80 → poverty = 40-65
5. Gold/civilian 80-120 → poverty = 30-40
6. Gold/civilian 120-200 → poverty = 20-30
7. Gold/civilian > 200 → poverty = 20

**Extraction Value**: ⭐⭐⭐⭐⭐ (Critical game balance mechanic)

---

##### 2. `calculate_inflation_base()`
```c
/*
 * calculate_inflation_base - Calculate national inflation rate
 *
 * Multi-factor inflation model incorporating:
 * - Tax burden (direct correlation)
 * - Military burden (civilians supporting military)
 * - Poverty levels (social instability)
 *
 * Parameters:
 *   tax_rate - National tax rate (0-100)
 *   military - Total military population
 *   civilians - Total civilian population
 *   poverty - Current poverty index (0-100)
 *
 * Returns:
 *   Inflation rate as short integer (0+)
 *   Higher values indicate worse economic conditions
 *
 * Testing: 5+ test cases covering factor interactions
 */
short calculate_inflation_base(int tax_rate, long military, long civilians, unsigned char poverty);
```

**Current Code** (lines 1614-1627):
- Tax rate contribution with randomization
- Military burden calculation (ratio-based)
- Poverty adjustment
- Previous inflation decay

**Test Cases**:
1. Zero military, zero taxes → minimal inflation
2. High tax rate → proportional inflation increase
3. High military/civilian ratio → burden inflation
4. High poverty → additional inflation penalty
5. Combined factors → cumulative effects

**Extraction Value**: ⭐⭐⭐⭐⭐ (Economic balance critical)

---

##### 3. `apply_inflation_to_treasury()`
```c
/*
 * apply_inflation_to_treasury - Apply inflation effects to national gold reserves
 *
 * Implements differential inflation application based on treasury size:
 * - Large treasuries (>1M): Division-based to prevent overflow
 * - Normal treasuries: Multiplication-based for precision
 *
 * Parameters:
 *   gold - Current national treasury
 *   inflation - Calculated inflation rate
 *
 * Returns:
 *   Adjusted treasury after inflation effects
 *
 * Testing: 4+ test cases covering size thresholds and overflow protection
 */
long apply_inflation_to_treasury(long gold, short inflation);
```

**Current Code** (lines 1633-1637):
- Overflow protection for large treasuries
- Precision maintenance for normal economies
- Mathematical equivalence between branches

**Test Cases**:
1. Small treasury (< 1M) → multiplication formula
2. Large treasury (> 1M) → division formula
3. Mathematical equivalence verification
4. Overflow boundary testing

**Extraction Value**: ⭐⭐⭐⭐ (Technical correctness critical)

---

##### 4. `calculate_charity_distribution()`
```c
/*
 * calculate_charity_distribution - Distribute wealth via charity system
 *
 * Calculates per-civilian charity distribution from national treasury based on
 * charity rate setting and gold increase during turn.
 *
 * Parameters:
 *   gold_increase - Gold gained during turn (spread.gold - curntn->tgold)
 *   charity_rate - National charity percentage (0-100)
 *   civilians - Total civilian population
 *
 * Returns:
 *   Per-civilian charity amount (0+ if civilians > 0, else 0)
 *
 * Testing: 3+ test cases for distribution scenarios
 */
long calculate_charity_distribution(long gold_increase, unsigned char charity_rate, long civilians);
```

**Current Code** (lines 1572-1582):
- Proportional distribution calculation
- Zero-population safety check
- Negative increase handling (no charity from losses)

**Test Cases**:
1. Positive gold increase → proportional distribution
2. Negative gold increase → zero charity
3. Zero civilians → zero charity (safety)

**Extraction Value**: ⭐⭐⭐ (Social system mechanic)

---

##### 5. `apply_charity_effects()`
```c
/*
 * apply_charity_effects - Modify popularity and poverty based on charity
 *
 * Applies positive social effects of wealth redistribution:
 * - Popularity increase (capped at MAXTGVAL)
 * - Poverty reduction (halved charity value)
 *
 * Parameters:
 *   current_popularity - Current national popularity (0-255)
 *   current_poverty - Current poverty index (0-100)
 *   charity_per_civilian - Per-civilian charity distributed
 *
 * Returns:
 *   Structure with new_popularity and new_poverty values
 *
 * Testing: 3+ test cases for effect calculations
 */
struct charity_effects {
    unsigned char new_popularity;
    unsigned char new_poverty;
};
struct charity_effects apply_charity_effects(unsigned char current_popularity,
                                             unsigned char current_poverty,
                                             long charity_per_civilian);
```

**Current Code** (lines 1605-1612):
- Popularity boost with cap
- Poverty reduction (halving formula)
- Boundary protection (no negative poverty)

**Test Cases**:
1. Large charity → capped popularity, significant poverty reduction
2. Small charity → minor effects
3. Boundary conditions (max popularity, zero poverty)

**Extraction Value**: ⭐⭐⭐⭐ (Social effects modeling)

---

#### Summary: Poverty/Inflation Extraction
- **Functions**: 5 pure calculation functions
- **Tests**: 15-20 unit tests
- **Effort**: 1.5-2 hours
- **Business Value**: ⭐⭐⭐⭐⭐ (Critical economic balance)
- **Risk**: Very Low (pure math, well-documented)
- **Target File**: `nation_economics.c` / `nation_economics.h`

---

### 🥈 Priority 2: Sector Attractiveness Calculations

**Location**: `attract()` function, lines 236-384
**Current State**: Standalone function but contains extractable sub-calculations
**Category**: Pure calculation (already marked "Category A - Unit Testable")

#### Extractable Functions

##### 1. `calculate_tradegood_attractiveness()`
```c
/*
 * calculate_tradegood_attractiveness - Base attractiveness from trade goods
 *
 * Calculates sector value based on trade good presence and designation match.
 * Only applies when sector designation matches trade good production type.
 *
 * Parameters:
 *   tradegood - Trade good type (TG_none, or specific good)
 *   designation - Sector designation (DMINE, DGOLDMINE, DFARM, etc.)
 *   trade_good_value - Character value from tg_value array
 *
 * Returns:
 *   Attractiveness bonus (0 if no match, value * TGATTR if matched)
 *
 * Testing: 4+ test cases for good/designation combinations
 */
int calculate_tradegood_attractiveness(int tradegood, int designation, char trade_good_value);
```

**Current Code** (lines 242-245):
- Trade good type checking
- Designation matching logic
- Special handling for mines (excluded from calculation)

**Test Cases**:
1. No trade good → 0 attractiveness
2. Trade good + matching designation → value * TGATTR
3. Mine designation → 0 (excluded)
4. Goldmine designation → 0 (excluded)

**Extraction Value**: ⭐⭐⭐⭐

---

##### 2. `calculate_designation_attractiveness()`
```c
/*
 * calculate_designation_attractiveness - Attractiveness based on sector improvements
 *
 * Calculates base attractiveness from sector designation (cities, farms, mines).
 * Implements special bonuses for high-resource sectors and food scarcity handling.
 *
 * Parameters:
 *   designation - Sector designation type
 *   jewels - Jewel resource level (0-10+)
 *   metal - Metal resource level (0-10+)
 *   food_production - Food produced by sector
 *   food_scarcity_bonus - Bonus for farms during food shortage (calculated separately)
 *
 * Returns:
 *   Designation-based attractiveness value (0+)
 *
 * Testing: 8+ test cases covering all designation types
 */
int calculate_designation_attractiveness(int designation, int jewels, int metal,
                                        int food_production, int food_scarcity_bonus);
```

**Current Code** (lines 247-271):
- Gold mine attractiveness (jewel-based, doubled if jewels >= 6)
- Farm attractiveness (food-based or scarcity bonus)
- City/Capitol/Town fixed bonuses
- Mine attractiveness (metal-based, doubled if metal >= 6)
- Habitable "other" designation bonus

**Test Cases**:
1. Goldmine with low jewels (<6) → GOLDATTR * jewels
2. Goldmine with high jewels (>=6) → GOLDATTR * jewels * 2
3. Farm with food scarcity → 50 * FARMATTR
4. Farm without scarcity → food_production * FARMATTR
5. City/Capitol/Town → CITYATTR/TOWNATTR
6. Mine with low metal → MINEATTR * metal
7. Mine with high metal → MINEATTR * metal * 2
8. Other habitable designations → OTHRATTR

**Extraction Value**: ⭐⭐⭐⭐⭐

---

##### 3. `calculate_farm_scarcity_bonus()`
```c
/*
 * calculate_farm_scarcity_bonus - Determine if nation has food shortage
 *
 * Checks if national food reserves are critically low relative to consumption,
 * triggering increased farm attractiveness for migration purposes.
 *
 * Parameters:
 *   nation_food - Current national food reserves
 *   eat_rate - Per-capita food consumption rate
 *   civilians - Total civilian population
 *
 * Returns:
 *   Boolean: 1 if food scarcity exists, 0 otherwise
 *
 * Formula: food * 250 <= eat_rate * (civilians * 11)
 *
 * Testing: 3+ test cases for scarcity thresholds
 */
int calculate_farm_scarcity_bonus(long nation_food, int eat_rate, long civilians);
```

**Current Code** (lines 253-255):
- Complex ratio comparison
- Threshold formula (250:11 ratio with eat_rate multiplier)

**Test Cases**:
1. Abundant food → no scarcity (0)
2. Low food → scarcity detected (1)
3. Edge case at threshold boundary

**Extraction Value**: ⭐⭐⭐

---

##### 4. `calculate_race_terrain_bonus()`
```c
/*
 * calculate_race_terrain_bonus - Race-specific environmental preferences
 *
 * Calculates attractiveness bonuses based on racial preferences for terrain,
 * vegetation, and improved sectors. Each race has unique preference profiles.
 *
 * Parameters:
 *   race - Race type (DWARF, ELF, HUMAN, ORC)
 *   designation - Sector designation
 *   jewels - Jewel resource level
 *   metal - Metal resource level
 *   vegetation - Vegetation type (WOOD, FOREST, etc.)
 *   altitude - Altitude type (MOUNTAIN, HILL, CLEAR, WATER)
 *
 * Returns:
 *   Race-specific attractiveness bonus
 *   Returns 0 if terrain incompatible with race (e.g., water for land races)
 *
 * Testing: 16+ test cases (4 races × 4 terrain combinations)
 */
int calculate_race_terrain_bonus(int race, int designation, int jewels, int metal,
                                 int vegetation, int altitude);
```

**Current Code** (lines 273-380):
- Four race-specific switch cases (DWARF, ELF, HUMAN, ORC)
- Each race has unique bonuses for:
  - Goldmines with high jewels
  - Mines with high metal
  - Towns/Cities/Capitols
  - Vegetation preferences (wood, forest)
  - Altitude preferences (mountain, hill, clear)
- Water/peak altitude zeroes out attractiveness

**Test Cases** (by race):
- **Dwarf**: Mountain/hill bonuses, mine preference
- **Elf**: Forest bonuses, goldmine preference
- **Human**: Balanced preferences (adaptable)
- **Orc**: Specific terrain preferences
- **All races**: Water/peak exclusion

**Extraction Value**: ⭐⭐⭐⭐⭐ (Complex race mechanics)

---

#### Summary: Attractiveness Extraction
- **Functions**: 4 pure calculation functions
- **Tests**: 12-18 unit tests
- **Effort**: 2 hours
- **Business Value**: ⭐⭐⭐⭐⭐ (Core population movement mechanic)
- **Risk**: Very Low (already marked unit testable)
- **Target File**: `sector_attractiveness.c` / `sector_attractiveness.h`

---

### 🥉 Priority 3: Population Growth Calculations

**Location**: `updsectors()` function, lines 1492-1522
**Current State**: Embedded in sector processing loop
**Category**: Demographic simulation

#### Extractable Functions

##### 1. `calculate_population_growth()`
```c
/*
 * calculate_population_growth - Calculate population increase for sector
 *
 * Implements size-based growth rates with urban reduction factors:
 * - Huge populations (>= ABSMAXPEOPLE): No growth (capped)
 * - Large populations (> TOMANYPEOPLE): Half growth rate (urbanization)
 * - Small populations (< 100): 10% growth rate (bootstrap)
 * - Normal populations: Full reproduction rate
 *
 * Urban designations (cities, capitols, towns) always use half rate regardless of size.
 *
 * Parameters:
 *   current_population - Current sector population (0+)
 *   reproduction_rate - Seasonal reproduction rate (calculated from nation repro / 4)
 *   designation - Sector designation (DCITY, DCAPITOL, DTOWN, or other)
 *
 * Returns:
 *   New population after growth (capped at ABSMAXPEOPLE)
 *
 * Testing: 6+ test cases for size/designation combinations
 */
long calculate_population_growth(long current_population, int reproduction_rate, int designation);
```

**Current Code** (lines 1492-1522):
- Four-tier population size handling
- Urban growth reduction (half rate)
- Maximum population cap enforcement
- Small population bootstrap growth

**Test Cases**:
1. Huge population (>= ABSMAXPEOPLE) → no growth, cap enforced
2. Large non-urban (> TOMANYPEOPLE) → repro/200 growth rate
3. Large urban (city/capitol/town) → repro/200 growth rate
4. Small population (< 100) → 10% growth
5. Normal population → repro/100 growth rate
6. Edge cases at threshold boundaries

**Extraction Value**: ⭐⭐⭐⭐⭐

---

##### 2. `calculate_resource_depletion()`
```c
/*
 * calculate_resource_depletion - Determine if mine resources should deplete
 *
 * Models mining efficiency reduction as population density increases.
 * High population relative to resource availability causes depletion.
 *
 * Parameters:
 *   population - Current sector population
 *   resource_amount - Metal or jewel resource level
 *   depletion_threshold - TOMUCHMINED constant (overmining threshold)
 *   random_factor - Random value 0-99 (from rand() % 100)
 *
 * Returns:
 *   1 if resource should deplete, 0 otherwise
 *
 * Formula: population * resource > random_factor * threshold
 *
 * Testing: 4+ test cases for depletion scenarios
 */
int calculate_resource_depletion(long population, int resource_amount,
                                int depletion_threshold, int random_factor);
```

**Current Code** (lines 1495-1521):
- Population/resource ratio calculation
- Randomized depletion threshold
- Different formulas for huge vs large populations (2x factor)

**Test Cases**:
1. Low population, high resources → no depletion
2. High population, low resources → likely depletion
3. Huge population (2x multiplier) → increased depletion chance
4. Random factor influence on probabilistic depletion

**Extraction Value**: ⭐⭐⭐⭐

---

##### 3. `should_exhaust_mine()`
```c
/*
 * should_exhaust_mine - Check if mine should convert to devastated sector
 *
 * Determines if mine/goldmine has exhausted all extractable resources
 * and should be converted to devastated designation.
 *
 * Parameters:
 *   designation - Sector designation (DMINE or DGOLDMINE)
 *   metal - Metal resource level (0+)
 *   jewels - Jewel resource level (0+)
 *
 * Returns:
 *   1 if mine should be exhausted, 0 otherwise
 *
 * Testing: 4+ test cases for exhaustion conditions
 */
int should_exhaust_mine(int designation, int metal, int jewels);
```

**Current Code** (lines 1524-1528):
- Goldmine exhaustion (jewels == 0)
- Mine exhaustion (metal == 0)
- Designation change to DDEVASTATED
- Trade good removal (TG_none)

**Test Cases**:
1. Goldmine with jewels > 0 → not exhausted
2. Goldmine with jewels == 0 → exhausted
3. Mine with metal > 0 → not exhausted
4. Mine with metal == 0 → exhausted

**Extraction Value**: ⭐⭐⭐

---

#### Summary: Population Growth Extraction
- **Functions**: 3 pure calculation functions
- **Tests**: 9-12 unit tests
- **Effort**: 1 hour
- **Business Value**: ⭐⭐⭐⭐ (Demographic engine)
- **Risk**: Low (well-documented thresholds)
- **Target File**: `population_dynamics.c` / `population_dynamics.h`

---

### 4️⃣ Priority 4: Siege Strength Calculations

**Location**: `updmil()` function, lines 1835-1867
**Current State**: Embedded in military update loop
**Category**: Military mechanics

#### Extractable Functions

##### 1. `calculate_siege_attacker_strength()`
```c
/*
 * calculate_siege_attacker_strength - Calculate total attacker strength in siege
 *
 * Sums military strength of all attacking units in sector, applying
 * special bonuses for siege engines (count as 3x regular troops).
 *
 * Parameters:
 *   armies - Array of all armies in the game
 *   target_x - Siege location X coordinate
 *   target_y - Siege location Y coordinate
 *   max_nations - Total number of nations (NTOTAL)
 *   max_armies - Maximum armies per nation (MAXARM)
 *
 * Returns:
 *   Total attacker strength value (soldiers + 3*siege_engines)
 *
 * Testing: 4+ test cases for strength calculations
 */
int calculate_siege_attacker_strength(struct s_nation nations[],
                                     int target_x, int target_y,
                                     int max_nations, int max_armies);
```

**Current Code** (lines 1837-1850):
- Nested nation/army loops
- Siege engine 3x multiplier
- Regular troop counting
- Status filtering (SIEGE status only)

**Test Cases**:
1. No attackers → strength = 0
2. Regular troops only → sum of soldiers
3. Siege engines only → 3x soldiers
4. Mixed force → regular + 3*siege engines

**Extraction Value**: ⭐⭐⭐⭐

---

##### 2. `calculate_siege_defender_strength()`
```c
/*
 * calculate_siege_defender_strength - Calculate total defender strength in siege
 *
 * Sums military strength of defending nation's forces, applying
 * penalty for militia units (count as 50% effective when besieged).
 *
 * Parameters:
 *   defender_nation - Defending nation structure
 *   target_x - Siege location X coordinate
 *   target_y - Siege location Y coordinate
 *   max_armies - Maximum armies per nation (MAXARM)
 *
 * Returns:
 *   Total defender strength (soldiers, militia at half strength)
 *
 * Testing: 3+ test cases for defender scenarios
 */
int calculate_siege_defender_strength(struct s_nation *defender_nation,
                                     int target_x, int target_y,
                                     int max_armies);
```

**Current Code** (lines 1852-1861):
- Owner nation army iteration
- Militia penalty (50% effectiveness)
- Regular troops full strength
- Location matching

**Test Cases**:
1. Regular troops only → full soldier count
2. Militia only → half soldier count
3. Mixed defenders → regular + militia/2

**Extraction Value**: ⭐⭐⭐⭐

---

##### 3. `is_siege_valid()`
```c
/*
 * is_siege_valid - Validate siege based on attacker/defender strength ratio
 *
 * Implements 2:1 siege rule: attackers must have at least twice the
 * defender strength to maintain a valid siege.
 *
 * Parameters:
 *   attacker_strength - Total attacker military strength
 *   defender_strength - Total defender military strength
 *
 * Returns:
 *   1 if siege is valid (attackers > 2 * defenders), 0 otherwise
 *
 * Testing: 3 test cases for ratio validation
 */
int is_siege_valid(int attacker_strength, int defender_strength);
```

**Current Code** (line 1862):
- Simple 2:1 ratio check
- Boolean return

**Test Cases**:
1. Attackers > 2*defenders → valid (1)
2. Attackers == 2*defenders → valid (1)
3. Attackers < 2*defenders → invalid (0)

**Extraction Value**: ⭐⭐⭐

---

#### Summary: Siege Calculations Extraction
- **Functions**: 3 pure calculation functions
- **Tests**: 9-12 unit tests
- **Effort**: 1 hour
- **Business Value**: ⭐⭐⭐⭐ (Military balance critical)
- **Risk**: Low (clear mathematical rules)
- **Target File**: `military_calculations.c` / `military_calculations.h`

---

### 5️⃣ Priority 5: Famine Death Calculations

**Location**: `updcomodities()` function, lines 2185-2194
**Current State**: Mixed with mail/news I/O
**Category**: Economic consequences

#### Extractable Functions

##### 1. `calculate_famine_deaths()`
```c
/*
 * calculate_famine_deaths - Determine population loss from famine
 *
 * Implements famine death rule: 1 death per 3 food shortage,
 * maximum 1/3 of sector population can die per famine event.
 *
 * Parameters:
 *   sector_population - Current sector population
 *   food_deficit - Negative food value (shortage amount)
 *
 * Returns:
 *   Number of deaths (min of population/3 and abs(food_deficit)/3)
 *
 * Testing: 4+ test cases for death calculations
 */
long calculate_famine_deaths(long sector_population, long food_deficit);
```

**Current Code** (lines 2185-2194):
- Two-path calculation (small vs large population)
- 1:3 food:death ratio
- Maximum 1/3 population loss cap
- Food deficit consumption

**Test Cases**:
1. Small deficit → proportional deaths (deficit/3)
2. Large deficit → capped deaths (population/3)
3. Edge case: deficit == 3*population
4. Edge case: tiny deficit (< 3)

**Extraction Value**: ⭐⭐⭐⭐

---

##### 2. `apply_famine_to_sector()`
```c
/*
 * apply_famine_to_sector - Update sector and food after famine deaths
 *
 * Calculates population reduction and food deficit changes after famine event.
 *
 * Parameters:
 *   sector_population - Current sector population
 *   food_deficit - Current food shortage (negative value)
 *
 * Returns:
 *   Structure containing new_population and new_food_deficit
 *
 * Testing: 3+ test cases for application logic
 */
struct famine_result {
    long new_population;
    long new_food_deficit;
};
struct famine_result apply_famine_to_sector(long sector_population, long food_deficit);
```

**Current Code** (lines 2185-2194):
- Death calculation
- Population reduction
- Food deficit update
- Two-phase logic based on population size

**Test Cases**:
1. Small population → entire deficit may be absorbed
2. Large population → deficit reduced by deaths
3. Verification: deaths = min(pop/3, deficit/3)

**Extraction Value**: ⭐⭐⭐⭐

---

##### 3. `calculate_food_spoilage()`
```c
/*
 * calculate_food_spoilage - Apply spoilage rate to remaining food
 *
 * Reduces food reserves based on national spoilage rate percentage,
 * modeling storage losses and distribution inefficiency.
 *
 * Parameters:
 *   current_food - Food reserves after consumption
 *   spoilage_rate - National spoilage percentage (0-100)
 *
 * Returns:
 *   Remaining food after spoilage
 *
 * Formula: food * (100 - spoilage_rate) / 100
 *
 * Testing: 3 test cases for spoilage rates
 */
long calculate_food_spoilage(long current_food, unsigned char spoilage_rate);
```

**Current Code** (lines 2218-2219):
- Percentage-based reduction
- Float intermediate calculation (precision)
- Conversion back to long

**Test Cases**:
1. Zero spoilage (0%) → no change
2. Moderate spoilage (20%) → 80% remains
3. High spoilage (50%) → 50% remains

**Extraction Value**: ⭐⭐⭐

---

#### Summary: Famine Calculations Extraction
- **Functions**: 3 pure calculation functions
- **Tests**: 9-12 unit tests
- **Effort**: 45 minutes
- **Business Value**: ⭐⭐⭐⭐ (Survival mechanic)
- **Risk**: Very Low (simple mathematical rules)
- **Target File**: `nation_economics.c` (add to Priority 1 file)

---

## Moderate Extraction Candidates

### Movement Point Calculations (updmil: lines 1808-1909)

**Challenge**: Tightly coupled with army status updates
**Extractable Logic**: Movement calculation formulas based on unit type, status, magic
**Effort**: Medium - requires careful separation of calculation from state updates
**Value**: ⭐⭐⭐ - Would improve testability but higher refactoring risk

**Potential Functions**:
- `calculate_base_movement(unit_type, nation_max_move, army_status)`
- `apply_magic_movement_modifiers(base_movement, magic_flags, ownership)`
- `calculate_group_movement(leader_movement, member_movements[])`

**Decision**: **DEFER** to later phase - requires more invasive refactoring

---

## Functions NOT Suitable for Extraction

### `update()` - Master Turn Coordinator (lines 102-182)
**Why Not**: Pure orchestration logic, no extractable calculations
**Pattern**: Sequential subsystem calls with integrity checks
**Value**: Already appropriate architecture (orchestrator pattern)

### `updexecs()` - Nation Execution Loop (lines 864-1045)
**Why Not**: Heavy I/O (file, mail, random execution), stateful
**Pattern**: Complex iteration with side effects
**Value**: Would require complete re-architecture (beyond scope)

### `updcapture()` - Sector Capture Processing (lines 1248-1354)
**Why Not**: State modification core purpose, mail notifications integral
**Pattern**: Territory ownership changes with notifications
**Value**: Appropriate for its purpose (state transformation)

### `updmil()` - Military Update (lines 1759-2057)
**Why Not**: 300+ lines of stateful updates, too large and complex
**Pattern**: Massive state transformation with naval/siege/movement updates
**Value**: Needs full re-architecture (separate epic)
**Note**: Siege calculations (lines 1835-1867) ARE extractable despite being in this function

### `updleader()` - Leader/Monster Spawning (lines 2357-2444)
**Why Not**: Side effects are primary purpose (unit creation)
**Pattern**: Probabilistic spawning with state modification
**Value**: Birth rate calculations could be extracted but low ROI

### `move_people()` - Population Migration (lines 2564-2631)
**Why Not**: Memory buffer manipulation, algorithmic not calculational
**Pattern**: Circular buffer matrix operations
**Value**: Already well-optimized for its purpose

---

## Extraction Summary

### Total Extraction Potential

| Priority | Functions | Tests | Effort | Target File |
|----------|-----------|-------|--------|-------------|
| 1. Poverty/Inflation | 5 | 15-20 | 1.5-2h | `nation_economics.c` |
| 2. Attractiveness | 4 | 12-18 | 2h | `sector_attractiveness.c` |
| 3. Population Growth | 3 | 9-12 | 1h | `population_dynamics.c` |
| 4. Siege Calculations | 3 | 9-12 | 1h | `military_calculations.c` |
| 5. Famine Deaths | 3 | 9-12 | 45min | `nation_economics.c` |
| **TOTAL** | **18** | **54-74** | **6-7h** | **4 new files** |

### Phase 10.2.2 Goal Achievement

**Goal**: 15-25 pure functions, 30-75 tests
**This File**: 18 functions, 54-74 tests
**Status**: ✅ **EXCEEDS GOAL** (this file alone meets entire phase target)

---

## Strategic Recommendations

### Re-engineering Decision Analysis

#### Evidence for Separate Branch/Version:

1. **Volume**: 18 extractable functions from ONE file
2. **Scope**: 4 new source files + 4 test files = 8 new files
3. **Effort**: 6-7 hours for one file (8-10 more files to analyze)
4. **Impact**: Core game mechanics refactored (economics, population, military)
5. **Architecture**: Fundamental separation of business logic from infrastructure

#### Branch Strategy Recommendation:

**Option A: Feature Branch (Recommended)**
- Branch: `feature/phase-10.2-pure-function-extraction`
- Base: `phase_10_deep_refactoring`
- Merge strategy: Squash merge or feature merge with comprehensive testing
- Version: Maintain 4.x (architectural refactor, not feature change)

**Option B: New Minor Version**
- Branch: `release/4.y` (where y = x + 1)
- Rationale: Significant architectural changes warrant version increment
- Migration: Provide compatibility notes for any API changes
- Version: 4.x → 4.y (minor version bump)

**Option C: Continue in Current Branch**
- Risk: High - substantial changes may destabilize current work
- Benefit: Simpler git history
- Recommendation: Only if extraction is done incrementally with heavy testing

### Recommended Approach:

```bash
# Create feature branch from current phase branch
git checkout phase_10_deep_refactoring
git checkout -b feature/phase-10.2-pure-function-extraction

# Work in feature branch with frequent commits
# Each extraction gets its own commit:
# - [EXTRACT] Poverty/inflation calculations → nation_economics.c
# - [EXTRACT] Sector attractiveness → sector_attractiveness.c
# etc.

# When complete, merge back to phase_10_deep_refactoring
# After full phase 10 completion, consider version increment for release
```

---

## Implementation Roadmap

### Phase 10.2.2 Execution Plan

#### Session 1: Foundation (2-3 hours)
1. **Setup** (30 min):
   - Create `nation_economics.c/h`, `sector_attractiveness.c/h`
   - Create corresponding test files
   - Update CMakeLists.txt
   - Create stub test framework

2. **Priority 1: Poverty/Inflation** (1.5h):
   - Extract 5 economic calculation functions
   - Write 15-20 unit tests
   - Verify all tests pass
   - Refactor `updsectors()` to use new functions
   - Verify zero behavioral change
   - Git commit

3. **Priority 2: Attractiveness (Start)** (1h):
   - Extract `calculate_tradegood_attractiveness()`
   - Extract `calculate_farm_scarcity_bonus()`
   - Write 7-9 tests
   - Git commit

#### Session 2: Completion (2-3 hours)
4. **Priority 2: Attractiveness (Finish)** (1h):
   - Extract `calculate_designation_attractiveness()`
   - Extract `calculate_race_terrain_bonus()`
   - Write 10-12 tests
   - Refactor `attract()` to use new functions
   - Git commit

5. **Priority 3: Population Growth** (1h):
   - Extract 3 population functions
   - Write 9-12 tests
   - Refactor `updsectors()` population code
   - Git commit

6. **Priorities 4-5: Military & Famine** (1h):
   - Extract siege calculations
   - Extract famine calculations
   - Write 18-24 tests
   - Git commits

7. **Validation** (30 min):
   - Run full test suite (verify 40-85 tests passing)
   - Run build with all warning levels
   - Manual smoke testing
   - Create session memory

---

## Testing Strategy

### Unit Test Coverage Goals

**Per-Function Testing**:
- Boundary conditions (min, max, zero, negative)
- Typical cases (common game scenarios)
- Edge cases (threshold boundaries)
- Error conditions (invalid inputs)

**Integration Testing**:
- Verify calling code produces identical results
- Test full turn processing with new functions
- Compare output with baseline (pre-extraction)

**Regression Protection**:
- All 10 existing tests must continue passing
- No warning regressions (maintain level 9 clean)
- Behavioral equivalence verification

---

## Risk Assessment

### Extraction Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Behavioral change | Low | High | Comprehensive unit tests, before/after comparison |
| Performance regression | Very Low | Medium | Profile before/after, pure functions may improve |
| Build breakage | Very Low | Medium | Incremental commits, CMake validation |
| Test maintenance burden | Medium | Low | Good naming, clear test structure |
| Over-engineering | Low | Medium | Extract only pure calculations, avoid premature optimization |

### Success Factors

✅ **High Success Probability**:
1. All functions already documented (Phase 3 complete)
2. Clear testing categories assigned
3. Pure calculation logic (minimal I/O coupling)
4. Well-understood game mechanics
5. Existing test infrastructure in place

---

## Dependencies & Prerequisites

### Before Starting Extraction:

✅ **Complete** (Already Done):
- [ ] Phase 3 documentation (✅ DONE)
- [ ] Testing infrastructure (✅ Unity framework operational)
- [ ] CMake build system (✅ Functional)
- [ ] Clean compilation (✅ Level 9 zero warnings)

⏳ **Required** (To Do):
- [ ] Create extraction analysis reports for other files
- [ ] Decide on branching strategy
- [ ] Create new source file stubs
- [ ] Update CMakeLists.txt for new files

---

## Conclusion

### Overall Assessment: ⭐⭐⭐⭐⭐ EXCELLENT

`update.c` represents an **ideal extraction candidate** with:
- Clear separation between calculation and I/O
- Well-documented functions with testing guidance
- High-value business logic (economics, population, military)
- Low extraction risk (pure mathematical functions)
- Substantial testability improvements (54-74 new tests)

### Key Decision Point:

**This file alone meets the entire Phase 10.2.2 goal**, but analysis of other files (forms.c, navy.c, combat.c, etc.) will likely reveal similar opportunities, suggesting **total modernization scope may be 100+ extracted functions** across the codebase.

**Recommendation**: Complete extraction analysis for all target files before beginning implementation to make informed branching/versioning decision.

---

**Report Generated**: 2025-10-10
**Next Steps**: Analyze forms.c, navy.c, combat.c, move.c, randeven.c, extcmds.c, cexecute.c
**Final Deliverable**: Comparative summary report with re-engineering recommendation

---

**Analysis Completed By**: Claude (claude-sonnet-4-5@20250929)
**Session**: Phase 10.2.2 Planning - Pure Function Extraction Assessment
