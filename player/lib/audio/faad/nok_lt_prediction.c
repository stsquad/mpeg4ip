/*
 * FAAD - Freeware Advanced Audio Decoder
 * Copyright (C) 2001 Menno Bakker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: nok_lt_prediction.c,v 1.4 2001/06/28 23:54:22 wmaycisco Exp $
 */

#ifdef WIN32
#include <windows.h>
#endif
#include <stdio.h>

#include "all.h"
#include "tns.h"
#include "block.h"
#include "nok_ltp_common.h"
#include "nok_lt_prediction.h"
#include "nok_ltp_common_internal.h"
#include "port.h"
#include "bits.h"
#include "util.h"

/*
  Initialize the history buffer for long term prediction
 */

void nok_init_lt_pred(NOK_LT_PRED_STATUS **lt_status, int channels)
{
	int ch;

	for (ch = 0; ch < channels; ch++) {
		lt_status[ch]->buffer = AllocMemory(NOK_LT_BLEN*sizeof(float));
#ifndef WIN32
		SetMemory(lt_status[ch]->buffer, 0, NOK_LT_BLEN*sizeof(float));
#endif
	}
}

void nok_end_lt_pred(NOK_LT_PRED_STATUS **lt_status, int channels)
{
	int ch;

	for (ch = 0; ch < channels; ch++) {
		if (lt_status[ch]->buffer) FreeMemory(lt_status[ch]->buffer);
	}
}


/**************************************************************************
  nok_lt_prediction
 *************************************************************************/

void nok_lt_predict(faacDecHandle hDecoder, Info *info, WINDOW_TYPE win_type, Wnd_Shape *win_shape,
					int *sbk_prediction_used, int *sfb_prediction_used, 
					NOK_LT_PRED_STATUS *lt_status, Float weight, int *delay, 
					Float *current_frame, int block_size_long, int block_size_medium, 
					int block_size_short, TNS_frame_info *tns_frame_info)
{
	int i, j, num_samples;
	float_ext *mdct_predicted;
	float_ext *predicted_samples;

	mdct_predicted = AllocMemory(2*NOK_MAX_BLOCK_LEN_LONG*sizeof(float_ext));
	predicted_samples = AllocMemory(2*NOK_MAX_BLOCK_LEN_LONG*sizeof(float_ext));


	switch(win_type) {

    case ONLY_LONG_WINDOW:
    case LONG_START_WINDOW:
    case LONG_STOP_WINDOW:
		if (sbk_prediction_used[0])
		{
			/* Prediction for time domain signal */
			num_samples =  2 * block_size_long;
			j = NOK_LT_BLEN - 2 * block_size_long - (delay[0] - DELAY / 2);
			if(NOK_LT_BLEN - j <  2 * block_size_long)
				num_samples = NOK_LT_BLEN - j;

			for(i = 0; i < num_samples; i++)
				predicted_samples[i] = weight * lt_status->buffer[i + j];
			for( ; i < 2 * block_size_long; i++)
				predicted_samples[i] = 0.0f;
			
			/* Transform prediction to frequency domain. */
			time2freq_adapt(hDecoder, win_type, win_shape, predicted_samples, mdct_predicted);
			
			/* Apply the TNS analysis filter to the predicted spectrum. */
			if(tns_frame_info != NULL)
				tns_filter_subblock(hDecoder, mdct_predicted, info->sfb_per_bk, info->sbk_sfb_top[0], 
				1, &tns_frame_info->info[0]);
			
			/* Clean those sfb's where prediction is not used. */
			for (i = 0, j = 0; i < info->sfb_per_bk; i++) {
				if (sfb_prediction_used[i + 1] == 0) {
					for (; j < info->sbk_sfb_top[0][i]; j++)
						mdct_predicted[j] = 0.0;
				} else {
					j = info->sbk_sfb_top[0][i];
				}
			}
					
			/* Add the prediction to dequantized spectrum. */
			for (i = 0; i < block_size_long; i++)
				current_frame[i] = current_frame[i] + mdct_predicted[i];
		}
		break;

	default:
		break;
	}

	FreeMemory(mdct_predicted);
	FreeMemory(predicted_samples);
}

/**************************************************************************
  nok_lt_update
 *************************************************************************/

void nok_lt_update(NOK_LT_PRED_STATUS *lt_status, Float *time_signal, 
				   Float *overlap_signal, int block_size_long) 
{
	int i;

	for(i = 0; i < NOK_LT_BLEN - 2 * block_size_long; i++)
		lt_status->buffer[i] = lt_status->buffer[i + block_size_long];

	for(i = 0; i < block_size_long; i++) 
	{
		lt_status->buffer[NOK_LT_BLEN - 2 * block_size_long + i] = 
			time_signal[i];
		
		lt_status->buffer[NOK_LT_BLEN - block_size_long + i] = 
			overlap_signal[i];
	}
}

/**************************************************************************
  nok_lt_decode
 *************************************************************************/
void nok_lt_decode(faacDecHandle hDecoder, int max_sfb, int *sbk_prediction_used,
				   int *sfb_prediction_used, Float *weight, int *delay)
{
	int i, last_band;

	if ((sbk_prediction_used[0] = faad_getbits(&hDecoder->ld, LEN_LTP_DATA_PRESENT)))
	{
		delay[0] = faad_getbits(&hDecoder->ld, LEN_LTP_LAG);
		*weight = codebook[faad_getbits(&hDecoder->ld, LEN_LTP_COEF)];

		last_band = (max_sfb < NOK_MAX_LT_PRED_LONG_SFB
			? max_sfb : NOK_MAX_LT_PRED_LONG_SFB) + 1;

		sfb_prediction_used[0] = sbk_prediction_used[0];
		for (i = 1; i < last_band; i++)
			sfb_prediction_used[i] = faad_getbits(&hDecoder->ld, LEN_LTP_LONG_USED);
		for (; i < max_sfb + 1; i++)
			sfb_prediction_used[i] = 0;
	}
}
