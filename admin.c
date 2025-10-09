/*
 * admin.c - Administrative functions and game management
 *
 * ADMINISTRATIVE CORE MODULE
 *
 * This module provides the primary administrative interface for the Conquer game system,
 * including world creation, player management, game execution, and comprehensive nation
 * attribute calculations. It serves as the central control point for all game
 * administration operations and implements the complete attribute calculation system.
 *
 * Core Administrative Functions:
 * 1. Game Lifecycle Management - World creation, updates, and execution control
 * 2. Player Administration - New player addition with security validation
 * 3. Attribute Calculation System - Complete nation attribute computation framework
 * 4. Security and Access Control - User authentication and permission management
 * 5. Command-Line Interface - Comprehensive argument processing and option handling
 * 6. Environment Integration - Data directory management and configuration
 * 7. File System Operations - Lock management and data persistence coordination
 * 8. Trade Good Processing - Exotic resource bonuses and economic calculations
 *
 * Administrative Components:
 * - World Creation Engine: New game initialization with optional scenario loading
 * - Player Management System: Secure new player addition with permission validation
 * - Update Execution Framework: Game turn processing with concurrency control
 * - Nation Attribute Calculator: Complete statistical computation for all nations
 * - Trade Good Processor: Economic bonus calculation from exotic resources
 * - Security Framework: Multi-level user authentication and access control
 * - Lock Management System: File-based concurrency control for multi-user safety
 * - Configuration System: Environment variable and command-line option processing
 *
 * Integration Points:
 * - Magic System: Magic power bonuses in attribute calculations
 * - Combat System: Military statistics and terror calculation
 * - Display System: Score reporting and information presentation
 * - I/O System: Data file operations and persistence management
 * - Command System: Administrative command processing framework
 * - Data Structures: Complete game state manipulation and validation
 *
 * Security Considerations:
 * - Multi-level authentication (game admin, nation leader, standard user)
 * - Conditional compilation flags for feature access control
 * - File lock management for concurrent operation safety
 * - Password validation and secure input handling
 * - Permission verification before destructive operations
 *
 * This file is part of Conquer.
 * Originally Copyright (C) 1988-1989 by Edward M. Barlow and Adam Bryant
 * Copyright (C) 2025 Juan Manuel Méndez Rey (Vejeta) - Licensed under GPL v3 with permission
 * from original authors
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
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <pwd.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <crypt.h>
#include <termios.h>
#include "header.h"
#include "data.h"
#include "safe_convert.h"
#include "safe_system.h"


/*Declarations*/
struct s_sector **sct;

/*
 * get_password - Modern replacement for deprecated getpass() function
 *
 * Securely reads a password from stdin without echoing characters.
 * Uses termios to disable terminal echo during input.
 *
 * Parameters:
 *   prompt - Text prompt to display to user
 *
 * Returns:
 *   Pointer to password string (caller must not free)
 *
 * Notes:
 *   - Uses static buffer (not thread-safe)
 *   - Automatically restores terminal echo state
 *   - POSIX-compliant replacement for getpass()
 */
static char *get_password(const char *prompt) {
    static char password[256];
    struct termios old_termios, new_termios;
    size_t len;

    printf("%s", prompt);
    fflush(stdout);

    /* Get current terminal settings */
    if (tcgetattr(STDIN_FILENO, &old_termios) != 0) {
        return NULL;
    }

    /* Disable echo */
    new_termios = old_termios;
    new_termios.c_lflag &= ~(tcflag_t)ECHO;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) != 0) {
        return NULL;
    }

    /* Read password */
    if (fgets(password, sizeof(password), stdin) == NULL) {
        tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
        return NULL;
    }

    /* Restore terminal settings */
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
    printf("\n");

    /* Remove newline if present */
    len = strlen(password);
    if (len > 0 && password[len - 1] == '\n') {
        password[len - 1] = '\0';
    }

    return password;
}
struct s_nation ntn[NTOTAL]; /* player nation stats */
struct s_world world;
/*is sector occupied by an army?*/
char **occ;
short **movecost;
long startgold = 0;

/*offset of upper left hand corner*/
short xoffset = 0, yoffset = 0;
/*current cursor postion (relative to 00 in upper corner)*/
/*	position is 2*x,y*/
short xcurs = 0, ycurs = 0;
/*display state*/
short dismode = 2;
/* nation id of owner*/
short country = 0;
struct s_nation *curntn;


/* Note: scenario is already declared in data.h when ADMIN is defined */
/* char scenario[256]; -- REMOVED to avoid conflict */

#ifdef REMAKE
int remake = FALSE;
#endif /*REMAKE*/

FILE *fexe;

