/*
 * DSP utils
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

#define avg2(a,b) ((a+b+1)>>1)
#define avg4(a,b,c,d) ((a+b+c+d+2)>>2)

static UINT8 cropTbl[256 + 2 * MAX_NEG_CROP];
UINT32 squareTbl[512];

void dsputil_init(void)
{
    int i;

    for(i=0;i<256;i++) cropTbl[i + MAX_NEG_CROP] = i;
    for(i=0;i<MAX_NEG_CROP;i++) {
        cropTbl[i] = 0;
        cropTbl[i + MAX_NEG_CROP + 256] = 255;
    }

    for(i=0;i<512;i++) {
        squareTbl[i] = (i - 256) * (i - 256);
    }
}

void get_pixels(DCTELEM *block, const UINT8 *pixels, int line_size)
{
    DCTELEM *p;
    const UINT8 *pix;
    int i;

    /* read the pixels */
    p = block;
    pix = pixels;
    for(i=0;i<8;i++) {
        p[0] = pix[0];
        p[1] = pix[1];
        p[2] = pix[2];
        p[3] = pix[3];
        p[4] = pix[4];
        p[5] = pix[5];
        p[6] = pix[6];
        p[7] = pix[7];
        pix += line_size;
        p += 8;
    }
}

void put_pixels(const DCTELEM *block, UINT8 *pixels, int line_size)
{
    const DCTELEM *p;
    UINT8 *pix;
    int i;
    UINT8 *cm = cropTbl + MAX_NEG_CROP;
    
    /* read the pixels */
    p = block;
    pix = pixels;
    for(i=0;i<8;i++) {
        pix[0] = cm[p[0]];
        pix[1] = cm[p[1]];
        pix[2] = cm[p[2]];
        pix[3] = cm[p[3]];
        pix[4] = cm[p[4]];
        pix[5] = cm[p[5]];
        pix[6] = cm[p[6]];
        pix[7] = cm[p[7]];
        pix += line_size;
        p += 8;
    }
}

#define PIXOP(OPNAME, OP)                                                                \
                                                                                         \
static void OPNAME ## _pixels(DCTELEM *block, const UINT8 *pixels, int line_size)        \
{                                                                                        \
    DCTELEM *p;                                                                          \
    const UINT8 *pix;                                                                    \
    int i;                                                                               \
                                                                                         \
    p = block;                                                                           \
    pix = pixels;                                                                        \
    for(i=0;i<8;i++) {                                                                   \
        p[0] OP pix[0];                                                                  \
        p[1] OP pix[1];                                                                  \
        p[2] OP pix[2];                                                                  \
        p[3] OP pix[3];                                                                  \
        p[4] OP pix[4];                                                                  \
        p[5] OP pix[5];                                                                  \
        p[6] OP pix[6];                                                                  \
        p[7] OP pix[7];                                                                  \
        pix += line_size;                                                                \
        p += 8;                                                                          \
    }                                                                                    \
}                                                                                        \
                                                                                         \
static void OPNAME ## _pixels_x2(DCTELEM *block, const UINT8 *pixels, int line_size)     \
{                                                                                        \
    DCTELEM *p;                                                                          \
    const UINT8 *pix;                                                                    \
    int i;                                                                               \
                                                                                         \
    p = block;                                                                           \
    pix = pixels;                                                                        \
    for(i=0;i<8;i++) {                                                                   \
        p[0] OP avg2(pix[0], pix[1]);                                                    \
        p[1] OP avg2(pix[1], pix[2]);                                                    \
        p[2] OP avg2(pix[2], pix[3]);                                                    \
        p[3] OP avg2(pix[3], pix[4]);                                                    \
        p[4] OP avg2(pix[4], pix[5]);                                                    \
        p[5] OP avg2(pix[5], pix[6]);                                                    \
        p[6] OP avg2(pix[6], pix[7]);                                                    \
        p[7] OP avg2(pix[7], pix[8]);                                                    \
        pix += line_size;                                                                \
        p += 8;                                                                          \
    }                                                                                    \
}                                                                                        \
                                                                                         \
static void OPNAME ## _pixels_y2(DCTELEM *block, const UINT8 *pixels, int line_size)     \
{                                                                                        \
    DCTELEM *p;                                                                          \
    const UINT8 *pix;                                                                    \
    const UINT8 *pix1;                                                                   \
    int i;                                                                               \
                                                                                         \
    p = block;                                                                           \
    pix = pixels;                                                                        \
    pix1 = pixels + line_size;                                                           \
    for(i=0;i<8;i++) {                                                                   \
        p[0] OP avg2(pix[0], pix1[0]);                                                   \
        p[1] OP avg2(pix[1], pix1[1]);                                                   \
        p[2] OP avg2(pix[2], pix1[2]);                                                   \
        p[3] OP avg2(pix[3], pix1[3]);                                                   \
        p[4] OP avg2(pix[4], pix1[4]);                                                   \
        p[5] OP avg2(pix[5], pix1[5]);                                                   \
        p[6] OP avg2(pix[6], pix1[6]);                                                   \
        p[7] OP avg2(pix[7], pix1[7]);                                                   \
        pix += line_size;                                                                \
        pix1 += line_size;                                                               \
        p += 8;                                                                          \
    }                                                                                    \
}                                                                                        \
                                                                                         \
static void OPNAME ## _pixels_xy2(DCTELEM *block, const UINT8 *pixels, int line_size)    \
{                                                                                        \
    DCTELEM *p;                                                                          \
    const UINT8 *pix;                                                                    \
    const UINT8 *pix1;                                                                   \
    int i;                                                                               \
                                                                                         \
    p = block;                                                                           \
    pix = pixels;                                                                        \
    pix1 = pixels + line_size;                                                           \
    for(i=0;i<8;i++) {                                                                   \
        p[0] OP avg4(pix[0], pix[1], pix1[0], pix1[1]);                                  \
        p[1] OP avg4(pix[1], pix[2], pix1[1], pix1[2]);                                  \
        p[2] OP avg4(pix[2], pix[3], pix1[2], pix1[3]);                                  \
        p[3] OP avg4(pix[3], pix[4], pix1[3], pix1[4]);                                  \
        p[4] OP avg4(pix[4], pix[5], pix1[4], pix1[5]);                                  \
        p[5] OP avg4(pix[5], pix[6], pix1[5], pix1[6]);                                  \
        p[6] OP avg4(pix[6], pix[7], pix1[6], pix1[7]);                                  \
        p[7] OP avg4(pix[7], pix[8], pix1[7], pix1[8]);                                  \
        pix += line_size;                                                                \
        pix1 += line_size;                                                               \
        p += 8;                                                                          \
    }                                                                                    \
}                                                                                        \
                                                                                         \
void (*OPNAME ## _pixels_tab[4])(DCTELEM *block, const UINT8 *pixels, int line_size) = { \
    OPNAME ## _pixels,                                                                   \
    OPNAME ## _pixels_x2,                                                                \
    OPNAME ## _pixels_y2,                                                                \
    OPNAME ## _pixels_xy2,                                                               \
};


PIXOP(add, +=)

PIXOP(sub, -=)

