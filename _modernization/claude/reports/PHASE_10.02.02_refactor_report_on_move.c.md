# Phase 10.2.2 - Deep Analysis Report: move.c

**Analysis Date**: 2025-10-12
**File**: move.c (881 lines, 2 functions)
**Phase**: 10.2.2 - Deep Refactoring Assessment
**Analyst**: Claude (claude-sonnet-4-5)

---

## Executive Summary

**File Type**: Type 2+ (Core Subsystem with Minor Debt)
**Overall Quality**: ⭐⭐⭐⭐ VERY GOOD (95% well-placed)

**Key Findings**:
- ✅ **Excellent architectural placement** for core movement interface
- ✅ **Good documentation** (comprehensive Phase 3 function headers)
- ✅ **Appropriate UI coupling** for interactive game system
- ⚠️ **Minor naming inconsistency** (file suggests pathfinding, contains UI)
- ⚠️ **Moderate configuration coupling** (6 magic numbers for game balance)
- ✅ **Very low extraction potential** (appropriate as interactive layer)

**3-Part Analysis Results**:
1. **Extraction Potential**: ⭐ VERY LOW (5-10% extractable, 0% recommended)
2. **Architectural Placement**: ⭐⭐⭐⭐ VERY GOOD (95% well-placed, 5% minor debt)
3. **Configuration Coupling**: ⭐⭐⭐ MODERATE (6 magic numbers need externalization)

---

## Part 1: Unit Testable Extraction Potential

### File Statistics

| Metric | Value |
|--------|-------|
| Total Lines | 881 |
| Total Functions | 2 |
| Function 1: mymove() | 605 lines (69%) |
| Function 2: getselunit() | 29 lines (3%) |
| Comments/Blank | 247 lines (28%) |

### Function-by-Function Extraction Analysis

#### Function 1: mymove() - Interactive Movement Interface

**Location**: Lines 121-726 (605 lines)
**Type**: Interactive UI orchestration function
**Extraction Potential**: ⭐ VERY LOW (~5%)

**Responsibilities**:
- Interactive keyboard input loop (getch())
- Real-time display updates (curses: mvprintw, refresh, etc.)
- Movement validation (terrain, costs, diplomatic restrictions)
- Movement cost application (deduct from unit move points)
- Army group coordination (leader-follower relationships)
- Zone of control enforcement (enemy presence movement penalties)
- Sector ownership changes (territory conquest mechanics)
- Map display updates (cursor positioning, sector highlighting)

**Coupling Analysis**:
- **UI Coupling**: DEEP - Uses curses extensively (mvprintw, getch, refresh, standout, etc.)
- **Game State Coupling**: DEEP - Modifies units, sectors, nations directly
- **Display Coupling**: DEEP - Coordinates with map rendering, highlighting, fog of war
- **Input Coupling**: DEEP - Keyboard handling for directional movement

**Possible Micro-Extractions** (NOT RECOMMENDED):

| Code Section | Lines | Potential Function | Why NOT Extract |
|--------------|-------|-------------------|-----------------|
| Army group movement calc | 184-200 (~15) | `calc_army_group_movement()` | Requires 8+ global variables, no computational benefit |
| Zone of control logic | 500-518 (~18) | `apply_zone_of_control()` | Needs full nation arrays, complex game state |
| Enemy soldier counting | 461-491 (~30) | `count_enemy_soldiers()` | Requires all nation data, not computation-heavy |
| Sector ownership check | 599-668 (~69) | `handle_sector_capture()` | Complex dependencies, UI integration |

**Why Extraction is Inappropriate**:
1. **Parameter explosion**: Each extraction would require 10-15+ parameters for game state
2. **No computational benefit**: Simple counting/checking logic, not algorithms
3. **Testing penalty**: Would still require full game state mocks, no testability gain
4. **Readability loss**: Function is already well-documented and readable
5. **Negative ROI**: Increases complexity without any architectural benefit

**Recommendation**: ✅ **KEEP AS-IS** - This function is appropriately structured as an interactive UI layer. Extraction would harm readability and maintainability.

#### Function 2: getselunit() - Unit Selection Calculator

**Location**: Lines 853-881 (29 lines)
**Type**: Deterministic selection calculation
**Extraction Potential**: ⭐⭐ MODERATE (50%)

