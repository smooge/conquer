/*
 * conqrast.c - Map visualization tool for Conquer games
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
#include <strings.h>
#include <string.h>
#include "../header.h"
#include "file.h"
#include HEADERFILE

/*********************************************************************
*                                                                    *
* This program draws a map of the conquer world on a bitmap system   *
*                                                                    *
* Written by Richard Caley July 1989 and hacked extensively          *
* thereafter.                                                        *
*                                                                    *
* You may copy, distribute, modify or do what you will with this     *
* code so long as this message remains in it and so long as you do   *
* not charge for it, nor distribute the program without the source.  *
*                                                                    *
* There is a horrid hack for getpass - to make this read the         *
* password from standard in one has to disconnect from the control   *
* terminal. GAG! It should be possible to disconnect just the        *
* conquer from the control terminal . . . someday.                   *
*                                                                    *
* See Readme for more details.                                       *
*                                                                    *
*********************************************************************/

#define GAP (2)			/* fudge factor - space above
				 * and below titles */

#define total_height (height*mag+ (do_title?4*GAP+2*font_height(titlefont):0))
				/* height of map and title bars */

#define yoffset(place) ((place)+(do_title?font_height(titlefont)+GAP+GAP:0))
				/* a y position */

#define tracef if(trace) printf

char *getpass();
void error(), ioerror();
char *passwd=NULL;
char *nation=NULL;
int turn;
int trace=0;
int bg=0;
char conquer_version[100];

#define conqrast_version "1.0"

#ifndef conqrast_name
#    define conqrast_name "Some conqrast" /* this is redefined in the header */
#    define driver_version "unknown driver"
#endif

#define HEADER_TAG "Conquer Version" /* just before first line of map */
#define HEADER_TAG_LENGTH (sizeof(HEADER_TAG)/sizeof(char)-1)

extern struct				/* maps from size of map to the font */
				/* to use */
    {				/* if size > mag use font */
    int mag;
    char *romanfont,*boldfont;
    } fonts[];

struct				/* Maps from terrain to brightness */
    {
    char c;			
    int val;			/* must be between 0 and 255 */
    } values[] =
        {
	' ', 256,
	'~', 0,
	'-', 64,
	'%', 128,
	'^', 192,
	'#', 224,
	'\0'
	    };

char *mapnames[] =
    {
    "This should never happen",
    "Altitude",
    "Vegetation",
    "Nation",
    "Designation"
    };

#define num_mapnames (sizeof(mapnames)/sizeof(char *))

/*********************************************************************
*                                                                    *
* Font mapping information. Fonts used by the system have symbolic   *
* names so they can be referred to on the commend line. font_table   *
* records which fonts go with which name.                            *
*                                                                    *
*********************************************************************/

enum mapfont
    {
    f_nation,
    f_designation,
    f_title,
    f_last
	};

struct 
    {
    char *name;
    font romanfont;
    font boldfont;
    } font_table[(unsigned)f_last+1] =
    {
    {"nation"},
    {"designation"},
    {"title"},
    {NULL}
    };

#define MAXDITHER 4		/* don't touch this */

short dit[1<<MAXDITHER][1<<MAXDITHER];

/*
 * set_default_fonts - Set default fonts based on map magnification level
 *
 * Automatically selects appropriate fonts for map display based on the
 * magnification factor. Searches through the global fonts[] array to find
 * fonts suitable for the given magnification level. Only sets fonts that
 * haven't been explicitly specified by the user (non-null entries in
 * font_table are preserved).
 *
 * The function implements font fallback logic: it searches for the largest
 * font that is still suitable for the given magnification. If no suitable
 * font is found, it errors out rather than using an inappropriate font.
 *
 * Parameters:
 *   mag - Map magnification factor (higher values = larger map pixels)
 *   f   - Font type enum (f_nation, f_designation, f_title) to configure
 *
 * Returns:
 *   void - Errors out if no suitable font can be found
 *
 * Side Effects:
 *   - Modifies font_table[f].romanfont if currently NULL
 *   - May modify font_table[f].boldfont if available and currently NULL
 *   - Calls font_named() to load fonts from system
 *   - Calls error() on font loading failures or no suitable fonts
 *
 * Testing Notes:
 *   Category: D (Mock Intensive) - Requires graphics system and font loading
 *   Approach: Mock font_named(), bad_font(), and error() functions
 *   Key Tests: Font selection logic, fallback behavior, error conditions
 *   Dependencies: Global fonts[] array, font_table[] array, graphics system
 *   Mock Requirements: Font loading system, error handling
 *   Complexity: Moderate - Font selection algorithm with error handling
 *
 * Notes:
 *   - Part of 1989 graphics abstraction layer
 *   - Designed for scalable map display with appropriate typography
 *   - Font selection based on readability at different magnifications
 *   - Preserves user-specified fonts (doesn't override explicit choices)
 */

