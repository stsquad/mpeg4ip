/*
 * Adapted from:
 *
 * H263/MPEG4 backend for ffmpeg encoder and decoder
 * Copyright (c) 2000,2001 Gerard Lantau.
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
#include "../portab.h"
#include "bitstream.h"
#include "h263.h"

/* VLC's */

#define MAX_RUN    64
#define MAX_LEVEL  64

/* run length table */
typedef struct RLTable {
    int n; /* number of entries of table_vlc minus 1 */
    int last; /* number of values for last = 0 */
    const UINT16 (*table_vlc)[2];
    const INT8 *table_run;
    const INT8 *table_level;
    UINT8 *index_run[2]; /* encoding only */
    INT8 *max_level[2]; /* encoding & decoding */
    INT8 *max_run[2];   /* encoding & decoding */
    H263_VLC vlc;            /* decoding only */
} RLTable;

static const UINT16 inter_vlc[103][2] = {
{ 0x2, 2 },{ 0xf, 4 },{ 0x15, 6 },{ 0x17, 7 },
{ 0x1f, 8 },{ 0x25, 9 },{ 0x24, 9 },{ 0x21, 10 },
{ 0x20, 10 },{ 0x7, 11 },{ 0x6, 11 },{ 0x20, 11 },
{ 0x6, 3 },{ 0x14, 6 },{ 0x1e, 8 },{ 0xf, 10 },
{ 0x21, 11 },{ 0x50, 12 },{ 0xe, 4 },{ 0x1d, 8 },
{ 0xe, 10 },{ 0x51, 12 },{ 0xd, 5 },{ 0x23, 9 },
{ 0xd, 10 },{ 0xc, 5 },{ 0x22, 9 },{ 0x52, 12 },
{ 0xb, 5 },{ 0xc, 10 },{ 0x53, 12 },{ 0x13, 6 },
{ 0xb, 10 },{ 0x54, 12 },{ 0x12, 6 },{ 0xa, 10 },
{ 0x11, 6 },{ 0x9, 10 },{ 0x10, 6 },{ 0x8, 10 },
{ 0x16, 7 },{ 0x55, 12 },{ 0x15, 7 },{ 0x14, 7 },
{ 0x1c, 8 },{ 0x1b, 8 },{ 0x21, 9 },{ 0x20, 9 },
{ 0x1f, 9 },{ 0x1e, 9 },{ 0x1d, 9 },{ 0x1c, 9 },
{ 0x1b, 9 },{ 0x1a, 9 },{ 0x22, 11 },{ 0x23, 11 },
{ 0x56, 12 },{ 0x57, 12 },{ 0x7, 4 },{ 0x19, 9 },
{ 0x5, 11 },{ 0xf, 6 },{ 0x4, 11 },{ 0xe, 6 },
{ 0xd, 6 },{ 0xc, 6 },{ 0x13, 7 },{ 0x12, 7 },
{ 0x11, 7 },{ 0x10, 7 },{ 0x1a, 8 },{ 0x19, 8 },
{ 0x18, 8 },{ 0x17, 8 },{ 0x16, 8 },{ 0x15, 8 },
{ 0x14, 8 },{ 0x13, 8 },{ 0x18, 9 },{ 0x17, 9 },
{ 0x16, 9 },{ 0x15, 9 },{ 0x14, 9 },{ 0x13, 9 },
{ 0x12, 9 },{ 0x11, 9 },{ 0x7, 10 },{ 0x6, 10 },
{ 0x5, 10 },{ 0x4, 10 },{ 0x24, 11 },{ 0x25, 11 },
{ 0x26, 11 },{ 0x27, 11 },{ 0x58, 12 },{ 0x59, 12 },
{ 0x5a, 12 },{ 0x5b, 12 },{ 0x5c, 12 },{ 0x5d, 12 },
{ 0x5e, 12 },{ 0x5f, 12 },{ 0x3, 7 },
};

static const INT8 inter_level[102] = {
  1,  2,  3,  4,  5,  6,  7,  8,
  9, 10, 11, 12,  1,  2,  3,  4,
  5,  6,  1,  2,  3,  4,  1,  2,
  3,  1,  2,  3,  1,  2,  3,  1,
  2,  3,  1,  2,  1,  2,  1,  2,
  1,  2,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  2,  3,  1,  2,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,
};

static const INT8 inter_run[102] = {
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  1,  1,  1,  1,
  1,  1,  2,  2,  2,  2,  3,  3,
  3,  4,  4,  4,  5,  5,  5,  6,
  6,  6,  7,  7,  8,  8,  9,  9,
 10, 10, 11, 12, 13, 14, 15, 16,
 17, 18, 19, 20, 21, 22, 23, 24,
 25, 26,  0,  0,  0,  1,  1,  2,
  3,  4,  5,  6,  7,  8,  9, 10,
 11, 12, 13, 14, 15, 16, 17, 18,
 19, 20, 21, 22, 23, 24, 25, 26,
 27, 28, 29, 30, 31, 32, 33, 34,
 35, 36, 37, 38, 39, 40,
};

static RLTable h263_rl_table = {
    102,
    58,
    inter_vlc,
    inter_run,
    inter_level,
};

