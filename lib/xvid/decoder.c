/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	decoder main
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
 *	This program is xvid_free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the xvid_free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the xvid_free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *************************************************************************/

/**************************************************************************
 *
 *	History:
 *
 *  29.03.2002  interlacing fix - compensated block wasn't being used when
 *              reconstructing blocks, thus artifacts
 *              interlacing speedup - used transfers to re-interlace
 *              interlaced decoding should be as fast as progressive now
 *  26.03.2002  interlacing support - moved transfers outside decode loop
 *	26.12.2001	decoder_mbinter: dequant/idct moved within if(coded) block
 *	22.12.2001	block based interpolation
 *	01.12.2001	inital version; (c)2001 peter ross <pross@cs.rmit.edu.au>
 *
 *************************************************************************/

#include <stdlib.h>
#include <string.h>  // memset

#include "xvid.h"
#include "portab.h"

#include "decoder.h"
#include "bitstream/bitstream.h"
#include "bitstream/mbcoding.h"

#include "quant/quant_h263.h"
#include "quant/quant_mpeg4.h"
#include "dct/idct.h"
#include "dct/fdct.h"
#include "utils/mem_transfer.h"
#include "image/interpolate8x8.h"

#include "bitstream/mbcoding.h"
#include "prediction/mbprediction.h"
#include "utils/timer.h"
#include "utils/emms.h"

#include "image/image.h"
#include "image/colorspace.h"
#include "utils/mem_align.h"

int decoder_alloc(XVID_DEC_PARAM * param)
{
  param->handle = xvid_malloc(sizeof(DECODER), CACHE_LINE);
  if (param->handle == NULL) 
    return XVID_ERR_MEMORY;
  return XVID_ERR_OK;
}

int decoder_initialize (DECODER *dec)
{
	dec->mb_width = (dec->width + 15) / 16;
	dec->mb_height = (dec->height + 15) / 16;

	dec->edged_width = 16 * dec->mb_width + 2 * EDGE_SIZE;
	dec->edged_height = 16 * dec->mb_height + 2 * EDGE_SIZE;
	
	if (image_create(&dec->cur, dec->edged_width, dec->edged_height))
	{
		xvid_free(dec);
		return XVID_ERR_MEMORY;
	}

	if (image_create(&dec->refn, dec->edged_width, dec->edged_height))
	{
		image_destroy(&dec->cur, dec->edged_width, dec->edged_height);
		xvid_free(dec);
		return XVID_ERR_MEMORY;
	}

	dec->mbs = xvid_malloc(sizeof(MACROBLOCK) * dec->mb_width * dec->mb_height, CACHE_LINE);
	if (dec->mbs == NULL)
	{
		image_destroy(&dec->cur, dec->edged_width, dec->edged_height);
		xvid_free(dec);
		return XVID_ERR_MEMORY;
	}

	init_timer();

	return XVID_ERR_OK;
}

int decoder_create(XVID_DEC_PARAM * param)
{
  DECODER *dec;

  decoder_alloc(param);
  if (param->handle == NULL) return XVID_ERR_MEMORY;

  dec = param->handle;
  dec->width = param->width;
  dec->height = param->height;
  return (decoder_initialize(dec));
}

int decoder_destroy(DECODER * dec)
{
	xvid_free(dec->mbs);
	image_destroy(&dec->refn, dec->edged_width, dec->edged_height);
	image_destroy(&dec->cur, dec->edged_width, dec->edged_height);
	xvid_free(dec);

	write_timer();
	return XVID_ERR_OK;
}



static const int32_t dquant_table[4] =
{
	-1, -2, 1, 2
};