**Responsibilities**:
- Count armies at cursor position
- Count navies at cursor position
- Calculate selected unit ID based on pager/selector
- Set global armornvy flag (ARMY/NAVY)

**Coupling Analysis**:
- **Global Variables**: selector, pager, XREAL, YREAL, country
- **Data Structures**: curntn->arm[], curntn->nvy[]
- **Side Effect**: Sets armornvy global flag

**Extractable Pure Form**:
```c
int calculate_selected_unit(
    int selector, int pager, int scrarm,
    int xreal, int yreal, int country,
    const struct army *armies, int max_armies,
    const struct navy *navies, int max_navies,
    int *out_unit_type  /* Returns ARMY or NAVY */
) {
    // Pure counting and index calculation
    // No global state access
    // Fully testable
}
```

**Extraction Benefits**:
- ✅ Pure function possible (deterministic)
- ✅ Unit testable with simple inputs
- ✅ No UI coupling

**Extraction Costs**:
- ⚠️ Function already simple (29 lines)
- ⚠️ Well-documented and readable
- ⚠️ Low computational complexity (O(n) scan)
- ⚠️ Testing benefit minimal (logic already straightforward)

**ROI Assessment**: ⭐⭐ LOW
- Extraction effort: 2-3 hours (create pure function, add tests, update caller)
- Benefit: Minor testability improvement for already-simple logic
- **Recommendation**: ❌ **DO NOT EXTRACT** - Cost exceeds benefit

### Overall Extraction Assessment

**Extractable Functions**: 0 (getselunit is borderline but not recommended)
**Total Functions**: 2
**Extraction Percentage**: 0% (by recommended count), 3% (by line count if getselunit extracted)
**Priority**: ⭐ VERY LOW

**Final Recommendation**: ✅ **NO EXTRACTION RECOMMENDED**

**Rationale**:
- mymove() is appropriately structured as interactive UI layer
- getselunit() is simple enough that extraction provides minimal benefit
- File serves its purpose well as movement interface
- Extraction would increase complexity without architectural improvement

---

## Part 2: Architectural Placement Analysis

### File Purpose vs. Reality

| Aspect | Expected | Actual | Assessment |
|--------|----------|--------|------------|
| **File name** | move.c | move.c | ✅ Appropriate |
| **Header comment** | "Unit movement and pathfinding" | Interactive movement UI | ⚠️ Minor discrepancy |
| **Primary function** | Movement algorithms | Movement interface | ⚠️ Naming issue |
| **Secondary function** | Pathfinding | Unit selection | ✅ Reasonable |

**Naming Inconsistency**: File name suggests pathfinding algorithms, but contains only interactive UI for movement. This is a **minor naming issue**, not a placement problem.

### Function-by-Function Placement Analysis

#### mymove() - Interactive Movement Interface (605 lines, 69%)

**Current Location**: move.c
**Belongs Here?**: ⭐⭐⭐⭐⭐ **PERFECT PLACEMENT**

**Responsibilities**:
1. **Interactive UI**: Keyboard input loop for directional movement
2. **Movement Validation**: Terrain passability, diplomatic restrictions, movement costs
3. **Cost Application**: Deduct movement points based on terrain and mode
4. **Army Groups**: Coordinate leader-follower movement relationships
5. **Zone of Control**: Enforce enemy presence movement penalties
6. **Territory Control**: Handle sector ownership changes on movement
7. **Display Integration**: Real-time map updates, cursor positioning, highlighting

**Cohesion Analysis**: ⭐⭐⭐⭐⭐ **EXCELLENT**
- All responsibilities directly relate to unit movement
- No unrelated functionality mixed in
- Clear single purpose: interactive movement interface

**Placement Verdict**: ✅ **PERFECT** - This IS the core movement system, appropriately placed.

#### getselunit() - Unit Selection Calculator (29 lines, 3%)

**Current Location**: move.c
**Belongs Here?**: ⭐⭐⭐ **ACCEPTABLE** (minor architectural debt)

**Responsibilities**:
- Calculate selected unit ID from cursor position and pager state
- Distinguish between armies and navies
- Set global unit type flag

**Usage Analysis**:
- **Primary Caller**: mymove() (called once at line 136)
- **Likely Other Callers**: Other UI functions needing unit selection
- **Coupling Risk**: Creates dependency if other modules import this function