static void init_rl(RLTable *rl)
{
    INT8 max_level[MAX_RUN+1], max_run[MAX_LEVEL+1];
    UINT8 index_run[MAX_RUN+1];
    int last, run, level, start, end, i;

    /* compute max_level[], max_run[] and index_run[] */
    for(last=0;last<2;last++) {
        if (last == 0) {
            start = 0;
            end = rl->last;
        } else {
            start = rl->last;
            end = rl->n;
        }

        memset(max_level, 0, MAX_RUN + 1);
        memset(max_run, 0, MAX_LEVEL + 1);
        memset(index_run, rl->n, MAX_RUN + 1);
        for(i=start;i<end;i++) {
            run = rl->table_run[i];
            level = rl->table_level[i];
            if (index_run[run] == rl->n)
                index_run[run] = i;
            if (level > max_level[run])
                max_level[run] = level;
            if (run > max_run[level])
                max_run[level] = run;
        }
        rl->max_level[last] = malloc(MAX_RUN + 1);
        memcpy(rl->max_level[last], max_level, MAX_RUN + 1);
        rl->max_run[last] = malloc(MAX_LEVEL + 1);
        memcpy(rl->max_run[last], max_run, MAX_LEVEL + 1);
        rl->index_run[last] = malloc(MAX_RUN + 1);
        memcpy(rl->index_run[last], index_run, MAX_RUN + 1);
    }
}

inline int get_rl_index(const RLTable *rl, int last, int run, int level)
{
    int index;
    index = rl->index_run[last][run];
    if (index >= rl->n)
        return rl->n;
    if (level > rl->max_level[last][run])
        return rl->n;
    return index + level - 1;
}

/* Encoding */

void h263_encode_coeff(
	Bitstream *bs,
	int16_t block[64],
	const uint16_t *zigzag,
	uint16_t intra)
{
    RLTable *rl = &h263_rl_table;
    static int once = 0;
    int level, run, last, i, j, last_index, last_non_zero, sign, slevel;
    int code;

    if (!once) {
        once = 1;
        init_rl(&h263_rl_table);
    }

    if (intra) {
	i = 1;
    } else {
	i = 0;
    }

    last_index = i;
    for (j = 63; j >= i; j--) {
	if (block[zigzag[j]] != 0) {
	    last_index = j;
	    break;
	}
    }

    last_non_zero = i - 1;

    for (; i <= last_index; i++) {
	j = zigzag[i];
	level = block[j];
	if (level) {
// DEBUG
printf("xvid i %d j %d bspos %d ", i, j, BitstreamPos(bs));
	    run = i - last_non_zero - 1;
	    last = (i == last_index);
	    sign = 0;
	    slevel = level;
	    if (level < 0) {
		sign = 1;
		level = -level;
	    }
            code = get_rl_index(rl, last, run, level);
            BitstreamPutBits(bs,
		rl->table_vlc[code][0], rl->table_vlc[code][1]);
            if (code == rl->n) {
                BitstreamPutBits(bs, last, 1);
                BitstreamPutBits(bs, run, 6);
                BitstreamPutBits(bs, slevel & 0xff, 8);
            } else {
                BitstreamPutBits(bs, sign, 1);
            }
// DEBUG
printf("code %d last %d run %d level %d vlc 0x%x (%d bits)\n",
	  code, last, run, level, rl->table_vlc[code][0], rl->table_vlc[code][1]);
	    last_non_zero = i;
	}
    }
}

#ifdef TBD
/* Decoding */

static void init_vlc_rl(RLTable *rl)
{
    init_vlc(&rl->vlc, 9, rl->n + 1, 
             &rl->table_vlc[0][1], 4, 2,
             &rl->table_vlc[0][0], 4, 2);
}

void h263_decode_init_vlc()
{
    static int done = 0;

    if (!done) {
        done = 1;
        init_rl(&h263_rl_table);
        init_vlc_rl(&h263_rl_table);
    }
}

int h263_decode_block(MpegEncContext * s, DCTELEM * block,
                             int n, int coded)
{
    int code, level, i, j, last, run;
    RLTable *rl = &h263_rl_table;

    if (s->mb_intra) {
	/* DC coef */
	level = get_bits(&s->gb, 8);
	if (level == 255)
		level = 128;
	block[0] = level;
	i = 1;
    } else {
	i = 0;
    }
    if (!coded) {
        s->block_last_index[n] = i - 1;
        return 0;
    }

    for(;;) {
        code = get_vlc(&s->gb, &rl->vlc);
        if (code < 0)
            return -1;
        if (code == rl->n) {
            /* escape */
            last = get_bits1(&s->gb);
            run = get_bits(&s->gb, 6);
            level = (INT8)get_bits(&s->gb, 8);
            if (s->h263_rv10 && level == -128) {
                /* XXX: should patch encoder too */
                level = get_bits(&s->gb, 12);
                level = (level << 20) >> 20;
            }
        } else {
            run = rl->table_run[code];
            level = rl->table_level[code];
            last = code >= rl->last;
            if (get_bits1(&s->gb))
                level = -level;
        }
        i += run;
        if (i >= 64)
            return -1;
	j = zigzag_direct[i];
        block[j] = level;
        if (last)
            break;
        i++;
    }
    s->block_last_index[n] = i;
    return 0;
}

#endif /* TBD */