// decode an intra macroblock
static
void decoder_mbintra(DECODER * dec,
		     MACROBLOCK * pMB,
		     const uint32_t x_pos,
		     const uint32_t y_pos,
		     const uint32_t acpred_flag,
		     const uint32_t cbp,
		     Bitstream * bs,
		     const uint32_t quant,
		     const uint32_t intra_dc_threshold)
{

	DECLARE_ALIGNED_MATRIX(block, 6, 64, int16_t, CACHE_LINE);
	DECLARE_ALIGNED_MATRIX(data,  6, 64, int16_t, CACHE_LINE);

	uint32_t stride = dec->edged_width;
	uint32_t stride2 = stride / 2;
	uint32_t next_block = stride * 8;
	uint32_t i;
	uint32_t iQuant = pMB->quant;
	uint8_t *pY_Cur, *pU_Cur, *pV_Cur;

	pY_Cur = dec->cur.y + (y_pos << 4) * stride + (x_pos << 4);
	pU_Cur = dec->cur.u + (y_pos << 3) * stride2 + (x_pos << 3);
	pV_Cur = dec->cur.v + (y_pos << 3) * stride2 + (x_pos << 3);

	memset(block, 0, 6*64*sizeof(int16_t));		// clear

	for (i = 0; i < 6; i++)
	{
		uint32_t iDcScaler = get_dc_scaler(iQuant, i < 4);
		int16_t predictors[8];
		int start_coeff;

		start_timer();
		predict_acdc(dec->mbs, x_pos, y_pos, dec->mb_width, i, &block[i*64], iQuant, iDcScaler, predictors);
		if (!acpred_flag)
		{
			pMB->acpred_directions[i] = 0;
		}
		stop_prediction_timer();

		if (quant < intra_dc_threshold)
		{
			int dc_size;
			int dc_dif;

			dc_size = i < 4 ?  get_dc_size_lum(bs) : get_dc_size_chrom(bs);
			dc_dif = dc_size ? get_dc_dif(bs, dc_size) : 0 ;

			if (dc_size > 8)
			{
				BitstreamSkip(bs, 1);		// marker
			}
		
			block[i*64 + 0] = dc_dif;
			start_coeff = 1;
		}
		else
		{
			start_coeff = 0;
		}

		start_timer();
		if (cbp & (1 << (5-i)))			// coded
		{
			get_intra_block(bs, &block[i*64], pMB->acpred_directions[i], start_coeff);
		}
		stop_coding_timer();

		start_timer();
		add_acdc(pMB, i, &block[i*64], iDcScaler, predictors);
		stop_prediction_timer();

		start_timer();
		if (dec->quant_type == 0)
		{
			dequant_intra(&data[i*64], &block[i*64], iQuant, iDcScaler);
		}
		else
		{
			dequant4_intra(&data[i*64], &block[i*64], iQuant, iDcScaler);
		}
		stop_iquant_timer();

		start_timer();
		idct(&data[i*64]);
		stop_idct_timer();
	}

	if (dec->interlacing && pMB->field_dct)
	{
		next_block = stride;
		stride *= 2;
	}

	start_timer();
	transfer_16to8copy(pY_Cur,                  &data[0*64], stride);
	transfer_16to8copy(pY_Cur + 8,              &data[1*64], stride);
	transfer_16to8copy(pY_Cur + next_block,     &data[2*64], stride);
	transfer_16to8copy(pY_Cur + 8 + next_block, &data[3*64], stride);
	transfer_16to8copy(pU_Cur,                  &data[4*64], stride2);
	transfer_16to8copy(pV_Cur,                  &data[5*64], stride2);
	stop_transfer_timer();
}





#define SIGN(X) (((X)>0)?1:-1)
#define ABS(X) (((X)>0)?(X):-(X))
static const uint32_t roundtab[16] =
{ 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2 };


// decode an inter macroblock

