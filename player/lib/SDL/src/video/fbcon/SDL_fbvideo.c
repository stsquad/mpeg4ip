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
 "@(#) $Id: SDL_fbvideo.c,v 1.1 2001/02/05 20:26:30 cahighlander Exp $";
#endif

/* Framebuffer console based SDL video driver implementation.
*/

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <asm/page.h>		/* For definition of PAGE_SIZE */

#include "SDL.h"
#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_mouse.h"
#include "SDL_sysvideo.h"
#include "SDL_pixels_c.h"
#include "SDL_events_c.h"
#include "SDL_fbvideo.h"
#include "SDL_fbmouse_c.h"
#include "SDL_fbevents_c.h"
#include "SDL_fb3dfx.h"
#include "SDL_fbmatrox.h"

/* Fix for broken 2.0.4-test1-ruby fb.h */
typedef struct fb_var_screeninfo fb_var_screeninfo;
typedef struct fb_fix_screeninfo fb_fix_screeninfo;
typedef struct fb_cmap fb_cmap;

/* A list of video resolutions that we query for (sorted largest to smallest) */
static SDL_Rect checkres[] = {
	{  0, 0, 1600, 1200 },		/* 16 bpp: 0x11E, or 286 */
	{  0, 0, 1408, 1056 },		/* 16 bpp: 0x19A, or 410 */
	{  0, 0, 1280, 1024 },		/* 16 bpp: 0x11A, or 282 */
	{  0, 0, 1152,  864 },		/* 16 bpp: 0x192, or 402 */
	{  0, 0, 1024,  768 },		/* 16 bpp: 0x117, or 279 */
	{  0, 0,  960,  720 },		/* 16 bpp: 0x18A, or 394 */
	{  0, 0,  800,  600 },		/* 16 bpp: 0x114, or 276 */
	{  0, 0,  768,  576 },		/* 16 bpp: 0x182, or 386 */
	{  0, 0,  640,  480 },		/* 16 bpp: 0x111, or 273 */
	{  0, 0,  640,  400 },		/*  8 bpp: 0x100, or 256 */
	{  0, 0,  512,  384 },
	{  0, 0,  320,  240 },
	{  0, 0,  320,  200 }
};
static struct {
	int xres;
	int yres;
	int pixclock;
	int left;
	int right;
	int upper;
	int lower;
	int hslen;
	int vslen;
	int sync;
	int vmode;
} vesa_timings[] = {
#ifdef USE_VESA_TIMINGS	/* Only tested on Matrox Millenium I */
	{  640,  400, 39771,  48, 16, 39,  8,  96, 2, 2, 0 },	/* 70 Hz */
	{  640,  480, 39683,  48, 16, 33, 10,  96, 2, 0, 0 },	/* 60 Hz */
	{  768,  576, 26101, 144, 16, 28,  6, 112, 4, 0, 0 },	/* 60 Hz */
	{  800,  600, 24038, 144, 24, 28,  8, 112, 6, 0, 0 },	/* 60 Hz */
	{  960,  720, 17686, 144, 24, 28,  8, 112, 4, 0, 0 },	/* 60 Hz */
	{ 1024,  768, 15386, 160, 32, 30,  4, 128, 4, 0, 0 },	/* 60 Hz */
	{ 1152,  864, 12286, 192, 32, 30,  4, 128, 4, 0, 0 },	/* 60 Hz */
	{ 1280, 1024,  9369, 224, 32, 32,  4, 136, 4, 0, 0 },	/* 60 Hz */
	{ 1408, 1056,  8214, 256, 40, 32,  5, 144, 5, 0, 0 },	/* 60 Hz */
	{ 1600, 1200,/*?*/0, 272, 48, 32,  5, 152, 5, 0, 0 },	/* 60 Hz */
#else
	/* You can generate these timings from your XF86Config file using
	   the 'modeline2fb' perl script included with the fbset package.
	   These timings were generated for Matrox Millenium I, 15" monitor.
	*/
	{  320,  200, 79440,  16, 16, 20,  4,  48, 1, 0, 2 },	/* 70 Hz */
	{  320,  240, 63492,  16, 16, 16,  4,  48, 2, 0, 2 },	/* 72 Hz */
	{  512,  384, 49603,  48, 16, 16,  1,  64, 3, 0, 0 },	/* 78 Hz */
	{  640,  400, 31746,  96, 32, 41,  1,  64, 3, 2, 0 },	/* 85 Hz */
	{  640,  480, 31746, 120, 16, 16,  1,  64, 3, 0, 0 },	/* 75 Hz */
	{  768,  576, 26101, 144, 16, 28,  6, 112, 4, 0, 0 },	/* 60 Hz */
	{  800,  600, 20000,  64, 56, 23, 37, 120, 6, 3, 0 },	/* 72 Hz */
	{  960,  720, 17686, 144, 24, 28,  8, 112, 4, 0, 0 },	/* 60 Hz */
	{ 1024,  768, 13333, 144, 24, 29,  3, 136, 6, 0, 0 },	/* 70 Hz */
	{ 1152,  864, 12286, 192, 32, 30,  4, 128, 4, 0, 0 },	/* 60 Hz */
	{ 1280, 1024,  9369, 224, 32, 32,  4, 136, 4, 0, 0 },	/* 60 Hz */
	{ 1408, 1056,  8214, 256, 40, 32,  5, 144, 5, 0, 0 },	/* 60 Hz */
	{ 1600, 1200,/*?*/0, 272, 48, 32,  5, 152, 5, 0, 0 },	/* 60 Hz */
#endif
};