**Alternative Placements**:
1. **ui_selection.c** - Generic UI selection utilities module
2. **unit_selection.c** - Dedicated unit selection module
3. **Keep in move.c** - Co-locate with primary user

**Cohesion Concern**: ⚠️ Function serves multiple callers, creates coupling

**Placement Verdict**: ⭐⭐⭐ **ACCEPTABLE** - Reasonable to co-locate with primary caller, represents only 3% of file. Minor architectural debt.

### Misplaced Functions Summary

| Function | Current Location | Ideal Location | Severity | Priority |
|----------|------------------|----------------|----------|----------|
| getselunit() | move.c | ui_selection.c (optional) | ⭐ MINOR | P3 (Low) |

**Misplaced Percentage**: 3% of code (1 of 2 functions, 29 of 881 lines)

### Missing Functionality Analysis

**File Name Suggests**: Pathfinding algorithms
**File Actually Contains**: Interactive movement UI

**What's Missing**:
- ❌ **Pathfinding algorithms**: A* search, Dijkstra, breadth-first search
- ❌ **Movement cost calculations**: Appear to be in external movecost[][] array
- ❌ **Path optimization**: Shortest path, fastest path calculations
- ✅ **Movement interface**: Present and well-implemented
- ✅ **Movement validation**: Present and comprehensive

**Verdict**: This is a **naming discrepancy**, not a **functionality problem**.

**Recommendation**: Consider renaming to `movement_ui.c` or `interactive_movement.c` in Phase 10.4+ to accurately reflect content.

### Architectural Debt Assessment

**Overall Placement Quality**: ⭐⭐⭐⭐ **VERY GOOD** (95% well-placed)

**Breakdown**:
- **Well-Placed**: mymove() (605 lines, 69%) - Perfect placement
- **Minor Debt**: getselunit() (29 lines, 3%) - Acceptable with minor coupling concern
- **Comments/Blank**: 247 lines (28%)

**Debt Severity**: ⭐ VERY LOW
- Only one borderline function (3% of code)
- No clear architectural violations
- Good cohesion overall

**Recommendations**:
1. ✅ **Keep mymove() in move.c** - Perfect placement, no changes needed
2. ⚠️ **Consider extracting getselunit()** to ui_selection.c in Phase 10.4+ (low priority)
3. 📝 **Consider renaming file** to movement_ui.c in Phase 10.4+ for clarity (cosmetic)

---

## Part 3: Configuration Coupling Identification

### Magic Numbers Inventory

**Total Magic Numbers Found**: 6
**Severity**: ⭐⭐⭐ MODERATE
**Impact**: Game balance, naval warfare, territory control

#### 1. Harbor Entry Cost

**Location**: Line 394-396
**Code**:
```c
if (P_NMOVE >= 3)
    P_NMOVE -= 3;
else {
    errormsg("You need 3 move points for that");
```

**Value**: 3 movement points
**Context**: Cost for naval fleet to enter harbor (town/capitol/city with port access)
**Game Impact**: Naval movement efficiency, trade route planning
**Recommendation**:
```c
#define HARBOR_ENTRY_COST 3
```
**Target File**: game_config.h (Phase 10.3+)

#### 2. Naval Landing Cost

**Location**: Line 414-421
**Code**:
```c
if (P_NMOVE >= 4)
    P_NMOVE = 0;
else {
    beep();
    valid = FALSE;
    errormsg("You need 4 move points to land");
```

**Value**: 4 movement points (consumes all remaining movement)
**Context**: Cost for amphibious landing/invasion from ships to land
**Game Impact**: Major balance factor for amphibious warfare, naval invasions
**Recommendation**:
```c
#define NAVAL_LANDING_COST 4
```
**Target File**: game_config.h (Phase 10.3+)

#### 3. Minimum Blocking Population

**Location**: Line 348
**Code**:
```c
&& (sct[XREAL][YREAL].people > 100)
&& (ntn[sct[XREAL][YREAL].owner].dstatus[country] > ALLIED)
```

**Value**: 100 people
**Context**: Minimum population required in a sector to block foreign army movement
**Game Impact**: Movement restrictions, territorial control, population importance
**Recommendation**:
```c
#define MIN_BLOCKING_POPULATION 100
```
**Target File**: game_config.h (Phase 10.3+)

#### 4. Movement Overflow Detection

**Location**: Line 511-512
**Code**:
```c
if (P_AMOVE > 150)
    P_AMOVE = 0;
```

