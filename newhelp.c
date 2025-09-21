/*
 * newhelp.c - Help system configuration script generator
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
 * SYSTEM OVERVIEW - Help System Template Processing
 *
 * This standalone utility generates sed script files that process help file templates
 * by substituting configuration macros with actual game values. The help system uses
 * a template-based approach where help files contain placeholder tokens (like XOWNER,
 * XVERSION, XMAXPTS) that are replaced with compile-time configuration values.
 *
 * ARCHITECTURE:
 * - Reads compile-time configuration from header.h, data.h, and patchlevel.h
 * - Generates two sed script files (sed.1 and sed.2) with substitution commands
 * - Supports both numeric constants and conditional compilation features
 * - Handles map symbols, cost values, racial attributes, and magical power costs
 * - Enables dynamic help content that reflects actual game configuration
 *
 * WORKFLOW:
 * 1. Open output files (sed.1 and sed.2) for sed script generation
 * 2. Generate substitution commands for basic configuration values
 * 3. Process conditional compilation flags to generate feature descriptions
 * 4. Generate map terrain and designation symbol mappings
 * 5. Output extensive numeric configuration parameters
 * 6. Handle race-specific magical and combat attributes
 *
 * INTEGRATION:
 * - Build Process: Executed during help file generation phase
 * - Help System: Scripts used by make/build system to process help templates
 * - Configuration: Automatically reflects current compile-time settings
 * - Documentation: Enables self-documenting help system
 */

#include <stdio.h>
#include "header.h"
#include "data.h"
#include "patchlevel.h"

/*
 * main - Generate sed script files for help system template processing
 *
 * This utility function creates two sed script files (sed.1 and sed.2) that contain
 * substitution commands for processing help file templates. The help system uses a
 * template-based approach where help files contain placeholder tokens that are
 * replaced with actual compile-time configuration values, game parameters, and
 * feature settings.
 *
 * The function processes multiple categories of configuration data:
 * - Basic game parameters (version, owner, limits, costs)
 * - Conditional compilation features (OGOD, MONSTER, NPC, etc.)
 * - Map terrain and designation symbols with their character representations
 * - Economic values (taxes, costs, maintenance, capacity)
 * - Race-specific attributes for combat effectiveness and magical power costs
 * - Military, civilian, and magical power costs for all four races
 *
 * The sed scripts are split into two files due to sed's command limit constraints.
 * The first file (sed.1) handles basic configuration, features, and map symbols.
 * The second file (sed.2) handles extensive numeric parameters and racial attributes.
 *
 * ALGORITHM:
 * 1. Open sed.1 and sed.2 files for writing with error checking
 * 2. Generate basic configuration substitutions (owner, version, game limits)
 * 3. Process conditional compilation flags and generate appropriate feature text
 * 4. Create map terrain symbol substitutions showing character representations
 * 5. Generate designation symbol substitutions with formatting
 * 6. Output extensive game parameter substitutions to both files
 * 7. Process race-specific attributes for four races (Elf, Dwarf, Human, Orc)
 * 8. Generate magical power cost substitutions for military, civilian, and general magic
 * 9. Close files and exit with success status
 *
 * OUTPUT FILES:
 * - sed.1: Basic configuration, features, map symbols, and core game parameters
 * - sed.2: Extended numeric parameters, racial attributes, and magical power costs
 *
 * TEMPLATE TOKENS:
 * - XOWNER, XLOGIN, XVERSION: Basic game identification and version information
 * - XMAXPTS, XMAXARM, XMAXNAVY: Game limits and capacity constraints
 * - XOGOD, XMONSTER, XNPC: Feature availability flags (True/False)
 * - ZMOUNTAIN, ZHILL, ZCLEAR: Map terrain symbols with character display
 * - ZMINE, ZFARM, ZFORT: Designation symbols with character display
 * - XTAXFOOD, XTAXMETAL, XTAXGOLD: Economic parameters and costs
 * - XEMNTNATTR, XDMNTNATTR: Race-specific combat effectiveness attributes
 * - XEMMAG, XDMMAG, XHMMAG, XOMMAG: Magical power costs by race
 *
 * ERROR HANDLING:
 * - File creation failures result in error messages and program termination
 * - Uses FAIL exit code for file operation errors
 * - Uses SUCCESS exit code for normal completion
 *
 * INTEGRATION:
 * - Build System: Called during help file generation process
 * - Help Templates: Processed by generated sed scripts to create final help files
 * - Configuration: Automatically reflects current compile-time settings
 * - Documentation: Enables dynamic help content matching game configuration
 *
 * Parameters: None (reads from compile-time configuration headers)
 *
 * Returns: Does not return (calls exit with status code)
 *   SUCCESS (0) - Sed scripts generated successfully
 *   FAIL (-1) - File creation or writing error occurred
 *
 * Side Effects:
 *   - Creates sed.1 and sed.2 files in current working directory
 *   - Overwrites existing sed script files if present
 *   - Exits program with status code (does not return to caller)
 *
 * Testing Notes:
 *   Category: C (System) - Requires build environment and file system access
 *   Approach: System testing with mock build environment and file verification
 *   Key Tests: File creation, content verification, error handling
 *   Dependencies: Compile-time headers, file system permissions, build environment
 *   Mock Requirements: Controlled build environment with known configuration values
 *   Complexity: Moderate - straightforward file generation but extensive configuration
 */
