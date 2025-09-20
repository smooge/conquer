/*
 * sunstuff.c - Sun workstation specific graphics code
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
 */

#include <stdio.h>
#include "sunconqrast.h"

struct				/* maps from size of map to the font */
				/* to use */
    {				/* if size > mag use font */
    int mag;
    char *font,*bold;
    } fonts[] =
        {
	9,  "/usr/lib/fonts/fixedwidthfonts/screen.r.7", NULL,
	11, "/usr/lib/fonts/fixedwidthfonts/cour.r.10",
	    "/usr/lib/fonts/fixedwidthfonts/cour.b.10",
	12, "/usr/lib/fonts/fixedwidthfonts/cour.r.12",
	    "/usr/lib/fonts/fixedwidthfonts/cour.b.12",
	14, "/usr/lib/fonts/fixedwidthfonts/cour.r.14",
	    "/usr/lib/fonts/fixedwidthfonts/cour.b.14",
	16, "/usr/lib/fonts/fixedwidthfonts/cour.r.16",
	    "/usr/lib/fonts/fixedwidthfonts/cour.b.16",
	18, "/usr/lib/fonts/fixedwidthfonts/cour.r.18",
	    "/usr/lib/fonts/fixedwidthfonts/cour.b.18",
	22, "/usr/lib/fonts/fixedwidthfonts/cour.r.24",
	    "/usr/lib/fonts/fixedwidthfonts/cour.b.24",
	999999, NULL
	};

static int encoding_type = RT_STANDARD ;

/*
 * argument_parse - Parse command line arguments for bitmap encoding options
 *
 * Processes command line arguments to set bitmap encoding type for Sun rasterfile
 * output. Currently supports the "-compact" option which switches from standard
 * encoding to byte-encoded format for more compact rasterfile output.
 *
 * This function is designed to be called iteratively for each command line
 * argument, returning 1 when it recognizes and processes an argument, 0 when
 * it doesn't recognize the argument (allowing other parsers to handle it).
 *
 * Parameters:
 *   argv - Pointer to array of command line argument strings (expects argv[0] to be current arg)
 *
 * Returns:
 *   1 if argument was recognized and processed (caller should advance argv)
 *   0 if argument was not recognized (caller should try other parsers)
 *
 * Side Effects:
 *   - Modifies global encoding_type variable when "-compact" is recognized
 *   - Sets encoding_type to RT_BYTE_ENCODED for compact rasterfile format
 *
 * Testing Notes:
 *   Category: A (Unit Testable) - Simple argument parsing with global state
 *   Approach: Unit tests with mocked global state and string comparison
 *   Key Tests: "-compact" recognition, case sensitivity, return values, state changes
 *   Dependencies: Global encoding_type variable, strcmp function
 *   Mock Requirements: None (standard library functions only)
 *   Complexity: Simple - Basic string comparison and global variable modification
 *
 * Notes:
 *   - Uses legacy K&R function declaration style
 *   - Designed for iterative parsing (typical 1989 Unix command line pattern)
 *   - Only recognizes "-compact" option, extensible for additional options
 *   - Global variable usage typical of legacy Sun graphics programming
 *   - RT_STANDARD and RT_BYTE_ENCODED constants defined in sunconqrast.h
 */
int
argument_parse(argv)

char *argv[];

{
if ( !strcmp(*argv,"-compact"))
    {
    encoding_type = RT_BYTE_ENCODED ;
    return 1;
    }
else
    return 0;
}

/*
 * write_bitmap - Write bitmap to Sun rasterfile format
 *
 * Writes a bitmap to a file in Sun rasterfile format using the Sun pixrect
 * library pr_dump function. The encoding type is determined by the global
 * encoding_type variable which can be set via argument_parse().
 *
 * The function opens the specified output file for writing and calls pr_dump
 * to perform the actual rasterfile output. Uses ioerror() for file opening
 * errors, providing consistent error reporting with program termination.
 *
 * Parameters:
 *   bm - Sun pixrect bitmap structure to write to file
 *   file - Output filename (null-terminated string)
 *   width - Width of bitmap in pixels (currently unused - ARGSUSED)
 *   height - Height of bitmap in pixels (currently unused - ARGSUSED)
 *
 * Returns:
 *   Return value from pr_dump() function (success/failure status)
 *   Typically 0 on success, non-zero on error
 *
 * Side Effects:
 *   - Creates or overwrites specified file
 *   - Terminates program via ioerror() if file cannot be opened
 *   - Uses global encoding_type for rasterfile format selection
 *
 * Testing Notes:
 *   Category: E (Skip/Platform Specific) - Requires Sun pixrect library and file system
 *   Approach: Integration testing on Sun platform or mock Sun pixrect library
 *   Key Tests: File creation, error handling, bitmap format output, encoding types
 *   Dependencies: Sun pixrect library (pr_dump), file system, ioerror function
 *   Mock Requirements: Complete Sun pixrect API simulation, file system mocking
 *   Complexity: Moderate - Platform-specific graphics API with file I/O
 *
 * Notes:
 *   - ARGSUSED comment indicates width/height parameters are unused by design
 *   - Uses legacy K&R function declaration style
 *   - Sun-specific pixrect library function pr_dump for rasterfile output
 *   - RMT_NONE parameter indicates no colormap transformation
 *   - Global encoding_type affects output format (RT_STANDARD or RT_BYTE_ENCODED)
 *   - File pointer not explicitly closed (pr_dump may handle this)
 */
