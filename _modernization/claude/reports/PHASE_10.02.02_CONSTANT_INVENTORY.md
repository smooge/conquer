# Phase 10.2.2 - Complete Constant Inventory
## Comprehensive Analysis of All Configuration Constants

**Generated**: 2025-10-10
**Purpose**: Complete inventory of all numeric constants across entire codebase
**Scope**: 28 C files + header.h + data.h
**Goal**: Enable intelligent design of game_balance.h without refactoring churn

---

## Executive Summary

### Constant Distribution

| Source | Count | Status |
|--------|-------|--------|
| **header.h** | 72 | ✅ EXTRACTED |
| **data.h** | 513 | ⏸️ IN PROGRESS |
| **C files (hardcoded)** | TBD | ⏸️ PENDING |
| **TOTAL** | 585+ | ⏸️ IN PROGRESS |

### Critical Findings

1. **72 game balance constants already defined in header.h**
2. **513 constants in data.h** (likely data structure initialization)
3. **223+ hardcoded constants identified in 4 analyzed C files**
4. **Significant overlap expected** between hardcoded and defined constants
5. **Strategic reorganization needed** rather than wholesale creation

---

## PART 1: HEADER.H CONSTANTS (72 Total)

### 1.1 System Limits (7 constants)

| Constant | Value | Category | Purpose |
|----------|-------|----------|---------|
| `NTOTAL` | 35 | System | Max nations (player + NPC + monster) |
| `MAXARM` | 50 | System | Maximum armies per nation |
| `MAXNAVY` | 10 | System | Maximum fleets per nation |
| `REVSPACE` | 5 | System | Revolt slots in nation list |
| `MAXNEWS` | 5 | System | Number of news files stored |
| `MASK` | 037 | System | Data file protection mask (umask) |
| `LASTADD` | 5 | System | Last turn for passwordless join |

**Analysis**: These are **system architecture constants** - should remain in header.h

---

### 1.2 World Generation (6 constants)

| Constant | Value | Category | Purpose |
|----------|-------|----------|---------|
| `TRADEPCT` | 75 | World Gen | % of sectors with exotic trade goods |
| `METALPCT` | 33 | World Gen | % of trade goods that are metals |
| `JEWELPCT` | 33 | World Gen | % of trade goods that are luxury items |
| `MONSTER` | 45 | World Gen | Sectors per pirate/savage/nomad nation |
| `NPC` | 45 | World Gen | Sectors per NPC nation |
| `PMOUNT` | 40.0f | World Gen | % of land that is mountains |

**Analysis**: **Game balance parameters** - candidates for game_balance.h

---

### 1.3 Random Events & Disasters (5 constants)

| Constant | Value | Category | Purpose |
|----------|-------|----------|---------|
| `PVULCAN` | 20 | Random Events | % chance of volcano eruption each round |
| `PSTORM` | 3 | Random Events | % chance storm strikes fleet |
| `RANEVENT` | 15 | Random Events | Enable random events flag |
| `PWEATHER` | 0 | Random Events | % for weather disasters |
| `PREVOLT` | 25 | Random Events | % per turn that revolt occurs |

**Analysis**: **Game balance parameters** - candidates for game_balance.h

---

### 1.4 Vision & Detection (5 constants)

| Constant | Value | Category | Purpose |
|----------|-------|----------|---------|
| `LANDSEE` | 2 | Vision | How far you can see from land |
| `NAVYSEE` | 1 | Vision | How far navies can see |
| `ARMYSEE` | 2 | Vision | How far armies can see |
| `PRTZONE` | 3 | Vision | How far pirates roam from base |
| `MEETNTN` | 2 | Vision | How close nations must be to adjust status |

**Analysis**: **Game balance parameters** - candidates for game_balance.h

---

### 1.5 Economy & Taxation (10 constants)

| Constant | Value | Category | Purpose |
|----------|-------|----------|---------|
| `TAXFOOD` | 5L | Economy | Tax per food unit |
| `TAXMETAL` | 8L | Economy | Tax per metal unit |
| `TAXGOLD` | 8L | Economy | Tax per gold unit |
| `TAXOTHR` | 3L | Economy | Tax per food point equivalent |
| `TAXCITY` | 100L | Economy | Tax per person in city |
| `TAXTOWN` | 80L | Economy | Tax per person in town |
| `MOVECOST` | 20L | Economy | Cost per move/screen |
| `DESCOST` | 2000L | Economy | Cost to redesignate + metal for cities |
| `FORTCOST` | 1000L | Economy | Cost to build fort point |
| `STOCKCOST` | 3000L | Economy | Cost to build stockade |

