/*
 * patchlevel.h - Version and patch level definitions for Conquer system
 *
 * This file is part of Conquer.
 * Originally Copyright (C) 1988-1989 by Edward M. Barlow and Adam Bryant
 * Copyright (C) 2025 Juan Manuel Méndez Rey (Vejeta) - Licensed under GPL v3 with permission from original authors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * OVERVIEW:
 * ========
 * This header defines version and patch level information for the Conquer
 * game system. It provides a centralized location for version tracking
 * that can be included by various components of the system.
 *
 * USAGE:
 * =====
 * This file is typically included by source files that need to display
 * version information, generate version strings, or perform version-dependent
 * logic. The PATCHLEVEL macro can be used in version display functions,
 * about dialogs, or compatibility checks.
 *
 * HISTORICAL CONTEXT:
 * ==================
 * The patch level "12" represents the final known version from the original
 * 1988-1989 development period. This numbering scheme was common in early
 * Unix software development, where patch levels tracked incremental fixes
 * and improvements to a base version.
 *
 * MODERNIZATION NOTES:
 * ===================
 * In modern software development, this would typically be replaced with
 * a more comprehensive versioning system (semantic versioning, build
 * automation, etc.). However, this simple approach was standard practice
 * for the era and serves its purpose effectively.
 */

/*
 * VERSION DEFINITIONS - Conquer System Version Information
 * =======================================================
 * Central version tracking for the entire Conquer game system.
 */

#define PATCHLEVEL	"12"    /* Final patch level from original 1988-1989 development */
                                /* Used for version display and compatibility identification */
