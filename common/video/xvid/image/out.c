/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	ouput colorspace conversions
 *
 *	This program is an implementation of a part of one or more MPEG-4
 *	Video tools as specified in ISO/IEC 14496-2 standard.  Those intending
 *	to use this software module in hardware or software products are
 *	advised that its use may infringe existing patents or copyrights, and
 *	any such use would be at such party's own risk.  The original
 *	developer of this software module and his/her company, and subsequent
 *	editors and their companies, will have no liability for use of this
 *	software or modifications or derivatives thereof.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *************************************************************************/

/**************************************************************************
 *
 *	History:
 *
 *	30.02.2002	out_yuv dst_stride2 fix
 *	26.02.2002	rgb555, rgb565
 *
 **************************************************************************/


#include <string.h>   // memcpy
#include "../portab.h"
#include "out.h"

// function pointers
color_outFuncPtr yuv_to_rgb555;
color_outFuncPtr yuv_to_rgb565;
color_outFuncPtr yuv_to_rgb24;
color_outFuncPtr yuv_to_rgb32;
color_outFuncPtr out_yuv;
color_outFuncPtr yuv_to_yuyv;
color_outFuncPtr yuv_to_uyvy;

#define RGB_Y	1.164
#define B_U		2.018
#define Y_ADD	16

#define G_U		0.391
#define G_V		0.813
#define U_ADD	128

#define R_V		1.596
#define V_ADD	128


#define SCALEBITS	13
#define FIX(x)		((uint16_t) ((x) * (1<<SCALEBITS) + 0.5))


#define MIN(A,B)	((A)<(B)?(A):(B))
#define MAX(A,B)	((A)>(B)?(A):(B))


int32_t RGB_Y_tab[256];
int32_t B_U_tab[256];
int32_t G_U_tab[256];
int32_t G_V_tab[256];
int32_t R_V_tab[256];


/* initialize rgb lookup tables */

void init_yuv_to_rgb(void) {
	int32_t i;

    for(i = 0; i < 256; i++) {
		RGB_Y_tab[i] = FIX(RGB_Y) * (i - Y_ADD);
		B_U_tab[i] = FIX(B_U) * (i - U_ADD);
		G_U_tab[i] = FIX(G_U) * (i - U_ADD);
		G_V_tab[i] = FIX(G_V) * (i - V_ADD);
		R_V_tab[i] = FIX(R_V) * (i - V_ADD);
	}
}



/* yuv 4:2:0 planar -> rgb555 + very simple error diffusion
*/

#define MK_RGB555(R,G,B)	((MAX(0,MIN(255, R)) << 7) & 0x7c00) | \
							((MAX(0,MIN(255, G)) << 2) & 0x03e0) | \
							((MAX(0,MIN(255, B)) >> 3) & 0x001f)