/*
 * main - Primary administrative entry point for Conquer game management
 *
 * Comprehensive administrative control center that handles all game management
 * operations including world creation, player addition, game execution, and
 * system configuration. Implements multi-level security, environment variable
 * processing, command-line argument parsing, and complete administrative workflow
 * management with extensive error handling and user feedback.
 *
 * Parameters:
 *   argc - Number of command-line arguments
 *   argv - Array of command-line argument strings
 *
 * Returns:
 *   Exits with SUCCESS (0) on successful completion
 *   Exits with FAIL (1) on error or invalid operation
 *
 * Side Effects:
 *   - Sets umask for file creation permissions
 *   - Changes working directory to game data directory
 *   - Creates or modifies game data files
 *   - Manages file locks for concurrent access control
 *   - May invoke world creation, player addition, or game update
 *   - Validates user permissions and authentication
 *   - Processes environment variables and command-line options
 *   - Calls newlogin(), makeworld(), update(), readdata(), writedata()
 *
 * Command-Line Options:
 *   -m: Create new world (requires admin privileges)
 *   -a: Add new player (with security validation)
 *   -x: Execute game update (requires admin privileges)
 *   -r SCENARIO: Read scenario files during world creation
 *   -d DIR: Specify custom data directory
 *   -?: Display help and usage information
 *
 * Environment Variables:
 *   CONQUER_OPTS: Game configuration options
 *     - G: Gaudy display mode (ignored in admin)
 *     - N/n: Nation name specification
 *     - D/d: Data directory specification (data=, datadir=, directory=, dir=)
 *
 * Security Levels:
 *   1. Game Administrator (LOGIN user): Full access to all operations
 *   2. Nation Leader (nation[0].leader): World creation and update access
 *   3. Standard User: Player addition only (with restrictions)
 *
 * Workflow Operations:
 *   World Creation (-m):
 *     - Validates admin permissions
 *     - Checks for existing game (prevents accidental destruction)
 *     - Optionally reads scenario files (-r flag)
 *     - Calls makeworld() for world generation
 *     - Removes update lock file
 *
 *   Player Addition (-a):
 *     - Checks for active update or admin sessions
 *     - Validates late-join permissions after LASTADD turns
 *     - Creates addition lock to prevent concurrent additions
 *     - Calls newlogin() for player registration
 *     - Removes addition lock
 *
 *   Game Update (-x):
 *     - Validates admin permissions
 *     - Checks for active player sessions (RUNSTOP)
 *     - Creates update lock to prevent concurrent updates
 *     - Calls update() for game processing
 *     - Calls writedata() for persistence
 *     - Removes update lock
 *     - Optional time logging (TIMELOG)
 *
 * File Lock Management:
 *   - Update lock (isonfile + "up"): Prevents concurrent updates
 *   - Addition lock (isonfile + "add"): Prevents concurrent player additions
 *   - Player locks (isonfile + nation_id): Tracks active player sessions
 *   - Admin lock (isonfile + "0"): Tracks admin session activity
 *
 * Error Handling:
 *   - Directory access validation
 *   - File existence checking
 *   - User permission verification
 *   - Lock conflict detection
 *   - Password validation
 *   - Command-line argument validation
 *   - Environment variable parsing
 *
 * Testing Notes:
 *   Category: C (System) - Requires full system setup and file operations
 *   Approach: System testing with mock user accounts and file permissions
 *   Key Tests: Command-line parsing, security validation, file operations, lock management
 *   Dependencies: File system, user accounts, data directory, game data files
 *   Mock Requirements: Mock filesystem, user database, permission system
 *   Complexity: Complex - Multi-user security, file operations, process coordination
 * @last_documented: 2025-09-20
 */
