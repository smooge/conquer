# Refactoring Plan: Turn-Based State Consolidation

**Created**: 2025-10-12
**Phase**: Future Refactoring (Phase 10.3+)
**Priority**: ⭐⭐⭐⭐ HIGH - Improves architectural clarity and state management
**Estimated Effort**: 4-6 hours
**Risk Level**: MODERATE - Requires careful analysis of reset points

---

## Executive Summary

**Problem**: Turn-based state counters are scattered across global variables with inconsistent reset mechanisms and unclear ownership. This creates architectural debt and makes it difficult to understand turn lifecycle and state management.

**Solution**: Consolidate all per-turn state into structured locations with clear ownership and lifecycle management.

**Impact**: Improves code clarity, makes turn state management explicit, provides foundation for future features (save/load, multiplayer, difficulty variants).

---

## Turn-Based Counter Inventory

### Currently Identified Turn-Based Counters

**1. roads_this_turn** (commands.c:38)
```c
// CURRENT
int roads_this_turn = 0;  // Global scope

// LOCATION: commands.c:38
// USED IN: redesignate() - lines 533-537
// RESET IN: main.c:1230 (when nation switches)
// PURPOSE: Enforce 2 roads per turn limit per nation
// SCOPE: Per-nation, per-turn
```

**2. mercgot** (main.c:50)
```c
// CURRENT
long mercgot = 0;  // Global scope

// LOCATION: main.c:50
// USED IN:
//   - commands.c:1372 (check mercenary recruitment limit)
//   - commands.c:1529 (increment when mercenaries drafted)
//   - cexecute.c:219 (add mercenaries during execution)
// RESET IN: main.c:50 (initialization only, never explicitly reset)
// PURPOSE: Track total mercenaries recruited across all nations
// SCOPE: Global, per-turn (but reset mechanism unclear)
```

**3. terror_adj** (forms.c:678)
```c
// CURRENT
int terror_adj = 0;  // Global scope

// LOCATION: forms.c:678
// USED IN:
//   - forms.c:1058, 1061 (limit terror changes)
//   - cexecute.c:265 (increment during execution)
// RESET IN: main.c:1231 (when nation switches)
// PURPOSE: Limit terror adjustments to once per turn
// SCOPE: Per-nation, per-turn
```

**4. startgold** (main.c:49)
```c
// CURRENT
long startgold = 0;  // Global scope

// LOCATION: main.c:49
// USED IN:
//   - cexecute.c:125 (set at start of non-update execution)
//   - reports.c:628, 654, 677, 679, 687, 695 (expense calculations)
// RESET IN: cexecute.c:125 (set at execution start)
// PURPOSE: Track starting treasury for expense reporting
// SCOPE: Per-nation, per-turn
```

**5. i_people** (sector structure - data.h:377)
```c
// CURRENT
struct s_sector {
    // ...
    short i_people;  /* initial civilians in sector */
};

// LOCATION: data.h:377 (struct member)
// SET IN: cexecute.c:134, 136 (at turn start)
// USED IN: commands.c:1344-1346 (draft limit calculation)
// RESET IN: cexecute.c:127-136 (turn initialization)
// PURPOSE: Track initial population for draft limits
// SCOPE: Per-sector, per-turn (baseline value)
// FORMULA: i_people * 192/256 = max 75% initial pop draftable
```

---

## Analysis Summary

### Counter Categories

**Category A: Per-Nation Turn Counters** (4 counters)
- roads_this_turn
- terror_adj
- startgold
- mercgot (partially - unclear if per-nation or global)

**Category B: Per-Sector Turn Initialization** (1 counter)
- i_people (already in sector structure - well-placed)

**Category C: Global Turn Counters** (unclear)
- mercgot (needs analysis - may be global across all nations)

### Current Problems

**Problem 1: Inconsistent Scope**
- Some counters are global but should be per-nation (roads_this_turn, terror_adj)
- Some counters have unclear scope (mercgot - global or per-nation?)
- Difficult to understand which state belongs to which nation

**Problem 2: Inconsistent Reset Mechanisms**
- roads_this_turn: Reset in main.c when switching nations (line 1230)
- terror_adj: Reset in main.c when switching nations (line 1231)
- startgold: Reset in cexecute.c at execution start (line 125)
- mercgot: Initialized in main.c (line 50), incremented but never explicitly reset
- i_people: Reset in cexecute.c at turn start (lines 127-136)

**Problem 3: Hidden Dependencies**
- Turn state scattered across multiple files
- Reset logic not co-located with state definition
- Difficult to understand complete turn lifecycle

**Problem 4: No Centralized Turn State Management**
- No single location to see all per-turn state
- No clear "begin turn" / "end turn" functions
- Reset logic embedded in various functions

