/**************************************************************************

This software module was originally developed by
Nokia in the course of development of the MPEG-2 AAC/MPEG-4 
Audio standard ISO/IEC13818-7, 14496-1, 2 and 3.
This software module is an implementation of a part
of one or more MPEG-2 AAC/MPEG-4 Audio tools as specified by the
MPEG-2 aac/MPEG-4 Audio standard. ISO/IEC  gives users of the
MPEG-2aac/MPEG-4 Audio standards free license to this software module
or modifications thereof for use in hardware or software products
claiming conformance to the MPEG-2 aac/MPEG-4 Audio  standards. Those
intending to use this software module in hardware or software products
are advised that this use may infringe existing patents. The original
developer of this software module, the subsequent
editors and their companies, and ISO/IEC have no liability for use of
this software module or modifications thereof in an
implementation. Copyright is not released for non MPEG-2 aac/MPEG-4
Audio conforming products. The original developer retains full right to
use the code for the developer's own purpose, assign or donate the code to a
third party and to inhibit third party from using the code for non
MPEG-2 aac/MPEG-4 Audio conforming products. This copyright notice
must be included in all copies or derivative works.
Copyright (c)1997.  

***************************************************************************/
/**************************************************************************
  nok_lt_prediction.c  -  Performs Long Term Prediction for the MPEG-4
  			  T/F Decoder.
	Author(s): Mikko Suonio, Juha Ojanperä
	Nokia Research Center, Speech and Audio Systems, 1997.
  *************************************************************************/


/**************************************************************************
  Version Control Information			Method: CVS
  Identifiers:
  $Revision: 1.3 $
  $Date: 2001/05/09 21:15:11 $ (check in)
  $Author: cahighlander $
  *************************************************************************/

#include <stdio.h>

#include "all.h"
#include "dolby_def.h"
#include "block.h"
#include "nok_ltp_common.h"
#include "nok_lt_prediction.h"
#include "nok_ltp_common_internal.h"
#include "port.h"

/*
  Initialize the history buffer for long term prediction
 */
  
void nok_init_lt_pred (NOK_LT_PRED_STATUS *lt_status)
{
	int i;

	for (i = 0; i < NOK_LT_BLEN; i++)
		lt_status->buffer[i] = 0;

	lt_status->weight = 0;
	for(i=0; i<NSHORT; i++)
		lt_status->sbk_prediction_used[i] = lt_status->delay[i] = 0;

	for(i=0; i<MAX_SCFAC_BANDS; i++)
		lt_status->sfb_prediction_used[i] = 0;
}



/**************************************************************************
  Title:	nok_lt_prediction

  Purpose:	Performs long term prediction using given coefficients.

  Usage:        nok_lt_predict (profile, info, win_type, win_shape,
  				sbk_prediction_used, sfb_prediction_used,
				lt_status, weight, delay, current_frame)

  Input:	profile	- currently not used	
  		info	- information on the current frame
			  nsbk: number of subblocks (subwindows)
			        in a block (window sequence)
			  bins_per_sbk: number of spectral coefficients
			  		in each subblock; currently
					we assume that they are of
					equal size
			  sfb_per_bk: total number of scalefactor bands
			  	       in a block
			  sfb_per_sbk: number of scalefactor bands
			  	       in each subblock
		win_type
			- window sequence (frame, block) type
		win_shape
			- shape of the mdct window
  		sbk_prediction_used
			- first item toggles prediction on(1)/off(0) for
			  all subblocks, next items toggle it on/off on
			  each subblock separately
  		sfb_prediction_used
			- first item is not used, but the following items
			  toggle prediction on(1)/off(0) on each
			  scalefactor-band of every subblock
		lt_status
			- history buffer for prediction
		weight	- a weight factor to apply to all subblocks
		delay	- array of delays to apply to each subblock
		current_frame	- the dequantized spectral coeffients and
			  prediction errors where prediction is used
		block_size_long
			- size of the long block
		block_size_medium
			- size of the medium block
		block_size_short
			- size of the short block

  Output:	current_frame
  			- the dequantized spectrum possible with a
			  prediction vector added

  References:	1.) double_to_int

  Explanation:	-

  Author(s):	Mikko Suonio, Juha Ojanperä
  *************************************************************************/
