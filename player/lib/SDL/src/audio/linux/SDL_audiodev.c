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
 "@(#) $Id: SDL_audiodev.c,v 1.1 2001/02/05 20:26:27 cahighlander Exp $";
#endif

/* Get the name of the audio device we use for output */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "SDL_audiodev_c.h"

#ifndef _PATH_DEV_DSP
#define _PATH_DEV_DSP	"/dev/dsp"
#endif


int SDL_OpenAudioPath(char *path, int maxlen, int flags)
{
	const char *audiodev;
	int audio_fd;
	char audiopath[1024];

	/* Figure out what our audio device is */
	if ( ((audiodev=getenv("SDL_PATH_DSP")) == NULL) &&
	     ((audiodev=getenv("AUDIODEV")) == NULL) ) {
		audiodev = _PATH_DEV_DSP;
	}
	audio_fd = open(audiodev, flags, 0);

	/* If the first open fails, look for other devices */
	if ( (audio_fd < 0) && (strlen(audiodev) < (sizeof(audiopath)-3)) ) {
		int exists, instance;
		struct stat sb;

		instance = 1;
		do { /* Don't use errno ENOENT - it may not be thread-safe */
			sprintf(audiopath, "%s%d", audiodev, instance++);
			exists = 0;
			if ( stat(audiopath, &sb) == 0 ) {
				exists = 1;
				audio_fd = open(audiopath, flags, 0); 
			}
		} while ( exists && (audio_fd < 0) );
		audiodev = audiopath;
	}
	if ( path != NULL ) {
		strncpy(path, audiodev, maxlen);
		path[maxlen-1] = '\0';
	}
	return(audio_fd);
}