void
set_default_fonts(mag,f)

int mag;
enum mapfont f;

{
    int i;

    if (font_table[(unsigned)f].romanfont==NULL) /* if there was no font specified */
	{
	for(i=0;;i++)
	    {			/* find one */
	    if (fonts[i].mag >= mag )
		break;
	    else if (fonts[i].mag < mag )
		{
		if(bad_font(font_table[(unsigned)f].romanfont=
			    font_named(fonts[i].romanfont)))
		    error("Can't get font '%s' as %s font",
			  fonts[i].romanfont,font_table[(unsigned)f].name);
		if ( font_table[(unsigned)f].boldfont==NULL && fonts[i].boldfont !=NULL)
		    if (bad_font(font_table[(unsigned)f].boldfont=
				 font_named(fonts[i].boldfont)))
			error("Can't get font '%s' as bold %s font",
			      fonts[i].romanfont,font_table[(unsigned)f].name);
		}
	    }
	if(font_table[(unsigned)f].romanfont==NULL)
	    error("No font small enough for size %d %s",mag,font_table[(unsigned)f].name);
	}
    
    }

/*
 * set_a_font - Configure a specific font from command line arguments
 *
 * Parses command line font specifications and updates the font_table
 * with user-specified fonts. Supports both roman and bold font variants
 * through a prefix naming convention. The function performs partial
 * string matching against known font categories.
 *
 * Font specification format:
 * - "nation" or prefix -> sets roman nation font
 * - "bnation" or "b" + prefix -> sets bold nation font
 * - Similar patterns for "designation" and "title" fonts
 *
 * Parameters:
 *   name     - Font category name (may have 'b' prefix for bold)
 *   fontname - System font name to load and assign
 *
 * Returns:
 *   void - Errors out if font name is unknown or font cannot be loaded
 *
 * Side Effects:
 *   - Modifies font_table[].romanfont or boldfont based on name
 *   - Calls font_named() to load the specified font from system
 *   - Calls error() on unknown font names or loading failures
 *   - Advances name pointer if 'b' prefix detected
 *
 * Testing Notes:
 *   Category: D (Mock Intensive) - Requires graphics system and font loading
 *   Approach: Mock font_named(), bad_font(), and error() functions
 *   Key Tests: Prefix parsing, partial matching, font loading, error cases
 *   Dependencies: Global font_table[] array, graphics font system
 *   Mock Requirements: Font loading system, error handling, string functions
 *   Complexity: Moderate - String parsing with font system integration
 *
 * Notes:
 *   - Command line interface for font customization (-f option)
 *   - Supports partial matching for user convenience
 *   - Bold font handling via 'b' prefix convention
 *   - Part of flexible font configuration system from 1989
 */

void
set_a_font(name,fontname)

char *name;
char *fontname;

{
    int i,bold=0,l;
    font thefont;

    if( name[0]=='b')
	{
	name++;
	bold=1;
	}

    l=strlen(name);

    for (i=0;i< (int)f_last;i++)
	if (!strncmp(name,font_table[i].name,l))
	    break;

    if ( i == (int)f_last)
	error("Unknown font name '%s'",name);

    thefont=font_named(fontname);

    if ( bad_font(thefont) )
	error("Can't get font '%s'",fontname);

    if (bold)
	font_table[i].boldfont=thefont;
    else
	font_table[i].romanfont=thefont;
    }

