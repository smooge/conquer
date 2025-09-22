# Compilation Analysis Report
**Generated**: 2025-09-22 07:53:32

## Summary Statistics
- **Total Warnings**: 39
- **Total Errors**: 121
- **Files with Warnings**: 5
- **Files with Errors**: 23

## Warning Categories
- **unknown**: 39

## Files with Compilation Errors (Blockers)
### admin.c (4 errors)
- Line 306: ‘scenario’ undeclared (first use in this function)
- Line 350: implicit declaration of function ‘verifydata’
- Line 412: implicit declaration of function ‘check_lock’; did you mean ‘checkout’?
- Line 736: implicit declaration of function ‘tofood’

### cexecute.c (4 errors)
- Line 351: implicit declaration of function ‘destroy’
- Line 417: conflicting types for ‘hangup’; have ‘void(void)’
- Line 351: implicit declaration of function ‘destroy’
- Line 417: conflicting types for ‘hangup’; have ‘void(void)’

### combat.c (5 errors)
- Line 1122: implicit declaration of function ‘tofood’
- Line 1368: implicit declaration of function ‘fltwhold’
- Line 1374: implicit declaration of function ‘fltmhold’
- Line 1379: implicit declaration of function ‘fltghold’
- Line 1524: implicit declaration of function ‘flthold’

### commands.c (8 errors)
- Line 132: implicit declaration of function ‘tofood’
- Line 133: implicit declaration of function ‘errormsg’
- Line 313: implicit declaration of function ‘clear_bottom’
- Line 362: implicit declaration of function ‘get_country’; did you mean ‘getcury’?
- Line 740: implicit declaration of function ‘flthold’
- ... and 3 more errors

### data.h (8 errors)
- Line 684: implicit declaration of function ‘addwships’; did you mean ‘subwships’?
- Line 686: implicit declaration of function ‘addgships’; did you mean ‘subgships’?
- Line 685: implicit declaration of function ‘addmships’; did you mean ‘submships’?
- Line 685: implicit declaration of function ‘addmships’; did you mean ‘submships’?
- Line 686: implicit declaration of function ‘addgships’; did you mean ‘subgships’?
- ... and 3 more errors

### display.c (2 errors)
- Line 246: implicit declaration of function ‘errormsg’
- Line 580: implicit declaration of function ‘tofood’

### extcmds.c (3 errors)
- Line 118: implicit declaration of function ‘clear_bottom’
- Line 151: implicit declaration of function ‘addgroup’
- Line 167: implicit declaration of function ‘errormsg’

### foo.h (6 errors)
- Line 125: conflicting types for ‘main’; have ‘int(void)’
- Line 175: conflicting types for ‘readmap’; have ‘void(void)’
- Line 201: conflicting types for ‘place’; have ‘void(char *)’
- Line 203: unknown type name ‘L_PTR’
- Line 203: unknown type name ‘L_PTR’
- ... and 1 more errors

### forms.c (4 errors)
- Line 482: implicit declaration of function ‘get_country’; did you mean ‘getcury’?
- Line 488: implicit declaration of function ‘errormsg’
- Line 985: implicit declaration of function ‘destroy’
- Line 1084: implicit declaration of function ‘clear_bottom’

### io.c (6 errors)
- Line 1070: implicit declaration of function ‘clear_bottom’
- Line 1076: implicit declaration of function ‘errormsg’
- Line 1342: implicit declaration of function ‘tofood’
- Line 1070: implicit declaration of function ‘clear_bottom’
- Line 1076: implicit declaration of function ‘errormsg’
- ... and 1 more errors

### magic.c (16 errors)
- Line 416: implicit declaration of function ‘num_powers’
- Line 478: implicit declaration of function ‘errormsg’
- Line 492: implicit declaration of function ‘orctake’
- Line 587: implicit declaration of function ‘destroy’
- Line 710: implicit declaration of function ‘updmove’
- ... and 11 more errors

