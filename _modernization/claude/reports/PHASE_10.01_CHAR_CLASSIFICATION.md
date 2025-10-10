# Char Declaration Classification Spreadsheet

**Phase**: 10.1 - Char Type Audit and Declaration Refactoring
**Date**: 2025-10-09
**Purpose**: Classify all plain `char` declarations by semantic intent and risk level
**GitHub Issue**: #10 (Char Signedness Portability)

---

## Classification Categories

- **Category A**: Text/strings → **keep as `char`** (no change needed)
  - String buffers, text storage, null-terminated strings
  - Example: `char filename[256]`, `char *message`

- **Category B**: Small unsigned integers (0-255) → **change to `unsigned char`**
  - Counts, indices, byte values, small positive integers
  - Example: `unsigned char count`, `unsigned char buffer[100]`

- **Category C**: Small signed integers (-128 to 127) → **change to `signed char`**
  - Deltas, differences, signed values
  - Example: `signed char temperature_delta`

- **Category D**: ctype.h parameters → **MUST be `unsigned char`** 🔴 CRITICAL
  - C standard requirement: ctype.h functions require unsigned char or EOF
  - Risk: Negative char → undefined behavior
  - Example: `unsigned char ch = *str; if (isspace(ch)) { ... }`

- **Category E**: Array indices → **MUST be `unsigned char` or `size_t`** 🔴 CRITICAL
  - Risk: Negative array index → undefined behavior / security vulnerability
  - Example: `unsigned char index; array[index] = value;`

---

## Risk Levels

- **🔴 HIGH RISK (CRITICAL)**: Fix immediately (Phase 10.1)
  - Array indexing with plain char (Category E)
  - ctype.h usage with plain char (Category D)
  - Security vulnerabilities, undefined behavior

- **🟡 MEDIUM RISK**: Fix this phase (Phase 10.1)
  - Calculations that may go negative
  - User input assignments
  - Type conversions between char/unsigned char
  - Portability issues

- **🟢 LOW RISK**: Document, defer to future phase (Phase 10.6+)
  - Text strings (Category A) - safe as-is
  - Internal implementation details with no sign dependency
  - Usage verified safe on both platforms

---

## HIGH RISK Files (Fix Immediately) 🔴

### 1. misc.c - Array Indexing (lines 2407-2440)

| Line | Current Declaration | Category | New Declaration | Usage | Risk Reason | Action |
|------|-------------------|----------|-----------------|-------|-------------|--------|
| 2407 | `veg_cost[(unsigned char)veg[j]]` | E | Fix `veg[]` to `unsigned char veg[]` | Array index | Negative index → UB | ✅ Fix declaration, remove cast |
| 2410 | `veg_cost[(unsigned char)veg[j]]` | E | Same | Array index | Same | ✅ Fix declaration, remove cast |
| 2413 | `veg_cost[(unsigned char)veg[j]]` | E | Same | Array index | Same | ✅ Fix declaration, remove cast |
| 2417 | `veg_cost[(unsigned char)veg[j]]` | E | Same | Array index | Same | ✅ Fix declaration, remove cast |
| 2430 | `ele_cost[(unsigned char)ele[j]]` | E | Fix `ele[]` to `unsigned char ele[]` | Array index | Negative index → UB | ✅ Fix declaration, remove cast |
| 2433 | `ele_cost[(unsigned char)ele[j]]` | E | Same | Array index | Same | ✅ Fix declaration, remove cast |
| 2436 | `ele_cost[(unsigned char)ele[j]]` | E | Same | Array index | Same | ✅ Fix declaration, remove cast |
| 2440 | `ele_cost[(unsigned char)ele[j]]` | E | Same | Array index | Same | ✅ Fix declaration, remove cast |

**Root Cause Analysis**:
- `veg[]` and `ele[]` arrays are declared as plain `char` in data.h
- Used as array indices in veg_cost[] and ele_cost[] arrays
- On x86_64, if char is negative, array index is negative → undefined behavior
- **Solution**: Change declarations to `unsigned char veg[]`, `unsigned char ele[]`
- **Impact**: Remove 8 `(unsigned char)` casts from misc.c

**Estimated Casts Removed**: 8
**Estimated Time**: 45-60 minutes

---

### 2. spew.c - ctype.h Usage (line 1244)

| Line | Current Code | Category | New Code | Usage | Risk Reason | Action |
|------|--------------|----------|----------|-------|-------------|--------|
| 1244 | `isspace((unsigned char)*end)` | D | Fix `end` to `unsigned char *end` OR use temp variable | ctype.h param | Negative char → UB per C standard | ✅ Fix declaration or add temp variable |

**Root Cause Analysis**:
- `end` is declared as `char *end`
- Used with `isspace()` which requires `unsigned char` or EOF
- C standard: Passing negative char to ctype.h is undefined behavior
- **Solution Option A**: Change `end` to `unsigned char *end` if only used for ctype.h
- **Solution Option B**: Create temp `unsigned char ch = *end; isspace(ch);`
- **Impact**: Remove 1 `(unsigned char)` cast