/*ARGSUSED*/
int
write_bitmap(bm,file,width,height)

bitmap bm;
char *file;
int width,height;

{
    FILE *f;

    if ((f=fopen(file,"w"))==NULL)
	    ioerror("Can't open output file %s",file);

    return pr_dump( bm, f, RMT_NONE, encoding_type,0);
    }



/*
 * display_bitmap - Display bitmap on Sun workstation framebuffer
 *
 * Displays a bitmap directly to the Sun workstation framebuffer device (/dev/fb)
 * using the Sun pixrect library. This provides immediate visual output to the
 * screen at the specified coordinates and dimensions.
 *
 * Opens the framebuffer device and performs a raster operation (ROP) to copy
 * the bitmap data to the screen. Uses PIX_SRC mode for direct pixel copy
 * without blending or other effects.
 *
 * Parameters:
 *   bm - Sun pixrect bitmap structure to display
 *   x - X coordinate on screen for bitmap placement
 *   y - Y coordinate on screen for bitmap placement
 *   w - Width of region to display (pixels)
 *   h - Height of region to display (pixels)
 *
 * Returns:
 *   void (no return value)
 *
 * Side Effects:
 *   - Opens framebuffer device /dev/fb
 *   - Modifies screen display immediately
 *   - May fail silently if framebuffer device unavailable
 *   - Screen pixrect remains open (potential resource leak)
 *
 * Testing Notes:
 *   Category: E (Skip/Platform Specific) - Requires Sun framebuffer and graphics hardware
 *   Approach: Integration testing on Sun platform or complete graphics system mock
 *   Key Tests: Framebuffer access, coordinate handling, bitmap display, error conditions
 *   Dependencies: Sun framebuffer device (/dev/fb), pixrect library (pr_open, pr_rop)
 *   Mock Requirements: Complete Sun graphics system simulation including device files
 *   Complexity: Moderate - Platform-specific direct hardware access
 *
 * Notes:
 *   - Direct framebuffer access typical of 1989 Sun workstation programming
 *   - No error checking on pr_open() - assumes framebuffer always available
 *   - Uses PIX_SRC raster operation for straightforward pixel copy
 *   - Source coordinates (0,0) implies copying entire bitmap
 *   - Screen pixrect not explicitly closed (resource management issue)
 *   - Immediate display without double buffering or refresh synchronization
 */
void
display_bitmap(bm,x,y,w,h)

bitmap bm;
int x,y,w,h;

{
    struct pixrect *screen=pr_open("/dev/fb");

    pr_rop(screen,x,y,w,h,PIX_SRC,bm,0,0);

    }

/*
 * bitmap_text - Render text to bitmap using Sun font system
 *
 * Renders text string to a bitmap using the Sun pixrect font system.
 * Sets up a pixrect position structure and calls pf_text() to perform
 * the actual text rendering with the specified font and color.
 *
 * This function serves as a wrapper around the Sun font rendering system,
 * providing a simplified interface for text rendering to bitmaps. The
 * position structure is configured with the target bitmap and coordinates.
 *
 * Parameters:
 *   pr - Target bitmap (pixrect) for text rendering
 *   x - X coordinate for text baseline positioning
 *   y - Y coordinate for text baseline positioning
 *   colour - Color value for text rendering (pixrect color format)
 *   pf - Font structure (Sun pixfont) to use for rendering
 *   str - Null-terminated text string to render
 *
 * Returns:
 *   void (no return value, but function signature suggests it should return something)
 *
 * Side Effects:
 *   - Modifies target bitmap with rendered text
 *   - Uses Sun font system for glyph rendering
 *   - Text positioning relative to baseline coordinates
 *
 * Testing Notes:
 *   Category: E (Skip/Platform Specific) - Requires Sun font system and pixrect library
 *   Approach: Integration testing on Sun platform or complete font system mock
 *   Key Tests: Text rendering, font handling, coordinate positioning, color application
 *   Dependencies: Sun pixrect library (pf_text), font system, bitmap structures
 *   Mock Requirements: Complete Sun font system simulation with glyph rendering
 *   Complexity: Moderate - Platform-specific font rendering with coordinate system
 *
 * Notes:
 *   - Missing return type in function declaration (should be void or int)
 *   - Uses legacy K&R function declaration style
 *   - pr_prpos structure setup pattern typical of Sun pixrect programming
 *   - Font rendering coordinates are baseline-relative (not top-left)
 *   - No bounds checking or error handling for text overflow
 *   - Depends on fonts[] array defined earlier in file for font selection
 */
bitmap_text(pr,x,y,colour,pf,str)

bitmap pr;
int x,y;
int colour;
font pf;
char *str;

{
    struct pr_prpos pos;
  
    pos.pos.x=x;
    pos.pos.y=y;
    pos.pr=pr;

    pf_text(pos,colour,pf,str);
    }

