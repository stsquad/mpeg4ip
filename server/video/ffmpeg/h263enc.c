/*
 * H263 backend for ffmpeg encoder
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
#include <netinet/in.h>
#include "common.h"
#include "dsputil.h"
#include "mpegvideo.h"
#include "h263data.h"
#include "mpeg4data.h"

#define NDEBUG
#include <assert.h>

static void h263_encode_block(MpegEncContext * s, DCTELEM * block,
			      int n);
static void h263_encode_motion(MpegEncContext * s, int val);
static void h263_pred_motion(MpegEncContext * s, int block, int *px, int *py);
static void mpeg4_encode_block(MpegEncContext * s, DCTELEM * block,
			       int n);

void h263_picture_header(MpegEncContext * s, int picture_number)
{
    int format;

    align_put_bits(&s->pb);
    put_bits(&s->pb, 22, 0x20);
    put_bits(&s->pb, 8, ((s->picture_number * 30) / s->frame_rate) & 0xff);

    put_bits(&s->pb, 1, 1);	/* marker */
    put_bits(&s->pb, 1, 0);	/* h263 id */
    put_bits(&s->pb, 1, 0);	/* split screen off */
    put_bits(&s->pb, 1, 0);	/* camera  off */
    put_bits(&s->pb, 1, 0);	/* freeze picture release off */

    if (s->width == 128 && s->height == 96)
	format = 1;
    else if (s->width == 176 && s->height == 144)
	format = 2;
    else if (s->width == 352 && s->height == 288)
	format = 3;
    else if (s->width == 704 && s->height == 576)
	format = 4;
    else if (s->width == 1408 && s->height == 1152)
	format = 5;
    else
	abort();

    put_bits(&s->pb, 3, format);

    put_bits(&s->pb, 1, (s->pict_type == P_TYPE));

    put_bits(&s->pb, 1, 0);	/* unrestricted motion vector: off */

    put_bits(&s->pb, 1, 0);	/* SAC: off */

    put_bits(&s->pb, 1, 0);	/* advanced prediction mode: off */

    put_bits(&s->pb, 1, 0);	/* not PB frame */

    put_bits(&s->pb, 5, s->qscale);

    put_bits(&s->pb, 1, 0);	/* Continuous Presence Multipoint mode: off */

    put_bits(&s->pb, 1, 0);	/* no PEI */
}

void h263_encode_mb(MpegEncContext * s,
		    DCTELEM block[6][64],
		    int motion_x, int motion_y)
{
    int cbpc, cbpy, i, cbp, pred_x, pred_y;

    if (!s->mb_intra) {
	/* compute cbp */
	cbp = 0;
	for (i = 0; i < 6; i++) {
	    if (s->block_last_index[i] >= 0)
		cbp |= 1 << (5 - i);
	}
	if ((cbp | motion_x | motion_y) == 0) {
	    /* skip macroblock */
	    put_bits(&s->pb, 1, 1);
	    return;
	}
	put_bits(&s->pb, 1, 0);	/* mb coded */
	cbpc = cbp & 3;
	put_bits(&s->pb,
		 inter_MCBPC_bits[cbpc],
		 inter_MCBPC_code[cbpc]);
	cbpy = cbp >> 2;
	cbpy ^= 0xf;
	put_bits(&s->pb, cbpy_tab[cbpy][1], cbpy_tab[cbpy][0]);

	/* motion vectors: 16x16 mode only now */
        h263_pred_motion(s, 0, &pred_x, &pred_y);
        
        h263_encode_motion(s, motion_x - pred_x);
        h263_encode_motion(s, motion_y - pred_y);
    } else {
	/* compute cbp */
	cbp = 0;
	for (i = 0; i < 6; i++) {
	    if (s->block_last_index[i] >= 1)
		cbp |= 1 << (5 - i);
	}

	cbpc = cbp & 3;
	if (s->pict_type == I_TYPE) {
	    put_bits(&s->pb,
		     intra_MCBPC_bits[cbpc],
		     intra_MCBPC_code[cbpc]);
	} else {
	    put_bits(&s->pb, 1, 0);	/* mb coded */
	    put_bits(&s->pb,
		     inter_MCBPC_bits[cbpc + 4],
		     inter_MCBPC_code[cbpc + 4]);
	}
	if (s->h263_pred) {
	    /* XXX: currently, we do not try to use ac prediction */
	    put_bits(&s->pb, 1, 0);	/* no ac prediction */
	}
	cbpy = cbp >> 2;
	put_bits(&s->pb, cbpy_tab[cbpy][1], cbpy_tab[cbpy][0]);
    }

    /* encode each block */
    if (s->h263_pred) {
	for (i = 0; i < 6; i++) {
	    mpeg4_encode_block(s, block[i], i);
	}
    } else {
	for (i = 0; i < 6; i++) {
	    h263_encode_block(s, block[i], i);
	}
    }
}

