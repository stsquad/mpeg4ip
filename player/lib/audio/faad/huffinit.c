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
 * $Id: huffinit.c,v 1.4 2001/06/28 23:54:22 wmaycisco Exp $
 */

#include <math.h>
#include "all.h"

void huffbookinit(faacDecHandle hDecoder)
{
    int i;

	hufftab(&book[1], book1, 4, HUF1SGN);
	hufftab(&book[2], book2, 4, HUF2SGN);
	hufftab(&book[3], book3, 4, HUF3SGN);
	hufftab(&book[4], book4, 4, HUF4SGN);
	hufftab(&book[5], book5, 2, HUF5SGN);
	hufftab(&book[6], book6, 2, HUF6SGN);
	hufftab(&book[7], book7, 2, HUF7SGN);
	hufftab(&book[8], book8, 2, HUF8SGN);
	hufftab(&book[9], book9, 2, HUF9SGN);
	hufftab(&book[10], book10, 2, HUF10SGN);
	hufftab(&book[11], book11, 2, HUF11SGN);

    for(i = 0; i < TEXP; i++)
	{
		hDecoder->exptable[i]  = (float)pow( 2.0, 0.25*i);
    }

    for(i = 0; i < MAX_IQ_TBL; i++)
	{
		hDecoder->iq_exp_tbl[i] = (float)pow(i, 4./3.);
    }

    infoinit(hDecoder, &samp_rate_info[hDecoder->mc_info.sampling_rate_idx]);
}

void infoinit(faacDecHandle hDecoder, SR_Info *sip)
{ 
    int i, j, k, n, ws;
    int *sfbands;
    Info *ip;

    /* long block info */
    ip = &hDecoder->only_long_info;
    hDecoder->win_seq_info[ONLY_LONG_WINDOW] = ip;
    ip->islong = 1;
    ip->nsbk = 1;
    ip->bins_per_bk = LN2;
    for (i=0; i<ip->nsbk; i++) {
		ip->sfb_per_sbk[i] = sip->nsfb1024;
		ip->sectbits[i] = LONG_SECT_BITS;
		ip->sbk_sfb_top[i] = sip->SFbands1024;
    }
    ip->sfb_width_128 = NULL;
    ip->num_groups = 1;
    ip->group_len[0] = 1;
    ip->group_offs[0] = 0;
    
    /* short block info */
    ip = &hDecoder->eight_short_info;
    hDecoder->win_seq_info[EIGHT_SHORT_WINDOW] = ip;
    ip->islong = 0;
    ip->nsbk = NSHORT;
    ip->bins_per_bk = LN2;
    for (i=0; i<ip->nsbk; i++) {
		ip->sfb_per_sbk[i] = sip->nsfb128;
		ip->sectbits[i] = SHORT_SECT_BITS;
		ip->sbk_sfb_top[i] = sip->SFbands128;
    }
    /* construct sfb width table */
    ip->sfb_width_128 = sfbwidth128;
    for (i=0, j=0, n=sip->nsfb128; i<n; i++) {
		k = sip->SFbands128[i];
		sfbwidth128[i] = k - j;
		j = k;
    }
    
    /* common to long and short */
    for (ws=0; ws<NUM_WIN_SEQ; ws++) {
        if ((ip = hDecoder->win_seq_info[ws]) == NULL)
			continue;
		
		ip->sfb_per_bk = 0;   
		k = 0;
		n = 0;
        for (i=0; i<ip->nsbk; i++) {
            /* compute bins_per_sbk */
			ip->bins_per_sbk[i] = ip->bins_per_bk / ip->nsbk;
			
            /* compute sfb_per_bk */
            ip->sfb_per_bk += ip->sfb_per_sbk[i];

            /* construct default (non-interleaved) bk_sfb_top[] */
            sfbands = ip->sbk_sfb_top[i];
            for (j=0; j < ip->sfb_per_sbk[i]; j++)
                ip->bk_sfb_top[j+k] = sfbands[j] + n;

            n += ip->bins_per_sbk[i];
            k += ip->sfb_per_sbk[i];
		}	    
    }	    
}