### main.c (8 errors)
- Line 310: implicit declaration of function ‘verifydata’
- Line 333: implicit declaration of function ‘check_lock’; did you mean ‘checkout’?
- Line 576: implicit declaration of function ‘updmove’
- Line 924: implicit declaration of function ‘units_in_sector’; did you mean ‘solds_in_sector’?
- Line 1052: implicit declaration of function ‘clear_bottom’
- ... and 3 more errors

### makeworl.c (4 errors)
- Line 342: implicit declaration of function ‘verifydata’
- Line 928: implicit declaration of function ‘tofood’
- Line 1426: ‘npcsfile’ undeclared (first use in this function); did you mean ‘newsfile’?
- Line 1506: implicit declaration of function ‘doclass’

### misc.c (5 errors)
- Line 2398: implicit declaration of function ‘errormsg’
- Line 2496: implicit declaration of function ‘clear_bottom’
- Line 875: implicit declaration of function ‘num_powers’
- Line 2398: implicit declaration of function ‘errormsg’
- Line 2496: implicit declaration of function ‘clear_bottom’

### move.c (2 errors)
- Line 141: implicit declaration of function ‘clear_bottom’
- Line 144: implicit declaration of function ‘errormsg’

### navy.c (4 errors)
- Line 1064: implicit declaration of function ‘clear_bottom’
- Line 1067: implicit declaration of function ‘errormsg’
- Line 1064: implicit declaration of function ‘clear_bottom’
- Line 1067: implicit declaration of function ‘errormsg’

### newlogin.c (2 errors)
- Line 1849: implicit declaration of function ‘tofood’
- Line 2200: implicit declaration of function ‘doclass’

### npc.c (4 errors)
- Line 492: implicit declaration of function ‘land_reachp’; did you mean ‘water_reachp’?
- Line 751: implicit declaration of function ‘tofood’
- Line 1724: implicit declaration of function ‘pceattr’; did you mean ‘prtattr’?
- Line 1821: implicit declaration of function ‘spreadsheet’

### psmap.c (3 errors)
- Line 251: ‘DEFAULTPAGE’ undeclared (first use in this function)
- Line 686: ‘PSFILE’ undeclared (first use in this function); did you mean ‘FILE’?
- Line 810: ‘DEFAULTPAGE’ undeclared (first use in this function)

### reports.c (9 errors)
- Line 236: implicit declaration of function ‘clear_bottom’
- Line 237: implicit declaration of function ‘errormsg’
- Line 423: implicit declaration of function ‘addgroup’
- Line 621: implicit declaration of function ‘spreadsheet’
- Line 635: implicit declaration of function ‘flthold’
- ... and 4 more errors

### test_headers.c (2 errors)
- Line 11: conflicting types for ‘main’; have ‘int(void)’
- Line 11: number of arguments doesn’t match prototype

### trade.c (6 errors)
- Line 181: implicit declaration of function ‘tofood’
- Line 352: implicit declaration of function ‘flthold’
- Line 598: implicit declaration of function ‘clear_bottom’
- Line 181: implicit declaration of function ‘tofood’
- Line 352: implicit declaration of function ‘flthold’
- ... and 1 more errors

### update.c (6 errors)
- Line 194: implicit declaration of function ‘destroy’
- Line 305: implicit declaration of function ‘tofood’
- Line 562: implicit declaration of function ‘land_reachp’; did you mean ‘water_reachp’?
- Line 1029: implicit declaration of function ‘updmove’
- Line 1603: implicit declaration of function ‘spreadsheet’
- ... and 1 more errors

## Files with Warnings
### header.h (32 warnings)
- **unknown**: 32 warnings
  - Line 492: ‘VERSION’ redefined
  - Line 492: ‘VERSION’ redefined
  - Line 492: ‘VERSION’ redefined
  - ... and 29 more

### extcmds.c (3 warnings)
- **unknown**: 3 warnings
  - Line 622: conflicting types for ‘errormsg’; have ‘void()’
  - Line 666: conflicting types for ‘clear_bottom’; have ‘void()’
  - Line 714: conflicting types for ‘addgroup’; have ‘void()’

