
/**************************************************************************
 *                                                                        *
 * This code is developed by Adam Li.  This software is an                *
 * implementation of a part of one or more MPEG-4 Video tools as          *
 * specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
 * software module in hardware or software products are advised that its  *
 * use may infringe existing patents or copyrights, and any such use      *
 * would be at such party's own risk.  The original developer of this     *
 * software module and his/her company, and subsequent editors and their  *
 * companies (including Project Mayo), will have no liability for use of  *
 * this software or modifications or derivatives thereof.                 *
 *                                                                        *
 * Project Mayo gives users of the Codec a license to this software       *
 * module or modifications thereof for use in hardware or software        *
 * products claiming conformance to the MPEG-4 Video Standard as          *
 * described in the Open DivX license.                                    *
 *                                                                        *
 * The complete Open DivX license can be found at                         *
 * http://www.projectmayo.com/opendivx/license.php .                      *
 *                                                                        *
 **************************************************************************/

/**************************************************************************
 *
 *  encore.c, MPEG-4 Encoder Core Engine
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Adam Li
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/

/* This file contains encore(), which is the main entrance function for   */
/* Encoder Core Engine. It also contains the functions to initialize the  */
/* parameters, and redirect the output stream                             */

#include "encore.h"

#include "vop_code.h"
#include "text_dct.h"
#include "rc_q2.h"
#include "bitstream.h"
#include "vm_common_defs.h"

typedef struct _REFERENCE
{
	unsigned long handle;
	float framerate;
	long bitrate;
	long rc_period;
	int x_dim, y_dim;
	int prev_rounding;
	int search_range;
	int max_quantizer;
	int min_quantizer;

	long seq;

	Vop *current;        /* the current frame to be encoded */
	Vop *reference;      /* the reference frame - reconstructed previous frame */
	Vop *reconstruct;    /* intermediate reconstructed frame - used in inter */
	Vop *error;          /* intermediate error frame - used in inter to hold prediction error */

	struct _REFERENCE *pnext;
} REFERENCE;

FILE *ftrace = NULL;
int max_quantizer, min_quantizer;

/* private functions used only in this file */
void init_vol_config(VolConfig *vol_config);
void init_vop(Vop *vop);
Int get_fcode (Int sr);
int PutVoVolHeader(int vol_width, int vol_height, int time_increment_resolution, float frame_rate);
static int YUV2YUV (int x_dim, int y_dim, void *yuv, void *y_out, void *u_out, void *v_out);

