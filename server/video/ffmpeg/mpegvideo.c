/*
 * The simplest mpeg encoder
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
#include <math.h>
#include "avcodec.h"
#include "dsputil.h"
#include "mpegvideo.h"

/* enable all paranoid tests for rounding, overflows, etc... */
//#define PARANOID

//#define DEBUG

/* for jpeg fast DCT */
#define CONST_BITS 14

static const unsigned short aanscales[64] = {
    /* precomputed values scaled up by 14 bits */
    16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
    22725, 31521, 29692, 26722, 22725, 17855, 12299,  6270,
    21407, 29692, 27969, 25172, 21407, 16819, 11585,  5906,
    19266, 26722, 25172, 22654, 19266, 15137, 10426,  5315,
    16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
    12873, 17855, 16819, 15137, 12873, 10114,  6967,  3552,
    8867, 12299, 11585, 10426,  8867,  6967,  4799,  2446,
    4520,  6270,  5906,  5315,  4520,  3552,  2446,  1247
};

static void encode_picture(MpegEncContext *s, int picture_number);
static void rate_control_init(MpegEncContext *s);
static int rate_estimate_qscale(MpegEncContext *s);
static void mpeg1_skip_picture(MpegEncContext *s, int pict_num);

#include "mpegencodevlc.h"

static void put_header(MpegEncContext *s, int header)
{
    align_put_bits(&s->pb);
    put_bits(&s->pb, 32, header);
}

static void convert_matrix(int *qmat, const UINT8 *quant_matrix, int qscale)
{
    int i;

    for(i=0;i<64;i++) {
        qmat[i] = (int)((1 << 22) * 16384.0 / (aanscales[i] * qscale * quant_matrix[i]));
    }
}


int MPV_encode_init(AVEncodeContext *avctx)
{
    MpegEncContext *s = avctx->priv_data;
    int pict_size, c_size;
    UINT8 *pict;

    dsputil_init();

    s->bit_rate = avctx->bit_rate;
    s->frame_rate = avctx->rate;
    s->width = avctx->width;
    s->height = avctx->height;
    s->gop_size = avctx->gop_size;
    if (s->gop_size <= 1) {
        s->intra_only = 1;
        s->gop_size = 12;
    } else {
        s->intra_only = 0;
    }
    s->full_search = 0;
    if (avctx->flags & CODEC_FLAG_HQ)
        s->full_search = 1;

    switch(avctx->codec->id) {
    case CODEC_ID_MPEG1VIDEO:
        s->out_format = FMT_MPEG1;
        break;
    case CODEC_ID_MJPEG:
        s->out_format = FMT_MJPEG;
        s->intra_only = 1; /* force intra only for jpeg */
        if (mjpeg_init(s) < 0)
            return -1;
        break;
    case CODEC_ID_H263:
        s->out_format = FMT_H263;
        break;
    case CODEC_ID_RV10:
        s->out_format = FMT_H263;
        s->h263_rv10 = 1;
        break;
    case CODEC_ID_DIVX:
        s->out_format = FMT_H263;
        s->h263_pred = 1;
        break;
    default:
        return -1;
    }

    switch(s->frame_rate) {
    case 24:
        s->frame_rate_index = 2;
        break;
    case 25:
        s->frame_rate_index = 3;
        break;
    case 30:
        s->frame_rate_index = 5;
        break;
    case 50:
        s->frame_rate_index = 6;
        break;
    case 60:
        s->frame_rate_index = 8;
        break;
    default:
        /* we accept lower frame rates than 24 for low bit rate mpeg */
        if (s->frame_rate >= 1 && s->frame_rate < 24) {
            s->frame_rate_index = 2;
        } else {
            return -1;
        }
        break;
    }

    /* init */
    s->mb_width = s->width / 16;
    s->mb_height = s->height / 16;
    
    c_size = s->width * s->height;
    pict_size = (c_size * 3) / 2;
    pict = malloc(pict_size);
    if (pict == NULL)
        goto fail;
    s->last_picture[0] = pict;
    s->last_picture[1] = pict + c_size;
    s->last_picture[2] = pict + c_size + (c_size / 4);
    
    pict = malloc(pict_size);
    if (pict == NULL) 
        goto fail;
    s->current_picture[0] = pict;
    s->current_picture[1] = pict + c_size;
    s->current_picture[2] = pict + c_size + (c_size / 4);

    if (s->out_format == FMT_H263) {
        int size;
        /* MV prediction */
        size = (2 * s->mb_width + 2) * (2 * s->mb_height + 2);
        s->motion_val = malloc(size * 2 * sizeof(INT16));
        if (s->motion_val == NULL)
            goto fail;
        memset(s->motion_val, 0, size * 2 * sizeof(INT16));
    }

    if (s->h263_pred) {
        int y_size, c_size, i, size;

        y_size = (2 * s->mb_width + 2) * (2 * s->mb_height + 2);
        c_size = (s->mb_width + 2) * (s->mb_height + 2);
        size = y_size + 2 * c_size;
        s->dc_val[0] = malloc(size * sizeof(INT16));
        if (s->dc_val[0] == NULL)
            goto fail;
        s->dc_val[1] = s->dc_val[0] + y_size;
        s->dc_val[2] = s->dc_val[1] + c_size;
        for(i=0;i<size;i++)
            s->dc_val[0][i] = 1024;
    }

    /* rate control init */
    rate_control_init(s);

    s->picture_number = 0;
    s->fake_picture_number = 0;
    /* motion detector init */
    s->f_code = 1;

    return 0;
 fail:
    if (s->motion_val)
        free(s->motion_val);
    if (s->dc_val[0])
        free(s->dc_val[0]);
    if (s->last_picture[0])
        free(s->last_picture[0]);
    if (s->current_picture[0])
        free(s->current_picture[0]);
    return -1;
}

