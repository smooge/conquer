# Session Memory: Phase 8.4.3.2 - sort.c Bounds Checking (NEXT)

**Date**: 2025-10-06
**Phase**: Phase 8.4.3.2 - sort.c build_node() validation
**Status**: 🔜 READY TO START
**Previous Phase**: 8.4.3.1 COMPLETE ✅ (m2alloc refactoring)

## Objective

Apply bounds checking and validation to sort.c allocation function `build_node()`, following the successful pattern established in Phase 8.4.3.1.

## Target Function Analysis

### build_node() - sort.c:512-533

**Current Implementation**:
```c
static L_PTR build_node(char data[], L_PTR nptr) {
    L_PTR temp;

    /* build the memory space */
    if((temp=(L_PTR)malloc(sizeof(L_DATA)))==(L_PTR)NULL) {
        fprintf(stderr,"Error in creating structure memory!\n");
        exit(EX_SOFTWARE);
    }
    if((temp->line=(char *)malloc((strlen(data)+1)*sizeof(char)))==NULL) {
        fprintf(stderr,"Error in creating data memory!\n");
        exit(EX_SOFTWARE);
    }

    /* assign the values */
    size_t data_len = strlen(data);
    memcpy(temp->line, data, data_len);
    temp->line[data_len] = '\0';
    temp->next = nptr;
    return(temp);
}
```

**Current Issues**:
1. No validation of `data` parameter (could be NULL)
2. No length limit on string allocation
3. Uses `exit()` on failure (untestable)
4. Potential integer overflow: `strlen(data) + 1`

## Refactoring Decision

### Option A: Full Refactoring (like m2alloc)
- Extract testable `build_node_safe()` core function
- Create backward-compatible wrapper `build_node()`
- Add comprehensive test suite
- **Pros**: Consistent with Phase 8.4.3.1, fully testable
- **Cons**: More work, only used in one utility program (sort)

### Option B: In-Place Validation
- Add parameter validation to existing function
- Keep `exit()` behavior for now
- Add basic input validation tests
- **Pros**: Faster, simpler for utility code
- **Cons**: Still uses exit(), harder to test comprehensively

## Recommended Approach

**Option B** - In-place validation for this utility:

**Rationale**:
- `sort.c` is a standalone utility program (not game library code)
- Only 1 allocation function vs m2alloc's 11 call sites
- Utility programs traditionally use exit() for failures
- Can still add validation without full refactoring

**Validation to Add**:
1. NULL check for `data` parameter
2. Length limit check (e.g., MAX_LINE_LENGTH)
3. Overflow check for `strlen(data) + 1`
4. Better error messages with details

## Implementation Plan

### Step 1: Add Validation
```c
static L_PTR build_node(char data[], L_PTR nptr) {
    L_PTR temp;

    /* Validate input parameter */
    if (data == NULL) {
        fprintf(stderr, "build_node: NULL data parameter\n");
        exit(EX_SOFTWARE);
    }

    /* Check string length and overflow */
    size_t data_len = strlen(data);
    if (data_len > MAX_LINE_LENGTH) {
        fprintf(stderr, "build_node: String too long (%zu > %d)\n",
                data_len, MAX_LINE_LENGTH);
        exit(EX_SOFTWARE);
    }

    /* Check for overflow in allocation size */
    if (data_len >= SIZE_MAX - 1) {
        fprintf(stderr, "build_node: String length overflow\n");
        exit(EX_SOFTWARE);
    }

    /* ... rest of function ... */
}
```

### Step 2: Update Documentation
- Document parameter requirements
- Document maximum string length
- Document error exit conditions
- Add testing notes

### Step 3: Testing
- Create basic validation tests (if feasible for utility)
- Or document manual testing approach
- Test with edge cases (NULL, empty, very long strings)

## Files to Modify

1. **sort.c** - Add validation to `build_node()`
2. **header.h** or **sort.c** - Define `MAX_LINE_LENGTH` constant (suggest 65536)

## Success Criteria

- [ ] NULL data parameter handled safely
- [ ] String length limits enforced
- [ ] Integer overflow prevented
- [ ] Documentation updated
- [ ] Compiles with no new warnings
- [ ] Sort utility still functions correctly
- [ ] Changes committed to git

## Next Steps After 8.4.3.2

**Phase 8.4.3.3**: spew.c allocation validation
- 3 allocation sites to review
- `calloc()` for classes array
- `malloc()` for definitions
- `duplicate_string()` function

## Notes

- Consider whether utility programs need full test coverage
- Balance testing effort vs risk (utility vs core game code)
- Keep pattern consistent but pragmatic

---
**Status**: Ready to begin
**Estimated Duration**: 30-60 minutes
**Risk Level**: LOW (adding validation to utility)
