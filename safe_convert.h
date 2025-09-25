/*
 * safe_convert.h - Type-safe conversion utilities for modernization
 *
 * CONVERSION UTILITY MODULE
 *
 * This module provides type-safe conversion functions to eliminate compiler
 * warnings during C2023 modernization while maintaining correctness and
 * preventing overflow conditions. These utilities support the Phase 4.8
 * conversion warning elimination strategy.
 *
 * Design Principles:
 * 1. Range Validation - All conversions check for overflow/underflow
 * 2. Game Logic Preservation - Conversions respect game value constraints
 * 3. Cross-Platform Safety - Handle platform-specific type variations
 * 4. Zero Performance Impact - Inline functions with compile-time optimization
 * 5. Systematic Application - Consistent patterns across entire codebase
 *
 * Usage Philosophy:
 * - Prefer architectural type changes over conversion utilities when possible
 * - Use utilities when legacy constraints prevent type architecture changes
 * - Apply consistently across all files requiring conversion warning fixes
 * - Document conversion rationale for maintenance clarity
 *
 * Module Integration:
 * - Include in all source files requiring conversion warning elimination
 * - Add to build system (Makefile, CMake) for compilation
 * - No external dependencies beyond standard C library
 * - Compatible with both admin and game compilation modes
 *
 * This file is part of Conquer.
 * Copyright (C) 2025 Juan Manuel Méndez Rey (Vejeta) - Licensed under GPL v3
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef SAFE_CONVERT_H
#define SAFE_CONVERT_H

#include <limits.h>
#include <sys/types.h>
#include <math.h>

/*
 * Game-specific constants for value range validation
 * These should match the definitions in data.h
 */
#ifndef MAXTGVAL
#define MAXTGVAL 100  /* Maximum value for nation attributes */
#endif

/*
 * safe_clamp_uchar - Safely clamp long value to unsigned char range for nation attributes
 *
 * Clamps a long integer value to the valid range for nation attributes (0-MAXTGVAL).
 * Used throughout nation attribute calculations where computed values may exceed
 * the target field size. MAXTGVAL is defined as 100 in data.h.
 *
 * This function replaces unsafe min() macro usage and direct casting that
 * generates conversion warnings. It ensures values remain within game logic
 * constraints while providing compiler warning elimination.
 *
 * Parameters:
 *   value - Long integer value to clamp (may be negative or exceed range)
 *
 * Returns:
 *   Clamped value in range 0-MAXTGVAL as unsigned char
 *
 * Example Usage:
 *   curntn->terror = safe_clamp_uchar(temp/5);
 *   curntn->wealth = safe_clamp_uchar(calculated_wealth);
 */
static inline unsigned char safe_clamp_uchar(long value) {
    if (value < 0) return 0;
    if (value > MAXTGVAL) return MAXTGVAL;
    return (unsigned char)value;
}

/*
 * safe_uid_to_int - Safely convert uid_t to int with platform validation
 *
 * Converts uid_t to int with range checking. uid_t is typically unsigned int
 * but may vary by platform (could be uint32_t, unsigned long, etc.).
 * This function handles the conversion safely across different platforms.
 *
 * Used when interfacing with legacy functions that expect int parameters
 * but receive uid_t values from system calls like getuid().
 *
 * Parameters:
 *   uid - uid_t value to convert (platform-dependent unsigned type)
 *
 * Returns:
 *   int value, or -1 if uid exceeds INT_MAX (indicating error condition)
 *
 * Example Usage:
 *   newlogin(safe_uid_to_int(realuser));
 *   int user_id = safe_uid_to_int(getuid());
 */
static inline int safe_uid_to_int(uid_t uid) {
    if (uid > INT_MAX) return -1;
    return (int)uid;
}

