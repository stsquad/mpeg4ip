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
 "@(#) $Id: SDL_cursor_c.h,v 1.1 2001/02/05 20:26:28 cahighlander Exp $";
#endif

/* Useful variables and functions from SDL_cursor.c */

extern int  SDL_CursorInit(Uint32 flags);
extern void SDL_LockCursor(void);
extern void SDL_UnlockCursor(void);
extern void SDL_CursorPaletteChanged(void);
extern void SDL_DrawCursor(SDL_Surface *screen);
extern void SDL_EraseCursor(SDL_Surface *screen);
extern void SDL_UpdateCursor(SDL_Surface *screen);
extern void SDL_ResetCursor(void);
extern void SDL_MoveCursor(int x, int y);
extern void SDL_CursorQuit(void);

/* State definitions for the SDL cursor */
#define CURSOR_VISIBLE	0x01
#define CURSOR_USINGSW	0x10
#define SHOULD_DRAWCURSOR(X) 						\
			(((X)&(CURSOR_VISIBLE|CURSOR_USINGSW)) ==  	\
					(CURSOR_VISIBLE|CURSOR_USINGSW))

extern int SDL_cursorstate;

