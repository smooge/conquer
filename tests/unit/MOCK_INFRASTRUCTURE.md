# Mock Infrastructure Requirements - Phase 6.4 Analysis

**Generated**: 2025-09-28
**Phase**: 6.4 - Function Documentation Analysis and Test Planning
**Purpose**: Detailed mock requirements and dependency analysis for Category A functions

## 📊 MOCK COMPLEXITY ANALYSIS

**Based on**: Top 15 Category A functions identified for Phase 6.5 implementation
**Testing Framework**: Unity C testing framework (operational)
**Mock Strategy**: Lightweight mocking for minimal dependencies

### **Mock Complexity Categories**
- **Level 0 (No Mocking)**: Pure functions with no external dependencies
- **Level 1 (Global State)**: Functions requiring global variable setup
- **Level 2 (Simple Mocking)**: Functions requiring basic function mocking
- **Level 3 (Complex Mocking)**: Functions requiring structure/file mocking

## 🟢 LEVEL 0 - NO MOCKING REQUIRED (3 functions)

**Immediate implementation candidates - pure algorithmic functions**

### **1. `comp_line()` (sort.c)**
- **Dependencies**: Global `compnum`, `iflag` variables, `UPPER` macro
- **Mock Requirements**: **NONE** - Pure function with global configuration
- **Test Setup**: Set global variables to known values
- **Implementation**: Direct function calls with various string combinations
- **Complexity**: **Simple** - No external function calls

### **2. `def_bonus()` (misc.c)**
- **Dependencies**: Constants and structure field access
- **Mock Requirements**: **NONE** - Calculation function
- **Test Setup**: Create mock sector structures with known values
- **Implementation**: Direct function calls with controlled inputs
- **Complexity**: **Simple** - Mathematical calculation only

### **3. `unitvalid()` (magic.c)**
- **Dependencies**: Minimal (documented as Category A)
- **Mock Requirements**: **NONE** - Pure logic function
- **Test Setup**: Direct input parameters
- **Implementation**: Boundary condition testing
- **Complexity**: **Simple** - Logic validation only

## 🟡 LEVEL 1 - GLOBAL STATE SETUP (4 functions)

**Functions requiring global variable initialization but no mocking**

### **4. `no_god()` (misc.c)**
- **Dependencies**: Global `country`, `curntn` variables, `ntn[]` array
- **Mock Requirements**: **Global state setup only**
- **Test Setup**: Initialize global variables to known states
- **Implementation**: Verify global state changes after function call
- **Complexity**: **Simple** - Global variable assignment verification

### **5. `flight_cost()` (misc.c)**
- **Dependencies**: Global arrays (`ele`, `veg`, `FElecost`, `FVegcost`)
- **Mock Requirements**: **Global arrays with known values**
- **Test Setup**: Initialize cost tables with test data
- **Implementation**: Direct function calls with coordinate parameters
- **Complexity**: **Simple** - Table lookup with boundary testing

### **6. `is_tech_good()` (misc.c)**
- **Dependencies**: `ntn[]` array, trade good constants
- **Mock Requirements**: **Mock nation data setup**
- **Test Setup**: Create nation structures with known ability/wealth values
- **Implementation**: Technology threshold validation testing
- **Complexity**: **Simple** - Threshold checking logic

### **7. `soldier_type()` (misc.c)**
- **Dependencies**: `ntn[]` nation data, power constants, `magic()` macro
- **Mock Requirements**: **Mock nation with power bitmasks**
- **Test Setup**: Create nations with specific magic power combinations
- **Implementation**: Priority logic testing for each power type
- **Complexity**: **Simple** - Priority checking with clear hierarchy

## 🟠 LEVEL 2 - SIMPLE FUNCTION MOCKING (4 functions)

**Functions requiring basic function call mocking**

### **8. `build_node()` (sort.c)**
- **Dependencies**: `malloc()`, `strcpy()`, `strlen()`, `exit()`
- **Mock Requirements**: **Memory allocation mocking**
- **Test Setup**: Mock malloc success/failure scenarios
- **Implementation**: Test allocation, string copying, error handling
- **Complexity**: **Moderate** - Memory allocation patterns

### **9. `num_powers()` (misc.c)**
- **Dependencies**: `magic()` macro, `powers[]` array, `abrt()` function
- **Mock Requirements**: **Mock magic() calls and nation data**
- **Test Setup**: Mock nations with known power bitmasks, mock abrt()
- **Implementation**: Count verification for each power category
- **Complexity**: **Moderate** - Function mocking with error conditions

### **10. `tofood()` (misc.c)**
- **Dependencies**: `vegfood` table, `tg_value` table, `magic()` macro
- **Mock Requirements**: **Mock magic() and global data tables**
- **Test Setup**: Mock sector data with known vegetation/race combinations
- **Implementation**: Food calculation verification with bonus systems
- **Complexity**: **Moderate** - Multiple bonus systems and race calculations

### **11. `tomagic()` (misc.c)**
- **Dependencies**: `num_powers()` function, `ntn[]` race data
- **Mock Requirements**: **Mock num_powers() calls**
- **Test Setup**: Mock nations with known race/power combinations
- **Implementation**: Cost calculation with exponential scaling
- **Complexity**: **Moderate** - Exponential calculations with race conditions