**Value**: 150
**Context**: Detects unsigned char underflow (when subtraction results in 255, 254, etc.)
**Game Impact**: Bug prevention - catches movement point arithmetic errors
**Technical Note**: P_AMOVE is unsigned char (0-255), negative values wrap to 255+
**Recommendation**:
```c
#define MAX_MOVEMENT_POINTS 150
/* Or more explicitly: */
#define MOVEMENT_UNDERFLOW_THRESHOLD 150
```
**Target File**: game_config.h (Phase 10.3+)

#### 5. Naval Encounter Stop Probability

**Location**: Line 524
**Code**:
```c
if ((P_NWAR(N_LIGHT) + P_NWAR(N_MEDIUM) + P_NWAR(N_HEAVY) < total)
    && (rand() % 4 == 0))
```

**Value**: 25% probability (1 in 4 chance)
**Context**: Probability that outnumbered naval fleet stops when encountering enemy warships
**Game Impact**: Naval combat engagement mechanics, fleet survival
**Recommendation**:
```c
#define NAVAL_ENCOUNTER_STOP_DIVISOR 4
/* Or more clearly: */
#define NAVAL_ENCOUNTER_STOP_CHANCE_PERCENT 25
```
**Target File**: game_config.h (Phase 10.3+)

#### 6. Overwhelming Force Multiplier

**Location**: Line 635
**Code**:
```c
|| (groupmen + othermen > 7 * total)
) && (curntn->dstatus[SOWN] > HOSTILE)
```

**Value**: 7x enemy strength
**Context**: Required force superiority to capture occupied enemy sector
**Game Impact**: **Major** combat balance factor, affects difficulty of sector conquest
**Recommendation**:
```c
#define OVERWHELMING_FORCE_MULTIPLIER 7
```
**Target File**: game_config.h (Phase 10.3+)

### Well-Defined Constants (Verification Needed)

**Constants Already Used** (likely in header.h or data.h):

| Constant | Usage Lines | Purpose | Verification Status |
|----------|-------------|---------|---------------------|
| TAKESECTOR | 615, 633, 657, 662 | Minimum soldiers to capture sector | ✅ Likely defined |
| LONGTRIP | 706, 712 | Max naval trip for civilian survival | ✅ Likely defined |
| WATER | 300, 581 | Altitude constant for water sectors | ✅ Likely defined |
| SCRARM | 859, 869 | Screen armies visible per page | ✅ Likely defined |
| NUMSTATUS | 158, 170, 190, 674 | Army status enumeration boundary | ✅ Likely defined |
| MINLEADER | 191, 345, 498, 601, 658 | Minimum leader unit type ID | ✅ Likely defined |
| MINMONSTER | 345, 498 | Minimum monster unit type ID | ✅ Likely defined |
| MAXARM | 142, 189, 272, etc. | Maximum armies per nation | ✅ Likely defined |
| MAXNAVY | 279, 480, 866 | Maximum navies per nation | ✅ Likely defined |

### Configuration Coupling Assessment

**Overall Rating**: ⭐⭐⭐ **MODERATE**

**Breakdown**:
- **Magic Numbers**: 6 requiring externalization
- **Well-Defined Constants**: 9+ already properly defined
- **Impact**: Moderate - affects game balance significantly
- **Concentration**: Focused in movement/combat rules
- **Current State**: Hardcoded but localized

**Severity Analysis**:

| Impact Category | Count | Examples | Priority |
|----------------|-------|----------|----------|
| **High Impact** | 2 | Naval landing (4), Overwhelming force (7x) | P2 (Medium) |
| **Medium Impact** | 3 | Harbor cost (3), Blocking pop (100), Encounter chance (25%) | P3 (Low) |
| **Low Impact** | 1 | Movement overflow (150) | P4 (Very Low) |

**Externalization Plan** (Phase 10.3+):

1. **Create game_config.h section** for movement configuration:
```c
/* Movement Cost Configuration */
#define HARBOR_ENTRY_COST 3
#define NAVAL_LANDING_COST 4

/* Movement Restrictions */
#define MIN_BLOCKING_POPULATION 100
#define MAX_MOVEMENT_POINTS 150

/* Combat Balance */
#define OVERWHELMING_FORCE_MULTIPLIER 7
#define NAVAL_ENCOUNTER_STOP_DIVISOR 4
```