**Estimated Casts Removed**: 1
**Estimated Time**: 30 minutes

---

### 3. update.c - Poverty Calculations (lines 1636-1649)

| Line | Current Code | Category | New Declaration | Usage | Risk Reason | Action |
|------|--------------|----------|-----------------|-------|-------------|--------|
| 1636 | `poverty = (unsigned char)20` | B | Verify `poverty` is `unsigned char` | Calculation result | May need bounds check | ✅ Verify declaration |
| 1638 | `poverty = (unsigned char)(95L - tgold/tciv)` | B | Add bounds checking | Calculation (can be negative) | Negative result → wrong unsigned value | ✅ Add explicit bounds check |
| 1641 | `poverty = (unsigned char)(65L - ...)` | B | Add bounds checking | Calculation (can be negative) | Same | ✅ Add explicit bounds check |
| 1644 | `poverty = (unsigned char)(40L - ...)` | B | Add bounds checking | Calculation (can be negative) | Same | ✅ Add explicit bounds check |
| 1647 | `poverty = (unsigned char)(30L - ...)` | B | Add bounds checking | Calculation (can be negative) | Same | ✅ Add explicit bounds check |
| 1649 | `poverty = (unsigned char)20` | B | Same as 1636 | Constant assignment | Safe | ✅ Verify declaration |

**Root Cause Analysis**:
- Poverty calculations: `95L - gold/civ` can go negative
- Casting negative long to unsigned char wraps to large positive value (incorrect!)
- **Solution**: Add explicit bounds checking:
  ```c
  long poverty_calc = 95L - curntn->tgold / curntn->tciv;
  if (poverty_calc < 0) {
      curntn->poverty = 0;
  } else if (poverty_calc > 255) {
      curntn->poverty = 255;
  } else {
      curntn->poverty = (unsigned char)poverty_calc;
  }
  ```
- **Impact**: Fix calculation logic, may keep some casts

**Estimated Casts Removed**: 0-2 (casts may remain with explicit bounds check)
**Estimated Time**: 45-60 minutes

---

### 4. forms.c - User Input (lines 1005, 1063-1065)

| Line | Current Code | Category | New Code | Usage | Risk Reason | Action |
|------|--------------|----------|----------|-------|-------------|--------|
| 1005 | `tax_rate = (unsigned char)intval` | B | Verify `tax_rate` is `unsigned char`, add bounds check | User input | User may input negative/large value | ✅ Verify + bounds check |
| 1063 | `terror += (unsigned char)intval` | B | Verify `terror` is `unsigned char`, add bounds check | User input calculation | May overflow | ✅ Verify + bounds check |
| 1064 | `popularity -= (unsigned char)intval` | B | Verify `popularity` is `unsigned char`, add bounds check | User input calculation | May underflow | ✅ Verify + bounds check |
| 1065 | `reputation -= (unsigned char)(intval+1)/2` | B | Verify `reputation` is `unsigned char`, add bounds check | User input calculation | May underflow | ✅ Verify + bounds check |

**Root Cause Analysis**:
- User input (`intval`) is cast to unsigned char without bounds checking
- If user enters negative value or value >255, results are incorrect
- **Solution**: Add input validation before assignment
- **Impact**: Fix input validation logic

**Estimated Casts Removed**: 0-2 (may need explicit conversion)
**Estimated Time**: 30-45 minutes

---

### 5. commands.c - Navy Crew Calculations (lines 856, 1037, 1057)

| Line | Current Code | Category | New Code | Usage | Risk Reason | Action |
|------|--------------|----------|----------|-------|-------------|--------|
| 856 | `P_NCREW += (unsigned char)amount` | B | Verify `P_NCREW` is `unsigned char`, add bounds check | Navy crew calculation | May overflow | ✅ Verify + bounds check |
| 1037 | `P_NCREW = (unsigned char)(tmpvar / flthold(nvynum))` | B | Add bounds checking | Division result | May be >255 | ✅ Add bounds check |
| 1057 | `P_NPEOP = (unsigned char)(onboard / fltmhold(nvynum))` | B | Add bounds checking | Division result | May be >255 | ✅ Add bounds check |

**Root Cause Analysis**:
- Navy crew calculations may exceed unsigned char range (0-255)
- Casting large value to unsigned char truncates (incorrect!)
- **Solution**: Add explicit bounds checking (clamp to 0-255)
- **Impact**: Fix calculation logic

**Estimated Casts Removed**: 0-1
**Estimated Time**: 30-45 minutes

---

### 6. reports.c - Navy Crew Calculations (lines 1255, 1257)