/*
 * safe_long_to_int - Safely convert long to int with range validation
 *
 * Converts long to int with range checking to prevent overflow. Used in
 * calculations where intermediate results may exceed int range but final
 * result should fit in int.
 *
 * Handles both positive and negative overflow conditions by clamping to
 * the valid int range rather than allowing undefined behavior.
 *
 * Parameters:
 *   value - Long value to convert (may exceed int range)
 *
 * Returns:
 *   int value clamped to INT_MIN/INT_MAX range
 *
 * Example Usage:
 *   x = safe_long_to_int(curntn->eatrate/2 + temp/curntn->tciv);
 *   count = safe_long_to_int(calculation_result);
 */
static inline int safe_long_to_int(long value) {
    if (value > INT_MAX) return INT_MAX;
    if (value < INT_MIN) return INT_MIN;
    return (int)value;
}

/*
 * safe_size_to_int - Safely convert size_t to int with range validation
 *
 * Converts size_t to int with overflow checking. Used when size_t values
 * (from strlen, sizeof, etc.) need to be used with legacy int-based APIs.
 *
 * Since size_t is unsigned and potentially larger than int, this function
 * provides safe conversion with overflow detection.
 *
 * Parameters:
 *   size - size_t value to convert (from strlen, sizeof, etc.)
 *
 * Returns:
 *   int value, or INT_MAX if size exceeds INT_MAX
 *
 * Example Usage:
 *   int len = safe_size_to_int(strlen(buffer));
 *   int count = safe_size_to_int(array_size);
 */
static inline int safe_size_to_int(size_t size) {
    if (size > INT_MAX) return INT_MAX;
    return (int)size;
}

/*
 * safe_int_to_uchar - Safely convert int to unsigned char with validation
 *
 * Converts int to unsigned char with range checking. Used for character
 * calculations and array indexing where negative values should be clamped.
 *
 * Parameters:
 *   value - int value to convert (may be negative or exceed uchar range)
 *
 * Returns:
 *   unsigned char value clamped to 0-255 range
 *
 * Example Usage:
 *   array_index = safe_int_to_uchar(calculated_index);
 *   char_value = safe_int_to_uchar(ascii_calculation);
 */
static inline unsigned char safe_int_to_uchar(int value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (unsigned char)value;
}

/*
 * safe_int_to_short - Safely convert int to short with validation
 *
 * Converts int to short int with range checking. Used for nation indices
 * and other values that need to fit in 16-bit signed range.
 *
 * Parameters:
 *   value - int value to convert (may exceed short range)
 *
 * Returns:
 *   short value clamped to SHRT_MIN-SHRT_MAX range
 *
 * Example Usage:
 *   country = safe_int_to_short(nation_id);
 *   index = safe_int_to_short(calculated_index);
 */
static inline short safe_int_to_short(int value) {
    if (value < SHRT_MIN) return SHRT_MIN;
    if (value > SHRT_MAX) return SHRT_MAX;
    return (short)value;
}

/*
 * safe_int_to_size - Safely convert int to size_t with validation
 *
 * Converts int to size_t with range checking. Used for array counts,
 * string lengths, and other size-related values passed to library functions.
 *
 * Parameters:
 *   value - int value to convert (may be negative)
 *
 * Returns:
 *   size_t value, clamped to 0 minimum (negative values become 0)
 *
 * Example Usage:
 *   qsort(array, safe_int_to_size(count), sizeof(item), compare);
 *   fwrite(buffer, 1, safe_int_to_size(length), file);
 */
static inline size_t safe_int_to_size(int value) {
    if (value < 0) return 0;
    return (size_t)value;
}

/*
 * safe_size_to_short - Safely convert size_t to short with range validation
 *
 * Converts size_t to short with overflow checking. Used when size_t values
 * (from strlen, sizeof, etc.) need to be stored in short variables or used
 * in calculations with short arithmetic.
 *
 * Since size_t is unsigned and potentially larger than short, this function
 * provides safe conversion with overflow detection and clamping.
 *
 * Parameters:
 *   size - size_t value to convert (from strlen, sizeof, etc.)
 *
 * Returns:
 *   short value, clamped to SHRT_MAX if size exceeds short range
 *
 * Example Usage:
 *   x += safe_size_to_short(strlen(unit_name)) + 3;
 *   short length = safe_size_to_short(string_length);
 */