int MPV_encode_end(AVEncodeContext *avctx)
{
    MpegEncContext *s = avctx->priv_data;
#if 0
    /* end of sequence */
    if (s->out_format == FMT_MPEG1) {
        put_header(s, SEQ_END_CODE);
    }

    if (!s->flush_frames)
        flush_put_bits(&s->pb);
#endif    
    if (s->motion_val)
        free(s->motion_val);
    if (s->h263_pred)
        free(s->dc_val[0]);
    free(s->last_picture[0]);
    free(s->current_picture[0]);
    if (s->out_format == FMT_MJPEG)
        mjpeg_close(s);
    return 0;
}

int MPV_encode_picture(AVEncodeContext *avctx,
                       unsigned char *buf, int buf_size, void *data)
{
    MpegEncContext *s = avctx->priv_data;
    int i;

    memcpy(s->new_picture, data, 3 * sizeof(UINT8 *));

    init_put_bits(&s->pb, buf, buf_size, NULL, NULL);

    /* group of picture */
    if (s->out_format == FMT_MPEG1) {
        unsigned int vbv_buffer_size;
        unsigned int time_code, fps, n;

        if ((s->picture_number % s->gop_size) == 0) {
            /* mpeg1 header repeated every gop */
            put_header(s, SEQ_START_CODE);
            
            put_bits(&s->pb, 12, s->width);
            put_bits(&s->pb, 12, s->height);
            put_bits(&s->pb, 4, 1); /* 1/1 aspect ratio */
            put_bits(&s->pb, 4, s->frame_rate_index);
            put_bits(&s->pb, 18, 0x3ffff);
            put_bits(&s->pb, 1, 1); /* marker */
            /* vbv buffer size: slightly greater than an I frame. We add
               some margin just in case */
            vbv_buffer_size = (3 * s->I_frame_bits) / (2 * 8);
            put_bits(&s->pb, 10, (vbv_buffer_size + 16383) / 16384); 
            put_bits(&s->pb, 1, 1); /* constrained parameter flag */
            put_bits(&s->pb, 1, 0); /* no custom intra matrix */
            put_bits(&s->pb, 1, 0); /* no custom non intra matrix */

            put_header(s, GOP_START_CODE);
            put_bits(&s->pb, 1, 0); /* do drop frame */
            /* time code : we must convert from the real frame rate to a
               fake mpeg frame rate in case of low frame rate */
            fps = frame_rate_tab[s->frame_rate_index];
            time_code = s->fake_picture_number;
            s->gop_picture_number = time_code;
            put_bits(&s->pb, 5, (time_code / (fps * 3600)) % 24);
            put_bits(&s->pb, 6, (time_code / (fps * 60)) % 60);
            put_bits(&s->pb, 1, 1);
            put_bits(&s->pb, 6, (time_code / fps) % 60);
            put_bits(&s->pb, 6, (time_code % fps));
            put_bits(&s->pb, 1, 1); /* closed gop */
            put_bits(&s->pb, 1, 0); /* broken link */
        }

        if (s->frame_rate < 24 && s->picture_number > 0) {
            /* insert empty P pictures to slow down to the desired
               frame rate. Each fake pictures takes about 20 bytes */
            fps = frame_rate_tab[s->frame_rate_index];
            n = ((s->picture_number * fps) / s->frame_rate) - 1;
            while (s->fake_picture_number < n) {
                mpeg1_skip_picture(s, s->fake_picture_number - 
                                   s->gop_picture_number); 
                s->fake_picture_number++;
            }

        }
        s->fake_picture_number++;
    }
    
    
    if (!s->intra_only) {
        /* first picture of GOP is intra */
        if ((s->picture_number % s->gop_size) == 0)
            s->pict_type = I_TYPE;
        else
            s->pict_type = P_TYPE;
    } else {
        s->pict_type = I_TYPE;
    }
    avctx->key_frame = (s->pict_type == I_TYPE);
    
    encode_picture(s, s->picture_number);
    
    /* swap current and last picture */
    for(i=0;i<3;i++) {
        UINT8 *tmp;
        
        tmp = s->last_picture[i];
        s->last_picture[i] = s->current_picture[i];
        s->current_picture[i] = tmp;
    }
    s->picture_number++;

    if (s->out_format == FMT_MJPEG)
        mjpeg_picture_trailer(s);

    flush_put_bits(&s->pb);
    s->total_bits += (s->pb.buf_ptr - s->pb.buf) * 8;
    avctx->quality = s->qscale;
    return s->pb.buf_ptr - s->pb.buf;
}

