/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999  Sam Lantinga

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
 "@(#) $Id: SDL_sysevents.c,v 1.1 2001/02/05 20:26:30 cahighlander Exp $";
#endif

#include <stdlib.h>
#include <stdio.h>

#include "SDL_events.h"
#include "SDL_video.h"
#include "SDL_error.h"
#include "SDL_syswm.h"
#include "SDL_sysevents.h"
#include "SDL_events_c.h"
#include "SDL_sysvideo.h"
#include "SDL_lowvideo.h"
#include "SDL_syswm_c.h"
#include "SDL_main.h"

#ifdef WMMSG_DEBUG
#include "wmmsg.h"
#endif

/* The window we use for everything... */
const char *SDL_Appname = NULL;
HINSTANCE SDL_Instance = NULL;
HWND SDL_Window = NULL;
RECT SDL_bounds = {0, 0, 0, 0};
int SDL_resizing = 0;
int mouse_relative = 0;
int posted = 0;


/* Functions called by the message processing function */
LONG
(*HandleMessage)(_THIS, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void (*WIN_RealizePalette)(_THIS);
void (*WIN_PaletteChanged)(_THIS, HWND window);
void (*WIN_SwapGamma)(_THIS);
void (*WIN_WinPAINT)(_THIS, HDC hdc);

#ifdef WM_MOUSELEAVE
/* 
   Special code to handle mouse leave events - this sucks...
   http://support.microsoft.com/support/kb/articles/q183/1/07.asp

   TrackMouseEvent() is only available on Win98 and WinNT.
   _TrackMouseEvent() is available on Win95, but isn't yet in the mingw32
   development environment, and only works on systems that have had IE 3.0
   or newer installed on them (which is not the case with the base Win95).
   Therefore, we implement our own version of _TrackMouseEvent() which
   uses our own implementation if TrackMouseEvent() is not available.
*/
static BOOL (WINAPI *_TrackMouseEvent)(TRACKMOUSEEVENT *ptme) = NULL;

static VOID CALLBACK
TrackMouseTimerProc(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	RECT rect;
	POINT pt;

	GetClientRect(hWnd, &rect);
	MapWindowPoints(hWnd, NULL, (LPPOINT)&rect, 2);
	GetCursorPos(&pt);
	if ( !PtInRect(&rect, pt) || (WindowFromPoint(pt) != hWnd) ) {
		if ( !KillTimer(hWnd, idEvent) ) {
			/* Error killing the timer! */
		}
		PostMessage(hWnd, WM_MOUSELEAVE, 0, 0);
	}
}
static BOOL WINAPI WIN_TrackMouseEvent(TRACKMOUSEEVENT *ptme)
{
	if ( ptme->dwFlags == TME_LEAVE ) {
		return SetTimer(ptme->hwndTrack, ptme->dwFlags, 100,
		                (TIMERPROC)TrackMouseTimerProc);
	}
	return FALSE;
}
#endif /* WM_MOUSELEAVE */

/* The main Win32 event handler */
static LONG CALLBACK WinMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	SDL_VideoDevice *this = current_video;
	static int mouse_pressed = 0;
	static int in_window = 0;
#ifdef WMMSG_DEBUG
	fprintf(stderr, "Received windows message:  ");
	if ( msg > MAX_WMMSG ) {
		fprintf(stderr, "%d", msg);
	} else {
		fprintf(stderr, "%s", wmtab[msg]);
	}
	fprintf(stderr, " -- 0x%X, 0x%X\n", wParam, lParam);
#endif
	switch (msg) {

		case WM_ACTIVATE: {
			SDL_VideoDevice *this = current_video;
			int active, minimized;

			minimized = HIWORD(wParam);
			active = LOWORD(wParam);
			if ( minimized && !active ) {
				if ( SDL_GetAppState() & SDL_APPACTIVE ) {
					WIN_SwapGamma(this);
				}
				posted = SDL_PrivateAppActive(0, SDL_APPACTIVE);
			}
			if ( !minimized && active ) {
				if ( this->input_grab != SDL_GRAB_OFF ) {
					WIN_GrabInput(this, SDL_GRAB_ON);
				}
				if ( !(SDL_GetAppState() & SDL_APPACTIVE) ) {
					WIN_SwapGamma(this);
				}
				posted = SDL_PrivateAppActive(1, SDL_APPACTIVE);
			}
			if ( !active ) {
				if ( this->input_grab != SDL_GRAB_OFF ) {
					WIN_GrabInput(this, SDL_GRAB_OFF);
				}
			}
		}
		break;

		case WM_MOUSEMOVE: {
			
			/* Mouse is handled by DirectInput when fullscreen */
			if ( SDL_VideoSurface && ! DIRECTX_FULLSCREEN() ) {
				Sint16 x, y;

				/* mouse has entered the window */
				if ( ! in_window ) {
#ifdef WM_MOUSELEAVE
					TRACKMOUSEEVENT tme;

					tme.cbSize = sizeof(tme);
					tme.dwFlags = TME_LEAVE;
					tme.hwndTrack = SDL_Window;
					_TrackMouseEvent(&tme);
#endif /* WM_MOUSELEAVE */
					in_window = TRUE;

					posted = SDL_PrivateAppActive(1, SDL_APPMOUSEFOCUS);
				}

				/* mouse has moved within the window */
				x = LOWORD(lParam);
				y = HIWORD(lParam);
				if ( mouse_relative ) {
					POINT center;
					center.x = (SDL_VideoSurface->w/2);
					center.y = (SDL_VideoSurface->h/2);
					x -= (Sint16)center.x;
					y -= (Sint16)center.y;
					if ( x || y ) {
						ClientToScreen(SDL_Window, &center);
						SetCursorPos(center.x, center.y);
						posted = SDL_PrivateMouseMotion(0, 1, x, y);
					}
				} else {
					posted = SDL_PrivateMouseMotion(0, 0, x, y);
				}
			}
		}
		return(0);

#ifdef WM_MOUSELEAVE
		case WM_MOUSELEAVE: {

			/* Mouse is handled by DirectInput when fullscreen */
			if ( SDL_VideoSurface && ! DIRECTX_FULLSCREEN() ) {
				/* mouse has left the window */
				/* or */
				/* Elvis has left the building! */
				in_window = FALSE;
				posted = SDL_PrivateAppActive(0, SDL_APPMOUSEFOCUS);
			}
		}
		return(0);
#endif /* WM_MOUSELEAVE */

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP: {
			/* Mouse is handled by DirectInput when fullscreen */
			if ( SDL_VideoSurface && ! DIRECTX_FULLSCREEN() ) {
				Sint16 x, y;
				Uint8 button, state;

				/* Figure out which button to use */
				switch (msg) {
					case WM_LBUTTONDOWN:
						button = 1;
						state = SDL_PRESSED;
						break;
					case WM_LBUTTONUP:
						button = 1;
						state = SDL_RELEASED;
						break;
					case WM_MBUTTONDOWN:
						button = 2;
						state = SDL_PRESSED;
						break;
					case WM_MBUTTONUP:
						button = 2;
						state = SDL_RELEASED;
						break;
					case WM_RBUTTONDOWN:
						button = 3;
						state = SDL_PRESSED;
						break;
					case WM_RBUTTONUP:
						button = 3;
						state = SDL_RELEASED;
						break;
				}
				if ( state == SDL_PRESSED ) {
					/* Grab mouse so we get up events */
					if ( ++mouse_pressed > 0 ) {
						SetCapture(hwnd);
					}
				} else {
					/* Release mouse after all up events */
					if ( --mouse_pressed <= 0 ) {
						ReleaseCapture();
						mouse_pressed = 0;
					}
				}
				if ( mouse_relative ) {
				/*	RJR: March 28, 2000
					report internal mouse position if in relative mode */
					x = 0; y = 0;
				} else {
					x = (Sint16)LOWORD(lParam);
					y = (Sint16)HIWORD(lParam);
				}
				posted = SDL_PrivateMouseButton(
							state, button, x, y);
			}
		}
		return(0);

		/* This message is sent as a way for us to "check" the values
		 * of a position change.  If we don't like it, we can adjust
		 * the values before they are changed.
		 */
		case WM_GETMINMAXINFO: {
			MINMAXINFO *info;
			RECT        size;
			int x, y;
			int width;
			int height;

			/* We don't want to clobber an internal resize */
			if ( SDL_resizing )
				return(0);

			/* We allow resizing with the SDL_RESIZABLE flag */
			if ( SDL_PublicSurface &&
				(SDL_PublicSurface->flags & SDL_RESIZABLE) ) {
				return(0);
			}

			/* Get the current position of our window */
			GetWindowRect(SDL_Window, &size);
			x = size.left;
			y = size.top;

			/* Calculate current width and height of our window */
			size.top = 0;
			size.left = 0;
			if ( SDL_PublicSurface != NULL ) {
				size.bottom = SDL_PublicSurface->h;
				size.right = SDL_PublicSurface->w;
			} else {
				size.bottom = 0;
				size.right = 0;
			}
			AdjustWindowRect(&size, GetWindowLong(hwnd, GWL_STYLE),
									FALSE);
			width = size.right - size.left;
			height = size.bottom - size.top;

			/* Fix our size to the current size */
			info = (MINMAXINFO *)lParam;
			info->ptMaxSize.x = width;
			info->ptMaxSize.y = height;
			info->ptMaxPosition.x = x;
			info->ptMaxPosition.y = y;
			info->ptMinTrackSize.x = width;
			info->ptMinTrackSize.y = height;
			info->ptMaxTrackSize.x = width;
			info->ptMaxTrackSize.y = height;
		}
		return(0);

		case WM_MOVE: {
			SDL_VideoDevice *this = current_video;

			GetClientRect(SDL_Window, &SDL_bounds);
			ClientToScreen(SDL_Window, (LPPOINT)&SDL_bounds);
			ClientToScreen(SDL_Window, (LPPOINT)&SDL_bounds+1);
			if ( this->input_grab != SDL_GRAB_OFF ) {
				ClipCursor(&SDL_bounds);
			}
		}
		break;
	
		case WM_SIZE: {
			if ( SDL_PublicSurface &&
				(SDL_PublicSurface->flags & SDL_RESIZABLE) ) {
				SDL_PrivateResize(LOWORD(lParam), HIWORD(lParam));
				return(0);
			}
		}
		break;

		/* We need to set the cursor */
		case WM_SETCURSOR: {
			Uint16 hittest;

			hittest = LOWORD(lParam);
			if ( hittest == HTCLIENT ) {
				SetCursor(SDL_hcursor);
				return(TRUE);
			}
		}
		break;

		/* We are about to get palette focus! */
		case WM_QUERYNEWPALETTE: {
			WIN_RealizePalette(current_video);
			return(TRUE);
		}
		break;

		/* Another application changed the palette */
		case WM_PALETTECHANGED: {
			WIN_PaletteChanged(current_video, (HWND)wParam);
		}
		break;

		/* We were occluded, refresh our display */
		case WM_PAINT: {
			HDC hdc;
			PAINTSTRUCT ps;

			hdc = BeginPaint(SDL_Window, &ps);
			if ( current_video->screen &&
			     !(current_video->screen->flags & SDL_OPENGL) ) {
				WIN_WinPAINT(current_video, hdc);
			}
			EndPaint(SDL_Window, &ps);
		}
		return(0);

		case WM_CLOSE: {
			if ( (posted = SDL_PrivateQuit()) )
				PostQuitMessage(0);
		}
		return(0);

		case WM_DESTROY: {
			PostQuitMessage(0);
		}
		return(0);

		default: {
			/* Special handling by the video driver */
			if (HandleMessage) {
				return(HandleMessage(current_video,
			                     hwnd, msg, wParam, lParam));
			}
		}
		break;
	}
	return(DefWindowProc(hwnd, msg, wParam, lParam));
}