static inline int mid_pred(int a, int b, int c)
{
    int vmin, vmax;
    vmin = a;
    if (b < vmin)
        vmin = b;
    if (c < vmin)
        vmin = c;

    vmax = a;
    if (b > vmax)
        vmax = b;
    if (c > vmax)
        vmax = c;

    return a + b + c - vmin - vmax;
}

static void h263_pred_motion(MpegEncContext * s, int block, int *px, int *py)
{
    int x, y, wrap;
    INT16 *A, *B, *C;

    x = 2 * s->mb_x + 1 + (block & 1);
    y = 2 * s->mb_y + 1 + ((block >> 1) & 1);
    wrap = 2 * s->mb_width + 2;

    /* special case for first line */
    if (y == 1) {
        A = s->motion_val[(x-1) + (y) * wrap];
        *px = A[0];
        *py = A[1];
        return;
    }

    switch(block) {
    default:
    case 0:
        A = s->motion_val[(x-1) + (y) * wrap];
        B = s->motion_val[(x) + (y-1) * wrap];
        C = s->motion_val[(x+2) + (y-1) * wrap];
        break;
    case 1:
    case 2:
        A = s->motion_val[(x-1) + (y) * wrap];
        B = s->motion_val[(x) + (y-1) * wrap];
        C = s->motion_val[(x+1) + (y-1) * wrap];
        break;
    case 3:
        A = s->motion_val[(x-1) + (y) * wrap];
        B = s->motion_val[(x-1) + (y-1) * wrap];
        C = s->motion_val[(x) + (y-1) * wrap];
        break;
    }
    *px = mid_pred(A[0], B[0], C[0]);
    *py = mid_pred(A[1], B[1], C[1]);
}


static void h263_encode_motion(MpegEncContext * s, int val)
{
    int range, l, m, bit_size, sign, code, bits;

    if (val == 0) {
        /* zero vector */
        code = 0;
        put_bits(&s->pb, mvtab[code][1], mvtab[code][0]);
    } else {
        bit_size = s->f_code - 1;
        range = 1 << bit_size;
        /* modulo encoding */
        l = range * 32;
        m = 2 * l;
        if (val < -l) {
            val += m;
        } else if (val >= l) {
            val -= m;
        }

        if (val >= 0) {
            val--;
            code = (val >> bit_size) + 1;
            bits = val & (range - 1);
            sign = 0;
        } else {
            val = -val;
            val--;
            code = (val >> bit_size) + 1;
            bits = val & (range - 1);
            sign = 1;
        }

        put_bits(&s->pb, mvtab[code][1] + 1, (mvtab[code][0] << 1) | sign); 
        if (bit_size > 0) {
            put_bits(&s->pb, bit_size, bits);
        }
    }
}


#define CODE_INTRA(run, level, last)             \
{                                                       \
    if (last == 0) {                                    \
        if (run == 0 && level < 28) {                   \
            code = coeff_tab4[level - 1][0];            \
            len = coeff_tab4[level - 1][1];             \
        } else if (run == 1 && level < 11) {            \
            code = coeff_tab5[level - 1][0];            \
            len = coeff_tab5[level - 1][1];             \
        } else if (run > 1 && run < 10 && level < 6) {  \
            code = coeff_tab6[run - 2][level - 1][0];   \
            len = coeff_tab6[run - 2][level - 1][1];    \
        } else if (run > 9 && run < 15 && level == 1) { \
            code = coeff_tab7[run - 10][0];             \
            len = coeff_tab7[run - 10][1];              \
        }                                               \
    } else {                                            \
        if (run == 0 && level < 9) {                    \
            code = coeff_tab8[level - 1][0];            \
            len = coeff_tab8[level - 1][1];             \
        } else if (run > 0 && run < 7 && level < 4) {   \
            code = coeff_tab9[run - 1][level - 1][0];   \
            len = coeff_tab9[run - 1][level - 1][1];    \
        } else if (run > 6 && run < 21 && level == 1) { \
            code = coeff_tab10[run - 7][0];             \
            len = coeff_tab10[run - 7][1];              \
        }                                               \
    }                                                   \
}

