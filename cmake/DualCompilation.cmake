# DualCompilation.cmake - Sophisticated Admin/Game Mode Compilation System
#
# This module implements the complex dual compilation architecture where the same
# source files are compiled twice with different preprocessor definitions to create
# different executables with different functionality.
#
# ARCHITECTURE OVERVIEW:
# - ADMIN mode (-DADMIN): Administrative executable with combat.c included
# - GAME mode (-DCONQUER): Player-facing executable without combat.c
# - Shared sources: Same files compiled for both modes with appropriate flags
# - Object library approach: Avoids source duplication while enabling dual compilation

# =============================================================================
# SOURCE FILE ORGANIZATION
# =============================================================================

# Shared source files - compiled for BOTH admin and game modes
# These correspond to files that appear in both AFILS and GFILS in the Makefile
set(SHARED_SOURCES
    cexecute.c
    io.c
    navy.c
    magic.c
    misc.c
    data.c
    trade.c
    check.c
)

# Admin-only source files - only compiled for admin executable
# These files only appear in AFILS in the Makefile
set(ADMIN_ONLY_SOURCES
    combat.c
    admin.c
    makeworl.c
    spew.c
    newlogin.c
    update.c
    npc.c
    randeven.c
)

# Game-only source files - only compiled for game executable
# These files only appear in GFILS in the Makefile
set(GAME_ONLY_SOURCES
    commands.c
    forms.c
    main.c
    move.c
    reports.c
    display.c
    extcmds.c
)

# =============================================================================
# OBJECT LIBRARY APPROACH FOR SHARED SOURCES
# =============================================================================

# Object library for shared sources compiled in ADMIN mode
# This replicates the Makefile's creation of cexecuteA.o, ioA.o, etc.
add_library(shared_admin_objects OBJECT ${SHARED_SOURCES})
target_compile_definitions(shared_admin_objects PRIVATE ADMIN)
target_link_libraries(shared_admin_objects PRIVATE conquer_warnings)

# Object library for shared sources compiled in GAME mode
# This replicates the Makefile's creation of cexecuteG.o, ioG.o, etc.
add_library(shared_game_objects OBJECT ${SHARED_SOURCES})
target_compile_definitions(shared_game_objects PRIVATE CONQUER)
target_link_libraries(shared_game_objects PRIVATE conquer_warnings)

# =============================================================================
# ADMINISTRATIVE EXECUTABLE (conqrun)
# =============================================================================

# Administrative executable - includes combat.c and admin functionality
# Equivalent to: conqrun: $(AOBJS) in Makefile
add_executable(conqrun
    ${ADMIN_ONLY_SOURCES}
    $<TARGET_OBJECTS:shared_admin_objects>
)

# Apply admin-specific compilation definitions
target_compile_definitions(conqrun PRIVATE ADMIN)

# Link with Phase 4 warning preservation and standard libraries
target_link_libraries(conqrun PRIVATE conquer_warnings)

# Add common compile definitions (paths, version, etc.)
target_compile_definitions(conqrun PRIVATE
    DEFAULTDIR="${CONQUER_DEFAULT_DIR}"
    EXEDIR="${CONQUER_EXE_DIR}"
    PATCHLEVEL="${CONQUER_PATCHLEVEL}"
    LOGIN="${CONQUER_LOGIN}"
)

# =============================================================================
# GAME EXECUTABLE (conquer)
# =============================================================================

# Game executable - player-facing version without combat.c
# Equivalent to: conquer: $(GOBJS) in Makefile
add_executable(conquer
    ${GAME_ONLY_SOURCES}
    $<TARGET_OBJECTS:shared_game_objects>
)

# Apply game-specific compilation definitions
target_compile_definitions(conquer PRIVATE CONQUER)

# Link with Phase 4 warning preservation and standard libraries
target_link_libraries(conquer PRIVATE conquer_warnings)

# Add common compile definitions (paths, version, etc.)
target_compile_definitions(conquer PRIVATE
    DEFAULTDIR="${CONQUER_DEFAULT_DIR}"
    EXEDIR="${CONQUER_EXE_DIR}"
    PATCHLEVEL="${CONQUER_PATCHLEVEL}"
    LOGIN="${CONQUER_LOGIN}"
)

# =============================================================================
# ADDITIONAL UTILITY EXECUTABLES
# =============================================================================

# Sort utility - simple single-file executable
# Equivalent to: conqsort: sort.c in Makefile
add_executable(conqsort sort.c)
target_link_libraries(conqsort PRIVATE conquer_warnings)

# PostScript map generator - single-file executable with special definitions
# Equivalent to: conqps: $(PSSRC) in Makefile
# TODO: Will be implemented in Phase 5.4 with PostScript-specific definitions

# =============================================================================
# BUILD INFORMATION AND VERIFICATION
# =============================================================================

# Display dual compilation information
message(STATUS "Dual Compilation System Configuration:")
message(STATUS "  Shared sources: ${SHARED_SOURCES}")
message(STATUS "  Admin-only sources: ${ADMIN_ONLY_SOURCES}")
message(STATUS "  Game-only sources: ${GAME_ONLY_SOURCES}")
message(STATUS "  Admin executable: conqrun (with -DADMIN)")
message(STATUS "  Game executable: conquer (with -DCONQUER)")

# Create verification target to ensure dual compilation works correctly
add_custom_target(verify_dual_compilation
    COMMAND ${CMAKE_COMMAND} -E echo "=== Dual Compilation Verification ==="
    COMMAND ${CMAKE_COMMAND} -E echo "Admin executable: $<TARGET_FILE:conqrun>"
    COMMAND ${CMAKE_COMMAND} -E echo "Game executable: $<TARGET_FILE:conquer>"
    COMMAND ${CMAKE_COMMAND} -E echo "Sort utility: $<TARGET_FILE:conqsort>"
    COMMAND ${CMAKE_COMMAND} -E echo "=== Verification Complete ==="
    COMMENT "Verifying dual compilation system executables"
    VERBATIM
)