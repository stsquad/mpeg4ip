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
 "@(#) $Id: SDL_macevents.c,v 1.1 2001/02/05 20:26:30 cahighlander Exp $";
#endif

#include <stdio.h>

#if TARGET_API_MAC_CARBON
#include <Carbon.h>
#else
#include <Script.h>
#include <LowMem.h>
#include <Devices.h>
#include <DiskInit.h>
#include <ToolUtils.h>
#endif

#include "SDL_events.h"
#include "SDL_video.h"
#include "SDL_error.h"
#include "SDL_syswm.h"
#include "SDL_events_c.h"
#include "SDL_cursor_c.h"
#include "SDL_sysevents.h"
#include "SDL_macevents_c.h"
#include "SDL_mackeys.h"

/* Macintosh resource constants */
#define mApple	128			/* Apple menu resource */
#define iAbout	1			/* About menu item */

/* Functions to handle the About menu */
static void Mac_DoAppleMenu(_THIS, long item);

/* The translation table from a macintosh key scancode to a SDL keysym */
static SDLKey MAC_keymap[256];
static SDL_keysym *TranslateKey(EventRecord *ev,SDL_keysym *keysym,int pressed);

/* Handle activation and deactivation  -- returns whether an event was posted */
static int Mac_HandleActivate(int activate)
{
	if ( activate ) {
		if ( !(SDL_cursorstate & CURSOR_VISIBLE) ) {
			HideCursor();
		}
		SDL_SetCursor(NULL);
	} else {
		if ( !(SDL_cursorstate & CURSOR_VISIBLE) ) {
			ShowCursor();
		}
#if TARGET_API_MAC_CARBON
		SetCursor(GetQDGlobalsArrow(NULL));
#else
		SetCursor(&theQD->arrow);
#endif
	}
	return(SDL_PrivateAppActive(activate, SDL_APPACTIVE));
}

static void myGlobalToLocal(_THIS, Point *pt)
{
	if ( SDL_VideoSurface && !(SDL_VideoSurface->flags&SDL_FULLSCREEN) ) {
		WindowPtr saveport;
		GetPort(&saveport);
		SetPort(SDL_Window);
		GlobalToLocal(pt);
		SetPort(saveport);
	}
}