int main(int argc, char **argv) {
    uid_t realuser;
    size_t l;
    register size_t i;
    register size_t j;
    int opt;
    char *name;
#ifndef __STDC__
    void srand();
#endif
    /* mflag = make world, a=add player, x=execute, p=print */
    /* rflag = make world from read in files */
    int mflag, aflag, xflag, rflag;
    char string[FILELTH];
    char defaultdir[BIGLTH], cq_opts[BIGLTH];
    struct passwd *pwent;

    umask(MASK);
    mflag = aflag = xflag = rflag = 0;
    srand((unsigned)time((long *)0));
    strncpy(datadir, "", sizeof(datadir) - 1);
    datadir[sizeof(datadir) - 1] = '\0';
    strncpy(cq_opts, "", sizeof(cq_opts) - 1);
    cq_opts[sizeof(cq_opts) - 1] = '\0';
    name = string;
    *name = 0;

    /* check conquer options */
    if (getenv(ENVIRON_OPTS) != NULL) {
        snprintf(cq_opts, sizeof(cq_opts), "%s", getenv(ENVIRON_OPTS));
    }
    if (cq_opts[0] != '\0') {
        l = strlen(cq_opts);
        for (i = 0; i < l; i++) {
            switch (cq_opts[i]) {
                case 'G':
                    /* ignore Gaudy display */
                    break;
                case 'N':
                case 'n':
                    /* ignore nation name */
                    for (; i < l && cq_opts[i] != ','; i++)
                        ;
                    break;
                case 'D':
                case 'd':
                    /* check for data directory */
                    if (strncmp(cq_opts + i + 1, "ata=", 4) == 0) {
                        i += 5;
                    } else if (strncmp(cq_opts + i + 1, "atadir=", 7) == 0) {
                        i += 8;
                    } else if (strncmp(cq_opts + i + 1, "irectory=", 9) == 0) {
                        i += 10;
                    } else if (strncmp(cq_opts + i + 1, "ir=", 3) == 0) {
                        i += 4;
                    } else {
                        fprintf(stderr, "conquer: invalid environment\n");
                        fprintf(stderr, "\t%s = %s\n", ENVIRON_OPTS, cq_opts);
                        fprintf(stderr, "\texpected <data=NAME>\n");
                        exit(FAIL);
                    }
                    if (i < l) {
                        /* grab the data directory */
                        for (j = 0; j < l - i && cq_opts[i + j] != ','; j++) {
                            datadir[j] = cq_opts[i + j];
                        }
                        datadir[j] = '\0';
                        i += j - 1;
                    }
                    break;
                case ' ':
                case ',':
                    /* ignore commas and spaces */
                    break;
                default:
                    /* complain */
                    fprintf(stderr, "conquer: invalid environment\n");
                    fprintf(stderr, "\t%s = %s\n", ENVIRON_OPTS, cq_opts);
                    fprintf(stderr, "\tunexpected option <%c>\n", cq_opts[i]);
                    exit(FAIL);
                    break;
            }
        }
    }

    /* process the command line arguments */
    while ((opt = getopt(argc, argv, "maxr:d:")) != EOF)
        switch (opt) {
            /* process the command line arguments */
            case 'm': /* make a new world*/
                mflag++;
                break;
            case 'a': /* anyone with password can add player*/
                aflag++;
                break;
            case 'x': /* execute (update) program*/
                xflag++;
                break;
            case 'r': /* read map file */
                rflag++;
                if (strlen(optarg) > NAMELTH) {
                    fprintf(stderr, "ERROR: MAPFILE STEM LONGER THAN %d\n", NAMELTH);
                    exit(FAIL);
                }
                strncpy(scenario, optarg, NAMELTH);
                scenario[NAMELTH] = '\0';
                break;
            case 'd':
                strncpy(datadir, optarg, sizeof(datadir) - 1);
                datadir[sizeof(datadir) - 1] = '\0';
                break;
            case '?': /*  print out command line arguments */
                printf("Command line format: %s [-max -dDIR -rSCENARIO]\n", argv[0]);
                printf("\t-m          make a world\n");
                printf("\t-a          add new player\n");
                printf("\t-x          execute (update) program\n");
                printf("\t-d DIR      to use play different game\n");
                /* printf("\t-r SCENARIO read map while making a new world\n\t\t\tuses
                 * SCENARIO.ele, SCENARIO.veg, &  SCENARIO.nat\n"); */
                exit(SUCCESS);
        }

    realuser = getuid();
    /* may now replace user identity */
    (void)setuid(geteuid());

    /* set proper defaultdir */
    if (datadir[0] != '/') {
        if (strlen(datadir) > 0) {
            snprintf(defaultdir, sizeof(defaultdir), "%s/%s", DEFAULTDIR, datadir);
        } else {
            strncpy(defaultdir, DEFAULTDIR, sizeof(defaultdir) - 1);
            defaultdir[sizeof(defaultdir) - 1] = '\0';
            strncpy(datadir, "[default]", sizeof(datadir) - 1);
            datadir[sizeof(datadir) - 1] = '\0';
        }
    } else {
        /* Copy absolute path from datadir
         * Use FILELTH (datadir size) to prevent buffer over-read of source buffer */
        strncpy(defaultdir, datadir, FILELTH - 1);
        defaultdir[FILELTH - 1] = '\0';
    }

    /* now that we have parsed the args, we can got to the
     * dir where the files are kept and do some work.
     */
    if (chdir(defaultdir)) {
        printf("unable to change dir to %s\n", defaultdir);
        exit(FAIL);
    }
    if ((mflag) || (rflag)) {
#ifdef REMAKE
        /* check if datafile currently exists*/
        if (access(datafile, 00) == 0) {
            /* read in the data*/
            readdata();
            verifydata(__FILE__, __LINE__);

            /* verify ability to remake the world */
            struct passwd *login_pw = getpwnam(LOGIN);
            if (login_pw == NULL) {
                printf("Error: User '%s' not found or access denied\n", LOGIN);
                exit(FAIL);
            }

            if ((realuser != login_pw->pw_uid)
                && ((pwent = getpwnam(ntn[0].leader)) == NULL || realuser != pwent->pw_uid)) {
                printf("Sorry -- you can not create a world\n");
                printf("you need to be logged in as %s", LOGIN);
                if (strcmp(LOGIN, ntn[0].leader) != 0) {
                    printf(" or %s", ntn[0].leader);
                }
                printf(".\n");
                exit(FAIL);
            }
            remake = TRUE;
            printf("************* WARNING!!!! *******************\n\n");
            printf("    There is already a game in progress.\n\n");
            printf("*********************************************\n\n");
            printf("Do you wish to destroy the current game? ");
            if (scanf("%79s", string) != 1) {
                /* Input error - default to "no" for safety */
                string[0] = '\0';
            }
            if (strcmp(string, "yes") != 0 && strcmp(string, "y") != 0) {
                printf("Okay... the world is left intact\n");
                exit(FAIL);
            }
            printf("Are you absolutely certain? ");
            if (scanf("%79s", string) != 1) {
                /* Input error - default to "no" for safety */
                string[0] = '\0';
            }
            if (strcmp(string, "yes") != 0 && strcmp(string, "y") != 0) {
                printf("Okay... the world is left intact\n");
                exit(FAIL);
            }
            printf("The re-destruction of the world has begun...\n");
            sleep(1);
        }
#else
        /* check for god permissions */
        struct passwd *login_pw2 = getpwnam(LOGIN);
        if (login_pw2 == NULL) {
            printf("Error: User '%s' not found or access denied\n", LOGIN);
            exit(FAIL);
        }
        if (realuser != login_pw2->pw_uid) {
            printf("Sorry -- you can not create a world\n");
            printf("you need to be logged in as %s.\n", LOGIN);
            exit(FAIL);
        }

        /* check if datafile already exists*/
        if (access(datafile, 00) == 0) {
            printf("ABORTING: File %s exists\n", datafile);
            printf("\tthis means that a game is in progress. To proceed, you must remove \n");
            printf("\tthe existing data file. This will, of course, destroy that game.\n\n");
            exit(FAIL);
        }
#endif /* REMAKE */

        makeworld(rflag);
        snprintf(string, sizeof(string), "%sup", isonfile);
        unlink(string);
        exit(SUCCESS);
    }

    /* read data*/
    readdata();
    verifydata(__FILE__, __LINE__);

    if (aflag) { /* a new player */
        snprintf(string, sizeof(string), "%sup", isonfile);
        if (check_lock(string, FALSE) == TRUE) {
            printf("Conquer is updating\n");
            printf("Please try again later.\n");
            exit(FAIL);
        }

        snprintf(string, sizeof(string), "%s0", isonfile);
        if (check_lock(string, FALSE) == TRUE) {
            printf("God is currently logged in.\n");
            printf("Please try again later.\n");
            exit(FAIL);
        }

        printf("\n********************************************");
        printf("\n*      PREPARING TO ADD NEW PLAYERS        *");
        printf("\n********************************************\n");
        if (TURN > LASTADD) {
            printf("more than %d turns have passed since game start!\n", LASTADD);
            printf("permission of game administrator required\n");
            if (strncmp(crypt(get_password("\nwhat is conquer super user password:"), SALT),
                        ntn[0].passwd, PASSLTH)
                != 0) {
                printf("sorry...\n");
                exit(FAIL);
            }
        }
        /* prevent more than one addition */
        snprintf(string, sizeof(string), "%sadd", isonfile);
        if (check_lock(string, TRUE) == TRUE) {
            printf("Someone else is adding\n");
            printf("Please try again later.\n");
            exit(FAIL);
        }

        /* disable interrupts */
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        newlogin(safe_uid_to_int(realuser));
        unlink(string);
        exit(SUCCESS);
    }

#ifdef OGOD
    struct passwd *login_pw3 = getpwnam(LOGIN);
    if (login_pw3 == NULL) {
        printf("Error: User '%s' not found or access denied\n", LOGIN);
        exit(FAIL);
    }

    if ((realuser != login_pw3->pw_uid)
        && ((pwent = getpwnam(ntn[0].leader)) == NULL || realuser != pwent->pw_uid)) {
        printf("Sorry -- you can not administrate conquer\n");
        printf("you need to be logged in as %s", LOGIN);
        if (strcmp(LOGIN, ntn[0].leader) != 0) {
            printf(" or %s", ntn[0].leader);
        }
        printf("\n");
        exit(FAIL);
    }
#endif /* OGOD */

    if (xflag) { /* update the game */
#ifndef OGOD
        struct passwd *login_pw4 = getpwnam(LOGIN);
        if (login_pw4 == NULL) {
            printf("Error: User '%s' not found or access denied\n", LOGIN);
            exit(FAIL);
        }

        if ((realuser != login_pw4->pw_uid)
            && ((pwent = getpwnam(ntn[0].leader)) == NULL || realuser != pwent->pw_uid)) {
            printf("sorry -- your uid is invalid for updating\n");
            printf("you need to be logged in as %s", LOGIN);
            if (strcmp(LOGIN, ntn[0].leader) != 0) {
                printf(" or %s", ntn[0].leader);
            }
            printf("\n");
            exit(FAIL);
        }
#endif /* OGOD */
#ifdef RUNSTOP
        /* check if any players are on */
        for (i = 0; i < NTOTAL; i++) {
            snprintf(string, sizeof(string), "%s%zu", isonfile, i);
            if (check_lock(string, FALSE) == TRUE) {
                printf("Nation %zu is still in the game.\n", i);
                printf("Update aborted.\n");
                exit(FAIL);
            }
        }
#endif /* RUNSTOP */
        snprintf(string, sizeof(string), "%sup", isonfile);
        if (check_lock(string, TRUE) == TRUE) {
            printf("Another update is still executing.\n");
            printf("Update aborted.\n");
            exit(FAIL);
        }
        update();
        writedata();
        unlink(string);
#ifdef TIMELOG
        /* Write timestamp using secure native C function instead of system() call */
        if (write_timestamp_to_file(timefile) != 0) {
            printf("Warning: Failed to write timestamp to %s\n", timefile);
        }
#endif /* TIMELOG */
        exit(SUCCESS);
    }
    printf("error: must specify an option\n");

    /*  print out command line arguments */
    printf("Command line format: %s [-max -dDIR]\n", argv[0]);
    printf("\t-a       add new player\n");
    printf("\t-d DIR   to use play different game\n");
    printf("\t-m       make a world\n");
    printf("\t-x       execute (update) program\n");
    exit(SUCCESS);
}

