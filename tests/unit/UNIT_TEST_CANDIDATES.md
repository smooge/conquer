# Unit Test Candidates - Phase 6.4 Function Categorization

**Generated**: 2025-09-28
**Phase**: 6.4 - Function Documentation Analysis and Test Planning
**Purpose**: Categorized list of functions by testing complexity for Phase 6.5 prioritization

## 📊 CATEGORIZATION SUMMARY

**Total Functions Analyzed**: 241 functions across 25 files
**Category A (Unit Testable)**: 18+ functions identified (high priority)
**Category B (Integration)**: Majority of functions (moderate priority)
**Category C (System)**: System-dependent functions (lower priority)
**Category D (Mock)**: Mock-intensive functions (specialized priority)
**Category E (Skip)**: Deferred until post-modernization

### **Testing Infrastructure Ready**
- ✅ **Unity Framework**: Operational with CMake integration
- ✅ **Category A Template**: Safe conversion testing provides pattern
- ✅ **Build System**: CTest automation ready for expansion

## 🥇 CATEGORY A (UNIT TESTABLE) - HIGH PRIORITY

**Definition**: Pure functions with minimal dependencies, excellent for immediate unit testing

### **sort.c - Sorting and String Functions (4 Category A functions)**

#### **1. `get_a_line()` - Input Line Processing**
- **Purpose**: Read and filter characters from input stream
- **Complexity**: Moderate - Character filtering with boundary conditions
- **Dependencies**: Global `infile` stream, `MAX_STR` constant
- **Mock Requirements**: FILE stream operations (getc, feof)
- **Testing Focus**: Valid chars, invalid chars, long lines, EOF conditions

#### **2. `print_list()` - Linked List Output**
- **Purpose**: Output sorted linked list to file stream
- **Complexity**: Simple - Straightforward linked list traversal
- **Dependencies**: Global `head` pointer, global `outfile` stream
- **Mock Requirements**: FILE stream operations (fprintf), linked list nodes
- **Testing Focus**: Empty list, single item, multiple items, I/O errors

#### **3. `place()` - Insertion Sort Implementation**
- **Purpose**: Insert items into sorted linked list
- **Complexity**: Moderate - Insertion sort logic with linked list manipulation
- **Dependencies**: Global `head` pointer, `comp_line()`, `build_node()`
- **Mock Requirements**: String comparison, memory allocation, linked list
- **Testing Focus**: Empty list, insert positions, duplicate entries

#### **4. `comp_line()` - String Comparison**
- **Purpose**: Configurable string comparison with case/length options
- **Complexity**: Simple - String comparison with global configuration
- **Dependencies**: Global `compnum`, `iflag` variables, `UPPER` macro
- **Mock Requirements**: None (pure function with global config)
- **Testing Focus**: Equal strings, case variations, length comparisons

#### **5. `build_node()` - Node Allocation**
- **Purpose**: Allocate and initialize linked list nodes
- **Complexity**: Simple - Memory allocation with error checking
- **Dependencies**: `malloc()`, `strcpy()`, `strlen()`, `exit()`
- **Mock Requirements**: Memory allocation functions, error handling
- **Testing Focus**: Normal allocation, allocation failures, string copying

### **misc.c - Utility Functions (9 Category A functions)**

#### **6. `score_one()` - Nation Scoring Calculation**
- **Purpose**: Calculate nation score based on class and resources
- **Complexity**: Moderate - Multiple class-specific calculations
- **Dependencies**: `num_powers()` function, `ntn[]` array, nation structure
- **Mock Requirements**: Mock nation data with known values
- **Testing Focus**: Each nation class bonus calculation, resource weighting

#### **7. `num_powers()` - Magic Power Counting**
- **Purpose**: Count nation's magic powers by category
- **Complexity**: Simple - Counting loop with category selection
- **Dependencies**: `magic()` macro, `powers[]` array, `ntn[]` data, `abrt()`
- **Mock Requirements**: Mock nations with known power bitmasks
- **Testing Focus**: Each power category count, error handling

#### **8. `tofood()` - Food Production Calculation**
- **Purpose**: Calculate food production from sector vegetation
- **Complexity**: Moderate - Multiple bonus systems and race calculations
- **Dependencies**: `vegfood` table, `tg_value` table, `magic()` macro
- **Mock Requirements**: Mock sectors with known vegetation/trade goods
- **Testing Focus**: Base vegetation values, elf bonuses, trade good bonuses

#### **9. `tomagic()` - Magic Cost Calculation**
- **Purpose**: Calculate magic power acquisition cost
- **Complexity**: Moderate - Exponential calculations with race conditions
- **Dependencies**: `num_powers()` function, `ntn[]` race data
- **Mock Requirements**: Mock nations with known race/power combinations
- **Testing Focus**: Race-specific costs, exponential scaling, overflow protection

#### **10. `flight_cost()` - Flight Movement Cost**
- **Purpose**: Calculate cost for flying movement
- **Complexity**: Simple - Table lookup with error handling
- **Dependencies**: Global arrays (`ele`, `veg`, `FElecost`, `FVegcost`)
- **Mock Requirements**: Mock sector data with known terrain values
- **Testing Focus**: Valid coordinates, invalid terrain, cost combinations

#### **11. `no_god()` - God Mode Deactivation**
- **Purpose**: Reset global state from god mode
- **Complexity**: Simple - Global variable assignment
- **Dependencies**: Global variables (`country`, `curntn`), nation array
- **Mock Requirements**: Minimal - global variable monitoring
- **Testing Focus**: God nation assignment, pointer setup, state consistency