---

## Proposed Solution

### Option 1: Add Fields to Nation Structure (RECOMMENDED)

**Rationale**: Per-nation turn state belongs in the nation structure for proper ownership.

**Implementation**:

```c
/* In data.h - struct s_nation */
struct s_nation {
    /* ... existing fields ... */

    /* PER-TURN STATE COUNTERS */
    short roads_built_this_turn;    /* Roads constructed this turn (max: 2) */
    short terror_adjustments;        /* Terror changes this turn (limit: once) */
    long starting_gold;              /* Treasury at turn start (for expense tracking) */
    long mercenaries_recruited;      /* Mercenaries drafted this turn */

    /* ... rest of structure ... */
};
```

**Benefits**:
- ✅ Clear ownership (state belongs to nation)
- ✅ Persistence (survives save/load if needed)
- ✅ Per-nation tracking (correct semantics)
- ✅ Easy to reset (iterate nations, reset fields)
- ✅ Extensible (easy to add more per-turn counters)

**Reset Point**:
```c
/* New function in update.c or cexecute.c */
void reset_nation_turn_state(int nation_id) {
    ntn[nation_id].roads_built_this_turn = 0;
    ntn[nation_id].terror_adjustments = 0;
    ntn[nation_id].starting_gold = ntn[nation_id].tgold;
    ntn[nation_id].mercenaries_recruited = 0;
}

/* Call at turn start for each nation */
for (i = 1; i < NTOTAL; i++) {
    if (isactive(ntn[i].active)) {
        reset_nation_turn_state(i);
    }
}
```

**mercgot Analysis**:

Need to determine if mercgot is:
- **Per-nation**: Track how many mercenaries each nation recruited this turn
- **Global**: Track total mercenaries recruited across all nations this turn

**Evidence suggests per-nation**:
- Used in commands.c:1372 to check if nation exceeds limit: `mercgot + men > MERCMEN / NTOTAL`
- Formula: MERCMEN / NTOTAL suggests per-nation quota
- Incremented when specific nation drafts mercenaries

**Recommendation**: Make mercgot per-nation (add to nation structure)

---

### Option 2: Create Turn State Structure

**Rationale**: Group all turn state in one place for clarity.

**Implementation**:

```c
/* New header: turn_state.h */

struct s_turn_state {
    /* Per-turn construction limits */
    short roads_built;              /* Roads constructed (limit: 2) */
    short ships_built;              /* Ships constructed (track if needed) */

    /* Per-turn adjustment limits */
    short terror_adjustments;       /* Terror changes (limit: once) */
    short popularity_adjustments;   /* Popularity changes (track if needed) */

    /* Per-turn recruitment tracking */
    long mercenaries_recruited;     /* Mercenaries drafted */
    long civilians_drafted;         /* Total civilians recruited */

    /* Per-turn financial tracking */
    long starting_gold;             /* Treasury at turn start */
    long starting_metals;           /* Metals at turn start */
    long starting_jewels;           /* Jewels at turn start */
};

/* Add to struct s_nation */
struct s_nation {
    /* ... existing fields ... */
    struct s_turn_state turn_state;  /* Per-turn state counters */
    /* ... rest of structure ... */
};
```

**Benefits**:
- ✅ Explicit grouping of all turn state
- ✅ Clear what gets reset each turn
- ✅ Extensible for future turn-based features
- ✅ Self-documenting code structure

**Drawbacks**:
- ⚠️ More complex refactoring
- ⚠️ Requires updating all access points
- ⚠️ May be over-engineering for current needs

---

## Recommended Implementation Plan

### Phase 1: Analysis and Validation (1 hour)

**Step 1.1: Confirm mercgot Scope**
- [ ] Search all uses of mercgot in codebase
- [ ] Determine if global or per-nation counter
- [ ] Document reset behavior (if any)
- [ ] Clarify MERCMEN / NTOTAL formula semantics

**Step 1.2: Identify Additional Turn Counters**
- [ ] Search for other variables reset at turn start
- [ ] Search for other per-turn limits or quotas
- [ ] Check update.c for turn processing logic
- [ ] Document all turn initialization code

**Step 1.3: Map Reset Points**
- [ ] Document where each counter is currently reset
- [ ] Identify proper turn start/end boundaries
- [ ] Find all places where nations switch (god mode, login)
- [ ] Determine correct reset semantics

---

### Phase 2: Implement Option 1 (2-3 hours)

