/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	colorspace conversions
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
 *	24.11.2001	accuracy improvement to yuyv/vyuy conversion
 *	28.10.2001	total rewrite <pross@cs.rmit.edu.au>
 *
 **************************************************************************/

#include <string.h>		// memcpy

#include "colorspace.h"

// function pointers

/* input */
color_inputFuncPtr rgb555_to_yv12;
color_inputFuncPtr rgb565_to_yv12;
color_inputFuncPtr rgb24_to_yv12;
color_inputFuncPtr rgb32_to_yv12;
color_inputFuncPtr yuv_to_yv12;
color_inputFuncPtr yuyv_to_yv12;
color_inputFuncPtr uyvy_to_yv12;

/* output */
color_outputFuncPtr yv12_to_rgb555;
color_outputFuncPtr yv12_to_rgb565;
color_outputFuncPtr yv12_to_rgb24;
color_outputFuncPtr yv12_to_rgb32;
color_outputFuncPtr yv12_to_yuv;
color_outputFuncPtr yv12_to_yuyv;
color_outputFuncPtr yv12_to_uyvy;

#define MIN(A,B)	((A)<(B)?(A):(B))
#define MAX(A,B)	((A)>(B)?(A):(B))

/*	rgb -> yuv def's

	this following constants are "official spec"
	Video Demystified" (ISBN 1-878707-09-4)

	rgb<->yuv _is_ lossy, since most programs do the conversion differently
		
	SCALEBITS/FIX taken from  ffmpeg
*/

#define Y_R_IN			0.257
#define Y_G_IN			0.504
#define Y_B_IN			0.098
#define Y_ADD_IN		16

#define U_R_IN			0.148
#define U_G_IN			0.291
#define U_B_IN			0.439
#define U_ADD_IN		128

#define V_R_IN			0.439
#define V_G_IN			0.368
#define V_B_IN			0.071
#define V_ADD_IN		128

#define SCALEBITS_IN	8
#define FIX_IN(x)		((uint16_t) ((x) * (1L<<SCALEBITS_IN) + 0.5))


int32_t RGB_Y_tab[256];
int32_t B_U_tab[256];
int32_t G_U_tab[256];
int32_t G_V_tab[256];
int32_t R_V_tab[256];


