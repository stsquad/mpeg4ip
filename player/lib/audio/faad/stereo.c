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
 * $Id: stereo.c,v 1.4 2001/06/28 23:54:22 wmaycisco Exp $
 */

#include "all.h"

void synt(Info *info, byte *group, byte *mask, Float *right, Float *left)
{
    Float vrr, vrl, *r, *l;
    int *band;
    int i, n, nn, b, bb, nband;

    /*mask is grouped */
    bb = 0;
    for(b = 0; b < info->nsbk; ){
		nband = info->sfb_per_sbk[b];
		band = info->sbk_sfb_top[b];

		b = *group++;		/*b = index of last sbk in group */
		for(; bb < b; bb++){	/* bb = sbk index */
			n = 0;
			for(i = 0; i < nband; i++){
				nn = band[i];	/* band is offset table, nn is last coef in band */
				if(mask[i]){
					r = right + n;
					l = left + n;
					for(; n < nn; n++){	/* n is coef index */
						vrr = *r;
						vrl = *l;
						*l = vrr + vrl;
						*r = vrl - vrr;
						r++;
						l++;
					}
				}
				n = nn;
			}
			right += info->bins_per_sbk[bb];
			left += info->bins_per_sbk[bb];

		}
		mask += info->sfb_per_sbk[bb-1];
    }
}


/* Map mask to intensity stereo signalling */
void map_mask(Info *info, byte *group, byte *mask, byte *cb_map)
{
    int sfb, b, nband;

    /* mask goes by group */
    for (b = 0; b < info->nsbk; ) {
		nband = info->sfb_per_sbk[b];

		for (sfb = 0; sfb<nband; sfb++){
			if (mask[sfb]) {
				if (cb_map[sfb] == INTENSITY_HCB)  {
					cb_map[sfb] = INTENSITY_HCB2;
					mask[sfb] = 0;
				} else if (cb_map[sfb] == INTENSITY_HCB2)  {
					cb_map[sfb] = INTENSITY_HCB;
					mask[sfb] = 0;
				} else if (cb_map[sfb] == NOISE_HCB)  {
					cb_map[sfb] = NOISE_HCB + 100;
					mask[sfb] = 0;
				}
			}
		}

		mask += info->sfb_per_sbk[b];
		cb_map += info->sfb_per_sbk[b];
		b = *group++;		/* b = index of last sbk in group */
    }
}