void
nok_lt_predict (int profile, Info *info, WINDOW_TYPE win_type,
		Wnd_Shape *win_shape,
		int *sbk_prediction_used,
		int *sfb_prediction_used, NOK_LT_PRED_STATUS *lt_status,
		Float weight, int *delay, Float *current_frame,
		int block_size_long, int block_size_medium,
		int block_size_short
		)
{
  int i, j;
  float_ext current_frame_double[NOK_MAX_BLOCK_LEN_LONG];
  float_ext mdct_predicted[2 * NOK_MAX_BLOCK_LEN_LONG];
  float_ext overlap_buffer[2 * NOK_MAX_BLOCK_LEN_LONG];
  float_ext predicted_samples[2 * NOK_MAX_BLOCK_LEN_LONG];

  /* printf ("long: %d, med: %d, short: %d.\n", block_size_long,
     block_size_medium, block_size_short); */

  switch(win_type)
    {
    case ONLY_LONG_WINDOW:
    case LONG_START_WINDOW:
    case LONG_STOP_WINDOW:
		if (sbk_prediction_used[0])
		{
			/* Prediction for time domain signal */
			j = NOK_LT_BLEN - 2 * block_size_long - delay[0];
			for (i = 0; i < 2 * block_size_long; i++)
				predicted_samples[i] = weight * lt_status->buffer[i + j];

			/* Transform prediction to frequency domain. */
			time2freq_adapt ((byte)win_type, win_shape, predicted_samples, mdct_predicted);

			/* Clean those sfb's where prediction is not used. */
			for (i = 0, j = 0; i < info->sfb_per_bk; i++)
				if (sfb_prediction_used[i + 1] == 0)
					for (; j < info->sbk_sfb_top[0][i]; j++)
						mdct_predicted[j] = 0.0;
				else
					j = info->sbk_sfb_top[0][i];

			/* Add the prediction to dequantized spectrum. */
			for (i = 0; i < block_size_long; i++)
				current_frame[i] = current_frame[i] + mdct_predicted[i];
		}

		for (i = 0; i < block_size_long; i++)
		{
			current_frame_double[i] = (float_ext)current_frame[i];
			overlap_buffer[i] = 0;
		}

		/* Finally update the time domain history buffer. */
		freq2time_adapt ((byte)win_type, win_shape, current_frame_double, overlap_buffer, predicted_samples);

		for (i = 0; i < NOK_LT_BLEN - block_size_long; i++)
			lt_status->buffer[i] = lt_status->buffer[i + block_size_long];

		j = NOK_LT_BLEN - 2 * block_size_long;
		for (i = 0; i < block_size_long; i++)
		{
			lt_status->buffer[i + j] = double_to_int (predicted_samples[i] + lt_status->buffer[i + j]);
			lt_status->buffer[NOK_LT_BLEN - block_size_long + i] =
				double_to_int (overlap_buffer[i]);
		}
		break;

    case EIGHT_SHORT_WINDOW:
	/* Prepare the buffer for all forthcoming short windows at once.
	This could be done as the long window case, but that would
	be inefficient. We have to take this into account when
		referencing the buffer.  */
#if 0
		for (i = 0; i < NOK_LT_BLEN - block_size_long; i++)
			lt_status->buffer[i] = lt_status->buffer[i + block_size_long];

		for (i = NOK_LT_BLEN - block_size_long; i < NOK_LT_BLEN; i++)
			lt_status->buffer[i] = 0;

		for (i = 0; i < block_size_long; i++)
		{
			current_frame_double[i] = (float_ext)current_frame[i];
			overlap_buffer[i] = 0;
		}

		freq2time_adapt (win_type, win_shape, current_frame_double, overlap_buffer, predicted_samples);

		j = NOK_LT_BLEN - 2 * block_size_long + SHORT_SQ_OFFSET;
		for (i = 0; i < block_size_long; i++)
		{
			lt_status->buffer[i + j] = double_to_int (predicted_samples[i] + lt_status->buffer[i + j]);
			lt_status->buffer[i + block_size_long + j] =
				double_to_int (overlap_buffer[i]);
		}
#endif
		break;
    default:
		break;
    }

}

/**************************************************************************
  Title:	nok_lt_decode

  Purpose:	Decode the bitstream elements for long term prediction

  Usage:	nok_lt_decode (win_type, max_sfb, sbk_prediction_used,
  				sfb_prediction_used, weight, delay)


  Input:	win_type
			- window sequence (frame, block) type
		max_sfb - number of scalefactor bands used in
			  the current frame
  Output:	sbk_prediction_used
			- first item toggles prediction on(1)/off(0) for
			  all subblocks, next items toggle it on/off on
			  each subblock separately
  		sfb_prediction_used
			- first item is not used, but the following items
			  toggle prediction on(1)/off(0) on each
			  scalefactor-band of every subblock
		weight	- a weight factor to apply to all subblocks
		delay	- array of delays to apply to each subblock

  References:	1.) getbits

  Author(s):	Mikko Suonio
  *************************************************************************/
void
nok_lt_decode (WINDOW_TYPE win_type, int max_sfb, int *sbk_prediction_used,
	       int *sfb_prediction_used, Float *weight, int *delay)
{
	int i, j, last_band;
	int prev_subblock;
	

	if ((sbk_prediction_used[0] = getbits (LEN_LTP_DATA_PRESENT)))
    {
		delay[0] = getbits (LEN_LTP_LAG);
		*weight = (float)codebook[getbits (LEN_LTP_COEF)];

		// wmay - problem here - types don't match
		if ((int)win_type != (int)EIGHT_SHORT_WINDOW)
		{
			last_band = (max_sfb < NOK_MAX_LT_PRED_LONG_SFB
				? max_sfb : NOK_MAX_LT_PRED_LONG_SFB) + 1;
			
			sfb_prediction_used[0] = sbk_prediction_used[0];
			for (i = 1; i < last_band; i++)
				sfb_prediction_used[i] = getbits (LEN_LTP_LONG_USED);
			for (; i < max_sfb + 1; i++)
				sfb_prediction_used[i] = 0;
		}
		else
		{
			last_band = (max_sfb < NOK_MAX_LT_PRED_SHORT_SFB) ? max_sfb : NOK_MAX_LT_PRED_SHORT_SFB;
			
			prev_subblock = -1;
			for (i = 0; i < NSHORT; i++)
			{
				if ((sbk_prediction_used[i+1] = getbits (LEN_LTP_SHORT_USED)))
				{
					if(prev_subblock == -1)
					{
						delay[i] = delay[0];
						prev_subblock = i;
					}
					else
					{
						if (getbits (LEN_LTP_SHORT_LAG_PRESENT))
						{
							delay[i] = getbits (LEN_LTP_SHORT_LAG);
							delay[i] -= NOK_LTP_LAG_OFFSET;
							delay[i] = delay[prev_subblock] - delay[i];
						}
						else
							delay[i] = delay[prev_subblock];
					}
					
					for (j = 0; j < last_band; j++)
						sfb_prediction_used[i * last_band + j] = 1;
				}
			}
		}
    }
}
