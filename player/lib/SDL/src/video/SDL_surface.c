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
 "@(#) $Id: SDL_surface.c,v 1.1 2001/02/05 20:26:29 cahighlander Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_sysvideo.h"
#include "SDL_cursor_c.h"
#include "SDL_blit.h"
#include "SDL_RLEaccel_c.h"
#include "SDL_pixels_c.h"
#include "SDL_memops.h"
#include "SDL_leaks.h"

/* Public routines */
/*
 * Create an empty RGB surface of the appropriate depth
 */
SDL_Surface * SDL_CreateRGBSurface (Uint32 flags,
			int width, int height, int depth,
			Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
	SDL_VideoDevice *video = current_video;
	SDL_VideoDevice *this  = current_video;
	SDL_Surface *screen;
	SDL_Surface *surface;

	/* Check to see if we desire the surface in video memory */
	if ( video ) {
		screen = SDL_PublicSurface;
	} else {
		screen = NULL;
	}
	if ( screen && ((screen->flags&SDL_HWSURFACE) == SDL_HWSURFACE) ) {
		if ( (flags&(SDL_SRCCOLORKEY|SDL_SRCALPHA)) != 0 ) {
			flags |= SDL_HWSURFACE;
		}
		if ( (flags & SDL_SRCCOLORKEY) == SDL_SRCCOLORKEY ) {
			if ( ! current_video->info.blit_hw_CC ) {
				flags &= ~SDL_HWSURFACE;
			}
		}
		if ( (flags & SDL_SRCALPHA) == SDL_SRCALPHA ) {
			if ( ! current_video->info.blit_hw_A ) {
				flags &= ~SDL_HWSURFACE;
			}
		}
	} else {
		flags &= ~SDL_HWSURFACE;
	}

	/* Allocate the surface */
	surface = (SDL_Surface *)malloc(sizeof(*surface));
	if ( surface == NULL ) {
		SDL_OutOfMemory();
		return(NULL);
	}
	surface->flags = SDL_SWSURFACE;
	if ( (flags & SDL_HWSURFACE) == SDL_HWSURFACE ) {
		depth = screen->format->BitsPerPixel;
		Rmask = screen->format->Rmask;
		Gmask = screen->format->Gmask;
		Bmask = screen->format->Bmask;
		Amask = screen->format->Amask;
	}
	surface->format = SDL_AllocFormat(depth, Rmask, Gmask, Bmask, Amask);
	if ( surface->format == NULL ) {
		free(surface);
		return(NULL);
	}
	if ( Amask ) {
		surface->flags |= SDL_SRCALPHA;
	}
	surface->w = width;
	surface->h = height;
	surface->pitch = SDL_CalculatePitch(surface);
	surface->pixels = NULL;
	surface->offset = 0;
	surface->hwdata = NULL;
	surface->map = NULL;
	surface->format_version = 0;
	SDL_SetClipRect(surface, NULL);

	/* Get the pixels */
	if ( ((flags&SDL_HWSURFACE) == SDL_SWSURFACE) || 
				(video->AllocHWSurface(this, surface) < 0) ) {
		if ( surface->w && surface->h ) {
			surface->pixels = malloc(surface->h*surface->pitch);
			if ( surface->pixels == NULL ) {
				SDL_FreeSurface(surface);
				SDL_OutOfMemory();
				return(NULL);
			}
			/* This is important for bitmaps */
			memset(surface->pixels, 0, surface->h*surface->pitch);
		}
	}

	/* Allocate an empty mapping */
	surface->map = SDL_AllocBlitMap();
	if ( surface->map == NULL ) {
		SDL_FreeSurface(surface);
		return(NULL);
	}

	/* The surface is ready to go */
	surface->refcount = 1;
#ifdef CHECK_LEAKS
	++surfaces_allocated;
#endif
	return(surface);
}
/*
 * Create an RGB surface from an existing memory buffer
 */
SDL_Surface * SDL_CreateRGBSurfaceFrom (void *pixels,
			int width, int height, int depth, int pitch,
			Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
	SDL_Surface *surface;

	surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 0, 0, depth,
	                               Rmask, Gmask, Bmask, Amask);
	if ( surface != NULL ) {
		surface->flags |= SDL_PREALLOC;
		surface->pixels = pixels;
		surface->w = width;
		surface->h = height;
		surface->pitch = pitch;
	}
	return(surface);
}
/*
 * Set the color key in a blittable surface
 */