/* The main MacOS event handler */
static int Mac_HandleEvents(_THIS, int wait4it)
{
	int i;
	EventRecord event;

#define cooperative_multitasking 1
	/* If we call WaitNextEvent(), MacOS will check other processes
	 * and allow them to run, and perform other high-level processing.
	 */
	if ( cooperative_multitasking || wait4it ) {
		UInt32 wait_time;

		/* Are we polling or not? */
		if ( wait4it ) {
			wait_time = 2147483647;
		} else {
			wait_time = 0;
		}
		WaitNextEvent(everyEvent, &event, wait_time, nil);
	} else {
		GetOSEvent(everyEvent, &event);
	}

	/* Check for special non-event keys */
	/* FIXME: these key events are delivered even when in background */
	if ( event.modifiers != last_mods ) {
		static struct {
			EventModifiers mask;
			SDLKey key;
		} mods[] = {
			{ cmdKey,		SDLK_LMETA },
			{ shiftKey,		SDLK_LSHIFT },
			{ rightShiftKey,	SDLK_RSHIFT },
			{ alphaLock,		SDLK_CAPSLOCK },
			{ optionKey,		SDLK_LALT },
			{ rightOptionKey,	SDLK_RALT },
			{ controlKey,		SDLK_LCTRL },
			{ rightControlKey,	SDLK_RCTRL },
			{ 0,			0 }
		};
		SDL_keysym keysym;
		Uint8 mode;
		EventModifiers mod, mask;
		

		/* Set up the keyboard event */
		keysym.scancode = 0;
		keysym.sym = SDLK_UNKNOWN;
		keysym.mod = KMOD_NONE;
		keysym.unicode = 0;

		/* See what has changed, and generate events */
		mod = event.modifiers;
		for ( i=0; mods[i].mask; ++i ) {
			mask = mods[i].mask;
			if ( (mod&mask) != (last_mods&mask) ) {
				keysym.sym = mods[i].key;
				if ( (mod&mask) ||
				     (mods[i].key == SDLK_CAPSLOCK) ) {
					mode = SDL_PRESSED;
				} else {
					mode = SDL_RELEASED;
				}
				SDL_PrivateKeyboard(mode, &keysym);
			}
		}

		/* Save state for next time */
		last_mods = mod;
	}

	/* Check for mouse motion */
	if ( (event.where.h != last_where.h) ||
	     (event.where.v != last_where.v) ) {
		Point pt;
		pt = last_where = event.where;
		myGlobalToLocal(this, &pt);
		SDL_PrivateMouseMotion(0, 0, pt.h, pt.v);
	}

	switch (event.what) {
	  case mouseDown: {
		WindowRef win;
		short area;
				
		area = FindWindow(event.where, &win);
		switch (area) {
		  case inMenuBar: /* Only the apple menu exists */
			Mac_DoAppleMenu(this, MenuSelect(event.where));
			HiliteMenu(0);
			break;
		  case inDrag:
#if TARGET_API_MAC_CARBON
			DragWindow(win, event.where, NULL);
#else
			DragWindow(win, event.where, &theQD->screenBits.bounds);
#endif
			break;
		  case inGoAway:
			if ( TrackGoAway(win, event.where) ) {
				SDL_PrivateQuit();
			}
			break;
		  case inContent:
			myGlobalToLocal(this, &event.where);
			/* Treat command-click as right mouse button */
			if ( event.modifiers & optionKey ) {
			    SDL_PrivateMouseButton(SDL_PRESSED,
					2,event.where.h,event.where.v);
			}
			else if ( event.modifiers & cmdKey ) {
			    SDL_PrivateMouseButton(SDL_PRESSED,
					3,event.where.h,event.where.v);
			} else {
			    SDL_PrivateMouseButton(SDL_PRESSED,
					1,event.where.h,event.where.v);
			}
			break;
#ifdef USE_APPEARANCE_MANAGER
		  /* FIXME: There's no grow box by default, how to add? */
		  case inGrow: {
			int newSize;

			/* Don't allow resize if video mode isn't resizable */
			if ( ! SDL_PublicSurface ||
			     ! (SDL_PublicSurface->flags & SDL_RESIZABLE) ) {
				break;
			}
			newSize = GrowWindow(win, event.where, &theQD->screenBits.bounds);
			if ( newSize ) {
				SizeWindow ( win, LoWord (newSize), HiWord (newSize), 1 );
				EraseRect ( &theQD->screenBits.bounds );
// The next comment not sure because it produces some strange results
//				int noerr;
/*				noerr = aglSetDrawable(glContext, (AGLDrawable)SDL_Window);
				if(!noerr) {
					SDL_SetError("Unable to bind GL context to window");
					return(-1);
				}
				Mac_GL_MakeCurrent ( this );
*/
				SDL_PrivateResize ( LoWord (newSize), HiWord (newSize) );
			}
		  	} break;
		  /* FIXME: There's no zoom box by default, how to add? */
		  case inZoomIn:
			if ( TrackBox (win, event.where, area )) {
				EraseRect ( &theQD->screenBits.bounds );
				ZoomWindow ( win, area, 0);
				// There should be something done like in inGrow case, but...
			}
			break;
		  /* FIXME: There's no zoom box by default, how to add? */
		  case inZoomOut:
			if ( TrackBox (win, event.where, area )) {
				EraseRect ( &theQD->screenBits.bounds );
				ZoomWindow ( win, area, 0);
				// There should be something done like in inGrow case, but...
			}
			break;
		  case inCollapseBox:
			if ( TrackBox (win, event.where, area )) {
				if ( IsWindowCollapsable(win) ) {
					CollapseWindow (win, !IsWindowCollapsed(win));
					// There should be something done like in inGrow case, but...
				}
			}
			break;
#endif /* USE_APPEARANCE_MANAGER */
		  case inSysWindow:
#if TARGET_API_MAC_CARBON
			/* Never happens in Carbon? */
#else
			SystemClick(&event, win);
#endif
			break;
		  default:
			break;
		}
	  }
	  break;
	  case mouseUp: {
		myGlobalToLocal(this, &event.where);
		/* Treat command-click as right mouse button */
		if ( event.modifiers & cmdKey ) {
		    SDL_PrivateMouseButton(SDL_RELEASED,
				3, event.where.h, event.where.v);
		}
		else if ( event.modifiers & optionKey ) {
		    SDL_PrivateMouseButton(SDL_RELEASED,
				2,event.where.h,event.where.v);
		} else {
		    SDL_PrivateMouseButton(SDL_RELEASED,
				1, event.where.h, event.where.v);
		}
	  }
	  break;
	  case keyDown: {
		SDL_keysym keysym;

		SDL_PrivateKeyboard(SDL_PRESSED,
			TranslateKey(&event, &keysym, 1));
	  }
	  break;
	  case keyUp: {
		SDL_keysym keysym;

		SDL_PrivateKeyboard(SDL_RELEASED,
			TranslateKey(&event, &keysym, 0));
	  }
	  break;
	  case updateEvt: {
		BeginUpdate(SDL_Window);
		if ( (SDL_VideoSurface->flags & SDL_HWSURFACE) ==
						SDL_SWSURFACE ) {
			SDL_UpdateRect(SDL_VideoSurface, 0, 0, 0, 0);
		}
		EndUpdate(SDL_Window);
	  }
	  break;
	  case activateEvt: {
		Mac_HandleActivate(!!(event.modifiers & activeFlag));
	  }
	  break;
	  case diskEvt: {
#if TARGET_API_MAC_CARBON
		/* What are we supposed to do? */;
#else
		if ( ((event.message>>16)&0xFFFF) != noErr ) {
			Point spot;
			SetPt(&spot, 0x0070, 0x0050);
			DIBadMount(spot, event.message);
		}
#endif
	  }
	  break;
	  case osEvt: {
		switch ((event.message>>24) & 0xFF) {
#if 0 /* Handled above the switch statement */
		  case mouseMovedMessage: {
			myGlobalToLocal(this, &event.where);
			SDL_PrivateMouseMotion(0, 0,
					event.where.h, event.where.v);
		  }
		  break;
#endif /* 0 */
		  case suspendResumeMessage: {
			Mac_HandleActivate(!!(event.message & resumeFlag));
		  }
		  break;
		}
	  }
	  break;
	  default: {
		;
	  }
	  break;
	}
	return (event.what != nullEvent);
}


