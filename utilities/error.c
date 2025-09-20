/*
 * error.c - Error handling and reporting utilities
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
#include <errno.h>
#include <stdlib.h>

/* int errno; */
/* char *sys_errlist[]; */

char *myname="Someone";

/*
 * ioerror - Report I/O error with system error message and terminate
 *
 * Reports an error message to stderr with printf-style formatting, appends
 * the system error message corresponding to the current errno value, and
 * terminates the program with exit code 1. This function is used for I/O
 * operations that fail and set errno.
 *
 * The function uses the legacy sys_errlist array to convert errno values
 * to human-readable error messages. On modern systems, strerror() would
 * be preferred, but this maintains compatibility with the original codebase.
 *
 * Parameters:
 *   str - Printf-style format string for the error message
 *   arg1 - First argument for format string (can be any type cast to int)
 *   arg2 - Second argument for format string (can be any type cast to int)
 *   arg3 - Third argument for format string (can be any type cast to int)
 *
 * Returns:
 *   This function does not return (calls exit(1))
 *
 * Side Effects:
 *   - Writes error message to stderr
 *   - Appends system error message based on current errno value
 *   - Terminates program execution with exit code 1
 *   - Uses global variable myname for program identification
 *
 * Testing Notes:
 *   Category: D (Mock Intensive) - Requires mocking stderr, errno, sys_errlist, exit
 *   Approach: Mock-based unit testing with captured stderr output
 *   Key Tests: Format string handling, errno message appending, program termination
 *   Dependencies: Global myname variable, errno value, sys_errlist array
 *   Mock Requirements: stderr capture, exit() interception, sys_errlist simulation
 *   Complexity: Moderate - Legacy error handling with variable arguments
 *
 * Notes:
 *   - Uses legacy K&R function declaration style with VARARGS1 comment
 *   - Supports up to 3 arguments in format string (legacy limitation)
 *   - Relies on global myname variable for program identification
 *   - Uses deprecated sys_errlist instead of modern strerror()
 *   - Always terminates program - no graceful error recovery
 *   - Format string vulnerability if user input passed as str parameter
 */
/*VARARGS1*/
void
ioerror(str,arg1,arg2,arg3)

char *str;
int arg1, arg2, arg3;

{
fprintf(stderr,"%s: ",myname);
fprintf(stderr,str,arg1,arg2,arg3);
fprintf(stderr," - %s\n",sys_errlist[errno]);
exit(1);
}

/*
 * error - Report general error message and terminate
 *
 * Reports an error message to stderr with printf-style formatting and
 * terminates the program with exit code 1. This function is used for
 * general error conditions that do not involve system I/O errors.
 *
 * Unlike ioerror(), this function does not append a system error message
 * and does not use errno. It simply formats and displays the provided
 * error message with the program name prefix.
 *
 * Parameters:
 *   str - Printf-style format string for the error message
 *   arg1 - First argument for format string (can be any type cast to int)
 *   arg2 - Second argument for format string (can be any type cast to int)
 *   arg3 - Third argument for format string (can be any type cast to int)
 *
 * Returns:
 *   This function does not return (calls exit(1))
 *
 * Side Effects:
 *   - Writes error message to stderr
 *   - Terminates program execution with exit code 1
 *   - Uses global variable myname for program identification
 *
 * Testing Notes:
 *   Category: D (Mock Intensive) - Requires mocking stderr and exit
 *   Approach: Mock-based unit testing with captured stderr output
 *   Key Tests: Format string handling, proper message formatting, program termination
 *   Dependencies: Global myname variable
 *   Mock Requirements: stderr capture, exit() interception
 *   Complexity: Simple - Basic error reporting with variable arguments
 *
 * Notes:
 *   - Uses legacy K&R function declaration style with VARARGS1 comment
 *   - Supports up to 3 arguments in format string (legacy limitation)
 *   - Relies on global myname variable for program identification
 *   - Always terminates program - no graceful error recovery
 *   - Format string vulnerability if user input passed as str parameter
 *   - Simpler version of ioerror() without errno handling
 */
/*VARARGS1*/
void
error(str,arg1,arg2,arg3)

char *str;
int arg1, arg2, arg3;

{
fprintf(stderr,"%s: ",myname);
fprintf(stderr,str,arg1,arg2,arg3);
fputc('\n',stderr);
exit(1);
}

