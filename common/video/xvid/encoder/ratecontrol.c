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

#include <stdio.h>
#include "encoder.h"
#include "ratecontrol.h"


extern FILE *ftrace;



void RateCtlInit(RateCtlParam * pCtl, double quant, double target_rate,

		 long rc_period, long rc_reaction_period,

		 long rc_reaction_ratio)

{

#ifdef _RC_

    fprintf(ftrace, "Initializing Rate Control module:\n");

    fprintf(ftrace, "Initial quantizer is %f.\n", quant);

    fprintf(ftrace, "Target rate is %f bits per frame.\n", target_rate);

    fprintf(ftrace, "RC averaging period is %d.\n", rc_period);

    fprintf(ftrace, "RC reaction period is %d.\n", rc_reaction_period);

    fprintf(ftrace, "RC reaction ratio is %d.\n", rc_reaction_ratio);

#endif



    pCtl->quant = quant;

    pCtl->rc_period = rc_period;

    pCtl->target_rate = target_rate;

    pCtl->reaction_ratio = rc_reaction_ratio;



    pCtl->average_delta = 1. / rc_period;

    pCtl->reaction_delta = 1. / rc_reaction_period;

    pCtl->average_rate = target_rate;

    pCtl->reaction_rate = target_rate;



    return;

}



int RateCtlGetQ(RateCtlParam * pCtl, double MAD)

{

    double quant;



    quant = pCtl->quant;



//      if (MAD < 5.) 

//              quant = pCtl->min_quant + (quant - pCtl->min_quant) * MAD / 5.;



    return (int) (quant + 0.5);

}



void RateCtlUpdate(RateCtlParam * pCtl, int current_frame)

{

    double rate, delta, decay;

    double target, current_target;

    double median_quant;



#ifdef _RC_

    fprintf(ftrace, "Quantizer is currently %f.\n", pCtl->quant);

    fprintf(ftrace, "Current frame is %d bits long.\n", current_frame);

#endif



    rate = pCtl->average_rate;

    delta = pCtl->average_delta;

    decay = 1 - delta;

    rate = rate * decay + current_frame * delta;

    pCtl->average_rate = rate;



    target = pCtl->target_rate;

    if (rate > target)

    {

	current_target = target - (rate - target);

	if (current_target < target * 0.75)

	    current_target = target * 0.75;

    }

    else

    {

	current_target = target;

    }



#ifdef _RC_

    fprintf(ftrace, "Target rate is %f.\n", target);

    fprintf(ftrace, "Average rate is %f.\n", rate);

    fprintf(ftrace, "Target rate for current frame is %f.\n", current_target);

#endif



    rate = pCtl->reaction_rate;

    delta = pCtl->reaction_delta;

    decay = 1 - delta;

    rate = rate * decay + current_frame * delta;

    pCtl->reaction_rate = rate;



    median_quant = pCtl->min_quant + (pCtl->max_quant - pCtl->min_quant) / 2;



    /* 

       reduce quantizer when the reaction rate is low */

    if (rate < current_target)

	pCtl->quant *=

	    (1 -

	     pCtl->reaction_delta * ((current_target - rate) /

				     current_target / 0.20));

    if (pCtl->quant < pCtl->min_quant)

	pCtl->quant = pCtl->min_quant;



    /* 

       increase quantizer when the reaction rate is high */

    if (rate > current_target)

    {

	/* 

	   slower increasement when the quant is higher than median */

	if (pCtl->quant > median_quant)

	    pCtl->quant *= (1 + pCtl->reaction_delta / pCtl->reaction_ratio);

	/* 

	   faster increasement when the quant is lower than median */

	else if (rate > current_target * 1.20)

	    pCtl->quant *= (1 + pCtl->reaction_delta);

	else

	    pCtl->quant *=

		(1 +

		 pCtl->reaction_delta * ((rate - current_target) /

					 current_target / 0.20));

    }

    if (pCtl->quant > pCtl->max_quant)

	pCtl->quant = pCtl->max_quant;



#ifdef _RC_

    fprintf(ftrace, "Reaction rate is %f.\n", rate);

    fprintf(ftrace, "Quantizer is updated to %f.\n", pCtl->quant);

#endif



    return;

}

