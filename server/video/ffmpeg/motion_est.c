/*
 * Motion estimation 
 * Copyright (c) 2000 Gerard Lantau.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdlib.h>
#include <stdio.h>
#include "avcodec.h"
#include "dsputil.h"
#include "mpegvideo.h"

static int pix_sum(UINT8 *pix, int line_size)
{
    int s, i, j;

    s = 0;
    for(i=0;i<16;i++) {
        for(j=0;j<16;j+=8) {
            s += pix[0];
            s += pix[1];
            s += pix[2];
            s += pix[3];
            s += pix[4];
            s += pix[5];
            s += pix[6];
            s += pix[7];
            pix += 8;
        }
        pix += line_size - 16;
    }
    return s;
}

static int pix_norm1(UINT8 *pix, int line_size)
{
    int s, i, j;
    UINT32 *sq = squareTbl + 256;

    s = 0;
    for(i=0;i<16;i++) {
        for(j=0;j<16;j+=8) {
            s += sq[pix[0]];
            s += sq[pix[1]];
            s += sq[pix[2]];
            s += sq[pix[3]];
            s += sq[pix[4]];
            s += sq[pix[5]];
            s += sq[pix[6]];
            s += sq[pix[7]];
            pix += 8;
        }
        pix += line_size - 16;
    }
    return s;
}

static int pix_norm(UINT8 *pix1, UINT8 *pix2, int line_size)
{
    int s, i, j;
    UINT32 *sq = squareTbl + 256;

    s = 0;
    for(i=0;i<16;i++) {
        for(j=0;j<16;j+=8) {
            s += sq[pix1[0] - pix2[0]];
            s += sq[pix1[1] - pix2[1]];
            s += sq[pix1[2] - pix2[2]];
            s += sq[pix1[3] - pix2[3]];
            s += sq[pix1[4] - pix2[4]];
            s += sq[pix1[5] - pix2[5]];
            s += sq[pix1[6] - pix2[6]];
            s += sq[pix1[7] - pix2[7]];
            pix1 += 8;
            pix2 += 8;
        }
        pix1 += line_size - 16;
        pix2 += line_size - 16;
    }
    return s;
}

#define USE_MMX
/* TEMP */
#undef USE_MMX

#ifdef USE_MMX

/* original mmx code from mpeg2movie by Adam Williams */

