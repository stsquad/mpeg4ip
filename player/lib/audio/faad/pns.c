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
 * $Id: pns.c,v 1.4 2001/06/28 23:54:22 wmaycisco Exp $
 */

#include "all.h"


#define MEAN_NRG 1.5625e+18      /* Theory: (2^31)^2 / 3 = 1.537228e+18 */

static void random2(long *seed)
{
	*seed = (1664525L * *seed) + 1013904223L;  /* Numerical recipes */
}


static void gen_rand_vector(float *spec, int size, long *state)  
/* Noise generator, generating vector with unity energy */
{
    int i;
	float s, norm, nrg= 0.0;

	norm = 1.0f / (float)sqrt( size * MEAN_NRG );

	for (i=0; i<size; i++)
	{
		random2(state);
		spec[i] = (float)(*state * norm);   
		nrg += spec[i] * spec[i];
	}

	s = 1.0f / (float)sqrt( nrg );
	for (i=0; i<size; i++)
		spec[i] *= s;      
}



/*
 * if (noise correlated) {
 *   restore saved left channel random generator state
 *   generate random values
 * } else {
 *   save current random generator state
 *   generate random values
 * }
 * scale according to scalefactor
 *
 * Important: needs to be called left channel, then right channel
 *            for each channel pair
 */

void pns(faacDecHandle hDecoder, MC_Info *mip, Info *info, int widx, int ch,
		 byte *group, byte *cb_map, int *factors, 
		 int *lpflag, Float *coef[Chans] )
{
    Ch_Info *cip = &mip->ch_info[ch];
    Float   *spec, *fp, scale;
    int     cb, corr_flag, sfb, n, nn, b, bb, nband;
    int   *band;
    long    *nsp;

    /* store original predictor flags when left channel of a channel pair */
    if ((cip->cpe  &&  cip->ch_is_left  &&  info->islong))
		for (sfb=0; sfb<info->sfb_per_sbk[0]; sfb++)
			hDecoder->lp_store[sfb+1] = lpflag[sfb+1];

	/* restore original predictor flags when right channel of a channel pair */
	if ((cip->cpe  &&  !cip->ch_is_left  &&  info->islong))
		for (sfb=0; sfb<info->sfb_per_sbk[0]; sfb++)
			lpflag[sfb+1] = hDecoder->lp_store[sfb+1];

	spec = coef[ ch ];
	nsp = hDecoder->noise_state_save;

    /* PNS goes by group */
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
                if (cb == NOISE_HCB  ||  cb == NOISE_HCB+100) {
                    /* found noise  substitution code book */

					/* disable prediction (only important for long blocks) */
					if (info->islong)  lpflag[1+sfb] = 0;

                    /* determine left/right correlation */
					corr_flag = (cb != NOISE_HCB);

                    /* reconstruct noise substituted values */
                    /* generate random noise */
                    fp = spec + n;
                    if (corr_flag)  {
                        /* Start with stored state */
                        gen_rand_vector( fp, nn-n, nsp+sfb );
                    } else {
                        /* Store current state and go */
                        nsp[sfb] = hDecoder->cur_noise_state;
                        gen_rand_vector( fp, nn-n, &hDecoder->cur_noise_state );
                    }

                    /* scale to target energy */
                    scale = (float)pow( 2.0, 0.25*(factors[sfb]) );
					for (; n < nn; n++) {	/* n is coef index */
                        *fp++ *= scale;
					}
				}
				n = nn;
			}
            spec += info->bins_per_sbk[bb];
			factors += nband;
		}
        nsp += info->sfb_per_sbk[bb-1];
		cb_map += info->sfb_per_sbk[bb-1];
    }
}


/********************************************************************************
 *** FUNCTION: predict_pns_reset()                                              *
 ***										*
 ***    carry out predictor reset for PNS scalefactor bands (long blocks)       *
 ***										*
 ********************************************************************************/
void predict_pns_reset(Info* info, PRED_STATUS *psp, byte *cb_map)
{
    int    nband, sfb, i, top;
    int    *band;

    if (info->islong) {

        nband = info->sfb_per_sbk[0];
        band = info->sbk_sfb_top[0];

        for (i=0,sfb=0; sfb<nband; sfb++)  {

            top = band[sfb];
            if (cb_map[sfb] == NOISE_HCB  ||  cb_map[sfb] == NOISE_HCB+100) {
                for (; i<top; i++)
                    reset_pred_state(&psp[i]);
            }
            i = top;
        }

    } /* end islong */
}

