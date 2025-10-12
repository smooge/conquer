# makeworl.c Test Fixture Analysis

**Analysis Date**: 2025-10-12
**Purpose**: Determine effort to make world generation functions usable for testing other game systems
**Analyst**: Claude (claude-sonnet-4-5)

---

## Executive Summary

**Current State**: makeworl.c has **heavy UI coupling** preventing use as test fixture generator

**User Interaction Points**: 35+ UI calls in makeworld(), 3+ in populate()

**Core Problem**: World generation requires interactive user input (passwords, world size, water %, NPC choices)

**Solution**: Create non-interactive wrapper functions that accept parameters instead of prompting

**Effort Estimate**: 6-10 hours to create testable world generation infrastructure

**ROI**: ⭐⭐⭐⭐⭐ CRITICAL - Unlocks testing for ~60-70% of game functions that require pre-built worlds

---

## User Interaction Analysis by Function

### Function 1: makeworld() - ❌ HEAVILY INTERACTIVE

**UI Interaction Points**: 35+ curses calls

**Interactive prompts**:
1. **Password setup** (lines 215-235):
   - Prompt for super-user password
   - Re-enter password for confirmation
   - Validation (4-PASSLTH characters)

2. **Demi-god setup** (lines 240-310):
   - Prompt for alternate administrator
   - System login validation
   - Default handling

3. **World dimensions** (lines 314-342):
   - Prompt for X sectors
   - Prompt for Y sectors
   - Validation (divisible by 8, >23)

4. **Water percentage** (lines 346-359):
   - Prompt for water % (0-100)
   - Validation

5. **Map reading** (line 361-364):
   - rflag parameter determines read vs. create

**Display output**: Progress messages throughout (35+ mvaddstr calls)

**Dependencies**:
- Curses initialized (newinit())
- File system (password files, news files, cleanup)
- Conditional compilation (REMAKE, CHECKUSER)

**Testability**: ⭐ VERY LOW - Cannot use as-is for test fixtures

### Function 2: createworld() - ✅ NON-INTERACTIVE (Usable!)

**UI Interaction Points**: 11 display messages (status only, no input)

**Input dependencies**:
- Global `pwater` (0-100) - water percentage
- Global `world.mapx`, `world.mapy` - world dimensions
- Global `sct[][]` - sector array (must be allocated)
- Global random seed (rand/srand)

**Output**:
- Modifies global `sct[][]` array with terrain
- Allocates/frees temporary arrays (tplace, area_map, type)

**Side effects**:
- Display messages (can be stubbed)
- Sleep calls (can be removed for testing)

**Testability**: ⭐⭐⭐⭐⭐ EXCELLENT
- **Can be called directly for tests**
- Just need to set globals first
- Remove sleep() calls for speed
- Stub display for batch testing

### Function 3: rawmaterials() - ✅ NON-INTERACTIVE (Usable!)

**UI Interaction Points**: 6 display messages (status only, no input)

**Input dependencies**:
- Global `sct[][]` - terrain must exist (from createworld)
- Global constants (TRADEPCT, METALPCT, JEWELPCT)
- Global trade good arrays

**Output**:
- Modifies `sct[][]` with resources and trade goods
- Calls populate()

**Side effects**:
- Display messages
- Sleep calls
- Calls populate() (which has interaction)

**Testability**: ⭐⭐⭐⭐ GOOD
- **Can be called directly**
- May need to stub populate() to avoid NPC interaction

### Function 4: populate() - ⚠️ PARTIALLY INTERACTIVE