/* insert a fake P picture */
static void mpeg1_skip_picture(MpegEncContext *s, int pict_num)
{
    unsigned int mb_incr;

    /* mpeg1 picture header */
    put_header(s, PICTURE_START_CODE);
    /* temporal reference */
    put_bits(&s->pb, 10, pict_num & 0x3ff); 
    
    put_bits(&s->pb, 3, P_TYPE);
    put_bits(&s->pb, 16, 0xffff); /* non constant bit rate */
    
    put_bits(&s->pb, 1, 1); /* integer coordinates */
    put_bits(&s->pb, 3, 1); /* forward_f_code */
    
    put_bits(&s->pb, 1, 0); /* extra bit picture */
    
    /* only one slice */
    put_header(s, SLICE_MIN_START_CODE);
    put_bits(&s->pb, 5, 1); /* quantizer scale */
    put_bits(&s->pb, 1, 0); /* slice extra information */
    
    mb_incr = 1;
    put_bits(&s->pb, mbAddrIncrTable[mb_incr][1], 
             mbAddrIncrTable[mb_incr][0]);
    
    /* empty macroblock */
    put_bits(&s->pb, 3, 1); /* motion only */
    
    /* zero motion x & y */
    put_bits(&s->pb, 1, 1); 
    put_bits(&s->pb, 1, 1); 

    /* output a number of empty slice */
    mb_incr = s->mb_width * s->mb_height - 1;
    while (mb_incr > 33) {
        put_bits(&s->pb, 11, 0x008);
        mb_incr -= 33;
    }
    put_bits(&s->pb, mbAddrIncrTable[mb_incr][1], 
             mbAddrIncrTable[mb_incr][0]);
    
    /* empty macroblock */
    put_bits(&s->pb, 3, 1); /* motion only */
    
    /* zero motion x & y */
    put_bits(&s->pb, 1, 1); 
    put_bits(&s->pb, 1, 1); 
}

static int dct_quantize(MpegEncContext *s, DCTELEM *block, int n, int qscale);
static void encode_block(MpegEncContext *s, 
                         DCTELEM *block, 
                         int component);
static void dct_unquantize(MpegEncContext *s, DCTELEM *block, int n, int qscale);
static void mpeg1_encode_mb(MpegEncContext *s, int mb_x, int mb_y,
                            DCTELEM block[6][64],
                            int motion_x, int motion_y);

