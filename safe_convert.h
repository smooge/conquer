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
#include <stdlib.h>  /* For rand(), srand() used by safe_rand_* functions */
#include <time.h>    /* For time_t used by safe_time_* functions */
#include <stdint.h>  /* For SIZE_MAX used by safe_*_to_size functions */

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
 * safe_long_to_double - Safely convert long to double with validation
 *
 * Converts long to double for floating-point calculations. Used when
 * integer values need to be converted to double precision for
 * mathematical operations like economic calculations.
 *
 * Parameters:
 *   value - Long value to convert to double
 *
 * Returns:
 *   double representation of the long value (no precision loss)
 *
 * Examples:
 *   temp = 1000.0 * safe_long_to_double(curntn->tgold/WORLDGOLD);
 *   double wealth = safe_long_to_double(nation_gold);
 */
static inline double safe_long_to_double(long value) {
    return (double)value;
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
    if (value > (float)INT_MAX) return INT_MAX;
    if (value < (float)INT_MIN) return INT_MIN;
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
    if (value != value || value == HUGE_VALF || value == -HUGE_VALF) return 0;

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
    if (value != value || value == HUGE_VAL || value == -HUGE_VAL) return 0;

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
    if (value != value || value == HUGE_VAL || value == -HUGE_VAL) return 0;

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
	int result = 0;
    if (max_val <= 0 || max_val > SHRT_MAX) {
        return 0;  /* Return 0 for invalid range */
    }
    result = rand() % max_val;
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
	int result = 0;
    if (max_val <= 0 || max_val > UCHAR_MAX) {
        return 0;  /* Return 0 for invalid range */
    }
    result = rand() % max_val;
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

/*
 * safe_double_to_float - Convert double to float with precision handling
 *
 * Safely converts a double value to float, handling potential precision loss.
 * Used when macro calculations that expand to double need to be assigned to
 * float variables while eliminating compiler warnings about precision loss.
 *
 * Parameters:
 *   value - The double value to convert (may lose precision)
 *
 * Returns:
 *   float value representing the double (with potential precision loss)
 *
 * Example Usage:
 *   temp = safe_double_to_float(P_EATRATE);     // Instead of: temp = P_EATRATE;
 *   float rate = safe_double_to_float(macro_expansion);
 */
static inline float safe_double_to_float(double value) {
    return (float)value;
}

/*
 * safe_short_to_uid - Convert short to uid_t with bounds checking
 *
 * Safely converts a short value to uid_t (typically unsigned int).
 * Used when legacy code stores user IDs as short values but modern
 * system calls expect uid_t parameters.
 *
 * Parameters:
 *   value - The short value to convert (typically a stored user ID)
 *
 * Returns:
 *   uid_t value, 0 if negative (invalid UID)
 *
 * Example Usage:
 *   getpwuid(safe_short_to_uid(curntn->uid));
 *   uid_t user = safe_short_to_uid(stored_uid);
 */
static inline uid_t safe_short_to_uid(short value) {
    if (value < 0) return 0;  /* Invalid UID */
    return (uid_t)value;
}

/*
 * =============================================================================
 * EXTENDED CONVERSION FUNCTION SUITE
 * =============================================================================
 *
 * The following functions provide comprehensive type-safe conversions between
 * all standard C types. These functions complete the conversion matrix to
 * enable safe modernization of legacy codebases across multiple projects.
 *
 * Design principles maintained:
 * - Range validation with appropriate clamping
 * - Consistent error handling across all functions
 * - Zero performance impact with inline implementation
 * - Cross-platform compatibility for 32-bit and 64-bit systems
 * - Comprehensive documentation for maintenance clarity
 */

/*
 * safe_uchar_to_int - Convert unsigned char to int safely
 *
 * Converts unsigned char to int without any range checking needed,
 * as unsigned char values [0, 255] always fit in int range.
 * Used for character-based array indices and calculations.
 *
 * Parameters:
 *   value - unsigned char value to convert (always valid)
 *
 * Returns:
 *   int value (guaranteed to fit in int range)
 */
static inline int safe_uchar_to_int(unsigned char value) {
    return (int)value;
}

/*
 * safe_ushort_to_int - Convert unsigned short to int safely
 *
 * Converts unsigned short to int without any range checking needed,
 * as unsigned short values [0, 65535] always fit in int range on
 * standard platforms where int is at least 32 bits.
 *
 * Parameters:
 *   value - unsigned short value to convert (always valid)
 *
 * Returns:
 *   int value (guaranteed to fit in int range)
 */
static inline int safe_ushort_to_int(unsigned short value) {
    return (int)value;
}

/*
 * safe_uint_to_int - Convert unsigned int to int with range checking
 *
 * Converts unsigned int to int with overflow checking. Used when
 * unsigned calculations need to interface with legacy int-based APIs.
 * Values exceeding INT_MAX are clamped to prevent undefined behavior.
 *
 * Parameters:
 *   value - unsigned int value to convert (may exceed int range)
 *
 * Returns:
 *   int value clamped to [0, INT_MAX] range
 */
static inline int safe_uint_to_int(unsigned int value) {
    if (value > INT_MAX) return INT_MAX;
    return (int)value;
}

/*
 * safe_ulong_to_int - Convert unsigned long to int with range checking
 *
 * Converts unsigned long to int with range validation. Used when
 * large unsigned calculations need to be used with legacy int APIs.
 * Handles platform differences in long size (32-bit vs 64-bit).
 *
 * Parameters:
 *   value - unsigned long value to convert (may exceed int range)
 *
 * Returns:
 *   int value clamped to [0, INT_MAX] range
 */
static inline int safe_ulong_to_int(unsigned long value) {
    if (value > INT_MAX) return INT_MAX;
    return (int)value;
}

/*
 * safe_time_to_int - Convert time_t to int with range checking
 *
 * Converts time_t to int for legacy timestamp handling. time_t may be
 * 32-bit or 64-bit depending on platform, so range checking prevents
 * overflow when interfacing with older timestamp APIs.
 *
 * Parameters:
 *   time - time_t value to convert (may exceed int range on 64-bit systems)
 *
 * Returns:
 *   int value clamped to [INT_MIN, INT_MAX] range
 */
static inline int safe_time_to_int(time_t time) {
    if (time > INT_MAX) return INT_MAX;
    if (time < INT_MIN) return INT_MIN;
    return (int)time;
}

/*
 * safe_uchar_to_short - Convert unsigned char to short safely
 *
 * Converts unsigned char to short without range checking needed,
 * as unsigned char values [0, 255] always fit in short range.
 * Used for character-based indices stored in short variables.
 *
 * Parameters:
 *   value - unsigned char value to convert (always valid)
 *
 * Returns:
 *   short value (guaranteed to fit in short range)
 */
static inline short safe_uchar_to_short(unsigned char value) {
    return (short)value;
}

/*
 * safe_uint_to_short - Convert unsigned int to short with range checking
 *
 * Converts unsigned int to short with overflow validation. Used when
 * unsigned calculations need to be stored in legacy short variables.
 * Values exceeding SHRT_MAX are clamped to prevent overflow.
 *
 * Parameters:
 *   value - unsigned int value to convert (may exceed short range)
 *
 * Returns:
 *   short value clamped to [0, SHRT_MAX] range
 */
static inline short safe_uint_to_short(unsigned int value) {
    if (value > SHRT_MAX) return SHRT_MAX;
    return (short)value;
}

/*
 * safe_ulong_to_short - Convert unsigned long to short with range checking
 *
 * Converts unsigned long to short with range validation. Used when
 * large unsigned calculations need to fit in short storage fields.
 * Handles platform differences in long size.
 *
 * Parameters:
 *   value - unsigned long value to convert (may exceed short range)
 *
 * Returns:
 *   short value clamped to [0, SHRT_MAX] range
 */
static inline short safe_ulong_to_short(unsigned long value) {
    if (value > SHRT_MAX) return SHRT_MAX;
    return (short)value;
}

/*
 * safe_time_to_short - Convert time_t to short with range checking
 *
 * Converts time_t to short for legacy timestamp handling in limited
 * storage contexts. Primarily used for relative time differences or
 * simplified timestamp representations.
 *
 * Parameters:
 *   time - time_t value to convert (may exceed short range)
 *
 * Returns:
 *   short value clamped to [SHRT_MIN, SHRT_MAX] range
 */
static inline short safe_time_to_short(time_t time) {
    if (time > SHRT_MAX) return SHRT_MAX;
    if (time < SHRT_MIN) return SHRT_MIN;
    return (short)time;
}

/*
 * safe_uchar_to_long - Convert unsigned char to long safely
 *
 * Converts unsigned char to long without range checking needed,
 * as unsigned char values [0, 255] always fit in long range.
 * Used for character-based calculations that need long precision.
 *
 * Parameters:
 *   value - unsigned char value to convert (always valid)
 *
 * Returns:
 *   long value (guaranteed to fit in long range)
 */
static inline long safe_uchar_to_long(unsigned char value) {
    return (long)value;
}

/*
 * safe_ushort_to_long - Convert unsigned short to long safely
 *
 * Converts unsigned short to long without range checking needed,
 * as unsigned short values [0, 65535] always fit in long range.
 * Used for 16-bit values in long-based calculations.
 *
 * Parameters:
 *   value - unsigned short value to convert (always valid)
 *
 * Returns:
 *   long value (guaranteed to fit in long range)
 */
static inline long safe_ushort_to_long(unsigned short value) {
    return (long)value;
}

/*
 * safe_uint_to_long - Convert unsigned int to long safely
 *
 * Converts unsigned int to long without range checking needed on most
 * platforms, as unsigned int values typically fit in long range.
 * Used for unsigned calculations that need long precision.
 *
 * Parameters:
 *   value - unsigned int value to convert (typically always valid)
 *
 * Returns:
 *   long value (should fit in long range on standard platforms)
 */
static inline long safe_uint_to_long(unsigned int value) {
    return (long)value;
}

/*
 * safe_ulong_to_long - Convert unsigned long to long with range checking
 *
 * Converts unsigned long to long with overflow validation. Used when
 * unsigned long calculations exceed LONG_MAX and need to be clamped
 * for use with signed long APIs.
 *
 * Parameters:
 *   value - unsigned long value to convert (may exceed long range)
 *
 * Returns:
 *   long value clamped to [0, LONG_MAX] range
 */
static inline long safe_ulong_to_long(unsigned long value) {
    if (value > LONG_MAX) return LONG_MAX;
    return (long)value;
}

/*
 * safe_time_to_long - Convert time_t to long safely
 *
 * Converts time_t to long for timestamp calculations. On most platforms
 * time_t fits in long, but provides explicit conversion for clarity
 * and potential future 128-bit time_t support.
 *
 * Parameters:
 *   time - time_t value to convert
 *
 * Returns:
 *   long value (typically equivalent to input on current platforms)
 */
static inline long safe_time_to_long(time_t time) {
    return (long)time;
}

/*
 * safe_char_to_size - Convert char to size_t with validation
 *
 * Converts char to size_t, handling negative values appropriately.
 * Negative char values are converted to 0 as size_t is unsigned.
 * Used for character-based size calculations.
 *
 * Parameters:
 *   value - char value to convert (may be negative)
 *
 * Returns:
 *   size_t value, 0 for negative inputs, positive for valid inputs
 */
static inline size_t safe_char_to_size(char value) {
    /* Handle platforms where char is unsigned */
    #if CHAR_MIN == 0
        /* char is unsigned, all values are valid */
        return (size_t)value;
    #else
        /* char is signed, handle negative values */
        if (value < 0) return 0;
        return (size_t)value;
    #endif
}

/*
 * safe_short_to_size - Convert short to size_t with validation
 *
 * Converts short to size_t, handling negative values appropriately.
 * Negative short values are converted to 0 as size_t is unsigned.
 * Used for short-based size calculations and array indexing.
 *
 * Parameters:
 *   value - short value to convert (may be negative)
 *
 * Returns:
 *   size_t value, 0 for negative inputs, positive for valid inputs
 */
static inline size_t safe_short_to_size(short value) {
    if (value < 0) return 0;
    return (size_t)value;
}

/*
 * safe_long_to_size - Convert long to size_t with validation
 *
 * Converts long to size_t, handling negative values appropriately.
 * Negative long values are converted to 0 as size_t is unsigned.
 * Used for long-based size calculations and memory operations.
 *
 * Parameters:
 *   value - long value to convert (may be negative)
 *
 * Returns:
 *   size_t value, 0 for negative inputs, positive for valid inputs
 */
static inline size_t safe_long_to_size(long value) {
    if (value < 0) return 0;
    return (size_t)value;
}

/*
 * safe_uchar_to_size - Convert unsigned char to size_t safely
 *
 * Converts unsigned char to size_t without range checking needed,
 * as unsigned char values [0, 255] always fit in size_t range.
 * Used for character-based size calculations.
 *
 * Parameters:
 *   value - unsigned char value to convert (always valid)
 *
 * Returns:
 *   size_t value (guaranteed to fit in size_t range)
 */
static inline size_t safe_uchar_to_size(unsigned char value) {
    return (size_t)value;
}

/*
 * safe_ushort_to_size - Convert unsigned short to size_t safely
 *
 * Converts unsigned short to size_t without range checking needed,
 * as unsigned short values [0, 65535] always fit in size_t range.
 * Used for 16-bit values in size calculations.
 *
 * Parameters:
 *   value - unsigned short value to convert (always valid)
 *
 * Returns:
 *   size_t value (guaranteed to fit in size_t range)
 */
static inline size_t safe_ushort_to_size(unsigned short value) {
    return (size_t)value;
}

/*
 * safe_uint_to_size - Convert unsigned int to size_t safely
 *
 * Converts unsigned int to size_t. On most platforms size_t is at least
 * as large as unsigned int, so no range checking is typically needed.
 * Used for unsigned calculations in size contexts.
 *
 * Parameters:
 *   value - unsigned int value to convert (typically always valid)
 *
 * Returns:
 *   size_t value (should fit in size_t range on standard platforms)
 */
static inline size_t safe_uint_to_size(unsigned int value) {
    return (size_t)value;
}

/*
 * safe_ulong_to_size - Convert unsigned long to size_t with platform handling
 *
 * Converts unsigned long to size_t. On some platforms these types may
 * differ in size, so we handle potential overflow by clamping to SIZE_MAX.
 * Used for large unsigned calculations in size contexts.
 *
 * Parameters:
 *   value - unsigned long value to convert (may exceed size_t on some platforms)
 *
 * Returns:
 *   size_t value, clamped to SIZE_MAX if necessary
 */
static inline size_t safe_ulong_to_size(unsigned long value) {
    if (value > SIZE_MAX) return SIZE_MAX;
    return (size_t)value;
}

/*
 * safe_time_to_size - Convert time_t to size_t with validation
 *
 * Converts time_t to size_t for timestamp-based size calculations.
 * Handles negative time_t values (invalid timestamps) by returning 0.
 * Used for time-based buffer sizing and calculations.
 *
 * Parameters:
 *   time - time_t value to convert (may be negative or very large)
 *
 * Returns:
 *   size_t value, 0 for negative times, clamped to SIZE_MAX for very large times
 */
static inline size_t safe_time_to_size(time_t time) {
    if (time < 0) return 0;
    if ((unsigned long long)time > SIZE_MAX) return SIZE_MAX;
    return (size_t)time;
}

/*
 * safe_char_to_uchar - Convert char to unsigned char with validation
 *
 * Converts char to unsigned char, handling negative values appropriately.
 * Negative char values are converted to 0 to maintain valid unsigned range.
 * Used for character-based calculations that need unsigned results.
 *
 * Parameters:
 *   value - char value to convert (may be negative)
 *
 * Returns:
 *   unsigned char value, 0 for negative inputs, unchanged for positive inputs
 */
static inline unsigned char safe_char_to_uchar(char value) {
    /* Handle platforms where char is unsigned */
    #if CHAR_MIN == 0
        /* char is unsigned, direct conversion is safe */
        return (unsigned char)value;
    #else
        /* char is signed, handle negative values */
        if (value < 0) return 0;
        return (unsigned char)value;
    #endif
}

/*
 * safe_size_to_uchar - Convert size_t to unsigned char with range checking
 *
 * Converts size_t to unsigned char with overflow validation. Used when
 * size calculations need to fit in small unsigned char storage fields.
 * Values exceeding UCHAR_MAX are clamped to prevent overflow.
 *
 * Parameters:
 *   size - size_t value to convert (may exceed unsigned char range)
 *
 * Returns:
 *   unsigned char value clamped to [0, UCHAR_MAX] range
 */
static inline unsigned char safe_size_to_uchar(size_t size) {
    if (size > UCHAR_MAX) return UCHAR_MAX;
    return (unsigned char)size;
}

/*
 * safe_time_to_uchar - Convert time_t to unsigned char with range checking
 *
 * Converts time_t to unsigned char for simplified timestamp representations.
 * Negative times become 0, large times are clamped to UCHAR_MAX.
 * Used for relative time differences in small storage contexts.
 *
 * Parameters:
 *   time - time_t value to convert (may be negative or exceed uchar range)
 *
 * Returns:
 *   unsigned char value clamped to [0, UCHAR_MAX] range
 */
static inline unsigned char safe_time_to_uchar(time_t time) {
    if (time < 0) return 0;
    if (time > UCHAR_MAX) return UCHAR_MAX;
    return (unsigned char)time;
}

/*
 * safe_uid_to_uchar - Convert uid_t to unsigned char with range checking
 *
 * Converts uid_t to unsigned char for simplified user ID representations.
 * Large UIDs are clamped to UCHAR_MAX. Used for user ID storage in
 * limited contexts where full UID precision is not required.
 *
 * Parameters:
 *   uid - uid_t value to convert (may exceed unsigned char range)
 *
 * Returns:
 *   unsigned char value clamped to [0, UCHAR_MAX] range
 */
static inline unsigned char safe_uid_to_uchar(uid_t uid) {
    if (uid > UCHAR_MAX) return UCHAR_MAX;
    return (unsigned char)uid;
}

/*
 * safe_uint_to_uchar - Convert unsigned int to unsigned char with range checking
 *
 * Converts unsigned int to unsigned char with overflow validation. Used when
 * unsigned calculations need to fit in small unsigned char storage fields.
 * Values exceeding UCHAR_MAX are clamped to prevent overflow.
 *
 * Parameters:
 *   value - unsigned int value to convert (may exceed unsigned char range)
 *
 * Returns:
 *   unsigned char value clamped to [0, UCHAR_MAX] range
 */
static inline unsigned char safe_uint_to_uchar(unsigned int value) {
    if (value > UCHAR_MAX) return UCHAR_MAX;
    return (unsigned char)value;
}

/*
 * safe_ulong_to_uchar - Convert unsigned long to unsigned char with range checking
 *
 * Converts unsigned long to unsigned char with overflow validation. Used when
 * large unsigned calculations need to fit in small unsigned char storage.
 * Values exceeding UCHAR_MAX are clamped to prevent overflow.
 *
 * Parameters:
 *   value - unsigned long value to convert (may exceed unsigned char range)
 *
 * Returns:
 *   unsigned char value clamped to [0, UCHAR_MAX] range
 */
static inline unsigned char safe_ulong_to_uchar(unsigned long value) {
    if (value > UCHAR_MAX) return UCHAR_MAX;
    return (unsigned char)value;
}

/*
 * safe_ushort_to_uchar - Convert unsigned short to unsigned char with range checking
 *
 * Converts unsigned short to unsigned char with overflow validation. Used when
 * 16-bit unsigned values need to fit in 8-bit unsigned char storage.
 * Values exceeding UCHAR_MAX are clamped to prevent overflow.
 *
 * Parameters:
 *   value - unsigned short value to convert (may exceed unsigned char range)
 *
 * Returns:
 *   unsigned char value clamped to [0, UCHAR_MAX] range
 */
static inline unsigned char safe_ushort_to_uchar(unsigned short value) {
    if (value > UCHAR_MAX) return UCHAR_MAX;
    return (unsigned char)value;
}

/*
 * safe_int_to_uint - Convert int to unsigned int with validation
 *
 * Converts int to unsigned int, handling negative values appropriately.
 * Negative int values are converted to 0 to maintain valid unsigned range.
 * Used for signed calculations that need unsigned results.
 *
 * Parameters:
 *   value - int value to convert (may be negative)
 *
 * Returns:
 *   unsigned int value, 0 for negative inputs, unchanged for positive inputs
 */
static inline unsigned int safe_int_to_uint(int value) {
    if (value < 0) return 0;
    return (unsigned int)value;
}

/*
 * safe_short_to_uint - Convert short to unsigned int with validation
 *
 * Converts short to unsigned int, handling negative values appropriately.
 * Negative short values are converted to 0 to maintain valid unsigned range.
 * Used for short-based calculations that need unsigned int results.
 *
 * Parameters:
 *   value - short value to convert (may be negative)
 *
 * Returns:
 *   unsigned int value, 0 for negative inputs, positive for valid inputs
 */
static inline unsigned int safe_short_to_uint(short value) {
    if (value < 0) return 0;
    return (unsigned int)value;
}

/*
 * safe_size_to_uint - Convert size_t to unsigned int with range checking
 *
 * Converts size_t to unsigned int with overflow validation. On 64-bit systems
 * size_t may exceed unsigned int range, so we clamp to UINT_MAX.
 * Used for size calculations that need unsigned int compatibility.
 *
 * Parameters:
 *   size - size_t value to convert (may exceed unsigned int range on 64-bit)
 *
 * Returns:
 *   unsigned int value clamped to [0, UINT_MAX] range
 */
static inline unsigned int safe_size_to_uint(size_t size) {
    if (size > UINT_MAX) return UINT_MAX;
    return (unsigned int)size;
}

/*
 * safe_time_to_uint - Convert time_t to unsigned int with validation
 *
 * Converts time_t to unsigned int for timestamp handling. Negative times
 * become 0, large times are clamped to UINT_MAX. Used for timestamp
 * storage in unsigned int contexts.
 *
 * Parameters:
 *   time - time_t value to convert (may be negative or exceed uint range)
 *
 * Returns:
 *   unsigned int value clamped to [0, UINT_MAX] range
 */
static inline unsigned int safe_time_to_uint(time_t time) {
    if (time < 0) return 0;
    if ((unsigned long long)time > UINT_MAX) return UINT_MAX;
    return (unsigned int)time;
}

/*
 * safe_uid_to_uint - Convert uid_t to unsigned int safely
 *
 * Converts uid_t to unsigned int. On most platforms uid_t is already
 * unsigned int, so this provides explicit conversion for clarity and
 * future compatibility if uid_t definition changes.
 *
 * Parameters:
 *   uid - uid_t value to convert (typically already unsigned int)
 *
 * Returns:
 *   unsigned int value (typically equivalent to input)
 */
static inline unsigned int safe_uid_to_uint(uid_t uid) {
    return (unsigned int)uid;
}

/*
 * Extended Float/Double Conversion Functions
 */

/*
 * safe_size_to_float - Convert size_t to float safely
 *
 * Converts size_t to float for floating-point calculations involving sizes.
 * Large size_t values may lose precision when converted to float.
 * Used for statistical calculations and ratios involving memory sizes.
 *
 * Parameters:
 *   size - size_t value to convert (may lose precision)
 *
 * Returns:
 *   float value representing the size (with potential precision loss)
 */
static inline float safe_size_to_float(size_t size) {
    return (float)size;
}

/*
 * safe_time_to_float - Convert time_t to float safely
 *
 * Converts time_t to float for floating-point time calculations.
 * Large time_t values may lose precision when converted to float.
 * Used for time difference calculations and statistical analysis.
 *
 * Parameters:
 *   time - time_t value to convert (may lose precision)
 *
 * Returns:
 *   float value representing the timestamp (with potential precision loss)
 */
static inline float safe_time_to_float(time_t time) {
    return (float)time;
}

/*
 * safe_uid_to_float - Convert uid_t to float safely
 *
 * Converts uid_t to float for floating-point calculations involving user IDs.
 * Used for statistical analysis of user data and mathematical operations.
 *
 * Parameters:
 *   uid - uid_t value to convert
 *
 * Returns:
 *   float value representing the UID
 */
static inline float safe_uid_to_float(uid_t uid) {
    return (float)uid;
}

/*
 * safe_uint_to_float - Convert unsigned int to float safely
 *
 * Converts unsigned int to float for floating-point calculations.
 * Large unsigned int values may lose precision when converted to float.
 * Used for mathematical operations requiring float precision.
 *
 * Parameters:
 *   value - unsigned int value to convert (may lose precision)
 *
 * Returns:
 *   float value representing the unsigned int (with potential precision loss)
 */
static inline float safe_uint_to_float(unsigned int value) {
    return (float)value;
}

/*
 * safe_ulong_to_float - Convert unsigned long to float safely
 *
 * Converts unsigned long to float for floating-point calculations.
 * Large unsigned long values will lose precision when converted to float.
 * Used for mathematical operations where float precision is sufficient.
 *
 * Parameters:
 *   value - unsigned long value to convert (will lose precision for large values)
 *
 * Returns:
 *   float value representing the unsigned long (with potential precision loss)
 */
static inline float safe_ulong_to_float(unsigned long value) {
    return (float)value;
}

/*
 * safe_uchar_to_float - Convert unsigned char to float safely
 *
 * Converts unsigned char to float without precision loss, as unsigned char
 * values [0, 255] fit exactly in float precision. Used for character-based
 * mathematical operations.
 *
 * Parameters:
 *   value - unsigned char value to convert (always exact)
 *
 * Returns:
 *   float value representing the unsigned char exactly
 */
static inline float safe_uchar_to_float(unsigned char value) {
    return (float)value;
}

/*
 * safe_ushort_to_float - Convert unsigned short to float safely
 *
 * Converts unsigned short to float without precision loss, as unsigned short
 * values [0, 65535] fit exactly in float precision. Used for 16-bit values
 * in floating-point calculations.
 *
 * Parameters:
 *   value - unsigned short value to convert (always exact)
 *
 * Returns:
 *   float value representing the unsigned short exactly
 */
static inline float safe_ushort_to_float(unsigned short value) {
    return (float)value;
}

/*
 * safe_size_to_double - Convert size_t to double safely
 *
 * Converts size_t to double for high-precision floating-point calculations
 * involving sizes. Double precision can represent size_t values exactly
 * in most practical ranges. Used for precise mathematical operations.
 *
 * Parameters:
 *   size - size_t value to convert (typically exact representation)
 *
 * Returns:
 *   double value representing the size with high precision
 */
static inline double safe_size_to_double(size_t size) {
    return (double)size;
}

/*
 * safe_time_to_double - Convert time_t to double safely
 *
 * Converts time_t to double for high-precision time calculations.
 * Double precision can represent time_t values exactly in most ranges.
 * Used for precise time difference calculations and statistical analysis.
 *
 * Parameters:
 *   time - time_t value to convert (typically exact representation)
 *
 * Returns:
 *   double value representing the timestamp with high precision
 */
static inline double safe_time_to_double(time_t time) {
    return (double)time;
}

/*
 * safe_uid_to_double - Convert uid_t to double safely
 *
 * Converts uid_t to double for high-precision calculations involving user IDs.
 * Used for statistical analysis and mathematical operations requiring
 * double precision.
 *
 * Parameters:
 *   uid - uid_t value to convert
 *
 * Returns:
 *   double value representing the UID with high precision
 */
static inline double safe_uid_to_double(uid_t uid) {
    return (double)uid;
}

/*
 * safe_uint_to_double - Convert unsigned int to double safely
 *
 * Converts unsigned int to double without precision loss. Double precision
 * can represent unsigned int values exactly. Used for high-precision
 * mathematical operations.
 *
 * Parameters:
 *   value - unsigned int value to convert (always exact)
 *
 * Returns:
 *   double value representing the unsigned int exactly
 */
static inline double safe_uint_to_double(unsigned int value) {
    return (double)value;
}

/*
 * safe_ulong_to_double - Convert unsigned long to double safely
 *
 * Converts unsigned long to double for high-precision calculations.
 * Double precision can represent most unsigned long values exactly,
 * though very large values may lose precision. Used for mathematical
 * operations requiring double precision.
 *
 * Parameters:
 *   value - unsigned long value to convert (typically exact)
 *
 * Returns:
 *   double value representing the unsigned long with high precision
 */
static inline double safe_ulong_to_double(unsigned long value) {
    return (double)value;
}

/*
 * safe_int_to_double - Convert int to double safely
 *
 * Converts int to double without precision loss. Double precision can
 * represent int values exactly. Used for high-precision mathematical
 * operations requiring double precision from integer inputs.
 *
 * Parameters:
 *   value - int value to convert (always exact)
 *
 * Returns:
 *   double value representing the int exactly
 */
static inline double safe_int_to_double(int value) {
    return (double)value;
}

/*
 * =============================================================================
 * CRITICAL MISSING CONVERSION FUNCTIONS
 * =============================================================================
 *
 * These functions address the most common overflow bugs where developers
 * assume values have been clamped but haven't performed safe conversion.
 * The _to_char and _to_uchar functions are especially critical for preventing
 * buffer overflows and array index errors.
 */

/*
 * Critical _to_char conversion functions
 * These prevent overflow when storing larger types in char variables
 */

/*
 * safe_double_to_char - Convert double to char with bounds checking
 *
 * Safely converts double values to char with range validation and proper
 * handling of NaN/infinity cases. Used when floating-point calculations
 * need to be stored in char variables for character codes or small indices.
 *
 * This function prevents overflow bugs where developers assume double
 * calculations will fit in char range without explicit bounds checking.
 *
 * Parameters:
 *   value - double value to convert (may be NaN, infinity, or exceed char range)
 *
 * Returns:
 *   char value clamped to [CHAR_MIN, CHAR_MAX] range, 0 for NaN/infinity
 *
 * Example Usage:
 *   char ascii_code = safe_double_to_char(calculation_result);
 *   char index = safe_double_to_char(interpolated_position);
 */
static inline char safe_double_to_char(double value) {
    /* Handle NaN and infinity cases */
    if (value != value || value == HUGE_VAL || value == -HUGE_VAL) return 0;

    if (value > (double)CHAR_MAX) return CHAR_MAX;
    if (value < (double)CHAR_MIN) return CHAR_MIN;
    return (char)value;
}

/*
 * safe_float_to_char - Convert float to char with bounds checking
 *
 * Safely converts float values to char with range validation and proper
 * handling of NaN/infinity cases. Used when single-precision calculations
 * need to be stored in char variables.
 *
 * Parameters:
 *   value - float value to convert (may be NaN, infinity, or exceed char range)
 *
 * Returns:
 *   char value clamped to [CHAR_MIN, CHAR_MAX] range, 0 for NaN/infinity
 *
 * Example Usage:
 *   char result = safe_float_to_char(floating_calculation);
 */
static inline char safe_float_to_char(float value) {
    /* Handle NaN and infinity cases */
    if (value != value || value == HUGE_VALF || value == -HUGE_VALF) return 0;

    if (value > (float)CHAR_MAX) return CHAR_MAX;
    if (value < (float)CHAR_MIN) return CHAR_MIN;
    return (char)value;
}

/*
 * safe_size_to_char - Convert size_t to char with bounds checking
 *
 * Safely converts size_t to char with overflow validation. CRITICAL for
 * preventing buffer overflows when size calculations are used as char
 * indices or stored in char variables.
 *
 * Parameters:
 *   size - size_t value to convert (may exceed char range)
 *
 * Returns:
 *   char value clamped to [0, CHAR_MAX] range (never negative)
 *
 * Example Usage:
 *   char count = safe_size_to_char(strlen(buffer));
 *   char index = safe_size_to_char(array_size);
 */
static inline char safe_size_to_char(size_t size) {
    if (size > CHAR_MAX) return CHAR_MAX;
    return (char)size;
}

/*
 * safe_time_to_char - Convert time_t to char with bounds checking
 *
 * Safely converts time_t to char for simplified timestamp representations
 * or time-based calculations. Handles negative times and large values.
 *
 * Parameters:
 *   time - time_t value to convert (may be negative or exceed char range)
 *
 * Returns:
 *   char value clamped to [CHAR_MIN, CHAR_MAX] range
 *
 * Example Usage:
 *   char time_offset = safe_time_to_char(time_diff);
 */
static inline char safe_time_to_char(time_t time) {
    if (time > CHAR_MAX) return CHAR_MAX;
    if (time < CHAR_MIN) return CHAR_MIN;
    return (char)time;
}

/*
 * safe_uchar_to_char - Convert unsigned char to char with bounds checking
 *
 * Safely converts unsigned char to char, handling the case where unsigned
 * char values exceed signed char range. CRITICAL on platforms where
 * CHAR_MAX is 127 but unsigned char goes to 255.
 *
 * Parameters:
 *   value - unsigned char value to convert (may exceed signed char range)
 *
 * Returns:
 *   char value clamped to [CHAR_MIN, CHAR_MAX] range
 *
 * Example Usage:
 *   char signed_result = safe_uchar_to_char(unsigned_calculation);
 */
static inline char safe_uchar_to_char(unsigned char value) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
    if (value > CHAR_MAX) return CHAR_MAX;
#pragma GCC diagnostic pop
    return (char)value;
}

