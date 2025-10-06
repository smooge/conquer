# =============================================================================
# Static Analysis Integration for Conquer Modernization Project
# =============================================================================
# This module provides integration for static analysis tools:
# - Clang Static Analyzer (scan-build)
# - Clang-Tidy (modernization checks)
# - Custom test targets for automated quality checks
#
# Phase 8.4: Memory Management Enhancement - Static Analysis Integration
#
# Usage:
#   include(StaticAnalysis) in main CMakeLists.txt
#   Enable with: cmake -DENABLE_STATIC_ANALYSIS=ON
#   Run tests: ctest -L static-analysis
#
# =============================================================================

# Option to enable static analysis (off by default for faster normal builds)
option(ENABLE_STATIC_ANALYSIS "Enable static analysis tools integration" OFF)

if(NOT ENABLE_STATIC_ANALYSIS)
    message(STATUS "Static analysis: DISABLED (enable with -DENABLE_STATIC_ANALYSIS=ON)")
    return()
endif()

message(STATUS "Static analysis: ENABLED")

# =============================================================================
# Tool Detection
# =============================================================================

# Detect scan-build (Clang Static Analyzer)
find_program(SCAN_BUILD_EXECUTABLE
    NAMES scan-build scan-build-17 scan-build-16 scan-build-15
    DOC "Path to scan-build executable"
)

if(SCAN_BUILD_EXECUTABLE)
    message(STATUS "  scan-build found: ${SCAN_BUILD_EXECUTABLE}")
    set(SCAN_BUILD_FOUND TRUE)
else()
    message(STATUS "  scan-build: NOT FOUND")
    set(SCAN_BUILD_FOUND FALSE)
endif()

# Detect clang-tidy
find_program(CLANG_TIDY_EXECUTABLE
    NAMES clang-tidy clang-tidy-17 clang-tidy-16 clang-tidy-15
    DOC "Path to clang-tidy executable"
)

if(CLANG_TIDY_EXECUTABLE)
    message(STATUS "  clang-tidy found: ${CLANG_TIDY_EXECUTABLE}")
    set(CLANG_TIDY_FOUND TRUE)
else()
    message(STATUS "  clang-tidy: NOT FOUND")
    set(CLANG_TIDY_FOUND FALSE)
endif()

# Check if at least one tool is available
if(NOT SCAN_BUILD_FOUND AND NOT CLANG_TIDY_FOUND)
    message(WARNING "No static analysis tools found. Install clang-tools for full analysis.")
    message(WARNING "  Debian/Ubuntu: sudo apt install clang-tools")
    message(WARNING "  Fedora/RHEL: sudo dnf install clang-tools-extra")
    message(WARNING "  macOS: brew install llvm")
    return()
endif()

# =============================================================================
# Scan-Build Integration (Clang Static Analyzer)
# =============================================================================

if(SCAN_BUILD_FOUND)
    # Create reports directory for scan-build output
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/reports/scan-build")

    # Full project scan-build analysis
    add_custom_target(scan-build-full
        COMMAND ${CMAKE_COMMAND} -E echo "=== Running Clang Static Analyzer (scan-build) on full project ==="
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/reports/scan-build/full"
        COMMAND ${SCAN_BUILD_EXECUTABLE}
            -o "${CMAKE_BINARY_DIR}/reports/scan-build/full"
            --status-bugs
            --use-analyzer=${CMAKE_C_COMPILER}
            ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --clean-first
        COMMAND ${CMAKE_COMMAND} -E echo "✓ Scan-build analysis complete"
        COMMAND ${CMAKE_COMMAND} -E echo "Report location: ${CMAKE_BINARY_DIR}/reports/scan-build/full/"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running full scan-build analysis on project"
        VERBATIM
    )

    # Targeted scan-build for memory management files (Phase 8.4 focus)
    add_custom_target(scan-build-memory
        COMMAND ${CMAKE_COMMAND} -E echo "=== Scan-build: Memory Management Files ==="
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/reports/scan-build/memory"
        COMMAND ${SCAN_BUILD_EXECUTABLE}
            -o "${CMAKE_BINARY_DIR}/reports/scan-build/memory"
            --status-bugs
            --use-analyzer=${CMAKE_C_COMPILER}
            ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target conquer --target conqrun --clean-first
        COMMAND ${CMAKE_COMMAND} -E echo "✓ Memory-focused scan-build complete"
        COMMAND ${CMAKE_COMMAND} -E echo "Report location: ${CMAKE_BINARY_DIR}/reports/scan-build/memory/"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running scan-build on memory management code"
        VERBATIM
    )

    # Quick scan-build check (no clean, faster iteration)
    add_custom_target(scan-build-quick
        COMMAND ${CMAKE_COMMAND} -E echo "=== Quick scan-build check (incremental) ==="
        COMMAND ${SCAN_BUILD_EXECUTABLE}
            -o "${CMAKE_BINARY_DIR}/reports/scan-build/quick"
            --status-bugs
            --use-analyzer=${CMAKE_C_COMPILER}
            ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E echo "✓ Quick scan-build complete"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Quick incremental scan-build analysis"
        VERBATIM
    )

    message(STATUS "  Scan-build targets: scan-build-full, scan-build-memory, scan-build-quick")
