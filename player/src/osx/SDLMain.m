/*   SDLMain.m - main entry point for our Cocoa-ized SDL app
       Darrell Walisser - dwaliss1@purdue.edu

    Feel free to customize this file to suit your needs
*/

#import "SDL.h"
#import "SDLMain.h"
#import "player_session_wrap.h"
#import <sys/param.h> /* for MAXPATHLEN */
#import <unistd.h>

static int    gArgc;
static char  **gArgv;

@implementation SDLMain
/* Invoked from the Quit menu item */
- (void) quit:(id)sender
{
	SDL_Event event;
	event.type = SDL_QUIT;
	SDL_PushEvent(&event);
	exit(0);
}

/* Set the working directory to the .app's parent directory */
- (void) setupWorkingDirectory
{
    char parentdir[MAXPATHLEN];
    char *c;
    
    strncpy ( parentdir, gArgv[0], MAXPATHLEN );
    c = (char*) parentdir;
    
    while (*c != '\0')     /* go to end */
        c++;
    
    while (*c != '/')      /* back up to parent */
        c--;
    
    *c = '\0';             /* cut off last part (binary name) */
    
    assert ( chdir (parentdir) == 0 );   /* chdir to the binary app's parent */
    assert ( chdir ("../../../") == 0 ); /* chdir to the .app's parent */
}

/* Called when the internal event loop has just started running */
- (void) applicationDidFinishLaunching: (NSNotification *) note
{
    /* Set the working directory to the .app's parent directory */
    [ self setupWorkingDirectory ];
}

@end

#ifdef main
#  undef main
#endif

/*
int SDL_HasAudioDelayMsec(void) { return 0; }
int SDL_AudioDelayMsec(void) { return 0; }
*/

/* Main entry point to executible - should *not* be SDL_main! */
int main (int argc, char **argv) {

    /* Copy the arguments into a global variable */
    int i;
    
    /* This is passed if we are launched by double-clicking */
    if ( argc >= 2 && strncmp (argv[1], "-psn", 4) == 0 ) {
        gArgc = 1;
    } else {
        gArgc = argc;
    }
    gArgv = (char**) malloc (sizeof(*gArgv) * (gArgc+1));
    assert (gArgv != NULL);
    for (i = 0; i < gArgc; i++) {
        gArgv[i] = argv[i];
    }
    gArgv[i] = NULL;

    player_initialize_plugins();
    NSApplicationMain (argc, argv);
    return 0;
}