/* Initialization/Query functions */
static int FB_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Rect **FB_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *FB_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
static int FB_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
static void FB_VideoQuit(_THIS);

/* Hardware surface functions */
static int FB_InitHWSurfaces(_THIS, caddr_t base, int size);
static void FB_FreeHWSurfaces(_THIS);
static int FB_AllocHWSurface(_THIS, SDL_Surface *surface);
static int FB_LockHWSurface(_THIS, SDL_Surface *surface);
static void FB_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void FB_FreeHWSurface(_THIS, SDL_Surface *surface);
static void FB_WaitVBL(_THIS);
static int FB_FlipHWSurface(_THIS, SDL_Surface *surface);

/* Internal palette functions */
static void FB_SavePalette(_THIS, struct fb_fix_screeninfo *finfo,
                                  struct fb_var_screeninfo *vinfo);
static void FB_RestorePalette(_THIS);

/* FB driver bootstrap functions */

static int FB_Available(void)
{
	int console;

	console = open("/dev/fb0", O_RDWR, 0);
	if ( console >= 0 ) {
		close(console);
	}
	return(console >= 0);
}

static void FB_DeleteDevice(SDL_VideoDevice *device)
{
	free(device->hidden);
	free(device);
}

static SDL_VideoDevice *FB_CreateDevice(int devindex)
{
	SDL_VideoDevice *this;

	/* Initialize all variables that we clean on shutdown */
	this = (SDL_VideoDevice *)malloc(sizeof(SDL_VideoDevice));
	if ( this ) {
		memset(this, 0, (sizeof *this));
		this->hidden = (struct SDL_PrivateVideoData *)
				malloc((sizeof *this->hidden));
	}
	if ( (this == NULL) || (this->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( this ) {
			free(this);
		}
		return(0);
	}
	memset(this->hidden, 0, (sizeof *this->hidden));
	wait_vbl = FB_WaitVBL;

	/* Set the function pointers */
	this->VideoInit = FB_VideoInit;
	this->ListModes = FB_ListModes;
	this->SetVideoMode = FB_SetVideoMode;
	this->SetColors = FB_SetColors;
	this->UpdateRects = NULL;
	this->VideoQuit = FB_VideoQuit;
	this->AllocHWSurface = FB_AllocHWSurface;
	this->CheckHWBlit = NULL;
	this->FillHWRect = NULL;
	this->SetHWColorKey = NULL;
	this->SetHWAlpha = NULL;
	this->LockHWSurface = FB_LockHWSurface;
	this->UnlockHWSurface = FB_UnlockHWSurface;
	this->FlipHWSurface = FB_FlipHWSurface;
	this->FreeHWSurface = FB_FreeHWSurface;
	this->SetIcon = NULL;
	this->SetCaption = NULL;
	this->GetWMInfo = NULL;
	this->FreeWMCursor = FB_FreeWMCursor;
	this->CreateWMCursor = FB_CreateWMCursor;
	this->ShowWMCursor = FB_ShowWMCursor;
	this->WarpWMCursor = FB_WarpWMCursor;
	this->InitOSKeymap = FB_InitOSKeymap;
	this->PumpEvents = FB_PumpEvents;

	this->free = FB_DeleteDevice;

	return this;
}

VideoBootStrap FBCON_bootstrap = {
	"fbcon", FB_Available, FB_CreateDevice
};

static int FB_CheckMode(_THIS, struct fb_var_screeninfo *vinfo,
                        int index, unsigned int *w, unsigned int *h)
{
	int mode_okay;

	mode_okay = 0;
	vinfo->bits_per_pixel = (index+1)*8;
	vinfo->xres = *w;
	vinfo->xres_virtual = *w;
	vinfo->yres = *h;
	vinfo->yres_virtual = *h;
	vinfo->activate = FB_ACTIVATE_TEST;
	if ( ioctl(console_fd, FBIOPUT_VSCREENINFO, vinfo) == 0 ) {
#ifdef FBCON_DEBUG
		fprintf(stderr, "Checked mode %dx%d at %d bpp, got mode %dx%d at %d bpp\n", *w, *h, (index+1)*8, vinfo->xres, vinfo->yres, vinfo->bits_per_pixel);
#endif
		if ( (((vinfo->bits_per_pixel+7)/8)-1) == index ) {
			*w = vinfo->xres;
			*h = vinfo->yres;
			mode_okay = 1;
		}
	}
	return mode_okay;
}

static int FB_AddMode(_THIS, int index, unsigned int w, unsigned int h)
{
	SDL_Rect *mode;
	int i;
	int next_mode;

	/* Check to see if we already have this mode */
	if ( SDL_nummodes[index] > 0 ) {
		mode = SDL_modelist[index][SDL_nummodes[index]-1];
		if ( (mode->w == w) && (mode->h == h) ) {
#ifdef FBCON_DEBUG
			fprintf(stderr, "We already have mode %dx%d at %d bytes per pixel\n", w, h, index+1);
#endif
			return(0);
		}
	}

	/* Only allow a mode if we have a valid timing for it */
	next_mode = 0;
	for ( i=0; i<(sizeof(vesa_timings)/sizeof(vesa_timings[0])); ++i ) {
		if ( (w == vesa_timings[i].xres) &&
		     (h == vesa_timings[i].yres) && vesa_timings[i].pixclock ) {
			next_mode = i;
			break;
		}
	}
	if ( ! next_mode ) {
#ifdef FBCON_DEBUG
		fprintf(stderr, "No valid timing line for mode %dx%d\n", w, h);
#endif
		return(0);
	}

	/* Set up the new video mode rectangle */
	mode = (SDL_Rect *)malloc(sizeof *mode);
	if ( mode == NULL ) {
		SDL_OutOfMemory();
		return(-1);
	}
	mode->x = 0;
	mode->y = 0;
	mode->w = w;
	mode->h = h;
#ifdef FBCON_DEBUG
	fprintf(stderr, "Adding mode %dx%d at %d bytes per pixel\n", w, h, index+1);
#endif

	/* Allocate the new list of modes, and fill in the new mode */
	next_mode = SDL_nummodes[index];
	SDL_modelist[index] = (SDL_Rect **)
	       realloc(SDL_modelist[index], (1+next_mode+1)*sizeof(SDL_Rect *));
	if ( SDL_modelist[index] == NULL ) {
		SDL_OutOfMemory();
		SDL_nummodes[index] = 0;
		free(mode);
		return(-1);
	}
	SDL_modelist[index][next_mode] = mode;
	SDL_modelist[index][next_mode+1] = NULL;
	SDL_nummodes[index]++;

	return(0);
}

static int FB_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;
	int i, j;
	int current_index;
	unsigned int current_w;
	unsigned int current_h;
	const char *SDL_fbdev;

	/* Initialize the library */
	SDL_fbdev = getenv("SDL_FBDEV");
	if ( SDL_fbdev == NULL ) {
		SDL_fbdev = "/dev/fb0";
	}
	console_fd = open(SDL_fbdev, O_RDWR, 0);
	if ( console_fd < 0 ) {
		SDL_SetError("Unable to open %s", SDL_fbdev);
		return(-1);
	}

	/* Create the hardware surface lock mutex */
	hw_lock = SDL_CreateMutex();
	if ( hw_lock == NULL ) {
		SDL_SetError("Unable to create lock mutex");
		FB_VideoQuit(this);
		return(-1);
	}

	/* Get the type of video hardware */
	if ( ioctl(console_fd, FBIOGET_FSCREENINFO, &finfo) < 0 ) {
		SDL_SetError("Couldn't get console hardware info");
		FB_VideoQuit(this);
		return(-1);
	}
	switch (finfo.visual) {
		case FB_VISUAL_TRUECOLOR:
		case FB_VISUAL_PSEUDOCOLOR:
		case FB_VISUAL_STATIC_PSEUDOCOLOR:
		case FB_VISUAL_DIRECTCOLOR:
			break;
		default:
			SDL_SetError("Unsupported console hardware");
			FB_VideoQuit(this);
			return(-1);
	}

	/* Check if the user wants to disable hardware acceleration */
	{ const char *fb_accel;
		fb_accel = getenv("SDL_FBACCEL");
		if ( fb_accel ) {
			finfo.accel = atoi(fb_accel);
		}
	}

	/* Memory map the device, compensating for buggy PPC mmap() */
	mapped_offset = (((long)finfo.smem_start) -
	                (((long)finfo.smem_start)&~(PAGE_SIZE-1)));
	mapped_memlen = finfo.smem_len+mapped_offset;
	mapped_mem = mmap(NULL, mapped_memlen,
	                  PROT_READ|PROT_WRITE, MAP_SHARED, console_fd, 0);
	if ( mapped_mem == (caddr_t)-1 ) {
		SDL_SetError("Unable to memory map the video hardware");
		mapped_mem = NULL;
		FB_VideoQuit(this);
		return(-1);
	}

	/* Determine the current screen depth */
	if ( ioctl(console_fd, FBIOGET_VSCREENINFO, &vinfo) < 0 ) {
		SDL_SetError("Couldn't get console pixel format");
		FB_VideoQuit(this);
		return(-1);
	}
	vformat->BitsPerPixel = vinfo.bits_per_pixel;
	for ( i=0; i<vinfo.red.length; ++i ) {
		vformat->Rmask <<= 1;
		vformat->Rmask |= (0x00000001<<vinfo.red.offset);
	}
	for ( i=0; i<vinfo.green.length; ++i ) {
		vformat->Gmask <<= 1;
		vformat->Gmask |= (0x00000001<<vinfo.green.offset);
	}
	for ( i=0; i<vinfo.blue.length; ++i ) {
		vformat->Bmask <<= 1;
		vformat->Bmask |= (0x00000001<<vinfo.blue.offset);
	}
	saved_vinfo = vinfo;

	/* Save hardware palette, if needed */
	FB_SavePalette(this, &finfo, &vinfo);

	/* If the I/O registers are available, memory map them so we
	   can take advantage of any supported hardware acceleration.
	 */
	vinfo.accel_flags = 0;	/* Temporarily reserve registers */
	ioctl(console_fd, FBIOPUT_VSCREENINFO, &vinfo);
	if ( finfo.accel && finfo.mmio_len ) {
		mapped_iolen = finfo.mmio_len;
		mapped_io = mmap(NULL, mapped_iolen, PROT_READ|PROT_WRITE,
		                 MAP_SHARED, console_fd, mapped_memlen);
		if ( mapped_io == (caddr_t)-1 ) {
			/* Hmm, failed to memory map I/O registers */
			mapped_io = NULL;
		}
	}

	/* Query for the list of available video modes */
	current_w = vinfo.xres;
	current_h = vinfo.yres;
	current_index = ((vinfo.bits_per_pixel+7)/8)-1;
	for ( i=0; i<NUM_MODELISTS; ++i ) {
		SDL_nummodes[i] = 0;
		SDL_modelist[i] = NULL;
		for ( j=0; j<(sizeof(checkres)/sizeof(checkres[0])); ++j ) {
			int w, h;

			/* See if we are querying for the current mode */
			w = checkres[j].w;
			h = checkres[j].h;
			if ( i == current_index ) {
				if ( (current_w > w) || (current_h > h) ) {
					/* Only check once */
					FB_AddMode(this, i,current_w,current_h);
					current_index = -1;
				}
			}
			if ( FB_CheckMode(this, &vinfo, i, &w, &h) ) {
				FB_AddMode(this, i, w, h);
			}
		}
	}

	/* Fill in our hardware acceleration capabilities */
	this->info.hw_available = 1;
	this->info.video_mem = finfo.smem_len/1024;
	if ( mapped_io ) {
		switch (finfo.accel) {
		    case FB_ACCEL_MATROX_MGA2064W:
		    case FB_ACCEL_MATROX_MGA1064SG:
		    case FB_ACCEL_MATROX_MGA2164W:
		    case FB_ACCEL_MATROX_MGA2164W_AGP:
		    case FB_ACCEL_MATROX_MGAG100:
		    /*case FB_ACCEL_MATROX_MGAG200: G200 acceleration broken! */
		    case FB_ACCEL_MATROX_MGAG400:
#ifdef FBACCEL_DEBUG
			printf("Matrox hardware accelerator!\n");
#endif
			FB_MatroxAccel(this, finfo.accel);
			break;
		    case FB_ACCEL_3DFX_BANSHEE:
#ifdef FBACCEL_DEBUG
			printf("3DFX hardware accelerator!\n");
#endif
			FB_3DfxAccel(this, finfo.accel);
			break;
		    default:
#ifdef FBACCEL_DEBUG
			printf("Unknown hardware accelerator.\n");
#endif
			break;
		}
	}

	/* Enable mouse and keyboard support */
	if ( FB_OpenKeyboard(this) < 0 ) {
		SDL_SetError("Unable to open keyboard");
		FB_VideoQuit(this);
		return(-1);
	}
	if ( FB_OpenMouse(this) < 0 ) {
		const char *sdl_nomouse;

		sdl_nomouse = getenv("SDL_NOMOUSE");
		if ( ! sdl_nomouse ) {
			SDL_SetError("Unable to open mouse");
			FB_VideoQuit(this);
			return(-1);
		}
	}

	/* We're done! */
	return(0);
}

