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
 * $Id: nok_ltp_common.h,v 1.3 2001/06/28 23:54:22 wmaycisco Exp $
 */

#ifndef _NOK_LTP_COMMON_H
#define _NOK_LTP_COMMON_H

/*
  Macro:	MAX_SHORT_WINDOWS
  Purpose:	Number of short windows in one long window.
  Explanation:	-  */
#ifndef MAX_SHORT_WINDOWS
#define MAX_SHORT_WINDOWS NSHORT
#endif

/*
  Macro:	MAX_SCFAC_BANDS
  Purpose:	Maximum number of scalefactor bands in one frame.
  Explanation:	-  */
#ifndef MAX_SCFAC_BANDS
#define MAX_SCFAC_BANDS MAXBANDS
#endif

/*
  Macro:	BLOCK_LEN_LONG
  Purpose:	Length of one long window
  Explanation:	-  */
#ifndef BLOCK_LEN_LONG
#define BLOCK_LEN_LONG LN2
#endif

/*
  Macro:	NOK_MAX_BLOCK_LEN_LONG
  Purpose:	Informs the routine of the maximum block size used.
  Explanation:	-  */
#define	NOK_MAX_BLOCK_LEN_LONG (BLOCK_LEN_LONG) 

/*
  Macro:	NOK_LT_BLEN
  Purpose:	Length of the history buffer.
  Explanation:	Has to hold 1.5 long windows of time domain data. */
#ifndef	NOK_LT_BLEN
#define NOK_LT_BLEN (3 * NOK_MAX_BLOCK_LEN_LONG)
#endif

/*
  Type:		NOK_LT_PRED_STATUS
  Purpose:	Type of the struct holding the LTP encoding parameters.
  Explanation:	-  */
typedef struct
  {
    int weight_idx;
    float weight;
    int sbk_prediction_used[MAX_SHORT_WINDOWS];
    int sfb_prediction_used[MAX_SCFAC_BANDS];
    int delay[MAX_SHORT_WINDOWS];
    int global_pred_flag;
    int side_info;
    float *buffer;
  }
NOK_LT_PRED_STATUS;


#endif /* _NOK_LTP_COMMON_H */
