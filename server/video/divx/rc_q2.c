
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
 *  rc_q2.c
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Adam Li
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/

/* This file contains some functions for rate control of the encoding.    */
/* Some codes of this project come from MoMuSys MPEG-4 implementation.    */
/* Please see seperate acknowledgement file for a list of contributors.   */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if !defined(WIN32)
#  include <unistd.h>
#endif

#include <ctype.h>

#include "rc.h"
#include "rc_hist.h"
#include "momusys.h"
#include "mom_structs.h"
#include "vm_enc_defs.h"

#include <errno.h>



extern FILE *ftrace;
extern int max_quantizer, min_quantizer;

static RC_HIST   rc_hist[3];
static RCQ2_DATA RCQ2_config;

/* private functions used in this file */
void rcQ2_init(RCQ2_DATA  *rcd, VolConfig *vol_config, Int *qp_first);
void RCQ2_init_vol (VolConfig *vol_config, Int *qp_first);
void rc_update_Q2_model (RC_HIST *rch, Double *X1, Double *X2);
Int  RCQ2_compute_quant (Double bit_rate, Int bits_frame, Int bits_prev,
	Int buff_size, Int buff_occup, Double X1, Double X2, Double mad, Int last_qp);
void rc_2ls_comp_q1 (Double *y, Double *x1, Int n, Double *p1, Double *p2);


/***********************************************************HeaderBegin*******
 * LEVEL 1
 ***********************************************************HeaderEnd*********/

/***********************************************************CommentBegin******
 *
 * -- RCQ2_init --
 *
 * Purpose :
 *      General initialization
 *
 ***********************************************************CommentEnd********/

void RCQ2_init(VolConfig *vol_config, int rc_period)
{
	Int qp_first[3];

	qp_first[0] = GetVolConfigIntraQuantizer(vol_config);
	qp_first[1] = GetVolConfigQuantizer(vol_config);
	qp_first[2] = 1;

	RCQ2_init_vol (vol_config, qp_first);

	// test RC - adamli
	RCQ2_config.decay = 1. - 1. / (Double) rc_period;
	RCQ2_config.rc_period = rc_period;
	RCQ2_config.target_rate = (int)(vol_config->bit_rate / vol_config->frame_rate);
	rc_hist[0].rate_history = 0.;
	rc_hist[0].history_length = 0;

}


/***********************************************************CommentBegin******
 *
 * -- RCQ2_Free --
 *
 * Purpose :
 *      Free initialized memory
 *
 ***********************************************************CommentEnd********/

void RCQ2_Free()
{
	RCQ2_DATA *rcd;

	rcd = &RCQ2_config;

	free(rcd->X1);
	free(rcd->X2);
	free(rcd->QPfirst);

	rch_free(rc_hist);
}


/***********************************************************CommentBegin******
 *
 * -- RCQ2_init_vol --
 *
 * Purpose :
 *      Initialization of Q2 rate control
 *
 * Arguments in :
 *      Int      vo_id        -  VO Id
 *      Int      vol_id       -  VOL Id
 *      Int      start        -  First input frame to process
 *      Int      end          -  Last input frame to process
 *      Int      intra_period -  # of frames between I frames +1
 *      Int      M            -  # of B frames between P frames +1
 *      Int      frame_inc    -  Frame temporal subsampling factor
 *      Float    frame_rate   -  Input frame rate (frames per second)
 *      Int      bit_rate     -  Individual bit-rate (bits per second)
 *      Int      qp_first     -  QP to use the first time RC is employed
 *
 ***********************************************************CommentEnd********/

void RCQ2_init_vol(
VolConfig *vol_config,
Int   *qp_first
)
{
	rcQ2_init(&RCQ2_config, vol_config, qp_first);
	rch_init(rc_hist, MAX_SLIDING_WINDOW);
}

void rcQ2_init(
RCQ2_DATA  *rcd,
VolConfig *vol_config,
Int *qp_first									  /* QP to use the first time RC is employed */
)
{
	Int   i, frame_inc, start, end;
	Float frame_rate;

	start = GetVolConfigStartFrame(vol_config);
	end   = GetVolConfigEndFrame(vol_config);
	frame_rate = GetVolConfigFrameRate(vol_config);
	frame_inc  = GetVolConfigFrameSkip(vol_config);

	rcd->bits_P_ave = 0;

	rcd->alpha_I = 3;
	rcd->alpha_B = 0.5;
	rcd->bit_rate = GetVolConfigBitrate(vol_config);
	rcd->R = (Int) ((float)rcd->bit_rate*(end-start+1)/(frame_rate*frame_inc));

	/* Initialization of model parameters: */

	rcd->X1=(Double *)calloc(sizeof(Double),3);
	rcd->X2=(Double *)calloc(sizeof(Double),3);
	for (i=0; i<3; i++)
	{
		rcd->X1[i] = rcd->bit_rate*frame_inc/2;
		/* Initial value specified in VM,
		but only used if the first model
		updating is delayed (by setting
		START_MODEL_UPDATING > 1) */
		rcd->X2[i] = 0;
	}

	rcd->QPfirst=(Int *)calloc(sizeof(Int),3);
	for (i=0; i<3; i++)
		rcd->QPfirst[i] = qp_first[i];
}