/*
 * scan_header - Parse Conquer map file headers and extract metadata
 *
 * Scans through map file input to locate and parse the standardized
 * Conquer map header. Extracts critical game state information including
 * map type, nation identity, turn number, and Conquer version. Handles
 * both file input and pipe input (from live Conquer process) with
 * different error handling for each case.
 *
 * Header format expected:
 * "Conquer Version X.Y : MapType Map ... Nation on Turn N"
 *
 * The function searches for HEADER_TAG ("Conquer Version") and parses
 * the structured information that follows. Map types are validated
 * against the known mapnames[] array.
 *
 * Parameters:
 *   f    - FILE pointer to read header from (file or pipe)
 *   pipe - Non-zero if input is a pipe (affects EOF error handling)
 *
 * Returns:
 *   Map type ID (1-4) corresponding to mapnames[] array index
 *   0 if map is invalid or unrecognized
 *
 * Side Effects:
 *   - Sets global conquer_version[] with parsed version string
 *   - Sets global nation pointer to identified nation or "god"
 *   - Sets global turn variable with current turn number
 *   - Advances file position past header
 *   - May call error() on parse failures or bad passwords
 *
 * Testing Notes:
 *   Category: B (Integration Required) - Requires file I/O and global state
 *   Approach: Integration testing with mock files and global variable checks
 *   Key Tests: Header parsing, map type recognition, EOF handling, pipe vs file
 *   Dependencies: Global variables (conquer_version, nation, turn), mapnames[]
 *   Mock Requirements: FILE operations, error handling, global state
 *   Complexity: Moderate - File parsing with multiple output side effects
 *
 * Notes:
 *   - Central parser for Conquer game file format from 1989
 *   - Handles both DEBUG and non-DEBUG Conquer output variants
 *   - Password validation through pipe EOF detection
 *   - Critical for identifying map content before rendering
 */

scan_header(f,pipe)

FILE *f;
int pipe;

{
    char line[1024];
    char which[100];
    static char who[100];
    int mapid;

    while ( fgets(line,1024,f) != NULL)	/* scan to top of map */
	{
	if (!strncmp(line,HEADER_TAG,HEADER_TAG_LENGTH))
	    break;
	}

    if (feof(f))
	{
	if(pipe)
	    error("Password incorrect");
	else
	    error("Unexpected end of map file");
	}

    if (sscanf(line,"Conquer Version %[0-9.] : %s Map %*s %*s %s on Turn %d",conquer_version,which,who,&turn)!=4)
	error("Bad title line (wrong version?) '%s'",line);

    for(mapid=1;mapid < num_mapnames;mapid++)
	if(!strcmp(mapnames[mapid],which))
	    break;

    if ( mapid==num_mapnames)
	mapid= 0;
    else if ( trace && !pipe)
	printf("Found %s map\n", which);

    if(!strcmp(who,"World"))
	nation="god";
    else
	nation=who;

    tracef("Nation is %s\n",nation);

    return mapid;
    }

/*
 * process_map_file - Process multiple maps from input files
 *
 * Scans through a map file to locate and register all available maps.
 * The function handles files containing multiple consecutive map dumps
 * by repeatedly calling scan_header() to identify each map and then
 * registering file positions through myopen(). This enables efficient
 * random access to different map types within the same file.
 *
 * The function implements a sophisticated file position tracking system:
 * it saves the position before each potential new map header, allowing
 * the system to rewind to the exact start of map data when needed.
 *
 * Parameters:
 *   name - Path to map file to process (must be readable)
 *
 * Returns:
 *   void - Errors out if file cannot be opened or contains bad maps
 *
 * Side Effects:
 *   - Opens the specified file for reading
 *   - Calls myopen() for each discovered map (registers with file manager)
 *   - Advances file position through all maps in the file
 *   - Leaves file open for later access by registered maps
 *   - May call error() on file access or map parsing failures
 *
 * Testing Notes:
 *   Category: B (Integration Required) - Requires file I/O and map file manager
 *   Approach: Integration testing with real map files and file position tracking
 *   Key Tests: Multiple map detection, file position management, error handling
 *   Dependencies: scan_header(), myopen(), file I/O system, HEADER_TAG
 *   Mock Requirements: File operations, map file manager (myopen/myisopen)
 *   Complexity: Moderate - File parsing with position management
 *
 * Notes:
 *   - Designed for batch processing of map dump files
 *   - Efficient file position caching for large map collections
 *   - Part of file management system for conqrast map visualization
 *   - Handles the complex multi-map file format from 1989 Conquer
 */