#define CODE_INTER(run, level, last)                     \
{                                                               \
    if (last == 0) {                                            \
        if (run < 2 && level < 13) {                            \
            len = coeff_tab0[run][level - 1][1];                \
            code = coeff_tab0[run][level - 1][0];               \
        } else if (run >= 2 && run < 27 && level < 5) {         \
            len = coeff_tab1[run - 2][level - 1][1];            \
            code = coeff_tab1[run - 2][level - 1][0];           \
        }                                                       \
    } else {                                                    \
        if (run < 2 && level < 4) {                             \
            len = coeff_tab2[run][level - 1][1];                \
            code = coeff_tab2[run][level - 1][0];               \
        } else if (run >= 2 && run < 42 && level == 1) {        \
            len = coeff_tab3[run - 2][1];                       \
            code = coeff_tab3[run - 2][0];                      \
        }                                                       \
    }                                                           \
}

static void h263_encode_block(MpegEncContext * s, DCTELEM * block, int n)
{
    int level, run, last, i, j, last_index, last_non_zero, sign, slevel;
    int code = 0, len;

    if (s->mb_intra) {
	/* DC coef */
	level = block[0];
	if (level == 128)
	    put_bits(&s->pb, 8, 0xff);
	else
	    put_bits(&s->pb, 8, level & 0xff);
	i = 1;
    } else {
	i = 0;
    }

    /* AC coefs */
    last_index = s->block_last_index[n];
    last_non_zero = i - 1;
    for (; i <= last_index; i++) {
	j = zigzag_direct[i];
	level = block[j];
	if (level) {
	    run = i - last_non_zero - 1;
	    last = (i == last_index);
	    sign = 0;
	    slevel = level;
	    if (level < 0) {
		sign = 1;
		level = -level;
	    }
            len = 0;
            CODE_INTER(run, level, last);
            if (len == 0) {
                put_bits(&s->pb, 7, 3);
                put_bits(&s->pb, 1, last);
                put_bits(&s->pb, 6, run);
                put_bits(&s->pb, 8, slevel & 0xff);
            } else {
                code = (code << 1) | sign;
                put_bits(&s->pb, len + 1, code);
            }
	    last_non_zero = i;
	}
    }
}

/* write RV 1.0 compatible frame header */
void rv10_encode_picture_header(MpegEncContext * s, int picture_number)
{
    align_put_bits(&s->pb);

    put_bits(&s->pb, 1, 1);	/* marker */

    put_bits(&s->pb, 1, (s->pict_type == P_TYPE));

    put_bits(&s->pb, 1, 0);	/* not PB frame */

    put_bits(&s->pb, 5, s->qscale);

    if (s->pict_type == I_TYPE) {
	/* specific MPEG like DC coding not used */
    }
    /* if multiple packets per frame are sent, the position at which
       to display the macro blocks is coded here */
    put_bits(&s->pb, 6, 0);	/* mb_x */
    put_bits(&s->pb, 6, 0);	/* mb_y */
    put_bits(&s->pb, 12, s->mb_width * s->mb_height);

    put_bits(&s->pb, 3, 0);	/* ignored */
}

/***************************************************/

/* write mpeg4 VOP header */
void mpeg4_encode_picture_header(MpegEncContext * s, int picture_number)
{
    align_put_bits(&s->pb);

    put_bits(&s->pb, 32, 0x1B6);	/* vop header */
    put_bits(&s->pb, 2, s->pict_type - 1);	/* pict type: I = 0 , P = 1 */
    /* XXX: time base + 1 not always correct */
    put_bits(&s->pb, 1, 1);
    put_bits(&s->pb, 1, 0);

    put_bits(&s->pb, 1, 1);	/* marker */
    put_bits(&s->pb, 4, 1);	/* XXX: correct time increment */
    put_bits(&s->pb, 1, 1);	/* marker */
    put_bits(&s->pb, 1, 1);	/* vop coded */
    if (s->pict_type == P_TYPE)
	put_bits(&s->pb, 1, 0);	/* rounding type */
    put_bits(&s->pb, 3, 0);	/* intra dc VLC threshold */

    put_bits(&s->pb, 5, s->qscale);

    if (s->pict_type != I_TYPE)
	put_bits(&s->pb, 3, s->f_code);	/* fcode_for */
}

void h263_dc_scale(MpegEncContext * s)
{
    int quant;

    quant = s->qscale;
    /* luminance */
    if (quant < 5)
	s->y_dc_scale = 8;
    else if (quant > 4 && quant < 9)
	s->y_dc_scale = (2 * quant);
    else if (quant > 8 && quant < 25)
	s->y_dc_scale = (quant + 8);
    else
	s->y_dc_scale = (2 * quant - 16);
    /* chrominance */
    if (quant < 5)
	s->c_dc_scale = 8;
    else if (quant > 4 && quant < 25)
	s->c_dc_scale = ((quant + 13) / 2);
    else
	s->c_dc_scale = (quant - 6);
}