void Mac_PumpEvents(_THIS)
{
	/* Process pending MacOS events */
	while ( Mac_HandleEvents(this, 0) ) {
		/* Loop and check again */;
	}
}

void Mac_InitOSKeymap(_THIS)
{
	int i;

	/* Map the MAC keysyms */
	for ( i=0; i<SDL_TABLESIZE(MAC_keymap); ++i )
		MAC_keymap[i] = SDLK_UNKNOWN;

	/* Defined MAC_* constants */
	MAC_keymap[MK_ESCAPE] = SDLK_ESCAPE;
	MAC_keymap[MK_F1] = SDLK_F1;
	MAC_keymap[MK_F2] = SDLK_F2;
	MAC_keymap[MK_F3] = SDLK_F3;
	MAC_keymap[MK_F4] = SDLK_F4;
	MAC_keymap[MK_F5] = SDLK_F5;
	MAC_keymap[MK_F6] = SDLK_F6;
	MAC_keymap[MK_F7] = SDLK_F7;
	MAC_keymap[MK_F8] = SDLK_F8;
	MAC_keymap[MK_F9] = SDLK_F9;
	MAC_keymap[MK_F10] = SDLK_F10;
	MAC_keymap[MK_F11] = SDLK_F11;
	MAC_keymap[MK_F12] = SDLK_F12;
	MAC_keymap[MK_PRINT] = SDLK_PRINT;
	MAC_keymap[MK_SCROLLOCK] = SDLK_SCROLLOCK;
	MAC_keymap[MK_PAUSE] = SDLK_PAUSE;
	MAC_keymap[MK_POWER] = SDLK_POWER;
	MAC_keymap[MK_BACKQUOTE] = SDLK_BACKQUOTE;
	MAC_keymap[MK_1] = SDLK_1;
	MAC_keymap[MK_2] = SDLK_2;
	MAC_keymap[MK_3] = SDLK_3;
	MAC_keymap[MK_4] = SDLK_4;
	MAC_keymap[MK_5] = SDLK_5;
	MAC_keymap[MK_6] = SDLK_6;
	MAC_keymap[MK_7] = SDLK_7;
	MAC_keymap[MK_8] = SDLK_8;
	MAC_keymap[MK_9] = SDLK_9;
	MAC_keymap[MK_0] = SDLK_0;
	MAC_keymap[MK_MINUS] = SDLK_MINUS;
	MAC_keymap[MK_EQUALS] = SDLK_EQUALS;
	MAC_keymap[MK_BACKSPACE] = SDLK_BACKSPACE;
	MAC_keymap[MK_INSERT] = SDLK_INSERT;
	MAC_keymap[MK_HOME] = SDLK_HOME;
	MAC_keymap[MK_PAGEUP] = SDLK_PAGEUP;
	MAC_keymap[MK_NUMLOCK] = SDLK_NUMLOCK;
	MAC_keymap[MK_KP_EQUALS] = SDLK_KP_EQUALS;
	MAC_keymap[MK_KP_DIVIDE] = SDLK_KP_DIVIDE;
	MAC_keymap[MK_KP_MULTIPLY] = SDLK_KP_MULTIPLY;
	MAC_keymap[MK_TAB] = SDLK_TAB;
	MAC_keymap[MK_q] = SDLK_q;
	MAC_keymap[MK_w] = SDLK_w;
	MAC_keymap[MK_e] = SDLK_e;
	MAC_keymap[MK_r] = SDLK_r;
	MAC_keymap[MK_t] = SDLK_t;
	MAC_keymap[MK_y] = SDLK_y;
	MAC_keymap[MK_u] = SDLK_u;
	MAC_keymap[MK_i] = SDLK_i;
	MAC_keymap[MK_o] = SDLK_o;
	MAC_keymap[MK_p] = SDLK_p;
	MAC_keymap[MK_LEFTBRACKET] = SDLK_LEFTBRACKET;
	MAC_keymap[MK_RIGHTBRACKET] = SDLK_RIGHTBRACKET;
	MAC_keymap[MK_BACKSLASH] = SDLK_BACKSLASH;
	MAC_keymap[MK_DELETE] = SDLK_DELETE;
	MAC_keymap[MK_END] = SDLK_END;
	MAC_keymap[MK_PAGEDOWN] = SDLK_PAGEDOWN;
	MAC_keymap[MK_KP7] = SDLK_KP7;
	MAC_keymap[MK_KP8] = SDLK_KP8;
	MAC_keymap[MK_KP9] = SDLK_KP9;
	MAC_keymap[MK_KP_MINUS] = SDLK_KP_MINUS;
	MAC_keymap[MK_CAPSLOCK] = SDLK_CAPSLOCK;
	MAC_keymap[MK_a] = SDLK_a;
	MAC_keymap[MK_s] = SDLK_s;
	MAC_keymap[MK_d] = SDLK_d;
	MAC_keymap[MK_f] = SDLK_f;
	MAC_keymap[MK_g] = SDLK_g;
	MAC_keymap[MK_h] = SDLK_h;
	MAC_keymap[MK_j] = SDLK_j;
	MAC_keymap[MK_k] = SDLK_k;
	MAC_keymap[MK_l] = SDLK_l;
	MAC_keymap[MK_SEMICOLON] = SDLK_SEMICOLON;
	MAC_keymap[MK_QUOTE] = SDLK_QUOTE;
	MAC_keymap[MK_RETURN] = SDLK_RETURN;
	MAC_keymap[MK_KP4] = SDLK_KP4;
	MAC_keymap[MK_KP5] = SDLK_KP5;
	MAC_keymap[MK_KP6] = SDLK_KP6;
	MAC_keymap[MK_KP_PLUS] = SDLK_KP_PLUS;
	//MAC_keymap[MK_LSHIFT] = SDLK_LSHIFT;
	MAC_keymap[MK_z] = SDLK_z;
	MAC_keymap[MK_x] = SDLK_x;
	MAC_keymap[MK_c] = SDLK_c;
	MAC_keymap[MK_v] = SDLK_v;
	MAC_keymap[MK_b] = SDLK_b;
	MAC_keymap[MK_n] = SDLK_n;
	MAC_keymap[MK_m] = SDLK_m;
	MAC_keymap[MK_COMMA] = SDLK_COMMA;
	MAC_keymap[MK_PERIOD] = SDLK_PERIOD;
	MAC_keymap[MK_SLASH] = SDLK_SLASH;
	MAC_keymap[MK_RSHIFT] = SDLK_RSHIFT;
	MAC_keymap[MK_UP] = SDLK_UP;
	MAC_keymap[MK_KP1] = SDLK_KP1;
	MAC_keymap[MK_KP2] = SDLK_KP2;
	MAC_keymap[MK_KP3] = SDLK_KP3;
	MAC_keymap[MK_KP_ENTER] = SDLK_KP_ENTER;
	MAC_keymap[MK_LCTRL] = SDLK_LCTRL;
	MAC_keymap[MK_LALT] = SDLK_LALT;
	MAC_keymap[MK_LMETA] = SDLK_LMETA;
	MAC_keymap[MK_SPACE] = SDLK_SPACE;
	MAC_keymap[MK_RMETA] = SDLK_RMETA;
	MAC_keymap[MK_RALT] = SDLK_RALT;
	MAC_keymap[MK_RCTRL] = SDLK_RCTRL;
	MAC_keymap[MK_LEFT] = SDLK_LEFT;
	MAC_keymap[MK_DOWN] = SDLK_DOWN;
	MAC_keymap[MK_RIGHT] = SDLK_RIGHT;
	MAC_keymap[MK_KP0] = SDLK_KP0;
	MAC_keymap[MK_KP_PERIOD] = SDLK_KP_PERIOD;
}