void
process_map_file(name)

char *name;

{
    int which;
    FILE *f;
    char line[1024];

    tracef("Searching file %s\n",name);

    if ((f=fopen(name,"r"))==NULL)
	ioerror("Can't open map file '%s'",name);

    while(1)
	{
	long top;

	which=scan_header(f,0);
    
	if(which==0)
	    error("Bad map file '%s'",name);

	myopen(which,f);
	top=ftell(f);
	while(fgets(line,1024,f)!=NULL)
	    {
	    if (!strncmp(line,HEADER_TAG,HEADER_TAG_LENGTH))
		break;
	    top=ftell(f);
	    }

	if(feof(f))
	    break;

	fseek(f,top,0);
	}
    }

/*
 * get_map_file - Retrieve maps from files or by running Conquer command
 *
 * Implements a sophisticated map acquisition system that first checks
 * for cached maps from processed files, and if not available, dynamically
 * runs the Conquer game to generate the requested map. This function
 * bridges the gap between static map files and live game data.
 *
 * The function handles the complex authentication and communication
 * protocol with the Conquer game, including password prompts, TTY
 * disconnection (a documented hack for getpass), and background
 * process management.
 *
 * Map acquisition flow:
 * 1. Check if map already available via myisopen()
 * 2. If not, prompt for password and nation (if not set)
 * 3. Disconnect from controlling TTY (getpass workaround)
 * 4. Optionally go into background mode
 * 5. Run "conquer -p" with authentication
 * 6. Parse output and register the new map
 *
 * Parameters:
 *   which   - Map type ID (1=Altitude, 2=Vegetation, 3=Nation, 4=Designation)
 *   args    - Additional arguments to pass to Conquer command
 *   tmpname - Temporary file path for Conquer output (deleted after use)
 *
 * Returns:
 *   Map type ID that was successfully acquired
 *
 * Side Effects:
 *   - May prompt user for password via getpass()
 *   - Sets global nation to "god" if not specified
 *   - Calls disconnect() to detach from controlling TTY
 *   - May call background() to become background process
 *   - Creates and deletes temporary files
 *   - Executes external "conquer" command via popen()
 *   - Registers new map via myopen()
 *
 * Testing Notes:
 *   Category: C (System Level Only) - Requires full Conquer game system
 *   Approach: System testing with real Conquer installation
 *   Key Tests: Map caching, password authentication, TTY handling, command execution
 *   Dependencies: Conquer game, myisopen/myopen system, password system, TTY
 *   Mock Requirements: Extensive - game system, authentication, process control
 *   Complexity: Complex - Multi-system integration with authentication
 *
 * Notes:
 *   - Contains documented "horrid hack" for getpass TTY disconnection
 *   - Designed for both interactive and batch operation modes
 *   - Critical component of live map visualization system
 *   - Handles authentication and process management for 1989 Unix systems
 */

int
get_map_file(which,args,tmpname)

int which;
char *args;
char *tmpname;

{
    char command[100];
    FILE *tmp;

    if ( ! myisopen(which))
	{
	if (nation==NULL)
	    {
	    tracef("Assuming god\n");

	    nation="god";
	    }

	tracef("Asking conquer for %s map\n",mapnames[which]);

	if (passwd==NULL)
	    {
	    passwd=getpass("Enter conquer password: ");
	    }

	tracef("Disconnecting ( HACK! ) bye bye\n");

	disconnect();		/* disconnect from controling tty */
	                        /* ( hack for getpass ) */

	if (bg)			/* go into background if asked */
	    {
	    tracef("Going into background\n");

	    background();
	    }

	sprintf(command,"conquer -p %s > %s 2>/dev/null",args,tmpname);
	
	if((tmp=popen(command,"w"))==NULL)
	    ioerror("can't run conquer");
	
	if (fprintf(tmp,"%s\n%s\n%c\n",nation,passwd, which+'0')==EOF)
	    error("Couldn't get map. Conquer may be updating.\n");
	
	pclose(tmp);
	
	if((tmp=fopen(tmpname,"r"))==NULL)
	    ioerror("can't open %s",tmpname);
	
	unlink(tmpname);		/* delete it */

	(void)scan_header(tmp,1);
	myopen(which,tmp);
	}

    return which;
    }