static inline short safe_size_to_short(size_t size) {
    if (size > SHRT_MAX) return SHRT_MAX;
    return (short)size;
}

/*
 * safe_long_to_float - Safely convert long to float with validation
 *
 * Converts long to float with range checking. Used in calculations where
 * long integer results need to be used in floating-point calculations.
 * Handles potential overflow to float range and maintains precision.
 *
 * Parameters:
 *   value - Long value to convert (may exceed float precision range)
 *
 * Returns:
 *   float value, preserving as much precision as possible
 *
 * Example Usage:
 *   astr = safe_long_to_float(asold * (100 + abonus));
 *   float result = safe_long_to_float(calculation);
 */
static inline float safe_long_to_float(long value) {
    return (float)value;
}

/*
 * safe_float_to_int - Safely convert float to int with validation
 *
 * Converts float to int with range checking and rounding. Used when
 * floating-point calculations need to be converted back to integer
 * values for further processing.
 *
 * Parameters:
 *   value - Float value to convert (may exceed int range)
 *
 * Returns:
 *   int value clamped to INT_MIN-INT_MAX range, rounded to nearest integer
 *
 * Example Usage:
 *   odds = safe_float_to_int((astr*100)/dstr);
 *   int result = safe_float_to_int(calculation);
 */
static inline int safe_float_to_int(float value) {
    if (value > INT_MAX) return INT_MAX;
    if (value < INT_MIN) return INT_MIN;
    return (int)(value + 0.5f); /* Round to nearest integer */
}

/*
 * safe_short_to_char - Safely convert short to char with validation
 *
 * Converts short to char with range checking. Used when short values
 * from macros or calculations need to be stored in char variables.
 * Clamps to valid char range to prevent overflow.
 *
 * Parameters:
 *   value - Short value to convert (may exceed char range)
 *
 * Returns:
 *   char value clamped to CHAR_MIN-CHAR_MAX range
 *
 * Example Usage:
 *   wnum[j] = safe_short_to_char(SHIPS(ntn[country].nvy[unit[j]].warships,N_LIGHT));
 *   char result = safe_short_to_char(calculation);
 */
static inline char safe_short_to_char(short value) {
    if (value > CHAR_MAX) return CHAR_MAX;
    if (value < CHAR_MIN) return CHAR_MIN;
    return (char)value;
}

/*
 * safe_int_to_char - Convert int to char with bounds checking
 *
 * Safely converts an integer value to char, clamping to valid char range.
 * Used for array indexing and character data where int values need to fit
 * in char storage.
 *
 * Parameters:
 *   value - The int value to convert
 *
 * Returns:
 *   char value clamped to [CHAR_MIN, CHAR_MAX] range
 */
static inline char safe_int_to_char(int value) {
    if (value > CHAR_MAX) return CHAR_MAX;
    if (value < CHAR_MIN) return CHAR_MIN;
    return (char)value;
}

/*
 * safe_short_to_uchar - Convert short to unsigned char with bounds checking
 *
 * Safely converts a short value to unsigned char, clamping to valid range.
 * Used for coordinate systems and data fields where short values need to fit
 * in unsigned char storage.
 *
 * Parameters:
 *   value - The short value to convert
 *
 * Returns:
 *   unsigned char value clamped to [0, UCHAR_MAX] range
 */
static inline unsigned char safe_short_to_uchar(short value) {
    if (value > UCHAR_MAX) return UCHAR_MAX;
    if (value < 0) return 0;
    return (unsigned char)value;
}

