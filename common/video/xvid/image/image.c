/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	bitstream writer
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
 *  23.12.2001  removed #ifdefs, added function pointers + init_common()
 *	22.12.2001	cpu #ifdefs
 *  19.12.2001  image_dump(); useful for debugging
 *	 6.12.2001	inital version; (c)2001 peter ross <pross@cs.rmit.edu.au>
 *
 *************************************************************************/


#include <stdlib.h>
#include <string.h>  // memcpy, memset

#include "../portab.h"
#include "../xvid.h"       // XVID_CSP_XXX's
#include "image.h"
#include "in.h"
#include "out.h"
#include "halfpel.h"
#include "../decore.h"    

#define SAFETY	64
#define EDGE_SIZE2  (EDGE_SIZE/2)

void init_image(uint32_t cpu_flags) {

	init_yuv_to_rgb();

	interpolate_halfpel_h = interpolate_halfpel_h_c;
	interpolate_halfpel_v = interpolate_halfpel_v_c;
	interpolate_halfpel_hv = interpolate_halfpel_hv_c;

	rgb24_to_yuv = rgb24_to_yuv_c;
	rgb32_to_yuv = rgb32_to_yuv_c;
	yuv_to_yuv = yuv_to_yuv_c;
	yuyv_to_yuv = yuyv_to_yuv_c;
	uyvy_to_yuv = uyvy_to_yuv_c;

	yuv_to_rgb24 = yuv_to_rgb24_c;
	yuv_to_rgb32 = yuv_to_rgb32_c;
	out_yuv = out_yuv_c;
	yuv_to_yuyv = yuv_to_yuyv_c;
	yuv_to_uyvy = yuv_to_uyvy_c;


#ifdef USE_MMX
	if((cpu_flags & XVID_CPU_MMX) > 0) {
		interpolate_halfpel_h = interpolate_halfpel_h_mmx;
		interpolate_halfpel_v = interpolate_halfpel_v_mmx;
		interpolate_halfpel_hv = interpolate_halfpel_hv_mmx;

		rgb24_to_yuv = rgb24_to_yuv_mmx;
		rgb32_to_yuv = rgb32_to_yuv_mmx;
		yuv_to_yuv = yuv_to_yuv_mmx;
		yuyv_to_yuv = yuyv_to_yuv_mmx;
		uyvy_to_yuv = uyvy_to_yuv_mmx;

		yuv_to_rgb24 = yuv_to_rgb24_mmx;
		yuv_to_rgb32 = yuv_to_rgb32_mmx;
		yuv_to_yuyv = yuv_to_yuyv_mmx;
		yuv_to_uyvy = yuv_to_uyvy_mmx;

	}

	if((cpu_flags & XVID_CPU_MMXEXT) > 0) {
		yuv_to_yuv = yuv_to_yuv_xmm;
	}

	if((cpu_flags & XVID_CPU_3DNOW) > 0) {
		interpolate_halfpel_h = interpolate_halfpel_h_3dn;
		interpolate_halfpel_v = interpolate_halfpel_v_3dn;
	}
#endif
}

uint32_t image_create(IMAGE * image, uint32_t edged_width, uint32_t edged_height)
{
	const uint32_t edged_width2 = edged_width / 2;
	const uint32_t edged_height2 = edged_height / 2;

	image->y = malloc(edged_width * edged_height + SAFETY);
	if (image->y == NULL)
	{
		return -1;
	}
	image->u = malloc(edged_width2 * edged_height2 + SAFETY);
	if (image->u == NULL)
	{
		free(image->y);
		return -1;
	}
	image->v = malloc(edged_width2 * edged_height2 + SAFETY);
	if (image->v == NULL)
	{
		free(image->u);
		free(image->y);
		return -1;
	}
	
	image->y += EDGE_SIZE * edged_width + EDGE_SIZE;
	image->u += EDGE_SIZE2 * edged_width2 + EDGE_SIZE2;
	image->v += EDGE_SIZE2 * edged_width2 + EDGE_SIZE2;

	return 0;
}