**Analysis**: **Game balance parameters** - candidates for game_balance.h

---

### 1.6 Population & Resources (6 constants)

| Constant | Value | Category | Purpose |
|----------|-------|----------|---------|
| `TOMANYPEOPLE` | 4000L | Population | Overpopulation threshold |
| `ABSMAXPEOPLE` | 50000L | Population | Absolute max people in any sector |
| `MILLSIZE` | 500L | Population | Min people to work a mill |
| `TOMUCHMINED` | 50000L | Resources | Units mined for 100% depletion |
| `DESFOOD` | 4 | Resources | Min food value to redesignate sector |
| `FINDPERCENT` | 1 | Resources | % chance to find gold/metal |

**Analysis**: **Game balance parameters** - candidates for game_balance.h

---

### 1.7 Naval Operations (6 constants)

| Constant | Value | Category | Purpose |
|----------|-------|----------|---------|
| `SHIPMAINT` | 4000L | Naval | Ship maintenance cost |
| `LONGTRIP` | 100 | Naval | Navy trip length for 100% attrition |
| `WARSHPCOST` | 20000L | Naval | Cost to build one light warship |
| `MERSHPCOST` | 25000L | Naval | Cost to build one light merchant |
| `GALSHPCOST` | 25000L | Naval | Cost to build one light galley |
| `N_CITYCOST` | 4 | Naval | Movement lost in (un)loading in cities |

**Analysis**: **Game balance parameters** - candidates for game_balance.h

---

### 1.8 Ship Characteristics (2 constants)

| Constant | Value | Category | Purpose |
|----------|-------|----------|---------|
| `SHIPCREW` | 100 | Ship Stats | Full strength crew on a ship |
| `SHIPHOLD` | 100L | Ship Stats | Storage space of a ship unit |

**Analysis**: **Game balance parameters** - candidates for game_balance.h

---

### 1.9 NPC Behavior (8 constants)

| Constant | Value | Category | Purpose |
|----------|-------|----------|---------|
| `CITYLIMIT` | 8L | NPC | % of NPC pop in sector before => city |
| `CITYPERCENT` | 20L | NPC | % of NPC pop able to be in cities |
| `MILRATIO` | 8L | NPC | Ratio civ:mil for NPCs |
| `MILINCAP` | 8L | NPC | Ratio (mil in cap):mil for NPCs |
| `MILINCITY` | 10L | NPC | Militia = people/MILINCITY in city/cap |
| `NPCTOOFAR` | 15 | NPC | NPCs shouldn't go this far from capitol |
| `BRIBE` | 50000L | NPC | Gold/1000 men to bribe |
| `METALORE` | 7L | NPC | Metal/soldier for +1% weapons |

**Analysis**: **Game balance parameters** - candidates for game_balance.h

---

### 1.10 Combat Mechanics (7 constants)

| Constant | Value | Category | Purpose |
|----------|-------|----------|---------|
| `MAXPTS` | 65 | Combat | Points for players to buy stuff at start |
| `PDEPLETE` | 30 | Combat | % of armies/sectors depleted without Capitol |
| `PFINDSCOUT` | 50 | Combat | % chance for capturing scouts |
| `TAKESECTOR` | min(500,max(75,tciv/350)) | Combat | Soldiers needed to take sector (formula) |
| `MAXLOSS` | 60 | Combat | Maximum % of men lost in 1:1 battle |
| `REBUILDCOST` | 3000L | Combat | Cost to remove a ruin |
| `DEF_BASE` | 10 | Combat | Base defense value (2x in city/caps) |

**Analysis**: **Game balance parameters** - candidates for game_balance.h

**IMPORTANT**: `MAXLOSS` (60) is ALREADY DEFINED in header.h but used in combat.c formulas

---

### 1.11 Fortification Values (3 constants)