### psmap.h (2 warnings)
- **unknown**: 2 warnings
  - Line 164: ‘VERSION’ redefined
  - Line 164: ‘VERSION’ redefined

### npc.c (1 warnings)
- **unknown**: 1 warnings
  - Line 3164: conflicting types for ‘pceattr’; have ‘void(void)’

### spew.c (1 warnings)
- **unknown**: 1 warnings
  - Line 139: ‘RANDOM’ redefined

## File Classification for Session Planning
- **admin.c**: 🔴 BLOCKED (4 errors, 0 warnings)
- **cexecute.c**: 🔴 BLOCKED (4 errors, 0 warnings)
- **combat.c**: 🔴 BLOCKED (5 errors, 0 warnings)
- **commands.c**: 🔴 BLOCKED (8 errors, 0 warnings)
- **data.h**: 🔴 BLOCKED (8 errors, 0 warnings)
- **display.c**: 🔴 BLOCKED (2 errors, 0 warnings)
- **extcmds.c**: 🔴 BLOCKED (3 errors, 3 warnings)
- **foo.h**: 🔴 BLOCKED (6 errors, 0 warnings)
- **forms.c**: 🔴 BLOCKED (4 errors, 0 warnings)
- **header.h**: 🔴 COMPLEX (0 errors, 32 warnings)
- **io.c**: 🔴 BLOCKED (6 errors, 0 warnings)
- **magic.c**: 🔴 BLOCKED (16 errors, 0 warnings)
- **main.c**: 🔴 BLOCKED (8 errors, 0 warnings)
- **makeworl.c**: 🔴 BLOCKED (4 errors, 0 warnings)
- **misc.c**: 🔴 BLOCKED (5 errors, 0 warnings)
- **move.c**: 🔴 BLOCKED (2 errors, 0 warnings)
- **navy.c**: 🔴 BLOCKED (4 errors, 0 warnings)
- **newlogin.c**: 🔴 BLOCKED (2 errors, 0 warnings)
- **npc.c**: 🔴 BLOCKED (4 errors, 1 warnings)
- **psmap.c**: 🔴 BLOCKED (3 errors, 0 warnings)
- **psmap.h**: 🟢 SMALL (0 errors, 2 warnings)
- **reports.c**: 🔴 BLOCKED (9 errors, 0 warnings)
- **spew.c**: 🟢 SMALL (0 errors, 1 warnings)
- **test_headers.c**: 🔴 BLOCKED (2 errors, 0 warnings)
- **trade.c**: 🔴 BLOCKED (6 errors, 0 warnings)
- **update.c**: 🔴 BLOCKED (6 errors, 0 warnings)

## Recommendations
### Priority 1: Fix Compilation Errors
- Fix admin.c compilation errors before proceeding
- Fix cexecute.c compilation errors before proceeding
- Fix combat.c compilation errors before proceeding
- Fix commands.c compilation errors before proceeding
- Fix data.h compilation errors before proceeding
- Fix display.c compilation errors before proceeding
- Fix extcmds.c compilation errors before proceeding
- Fix foo.h compilation errors before proceeding
- Fix forms.c compilation errors before proceeding
- Fix io.c compilation errors before proceeding
- Fix magic.c compilation errors before proceeding
- Fix main.c compilation errors before proceeding
- Fix makeworl.c compilation errors before proceeding
- Fix misc.c compilation errors before proceeding
- Fix move.c compilation errors before proceeding
- Fix navy.c compilation errors before proceeding
- Fix newlogin.c compilation errors before proceeding
- Fix npc.c compilation errors before proceeding
- Fix psmap.c compilation errors before proceeding
- Fix reports.c compilation errors before proceeding
- Fix test_headers.c compilation errors before proceeding
- Fix trade.c compilation errors before proceeding
- Fix update.c compilation errors before proceeding
### Priority 2: Address Top Warning Categories
- **unknown**: 39 instances - consider category-focused session