SDL_Rect **FB_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
	return(SDL_modelist[((format->BitsPerPixel+7)/8)-1]);
}

/* Various screen update functions available */
static void FB_DirectUpdate(_THIS, int numrects, SDL_Rect *rects);

#ifdef FBCON_DEBUG
static void print_vinfo(struct fb_var_screeninfo *vinfo)
{
	fprintf(stderr, "Printing vinfo:\n");
	fprintf(stderr, "\txres: %d\n", vinfo->xres);
	fprintf(stderr, "\tyres: %d\n", vinfo->yres);
	fprintf(stderr, "\txres_virtual: %d\n", vinfo->xres_virtual);
	fprintf(stderr, "\tyres_virtual: %d\n", vinfo->yres_virtual);
	fprintf(stderr, "\txoffset: %d\n", vinfo->xoffset);
	fprintf(stderr, "\tyoffset: %d\n", vinfo->yoffset);
	fprintf(stderr, "\tbits_per_pixel: %d\n", vinfo->bits_per_pixel);
	fprintf(stderr, "\tgrayscale: %d\n", vinfo->grayscale);
	fprintf(stderr, "\tnonstd: %d\n", vinfo->nonstd);
	fprintf(stderr, "\tactivate: %d\n", vinfo->activate);
	fprintf(stderr, "\theight: %d\n", vinfo->height);
	fprintf(stderr, "\twidth: %d\n", vinfo->width);
	fprintf(stderr, "\taccel_flags: %d\n", vinfo->accel_flags);
	fprintf(stderr, "\tpixclock: %d\n", vinfo->pixclock);
	fprintf(stderr, "\tleft_margin: %d\n", vinfo->left_margin);
	fprintf(stderr, "\tright_margin: %d\n", vinfo->right_margin);
	fprintf(stderr, "\tupper_margin: %d\n", vinfo->upper_margin);
	fprintf(stderr, "\tlower_margin: %d\n", vinfo->lower_margin);
	fprintf(stderr, "\thsync_len: %d\n", vinfo->hsync_len);
	fprintf(stderr, "\tvsync_len: %d\n", vinfo->vsync_len);
	fprintf(stderr, "\tsync: %d\n", vinfo->sync);
	fprintf(stderr, "\tvmode: %d\n", vinfo->vmode);
	fprintf(stderr, "\tred: %d/%d\n", vinfo->red.length, vinfo->red.offset);
	fprintf(stderr, "\tgreen: %d/%d\n", vinfo->green.length, vinfo->green.offset);
	fprintf(stderr, "\tblue: %d/%d\n", vinfo->blue.length, vinfo->blue.offset);
	fprintf(stderr, "\talpha: %d/%d\n", vinfo->transp.length, vinfo->transp.offset);
}
#endif

