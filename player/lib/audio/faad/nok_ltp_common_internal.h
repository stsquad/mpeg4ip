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
 * $Id: nok_ltp_common_internal.h,v 1.4 2001/12/14 22:17:45 wmaycisco Exp $
 */

#ifndef _NOK_LTP_COMMON_INTERNAL_H
#define _NOK_LTP_COMMON_INTERNAL_H


/*
  Purpose:      Number of LTP coefficients. */
#define LPC 1

/*
  Purpose:      Maximum LTP lag.  */
#define MAX_LTP_DELAY 2048

/*
  Purpose:	Length of the bitstream element ltp_data_present.  */
#define	LEN_LTP_DATA_PRESENT 1

/*
  Purpose:	Length of the bitstream element ltp_lag.  */
#define	LEN_LTP_LAG 11

/*
  Purpose:	Length of the bitstream element ltp_coef.  */
#define	LEN_LTP_COEF 3

/*
  Purpose:	Length of the bitstream element ltp_short_used.  */
#define	LEN_LTP_SHORT_USED 1

/*
  Purpose:	Length of the bitstream element ltp_short_lag_present.  */
#define	LEN_LTP_SHORT_LAG_PRESENT 1

/*
  Purpose:	Length of the bitstream element ltp_short_lag.  */
#define	LEN_LTP_SHORT_LAG 5

/*
  Purpose:	Offset of the lags written in the bitstream.  */
#define	NOK_LTP_LAG_OFFSET 16

/*
  Purpose:	Length of the bitstream element ltp_long_used.  */
#define	LEN_LTP_LONG_USED 1

/*
  Purpose:	Upper limit for the number of scalefactor bands
   		which can use lt prediction with long windows.
  Explanation:	Bands 0..NOK_MAX_LT_PRED_SFB-1 can use lt prediction.  */
#define	NOK_MAX_LT_PRED_LONG_SFB 40

/*
  Purpose:	Upper limit for the number of scalefactor bands
   		which can use lt prediction with short windows.
  Explanation:	Bands 0..NOK_MAX_LT_PRED_SFB-1 can use lt prediction.  */
#define	NOK_MAX_LT_PRED_SHORT_SFB 8

/*
   Purpose:      Buffer offset to maintain block alignment.
   Explanation:  This is only used for a short window sequence.  */
#define SHORT_SQ_OFFSET (BLOCK_LEN_LONG-(BLOCK_LEN_SHORT*4+BLOCK_LEN_SHORT/2))

/*
  Purpose:	Number of codes for LTP weight. */
#define CODESIZE 8

/*
   Purpose:      Float type for external data.
   Explanation:  - */
typedef Float float_ext;

/*
  Purpose:	Codebook for LTP weight coefficients.  */
static Float codebook[CODESIZE] =
{
  0.570829f,
  0.696616f,
  0.813004f,
  0.911304f,
  0.984900f,
  1.067894f,
  1.194601f,
  1.369533f
};

#endif /* _NOK_LTP_COMMON_INTERNAL_H */