static inline void mpeg4_encode_dc(MpegEncContext * s, int level, int n)
{
    int size, v, a, b, c, x, y, wrap, pred, scale;
    UINT16 *dc_val;

    /* find prediction */
    if (n < 4) {
	x = 2 * s->mb_x + 1 + (n & 1);
	y = 2 * s->mb_y + 1 + ((n & 2) >> 1);
	wrap = s->mb_width * 2 + 2;
	dc_val = s->dc_val[0];
	scale = s->y_dc_scale;
    } else {
	x = s->mb_x + 1;
	y = s->mb_y + 1;
	wrap = s->mb_width + 2;
	dc_val = s->dc_val[n - 4 + 1];
	scale = s->c_dc_scale;
    }

    /* B C
     * A X 
     */
    a = dc_val[(x - 1) + (y) * wrap];
    b = dc_val[(x - 1) + (y - 1) * wrap];
    c = dc_val[(x) + (y - 1) * wrap];

    if (abs(a - b) < abs(b - c)) {
	pred = c;
    } else {
	pred = a;
    }
    /* we assume pred is positive */
    pred = (pred + (scale >> 1)) / scale;

    /* update predictor */
    dc_val[(x) + (y) * wrap] = level * scale;

    /* do the prediction */
    level -= pred;

    /* find number of bits */
    size = 0;
    v = abs(level);
    while (v) {
	v >>= 1;
	size++;
    }

    if (n < 4) {
	/* luminance */
	put_bits(&s->pb, DCtab_lum[size][1], DCtab_lum[size][0]);
    } else {
	/* chrominance */
	put_bits(&s->pb, DCtab_chrom[size][1], DCtab_chrom[size][0]);
    }

    /* encode remaining bits */
    if (size > 0) {
	if (level < 0)
	    level = (-level) ^ ((1 << size) - 1);
	put_bits(&s->pb, size, level);
	if (size > 8)
	    put_bits(&s->pb, 1, 1);
    }
}

static void mpeg4_encode_block(MpegEncContext * s, DCTELEM * block, int n)
{
    int level, run, last, i, j, last_index, last_non_zero, sign, slevel, level1, run1;
    int code = 0, len;

    if (s->mb_intra) {
	/* mpeg4 based DC predictor */
	mpeg4_encode_dc(s, block[0], n);
	i = 1;
    } else {
	i = 0;
    }

    /* AC coefs */
    last_index = s->block_last_index[n];
    last_non_zero = i - 1;
    for (; i <= last_index; i++) {
	j = zigzag_direct[i];
	level = block[j];
	if (level) {
	    run = i - last_non_zero - 1;
	    last = (i == last_index);
	    sign = 0;
	    slevel = level;
	    if (level < 0) {
		sign = 1;
		level = -level;
	    }
            len = 0;
	    if (s->mb_intra) {
		/* intra mb */
                CODE_INTRA(run, level, last);
                if (len)
                    goto esc0;
                /* first escape */
                level1 = level - intra_max_level[last][run];
                assert(level1 >= 0);
                if (level1 < 28) {
                    CODE_INTRA(run, level1, last);
                    if (len)
                        goto esc1;
                }
                /* second escape */
                run1 = run - (intra_max_run[last][level]+1);
                if (level < 28 && run1 >= 0) {
                    CODE_INTRA(run1, level, last);
                    if (len)
                        goto esc2;
                }
	    } else {
		/* inter mb */
                CODE_INTER(run, level, last);
                if (len)
                    goto esc0;
                /* first escape */
                level1 = level - inter_max_level[last][run];
                assert(level1 >= 0);
                if (level1 < 13) {
                    CODE_INTER(run, level1, last);
                    if (len)
                        goto esc1;
                }
                /* second escape */
                run1 = run - (inter_max_run[last][level]+1);
                if (level < 13 && run1 >= 0) {
                    CODE_INTER(run1, level, last);
                    if (len)
                        goto esc2;
                }
            }
            /* third escape */
            put_bits(&s->pb, 7, 3);
            put_bits(&s->pb, 2, 3);
            put_bits(&s->pb, 1, last);
            put_bits(&s->pb, 6, run);
            put_bits(&s->pb, 1, 1); /* marker */
            put_bits(&s->pb, 12, slevel & 0xfff);
            put_bits(&s->pb, 1, 1); /* marker */
            goto next;
        esc2:
            put_bits(&s->pb, 7, 3);
            put_bits(&s->pb, 2, 2);
            goto esc0;
        esc1:
            put_bits(&s->pb, 7, 3);
            put_bits(&s->pb, 1, 0);
        esc0:
            code = (code << 1) | sign;
            put_bits(&s->pb, len + 1, code);
        next:
	    last_non_zero = i;
	}
    }
}