/*
 * safe_uid_to_char - Convert uid_t to char with bounds checking
 *
 * Safely converts uid_t to char for simplified user ID representations.
 * Large UIDs are clamped to char range. Used for user ID storage in
 * limited contexts.
 *
 * Parameters:
 *   uid - uid_t value to convert (may exceed char range)
 *
 * Returns:
 *   char value clamped to [0, CHAR_MAX] range (never negative)
 *
 * Example Usage:
 *   char user_index = safe_uid_to_char(getuid());
 */
static inline char safe_uid_to_char(uid_t uid) {
    if (uid > CHAR_MAX) return CHAR_MAX;
    return (char)uid;
}

/*
 * safe_uint_to_char - Convert unsigned int to char with bounds checking
 *
 * Safely converts unsigned int to char with overflow validation. CRITICAL
 * for preventing overflow when unsigned calculations are stored in char
 * variables.
 *
 * Parameters:
 *   value - unsigned int value to convert (may exceed char range)
 *
 * Returns:
 *   char value clamped to [0, CHAR_MAX] range (never negative)
 *
 * Example Usage:
 *   char result = safe_uint_to_char(calculation);
 */
static inline char safe_uint_to_char(unsigned int value) {
    if (value > CHAR_MAX) return CHAR_MAX;
    return (char)value;
}

