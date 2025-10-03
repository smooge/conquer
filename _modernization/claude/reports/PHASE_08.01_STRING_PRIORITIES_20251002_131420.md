# 📊 Legacy Pattern Detection Report

**Generated**: 2025-10-02 13:14:20

## 🎯 Executive Summary

| Metric | Value |
|--------|-------|
| **Total Files Scanned** | 51 |
| **Files Requiring Modernization** | 21 (41.2%) |
| **Total Legacy Patterns Found** | 151 |
| **Categories with Issues** | 1 |

## ⚡ Priority Dashboard

| Priority | Category | Files | Instances | Description |
|----------|----------|-------|-----------|-------------|
| 🔴 HIGH | **String** | 21 | 151 | Security risk - buffer overflows |

## 📈 Modernization Effort Estimate

- **String**: 21 files × 2h = 42.0h 🔴 HIGH

**Total Estimated Effort**: 42.0 hours (5.2 days)

## 📁 Files by Priority

### 🔴 HIGH String (21 files)
*Security risk - buffer overflows*

- `admin.c` (14 patterns)
- `cexecute.c` (3 patterns)
- `commands.c` (6 patterns)
- `forms.c` (11 patterns)
- `io.c` (4 patterns)
- `magic.c` (4 patterns)
- `main.c` (23 patterns)
- `misc.c` (15 patterns)
- `newlogin.c` (14 patterns)
- `psmap.c` (2 patterns)
- `randeven.c` (28 patterns)
- `safe_system.c` (1 patterns)
- `sort.c` (2 patterns)
- `spew.c` (5 patterns)
- `tests/integration/test_example_integration.c` (1 patterns)
- `tests/regression/test_example_regression.c` (2 patterns)
- `tests/unit/mock_infrastructure.h` (2 patterns)
- `tests/unit/test_sort_utils.c` (4 patterns)
- `tests/unit/test_spew_utils.c` (1 patterns)
- `trade.c` (4 patterns)
- `update.c` (5 patterns)

## 🔍 Detailed Pattern Analysis

*Note: This section provides technical details for developers. Management can focus on the summary above.*

### 📄 `admin.c` (14 patterns)

#### 🔴 HIGH String (14 instances)

**Lines 285, 286, 365, 368, 389, 390, 393**: Unsafe strcpy() - no bounds checking
```c
// 7 occurrences found
// Example: strcpy(datadir,"");
```

**Lines 387, 471, 481, 488, 508, 566, 574**: Unsafe sprintf() - no bounds checking
```c
// 7 occurrences found
// Example: sprintf(defaultdir, "%s/%s", DEFAULTDIR, datadir);
```


### 📄 `cexecute.c` (3 patterns)

#### 🔴 HIGH String (3 instances)

**Line 230**: Unsafe strcpy() - no bounds checking
```c
strcpy(curntn->name,comment);
```

**Lines 137, 428**: Unsafe sprintf() - no bounds checking
```c
// 2 occurrences found
// Example: sprintf(line,"%s%d",exefile,country);
```


### 📄 `commands.c` (6 patterns)

#### 🔴 HIGH String (6 instances)

**Lines 1584, 1609, 1762, 1770, 1784**: Unsafe strcpy() - no bounds checking
```c
// 5 occurrences found
// Example: if(msglen<LINELTH) strcpy(save[msglen],line);
```

**Line 136**: Unsafe sprintf() - no bounds checking
```c
sprintf(buf,"Must first burn down city/capitol (designate as '%c')",DRUIN);
```


### 📄 `forms.c` (11 patterns)

#### 🔴 HIGH String (11 instances)

**Lines 780, 1042, 1128, 1376, 1410**: Unsafe strcpy() - no bounds checking
```c
// 5 occurrences found
// Example: strcpy(curntn->name,string);
```

**Lines 516, 975, 991, 1101, 1332, 1359**: Unsafe sprintf() - no bounds checking
```c
// 6 occurrences found
// Example: sprintf(name,"%s%d",exefile,nation);
```


### 📄 `io.c` (4 patterns)

#### 🔴 HIGH String (4 instances)

**Lines 1390, 1408**: Unsafe strcpy() - no bounds checking
```c
// 2 occurrences found
// Example: strcpy(line,scenario);
```

**Lines 1391, 1409**: Unsafe strcat() - no bounds checking
```c
// 2 occurrences found
// Example: strcat(line,".ele");
```


### 📄 `magic.c` (4 patterns)

#### 🔴 HIGH String (4 instances)

**Lines 887, 895, 1571, 1620**: Unsafe sprintf() - no bounds checking
```c
// 4 occurrences found
// Example: sprintf(line,"you dont have %d spell points",s_cost);
```


### 📄 `main.c` (23 patterns)

#### 🔴 HIGH String (23 instances)

**Lines 142, 143, 144, 228, 231, 263, 267, 372, 376, 570, 650, 1139**: Unsafe strcpy() - no bounds checking
```c
// 12 occurrences found
// Example: strcpy(name,"");
```

**Lines 261, 334, 422, 625, 645, 648, 1156, 1161, 1162, 1178, 1659**: Unsafe sprintf() - no bounds checking
```c
// 11 occurrences found
// Example: sprintf(defaultdir, "%s/%s", DEFAULTDIR, optarg);
```