static void encode_picture(MpegEncContext *s, int picture_number)
{
    int mb_x, mb_y;
    UINT8 *ptr;
    DCTELEM block[6][64];
    int i, motion_x, motion_y;

    s->picture_number = picture_number;
#if 1
    s->qscale = rate_estimate_qscale(s);
#else
    s->qscale = 10;
#endif
    /* precompute matrix */
    if (s->out_format == FMT_MJPEG) {
        /* for mjpeg, we do include qscale in the matrix */
        s->init_intra_matrix[0] = default_intra_matrix[0];
        for(i=1;i<64;i++)
            s->init_intra_matrix[i] = (default_intra_matrix[i] * s->qscale) >> 3;
        convert_matrix(s->intra_matrix, s->init_intra_matrix, 8);
    } else {
        convert_matrix(s->intra_matrix, default_intra_matrix, s->qscale);
        convert_matrix(s->non_intra_matrix, default_non_intra_matrix, s->qscale);
    }

    switch(s->out_format) {
    case FMT_MJPEG:
        mjpeg_picture_header(s);
        break;
    case FMT_H263:
        if (s->h263_pred) 
            mpeg4_encode_picture_header(s, picture_number);
        else if (s->h263_rv10) 
            rv10_encode_picture_header(s, picture_number);
        else
            h263_picture_header(s, picture_number);
        break;
    case FMT_MPEG1:
        /* mpeg1 picture header */
        put_header(s, PICTURE_START_CODE);
        /* temporal reference */
        put_bits(&s->pb, 10, (s->fake_picture_number - 
                              s->gop_picture_number) & 0x3ff); 
        
        put_bits(&s->pb, 3, s->pict_type);
        put_bits(&s->pb, 16, 0xffff); /* non constant bit rate */
        
        if (s->pict_type == P_TYPE) {
            put_bits(&s->pb, 1, 0); /* half pel coordinates */
            put_bits(&s->pb, 3, s->f_code); /* forward_f_code */
        }
        
        put_bits(&s->pb, 1, 0); /* extra bit picture */
        
        /* only one slice */
        put_header(s, SLICE_MIN_START_CODE);
        put_bits(&s->pb, 5, s->qscale); /* quantizer scale */
        put_bits(&s->pb, 1, 0); /* slice extra information */
        break;
    }
        
    /* init last dc values */
    /* note: quant matrix value (8) is implied here */
    s->last_dc[0] = 128;
    s->last_dc[1] = 128;
    s->last_dc[2] = 128;
    s->mb_incr = 1;
    s->last_motion_x = 0;
    s->last_motion_y = 0;

    for(mb_y=0; mb_y < s->mb_height; mb_y++) {
        for(mb_x=0; mb_x < s->mb_width; mb_x++) {

            s->mb_x = mb_x;
            s->mb_y = mb_y;

            /* compute motion vector and macro block type (intra or non intra) */
            motion_x = 0;
            motion_y = 0;
            if (s->pict_type == P_TYPE) {
                s->mb_intra = estimate_motion(s, mb_x, mb_y,
                                              &motion_x,
                                              &motion_y);
            } else {
                s->mb_intra = 1;
            }

            /* get the pixels */
            ptr = s->new_picture[0] + (mb_y * 16 * s->width) + mb_x * 16;
            get_pixels(block[0], ptr, s->width);
            get_pixels(block[1], ptr + 8, s->width);
            get_pixels(block[2], ptr + 8 * s->width, s->width);
            get_pixels(block[3], ptr + 8 * s->width + 8, s->width);
            ptr = s->new_picture[1] + (mb_y * 8 * (s->width >> 1)) + mb_x * 8;
            get_pixels(block[4],ptr, s->width >> 1);

            ptr = s->new_picture[2] + (mb_y * 8 * (s->width >> 1)) + mb_x * 8;
            get_pixels(block[5],ptr, s->width >> 1);

            /* subtract previous frame if non intra */
            if (!s->mb_intra) {
                int dxy, offset, mx, my;

                dxy = ((motion_y & 1) << 1) | (motion_x & 1);
                ptr = s->last_picture[0] + 
                    ((mb_y * 16 + (motion_y >> 1)) * s->width) + 
                    (mb_x * 16 + (motion_x >> 1));

                sub_pixels_2(block[0], ptr, s->width, dxy);
                sub_pixels_2(block[1], ptr + 8, s->width, dxy);
                sub_pixels_2(block[2], ptr + s->width * 8, s->width, dxy);
                sub_pixels_2(block[3], ptr + 8 + s->width * 8, s->width ,dxy);

                if (s->out_format == FMT_H263) {
                    /* special rounding for h263 */
                    dxy = 0;
                    if ((motion_x & 3) != 0)
                        dxy |= 1;
                    if ((motion_y & 3) != 0)
                        dxy |= 2;
                    mx = motion_x >> 2;
                    my = motion_y >> 2;
                } else {
                    mx = motion_x / 2;
                    my = motion_y / 2;
                    dxy = ((my & 1) << 1) | (mx & 1);
                    mx >>= 1;
                    my >>= 1;
                }
                offset = ((mb_y * 8 + my) * (s->width >> 1)) + (mb_x * 8 + mx);
                ptr = s->last_picture[1] + offset;
                sub_pixels_2(block[4], ptr, s->width >> 1, dxy);
                ptr = s->last_picture[2] + offset;
                sub_pixels_2(block[5], ptr, s->width >> 1, dxy);
            }

            /* DCT & quantize */
            if (s->h263_pred) {
                h263_dc_scale(s);
            } else {
                /* default quantization values */
                s->y_dc_scale = 8;
                s->c_dc_scale = 8;
            }

            for(i=0;i<6;i++) {
                int last_index;
                last_index = dct_quantize(s, block[i], i, s->qscale);
                s->block_last_index[i] = last_index;
            }

            /* huffman encode */
            switch(s->out_format) {
            case FMT_MPEG1:
                mpeg1_encode_mb(s, mb_x, mb_y, block, motion_x, motion_y);
                break;
            case FMT_H263:
                h263_encode_mb(s, block, motion_x, motion_y);
                break;
            case FMT_MJPEG:
                mjpeg_encode_mb(s, block);
                break;
            }

            /* update DC predictors for P macroblocks */
            if (!s->mb_intra) {
                if (s->h263_pred) {
                    int wrap, x, y;
                    wrap = 2 * s->mb_width + 2;
                    x = 2 * mb_x + 1;
                    y = 2 * mb_y + 1;
                    s->dc_val[0][(x) + (y) * wrap] = 1024;
                    s->dc_val[0][(x + 1) + (y) * wrap] = 1024;
                    s->dc_val[0][(x) + (y + 1) * wrap] = 1024;
                    s->dc_val[0][(x + 1) + (y + 1) * wrap] = 1024;
                    wrap = s->mb_width + 2;
                    x = mb_x + 1;
                    y = mb_y + 1;
                    s->dc_val[1][(x) + (y) * wrap] = 1024;
                    s->dc_val[2][(x) + (y) * wrap] = 1024;
                } else {
                    s->last_dc[0] = 128;
                    s->last_dc[1] = 128;
                    s->last_dc[2] = 128;
                }
            }

            /* update motion predictor */
            if (s->out_format == FMT_H263) {
                int x, y, wrap;
                
                x = 2 * mb_x + 1;
                y = 2 * mb_y + 1;
                wrap = 2 * s->mb_width + 2;
                s->motion_val[(x) + (y) * wrap][0] = motion_x;
                s->motion_val[(x) + (y) * wrap][1] = motion_y;
                s->motion_val[(x + 1) + (y) * wrap][0] = motion_x;
                s->motion_val[(x + 1) + (y) * wrap][1] = motion_y;
                s->motion_val[(x) + (y + 1) * wrap][0] = motion_x;
                s->motion_val[(x) + (y + 1) * wrap][1] = motion_y;
                s->motion_val[(x + 1) + (y + 1) * wrap][0] = motion_x;
                s->motion_val[(x + 1) + (y + 1) * wrap][1] = motion_y;
            } else {
                s->last_motion_x = motion_x;
                s->last_motion_y = motion_y;
            }
            
            /* decompress blocks so that we keep the state of the decoder */
            if (!s->intra_only) {
                for(i=0;i<6;i++) {
                    if (s->block_last_index[i] >= 0) {
                        dct_unquantize(s, block[i], i, s->qscale);
                        j_rev_dct (block[i]);
                    }
                }

                if (!s->mb_intra) {
                    int dxy, offset, mx, my;
                    dxy = ((motion_y & 1) << 1) | (motion_x & 1);
                    ptr = s->last_picture[0] + 
                        ((mb_y * 16 + (motion_y >> 1)) * s->width) + 
                        (mb_x * 16 + (motion_x >> 1));
                    
                    add_pixels_2(block[0], ptr, s->width, dxy);
                    add_pixels_2(block[1], ptr + 8, s->width, dxy);
                    add_pixels_2(block[2], ptr + s->width * 8, s->width, dxy);
                    add_pixels_2(block[3], ptr + 8 + s->width * 8, s->width, dxy);

                    if (s->out_format == FMT_H263) {
                        /* special rounding for h263 */
                        dxy = 0;
                        if ((motion_x & 3) != 0)
                            dxy |= 1;
                        if ((motion_y & 3) != 0)
                            dxy |= 2;
                        mx = motion_x >> 2;
                        my = motion_y >> 2;
                    } else {
                        mx = motion_x / 2;
                        my = motion_y / 2;
                        dxy = ((my & 1) << 1) | (mx & 1);
                        mx >>= 1;
                        my >>= 1;
                    }
                    offset = ((mb_y * 8 + my) * (s->width >> 1)) + (mb_x * 8 + mx);
                    ptr = s->last_picture[1] + offset;
                    add_pixels_2(block[4], ptr, s->width >> 1, dxy);
                    ptr = s->last_picture[2] + offset;
                    add_pixels_2(block[5], ptr, s->width >> 1, dxy);
                }

                /* write the pixels */
                ptr = s->current_picture[0] + (mb_y * 16 * s->width) + mb_x * 16;
                put_pixels(block[0], ptr, s->width);
                put_pixels(block[1], ptr + 8, s->width);
                put_pixels(block[2], ptr + 8 * s->width, s->width);
                put_pixels(block[3], ptr + 8 * s->width + 8, s->width);
                ptr = s->current_picture[1] + (mb_y * 8 * (s->width >> 1)) + mb_x * 8;
                put_pixels(block[4],ptr, s->width >> 1);
                
                ptr = s->current_picture[2] + (mb_y * 8 * (s->width >> 1)) + mb_x * 8;
                put_pixels(block[5],ptr, s->width >> 1);
            }
        }
    }
}