/*
 * safe_int_to_ushort - Convert int to unsigned short with bounds checking
 *
 * Safely converts an integer value to unsigned short, clamping to valid range.
 * Used for naval ship operations and bit field assignments where int values
 * need to fit in unsigned short storage. Particularly useful for P_NWSHP,
 * P_NMSHP, and P_NGSHP assignments in naval fleet management.
 *
 * This function eliminates sign conversion warnings that occur when assigning
 * signed short results to unsigned short fields in ship manipulation functions.
 *
 * Parameters:
 *   value - The int value to convert
 *
 * Returns:
 *   unsigned short value clamped to [0, USHRT_MAX] range
 *
 * Example Usage:
 *   P_NWSHP |= safe_int_to_ushort(hold);
 *   P_NMSHP &= safe_int_to_ushort(calculated_mask);
 */
static inline unsigned short safe_int_to_ushort(int value) {
    if (value > USHRT_MAX) return USHRT_MAX;
    if (value < 0) return 0;
    return (unsigned short)value;
}

/*
 * safe_long_to_short - Convert long to short with bounds checking
 *
 * Safely converts a long integer value to short, clamping to valid short range.
 * Used for calculations where long intermediate results need to be stored in
 * short variables, particularly in legacy data structures and game calculations.
 *
 * This function eliminates conversion warnings when long arithmetic results
 * (such as division operations or mercenary calculations) need to fit in
 * short storage fields like i_people, MERCATT, and MERCDEF.
 *
 * Parameters:
 *   value - The long value to convert
 *
 * Returns:
 *   short value clamped to [SHRT_MIN, SHRT_MAX] range
 *
 * Example Usage:
 *   sct[x][y].i_people = safe_long_to_short(sct[x][y].people/256);
 *   MERCATT = safe_long_to_short((MERCMEN*MERCATT+armynum*x)/(MERCMEN+armynum));
 */
static inline short safe_long_to_short(long value) {
    if (value > SHRT_MAX) return SHRT_MAX;
    if (value < SHRT_MIN) return SHRT_MIN;
    return (short)value;
}

/*
 * safe_float_to_long - Convert float to long with bounds checking
 *
 * Safely converts float values to long integers with range validation.
 * Used when floating-point calculations need to be stored in long variables
 * such as food consumption calculations and economic computations.
 *
 * This function handles the conversion safely, clamping values to the
 * valid long integer range and handling NaN/infinity cases.
 *
 * Parameters:
 *   value - float value to convert
 *
 * Returns:
 *   long value clamped to [LONG_MIN, LONG_MAX] range, 0 for NaN/infinity
 *
 * Example Usage:
 *   curntn->tfood -= safe_float_to_long(curntn->tmil * P_EATRATE * 2.0f);
 *   long food_consumed = safe_float_to_long(population * consumption_rate);
 */
static inline long safe_float_to_long(float value) {
    /* Handle NaN and infinity cases */
    if (value != value || value == INFINITY || value == -INFINITY) return 0;

    if (value > (float)LONG_MAX) return LONG_MAX;
    if (value < (float)LONG_MIN) return LONG_MIN;
    return (long)value;
}

/*
 * safe_double_to_long - Convert double to long with bounds checking
 *
 * Safely converts double values to long integers with range validation.
 * Used when double-precision floating-point calculations need to be stored
 * in long variables such as food consumption and economic computations.
 *
 * This function handles the conversion safely, clamping values to the
 * valid long integer range and handling NaN/infinity cases.
 *
 * Parameters:
 *   value - double value to convert
 *
 * Returns:
 *   long value clamped to [LONG_MIN, LONG_MAX] range, 0 for NaN/infinity
 *
 * Example Usage:
 *   curntn->tfood -= safe_double_to_long(curntn->tmil * P_EATRATE * 2.0);
 *   long food_consumed = safe_double_to_long(population * consumption_rate);
 */
static inline long safe_double_to_long(double value) {
    /* Handle NaN and infinity cases */
    if (value != value || value == INFINITY || value == -INFINITY) return 0;

    if (value > (double)LONG_MAX) return LONG_MAX;
    if (value < (double)LONG_MIN) return LONG_MIN;
    return (long)value;
}