#### **12. `soldier_type()` - Army Type Priority**
- **Purpose**: Determine optimal army type based on magic powers
- **Complexity**: Simple - Priority checking with clear hierarchy
- **Dependencies**: `magic()` macro, `ntn[]` nation data, power constants
- **Mock Requirements**: Mock nations with known power combinations
- **Testing Focus**: Each magic power priority, combination effects

#### **13. `is_tech_good()` - Technology Validation**
- **Purpose**: Validate if nation can handle advanced technology
- **Complexity**: Simple - Threshold checking with clear logic
- **Dependencies**: `tofood()` function, `ntn[]` nation data
- **Mock Requirements**: Mock nations with known ability/wealth levels
- **Testing Focus**: Technology thresholds, wealth requirements, food viability

#### **14. `def_bonus()` - Defense Bonus Calculation**
- **Purpose**: Calculate defensive bonus for sectors
- **Complexity**: Simple - Calculation with designation-based logic
- **Dependencies**: `magic()` macro, sector designation/fortress fields
- **Mock Requirements**: Mock sectors with known designations
- **Testing Focus**: Each designation type, fortress scaling, magic bonus

### **magic.c - Magic System (1+ Category A functions)**

#### **15. `unitvalid()` - Unit Validation** ⭐ **DOCUMENTED EXAMPLE**
- **Purpose**: Pure logic with minimal dependencies
- **Complexity**: Simple (as noted in existing categorization)
- **Dependencies**: Minimal (as noted in documentation)
- **Mock Requirements**: Minimal
- **Testing Focus**: Unit validation logic, boundary conditions

## 🥈 CATEGORY B (INTEGRATION) - MEDIUM PRIORITY

**Definition**: Functions requiring system state or multiple modules

### **Examples from High-Priority Files**
- **check.c**: All 5 functions (validation requires full game state)
- **sort.c**: `main_sort()` function (file I/O and command-line processing)
- **magic.c**: `getmagic`, `exenewmgk`, `removemgk`, `takeover` (4 functions)

**Total Category B**: Estimated 150+ functions across all files

## 🥉 CATEGORY C (SYSTEM) - LOWER PRIORITY

**Definition**: Functions requiring full system initialization

### **Examples**
- **check.c**: `lock_check()` (filesystem and platform-specific locking)
- **io.c**: Most file I/O functions (16 functions)
- **newlogin.c**: Authentication functions (17 functions)

**Total Category C**: Estimated 60+ functions

## 🎯 CATEGORY D (MOCK) - SPECIALIZED PRIORITY

**Definition**: Functions testable with extensive mocking

### **Examples**
- **check.c**: `debugdata()` (debug function requiring controlled environment)
- Functions requiring complex external dependencies

**Total Category D**: Estimated 15+ functions

## ⏸️ CATEGORY E (SKIP) - DEFERRED

**Definition**: Functions to skip until post-modernization

### **Examples**
- Legacy functions with unclear behavior
- Functions scheduled for replacement in later phases

**Total Category E**: Estimated 5+ functions

## 🎯 PHASE 6.5 PRIORITY RANKING

### **Top 10 Category A Functions for Immediate Implementation**

1. **`comp_line()`** (sort.c) - Pure string comparison, no mocking needed
2. **`no_god()`** (misc.c) - Simple global state assignment
3. **`def_bonus()`** (misc.c) - Simple calculation with clear formula
4. **`is_tech_good()`** (misc.c) - Clear threshold validation logic
5. **`soldier_type()`** (misc.c) - Priority logic with magic power evaluation
6. **`flight_cost()`** (misc.c) - Table lookup with error handling
7. **`build_node()`** (sort.c) - Memory allocation with error checking
8. **`print_list()`** (sort.c) - Linked list traversal and output
9. **`unitvalid()`** (magic.c) - Documented Category A example
10. **`place()`** (sort.c) - Insertion sort logic

### **Selection Criteria**
- **Minimal Dependencies**: Functions with fewest external requirements
- **Clear Logic**: Well-defined inputs, outputs, and behavior
- **Testing Value**: Functions that provide good testing coverage
- **Implementation Simplicity**: Easiest to implement and verify

### **Estimated Implementation Effort**
- **Session 1**: Functions 1-3 (simple functions, 2-3 hours)
- **Session 2**: Functions 4-6 (moderate complexity, 2-3 hours)
- **Session 3**: Functions 7-10 (complex logic/mocking, 3-4 hours)

## 📋 NEXT STEPS FOR PHASE 6.5

### **Immediate Requirements**
1. **Mock Infrastructure Planning**: Design reusable mocking utilities
2. **Test Template Creation**: Standard patterns for Category A functions
3. **Dependency Analysis**: Detailed analysis of top 10 functions
4. **Implementation Strategy**: Session-by-session breakdown

### **Success Criteria for Category A Testing**
- **Unit Tests**: 10+ Category A functions tested
- **Test Coverage**: >90% line coverage for tested functions
- **Mock Infrastructure**: Reusable utilities for future testing
- **Documentation**: Clear patterns for testing additional functions

---

**Categorization Status**: Complete for Priority Analysis
**Next Phase**: Detailed Category A Function Analysis and Mock Planning
**Ready for Phase 6.5**: Top 10 functions identified and prioritized
**Infrastructure**: Unity framework operational and ready for expansion