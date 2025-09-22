# Testing Directory Structure

This directory contains all testing infrastructure for the conquer-4.x modernization project.

## Directory Structure

- **`unit/`** - Unit tests for individual functions and modules
- **`integration/`** - Integration tests for component interactions
- **`regression/`** - Regression tests to ensure modernization preserves functionality
- **`utils/`** - Testing utility programs and compilation wrappers
- **`fixtures/`** - Test data files and mock inputs
- **`scripts/`** - Test runner scripts and automation utilities

## Utility Programs

### `utils/test_data_h.c`
Compilation wrapper for testing data.h changes. Includes proper header.h dependency.

**Usage:**
```bash
# Test data.h compilation health
gcc -O2 -g -std=c99 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE \
  -DDEFAULTDIR="/home/ssmoogan/conquer/lib" -DEXEDIR="/home/ssmoogan/conquer/bin" \
  -DVERSION="4" -DPATCHLEVEL="12" -DLOGIN="ssmoogan" -DCONQUER \
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