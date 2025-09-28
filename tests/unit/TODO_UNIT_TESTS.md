# Phase 6.5: Unit Test Implementation TODO

**Phase**: 6.5 - Unit Test Implementation
**Duration**: 3 sessions (7-10 hours total)
**Objective**: Implement comprehensive unit tests for top 15 Category A functions
**Prerequisites**: ✅ Phase 6.4 Complete - Function analysis and mock planning ready

## 🎯 PHASE 6.5 OBJECTIVES

**Primary Goal**: Implement unit tests for 15 priority Category A functions across 3 sessions with increasing mock complexity.

**Success Metrics**:
- **Functions Tested**: 15 Category A functions with comprehensive test coverage
- **Test Coverage**: >90% line coverage for all tested functions
- **Mock Infrastructure**: Reusable utilities for future function testing
- **Integration**: All tests automated with CMake/CTest
- **Documentation**: Clear patterns established for additional function testing

## 📋 SESSION BREAKDOWN

### **SESSION 1: Level 0-1 Functions (2-3 hours)**
**Target**: 7 functions with minimal mocking requirements
**Mock Complexity**: Global state setup only, no external function mocking

#### **Level 0 Functions (No Mocking) - 3 functions**

##### **Task 1.1: `comp_line()` Testing (sort.c)**
**Estimated Time**: 30-45 minutes
**Complexity**: Simple - Pure string comparison function

**Implementation Steps**:
- [ ] Create `test_comp_line.c` test file
- [ ] Set up global variables (`compnum`, `iflag`)
- [ ] Test equal strings with various configurations
- [ ] Test case-sensitive vs case-insensitive comparison
- [ ] Test length-limited comparison (`compnum` variations)
- [ ] Test boundary conditions (empty strings, null characters)
- [ ] Verify return values (-1, 0, 1) for all scenarios

**Test Cases Required**:
```c
// Basic comparison tests
TEST_ASSERT_EQUAL_INT(-1, comp_line("a", "b"));
TEST_ASSERT_EQUAL_INT(0, comp_line("test", "test"));
TEST_ASSERT_EQUAL_INT(1, comp_line("z", "a"));

// Case sensitivity tests (iflag variations)
// Length limitation tests (compnum variations)
// Boundary condition tests
```

##### **Task 1.2: `def_bonus()` Testing (misc.c)**
**Estimated Time**: 30-45 minutes
**Complexity**: Simple - Mathematical calculation function

**Implementation Steps**:
- [ ] Create mock sector structures with known designations
- [ ] Test each designation type (city, capitol, fortress, etc.)
- [ ] Test fortress level scaling
- [ ] Test magic bonus scenarios
- [ ] Verify calculation formulas match documentation
- [ ] Test boundary conditions and edge cases

##### **Task 1.3: `unitvalid()` Testing (magic.c)**
**Estimated Time**: 30-45 minutes
**Complexity**: Simple - Logic validation function

**Implementation Steps**:
- [ ] Analyze function signature and behavior
- [ ] Create test cases for valid/invalid units
- [ ] Test boundary conditions
- [ ] Verify validation logic matches documentation

#### **Level 1 Functions (Global State Setup) - 4 functions**

##### **Task 1.4: `no_god()` Testing (misc.c)**
**Estimated Time**: 30-45 minutes
**Complexity**: Simple - Global variable assignment

**Implementation Steps**:
- [ ] Create global variable setup/cleanup utilities
- [ ] Initialize `country`, `curntn`, `ntn[]` to known states
- [ ] Call `no_god()` and verify state changes
- [ ] Test from various initial states
- [ ] Verify pointer assignments and consistency

##### **Task 1.5: `flight_cost()` Testing (misc.c)**
**Estimated Time**: 45-60 minutes
**Complexity**: Simple - Table lookup with boundary testing

**Implementation Steps**:
- [ ] Initialize global cost tables (`ele`, `veg`, `FElecost`, `FVegcost`)
- [ ] Test valid coordinates with known terrain values
- [ ] Test invalid coordinates and boundary conditions
- [ ] Verify cost calculation accuracy
- [ ] Test various terrain/elevation combinations

##### **Task 1.6: `is_tech_good()` Testing (misc.c)**
**Estimated Time**: 45-60 minutes
**Complexity**: Simple - Threshold validation logic

**Implementation Steps**:
- [ ] Create mock nation data with known ability/wealth values
- [ ] Test technology thresholds for each trade good
- [ ] Test wealth requirements validation
- [ ] Test food viability requirements
- [ ] Verify threshold logic matches game balance