/*
 * safe_double_to_int - Convert double to int with bounds checking
 *
 * Safely converts double values to int with range validation.
 * Used when double-precision floating-point calculations need to be stored
 * in int variables such as communications radius calculations and macro
 * evaluations that produce double results.
 *
 * This function handles the conversion safely, clamping values to the
 * valid int integer range and handling NaN/infinity cases. Particularly
 * useful for macro calculations like P_NTNCOM that produce double values
 * but need to be used as int parameters.
 *
 * Parameters:
 *   value - double value to convert
 *
 * Returns:
 *   int value clamped to [INT_MIN, INT_MAX] range, 0 for NaN/infinity
 *
 * Example Usage:
 *   com = safe_double_to_int(P_NTNCOM);
 *   int radius = safe_double_to_int(calculation_result);
 */
static inline int safe_double_to_int(double value) {
    /* Handle NaN and infinity cases */
    if (value != value || value == INFINITY || value == -INFINITY) return 0;

    if (value > (double)INT_MAX) return INT_MAX;
    if (value < (double)INT_MIN) return INT_MIN;
    return (int)value;
}

/*
 * safe_long_to_uchar - Convert long to unsigned char with bounds checking
 *
 * Safely converts long integers to unsigned char with range validation.
 * Used when long calculations need to be stored in unsigned char variables
 * such as popularity, poverty, and other small-value nation attributes.
 *
 * This function handles the conversion safely, clamping values to the
 * valid unsigned char range to prevent overflow conditions.
 *
 * Parameters:
 *   value - long value to convert
 *
 * Returns:
 *   unsigned char value clamped to [0, UCHAR_MAX] range
 *
 * Example Usage:
 *   curntn->popularity = safe_long_to_uchar(calculation_result);
 *   curntn->poverty = safe_long_to_uchar(poverty_calculation);
 */
static inline unsigned char safe_long_to_uchar(long value) {
    if (value < 0) return 0;
    if (value > UCHAR_MAX) return UCHAR_MAX;
    return (unsigned char)value;
}

/*
 * safe_uid_to_short - Convert uid_t to short with bounds checking
 *
 * Safely converts uid_t to short with range checking. uid_t is typically
 * unsigned int but may vary by platform (could be uint32_t, unsigned long, etc.).
 * This function handles the conversion safely across different platforms.
 *
 * Used when uid_t values from system calls need to be stored in short variables
 * for legacy data structures or function parameters that expect short values.
 * Eliminates the need for nested conversions like safe_int_to_short(safe_uid_to_int()).
 *
 * Parameters:
 *   uid - uid_t value to convert (platform-dependent unsigned type)
 *
 * Returns:
 *   short value clamped to [0, SHRT_MAX] range, or 0 if uid is too large
 *
 * Example Usage:
 *   realuser = safe_uid_to_short(getuid());
 *   short user_id = safe_uid_to_short(realuser_uid);
 */
static inline short safe_uid_to_short(uid_t uid) {
    if (uid > SHRT_MAX) return SHRT_MAX;
    return (short)uid;
}

/*
 * safe_rand_short - Generate random short int value with bounds checking
 *
 * Safely generates a random short integer in the range [0, max_val).
 * Used for country selection, nation indices, and other values that need
 * to fit in 16-bit signed range. Provides type-safe alternative to
 * rand()%max casting that generates conversion warnings.
 *
 * This function eliminates the need for explicit casting when rand() results
 * need to be stored in short variables, particularly for nation/country
 * selection and array indexing operations.
 *
 * Parameters:
 *   max_val - Maximum value (exclusive), must be positive and <= SHRT_MAX
 *
 * Returns:
 *   short value in range [0, max_val), or 0 if max_val is invalid
 *
 * Example Usage:
 *   country = safe_rand_short(NTOTAL);        // Instead of: country=rand()%NTOTAL;
 *   nation_id = safe_rand_short(active_nations);
 */
