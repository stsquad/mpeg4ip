/* 
 *  enc_input.c
 *
 *     Copyright (C) Peter Schlaile - Feb 2001
 *
 *  This file is part of libdv, a free DV (IEC 61834/SMPTE 314M)
 *  codec.
 *
 *  libdv is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your
 *  option) any later version.
 *   
 *  libdv is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 *  The libdv homepage is http://libdv.sourceforge.net/.  
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "enc_input.h"
#include "encode.h"
#include "dct.h"
#include "dv_types.h"
#if ARCH_X86
#include "mmx.h"
#else
#include <math.h>
#endif

#if HAVE_LINUX_VIDEODEV_H
#define HAVE_DEV_VIDEO  1
#endif

#if HAVE_DEV_VIDEO
#include <sys/types.h>
#include <linux/videodev.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#endif

// #define ARCH_X86 0

#if !ARCH_X86
inline gint f2b(float f)
{
	int b = rint(f);
	if (b < 0)
		b = 0;
	if (b > 255)
		b = 255;
	
	return b;
}


inline gint f2sb(float f)
{
	int b = rint(f);
	
	return b;
}
#endif

extern void rgbtoycb_mmx(unsigned char* inPtr, int rows, int columns,
			 short* outyPtr, short* outuPtr, short* outvPtr);

void dv_enc_rgb_to_ycb(unsigned char* img_rgb, int height,
		       short* img_y, short* img_cr, short* img_cb)
{
#if !ARCH_X86
#if 1
       int i;
       int ti;
       unsigned char *ip;
       register long r,g,b ;
       long colr, colb;
       register short int *ty, *tr, *tb;
       ip =  img_rgb;  
       colr = colb =  0;
       ty = img_y;     
       tr = img_cr;
       tb = img_cb;
       ti = height * DV_WIDTH ;
       for (i = 0; i < ti; i++) {
	       r = *ip++;
	       g = *ip++; 
	       b = *ip++;

	       *ty++ =  (( ( (16828 * r) + (33038 * g) + (6416 * b) )
			   >> 16   ) - 128 + 16) << DCT_YUV_PRECISION;

	       colr +=  ( (28784 * r) + (-24121 * g) + (-4663 * b) ) ; 
	       colb +=  ( (-9729 * r) + (-19055 * g) + (28784 * b) ) ;
	       if (! (i % 2)) {
		       *tr++ = colr >> (16 + 1 - DCT_YUV_PRECISION);
		       *tb++ = colb >> (16 + 1 - DCT_YUV_PRECISION);
		       colr = colb = 0;
	       }       
       }
#else
	int x,y;
	/* This is the RGB -> YUV color matrix */
	static const double cm[3][3] = {
		{.299 * 219.0/255.0,.587* 219.0/255.0,.114* 219.0/255.0},
		{.5* 224.0/255.0,-.419* 224.0/255.0,-.081* 224.0/255.0},
		{-.169 * 224.0/255.0,-.331* 224.0/255.0,.5* 224.0/255.0}
	};

	double tmp_cr[DV_PAL_HEIGHT][DV_WIDTH];
	double tmp_cb[DV_PAL_HEIGHT][DV_WIDTH];
	double fac = pow(2, DCT_YUV_PRECISION);
	int i,j;

	for (y = 0; y < height; y++) {
		for (x = 0; x < DV_WIDTH; x++) {
			register double cy, cr, cb;
			register int r = img_rgb[(y * DV_WIDTH + x)*3 + 0];
			register int g = img_rgb[(y * DV_WIDTH + x)*3 + 1];
			register int b = img_rgb[(y * DV_WIDTH + x)*3 + 2];
			cy =    (cm[0][0] * r) + (cm[0][1] * g) +
				(cm[0][2] * b);
			cr =    (cm[1][0] * r) + (cm[1][1] * g) +
				(cm[1][2] * b);
			cb =    (cm[2][0] * r) + (cm[2][1] * g) +
				(cm[2][2] * b);
			
			img_y[y * DV_WIDTH + x] = 
				(f2sb(cy) - 128 + 16) * fac;
			tmp_cr[y][x] = cr * fac;
			tmp_cb[y][x] = cb * fac;
		}
	}
	for (y = 0; y < height; y++) {
		for (x = 0; x < DV_WIDTH / 2; x++) {
			img_cr[y * DV_WIDTH / 2 + x] = 
				f2sb((tmp_cr[y][2*x] +
				      tmp_cr[y][2*x+1]) / 2.0);
			img_cb[y * DV_WIDTH / 2 + x] = 
				f2sb((tmp_cb[y][2*x] +
				      tmp_cb[y][2*x+1]) / 2.0);
		}
	}