/* rgb555 -> yuv 4:2:0 planar */
void rgb555_to_yv12_c(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
					uint8_t *src, int width, int height, int y_stride)
{
	int32_t src_stride = width * 2;
	uint32_t y_dif = y_stride - width;
	uint32_t uv_dif = (y_stride - width) / 2;
	uint32_t x, y;

	if (height < 0)
	{
		height = -height;
		src += (height - 1) * src_stride;
		src_stride = -src_stride;
	}

	
	for (y = height / 2; y; y--) 
	{
		// process one 2x2 block per iteration
		for (x = 0; x < (uint32_t)width; x += 2)
		{
			int rgb, r, g, b, r4, g4, b4;

			rgb = *(uint16_t*)(src+x*2);
			b4 = b = (rgb << 3) & 0xf8;
			g4 = g = (rgb >> 2) & 0xf8;
			r4 = r = (rgb >> 7) & 0xf8;
            y_out[0] =(uint8_t)((
						  FIX_IN(Y_R_IN) * r 
						+ FIX_IN(Y_G_IN) * g 
						+ FIX_IN(Y_B_IN) * b) >> SCALEBITS_IN) + Y_ADD_IN;

			rgb = *(uint16_t*)(src+x*2+src_stride);
			b4 += b = (rgb << 3) & 0xf8;
			g4 += g = (rgb >> 2) & 0xf8;
			r4 += r = (rgb >> 7) & 0xf8;
            y_out[y_stride] =(uint8_t)((
						  FIX_IN(Y_R_IN) * r 
						+ FIX_IN(Y_G_IN) * g 
						+ FIX_IN(Y_B_IN) * b) >> SCALEBITS_IN) + Y_ADD_IN;

			rgb = *(uint16_t*)(src+x*2+2);
			b4 += b = (rgb << 3) & 0xf8;
			g4 += g = (rgb >> 2) & 0xf8;
			r4 += r = (rgb >> 7) & 0xf8;
            y_out[1] =(uint8_t)((
						  FIX_IN(Y_R_IN) * r 
						+ FIX_IN(Y_G_IN) * g 
						+ FIX_IN(Y_B_IN) * b) >> SCALEBITS_IN) + Y_ADD_IN;

			rgb = *(uint16_t*)(src+x*2+src_stride+2);
			b4 += b = (rgb << 3) & 0xf8;
			g4 += g = (rgb >> 2) & 0xf8;
			r4 += r = (rgb >> 7) & 0xf8;
            y_out[y_stride + 1] =(uint8_t)((
						  FIX_IN(Y_R_IN) * r 
						+ FIX_IN(Y_G_IN) * g 
						+ FIX_IN(Y_B_IN) * b) >> SCALEBITS_IN) + Y_ADD_IN;

			*u_out++ = (uint8_t)((
						- FIX_IN(U_R_IN) * r4 
						- FIX_IN(U_G_IN) * g4
						+ FIX_IN(U_B_IN) * b4) >> (SCALEBITS_IN + 2)) + U_ADD_IN;
			
			
            *v_out++ = (uint8_t)((
						  FIX_IN(V_R_IN) * r4
						- FIX_IN(V_G_IN) * g4
						- FIX_IN(V_B_IN) * b4) >> (SCALEBITS_IN + 2)) + V_ADD_IN; 

			y_out += 2;
		}
		src += src_stride * 2;
		y_out += y_dif + y_stride;
		u_out += uv_dif;
		v_out += uv_dif;
	}
}



/* rgb565_to_yuv_c
	NOTE:	identical to rgb555 except for shift/mask 
			not tested */

void rgb565_to_yv12_c(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
					uint8_t *src, int width, int height, int y_stride)
{
	int32_t src_stride = width * 2;

	uint32_t y_dif = y_stride - width;
	uint32_t uv_dif = (y_stride - width) / 2;
	uint32_t x, y;

	if (height < 0)
	{
		height = -height;
		src += (height - 1) * src_stride;
		src_stride = -src_stride;
	}

	
	for (y = height / 2; y; y--) 
	{
		// process one 2x2 block per iteration
		for (x = 0; x < (uint32_t)width; x += 2)
		{
			int rgb, r, g, b, r4, g4, b4;

			rgb = *(uint16_t*)(src+x*2);
			b4 = b = (rgb << 3) & 0xf8;
			g4 = g = (rgb >> 3) & 0xfc;
			r4 = r = (rgb >> 8) & 0xf8;
            y_out[0] =(uint8_t)((
						  FIX_IN(Y_R_IN) * r 
						+ FIX_IN(Y_G_IN) * g 
						+ FIX_IN(Y_B_IN) * b) >> SCALEBITS_IN) + Y_ADD_IN;

			rgb = *(uint16_t*)(src+x*2+src_stride);
			b4 += b = (rgb << 3) & 0xf8;
			g4 += g = (rgb >> 3) & 0xfc;
			r4 += r = (rgb >> 8) & 0xf8;
            y_out[y_stride] =(uint8_t)((
						  FIX_IN(Y_R_IN) * r 
						+ FIX_IN(Y_G_IN) * g 
						+ FIX_IN(Y_B_IN) * b) >> SCALEBITS_IN) + Y_ADD_IN;

			rgb = *(uint16_t*)(src+x*2+2);
			b4 += b = (rgb << 3) & 0xf8;
			g4 += g = (rgb >> 3) & 0xfc;
			r4 += r = (rgb >> 8) & 0xf8;
            y_out[1] =(uint8_t)((
						  FIX_IN(Y_R_IN) * r 
						+ FIX_IN(Y_G_IN) * g 
						+ FIX_IN(Y_B_IN) * b) >> SCALEBITS_IN) + Y_ADD_IN;

			rgb = *(uint16_t*)(src+x*2+src_stride+2);
			b4 += b = (rgb << 3) & 0xf8;
			g4 += g = (rgb >> 3) & 0xfc;
			r4 += r = (rgb >> 8) & 0xf8;
            y_out[y_stride + 1] =(uint8_t)((
						  FIX_IN(Y_R_IN) * r 
						+ FIX_IN(Y_G_IN) * g 
						+ FIX_IN(Y_B_IN) * b) >> SCALEBITS_IN) + Y_ADD_IN;

			*u_out++ = (uint8_t)((
						- FIX_IN(U_R_IN) * r4 
						- FIX_IN(U_G_IN) * g4
						+ FIX_IN(U_B_IN) * b4) >> (SCALEBITS_IN + 2)) + U_ADD_IN;
			
			
            *v_out++ = (uint8_t)((
						  FIX_IN(V_R_IN) * r4
						- FIX_IN(V_G_IN) * g4
						- FIX_IN(V_B_IN) * b4) >> (SCALEBITS_IN + 2)) + V_ADD_IN; 

			y_out += 2;
		}
		src += src_stride * 2;
		y_out += y_dif + y_stride;
		u_out += uv_dif;
		v_out += uv_dif;
	}
}




