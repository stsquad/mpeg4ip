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
/*
 * $Id: stereo.c,v 1.6 2002/01/11 00:55:17 wmaycisco Exp $
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

        b = *group++;       /*b = index of last sbk in group */
        for(; bb < b; bb++){    /* bb = sbk index */
            n = 0;
            for(i = 0; i < nband; i++){
                nn = band[i];   /* band is offset table, nn is last coef in band */
                if(mask[i]){
                    r = right + n;
                    l = left + n;
                    for(; n < nn; n++){ /* n is coef index */
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
        b = *group++;       /* b = index of last sbk in group */
    }
}