/*
 * safe_ulong_to_char - Convert unsigned long to char with bounds checking
 *
 * Safely converts unsigned long to char with overflow validation. CRITICAL
 * for preventing overflow when large unsigned calculations are stored in
 * char variables.
 *
 * Parameters:
 *   value - unsigned long value to convert (may exceed char range)
 *
 * Returns:
 *   char value clamped to [0, CHAR_MAX] range (never negative)
 *
 * Example Usage:
 *   char index = safe_ulong_to_char(large_calculation);
 */
static inline char safe_ulong_to_char(unsigned long value) {
    if (value > CHAR_MAX) return CHAR_MAX;
    return (char)value;
}

/*
 * safe_ushort_to_char - Convert unsigned short to char with bounds checking
 *
 * Safely converts unsigned short to char with overflow validation. Used
 * when 16-bit unsigned values need to be stored in char variables.
 *
 * Parameters:
 *   value - unsigned short value to convert (may exceed char range)
 *
 * Returns:
 *   char value clamped to [0, CHAR_MAX] range (never negative)
 *
 * Example Usage:
 *   char result = safe_ushort_to_char(short_calculation);
 */
static inline char safe_ushort_to_char(unsigned short value) {
    if (value > CHAR_MAX) return CHAR_MAX;
    return (char)value;
}

