/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS in the course of 
development of the MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 
14496-1,2 and 3. This software module is an implementation of a part of one or more 
MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4 
Audio standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio 
standards free license to this software module or modifications thereof for use in 
hardware or software products claiming conformance to the MPEG-2 NBC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or 
software products are advised that this use may infringe existing patents. 
The original developer of this software module and his/her company, the subsequent 
editors and their companies, and ISO/IEC have no liability for use of this software 
module or modifications thereof in an implementation. Copyright is not released for 
non MPEG-2 NBC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the 
code to a third party and to inhibit third party from using the code for non 
MPEG-2 NBC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works." 
Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/
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