SDL_Surface *FB_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;
	int i;
	Uint32 Rmask;
	Uint32 Gmask;
	Uint32 Bmask;
	caddr_t surfaces_mem;
	int surfaces_len;

	/* Restore the original palette */
	FB_RestorePalette(this);

	/* Set the video mode and get the final screen format */
	if ( ioctl(console_fd, FBIOGET_VSCREENINFO, &vinfo) < 0 ) {
		SDL_SetError("Couldn't get console screen info");
		return(NULL);
	}
#ifdef FBCON_DEBUG
	fprintf(stderr, "Printing original vinfo:\n");
	print_vinfo(&vinfo);
#endif
	vinfo.activate = FB_ACTIVATE_NOW;
	vinfo.accel_flags = 0;
	vinfo.bits_per_pixel = bpp;
	vinfo.xres = width;
	vinfo.xres_virtual = width;
	vinfo.yres = height;
	if ( flags & SDL_DOUBLEBUF ) {
		vinfo.yres_virtual = height*2;
	} else {
		vinfo.yres_virtual = height;
	}
	vinfo.xoffset = 0;
	vinfo.yoffset = 0;
	vinfo.red.length = vinfo.red.offset = 0;
	vinfo.green.length = vinfo.green.offset = 0;
	vinfo.blue.length = vinfo.blue.offset = 0;
	vinfo.transp.length = vinfo.transp.offset = 0;
	/* FIXME: Detect current video mode */
	/* Check for VESA timings (FIXME: allow modelines?) */
	for ( i=0; i<(sizeof(vesa_timings)/sizeof(vesa_timings[0])); ++i ) {
		if ( (vinfo.xres == vesa_timings[i].xres) &&
		     (vinfo.yres == vesa_timings[i].yres) ) {
#ifdef FBCON_DEBUG
	fprintf(stderr, "Using VESA timings for %dx%d\n",vinfo.xres,vinfo.yres);
#endif
			if ( vesa_timings[i].pixclock ) {
				vinfo.pixclock = vesa_timings[i].pixclock;
			}
			vinfo.left_margin = vesa_timings[i].left;
			vinfo.right_margin = vesa_timings[i].right;
			vinfo.upper_margin = vesa_timings[i].upper;
			vinfo.lower_margin = vesa_timings[i].lower;
			vinfo.hsync_len = vesa_timings[i].hslen;
			vinfo.vsync_len = vesa_timings[i].vslen;
			vinfo.sync = vesa_timings[i].sync;
			vinfo.vmode = vesa_timings[i].vmode;
			break;
		}
	}