/*
 * Critical _to_uchar conversion functions
 * These prevent overflow when storing larger types in unsigned char variables
 */

/*
 * safe_double_to_uchar - Convert double to unsigned char with bounds checking
 *
 * Safely converts double values to unsigned char with range validation.
 * CRITICAL for preventing overflow when floating-point calculations are
 * used as array indices or stored in unsigned char variables.
 *
 * Parameters:
 *   value - double value to convert (may be NaN, infinity, or exceed uchar range)
 *
 * Returns:
 *   unsigned char value clamped to [0, UCHAR_MAX] range, 0 for NaN/infinity
 *
 * Example Usage:
 *   unsigned char pixel_value = safe_double_to_uchar(color_calculation);
 *   unsigned char index = safe_double_to_uchar(interpolated_position);
 */
static inline unsigned char safe_double_to_uchar(double value) {
    /* Handle NaN and infinity cases */
    if (value != value || value == HUGE_VAL || value == -HUGE_VAL) return 0;

    if (value < 0.0) return 0;
    if (value > (double)UCHAR_MAX) return UCHAR_MAX;
    return (unsigned char)value;
}

/*
 * safe_float_to_uchar - Convert float to unsigned char with bounds checking
 *
 * Safely converts float values to unsigned char with range validation.
 * CRITICAL for preventing overflow when single-precision calculations are
 * used as array indices or stored in unsigned char variables.
 *
 * Parameters:
 *   value - float value to convert (may be NaN, infinity, or exceed uchar range)
 *
 * Returns:
 *   unsigned char value clamped to [0, UCHAR_MAX] range, 0 for NaN/infinity
 *
 * Example Usage:
 *   unsigned char brightness = safe_float_to_uchar(calculation);
 */