2. **Update move.c** to use defined constants
3. **Test gameplay balance** after externalization
4. **Document configuration** in game design notes

**Estimated Effort**: 1-2 hours (define constants, update code, test)

---

## Comparison to Previous Files

### File Type Classification

**move.c Type**: Type 2+ (Core Subsystem with Minor Debt)

**Similar Files**:
- ✅ **check.c** - Type 2+ (Core validation, 20% misplaced, 3 magic numbers)
- ✅ **extcmds.c** - Type 2+ (Command subsystem, 33% misplaced, 6 magic numbers)

**Differences from Similar Files**:
- move.c has **better placement** (5% vs. 20-33% misplaced)
- move.c has **similar config coupling** (6 vs. 3-6 magic numbers)
- move.c has **larger code size** (881 vs. 250-450 lines)
- move.c has **deeper UI integration** (appropriate for interactive system)

### Quality Metrics Comparison

| File | Extraction | Placement | Config | Overall |
|------|-----------|-----------|--------|---------|
| **m2alloc.c** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | Gold Standard |
| **cexecute.c** | ⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐ | Very Good |
| **check.c** | ⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ | Good |
| **extcmds.c** | ⭐ | ⭐⭐⭐ | ⭐⭐⭐ | Moderate |
| **move.c** | ⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ | **Very Good** |

**move.c Overall Quality**: ⭐⭐⭐⭐ **VERY GOOD**

**Strengths**:
- ✅ Excellent placement for core movement system
- ✅ Appropriate UI coupling for interactive interface
- ✅ Good documentation (comprehensive function headers)
- ✅ Clear responsibilities and cohesion

**Weaknesses**:
- ⚠️ Minor naming inconsistency (file vs. content)
- ⚠️ One borderline misplaced function (getselunit)
- ⚠️ Moderate configuration coupling (6 magic numbers)

---

## Refactoring Recommendations

### Priority 1: No Immediate Action Required ✅

**Recommendation**: Leave file as-is during Phase 10.2.2 analysis
**Rationale**: File is well-structured for its purpose, minimal architectural debt

### Priority 2: Configuration Externalization (Phase 10.3+) 📝

**Estimated Effort**: 1-2 hours
**Action Items**:
1. Create movement configuration section in game_config.h
2. Define 6 magic number constants with documentation
3. Update move.c to use defined constants
4. Test gameplay balance after changes
5. Document configuration in game design notes

**Expected Benefits**:
- ✅ Game balance tuning without code changes
- ✅ Clear documentation of movement mechanics
- ✅ Easier experimentation with game parameters

### Priority 3: Minor Architectural Cleanup (Phase 10.4+) 🔧

**Estimated Effort**: 2-3 hours
**Action Items**:
1. **Extract getselunit()** to ui_selection.c (optional)
   - Create new ui_selection.c module for selection utilities
   - Move getselunit() with proper parameter passing
   - Update callers across codebase
   - Add unit tests for selection logic
   - **Benefit**: Reduces coupling, improves reusability

2. **Rename file** to movement_ui.c (cosmetic, optional)
   - Update build system (CMakeLists.txt, Makefile)
   - Update #include references if needed
   - Update documentation references
   - **Benefit**: Clarifies file purpose (UI, not algorithms)

**Expected Benefits**:
- ✅ Clearer separation of concerns
- ✅ Improved naming consistency
- ✅ Better code organization

### Non-Recommendations (DO NOT DO) ❌

**1. DO NOT extract mymove() sub-functions**
- Reason: Parameter explosion, negative ROI, readability loss
- Keep function as monolithic interactive UI orchestrator

**2. DO NOT attempt pure function extraction**
- Reason: Deep game state coupling is appropriate for interactive system
- Interactive UI layer should coordinate game state directly

**3. DO NOT split movement modes into separate functions**
- Reason: Shared validation and display logic, already readable
- Single function handles all modes appropriately

---

## Testing Implications

### Current Testing Status

**Existing Documentation**: ✅ Comprehensive Phase 3 function headers
**Testing Category**: B (Integration) - Requires full game state and display system

**Both Functions**:
- Category B: Integration testing required
- Dependencies: Game map, unit data, movement costs, terminal interface
- Mock Requirements: Extensive game state, curses mocking

### Recommended Testing Strategy

**Integration Testing** (Primary Approach):

**Test Suite**: `tests/integration/test_movement_ui.c`