static void decoder_mbinter(DECODER * dec,
			    const MACROBLOCK * pMB,
			    const uint32_t x_pos,
			    const uint32_t y_pos,
			    const uint32_t acpred_flag,
			    const uint32_t cbp,
			    Bitstream * bs,
			    const uint32_t quant,
			    const uint32_t rounding)
{

	DECLARE_ALIGNED_MATRIX(block,6, 64, int16_t, CACHE_LINE);
	DECLARE_ALIGNED_MATRIX(data, 6, 64, int16_t, CACHE_LINE);

	uint32_t stride = dec->edged_width;
	uint32_t stride2 = stride / 2;
	uint32_t next_block = stride * 8;
	uint32_t i;
	uint32_t iQuant = pMB->quant;
	uint8_t *pY_Cur, *pU_Cur, *pV_Cur;
	int uv_dx, uv_dy;

	pY_Cur = dec->cur.y + (y_pos << 4) * stride + (x_pos << 4);
	pU_Cur = dec->cur.u + (y_pos << 3) * stride2 + (x_pos << 3);
	pV_Cur = dec->cur.v + (y_pos << 3) * stride2 + (x_pos << 3);

	if (pMB->mode == MODE_INTER || pMB->mode == MODE_INTER_Q)
	{
		uv_dx = pMB->mvs[0].x;
		uv_dy = pMB->mvs[0].y;

		uv_dx = (uv_dx & 3) ? (uv_dx >> 1) | 1 : uv_dx / 2;
		uv_dy = (uv_dy & 3) ? (uv_dy >> 1) | 1 : uv_dy / 2;
	}
	else
	{
		int sum;
		sum = pMB->mvs[0].x + pMB->mvs[1].x + pMB->mvs[2].x + pMB->mvs[3].x;
		uv_dx = (sum == 0 ? 0 : SIGN(sum) * (roundtab[ABS(sum) % 16] + (ABS(sum) / 16) * 2) );

		sum = pMB->mvs[0].y + pMB->mvs[1].y + pMB->mvs[2].y + pMB->mvs[3].y;
		uv_dy = (sum == 0 ? 0 : SIGN(sum) * (roundtab[ABS(sum) % 16] + (ABS(sum) / 16) * 2) );
	}

	start_timer();
	interpolate8x8_switch(dec->cur.y, dec->refn.y, 16*x_pos,     16*y_pos    , pMB->mvs[0].x, pMB->mvs[0].y, stride,  rounding);
	interpolate8x8_switch(dec->cur.y, dec->refn.y, 16*x_pos + 8, 16*y_pos    , pMB->mvs[1].x, pMB->mvs[1].y, stride,  rounding);
	interpolate8x8_switch(dec->cur.y, dec->refn.y, 16*x_pos,     16*y_pos + 8, pMB->mvs[2].x, pMB->mvs[2].y, stride,  rounding);
	interpolate8x8_switch(dec->cur.y, dec->refn.y, 16*x_pos + 8, 16*y_pos + 8, pMB->mvs[3].x, pMB->mvs[3].y, stride,  rounding);
	interpolate8x8_switch(dec->cur.u, dec->refn.u, 8*x_pos,      8*y_pos,      uv_dx,         uv_dy,         stride2, rounding);
	interpolate8x8_switch(dec->cur.v, dec->refn.v, 8*x_pos,      8*y_pos,      uv_dx,         uv_dy,         stride2, rounding);
	stop_comp_timer();

	for (i = 0; i < 6; i++)
	{
		if (cbp & (1 << (5-i)))			// coded
		{
			memset(&block[i*64], 0, 64 * sizeof(int16_t));		// clear

			start_timer();
			get_inter_block(bs, &block[i*64]);
			stop_coding_timer();

			start_timer();
			if (dec->quant_type == 0)
			{
				dequant_inter(&data[i*64], &block[i*64], iQuant);
			}
			else
			{
				dequant4_inter(&data[i*64], &block[i*64], iQuant);
			}
			stop_iquant_timer();

			start_timer();
			idct(&data[i*64]);
			stop_idct_timer();
		}
	}

	if (dec->interlacing && pMB->field_dct)
	{
		next_block = stride;
		stride *= 2;
	}

	start_timer();
	if (cbp & 32)
		transfer_16to8add(pY_Cur,                  &data[0*64], stride);
	if (cbp & 16)
		transfer_16to8add(pY_Cur + 8,              &data[1*64], stride);
	if (cbp & 8)
		transfer_16to8add(pY_Cur + next_block,     &data[2*64], stride);
	if (cbp & 4)
		transfer_16to8add(pY_Cur + 8 + next_block, &data[3*64], stride);
	if (cbp & 2)
		transfer_16to8add(pU_Cur,                  &data[4*64], stride2);
	if (cbp & 1)
		transfer_16to8add(pV_Cur,                  &data[5*64], stride2);
	stop_transfer_timer();
}


static void decoder_iframe(DECODER * dec, Bitstream * bs, int quant, int intra_dc_threshold)
{

	uint32_t x, y;

	for (y = 0; y < dec->mb_height; y++)
	{
		for (x = 0; x < dec->mb_width; x++)
		{
			MACROBLOCK * mb = &dec->mbs[y*dec->mb_width + x];

			uint32_t mcbpc;
			uint32_t cbpc;
			uint32_t acpred_flag;
			uint32_t cbpy;
			uint32_t cbp;

			mcbpc = get_mcbpc_intra(bs);
			mb->mode = mcbpc & 7;
			cbpc = (mcbpc >> 4);

			acpred_flag = BitstreamGetBit(bs);

			if (mb->mode == MODE_STUFFING)
			{
				DEBUG("-- STUFFING ?");
				continue;
			}

			cbpy = get_cbpy(bs, 1);
			cbp = (cbpy << 2) | cbpc;

			if (mb->mode == MODE_INTRA_Q)
			{
				quant += dquant_table[BitstreamGetBits(bs,2)];
				if (quant > 31)
				{
					quant = 31;
				}
				else if (quant < 1)
				{
					quant = 1;
				}
			}
			mb->quant = quant;

			if (dec->interlacing)
			{
				mb->field_dct = BitstreamGetBit(bs);
				DEBUG1("deci: field_dct: ", mb->field_dct);
			}

			decoder_mbintra(dec, mb, x, y, acpred_flag, cbp, bs, quant, intra_dc_threshold);
		}
	}

}


