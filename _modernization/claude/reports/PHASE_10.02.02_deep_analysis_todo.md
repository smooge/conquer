# Phase 10.2.2 - Deep Analysis TODO List

**Objective**: Comprehensive 3-part analysis of ALL C/H files to determine re-engineering scope

**Analysis Date**: 2025-10-10
**Phase**: 10.2.2 - Deep Refactoring Assessment
**Strategy**: ONE FILE AT A TIME with session checkpoints

---

## Analysis Requirements (3-Part Framework)

For EACH file, analyze:

1. **Unit Testable Extraction Potential**
   - Which functions can be extracted into pure, testable C files?
   - How many unit tests would be created?
   - What is the extraction effort and ROI?

2. **Architectural Placement Analysis**
   - Does each function belong in its current file location?
   - What functions are misplaced and need relocation?
   - What new modules should be created?

3. **Configuration Coupling Identification**
   - What magic numbers need externalization to game_data.h/game_config.h?
   - What hard-coded constants affect game balance?
   - What configuration dependencies exist?

---

## Checkpoint Strategy

**CRITICAL**: Complete ONE file analysis, then save session memory before moving to next file.

**Process**:
1. Analyze single file using 3-part framework
2. Create comprehensive report: `PHASE_10.02.02_refactor_report_on_<filename>.md`
3. Mark file as completed in this TODO
4. Save session memory: `SESSION_MEMORY_PHASE_10.2.2_<filename>_YYYY-MM-DD_HHMMSS.md`
5. Move to next file

**Why**: New Claude context usage ~2x previous rate, checkpoint frequently to avoid compaction loss

---

## File Analysis Status

### ✅ Completed Files (Full 3-Part Analysis)

- [x] **admin.c** - COMPLETE
  - Standard report: `PHASE_10.02.02_refactor_report_on_admin.c.md` (extraction only)
  - Architectural debt: `PHASE_10.02.02_ARCHITECTURAL_DEBT_admin.c.md` (placement + config)
  - Status: ⭐ VERY LOW extraction priority, ⭐⭐⭐⭐⭐ CRITICAL architectural debt (29% misplaced)
  - Key Finding: att_base() (279 lines) belongs in nation_attributes.c, 50+ magic numbers need externalization

- [x] **m2alloc.c** - COMPLETE (2025-10-10)
  - Full report: `PHASE_10.02.02_refactor_report_on_m2alloc.c.md`
  - Session memory: `SESSION_MEMORY_PHASE_10.2.2_m2alloc_COMPLETE_2025-10-10_153000.md`
  - Status: ⭐⭐⭐⭐⭐ GOLD STANDARD - Model file, no changes needed
  - Key Finding: Perfect extraction (already pure utility), zero architectural debt, zero config coupling

- [x] **cexecute.c** - COMPLETE (2025-10-10)
  - Full report: `PHASE_10.02.02_refactor_report_on_cexecute.c.md`
  - Status: ⭐ VERY LOW extraction priority, ⭐⭐⭐⭐⭐ ZERO architectural debt, ⭐⭐ MODERATE config coupling
  - Key Finding: Core game engine with necessary coupling, 0% misplaced functions, 8 magic numbers to externalize

### ⚠️ Partial Analysis (Extraction Only - Need Architectural + Configuration Analysis)

- [ ] **update.c** - INCOMPLETE (only extraction analysis done)
  - Existing: `PHASE_10.02.02_refactor_report_on_update.c.md`
  - Missing: Architectural placement analysis, configuration coupling analysis
  - Action: Create supplemental architectural debt report

- [ ] **forms.c** - INCOMPLETE (only extraction analysis done)
  - Existing: `PHASE_10.02.02_refactor_report_on_forms.c.md`
  - Missing: Architectural placement analysis, configuration coupling analysis
  - Action: Create supplemental architectural debt report

- [ ] **navy.c** - INCOMPLETE (only extraction analysis done)
  - Existing: `PHASE_10.02.02_refactor_report_on_navy.c.md`
  - Missing: Architectural placement analysis, configuration coupling analysis
  - Action: Create supplemental architectural debt report

- [ ] **combat.c** - INCOMPLETE (only extraction analysis done)
  - Existing: `PHASE_10.02.02_refactor_report_on_combat.c.md`
  - Missing: Architectural placement analysis, configuration coupling analysis
  - Action: Create supplemental architectural debt report

### 📋 Pending Files (User-Specified Analysis Order)

**Priority Order**: User-specified sequence for systematic deep analysis