/***********************************************************CommentBegin******
 *
 * -- RCQ2_QuantAdjust --
 *
 * Purpose :
 *      Quantizer obtainment: Called after mad is known.
 *
 * Arguments in :
 *      Double   mad          -  Pred. error mad
 *      UInt     num_pels_vop -  # pels per VOP
 *      Int      frame_type   -  I=0, P=1, B=2
 *
 ***********************************************************CommentEnd********/

Int RCQ2_QuantAdjust(
Double mad,										  /* Pred. error mad */
UInt   num_pels_vop,
Int    frame_type
)
{
	Int qp, bits_P, bits_frame=0;
												  /* Histories for frame type */
	RC_HIST   *rch=&rc_hist[frame_type];
	RCQ2_DATA *rcd=&RCQ2_config;

	Double history, decay;
	Int history_length, rc_period;

	Int target_rate, average_rate;

	// test RC - adamli
		history_length = rc_hist[0].history_length;
		history = rc_hist[0].rate_history;
		decay = RCQ2_config.decay;
		rc_period = RCQ2_config.rc_period;
		if ((history_length < rc_period) && (history_length != 0)) 
			average_rate = (int)(history / history_length);
		else 
			average_rate = (int)(history * (1 - decay));
#ifdef _RC_
		fprintf(ftrace, "Decay is %f.\n", decay);
		fprintf(ftrace, "Average frame rate is %d.\n", average_rate);
#endif

	/* Computation of QP */
	if (rch_first(rch))
		qp = rcQ2_get_QPfirst(rcd, frame_type);
	else
	{
		/* RC-BIP: Calculate bits_frame (target0) depending on frame type */

/*		bits_P = (Int)( rcd->R / ( rcd->alpha_I * rcd->num_I_Frame
			+ rcd->alpha_B * rcd->num_B_Frame
			+ rcd->num_P_Frame));
*/
		// test RC - adamli
		target_rate = rcd->target_rate;
		if (history_length < rc_period / 2)
			bits_P = target_rate;
		else
			bits_P = target_rate + (target_rate - average_rate);
#ifdef _RC_
		fprintf(ftrace, "Average target rate is %d.\n", target_rate);
		fprintf(ftrace, "Target rate for this frame is %d.\n", bits_P);
#endif

		switch (frame_type)
		{
			case I_VOP:
				bits_frame = (Int)( rcd->alpha_I * bits_P);
				break;
			case B_VOP:
				bits_frame = (Int)( rcd->alpha_B * bits_P);
				break;
			case P_VOP:
				bits_frame = bits_P;
				break;
			default:
				break;
		}

		#ifdef _RC_DEBUG_
		fprintf(ftrace, "RCQ2_QuantAdjust: R= %f  numFrame= %d  target0= %f\n",
			(double)(rcd->R), rcd->numFrame, (double)rcd->R/rcd->numFrame);
		#endif

		qp=RCQ2_compute_quant(rcd->bit_rate,
			bits_frame, /* rcd->R/rcd->numFrame,*//* RC-BIP */
												  /* RC-BIP */
			rch_get_n(rch)>0/*1*/ ? rch_get_last_bits_vop(rch) : 0,
			rcd->VBV_size,
			rcd->VBV_fullness,
			rcd->X1[frame_type],				  /* RC-BIP */
			rcd->X2[frame_type],				  /* RC-BIP */
			mad,
			rch_get_last_qp(rch)				  /* RC-BIP */
			);

	}

	/* Storage of result */
	rch_store_before(rch, (Int) num_pels_vop, mad, qp);

#ifdef _RC_
	fprintf(ftrace, "Quantizer for this frame is %d.\n", qp);
#endif

	return qp;
}


/***********************************************************CommentBegin******
 *
 * -- rc_update_Q2_model --
 *
 * Purpose :
 *      Obtainment of Q2 model
 *
 * Arguments in :
 *      RC_HIST     *rch     -  VO Id
 *
 * Arguments in/out :
 *      Double      *X1      -  Old and computed parameter
 *      Double      *X2      -  Old and computed parameter
 *
 ***********************************************************CommentEnd********/