void yuv_to_rgb555_c(uint8_t *dst, int dst_stride,
				 uint8_t *y_src, uint8_t *u_src, uint8_t * v_src, 
				 int y_stride, int uv_stride,
				 int width, int height)
{
	const uint32_t dst_dif = 4 * dst_stride - 2 * width;
	int32_t y_dif = 2 * y_stride - width;
	
	uint8_t *dst2 = dst + 2 * dst_stride;
	uint8_t *y_src2 = y_src + y_stride;
	uint32_t x, y;
	
	if (height < 0) {
		height = -height;
		y_src += (height - 1) * y_stride;
		y_src2 = y_src - y_stride;
		u_src += (height / 2 - 1) * uv_stride;
		v_src += (height / 2 - 1) * uv_stride;
		y_dif = -width - 2 * y_stride;
		uv_stride = -uv_stride;
	}

	for (y = height / 2; y; y--) 
	{
		int r, g, b;
		int r2, g2, b2;

		r = g = b = 0;
		r2 = g2 = b2 = 0;

		// process one 2x2 block per iteration
		for (x = 0; x < (uint32_t)width / 2; x++)
		{
			int u, v;
			int b_u, g_uv, r_v, rgb_y;
			
			u = u_src[x];
			v = v_src[x];

			b_u = B_U_tab[u];
			g_uv = G_U_tab[u] + G_V_tab[v];
			r_v = R_V_tab[v];

			rgb_y = RGB_Y_tab[*y_src];
			b = (b & 0x7) + ((rgb_y + b_u) >> SCALEBITS);
			g = (g & 0x7) + ((rgb_y - g_uv) >> SCALEBITS);
			r = (r & 0x7) + ((rgb_y + r_v) >> SCALEBITS);
			*(uint16_t*)dst = MK_RGB555(r,g,b);

			y_src++;
			rgb_y = RGB_Y_tab[*y_src];
			b = (b & 0x7) + ((rgb_y + b_u) >> SCALEBITS);
			g = (g & 0x7) + ((rgb_y - g_uv) >> SCALEBITS);
			r = (r & 0x7) + ((rgb_y + r_v) >> SCALEBITS);
			*(uint16_t*)(dst+2) = MK_RGB555(r,g,b);
			y_src++;

			rgb_y = RGB_Y_tab[*y_src2];
			b2 = (b2 & 0x7) + ((rgb_y + b_u) >> SCALEBITS);
			g2 = (g2 & 0x7) + ((rgb_y - g_uv) >> SCALEBITS);
			r2 = (r2 & 0x7) + ((rgb_y + r_v) >> SCALEBITS);
			*(uint16_t*)(dst2) = MK_RGB555(r2,g2,b2);
			y_src2++;

			rgb_y = RGB_Y_tab[*y_src2];
			b2 = (b2 & 0x7) + ((rgb_y + b_u) >> SCALEBITS);
			g2 = (g2 & 0x7) + ((rgb_y - g_uv) >> SCALEBITS);
			r2 = (r2 & 0x7) + ((rgb_y + r_v) >> SCALEBITS);
			*(uint16_t*)(dst2+2) = MK_RGB555(r2,g2,b2);
			y_src2++;

			dst += 4;
			dst2 += 4;
		}

		dst += dst_dif;
		dst2 += dst_dif;

		y_src += y_dif;
		y_src2 += y_dif;

		u_src += uv_stride;
		v_src += uv_stride;
	}
}


/* yuv 4:2:0 planar -> rgb565 + very simple error diffusion
	NOTE:	identical to rgb555 except for shift/mask  */


#define MK_RGB565(R,G,B)	((MAX(0,MIN(255, R)) << 8) & 0xf800) | \
							((MAX(0,MIN(255, G)) << 3) & 0x07e0) | \
							((MAX(0,MIN(255, B)) >> 3) & 0x001f)

