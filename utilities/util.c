/*
 * util.c - General utility functions for conqrast
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
#include <sgtty.h>
#include <stdlib.h>

/*********************************************************************
*                                                                    *
* CONQRAST GRAPHICS UTILITY FUNCTIONS                                *
*                                                                    *
* This file contains utility functions split from the main conqrast  *
* graphics program to improve modularity and maintainability. These  *
* functions provide essential graphics, process management, and user  *
* interface support for the conqrast raster graphics system.         *
*                                                                    *
* Key Components:                                                     *
* - Dithering Matrix Generation: build_dither_matrix()               *
* - Process Management: disconnect(), background()                   *
* - User Interface: help_message()                                   *
*                                                                    *
* Architecture Overview:                                              *
* The utilities support the main conqrast graphics rendering system  *
* by providing fundamental operations for image processing (dithering)*
* and process control (daemon creation and terminal detachment).     *
*                                                                    *
* Dependencies:                                                       *
* - Standard C library (stdio.h, stdlib.h)                          *
* - UNIX terminal control (sgtty.h)                                 *
* - UNIX process management (fork, ioctl)                           *
*                                                                    *
* Usage Context:                                                      *
* These functions are called by the main conqrast program during     *
* initialization and runtime to handle graphics processing setup     *
* and background process creation.                                   *
*                                                                    *
*********************************************************************/


/*********************************************************************
*                                                                    *
* routine to build the dither matrix used to shade                   *
*                                                                    *
*********************************************************************/

#define MAXDITHER 4		/* don't touch this */

short dit[1<<MAXDITHER][1<<MAXDITHER];

/*
 * build_dither_matrix - Generate ordered dithering matrix for graphics shading
 *
 * Creates a dithering matrix using the recursive halftone algorithm to provide
 * ordered dithering for graphics operations. The matrix size is 2^depth x 2^depth
 * and uses standard Floyd-Steinberg dithering patterns for smooth gradients.
 *
 * The algorithm starts with a 2x2 base matrix and recursively builds larger
 * matrices by doubling the size and scaling values. Each matrix element contains
 * a threshold value used for dithering decisions in graphics rendering.
 *
 * Parameters:
 *   depth - Matrix dimension exponent (final size will be 2^depth x 2^depth)
 *           Valid range: 1-4 (matrices from 2x2 to 16x16)
 *           Values outside range may cause buffer overflow
 *
 * Returns:
 *   void - Results stored in global dit[][] matrix
 *
 * Side Effects:
 *   - Modifies global dit[1<<MAXDITHER][1<<MAXDITHER] matrix
 *   - Matrix values scaled to 0-255 range for 8-bit graphics
 *   - 16x16 matrices use raw threshold values without scaling
 *
 * Testing Notes:
 *   Category: A (Unit) - Algorithmic function with deterministic output
 *   Approach: Unit tests with known matrix patterns and boundary conditions
 *   Key Tests: [depth=1 (2x2), depth=2 (4x4), depth=3 (8x8), depth=4 (16x16)]
 *   Dependencies: Global dit[][] array must be allocated
 *   Mock Requirements: None - pure algorithm
 *   Complexity: Simple - Mathematical pattern generation
 *
 * Notes:
 *   - Thread safety: Not thread-safe due to global matrix modification
 *   - Performance: O(4^depth) time complexity for matrix generation
 *   - Historical: Classic ordered dithering algorithm for raster graphics
 *   - Security: No bounds checking on depth parameter (potential overflow)
 */
void
build_dither_matrix(depth)

int depth;

{
    register int d,i,j,p;
    depth= 1<<depth;

    dit[0][0]=0; dit[1][0]=2;

    dit[0][1]=3; dit[1][1]=1;

    for(d=2;d<depth;d=d<<1)
	for(i=0;i<d;i++)
	    for(j=0;j<d;j++)
		{
		p= (dit[i][j] *=4);
		dit[d+i][j] = p+2;
		dit[i][d+j] = p+3;
		dit[d+i][d+j] = p+1;
		}

    if (depth != 16 )
	for(i=0;i<depth;i++)
	    for (j=0;j<depth;j++)
		dit[i][j] = (dit[i][j]*256)/d/d;

    }

/*********************************************************************
*                                                                    *
* disconnect from controlling tty                                    *
*                                                                    *
*********************************************************************/