static void mpeg1_encode_motion(MpegEncContext *s, int val)
{
    int code, bit_size, l, m, bits, range;

    if (val == 0) {
        /* zero vector */
        code = 0;
        put_bits(&s->pb,
                 mbMotionVectorTable[code + 16][1], 
                 mbMotionVectorTable[code + 16][0]); 
    } else {
        bit_size = s->f_code - 1;
        range = 1 << bit_size;
        /* modulo encoding */
        l = 16 * range;
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
        } else {
            val = -val;
            val--;
            code = (val >> bit_size) + 1;
            bits = val & (range - 1);
            code = -code;
        }
        put_bits(&s->pb,
                 mbMotionVectorTable[code + 16][1], 
                 mbMotionVectorTable[code + 16][0]); 
        if (bit_size > 0) {
            put_bits(&s->pb, bit_size, bits);
        }
    }
}

static void mpeg1_encode_mb(MpegEncContext *s, int mb_x, int mb_y,
                            DCTELEM block[6][64],
                            int motion_x, int motion_y)
{
    int mb_incr, i, cbp;

    /* compute cbp */
    cbp = 0;
    for(i=0;i<6;i++) {
        if (s->block_last_index[i] >= 0)
            cbp |= 1 << (5 - i);
    }

    /* skip macroblock, except if first or last macroblock of a slice */
    if ((cbp | motion_x | motion_y) == 0 &&
        (!((mb_x | mb_y) == 0 ||
           (mb_x == s->mb_width - 1 && mb_y == s->mb_height - 1)))) {
        s->mb_incr++;
    } else {
        /* output mb incr */
        mb_incr = s->mb_incr;

        while (mb_incr > 33) {
            put_bits(&s->pb, 11, 0x008);
            mb_incr -= 33;
        }
        put_bits(&s->pb, mbAddrIncrTable[mb_incr][1], 
                 mbAddrIncrTable[mb_incr][0]);
        
        if (s->pict_type == I_TYPE) {
            put_bits(&s->pb, 1, 1); /* macroblock_type : macroblock_quant = 0 */
        } else {
            if (s->mb_intra) {
                put_bits(&s->pb, 5, 0x03);
            } else {
                if (cbp != 0) {
                    if (motion_x == 0 && motion_y == 0) {
                        put_bits(&s->pb, 2, 1); /* macroblock_pattern only */
                        put_bits(&s->pb, mbPatTable[cbp][1], mbPatTable[cbp][0]);
                    } else {
                        put_bits(&s->pb, 1, 1); /* motion + cbp */
                        mpeg1_encode_motion(s, motion_x - s->last_motion_x); 
                        mpeg1_encode_motion(s, motion_y - s->last_motion_y); 
                        put_bits(&s->pb, mbPatTable[cbp][1], mbPatTable[cbp][0]);
                    }
                } else {
                    put_bits(&s->pb, 3, 1); /* motion only */
                    mpeg1_encode_motion(s, motion_x - s->last_motion_x); 
                    mpeg1_encode_motion(s, motion_y - s->last_motion_y); 
                }
            }
        }
        
        for(i=0;i<6;i++) {
            if (cbp & (1 << (5 - i))) {
                encode_block(s, block[i], i);
            }
        }
        s->mb_incr = 1;
    }
}

