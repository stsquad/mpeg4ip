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
 * $Id: nok_lt_prediction.h,v 1.4 2001/06/28 23:54:22 wmaycisco Exp $
 */

#ifndef _NOK_LT_PREDICTION_H
#define _NOK_LT_PREDICTION_H

#include "all.h"
#include "block.h"
#include "nok_ltp_common.h"

#define truncate(sig_in) \
         ((sig_in) > 32767 ? 32767 : ( \
         (sig_in) < -32768 ? -32768 : ( \
         (sig_in) > 0.0 ? (sig_in)+0.5 : ( \
         (sig_in) <= 0.0 ? (sig_in)-0.5 : 0)))) 


extern void nok_init_lt_pred(NOK_LT_PRED_STATUS **lt_status, int channels);
extern void nok_end_lt_pred(NOK_LT_PRED_STATUS **lt_status, int channels);

extern void nok_lt_predict(faacDecHandle hDecoder, Info *info, WINDOW_TYPE win_type, Wnd_Shape *win_shape,
						   int *sbk_prediction_used, int *sfb_prediction_used, 
						   NOK_LT_PRED_STATUS *lt_status, Float weight, int *delay, 
						   Float *current_frame, int block_size_long, int block_size_medium, 
						   int block_size_short, TNS_frame_info *tns_frame_info);

extern void nok_lt_update(NOK_LT_PRED_STATUS *lt_status, Float *time_signal, 
						  Float *overlap_signal, int block_size_long);

extern void nok_lt_decode(faacDecHandle hDecoder, int max_sfb, int *sbk_prediction_used, 
						  int *sfb_prediction_used, Float *weight, int *delay);

#endif /* not defined _NOK_LT_PREDICTION_H */