int SDL_SetColorKey (SDL_Surface *surface, Uint32 flag, Uint32 key)
{
	if(surface->flags & SDL_RLEACCEL
	   && (!(flag & SDL_RLEACCEL) || key != surface->format->colorkey)) {
	        /* un-encode existing RLE */
	        SDL_UnRLESurface(surface, 1);
	}

	if ( flag ) {
		SDL_VideoDevice *video = current_video;
		SDL_VideoDevice *this  = current_video;


		surface->flags |= SDL_SRCCOLORKEY;
		surface->format->colorkey = key;
		if ( (surface->flags & SDL_HWACCEL) == SDL_HWACCEL ) {
			if ( (video->SetHWColorKey == NULL) ||
			     (video->SetHWColorKey(this, surface, key) < 0) ) {
				surface->flags &= ~SDL_HWACCEL;
			}
		}
		if ( flag & (SDL_RLEACCEL|SDL_RLEACCELOK) ) {
			surface->flags |= SDL_RLEACCELOK;
		} else {
			surface->flags &= ~SDL_RLEACCELOK;
		}
	} else {
		surface->flags &= ~(SDL_SRCCOLORKEY|SDL_RLEACCELOK);
		surface->format->colorkey = 0;
	}
	SDL_InvalidateMap(surface->map);
	return(0);
}
int SDL_SetAlpha (SDL_Surface *surface, Uint32 flag, Uint8 value)
{
	Uint32 oldflags = surface->flags;
	if ( flag ) {
		SDL_VideoDevice *video = current_video;
		SDL_VideoDevice *this  = current_video;

		surface->flags |= SDL_SRCALPHA;
		surface->format->alpha = value;
		if ( (surface->flags & SDL_HWACCEL) == SDL_HWACCEL ) {
			if ( (video->SetHWAlpha == NULL) ||
			     (video->SetHWAlpha(this, surface, value) < 0) ) {
				surface->flags &= ~SDL_HWACCEL;
			}
		}
		if( flag & (SDL_RLEACCEL | SDL_RLEACCELOK))
		        surface->flags |= SDL_RLEACCELOK;
		else
		        surface->flags &= ~SDL_RLEACCELOK;
	} else {
		surface->flags &= ~SDL_SRCALPHA;
		surface->format->alpha = SDL_ALPHA_OPAQUE;
	}
	/*
	 * The representation for software surfaces is independent of
	 * per-surface alpha, so no need to invalidate the blit mapping
	 * if just the alpha value was changed
	 */
	if((surface->flags & SDL_HWACCEL) == SDL_HWACCEL
	   || oldflags != surface->flags)
	    SDL_InvalidateMap(surface->map);
	return(0);
}
/*
 * A function to calculate the intersection of two rectangles:
 * return true if the rectangles intersect, false otherwise
 */
static __inline__
SDL_bool SDL_IntersectRect(SDL_Rect *A, SDL_Rect *B, SDL_Rect *intersection)
{
	int Amin, Amax, Bmin, Bmax;

	/* Horizontal intersection */
	Amin = A->x;
	Amax = Amin + A->w;
	Bmin = B->x;
	Bmax = Bmin + B->w;
	if(Bmin > Amin)
	        Amin = Bmin;
	intersection->x = Amin;
	if(Bmax < Amax)
	        Amax = Bmax;
	intersection->w = Amax - Amin > 0 ? Amax - Amin : 0;

	/* Vertical intersection */
	Amin = A->y;
	Amax = Amin + A->h;
	Bmin = B->y;
	Bmax = Bmin + B->h;
	if(Bmin > Amin)
	        Amin = Bmin;
	intersection->y = Amin;
	if(Bmax < Amax)
	        Amax = Bmax;
	intersection->h = Amax - Amin > 0 ? Amax - Amin : 0;

	return (intersection->w && intersection->h);
}
/*
 * Set the clipping rectangle for a blittable surface
 */
SDL_bool SDL_SetClipRect(SDL_Surface *surface, SDL_Rect *rect)
{
	SDL_Rect full_rect;

	/* Don't do anything if there's no surface to act on */
	if ( ! surface ) {
		return SDL_FALSE;
	}

	/* Set up the full surface rectangle */
	full_rect.x = 0;
	full_rect.y = 0;
	full_rect.w = surface->w;
	full_rect.h = surface->h;

	/* Set the clipping rectangle */
	if ( ! rect ) {
		surface->clip_rect = full_rect;
		return 1;
	}
	return SDL_IntersectRect(rect, &full_rect, &surface->clip_rect);
}
/* 
 * Set up a blit between two surfaces -- split into three parts:
 * The upper part, SDL_UpperBlit(), performs clipping and rectangle 
 * verification.  The lower part is a pointer to a low level
 * accelerated blitting function.
 *
 * These parts are separated out and each used internally by this 
 * library in the optimimum places.  They are exported so that if
 * you know exactly what you are doing, you can optimize your code
 * by calling the one(s) you need.
 */
