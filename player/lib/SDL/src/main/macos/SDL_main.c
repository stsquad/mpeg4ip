/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000  Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@devolution.com
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id: SDL_main.c,v 1.1 2001/02/05 20:26:28 cahighlander Exp $";
#endif

/* This file takes care of command line argument parsing, and stdio redirection
   in the MacOS environment.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>	
#if TARGET_API_MACOS_CARBON
#include <Carbon.h>
#else
#include <Dialogs.h>
#include <Fonts.h>
#include <Events.h>
#include <Resources.h>
#endif

/* Include the SDL main definition header */
#include "SDL.h"
#include "SDL_main.h"
#ifdef main
#undef main
#endif

/* The standard output files */
#define STDOUT_FILE	"stdout.txt"
#define STDERR_FILE	"stderr.txt"

#if !defined(__MWERKS__) && !TARGET_API_MAC_CARBON
	/* In MPW, the qd global has been removed from the libraries */
	QDGlobals qd;
#endif

/* See if the command key is held down at startup */
static Boolean CommandKeyIsDown(void)
{
	KeyMap  theKeyMap;

	GetKeys(theKeyMap);

	if (((unsigned char *) theKeyMap)[6] & 0x80) {
		return(true);
	}
	return(false);
}

/* Parse a command line buffer into arguments */
static int ParseCommandLine(char *cmdline, char **argv)
{
	char *bufp;
	int argc;

	argc = 0;
	for ( bufp = cmdline; *bufp; ) {
		/* Skip leading whitespace */
		while ( isspace(*bufp) ) {
			++bufp;
		}
		/* Skip over argument */
		if ( *bufp == '"' ) {
			++bufp;
			if ( *bufp ) {
				if ( argv ) {
					argv[argc] = bufp;
				}
				++argc;
			}
			/* Skip over word */
			while ( *bufp && (*bufp != '"') ) {
				++bufp;
			}
		} else {
			if ( *bufp ) {
				if ( argv ) {
					argv[argc] = bufp;
				}
				++argc;
			}
			/* Skip over word */
			while ( *bufp && ! isspace(*bufp) ) {
				++bufp;
			}
		}
		if ( *bufp ) {
			if ( argv ) {
				*bufp = '\0';
			}
			++bufp;
		}
	}
	if ( argv ) {
		argv[argc] = NULL;
	}
	return(argc);
}

/* Remove the output files if there was no output written */
static void cleanup_output(void)
{
	FILE *file;
	int empty;

	/* Flush the output in case anything is queued */
	fclose(stdout);
	fclose(stderr);

	/* See if the files have any output in them */
	file = fopen(STDOUT_FILE, "rb");
	if ( file ) {
		empty = (fgetc(file) == EOF) ? 1 : 0;
		fclose(file);
		if ( empty ) {
			remove(STDOUT_FILE);
		}
	}
	file = fopen(STDERR_FILE, "rb");
	if ( file ) {
		empty = (fgetc(file) == EOF) ? 1 : 0;
		fclose(file);
		if ( empty ) {
			remove(STDERR_FILE);
		}
	}
}