| Constant | Value | Category | Purpose |
|----------|-------|----------|---------|
| `FORTSTR` | 5 | Fortification | % per fortress point in forts |
| `TOWNSTR` | 5 | Fortification | % per fortress point in towns |
| `CITYSTR` | 8 | Fortification | % per fortress point in cities |

**Analysis**: **Game balance parameters** - candidates for game_balance.h

---

### 1.12 Magic System (2 constants)

| Constant | Value | Category | Purpose |
|----------|-------|----------|---------|
| `ORCTAKE` | 100000L | Magic | Jewel cost for orc takeover |
| `TAKEPOINTS` | 10 | Magic | Spell points for orc takeover |

**Analysis**: **Game balance parameters** - candidates for game_balance.h

---

### 1.13 Mercenary System (3 constants)

| Constant | Value | Category | Purpose |
|----------|-------|----------|---------|
| `ST_MMEN` | (NTOTAL*500) | Mercenary | Starting mercenary pool (formula) |
| `ST_MATT` | 40 | Mercenary | Mercenary attack bonus |
| `ST_MDEF` | 40 | Mercenary | Mercenary defense bonus |

**Analysis**: **Game balance parameters** - candidates for game_balance.h

---

### 1.14 Game Configuration (2 constants)

| Constant | Value | Category | Purpose |
|----------|-------|----------|---------|
| `LATESTART` | 2 | Game Config | New player gets 1 point/LATESTART turns |
| `VERSION` | "Version 4" | Game Config | Version number string |

**Analysis**: `VERSION` is metadata, keep in header.h. `LATESTART` is game balance.

---

## PART 2: HEADER.H CONSTANT CATEGORIZATION

### Summary by Category

| Category | Count | Game Balance? | Recommendation |
|----------|-------|---------------|----------------|
| **System Limits** | 7 | NO | Keep in header.h (architecture) |
| **World Generation** | 6 | YES | Move to game_balance.h |
| **Random Events** | 5 | YES | Move to game_balance.h |
| **Vision & Detection** | 5 | YES | Move to game_balance.h |
| **Economy & Taxation** | 10 | YES | Move to game_balance.h |
| **Population** | 6 | YES | Move to game_balance.h |
| **Naval Operations** | 8 | YES | Move to game_balance.h |
| **NPC Behavior** | 8 | YES | Move to game_balance.h |
| **Combat Mechanics** | 7 | YES | Move to game_balance.h |
| **Fortification** | 3 | YES | Move to game_balance.h |
| **Magic System** | 2 | YES | Move to game_balance.h |
| **Mercenary System** | 3 | YES | Move to game_balance.h |
| **Game Configuration** | 2 | MIXED | VERSION stays, LATESTART moves |

**Recommendation**: **64 of 72 constants should move to game_balance.h**

**Keep in header.h (8 constants)**:
- NTOTAL, MAXARM, MAXNAVY, REVSPACE, MAXNEWS, MASK, LASTADD, VERSION

---

## PART 3: DATA.H ANALYSIS (513 Constants)

**Status**: ⏸️ PENDING EXTRACTION

**Expected Content**:
- Unit type definitions and arrays
- Terrain type definitions
- Designation type definitions
- Magic power definitions
- Command code definitions
- Data structure initialization values

**Next Step**: Extract and categorize all 513 constants from data.h

---

## PART 4: HARDCODED CONSTANTS IN C FILES

### 4.1 Files Already Analyzed (4 files)

| File | Hardcoded Constants | Status |
|------|---------------------|--------|
| **combat.c** | **86+** | ✅ COMPLETE INVENTORY |
| **update.c** | **61+** | ✅ COMPLETE INVENTORY |
| **navy.c** | **4** | ✅ COMPLETE INVENTORY |
| **forms.c** | **22** | ✅ COMPLETE INVENTORY |
| **Subtotal** | **173+** | - |

### 4.2 Files Pending Analysis (24 files)

| File | Lines | Priority | Expected Constants |
|------|-------|----------|-------------------|
| misc.c | 4765 | HIGH | 50-100 (utilities) |
| npc.c | 3467 | HIGH | 30-60 (NPC AI) |
| commands.c | 2526 | MEDIUM | 20-40 (command parsing) |
| trade.c | 1874 | HIGH | 30-50 (trade mechanics) |
| magic.c | 1710 | HIGH | 40-80 (magic formulas) |
| makeworl.c | 1711 | MEDIUM | 30-60 (world gen) |
| reports.c | 1508 | LOW | 10-20 (reporting) |
| move.c | 881 | MEDIUM | 15-30 (movement) |
| **Others (16 files)** | <1000 ea | LOW | 5-15 each |