#ifdef FBCON_DEBUG
	fprintf(stderr, "Printing wanted vinfo:\n");
	print_vinfo(&vinfo);
#endif
	if ( ioctl(console_fd, FBIOPUT_VSCREENINFO, &vinfo) < 0 ) {
		vinfo.yres_virtual = height;
		if ( ioctl(console_fd, FBIOPUT_VSCREENINFO, &vinfo) < 0 ) {
			SDL_SetError("Couldn't set console screen info");
			return(NULL);
		}
	}
	cache_vinfo = vinfo;
#ifdef FBCON_DEBUG
	fprintf(stderr, "Printing actual vinfo:\n");
	print_vinfo(&vinfo);
#endif
	Rmask = 0;
	for ( i=0; i<vinfo.red.length; ++i ) {
		Rmask <<= 1;
		Rmask |= (0x00000001<<vinfo.red.offset);
	}
	Gmask = 0;
	for ( i=0; i<vinfo.green.length; ++i ) {
		Gmask <<= 1;
		Gmask |= (0x00000001<<vinfo.green.offset);
	}
	Bmask = 0;
	for ( i=0; i<vinfo.blue.length; ++i ) {
		Bmask <<= 1;
		Bmask |= (0x00000001<<vinfo.blue.offset);
	}
	if ( ! SDL_ReallocFormat(current, vinfo.bits_per_pixel,
	                                  Rmask, Gmask, Bmask, 0) ) {
		return(NULL);
	}

	/* Get the fixed information about the console hardware.
	   This is necessary since finfo.line_length changes.
	 */
	if ( ioctl(console_fd, FBIOGET_FSCREENINFO, &finfo) < 0 ) {
		SDL_SetError("Couldn't get console hardware info");
		return(NULL);
	}

	/* Save hardware palette, if needed */
	FB_SavePalette(this, &finfo, &vinfo);

	/* Set up the new mode framebuffer */
	current->flags = (SDL_FULLSCREEN|SDL_HWSURFACE);
	current->w = vinfo.xres;
	current->h = vinfo.yres;
	current->pitch = finfo.line_length;
	current->pixels = mapped_mem+mapped_offset;

	/* Let the application know we have a hardware palette */
	switch (finfo.visual) {
	    case FB_VISUAL_PSEUDOCOLOR:
		current->flags |= SDL_HWPALETTE;
		break;
	    default:
		break;
	}

	/* Update for double-buffering, if we can */
	if ( flags & SDL_DOUBLEBUF ) {
		if ( vinfo.yres_virtual == (height*2) ) {
			current->flags |= SDL_DOUBLEBUF;
			flip_page = 0;
			flip_address[0] = (caddr_t)current->pixels;
			flip_address[1] = (caddr_t)current->pixels+
			                           current->h*current->pitch;
			FB_FlipHWSurface(this, current);
		}
	}

	/* Set up the information for hardware surfaces */
	surfaces_mem = (caddr_t)current->pixels +
	                        vinfo.yres_virtual*current->pitch;
	surfaces_len = (mapped_memlen-(surfaces_mem-mapped_mem));
	FB_FreeHWSurfaces(this);
	FB_InitHWSurfaces(this, surfaces_mem, surfaces_len);

	/* Set the update rectangle function */
	this->UpdateRects = FB_DirectUpdate;

	/* We're done */
	return(current);
}