int encore(unsigned long handle, unsigned long enc_opt, void *param1, void *param2)
{
	static REFERENCE *ref = NULL;
	static VolConfig *vol_config;
	// a link list to keep the reference frame for all instances

	REFERENCE *ref_curr, *ref_last = NULL;
	int x_dim, y_dim, size, length;
	int headerbits = 0;
	Vop *curr;

	ref_curr = ref_last = ref;
	while (ref_curr != NULL)
	{
		if (ref_curr->handle == handle) break;
		ref_last = ref_curr;
		ref_curr = ref_last->pnext;
	}
	// create a reference for the new handle when no match is found
	if (ref_curr == NULL)
	{
		if (enc_opt & ENC_OPT_RELEASE) return ENC_OK;
		ref_curr = (REFERENCE *)malloc(sizeof(REFERENCE));
		ref_curr->handle = handle;
		ref_curr->seq = 0;
		ref_curr->pnext = NULL;
		if (ref) ref_last->pnext = ref_curr;
		else ref = ref_curr;
	}
	// initialize for a handle if requested
	if (enc_opt & ENC_OPT_INIT)
	{
		init_fdct_enc();
		init_idct_enc();

		// initializing rate control
		ref_curr->framerate = ((ENC_PARAM *)param1)->framerate;
		ref_curr->bitrate = ((ENC_PARAM *)param1)->bitrate;
		ref_curr->rc_period = ((ENC_PARAM *)param1)->rc_period;
		ref_curr->x_dim = ((ENC_PARAM *)param1)->x_dim;
		ref_curr->y_dim = ((ENC_PARAM *)param1)->y_dim;
		ref_curr->search_range = ((ENC_PARAM *)param1)->search_range;
		ref_curr->max_quantizer = ((ENC_PARAM *)param1)->max_quantizer;
		ref_curr->min_quantizer = ((ENC_PARAM *)param1)->min_quantizer;

		ref_curr->current = AllocVop(ref_curr->x_dim, ref_curr->y_dim);
		ref_curr->reference = AllocVop(ref_curr->x_dim + 2 * 16, 
			ref_curr->y_dim + 2 * 16);
		ref_curr->reconstruct = AllocVop(ref_curr->x_dim, ref_curr->y_dim);
		ref_curr->error = AllocVop(ref_curr->x_dim, ref_curr->y_dim);
		init_vop(ref_curr->current);
		init_vop(ref_curr->reference);
		init_vop(ref_curr->reconstruct);
		init_vop(ref_curr->error);
		ref_curr->reference->hor_spat_ref = -16;
		ref_curr->reference->ver_spat_ref = -16;
		SetConstantImage(ref_curr->reference->y_chan, 0);

		vol_config = (VolConfig *)malloc(sizeof(VolConfig));
		init_vol_config(vol_config);
		vol_config->frame_rate = ref_curr->framerate;
		vol_config->bit_rate = ref_curr->bitrate;
		RCQ2_init(vol_config, ref_curr->rc_period);

#ifdef _RC_
		ftrace = fopen("trace.txt", "w");
#endif
		return ENC_OK;
	}
	// release the reference associated with the handle if requested
	if (enc_opt & ENC_OPT_RELEASE)
	{
		RCQ2_Free();
		if (ref_curr == ref) ref = NULL;
		else ref_last->pnext = ref_curr->pnext;

		if (ref_curr->current) FreeVop(ref_curr->current);
		if (ref_curr->reference) FreeVop(ref_curr->reference);
		if (ref_curr->reconstruct) FreeVop(ref_curr->reconstruct);
		if (ref_curr->error) FreeVop(ref_curr->error);

		free(ref_curr);
		free(vol_config);
		if (ftrace) {
			fclose(ftrace);
			ftrace = NULL;
		};
		return ENC_OK;
	}

#ifdef _RC_
	fflush(ftrace);
	fprintf(ftrace, "\nNow coding frame # %d.\n", ref_curr->seq);
#endif

	// initialize the parameters (need to be cleaned later)

	max_quantizer = ref_curr->max_quantizer;
	min_quantizer = ref_curr->min_quantizer;

	x_dim = ref_curr->x_dim;
	y_dim = ref_curr->y_dim;
	size = x_dim * y_dim;

	curr = ref_curr->current;
	curr->width = x_dim;
	curr->height = y_dim;
	curr->sr_for = ref_curr->search_range;
	curr->fcode_for = get_fcode(curr->sr_for);

	// do transformation for the input image
	// this is needed because the legacy MoMuSys code uses short int for each data
	YUV2YUV(x_dim, y_dim, ((ENC_FRAME *)param1)->image,
		curr->y_chan->f, curr->u_chan->f, curr->v_chan->f);

	// adjust the rounding_type for the current image
	curr->rounding_type = 1 - ref_curr->prev_rounding;

	Bitstream_Init((void *)(((ENC_FRAME *)param1)->bitstream));

	if (ref_curr->seq == 0) {
		headerbits = PutVoVolHeader(x_dim, y_dim, curr->time_increment_resolution, ref_curr->framerate);
		curr->prediction_type = I_VOP;
	}
	else curr->prediction_type = P_VOP;

#ifdef _RC_
	fprintf(ftrace, "\nCoding frame #%d\n", ref_curr->seq);
#endif

	// Code the image data (YUV) of the current image
	VopCode(curr,
		ref_curr->reference,
		ref_curr->reconstruct,
		ref_curr->error,
		1, //enable_8x8_mv,
		(float)ref_curr->seq/ref_curr->framerate,  // time
		vol_config);

	length = Bitstream_Close();
	((ENC_FRAME *)param1)->length = length;

	// update the rate control parameters
	RCQ2_Update2OrderModel(	/*num_bits.vop,*/length * 8 - headerbits,
		GetVopPredictionType(curr));

	ref_curr->prev_rounding = curr->rounding_type;
	ref_curr->seq ++;

	if (curr->prediction_type == I_VOP)
		((ENC_RESULT *)param2)->isKeyFrame = 1;
	else 
		((ENC_RESULT *)param2)->isKeyFrame = 0;

	return ENC_OK;
}


void init_vol_config(VolConfig *vol_config)
{
	/* configure VOL */
	vol_config->M = 1;
	vol_config->frame_skip = 1;
	vol_config->quantizer = 8;
	vol_config->intra_quantizer = 8;
	vol_config->modulo_time_base[0] =0;
	vol_config->modulo_time_base[1] =0;
	vol_config->frame_rate = 30;
	vol_config->bit_rate = 800000;
}


void init_vop(Vop *vop)
{
	/* initialize VOPs */
	vop->quant_precision = 5;
	vop->bits_per_pixel = 8;
//	vop->time_increment_resolution = 15 ;
	vop->time_increment_resolution = 30000;
	vop->intra_acdc_pred_disable = 0;
	vop->intra_dc_vlc_thr = 0;

	vop->sr_for = 512;
	vop->fcode_for = get_fcode(512);

	vop->y_chan->type = SHORT_TYPE;
	vop->u_chan->type = SHORT_TYPE;
	vop->v_chan->type = SHORT_TYPE;

	vop->hor_spat_ref = 0;
	vop->ver_spat_ref = 0;

}