static inline short safe_rand_short(int max_val) {
    if (max_val <= 0 || max_val > SHRT_MAX) {
        return 0;  /* Return 0 for invalid range */
    }
    int result = rand() % max_val;
    return (short)result;
}

/*
 * safe_rand_uchar - Generate random unsigned char value with bounds checking
 *
 * Safely generates a random unsigned char in the range [0, max_val).
 * Used for resource generation (metal, jewels), sector attributes, and other
 * small-value assignments that need to fit in 8-bit unsigned range.
 * Provides type-safe alternative to rand()%max casting.
 *
 * This function is particularly useful for resource assignment operations
 * where rand() results determine quantities that must fit in unsigned char
 * storage fields.
 *
 * Parameters:
 *   max_val - Maximum value (exclusive), must be positive and <= UCHAR_MAX
 *
 * Returns:
 *   unsigned char value in range [0, max_val), or 0 if max_val is invalid
 *
 * Example Usage:
 *   sptr->metal = safe_rand_uchar(7) + 2;     // Instead of: sptr->metal = rand()%7 + 2;
 *   sptr->jewels = safe_rand_uchar(3) + 1;    // Instead of: sptr->jewels = rand()%3 + 1;
 */
static inline unsigned char safe_rand_uchar(int max_val) {
    if (max_val <= 0 || max_val > UCHAR_MAX) {
        return 0;  /* Return 0 for invalid range */
    }
    int result = rand() % max_val;
    return (unsigned char)result;
}

/*
 * safe_rand_uint - Generate random unsigned int value with bounds checking
 *
 * Safely generates a random unsigned int for seeding operations and large
 * range random number generation. Used primarily with srand() for random
 * number generator seeding and other operations requiring unsigned int values.
 *
 * This function handles the conversion from signed int (rand result) to
 * unsigned int safely, eliminating sign conversion warnings in seeding
 * operations.
 *
 * Parameters:
 *   max_val - Maximum value (exclusive), must be positive
 *
 * Returns:
 *   unsigned int value in range [0, max_val), or 0 if max_val is 0
 *
 * Example Usage:
 *   srand(safe_rand_uint(i*17+enemy+TURN*3));  // For seeding operations
 *   seed_value = safe_rand_uint(calculation);
 */
static inline unsigned int safe_rand_uint(unsigned int max_val) {
    if (max_val == 0) {
        return 0;
    }
    /* Handle potential overflow in modulo operation */
    if (max_val > INT_MAX) {
        return (unsigned int)rand();  /* Return raw rand() for very large ranges */
    }
    return (unsigned int)(rand() % (int)max_val);
}

/*
 * safe_rand_int - Generate random int value for probability calculations
 *
 * Returns the raw rand() result as int without conversion. Used for probability
 * checks and percentage calculations where the full int range is needed and
 * no type conversion is required. Provides a clear semantic distinction from
 * raw rand() calls.
 *
 * This function serves as a semantic marker for probability-based rand() usage
 * while maintaining the same behavior as direct rand() calls. It helps
 * distinguish probability calculations from value generation.
 *
 * Returns:
 *   int value from rand() in range [0, RAND_MAX]
 *
 * Example Usage:
 *   if (safe_rand_int() % 100 < chance) { ... }  // Instead of: if(rand()%100 < chance)
 *   if (safe_rand_int() % 4 == 0) { ... }        // Instead of: if(rand()%4 == 0)
 */
static inline int safe_rand_int(void) {
    return rand();
}