#endif
#else
	rgbtoycb_mmx(img_rgb, height, DV_WIDTH, (short*) img_y,
		     (short*) img_cr, (short*) img_cb);
	emms();
#endif
}


static unsigned char* readbuf = NULL;
static unsigned char* real_readbuf = 0; /* for wrong interlacing */
static int force_dct = 0;
static int wrong_interlace = 0;

static short* img_y = NULL; /* [DV_PAL_HEIGHT * DV_WIDTH]; */
static short* img_cr = NULL; /* [DV_PAL_HEIGHT * DV_WIDTH / 2]; */
static short* img_cb = NULL; /* [DV_PAL_HEIGHT * DV_WIDTH / 2]; */

#if !ARCH_X86

int need_dct_248_transposed(dv_coeff_t * bl)
{
	int res_cols = 1;
	int res_rows = 1;
	int i,j;
	
	for (j = 0; j < 7; j ++) {
		for (i = 0; i < 8; i++) {
			int a = bl[i * 8 + j] - bl[i * 8 + j + 1];
			int b = (a >> 15);
			a ^= b;
			a -= b;
			res_cols += a;
		}
	}

	for (j = 0; j < 7; j ++) {
		for (i = 0; i < 8; i++) {
			int a = bl[j * 8 + i] - bl[(j + 1) * 8 + i];
			int b = (a >> 15);
			a ^= b;
			a -= b;
			res_rows += a;
		}
	}

	return ((res_cols * 65536 / res_rows) > DCT_248_THRESHOLD);
}

#else

extern int need_dct_248_mmx_rows(dv_coeff_t * bl);

extern void transpose_mmx(short * dst);
extern void ppm_copy_y_block_mmx(short * dst, short * src);
extern void ppm_copy_pal_c_block_mmx(short * dst, short * src);
extern void ppm_copy_ntsc_c_block_mmx(short * dst, short * src);

static void finish_mb_mmx(dv_macroblock_t* mb)
{
	int b;
	int need_dct_248_rows[6];
	dv_block_t* bl = mb->b;

	if (force_dct != -1) {
		for (b = 0; b < 6; b++) {
			bl[b].dct_mode = force_dct;
		}
	} else {
		for (b = 0; b < 6; b++) {
			need_dct_248_rows[b]
				= need_dct_248_mmx_rows(bl[b].coeffs) + 1;
		}
	}
	transpose_mmx(bl[0].coeffs);
	transpose_mmx(bl[1].coeffs);
	transpose_mmx(bl[2].coeffs);
	transpose_mmx(bl[3].coeffs);
	transpose_mmx(bl[4].coeffs);
	transpose_mmx(bl[5].coeffs);

	if (force_dct == -1) {
		for (b = 0; b < 6; b++) {
			bl[b].dct_mode = 
				((need_dct_248_rows[b] * 65536 / 
				  (need_dct_248_mmx_rows(bl[b].coeffs) + 1))
				 > DCT_248_THRESHOLD) ? DV_DCT_248 : DV_DCT_88;
		}
	}
}

#endif /* ARCH_X86 */

static int read_ppm_stream(FILE* f, int * isPAL, int * height_)
{
	int height, width;
	char line[200];
	fgets(line, sizeof(line), f);
	if (feof(f)) {
		return -1;
	}
	do {
		fgets(line, sizeof(line), f); /* P6 */
	} while ((line[0] == '#'||(line[0] == '\n')) && !feof(f));
	if (sscanf(line, "%d %d\n", &width, &height) != 2) {
		fprintf(stderr, "Bad PPM file!\n");
		return -1;
	}
	if (width != DV_WIDTH || 
	    (height != DV_PAL_HEIGHT && height != DV_NTSC_HEIGHT)) {
		fprintf(stderr, "Invalid picture size! (%d, %d)\n"
			"Allowed sizes are 720x576 for PAL and "
			"720x480 for NTSC\n"
			"Probably you should try ppmqscale...\n", 
			width, height);
		return -1;
	}
	fgets(line, sizeof(line), f);	/* 255 */
	
	fread(readbuf, 1, 3 * DV_WIDTH * height, f);

	*height_ = height;
	*isPAL = (height == DV_PAL_HEIGHT);

	if (wrong_interlace) {
		memcpy(readbuf + DV_WIDTH * height*3, 
		       readbuf + DV_WIDTH * (height-1)*3, DV_WIDTH*3);
	}

	return 0;
}