int SDL_LowerBlit (SDL_Surface *src, SDL_Rect *srcrect,
				SDL_Surface *dst, SDL_Rect *dstrect)
{
	SDL_blit do_blit;

	/* Check to make sure the blit mapping is valid */
	if ( (src->map->dst != dst) ||
             (src->map->dst->format_version != src->map->format_version) ) {
		if ( SDL_MapSurface(src, dst) < 0 ) {
			return(-1);
		}
	}

	/* Figure out which blitter to use */
	if ( (src->flags & SDL_HWACCEL) == SDL_HWACCEL ) {
		do_blit = src->map->hw_blit;
	} else {
		do_blit = src->map->sw_blit;
	}
	return(do_blit(src, srcrect, dst, dstrect));
}


int SDL_UpperBlit (SDL_Surface *src, SDL_Rect *srcrect,
		   SDL_Surface *dst, SDL_Rect *dstrect)
{
        SDL_Rect fulldst;
	int srcx, srcy, w, h;

	if(dstrect == NULL) {
	        fulldst.x = fulldst.y = 0;
		dstrect = &fulldst;
	}

	/* clip the source rectangle to the source surface */
	if(srcrect) {
	        int maxw, maxh;
	
		srcx = srcrect->x;
		w = srcrect->w;
		if(srcx < 0) {
		        w += srcx;
			dstrect->x -= srcx;
			srcx = 0;
		}
		maxw = src->w - srcx;
		if(maxw < w)
			w = maxw;

		srcy = srcrect->y;
		h = srcrect->h;
		if(srcy < 0) {
		        h += srcy;
			dstrect->y -= srcy;
			srcy = 0;
		}
		maxh = src->h - srcy;
		if(maxh < h)
			h = maxh;
	    
	} else {
	        srcx = srcy = 0;
		w = src->w;
		h = src->h;
	}

	/* clip the destination rectangle against the clip rectangle */
	{
	        SDL_Rect *clip = &dst->clip_rect;
		int dx, dy;

		dx = clip->x - dstrect->x;
		if(dx > 0) {
			w -= dx;
			dstrect->x += dx;
			srcx += dx;
		}
		dx = dstrect->x + w - clip->x - clip->w;
		if(dx > 0)
			w -= dx;

		dy = clip->y - dstrect->y;
		if(dy > 0) {
			h -= dy;
			dstrect->y += dy;
			srcy += dy;
		}
		dy = dstrect->y + h - clip->y - clip->h;
		if(dy > 0)
			h -= dy;
	}

	if(w > 0 && h > 0) {
	        SDL_Rect sr;
	        sr.x = srcx;
		sr.y = srcy;
		sr.w = dstrect->w = w;
		sr.h = dstrect->h = h;
		return SDL_LowerBlit(src, &sr, dst, dstrect);
	}
	dstrect->w = dstrect->h = 0;
	return 0;
}

/* 
 * This function performs a fast fill of the given rectangle with 'color'
 */