## 🔴 LEVEL 3 - COMPLEX MOCKING (4 functions)

**Functions requiring structure or I/O mocking**

### **12. `get_a_line()` (sort.c)**
- **Dependencies**: Global `infile` stream, `MAX_STR` constant
- **Mock Requirements**: **FILE stream operations (getc, feof)**
- **Test Setup**: Mock file streams with controlled character sequences
- **Implementation**: Character filtering and boundary condition testing
- **Complexity**: **Complex** - FILE I/O mocking with state management

### **13. `print_list()` (sort.c)**
- **Dependencies**: Global `head` pointer, global `outfile` stream
- **Mock Requirements**: **FILE stream operations (fprintf), linked list**
- **Test Setup**: Mock linked lists and output streams
- **Implementation**: List traversal and output verification
- **Complexity**: **Complex** - Combined data structure and I/O mocking

### **14. `place()` (sort.c)**
- **Dependencies**: Global `head` pointer, `comp_line()`, `build_node()`
- **Mock Requirements**: **Linked list manipulation, function mocking**
- **Test Setup**: Mock linked list states and helper functions
- **Implementation**: Insertion sort algorithm verification
- **Complexity**: **Complex** - Insertion sort logic with linked list state

### **15. `score_one()` (misc.c)**
- **Dependencies**: `num_powers()` function, `ntn[]` array, nation structure
- **Mock Requirements**: **Mock nation data with complex structures**
- **Test Setup**: Mock complete nation structures with known statistics
- **Implementation**: Class-specific scoring calculation verification
- **Complexity**: **Complex** - Multiple calculation systems and mock dependencies

## 🛠️ REQUIRED MOCK UTILITIES

### **Basic Mock Infrastructure**
```c
// Global state management
void setup_test_globals(void);
void cleanup_test_globals(void);

// Nation data mocking
typedef struct test_nation {
    int class;
    int race;
    long gold;
    long civilians;
    // ... other fields as needed
} test_nation_t;

void setup_mock_nation(int nation_id, test_nation_t *data);
```

### **Function Mocking Utilities**
```c
// Mock magic() macro
int mock_magic_result;
#define magic(n, p) (mock_magic_result)

// Mock malloc/memory functions
void* mock_malloc(size_t size);
void setup_malloc_failure(int fail_after_n_calls);

// Mock FILE operations
typedef struct {
    char *buffer;
    int position;
    int length;
} mock_file_t;

mock_file_t* create_mock_file(const char *content);
int mock_getc(FILE *stream);
int mock_feof(FILE *stream);
```

### **Data Structure Utilities**
```c
// Linked list testing utilities
typedef struct test_list_node {
    char *line;
    struct test_list_node *next;
} test_list_node_t;

test_list_node_t* create_test_list(char *lines[], int count);
void verify_list_order(test_list_node_t *head, char *expected[], int count);
```

## 📋 IMPLEMENTATION STRATEGY

### **Phase 6.5 Session Breakdown**

#### **Session 1: Level 0 + Level 1 Functions (2-3 hours)**
**Target**: 7 functions with minimal mocking requirements
- `comp_line()`, `def_bonus()`, `unitvalid()` (Level 0)
- `no_god()`, `flight_cost()`, `is_tech_good()`, `soldier_type()` (Level 1)

**Setup Required**:
- Basic global variable management utilities
- Mock nation/sector structure creation
- Direct function call testing patterns

#### **Session 2: Level 2 Functions (2-3 hours)**
**Target**: 4 functions with simple function mocking
- `build_node()`, `num_powers()`, `tofood()`, `tomagic()`

**Setup Required**:
- Function mocking infrastructure
- Memory allocation mocking
- Error condition simulation

#### **Session 3: Level 3 Functions (3-4 hours)**
**Target**: 4 functions with complex mocking
- `get_a_line()`, `print_list()`, `place()`, `score_one()`

**Setup Required**:
- FILE I/O mocking utilities
- Linked list testing infrastructure
- Complex state management

### **Success Metrics**
- **Test Coverage**: >90% line coverage for all tested functions
- **Mock Reusability**: Utilities usable for future function testing
- **Documentation**: Clear patterns for testing additional Category A functions
- **Automation**: All tests integrated with CMake/CTest

## 🔧 MOCK DEVELOPMENT PRIORITIES

### **Immediate Requirements (Session 1)**
1. **Global State Management**: Setup/cleanup utilities for global variables
2. **Mock Data Structures**: Basic nation/sector structure creation
3. **Test Assertion Helpers**: Custom assertions for game-specific data

### **Medium-Term Requirements (Session 2)**
1. **Function Mocking**: Basic function pointer substitution
2. **Memory Testing**: Allocation/failure simulation
3. **Error Condition Testing**: Controlled error scenario creation

### **Advanced Requirements (Session 3)**
1. **I/O Mocking**: FILE stream operation simulation
2. **State Machine Testing**: Complex linked list manipulation
3. **Integration Preparation**: Foundation for Category B function testing

---

**Mock Analysis Status**: Complete for Top 15 Category A Functions
**Implementation Ready**: Level 0-1 functions can begin immediately
**Infrastructure Plan**: Incremental mock utility development
**Total Estimated Effort**: 7-10 hours across 3 sessions