static int ppm_init(int wrong_interlace_, int force_dct_) 
{
	wrong_interlace = wrong_interlace_;

	readbuf = (unsigned char*) calloc(DV_WIDTH * (DV_PAL_HEIGHT + 1), 3);
	real_readbuf = readbuf;
	if (wrong_interlace) {
		real_readbuf += DV_WIDTH * 3;
	}
	force_dct = force_dct_;

	img_y = (short*) calloc(DV_PAL_HEIGHT * DV_WIDTH, sizeof(short));
	img_cr = (short*) calloc(DV_PAL_HEIGHT * DV_WIDTH / 2, sizeof(short));
	img_cb = (short*) calloc(DV_PAL_HEIGHT * DV_WIDTH / 2, sizeof(short));

	return 0;
}

static void ppm_finish()
{
	free(readbuf);
	free(img_y);
	free(img_cr);
	free(img_cb);
}

static int ppm_load(const char* filename, int * isPAL)
{
	FILE* ppm_in = NULL;
	int rval = -1;
	int height;

	if (strcmp(filename, "-") == 0) {
		ppm_in = stdin;
	} else {
		ppm_in = fopen(filename, "r");
		if (ppm_in == NULL) {
			return -1;
		}
	}

	rval = read_ppm_stream(ppm_in, isPAL, &height);
	if (ppm_in != stdin) {
		fclose(ppm_in);
	}
	if (rval != -1) {
		dv_enc_rgb_to_ycb(real_readbuf, height, img_y, img_cr, img_cb);
	}
	return rval;
}

static int ppm_skip(const char* filename, int * isPAL)
{
	int rval = 0;
	int height;

	if (strcmp(filename, "-") == 0) {
		rval = read_ppm_stream(stdin, isPAL, &height);
	} 

	return rval;
}

static void ppm_fill_macroblock(dv_macroblock_t *mb, int isPAL)
{
	int y = mb->y;
	int x = mb->x;
	dv_block_t* bl = mb->b;

#if !ARCH_X86
	if (isPAL || mb->x == DV_WIDTH- 16) { /* PAL or rightmost NTSC block */
		int i,j;
		for (j = 0; j < 8; j++) {
			for (i = 0; i < 8; i++) {
				bl[0].coeffs[8 * i + j] = 
					img_y[(y + j) * DV_WIDTH +  x + i];
				bl[1].coeffs[8 * i + j] = 
					img_y[(y + j) * DV_WIDTH +  x + 8 + i];
				bl[2].coeffs[8 * i + j] = 
					img_y[(y + 8 + j) * DV_WIDTH + x + i];
				bl[3].coeffs[8 * i + j] = 
					img_y[(y + 8 + j) * DV_WIDTH 
					     + x + 8 + i];
				bl[4].coeffs[8 * i + j] = 
					(img_cr[(y + 2*j) * DV_WIDTH/2 
					       + x / 2 + i]
					+ img_cr[(y + 2*j + 1) * DV_WIDTH/2
						+ x / 2 + i]) >> 1;
				bl[5].coeffs[8 * i + j] = 
					(img_cb[(y + 2*j) * DV_WIDTH/2
					      + x / 2 + i]
					+ img_cb[(y + 2*j + 1) * DV_WIDTH/2
						+ x / 2 + i]) >> 1;
			}
		}
	} else {                        /* NTSC */
		int i,j;
		for (j = 0; j < 8; j++) {
			for (i = 0; i < 8; i++) {
				bl[0].coeffs[8 * i + j] = 
					img_y[(y + j) * DV_WIDTH +  x + i];
				bl[1].coeffs[8 * i + j] = 
					img_y[(y + j) * DV_WIDTH +  x + 8 + i];
				bl[2].coeffs[8 * i + j] = 
					img_y[(y + j) * DV_WIDTH + x + 16 + i];
				bl[3].coeffs[8 * i + j] = 
					img_y[(y + j) * DV_WIDTH + x + 24 + i];
				bl[4].coeffs[8 * i + j] = 
					(img_cr[(y + j) * DV_WIDTH/2
					       + x / 2 + i*2]
					 + img_cr[(y + j) * DV_WIDTH/2 
						 + x / 2 + 1 + i*2]) >> 1;
				bl[5].coeffs[8 * i + j] = 
					(img_cb[(y + j) * DV_WIDTH/2
					       + x / 2 + i*2]
					 + img_cb[(y + j) * DV_WIDTH/2 
						 + x / 2 + 1 + i*2]) >> 1;
			}
		}
	}
	if (force_dct != -1) {
		int b;
		for (b = 0; b < 6; b++) {
			bl[b].dct_mode = force_dct;
		}
	} else {
		int b;
		for (b = 0; b < 6; b++) {
			bl[b].dct_mode = need_dct_248_transposed(bl[b].coeffs) 
				? DV_DCT_248 : DV_DCT_88;
		}
	}
#else
	if (isPAL || mb->x == DV_WIDTH- 16) { /* PAL or rightmost NTSC block */
		short* start_y = img_y + y * DV_WIDTH + x;
		ppm_copy_y_block_mmx(bl[0].coeffs, start_y);
		ppm_copy_y_block_mmx(bl[1].coeffs, start_y + 8);
		ppm_copy_y_block_mmx(bl[2].coeffs, start_y + 8 * DV_WIDTH);
		ppm_copy_y_block_mmx(bl[3].coeffs, start_y + 8 * DV_WIDTH + 8);
		ppm_copy_pal_c_block_mmx(bl[4].coeffs,
					 img_cr+y * DV_WIDTH/2+ x/2);
		ppm_copy_pal_c_block_mmx(bl[5].coeffs,
					 img_cb+y * DV_WIDTH/2+ x/2);
	} else {                              /* NTSC */
		short* start_y = img_y + y * DV_WIDTH + x;
		ppm_copy_y_block_mmx(bl[0].coeffs, start_y);
		ppm_copy_y_block_mmx(bl[1].coeffs, start_y + 8);
		ppm_copy_y_block_mmx(bl[2].coeffs, start_y + 16);
		ppm_copy_y_block_mmx(bl[3].coeffs, start_y + 24);
		ppm_copy_ntsc_c_block_mmx(bl[4].coeffs,
					  img_cr + y*DV_WIDTH/2 + x/2);
		ppm_copy_ntsc_c_block_mmx(bl[5].coeffs,
					  img_cb + y*DV_WIDTH/2 + x/2);
	}

	finish_mb_mmx(mb);

	emms();
#endif
}