/*
 * att_setup - Initialize nation attributes to starting values
 *
 * Sets up initial attribute values for nations at game start or during
 * administration. Establishes base values for farming, economy, politics,
 * and resource extraction capabilities. Can initialize a specific nation
 * or all active nations in the game. Includes magic-based bonuses for
 * mining ability and sets consistent starting parameters.
 *
 * Parameters:
 *   cntry - Nation ID to initialize (0 = initialize all active nations)
 *
 * Returns:
 *   None (void function)
 *
 * Side Effects:
 *   - Modifies nation attribute values in ntn[] array
 *   - Sets farm_ability, poverty, popularity, reputation, prestige
 *   - Sets eatrate, tax_rate, mine_ability, knowledge, charity
 *   - Applies magic power bonuses (MINER increases mine_ability)
 *   - Only processes active nations (isntn() check)
 *
 * Attribute Initialization Values:
 *   farm_ability: 10 (base agricultural productivity)
 *   poverty: 95 (high initial poverty level)
 *   popularity: 50 (neutral public opinion)
 *   reputation: 50 (neutral international standing)
 *   prestige: 50 (neutral power projection)
 *   eatrate: 25 (base food consumption rate)
 *   tax_rate: 10 (base taxation level)
 *   mine_ability: 25 (with MINER magic) or 10 (base mining capability)
 *   knowledge: 10 (base intellectual capacity)
 *   charity: 0 (no initial charitable giving)
 *
 * Magic Integration:
 *   - MINER magic power: Increases mine_ability from 10 to 25
 *   - Uses magic() function for power verification
 *   - Provides 150% mining bonus for magical nations
 *
 * Administrative Usage:
 *   - Game initialization: Setup all nations (cntry = 0)
 *   - Nation reset: Reset specific nation (cntry = nation_id)
 *   - Debugging: Restore known attribute states
 *   - Testing: Establish consistent starting conditions
 *
 * Testing Notes:
 *   Category: A (Unit) - Isolated attribute manipulation
 *   Approach: Unit tests with mock nation data and magic system
 *   Key Tests: Single nation setup, all nations setup, magic bonus application
 *   Dependencies: ntn[] array, magic() function, isntn() validation
 *   Mock Requirements: Mock magic system, mock nation data structures
 *   Complexity: Simple - Direct attribute assignment with conditional logic
 * @last_documented: 2025-09-20
 */
