#include <string.h>   // memcpy
#include "../portab.h"
#include "out.h"

// function pointers
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
		dst += dst_stride;
		u_src += uv_stride;
	}

	for (y = height >> 1; y; y--) {
	    memcpy(dst, v_src, width2);
		dst += dst_stride;
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