static int dct_quantize(MpegEncContext *s, 
                        DCTELEM *block, int n,
                        int qscale)
{
    int i, j, level, last_non_zero, q;
    const int *qmat;

    jpeg_fdct_ifast (block);

    if (s->mb_intra) {
        if (n < 4)
            q = s->y_dc_scale;
        else
            q = s->c_dc_scale;
        q = q << 3;
        
        /* note: block[0] is assumed to be positive */
        block[0] = (block[0] + (q >> 1)) / q;
        i = 1;
        last_non_zero = 0;
        if (s->out_format == FMT_H263) {
            qmat = s->non_intra_matrix;
        } else {
            qmat = s->intra_matrix;
        }
    } else {
        i = 0;
        last_non_zero = -1;
        qmat = s->non_intra_matrix;
    }

    for(;i<64;i++) {
        j = zigzag_direct[i];
        level = block[j];
        /* XXX: overflow can occur here at qscale < 3 */
        level = level * qmat[j];
#ifdef PARANOID
        {
            int level1;
            level1 = ((long long)block[j] * (long long)(qmat[j]));
            if (level1 != level)
                fprintf(stderr, "quant error %d %d\n", level, level1);
        }
#endif
        /* XXX: slight error for the low range */
        //if (level <= -(1 << 22) || level >= (1 << 22)) 
        if (((level << 9) >> 9) != level) {
            level = level / (1 << 22);
            /* XXX: currently, this code is not optimal. the range should be:
               mpeg1: -255..255
               mpeg2: -2048..2047
               h263:  -128..127
               mpeg4: -2048..2047
            */
            if (level > 127)
                level = 127;
            else if (level < -128)
                level = -128;
            block[j] = level;
            last_non_zero = i;
        } else {
            block[j] = 0;
        }
    }
    return last_non_zero;
}