void att_setup(int cntry) {
    int nat;
    for (nat = 0; nat < NTOTAL; nat++)
        if (isntn(ntn[nat].active))
            if ((cntry == 0) || (nat == cntry)) {
                ntn[nat].farm_ability = 10;
                ntn[nat].poverty = 95;
                ntn[nat].popularity = ntn[nat].reputation = ntn[nat].prestige = 50;
                ntn[nat].eatrate = 25;
                ntn[nat].tax_rate = 10;
                if (magic(nat, MINER))
                    ntn[nat].mine_ability = 25;
                else
                    ntn[nat].mine_ability = 10;
                ntn[nat].knowledge = 10;
                ntn[nat].charity = 0;
            }
}

/*
 * att_base - Calculate comprehensive base attribute values for all nations
 *
 * Performs complete nation attribute calculation including economic indicators,
 * military statistics, agricultural productivity, technological advancement,
 * political metrics, and magical bonuses. This is the core attribute calculation
 * engine that processes all active nations and computes relative world statistics
 * for comparative analysis. Includes complex seasonal food calculations,
 * population dynamics, infrastructure benefits, and class-based bonuses.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   None (void function)
 *
 * Side Effects:
 *   - Updates all nation attributes in ntn[] array
 *   - Calculates global world statistics (WORLD* variables)
 *   - Modifies eatrate based on seasonal food availability
 *   - Applies magic power bonuses to various attributes
 *   - Applies nation class bonuses and penalties
 *   - Enforces maximum value limits (MAXTGVAL)
 *   - Processes all sectors for infrastructure calculations
 *
 * Attribute Calculations:
 *   Economic Metrics:
 *     - wealth: Based on gold, jewels, metals relative to world totals
 *     - spoilrate: Food spoilage based on granaries and cities
 *     - eatrate: Seasonal food consumption with dynamic adjustment
 *
 *   Military and Political:
 *     - terror: Based on military/civilian ratio and mercenary presence
 *     - power: Combination of score and military strength relative to world
 *     - prestige: Average of prestige, power, and wealth
 *     - reputation: Random fluctuation with bounds checking
 *     - popularity: Based on wealth, food, clerics, and current popularity
 *
 *   Production and Technology:
 *     - farm_ability: Food production efficiency per civilian
 *     - mine_ability: Mining productivity with infrastructure bonuses
 *     - knowledge: Based on cities, towns, and university scholars
 *     - communications: Transportation efficiency from infrastructure
 *
 * Infrastructure Processing:
 *   - Cities and Towns: Population centers for economic calculation
 *   - Mines: Metal production with trade good validation
 *   - Farms: Food production with efficiency calculations
 *   - Universities: Knowledge generation from scholars
 *   - Churches: Religious influence for popularity
 *   - Blacksmiths: Industrial capacity for mining
 *   - Roads: Transportation network efficiency
 *   - Granaries: Food storage and spoilage reduction
 *   - Capitols: Triple city value with enhanced bonuses
 *
 * Seasonal Food Calculations:
 *   Winter: 180 * food / (eatrate + 25) - harsh survival
 *   Spring: 204 * food / (eatrate + 25) - recovery period
 *   Summer: 250 * food / (eatrate + 25) - abundant growth
 *   Fall: 312 * food / (eatrate + 25) - harvest time
 *
 * Magic Power Integration:
 *   - MINER: +15 mine_ability bonus
 *   - STEEL: +15 mine_ability bonus
 *   - SLAVER: +PWR_NA terror increase
 *   - RELIGION: +PWR_NA popularity increase
 *   - URBAN: -PWR_NA popularity penalty
 *   - DEMOCRACY: +25 eatrate, -PWR_NA terror, +charity
 *   - KNOWALL: +PWR_NA knowledge bonus
 *   - ARCHITECT: -PWR_NA spoilrate improvement
 *   - ROADS: +50 communications, +PWR_NA terror
 *   - DESTROYER: +PWR_NA terror increase
 *   - VAMPIRE: +PWR_NA terror increase
 *
 * Nation Class Bonuses:
 *   - C_NPC: +popularity, +terror
 *   - C_KING: +popularity
 *   - C_TRADER: +wealth, +popularity, +prestige
 *   - C_EMPEROR: +wealth, +popularity, +prestige
 *   - C_WIZARD: +knowledge
 *   - C_PRIEST: +popularity
 *   - C_PIRATE: +terror
 *   - C_WARLORD: +prestige (recursive scaling)
 *   - C_DEMON: +terror
 *   - C_DRAGON: +terror
 *   - C_SHADOW: +terror
 *
 * World Statistics Calculated:
 *   - WORLDJEWELS, WORLDGOLD, WORLDMETAL: Resource totals
 *   - WORLDFOOD, WORLDSCORE, WORLDCIV: Development metrics
 *   - WORLDSCT, WORLDMIL, WORLDNTN: Territory and military totals
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires complete game state and sector data
 *   Approach: Integration testing with full world setup and magic system
 *   Key Tests: World statistics calculation, seasonal variations, magic bonuses, class effects
 *   Dependencies: Complete sector array, nation data, magic system, world state
 *   Mock Requirements: Full world map, complete nation setup, magic power system
 *   Complexity: Complex - Multi-system integration with extensive calculations
 * @last_documented: 2025-09-20
 */