/*
 * main - Complete application with argument parsing and map rendering
 *
 * The primary entry point for the conqrast map visualization tool. This
 * function implements a comprehensive command-line interface for generating
 * graphical representations of Conquer game maps. It orchestrates the entire
 * map rendering pipeline from argument parsing through final output.
 *
 * Application workflow:
 * 1. Parse extensive command-line options (fonts, maps, display modes)
 * 2. Configure font systems based on selected map types and magnification
 * 3. Initialize graphics system (screen or memory bitmap)
 * 4. Acquire map data (from files or live Conquer process)
 * 5. Render altitude/topography layer if requested
 * 6. Render nation/designation overlays if requested
 * 7. Add decorative borders and title information
 * 8. Output to file or display on screen
 *
 * Command-line interface supports:
 * - Output control (-o file/screen)
 * - Map types (-a altitude, -n nations, -d designations, -b barbarians)
 * - Font customization (-f fonttype fontname)
 * - Live updates (-l) and background operation (-bg)
 * - World selection (-D world) and nation perspective (-N nation)
 * - Magnification control (-m factor)
 * - Terrain brightness adjustment (-cX value)
 * - Title control (-t) and help (-h)
 *
 * Parameters:
 *   argc - Command line argument count
 *   argv - Command line argument vector
 *
 * Returns:
 *   Program exit (does not return to caller)
 *
 * Side Effects:
 *   - Parses and modifies global configuration variables
 *   - Calls process_map_file() for each map file argument
 *   - Initializes graphics system via initialise_bitmaps()
 *   - Creates and manipulates bitmap graphics
 *   - May execute external Conquer commands
 *   - Outputs graphics to files or screen
 *   - Exits program when complete
 *
 * Testing Notes:
 *   Category: C (System Level Only) - Complete application integration
 *   Approach: System testing with full command-line argument sets
 *   Key Tests: Argument parsing, graphics pipeline, map rendering, output modes
 *   Dependencies: Full graphics system, file I/O, Conquer game, command-line parsing
 *   Mock Requirements: Comprehensive - entire application stack
 *   Complexity: Complex - Full application orchestration with extensive CLI
 *
 * Notes:
 *   - Complete standalone map visualization application from 1989
 *   - Sophisticated graphics rendering with multiple overlay modes
 *   - Flexible output options for batch and interactive use
 *   - Integrates multiple complex subsystems (graphics, fonts, file I/O, game comm)
 */

/*ARGSUSED*/
main(argc,argv)

int argc;
char *argv[];