static void get_motion_vector(DECODER *dec, Bitstream *bs, int x, int y, int k, VECTOR * mv, int fcode)
{

	int scale_fac = 1 << (fcode - 1);
	int high = (32 * scale_fac) - 1;
	int low = ((-32) * scale_fac);
	int range = (64 * scale_fac);

	VECTOR pmv[4];
	uint32_t psad[4];

	int mv_x, mv_y;
	int pmv_x, pmv_y;


	get_pmvdata(dec->mbs, x, y, dec->mb_width, k, pmv, psad);

	pmv_x = pmv[0].x;
	pmv_y = pmv[0].y;

	mv_x = get_mv(bs, fcode);
	mv_y = get_mv(bs, fcode);
	
	mv_x += pmv_x;
	mv_y += pmv_y;

	if (mv_x < low)
	{
		mv_x += range;
	} 
	else if (mv_x > high)
	{
		mv_x -= range;
	}

	if (mv_y < low)
	{
		mv_y += range;
	} 
	else if (mv_y > high)
	{
		mv_y -= range;
	}

	mv->x = mv_x;
	mv->y = mv_y;

}

static
void decoder_pframe(DECODER * dec, Bitstream * bs, int rounding, int quant, int fcode, int intra_dc_threshold)
{

	uint32_t x, y;

	image_swap(&dec->cur, &dec->refn);
	
	start_timer();
	image_setedges(&dec->refn, dec->edged_width, dec->edged_height, dec->width, dec->height, dec->interlacing);
	stop_edges_timer();

	for (y = 0; y < dec->mb_height; y++)
	{
		for (x = 0; x < dec->mb_width; x++)
		{
			MACROBLOCK * mb = &dec->mbs[y*dec->mb_width + x];

			if (!BitstreamGetBit(bs))			// not_coded
			{
				uint32_t mcbpc;
				uint32_t cbpc;
				uint32_t acpred_flag;
				uint32_t cbpy;
				uint32_t cbp;
				uint32_t intra;

				mcbpc = get_mcbpc_inter(bs);
				mb->mode = mcbpc & 7;
				cbpc = (mcbpc >> 4);
				acpred_flag = 0;

				intra = (mb->mode == MODE_INTRA || mb->mode == MODE_INTRA_Q);
				
				if (intra)
				{
					acpred_flag = BitstreamGetBit(bs);
				}

				if (mb->mode == MODE_STUFFING)
				{
					DEBUG("-- STUFFING ?");
					continue;
				}

				cbpy = get_cbpy(bs, intra);
				cbp = (cbpy << 2) | cbpc;

				if (mb->mode == MODE_INTER_Q || mb->mode == MODE_INTRA_Q)
				{
					quant += dquant_table[BitstreamGetBits(bs,2)];
					if (quant > 31)
					{
						quant = 31;
					}
					else if (mb->quant < 1)
					{
						quant = 1;
					}
				}
				mb->quant = quant;

				if (dec->interlacing)
				{
					mb->field_dct = BitstreamGetBit(bs);
					DEBUG1("decp: field_dct: ", mb->field_dct);

					if (mb->mode == MODE_INTER || mb->mode == MODE_INTER_Q)
					{
						mb->field_pred = BitstreamGetBit(bs);
						DEBUG1("decp: field_pred: ", mb->field_pred);

						if (mb->field_pred)
						{
							mb->field_for_top = BitstreamGetBit(bs);
							DEBUG1("decp: field_for_top: ", mb->field_for_top);
							mb->field_for_bot = BitstreamGetBit(bs);
							DEBUG1("decp: field_for_bot: ", mb->field_for_bot);
						}
					}
				}

				if (mb->mode == MODE_INTER || mb->mode == MODE_INTER_Q)
				{
					if (dec->interlacing && mb->field_pred)
					{
						get_motion_vector(dec, bs, x, y, 0, &mb->mvs[0], fcode);
						get_motion_vector(dec, bs, x, y, 0, &mb->mvs[1], fcode);
					}
					else
					{
						get_motion_vector(dec, bs, x, y, 0, &mb->mvs[0], fcode);
						mb->mvs[1].x = mb->mvs[2].x = mb->mvs[3].x = mb->mvs[0].x;
						mb->mvs[1].y = mb->mvs[2].y = mb->mvs[3].y = mb->mvs[0].y;
					}
				}
				else if (mb->mode == MODE_INTER4V /* || mb->mode == MODE_INTER4V_Q */)
				{
					get_motion_vector(dec, bs, x, y, 0, &mb->mvs[0], fcode);
					get_motion_vector(dec, bs, x, y, 1, &mb->mvs[1], fcode);
					get_motion_vector(dec, bs, x, y, 2, &mb->mvs[2], fcode);
					get_motion_vector(dec, bs, x, y, 3, &mb->mvs[3], fcode);
				}
				else  // MODE_INTRA, MODE_INTRA_Q
				{
					mb->mvs[0].x = mb->mvs[1].x = mb->mvs[2].x = mb->mvs[3].x = 0;
					mb->mvs[0].y = mb->mvs[1].y = mb->mvs[2].y = mb->mvs[3].y = 0;
					decoder_mbintra(dec, mb, x, y, acpred_flag, cbp, bs, quant, intra_dc_threshold);
					continue;
				}

				decoder_mbinter(dec, mb, x, y, acpred_flag, cbp, bs, quant, rounding);
			}
			else	// not coded
			{

				mb->mode = MODE_NOT_CODED; 
				mb->mvs[0].x = mb->mvs[1].x = mb->mvs[2].x = mb->mvs[3].x = 0;
				mb->mvs[0].y = mb->mvs[1].y = mb->mvs[2].y = mb->mvs[3].y = 0;
					
				// copy macroblock directly from ref to cur

				start_timer();

				transfer8x8_copy(dec->cur.y + (16*y)*dec->edged_width + (16*x), 
						 dec->refn.y + (16*y)*dec->edged_width + (16*x), 
						 dec->edged_width);

				transfer8x8_copy(dec->cur.y + (16*y)*dec->edged_width + (16*x+8), 
						 dec->refn.y + (16*y)*dec->edged_width + (16*x+8), 
						 dec->edged_width);

				transfer8x8_copy(dec->cur.y + (16*y+8)*dec->edged_width + (16*x), 
						 dec->refn.y + (16*y+8)*dec->edged_width + (16*x), 
						 dec->edged_width);
					
				transfer8x8_copy(dec->cur.y + (16*y+8)*dec->edged_width + (16*x+8), 
						 dec->refn.y + (16*y+8)*dec->edged_width + (16*x+8), 
						 dec->edged_width);

				transfer8x8_copy(dec->cur.u + (8*y)*dec->edged_width/2 + (8*x), 
						 dec->refn.u + (8*y)*dec->edged_width/2 + (8*x), 
						 dec->edged_width/2);

				transfer8x8_copy(dec->cur.v + (8*y)*dec->edged_width/2 + (8*x), 
						 dec->refn.v + (8*y)*dec->edged_width/2 + (8*x), 
						 dec->edged_width/2);

				stop_transfer_timer();
			}
		}
	}
}