void att_base(void) {
    long cityfolk, townfolk, scholars, foodpts, minepts, roads, clerics, ngrain;
    long blksmths;
    long mercs, armynum, ncities;
    long temp = 0;
    int x, y;
    char d;

    /* set to one so NEVER will have a divide by 0 */
    WORLDJEWELS = 1;
    WORLDGOLD = 1;
    WORLDMETAL = 1;
    WORLDFOOD = 1;
    WORLDSCORE = 1;
    WORLDCIV = 1;
    WORLDSCT = 1;
    WORLDMIL = 1;
    WORLDNTN = 0;
    for (country = 1; country < NTOTAL; country++) {
        curntn = &ntn[country];
        if (!isntn(curntn->active))
            continue;
        WORLDNTN++;
        WORLDJEWELS += curntn->jewels;
        if (curntn->tgold > 0)
            WORLDGOLD += curntn->tgold;
        WORLDMETAL += curntn->metals;
        WORLDFOOD += curntn->tfood;
        WORLDSCORE += curntn->score;
        WORLDCIV += curntn->tciv;
        WORLDSCT += curntn->tsctrs;
        WORLDMIL += curntn->tmil;
    }
    if (WORLDGOLD == 0)
        WORLDGOLD = 1;

    /* count the number of sectors */
    for (country = 1; country < NTOTAL; country++) {
        if (!isntn(ntn[country].active))
            continue;
        curntn = &ntn[country];
        cityfolk = townfolk = scholars = foodpts = minepts = roads = clerics = ngrain =
            ncities = blksmths = 0;

        for (x = 0; x < MAPX; x++)
            for (y = 0; y < MAPY; y++)
                if (sct[x][y].owner == country) {
                    d = sct[x][y].designation;
                    if (d == DTOWN)
                        townfolk += sct[x][y].people;
                    else if (d == DCITY) {
                        cityfolk += sct[x][y].people;
                        ncities++;
                    } else if (d == DMINE) {
                        if (tg_ok(country, &sct[x][y]))
                            minepts += sct[x][y].metal;
                    } else if (d == DFARM)
                        foodpts += sct[x][y].people * tofood(&sct[x][y], country);
                    else if (d == DCAPITOL) {
                        ncities += 3;
                        cityfolk += sct[x][y].people;
                    } else if (d == DUNIVERSITY)
                        scholars += sct[x][y].people;
                    else if (d == DROAD)
                        roads++;
                    else if (d == DCHURCH)
                        clerics += sct[x][y].people;
                    else if (d == DGRANARY)
                        ngrain++;
                    else if (d == DBLKSMITH)
                        blksmths += sct[x][y].people;
                }
        cityfolk /= 167;
        townfolk /= 167;
        scholars /= 167;
        clerics /= 167;
        blksmths /= 167;

        /* use prior eatrate a weighting factor */
        if (curntn->eatrate < 25)
            curntn->eatrate = 25;
        if (TURN != 1) { /* remember eatrate is scaled by 25 */
            switch (SEASON(TURN)) {
                case WINTER: /* 7 food/person for each eatrate */
                    temp = 180L * curntn->tfood / (curntn->eatrate + 25L);
                    break;
                case SPRING: /* 6 food/person for each eatrate */
                    temp = 204L * curntn->tfood / (curntn->eatrate + 25L);
                    break;
                case SUMMER: /* 5 food/person for each eatrate */
                    temp = 250L * curntn->tfood / (curntn->eatrate + 25L);
                    break;
                case FALL: /* 4 food/person for each eatrate */
                    temp = 312L * curntn->tfood / (curntn->eatrate + 25L);
                    break;
            }
            if (curntn->tciv > 0)
                x = safe_long_to_int(curntn->eatrate / 2 + temp / curntn->tciv);
            else
                x = 25;
            if (x < MAXTGVAL)
                curntn->eatrate = (char)x;
            else
                curntn->eatrate = MAXTGVAL;
            if (curntn->eatrate < 25)
                curntn->eatrate = 25;

        } else {
            cityfolk = 10; /* Aproximate steady state */
        }

        if (30 <= 1 + ngrain + ncities)
            curntn->spoilrate = 1;
        else
            curntn->spoilrate = safe_clamp_nation_attr(30 - ngrain - ncities);
        if (curntn->tfood > curntn->tciv * 10)
            curntn->spoilrate = 30;

        /* get number of mercenaries */
        mercs = 0;
        for (armynum = 0; armynum < MAXARM; armynum++)
            if (P_ATYPE == A_MERCENARY)
                mercs += P_ASOLD;
        if (curntn->tmil > 0 && curntn->tciv > 0)
            temp = (1000 * curntn->tmil) / curntn->tciv + (1000 * mercs) / curntn->tmil;
        else
            temp = 0;
        curntn->terror = safe_clamp_nation_attr(temp / 5);

        temp = (5 * townfolk / 2 + 5 * cityfolk) + roads * 5;
        curntn->communications = safe_clamp_nation_attr(temp);

        temp = 1000 * curntn->score / WORLDSCORE + 1000 * curntn->tmil / WORLDMIL;
        curntn->power = safe_clamp_nation_attr(temp / 5);

        /* calculate national wealth */
        temp = curntn->tgold;
        if (temp < 0)
            temp = 0;
        temp = safe_double_to_long(
            1000.0 * (safe_long_to_double(temp) / safe_long_to_double(WORLDGOLD))
            + 1000.0 * (safe_long_to_double(curntn->jewels) / safe_long_to_double(WORLDJEWELS))
            + 1000.0 * (safe_long_to_double(curntn->metals) / safe_long_to_double(WORLDMETAL))
            + safe_long_to_double(cityfolk) * (4.0 / 3.0)
            + safe_long_to_double(townfolk) * (5.0 / 6.0));
        if (temp >= curntn->wealth) {
            curntn->wealth = safe_clamp_nation_attr(temp / 10);
        } else {
            curntn->wealth -= safe_clamp_nation_attr((curntn->wealth - temp) / 4);
        }

        if (TURN != 1) {
            curntn->reputation += safe_clamp_nation_attr(rand() % 8 - 3);
            curntn->reputation = safe_clamp_nation_attr(curntn->reputation);

            temp = (curntn->prestige + curntn->power + curntn->wealth) / 3;
            curntn->prestige = safe_clamp_nation_attr(temp);

            if (curntn->tciv > 0)
                temp = foodpts * 10 / curntn->tciv;
            else
                temp = 0;
            curntn->farm_ability = safe_clamp_nation_attr(temp);
        }

        /* calcualte mining ability */
        temp = (minepts / 3 + cityfolk / 2 + townfolk / 2 + blksmths);
        if (magic(country, MINER))
            temp += 15;
        if (magic(country, STEEL))
            temp += 15;
        if (temp >= curntn->mine_ability) {
            curntn->mine_ability = safe_clamp_nation_attr(temp);
        } else {
            curntn->mine_ability -= safe_clamp_nation_attr((curntn->mine_ability - temp) / 4);
        }

        /* calculate knowledge */
        temp = cityfolk / 2 + townfolk / 6 + scholars / 2;
        curntn->knowledge = safe_clamp_nation_attr(temp);

        /* find national popularity */
        temp = safe_double_to_long((curntn->wealth + 10 * P_EATRATE
                                    + safe_long_to_double(clerics) + curntn->popularity)
                                   / 2);
        curntn->popularity = safe_clamp_nation_attr(temp);

        if (magic(country, SLAVER))
            curntn->terror += PWR_NA;
        if (magic(country, RELIGION))
            curntn->popularity += PWR_NA;
        if (magic(country, URBAN)) {
            if (curntn->popularity > PWR_NA)
                curntn->popularity -= PWR_NA;
            else
                curntn->popularity = 0;
        }
        if (magic(country, DEMOCRACY)) {
            curntn->eatrate += 25;
            if (curntn->terror > PWR_NA)
                curntn->terror -= PWR_NA;
            else
                curntn->terror = 0;
            if (curntn->charity < 15) {
                /* it creeps up */
                curntn->charity += 2;
            }
        }
        if (magic(country, KNOWALL))
            curntn->knowledge += PWR_NA;
        if (magic(country, ARCHITECT)) {
            if (curntn->spoilrate >= PWR_NA)
                curntn->spoilrate -= PWR_NA;
            else
                curntn->spoilrate = 1;
        }
        if (magic(country, ROADS))
            curntn->communications += 50;
        if (magic(country, DESTROYER))
            curntn->terror += PWR_NA;
        if (magic(country, ROADS))
            curntn->terror += PWR_NA;
        if (magic(country, VAMPIRE))
            curntn->terror += PWR_NA;

        switch (curntn->class) {
            case C_NPC:
                curntn->popularity += CLA_NA;
                curntn->terror += CLA_NA;
                break;
            case C_KING:
                curntn->popularity += CLA_NA;
                break;
            case C_TRADER:
                curntn->wealth += CLA_NA;
                curntn->popularity += CLA_NA;
                curntn->prestige += CLA_NA / 3;
                break;
            case C_EMPEROR:
                curntn->wealth += CLA_NA;
                curntn->popularity += CLA_NA;
                curntn->prestige += CLA_NA / 3;
                break;
            case C_WIZARD:
                curntn->knowledge += CLA_NA;
                break;
            case C_PRIEST:
                curntn->popularity += CLA_NA;
                break;
            case C_PIRATE:
                curntn->terror += CLA_NA;
                break;
                /* for warlord remember it is recursive */
            case C_WARLORD:
                curntn->prestige += CLA_NA * 2 / 3;
                break;
            case C_DEMON:
                curntn->terror += CLA_NA;
                break;
            case C_DRAGON:
                curntn->terror += CLA_NA;
                break;
            case C_SHADOW:
                curntn->terror += CLA_NA;
                break;
        }

        curntn->prestige = min(curntn->prestige, MAXTGVAL);
        curntn->popularity = min(curntn->popularity, MAXTGVAL);
        curntn->power = min(curntn->power, MAXTGVAL);
        curntn->communications = min(curntn->communications, 2 * MAXTGVAL);
        curntn->wealth = min(curntn->wealth, MAXTGVAL);
        curntn->eatrate = min(curntn->eatrate, MAXTGVAL);
        curntn->knowledge = min(curntn->knowledge, MAXTGVAL);
        curntn->farm_ability = min(curntn->farm_ability, MAXTGVAL);
        curntn->mine_ability = min(curntn->mine_ability, MAXTGVAL);
        curntn->terror = min(curntn->terror, MAXTGVAL);
        curntn->reputation = min(curntn->reputation, MAXTGVAL);
    }
}