void image_destroy(IMAGE * image, uint32_t edged_width, uint32_t edged_height)
{
	const uint32_t edged_width2 = edged_width / 2;

	free(image->y - (EDGE_SIZE * edged_width + EDGE_SIZE) );
	free(image->u - (EDGE_SIZE2 * edged_width2 + EDGE_SIZE2));
	free(image->v - (EDGE_SIZE2 * edged_width2 + EDGE_SIZE2));
}

void image_swap(IMAGE * image1, IMAGE * image2)
{
    uint8_t * tmp;

    tmp = image1->y;
	image1->y = image2->y;
	image2->y = tmp;

    tmp = image1->u;
	image1->u = image2->u;
	image2->u = tmp;

    tmp = image1->v;
	image1->v = image2->v;
	image2->v = tmp;
}


void image_copy(IMAGE *image1, IMAGE * image2, uint32_t edged_width, uint32_t height)
{
	memcpy(image1->y, image2->y, edged_width * height);
	memcpy(image1->u, image2->u, edged_width * height / 4);
	memcpy(image1->v, image2->v, edged_width * height / 4);
}


void image_setedges(IMAGE * image, uint32_t edged_width, uint32_t edged_height, uint32_t width, uint32_t height)
{
	const uint32_t edged_width2 = edged_width / 2;
	const uint32_t width2 = width / 2;
	uint32_t i;
	uint8_t * dst;
	uint8_t * src;

   
    dst = image->y - (EDGE_SIZE + EDGE_SIZE * edged_width);
    src = image->y;

    for (i = 0; i < EDGE_SIZE; i++)
    {
		memset(dst, *src, EDGE_SIZE);
		memcpy(dst + EDGE_SIZE, src, width);
		memset(dst + edged_width - EDGE_SIZE, src[width - 1], EDGE_SIZE);
		dst += edged_width;
    }

    for (i = 0; i < height; i++)
    {
		memset(dst, *src, EDGE_SIZE);
		memset(dst + edged_width - EDGE_SIZE, src[width - 1], EDGE_SIZE);
		dst += edged_width;
		src += edged_width;
    }
    
	src -= edged_width;
    for (i = 0; i < EDGE_SIZE; i++)
    {
		memset(dst, *src, EDGE_SIZE);
		memcpy(dst + EDGE_SIZE, src, width);
		memset(dst + edged_width - EDGE_SIZE, src[width - 1], EDGE_SIZE);
		dst += edged_width;
    }


//U
    dst = image->u - (EDGE_SIZE2 + EDGE_SIZE2 * edged_width2);
    src = image->u;

    for (i = 0; i < EDGE_SIZE2; i++)
	{
		memset(dst, *src, EDGE_SIZE2);
		memcpy(dst + EDGE_SIZE2, src, width2);
		memset(dst + edged_width2 - EDGE_SIZE2, src[width2 - 1], EDGE_SIZE2);
		dst += edged_width2;
    }

    for (i = 0; i < height / 2; i++)
    {
		memset(dst, *src, EDGE_SIZE2);
		memset(dst + edged_width2 - EDGE_SIZE2, src[width2 - 1],
	       EDGE_SIZE2);
		dst += edged_width2;
		src += edged_width2;
    }
    src -= edged_width2;
    for (i = 0; i < EDGE_SIZE2; i++)
	{
		memset(dst, *src, EDGE_SIZE2);
		memcpy(dst + EDGE_SIZE2, src, width2);
		memset(dst + edged_width2 - EDGE_SIZE2, src[width2 - 1],
	       EDGE_SIZE2);
		dst += edged_width2;
    }

	
// V
	dst = image->v - (EDGE_SIZE2 + EDGE_SIZE2 * edged_width2);
	src = image->v;

	for (i = 0; i < EDGE_SIZE2; i++)
	{
		memset(dst, *src, EDGE_SIZE2);
		memcpy(dst + EDGE_SIZE2, src, width2);
		memset(dst + edged_width2 - EDGE_SIZE2, src[width2 - 1], EDGE_SIZE2);
		dst += edged_width2;
    }
    
	for (i = 0; i < height / 2; i++)
    {
		memset(dst, *src, EDGE_SIZE2);
		memset(dst + edged_width2 - EDGE_SIZE2, src[width2 - 1], EDGE_SIZE2);
		dst += edged_width2;
		src += edged_width2;
    }
    src -= edged_width2;
    for (i = 0; i < EDGE_SIZE2; i++)
	{
		memset(dst, *src, EDGE_SIZE2);
		memcpy(dst + EDGE_SIZE2, src, width2);
		memset(dst + edged_width2 - EDGE_SIZE2, src[width2 - 1], EDGE_SIZE2);
		dst += edged_width2;
	}
}