/*	rgb24 -> yuv 4:2:0 planar 

	NOTE: always flips.
*/

void rgb24_to_yv12_c(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
					uint8_t *src, int width, int height, int stride)
{
    uint32_t width3 = (width << 1) + width;		/* width * 3 */
	uint32_t src_dif = (width << 3) + width;		/* width3 * 3 */
	uint32_t y_dif = (stride << 1) - width;
	uint32_t uv_dif = (stride - width) >> 1;
    uint32_t x, y;

	src += (height - 2) * width3;
	
	
    for(y = height >> 1; y; y--) {
		for(x = width >> 1; x; x--) {
			uint32_t r, g, b, r4, g4, b4;

            b4 = b = src[0];
            g4 = g = src[1];
            r4 = r = src[2];
            y_out[stride + 0] =(uint8_t)((
						  FIX_IN(Y_R_IN) * r 
						+ FIX_IN(Y_G_IN) * g 
						+ FIX_IN(Y_B_IN) * b) >> SCALEBITS_IN) + Y_ADD_IN; 

            b4 += (b = src[3]);
            g4 += (g = src[4]);
            r4 += (r = src[5]);
            y_out[stride + 1] = (uint8_t)((
						  FIX_IN(Y_R_IN) * r 
						+ FIX_IN(Y_G_IN) * g 
						+ FIX_IN(Y_B_IN) * b) >> SCALEBITS_IN) + Y_ADD_IN;

            b4 += (b = src[width3 + 0]);
            g4 += (g = src[width3 + 1]);
            r4 += (r = src[width3 + 2]);
            y_out[0] = (uint8_t)((
						FIX_IN(Y_R_IN) * r + 
						FIX_IN(Y_G_IN) * g + 
						FIX_IN(Y_B_IN) * b) >> SCALEBITS_IN) + Y_ADD_IN;

            b4 += (b = src[width3 + 3]);
            g4 += (g = src[width3 + 4]);
            r4 += (r = src[width3 + 5]);
            y_out[1] = (uint8_t)((
						  FIX_IN(Y_R_IN) * r
						+ FIX_IN(Y_G_IN) * g
						+ FIX_IN(Y_B_IN) * b) >> SCALEBITS_IN) + Y_ADD_IN;
            
			*u_out++ = (uint8_t)((
						- FIX_IN(U_R_IN) * r4 
						- FIX_IN(U_G_IN) * g4
						+ FIX_IN(U_B_IN) * b4) >> (SCALEBITS_IN + 2)) + U_ADD_IN;
			
			
            *v_out++ = (uint8_t)((
						  FIX_IN(V_R_IN) * r4
						- FIX_IN(V_G_IN) * g4
						- FIX_IN(V_B_IN) * b4) >> (SCALEBITS_IN + 2)) + V_ADD_IN; 
					
			
			src += 6;
			y_out += 2;
		}
		src -= src_dif;
		y_out += y_dif;
		u_out += uv_dif;
		v_out += uv_dif;
    }
}