{
    char line[1024];
    char desline[1024];
    char tmpname[20];
    char args[100];
    char *screen_name=get_default_screen_name();
    screen_type screen=NULL;
    int  mapfile;
    int destmapfile;
    int mag=11;
    bitmap pr;
    int width,height;
    extern char *myname;
    int x,y,i,j,val;
    char c;
    char *mapname=NULL;
    char *world=NULL;
    int do_nations=0,barbarians=0,markbarbs=0;
    int all=0,live=0,altitude=1;
    int desig=0;
    int do_title=1;
    font titlefont;
    int boldtitle=0;		/* indicates that the title font should be
				   emboldened - ie `titlefont' is roman and
				   so _we_ must do the bold */


    myname=argv[0];

    strcpy(args,"");

    while(*(++argv)!=NULL)
	if (!strncmp(*argv,"-o",2))
	    {
	    mapname= *(++argv);
	    if ( is_screenname(mapname))
		{
		screen_name=mapname;
		mapname=NULL;
		}
	    }
	else if (!strcmp(*argv,"-l"))
	    live++;
	else if (!strcmp(*argv,"-D"))
	    sprintf(args,"%s -d %s",args,world= *(++argv));
	else if (!strcmp(*argv,"-N"))
	    nation= *(++argv);
	else if (!strcmp(*argv,"-bg"))
	    bg++;
	else if (!strcmp(*argv,"-a"))
	    altitude=0;
	else if (!strcmp(*argv,"-f"))
	    {
	    char *which= *(++argv), *fontname= *(++argv);
	    set_a_font(which,fontname);
	    }
	else if (!strncmp(*argv,"-b",2))
	    {
	    barbarians++;
	    if ( (*argv)[2]=='a')
		markbarbs++;
	    }
	else if (!strcmp(*argv,"-m"))
	    mag=atoi(*(++argv));
	else if (!strncmp(*argv,"-c",2))
	    {
	    c=(*argv)[2];
	    for (i=0;;i++)
		{
		if (values[i].c =='\0')
		    error("Unknown contour '%c'",c);
		else if ( values[i].c == c)
		    {
		    values[i].val=atoi(*(++argv));
		    break;
		    }
		}
	    }
	else if (!strncmp(*argv,"-n",2))
	    {
	    do_nations++;
	    if ( (*argv)[2]=='a')
		++all;
	    }
	else if (!strncmp(*argv,"-d",2))
	    {
	    desig++;
	    }
	else if (!strcmp(*argv,"-h"))
	    help_message(myname);
	else if (!strcmp(*argv,"-t"))
	    do_title=0;
	else if (!strcmp(*argv,"-v"))
	    trace++;
	else if ( (*argv)[0]=='-')
	    {
	    printf("Unknown option '%s'\n",*argv);
	    help_message(myname);
	    }
        else
	    process_map_file(*argv);

    if (do_nations)		/* if we have to put in nations */
	set_default_fonts(mag,f_nation);

    if (desig)
	{
	if (all)
	    error("Only one of -d and -na allowed!");

	set_default_fonts(mag,f_designation);
	}

    if (do_title)
	{
	set_default_fonts(mag+2,f_title);
	if ( (titlefont=font_table[(unsigned)f_title].boldfont)==NULL)
	    {
	    boldtitle=1;
	    titlefont=font_table[(unsigned)f_title].romanfont;
	    }

	}

    screen=screen_named(screen_name);

    sprintf(tmpname,"/tmp/conq%d",getpid());

    mapfile=get_map_file(altitude?1:3, /* get a map */
			 args,tmpname);

    if(mygets(line,1024,mapfile)==NULL)
	error("empty map!");

    width=strlen(line)-1;	/* count height and width */

    for(height=1;mygets(line,1024,mapfile)!=NULL;height++)
	if(!strncmp(line,HEADER_TAG,HEADER_TAG_LENGTH)||
	   !strncmp(line,"reading",7))
	    break;

    tracef("Size %d X %d, image %d X %d\n",width,height,width*mag,height*mag);

    initialise_bitmaps();

    if(live)			/* if live do it on the screen */
	{
	if ((pr=get_screen_bitmap(screen))==(bitmap)NULL)
	    ioerror("Can't open screen %s",screen_name);
	}
    else			/* otherwise in memory */
	pr=create_bitmap(width*mag,total_height);

    myrewind(mapfile);

    if (altitude)		/* if needed do altitude */
	{
	int xx,yy;
	tracef("Drawing topography\n");

	build_dither_matrix(MAXDITHER);

	yy=yoffset(0);
	for(y=0;y<height;y++,yy+=mag)
	    {
	    if (mygets(line,1024,mapfile)==NULL)
		error("unexpected end of file!");
	
	    for(xx=x=0;x<width;x++,xx+=mag)
		{
		val= -1;
		for(i=0;values[i].c!='\0';i++)
		    if ( values[i].c==line[x] )
			val=values[i].val;
	    
		if ( val<0 )
		    {		/* non fatal, I like it that way */
		    printf("unknown character %c\n",line[x]);
		    continue;
		    }
	    
		for(i=0;i<mag;i++)
		    for(j=0;j<mag;j++)
			{
			set_bit(pr,xx+i,yy+j,dit[(xx+i)%16][(yy+j)%16] >=val?1:0);
			}
		}
	    }
    
	myclose(mapfile);
	}

    if(do_nations||desig)	/* nations */
	{
	font nf, bnf;
	font df;
	int xx,yy;
	char des,
	current='\0';		/* keeps track of which nation we are in */
	char last[1024];	/* ditto vertically for each column */

	tracef("Drawing nations\n");

	if (do_nations)
	    {
	    nf = font_table[(unsigned)f_nation].romanfont;
	    bnf = font_table[(unsigned)f_nation].boldfont;
	    }

	if (desig)
	    {
	    df = font_table[(unsigned)f_designation].romanfont;
	    }

	if (altitude)		/* if no altitude this was gotten to */
	    /* count the size */
	    mapfile=get_map_file(3,args,tmpname);

	destmapfile=get_map_file(4,args,tmpname);

	yy=yoffset(mag);
	for(y=0;y<height;y++,yy+=mag)
	    {
	    if(mygets(line,1024,mapfile)==NULL)
		error("unexpected end of file in nation map");
	    if(mygets(desline,1024,destmapfile)==NULL)
		error("unexpected end of file in designation map");

	    current='\0';	       
	    for(x=0;x<width;x++)
		{
		c=line[x];
		des=desline[x];

		if (index("~-%#^",c)!=NULL) /* if no-one owns */
		    c=' ';
		else if ( !barbarians && c == '*' ) /* if barbarian or lizard */
		    c=' ';

		xx=x*mag;
	    
		if (do_nations && c !=current )	/* crossing a vertical border */
		    {
		    draw_line(pr,xx-1,yy,xx-1,yy-mag,BLACK);
		    draw_line(pr,xx,yy,xx,yy-mag,WHITE);
		    draw_line(pr,xx+1,yy,xx+1,yy-mag,BLACK);
		    current=c;
		    }

		if ( do_nations & last[x] != c)	/* horizontal border above */
		    {
		    draw_line(pr,xx,yy-mag,xx+mag,yy-mag,WHITE);
		    draw_line(pr,xx,yy-mag+1,xx+mag,yy-mag+1,BLACK);
		    draw_line(pr,xx,yy-mag-1,xx+mag,yy-mag-1,BLACK);
		    last[x]=c;
		    }
		
		/*
		 * The following is convoluted to get the right things bold
		 * I think it copes with all cases 
		 */

		if ( do_nations && des == 'C' )
		    {
		    if (all||desig)
			{
			if (bnf)
			    put_txt(pr,xx,yy,bnf,c,0);
			else
			    put_txt(pr,xx,yy,nf,c,1);
			}
		    else
			put_txt(pr,xx,yy,nf,c,0);
		    }
		else if (do_nations && all && c != ' ' && ( c != '*' || markbarbs ) )
		    put_txt(pr,xx,yy,nf,c,0);
		else if (desig && des != '-')
		    put_txt(pr,xx,yy,df,des,0);
		}
	    }
	    
	    
	myclose(mapfile);
	myclose(destmapfile);
	}

				/* line down right */
    draw_line(pr,mag*width-1,yoffset(0),mag*width-1,yoffset(mag*height),BLACK);
    draw_line(pr,mag*width-2,yoffset(0),mag*width-2,yoffset(mag*height),WHITE);
    draw_line(pr,mag*width-3,yoffset(0),mag*width-3,yoffset(mag*height),BLACK);
				/* left */
    draw_line(pr,0,yoffset(0),0,yoffset(mag*height),BLACK);
    draw_line(pr,1,yoffset(0),1,yoffset(mag*height),WHITE);
    draw_line(pr,2,yoffset(0),2,yoffset(mag*height),BLACK);
				/* top */
    draw_line(pr,0,yoffset(0),mag*width,yoffset(0),BLACK);
    draw_line(pr,0,yoffset(1),mag*width,yoffset(1),WHITE);
    draw_line(pr,0,yoffset(2),mag*width,yoffset(2),BLACK);
				/* bottom */
    draw_line(pr,0,yoffset(mag*height-1),mag*width,yoffset(mag*height-1),BLACK);
    draw_line(pr,0,yoffset(mag*height-2),mag*width,yoffset(mag*height-2),WHITE);
    draw_line(pr,0,yoffset(mag*height-3),mag*width,yoffset(mag*height-3),BLACK);

    if (do_title)
	{
	sprintf(line,"Conquer Version %s",conquer_version);
	put_txt(pr,mag,
		font_baseline(titlefont)+GAP+1,titlefont,'\0',boldtitle,line);

	sprintf(line,"%s Version %s/%s",conqrast_name,conqrast_version,driver_version);
	put_txt(pr,width*mag-strlen(line)*font_width(titlefont)-GAP-GAP,
		font_baseline(titlefont)+GAP+1,titlefont,'\0',boldtitle,line);

	if ( nation==NULL || !strcmp(nation,"god"))
	    sprintf(line,"Conquer World Map For Turn %d",turn);
	else
	    sprintf(line,"Conquer Map For Nation %s On Turn %d",nation,turn);

	put_txt(pr,(width*mag-strlen(line)*font_width(titlefont))/2,
		height*mag+GAP+font_height(titlefont)+GAP+
		GAP+font_baseline(titlefont),titlefont,'\0',boldtitle,line);
	}

    if (mapname !=NULL)		/* told to save in file */
	{
	write_bitmap(pr,mapname,width*mag,total_height);
	destroy_bitmap(pr);
	}
    else if (!live)		/* not drawn already */
	{
	display_bitmap(pr,0,0,width*mag,total_height);
	destroy_bitmap(pr);
	}
    else
	destroy_bitmap(pr);

    finish_bitmaps();

    tracef("done\n");

    }