void image_interpolate(const IMAGE * refn, 
					   IMAGE * refh, IMAGE * refv,	IMAGE * refhv, 
					   uint32_t edged_width, uint32_t edged_height, uint32_t rounding)
{
    uint32_t offset;

	offset = EDGE_SIZE * (edged_width + 1);

	interpolate_halfpel_h(
		refh->y - offset,
		refn->y - offset, 
		edged_width, edged_height,
		rounding);

	interpolate_halfpel_v(
		refv->y - offset,
		refn->y - offset, 
		edged_width, edged_height,
		rounding);

	interpolate_halfpel_hv(
		refhv->y - offset,
		refn->y - offset,
		edged_width, edged_height,
		rounding);


	offset = EDGE_SIZE2 * (edged_width / 2 + 1);

    interpolate_halfpel_h(
		refh->u - offset,
		refn->u - offset, 
		edged_width / 2, edged_height / 2,
		rounding);

    interpolate_halfpel_v(
		refv->u - offset,
		refn->u - offset, 
		edged_width / 2, edged_height / 2,
		rounding);

    interpolate_halfpel_hv(
		refhv->u - offset,
		refn->u - offset, 
		edged_width / 2, edged_height / 2,
		rounding);


    interpolate_halfpel_h(
		refh->v - offset,
		refn->v - offset, 
		edged_width / 2, edged_height / 2,
		rounding);

    interpolate_halfpel_v(
		refv->v - offset,
		refn->v - offset, 
		edged_width / 2, edged_height / 2,
		rounding);

	interpolate_halfpel_hv(
		refhv->v - offset,
		refn->v - offset, 
		edged_width / 2, edged_height / 2,
		rounding);
}


int image_input(IMAGE * image, uint32_t width, int height, uint32_t edged_width,
			int8_t * src, int csp)
{

/*	if (csp & XVID_CSP_VFLIP)
	{
		height = -height;
	}
*/

	switch(csp & ~XVID_CSP_VFLIP)
	{
	case XVID_CSP_RGB24 :
		rgb24_to_yuv(image->y, image->u, image->v, src, 
						width, height, edged_width);
		return 0;

	case XVID_CSP_RGB32 :
		rgb32_to_yuv(image->y, image->u, image->v, src, 
						width, height, edged_width);
		return 0;

	case XVID_CSP_I420 :
		yuv_to_yuv(image->y, image->u, image->v, src, 
						width, height, edged_width);
		return 0;

	case XVID_CSP_YV12 :	/* u/v swapped */
		yuv_to_yuv(image->y, image->v, image->u, src, 
						width, height, edged_width);
		return 0;

	case XVID_CSP_YUY2 :
		yuyv_to_yuv(image->y, image->u, image->v, src, 
						width, height, edged_width);
		return 0;

	case XVID_CSP_YVYU :	/* u/v swapped */
		yuyv_to_yuv(image->y, image->v, image->u, src, 
						width, height, edged_width);
		return 0;

	case XVID_CSP_UYVY :
		uyvy_to_yuv(image->y, image->u, image->v, src, 
						width, height, edged_width);
		return 0;

	case XVID_CSP_NULL :
		break;
		
    }

	return -1;
}