**Step 2.1: Add Fields to Nation Structure**
```c
/* In data.h - struct s_nation */

/* Add after existing nation fields, before end of struct */

/* PER-TURN STATE COUNTERS */
short roads_built_this_turn;    /* Roads constructed this turn (max: 2) */
short terror_adjustments;        /* Terror changes this turn (limit: once) */
long starting_gold;              /* Treasury at turn start (for expense tracking) */
long mercenaries_recruited;      /* Mercenaries drafted this turn */
```

**Step 2.2: Update All Access Points**

**roads_this_turn migration**:
- [ ] commands.c:533-537 → `curntn->roads_built_this_turn`
- [ ] Remove global declaration (commands.c:38)
- [ ] Remove reset in main.c:1230

**terror_adj migration**:
- [ ] forms.c:1058, 1061 → `curntn->terror_adjustments`
- [ ] cexecute.c:265 → `curntn->terror_adjustments`
- [ ] Remove global declaration (forms.c:678)
- [ ] Remove reset in main.c:1231

**startgold migration**:
- [ ] reports.c (all locations) → `curntn->starting_gold`
- [ ] cexecute.c:125 → `curntn->starting_gold = curntn->tgold;`
- [ ] Remove global declaration (main.c:49)

**mercgot migration** (if per-nation):
- [ ] commands.c:1372, 1529 → `curntn->mercenaries_recruited`
- [ ] cexecute.c:219 → `curntn->mercenaries_recruited`
- [ ] Remove global declaration (main.c:50)

**Step 2.3: Create Turn State Reset Function**
```c
/* In cexecute.c or new turn_state.c */

/*
 * reset_nation_turn_state - Reset per-turn state counters for a nation
 *
 * Initializes all per-turn counters to their starting values at the
 * beginning of a nation's turn. Called during turn processing and
 * when switching nations in god mode.
 *
 * Parameters:
 *   nation_id - Nation to reset turn state for
 */
void reset_nation_turn_state(int nation_id) {
    if (nation_id < 0 || nation_id >= NTOTAL) return;

    struct s_nation *ntn_ptr = &ntn[nation_id];

    /* Reset construction limits */
    ntn_ptr->roads_built_this_turn = 0;

    /* Reset adjustment limits */
    ntn_ptr->terror_adjustments = 0;

    /* Reset recruitment tracking */
    ntn_ptr->mercenaries_recruited = 0;

    /* Set financial baselines */
    ntn_ptr->starting_gold = ntn_ptr->tgold;
}
```

**Step 2.4: Add Reset Calls**

**Location 1: Nation Switch (main.c:1230)**
```c
/* Replace current reset code */
// OLD:
roads_this_turn = 0;
terror_adj = 0;

// NEW:
reset_nation_turn_state(country);
```

**Location 2: Turn Start (cexecute.c or update.c)**
```c
/* Add to execute() or turn processing */
if (isupdate == 0) {
    reset_nation_turn_state(country);
}
```

**Location 3: Update Processing (update.c)**
```c
/* In update loop, reset all nations */
for (i = 1; i < NTOTAL; i++) {
    if (isactive(ntn[i].active)) {
        reset_nation_turn_state(i);
    }
}
```

---

### Phase 3: Testing and Validation (1 hour)

**Step 3.1: Unit Tests**
- [ ] Test reset_nation_turn_state() function
- [ ] Verify all counters reset properly
- [ ] Test boundary conditions (invalid nation IDs)

**Step 3.2: Integration Tests**
- [ ] Test road building limit (2 per turn)
- [ ] Test terror adjustment limit
- [ ] Test mercenary recruitment tracking
- [ ] Test expense reporting (starting gold)
- [ ] Test nation switching (god mode)
- [ ] Test turn processing (update)

**Step 3.3: Regression Tests**
- [ ] Verify existing game behavior unchanged
- [ ] Test save/load compatibility (if applicable)
- [ ] Test multiplayer scenarios (if applicable)

---

### Phase 4: Documentation (30 minutes)

**Step 4.1: Code Documentation**
- [ ] Document reset_nation_turn_state() function
- [ ] Add comments to new struct fields
- [ ] Update architectural documentation

**Step 4.2: Design Documentation**
- [ ] Document turn state lifecycle
- [ ] Document reset points and timing
- [ ] Update system architecture diagrams

---

## Implementation Checklist

### Pre-Implementation
- [x] Identify all turn-based counters
- [ ] Determine counter scope (per-nation vs global)
- [ ] Map all reset points
- [ ] Identify all access points
- [ ] Create implementation plan

### Implementation
- [ ] Add fields to nation structure
- [ ] Create reset function
- [ ] Update roads_this_turn access points
- [ ] Update terror_adj access points
- [ ] Update startgold access points
- [ ] Update mercgot access points (if per-nation)
- [ ] Remove global variable declarations
- [ ] Add reset calls at appropriate points