static int read_pgm_stream(FILE* f, int * isPAL, int * height_)
{
	int height, width;
	char line[200];
	fgets(line, sizeof(line), f);
	if (feof(f)) {
		return -1;
	}
	do {
		fgets(line, sizeof(line), f); /* P5 */
	} while ((line[0] == '#'||(line[0] == '\n')) && !feof(f));
	if (sscanf(line, "%d %d\n", &width, &height) != 2) {
		fprintf(stderr, "Bad PGM file!\n");
		return -1;
	}
	height = height / 3 * 2;
	if (width != DV_WIDTH || 
	    (height != DV_PAL_HEIGHT && height != DV_NTSC_HEIGHT)) {
		fprintf(stderr, "Invalid picture size! (%d, %d)\n"
			"Allowed sizes are 720x864 for PAL and "
			"720x720 for NTSC\n"
			"Probably you should try ppms and ppmqscale...\n" 
			"(Or write pgmqscale and include it in libdv ;-)\n",
			width, height);
		return -1;
	}
	fgets(line, sizeof(line), f);	/* 255 */
	
	fread(readbuf, 1, DV_WIDTH * height * 3 / 2, f);

	*height_ = height;
	*isPAL = (height == DV_PAL_HEIGHT);

	if (wrong_interlace) {
		memcpy(readbuf + DV_WIDTH * height, 
		       readbuf + DV_WIDTH * (height-1), DV_WIDTH);
		memcpy(readbuf + DV_WIDTH * (height*3/2), 
		       readbuf + DV_WIDTH * (height*3/2-1), DV_WIDTH);
	}

	return 0;
}

static int pgm_init(int wrong_interlace_, int force_dct_) 
{
	wrong_interlace = wrong_interlace_;
	force_dct = force_dct_;

	readbuf = (unsigned char*) calloc(DV_WIDTH * (DV_PAL_HEIGHT + 1), 3);
	real_readbuf = readbuf;
	if (wrong_interlace) {
		real_readbuf += DV_WIDTH;
	}
	return 0;
}

static void pgm_finish()
{
	free(readbuf);
}

