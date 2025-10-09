cmake_minimum_required(VERSION 3.15...3.28)

# Parse version from patchlevel.h - Single Source of Truth
file(READ "patchlevel.h" PATCHLEVEL_CONTENT)
string(REGEX MATCH "#define PATCHLEVEL[ \t]+\"([0-9]+)\"" _ ${PATCHLEVEL_CONTENT})
if(NOT CMAKE_MATCH_1)
    message(FATAL_ERROR "Could not parse PATCHLEVEL from patchlevel.h")
endif()
set(CONQUER_PATCHLEVEL ${CMAKE_MATCH_1})

# Project definition with version from patchlevel.h
project(Conquer
    VERSION 4.${CONQUER_PATCHLEVEL}
    DESCRIPTION "Classic strategy game - modernized"
    LANGUAGES C
)

# Set C standard to C2x (C23) as required by Phase 4
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# POSIX compliance as required by project standards
add_compile_definitions(_POSIX_C_SOURCE=200809L)

# Enable CMake build system detection so config.h gets included properly
add_compile_definitions(CONQUER_CMAKE_BUILD=1)

# Add our CMake modules directory
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Phase 4 warning preservation - CRITICAL REQUIREMENT
include(CompilerWarnings)

# Phase 5.2: Dual compilation system implementation
include(DualCompilation)

# Phase 8.4: Static analysis tools integration
include(StaticAnalysis)

# Code formatting tools integration
include(CodeFormatting)

# Basic compiler and build type setup
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Project installation configuration - Unix standard directory layout
# Default installation prefix to game directory instead of system-wide /usr/local
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/projects/conquer" CACHE PATH "Installation prefix" FORCE)
endif()

# Standard Unix directory layout derived from prefix
set(CONQUER_EXE_DIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH "Executable installation directory")
set(CONQUER_DEFAULT_DIR "${CMAKE_INSTALL_PREFIX}/share/conquer" CACHE PATH "Default game data directory")
set(CONQUER_MAN_DIR "${CMAKE_INSTALL_PREFIX}/share/man" CACHE PATH "Manual page directory")

# User login for build-time embedding
execute_process(
    COMMAND whoami
    OUTPUT_VARIABLE CONQUER_LOGIN
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Validate LOGIN detection for config.h generation
if(NOT CONQUER_LOGIN)
    message(WARNING "LOGIN detection failed - using fallback")
    set(CONQUER_LOGIN "unknown")
endif()

# Set LOGIN as a global cache variable to ensure it persists
set(CONQUER_LOGIN "${CONQUER_LOGIN}" CACHE STRING "Build user login name" FORCE)
message(STATUS "LOGIN for config.h: CONQUER_LOGIN='${CONQUER_LOGIN}'")

# Build timestamp for configuration
string(TIMESTAMP CONQUER_BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)

# Generate configuration header from template
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
    @ONLY
)

# Make generated config.h available to all targets
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# Basic build information
message(STATUS "Conquer ${PROJECT_VERSION} - Modern CMake Build System")
message(STATUS "Patchlevel: ${CONQUER_PATCHLEVEL} (from patchlevel.h)")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C standard: C${CMAKE_C_STANDARD}")
message(STATUS "Phase 4 warning preservation: ENABLED")
message(STATUS "Config header: ${CMAKE_CURRENT_BINARY_DIR}/config.h")

# Dual compilation system creates conquer, conqrun, and conqsort executables
# All configuration is handled by the DualCompilation module

#
# TESTING INFRASTRUCTURE - Phase 6.1 Unity Integration
#
option(ENABLE_TESTING "Enable testing infrastructure" ON)