/*
 * att_bonus - Calculate nation attribute bonuses from exotic trade goods
 *
 * Processes all map sectors to identify exotic trade goods and applies their
 * bonuses to nation attributes. This function handles the economic simulation
 * of rare resources, luxury goods, and special materials that provide significant
 * advantages to nations that control them. Includes sector designation validation,
 * trade good compatibility checking, and progressive bonus application across
 * multiple attribute categories.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   None (void function)
 *
 * Side Effects:
 *   - Modifies nation attributes based on controlled trade goods
 *   - Applies bonuses to popularity, communications, spoilrate, knowledge
 *   - Modifies farm_ability, spellpts, and terror attributes
 *   - Enforces maximum attribute limits during bonus application
 *   - Outputs progress message "working on exotic trade goods"
 *   - Only processes sectors with valid trade good access (tg_ok())
 *
 * Trade Good Categories and Effects:
 *   Popularity Enhancers (END_POPULARITY):
 *     - Luxury goods, cultural items, entertainment resources
 *     - Direct popularity bonus from trade good value
 *     - Represents improved quality of life and satisfaction
 *
 *   Communication Boosters (END_COMMUNICATION):
 *     - Transportation, communication, and infrastructure goods
 *     - Enhances information flow and coordination
 *     - Improves administrative efficiency and trade networks
 *
 *   Food Preservation (END_SPOILRATE):
 *     - Preservation techniques, storage technologies, climate control
 *     - Reduces food spoilage rate for better resource efficiency
 *     - Minimum spoilrate of 1 maintained for game balance
 *
 *   Knowledge Resources (END_KNOWLEDGE):
 *     - Educational materials, scientific instruments, cultural artifacts
 *     - Enhances research capabilities and technological advancement
 *     - Represents access to information and learning resources
 *
 *   Agricultural Enhancement (END_FARM):
 *     - Farming tools, techniques, seeds, livestock improvements
 *     - Increases agricultural productivity and efficiency
 *     - Represents advanced farming methods and crop varieties
 *
 *   Magical Components (END_SPELL):
 *     - Rare magical materials, components, and artifacts
 *     - Provides spell points based on sector population
 *     - Formula: spellpts += people/1000 + 1 (minimum 1 point per sector)
 *
 *   Terror Weapons (END_TERROR):
 *     - Military technologies, weapons, intimidation tools
 *     - Increases terror rating for psychological warfare
 *     - Represents advanced military capabilities and fear tactics
 *
 * Sector Designation Compatibility:
 *   The function validates that trade goods are compatible with sector types:
 *   - Exact match: Trade good sector type == current designation
 *   - City upgrades: DTOWN goods work in DCITY and DCAPITOL
 *   - Capital benefits: DCITY goods work in DCAPITOL
 *   - University access: DUNIVERSITY goods work in DCITY and DCAPITOL
 *   - Universal goods: 'x' type works in any sector
 *
 * Trade Good Validation:
 *   - Uses tg_ok() to verify nation has access to trade good
 *   - Checks sector ownership and control
 *   - Validates trade route access and economic control
 *   - Ensures only legitimate bonuses are applied
 *
 * Bonus Application Logic:
 *   - Value extraction: (*(tg_value+good) - '0') converts ASCII to numeric
 *   - Boundary checking: Prevents attribute overflow beyond MAXTGVAL
 *   - Progressive enhancement: Bonuses accumulate across multiple sectors
 *   - Category-specific limits: Different maximum values for different attributes
 *
 * Economic Simulation:
 *   - Represents economic advantage from rare resource control
 *   - Models trade network benefits and luxury access
 *   - Simulates technological transfer and knowledge exchange
 *   - Implements resource scarcity and competitive advantage
 *
 * Performance Considerations:
 *   - Full map scan: O(MAPX * MAPY) complexity
 *   - Trade good lookup: Constant time array access
 *   - Validation overhead: tg_ok() function calls for each sector
 *   - Progress output: Single message for user feedback
 *
 * Testing Notes:
 *   Category: B (Integration) - Requires complete world map and trade good system
 *   Approach: Integration testing with mock trade good distribution and values
 *   Key Tests: Trade good compatibility, bonus calculation, attribute limits, sector validation
 *   Dependencies: Complete sector array, trade good tables, tg_ok() validation, nation data
 *   Mock Requirements: Mock world map with trade goods, mock trade good value tables
 *   Complexity: Moderate - Map processing with trade good validation and bonus application
 * @last_documented: 2025-09-20
 */