#### Next Files (Immediate Queue)
3. [x] **m2alloc.c** - COMPLETE (Gold standard)
4. [x] **cexecute.c** - COMPLETE (Core engine, config cleanup only)
5. [ ] **check.c** - Validation and checking
6. [ ] **newhelp.c** - Help system
7. [ ] **sort.c** - Sorting utilities
8. [ ] **extcmds.c** - Extended commands
9. [ ] **header.h** - Main header (architectural debt + config only)
10. [ ] **data.c** - Data structures
11. [ ] **data.h** - Data structures header (architectural debt + config only)
12. [ ] **move.c** (881 lines) - Movement and pathfinding
13. [ ] **psmap.c** - PostScript map generation
14. [ ] **psmap.h** - PostScript map header (architectural debt + config only)
15. [ ] **display.c** - Display rendering
16. [ ] **io.c** - Input/output operations
17. [ ] **magic.c** - Magic system
18. [ ] **makeworl.c** - World generation
19. [ ] **reports.c** - Report generation
20. [ ] **spew.c** - Text output
21. [ ] **main.c** - Program entry point
22. [ ] **trade.c** - Trade system
23. [ ] **trade.h** - Trade system header (architectural debt + config only)
24. [ ] **randeven.c** - Random events and probability
25. [ ] **commands.c** - Command processing
26. [ ] **newlogin.c** - Player login system
27. [ ] **newlogin.h** - Login system header (architectural debt + config only)
28. [ ] **npc.c** - NPC behavior
29. [ ] **misc.c** - Miscellaneous utilities

**Total Remaining**: 27 files (25 after current file)

### 📄 Header Files (Architectural Debt + Configuration Coupling Only)

- [ ] **header.h** - Main header
- [ ] **data.h** - Data structures header
- [ ] **psmap.h** - PostScript map header
- [ ] **trade.h** - Trade system header
- [ ] **newlogin.h** - Login system header
- [ ] **patchlevel.h** - Version information

**Note**: Header files analyzed for architectural debt and configuration coupling only (no extraction analysis)

---

## Execution Plan

### Session 1: Complete Partial Files (4 files)
**Checkpoint after EACH file**

1. **update.c** - Create architectural debt supplement
   - Identify misplaced functions
   - Document magic numbers
   - Checkpoint: Save session memory

2. **forms.c** - Create architectural debt supplement
   - Identify misplaced functions (likely minimal - UI layer)
   - Document magic numbers
   - Checkpoint: Save session memory

3. **navy.c** - Create architectural debt supplement
   - Identify misplaced functions
   - Document magic numbers
   - Checkpoint: Save session memory

4. **combat.c** - Create architectural debt supplement
   - Identify misplaced functions
   - Document magic numbers
   - Checkpoint: Save session memory

### Session 2-N: High Priority Game Engine Files
**Checkpoint after EACH file**

5. **move.c** - Full 3-part analysis
   - Extraction potential
   - Architectural placement
   - Configuration coupling
   - Checkpoint: Save session memory

6. **randeven.c** - Full 3-part analysis
   - Checkpoint: Save session memory

7. Continue with remaining game engine files...
   - Checkpoint after EACH file

### Final Sessions: Summary and Decision

- [ ] **Create comprehensive summary report**
  - Aggregate all findings
  - Total extraction potential across codebase
  - Total architectural debt identified
  - Total configuration coupling issues
  - File: `PHASE_10.02.02_COMPREHENSIVE_SUMMARY.md`

- [ ] **Provide re-engineering decision and roadmap**
  - Decision: Feature branch vs. version increment (4.x → 4.y)
  - Justification based on scope analysis
  - Implementation roadmap
  - File: `PHASE_10.02.02_REENGINEERING_DECISION.md`

---

## Report Naming Convention

**Standard Report** (all 3 parts in one file):
```
PHASE_10.02.02_refactor_report_on_<filename>.md
```

**Architectural Debt Supplement** (when adding to existing extraction-only report):
```
PHASE_10.02.02_ARCHITECTURAL_DEBT_<filename>.md
```

**Session Memory Checkpoints**:
```
SESSION_MEMORY_PHASE_10.2.2_<filename>_YYYY-MM-DD_HHMMSS.md
```

---

## Priority Order Rationale

1. **Complete partial files first** - Finish what we started (update, forms, navy, combat)
2. **Game engine files** - Highest impact on re-engineering decision (move, randeven, magic)
3. **Game logic files** - Core gameplay mechanics (trade, npc, extcmds)
4. **Infrastructure files** - Support systems (io, data, commands)
5. **UI/Display files** - Presentation layer (display, forms, newhelp)
6. **Utilities** - Support code (misc, m2alloc, sort)
7. **Headers** - Configuration and architectural patterns

---

## Success Criteria

✅ **Complete** when:
- All C files have 3-part analysis reports
- All H files have architectural debt + configuration coupling analysis
- Comprehensive summary report created
- Re-engineering decision provided with justification

**Estimated Remaining Work**:
- 4 architectural supplements (1-2 hours)
- 23 full file analyses (23-30 hours at ~1 hour/file with checkpoints)
- 6 header analyses (3-6 hours)
- Summary and decision (2-3 hours)
- **Total**: ~30-40 hours of analysis work

---

**Created**: 2025-10-10
**Last Updated**: 2025-10-10 16:00
**Next Action**: Continue user-specified analysis order starting with check.c (file #5)