if(ENABLE_TESTING)
    enable_testing()
    message(STATUS "Testing infrastructure: ENABLED")
    add_subdirectory(tests)

    # Phase 6.2: Configure cross-compiler test scripts
    message(STATUS "Configuring Phase 6.2 cross-compiler test scripts")

    # Create build/tests/scripts directory
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/tests/scripts")

    # Create build/reports directory for test output
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/reports")

    # Set CMake variables for script configuration
    set(DEFAULT_DEFAULTDIR "${CONQUER_DEFAULT_DIR}")
    set(DEFAULT_EXEDIR "${CONQUER_EXE_DIR}")
    set(DEFAULT_SHARDIR "${CMAKE_INSTALL_PREFIX}/share")
    set(DEFAULT_LOGIN "${CONQUER_LOGIN}")

    # Configure test script templates with CMake variables
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/scripts/test_gcc.sh.in"
        "${CMAKE_BINARY_DIR}/tests/scripts/test_gcc.sh"
        @ONLY
    )

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/scripts/test_clang.sh.in"
        "${CMAKE_BINARY_DIR}/tests/scripts/test_clang.sh"
        @ONLY
    )

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/scripts/run_all_compiler_tests.sh.in"
        "${CMAKE_BINARY_DIR}/tests/scripts/run_all_compiler_tests.sh"
        @ONLY
    )

    # Make test scripts executable
    file(CHMOD "${CMAKE_BINARY_DIR}/tests/scripts/test_gcc.sh"
         PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                     GROUP_READ GROUP_EXECUTE
                     WORLD_READ WORLD_EXECUTE)

    file(CHMOD "${CMAKE_BINARY_DIR}/tests/scripts/test_clang.sh"
         PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                     GROUP_READ GROUP_EXECUTE
                     WORLD_READ WORLD_EXECUTE)

    file(CHMOD "${CMAKE_BINARY_DIR}/tests/scripts/run_all_compiler_tests.sh"
         PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                     GROUP_READ GROUP_EXECUTE
                     WORLD_READ WORLD_EXECUTE)

    message(STATUS "Cross-compiler test scripts configured in build/tests/scripts/")
    message(STATUS "Test reports will be saved to build/reports/")
else()
    message(STATUS "Testing infrastructure: DISABLED")
endif()

#
# VERIFICATION TARGETS - Phase 4 Warning Compliance Validation
#

# Display active warning flags for verification
add_custom_target(verify_phase4_warnings
    COMMAND ${CMAKE_COMMAND} -E echo "=== Phase 4 Warning Flag Verification ==="
    COMMAND ${CMAKE_C_COMPILER} -Q --help=warning | grep -E "(Wall|Wextra|Wpedantic|Wformat|Wconversion|Wsign-conversion|Wimplicit-fallthrough|Wstrict-prototypes|Wold-style|Wshadow|Wmissing-prototypes|Wcast-qual|fanalyzer)" || true
    COMMAND ${CMAKE_COMMAND} -E echo "=== End Warning Flag Verification ==="
    COMMENT "Displaying active Phase 4 warning flags for verification"
    VERBATIM
)

# Test for warning regressions in dual compilation
add_custom_target(test_warning_regression
    COMMAND ${CMAKE_COMMAND} -E echo "=== Testing for Warning Regressions ==="
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target conquer --verbose 2>&1 | grep -i warning || ${CMAKE_COMMAND} -E echo "SUCCESS: No warnings in game executable"
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target conqrun --verbose 2>&1 | grep -i warning || ${CMAKE_COMMAND} -E echo "SUCCESS: No warnings in admin executable"
    COMMAND ${CMAKE_COMMAND} -E echo "=== Warning Regression Test Complete ==="
    COMMENT "Testing for any new warnings introduced since Phase 4"
    VERBATIM
)

# Comprehensive dual compilation test with verbose output
add_custom_target(verify_phase4_compliance
    COMMAND ${CMAKE_COMMAND} -E echo "=== Phase 4 Compliance Verification ==="
    COMMAND ${CMAKE_COMMAND} -E echo "Compiler: ${CMAKE_C_COMPILER}"
    COMMAND ${CMAKE_COMMAND} -E echo "C Standard: C${CMAKE_C_STANDARD}"
    COMMAND ${CMAKE_COMMAND} -E echo "Build Type: ${CMAKE_BUILD_TYPE}"
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target conquer --verbose
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target conqrun --verbose
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target conqsort --verbose
    COMMAND ${CMAKE_COMMAND} -E echo "=== Phase 4 Compliance Test COMPLETE ==="
    COMMENT "Comprehensive Phase 4 warning compliance verification"
    VERBATIM
)

#
# INSTALLATION CONFIGURATION - Complete Makefile Conversion
#