static int pgm_load(const char* filename, int * isPAL)
{
	FILE* pgm_in = NULL;
	int rval = -1;
	int height;

	if (strcmp(filename, "-") == 0) {
		pgm_in = stdin;
	} else {
		pgm_in = fopen(filename, "r");
		if (pgm_in == NULL) {
			return -1;
		}
	}

	rval = read_pgm_stream(pgm_in, isPAL, &height);
	if (pgm_in != stdin) {
		fclose(pgm_in);
	}
	return rval;
}

static int pgm_skip(const char* filename, int * isPAL)
{
	int rval = 0;
	int height;

	if (strcmp(filename, "-") == 0) {
		rval = read_pgm_stream(stdin, isPAL, &height);
	} 

	return rval;
}

#if !ARCH_X86
static inline short pgm_get_y(int y, int x)
{
	return (((short) real_readbuf[y * DV_WIDTH + x]) - 128 + 16)
		<< DCT_YUV_PRECISION;
}

static inline short pgm_get_cr_pal(int y, int x)
{
	return (real_readbuf[DV_PAL_HEIGHT * DV_WIDTH + DV_WIDTH/2  
			    + y * DV_WIDTH + x] 
		- 128) << DCT_YUV_PRECISION;

}

static inline short pgm_get_cb_pal(int y, int x)
{
	return (real_readbuf[DV_PAL_HEIGHT * DV_WIDTH +
			    + y * DV_WIDTH + x] - 128) << DCT_YUV_PRECISION;

}

static inline short pgm_get_cr_ntsc(int y, int x)
{
	return ((((short) real_readbuf[DV_NTSC_HEIGHT * DV_WIDTH+ DV_WIDTH/2
				      + y * DV_WIDTH + x]) - 128)
		+ (((short) real_readbuf[DV_NTSC_HEIGHT * DV_WIDTH+DV_WIDTH/2
					+ y * DV_WIDTH + x + 1]) - 128))
						<< (DCT_YUV_PRECISION - 1);
}

static inline short pgm_get_cb_ntsc(int y, int x)
{
	return ((((short) real_readbuf[DV_NTSC_HEIGHT * DV_WIDTH
				      + y * DV_WIDTH + x]) - 128)
		+ (((short) real_readbuf[DV_NTSC_HEIGHT * DV_WIDTH
					+ y * DV_WIDTH + x + 1]) - 128))
						<< (DCT_YUV_PRECISION - 1);
}

#else
extern void pgm_copy_y_block_mmx(short * dst, unsigned char * src);
extern void pgm_copy_pal_c_block_mmx(short * dst, unsigned char * src);
extern void pgm_copy_ntsc_c_block_mmx(short * dst, unsigned char * src);
#endif