**Estimated Total Hardcoded**: **400-700 constants across all C files**

---

## PART 5: CROSS-REFERENCE ANALYSIS

### 5.1 Known Overlaps (Examples from combat.c)

| Hardcoded in C | Already in header.h | Status |
|----------------|---------------------|--------|
| `60` (MAXLOSS in combat.c) | `MAXLOSS 60` | ✅ DEFINED |
| Various combat formulas | Not defined | ❌ MISSING |
| Naval capture percentages | Not defined | ❌ MISSING |
| Retreat probabilities | Not defined | ❌ MISSING |

**Critical Discovery**: Many hardcoded constants in combat.c are NOT in header.h

### 5.2 Expected Patterns

**Pattern 1: Partial Definition**
- Some constants defined in header.h
- Related constants hardcoded in C files
- Example: MAXLOSS defined, but loss calculation divisors (12, 16, 8) hardcoded

**Pattern 2: Complete Missing Systems**
- Entire formula systems not externalized
- Example: Naval capture percentage formula (20+ constants) not in header.h

**Pattern 3: Data Structure Constants**
- Likely in data.h (513 constants)
- Need to verify which are balance vs structure

---

## PART 6: NEXT STEPS (SYSTEMATIC APPROACH)

### Step 1: Complete data.h Extraction ⏸️ IN PROGRESS
- Extract all 513 constants
- Categorize by type (data structure vs game balance)
- Identify overlap with header.h

### Step 2: Scan All C Files for Hardcoded Constants ⏸️ PENDING
- Automated scan using grep/regex
- Manual review for context
- Categorize by system

### Step 3: Cross-Reference Analysis ⏸️ PENDING
- Match hardcoded constants to existing defines
- Identify missing definitions
- Find duplicate/conflicting values

### Step 4: Create Master Inventory ⏸️ PENDING
- Complete list of all constants
- Source file tracking
- Usage frequency analysis
- Conflict resolution

### Step 5: Design game_balance.h Structure ⏸️ PENDING
- Organize by game system
- Clear naming conventions
- Documentation for each constant
- Migration plan from header.h

---

## PART 7: PRELIMINARY RECOMMENDATIONS

### 7.1 File Organization Strategy

**Keep in header.h** (System Architecture):
```c
/* System limits and architecture */
#define NTOTAL 35
#define MAXARM 50
#define MAXNAVY 10
#define REVSPACE 5
#define MAXNEWS 5
#define MASK 037
#define LASTADD 5
#define VERSION "Version 4"
```

**Move to game_balance.h** (Game Balance - 64 constants):
```c
/* ========== WORLD GENERATION ========== */
#define TRADEPCT 75
#define METALPCT 33
#define JEWELPCT 33
// ... (6 total)

/* ========== COMBAT MECHANICS ========== */
#define MAXLOSS 60
#define DEF_BASE 10
#define FORTSTR 5
// ... (10 total)

/* ========== ECONOMY & TAXATION ========== */
#define TAXFOOD 5L
#define TAXMETAL 8L
// ... (10 total)

/* ... (8 more categories) ... */
```

**Keep in data.h** (Data Structure Initialization):
- Unit type arrays
- Terrain definitions
- Command codes
- Enumeration values

---

## PART 8: TIMELINE ESTIMATE

| Task | Effort | Status |
|------|--------|--------|
| Extract data.h constants | 1-2 hours | PENDING |
| Scan 24 C files | 2-3 hours | PENDING |
| Cross-reference analysis | 1-2 hours | PENDING |
| Master inventory creation | 1-2 hours | PENDING |
| game_balance.h design | 2-3 hours | PENDING |
| **TOTAL** | **7-12 hours** | **20% COMPLETE** |

---

**Document Status**: ✅ Part 1 Complete (header.h analysis)
**Next Action**: Extract and analyze data.h constants
**Saved**: 2025-10-10

