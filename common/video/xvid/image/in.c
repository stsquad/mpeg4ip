/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	input colorspace conversions
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
 *	26.02.2002	rgb555, rgb565
 *	24.11.2001	accuracy improvement to yuyv/vyuy conversion
 *	28.10.2001	total rewrite <pross@cs.rmit.edu.au>
 *
 **************************************************************************/


#include <string.h>		// memcpy

#include "in.h"
#include "../portab.h"


// function pointers
color_inFuncPtr rgb555_to_yuv;
color_inFuncPtr rgb565_to_yuv;
color_inFuncPtr rgb24_to_yuv;
color_inFuncPtr rgb32_to_yuv;
color_inFuncPtr yuv_to_yuv;
color_inFuncPtr yuyv_to_yuv;
color_inFuncPtr uyvy_to_yuv;


/*	rgb -> yuv def's

	this following constants are "official spec"
	Video Demystified" (ISBN 1-878707-09-4)

	rgb<->yuv _is_ lossy, sicne most programs do the conversion differently
		
	SCALEBITS/FIX taken from  ffmpeg
*/

#define Y_R		0.257
#define Y_G		0.504
#define Y_B		0.098
#define Y_ADD	16

#define U_R		0.148
#define U_G		0.291
#define U_B		0.439
#define U_ADD	128

#define V_R		0.439
#define V_G		0.368
#define V_B		0.071
#define V_ADD	128


#define SCALEBITS 8
#define FIX(x)		((int) ((x) * (1L<<SCALEBITS) + 0.5))




/* rgb555 -> yuv 4:2:0 planar */
void rgb555_to_yuv_c(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
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
						  FIX(Y_R) * r 
						+ FIX(Y_G) * g 
						+ FIX(Y_B) * b) >> SCALEBITS) + Y_ADD;

			rgb = *(uint16_t*)(src+x*2+src_stride);
			b4 += b = (rgb << 3) & 0xf8;
			g4 += g = (rgb >> 2) & 0xf8;
			r4 += r = (rgb >> 7) & 0xf8;
            y_out[y_stride] =(uint8_t)((
						  FIX(Y_R) * r 
						+ FIX(Y_G) * g 
						+ FIX(Y_B) * b) >> SCALEBITS) + Y_ADD;

			rgb = *(uint16_t*)(src+x*2+2);
			b4 += b = (rgb << 3) & 0xf8;
			g4 += g = (rgb >> 2) & 0xf8;
			r4 += r = (rgb >> 7) & 0xf8;
            y_out[1] =(uint8_t)((
						  FIX(Y_R) * r 
						+ FIX(Y_G) * g 
						+ FIX(Y_B) * b) >> SCALEBITS) + Y_ADD;

			rgb = *(uint16_t*)(src+x*2+src_stride+2);
			b4 += b = (rgb << 3) & 0xf8;
			g4 += g = (rgb >> 2) & 0xf8;
			r4 += r = (rgb >> 7) & 0xf8;
            y_out[y_stride + 1] =(uint8_t)((
						  FIX(Y_R) * r 
						+ FIX(Y_G) * g 
						+ FIX(Y_B) * b) >> SCALEBITS) + Y_ADD;

			*u_out++ = (uint8_t)((
						- FIX(U_R) * r4 
						- FIX(U_G) * g4
						+ FIX(U_B) * b4) >> (SCALEBITS + 2)) + U_ADD;
			
			
            *v_out++ = (uint8_t)((
						  FIX(V_R) * r4
						- FIX(V_G) * g4
						- FIX(V_B) * b4) >> (SCALEBITS + 2)) + V_ADD; 

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