#ifdef FBCON_DEBUG
static void FB_DumpHWSurfaces(_THIS)
{
	vidmem_bucket *bucket;

	printf("Memory left: %d (%d total)\n", surfaces_memleft, surfaces_memtotal);
	printf("\n");
	printf("         Base  Size\n");
	for ( bucket=&surfaces; bucket; bucket=bucket->next ) {
		printf("Bucket:  %p, %d (%s)\n", bucket->base, bucket->size, bucket->used ? "used" : "free");
		if ( bucket->prev ) {
			if ( bucket->base != bucket->prev->base+bucket->prev->size ) {
				printf("Warning, corrupt bucket list! (prev)\n");
			}
		} else {
			if ( bucket != &surfaces ) {
				printf("Warning, corrupt bucket list! (!prev)\n");
			}
		}
		if ( bucket->next ) {
			if ( bucket->next->base != bucket->base+bucket->size ) {
				printf("Warning, corrupt bucket list! (next)\n");
			}
		}
	}
	printf("\n");
}
#endif

static int FB_InitHWSurfaces(_THIS, caddr_t base, int size)
{
	surfaces.prev = NULL;
	surfaces.used = 0;
	surfaces.base = base;
	surfaces.size = size;
	surfaces.next = NULL;
	surfaces_memtotal = size;
	surfaces_memleft = size;
	return(0);
}
static void FB_FreeHWSurfaces(_THIS)
{
	vidmem_bucket *bucket, *freeable;

	bucket = surfaces.next;
	while ( bucket ) {
		freeable = bucket;
		bucket = bucket->next;
		free(freeable);
	}
	surfaces.next = NULL;
}