Int get_fcode (Int sr)
{
	if (sr<=16) return 1;
	else if (sr<=32) return 2;
	else if (sr<=64) return 3;
	else if (sr<=128) return 4;
	else if (sr<=256) return 5;
	else if (sr<=512) return 6;
	else if (sr<=1024) return 7;
	else return (-1);
}

int PutVoVolHeader(int vol_width, int vol_height, int time_increment_resolution, float frame_rate)
{
	int written = 0;
	int bits, fixed_vop_time_increment;

	Bitstream_PutBits(VO_START_CODE_LENGTH, VO_START_CODE);
	Bitstream_PutBits(5, 0);				/* vo_id = 0								*/
	written += VO_START_CODE_LENGTH + 5;

	Bitstream_PutBits(VOL_START_CODE_LENGTH, VOL_START_CODE);
	Bitstream_PutBits(4, 0);				/* vol_id = 0								*/
	written += VOL_START_CODE_LENGTH + 4;

	Bitstream_PutBits(1, 0);				/* random_accessible_vol = 0				*/
	Bitstream_PutBits(8, 1);				/* video_object_type_indication = 1 video	*/
	Bitstream_PutBits(1, 1);				/* is_object_layer_identifier = 1			*/
	Bitstream_PutBits(4, 2);				/* visual_object_layer_ver_id = 2			*/
	Bitstream_PutBits(3, 1);				/* visual_object_layer_priority = 1			*/
	written += 1 + 8 + 1 + 4 + 3;

	Bitstream_PutBits(4, 1);				/* aspect_ratio_info = 1					*/
	Bitstream_PutBits(1, 0);				/* vol_control_parameter = 0				*/
	Bitstream_PutBits(2, 0);				/* vol_shape = 0 rectangular				*/
	Bitstream_PutBits(1, 1);				/* marker									*/
	written += 4 + 1 + 2 + 1;

	Bitstream_PutBits(16, time_increment_resolution);
	Bitstream_PutBits(1, 1);				/* marker									*/
	Bitstream_PutBits(1, 1);				/* fixed_vop_rate = 1						*/
	bits = (int)ceil(log((double)time_increment_resolution)/log(2.0));
    if (bits<1) bits=1;
	fixed_vop_time_increment = (int)(time_increment_resolution / frame_rate + 0.1);
	Bitstream_PutBits(bits, fixed_vop_time_increment);
	Bitstream_PutBits(1, 1);				/* marker									*/
	written += 16 + 1 + 1 + bits + 1;

	Bitstream_PutBits(13, vol_width);
	Bitstream_PutBits(1, 1);				/* marker									*/
	Bitstream_PutBits(13, vol_height);
	Bitstream_PutBits(1, 1);				/* marker									*/
	written += 13 + 1 + 13 + 1;

	Bitstream_PutBits(1, 0);				/* interlaced = 0							*/
	Bitstream_PutBits(1, 1);				/* OBMC_disabled = 1						*/
	Bitstream_PutBits(2, 0);				/* vol_sprite_usage = 0						*/
	Bitstream_PutBits(1, 0);				/* not_8_bit = 0							*/
	written += 1 + 1 + 2 + 1;

	Bitstream_PutBits(1, 0);				/* vol_quant_type = 0						*/
	Bitstream_PutBits(1, 0);				/* vol_quarter_pixel = 0					*/
	Bitstream_PutBits(1, 1);				/* complexity_estimation_disabled = 1		*/
	Bitstream_PutBits(1, 1);				/* resync_marker_disabled = 1				*/
	Bitstream_PutBits(1, 0);				/* data_partitioning_enabled = 0			*/
	Bitstream_PutBits(1, 0);				/* scalability = 0							*/
	written += 1 + 1 + 1 + 1 + 1 + 1;

	written += Bitstream_NextStartCode();

	return(written);
}

static int YUV2YUV (int x_dim, int y_dim, void *yuv, void *y_out, void *u_out, void *v_out)
{
	// All this conversion does is to turn data from unsigned char to short int,
	// since legacy MoMuSys uses short int.
	unsigned char *in;
	short int *out;
	long size;

	in = yuv;
	
	out = y_out;
	size = x_dim * y_dim;
	while (size --) *(out ++) = *(in ++);

	out = u_out;
	size = x_dim * y_dim / 4;
	while (size --) *(out ++) = *(in ++);

	out = v_out;
	size = x_dim * y_dim / 4;
	while (size --) *(out ++) = *(in ++);

	return 0;
}