static void pgm_fill_macroblock(dv_macroblock_t *mb, int isPAL)
{
	int y = mb->y;
	int x = mb->x;
	dv_block_t* bl = mb->b;
#if !ARCH_X86
	if (isPAL || mb->x == DV_WIDTH- 16) { /* PAL or rightmost NTSC block */
		int i,j;
		for (j = 0; j < 8; j++) {
			for (i = 0; i < 8; i++) {
				bl[0].coeffs[8*i + j] = pgm_get_y(y+j,x+i);
				bl[1].coeffs[8*i + j] = pgm_get_y(y+j,x+8+i);
				bl[2].coeffs[8*i + j] = pgm_get_y(y+8+j,x+i);
				bl[3].coeffs[8*i + j] = pgm_get_y(y+8+j,x+8+i);
				bl[4].coeffs[8*i + j] = 
					pgm_get_cr_pal(y/2+j, x/2+i);
				bl[5].coeffs[8*i + j] = 
					pgm_get_cb_pal(y/2+j, x/2+i);
			}
		}
	} else {                        /* NTSC */
		int i,j;
		for (i = 0; i < 8; i++) {
			for (j = 0; j < 8; j++) {
				bl[0].coeffs[8*i + j] = pgm_get_y(y+j,x+i);
				bl[1].coeffs[8*i + j] = pgm_get_y(y+j,x+8+i);
				bl[2].coeffs[8*i + j] = pgm_get_y(y+j,x+16+i);
				bl[3].coeffs[8*i + j] = pgm_get_y(y+j,x+24+i);
			}
			for (j = 0; j < 4; j++) {
				bl[4].coeffs[8*i + j*2] = 
					bl[4].coeffs[8*i + j*2 + 1] = 
					pgm_get_cr_ntsc(y + j, x/2 + i * 2);
				bl[5].coeffs[8*i + j*2] = 
					bl[5].coeffs[8*i + j*2 + 1] = 
					pgm_get_cb_ntsc(y + j, x/2 + i * 2);
			}
		}
	}
	if (force_dct != -1) {
		int b;
		for (b = 0; b < 6; b++) {
			bl[b].dct_mode = force_dct;
		}
	} else {
		int b;
		for (b = 0; b < 6; b++) {
			bl[b].dct_mode = need_dct_248_transposed(bl[b].coeffs) 
				? DV_DCT_248 : DV_DCT_88;
		}
	}
#else
	if (isPAL || mb->x == DV_WIDTH- 16) { /* PAL or rightmost NTSC block */
		unsigned char* start_y = real_readbuf + y * DV_WIDTH + x;
		unsigned char* img_cr = real_readbuf 
			+ (isPAL ? (DV_WIDTH * DV_PAL_HEIGHT)
			   : (DV_WIDTH * DV_NTSC_HEIGHT)) + DV_WIDTH / 2;
		unsigned char* img_cb = real_readbuf 
			+ (isPAL ? (DV_WIDTH * DV_PAL_HEIGHT) 
			   : (DV_WIDTH * DV_NTSC_HEIGHT));

		pgm_copy_y_block_mmx(bl[0].coeffs, start_y);
		pgm_copy_y_block_mmx(bl[1].coeffs, start_y + 8);
		pgm_copy_y_block_mmx(bl[2].coeffs, start_y + 8 * DV_WIDTH);
		pgm_copy_y_block_mmx(bl[3].coeffs, start_y + 8 * DV_WIDTH + 8);
		pgm_copy_pal_c_block_mmx(bl[4].coeffs,
					 img_cr + y * DV_WIDTH / 2 + x / 2);
		pgm_copy_pal_c_block_mmx(bl[5].coeffs,
					 img_cb + y * DV_WIDTH / 2 + x / 2);
	} else {                              /* NTSC */
		unsigned char* start_y = real_readbuf + y * DV_WIDTH + x;
		unsigned char* img_cr = real_readbuf 
			+ DV_WIDTH * DV_NTSC_HEIGHT + DV_WIDTH / 2;
		unsigned char* img_cb = real_readbuf 
			+ DV_WIDTH * DV_NTSC_HEIGHT;
		pgm_copy_y_block_mmx(bl[0].coeffs, start_y);
		pgm_copy_y_block_mmx(bl[1].coeffs, start_y + 8);
		pgm_copy_y_block_mmx(bl[2].coeffs, start_y + 16);
		pgm_copy_y_block_mmx(bl[3].coeffs, start_y + 24);
		pgm_copy_ntsc_c_block_mmx(bl[4].coeffs,
					  img_cr + y * DV_WIDTH / 2 + x / 2);
		pgm_copy_ntsc_c_block_mmx(bl[5].coeffs,
					  img_cb + y * DV_WIDTH / 2 + x / 2);
	}

	finish_mb_mmx(mb);

	emms();
#endif
}



#if HAVE_DEV_VIDEO
#define ioerror(msg, res) \
        if (res == -1) { \
                perror(msg); \
                return(-1); \
        }

static int vid_in = -1;
static unsigned char * vid_map;
static struct video_mmap gb_frames[VIDEO_MAX_FRAME];
static struct video_mbuf gb_buffers;
static int frame_counter = 0;

static int init_vid_device(const char* filename)
{
	int width = DV_WIDTH;
	int height = DV_PAL_HEIGHT;
	long i;

	vid_in = open(filename, O_RDWR);

	ioerror("open", vid_in);
	
	ioerror("VIDIOCGMBUF", ioctl(vid_in, VIDIOCGMBUF, &gb_buffers));
	vid_map = (unsigned char*) mmap(0, gb_buffers.size, 
					PROT_READ | PROT_WRITE,
					MAP_SHARED, vid_in, 0);
	ioerror("mmap", (long) vid_map);

	fprintf(stderr, "encodedv-capture: found %d buffers\n", 
		gb_buffers.frames);

	for (i = 0; i < gb_buffers.frames; i++) {
		gb_frames[i].frame = i;
		gb_frames[i].width = width;
		gb_frames[i].height = height;
		gb_frames[i].format = VIDEO_PALETTE_YUV422P;
	}

	for (i = 0; i < gb_buffers.frames; i++) {
		ioerror("VIDIOCMCAPTURE", 
			ioctl(vid_in, VIDIOCMCAPTURE, &gb_frames[i]));
	}
	return 0;
}