void att_bonus(void) {
    short x, y, nation, good;
    struct s_sector *sptr;
    printf("working on exotic trade goods\n");
    for (x = 0; x < MAPX; x++)
        for (y = 0; y < MAPY; y++) {
            if (!isntn(ntn[sct[x][y].owner].active))
                continue;

            sptr = &sct[x][y];
            nation = sptr->owner;
            curntn = &ntn[nation];

            if (!tg_ok(nation, sptr))
                continue;

            good = sptr->tradegood;

            if ((*(tg_stype + good) == sptr->designation)
                || ((*(tg_stype + good) == DTOWN) && (sptr->designation == DCITY))
                || ((*(tg_stype + good) == DTOWN) && (sptr->designation == DCAPITOL))
                || ((*(tg_stype + good) == DCITY) && (sptr->designation == DCAPITOL))
                || ((*(tg_stype + good) == DUNIVERSITY) && (sptr->designation == DCITY))
                || ((*(tg_stype + good) == DUNIVERSITY) && (sptr->designation == DCAPITOL))
                || (*(tg_stype + good) == 'x')) {
                if (good <= END_POPULARITY) {
                    curntn->popularity += safe_clamp_nation_attr(*(tg_value + good) - '0');
                    curntn->popularity = safe_clamp_nation_attr(curntn->popularity);
                } else if (good <= END_COMMUNICATION) {
                    if (curntn->communications + (*(tg_value + good) - '0') < 2 * MAXTGVAL)
                        curntn->communications +=
                            safe_clamp_nation_attr(*(tg_value + good) - '0');
                    else
                        curntn->communications = 2 * MAXTGVAL;
                } else if (good <= END_EATRATE) { /* eatrate scaled already */
                    /* no tradegoods for eatrate */
                    curntn->eatrate = min(MAXTGVAL, curntn->eatrate);
                } else if (good <= END_SPOILRATE) {
                    if (curntn->spoilrate > (*(tg_value + good) - '0'))
                        curntn->spoilrate -= safe_clamp_nation_attr(*(tg_value + good) - '0');
                    else
                        curntn->spoilrate = 1;
                } else if (good <= END_KNOWLEDGE) {
                    if (curntn->knowledge + (*(tg_value + good) - '0') < MAXTGVAL)
                        curntn->knowledge += safe_clamp_nation_attr(*(tg_value + good) - '0');
                    else
                        curntn->knowledge = MAXTGVAL;
                } else if (good <= END_FARM) {
                    if (curntn->farm_ability + (*(tg_value + good) - '0') < MAXTGVAL)
                        curntn->farm_ability +=
                            safe_clamp_nation_attr(*(tg_value + good) - '0');
                    else
                        curntn->farm_ability = MAXTGVAL;
                } else if (good <= END_SPELL) {
                    curntn->spellpts += (short)(sptr->people / 1000 + 1);
                } else if (good <= END_TERROR) {
                    if (curntn->terror + (*(tg_value + good) - '0') < MAXTGVAL)
                        curntn->terror += safe_clamp_nation_attr(*(tg_value + good) - '0');
                    else
                        curntn->terror = MAXTGVAL;
                }
            }
        }
}
