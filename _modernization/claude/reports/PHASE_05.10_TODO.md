# Phase 5.10: Complete Missing CMake Features

**Phase**: 5.10 - Missing CMake Feature Implementation
**Date Created**: 2025-09-28
**Priority**: High - Critical gaps that should have been completed in Phase 5

## Overview

During Phase 5 CMake implementation, several critical features from the original Makefile were not implemented. These gaps were discovered during Phase 5 review and must be addressed to achieve full Makefile-to-CMake conversion parity.

**Critical Issue**: Help file generation system is completely missing - this is core game functionality that players need to understand how to play the game.

## Excluded Features (DO NOT IMPLEMENT)

- **make lint target** - Not implementing due to modern tooling preferences
- **setuid installation features** - Code is NOT built for modern setuid safety

## Missing Features Todo List

### 🎯 Priority 1: Core Game Functionality

#### Help File Generation System
- [ ] **Implement help file generation system in CMake**
  - Status: Not Started
  - Description: Complete system missing from CMake
  - Dependencies: newhelp utility, sed processing

- [ ] **Create newhelp utility build target in CMake**
  - Status: Not Started
  - Description: Build newhelp from dataG.o and newhelp.o
  - Notes: Required for sed script generation

- [ ] **Add sed script generation and processing for help files**
  - Status: Not Started
  - Description: Generate sed.1 and sed.2 scripts via newhelp
  - Dependencies: newhelp utility

- [ ] **Implement help0-help5 generation from txt0-txt5 sources**
  - Status: Not Started
  - Description: Process txt files through sed scripts to create help files
  - Dependencies: sed scripts, source txt files

### 🎯 Priority 2: Installation and Setup

#### Game World Setup
- [ ] **Add new_game target with automatic world creation**
  - Status: Not Started
  - Description: Replicate Makefile's automatic world setup
  - Features: Run $(EXEDIR)/$(TARGET_ADMIN) -m and -a

- [ ] **Implement automatic copying of game data files (nations, rules)**
  - Status: Not Started
  - Description: Copy essential game files during new_game
  - Files: nations, rules to data directory

- [ ] **Add special file permissions setup for game files**
  - Status: Not Started
  - Description: Set proper permissions (excluding setuid)
  - Notes: 0600 for nations, 0700 for run script, etc.

- [ ] **Update installation targets to match Makefile functionality**
  - Status: Not Started
  - Description: Ensure full installation parity
  - Exclude: setuid permissions (security reasons)

### 🎯 Priority 3: Development Targets ✅ COMPLETE

#### Build Verification and Information
- [x] **Add check target for basic executable verification**
  - Status: ✅ COMPLETE
  - Description: Verify all executables built correctly
  - Features: Test executable existence and basic functionality
  - Implementation: Tests conquer, conqrun, conqsort executables

- [x] **Add config target to display build configuration**
  - Status: ✅ COMPLETE
  - Description: Show platform, compiler, flags, etc.
  - Features: Match Makefile's config output format
  - Implementation: Shows comprehensive build configuration details

- [x] **Implement docs/documentation generation target**
  - Status: ✅ COMPLETE
  - Description: Generate conquer.doc from help files
  - Process: Concatenate help files with sed processing
  - Implementation: Creates 72KB conquer.doc from help0-help5

### 🎯 Priority 4: Archive and Maintenance

#### Archive Generation
- [ ] **Add cpio archive generation target**
  - Status: Not Started
  - Description: Create cpio archives for distribution
  - Features: Find and archive relevant files

- [ ] **Add shar archive generation target with manifest**
  - Status: Not Started
  - Description: Create shell archives with file manifest
  - Features: Generate MANIFEST with wc output

#### Cleanup Enhancement
- [ ] **Implement clobber target for comprehensive cleanup**
  - Status: Not Started
  - Description: More thorough cleanup than standard clean
  - Features: Remove all generated files, help files, utilities

### 🎯 Priority 5: Build System Improvements

#### Build Configuration
- [ ] **Add build type selection support (debug/release)**
  - Status: Not Started
  - Description: Replicate BUILD_TYPE=debug|release functionality
  - Features: Different optimization and debug flags

- [ ] **Enhance platform-specific library detection logic**
  - Status: Not Started
  - Description: Improve library detection for different platforms
  - Features: Better Linux/macOS/BSD library handling

## Implementation Strategy

### Session Planning
1. **Session 1**: Help file generation system (Priority 1) ✅ COMPLETE
2. **Session 2**: new_game target and installation features (Priority 2) ✅ COMPLETE
3. **Session 3**: Development targets (Priority 3) ✅ COMPLETE
4. **Session 4**: Archive and maintenance targets (Priority 4) 🎯 NEXT
5. **Session 5**: Build system improvements and final verification (Priority 5)

### Success Criteria
- [ ] CMake system has full feature parity with original Makefile
- [ ] Help files generate correctly and match original output
- [ ] new_game target creates playable game world
- [ ] All development targets work as expected
- [ ] Archive targets produce valid distributions
- [ ] Build system handles all original Makefile configurations

### Verification Requirements
- [ ] Compare CMake and Makefile build outputs
- [ ] Test help file generation matches original
- [ ] Verify new_game creates functional world
- [ ] Test all new targets function correctly
- [ ] Confirm no regressions in existing functionality

## Notes

**Timeline**: This represents approximately 5 focused sessions of work to complete all missing features.

**Risk**: Without help file generation, the game is not fully functional for end users.

**Dependencies**: Most features are independent, but help system has internal dependencies (newhelp → sed scripts → help files).

---

**Created**: 2025-09-28
**Phase**: 5.10 Missing CMake Features
**Next Session**: Start with Priority 1 (Help File Generation System)