static void dct_unquantize(MpegEncContext *s, 
                           DCTELEM *block, int n, int qscale)
{
    int i, level;
    const UINT8 *quant_matrix;

    if (s->mb_intra) {
        if (n < 4) 
            block[0] = block[0] * s->y_dc_scale;
        else
            block[0] = block[0] * s->c_dc_scale;
        if (s->out_format == FMT_H263) {
            i = 1;
            goto unquant_even;
        }
        quant_matrix = default_intra_matrix;
        for(i=1;i<64;i++) {
            level = block[i];
            if (level) {
                if (level < 0) {
                    level = -level;
                    level = (int)(level * qscale * quant_matrix[i]) >> 3;
                    level = (level - 1) | 1;
                    level = -level;
                } else {
                    level = (int)(level * qscale * quant_matrix[i]) >> 3;
                    level = (level - 1) | 1;
                }
#ifdef PARANOID
                if (level < -2048 || level > 2047)
                    fprintf(stderr, "unquant error %d %d\n", i, level);
#endif
                block[i] = level;
            }
        }
    } else {
        i = 0;
    unquant_even:
        quant_matrix = default_non_intra_matrix;
        for(;i<64;i++) {
            level = block[i];
            if (level) {
                if (level < 0) {
                    level = -level;
                    level = (((level << 1) + 1) * qscale *
                             ((int) (quant_matrix[i]))) >> 4;
                    level = (level - 1) | 1;
                    level = -level;
                } else {
                    level = (((level << 1) + 1) * qscale *
                             ((int) (quant_matrix[i]))) >> 4;
                    level = (level - 1) | 1;
                }
#ifdef PARANOID
                if (level < -2048 || level > 2047)
                    fprintf(stderr, "unquant error %d %d\n", i, level);
#endif
                block[i] = level;
            }
        }
    }
}
                         

static inline void encode_dc(MpegEncContext *s, int diff, int component)
{
    int adiff, index;

    //    printf("dc=%d c=%d\n", diff, component);
    adiff = abs(diff);
    index = vlc_dc_table[adiff];
    if (component == 0) {
        put_bits(&s->pb, vlc_dc_lum_bits[index], vlc_dc_lum_code[index]);
    } else {
        put_bits(&s->pb, vlc_dc_chroma_bits[index], vlc_dc_chroma_code[index]);
    }
    if (diff > 0) {
        put_bits(&s->pb, index, (diff & ((1 << index) - 1)));
    } else if (diff < 0) {
        put_bits(&s->pb, index, ((diff - 1) & ((1 << index) - 1)));
    }
}

static void encode_block(MpegEncContext *s, 
                         DCTELEM *block, 
                         int n)
{
    int alevel, level, last_non_zero, dc, diff, i, j, run, last_index;
    int code, nbits, component;
    
    last_index = s->block_last_index[n];

    /* DC coef */
    if (s->mb_intra) {
        component = (n <= 3 ? 0 : n - 4 + 1);
        dc = block[0]; /* overflow is impossible */
        diff = dc - s->last_dc[component];
        encode_dc(s, diff, component);
        s->last_dc[component] = dc;
        i = 1;
    } else {
        /* encode the first coefficient : needs to be done here because
           it is handled slightly differently */
        level = block[0];
        if (abs(level) == 1) {
                code = ((UINT32)level >> 31); /* the sign bit */
                put_bits(&s->pb, 2, code | 0x02);
                i = 1;
        } else {
            i = 0;
            last_non_zero = -1;
            goto next_coef;
        }
    }

    /* now quantify & encode AC coefs */
    last_non_zero = i - 1;
    for(;i<=last_index;i++) {
        j = zigzag_direct[i];
        level = block[j];
    next_coef:
#if 0
        if (level != 0)
            printf("level[%d]=%d\n", i, level);
#endif            
        /* encode using VLC */
        if (level != 0) {
            run = i - last_non_zero - 1;
            alevel = abs(level);
            //            printf("run=%d level=%d\n", run, level);
            if ( (run < HUFF_MAXRUN) && (alevel < huff_maxlevel[run])) {
                /* encode using the Huffman tables */
                code = (huff_table[run])[alevel];
                nbits = (huff_bits[run])[alevel];
                code |= ((UINT32)level >> 31); /* the sign bit */

                put_bits(&s->pb, nbits, code);
            } else {
                /* escape: only clip in this case */
                put_bits(&s->pb, 6, 0x1);
                put_bits(&s->pb, 6, run);
                if (alevel < 128) {
                    put_bits(&s->pb, 8, level & 0xff);
                } else {
                    if (level < 0) {
                        put_bits(&s->pb, 16, 0x8001 + level + 255);
                    } else {
                        put_bits(&s->pb, 16, level & 0xffff);
                    }
                }
            }
            last_non_zero = i;
        }
    }
    /* end of block */
    put_bits(&s->pb, 2, 0x2);
}