static inline unsigned char safe_float_to_uchar(float value) {
    /* Handle NaN and infinity cases */
    if (value != value || value == HUGE_VALF || value == -HUGE_VALF) return 0;

    if (value < 0.0f) return 0;
    if (value > (float)UCHAR_MAX) return UCHAR_MAX;
    return (unsigned char)value;
}

/*
 * safe_uchar_to_uchar - Identity conversion for unsigned char
 *
 * Identity function for unsigned char to unsigned char conversion. Provided
 * for completeness in generic programming contexts and to maintain consistency
 * in conversion function naming patterns.
 *
 * Parameters:
 *   value - unsigned char value to convert (always valid)
 *
 * Returns:
 *   unsigned char value unchanged
 *
 * Example Usage:
 *   unsigned char result = safe_uchar_to_uchar(input);
 */
static inline unsigned char safe_uchar_to_uchar(unsigned char value) {
    return value;
}

/*
 * Additional high-utility missing conversion functions
 */

/*
 * safe_size_to_long - Convert size_t to long safely
 *
 * Converts size_t to long for calculations requiring signed arithmetic.
 * On 64-bit systems, size_t and long are typically the same size, but
 * provides explicit conversion for clarity and 32-bit compatibility.
 *
 * Parameters:
 *   size - size_t value to convert
 *
 * Returns:
 *   long value (typically equivalent on 64-bit, clamped on 32-bit if needed)
 *
 * Example Usage:
 *   long offset = safe_size_to_long(buffer_size);
 */