static SDL_keysym *TranslateKey(EventRecord *ev,SDL_keysym *keysym,int pressed)
{
	/* Set the keysym information */
	keysym->scancode = (ev->message&keyCodeMask)>>8;
	keysym->sym = MAC_keymap[keysym->scancode];
	keysym->mod = KMOD_NONE;
	keysym->unicode = 0;
	if ( pressed && SDL_TranslateUNICODE ) {
		static unsigned long state = 0;
		static Ptr keymap = nil;
		Ptr new_keymap;

		/* Get the current keyboard map resource */
		new_keymap = (Ptr)GetScriptManagerVariable(smKCHRCache);
		if ( new_keymap != keymap ) {
			keymap = new_keymap;
			state = 0;
		}
		keysym->unicode = KeyTranslate(keymap,
			keysym->scancode|ev->modifiers, &state) & 0xFFFF;
	}
	return(keysym);
}

void Mac_InitEvents(_THIS)
{
	/* Create apple menu bar */
	apple_menu = GetMenu(mApple);
	if ( apple_menu != nil ) {
		AppendResMenu(apple_menu, 'DRVR');
		InsertMenu(apple_menu, 0);
	}
	DrawMenuBar();

	/* Get rid of spurious events at startup */
	FlushEvents(everyEvent, 0);
	
	/* Allow every event but keyrepeat */
	SetEventMask(everyEvent - autoKeyMask);
}

void Mac_QuitEvents(_THIS)
{
	ClearMenuBar();
	if ( apple_menu != nil ) {
		ReleaseResource((char **)apple_menu);
	}

	/* Clean up pending events */
	FlushEvents(everyEvent, 0);
}

static void Mac_DoAppleMenu(_THIS, long choice)
{
#if !TARGET_API_MAC_CARBON  /* No Apple menu in OS X */
	short menu, item;

	item = (choice&0xFFFF);
	choice >>= 16;
	menu = (choice&0xFFFF);
	
	switch (menu) {
		case mApple: {
			switch (item) {
				case iAbout: {
					/* Run the about box */;
				}
				break;
				default: {
					Str255 name;
					
					GetMenuItemText(apple_menu, item, name);
					OpenDeskAcc(name);
				}
				break;
			}
		}
		break;
		default: {
			/* Ignore other menus */;
		}
	}
#endif /* !TARGET_API_MAC_CARBON */
}

#if !TARGET_API_MAC_CARBON
/* Since we don't initialize QuickDraw, we need to get a pointer to qd */
QDGlobals *theQD = NULL;

/* Exported to the macmain code */
void SDL_InitQuickDraw(struct QDGlobals *the_qd)
{
	theQD = the_qd;
}
#endif