static int video_init(int wrong_interlace_, int force_dct_)
{
	wrong_interlace = wrong_interlace_;
	force_dct = force_dct_;

	return 0;
}

static void video_finish()
{
	close(vid_in);
}

static int fnumber = -1;

static int video_load(const char* filename, int * isPAL)
{
	if (vid_in == -1) {
		if (init_vid_device(filename) < 0) {
			return -1;
		}
	} else {
		ioerror("VIDIOCMCAPTURE",
			ioctl(vid_in, VIDIOCMCAPTURE, &gb_frames[fnumber]));
	}

	fnumber = (frame_counter++) % gb_buffers.frames;

	ioerror("VIDIOCSYNC", ioctl(vid_in, VIDIOCSYNC, &gb_frames[fnumber]));
	
	real_readbuf = vid_map + gb_buffers.offsets[fnumber];

	*isPAL = 1;

	return 0;
}

#if !ARCH_X86
static inline short video_get_y(int y, int x)
{
	return (((short) real_readbuf[y * DV_WIDTH + x]) - 128)
		<< DCT_YUV_PRECISION;
}

static inline short video_get_cr_pal(int y, int x)
{
	return ((((short) real_readbuf[DV_PAL_HEIGHT * DV_WIDTH
				      + y * DV_WIDTH + x]) - 128)
		+ (((short) real_readbuf[DV_PAL_HEIGHT * DV_WIDTH
					+ y * DV_WIDTH + DV_WIDTH / 2+ x]) 
		   - 128)) << (DCT_YUV_PRECISION - 1);
}

static inline short video_get_cb_pal(int y, int x)
{
	return ((((short) real_readbuf[DV_PAL_HEIGHT * DV_WIDTH * 3/2
				      + y * DV_WIDTH + x]) - 128)
		+ (((short) real_readbuf[DV_PAL_HEIGHT * DV_WIDTH * 3/2
					+ y * DV_WIDTH + DV_WIDTH / 2+ x]) 
		   - 128)) << (DCT_YUV_PRECISION - 1);
}

static inline short video_get_cr_ntsc(int y, int x)
{
	return ((((short) real_readbuf[DV_NTSC_HEIGHT * DV_WIDTH
				      + y * DV_WIDTH + x]) - 128)
		+ (((short) real_readbuf[DV_NTSC_HEIGHT * DV_WIDTH
					+ y * DV_WIDTH + x + 1]) - 128))
						<< (DCT_YUV_PRECISION - 1);
}

static inline short video_get_cb_ntsc(int y, int x)
{
	return ((((short) real_readbuf[DV_NTSC_HEIGHT * DV_WIDTH*3/2
				      + y * DV_WIDTH + x]) - 128)
		+ (((short) real_readbuf[DV_NTSC_HEIGHT * DV_WIDTH*3/2
					+ y * DV_WIDTH + x + 1]) - 128))
						<< (DCT_YUV_PRECISION - 1);
}


#else
extern void video_copy_y_block_mmx(short * dst, unsigned char * src);
extern void video_copy_pal_c_block_mmx(short * dst, unsigned char * src);
extern void video_copy_ntsc_c_block_mmx(short * dst, unsigned char * src);
#endif