### 📄 `misc.c` (15 patterns)

#### 🔴 HIGH String (15 instances)

**Lines 1613, 2681, 2687, 2699, 2705, 2716, 2720, 2817, 2820, 2921, 2929, 2937, 2945, 2953, 2961**: Unsafe sprintf() - no bounds checking
```c
// 15 occurrences found
// Example: sprintf(buf,"%s%d",msgfile,cntry);
```


### 📄 `newlogin.c` (14 patterns)

#### 🔴 HIGH String (14 instances)

**Lines 1172, 1220**: Unsafe strcpy() - no bounds checking
```c
// 2 occurrences found
// Example: strcpy(curntn->name,tempc);
```

**Lines 822, 825, 834, 839, 1120, 1130, 1476, 1567, 1619, 2063, 2071, 2396**: Unsafe sprintf() - no bounds checking
```c
// 12 occurrences found
// Example: sprintf(tempc,"%s %s", LType[spent[item]], Mitems[item]);
```


### 📄 `psmap.c` (2 patterns)

#### 🔴 HIGH String (2 instances)

**Lines 787, 795**: Unsafe strcpy() - no bounds checking
```c
// 2 occurrences found
// Example: strcpy(progname, argv[0]);
```


### 📄 `randeven.c` (28 patterns)

#### 🔴 HIGH String (28 instances)

**Lines 250, 251, 387, 408, 435, 439, 443, 624, 729, 766, 778, 784, 854, 859, 864, 1040, 1289, 1503**: Unsafe strcpy() - no bounds checking
```c
// 18 occurrences found
// Example: strcpy(ntn[newntn].leader,"rebel");
```

**Lines 396, 448, 450, 829, 845, 872, 965, 1069, 1080, 1272**: Unsafe sprintf() - no bounds checking
```c
// 10 occurrences found
// Example: sprintf(eventstr,"rebellion joins nation %s",ntn[new].name);
```


### 📄 `safe_system.c` (1 patterns)

#### 🔴 HIGH String (1 instances)

**Line 296**: Unsafe strcpy() - no bounds checking
```c
strcpy(node->line, line);
```


### 📄 `sort.c` (2 patterns)

#### 🔴 HIGH String (2 instances)

**Lines 502, 528**: Unsafe strcpy() - no bounds checking
```c
// 2 occurrences found
// Example: *   Dependencies: malloc(), strcpy(), strlen(), exit()
```


### 📄 `spew.c` (5 patterns)

#### 🔴 HIGH String (5 instances)

**Lines 273, 1189, 1352, 1427, 1438**: Unsafe strcpy() - no bounds checking
```c
// 5 occurrences found
// Example: strcpy(main_class, "MAIN/ ");
```


### 📄 `tests/integration/test_example_integration.c` (1 patterns)

#### 🔴 HIGH String (1 instances)

**Line 36**: Unsafe strcpy() - no bounds checking
```c
strcpy(buffer, test_data);
```


### 📄 `tests/regression/test_example_regression.c` (2 patterns)

#### 🔴 HIGH String (2 instances)

**Line 40**: Unsafe strcpy() - no bounds checking
```c
strcpy(game_name, "Conquer");
```

**Line 109**: Unsafe sprintf() - no bounds checking
```c
sprintf(buffer, "Test %d", 42);
```


### 📄 `tests/unit/mock_infrastructure.h` (2 patterns)

#### 🔴 HIGH String (2 instances)

**Lines 67, 75**: Unsafe strcpy() - no bounds checking
```c
// 2 occurrences found
// Example: strcpy(mock->nations[0].name, "TestNation0");
```


### 📄 `tests/unit/test_sort_utils.c` (4 patterns)

#### 🔴 HIGH String (4 instances)

**Lines 41, 77, 205, 211**: Unsafe strcpy() - no bounds checking
```c
// 4 occurrences found
// Example: (void) strcpy(temp->line,data);
```


### 📄 `tests/unit/test_spew_utils.c` (1 patterns)

#### 🔴 HIGH String (1 instances)

**Line 54**: Unsafe strcpy() - no bounds checking
```c
strcpy(copy, str);
```


### 📄 `trade.c` (4 patterns)

#### 🔴 HIGH String (4 instances)

**Lines 1293, 1294**: Unsafe strcpy() - no bounds checking
```c
// 2 occurrences found
// Example: strcpy(cname[0],ntn[cntry1].name);
```

**Lines 1291, 1292**: Unsafe sprintf() - no bounds checking
```c
// 2 occurrences found
// Example: sprintf(filename[0],"%s%d",msgfile,cntry1);
```


### 📄 `update.c` (5 patterns)

#### 🔴 HIGH String (5 instances)

**Lines 80, 151, 212, 223, 741**: Unsafe sprintf() - no bounds checking
```c
// 5 occurrences found
// Example: sprintf(tempstr,"%-60.0lf",d);
```


---

## 🚀 Recommended Next Steps

1. **Phase 8.1**: Address HIGH priority security issues (string/system patterns)
2. **Phase 8.2**: Modernize memory management patterns
3. **Phase 8.3**: Update function declarations and improve code quality
4. **Phase 8.4**: Complete remaining low-priority modernizations

*Report generated by Legacy Pattern Detection Script v2.0*