endif()

# =============================================================================
# Clang-Tidy Integration
# =============================================================================

if(CLANG_TIDY_FOUND)
    # Generate compilation database for clang-tidy
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands for clang-tidy" FORCE)
    message(STATUS "  Compilation database: ${CMAKE_BINARY_DIR}/compile_commands.json")

    # Create reports directory for clang-tidy output
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/reports/clang-tidy")

    # Define source file groups for targeted analysis
    set(MEMORY_MGMT_FILES
        ${CMAKE_SOURCE_DIR}/display.c
        ${CMAKE_SOURCE_DIR}/misc.c
        ${CMAKE_SOURCE_DIR}/makeworl.c
        ${CMAKE_SOURCE_DIR}/combat.c
        ${CMAKE_SOURCE_DIR}/update.c
        ${CMAKE_SOURCE_DIR}/io.c
    )

    set(SECURITY_CRITICAL_FILES
        ${CMAKE_SOURCE_DIR}/safe_system.c
        ${CMAKE_SOURCE_DIR}/newlogin.c
        ${CMAKE_SOURCE_DIR}/admin.c
        ${CMAKE_SOURCE_DIR}/check.c
    )

    # Clang-tidy configuration with modernization checks
    set(CLANG_TIDY_CHECKS
        "-checks=\
readability-*,\
modernize-*,\
bugprone-*,\
clang-analyzer-*,\
performance-*,\
portability-*,\
cert-*,\
-modernize-use-trailing-return-type,\
-readability-magic-numbers"
    )

    # Full project clang-tidy analysis
    add_custom_target(clang-tidy-full
        COMMAND ${CMAKE_COMMAND} -E echo "=== Running clang-tidy on full project ==="
        COMMAND ${CLANG_TIDY_EXECUTABLE}
            -p ${CMAKE_BINARY_DIR}
            ${CLANG_TIDY_CHECKS}
            ${CMAKE_SOURCE_DIR}/*.c
            2>&1 | tee ${CMAKE_BINARY_DIR}/reports/clang-tidy/full_report.txt
        COMMAND ${CMAKE_COMMAND} -E echo "✓ Clang-tidy analysis complete"
        COMMAND ${CMAKE_COMMAND} -E echo "Report: ${CMAKE_BINARY_DIR}/reports/clang-tidy/full_report.txt"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running full clang-tidy analysis"
        VERBATIM
    )

    # Memory management focused clang-tidy
    add_custom_target(clang-tidy-memory
        COMMAND ${CMAKE_COMMAND} -E echo "=== Clang-tidy: Memory Management Files ==="
        COMMAND ${CLANG_TIDY_EXECUTABLE}
            -p ${CMAKE_BINARY_DIR}
            ${CLANG_TIDY_CHECKS}
            ${MEMORY_MGMT_FILES}
            2>&1 | tee ${CMAKE_BINARY_DIR}/reports/clang-tidy/memory_report.txt
        COMMAND ${CMAKE_COMMAND} -E echo "✓ Memory-focused clang-tidy complete"
        COMMAND ${CMAKE_COMMAND} -E echo "Report: ${CMAKE_BINARY_DIR}/reports/clang-tidy/memory_report.txt"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-tidy on memory management code"
        VERBATIM
    )

    # Security critical files clang-tidy
    add_custom_target(clang-tidy-security
        COMMAND ${CMAKE_COMMAND} -E echo "=== Clang-tidy: Security Critical Files ==="
        COMMAND ${CLANG_TIDY_EXECUTABLE}
            -p ${CMAKE_BINARY_DIR}
            -checks="cert-*,bugprone-*,clang-analyzer-security.*"
            ${SECURITY_CRITICAL_FILES}
            2>&1 | tee ${CMAKE_BINARY_DIR}/reports/clang-tidy/security_report.txt
        COMMAND ${CMAKE_COMMAND} -E echo "✓ Security-focused clang-tidy complete"
        COMMAND ${CMAKE_COMMAND} -E echo "Report: ${CMAKE_BINARY_DIR}/reports/clang-tidy/security_report.txt"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-tidy on security critical code"
        VERBATIM
    )

    # Clang-tidy with auto-fix (use with caution)
    add_custom_target(clang-tidy-fix
        COMMAND ${CMAKE_COMMAND} -E echo "=== Clang-tidy: Auto-fix mode (CAUTION) ==="
        COMMAND ${CMAKE_COMMAND} -E echo "This will modify source files. Ensure clean git state!"
        COMMAND ${CLANG_TIDY_EXECUTABLE}
            -p ${CMAKE_BINARY_DIR}
            -checks="modernize-*,readability-*"
            -fix
            ${CMAKE_SOURCE_DIR}/*.c
            2>&1 | tee ${CMAKE_BINARY_DIR}/reports/clang-tidy/fix_report.txt
        COMMAND ${CMAKE_COMMAND} -E echo "✓ Auto-fix complete - review changes before committing!"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-tidy with automatic fixes"
        VERBATIM
    )

    message(STATUS "  Clang-tidy targets: clang-tidy-full, clang-tidy-memory, clang-tidy-security, clang-tidy-fix")
endif()

# =============================================================================
# Combined Analysis Targets
# =============================================================================

if(SCAN_BUILD_FOUND AND CLANG_TIDY_FOUND)
    # Run all static analysis tools
    add_custom_target(analyze-all
        COMMAND ${CMAKE_COMMAND} -E echo "=== Running ALL Static Analysis Tools ==="
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target scan-build-full
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clang-tidy-full
        COMMAND ${CMAKE_COMMAND} -E echo "✓ All static analysis complete"
        COMMAND ${CMAKE_COMMAND} -E echo "Reports in: ${CMAKE_BINARY_DIR}/reports/"
        COMMENT "Running all static analysis tools"
        VERBATIM
    )

    # Phase 8.4 specific analysis (memory management focus)
    add_custom_target(analyze-phase84
        COMMAND ${CMAKE_COMMAND} -E echo "=== Phase 8.4: Memory Management Analysis ==="
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target scan-build-memory
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clang-tidy-memory
        COMMAND ${CMAKE_COMMAND} -E echo "✓ Phase 8.4 analysis complete"
        COMMAND ${CMAKE_COMMAND} -E echo "Compare results to manual Phase 8.4.2 findings"
        COMMENT "Phase 8.4 focused static analysis"
        VERBATIM
    )

    message(STATUS "  Combined targets: analyze-all, analyze-phase84")
endif()

# =============================================================================
# CTest Integration - Static Analysis as Tests
# =============================================================================

if(ENABLE_TESTING)
    # Add scan-build as a test (only if available)
    if(SCAN_BUILD_FOUND)
        add_test(
            NAME StaticAnalysis_ScanBuild_Memory
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target scan-build-memory
        )
        set_tests_properties(StaticAnalysis_ScanBuild_Memory PROPERTIES
            LABELS "static-analysis;phase8;memory"
            TIMEOUT 600
        )
    endif()

    # Add clang-tidy as a test (only if available)
    if(CLANG_TIDY_FOUND)
        add_test(
            NAME StaticAnalysis_ClangTidy_Memory
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clang-tidy-memory
        )
        set_tests_properties(StaticAnalysis_ClangTidy_Memory PROPERTIES
            LABELS "static-analysis;phase8;memory"
            TIMEOUT 300
        )

        add_test(
            NAME StaticAnalysis_ClangTidy_Security
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clang-tidy-security
        )
        set_tests_properties(StaticAnalysis_ClangTidy_Security PROPERTIES
            LABELS "static-analysis;security"
            TIMEOUT 300
        )
    endif()

    message(STATUS "  Static analysis tests added to CTest suite")
    message(STATUS "  Run with: ctest -L static-analysis")
endif()

# =============================================================================
# Summary and Usage Instructions
# =============================================================================

message(STATUS "")
message(STATUS "=== Static Analysis Tools Configuration ===")
if(SCAN_BUILD_FOUND)
    message(STATUS "✓ Scan-build targets available:")
    message(STATUS "  - make scan-build-full      (full project analysis)")
    message(STATUS "  - make scan-build-memory    (Phase 8.4 memory files)")
    message(STATUS "  - make scan-build-quick     (incremental check)")
endif()

if(CLANG_TIDY_FOUND)
    message(STATUS "✓ Clang-tidy targets available:")
    message(STATUS "  - make clang-tidy-full      (full project analysis)")
    message(STATUS "  - make clang-tidy-memory    (Phase 8.4 memory files)")
    message(STATUS "  - make clang-tidy-security  (security critical files)")
    message(STATUS "  - make clang-tidy-fix       (auto-fix - use with caution)")
endif()

if(SCAN_BUILD_FOUND AND CLANG_TIDY_FOUND)
    message(STATUS "✓ Combined analysis targets:")
    message(STATUS "  - make analyze-all          (all tools, full analysis)")
    message(STATUS "  - make analyze-phase84      (Phase 8.4 validation)")
endif()

if(ENABLE_TESTING)
    message(STATUS "✓ CTest integration:")
    message(STATUS "  - ctest -L static-analysis  (run as automated tests)")
endif()

message(STATUS "")
message(STATUS "Reports saved to: ${CMAKE_BINARY_DIR}/reports/")
message(STATUS "===========================================")
message(STATUS "")