**Test Categories**:

1. **Basic Movement Tests** (10 tests)
   - Single-step army movement (plains, hills, mountains, water)
   - Single-step navy movement (shallow water, deep water, harbors)
   - Boundary checking (map edges, invalid coordinates)
   - Movement point deduction (various terrain costs)

2. **Army Group Movement** (5 tests)
   - General moving army group together
   - Breaking unit from army group (confirmation)
   - Group movement point synchronization
   - Infantry vs. leader movement calculations

3. **Flight Mode Tests** (4 tests)
   - Flight cost calculation
   - Landing mechanics
   - Drowning prevention (water sectors)
   - Altitude restrictions

4. **Naval Movement Tests** (6 tests)
   - Harbor entry (3 movement points)
   - Amphibious landing (4 movement points, invasion confirmation)
   - Deep water restrictions (light ships)
   - Naval encounter probability (25% stop chance)
   - Civilian survival calculation (long trips)
   - Army transport on ships

5. **Zone of Control Tests** (5 tests)
   - ZOC movement stopping (outnumbered)
   - ZOC movement penalty (proportional reduction)
   - Scout/ninja ZOC exemption
   - Flying unit ZOC exemption
   - Naval ZOC encounter mechanics

6. **Sector Ownership Tests** (6 tests)
   - Unowned sector capture (TAKESECTOR threshold)
   - Enemy sector capture (overwhelming force 7x)
   - Occupied sector failure (insufficient force)
   - Population flee mechanics (race differences, slaver magic)
   - Town/city entry (no automatic capture)
   - Diplomatic restrictions (allied/war status)

7. **Unit Selection Tests** (4 tests)
   - Single army selection (getselunit)
   - Single navy selection (getselunit)
   - Multiple units paging (SCRARM boundary)
   - Empty sector (no unit found)

**Total Recommended Tests**: ~40 integration tests

**Estimated Test Creation Effort**: 8-12 hours

### Testing Challenges

**Challenge 1: Curses Mocking**
- **Problem**: Heavy curses dependency (getch, mvprintw, refresh)
- **Solution**: Use ncurses test harness or mock curses library
- **Alternative**: Refactor display calls to wrapper functions (Phase 10.4+)

**Challenge 2: Global Game State**
- **Problem**: Requires full sct[][], ntn[], curntn initialization
- **Solution**: Create comprehensive test fixtures with minimal game state
- **Estimated Fixture Creation**: 4-6 hours

**Challenge 3: Random Number Testing**
- **Problem**: Naval encounter uses `rand() % 4` (line 524)
- **Solution**: Seed random number generator for deterministic tests
- **Alternative**: Refactor to dependency injection (Phase 10.4+)

### Testing ROI Assessment

**Investment**: 12-18 hours (fixtures + tests)
**Benefit**: High - Movement is core gameplay mechanic
**Priority**: ⭐⭐⭐⭐ HIGH (test during Phase 8 expansion)

**Justification**:
- Movement affects almost every player action
- Complex interactions between modes (army/navy/flight)
- Many edge cases (boundaries, ZOC, ownership)
- Configuration changes need regression protection

---

## Patterns and Insights

### Pattern 1: Interactive UI Orchestration Function

**Observation**: mymove() is a 605-line orchestration function
**Pattern**: Large interactive UI functions are acceptable in game architectures
**Why Acceptable**:
- Interactive systems coordinate many concerns simultaneously
- Splitting would obscure control flow
- Already well-documented and readable
- Extraction provides negative ROI

**Contrast**: Modern web/enterprise apps prefer smaller functions
**Context**: Terminal-based game UI has different architectural constraints
**Lesson**: Architecture patterns depend on domain - interactive UI is different from business logic

### Pattern 2: Encoding Scheme for Type Distinction

**Implementation**: getselunit() returns:
- Army IDs: 0 to MAXARM-1
- Navy IDs: MAXARM to MAXARM+MAXNAVY-1

**Pattern**: Offset encoding to distinguish types without separate flag
**Benefits**:
- ✅ Single return value encodes both ID and type
- ✅ Caller can determine type from ID range
- ✅ No ambiguity (army 0 vs. navy 0)

**Drawbacks**:
- ⚠️ Requires decoding (selunit >= MAXARM check)
- ⚠️ Less explicit than struct return { int id; enum type; }