/*	rgb32 -> yuv 4:2:0 planar 

	NOTE: always flips
*/

void rgb32_to_yv12_c(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
					uint8_t *src, int width, int height, int stride)
{
    uint32_t width4 = (width << 2);		/* width * 4 */
	uint32_t src_dif = 3 * width4;
	uint32_t y_dif = (stride << 1) - width;
	uint32_t uv_dif = (stride - width) >> 1;
    uint32_t x, y;
	
	src += (height - 2) * width4;

    for(y = height >> 1; y; y--) {
		for(x = width >> 1; x; x--) {
			uint32_t r, g, b, r4, g4, b4;

            b4 = b = src[0];
            g4 = g = src[1];
            r4 = r = src[2];
            y_out[stride + 0] =(uint8_t)((
						  FIX_IN(Y_R_IN) * r 
						+ FIX_IN(Y_G_IN) * g 
						+ FIX_IN(Y_B_IN) * b) >> SCALEBITS_IN) + Y_ADD_IN;

            b4 += (b = src[4]);
            g4 += (g = src[5]);
            r4 += (r = src[6]);
            y_out[stride + 1] =(uint8_t)((
						  FIX_IN(Y_R_IN) * r 
						+ FIX_IN(Y_G_IN) * g 
						+ FIX_IN(Y_B_IN) * b) >> SCALEBITS_IN) + Y_ADD_IN;

            b4 += (b = src[width4 + 0]);
            g4 += (g = src[width4 + 1]);
            r4 += (r = src[width4 + 2]);

            y_out[0] =(uint8_t)((
						  FIX_IN(Y_R_IN) * r 
						+ FIX_IN(Y_G_IN) * g 
						+ FIX_IN(Y_B_IN) * b) >> SCALEBITS_IN) + Y_ADD_IN;
            
            b4 += (b = src[width4 + 4]);
            g4 += (g = src[width4 + 5]);
            r4 += (r = src[width4 + 6]);
            y_out[1] =(uint8_t)((
						  FIX_IN(Y_R_IN) * r 
						+ FIX_IN(Y_G_IN) * g 
						+ FIX_IN(Y_B_IN) * b) >> SCALEBITS_IN) + Y_ADD_IN;
            
			*u_out++ = (uint8_t)((
						- FIX_IN(U_R_IN) * r4 
						- FIX_IN(U_G_IN) * g4
						+ FIX_IN(U_B_IN) * b4) >> (SCALEBITS_IN + 2)) + U_ADD_IN;

            *v_out++ = (uint8_t)((
						  FIX_IN(V_R_IN) * r4
						- FIX_IN(V_G_IN) * g4
						- FIX_IN(V_B_IN) * b4) >> (SCALEBITS_IN + 2)) + V_ADD_IN;
			
			src += 8;
			y_out += 2;
		}
		src -= src_dif;
		y_out += y_dif;
		u_out += uv_dif;
		v_out += uv_dif;
    }
}

/*	yuv planar -> yuv 4:2:0 planar
   
	NOTE: does not flip */

void yuv_to_yv12_c(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
				uint8_t *src, int width, int height, int stride)
{
	uint32_t stride2 = stride >> 1;
	uint32_t width2 = width >> 1;
    uint32_t y;

	for (y = height; y; y--)	{
	    memcpy(y_out, src, width);
	    src += width;
		y_out += stride;
	}

	for (y = height >> 1; y; y--) {
	    memcpy(u_out, src, width2);
		src += width2;
		u_out += stride2;
	}

	for (y = height >> 1; y; y--) {
	    memcpy(v_out, src, width2);
		src += width2;
		v_out+= stride2;
	}
}

