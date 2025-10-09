# Code Formatting Module for Conquer 4.x
# Provides targets for auditing and fixing code formatting issues
# Uses clang-format based on .clang-format configuration
# Created: 2025-10-07

if(NOT CLANG_FORMAT_EXECUTABLE)
    find_program(CLANG_FORMAT_EXECUTABLE
        NAMES clang-format
        DOC "Path to clang-format executable"
    )
endif()

if(CLANG_FORMAT_EXECUTABLE)
    message(STATUS "Found clang-format: ${CLANG_FORMAT_EXECUTABLE}")

    # Get clang-format version
    execute_process(
        COMMAND ${CLANG_FORMAT_EXECUTABLE} --version
        OUTPUT_VARIABLE CLANG_FORMAT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "Clang-format version: ${CLANG_FORMAT_VERSION}")

    # Collect all C source and header files
    file(GLOB_RECURSE ALL_C_SOURCES
        "${CMAKE_SOURCE_DIR}/*.c"
    )
    file(GLOB_RECURSE ALL_C_HEADERS
        "${CMAKE_SOURCE_DIR}/*.h"
    )

    # Exclude build directory and any other generated files
    list(FILTER ALL_C_SOURCES EXCLUDE REGEX "${CMAKE_BINARY_DIR}/.*")
    list(FILTER ALL_C_HEADERS EXCLUDE REGEX "${CMAKE_BINARY_DIR}/.*")

    set(ALL_FORMAT_FILES ${ALL_C_SOURCES} ${ALL_C_HEADERS})

    # Create reports directory for formatting reports
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/reports/formatting)

    # ==================================================================
    # Target: format-check
    # Audit code formatting without making changes
    # Generates report of files that need formatting
    # ==================================================================
    add_custom_target(format-check
        COMMAND ${CMAKE_COMMAND} -E echo "Checking code formatting..."
        COMMAND ${CMAKE_COMMAND} -E echo "Report will be saved to: reports/formatting/format_check.txt"
        COMMAND ${CLANG_FORMAT_EXECUTABLE}
            --dry-run
            --Werror
            --style=file
            ${ALL_FORMAT_FILES}
            > ${CMAKE_BINARY_DIR}/reports/formatting/format_check.txt 2>&1 || true
        COMMAND ${CMAKE_COMMAND} -E echo "Format check complete. Review reports/formatting/format_check.txt"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Auditing code formatting (no changes made)"
        VERBATIM
    )

    # ==================================================================
    # Target: format-check-verbose
    # Detailed formatting audit with line-by-line differences
    # Shows exactly what would change for each file
    # ==================================================================
    add_custom_target(format-check-verbose
        COMMAND ${CMAKE_COMMAND} -E echo "Running detailed formatting audit..."
        COMMAND ${CMAKE_COMMAND} -E echo "Checking ${CMAKE_SOURCE_DIR}/*.c and *.h"
        COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_BINARY_DIR}/reports/formatting/format_diff.txt
        COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/reports/formatting/format_diff.txt
        COMMAND ${CMAKE_COMMAND} -E echo "Generating formatting diffs..."
        COMMAND bash -c "for file in ${CMAKE_SOURCE_DIR}/*.c ${CMAKE_SOURCE_DIR}/*.h; do \
            if [ -f \"$$file\" ]; then \
                echo \"=== Checking $$file ===\" >> ${CMAKE_BINARY_DIR}/reports/formatting/format_diff.txt; \
                ${CLANG_FORMAT_EXECUTABLE} --style=file $$file | diff -u $$file - >> ${CMAKE_BINARY_DIR}/reports/formatting/format_diff.txt 2>&1 || true; \
            fi; \
        done"
        COMMAND ${CMAKE_COMMAND} -E echo "Detailed formatting report saved to: reports/formatting/format_diff.txt"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating detailed formatting differences"
        VERBATIM
    )

    # ==================================================================
    # Target: format-fix
    # Apply formatting fixes to all source files
    # Creates backups (.orig) before modifying files
    # ==================================================================
    add_custom_target(format-fix
        COMMAND ${CMAKE_COMMAND} -E echo "WARNING: This will modify source files!"
        COMMAND ${CMAKE_COMMAND} -E echo "Creating backups with .orig extension..."
        COMMAND bash -c "for file in ${CMAKE_SOURCE_DIR}/*.c ${CMAKE_SOURCE_DIR}/*.h; do \
            if [ -f \"$$file\" ]; then \
                cp \"$$file\" \"$$file.orig\"; \
            fi; \
        done"
        COMMAND ${CMAKE_COMMAND} -E echo "Applying formatting fixes..."
        COMMAND ${CLANG_FORMAT_EXECUTABLE}
            -i
            --style=file
            ${ALL_FORMAT_FILES}
        COMMAND ${CMAKE_COMMAND} -E echo "Formatting complete!"
        COMMAND ${CMAKE_COMMAND} -E echo "Original files backed up with .orig extension"
        COMMAND ${CMAKE_COMMAND} -E echo "Review changes with: git diff"
        COMMAND ${CMAKE_COMMAND} -E echo "Remove backups with: make format-clean-backups"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Applying clang-format to all source files"
        VERBATIM
    )

    # ==================================================================
    # Target: format-fix-single
    # Apply formatting to a single file specified by FORMAT_FILE variable
    # Usage: FORMAT_FILE=combat.c cmake --build build --target format-fix-single
    # ==================================================================
    set(FORMAT_FILE "" CACHE STRING "Single file to format")
    add_custom_target(format-fix-single
        COMMAND ${CMAKE_COMMAND} -E echo "Formatting single file: ${FORMAT_FILE}"
        COMMAND bash -c "if [ -n \"${FORMAT_FILE}\" ] && [ -f \"${CMAKE_SOURCE_DIR}/${FORMAT_FILE}\" ]; then \
                cp \"${CMAKE_SOURCE_DIR}/${FORMAT_FILE}\" \"${CMAKE_SOURCE_DIR}/${FORMAT_FILE}.orig\"; \
                ${CLANG_FORMAT_EXECUTABLE} -i --style=file \"${CMAKE_SOURCE_DIR}/${FORMAT_FILE}\"; \
                echo \"Formatted ${FORMAT_FILE} (backup: ${FORMAT_FILE}.orig)\"; \
            else \
                echo \"ERROR: FORMAT_FILE not set or file not found: ${FORMAT_FILE}\"; \
                echo \"Usage: FORMAT_FILE=combat.c cmake --build build --target format-fix-single\"; \
                exit 1; \
            fi"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Applying clang-format to single file"
        VERBATIM
    )

    # ==================================================================
    # Target: format-clean-backups
    # Remove all .orig backup files created by format-fix
    # ==================================================================
    add_custom_target(format-clean-backups
        COMMAND ${CMAKE_COMMAND} -E echo "Removing .orig backup files..."
        COMMAND bash -c "find ${CMAKE_SOURCE_DIR} -name '*.orig' -type f -delete"
        COMMAND ${CMAKE_COMMAND} -E echo "Backup files removed"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Cleaning up formatting backup files"
        VERBATIM
    )

    # ==================================================================
    # Target: format-report
    # Generate comprehensive formatting statistics
    # Shows file count, line count, and formatting issues
    # ==================================================================
    add_custom_target(format-report
        COMMAND ${CMAKE_COMMAND} -E echo "=== Code Formatting Report ==="
        COMMAND bash -c "echo \"Total C source files: `find ${CMAKE_SOURCE_DIR} -name '*.c' -not -path '${CMAKE_BINARY_DIR}/*' | wc -l`\""
        COMMAND bash -c "echo \"Total C header files: `find ${CMAKE_SOURCE_DIR} -name '*.h' -not -path '${CMAKE_BINARY_DIR}/*' | wc -l`\""
        COMMAND bash -c "echo \"Total lines of code: `cat ${CMAKE_SOURCE_DIR}/*.c ${CMAKE_SOURCE_DIR}/*.h 2>/dev/null | wc -l`\""
        COMMAND ${CMAKE_COMMAND} -E echo "Checking formatting compliance..."
        COMMAND ${CLANG_FORMAT_EXECUTABLE}
            --dry-run
            --Werror
            ${ALL_FORMAT_FILES}
            > ${CMAKE_BINARY_DIR}/reports/formatting/format_report.txt 2>&1 || true
        COMMAND bash -c "if [ -s ${CMAKE_BINARY_DIR}/reports/formatting/format_report.txt ]; then \
            echo \"Files needing formatting: `grep -c 'would be formatted' ${CMAKE_BINARY_DIR}/reports/formatting/format_report.txt 2>/dev/null || echo 0`\"; \
        else \
            echo \"All files properly formatted!\"; \
        fi"
        COMMAND ${CMAKE_COMMAND} -E echo "Detailed report: reports/formatting/format_report.txt"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating formatting statistics report"
        VERBATIM
    )

    # Print helpful message
    message(STATUS "Code formatting targets available:")
    message(STATUS "  make format-check          - Audit formatting without changes")
    message(STATUS "  make format-check-verbose  - Show detailed formatting diffs")
    message(STATUS "  make format-fix            - Apply formatting to all files (creates .orig backups)")
    message(STATUS "  make format-fix-single     - Format single file (set FORMAT_FILE=filename)")
    message(STATUS "  make format-clean-backups  - Remove .orig backup files")
    message(STATUS "  make format-report         - Generate formatting statistics")

else()
    message(WARNING "clang-format not found - formatting targets will not be available")
    message(WARNING "Install clang-format to enable code formatting features")
endif()