/*
 * put_txt - Text rendering with highlighting and outline effects
 *
 * Renders text characters or strings onto bitmaps with sophisticated
 * visual effects including outline (shadow) and bold highlighting.
 * The function implements a multi-pass rendering technique to create
 * readable text that stands out against complex map backgrounds.
 *
 * Rendering technique:
 * 1. Black outline pass: Renders black text in 3x3 (or 4x4 if highlighted)
 *    grid around the target position for outline effect
 * 2. White text pass: Renders white text at target position for contrast
 * 3. Bold effect: Achieved by "smearing" (rendering multiple offset copies)
 *
 * The function supports both single character and string rendering modes
 * determined by the character parameter. When c is '\0', the str parameter
 * is rendered as a complete string.
 *
 * Parameters:
 *   pr        - Bitmap to render text onto
 *   xx        - X coordinate for text placement (adjusted internally)
 *   yy        - Y coordinate for text placement (adjusted internally)
 *   pf        - Font to use for rendering
 *   c         - Character to render ('\0' means use str parameter instead)
 *   highlight - Non-zero enables bold/highlighting effect
 *   str       - String to render (used when c is '\0')
 *
 * Returns:
 *   void
 *
 * Side Effects:
 *   - Modifies the bitmap by drawing text with outline effects
 *   - Uses static buffer for single character to string conversion
 *   - Calls bitmap_text() multiple times for layered rendering
 *   - Adjusts coordinates internally for proper outline positioning
 *
 * Testing Notes:
 *   Category: D (Mock Intensive) - Requires graphics system and bitmap operations
 *   Approach: Mock bitmap_text(), create test bitmaps, verify rendering calls
 *   Key Tests: Single char vs string, highlight effects, coordinate adjustment
 *   Dependencies: Graphics system, bitmap operations, font system
 *   Mock Requirements: bitmap_text(), font system, coordinate calculations
 *   Complexity: Moderate - Multi-pass rendering with coordinate manipulation
 *
 * Notes:
 *   - 1989 graphics technique for readable text on complex backgrounds
 *   - Bold effect described as "smearing" by original author
 *   - Outline technique creates drop-shadow effect for visibility
 *   - Dual-mode operation (character vs string) for flexibility
 *   - Part of sophisticated map labeling system
 */

/*VARARGS6*/
put_txt(pr,xx,yy,pf,c,highlight,str)

bitmap pr;
int xx,yy;
font pf;
char c;
int highlight;
char *str;

{
    int i,j,size;
    static char st[2];
    int x,y;

    if ( c !='\0' )
	{
	st[0]=c;
	st[1]='\0';
	str=st;
	}

    if (highlight)
	size=1;
    else
	size=0;

    xx+= 2;
    yy-= 2;

    for(i=0,x=xx-1; i<3+size; i++,x++)
	for(j=0,y=yy+1; j<3+size; j++,y--)
	    bitmap_text(pr,x,y,DRAW_BLACK,pf,str);


    for(i=0,x=xx;i<1+size;i++,x++)
	for(j=0,y=yy;j<1+size;j++,y--)
	    bitmap_text(pr,x,y,DRAW_WHITE,pf,str);

    }