int decoder_decode(DECODER * dec, XVID_DEC_FRAME * frame)
{

	Bitstream bs;
	uint32_t rounding;
	uint32_t quant;
	uint32_t fcode;
	uint32_t intra_dc_threshold;

	start_global_timer();
	
	BitstreamInit(&bs, frame->bitstream, frame->length);

	switch (BitstreamReadHeaders(&bs, dec, &rounding, &quant, &fcode, &intra_dc_threshold, 0))
	{
	case P_VOP :
		decoder_pframe(dec, &bs, rounding, quant, fcode, intra_dc_threshold);
		break;

	case I_VOP :
		//DEBUG1("",intra_dc_threshold);
		decoder_iframe(dec, &bs, quant, intra_dc_threshold);
		break;

	case B_VOP :	// ignore
		break;
	
	case N_VOP :	// vop not coded
		break;

	default :
		return XVID_ERR_FAIL;
	}

	frame->length = BitstreamPos(&bs) / 8;

	start_timer();
	image_output(&dec->cur, dec->width, dec->height, dec->edged_width,
		     frame->image, frame->stride, frame->colorspace);
	stop_conv_timer();
	
	emms();

	stop_global_timer();

	return XVID_ERR_OK;

}

int decoder_find_vol (DECODER * dec, 
		      XVID_DEC_FRAME * frame, 
		      XVID_DEC_PARAM * param)
{
	Bitstream bs;
	uint32_t rounding;
	uint32_t quant;
	uint32_t fcode;
	uint32_t intra_dc_threshold;
	int ret;

	BitstreamInit(&bs, frame->bitstream, frame->length);

	ret = BitstreamReadHeaders(&bs, dec, &rounding, &quant, &fcode, &intra_dc_threshold, 1);
	frame->length = BitstreamPos(&bs) / 8;

	if (ret > 0) {
	  param->width = dec->width;
	  param->height = dec->height;

	  return decoder_initialize(dec);
	}
	if (ret < 0) return XVID_ERR_FORMAT;
	return XVID_ERR_FAIL;
}