static int FB_AllocHWSurface(_THIS, SDL_Surface *surface)
{
	vidmem_bucket *bucket;
	int size;
	int extra;

/* Temporarily, we only allow surfaces the same width as display.
   Some blitters require the pitch between two hardware surfaces
   to be the same.  Others have interesting alignment restrictions.
   Until someone who knows these details looks at the code...
*/
if ( surface->pitch > SDL_VideoSurface->pitch ) {
	SDL_SetError("Surface requested wider than screen");
	return(-1);
}
surface->pitch = SDL_VideoSurface->pitch;
	size = surface->h * surface->pitch;
#ifdef FBCON_DEBUG
	fprintf(stderr, "Allocating bucket of %d bytes\n", size);
#endif

	/* Quick check for available mem */
	if ( size > surfaces_memleft ) {
		SDL_SetError("Not enough video memory");
		return(-1);
	}

	/* Search for an empty bucket big enough */
	for ( bucket=&surfaces; bucket; bucket=bucket->next ) {
		if ( ! bucket->used && (size <= bucket->size) ) {
			break;
		}
	}
	if ( bucket == NULL ) {
		SDL_SetError("Video memory too fragmented");
		return(-1);
	}

	/* Create a new bucket for left-over memory */
	extra = (bucket->size - size);
	if ( extra ) {
		vidmem_bucket *newbucket;

#ifdef FBCON_DEBUG
	fprintf(stderr, "Adding new free bucket of %d bytes\n", extra);
#endif
		newbucket = (vidmem_bucket *)malloc(sizeof(*newbucket));
		if ( newbucket == NULL ) {
			SDL_OutOfMemory();
			return(-1);
		}
		newbucket->prev = bucket;
		newbucket->used = 0;
		newbucket->base = bucket->base+size;
		newbucket->size = extra;
		newbucket->next = bucket->next;
		if ( bucket->next ) {
			bucket->next->prev = newbucket;
		}
		bucket->next = newbucket;
	}

	/* Set the current bucket values and return it! */
	bucket->used = 1;
	bucket->size = size;
#ifdef FBCON_DEBUG
	fprintf(stderr, "Allocated %d bytes at %p\n", bucket->size, bucket->base);
#endif
	surfaces_memleft -= size;
	surface->flags |= SDL_HWSURFACE;
	surface->pixels = bucket->base;
	return(0);
}
static void FB_FreeHWSurface(_THIS, SDL_Surface *surface)
{
	vidmem_bucket *bucket, *freeable;

	/* Look for the bucket in the current list */
	for ( bucket=&surfaces; bucket; bucket=bucket->next ) {
		if ( bucket->base == (caddr_t)surface->pixels ) {
			break;
		}
	}
	if ( (bucket == NULL) || ! bucket->used ) {
		return;
	}

	/* Add the memory back to the total */
#ifdef FBCON_DEBUG
	printf("Freeing bucket of %d bytes\n", bucket->size);
#endif
	surfaces_memleft += bucket->size;

	/* Can we merge the space with surrounding buckets? */
	bucket->used = 0;
	if ( bucket->next && ! bucket->next->used ) {
#ifdef FBCON_DEBUG
	printf("Merging with next bucket, for %d total bytes\n", bucket->size+bucket->next->size);
#endif
		freeable = bucket->next;
		bucket->size += bucket->next->size;
		bucket->next = bucket->next->next;
		if ( bucket->next ) {
			bucket->next->prev = bucket;
		}
		free(freeable);
	}
	if ( bucket->prev && ! bucket->prev->used ) {
#ifdef FBCON_DEBUG
	printf("Merging with previous bucket, for %d total bytes\n", bucket->prev->size+bucket->size);
#endif
		freeable = bucket;
		bucket->prev->size += bucket->size;
		bucket->prev->next = bucket->next;
		if ( bucket->next ) {
			bucket->next->prev = bucket->prev;
		}
		free(freeable);
	}
	surface->pixels = NULL;
}
static int FB_LockHWSurface(_THIS, SDL_Surface *surface)
{
	if ( surface == SDL_VideoSurface ) {
		SDL_mutexP(hw_lock);
	}
	return(0);
}
static void FB_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
	if ( surface == SDL_VideoSurface ) {
		SDL_mutexV(hw_lock);
	}
}

static void FB_WaitVBL(_THIS)
{
#ifdef FBIOWAITRETRACE
	ioctl(console_fd, FBIOWAITRETRACE, 0);
#endif
	return;
}

static int FB_FlipHWSurface(_THIS, SDL_Surface *surface)
{
	/* Wait for vertical retrace and then flip display */
	cache_vinfo.yoffset = flip_page*surface->h;
	wait_vbl(this);
	if ( ioctl(console_fd, FBIOPAN_DISPLAY, &cache_vinfo) < 0 ) {
		SDL_SetError("ioctl(FBIOPAN_DISPLAY) failed");
		return(-1);
	}
	flip_page = !flip_page;

	surface->pixels = flip_address[flip_page];
	return(0);
}

static void FB_DirectUpdate(_THIS, int numrects, SDL_Rect *rects)
{
	/* The application is already updating the visible video memory */
	return;
}

void FB_SavePaletteTo(_THIS, int palette_len, __u16 *area)
{
	struct fb_cmap cmap;

	cmap.start = 0;
	cmap.len = palette_len;
	cmap.red = &area[0*palette_len];
	cmap.green = &area[1*palette_len];
	cmap.blue = &area[2*palette_len];
	cmap.transp = NULL;
	ioctl(console_fd, FBIOGETCMAP, &cmap);
}

void FB_RestorePaletteFrom(_THIS, int palette_len, __u16 *area)
{
	struct fb_cmap cmap;

	cmap.start = 0;
	cmap.len = palette_len;
	cmap.red = &area[0*palette_len];
	cmap.green = &area[1*palette_len];
	cmap.blue = &area[2*palette_len];
	cmap.transp = NULL;
	ioctl(console_fd, FBIOPUTCMAP, &cmap);
}

static void FB_SavePalette(_THIS, struct fb_fix_screeninfo *finfo,
                                  struct fb_var_screeninfo *vinfo)
{
	int i;