#ifdef MPEG4IP
void yuv_to_yv12_clip_c(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
				uint8_t *src, int width, int height, int raw_height, int stride)
{
	uint32_t stride2 = stride >> 1;
	uint32_t width2 = width >> 1;
    uint32_t y;
	uint32_t yoffset = ((raw_height - height) / 2) * width;

	src += yoffset;

	for (y = height; y; y--)	{
	    memcpy(y_out, src, width);
	    src += width;
		y_out += stride;
	}

	src += yoffset + (yoffset / 4);

	for (y = height >> 1; y; y--) {
	    memcpy(u_out, src, width2);
		src += width2;
		u_out += stride2;
	}

	src += yoffset / 2;

	for (y = height >> 1; y; y--) {
	    memcpy(v_out, src, width2);
		src += width2;
		v_out+= stride2;
	}
}
#endif


/* yuyv (yuv2) packed -> yuv 4:2:0 planar
   
   NOTE: does not flip */

void yuyv_to_yv12_c(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
					uint8_t *src, int width, int height, int stride)
{
	uint32_t width2 = width + width;
	uint32_t y_dif = stride - width;
	uint32_t uv_dif = y_dif >> 1;
	uint32_t x, y;

	for (y = height >> 1; y; y--) {
        
		for (x = width >> 1; x; x--) {
            *y_out++ = *src++;
			//*u_out++ = *src++;
			*u_out++ = (*(src+width2) + *src) >> 1;	src++;
			*y_out++ = *src++;
			//*v_out++ = *src++;
			*v_out++ = (*(src+width2) + *src) >> 1; src++;
			
		}

		y_out += y_dif;
		u_out += uv_dif;
		v_out += uv_dif; 

		for (x = width >> 1; x; x--) {
			*y_out++ = *src++;
			src++;
			*y_out++ = *src++;
			src++;
		}

		y_out += y_dif;

    }

}



/* uyvy packed -> yuv 4:2:0 planar
   
   NOTE: does not flip */


void uyvy_to_yv12_c(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
					uint8_t *src, int width, int height, int stride)
{
	uint32_t width2 = width + width;
	uint32_t y_dif = stride - width;
	uint32_t uv_dif = y_dif >> 1;
    uint32_t x, y;

	for (y = height >> 1; y; y--) {
        
		for (x = width >> 1; x; x--) {
			*u_out++ = *src++;
            // *u_out++ = (*(src+width2) + *src++) >> 1;
			*y_out++ = *src++;
			//*v_out++ = *src++;
			*v_out++ = (*(src+width2) + *src) >> 1; src++;
			*y_out++ = *src++;
         }

		y_out += y_dif;
		u_out += uv_dif;;
		v_out += uv_dif;; 

		for (x = width >> 1; x; x--) {
			src++;
			*y_out++ = *src++;
			src++;
			*y_out++ = *src++;
		}

		y_out += y_dif;
    }
}


/*	yuv -> rgb def's */

#define RGB_Y_OUT		1.164
#define B_U_OUT			2.018
#define Y_ADD_OUT		16

#define G_U_OUT			0.391
#define G_V_OUT			0.813
#define U_ADD_OUT		128

#define R_V_OUT			1.596
#define V_ADD_OUT		128


#define SCALEBITS_OUT	13
#define FIX_OUT(x)		((uint16_t) ((x) * (1L<<SCALEBITS_OUT) + 0.5))


/* initialize rgb lookup tables */

void colorspace_init(void) {
	int32_t i;

    for(i = 0; i < 256; i++) {
		RGB_Y_tab[i] = FIX_OUT(RGB_Y_OUT) * (i - Y_ADD_OUT);
		B_U_tab[i] = FIX_OUT(B_U_OUT) * (i - U_ADD_OUT);
		G_U_tab[i] = FIX_OUT(G_U_OUT) * (i - U_ADD_OUT);
		G_V_tab[i] = FIX_OUT(G_V_OUT) * (i - V_ADD_OUT);
		R_V_tab[i] = FIX_OUT(R_V_OUT) * (i - V_ADD_OUT);
	}
}