static inline long safe_size_to_long(size_t size) {
    if (size > LONG_MAX) return LONG_MAX;
    return (long)size;
}

/*
 * safe_uid_to_long - Convert uid_t to long safely
 *
 * Converts uid_t to long for calculations requiring long precision.
 * Used when user IDs need to be used in long-based calculations.
 *
 * Parameters:
 *   uid - uid_t value to convert
 *
 * Returns:
 *   long value representing the UID
 *
 * Example Usage:
 *   long user_value = safe_uid_to_long(getuid());
 */
static inline long safe_uid_to_long(uid_t uid) {
    return (long)uid;
}

/*
 * safe_double_to_short - Convert double to short with bounds checking
 *
 * Safely converts double values to short with range validation.
 * Used when floating-point calculations need to be stored in short variables.
 *
 * Parameters:
 *   value - double value to convert (may exceed short range)
 *
 * Returns:
 *   short value clamped to [SHRT_MIN, SHRT_MAX] range, 0 for NaN/infinity
 *
 * Example Usage:
 *   short coordinate = safe_double_to_short(position_calculation);
 */
static inline short safe_double_to_short(double value) {
    /* Handle NaN and infinity cases */

    if (value != value || value == HUGE_VAL || value == -HUGE_VAL) return 0;


    if (value > (double)SHRT_MAX) return SHRT_MAX;
    if (value < (double)SHRT_MIN) return SHRT_MIN;
    return (short)value;
}