void yuv_to_rgb565_c(uint8_t *dst, int dst_stride,
				 uint8_t *y_src, uint8_t *u_src, uint8_t * v_src, 
				 int y_stride, int uv_stride,
				 int width, int height)
{
	const uint32_t dst_dif = 4 * dst_stride - 2 * width;
	int32_t y_dif = 2 * y_stride - width;
	
	uint8_t *dst2 = dst + 2 * dst_stride;
	uint8_t *y_src2 = y_src + y_stride;
	uint32_t x, y;
	
	if (height < 0) { // flip image?
		height = -height;
		y_src += (height - 1) * y_stride;
		y_src2 = y_src - y_stride;
		u_src += (height / 2 - 1) * uv_stride;
		v_src += (height / 2 - 1) * uv_stride;
		y_dif = -width - 2 * y_stride;
		uv_stride = -uv_stride;
	}

	for (y = height / 2; y; y--) 
	{
		int r, g, b;
		int r2, g2, b2;

		r = g = b = 0;
		r2 = g2 = b2 = 0;

		// process one 2x2 block per iteration
		for (x = 0; x < (uint32_t)width / 2; x++)
		{
			int u, v;
			int b_u, g_uv, r_v, rgb_y;
			
			u = u_src[x];
			v = v_src[x];

			b_u = B_U_tab[u];
			g_uv = G_U_tab[u] + G_V_tab[v];
			r_v = R_V_tab[v];

			rgb_y = RGB_Y_tab[*y_src];
			b = (b & 0x7) + ((rgb_y + b_u) >> SCALEBITS);
			g = (g & 0x7) + ((rgb_y - g_uv) >> SCALEBITS);
			r = (r & 0x7) + ((rgb_y + r_v) >> SCALEBITS);
			*(uint16_t*)dst = MK_RGB565(r,g,b);

			y_src++;
			rgb_y = RGB_Y_tab[*y_src];
			b = (b & 0x7) + ((rgb_y + b_u) >> SCALEBITS);
			g = (g & 0x7) + ((rgb_y - g_uv) >> SCALEBITS);
			r = (r & 0x7) + ((rgb_y + r_v) >> SCALEBITS);
			*(uint16_t*)(dst+2) = MK_RGB565(r,g,b);
			y_src++;

			rgb_y = RGB_Y_tab[*y_src2];
			b2 = (b2 & 0x7) + ((rgb_y + b_u) >> SCALEBITS);
			g2 = (g2 & 0x7) + ((rgb_y - g_uv) >> SCALEBITS);
			r2 = (r2 & 0x7) + ((rgb_y + r_v) >> SCALEBITS);
			*(uint16_t*)(dst2) = MK_RGB565(r2,g2,b2);
			y_src2++;

			rgb_y = RGB_Y_tab[*y_src2];
			b2 = (b2 & 0x7) + ((rgb_y + b_u) >> SCALEBITS);
			g2 = (g2 & 0x7) + ((rgb_y - g_uv) >> SCALEBITS);
			r2 = (r2 & 0x7) + ((rgb_y + r_v) >> SCALEBITS);
			*(uint16_t*)(dst2+2) = MK_RGB565(r2,g2,b2);
			y_src2++;

			dst += 4;
			dst2 += 4;
		}

		dst += dst_dif;
		dst2 += dst_dif;

		y_src += y_dif;
		y_src2 += y_dif;

		u_src += uv_stride;
		v_src += uv_stride;
	}
}


	
/* yuv 4:2:0 planar -> rgb24 */

void yuv_to_rgb24_c(uint8_t *dst, int dst_stride,
				 uint8_t *y_src, uint8_t *u_src, uint8_t * v_src, 
				 int y_stride, int uv_stride,
				 int width, int height)
{
	const uint32_t dst_dif = 6 * dst_stride - 3 * width;
	int32_t y_dif = 2 * y_stride - width;
	
	uint8_t *dst2 = dst + 3 * dst_stride;
	uint8_t *y_src2 = y_src + y_stride;
	uint32_t x, y;
	
	if (height < 0) { // flip image?
		height = -height;
		y_src += (height - 1) * y_stride;
		y_src2 = y_src - y_stride;
		u_src += (height / 2 - 1) * uv_stride;
		v_src += (height / 2 - 1) * uv_stride;
		y_dif = -width - 2 * y_stride;
		uv_stride = -uv_stride;
	}

	for (y = height / 2; y; y--) 
	{
		// process one 2x2 block per iteration
		for (x = 0; x < (uint32_t)width / 2; x++)
		{
			int u, v;
			int b_u, g_uv, r_v, rgb_y;
			int r, g, b;

			u = u_src[x];
			v = v_src[x];

			b_u = B_U_tab[u];
			g_uv = G_U_tab[u] + G_V_tab[v];
			r_v = R_V_tab[v];

			rgb_y = RGB_Y_tab[*y_src];
			b = (rgb_y + b_u) >> SCALEBITS;
			g = (rgb_y - g_uv) >> SCALEBITS;
			r = (rgb_y + r_v) >> SCALEBITS;
			dst[0] = MAX(0,MIN(255, b));
			dst[1] = MAX(0,MIN(255, g));
			dst[2] = MAX(0,MIN(255, r));

			y_src++;
			rgb_y = RGB_Y_tab[*y_src];
			b = (rgb_y + b_u) >> SCALEBITS;
			g = (rgb_y - g_uv) >> SCALEBITS;
			r = (rgb_y + r_v) >> SCALEBITS;
			dst[3] = MAX(0,MIN(255, b));
			dst[4] = MAX(0,MIN(255, g));
			dst[5] = MAX(0,MIN(255, r));
			y_src++;

			rgb_y = RGB_Y_tab[*y_src2];
			b = (rgb_y + b_u) >> SCALEBITS;
			g = (rgb_y - g_uv) >> SCALEBITS;
			r = (rgb_y + r_v) >> SCALEBITS;
			dst2[0] = MAX(0,MIN(255, b));
			dst2[1] = MAX(0,MIN(255, g));
			dst2[2] = MAX(0,MIN(255, r));
			y_src2++;

			rgb_y = RGB_Y_tab[*y_src2];
			b = (rgb_y + b_u) >> SCALEBITS;
			g = (rgb_y - g_uv) >> SCALEBITS;
			r = (rgb_y + r_v) >> SCALEBITS;
			dst2[3] = MAX(0,MIN(255, b));
			dst2[4] = MAX(0,MIN(255, g));
			dst2[5] = MAX(0,MIN(255, r));
			y_src2++;

			dst += 6;
			dst2 += 6;
		}

		dst += dst_dif;
		dst2 += dst_dif;

		y_src += y_dif;
		y_src2 += y_dif;

		u_src += uv_stride;
		v_src += uv_stride;
	}
}