int SDL_FillRect(SDL_Surface *dst, SDL_Rect *dstrect, Uint32 color)
{
	SDL_VideoDevice *video = current_video;
	SDL_VideoDevice *this  = current_video;
	int x, y;
	Uint8 *row;

	/* If 'dstrect' == NULL, then fill the whole surface */
	if ( dstrect ) {
		/* Perform clipping */
		if ( !SDL_IntersectRect(dstrect, &dst->clip_rect, dstrect) ) {
			return(0);
		}
	} else {
		dstrect = &dst->clip_rect;
	}

	/* Check for hardware acceleration */
	if ( ((dst->flags & SDL_HWSURFACE) == SDL_HWSURFACE) &&
					video->info.blit_fill ) {
		return(video->FillHWRect(this, dst, dstrect, color));
	}

	/* Perform software fill */
	if ( SDL_LockSurface(dst) != 0 ) {
		return(-1);
	}
	row = (Uint8 *)dst->pixels+dstrect->y*dst->pitch+
			dstrect->x*dst->format->BytesPerPixel;
	if ( dst->format->palette || (color == 0) ) {
		x = dstrect->w*dst->format->BytesPerPixel;
#ifdef SDL_memset4
		if ( !color && !((long)row&3) && !(x&3) && !(dst->pitch&3) ) {
			for ( y=dstrect->h; y; --y ) {
				SDL_memset4(row, 0, x);
				row += dst->pitch;
			}
		} else
#endif
		for ( y=dstrect->h; y; --y ) {
#ifdef __powerpc__	/* SIGBUS when using memset() ?? */
			if ( color ) {
				Uint8 *rowp = row;
				int left = x;
				while ( left-- ) { *rowp++ = color; }
			} else {
				Uint8 *rowp = row;
				int left = x;
				while ( left-- ) { *rowp++ = 0; }
			}
#else
			memset(row, color, x);
#endif
			row += dst->pitch;
		}
	} else {
		switch (dst->format->BytesPerPixel) {
		    case 2: {
			Uint16 *pixels;
			for ( y=dstrect->h; y; --y ) {
				pixels = (Uint16 *)row;
				for ( x=dstrect->w/4; x; --x ) {
					*pixels++ = color;
					*pixels++ = color;
					*pixels++ = color;
					*pixels++ = color;
				}
				switch (dstrect->w%4) {
					case 3:
						*pixels++ = color;
					case 2:
						*pixels++ = color;
					case 1:
						*pixels++ = color;
				}
				row += dst->pitch;
			}
		    }
		    break;

		    case 3: {
			Uint8 *pixels;
			if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
			    color <<= 8;
			for ( y=dstrect->h; y; --y ) {
				pixels = row;
				for ( x=dstrect->w; x; --x ) {
					memcpy(pixels, &color, 3);
					pixels += 3;
				}
				row += dst->pitch;
			}
		    }
		    break;

		    case 4: {
			Uint32 *pixels;
			for ( y=dstrect->h; y; --y ) {
				pixels = (Uint32 *)row;
				for ( x=dstrect->w/4; x; --x ) {
					*pixels++ = color;
					*pixels++ = color;
					*pixels++ = color;
					*pixels++ = color;
				}
				switch (dstrect->w%4) {
					case 3:
						*pixels++ = color;
					case 2:
						*pixels++ = color;
					case 1:
						*pixels++ = color;
				}
				row += dst->pitch;
			}
		    }
		    break;
		}
	}
	SDL_UnlockSurface(dst);

	/* We're done! */
	return(0);
}

/*
 * Lock a surface to directly access the pixels
 * -- Do not call this from any blit function, as SDL_DrawCursor() may recurse
 *    Instead, use:
 *    if ( (surface->flags & SDL_HWSURFACE) == SDL_HWSURFACE )
 *               video->LockHWSurface(video, surface);
 */
int SDL_LockSurface (SDL_Surface *surface)
{
	/* Perform the lock */
	if ( surface->flags & (SDL_HWSURFACE|SDL_ASYNCBLIT) ) {
		SDL_VideoDevice *video = current_video;
		SDL_VideoDevice *this  = current_video;
		if ( video->LockHWSurface(this, surface) < 0 ) {
			return(-1);
		}
	}
	if ( surface->flags & SDL_RLEACCEL ) {
		SDL_UnRLESurface(surface, 1);
		surface->flags |= SDL_RLEACCEL;	/* remember accel'd state */
	}
	/* This needs to be done here in case pixels changes value */
	surface->pixels = (Uint8 *)surface->pixels + surface->offset;
	return(0);
}
/*
 * Unlock a previously locked surface
 * -- Do not call this from any blit function, as SDL_DrawCursor() may recurse
 *    Instead, use:
 *    if ( (surface->flags & SDL_HWSURFACE) == SDL_HWSURFACE )
 *               video->UnlockHWSurface(video, surface);
 */
void SDL_UnlockSurface (SDL_Surface *surface)
{
	/* Perform the unlock */
	surface->pixels = (Uint8 *)surface->pixels - surface->offset;

	/* Unlock hardware or accelerated surfaces */
	if ( surface->flags & (SDL_HWSURFACE|SDL_ASYNCBLIT) ) {
		SDL_VideoDevice *video = current_video;
		SDL_VideoDevice *this  = current_video;
		video->UnlockHWSurface(this, surface);
	} else {
		/* Update RLE encoded surface with new data */
		if ( (surface->flags & SDL_RLEACCEL) == SDL_RLEACCEL ) {
		        surface->flags &= ~SDL_RLEACCEL; /* stop lying */
			SDL_RLESurface(surface);
		}
	}
}

/* 
 * Convert a surface into the specified pixel format.
 */
