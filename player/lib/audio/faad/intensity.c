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
 * $Id: intensity.c,v 1.4 2001/06/28 23:54:22 wmaycisco Exp $
 */

#include <math.h>
#include "all.h"

/*
 * if (chan==RIGHT) { 
 *  do IS decoding for this channel (scale left ch. values with
 *  factor(SFr-SFl) )
 *  reset all lpflags for which IS is on
 *  pass decoded IS values to predict
 * }
 */
void intensity( MC_Info *mip, Info *info, int widx, int ch,
			   byte *group, byte *cb_map, int *factors, 
			   int *lpflag, Float *coef[Chans] )
{
    Ch_Info *cip = &mip->ch_info[ch];
    Float   *left, *right, *r, *l, scale;
    int     cb, sign_sfb, sfb, n, nn, b, bb, nband;
    int   *band;
    
    if (!(cip->cpe && !cip->ch_is_left))
		return;
	
    right = coef[ ch ];
    left  = coef[ cip->paired_ch ];
    
    /* Intensity goes by group */
    bb = 0;
    for (b = 0; b < info->nsbk; ) {
	nband = info->sfb_per_sbk[b];
	band = info->sbk_sfb_top[b];

	b = *group++;		/* b = index of last sbk in group */
	for (; bb < b; bb++) {	/* bb = sbk index */
	    n = 0;
	    for (sfb = 0; sfb < nband; sfb++){
		nn = band[sfb];	/* band is offset table, nn is last coef in band */
		cb = cb_map[sfb];
		if (cb == INTENSITY_HCB  ||  cb == INTENSITY_HCB2) {
		    /* found intensity code book */ 
		    /* disable prediction (only important for long blocks) */
		    lpflag[1+sfb] = 0;
		    
	            sign_sfb = (cb == INTENSITY_HCB) ? 1 : -1;
	            scale = sign_sfb * (float)pow( 0.5, 0.25*(factors[sfb]) );

		    /* reconstruct right intensity values */
		    r = right + n;
		    l = left + n;
		    for (; n < nn; n++) {	/* n is coef index */
			*r++ = *l++ * scale;
		    }
		}
		n = nn;
	    }
	    right += info->bins_per_sbk[bb];
	    left += info->bins_per_sbk[bb];
	    factors += nband;
	}
	cb_map += info->sfb_per_sbk[bb-1];
    }
}
