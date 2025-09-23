# Compilation Analysis Report
**Generated**: 2025-09-22 08:45:56

## Summary Statistics
- **Total Warnings**: 35
- **Total Errors**: 9
- **Files with Warnings**: 3
- **Files with Errors**: 5

## Warning Categories
- **unknown**: 35

## Files with Compilation Errors (Blockers)
### admin.c (1 errors)
- Line 306: ‘scenario’ undeclared (first use in this function)

### commands.c (1 errors)
- Line 1636: implicit declaration of function ‘move_file’; did you mean ‘move_people’?

### main.c (3 errors)
- Line 591: passing argument 2 of ‘signal’ from incompatible pointer type
- Line 592: passing argument 2 of ‘signal’ from incompatible pointer type
- Line 981: implicit declaration of function ‘trade’; did you mean ‘trace’?

### makeworl.c (1 errors)
- Line 1426: ‘npcsfile’ undeclared (first use in this function); did you mean ‘newsfile’?

### psmap.c (3 errors)
- Line 251: ‘DEFAULTPAGE’ undeclared (first use in this function)
- Line 686: ‘PSFILE’ undeclared (first use in this function); did you mean ‘FILE’?
- Line 810: ‘DEFAULTPAGE’ undeclared (first use in this function)

## Files with Warnings
### header.h (32 warnings)
- **unknown**: 32 warnings
  - Line 492: ‘VERSION’ redefined
  - Line 492: ‘VERSION’ redefined
  - Line 492: ‘VERSION’ redefined
  - ... and 29 more

### psmap.h (2 warnings)
- **unknown**: 2 warnings
  - Line 164: ‘VERSION’ redefined
  - Line 164: ‘VERSION’ redefined

### spew.c (1 warnings)
- **unknown**: 1 warnings
  - Line 139: ‘RANDOM’ redefined

## File Classification for Session Planning
- **admin.c**: 🔴 BLOCKED (1 errors, 0 warnings)
- **commands.c**: 🔴 BLOCKED (1 errors, 0 warnings)
- **header.h**: 🔴 COMPLEX (0 errors, 32 warnings)
- **main.c**: 🔴 BLOCKED (3 errors, 0 warnings)
- **makeworl.c**: 🔴 BLOCKED (1 errors, 0 warnings)
- **psmap.c**: 🔴 BLOCKED (3 errors, 0 warnings)
- **psmap.h**: 🟢 SMALL (0 errors, 2 warnings)
- **spew.c**: 🟢 SMALL (0 errors, 1 warnings)

## Recommendations
### Priority 1: Fix Compilation Errors
- Fix admin.c compilation errors before proceeding
- Fix commands.c compilation errors before proceeding
- Fix main.c compilation errors before proceeding
- Fix makeworl.c compilation errors before proceeding
- Fix psmap.c compilation errors before proceeding
### Priority 2: Address Top Warning Categories
- **unknown**: 35 instances - consider category-focused session