void rc_update_Q2_model(
RC_HIST *rch,
Double  *X1,									  /* <-> Old and computed params. */
Double  *X2
)
{
	Double RE[MAX_SLIDING_WINDOW];				  /* R/E data */
	Double invQP[MAX_SLIDING_WINDOW];			  /* Inverse QP   */
//	Double invQP2[MAX_SLIDING_WINDOW];			  /* Inverse QP^2 */

	Int    n;									  /* Number of data to employ */
	Double last_mad;
	Double plast_mad=0;
	Int    i, count;

	Int *bits_text=rch_get_bits_text(rch);
	Double *mad_text = rch_get_mad_text(rch);
	Int *qp = rch_get_qp(rch);

	#ifdef _RC_DEBUG_
	fprintf(ftrace, "RC: RC_Q2: --> rc_update_Q2_model\n");
	fflush(ftrace);
	rch_print(rch, ftrace, "rc_update_Q2_model");
	#endif

	if (rch_get_n(rch)<START_MODEL_UPDATING)
		return;

	last_mad =rch_get_last_mad_text(rch);
	if (rch_get_n(rch)>1)
		plast_mad=rch_get_plast_mad_text(rch);

	/* Computation of number of data to employ */
	n=MIN(rch_get_n(rch),MAX_SLIDING_WINDOW);

	#ifdef _RC_DEBUG_
	fprintf(ftrace, "rc_update_Q2_model: n= %d\n", n);
	#endif
	if (rch_get_n(rch)>1)
		n = (plast_mad/last_mad > 1.0) ? (Int)((last_mad/plast_mad)*n+ 1)
			: (Int)((plast_mad/last_mad)*n + 1);
	else
		n = 1;

	#ifdef _RC_DEBUG_
	fprintf(ftrace, "rc_update_Q2_model: plastmad= %f  lastmad= %f  n= %d\n",  plast_mad, last_mad, n);
	#endif

	n=MIN(n,MIN(rch_get_n(rch),MAX_SLIDING_WINDOW));

	/* Arrays initialization */
	for (i=rch_dec_mod(rch,rch_get_ptr(rch)),count=0; count<n; count++,i=rch_dec_mod(rch,i))
	{
		/* As the VM says, model only takes into account texture bits, although it is
		   used to estimate the total number of bits !!! */
		RE[count]=(Double)bits_text[i]/mad_text[i];

		invQP[count] = 1./(Double)(qp[i]);
/*		invQP2[count] = invQP[count] * invQP[count];

		#ifdef _RC_DEBUG_log_sgicc_dbg

		fprintf(ftrace, "rc_update_Q2_model: %d: RE= %f  IQ= %f  IQ2= %f\n",
			count, RE[count], invQP[count], invQP2[count]);
		#endif
*/
	}

	// Instead of the 2nd order RC model, we now use a more reliable and much simple
	// 1st order RC model.
	rc_2ls_comp_q1(RE, invQP, n, X1, X2);

	#ifdef _RC_DEBUG_
	fprintf(ftrace, "RC: RC_Q2: <-- rc_update_Q2_model: X1= %f  X2= %f  \n", *X1, *X2);
	#endif
}


/***********************************************************CommentBegin******
 *
 * -- RCQ2_Update2OrderModel --
 *
 * Purpose :
 *      Updates model after encoding a frame.
 *
 * Arguments in :
 *      Int      vopbits      -  Total number of bits
 *      Int      frame_type   -  I=0, P=1, B=2
 *
 ***********************************************************CommentEnd********/

void RCQ2_Update2OrderModel(
Int vopbits,									  /* Total number of bits */
Int frame_type
)
{
	RCQ2_DATA *rcd=&RCQ2_config;
	RC_HIST   *rch=&rc_hist[frame_type];

	Double decay, history;
	Int history_length, rc_period;

	// test RC - adamli
	history_length = rc_hist[0].history_length;
	history = rc_hist[0].rate_history;
	decay = RCQ2_config.decay;
	rc_period = RCQ2_config.rc_period;
	if (history_length < rc_period) {
		history += vopbits;
	} else if (history_length == rc_period) {
		history += vopbits;
		history /= (history_length + 1);
		history /= (1. - decay);
	} else 
		history = vopbits + history * decay;
	rc_hist[0].rate_history = history;
	rc_hist[0].history_length ++;

	rch_store_after(rch, vopbits/*-vopheader*/, vopbits, 0/*dist*/);

	rc_update_Q2_model(rch, &rcd->X1[frame_type], &rcd->X2[frame_type]);

}


/***********************************************************HeaderBegin*******
 * LEVEL 2 (NO ACCESS TO STATIC VARS)
 ***********************************************************HeaderEnd*********/