/* yuv 4:2:0 planar -> rgb32 */

void yuv_to_rgb32_c(uint8_t *dst, int dst_stride,
				 uint8_t *y_src, uint8_t *v_src, uint8_t * u_src,
				 int y_stride, int uv_stride,
				 int width, int height)
{
	const uint32_t dst_dif = 8 * dst_stride - 4 * width;
	int32_t y_dif = 2 * y_stride - width;
	
	uint8_t *dst2 = dst + 4 * dst_stride;
	uint8_t *y_src2 = y_src + y_stride;
	uint32_t x, y;
	
	if (height < 0) { // flip image?
		height = -height;
		y_src += (height - 1) * y_stride;
		y_src2 = y_src - y_stride;
		u_src += (height / 2 - 1) * uv_stride;
		v_src += (height / 2 - 1) * uv_stride;
		y_dif = -width - 2 * y_stride;
		uv_stride = -uv_stride;
	}

	for (y = height / 2; y; y--) 
	{
		// process one 2x2 block per iteration
		for (x = 0; x < (uint32_t)width / 2; x++)
		{
			int u, v;
			int b_u, g_uv, r_v, rgb_y;
			int r, g, b;

			u = u_src[x];
			v = v_src[x];

			b_u = B_U_tab[u];
			g_uv = G_U_tab[u] + G_V_tab[v];
			r_v = R_V_tab[v];

			rgb_y = RGB_Y_tab[*y_src];
			b = (rgb_y + b_u) >> SCALEBITS;
			g = (rgb_y - g_uv) >> SCALEBITS;
			r = (rgb_y + r_v) >> SCALEBITS;
			dst[0] = MAX(0,MIN(255, r));
			dst[1] = MAX(0,MIN(255, g));
			dst[2] = MAX(0,MIN(255, b));
			dst[3] = 0;

			y_src++;
			rgb_y = RGB_Y_tab[*y_src];
			b = (rgb_y + b_u) >> SCALEBITS;
			g = (rgb_y - g_uv) >> SCALEBITS;
			r = (rgb_y + r_v) >> SCALEBITS;
			dst[4] = MAX(0,MIN(255, r));
			dst[5] = MAX(0,MIN(255, g));
			dst[6] = MAX(0,MIN(255, b));
			dst[7] = 0;
			y_src++;

			rgb_y = RGB_Y_tab[*y_src2];
			b = (rgb_y + b_u) >> SCALEBITS;
			g = (rgb_y - g_uv) >> SCALEBITS;
			r = (rgb_y + r_v) >> SCALEBITS;
			dst2[0] = MAX(0,MIN(255, r));
			dst2[1] = MAX(0,MIN(255, g));
			dst2[2] = MAX(0,MIN(255, b));
			dst2[3] = 0;
			y_src2++;

			rgb_y = RGB_Y_tab[*y_src2];
			b = (rgb_y + b_u) >> SCALEBITS;
			g = (rgb_y - g_uv) >> SCALEBITS;
			r = (rgb_y + r_v) >> SCALEBITS;
			dst2[4] = MAX(0,MIN(255, r));
			dst2[5] = MAX(0,MIN(255, g));
			dst2[6] = MAX(0,MIN(255, b));
			dst2[7] = 0;
			y_src2++;

			dst += 8;
			dst2 += 8;
		}

		dst += dst_dif;
		dst2 += dst_dif;

		y_src += y_dif;
		y_src2 += y_dif;

		u_src += uv_stride;
		v_src += uv_stride;
	}
}



