# Realistic Function Testability Assessment

**Generated**: 2025-09-28
**Phase**: 6.5 - Unit Test Implementation Reality Check
**Purpose**: Honest assessment of what functions can be tested without fragile code copying

## 🎯 TESTING PHILOSOPHY

**Core Principle**: Tests should validate actual compiled code, not copied implementations.

**Testing Approaches**:
1. **Pure Functions**: Direct testing with proper linking
2. **State-Dependent Functions**: Mock/stub external dependencies
3. **Complex Functions**: Mark for refactoring in Phase 8/9
4. **System Functions**: Integration testing only

## ✅ SUCCESSFULLY TESTABLE FUNCTIONS

### **Level 0: Pure Functions (No Dependencies)**

#### **1. `comp_line()` (sort.c) - ✅ WORKING**
- **Status**: 15 tests passing
- **Approach**: Static function - implementation copied for testing
- **Dependencies**: Global variables `compnum`, `iflag` (manageable)
- **Test Coverage**: Comprehensive (case sensitivity, length limits, boundaries)

#### **2. Functions from misc.c with Object Linking**
**Approach**: Link with shared_admin_objects or shared_game_objects

**Candidates for Testing**:
- `todigit()` - Character to digit conversion (pure function)
- `is_habitable()` - Sector habitability check (may need mock data)
- `solds_in_sector()` - Count soldiers (needs world data)
- `units_in_sector()` - Count units (needs world data)

**Linking Strategy**:
```cmake
add_conquer_test(test_misc_functions test_misc_functions.c)
target_link_libraries(test_misc_functions PRIVATE shared_admin_objects)
```

### **Level 1: Functions with Minimal Global State**

#### **3. `unitvalid()` (magic.c) - Needs Mocking**
- **Dependencies**:
  - Global `country` variable
  - `ntn[]` array (nation data)
  - `magic()` macro accessing nation powers
  - `defaultunit()` function
- **Mock Requirements**: Nation data structures, power bitmasks
- **Testability**: Medium - requires comprehensive mock infrastructure

## ❌ FUNCTIONS REQUIRING REFACTORING

### **Too Much Global State (Phase 8/9 Candidates)**

#### **1. `get_number()` (misc.c)**
- **Issue**: Heavy curses library dependencies (stdscr, getch, addch, refresh)
- **Refactor Need**: Extract input logic from display logic
- **Future Approach**: Pure numeric parsing function + display wrapper

#### **2. `score_one()` (misc.c)**
- **Issue**: Massive dependencies on global game state
- **Dependencies**: Nation data, world data, economic calculations
- **Refactor Need**: Break into smaller calculation functions

#### **3. Most Display/I/O Functions**
- **Issue**: Direct terminal/file dependencies
- **Examples**: `prep()`, most functions using global arrays
- **Refactor Need**: Dependency injection patterns

### **System-Level Functions (Integration Testing Only)**

#### **1. `move_file()` (misc.c)**
- **Issue**: Filesystem operations (link/unlink)
- **Testing**: System/integration tests with temporary files
- **Unit Testing**: Not appropriate

#### **2. Network/File I/O Functions**
- **Issue**: External system dependencies
- **Testing**: Integration testing with controlled environments

## 📋 REALISTIC PHASE 6.5 PLAN

### **Session 1: Achievable Goals (2-3 hours)**

#### **A. Complete Level 0 Testing**
1. **comp_line()** - ✅ Done (15 tests passing)
2. **todigit() with proper linking** - Test character conversion
3. **Create mock infrastructure templates** for Level 1

#### **B. Mock Infrastructure Development**
1. **Global state management utilities**
2. **Nation data structure mocking**
3. **Simple power bitmask simulation**

#### **C. Limited Level 1 Testing**
1. **unitvalid() with basic mocking** - Test a few unit types
2. **Document mock requirements** for expansion

### **Session 1 Success Criteria**
- **2-3 functions tested** with proper linking (no code copying)
- **Mock infrastructure** operational for future sessions
- **Documentation** of refactoring needs for Phase 8/9

## 🔧 TECHNICAL IMPLEMENTATION

### **Proper Object Linking Strategy**
```cmake
# Link with compiled objects to test actual functions
add_conquer_test(test_todigit test_todigit.c)
target_link_libraries(test_todigit PRIVATE shared_admin_objects)

# Include necessary headers
target_include_directories(test_todigit PRIVATE "${CMAKE_SOURCE_DIR}")
```

### **Mock Infrastructure Templates**
```c
// Global state management
void setup_test_globals(void);
void cleanup_test_globals(void);

// Nation data mocking
typedef struct mock_nation {
    unsigned long powers;
    int race;
    // ... minimal required fields
} mock_nation_t;

extern mock_nation_t test_nations[MAXNTN];
extern int test_country;
```

## 📝 REFACTORING RECOMMENDATIONS

### **Phase 8/9 Function Extraction Targets**

#### **1. Extract Pure Logic from I/O**
```c
// Current: get_number() - mixed I/O and logic
// Refactor to:
int parse_numeric_input(const char* input, int max_digits);
void display_number_prompt(int current_value);
```

#### **2. Dependency Injection Patterns**
```c
// Current: score_one(int nation) - uses global state
// Refactor to:
int calculate_nation_score(const nation_data_t* nation,
                          const world_data_t* world);
```

#### **3. Function Decomposition**
- Break large functions into smaller, testable components
- Separate calculation logic from data access
- Extract constants and magic numbers into named parameters

## 🎯 SUCCESS METRICS

### **Phase 6.5 Realistic Goals**
- **3-5 functions tested** with proper methodology
- **0 code copying** - all tests use actual compiled functions
- **Mock infrastructure** ready for Category B functions
- **Refactoring list** documented for future phases

### **Long-Term Vision (Phase 8/9)**
- **50+ functions testable** after refactoring
- **Comprehensive mock libraries** for complex state
- **CI/CD integration** with reliable test suite
- **Regression protection** for all modernization work

---

**Key Insight**: Phase 6.5 is about establishing **correct testing patterns** and **infrastructure**, not maximizing function count. Quality over quantity ensures sustainable testing as we modernize the codebase.