int
main(void)
{
	FILE *fp, *fp2;
	if((fp=fopen("sed.1","w"))==NULL) {
		printf("cannot open sed.1 file\n");
		exit(FAIL);
	}
	/* needed as sed was reaching its limit on number of commands */
	if((fp2=fopen("sed.2","w"))==NULL) {	
		printf("cannot open sed.2 file\n");
		exit(FAIL);
	}

	/* create sed script */
	fprintf(fp,"s/XOWNER/%s/g\n",OWNER);
	fprintf(fp,"s/XLOGIN/%s/g\n",LOGIN);
	fprintf(fp,"s/XVERSION/%s.%s/g\n",VERSION,PATCHLEVEL);
	fprintf(fp,"s/XPVULCAN/%d/g\n",PVULCAN);
	fprintf(fp,"s/XPMOUNT/%d/g\n",PMOUNT);
	fprintf(fp,"s/XDEPLETE/%d/g\n",PDEPLETE);
	fprintf(fp,"s/XPSTORM/%d/g\n",PSTORM);
	fprintf(fp,"s/XNTOTAL/%d/g\n",NTOTAL);
	fprintf(fp,"s/XMAXPTS/%d/g\n",MAXPTS);
	fprintf(fp,"s/XMAXARM/%d/g\n",MAXARM);
	fprintf(fp,"s/XTRADEPCT/%d/g\n",TRADEPCT);
	fprintf(fp,"s/XMAXNAVY/%d/g\n",MAXNAVY);
	fprintf(fp,"s/XBREAKJIHAD/%ld/g\n",BREAKJIHAD);
	fprintf(fp,"s/XCONQENV/%s/g\n",ENVIRON_OPTS);
	fprintf(fp,"s/XNCITYCOST/%d/g\n",N_CITYCOST);
	fprintf(fp,"s/XREBUILDCOST/%ld/g\n",REBUILDCOST);

	/* check all the defined options */
#ifdef OGOD
	fprintf(fp,"s/XOGOD/(True )/g\n");
#else
	fprintf(fp,"s/XOGOD/(False)/g\n");
#endif	
#ifdef DERVDESG
	fprintf(fp,"s/XDERVDESG/Dervish can/g\n");
#else
	fprintf(fp,"s/XDERVDESG/Dervish cannot/g\n");
#endif
#ifdef SYSMAIL
	fprintf(fp,"s/XSYSMAIL/(True )/g\n");
#else
	fprintf(fp,"s/XSYSMAIL/(False)/g\n");
#endif
#ifdef SYSV
	fprintf(fp,"s/XSYSV/(True ) /g\n");
#endif
#ifdef BSD
	fprintf(fp,"s/XSYSV/(False:BSD)/g\n");
#endif
#ifdef MONSTER
	fprintf(fp,"s/XMONSTER/(True )/g\n");
#else
	fprintf(fp,"s/XMONSTER/(False)/g\n");
#endif
#ifdef NPC
	fprintf(fp,"s/XNPC/(True )/g\n");
#ifdef CMOVE
	fprintf(fp,"s/XCMOVE/(True )/g\n");
#else
	fprintf(fp,"s/XCMOVE/(False)/g\n");
#endif
#else
	fprintf(fp,"s/XCMOVE/(False)/g\n");
	fprintf(fp,"s/XNPC/(False)/g\n");
#endif
#ifdef CHEAT
	fprintf(fp,"s/XCHEAT/(True )/g\n");
#else
	fprintf(fp,"s/XCHEAT/(False)/g\n");
#endif
#ifdef RANEVENT
	fprintf(fp,"s/XRANEVENT/(True )/g\n");
#else
	fprintf(fp,"s/XRANEVENT/(False)/g\n");
#endif
#ifdef VULCANIZE
	fprintf(fp,"s/XVULCANIZE/(True )/g\n");
#else
	fprintf(fp,"s/XVULCANIZE/(False)/g\n");
#endif
#ifdef STORMS
	fprintf(fp,"s/XSTORMS/(True )/g\n");
#else
	fprintf(fp,"s/XSTORMS/(False)/g\n");
#endif
#ifdef ORCTAKE
	fprintf(fp,"s/XORCTAKE/(True )/g\n");
#else
	fprintf(fp,"s/XORCTAKE/(False)/g\n");
#endif
#ifdef HIDELOC
	fprintf(fp,"s/XHIDELOC/(True )/g\n");
#else
	fprintf(fp,"s/XHIDELOC/(False)/g\n");
#endif
#ifdef TRADE
	fprintf(fp,"s/XTRADE/(True )/g\n");
#else
	fprintf(fp,"s/XTRADE/(False)/g\n");
#endif
	/* map sectors */
	fprintf(fp,"s/ZMOUNTAIN/MOUNTAIN (%c)/g\n",MOUNTAIN);
	fprintf(fp,"s/ZHILL/HILL (%c)/g\n",HILL);
	fprintf(fp,"s/ZCLEAR/CLEAR (%c)/g\n",CLEAR);
	fprintf(fp,"s/ZWATER/WATER (%c)/g\n",WATER);
	fprintf(fp,"s/ZPEAK/PEAK (%c)/g\n",PEAK);
	fprintf(fp,"s/ZVOLCANO/VOLCANO (%c)/g\n",VOLCANO);
	fprintf(fp,"s/ZDESERT/DESERT (%c)/g\n",DESERT);
	fprintf(fp,"s/ZJUNGLE/JUNGLE (%c)/g\n",JUNGLE);
	fprintf(fp,"s/ZTUNDRA/TUNDRA (%c)/g\n",TUNDRA);
	fprintf(fp,"s/ZICE/ICE (%c)/g\n",ICE);
	fprintf(fp,"s/ZSWAMP/SWAMP (%c)/g\n",SWAMP);
	fprintf(fp,"s/ZGOOD/GOOD (%c)/g\n",GOOD);
	fprintf(fp,"s/ZFOREST/FOREST (%c)/g\n",FOREST);
	fprintf(fp,"s/ZLIGHT/LIGHT VEG (%c)/g\n",LT_VEG);
	fprintf(fp,"s/ZWOOD/WOOD (%c)/g\n",WOOD);
	fprintf(fp,"s/ZBARREN/BARREN (%c)/g\n",BARREN);

	/* designations */
	fprintf(fp,"s/ZMINE/MINE      (%c)/g\n",DMINE);
	fprintf(fp,"s/ZFARM/FARM      (%c)/g\n",DFARM);
	fprintf(fp,"s/ZGOLD/GOLD MINE (%c)/g\n",DGOLDMINE);
	fprintf(fp,"s/ZFORT/FORT      (%c)/g\n",DFORT);
	fprintf(fp,"s/ZDEVASTATED/DEVASTATED(%c)/g\n",DDEVASTATED);
	fprintf(fp,"s/ZTOWN/TOWN      (%c)/g\n",DTOWN);
	fprintf(fp,"s/ZCITY/CITY      (%c)/g\n",DCITY);
	fprintf(fp,"s/ZCAPITOL/CAPITOL   (%c)/g\n",DCAPITOL);
	fprintf(fp,"s/ZSPECIAL/SPECIAL   (%c)/g\n",DSPECIAL);
	if (DRUIN=='&')
	fprintf(fp,"s/ZRUIN/RUIN      (\\%c)/g\n",DRUIN);
	else
	fprintf(fp,"s/ZRUIN/RUIN      (%c)/g\n",DRUIN);
	fprintf(fp,"s/ZUNIVERSITY/UNIVERSITY(%c)/g\n",DUNIVERSITY);
	fprintf(fp,"s/ZBLACKSMITH/BLACKSMITH(%c)/g\n",DBLKSMITH);
	fprintf(fp,"s/ZLUMBERYARD/LUMBERYARD(%c)/g\n",DLUMBERYD);
	fprintf(fp,"s/ZROAD/ROAD      (%c)/g\n",DROAD);
	fprintf(fp,"s/ZMILL/MILL      (%c)/g\n",DMILL);
	fprintf(fp,"s/ZGRANARY/GRANARY   (%c)/g\n",DGRANARY);
	fprintf(fp,"s/ZCHURCH/CHURCH    (%c)/g\n",DCHURCH);
	fprintf(fp,"s/ZSTOCKADE/STOCKADE  (%c)/g\n",DSTOCKADE);
	fprintf(fp,"s/ZSTOCKCOST/%ld/g\n",STOCKCOST);
	
	/* other defines */
#ifdef RANEVENT
	fprintf(fp,"s/XNORANDEVENTS/%d/g\n",RANEVENT);
#else
	fprintf(fp,"s/XNORANDEVENTS/0/g\n");
#endif
	fprintf(fp,"s/XLANDSEE/%d/g\n",LANDSEE);
	fprintf(fp,"s/XNAVYSEE/%d/g\n",NAVYSEE);
	fprintf(fp,"s/XARMYSEE/%d/g\n",ARMYSEE);
	fprintf(fp,"s/XTAXFOOD/%ld/g\n",TAXFOOD);
	fprintf(fp,"s/XTAXMETAL/%ld/g\n",TAXMETAL);
	fprintf(fp,"s/XTAXGOLD/%ld/g\n",TAXGOLD);
	fprintf(fp,"s/XTAXCITY/%ld/g\n",TAXCITY);
	fprintf(fp,"s/XTAXTOWN/%ld/g\n",TAXTOWN);
	fprintf(fp,"s/XSHIPMAINT/%ld/g\n",SHIPMAINT);
	fprintf(fp,"s/XSHIPCP/%ld/g\n",SHIPHOLD);
	fprintf(fp,"s/XDESCOST/%ld/g\n",DESCOST);
	fprintf(fp,"s/XFORTCOST/%ld/g\n",FORTCOST);
	fprintf(fp,"s/XWARSHPCOST/%ld/g\n",WARSHPCOST);
	fprintf(fp,"s/XMERSHPCOST/%ld/g\n",MERSHPCOST);
	fprintf(fp,"s/XGALSHPCOST/%ld/g\n",GALSHPCOST);
	fprintf(fp,"s/XSHIPCREW/%d/g\n",SHIPCREW);
	fprintf(fp,"s/XSHIPHOLD/%ld/g\n",SHIPHOLD);
	fprintf(fp,"s/XWARSPD/%d/g\n",N_WSPD);
	fprintf(fp,"s/XGALSPD/%d/g\n",N_GSPD);
	fprintf(fp,"s/XMERSPD/%d/g\n",N_MSPD);
	fprintf(fp,"s/XSIZESPD/%d/g\n",N_SIZESPD);
	fprintf(fp,"s/XNMASK/%d/g\n",(int)N_MASK);
	fprintf(fp,"s/XCITYLIMIT/%ld/g\n",CITYLIMIT);
	fprintf(fp,"s/XMILRATIO/%ld/g\n",MILRATIO);
	fprintf(fp,"s/XMILINCAP/%ld/g\n",MILINCAP);
	fprintf(fp,"s/XBRIBE/%ld/g\n",BRIBE);
	fprintf(fp,"s/XDESFOOD/%d/g\n",DESFOOD);
	fprintf(fp,"s/XTOMUCHMINED/%ld/g\n",TOMUCHMINED);
	fprintf(fp,"s/XTOMANYPEOPLE/%ld/g\n",TOMANYPEOPLE);
	fprintf(fp,"s/XABSMAXPEOPLE/%ld/g\n",ABSMAXPEOPLE);
	fprintf(fp,"s/XFINDPERCENT/%d/g\n",FINDPERCENT);
#ifdef ORCTAKE
	fprintf(fp,"s/XTAKEPRICE/%ld/g\n",ORCTAKE);
#endif /* ORCTAKE */

	fprintf(fp,"s/XTGATTR/%d/g\n",TGATTR);
	fprintf(fp,"s/XOTHRATTR/%d/g\n",OTHRATTR);
	fprintf(fp2,"s/XGOLDATTR/%d/g\n",GOLDATTR);
	fprintf(fp2,"s/XFARMATTR/%d/g\n",FARMATTR);
	fprintf(fp2,"s/XMINEATTR/%d/g\n",MINEATTR);
	fprintf(fp2,"s/XTOWNATTR/%d/g\n",TOWNATTR);
	fprintf(fp2,"s/XCITYATTR/%d/g\n",CITYATTR);
	fprintf(fp2,"s/XCAPATTR/%d/g\n",CITYATTR);

	fprintf(fp2,"s/XDMNTNATTR/%3d/g\n",DMNTNATTR);
	fprintf(fp2,"s/XDHILLATTR/%3d/g\n",DHILLATTR);
	fprintf(fp2,"s/XDCLERATTR/%3d/g\n",DCLERATTR);
	fprintf(fp2,"s/XDCITYATTR/%3d/g\n",DCITYATTR);
	fprintf(fp2,"s/XDTOWNATTR/%3d/g\n",DTOWNATTR);
	fprintf(fp2,"s/XDGOLDATTR/%3d/g\n",DGOLDATTR);
	fprintf(fp2,"s/XDMINEATTR/%3d/g\n",DMINEATTR);
	fprintf(fp2,"s/XDFOREATTR/%3d/g\n",DFOREATTR);
	fprintf(fp2,"s/XDWOODATTR/%3d/g\n",DWOODATTR);

	fprintf(fp2,"s/XEMNTNATTR/%3d/g\n",EMNTNATTR);
	fprintf(fp2,"s/XEHILLATTR/%3d/g\n",EHILLATTR);
	fprintf(fp2,"s/XECLERATTR/%3d/g\n",ECLERATTR);
	fprintf(fp2,"s/XECITYATTR/%3d/g\n",ECITYATTR);
	fprintf(fp2,"s/XETOWNATTR/%3d/g\n",ETOWNATTR);
	fprintf(fp2,"s/XEGOLDATTR/%3d/g\n",EGOLDATTR);
	fprintf(fp2,"s/XEMINEATTR/%3d/g\n",EMINEATTR);
	fprintf(fp2,"s/XEFOREATTR/%3d/g\n",EFOREATTR);
	fprintf(fp2,"s/XEWOODATTR/%3d/g\n",EWOODATTR);

	fprintf(fp2,"s/XOMNTNATTR/%3d/g\n",OMNTNATTR);
	fprintf(fp2,"s/XOHILLATTR/%3d/g\n",OHILLATTR);
	fprintf(fp2,"s/XOCLERATTR/%3d/g\n",OCLERATTR);
	fprintf(fp2,"s/XOCITYATTR/%3d/g\n",OCITYATTR);
	fprintf(fp2,"s/XOTOWNATTR/%3d/g\n",OTOWNATTR);
	fprintf(fp2,"s/XOGOLDATTR/%3d/g\n",OGOLDATTR);
	fprintf(fp2,"s/XOMINEATTR/%3d/g\n",OMINEATTR);
	fprintf(fp2,"s/XOFOREATTR/%3d/g\n",OFOREATTR);
	fprintf(fp2,"s/XOWOODATTR/%3d/g\n",OWOODATTR);

	fprintf(fp2,"s/XHMNTNATTR/%3d/g\n",HMNTNATTR);
	fprintf(fp2,"s/XHHILLATTR/%3d/g\n",HHILLATTR);
	fprintf(fp2,"s/XHCLERATTR/%3d/g\n",HCLERATTR);
	fprintf(fp2,"s/XHCITYATTR/%3d/g\n",HCITYATTR);
	fprintf(fp2,"s/XHTOWNATTR/%3d/g\n",HTOWNATTR);
	fprintf(fp2,"s/XHGOLDATTR/%3d/g\n",HGOLDATTR);
	fprintf(fp2,"s/XHMINEATTR/%3d/g\n",HMINEATTR);
	fprintf(fp2,"s/XHFOREATTR/%3d/g\n",HFOREATTR);
	fprintf(fp2,"s/XHWOODATTR/%3d/g\n",HWOODATTR);

	/* magical power costs for races */
#ifdef ELFMAGIC
	fprintf(fp2,"s/XEMMAG/%3ldK/g\n",ELFMAGIC/1000L);
#else
	fprintf(fp2,"s/XEMMAG/%3ldK/g\n",BASEMAGIC/1000L);
#endif
#ifdef DWFMAGIC
	fprintf(fp2,"s/XDMMAG/%3ldK/g\n",DWFMAGIC/1000L);
#else
	fprintf(fp2,"s/XDMMAG/%3ldK/g\n",BASEMAGIC/1000L);
#endif
#ifdef HUMMAGIC
	fprintf(fp2,"s/XHMMAG/%3ldK/g\n",HUMMAGIC/1000L);
#else
	fprintf(fp2,"s/XHMMAG/%3ldK/g\n",BASEMAGIC/1000L);
#endif
#ifdef ORCMAGIC
	fprintf(fp2,"s/XOMMAG/%3ldK/g\n",ORCMAGIC/1000L);
#else
	fprintf(fp2,"s/XOMMAG/%3ldK/g\n",BASEMAGIC/1000L);
#endif

	/* civilian power costs for races */
#ifdef ELFCIVIL
	fprintf(fp2,"s/XECMAG/%3ldK/g\n",ELFCIVIL/1000L);
#else
	fprintf(fp2,"s/XECMAG/%3ldK/g\n",BASEMAGIC/1000L);
#endif
#ifdef DWFCIVIL
	fprintf(fp2,"s/XDCMAG/%3ldK/g\n",DWFCIVIL/1000L);
#else
	fprintf(fp2,"s/XDCMAG/%3ldK/g\n",BASEMAGIC/1000L);
#endif
#ifdef HUMCIVIL
	fprintf(fp2,"s/XHCMAG/%3ldK/g\n",HUMCIVIL/1000L);
#else
	fprintf(fp2,"s/XHCMAG/%3ldK/g\n",BASEMAGIC/1000L);
#endif
#ifdef ORCCIVIL
	fprintf(fp2,"s/XOCMAG/%3ldK/g\n",ORCCIVIL/1000L);
#else
	fprintf(fp2,"s/XOCMAG/%3ldK/g\n",BASEMAGIC/1000L);
#endif

	/* military power costs for races */
#ifdef ELFMILIT
	fprintf(fp2,"s/XEWMAG/%3ldK/g\n",ELFMILIT/1000L);
#else
	fprintf(fp2,"s/XEWMAG/%3ldK/g\n",BASEMAGIC/1000L);
#endif
#ifdef DWFMILIT
	fprintf(fp2,"s/XDWMAG/%3ldK/g\n",DWFMILIT/1000L);
#else
	fprintf(fp2,"s/XDWMAG/%3ldK/g\n",BASEMAGIC/1000L);
#endif
#ifdef HUMMILIT
	fprintf(fp2,"s/XHWMAG/%3ldK/g\n",HUMMILIT/1000L);
#else
	fprintf(fp2,"s/XHWMAG/%3ldK/g\n",BASEMAGIC/1000L);
#endif
#ifdef ORCMILIT
	fprintf(fp2,"s/XOWMAG/%3ldK/g\n",ORCMILIT/1000L);
#else
	fprintf(fp2,"s/XOWMAG/%3ldK/g\n",BASEMAGIC/1000L);
#endif
	fclose(fp);
	fclose(fp2);
	return SUCCESS;
}