/* This allows the SDL_WINDOWID hack */
const char *SDL_windowid = NULL;

/* Register the class for this application -- exported for winmain.c */
int SDL_RegisterApp(char *name, Uint32 style, void *hInst)
{
	static int initialized = 0;
	WNDCLASS class;
	HMODULE handle;

	/* Only do this once... */
	if ( initialized ) {
		return(0);
	}

	/* Prevent us from doing special message handling */
	HandleMessage = NULL;

	/* This function needs to be passed the correct process handle
	   by the application.  The following call just returns a handle
	   to the SDL DLL, which is useless for our purposes and causes
	   DirectInput to fail to initialize.
	 */
	if ( ! hInst ) {
		hInst = GetModuleHandle(NULL);
	}

	/* Register the application class */
	class.hCursor		= NULL;
	class.hIcon		= LoadImage(hInst, name, IMAGE_ICON,
                                                        0, 0, LR_DEFAULTCOLOR);
	class.lpszMenuName	= "(none)";
	class.lpszClassName	= name;
	class.hbrBackground	= GetStockObject(BLACK_BRUSH);
	class.hInstance		= hInst ? hInst : GetModuleHandle(0);
	class.style		= style | CS_OWNDC;
	class.lpfnWndProc	= WinMessage;
	class.cbWndExtra	= 0;
	class.cbClsExtra	= 0;
	if ( ! RegisterClass(&class) ) {
		SDL_SetError("Couldn't register application class");
		return(-1);
	}
	SDL_Appname = name;
	SDL_Instance = hInst;

#ifdef WM_MOUSELEAVE
	/* Get the version of TrackMouseEvent() we use */
	_TrackMouseEvent = NULL;
	handle = GetModuleHandle("USER32.DLL");
	if ( handle ) {
		_TrackMouseEvent = (BOOL (WINAPI *)(TRACKMOUSEEVENT *))GetProcAddress(handle, "TrackMouseEvent");
	}
	if ( _TrackMouseEvent == NULL ) {
		_TrackMouseEvent = WIN_TrackMouseEvent;
	}
#endif /* WM_MOUSELEAVE */

	/* Check for SDL_WINDOWID hack */
	SDL_windowid = getenv("SDL_WINDOWID");

	initialized = 1;
	return(0);
}