| Line | Current Code | Category | New Code | Usage | Risk Reason | Action |
|------|--------------|----------|----------|-------|-------------|--------|
| 1255 | `P_NCREW = (unsigned char)(crew / flthold(nvynum))` | B | Add bounds checking | Division result | May be >255 | ✅ Add bounds check |
| 1257 | `P_NPEOP = (unsigned char)(people / fltmhold(nvynum))` | B | Add bounds checking | Division result | May be >255 | ✅ Add bounds check |

**Root Cause Analysis**:
- Similar to commands.c, navy crew calculations may exceed range
- **Solution**: Add explicit bounds checking
- **Impact**: Fix calculation logic

**Estimated Casts Removed**: 0
**Estimated Time**: 20-30 minutes

---

## HIGH RISK Summary

| File | Lines | Casts Before | Estimated Casts After | Risk Category | Fix Strategy | Time Est. |
|------|-------|--------------|----------------------|---------------|--------------|-----------|
| misc.c | 2407-2440 | 8 | 0 | Array indexing (E) | Fix veg[], ele[] declarations | 45-60 min |
| spew.c | 1244 | 1 | 0 | ctype.h (D) | Fix end declaration or temp var | 30 min |
| update.c | 1636-1649 | 6 | 4-6 | Calculations (B) | Add bounds checking | 45-60 min |
| forms.c | 1005, 1063-1065 | 4 | 2-4 | User input (B) | Add input validation | 30-45 min |
| commands.c | 856, 1037, 1057 | 3 | 2-3 | Navy calc (B) | Add bounds checking | 30-45 min |
| reports.c | 1255, 1257 | 2 | 2 | Navy calc (B) | Add bounds checking | 20-30 min |
| **TOTAL** | - | **24** | **10-15** | - | - | **3-4.5 hours** |

**Expected Cast Reduction**: 24 → 10-15 (9-14 casts removed, ~42-58% reduction)

---

## MEDIUM RISK Files (Fix This Phase) 🟡

### Additional Casts Found (25 more casts in other files)

Analysis required for:
- admin.c, combat.c, newlogin.c, randeven.c, makeworl.c (ADMIN files)
- main.c, display.c, extcmds.c, move.c (GAME files)
- cexecute.c, io.c, navy.c, magic.c, data.c, trade.c (DUAL files)

**Estimated Additional Casts**: 25 (49 total - 24 HIGH RISK = 25 MEDIUM/LOW)

**Next Steps**:
1. Complete HIGH RISK fixes first
2. Review remaining 25 casts during HIGH RISK fixes
3. Classify as MEDIUM or LOW RISK
4. Address MEDIUM RISK in Phase 10.1
5. Defer LOW RISK to Phase 10.6+

---

## LOW RISK (Deferred) 🟢

### Text Strings (Category A)

Most of the **207 char pointer declarations** and **111 char array declarations** are likely text strings, which are safe to keep as plain `char`.

**Examples**:
- `char filename[256]` - Text string, safe
- `char *message` - Text string, safe
- `char buffer[BUFSIZ]` - Text buffer, safe

**Strategy**:
- Review during HIGH RISK fixes
- Document as Category A (safe as-is)
- No changes needed for Phase 10.1
- Defer detailed analysis to Phase 10.6+ if needed

---

## Audit Statistics

**Total Audit Findings**:
- Plain char declarations: **347 total**
  - Variable declarations: 12
  - Pointer declarations: 207
  - Array declarations: 111
  - Function parameters: 17

- Unsigned char casts: **49 occurrences**
  - HIGH RISK files: 24 casts
  - MEDIUM/LOW RISK files: 25 casts

- ctype.h function usage: **23 calls**
  - is* functions: 20
  - to* functions: 3

**Sign Conversion Warnings**: **167 warnings** (from baseline test with -fsigned-char)

---

## Phase 10.1 Focus (HIGH RISK Only)

**Files to Fix**: 6 (misc.c, spew.c, update.c, forms.c, commands.c, reports.c)
**Casts to Address**: 24 (of 49 total)
**Expected Cast Reduction**: 9-14 casts removed (~42-58% of HIGH RISK casts)
**Estimated Time**: 3-4.5 hours

**Success Criteria**:
- ✅ All array indexing uses explicit `unsigned char` (Category E)
- ✅ All ctype.h usage uses explicit `unsigned char` (Category D)
- ✅ All calculations have explicit bounds checking (Category B)
- ✅ No undefined behavior on x86_64
- ✅ Tests pass (10/10)
- ✅ Sign conversion warnings reduced significantly

---

## Testing Protocol

**After EACH file fix**:
1. Compile with `test_char_signed.sh filename.c`
2. Verify no new warnings introduced
3. Run full test suite: `cmake --build build --clean-first && ctest --test-dir build`
4. Verify 10/10 tests pass
5. Update this spreadsheet with "After" metrics
6. Git commit with before/after metrics

---

**Last Updated**: 2025-10-09
**Next Update**: After completing misc.c fix (first HIGH RISK file)
**Status**: Audit complete, ready for HIGH RISK fixes
**Next Task**: Fix misc.c array indexing (Task 4.1)