# Calculate relative installation paths from configured directories
file(RELATIVE_PATH CONQUER_EXE_RELPATH "${CMAKE_INSTALL_PREFIX}" "${CONQUER_EXE_DIR}")
file(RELATIVE_PATH CONQUER_DATA_RELPATH "${CMAKE_INSTALL_PREFIX}" "${CONQUER_DEFAULT_DIR}")
file(RELATIVE_PATH CONQUER_MAN_RELPATH "${CMAKE_INSTALL_PREFIX}" "${CONQUER_MAN_DIR}")

# Install all executables to configured executable directory
# Note: CMake doesn't support setuid bits (4751), those must be set manually after installation
install(TARGETS conquer conqrun conqsort conqps
    RUNTIME DESTINATION "${CONQUER_EXE_RELPATH}"
    COMPONENT Executables
)

# Install PostScript data file to data directory
install(FILES psmap.ps
    DESTINATION "${CONQUER_DATA_RELPATH}"
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    COMPONENT Data
)

# Install manual page to proper man directory as conquer.6
install(FILES man.pag
    DESTINATION "${CONQUER_MAN_RELPATH}/man6"
    RENAME conquer.6
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    COMPONENT Documentation
)

# =============================================================================
# HELP FILE GENERATION SYSTEM
# =============================================================================
# Replicates the original Makefile's helpfile target functionality
# Process: newhelp → sed.1, sed.2 → txt0-txt5 processed to help0-help5

# Build newhelp utility (required for sed script generation)
# newhelp needs data.c compiled with -DCONQUER (equivalent to dataG.o in Makefile)
add_library(newhelp_data_obj OBJECT data.c)
target_compile_definitions(newhelp_data_obj PRIVATE CONQUER)
target_link_libraries(newhelp_data_obj PRIVATE conquer_warnings)

add_executable(newhelp newhelp.c $<TARGET_OBJECTS:newhelp_data_obj>)
target_link_libraries(newhelp PRIVATE conquer_warnings ${MATH_LIBRARY})

# Custom target to generate sed scripts using newhelp
add_custom_target(generate_sed_scripts
    COMMAND newhelp
    DEPENDS newhelp
    COMMENT "Generating sed.1 and sed.2 scripts for help file processing"
    BYPRODUCTS sed.1 sed.2
)

# Function to create help file generation rules
function(add_help_file_generation help_num)
    add_custom_command(
        OUTPUT help${help_num}
        COMMAND ${CMAKE_COMMAND} -E echo "Building help${help_num}..."
        COMMAND cat ${CMAKE_CURRENT_SOURCE_DIR}/txt${help_num} | sed -f sed.1 | sed -f sed.2 > help${help_num}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/txt${help_num} generate_sed_scripts
        COMMENT "Processing txt${help_num} to generate help${help_num}"
        VERBATIM
    )
endfunction()

# Generate all help files (help0 through help5)
foreach(help_num RANGE 0 5)
    add_help_file_generation(${help_num})
endforeach()

# Main helpfile target that builds all help files
add_custom_target(helpfile
    DEPENDS help0 help1 help2 help3 help4 help5
    COMMENT "✓ Help files built"
)

# Add helpfile as a dependency for the main targets since game needs help files
add_dependencies(conquer helpfile)
add_dependencies(conqrun helpfile)

# Install help files with proper permissions
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/help0
    ${CMAKE_CURRENT_BINARY_DIR}/help1
    ${CMAKE_CURRENT_BINARY_DIR}/help2
    ${CMAKE_CURRENT_BINARY_DIR}/help3
    ${CMAKE_CURRENT_BINARY_DIR}/help4
    ${CMAKE_CURRENT_BINARY_DIR}/help5
    DESTINATION ${CONQUER_DEFAULT_DIR}
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    COMPONENT Data
)

# =============================================================================
# NEW GAME WORLD SETUP TARGET
# =============================================================================
# Replicates the original Makefile's new_game target functionality
# Process: Create directories → Copy executables → Set permissions → Copy data → Initialize world

# Custom target for setting up a new game world
add_custom_target(new_game
    DEPENDS conquer conqrun conqsort conqps helpfile
    COMMENT "Setting up new game world..."
    VERBATIM
)

