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
 "@(#) $Id: SDL_lowvideo.h,v 1.1 2001/02/05 20:26:30 cahighlander Exp $";
#endif

#ifndef _SDL_lowvideo_h
#define _SDL_lowvideo_h

#if TARGET_API_MAC_CARBON
#include <Carbon.h>
#else
#include <Quickdraw.h>
#include <Palettes.h>
#include <Menus.h>
#include <DrawSprocket.h>
#endif

#ifdef HAVE_OPENGL
#include <GL/gl.h>
#include <agl.h>
#endif

#include "SDL_video.h"
#include "SDL_sysvideo.h"

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_VideoDevice *this

#if !TARGET_API_MAC_CARBON  /* not available in OS X (or more accurately, Carbon) */
/* Global QuickDraw data */
extern QDGlobals *theQD;
#endif

/* Private display data */
struct SDL_PrivateVideoData {
	GDevice **SDL_Display;
	WindowRef SDL_Window;
	SDL_Rect **SDL_modelist;
	CTabHandle SDL_CTab;
	PaletteHandle SDL_CPal;

	/* For saving and restoring the menu bar */
	short mBarHeight;

	/* Information about the last cursor position */
	Point last_where;

	/* Information about the last key modifiers */
	EventModifiers last_mods;

	/* A handle to the Apple Menu */
	MenuRef apple_menu;

	/* Information used by DrawSprocket driver */
	struct DSpInfo *dspinfo;

#ifdef HAVE_OPENGL
	AGLContext appleGLContext;
#endif
};
/* Old variable names */
#define SDL_Display		(this->hidden->SDL_Display)
#define SDL_Window		(this->hidden->SDL_Window)
#define SDL_modelist		(this->hidden->SDL_modelist)
#define SDL_CTab		(this->hidden->SDL_CTab)
#define SDL_CPal		(this->hidden->SDL_CPal)
#define mBarHeight		(this->hidden->mBarHeight)
#define last_where		(this->hidden->last_where)
#define last_mods		(this->hidden->last_mods)
#define apple_menu		(this->hidden->apple_menu)
#define glContext		(this->hidden->appleGLContext)

#endif /* _SDL_lowvideo_h */