### Testing
- [ ] Write unit tests for reset function
- [ ] Write integration tests for turn state
- [ ] Run regression tests
- [ ] Verify game behavior unchanged

### Documentation
- [ ] Document new struct fields
- [ ] Document reset function
- [ ] Update architecture documentation
- [ ] Create session memory checkpoint

---

## Migration Notes

### Backward Compatibility

**Save Files**: If save files exist, need to handle:
- [ ] Add new fields to save file format (if applicable)
- [ ] Provide default values for old save files
- [ ] Test load/save compatibility

**Network Protocol**: If multiplayer exists, need to:
- [ ] Update network data structures
- [ ] Handle protocol version compatibility
- [ ] Test client/server communication

### Rollback Plan

If implementation causes issues:
1. Keep old global variables temporarily
2. Copy values between global and struct fields
3. Verify behavior identical
4. Remove global variables after validation period

---

## Future Enhancements

### Additional Turn State Candidates

**Construction Tracking**:
- Ships built this turn (for limits if needed)
- Fortifications built this turn (for limits if needed)
- Designations changed this turn (for limits if needed)

**Financial Tracking**:
- Gold spent this turn (for budgeting)
- Gold earned this turn (for reporting)
- Trade volume this turn (for economics)

**Military Tracking**:
- Soldiers drafted this turn (for limits)
- Battles fought this turn (for fatigue)
- Casualties this turn (for reporting)

### Turn State Analytics

Once consolidated, could add:
- Turn history tracking (last N turns)
- Trend analysis (spending patterns)
- Automated warnings (over-recruiting)
- Performance metrics (efficiency tracking)

---

## Benefits

### Immediate Benefits

1. **Architectural Clarity**
   - Clear ownership of turn state
   - Explicit lifecycle management
   - Self-documenting code structure

2. **Code Maintainability**
   - Centralized reset logic
   - Easy to add new turn counters
   - Reduced global variables

3. **Reduced Bugs**
   - Clear initialization points
   - No forgotten resets
   - Proper scope management

### Long-Term Benefits

1. **Feature Enablement**
   - Save/load turn state correctly
   - Multiplayer turn synchronization
   - Turn history and replay

2. **Game Balance**
   - Easy to adjust turn limits
   - Transparent limit enforcement
   - Configurable difficulty variants

3. **Testing**
   - Easier to test turn logic
   - Clear state boundaries
   - Reproducible turn scenarios

---

## Risk Assessment

### Risks

**MODERATE Risk: State Reset Timing**
- **Risk**: Reset at wrong point in turn cycle
- **Mitigation**: Careful analysis of current reset points
- **Mitigation**: Comprehensive testing of turn processing
- **Mitigation**: Gradual rollout with backward compatibility

**LOW Risk: Access Point Migration**
- **Risk**: Missing some access points during migration
- **Mitigation**: Comprehensive grep for all uses
- **Mitigation**: Compiler warnings for undefined globals
- **Mitigation**: Integration tests catch missed migrations

**LOW Risk: Scope Misunderstanding**
- **Risk**: mercgot may be global, not per-nation
- **Mitigation**: Thorough analysis of usage patterns
- **Mitigation**: Testing with multiple nations
- **Mitigation**: Consultation of game mechanics documentation

### Success Criteria

✅ **Complete** when:
- All per-turn state moved to nation structure
- All global turn counters removed
- Reset function implemented and tested
- All access points updated
- Integration tests passing
- Documentation complete

---

## Estimated Timeline

**Phase 1 (Analysis)**: 1 hour
**Phase 2 (Implementation)**: 2-3 hours
**Phase 3 (Testing)**: 1 hour
**Phase 4 (Documentation)**: 30 minutes

**Total**: 4-6 hours

**Recommended Schedule**:
- Session 1 (2 hours): Analysis + Begin Implementation
- Session 2 (2 hours): Complete Implementation + Testing
- Session 3 (1 hour): Final Testing + Documentation

---

## References

**Related Files**:
- commands.c (roads_this_turn usage)
- main.c (reset points, global declarations)
- forms.c (terror_adj usage)
- cexecute.c (startgold, i_people initialization)
- reports.c (startgold expense calculations)
- data.h (nation structure definition)

**Related Patterns**:
- Turn state management
- Per-entity state ownership
- Lifecycle management
- Global variable elimination

**Related Issues**:
- Architectural debt reduction
- State management clarity
- Testing infrastructure improvement

---

**Created**: 2025-10-12
**Last Updated**: 2025-10-12
**Status**: PLANNED (Not Yet Implemented)
**Phase**: Future Refactoring (Phase 10.3+)
