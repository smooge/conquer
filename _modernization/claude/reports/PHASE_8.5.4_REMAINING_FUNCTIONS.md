# Phase 8.5.4 - Remaining Functions Documentation Status

**Generated**: 2025-10-08
**Status**: After running `add_doc_dates.py` on all *.c files

---

## Overall Progress

**Total Functions**: 251
**Documented**: 202 (80.5%)
**Missing**: 49 (19.5%)

---

## Complete Missing Functions List (49 functions)

### 📝 HIGH PRIORITY - Near Complete Files (9 functions)

#### misc.c - 4 functions (88.6% → 100%)
- `prep()` - Line 1548
- `flightcost()` - Line 2379
- `tg_ok()` - Line 3990
- `fort_val()` - Line 4090

#### main.c - 1 function (90.9% → 100%)
- `sect_info()` - Line 42

#### randeven.c - 2 functions (80.0% → 100%)
- `reduce()` - Line 1860
- `weather()` - Line 1905

#### newlogin.c - 3 functions (82.4% → 100%)
- `nstartcst()` - Line 172
- `newerror()` - Line 612
- `getclass()` - Line 2174

---

### 🔶 MEDIUM PRIORITY - Partially Complete Files (20 functions)

#### npc.c - 5 functions (76.2% → 100%)
- `n_redes()` - Line 735
- `n_defend()` - Line 2289
- `n_survive()` - Line 2838
- `atkattr()` - Line 3068
- `pceattr()` - Line 3168

#### makeworl.c - 2 functions (66.7% → 100%)
- `fill_edge()` - Line 1051
- `populate()` - Line 1166

#### forms.c - 2 functions (66.7% → 100%)
- `change()` - Line 646
- `help()` - Line 1076

#### combat.c - 3 functions (62.5% → 100%)
- `navalcbt()` - Line 1302
- `capture()` - Line 1840
- `show_ships()` - Line 1941

#### psmap.c - 8 functions (52.9% → 100%) ⚠️ Most missing in this category
- `parsepagesize()` - Line 89
- `setpagesize()` - Line 90
- `get_pagesize()` - Line 91
- `psstring()` - Line 92
- `isinstr()` - Line 93
- `getmaptype()` - Line 94
- `readmap()` - Line 95
- `buildps()` - Line 96

---

### 🔴 LOW PRIORITY - Incomplete Files (20 functions)

#### update.c - 7 functions (50.0% → 100%)
- `do_lizard()` - Line 1128
- `updcapture()` - Line 1267
- `updsectors()` - Line 1471
- `updmil()` - Line 1768
- `updcomodities()` - Line 2158
- `updleader()` - Line 2356
- `move_people()` - Line 2552

#### commands.c - 6 functions (33.3% → 100%)
- `desg_ok()` - Line 116
- `redesignate()` - Line 285
- `construct()` - Line 637
- `draft()` - Line 1102
- `rmessage()` - Line 1511
- `moveciv()` - Line 2021

#### safe_system.c - 4 functions (0.0% → 100%) ⚠️ No documentation yet
- `append_file_to_file()` - Line 63
- `write_timestamp_to_file()` - Line 139
- `secure_file_delete()` - Line 199
- `sort_file_in_place()` - Line 446

#### move.c - 1 function (50.0% → 100%)
- `getselunit()` - Line 780

#### spew.c - 1 function (50.0% → 100%)
- `makemess()` - Line 1659

---

## Files Already Complete (12 files, 89 functions) ✅

- admin.c (4/4)
- cexecute.c (2/2)
- check.c (5/5)
- display.c (10/10)
- extcmds.c (9/9)
- io.c (15/15)
- magic.c (10/10)
- navy.c (13/13)
- newhelp.c (1/1)
- reports.c (4/4)
- sort.c (1/1)
- trade.c (15/15)

---

## Recommended Completion Strategy

### Phase 1: Finish High Priority (9 functions) → 84.1% coverage
**Estimated Time**: 2-3 hours

1. **misc.c** - 4 functions (current focus)
2. **main.c** - 1 function
3. **randeven.c** - 2 functions
4. **newlogin.c** - 3 functions

**Impact**: Quick wins, bring nearly-complete files to 100%

---

### Phase 2: Small Medium Priority Files (7 functions) → 86.9% coverage
**Estimated Time**: 1.5-2 hours

5. **makeworl.c** - 2 functions
6. **forms.c** - 2 functions
7. **combat.c** - 3 functions

**Impact**: Complete 3 more files, maintain momentum

---

### Phase 3: Large Medium Priority File (13 functions) → 90.0% coverage ⭐
**Estimated Time**: 2.5-3 hours

8. **npc.c** - 5 functions
9. **psmap.c** - 8 functions

**Impact**: Reach 90% documentation milestone

---

### Phase 4: Low Priority Medium Files (14 functions) → 95.6% coverage
**Estimated Time**: 3-4 hours

10. **commands.c** - 6 functions
11. **update.c** - 7 functions
12. **move.c** - 1 function

**Impact**: Complete core game logic files

---

### Phase 5: Final Cleanup (6 functions) → 100% coverage 🎉
**Estimated Time**: 1-1.5 hours

13. **safe_system.c** - 4 functions
14. **spew.c** - 1 function

**Impact**: Achieve 100% documentation coverage

---

## Total Estimated Time to 100%

- **Phase 1-2**: 3.5-5 hours (reach 87%)
- **Phase 3**: 2.5-3 hours (reach 90% milestone)
- **Phase 4-5**: 4-5.5 hours (reach 100%)
- **Grand Total**: 10-13.5 hours to complete all documentation

---

## Current Session Focus

**File**: misc.c
**Remaining**: 4 functions
**Target**: 35/35 (100%)

**Next Functions**:
1. `prep()` - Line 1548
2. `flightcost()` - Line 2379
3. `tg_ok()` - Line 3990
4. `fort_val()` - Line 4090

**After misc.c**: Move to main.c (1 function) for quick completion

---

## Notes

- All files have been processed with `add_doc_dates.py` for accurate tracking
- Documentation uses "Testing Notes:" as definitive marker for comprehensive docs
- All documented functions include `@last_documented: YYYY-MM-DD` field
- Documentation-only changes (no code modifications in Phase 8.5.4)