**UI Interaction Points**:
- 0 for monsters (always runs)
- 3+ for NPCs (conditional compilation #ifdef NPC)

**Interactive prompts** (under #ifdef NPC):
1. **NPC nations prompt** (lines 1572-1580):
   - "Do you want NPC nations?" (y/n)
   - Can skip by answering 'n'

2. **Default NPC file** (lines 1583-1599):
   - "Use default NPC nations file?" (y/n)
   - Can skip by providing npcsfile

**Input dependencies**:
- Global `sct[][]` - terrain and resources must exist
- File: npcsfile (NPC nation definitions)
- Files: helpfiles (validation check)
- Conditional: MONSTER flag (monster placement)
- Conditional: NPC flag (NPC nation loading)

**Output**:
- Creates god nation (ntn[0])
- Places monster nations (pirates, savages, nomads, lizards)
- Loads and places NPC nations from file
- Sets diplomatic relationships

**Testability**: ⭐⭐⭐ MODERATE
- **Monster placement is non-interactive**
- **NPC loading requires user input OR file**
- Can bypass NPC interaction by:
  - Disabling #ifdef NPC
  - Pre-creating NPC file
  - Stubbing file prompts

### Function 5: zeroworld() - ✅ NON-INTERACTIVE (Usable!)

**UI Interaction Points**: 0 (pure logic)

**Input dependencies**:
- Global `ntn[]` array (must be allocated)

**Output**:
- Initializes all nation structures to defaults

**Testability**: ⭐⭐⭐⭐⭐ PERFECT
- **Already usable for tests**
- No dependencies
- Pure initialization

### Function 6: fill_edge() - ✅ NON-INTERACTIVE (Usable!)

**UI Interaction Points**: 0 (pure logic)

**Input dependencies**:
- Global `area_map[][]` and `type[][]` arrays

**Output**:
- Fills edge sectors based on area types

**Testability**: ⭐⭐⭐⭐⭐ PERFECT
- **Already usable for tests**
- Helper function for createworld()

---

## Global Dependencies Analysis

### Critical Global State

**World structure** (defined in data.h):
```c
struct world {
    short mapx;  // X dimension
    short mapy;  // Y dimension
} world;
```

**Sector array** (dynamically allocated):
```c
struct s_sector **sct;  // Allocated via getspace()
```

**Nation array** (statically allocated):
```c
struct nation ntn[NTOTAL];  // All nations
```

**World generation parameters**:
```c
int pwater;  // Water percentage (0-100)
```

**Random number generator**:
```c
srand(seed);  // Must seed for reproducibility
```

### Memory Allocation

**getspace()** - Allocates world memory (called in makeworld line 343):
- Allocates `sct[][]` array based on `world.mapx` × `world.mapy`
- Must be called before createworld()

**m2alloc()** - Allocates 2D arrays (used in createworld):
- Used for temporary arrays (tplace, area_map, type)
- Handled internally by createworld()

---

## Creating Test-Friendly World Generation

### Approach 1: Wrapper Function (Recommended)

**Create**: `create_test_world()` - Non-interactive world generator

**Signature**:
```c
/**
 * create_test_world - Generate world for testing without user interaction
 *
 * Creates a complete game world (terrain, resources, nations) using
 * provided parameters instead of prompting user. Suitable for creating
 * test fixtures and automated testing.
 *
 * Parameters:
 *   config - World generation configuration
 *
 * Returns:
 *   0 on success, -1 on error
 */
int create_test_world(struct test_world_config *config);
```

**Configuration structure**:
```c
struct test_world_config {
    /* World dimensions */
    short map_width;      // Must be divisible by 8, >= 24
    short map_height;     // Must be divisible by 8, >= 24

    /* Generation parameters */
    int water_percent;    // 0-100
    unsigned int seed;    // RNG seed for reproducibility

    /* Nations */
    bool create_monsters; // Create monster nations (pirates, etc.)
    bool create_npcs;     // Create NPC nations from file
    char *npc_file;       // Path to NPC nation file (NULL = skip)

    /* Password (for god nation) */
    char god_password[PASSLTH + 1];  // Pre-encrypted or plain
    char demi_god[LEADERLTH + 1];    // Alternate admin (empty = none)

    /* Testing options */
    bool silent;          // Suppress display output
    bool skip_sleep;      // Skip sleep() calls for speed
};
```

**Implementation**:
```c
int create_test_world(struct test_world_config *config) {
    /* 1. Validate configuration */
    if (config->map_width % 8 != 0 || config->map_width < 24) return -1;
    if (config->map_height % 8 != 0 || config->map_height < 24) return -1;
    if (config->water_percent < 0 || config->water_percent > 100) return -1;

    /* 2. Set global state */
    world.mapx = config->map_width;
    world.mapy = config->map_height;
    pwater = config->water_percent;
    srand(config->seed);  // Reproducible worlds

    /* 3. Allocate world memory */
    getspace();

    /* 4. Initialize nations */
    zeroworld();

    /* 5. Set up god nation (without password prompts) */
    setup_god_nation(config->god_password, config->demi_god);

    /* 6. Generate terrain */
    createworld_silent(config->silent, config->skip_sleep);

    /* 7. Place resources */
    rawmaterials_silent(config->silent, config->skip_sleep);

    /* 8. Populate world (monsters + NPCs) */
    if (config->create_monsters) {
        populate_monsters_silent(config->silent);
    }

    if (config->create_npcs && config->npc_file != NULL) {
        populate_npcs_from_file(config->npc_file, config->silent);
    }

    return 0;
}
```

**Benefits**:
- ✅ No user interaction required
- ✅ Reproducible worlds (fixed seed)
- ✅ Configurable (dimensions, water %, monsters, NPCs)
- ✅ Fast (skip sleep calls)
- ✅ Silent (suppress display output)

**Effort**: 4-6 hours

### Approach 2: Refactor Existing Functions

**Modify existing functions** to accept parameters:

**Changes needed**:
1. **makeworld() → makeworld_ex()**:
   - Add `struct world_params *params` parameter
   - Replace prompts with parameter reads
   - Keep existing makeworld() as wrapper calling makeworld_ex()

2. **createworld() → createworld_ex()**:
   - Add parameters: `int water_pct, bool silent, bool fast`
   - Replace global `pwater` with parameter
   - Conditional display output
   - Conditional sleep calls

3. **populate() → populate_ex()**:
   - Add parameters: `bool monsters, bool npcs, char *npc_file, bool silent`
   - Replace prompts with parameters
   - Conditional file loading

**Benefits**:
- ✅ Backward compatible (keep original functions)
- ✅ More granular control

**Drawbacks**:
- ⚠️ More invasive changes
- ⚠️ Duplicates code (original + _ex versions)

**Effort**: 6-8 hours

### Approach 3: Stub Display Functions (Quick & Dirty)

**Create stub versions** of display functions for testing:

**Stub header** (test_stubs.h):
```c
#ifdef TESTING
#define mvaddstr(y, x, str) ((void)0)
#define refresh() ((void)0)
#define sleep(n) ((void)0)
#define newerror(msg) ((void)0)
#define newmsg(msg) ((void)0)
#endif
```

**Still requires**:
- Mocking user input functions (get_pass, get_number, etc.)
- Setting globals before calling functions

**Benefits**:
- ✅ Minimal code changes
- ✅ Quick to implement

**Drawbacks**:
- ⚠️ Still need to handle prompts (mock inputs)
- ⚠️ Not clean solution
- ⚠️ Fragile (depends on #ifdef)

**Effort**: 1-2 hours

---

## Recommended Implementation Plan

### Phase 1: Core Infrastructure (4-6 hours)

**Deliverables**:
1. `struct test_world_config` definition
2. `create_test_world()` wrapper function
3. Helper functions:
   - `setup_god_nation()` - Non-interactive god setup
   - `createworld_silent()` - Terrain generation without display
   - `rawmaterials_silent()` - Resource placement without display
   - `populate_monsters_silent()` - Monster placement only
   - `populate_npcs_from_file()` - NPC loading from file

**Files**:
- `test_world_generator.c` - New file with test fixture functions
- `test_world_generator.h` - Header with config struct

**Approach**:
- Extract core logic from existing functions
- Call existing functions with globals pre-set
- Stub display output (conditional compilation or wrapper)

### Phase 2: Test Fixtures Library (2-4 hours)

**Deliverables**:
1. Pre-defined world configurations:
   - `test_world_small()` - 24×24, 50% water, no monsters
   - `test_world_medium()` - 64×64, 60% water, monsters
   - `test_world_large()` - 128×128, 40% water, monsters + NPCs
   - `test_world_water()` - 32×32, 90% water (naval testing)
   - `test_world_land()` - 32×32, 10% water (land testing)

2. Seed-based fixtures:
   - Same seed = same world (reproducibility)
   - Document seed → world characteristics mapping

**Files**:
- `test_fixtures.c` - Pre-built world generator
- `test_fixtures.h` - Fixture function declarations

### Phase 3: Integration with Test Suite (2-3 hours)

**Deliverables**:
1. Setup/teardown functions:
   - `setup_test_world()` - Create world before test
   - `teardown_test_world()` - Free memory after test

2. World validation:
   - `validate_world()` - Verify world is properly generated
   - `validate_terrain()` - Check terrain consistency
   - `validate_resources()` - Check resource placement

**Integration**:
```c
// Example test using world fixture
void test_nation_expansion(void) {
    struct test_world_config config = {
        .map_width = 32,
        .map_height = 32,
        .water_percent = 50,
        .seed = 12345,
        .create_monsters = true,
        .create_npcs = false,
        .silent = true,
        .skip_sleep = true
    };

    // Create world
    assert_equal(0, create_test_world(&config));

    // Validate world
    assert_true(validate_world());

    // Test nation expansion logic
    test_expand_nation(1, 10, 10);  // Nation 1, sector (10,10)

    // Verify results
    assert_equal(ntn[1].tsctrs, 5);  // Expanded to 5 sectors

    // Cleanup
    teardown_test_world();
}
```

---

## Affected Test Categories

### Tests Unlocked by Test World Fixtures

**Combat System Tests** (combat.c):
- Army movement on terrain
- Naval combat in water sectors
- Fortress siege mechanics
- Terrain effects on combat

**Economic Tests** (trade.c):
- Trade good collection
- Resource extraction
- Market mechanics
- Economic calculations

**Magic System Tests** (magic.c):
- Spell effects on terrain
- Terraforming mechanics
- Power acquisition in context
- Unit summoning

**Movement Tests** (move.c):
- Movement cost calculation
- Terrain navigation
- Pathfinding validation
- Movement restrictions

**NPC Behavior Tests** (npc.c):
- Monster AI decisions
- NPC nation interactions
- Diplomatic mechanics
- Territory expansion

**Display Tests** (display.c):
- Map rendering
- Sector highlighting
- UI state management
- View navigation

**I/O Tests** (io.c):
- World saving/loading
- Turn processing
- File format validation

**Estimated Impact**: ~60-70% of game systems require pre-built worlds for meaningful testing

---

## Effort Summary

### Total Implementation Effort

**Phase 1**: Core Infrastructure (4-6 hours)
- create_test_world() wrapper
- Silent generation helpers
- Configuration structure

**Phase 2**: Fixture Library (2-4 hours)
- Pre-defined world generators
- Seed-based reproducibility
- World characteristic documentation

**Phase 3**: Test Integration (2-3 hours)
- Setup/teardown functions
- Validation helpers
- Integration examples

**Total**: 8-13 hours

### ROI Analysis

**Effort**: 8-13 hours
**Unlock**: Testing for 60-70% of game systems
**Value per hour**: ~5-9% of codebase becomes testable per hour
**ROI**: ⭐⭐⭐⭐⭐ CRITICAL - Highest testing ROI in entire project

**Comparison**:
- Unit test extraction: 13-18 hours, 70-95 tests, 10-15% of makeworl.c
- Test fixture creation: 8-13 hours, unlocks 60-70% of game systems

**Recommendation**: ⭐⭐⭐⭐⭐ IMMEDIATE PRIORITY - Do this before extensive unit test extraction

---

## Key Obstacles and Solutions

### Obstacle 1: Curses Dependency

**Problem**: makeworld() calls curses functions (mvaddstr, refresh, etc.)

**Solutions**:
1. **Conditional compilation**:
   ```c
   #ifdef TESTING
   #define mvaddstr(y, x, str) ((void)0)
   #endif
   ```

2. **Silent flag**:
   ```c
   if (!config->silent) {
       mvaddstr(10, 0, "Creating world");
   }
   ```

3. **Separate test functions**:
   - Keep original makeworld() for interactive use
   - Create create_test_world() for testing

**Recommendation**: Option 3 (separate functions) - cleanest approach

### Obstacle 2: Password Prompts

**Problem**: makeworld() requires interactive password entry

**Solution**: Pre-set god nation password in test config
```c
struct test_world_config config = {
    .god_password = "testpass",  // Pre-set password
    ...
};
```

**Implementation**: setup_god_nation() helper bypasses prompts

### Obstacle 3: NPC File Prompts

**Problem**: populate() prompts for NPC nation file

**Solutions**:
1. **Disable NPC flag**:
   ```c
   config->create_npcs = false;  // Skip NPC nations
   ```

2. **Pre-specify NPC file**:
   ```c
   config->npc_file = "test_npcs.txt";  // Use test file
   ```

3. **Stub NPC loading**:
   - Test only monster placement
   - Skip NPC nations

**Recommendation**: Option 2 (pre-specify file) - enables full testing

### Obstacle 4: Random Number Generation

**Problem**: Unseeded RNG produces different worlds each run

**Solution**: Seed RNG in test config
```c
config->seed = 12345;  // Same seed = same world
srand(config->seed);
```

**Benefits**:
- ✅ Reproducible test worlds
- ✅ Debuggable failures (re-run with same seed)
- ✅ Documented world characteristics per seed

### Obstacle 5: Memory Allocation

**Problem**: getspace() must be called before world generation

**Solution**: create_test_world() calls getspace() internally
```c
int create_test_world(struct test_world_config *config) {
    /* Set dimensions */
    world.mapx = config->map_width;
    world.mapy = config->map_height;

    /* Allocate memory */
    getspace();  // Handles sct[][] allocation

    /* Generate world */
    createworld_silent(...);
    ...
}
```

**Cleanup**: teardown_test_world() frees allocated memory

---

## Testing Examples

### Example 1: Basic World Generation Test

```c
void test_basic_world_generation(void) {
    struct test_world_config config = {
        .map_width = 32,
        .map_height = 32,
        .water_percent = 50,
        .seed = 12345,
        .create_monsters = false,
        .create_npcs = false,
        .god_password = "test",
        .demi_god = "",
        .silent = true,
        .skip_sleep = true
    };

    // Generate world
    int result = create_test_world(&config);
    assert_equal(0, result);

    // Validate water percentage (within ±5%)
    int water_count = 0;
    for (int x = 0; x < 32; x++) {
        for (int y = 0; y < 32; y++) {
            if (sct[x][y].altitude == WATER) {
                water_count++;
            }
        }
    }

    float water_pct = (water_count * 100.0) / (32 * 32);
    assert_in_range(water_pct, 45.0, 55.0);  // 50% ±5%

    // Cleanup
    teardown_test_world();
}
```

### Example 2: Combat System Test with World

```c
void test_army_movement_costs(void) {
    // Create test world
    create_test_world(&standard_config);

    // Place army in clear terrain
    place_test_army(1, 10, 10, 1000);  // Nation 1, (10,10), 1000 soldiers

    // Test movement to different terrain types
    int clear_cost = calculate_movement_cost(10, 10, 11, 10);  // Clear to clear
    int hill_cost = calculate_movement_cost(10, 10, 12, 10);   // Clear to hill

    // Verify costs
    assert_equal(1, clear_cost);  // Clear terrain = 1 move
    assert_equal(2, hill_cost);   // Hill terrain = 2 moves

    teardown_test_world();
}
```

### Example 3: Economic System Test

```c
void test_resource_extraction(void) {
    // Create resource-rich world
    struct test_world_config config = {
        .map_width = 32,
        .map_height = 32,
        .water_percent = 30,  // More land for resources
        .seed = 99999,        // Seed known to have metal deposits
        .create_monsters = false,
        .create_npcs = false,
        .silent = true,
        .skip_sleep = true
    };

    create_test_world(&config);

    // Find sector with metal
    int metal_x = -1, metal_y = -1;
    for (int x = 0; x < 32; x++) {
        for (int y = 0; y < 32; y++) {
            if (sct[x][y].metal > 0) {
                metal_x = x;
                metal_y = y;
                break;
            }
        }
        if (metal_x >= 0) break;
    }

    assert_not_equal(-1, metal_x);  // Found metal sector

    // Test mining
    place_test_nation(1, metal_x, metal_y);
    long initial_metal = ntn[1].metals;

    mine_sector(1, metal_x, metal_y);

    assert_greater_than(ntn[1].metals, initial_metal);  // Extracted metal

    teardown_test_world();
}
```

---

## Implementation Checklist

### Phase 1: Core Infrastructure

- [ ] Define `struct test_world_config` in new header
- [ ] Implement `create_test_world()` wrapper function
- [ ] Implement `setup_god_nation()` helper
- [ ] Implement `createworld_silent()` (or add silent flag)
- [ ] Implement `rawmaterials_silent()` (or add silent flag)
- [ ] Implement `populate_monsters_silent()` (extract from populate)
- [ ] Implement `populate_npcs_from_file()` (extract from populate)
- [ ] Test basic world generation (single fixture)

### Phase 2: Fixture Library

- [ ] Create test_fixtures.c and test_fixtures.h
- [ ] Implement `test_world_small()` (24×24, 50% water)
- [ ] Implement `test_world_medium()` (64×64, 60% water)
- [ ] Implement `test_world_large()` (128×128, 40% water)
- [ ] Implement `test_world_water()` (32×32, 90% water)
- [ ] Implement `test_world_land()` (32×32, 10% water)
- [ ] Document seed → world characteristic mappings
- [ ] Test reproducibility (same seed = same world)

### Phase 3: Test Integration

- [ ] Implement `setup_test_world()` setup function
- [ ] Implement `teardown_test_world()` cleanup function
- [ ] Implement `validate_world()` validation
- [ ] Implement `validate_terrain()` terrain checks
- [ ] Implement `validate_resources()` resource checks
- [ ] Create example tests using fixtures
- [ ] Document fixture usage in testing guide
- [ ] Integrate with existing test suite

---

## Recommendation

**Priority**: ⭐⭐⭐⭐⭐ CRITICAL - Highest testing ROI in project

**Approach**: Wrapper Function (Approach 1)
- Cleanest implementation
- Non-invasive (no changes to existing functions)
- Backward compatible
- Most maintainable

**Timeline**:
- Week 1: Phase 1 (core infrastructure)
- Week 2: Phase 2 (fixture library)
- Week 3: Phase 3 (test integration)

**Total Effort**: 8-13 hours

**Impact**: Unlocks testing for ~60-70% of game systems

**Next Steps**:
1. Create test_world_generator.h with config struct
2. Implement create_test_world() wrapper
3. Create one test fixture to validate approach
4. Expand fixture library based on test needs

---

**Analysis Complete**: 2025-10-12
**Recommendation**: Proceed with test fixture infrastructure before extensive unit test extraction