**Modern Alternative** (Phase 10.4+):
```c
struct selected_unit {
    int unit_id;      /* 0 to MAXARM-1 or 0 to MAXNAVY-1 */
    enum unit_type type;  /* ARMY or NAVY */
};

struct selected_unit getselunit(void);
```

**Verdict**: Legacy encoding acceptable, but modern struct return is clearer

### Pattern 3: Movement Cost Externalization

**Observation**: move.c uses external movecost[][] array
**Pattern**: Movement costs calculated/stored elsewhere, consumed here
**Architecture**:
- **Separation**: Cost calculation vs. cost application
- **move.c**: Applies costs, handles UI interaction
- **Unknown module**: Calculates terrain-based costs

**Missing Documentation**: Where is movecost[][] populated?
**Action**: Document movement cost initialization in future analysis

### Pattern 4: Magic Number Clustering

**Observation**: 6 magic numbers, all related to game balance
**Pattern**: Configuration coupling concentrated in gameplay mechanics
**Implication**: Externalizing these affects game balance tuning
**Best Practice**: Group related constants in configuration header

```c
/* Movement Configuration */
#define HARBOR_ENTRY_COST 3
#define NAVAL_LANDING_COST 4
#define MIN_BLOCKING_POPULATION 100

/* Combat Balance */
#define OVERWHELMING_FORCE_MULTIPLIER 7
```

---

## File Classification Summary

### File Type: Type 2+ (Core Subsystem with Minor Debt)

**Characteristics**:
- ✅ Core gameplay system (movement interface)
- ✅ Mostly well-placed functions (95%)
- ⚠️ Minor architectural debt (5% - getselunit placement)
- ⚠️ Moderate configuration coupling (6 magic numbers)
- ✅ Good documentation
- ✅ Appropriate UI integration

**Comparison to Type 2 (Pure Core Engine)**:
- Similar low extraction potential
- Similar low architectural debt
- Move.c has UI coupling (cexecute.c has game loop coupling)

**Comparison to Type 3 (Mixed Responsibility)**:
- Much better placement (5% vs. 29% misplaced)
- Similar config coupling (6 vs. 50+ magic numbers)
- Better cohesion (single purpose vs. multiple concerns)

### Quality Assessment

**Overall Grade**: ⭐⭐⭐⭐ **VERY GOOD** (A- grade, 90-95%)

**Strengths**:
1. ✅ **Excellent architectural placement** for movement system
2. ✅ **Comprehensive documentation** (Phase 3 function headers)
3. ✅ **Appropriate coupling** for interactive UI layer
4. ✅ **Good cohesion** - single clear purpose
5. ✅ **Readable code** - despite size, well-structured

**Weaknesses**:
1. ⚠️ **Moderate configuration coupling** - 6 magic numbers
2. ⚠️ **Minor naming inconsistency** - file name vs. content
3. ⚠️ **Minor architectural debt** - getselunit placement
4. ⚠️ **Deep UI coupling** - makes testing complex (appropriate for purpose)

**Verdict**: File is in very good condition, minimal refactoring needed, appropriate for its role as interactive movement interface.

---

## Session Statistics

**Analysis Duration**: ~90 minutes
**Functions Analyzed**: 2
**Lines Analyzed**: 881
**Magic Numbers Found**: 6
**Architectural Issues**: 1 minor (getselunit placement)
**Extraction Opportunities**: 0 recommended

---

## Conclusion

**move.c Assessment**: ⭐⭐⭐⭐ VERY GOOD (95% well-placed, minimal debt)

**Key Takeaways**:
1. ✅ **Leave as-is during Phase 10.2.2** - No immediate refactoring needed
2. 📝 **Externalize configuration** in Phase 10.3 (6 magic numbers, 1-2 hours)
3. 🔧 **Optional cleanup** in Phase 10.4+ (getselunit extraction, file rename)
4. ✅ **Appropriate architecture** for interactive UI system
5. ⭐⭐⭐⭐ **Test in Phase 8** - High priority integration testing (40 tests)

**Recommendation**: This file exemplifies **good interactive UI architecture** for terminal-based games. The large orchestration function is appropriate, coupling is justified, and documentation is excellent. Minimal refactoring effort required.

---

**Report Generated**: 2025-10-12
**Next File**: psmap.c (file #13)
**Phase Progress**: 11 of 30 files complete (37%)