void rgb565_to_yuv_c(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
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
						  FIX(Y_R) * r 
						+ FIX(Y_G) * g 
						+ FIX(Y_B) * b) >> SCALEBITS) + Y_ADD;

			rgb = *(uint16_t*)(src+x*2+src_stride);
			b4 += b = (rgb << 3) & 0xf8;
			g4 += g = (rgb >> 3) & 0xfc;
			r4 += r = (rgb >> 8) & 0xf8;
            y_out[y_stride] =(uint8_t)((
						  FIX(Y_R) * r 
						+ FIX(Y_G) * g 
						+ FIX(Y_B) * b) >> SCALEBITS) + Y_ADD;

			rgb = *(uint16_t*)(src+x*2+2);
			b4 += b = (rgb << 3) & 0xf8;
			g4 += g = (rgb >> 3) & 0xfc;
			r4 += r = (rgb >> 8) & 0xf8;
            y_out[1] =(uint8_t)((
						  FIX(Y_R) * r 
						+ FIX(Y_G) * g 
						+ FIX(Y_B) * b) >> SCALEBITS) + Y_ADD;

			rgb = *(uint16_t*)(src+x*2+src_stride+2);
			b4 += b = (rgb << 3) & 0xf8;
			g4 += g = (rgb >> 3) & 0xfc;
			r4 += r = (rgb >> 8) & 0xf8;
            y_out[y_stride + 1] =(uint8_t)((
						  FIX(Y_R) * r 
						+ FIX(Y_G) * g 
						+ FIX(Y_B) * b) >> SCALEBITS) + Y_ADD;

			*u_out++ = (uint8_t)((
						- FIX(U_R) * r4 
						- FIX(U_G) * g4
						+ FIX(U_B) * b4) >> (SCALEBITS + 2)) + U_ADD;
			
			
            *v_out++ = (uint8_t)((
						  FIX(V_R) * r4
						- FIX(V_G) * g4
						- FIX(V_B) * b4) >> (SCALEBITS + 2)) + V_ADD; 

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

void rgb24_to_yuv_c(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
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
						  FIX(Y_R) * r 
						+ FIX(Y_G) * g 
						+ FIX(Y_B) * b) >> SCALEBITS) + Y_ADD; 

            b4 += (b = src[3]);
            g4 += (g = src[4]);
            r4 += (r = src[5]);
            y_out[stride + 1] = (uint8_t)((
						  FIX(Y_R) * r 
						+ FIX(Y_G) * g 
						+ FIX(Y_B) * b) >> SCALEBITS) + Y_ADD;

            b4 += (b = src[width3 + 0]);
            g4 += (g = src[width3 + 1]);
            r4 += (r = src[width3 + 2]);
            y_out[0] = (uint8_t)((
						FIX(Y_R) * r + 
						FIX(Y_G) * g + 
						FIX(Y_B) * b) >> SCALEBITS) + Y_ADD;

            b4 += (b = src[width3 + 3]);
            g4 += (g = src[width3 + 4]);
            r4 += (r = src[width3 + 5]);
            y_out[1] = (uint8_t)((
						  FIX(Y_R) * r
						+ FIX(Y_G) * g
						+ FIX(Y_B) * b) >> SCALEBITS) + Y_ADD;
            
			*u_out++ = (uint8_t)((
						- FIX(U_R) * r4 
						- FIX(U_G) * g4
						+ FIX(U_B) * b4) >> (SCALEBITS + 2)) + U_ADD;
			
			
            *v_out++ = (uint8_t)((
						  FIX(V_R) * r4
						- FIX(V_G) * g4
						- FIX(V_B) * b4) >> (SCALEBITS + 2)) + V_ADD; 
					
			
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

void rgb32_to_yuv_c(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
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
						  FIX(Y_R) * r 
						+ FIX(Y_G) * g 
						+ FIX(Y_B) * b) >> SCALEBITS) + Y_ADD;

            b4 += (b = src[4]);
            g4 += (g = src[5]);
            r4 += (r = src[6]);
            y_out[stride + 1] =(uint8_t)((
						  FIX(Y_R) * r 
						+ FIX(Y_G) * g 
						+ FIX(Y_B) * b) >> SCALEBITS) + Y_ADD;

            b4 += (b = src[width4 + 0]);
            g4 += (g = src[width4 + 1]);
            r4 += (r = src[width4 + 2]);

            y_out[0] =(uint8_t)((
						  FIX(Y_R) * r 
						+ FIX(Y_G) * g 
						+ FIX(Y_B) * b) >> SCALEBITS) + Y_ADD;
            
            b4 += (b = src[width4 + 4]);
            g4 += (g = src[width4 + 5]);
            r4 += (r = src[width4 + 6]);
            y_out[1] =(uint8_t)((
						  FIX(Y_R) * r 
						+ FIX(Y_G) * g 
						+ FIX(Y_B) * b) >> SCALEBITS) + Y_ADD;
            
			*u_out++ = (uint8_t)((
						- FIX(U_R) * r4 
						- FIX(U_G) * g4
						+ FIX(U_B) * b4) >> (SCALEBITS + 2)) + U_ADD;

            *v_out++ = (uint8_t)((
						  FIX(V_R) * r4
						- FIX(V_G) * g4
						- FIX(V_B) * b4) >> (SCALEBITS + 2)) + V_ADD;
			
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

void yuv_to_yuv_c(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
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
void yuv_to_yuv_clip_c(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
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

void yuyv_to_yuv_c(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
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


void uyvy_to_yuv_c(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
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
