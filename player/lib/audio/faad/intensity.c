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
 * $Id: intensity.c,v 1.6 2002/01/11 00:55:17 wmaycisco Exp $
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

    b = *group++;       /* b = index of last sbk in group */
    for (; bb < b; bb++) {  /* bb = sbk index */
        n = 0;
        for (sfb = 0; sfb < nband; sfb++){
        nn = band[sfb]; /* band is offset table, nn is last coef in band */
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
            for (; n < nn; n++) {   /* n is coef index */
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