# Create required directories
add_custom_command(TARGET new_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CONQUER_EXE_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CONQUER_DEFAULT_DIR}
    COMMENT "Creating game directories..."
    VERBATIM
)

# Copy executables to target directory
add_custom_command(TARGET new_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:conquer> ${CONQUER_EXE_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:conqrun> ${CONQUER_EXE_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:conqsort> ${CONQUER_EXE_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:conqps> ${CONQUER_EXE_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/psmap.ps ${CONQUER_DEFAULT_DIR}/
    COMMENT "Copying executables to game directory..."
    VERBATIM
)

# Copy help files to data directory
add_custom_command(TARGET new_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/help0 ${CONQUER_DEFAULT_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/help1 ${CONQUER_DEFAULT_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/help2 ${CONQUER_DEFAULT_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/help3 ${CONQUER_DEFAULT_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/help4 ${CONQUER_DEFAULT_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/help5 ${CONQUER_DEFAULT_DIR}/
    COMMENT "Copying help files to data directory..."
    VERBATIM
)

# Copy game data files (nations, rules) to data directory
add_custom_command(TARGET new_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/nations ${CONQUER_DEFAULT_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/rules ${CONQUER_DEFAULT_DIR}/
    COMMENT "Copying game data files..."
    VERBATIM
)

# Set file permissions (excluding setuid for security - documented in todo exclusions)
add_custom_command(TARGET new_game POST_BUILD
    COMMAND chmod 755 ${CONQUER_EXE_DIR}
    COMMAND chmod 750 ${CONQUER_DEFAULT_DIR}
    COMMAND chmod 0755 ${CONQUER_EXE_DIR}/conquer ${CONQUER_EXE_DIR}/conqrun
    COMMAND chmod 0755 ${CONQUER_EXE_DIR}/conqsort ${CONQUER_EXE_DIR}/conqps
    COMMAND chmod 0644 ${CONQUER_DEFAULT_DIR}/psmap.ps
    COMMAND chmod 0600 ${CMAKE_CURRENT_SOURCE_DIR}/nations
    COMMAND chmod 0700 ${CMAKE_CURRENT_SOURCE_DIR}/run
    COMMENT "Setting file permissions..."
    VERBATIM
)

# Display world creation instructions (interactive commands require manual execution)
add_custom_command(TARGET new_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "✓ Game setup complete!"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "To complete world creation, run these commands manually:"
    COMMAND ${CMAKE_COMMAND} -E echo "  ${CONQUER_EXE_DIR}/conqrun -m    # Create world (interactive - sets super-user password)"
    COMMAND ${CMAKE_COMMAND} -E echo "  ${CONQUER_EXE_DIR}/conqrun -a    # Add players (interactive)"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Alternative: Use 'expect' script for automation (see scripts/)"
    COMMENT "Game setup complete - manual world creation required"
    VERBATIM
)

# Installation summary message
install(CODE "message(STATUS \"Installation complete to prefix-based paths:\")")
install(CODE "message(STATUS \"  Install Prefix: ${CMAKE_INSTALL_PREFIX}\")")
install(CODE "message(STATUS \"  Executables: ${CONQUER_EXE_DIR}\")")
install(CODE "message(STATUS \"  Data directory: ${CONQUER_DEFAULT_DIR}\")")
install(CODE "message(STATUS \"  Manual pages: ${CONQUER_MAN_DIR}\")")
install(CODE "message(STATUS \"Note: Set setuid permissions manually if needed:\")")
install(CODE "message(STATUS \"  chmod 4755 ${CONQUER_EXE_DIR}/conquer\")")
install(CODE "message(STATUS \"  chmod 4755 ${CONQUER_EXE_DIR}/conqrun\")")
install(CODE "message(STATUS \"To use man page: MANPATH=${CONQUER_MAN_DIR} man conquer\")")

# =============================================================================
# Development Targets
# =============================================================================

# check - Basic executable verification
add_custom_target(check
    COMMAND ${CMAKE_COMMAND} -E echo "Running basic tests..."
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMENT "Verifying build executables"
)

# Check that main executables exist and are executable
add_custom_command(TARGET check POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Checking game executable..."
    COMMAND test -x $<TARGET_FILE:conquer>
    COMMAND ${CMAKE_COMMAND} -E echo "✓ Game executable OK"
    COMMAND ${CMAKE_COMMAND} -E echo "Checking admin executable..."
    COMMAND test -x $<TARGET_FILE:conqrun>
    COMMAND ${CMAKE_COMMAND} -E echo "✓ Admin executable OK"
    COMMAND ${CMAKE_COMMAND} -E echo "Checking sort executable..."
    COMMAND test -x $<TARGET_FILE:conqsort>
    COMMAND ${CMAKE_COMMAND} -E echo "✓ Sort executable OK"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "✓ Build verification complete"
    COMMENT "Build verification complete"
    VERBATIM
)

# check depends on all main executables
add_dependencies(check conquer conqrun conqsort)

# config - Display build configuration
add_custom_target(config
    COMMAND ${CMAKE_COMMAND} -E echo "Build Configuration:"
    COMMAND ${CMAKE_COMMAND} -E echo "  Platform: ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_PROCESSOR})"
    COMMAND ${CMAKE_COMMAND} -E echo "  Generator: ${CMAKE_GENERATOR}"
    COMMAND ${CMAKE_COMMAND} -E echo "  C Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}"
    COMMAND ${CMAKE_COMMAND} -E echo "  C Compiler Path: ${CMAKE_C_COMPILER}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Build Type: ${CMAKE_BUILD_TYPE}"
    COMMAND ${CMAKE_COMMAND} -E echo "  C Standard: ${CMAKE_C_STANDARD}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Install Prefix: ${CMAKE_INSTALL_PREFIX}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Executable Directory: ${CONQUER_EXE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Data Directory: ${CONQUER_DEFAULT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Manual Directory: ${CONQUER_MAN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "  C Flags: ${CMAKE_C_FLAGS}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Source Directory: ${CMAKE_SOURCE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Binary Directory: ${CMAKE_BINARY_DIR}"
    COMMENT "Displaying build configuration"
    VERBATIM
)

# docs - Generate documentation from help files
add_custom_target(docs
    COMMAND ${CMAKE_COMMAND} -E echo "Creating documentation..."
    DEPENDS help0 help1 help2 help3 help4 help5
    COMMENT "Generating conquer.doc from help files"
)

# Generate conquer.doc by concatenating help files and removing DONE/END markers
add_custom_command(TARGET docs POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Concatenating help files to conquer.doc..."
    COMMAND cat help0 help1 help2 help3 help4 help5 | sed -e "s/^DONE//g" | sed -e "s/^END//g" > conquer.doc
    COMMAND ${CMAKE_COMMAND} -E echo "✓ Documentation generated: conquer.doc"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Documentation generation complete"
    VERBATIM
)

# =============================================================================
# Archive and Maintenance Targets (Priority 4)
# =============================================================================

# Define file sets for archive creation
set(SOURCE_FILES
    admin.c cexecute.c check.c combat.c commands.c data.c display.c
    extcmds.c forms.c io.c m2alloc.c magic.c main.c makeworl.c misc.c move.c
    navy.c newhelp.c newlogin.c npc.c psmap.c randeven.c reports.c
    safe_system.c sort.c spew.c trade.c update.c
)

set(HEADER_FILES
    data.h header.h newlogin.h patchlevel.h psmap.h safe_convert.h safe_system.h trade.h
)

set(SUPPORT_FILES
    nations Makefile README.md run man.pag rules execute messages
    news commerce CONQPS.INFO MANIFEST
)

# Help files - check dynamically if they exist
set(HELP_FILES)
foreach(i RANGE 0 5)
    if(EXISTS ${CMAKE_SOURCE_DIR}/help${i})
        list(APPEND HELP_FILES help${i})
    endif()
endforeach()

set(ALL_ARCHIVE_FILES ${SOURCE_FILES} ${HEADER_FILES} ${SUPPORT_FILES} ${HELP_FILES})

# cpio - Create cpio archive of source files
add_custom_target(cpio
    COMMAND ${CMAKE_COMMAND} -E echo "Creating cpio archive..."
    COMMAND ${CMAKE_COMMAND} -E remove -f core cpiosv
    COMMAND find . -name '*[CrpsEech]' -print | cpio -ocBv > cpiosv
    COMMAND ${CMAKE_COMMAND} -E echo "✓ CPIO archive created: cpiosv"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Creating CPIO archive of source files"
    VERBATIM
)

# tar - Create tar.gz archive for distribution
add_custom_target(tar
    COMMAND ${CMAKE_COMMAND} -E echo "Creating tar archive..."
    COMMAND ${CMAKE_COMMAND} -E echo " lines   words chars   FILENAME" > MANIFEST
    COMMAND wc ${SOURCE_FILES} ${HEADER_FILES} >> MANIFEST 2>/dev/null || echo "Generating file manifest..."
    COMMAND tar -czf conquer-4.x-source.tar.gz ${SOURCE_FILES} ${HEADER_FILES} ${HELP_FILES} MANIFEST
    COMMAND ${CMAKE_COMMAND} -E echo "✓ Tar archive created: conquer-4.x-source.tar.gz"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Creating tar.gz archive for distribution"
    VERBATIM
)

# zip - Create zip archive for distribution
add_custom_target(zip
    COMMAND ${CMAKE_COMMAND} -E echo "Creating zip archive..."
    COMMAND ${CMAKE_COMMAND} -E echo " lines   words chars   FILENAME" > MANIFEST
    COMMAND wc ${SOURCE_FILES} ${HEADER_FILES} >> MANIFEST 2>/dev/null || echo "Generating file manifest..."
    COMMAND zip -9 conquer-4.x-source.zip ${SOURCE_FILES} ${HEADER_FILES} ${HELP_FILES} MANIFEST
    COMMAND ${CMAKE_COMMAND} -E echo "✓ Zip archive created: conquer-4.x-source.zip"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Creating zip archive for distribution"
    VERBATIM
)

# clobber - Clean all generated files (beyond standard clean)
add_custom_target(clobber
    COMMAND ${CMAKE_COMMAND} -E echo "Removing all generated files..."
    # Remove standard build artifacts first
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    # Remove generated help files
    COMMAND ${CMAKE_COMMAND} -E remove -f help0 help1 help2 help3 help4 help5
    # Remove generated executables and utilities
    COMMAND ${CMAKE_COMMAND} -E remove -f conquer conqrun conqsort psmap newhelp
    # Remove generated data files
    COMMAND ${CMAKE_COMMAND} -E remove -f insthelp helpfile newhelp
    # Remove admin/game input files
    COMMAND ${CMAKE_COMMAND} -E remove -f inconquer inconqrun inconqsort inpsmap
    # Remove temporary and working files
    COMMAND ${CMAKE_COMMAND} -E remove -f helpscr.1 helpscr.2
    # Remove documentation
    COMMAND ${CMAKE_COMMAND} -E remove -f conquer.doc
    # Remove archives
    COMMAND ${CMAKE_COMMAND} -E remove -f cpiosv conquer-4.x-source.tar.gz conquer-4.x-source.zip
    # Remove core files
    COMMAND ${CMAKE_COMMAND} -E remove -f core
    COMMAND ${CMAKE_COMMAND} -E echo "✓ All generated files removed"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Comprehensive cleanup of all generated files"
    VERBATIM
)

# =============================================================================
# Priority 5: Build System Improvements (Final CMake Modernization)
# =============================================================================

# Enhanced build type selection support (debug/release)
# Replicate the original Makefile's BUILD_TYPE=debug|release functionality
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Debug build configuration
    set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG -Wall -Wextra -Wpedantic")
    message(STATUS "Debug build enabled: full debugging symbols, no optimization")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Release build configuration
    set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG -Wall -Wextra -Wpedantic")
    message(STATUS "Release build enabled: optimized, no debug symbols")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    # Release with debug info
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG -Wall -Wextra -Wpedantic")
    message(STATUS "Release with debug info: optimized with debug symbols")
else()
    # Default to Release if not specified
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    message(STATUS "Build type not specified, defaulting to Release")
endif()

# build_type - Display and allow changing build configuration
add_custom_target(build_type
    COMMAND ${CMAKE_COMMAND} -E echo "Current Build Configuration:"
    COMMAND ${CMAKE_COMMAND} -E echo "  Build Type: ${CMAKE_BUILD_TYPE}"
    COMMAND ${CMAKE_COMMAND} -E echo "  C Flags: ${CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE}}"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Available Build Types:"
    COMMAND ${CMAKE_COMMAND} -E echo "  Debug     - Full debugging, no optimization (-g -O0 -DDEBUG)"
    COMMAND ${CMAKE_COMMAND} -E echo "  Release   - Optimized, no debug symbols (-O2 -DNDEBUG)"
    COMMAND ${CMAKE_COMMAND} -E echo "  RelWithDebInfo - Optimized with debug symbols (-O2 -g -DNDEBUG)"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "To change: cmake -DCMAKE_BUILD_TYPE=Debug|Release|RelWithDebInfo ."
    COMMENT "Displaying build type configuration"
    VERBATIM
)

# Enhanced platform-specific library detection and configuration
# Detect platform capabilities and optimize accordingly
message(STATUS "=== Platform-Specific Configuration ===")

# Platform identification
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Platform: Linux - Enabling Linux-specific optimizations")
    # Linux-specific configurations
    add_compile_definitions(LINUX_PLATFORM=1)
    # Check for Linux-specific libraries
    find_library(DL_LIB dl)
    if(DL_LIB)
        message(STATUS "Dynamic linking library found: ${DL_LIB}")
    endif()

elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    message(STATUS "Platform: FreeBSD - Enabling BSD-specific optimizations")
    # FreeBSD-specific configurations
    add_compile_definitions(BSD_PLATFORM=1)
    # FreeBSD may need different library search paths

elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(STATUS "Platform: macOS - Enabling macOS-specific optimizations")
    # macOS-specific configurations
    add_compile_definitions(MACOS_PLATFORM=1)
    # macOS uses different library conventions

else()
    message(STATUS "Platform: ${CMAKE_SYSTEM_NAME} - Using generic Unix configuration")
    add_compile_definitions(UNIX_PLATFORM=1)
endif()

# Enhanced library detection with fallbacks and alternatives
message(STATUS "=== Enhanced Library Detection ===")

# Improved ncurses detection with multiple fallback options
unset(NCURSES_FOUND)
unset(NCURSES_LIBRARIES)

# Try pkg-config first (most reliable)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(NCURSES QUIET ncurses)
    if(NCURSES_FOUND)
        message(STATUS "Ncurses found via pkg-config: ${NCURSES_LIBRARIES}")
    endif()
endif()

# Fall back to direct library search with multiple name variants
if(NOT NCURSES_FOUND)
    find_library(NCURSES_LIB NAMES ncurses ncurses5 ncurses6 curses)
    if(NCURSES_LIB)
        set(NCURSES_LIBRARIES ${NCURSES_LIB})
        set(NCURSES_FOUND TRUE)
        message(STATUS "Ncurses found via library search: ${NCURSES_LIB}")
    endif()
endif()

# Platform-specific ncurses search
if(NOT NCURSES_FOUND)
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        find_library(NCURSES_LIB NAMES ncurses HINTS /usr/local/lib /opt/homebrew/lib)
        if(NCURSES_LIB)
            set(NCURSES_LIBRARIES ${NCURSES_LIB})
            set(NCURSES_FOUND TRUE)
            message(STATUS "Ncurses found on macOS: ${NCURSES_LIB}")
        endif()
    endif()
endif()

if(NOT NCURSES_FOUND)
    message(WARNING "Ncurses library not found - executables may not link properly")
    message(WARNING "Consider installing: libncurses-dev (Debian/Ubuntu) or ncurses-devel (Fedora/RHEL)")
endif()

# Enhanced crypt library detection
unset(CRYPT_LIB)
find_library(CRYPT_LIB NAMES crypt)
if(CRYPT_LIB)
    message(STATUS "Crypt library found: ${CRYPT_LIB}")
else()
    # Some systems have crypt in libc
    include(CheckSymbolExists)
    check_symbol_exists(crypt "crypt.h" CRYPT_IN_LIBC)
    if(CRYPT_IN_LIBC)
        message(STATUS "Crypt functions found in libc")
        set(CRYPT_LIB "")  # No separate library needed
    else()
        message(WARNING "Crypt library not found - password functions may not work")
        message(WARNING "Consider installing: libcrypt-dev (Debian/Ubuntu) or glibc-crypt (some systems)")
    endif()
endif()

# Math library detection (usually required for linking)
find_library(MATH_LIB m)
if(MATH_LIB)
    message(STATUS "Math library found: ${MATH_LIB}")
else()
    message(STATUS "Math library not found - may be in libc")
endif()

# Developer convenience features and build helpers
message(STATUS "=== Developer Convenience Features ===")

# info - Comprehensive project and build information
add_custom_target(info
    COMMAND ${CMAKE_COMMAND} -E echo "=== Conquer ${PROJECT_VERSION} Build Information ==="
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Project Information:"
    COMMAND ${CMAKE_COMMAND} -E echo "  Name: ${PROJECT_NAME}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Version: ${PROJECT_VERSION}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Patchlevel: ${CONQUER_PATCHLEVEL}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Description: ${PROJECT_DESCRIPTION}"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Build Configuration:"
    COMMAND ${CMAKE_COMMAND} -E echo "  Platform: ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_PROCESSOR})"
    COMMAND ${CMAKE_COMMAND} -E echo "  Generator: ${CMAKE_GENERATOR}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Build Type: ${CMAKE_BUILD_TYPE}"
    COMMAND ${CMAKE_COMMAND} -E echo "  C Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}"
    COMMAND ${CMAKE_COMMAND} -E echo "  C Standard: C${CMAKE_C_STANDARD}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Source Dir: ${CMAKE_SOURCE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Binary Dir: ${CMAKE_BINARY_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Installation Configuration:"
    COMMAND ${CMAKE_COMMAND} -E echo "  Prefix: ${CMAKE_INSTALL_PREFIX}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Executables: ${CONQUER_EXE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Game Data: ${CONQUER_DEFAULT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Manual Pages: ${CONQUER_MAN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Build User: ${CONQUER_LOGIN}"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Library Dependencies:"
    COMMAND ${CMAKE_COMMAND} -E echo "  Ncurses: ${NCURSES_LIBRARIES}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Crypt: ${CRYPT_LIB}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Math: ${MATH_LIB}"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Available Targets:"
    COMMAND ${CMAKE_COMMAND} -E echo "  Executables: conquer conqrun conqsort conqps"
    COMMAND ${CMAKE_COMMAND} -E echo "  Help: help0-help5 (help file generation)"
    COMMAND ${CMAKE_COMMAND} -E echo "  Installation: install install_game install_admin"
    COMMAND ${CMAKE_COMMAND} -E echo "  Development: check config docs build_type info"
    COMMAND ${CMAKE_COMMAND} -E echo "  Archives: cpio tar zip"
    COMMAND ${CMAKE_COMMAND} -E echo "  Cleanup: clean clobber"
    COMMAND ${CMAKE_COMMAND} -E echo "  Testing: Unity framework integration available"
    COMMENT "Displaying comprehensive project information"
    VERBATIM
)

# rebuild - Clean and rebuild everything
add_custom_target(rebuild
    COMMAND ${CMAKE_COMMAND} -E echo "=== Rebuilding Conquer ${PROJECT_VERSION} ==="
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target all
    COMMAND ${CMAKE_COMMAND} -E echo "✓ Rebuild complete"
    COMMENT "Clean rebuild of all targets"
    VERBATIM
)

# configure_summary - Post-configuration summary for users
add_custom_target(configure_summary
    COMMAND ${CMAKE_COMMAND} -E echo "=== CMake Configuration Summary ==="
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "✓ Project: Conquer ${PROJECT_VERSION} (Patchlevel ${CONQUER_PATCHLEVEL})"
    COMMAND ${CMAKE_COMMAND} -E echo "✓ Platform: ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_PROCESSOR})"
    COMMAND ${CMAKE_COMMAND} -E echo "✓ Build Type: ${CMAKE_BUILD_TYPE}"
    COMMAND ${CMAKE_COMMAND} -E echo "✓ C Standard: C${CMAKE_C_STANDARD}"
    COMMAND ${CMAKE_COMMAND} -E echo "✓ Install Prefix: ${CMAKE_INSTALL_PREFIX}"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Ready to build! Run: cmake --build build"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMENT "Configuration summary"
    VERBATIM
)