/* yuv 4:2:0 planar -> rgb555 + very simple error diffusion
*/

#define MK_RGB555(R,G,B)	((MAX(0,MIN(255, R)) << 7) & 0x7c00) | \
							((MAX(0,MIN(255, G)) << 2) & 0x03e0) | \
							((MAX(0,MIN(255, B)) >> 3) & 0x001f)


void yv12_to_rgb555_c(uint8_t *dst, int dst_stride,
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
			b = (b & 0x7) + ((rgb_y + b_u) >> SCALEBITS_OUT);
			g = (g & 0x7) + ((rgb_y - g_uv) >> SCALEBITS_OUT);
			r = (r & 0x7) + ((rgb_y + r_v) >> SCALEBITS_OUT);
			*(uint16_t*)dst = MK_RGB555(r,g,b);

			y_src++;
			rgb_y = RGB_Y_tab[*y_src];
			b = (b & 0x7) + ((rgb_y + b_u) >> SCALEBITS_OUT);
			g = (g & 0x7) + ((rgb_y - g_uv) >> SCALEBITS_OUT);
			r = (r & 0x7) + ((rgb_y + r_v) >> SCALEBITS_OUT);
			*(uint16_t*)(dst+2) = MK_RGB555(r,g,b);
			y_src++;

			rgb_y = RGB_Y_tab[*y_src2];
			b2 = (b2 & 0x7) + ((rgb_y + b_u) >> SCALEBITS_OUT);
			g2 = (g2 & 0x7) + ((rgb_y - g_uv) >> SCALEBITS_OUT);
			r2 = (r2 & 0x7) + ((rgb_y + r_v) >> SCALEBITS_OUT);
			*(uint16_t*)(dst2) = MK_RGB555(r2,g2,b2);
			y_src2++;

			rgb_y = RGB_Y_tab[*y_src2];
			b2 = (b2 & 0x7) + ((rgb_y + b_u) >> SCALEBITS_OUT);
			g2 = (g2 & 0x7) + ((rgb_y - g_uv) >> SCALEBITS_OUT);
			r2 = (r2 & 0x7) + ((rgb_y + r_v) >> SCALEBITS_OUT);
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

void yv12_to_rgb565_c(uint8_t *dst, int dst_stride,
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
			b = (b & 0x7) + ((rgb_y + b_u) >> SCALEBITS_OUT);
			g = (g & 0x7) + ((rgb_y - g_uv) >> SCALEBITS_OUT);
			r = (r & 0x7) + ((rgb_y + r_v) >> SCALEBITS_OUT);
			*(uint16_t*)dst = MK_RGB565(r,g,b);

			y_src++;
			rgb_y = RGB_Y_tab[*y_src];
			b = (b & 0x7) + ((rgb_y + b_u) >> SCALEBITS_OUT);
			g = (g & 0x7) + ((rgb_y - g_uv) >> SCALEBITS_OUT);
			r = (r & 0x7) + ((rgb_y + r_v) >> SCALEBITS_OUT);
			*(uint16_t*)(dst+2) = MK_RGB565(r,g,b);
			y_src++;

			rgb_y = RGB_Y_tab[*y_src2];
			b2 = (b2 & 0x7) + ((rgb_y + b_u) >> SCALEBITS_OUT);
			g2 = (g2 & 0x7) + ((rgb_y - g_uv) >> SCALEBITS_OUT);
			r2 = (r2 & 0x7) + ((rgb_y + r_v) >> SCALEBITS_OUT);
			*(uint16_t*)(dst2) = MK_RGB565(r2,g2,b2);
			y_src2++;

			rgb_y = RGB_Y_tab[*y_src2];
			b2 = (b2 & 0x7) + ((rgb_y + b_u) >> SCALEBITS_OUT);
			g2 = (g2 & 0x7) + ((rgb_y - g_uv) >> SCALEBITS_OUT);
			r2 = (r2 & 0x7) + ((rgb_y + r_v) >> SCALEBITS_OUT);
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

void yv12_to_rgb24_c(uint8_t *dst, int dst_stride,
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
			b = (rgb_y + b_u) >> SCALEBITS_OUT;
			g = (rgb_y - g_uv) >> SCALEBITS_OUT;
			r = (rgb_y + r_v) >> SCALEBITS_OUT;
			dst[0] = MAX(0,MIN(255, b));
			dst[1] = MAX(0,MIN(255, g));
			dst[2] = MAX(0,MIN(255, r));

			y_src++;
			rgb_y = RGB_Y_tab[*y_src];
			b = (rgb_y + b_u) >> SCALEBITS_OUT;
			g = (rgb_y - g_uv) >> SCALEBITS_OUT;
			r = (rgb_y + r_v) >> SCALEBITS_OUT;
			dst[3] = MAX(0,MIN(255, b));
			dst[4] = MAX(0,MIN(255, g));
			dst[5] = MAX(0,MIN(255, r));
			y_src++;

			rgb_y = RGB_Y_tab[*y_src2];
			b = (rgb_y + b_u) >> SCALEBITS_OUT;
			g = (rgb_y - g_uv) >> SCALEBITS_OUT;
			r = (rgb_y + r_v) >> SCALEBITS_OUT;
			dst2[0] = MAX(0,MIN(255, b));
			dst2[1] = MAX(0,MIN(255, g));
			dst2[2] = MAX(0,MIN(255, r));
			y_src2++;

			rgb_y = RGB_Y_tab[*y_src2];
			b = (rgb_y + b_u) >> SCALEBITS_OUT;
			g = (rgb_y - g_uv) >> SCALEBITS_OUT;
			r = (rgb_y + r_v) >> SCALEBITS_OUT;
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

void yv12_to_rgb32_c(uint8_t *dst, int dst_stride,
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
			b = (rgb_y + b_u) >> SCALEBITS_OUT;
			g = (rgb_y - g_uv) >> SCALEBITS_OUT;
			r = (rgb_y + r_v) >> SCALEBITS_OUT;
			dst[0] = MAX(0,MIN(255, r));
			dst[1] = MAX(0,MIN(255, g));
			dst[2] = MAX(0,MIN(255, b));
			dst[3] = 0;

			y_src++;
			rgb_y = RGB_Y_tab[*y_src];
			b = (rgb_y + b_u) >> SCALEBITS_OUT;
			g = (rgb_y - g_uv) >> SCALEBITS_OUT;
			r = (rgb_y + r_v) >> SCALEBITS_OUT;
			dst[4] = MAX(0,MIN(255, r));
			dst[5] = MAX(0,MIN(255, g));
			dst[6] = MAX(0,MIN(255, b));
			dst[7] = 0;
			y_src++;

			rgb_y = RGB_Y_tab[*y_src2];
			b = (rgb_y + b_u) >> SCALEBITS_OUT;
			g = (rgb_y - g_uv) >> SCALEBITS_OUT;
			r = (rgb_y + r_v) >> SCALEBITS_OUT;
			dst2[0] = MAX(0,MIN(255, r));
			dst2[1] = MAX(0,MIN(255, g));
			dst2[2] = MAX(0,MIN(255, b));
			dst2[3] = 0;
			y_src2++;

			rgb_y = RGB_Y_tab[*y_src2];
			b = (rgb_y + b_u) >> SCALEBITS_OUT;
			g = (rgb_y - g_uv) >> SCALEBITS_OUT;
			r = (rgb_y + r_v) >> SCALEBITS_OUT;
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

void yv12_to_yuv_c(uint8_t *dst, int dst_stride,
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

void yv12_to_yuyv_c(uint8_t *dst, int dst_stride,
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

void yv12_to_uyvy_c(uint8_t *dst, int dst_stride,
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