/***********************************************************CommentBegin******
 *
 * -- RCQ2_compute_quant --
 *
 * Purpose :
 *      Q2 quantizer computation
 *
 * Arguments in :
 *      Double   bit_rate     -  Bit rate (bits per second)
 *      Int      bits_frame   -  Target bits per frame
 *      Int      bits_prev    -  Total bits previous frame
 *      Double   X1           -  Model parameter
 *      Double   X2           -  Model parameter
 *      Double   mad          -  Pred. error. magnitude
 *      Int      last_qp      -  Previous QP
 *
 ***********************************************************CommentEnd********/

Int RCQ2_compute_quant(
Double bit_rate,								  /* Bit rate (bits per second) */
Int    bits_frame,								  /* Target bits per frame */
Int    bits_prev,								  /* Total bits previous frame */
Int    buff_size,								  /* Buffer size */
Int    buff_occup,								  /* Buffer occupancy */
Double X1,										  /* Model parameters */
Double X2,
Double mad,										  /* Pred. error. magnitude */
Int    last_qp									  /* Previous QP */
)
{
	Double target=(Double)bits_frame;
	Double qp;

	target = floor(target);

	#ifdef _RC_
	fprintf(ftrace, "--> RCQ2_compute_quant\n");
	fprintf(ftrace, "RC: RC_Q2: --> RCQ2_compute_quant\n");
	fprintf(ftrace, "RC:        bits_frame= %d\n", bits_frame);
	fprintf(ftrace, "RC:        bits_prev= %d\n", bits_prev);
	fprintf(ftrace, "RC:        X1= %f\n", X1);
	fprintf(ftrace, "RC:        X2= %f\n", X2);
	fprintf(ftrace, "RC:        mad= %f\n", mad);
	fprintf(ftrace, "RC:        last_qp= %d\n", last_qp);
	fprintf(ftrace, "<-- RCQ2_compute_quant\n");
	fflush(ftrace);
	#endif

	#ifdef _RC_
	fprintf(ftrace, "RCQ2_compute_quant:        target0= %f\n", target);
	fflush(ftrace);
	#endif

	/*** Low-pass filtering of the target ***/
	if (bits_prev > 0)
		target = target*(1.-PAST_PERCENT)+bits_prev*PAST_PERCENT;
	target = floor(target);

	#ifdef _RC_
	fprintf(ftrace, "RCQ2_compute_quant:        target1= %f\n", target);
	fflush(ftrace);
	#endif

	/*** Lower-bounding of target ***/
	target = MAX(bit_rate/40, target);
	target = floor(target);

	#ifdef _RC_
	fprintf(ftrace, "RCQ2_compute_quant:        target3= %f\n", target);
	fflush(ftrace);
	#endif

	if (target<=0)
		return (max_quantizer);

	if (X2==0)
		qp = X1*mad/target;
	else
	{
		Double disc=X1*X1+4*X2*target/mad;
		if (disc < 0)
		{
			#ifdef _RC_DEBUG_
			fprintf(ftrace, "RCQ2_compute_quant: Second order model without solution: X2 set to 0\n");
			fflush(ftrace);
			#endif
			qp=X1*mad/target;
		}
		else
		{
			qp = (2*X2*mad)/(sqrt((X1*mad)*(X1*mad)+4*X2*mad*target)-X1*mad);
		}
	}
	#ifdef _RC_
	fprintf(ftrace, "RCQ2_compute_quant: qp0= %f\n", qp);
	#endif
	qp = (Int) floor(qp+0.5);

	#ifdef _RC_
	fprintf(ftrace, "RCQ2_compute_quant: qp1= %f\n", qp);
	fflush(ftrace);
	#endif

	/*** Bounding of QP variation ***/
	qp = MIN3(ceil(last_qp*1.25), qp, max_quantizer);
	qp = MAX3(ceil(last_qp*0.50), qp, min_quantizer);

	#ifdef _RC_
	fprintf(ftrace, "RCQ2_compute_quant: qp2= %f\n", qp);
	fflush(ftrace);
	#endif

	#ifdef _RC_
	fprintf(ftrace, "<-- RCQ2_compute_quant\n");
	fflush(ftrace);
	#endif

	return (Int)(qp+.5);
}


// added to do the 1st order rate control
// adamli - 12/01/2000
void rc_2ls_comp_q1(
Double *y,
Double *x1,
Int     n,										  /* Number of data */
Double *p1,
Double *p2
)
{
	/* testing 1st order rate control - adamli 12/3/2000 */
	Double sum = 0;
	Int i;

	for (i = 0; i < n; i++)
		sum += (y[i]) / (x1[i]);

	*p1 = sum / n;
	*p2 = 0.;

	return;
}