/* This is where execution begins */
int main(int argc, char *argv[])
{
	int nargs;
	char **args;
	short itemHit;
	short	dummyType; /* MJS */
	Rect	dummyRect; /* MJS */
	Handle dummyHandle;
	DialogPtr commandDialog, errorDialog;
	Handle	commandLineHandle;
	Boolean	captureFile = 1;
	Str255	commandText;
	char   *commandLine;
	int     commandLineLen;
	long	i;
	

	/* Hush little compiler don't you cry,
	   Momma's gonna use argc, and I don't know why.
	   La la la la la la la la...
	 */
	if ( argc > 1 ) {
		/* Hum, command line arguments? */
	}

	/* Kyle's SDL command-line dialog code ... */
#if !TARGET_API_MAC_CARBON
	InitGraf(&qd.thePort);
#endif
	InitFonts();
#if !TARGET_API_MAC_CARBON
	InitWindows();
	InitMenus();
	InitDialogs(nil);
#endif
	InitCursor();
	FlushEvents(everyEvent,0);
#if !TARGET_API_MAC_CARBON
	MaxApplZone();
#endif
	MoreMasters();	/* What is this?? */
	MoreMasters();	/* What is this?? */

	/* Intialize SDL, and put up a dialog if we fail */
	if ( SDL_Init(0) < 0 ) {
#define kErr_OK		1
#define kErr_Text	2
		errorDialog = GetNewDialog(1001, nil, (WindowPtr)-1);
		DrawDialog( errorDialog );
		GetDialogItem(errorDialog, kErr_Text, &dummyType, &dummyHandle, &dummyRect); /* MJS */
		SetDialogItemText(dummyHandle, "\pError Initializing SDL");
		SetPort( errorDialog );
		do {
			ModalDialog( nil, &itemHit);
		} while ( itemHit != kErr_OK );
		DisposeDialog( errorDialog );
		exit(-1);
	}
	atexit(cleanup_output);
	atexit(SDL_Quit);

	/* Set up SDL's QuickDraw environment */
#if !TARGET_API_MAC_CARBON
	SDL_InitQuickDraw(&qd);
#endif

	commandLineHandle = Get1Resource( 'CLne', 128 );
	if (commandLineHandle != NULL) {
		HLock(commandLineHandle);
		/* CLne resource consist of a pascal string and a boolean */
		for ( i = 0; (i <= (*commandLineHandle)[0]) &&
		            (i < GetHandleSize(commandLineHandle)); i++ ) {
			commandText[i] = (*commandLineHandle)[i];
		}
		captureFile = (*commandLineHandle)[i];
		ReleaseResource( commandLineHandle );
	
	} else {
		commandText[0] = '\0';
	}
	if ( CommandKeyIsDown() ) {
#define kCL_OK		1
#define kCL_Cancel	2
#define kCL_Text	3
#define kCL_File	4
		commandDialog = GetNewDialog( 1000, nil, (DialogPtr)-1 );
		SetPort(commandDialog);
		GetDialogItem(commandDialog, kCL_File, &dummyType, &dummyHandle, &dummyRect); /* MJS */
		SetControlValue((ControlHandle)dummyHandle, true );
		GetDialogItem(commandDialog, kCL_Text, &dummyType, &dummyHandle, &dummyRect);
		SetDialogItemText(dummyHandle, commandText);
		do {
			ModalDialog(nil, &itemHit);
			if ( itemHit == kCL_File ) {
				GetDialogItem(commandDialog, kCL_File, &dummyType, &dummyHandle, &dummyRect); /* MJS */
				SetControlValue((ControlHandle)dummyHandle, !GetControlValue((ControlHandle)dummyHandle) );
			}
		} while (itemHit != kCL_OK && itemHit != kCL_Cancel);
		GetDialogItem(commandDialog, kCL_Text, &dummyType, &dummyHandle, &dummyRect); /* MJS */
		GetDialogItemText(dummyHandle, commandText);
		GetDialogItem(commandDialog, kCL_File, &dummyType, &dummyHandle, &dummyRect); /* MJS */
		captureFile = GetControlValue((ControlHandle)dummyHandle);
		DisposeDialog(commandDialog);

		if (itemHit == kCL_Cancel ) {
			exit(0);
		}
	}

	/* Redirect standard I/O to files */
	if ( captureFile ) {
		freopen(STDOUT_FILE, "w", stdout);
		freopen(STDERR_FILE, "w", stderr);
	} else {
		fclose(stdout);
		fclose(stderr);
	}

#if !defined(__MWERKS__) /* MJS */
	/* Copy the command line to a C string */
	commandLineLen = strlen(argv[0]) + 1 + commandText[0] + 1;
	commandLine = (char *)malloc(commandLineLen);
	if ( commandLine == NULL ) {
		exit(-1);
	}
	strcpy(commandLine, argv[0]);
	if ( commandText[0] ) {
		strcat(commandLine, " ");
		strncat(commandLine, (char *)&commandText[1], commandText[0]);
	}
	commandLine[commandLineLen-1] = '\0';
#else /* MJS -> */
	/* Just parse the whole command line */
	commandLine = (char *)malloc(commandText[0]+1);
	if ( commandLine == NULL ) {
		exit(-1);
	}
	BlockMoveData(commandText+1, commandLine, commandText[0] + 1);
	commandLine[commandText[0]] = 0;
#endif /* <- MJS */

	/* Parse it into argv and argc */
	nargs = ParseCommandLine(commandLine, NULL);
	args = (char **)malloc((nargs+1)*(sizeof *args));
	if ( args == NULL ) {
		exit(-1);
	}
	ParseCommandLine(commandLine, args);

	/* Run the main application code */
	SDL_main(nargs, args);
	free(args);
	free(commandLine);

	/* Exit cleanly, calling atexit() functions */
	exit(0);

	/* Never reached, but keeps the compiler quiet */
	return(0);
}