##### **Task 1.7: `soldier_type()` Testing (misc.c)**
**Estimated Time**: 45-60 minutes
**Complexity**: Simple - Priority logic with magic power evaluation

**Implementation Steps**:
- [ ] Create mock nations with specific power combinations
- [ ] Test each magic power priority scenario
- [ ] Test combination effects and hierarchy
- [ ] Test default fallback behavior
- [ ] Verify undead powers take absolute priority

**Session 1 Deliverables**:
- [ ] Test files for 7 functions integrated with CMake
- [ ] Global state management utilities created
- [ ] Basic mock data structure utilities
- [ ] All tests passing with CTest automation

---

### **SESSION 2: Level 2 Functions (2-3 hours)**
**Target**: 4 functions with simple function mocking
**Mock Complexity**: Basic function call mocking and error simulation

#### **Mock Infrastructure Development**

##### **Task 2.1: Function Mocking Utilities (45-60 minutes)**

**Implementation Steps**:
- [ ] Create function pointer substitution framework
- [ ] Implement mock `malloc()` with failure simulation
- [ ] Create mock `magic()` macro with controlled return values
- [ ] Implement mock `abrt()` function with test integration
- [ ] Create memory allocation tracking utilities

**Mock Utilities Required**:
```c
// Memory allocation mocking
extern int mock_malloc_fail_after;
extern int mock_malloc_call_count;
void* mock_malloc(size_t size);
void setup_malloc_failure(int fail_after_n_calls);

// Magic function mocking
extern int mock_magic_return_value;
#define magic(n, p) (mock_magic_return_value)

// Error function mocking
extern int mock_abrt_called;
void mock_abrt(void);
```

#### **Function Testing Implementation**

##### **Task 2.2: `build_node()` Testing (sort.c)**
**Estimated Time**: 45-60 minutes
**Complexity**: Moderate - Memory allocation patterns

**Implementation Steps**:
- [ ] Test successful node allocation and initialization
- [ ] Test malloc failure scenarios with mock
- [ ] Verify string copying and linkage
- [ ] Test with various string lengths
- [ ] Verify error handling on allocation failure

##### **Task 2.3: `num_powers()` Testing (misc.c)**
**Estimated Time**: 45-60 minutes
**Complexity**: Moderate - Function mocking with error conditions

**Implementation Steps**:
- [ ] Create mock nations with known power bitmasks
- [ ] Test count verification for each power category
- [ ] Test M_ALL comprehensive count scenario
- [ ] Test error handling with invalid type parameter
- [ ] Verify `abrt()` is called for invalid inputs

##### **Task 2.4: `tofood()` Testing (misc.c)**
**Estimated Time**: 60-75 minutes
**Complexity**: Moderate - Multiple bonus systems and race calculations

**Implementation Steps**:
- [ ] Create mock sectors with known vegetation/trade goods
- [ ] Test base vegetation value calculations
- [ ] Test elf racial bonuses and penalties
- [ ] Test trade good bonus calculations
- [ ] Verify race-specific adaptations to terrain

##### **Task 2.5: `tomagic()` Testing (misc.c)**
**Estimated Time**: 60-75 minutes
**Complexity**: Moderate - Exponential calculations with race conditions

**Implementation Steps**:
- [ ] Create mock nations with known race/power combinations
- [ ] Test race-specific base cost calculations
- [ ] Test exponential scaling with power levels
- [ ] Test overflow protection mechanisms
- [ ] Verify cost progression balances game economy

**Session 2 Deliverables**:
- [ ] Function mocking infrastructure operational
- [ ] 4 additional functions tested with mock dependencies
- [ ] Memory allocation testing patterns established
- [ ] Error condition simulation working

---

### **SESSION 3: Level 3 Functions (3-4 hours)**
**Target**: 4 functions with complex mocking
**Mock Complexity**: FILE I/O mocking and complex state management

#### **Advanced Mock Infrastructure**

##### **Task 3.1: FILE I/O Mocking Framework (60-90 minutes)**

**Implementation Steps**:
- [ ] Create mock FILE structure and operations
- [ ] Implement controlled character sequence simulation
- [ ] Create output capture and verification utilities
- [ ] Implement linked list testing infrastructure
- [ ] Create complex state management utilities