static void video_fill_macroblock(dv_macroblock_t *mb, int isPAL)
{
	int y = mb->y;
	int x = mb->x;
	dv_block_t* bl = mb->b;
#if !ARCH_X86
	if (isPAL || mb->x == DV_WIDTH- 16) { /* PAL or rightmost NTSC block */
		int i,j;
		for (j = 0; j < 8; j++) {
			for (i = 0; i < 8; i++) {
				bl[0].coeffs[8*i + j] = video_get_y(y+j,x+i);
				bl[1].coeffs[8*i + j] = video_get_y(y+j,x+8+i);
				bl[2].coeffs[8*i + j]=video_get_y(y+8+j,x+i);
				bl[3].coeffs[8*i + j]=video_get_y(y+8+j,x+8+i);
				bl[4].coeffs[8*i + j] = 
					video_get_cr_pal(y/2+j, x/2+i);
				bl[5].coeffs[8*i + j] = 
					video_get_cb_pal(y/2+j, x/2+i);
			}
		}
	} else {                        /* NTSC */
		int i,j;
		for (i = 0; i < 8; i++) {
			for (j = 0; j < 8; j++) {
				bl[0].coeffs[8*i + j] = video_get_y(y+j,x+i);
				bl[1].coeffs[8*i + j] = video_get_y(y+j,x+8+i);
				bl[2].coeffs[8*i + j]= video_get_y(y+j,x+16+i);
				bl[3].coeffs[8*i + j]= video_get_y(y+j,x+24+i);
				bl[4].coeffs[8*i + j] = 
					video_get_cr_ntsc(y/2+j, x/2+i);
				bl[5].coeffs[8*i + j] = 
					video_get_cb_ntsc(y/2+j, x/2+i);
			}
		}
	}
	if (force_dct != -1) {
		int b;
		for (b = 0; b < 6; b++) {
			bl[b].dct_mode = force_dct;
		}
	} else {
		int b;
		for (b = 0; b < 6; b++) {
			bl[b].dct_mode = need_dct_248_transposed(bl[b].coeffs) 
				? DV_DCT_248 : DV_DCT_88;
		}
	}
#else
	if (isPAL || mb->x == DV_WIDTH- 16) { /* PAL or rightmost NTSC block */
		unsigned char* start_y = real_readbuf + y * DV_WIDTH + x;
		unsigned char* img_cr = real_readbuf 
			+ (isPAL ? DV_WIDTH * DV_PAL_HEIGHT * 3/2  
			   : DV_WIDTH * DV_NTSC_HEIGHT * 3/2);
		unsigned char* img_cb = real_readbuf 
			+ (isPAL ? DV_WIDTH * DV_PAL_HEIGHT
			   : DV_WIDTH * DV_NTSC_HEIGHT);

		video_copy_y_block_mmx(bl[0].coeffs, start_y);
		video_copy_y_block_mmx(bl[1].coeffs, start_y + 8);
		video_copy_y_block_mmx(bl[2].coeffs, start_y + 8 * DV_WIDTH);
		video_copy_y_block_mmx(bl[3].coeffs, start_y + 8 * DV_WIDTH+8);
		video_copy_pal_c_block_mmx(bl[4].coeffs,
					 img_cr + y * DV_WIDTH / 2 + x / 2);
		video_copy_pal_c_block_mmx(bl[5].coeffs,
					 img_cb + y * DV_WIDTH / 2 + x / 2);
	} else {                              /* NTSC */
		unsigned char* start_y = real_readbuf + y * DV_WIDTH + x;
		unsigned char* img_cr = real_readbuf 
			+ DV_WIDTH * DV_NTSC_HEIGHT * 3 / 2;
		unsigned char* img_cb = real_readbuf 
			+ DV_WIDTH * DV_NTSC_HEIGHT;
		video_copy_y_block_mmx(bl[0].coeffs, start_y);
		video_copy_y_block_mmx(bl[1].coeffs, start_y + 8);
		video_copy_y_block_mmx(bl[2].coeffs, start_y + 16);
		video_copy_y_block_mmx(bl[3].coeffs, start_y + 24);
		video_copy_ntsc_c_block_mmx(bl[4].coeffs,
					  img_cr + y * DV_WIDTH / 2 + x / 2);
		video_copy_ntsc_c_block_mmx(bl[5].coeffs,
					  img_cb + y * DV_WIDTH / 2 + x / 2);
	}

	finish_mb_mmx(mb);

	emms();
#endif
}

#endif


static dv_enc_input_filter_t filters[DV_ENC_MAX_INPUT_FILTERS] = {
	{ ppm_init, ppm_finish, ppm_load, ppm_skip, 
	  ppm_fill_macroblock, "ppm" },
#if HAVE_DEV_VIDEO
	{ video_init, video_finish, video_load, video_load, 
	  video_fill_macroblock,"video" },
#endif
	{ pgm_init, pgm_finish, pgm_load, pgm_skip, 
	  pgm_fill_macroblock, "pgm" },
	{ NULL, NULL, NULL, NULL }};

void dv_enc_register_input_filter(dv_enc_input_filter_t filter)
{
	dv_enc_input_filter_t * p = filters;
	while (p->filter_name) p++;
	*p++ = filter;
	p->filter_name = NULL;
}

int get_dv_enc_input_filters(dv_enc_input_filter_t ** filters_,
			     int * count)
{
	dv_enc_input_filter_t * p = filters;

	*count = 0;
	while (p->filter_name) p++, (*count)++;

	*filters_ = filters;
	return 0;
}
