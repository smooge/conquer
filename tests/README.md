# Testing Infrastructure for Conquer 4.x Modernization

This directory contains the comprehensive testing infrastructure for the Conquer 4.x modernization project, built using the Unity C testing framework.

## Overview

The testing infrastructure supports the modernization process by providing:
- **Regression Testing**: Ensure modernization doesn't break existing functionality
- **Unit Testing**: Test individual functions in isolation
- **Integration Testing**: Test interactions between modules
- **Cross-Platform Validation**: Verify POSIX compliance across target platforms

## Directory Structure

- **`framework/`** - Unity testing framework (3 files: unity.c, unity.h, unity_internals.h)
- **`unit/`** - Unit tests for individual functions and modules
- **`integration/`** - Integration tests for component interactions
- **`regression/`** - Regression tests to ensure modernization preserves functionality
- **`utils/`** - Testing utility programs and compilation wrappers
- **`fixtures/`** - Test data files and mock inputs
- **`scripts/`** - Test runner scripts and automation utilities

## Unity Testing Framework (Phase 6.1)

**Version**: Latest from ThrowTheSwitch/Unity repository
**Integration Method**: Direct file inclusion (not git submodule)
**Rationale**: Simplicity, self-contained, easy to switch to submodule later

### Running Tests

```bash
# Using CMake targets (recommended)
make run_all_tests
make run_unit_tests
make run_integration_tests
make run_regression_tests
make test_info

# Using CTest directly
ctest --output-on-failure
ctest -L unit --output-on-failure

# Running individual tests
./tests/unit/test_example_unit
```

## Utility Programs

### `utils/test_data_h.c`
Compilation wrapper for testing data.h changes. Includes proper header.h dependency.

**Usage:**
```bash
# Test data.h compilation health
gcc -O2 -g -std=c99 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE \
  -DDEFAULTDIR="/home/ssmoogen/conquer/lib" -DEXEDIR="/home/ssmoogen/conquer/bin" \
  -DPATCHLEVEL="12" -DLOGIN="ssmoogen" -DCONQUER \
  -c tests/utils/test_data_h.c -o /tmp/test_data_h.o
```

## File Naming Conventions

- **Unit tests**: `test_<module_name>.c` (e.g., `test_string_utils.c`)
- **Integration tests**: `test_integration_<feature>.c`
- **Regression tests**: `test_regression_<issue_id>.c`
- **Utility programs**: `test_<purpose>.c` or descriptive names

## Git Management

- **Source files**: All `.c` and `.h` files are tracked in git
- **Executables**: Test executables are ignored via .gitignore
- **Output files**: Test output files (*.out) are ignored