	/* Save hardware palette, if needed */
	if ( finfo->visual == FB_VISUAL_PSEUDOCOLOR ) {
		saved_cmaplen = 1<<vinfo->bits_per_pixel;
		saved_cmap=(__u16 *)malloc(3*saved_cmaplen*sizeof(*saved_cmap));
		if ( saved_cmap != NULL ) {
			FB_SavePaletteTo(this, saved_cmaplen, saved_cmap);
		}
	}

	/* Added support for FB_VISUAL_DIRECTCOLOR.
	   With this mode pixel information is passed through the palette...
	   Neat fading and gamma correction effects can be had by simply
	   fooling around with the palette instead of changing the pixel
	   values themselves... Very neat!

	   Adam Meyerowitz 1/19/2000
	   ameyerow@optonline.com
	*/
	if ( finfo->visual == FB_VISUAL_DIRECTCOLOR ) {
		__u16 new_entries[3*256];

		/* Save the colormap */
		saved_cmaplen = 256;
		saved_cmap=(__u16 *)malloc(3*saved_cmaplen*sizeof(*saved_cmap));
		if ( saved_cmap != NULL ) {
			FB_SavePaletteTo(this, saved_cmaplen, saved_cmap);
		}

		/* Allocate new identity colormap */
		for ( i=0; i<256; ++i ) {
	      		new_entries[(0*256)+i] =
			new_entries[(1*256)+i] =
			new_entries[(2*256)+i] = (i<<8)|i;
		}
		FB_RestorePaletteFrom(this, 256, new_entries);
	}
}

static void FB_RestorePalette(_THIS)
{
	/* Restore the original palette */
	if ( saved_cmap ) {
		FB_RestorePaletteFrom(this, saved_cmaplen, saved_cmap);
		free(saved_cmap);
		saved_cmap = NULL;
	}
}

static int FB_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
	int i;
	__u16 r[256];
	__u16 g[256];
	__u16 b[256];
	struct fb_cmap cmap;

	/* Set up the colormap */
	for (i = 0; i < ncolors; i++) {
		r[i] = colors[i].r << 8;
		g[i] = colors[i].g << 8;
		b[i] = colors[i].b << 8;
	}
	cmap.start = firstcolor;
	cmap.len = ncolors;
	cmap.red = r;
	cmap.green = g;
	cmap.blue = b;
	cmap.transp = NULL;

	if( (ioctl(console_fd, FBIOPUTCMAP, &cmap) < 0) &&
	    !(this->screen->flags & SDL_HWPALETTE) ) {
	        colors = this->screen->format->palette->colors;
		ncolors = this->screen->format->palette->ncolors;
		cmap.start = 0;
		cmap.len = ncolors;
		if ( ioctl(console_fd, FBIOGETCMAP, &cmap) == 0 ) {
			for ( i=ncolors-1; i>=0; --i ) {
				colors[i].r = (r[i]>>8);
				colors[i].g = (g[i]>>8);
				colors[i].b = (b[i]>>8);
			}
		}
		return(0);
	}
	return(1);
}

/* Note:  If we are terminated, this could be called in the middle of
   another SDL video routine -- notably UpdateRects.
*/
void FB_VideoQuit(_THIS)
{
	int i, j;

	if ( this->screen ) {
		/* Clear screen and tell SDL not to free the pixels */
		if ( this->screen->pixels ) {
#ifdef __powerpc__	/* SIGBUS when using memset() ?? */
			Uint8 *rowp = (Uint8 *)this->screen->pixels;
			int left = this->screen->pitch*this->screen->h;
			while ( left-- ) { *rowp++ = 0; }
#else
			memset(this->screen->pixels,0,this->screen->h*this->screen->pitch);
#endif
			this->screen->pixels = NULL;
		}
	}

	/* Clear the lock mutex */
	if ( hw_lock != NULL ) {
		SDL_DestroyMutex(hw_lock);
		hw_lock = NULL;
	}

	/* Clean up defined video modes */
	for ( i=0; i<NUM_MODELISTS; ++i ) {
		if ( SDL_modelist[i] != NULL ) {
			for ( j=0; SDL_modelist[i][j]; ++j ) {
				free(SDL_modelist[i][j]);
			}
			free(SDL_modelist[i]);
			SDL_modelist[i] = NULL;
		}
	}

	/* Clean up the memory bucket list */
	FB_FreeHWSurfaces(this);

	/* Close console and input file descriptors */
	if ( console_fd > 0 ) {
		/* Unmap the video framebuffer and I/O registers */
		if ( mapped_mem ) {
			munmap(mapped_mem, mapped_memlen);
			mapped_mem = NULL;
		}
		if ( mapped_io ) {
			munmap(mapped_io, mapped_iolen);
			mapped_io = NULL;
		}

		/* Restore the original video mode and palette */
		FB_RestorePalette(this);
		ioctl(console_fd, FBIOPUT_VSCREENINFO, &saved_vinfo);

		/* We're all done with the framebuffer */
		close(console_fd);
		console_fd = -1;
	}
	FB_CloseMouse(this);
	FB_CloseKeyboard(this);
}