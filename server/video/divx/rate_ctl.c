
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
 *  rate_ctl.c
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Adam Li
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/

/* This file contains some functions for rate control of the encoding.    */

#include "stdio.h"

extern FILE *ftrace;
extern int max_quantizer, min_quantizer;

typedef struct _rc_param_ {
	double quant;
	int rc_period;
	double target_rate;
	double average_rate;
	double reaction_rate;
	double average_delta;
	double reaction_delta;
	double reaction_ratio;
} RC_Param;

static RC_Param rc_param;

void RateCtlInit(double quant, double target_rate, 
	long rc_period, long rc_reaction_period, long rc_reaction_ratio)
{
#ifdef _RC_
	fprintf(ftrace, "Initializing Rate Control module:\n");
	fprintf(ftrace, "Initial quantizer is %f.\n", quant);
	fprintf(ftrace, "Target rate is %f bits per frame.\n", target_rate);
	fprintf(ftrace, "RC averaging period is %d.\n", rc_period);
	fprintf(ftrace, "RC reaction period is %d.\n", rc_reaction_period);
	fprintf(ftrace, "RC reaction ratio is %d.\n", rc_reaction_ratio);
#endif

	rc_param.quant = quant;
	rc_param.rc_period = rc_period;
	rc_param.target_rate = target_rate;
	rc_param.reaction_ratio = rc_reaction_ratio;

	rc_param.average_delta = 1. / rc_period;
	rc_param.reaction_delta = 1. / rc_reaction_period;	
	rc_param.average_rate = target_rate;
	rc_param.reaction_rate = target_rate;

	return;
}

int RateCtlGetQ(double MAD)
{
	double quant;

	quant = rc_param.quant;

//	if (MAD < 5.) 
//		quant = min_quantizer + (quant - min_quantizer) * MAD / 5.;

	return (int)(quant + 0.5);
}

void RateCtlUpdate(int current_frame)
{
	double rate, delta, decay;
	double target, current_target;
	double median_quant;

#ifdef _RC_
	fprintf(ftrace, "Quantizer is currently %f.\n", rc_param.quant);
	fprintf(ftrace, "Current frame is %d bits long.\n", current_frame);
#endif

	rate = rc_param.average_rate;
	delta = rc_param.average_delta;
	decay = 1 - delta;
	rate = rate * decay + current_frame * delta;
	rc_param.average_rate = rate;

	target = rc_param.target_rate;
	if (rate > target) {
		current_target = target - (rate - target);
		if (current_target < target * 0.75) current_target = target * 0.75;
	} else {
		current_target = target;
	}

#ifdef _RC_
	fprintf(ftrace, "Target rate is %f.\n", target);
	fprintf(ftrace, "Average rate is %f.\n", rate);
	fprintf(ftrace, "Target rate for current frame is %f.\n", current_target);
#endif

	rate = rc_param.reaction_rate;
	delta = rc_param.reaction_delta;
	decay = 1 - delta;
	rate = rate * decay + current_frame * delta;
	rc_param.reaction_rate = rate;

	median_quant = min_quantizer + (max_quantizer - min_quantizer) / 2;

	/* reduce quantizer when the reaction rate is low */
	if (rate < current_target) rc_param.quant *= 
		(1 - rc_param.reaction_delta * ((current_target - rate) / current_target / 0.20) );
	if (rc_param.quant < min_quantizer) rc_param.quant = min_quantizer;

	/* increase quantizer when the reaction rate is high */
	if (rate > current_target) {
		/* slower increasement when the quant is higher than median */
		if (rc_param.quant > median_quant) 		
			rc_param.quant *= (1 + rc_param.reaction_delta / rc_param.reaction_ratio);
		/* faster increasement when the quant is lower than median */
		else if (rate > current_target * 1.20) rc_param.quant *=
			(1 + rc_param.reaction_delta);
		else rc_param.quant *= 
			(1 + rc_param.reaction_delta * ((rate - current_target) / current_target / 0.20) );
	}
	if (rc_param.quant > max_quantizer) rc_param.quant = max_quantizer;

#ifdef _RC_
	fprintf(ftrace, "Reaction rate is %f.\n", rate);
	fprintf(ftrace, "Quantizer is updated to %f.\n", rc_param.quant);
#endif

	return;
}
	