/*	yuv 4:2:0 planar -> yuv planar */

// Note: untested!! //
void out_yuv_c(uint8_t *dst, int dst_stride,
				 uint8_t *y_src, uint8_t *u_src, uint8_t * v_src, 
				 int y_stride, int uv_stride,
				 int width, int height)
{
	uint32_t dst_stride2 = dst_stride >> 1;
	uint32_t width2 = width >> 1;
    uint32_t y;

	if (height < 0) {
		height = -height;
		y_src += (height - 1) * y_stride ;
		u_src += (height / 2 - 1) * uv_stride;
		v_src += (height / 2 - 1) * uv_stride;
		y_stride = -y_stride;
		uv_stride = -uv_stride;
	}

	for (y = height; y; y--)	{
	    memcpy(dst, y_src, width);
	    dst += dst_stride;
		y_src += y_stride;
	}

	for (y = height >> 1; y; y--) {
	    memcpy(dst, u_src, width2);
		dst += dst_stride2;
		u_src += uv_stride;
	}

	for (y = height >> 1; y; y--) {
	    memcpy(dst, v_src, width2);
		dst += dst_stride2;
		v_src += uv_stride;
	}
}




/* yuv 4:2:0 planar -> yuyv (yuv2) packed */

void yuv_to_yuyv_c(uint8_t *dst, int dst_stride,
				 uint8_t *y_src, uint8_t *u_src, uint8_t * v_src, 
				 int y_stride, int uv_stride,
				 int width, int height)
{
	const uint32_t dst_dif = 2*(dst_stride - width);
	uint32_t x, y;

	if (height < 0) {
		height = -height;
		y_src += (height   - 1) * y_stride ;
		u_src += (height / 2 - 1) * uv_stride;
		v_src += (height / 2 - 1) * uv_stride;
		y_stride = -y_stride;
		uv_stride = -uv_stride;
	}

	for (y = 0; y < (uint32_t)height; y++)
	{
		for (x = 0; x < (uint32_t)width / 2; x++)
		{
			dst[0] = y_src[2*x];
			dst[1] = u_src[x];
			dst[2] = y_src[2*x + 1];
			dst[3] = v_src[x];
			dst += 4;
		}
		dst += dst_dif;
		y_src += y_stride;
		if (y&1)
		{
			u_src += uv_stride;
			v_src += uv_stride;
		}
	}
}



/* yuv 4:2:0 planar -> uyvy packed */

void yuv_to_uyvy_c(uint8_t *dst, int dst_stride,
				 uint8_t *y_src, uint8_t *u_src, uint8_t * v_src, 
				 int y_stride, int uv_stride,
				 int width, int height)
{
	const uint32_t dst_dif = 2*(dst_stride - width);
	uint32_t x, y;

	if (height < 0) {
		height = -height;
		y_src += (height   - 1) * y_stride ;
		u_src += (height / 2 - 1) * uv_stride;
		v_src += (height / 2 - 1) * uv_stride;
		y_stride = -y_stride;
		uv_stride = -uv_stride;
	}

	for (y = 0; y < (uint32_t)height; y++)
	{
		for (x = 0; x < (uint32_t)width / 2; x++)
		{
			dst[0] = u_src[x];
			dst[1] = y_src[2*x];
			dst[2] = v_src[x];
			dst[3] = y_src[2*x + 1];
			dst += 4;
		}
		dst += dst_dif;
		y_src += y_stride;
		if (y&1)
		{
			u_src += uv_stride;
			v_src += uv_stride;
		}
	}
}