/* rate control */

/* an I frame is I_FRAME_SIZE_RATIO bigger than a P frame */
#define I_FRAME_SIZE_RATIO 3.0
#define QSCALE_K           20

static void rate_control_init(MpegEncContext *s)
{
    s->wanted_bits = 0;

    if (s->intra_only) {
        s->I_frame_bits = s->bit_rate / s->frame_rate;
        s->P_frame_bits = s->I_frame_bits;
    } else {
        s->P_frame_bits = (int) ((float)(s->gop_size * s->bit_rate) / 
                    (float)(s->frame_rate * (I_FRAME_SIZE_RATIO + s->gop_size - 1)));
        s->I_frame_bits = (int)(s->P_frame_bits * I_FRAME_SIZE_RATIO);
    }
    
#if defined(DEBUG)
    printf("I_frame_size=%d P_frame_size=%d\n",
           s->I_frame_bits, s->P_frame_bits);
#endif
}


/*
 * This heuristic is rather poor, but at least we do not have to
 * change the qscale at every macroblock.
 */
static int rate_estimate_qscale(MpegEncContext *s)
{
    long long total_bits = s->total_bits;
    float q;
    int qscale, diff, qmin;

    if (s->pict_type == I_TYPE) {
        s->wanted_bits += s->I_frame_bits;
    } else {
        s->wanted_bits += s->P_frame_bits;
    }
    diff = s->wanted_bits - total_bits;
    q = 31.0 - (float)diff / (QSCALE_K * s->mb_height * s->mb_width);
    /* adjust for I frame */
    if (s->pict_type == I_TYPE && !s->intra_only) {
        q /= I_FRAME_SIZE_RATIO;
    }

    /* using a too small Q scale leeds to problems in mpeg1 and h263
       because AC coefficients are clamped to 255 or 127 */
    qmin = 3;
    if (q < qmin)
        q = qmin;
    else if (q > 31)
        q = 31;
    qscale = (int)(q + 0.5);
#if defined(DEBUG)
    printf("%d: total=%Ld br=%0.1f diff=%d qest=%0.1f\n", 
           s->picture_number, 
           total_bits, (float)s->frame_rate * total_bits / s->picture_number, 
           diff, q);
#endif
    return qscale;
}

AVEncoder mpeg1video_encoder = {
    "mpeg1video",
    CODEC_TYPE_VIDEO,
    CODEC_ID_MPEG1VIDEO,
    sizeof(MpegEncContext),
    MPV_encode_init,
    MPV_encode_picture,
    MPV_encode_end,
};

AVEncoder h263_encoder = {
    "h263",
    CODEC_TYPE_VIDEO,
    CODEC_ID_H263,
    sizeof(MpegEncContext),
    MPV_encode_init,
    MPV_encode_picture,
    MPV_encode_end,
};

AVEncoder rv10_encoder = {
    "rv10",
    CODEC_TYPE_VIDEO,
    CODEC_ID_RV10,
    sizeof(MpegEncContext),
    MPV_encode_init,
    MPV_encode_picture,
    MPV_encode_end,
};

AVEncoder mjpeg_encoder = {
    "mjpeg",
    CODEC_TYPE_VIDEO,
    CODEC_ID_MJPEG,
    sizeof(MpegEncContext),
    MPV_encode_init,
    MPV_encode_picture,
    MPV_encode_end,
};

AVEncoder divx_encoder = {
    "divx",
    CODEC_TYPE_VIDEO,
    CODEC_ID_DIVX,
    sizeof(MpegEncContext),
    MPV_encode_init,
    MPV_encode_picture,
    MPV_encode_end,
};