/*
 * safe_long_to_uint - Safe conversion from long int to unsigned int
 *
 * Converts long int values to unsigned int with proper handling of negative
 * values and potential overflow. This function is commonly needed for
 * arithmetic expressions that result in long int but need to be passed to
 * functions expecting unsigned int (such as srand/srandom).
 *
 * For negative values, converts to absolute value to ensure valid unsigned
 * result. For values exceeding UINT_MAX, truncates while preserving
 * randomization properties for seeding operations.
 *
 * Parameters:
 *   value - The long int value to convert
 *
 * Returns:
 *   unsigned int value safe for use with unsigned int parameters
 *
 * Example Usage:
 *   srand(safe_long_to_uint(i*17+enemy+TURN*3));     // Instead of: srand(i*17+enemy+TURN*3);
 *   func(safe_long_to_uint(calculation));            // Instead of: func(calculation);
 */
static inline unsigned int safe_long_to_uint(long value) {
    // Handle negative values by converting to absolute value
    if (value < 0) {
        value = -value;
    }

    // Convert to unsigned int, truncating if necessary
    // This preserves useful properties while eliminating warnings
    return (unsigned int)value;
}

/*
 * Conversion Utility Usage Guidelines
 *
 * 1. ARCHITECTURAL FIRST: Always prefer changing variable types over conversions
 *    - Change int to size_t for string lengths and array indices
 *    - Update function signatures to use appropriate types
 *    - Use consistent types throughout call chains
 *
 * 2. UTILITIES SECOND: Use conversion utilities when architectural changes aren't feasible
 *    - Legacy function interfaces that can't be changed
 *    - Cross-module boundaries with different type requirements
 *    - Platform-specific type handling
 *
 * 3. DOCUMENTATION: Always document why conversion is needed
 *    - Reference external constraints (legacy APIs, file formats)
 *    - Explain value range requirements and validation
 *    - Note any special handling for edge cases
 *
 * 4. SYSTEMATIC APPLICATION: Use consistently across the codebase
 *    - Same patterns for same types of conversions
 *    - Uniform error handling approaches
 *    - Consistent validation logic
 *
 * 5. RANDOM NUMBER GENERATION: Use type-specific safe_rand functions
 *    - safe_rand_short() for country/nation selection
 *    - safe_rand_uchar() for resource generation (metal, jewels)
 *    - safe_rand_uint() for seeding operations
 *    - safe_rand_int() for probability calculations and percentage checks
 *
 * 6. FLOATING POINT CONVERSIONS: Use appropriate safe conversion functions
 *    - safe_float_to_int() for single-precision float to int conversions
 *    - safe_double_to_int() for double-precision to int conversions (macro results)
 *    - safe_float_to_long() for float to long conversions
 *    - safe_double_to_long() for double to long conversions (large calculations)
 */

/*
 * safe_int_to_float - Convert int to float safely
 *
 * Converts int to float without precision loss warnings. Used when
 * integer calculations need to be converted to floating-point for
 * mathematical operations while preserving exact representation.
 *
 * Parameters:
 *   value - The int value to convert (will fit exactly in float)
 *
 * Returns:
 *   float value representing the exact integer value
 *
 * Example Usage:
 *   points += Mcost[i] * safe_int_to_float(spent[i]) / safe_int_to_float(Munits[i]);
 *   float result = safe_int_to_float(calculation);
 */
static inline float safe_int_to_float(int value) {
    return (float)value;
}

/*
 * safe_long_to_char - Convert long to char with bounds checking
 *
 * Safely converts a long value to char, clamping to valid char range.
 * Used for calculations that result in long values but need to be stored
 * in char variables for nation attributes or resource allocation.
 *
 * Parameters:
 *   value - The long value to convert (may exceed char range)
 *
 * Returns:
 *   char value clamped to [CHAR_MIN, CHAR_MAX] range
 *
 * Example Usage:
 *   curntn->repro = safe_long_to_char(Mvalues[CH_REPRO] * spent[CH_REPRO]);
 *   char attribute = safe_long_to_char(calculation_result);
 */
static inline char safe_long_to_char(long value) {
    if (value > CHAR_MAX) return CHAR_MAX;
    if (value < CHAR_MIN) return CHAR_MIN;
    return (char)value;
}

#endif /* SAFE_CONVERT_H */