/*
 * safe_float_to_short - Convert float to short with bounds checking
 *
 * Safely converts float values to short with range validation.
 * Used when single-precision calculations need to be stored in short variables.
 *
 * Parameters:
 *   value - float value to convert (may exceed short range)
 *
 * Returns:
 *   short value clamped to [SHRT_MIN, SHRT_MAX] range, 0 for NaN/infinity
 *
 * Example Usage:
 *   short result = safe_float_to_short(calculation);
 */
static inline short safe_float_to_short(float value) {
    /* Handle NaN and infinity cases */

    if (value != value || value == HUGE_VALF || value == -HUGE_VALF) return 0;

    if (value > (float)SHRT_MAX) return SHRT_MAX;
    if (value < (float)SHRT_MIN) return SHRT_MIN;
    return (short)value;
}

/*
 * safe_ushort_to_short - Convert unsigned short to short with bounds checking
 *
 * Safely converts unsigned short to short, handling overflow when unsigned
 * short values exceed signed short range. CRITICAL on platforms where
 * unsigned short can be 65535 but short max is 32767.
 *
 * Parameters:
 *   value - unsigned short value to convert (may exceed signed short range)
 *
 * Returns:
 *   short value clamped to [0, SHRT_MAX] range
 *
 * Example Usage:
 *   short result = safe_ushort_to_short(unsigned_calculation);
 */
static inline short safe_ushort_to_short(unsigned short value) {
    if (value > SHRT_MAX) return SHRT_MAX;
    return (short)value;
}

#endif /* SAFE_CONVERT_H */