int image_output(IMAGE * image, uint32_t width, int height, uint32_t edged_width,
			int8_t * dst, uint32_t dst_stride,	int csp)
{
	if (csp & XVID_CSP_VFLIP)
	{
		height = -height;
	}

	switch(csp & ~XVID_CSP_VFLIP)
	{
	case XVID_CSP_RGB24 :
		yuv_to_rgb24(dst, dst_stride,
				image->y, image->u, image->v, edged_width, edged_width / 2,
				width, height);
		return 0;

	case XVID_CSP_RGB32 :
		yuv_to_rgb32(dst, dst_stride,
				image->y, image->u, image->v, edged_width, edged_width / 2,
				width, height);
		return 0;
	
	case XVID_CSP_I420 :
		out_yuv(dst, dst_stride,
				image->y, image->u, image->v, edged_width, edged_width / 2,
				width, height);
		return 0;

	case XVID_CSP_YV12 :	// u,v swapped
		out_yuv(dst, dst_stride,
				image->y, image->v, image->u, edged_width, edged_width / 2,
				width, height);
		return 0;

	case XVID_CSP_YUY2 :
		yuv_to_yuyv(dst, dst_stride,
				image->y, image->u, image->v, edged_width, edged_width / 2,
				width, height);
		return 0;

	case XVID_CSP_YVYU :	// u,v swapped
		yuv_to_yuyv(dst, dst_stride,
				image->y, image->v, image->u, edged_width, edged_width / 2,
				width, height);
		return 0;

	case XVID_CSP_UYVY :
		yuv_to_uyvy(dst, dst_stride,
				image->y, image->u, image->v, edged_width, edged_width / 2,
				width, height);
		return 0;

        case XVID_CSP_USER :
                ((DEC_PICTURE*)dst)->y = image->y;
                ((DEC_PICTURE*)dst)->u = image->u;
                ((DEC_PICTURE*)dst)->v = image->v;
                ((DEC_PICTURE*)dst)->stride_y = edged_width;
                ((DEC_PICTURE*)dst)->stride_uv = edged_width/2;
                return 0;
                                 
	case XVID_CSP_NULL :
		return 0;

	}

	return -1;
}


#include <stdio.h>
#include <string.h>

int image_dump_pgm(uint8_t * bmp, uint32_t width, uint32_t height, char * filename)
{
	FILE * f;
	char hdr[1024];
	
	f = fopen(filename, "wb");
	if ( f == NULL)
	{
		return -1;
	}
	sprintf(hdr, "P5\n#xvid\n%i %i\n255\n", width, height);
	fwrite(hdr, strlen(hdr), 1, f);
	fwrite(bmp, width, height, f);
	fclose(f);

	return 0;
}


/* dump image+edges to yuv pgm files */

int image_dump(IMAGE * image, uint32_t edged_width, uint32_t edged_height, char * path, int number)
{
	char filename[1024];

	sprintf(filename, "%s_%i_%c.pgm", path, number, 'y');
	image_dump_pgm(
		image->y - (EDGE_SIZE * edged_width + EDGE_SIZE),
		edged_width, edged_height, filename);

	sprintf(filename, "%s_%i_%c.pgm", path, number, 'u');
	image_dump_pgm(
		image->u - (EDGE_SIZE2 * edged_width / 2 + EDGE_SIZE2),
		edged_width / 2, edged_height / 2, filename);

	sprintf(filename, "%s_%i_%c.pgm", path, number, 'v');
	image_dump_pgm(
		image->v - (EDGE_SIZE2 * edged_width / 2 + EDGE_SIZE2),
		edged_width / 2, edged_height / 2, filename);

	return 0;
}
