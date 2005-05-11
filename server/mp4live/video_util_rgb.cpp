
/**************************************************************************
 *                                                                        *
 * This code is developed by Adam Li.  This software is an                *
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
 * http://www.projectmayo.com/opendivx/license.php .                      *
 *                                                                        *
 **************************************************************************/

/**************************************************************************
 *
 *  rgb2yuv.c, 24-bit RGB bitmap to YUV converter
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Adam Li
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/

/* This file contains RGB to YUV transformation functions.                */

#include "mp4live.h"
#include "video_util_rgb.h"

static float RGBYUV02990[256], RGBYUV05870[256], RGBYUV01140[256];
static float RGBYUV01684[256], RGBYUV03316[256];
static float RGBYUV04187[256], RGBYUV00813[256];

void InitLookupTable();

/************************************************************************
 *
 *  int RGB2YUV (int x_dim, int y_dim, void *bmp, YUV *yuv)
 *
 *	Purpose :	It takes a 24-bit RGB bitmap and convert it into
 *				YUV (4:2:0) format
 *
 *  Input :		x_dim	the x dimension of the bitmap
 *				y_dim	the y dimension of the bitmap
 *				bmp		pointer to the buffer of the bitmap
 *				yuv		pointer to the YUV structure
 *
 *  Output :	0		OK
 *				1		wrong dimension
 *				2		memory allocation error
 *
 *	Side Effect :
 *				None
 *
 *	Date :		09/28/2000
 *
 *  Contacts:
 *
 *  Adam Li
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 ************************************************************************/

int RGB2YUV (int x_dim, int y_dim, void *bmp, void *y_out, void *u_out, void *v_out, int flip, bool have_rgb)
{
	static int init_done = 0;

	long i, j, size;
	uint8_t *offset;
	uint8_t *r, *g, *b;
	uint8_t *y, *u, *v;
	uint8_t *pu1, *pu2, *pv1, *pv2, *psu, *psv;
	uint8_t *y_buffer, *u_buffer, *v_buffer;
	uint8_t *sub_u_buf, *sub_v_buf;
	uint r_offset, b_offset, g_offset;

	if (init_done == 0)
	{
		InitLookupTable();
		init_done = 1;
	}

	// check to see if x_dim and y_dim are divisible by 2
	if ((x_dim % 2) || (y_dim % 2)) return 1;
	size = x_dim * y_dim;

	// allocate memory
	y_buffer = (uint8_t *)y_out;
	sub_u_buf = (uint8_t *)u_out;
	sub_v_buf = (uint8_t *)v_out;
	u_buffer = (uint8_t *)malloc(size * sizeof(uint8_t));
	v_buffer = (uint8_t *)malloc(size * sizeof(uint8_t));
	if (!(u_buffer && v_buffer))
	{
		if (u_buffer) free(u_buffer);
		if (v_buffer) free(v_buffer);
		return 2;
	}

	offset = (uint8_t *)bmp;
	y = y_buffer;
	u = u_buffer;
	v = v_buffer;
	if (have_rgb) {
	  r_offset = 0;
	  g_offset = 1;
	  b_offset = 2;
	} else {
	  b_offset = 0;
	  g_offset = 1;
	  r_offset = 2;
	}
	// convert RGB to YUV
	if (!flip) {
		for (j = 0; j < y_dim; j ++)
		{
			y = y_buffer + (y_dim - j - 1) * x_dim;
			u = u_buffer + (y_dim - j - 1) * x_dim;
			v = v_buffer + (y_dim - j - 1) * x_dim;

			for (i = 0; i < x_dim; i ++) {
			  b = offset + b_offset;
			  g = offset + g_offset;
			  r = offset + r_offset;
				*y = (uint8_t)(  RGBYUV02990[*r] + RGBYUV05870[*g] + RGBYUV01140[*b]);
				*u = (uint8_t)(- RGBYUV01684[*r] - RGBYUV03316[*g] + (*b)/2          + 128);
				*v = (uint8_t)(  (*r)/2          - RGBYUV04187[*g] - RGBYUV00813[*b] + 128);
				offset += 3;
				y ++;
				u ++;
				v ++;
			}
		}
	} else {
		for (i = 0; i < size; i++)
		{
		  b = offset + b_offset;
			  g = offset + g_offset;
			  r = offset + r_offset;
			*y = (uint8_t)(  RGBYUV02990[*r] + RGBYUV05870[*g] + RGBYUV01140[*b]);
			*u = (uint8_t)(- RGBYUV01684[*r] - RGBYUV03316[*g] + (*b)/2          + 128);
			*v = (uint8_t)(  (*r)/2          - RGBYUV04187[*g] - RGBYUV00813[*b] + 128);
			offset += 3;
			y ++;
			u ++;
			v ++;
		}
	}

	// subsample UV
	for (j = 0; j < y_dim/2; j ++)
	{
		psu = sub_u_buf + j * x_dim / 2;
		psv = sub_v_buf + j * x_dim / 2;
		pu1 = u_buffer + 2 * j * x_dim;
		pu2 = u_buffer + (2 * j + 1) * x_dim;
		pv1 = v_buffer + 2 * j * x_dim;
		pv2 = v_buffer + (2 * j + 1) * x_dim;
		for (i = 0; i < x_dim/2; i ++)
		{
			*psu = (*pu1 + *(pu1+1) + *pu2 + *(pu2+1)) / 4;
			*psv = (*pv1 + *(pv1+1) + *pv2 + *(pv2+1)) / 4;
			psu ++;
			psv ++;
			pu1 += 2;
			pu2 += 2;
			pv1 += 2;
			pv2 += 2;
		}
	}

	free(u_buffer);
	free(v_buffer);

	return 0;
}


void InitLookupTable()
{
	int i;

	for (i = 0; i < 256; i++) RGBYUV02990[i] = (float)0.2990 * i;
	for (i = 0; i < 256; i++) RGBYUV05870[i] = (float)0.5870 * i;
	for (i = 0; i < 256; i++) RGBYUV01140[i] = (float)0.1140 * i;
	for (i = 0; i < 256; i++) RGBYUV01684[i] = (float)0.1684 * i;
	for (i = 0; i < 256; i++) RGBYUV03316[i] = (float)0.3316 * i;
	for (i = 0; i < 256; i++) RGBYUV04187[i] = (float)0.4187 * i;
	for (i = 0; i < 256; i++) RGBYUV00813[i] = (float)0.0813 * i;
}