/*
 * disconnect - Detach process from controlling terminal
 *
 * Disconnects the current process from its controlling terminal using the
 * TIOCNOTTY ioctl. This is typically used when a process wants to become
 * a daemon or run independently of the terminal that started it.
 *
 * The function opens /dev/tty (the controlling terminal), sends the TIOCNOTTY
 * ioctl to detach from it, then closes the file descriptor. After this call,
 * the process will no longer receive terminal-related signals like SIGINT.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Process loses controlling terminal association
 *   - No longer receives terminal signals (SIGINT, SIGTERM from terminal)
 *   - Opens and closes /dev/tty file descriptor
 *   - May fail silently if /dev/tty cannot be opened
 *
 * Testing Notes:
 *   Category: C (System) - Requires terminal and process control testing
 *   Approach: System testing with process state verification
 *   Key Tests: [Terminal detachment verification, signal isolation testing]
 *   Dependencies: UNIX terminal system, /dev/tty device availability
 *   Mock Requirements: Mock ioctl and file operations for unit testing
 *   Complexity: Moderate - System-level terminal control
 *
 * Notes:
 *   - Thread safety: Safe - operates on process-level terminal state
 *   - Performance: Single system call overhead
 *   - Historical: Classic UNIX daemon detachment technique
 *   - Portability: UNIX-specific (TIOCNOTTY ioctl)
 *   - Error handling: No error checking on open/ioctl/close operations
 */
void
disconnect()

{
    int tty;

    tty=open("/dev/tty",0);

    ioctl(tty,TIOCNOTTY,0);

    close(tty);
    }

/*********************************************************************
*                                                                    *
* go into background                                                 *
*                                                                    *
*********************************************************************/

/*
 * background - Fork process into background and display PID
 *
 * Creates a background process using fork() to allow the program to run
 * independently of the current shell session. The parent process exits
 * immediately while the child continues execution in the background.
 *
 * After forking, the child process prints its process ID (PID) in parentheses
 * to inform the user of the background process identifier. This follows
 * standard UNIX shell conventions for background process notification.
 *
 * This function is typically used in conjunction with disconnect() to create
 * a complete daemon-like background process that is independent of the terminal.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   void - Function only returns in child process (parent exits)
 *
 * Side Effects:
 *   - Creates new child process via fork()
 *   - Parent process terminates with exit(0)
 *   - Child process prints PID to stdout
 *   - Child becomes orphaned process (adopted by init)
 *
 * Testing Notes:
 *   Category: C (System) - Requires process management and fork testing
 *   Approach: System testing with process monitoring and PID verification
 *   Key Tests: [Fork success, parent termination, child continuation, PID output]
 *   Dependencies: UNIX process system, fork() availability
 *   Mock Requirements: Mock fork() and exit() for unit testing
 *   Complexity: Moderate - Process control and lifecycle management
 *
 * Notes:
 *   - Thread safety: Not applicable - creates separate process
 *   - Performance: Fork overhead plus process creation
 *   - Historical: Standard UNIX background process creation pattern
 *   - Portability: UNIX-specific (fork() system call)
 *   - Error handling: No error checking on fork() failure
 *   - Security: Child inherits parent's privileges and file descriptors
 */
void
background()

{
    switch (fork())
	{
    case 0: break;

    default: exit(0);
	}
    printf("(%d)\n",getpid());
    }

/*********************************************************************
*                                                                    *
* Print a useage message and exit. This needs to be expanded.        *
*                                                                    *
*********************************************************************/

/*
 * help_message - Display usage information and exit program
 *
 * Prints comprehensive command-line usage information for the conqrast graphics
 * utility program. Shows all available command-line options with their syntax
 * and terminates the program with exit status 1.
 *
 * The function displays a formatted help message showing the program name and
 * all supported command-line flags including output options (-o), nation selection
 * (-N), game directory (-D), contour settings (-c), font specification (-f),
 * and various display modes and options.
 *
 * After printing the usage information, the function calls exit(1) to terminate
 * the program, indicating an error condition (typically called when invalid
 * arguments are provided).
 *
 * Parameters:
 *   me - Program name string (argv[0]) to display in usage message
 *        Must not be NULL (no validation performed)
 *
 * Returns:
 *   void - Function never returns (calls exit(1))
 *
 * Side Effects:
 *   - Prints usage message to stdout
 *   - Terminates program with exit status 1
 *   - No cleanup of resources before exit
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires output capture and exit handling
 *   Approach: Integration testing with output redirection and exit monitoring
 *   Key Tests: [Usage message format, program name substitution, exit status]
 *   Dependencies: Standard output stream, exit() function
 *   Mock Requirements: Mock printf and exit for unit testing
 *   Complexity: Simple - Formatted output and program termination
 *
 * Notes:
 *   - Thread safety: Not applicable - terminates entire process
 *   - Performance: Minimal - simple string output
 *   - Historical: Standard UNIX help message pattern
 *   - Portability: Standard C functions (printf, exit)
 *   - Error handling: No validation of me parameter (potential NULL dereference)
 *   - Usage pattern: Called when command-line parsing fails
 */
void
help_message(me)

char *me;

{
    printf("%s: Usage --\n",me);
    printf("\n%s\t\t-o <file-or-display>\n",me);
    printf("\t\t-N <nation>\n");
    printf("\t\t-D <game-directory>\n");
    printf("\t\t-c<contour-character> <brightness>\n");
    printf("\t\t-f <which-font> <font>\n");
    printf("\t\t-d -n{a} -b{a} -m <magnification> -a -bg -l -t -v\n");
    exit(1);
    }