SDL_Surface * SDL_ConvertSurface (SDL_Surface *surface,
					SDL_PixelFormat *format, Uint32 flags)
{
	SDL_Surface *convert;
	Uint32 colorkey = 0;
	Uint8 alpha = 0;
	Uint32 surface_flags;
	SDL_Rect bounds;

	/* Check for empty destination palette! (results in empty image) */
	if ( format->palette != NULL ) {
		int i;
		for ( i=0; i<format->palette->ncolors; ++i ) {
			if ( (format->palette->colors[i].r != 0) ||
			     (format->palette->colors[i].g != 0) ||
			     (format->palette->colors[i].b != 0) )
				break;
		}
		if ( i == format->palette->ncolors ) {
			SDL_SetError("Empty destination palette");
			return(NULL);
		}
	}

	/* Create a new surface with the desired format */
	convert = SDL_CreateRGBSurface(flags,
				surface->w, surface->h, format->BitsPerPixel,
		format->Rmask, format->Gmask, format->Bmask, format->Amask);
	if ( convert == NULL ) {
		return(NULL);
	}

	/* Copy the palette if any */
	if ( format->palette && convert->format->palette ) {
		memcpy(convert->format->palette->colors,
				format->palette->colors,
				format->palette->ncolors*sizeof(SDL_Color));
		convert->format->palette->ncolors = format->palette->ncolors;
	}

	/* Save the original surface color key and alpha */
	surface_flags = surface->flags;
	if ( (surface_flags & SDL_SRCCOLORKEY) == SDL_SRCCOLORKEY ) {
		colorkey = surface->format->colorkey;
		SDL_SetColorKey(surface, 0, 0);
	}
	if ( (surface_flags & SDL_SRCALPHA) == SDL_SRCALPHA ) {
		alpha = surface->format->alpha;
		SDL_SetAlpha(surface, 0, 0);
	}

	/* Copy over the image data */
	bounds.x = 0;
	bounds.y = 0;
	bounds.w = surface->w;
	bounds.h = surface->h;
	SDL_LowerBlit(surface, &bounds, convert, &bounds);

	/* Clean up the original surface, and update converted surface */
	if ( convert != NULL ) {
		SDL_SetClipRect(convert, &surface->clip_rect);
	}
	if ( (surface_flags & SDL_SRCCOLORKEY) == SDL_SRCCOLORKEY ) {
		if ( convert != NULL ) {
			Uint8 keyR, keyG, keyB;

			SDL_GetRGB(colorkey,surface->format,&keyR,&keyG,&keyB);
			SDL_SetColorKey(convert, 
				surface_flags&(SDL_SRCCOLORKEY|SDL_RLEACCELOK), 
				SDL_MapRGB(convert->format, keyR, keyG, keyB));
		}
		SDL_SetColorKey(surface, 
				surface_flags&(SDL_SRCCOLORKEY|SDL_RLEACCELOK), 
								colorkey);
	}
	if ( (surface_flags & SDL_SRCALPHA) == SDL_SRCALPHA ) {
		if ( convert != NULL ) {
		        SDL_SetAlpha(convert, surface_flags & SDL_SRCALPHA,
				     alpha);
		}
		SDL_SetAlpha(surface, surface_flags&SDL_SRCALPHA, alpha);
	}

	/* We're ready to go! */
	return(convert);
}

/*
 * Free a surface created by the above function.
 */
void SDL_FreeSurface (SDL_Surface *surface)
{
	/* Free anything that's not NULL, and not the screen surface */
	if ((surface == NULL) ||
	    (current_video &&
	    ((surface == SDL_ShadowSurface)||(surface == SDL_VideoSurface)))) {
		return;
	}
	if ( --surface->refcount > 0 ) {
		return;
	}
	if ( (surface->flags & SDL_RLEACCEL) == SDL_RLEACCEL ) {
	        SDL_UnRLESurface(surface, 0);
	}
	if ( surface->format ) {
		SDL_FreeFormat(surface->format);
		surface->format = NULL;
	}
	if ( surface->map != NULL ) {
		SDL_FreeBlitMap(surface->map);
		surface->map = NULL;
	}
	if ( (surface->flags & SDL_HWSURFACE) == SDL_HWSURFACE ) {
		SDL_VideoDevice *video = current_video;
		SDL_VideoDevice *this  = current_video;
		video->FreeHWSurface(this, surface);
	}
	if ( surface->pixels &&
	     ((surface->flags & SDL_PREALLOC) != SDL_PREALLOC) ) {
		free(surface->pixels);
	}
	free(surface);
#ifdef CHECK_LEAKS
	--surfaces_allocated;
#endif
}
