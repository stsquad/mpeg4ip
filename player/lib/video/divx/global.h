/**************************************************************************
 *                                                                        *
 * This code has been developed by Andrea Graziani. This software is an   *
 * implementation of a part of one or more MPEG-4 Video tools as          *
 * specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
 * software module in hardware or software products are advised that its  *
 * use may infringe existing patents or copyrights, and any such use      *
 * would be at such party's own risk.  The original developer of this     *
 * software module and his/her company, and subsequent editors and their  *
 * companies (including Project Mayo), will have no liability for use of  *
 * this software or modifications or derivatives thereof.                 *
 *                                                                        *
 * Project Mayo gives users of the Codec a license to this software       *
 * module or modifications thereof for use in hardware or software        *
 * products claiming conformance to the MPEG-4 Video Standard as          *
 * described in the Open DivX license.                                    *
 *                                                                        *
 * The complete Open DivX license can be found at                         *
 * http://www.projectmayo.com/opendivx/license.php                        *
 *                                                                        *
 **************************************************************************/
/**
*  Copyright (C) 2001 - Project Mayo
 *
 * Andrea Graziani (Ag)
 * < e7abe7a@projectmayo.com >
*
**/
// global.h //

#ifndef __GLOBAL_H__
#define __GLOBAL_H__ 1
/* GLOBAL is defined in only one file */

#include <stdio.h>

#ifndef GLOBAL
#define EXTERN extern
#else
#define EXTERN
#endif

/**
 *	macros 
**/

#define mmax(a, b)        ((a) > (b) ? (a) : (b))
#define mmin(a, b)        ((a) < (b) ? (a) : (b))
#define mnint(a)        ((a) < 0 ? (int)(a - 0.5) : (int)(a + 0.5))
#ifndef sign
#define sign(a)         ((a) < 0 ? -1 : 1)
#endif

/** 
 *	prototypes of global functions
**/

// getbits.c 
typedef unsigned int (*get_t)(void *);
typedef void (*bookmark_t)(void *, int );
void initbits (get_t get, bookmark_t bookmark, void *userdata);
void fillbfr (void);
unsigned int showbits (int n);
unsigned int getbits1 (void);
void flushbits (int n);
unsigned int divx_getbits (unsigned int n);
#define getbits divx_getbits

// idct.c 
void idct (short *block);
void init_idct (void);

// recon.c 
void reconstruct (int bx, int by, int mode);

// mp4_picture.c
void PictureDisplay (unsigned char *bmp, int render_flag);

/** 
 *		global variables 
**/

// zig-zag scan
EXTERN unsigned char zig_zag_scan[64]
#ifdef GLOBAL
=
{
  0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5,
  12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28,
  35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
  58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
}
#endif
;

// other scan orders
EXTERN unsigned char alternate_horizontal_scan[64]
#ifdef GLOBAL
=
{
   0,  1,  2,  3,  8,  9, 16, 17, 
	10, 11,  4,  5,  6,  7, 15, 14,
  13, 12, 19, 18, 24, 25, 32, 33, 
	26, 27, 20, 21, 22, 23, 28, 29,
  30, 31, 34, 35, 40, 41, 48, 49, 
	42, 43, 36, 37, 38, 39, 44, 45,
  46, 47, 50, 51, 56, 57, 58, 59, 
	52, 53, 54, 55, 60, 61, 62, 63
}
#endif
;

EXTERN unsigned char alternate_vertical_scan[64]
#ifdef GLOBAL
=
{
   0,  8, 16, 24,  1,  9,  2, 10, 
	17, 25, 32, 40, 48, 56, 57, 49,
  41, 33, 26, 18,  3, 11,  4, 12, 
	19, 27, 34, 42, 50, 58, 35, 43,
  51, 59, 20, 28,  5, 13,  6, 14, 
	21, 29, 36, 44, 52, 60, 37, 45,
  53, 61, 22, 30,  7, 15, 23, 31, 
	38, 46, 54, 62, 39, 47, 55, 63
}
#endif
;

EXTERN unsigned char *clp;
EXTERN int horizontal_size, vertical_size, mb_width, mb_height;
EXTERN int juice_hor, juice_ver;
EXTERN int coded_picture_width, coded_picture_height;
EXTERN int chrom_width, chrom_height; 

EXTERN int roundtab[16]
#ifdef GLOBAL
= {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2}
#endif
;

/**
 *	mp4 stuff
**/
#include "mp4_decoder.h"
#include "mp4_header.h"

EXTERN int	juice_flag,
  output_flag;
EXTERN char * outputname;
EXTERN int   flag_invert;
EXTERN int	post_flag,
						pp_options;
EXTERN mp4_header mp4_hdr;
EXTERN struct _base
{
  // bit input
  int infile;
  unsigned char rdbfr[2051];
  unsigned char *rdptr;
  unsigned char inbfr[16];
  int incnt;
  int bitcnt;
  // block data
  short block[6][64];
} base, *ld;
EXTERN int MV[2][6][MBR+1][MBC+2];
EXTERN int modemap[MBR+1][MBC+2];
EXTERN int quant_store[MBR+1][MBC+1]; // [Review]
EXTERN struct _ac_dc
{
	int dc_store_lum[2*MBR+1][2*MBC+1];
	int ac_left_lum[2*MBR+1][2*MBC+1][7];
	int ac_top_lum[2*MBR+1][2*MBC+1][7];

	int dc_store_chr[2][MBR+1][MBC+1];
	int ac_left_chr[2][MBR+1][MBC+1][7];
	int ac_top_chr[2][MBR+1][MBC+1][7];

	int predict_dir;

} ac_dc, *coeff_pred;
EXTERN unsigned char	*edged_ref[3],
											*edged_for[3],
											*frame_ref[3],
											*frame_for[3],
											*display_frame[3];
#endif