static void inline mmx_start_block(void)
{
	asm(" 
		.align 8
		pxor %%mm7, %%mm7; 
		pxor %%mm6, %%mm6;
		" : : );
}

static void inline mmx_absdiff(unsigned char *a, unsigned char *b)
{
	asm("
		.align 8
		movq		(%%ebx),	%%mm0;     // Get first half of row1
		movq		(%%ecx),	%%mm1;     // Get first half of row2
		movq		%%mm0,		%%mm2;     // Make a copy of row1 for absdiff operation
		movq		8(%%ebx),	%%mm3;     // Get second half of row1
		psubusb		%%mm1,		%%mm0;     // Subtract the first halves one way
		psubusb		%%mm2,		%%mm1;     // Subtract the other way
		movq        8(%%ecx),   %%mm4;     // Get second half of row2
		por			%%mm1,      %%mm0;     // Merge first half results
		movq		%%mm3,		%%mm5;     // Copy for absdiff operation
		movq		%%mm0,		%%mm1;     // Keep a copy
		psubusb		%%mm4,		%%mm3;     // Subtract second halves one way
		punpcklbw	%%mm6,		%%mm0;     // Unpack to higher precision for accumulation
		psubusb		%%mm5,		%%mm4;     // Subtract the other way
		psrlq		$32,		%%mm1;     // Shift registeres for accumulation
		por			%%mm4,		%%mm3;     // merge results of 2nd half
		punpcklbw	%%mm6,		%%mm1;     // unpack to higher precision for accumulation
		movq		%%mm3,		%%mm4;     // keep a copy
		punpcklbw	%%mm6,		%%mm3;     // unpack to higher precision for accumulation
		paddw		%%mm0,		%%mm7;     // accumulate difference
		psrlq		$32,		%%mm4;     // shift results for accumulation
		paddw		%%mm1,		%%mm7;     // accumulate difference
		punpcklbw	%%mm6,		%%mm4;     // unpack to higher precision for accumulation
		paddw		%%mm3,		%%mm7;     // accumulate difference
		paddw		%%mm4,		%%mm7;     // accumulate difference
		"
		: 
		: "b" (a), "c" (b) 
		: "st" );
}

static inline unsigned int mmx_accum_absdiff()
{
	unsigned long long r = 0;
	asm("
		.align 8
		movq		%%mm7,	%%mm5;
		movq		%%mm7,	%%mm4;
		punpcklwd	%%mm6,	%%mm4;
		punpckhwd	%%mm6,	%%mm5;
		paddd		%%mm5,	%%mm4;
		movq		%%mm4,	%%mm5;
		punpckldq	%%mm6,	%%mm5;
		punpckhdq	%%mm6,	%%mm4;
		paddd		%%mm5,	%%mm4;
		movq		%%mm4,	(%%ebx);
		emms;
		"
		: :  "b" (&r));

	return r;
}

static unsigned int pix_abs16x16(UINT8 *pix1, UINT8 *pix2, int line_size)
{
    int i;

    mmx_start_block();
    for(i=0;i<16;i++) {
        mmx_absdiff(pix1, pix2);
        pix1 += line_size;
        pix2 += line_size;
    }
    return mmx_accum_absdiff();
}

#else

static int pix_abs16x16(UINT8 *pix1, UINT8 *pix2, int line_size)
{
    int s, i;

    s = 0;
    for(i=0;i<16;i++) {
        s += abs(pix1[0] - pix2[0]);
        s += abs(pix1[1] - pix2[1]);
        s += abs(pix1[2] - pix2[2]);
        s += abs(pix1[3] - pix2[3]);
        s += abs(pix1[4] - pix2[4]);
        s += abs(pix1[5] - pix2[5]);
        s += abs(pix1[6] - pix2[6]);
        s += abs(pix1[7] - pix2[7]);
        s += abs(pix1[8] - pix2[8]);
        s += abs(pix1[9] - pix2[9]);
        s += abs(pix1[10] - pix2[10]);
        s += abs(pix1[11] - pix2[11]);
        s += abs(pix1[12] - pix2[12]);
        s += abs(pix1[13] - pix2[13]);
        s += abs(pix1[14] - pix2[14]);
        s += abs(pix1[15] - pix2[15]);
        pix1 += line_size;
        pix2 += line_size;
    }
    return s;
}

#endif

#define avg2(a,b) ((a+b+1)>>1)
#define avg4(a,b,c,d) ((a+b+c+d+2)>>2)

static int pix_abs16x16_x2(UINT8 *pix1, UINT8 *pix2, int line_size)
{
    int s, i;

    s = 0;
    for(i=0;i<16;i++) {
        s += abs(pix1[0] - avg2(pix2[0], pix2[1]));
        s += abs(pix1[1] - avg2(pix2[1], pix2[2]));
        s += abs(pix1[2] - avg2(pix2[2], pix2[3]));
        s += abs(pix1[3] - avg2(pix2[3], pix2[4]));
        s += abs(pix1[4] - avg2(pix2[4], pix2[5]));
        s += abs(pix1[5] - avg2(pix2[5], pix2[6]));
        s += abs(pix1[6] - avg2(pix2[6], pix2[7]));
        s += abs(pix1[7] - avg2(pix2[7], pix2[8]));
        s += abs(pix1[8] - avg2(pix2[8], pix2[9]));
        s += abs(pix1[9] - avg2(pix2[9], pix2[10]));
        s += abs(pix1[10] - avg2(pix2[10], pix2[11]));
        s += abs(pix1[11] - avg2(pix2[11], pix2[12]));
        s += abs(pix1[12] - avg2(pix2[12], pix2[13]));
        s += abs(pix1[13] - avg2(pix2[13], pix2[14]));
        s += abs(pix1[14] - avg2(pix2[14], pix2[15]));
        s += abs(pix1[15] - avg2(pix2[15], pix2[16]));
        pix1 += line_size;
        pix2 += line_size;
    }
    return s;
}

static int pix_abs16x16_y2(UINT8 *pix1, UINT8 *pix2, int line_size)
{
    int s, i;
    UINT8 *pix3 = pix2 + line_size;

    s = 0;
    for(i=0;i<16;i++) {
        s += abs(pix1[0] - avg2(pix2[0], pix3[0]));
        s += abs(pix1[1] - avg2(pix2[1], pix3[1]));
        s += abs(pix1[2] - avg2(pix2[2], pix3[2]));
        s += abs(pix1[3] - avg2(pix2[3], pix3[3]));
        s += abs(pix1[4] - avg2(pix2[4], pix3[4]));
        s += abs(pix1[5] - avg2(pix2[5], pix3[5]));
        s += abs(pix1[6] - avg2(pix2[6], pix3[6]));
        s += abs(pix1[7] - avg2(pix2[7], pix3[7]));
        s += abs(pix1[8] - avg2(pix2[8], pix3[8]));
        s += abs(pix1[9] - avg2(pix2[9], pix3[9]));
        s += abs(pix1[10] - avg2(pix2[10], pix3[10]));
        s += abs(pix1[11] - avg2(pix2[11], pix3[11]));
        s += abs(pix1[12] - avg2(pix2[12], pix3[12]));
        s += abs(pix1[13] - avg2(pix2[13], pix3[13]));
        s += abs(pix1[14] - avg2(pix2[14], pix3[14]));
        s += abs(pix1[15] - avg2(pix2[15], pix3[15]));
        pix1 += line_size;
        pix2 += line_size;
    }
    return s;
}

static int pix_abs16x16_xy2(UINT8 *pix1, UINT8 *pix2, int line_size)
{
    int s, i;
    UINT8 *pix3 = pix2 + line_size;

    s = 0;
    for(i=0;i<16;i++) {
        s += abs(pix1[0] - avg4(pix2[0], pix2[1], pix3[0], pix3[1]));
        s += abs(pix1[1] - avg4(pix2[1], pix2[2], pix3[1], pix3[2]));
        s += abs(pix1[2] - avg4(pix2[2], pix2[3], pix3[2], pix3[3]));
        s += abs(pix1[3] - avg4(pix2[3], pix2[4], pix3[3], pix3[4]));
        s += abs(pix1[4] - avg4(pix2[4], pix2[5], pix3[4], pix3[5]));
        s += abs(pix1[5] - avg4(pix2[5], pix2[6], pix3[5], pix3[6]));
        s += abs(pix1[6] - avg4(pix2[6], pix2[7], pix3[6], pix3[7]));
        s += abs(pix1[7] - avg4(pix2[7], pix2[8], pix3[7], pix3[8]));
        s += abs(pix1[8] - avg4(pix2[8], pix2[9], pix3[8], pix3[9]));
        s += abs(pix1[9] - avg4(pix2[9], pix2[10], pix3[9], pix3[10]));
        s += abs(pix1[10] - avg4(pix2[10], pix2[11], pix3[10], pix3[11]));
        s += abs(pix1[11] - avg4(pix2[11], pix2[12], pix3[11], pix3[12]));
        s += abs(pix1[12] - avg4(pix2[12], pix2[13], pix3[12], pix3[13]));
        s += abs(pix1[13] - avg4(pix2[13], pix2[14], pix3[13], pix3[14]));
        s += abs(pix1[14] - avg4(pix2[14], pix2[15], pix3[14], pix3[15]));
        s += abs(pix1[15] - avg4(pix2[15], pix2[16], pix3[15], pix3[16]));
        pix1 += line_size;
        pix2 += line_size;
    }
    return s;
}

static void no_motion_search(MpegEncContext *s, 
                             int *mx_ptr, int *my_ptr)
{
    *mx_ptr = 0;
    *my_ptr = 0;
}

static void full_motion_search(MpegEncContext *s, 
                               int *mx_ptr, int *my_ptr)
{
    int range, x1, y1, x2, y2, xx, yy, x, y;
    int mx, my, mx1, my1, dmin, d;
    UINT8 *pix;

    range = 8 * (1 << (s->f_code - 1));
    if (s->out_format == FMT_H263)
        range = range * 2;
    /* do not allow vectors outside the image (will change one day for
       h263) */
    xx = 16 * s->mb_x;
    yy = 16 * s->mb_y;
    x1 = xx - range + 1; /* we loose one pixel to avoid boundary pb with half pixel pred */
    if (x1 < 0)
        x1 = 0;
    x2 = xx + range - 1;
    if (x2 > (s->width - 16))
        x2 = s->width - 16;
    y1 = yy - range + 1;
    if (y1 < 0)
        y1 = 0;
    y2 = yy + range - 1;
    if (y2 > (s->height - 16))
        y2 = s->height - 16;
    pix = s->new_picture[0] + (yy * s->width) + xx;
    dmin = 0x7fffffff;
    mx = 0;
    my = 0;
    for(y=y1;y<=y2;y++) {
        for(x=x1;x<=x2;x++) {
            d = pix_abs16x16(pix, s->last_picture[0] + (y * s->width) + x, 
                             s->width);
            if (d < dmin ||
                (d == dmin && 
                 (abs(x - xx) + abs(y - yy)) < 
                 (abs(mx - xx) + abs(my - yy)))) {
                dmin = d;
                mx = x;
                my = y;
            }
        }
    }

    /* half pixel search */
    mx1 = mx = 2 * mx;
    my1 = my = 2 * my;

    if (mx > 0 && mx < 2 * (s->width - 16) &&
        my > 0 && my < 2 * (s->height - 16)) {
        int dx, dy, px, py;
        UINT8 *ptr;
        for(dy=-1;dy<=1;dy++) {
            for(dx=-1;dx<=1;dx++) {
                if (dx != 0 || dy != 0) {
                    px = mx1 + dx;
                    py = my1 + dy;
                    ptr = s->last_picture[0] + ((py >> 1) * s->width) + (px >> 1);
                    switch(((py & 1) << 1) | (px & 1)) {
                    default:
                    case 0:
                        d = pix_abs16x16(pix, ptr, s->width);
                        break;
                    case 1:
                        d = pix_abs16x16_x2(pix, ptr, s->width);
                        break;
                    case 2:
                        d = pix_abs16x16_y2(pix, ptr, s->width);
                        break;
                    case 3:
                        d = pix_abs16x16_xy2(pix, ptr, s->width);
                        break;
                    }
                    if (d < dmin) {
                        dmin = d;
                        mx = px;
                        my = py;
                    }
                }
            }
        }
    }

    *mx_ptr = mx - 2 * xx;
    *my_ptr = my - 2 * yy;
#if 0
    if (*mx_ptr < -(2 * range) || *mx_ptr >= (2 * range) ||
        *my_ptr < -(2 * range) || *my_ptr >= (2 * range)) {
        fprintf(stderr, "error %d %d\n", *mx_ptr, *my_ptr);
    }
#endif
}


#if 1

int estimate_motion(MpegEncContext *s, 
                    int mb_x, int mb_y,
                    int *mx_ptr, int *my_ptr)
{
    UINT8 *pix, *ppix;
    int sum, varc, vard, mx, my;

    
    if (s->full_search) {
        full_motion_search(s, &mx, &my);
    } else {
        no_motion_search(s, &mx, &my);
    }

    /* intra / predictive decision */

    pix = s->new_picture[0] + (mb_y * 16 * s->width) + mb_x * 16;
    ppix = s->last_picture[0] + 
        ((mb_y * 16 + (my >> 1)) * s->width) + (mb_x * 16 + (mx >> 1));

    sum = pix_sum(pix, s->width);
    varc = pix_norm1(pix, s->width);
    vard = pix_norm(pix, ppix, s->width);
    
    vard = vard >> 8;
    sum = sum >> 8;
    varc = (varc >> 8)  - (sum * sum);
#if 0
    printf("varc=%d (sum=%d) vard=%d mx=%d my=%d\n",
           varc, sum, vard, mx, my);
#endif
    if (vard <= 64) {
        *mx_ptr = mx;
        *my_ptr = my;
	return 0;
    } else if (vard < varc) {
        *mx_ptr = mx;
        *my_ptr = my;
	return 0;
    } else {
        *mx_ptr = 0;
        *my_ptr = 0;
        return 1;
    }
}

#else

/* test version which generates valid random vectors */
int estimate_motion(MpegEncContext *s, 
                    int mb_x, int mb_y,
                    int *mx_ptr, int *my_ptr)
{
     int xx, yy, x1, y1, x2, y2, range;

     if ((random() % 10) >= 5) {
         range = 8 * (1 << (s->f_code - 1));
         if (s->out_format == FMT_H263)
             range = range * 2;

         xx = 16 * s->mb_x;
         yy = 16 * s->mb_y;
         x1 = xx - range;
         if (x1 < 0)
             x1 = 0;
         x2 = xx + range - 1;
         if (x2 > (s->width - 16))
             x2 = s->width - 16;
         y1 = yy - range;
         if (y1 < 0)
             y1 = 0;
         y2 = yy + range - 1;
         if (y2 > (s->height - 16))
             y2 = s->height - 16;

         *mx_ptr = (random()%(2 * (x2 - x1 + 1))) + 2 * (x1 - xx);
         *my_ptr = (random()%(2 * (y2 - y1 + 1))) + 2 * (y1 - yy);
         return 0;
    } else {
        *mx_ptr = 0;
        *my_ptr = 0;
        return 1;
    }
}

#endif