**Advanced Mock Utilities Required**:
```c
// FILE I/O mocking
typedef struct {
    char *buffer;
    int position;
    int length;
    int eof_flag;
} mock_file_t;

mock_file_t* create_mock_file(const char *content);
int mock_getc(FILE *stream);
int mock_feof(FILE *stream);
int mock_fprintf(FILE *stream, const char *format, ...);

// Linked list testing
typedef struct test_list_node {
    char *line;
    struct test_list_node *next;
} test_list_node_t;

test_list_node_t* create_test_list(char *lines[], int count);
void verify_list_order(test_list_node_t *head, char *expected[], int count);
```

#### **Complex Function Testing**

##### **Task 3.2: `get_a_line()` Testing (sort.c)**
**Estimated Time**: 60-75 minutes
**Complexity**: Complex - FILE I/O mocking with state management

**Implementation Steps**:
- [ ] Create mock input streams with controlled character sequences
- [ ] Test valid character filtering (printable ASCII, tabs, spaces)
- [ ] Test invalid character handling
- [ ] Test line length boundary conditions (`MAX_STR`)
- [ ] Test EOF conditions and stream state management

##### **Task 3.3: `print_list()` Testing (sort.c)**
**Estimated Time**: 45-60 minutes
**Complexity**: Complex - Combined data structure and I/O mocking

**Implementation Steps**:
- [ ] Create mock linked lists with known content
- [ ] Create mock output streams with capture capability
- [ ] Test empty list output
- [ ] Test single item and multiple item output
- [ ] Verify output formatting and I/O error handling

##### **Task 3.4: `place()` Testing (sort.c)**
**Estimated Time**: 75-90 minutes
**Complexity**: Complex - Insertion sort logic with linked list state

**Implementation Steps**:
- [ ] Test insertion into empty list
- [ ] Test insertion at beginning, middle, and end positions
- [ ] Test duplicate entry handling
- [ ] Verify sort order maintenance
- [ ] Test various insertion sequences for correctness

##### **Task 3.5: `score_one()` Testing (misc.c)**
**Estimated Time**: 75-90 minutes
**Complexity**: Complex - Multiple calculation systems and mock dependencies

**Implementation Steps**:
- [ ] Create comprehensive mock nation structures
- [ ] Test each nation class scoring calculation
- [ ] Test resource weighting accuracy
- [ ] Test magic power calculation integration
- [ ] Verify class-specific optimization strategies

**Session 3 Deliverables**:
- [ ] Advanced FILE I/O mocking infrastructure
- [ ] Linked list testing utilities
- [ ] 4 complex functions tested with full mock dependencies
- [ ] Complete Category A testing foundation established

---

## 🎯 OVERALL SUCCESS CRITERIA

### **Technical Requirements**
- [ ] **All 15 functions tested** with comprehensive coverage
- [ ] **>90% line coverage** achieved for tested functions
- [ ] **All tests pass** with both GCC and Clang compilers
- [ ] **CMake integration** for all test files
- [ ] **CTest automation** for continuous testing

### **Infrastructure Requirements**
- [ ] **Reusable mock utilities** for future function testing
- [ ] **Clear testing patterns** documented for Category B functions
- [ ] **Global state management** utilities operational
- [ ] **Error simulation** capabilities working
- [ ] **I/O mocking framework** ready for integration testing

### **Documentation Requirements**
- [ ] **Testing patterns documented** for each mock complexity level
- [ ] **Mock utility usage examples** provided
- [ ] **Integration with Category B** preparation documented
- [ ] **Performance benchmarks** for test execution time

## 📋 CRITICAL SUCCESS FACTORS

### **Session Management**
- **Time Boxing**: Stick to estimated times per function
- **Incremental Progress**: Complete easier functions first to build momentum
- **Mock Reusability**: Design utilities for multiple function reuse
- **Documentation**: Document patterns immediately for future reference

### **Quality Assurance**
- **Test Coverage**: Verify >90% coverage for each function
- **Cross-Platform**: Test with both GCC and Clang compilers
- **Error Handling**: Test both success and failure scenarios
- **Integration**: Ensure all tests integrate with existing test infrastructure

### **Risk Mitigation**
- **Mock Complexity**: Start simple and add complexity incrementally
- **Time Management**: Allow buffer time for complex mock development
- **Function Dependencies**: Understand all dependencies before starting tests
- **Infrastructure First**: Establish mock utilities before function testing

---

**Phase 6.5 Status**: Ready to Begin - Comprehensive implementation plan complete
**Infrastructure**: Unity framework operational, analysis complete
**Next Session**: Begin with Level 0-1 functions (7 functions, 2-3 hours)
**Total Estimated Duration**: 7-10 hours across 3 focused sessions