/*
 * sunconqrast.h - Sun workstation graphics definitions and platform abstraction
 *
 * This file is part of Conquer Utilities.
 * Originally Copyright (C) 1989 by Richard Caley
 * Copyright (C) 2025 Juan Manuel Méndez Rey (Vejeta) - Licensed under GPL v3 with permission from original license
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
 * This header provides Sun workstation-specific graphics definitions using
 * the SunView pixrect library. It represents the primary, fully-implemented
 * graphics backend for the conqrast map visualization tool, contrasting with
 * the incomplete X11 implementation in xconqrast.h.
 *
 * PLATFORM ABSTRACTION:
 * ====================
 * This file implements the complete graphics abstraction layer for Sun
 * workstations using the pixrect library. All graphics operations are
 * mapped to equivalent pixrect functions, providing a comprehensive and
 * working implementation of the graphics backend interface.
 *
 * IMPLEMENTATION STATUS:
 * =====================
 * Unlike the X11 counterpart, this header provides complete implementations
 * for all graphics operations using Sun's pixrect library. This was likely
 * the primary development platform in 1989, with X11 support added later
 * as a secondary target.
 *
 * HISTORICAL CONTEXT:
 * ==================
 * Represents mature Sun workstation graphics programming from the late 1980s,
 * using the pixrect library which was Sun's primary graphics API before the
 * adoption of X11. This shows the state-of-the-art Unix graphics programming
 * on Sun hardware during the peak of Sun's influence in the workstation market.
 */

/*
 * VERSION AND IDENTIFICATION CONSTANTS
 * ===================================
 * Driver identification for the Sun pixrect backend implementation.
 */
#define driver_version "1"              /* Sun pixrect driver version */
#define conqrast_name "Sunconqrast"     /* Platform-specific tool name */

/*
 * SUN PIXRECT LIBRARY INTEGRATION
 * ===============================
 * Includes the comprehensive Sun pixrect graphics library which provides
 * all fundamental graphics operations for Sun workstations.
 */
#include <pixrect/pixrect_hs.h>

/*
 * TYPE DEFINITIONS - Sun Pixrect Abstraction
 * ==========================================
 * Maps generic graphics types to Sun-specific pixrect structures.
 * These provide a clean abstraction layer for conqrast.c.
 */
typedef struct pixrect * bitmap;        /* Pixrect structure for bitmap operations */
typedef char * screen_type;             /* Device path string (e.g., "/dev/fb") */
typedef struct pixfont * font;          /* Pixfont structure for text rendering */

/*
 * INITIALIZATION AND CLEANUP MACROS - Empty Implementation
 * ========================================================
 * Sun pixrect library handles initialization automatically, so these
 * are defined as empty operations for API compatibility.
 */
#define initialise_bitmaps() /* EMPTY */
    /* No explicit initialization needed for pixrect system */
#define finish_bitmaps() /* EMPTY */
    /* No explicit cleanup needed - handled by process termination */

/*
 * DEVICE AND SCREEN MANAGEMENT - Sun Framebuffer Interface
 * ========================================================
 * Screen and device handling using Sun's framebuffer device model.
 * Uses /dev/fb as the standard framebuffer device path.
 */
#define is_screenname(mapname) (!strncmp(mapname,"/dev/",5))
    /* Check if name refers to a device (starts with "/dev/") */

#define get_default_screen_name() "/dev/fb"
    /* Default Sun framebuffer device path */

#define screen_named(name) (name)
    /* Screen names are device paths - no transformation needed */

#define get_screen_bitmap(screen) (pr_open(screen))
    /* Open framebuffer device as pixrect for direct screen access */

/*
 * BITMAP LIFECYCLE MANAGEMENT - Pixrect Memory Operations
 * =======================================================
 * Bitmap creation, destruction, and memory management using pixrect APIs.
 */
#define create_bitmap(width,height) (mem_create(width,height,1))
    /* Create in-memory pixrect with specified dimensions (1-bit depth) */

#define destroy_bitmap(bitmap) pr_close(bitmap)
    /* Properly close and free pixrect resources */

/*
 * FONT SYSTEM INTEGRATION - Sun Pixfont Support
 * =============================================
 * Complete font handling using Sun's pixfont system with proper
 * metric calculations and resource management.
 */
#define font_named(name) pf_open(name)
    /* Load font by name using pixfont system */

#define bad_font(font) ((font)==NULL)
    /* Simple NULL check for font loading validation */

#define font_height(font) ((font->pf_defaultsize).y)
    /* Extract font height from pixfont default size metrics */

#define font_width(font) ((font->pf_defaultsize).x)
    /* Extract font width from pixfont default size metrics */

#define font_baseline(font) (0-(font->pf_char)['A'].pc_home.y+1)
    /* Calculate baseline offset using 'A' character home position */

/*
 * COLOR DEFINITIONS - Pixrect Raster Operations
 * =============================================
 * Color constants using Sun pixrect raster operation (ROP) definitions
 * for proper color handling and blending operations.
 */
#define BLACK PIX_SET                   /* Set pixels (foreground color) */
#define WHITE PIX_CLR                   /* Clear pixels (background color) */
#define DRAW_BLACK (PIX_DST|PIX_SRC)    /* OR operation for black text drawing */
#define DRAW_WHITE (PIX_DST&PIX_NOT(PIX_SRC)) /* AND-NOT operation for white text */

/*
 * DRAWING PRIMITIVES - Direct Pixrect Operations
 * ==============================================
 * Pixel and line drawing operations mapped directly to pixrect functions
 * for optimal performance and full feature utilization.
 */
#define set_bit(bitmap,x,y,val) pr_put(bitmap,x,y,val)
    /* Set individual pixel using pixrect pr_put function */

#define draw_line(bitmap,x1,y1,x2,y2,colour) pr_vector(bitmap,x1,y1,x2,y2,colour,0)
    /* Draw line using pixrect vector function with solid line style (0) */
