/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by
Bernd Edler and Hendrik Fuchs, University of Hannover in the course of
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
 * $Id: monopred.c,v 1.4 2002/01/11 00:55:17 wmaycisco Exp $
 */

/***********************************************************************************
 * MONOPRED                                    *
 *                                         *
 *  Contains the core functions for an intra channel (or mono) predictor       *
 *  using a backward adaptive lattice predictor.                   *
 *                                         *
 *  init_pred_stat():   initialisation of all predictor parameters     *
 *  monopred():     calculation of a predicted value from          *
 *              preceeding (quantised) samples             *
 *  predict():      carry out prediction for all spectral lines    *
 *  predict_reset():    carry out cyclic predictor reset mechanism     *
 *              (long blocks) resp. full reset (short blocks)      *
 *                                         *
 *  Internal Functions:                            *
 *    reset_pred_state():   reset the predictor state variables        *
 *                                         *
 **********************************************************************************/

#include "all.h"
#include "util.h"

#define GRAD PRED_ORDER
#define ALPHA PRED_ALPHA
#define A PRED_A
#define B PRED_B


/* this works for all float values,
 * but does not conform to IEEE conventions of
 * round to nearest, even
 */

/* Schuyler's bug fix */
static void flt_round(float *pf)
{
    int flg;
    u_int32_t tmp;
    float *pt = (float *)&tmp;
    *pt = *pf;
    flg = tmp & (u_int32_t)0x00008000;
    tmp &= (u_int32_t)0xffff0000;
    *pf = *pt;
    /* round 1/2 lsb toward infinity */
    if (flg) {
        tmp &= (u_int32_t)0xff800000;       /* extract exponent and sign */
        tmp |= (u_int32_t)0x00010000;       /* insert 1 lsb */
        *pf += *pt;                     /* add 1 lsb and elided one */
        tmp &= (u_int32_t)0xff800000;       /* extract exponent and sign */
        *pf -= *pt;                     /* subtract elided one */
    }
}


/* This only works for 1.0 < float < 2.0 - 2^-24 !
 *
 * Comparison of the performance of the two rounding routines:
 *      old (above) new (below)
 * Max error    0.00385171  0.00179992
 * RMS error    0.00194603  0.00109221
 */

/* New bug fixed version */
static void inv_table_flt_round(float *ftmp)
{
    int exp;
    double mnt;
    float descale;

    mnt = frexp((double)*ftmp, &exp);
    descale = (float)ldexp(1.0, exp + 15);
    *ftmp += descale;
    *ftmp -= descale;
}

static void make_inv_tables(faacDecHandle hDecoder)
{
    int i;
    u_int32_t tmp1, tmp;
    float *pf = (float *)&tmp;
    float ftmp;

    *pf = 1.0;
    tmp1 = tmp;             /* float 1.0 */
    for (i=0; i<128; i++) {
        tmp = tmp1 + (i<<16);       /* float 1.m, 7 msb only */
        ftmp = B / *pf;
        inv_table_flt_round(&ftmp); /* round to 16 bits */
        hDecoder->mnt_table[i] = ftmp;
        /* printf("%3d %08x %f\n", i, tmp, ftmp); */
    }
    for (i=0; i<256; i++) {
        tmp = (i<<23);          /* float 1.0 * 2^exp */
        if (*pf > MINVAR) {
            ftmp = 1.0f / *pf;
        } else {
            ftmp = 0;
        }
        hDecoder->exp_table[i] = ftmp;
        /* printf("%3d %08x %g\n", i, tmp, ftmp); */
    }
}

/* Bug-fixed version (big-little endian problem) */
static void inv_quant_pred_state(TMP_PRED_STATUS *tmp_psp, PRED_STATUS *psp)
{
    int i;
    short *p2;
    u_int32_t *p1_tmp;

    p1_tmp = (u_int32_t *)tmp_psp;
    p2 = (short *) psp;

    for (i=0; i<MAX_PRED_BINS*6; i++)
        p1_tmp[i] = ((u_int32_t)p2[i])<<16;
}

#define FAST_QUANT

/* Bug-fixed version (big-little endian problem) */
static void quant_pred_state(PRED_STATUS *psp, TMP_PRED_STATUS *tmp_psp)
{
    int i;
    short *p1;
    u_int32_t *p2_tmp;

#ifdef  FAST_QUANT
    p1 = (short *) psp;
    p2_tmp = (u_int32_t *)tmp_psp;

    for (i=0; i<MAX_PRED_BINS*6;i++)
      p1[i] = (short) (p2_tmp[i]>>16);

#else
    int j;
    for (i=0; i<MAX_PRED_BINS; i++) {
    p1 = (short *) &psp[i];
      p2_tmp = (u_int32_t *)tmp_psp;

  for (j=0; j<6; j++)
        p1[j] = (short) (p2_tmp[i]>>16);
    }
#endif
}

/********************************************************************************
 *** FUNCTION: reset_pred_state()                       *
 ***                                        *
 ***    reset predictor state variables                     *
 ***                                        *
 ********************************************************************************/
void reset_pred_state(PRED_STATUS *psp)
{
    psp->r[0] = Q_ZERO;
    psp->r[1] = Q_ZERO;
    psp->kor[0] = Q_ZERO;
    psp->kor[1] = Q_ZERO;
    psp->var[0] = Q_ONE;
    psp->var[1] = Q_ONE;
}

/********************************************************************************
 *** FUNCTION: init_pred_stat()                         *
 ***                                        *
 ***    initialisation of all predictor parameter               *
 ***                                        *
 ********************************************************************************/
void init_pred_stat(faacDecHandle hDecoder, PRED_STATUS *psp, int first_time)
{
    /* Initialisation */
    if (first_time) {
        make_inv_tables(hDecoder);
    }

    reset_pred_state(psp);
}

void init_pred(faacDecHandle hDecoder, PRED_STATUS **sp_status, int channels)
{
    int i, ch;

    for (ch = 0; ch < channels; ch++) {
        for (i = 0; i < LN2; i++) {
            init_pred_stat(hDecoder, &sp_status[ch][i], ((ch==0)&&(i==0)));
        }
    }
}

/********************************************************************************
 *** FUNCTION: monopred()                           *
 ***                                        *
 ***    calculation of a predicted value from preceeding (quantised) samples    *
 ***    using a second order backward adaptive lattice predictor with full  *
 ***    LMS adaption algorithm for calculation of predictor coefficients    *
 ***                                        *
 ***    parameters: pc: pointer to this quantised sample        *
 ***            psp:    pointer to structure with predictor status  *
 ***            pred_flag:  1 if prediction is used         *
 ***                                        *
 ********************************************************************************/

static void monopred(faacDecHandle hDecoder, Float *pc, PRED_STATUS *psp, TMP_PRED_STATUS *pst, int pred_flag)
{
    float qc = *pc;     /* quantized coef */
    float pv;           /* predicted value */
    float dr1;          /* difference in the R-branch */
    float e0,e1;        /* "partial" prediction errors (E-branch) */
    float r0,r1;        /* content of delay elements */
    float k1,k2;        /* predictor coefficients */

    float *R = pst->r;      /* content of delay elements */
    float *KOR = pst->kor;  /* estimates of correlations */
    float *VAR = pst->var;  /* estimates of variances */
    u_int32_t tmp;
    int i, j;

    r0=R[0];
    r1=R[1];

    /* Calculation of predictor coefficients to be used for the
     * calculation of the current predicted value based on previous
     * block's state
     */

    /* the test, division and rounding is be pre-computed in the tables
     * equivalent calculation is:
     * k1 = (VAR[1-1]>MINVAR) ? KOR[1-1]/VAR[1-1]*B : 0.0F;
     * k2 = (VAR[2-1]>MINVAR) ? KOR[2-1]/VAR[2-1]*B : 0.0F;
     */
    tmp = psp->var[1-1];
    j = (tmp >> 7);
    i = tmp & 0x7f;
    k1 = KOR[1-1] * hDecoder->exp_table[j] * hDecoder->mnt_table[i];

    tmp = psp->var[2-1];
    j = (tmp >> 7);
    i = tmp & 0x7f;
    k2 = KOR[2-1] * hDecoder->exp_table[j] * hDecoder->mnt_table[i];

    /* Predicted value */
    pv  = k1*r0 + k2*r1;
    flt_round(&pv);
    if (pred_flag)
        *pc = qc + pv;
/* printf("P1: %8.2f %8.2f\n", pv, *pc); */

    /* Calculate state for use in next block */

    /* E-Branch:
     *  Calculate the partial prediction errors using the old predictor coefficients
     *  and the old r-values in order to reconstruct the predictor status of the
     *  previous step
     */

    e0 = *pc;
    e1 = e0-k1*r0;

    /* Difference in the R-Branch:
     *  Calculate the difference in the R-Branch using the old predictor coefficients and
     *  the old partial prediction errors as calculated above in order to reconstruct the
     *  predictor status of the previous step
     */

    dr1 = k1*e0;

    /* Adaption of variances and correlations for predictor coefficients:
     *  These calculations are based on the predictor status of the previous step and give
     *  the new estimates of variances and correlations used for the calculations of the
     *  new predictor coefficients to be used for calculating the current predicted value
     */

    VAR[1-1] = ALPHA*VAR[1-1]+(0.5F)*(r0*r0 + e0*e0);   /* float const */
    KOR[1-1] = ALPHA*KOR[1-1] + r0*e0;
    VAR[2-1] = ALPHA*VAR[2-1]+(0.5F)*(r1*r1 + e1*e1);   /* float const */
    KOR[2-1] = ALPHA*KOR[2-1] + r1*e1;

    /* Summation and delay in the R-Branch => new R-values */

    r1 = A*(r0-dr1);
    r0 = A*e0;

    R[0]=r0;
    R[1]=r1;
}




/********************************************************************************
 *** FUNCTION: predict()                            *
 ***                                        *
 ***    carry out prediction for all allowed spectral lines         *
 ***                                        *
 ********************************************************************************/


int predict(faacDecHandle hDecoder, Info* info, int profile, int *lpflag, PRED_STATUS *psp, Float *coef)
{
    int j, k, b, to, flag0;
    int *top;

    if (hDecoder->mc_info.object_type != AACMAIN) {
        if (*lpflag == 0) {
            /* prediction calculations not required */
            return 0;
        }
        else {
            return -1;
        }
    }

    if (info->islong) {
        TMP_PRED_STATUS tmp_ps[MAX_PRED_BINS];
        inv_quant_pred_state(tmp_ps, psp);
        b = 0;
        k = 0;
        top = info->sbk_sfb_top[b];
        flag0 = *lpflag++;
        for (j = 0; j < pred_max_bands(hDecoder); j++) {
            to = *top++;
            if (flag0 && *lpflag++) {
                for ( ; k < to; k++) {
                    monopred(hDecoder, &coef[k], &psp[k], &tmp_ps[k], 1);
                }
            } else {
                for ( ; k < to; k++) {
                    monopred(hDecoder, &coef[k], &psp[k], &tmp_ps[k], 0);
                }
            }
        }
        quant_pred_state(psp, tmp_ps);
    }
    return 0;
}





/********************************************************************************
 *** FUNCTION: predict_reset()                          *
 ***                                        *
 ***    carry out cyclic predictor reset mechanism (long blocks)        *
 ***    resp. full reset (short blocks)                     *
 ***                                        *
 ********************************************************************************/
int predict_reset(faacDecHandle hDecoder, Info* info, int *prstflag, PRED_STATUS **psp,
                   int firstCh, int lastCh, int *last_rstgrp_num)
{
    int j, prstflag0, prstgrp, ch;

    prstgrp = 0;
    if (info->islong) {
        prstflag0 = *prstflag++;
        if (prstflag0) {

            /* for loop modified because of bit-reversed group number */
            for (j=0; j<LEN_PRED_RSTGRP-1; j++) {
                prstgrp |= prstflag[j];
                prstgrp <<= 1;
            }
            prstgrp |= prstflag[LEN_PRED_RSTGRP-1];

            if ( (prstgrp<1) || (prstgrp>30) ) {
                return -1;
            }

            for (ch=firstCh; ch<=lastCh; ch++) {
                /* check if two consecutive reset group numbers are incremented by one
                   (this is a poor check, but we don't have much alternatives) */
                if ((hDecoder->warn_flag) && (last_rstgrp_num[ch] < 30) && (last_rstgrp_num[ch] != 0)) {
                    if ((last_rstgrp_num[ch] + 1) != prstgrp) {
                        hDecoder->warn_flag = 0;
                    }
                }
                last_rstgrp_num[ch] = prstgrp;
                for (j=prstgrp-1; j<LN2; j+=30) {
                    reset_pred_state(&psp[ch][j]);
                }
            }
        } /* end predictor reset */
    } /* end islong */
    else { /* short blocks */
        /* complete prediction reset in all bins */
        for (ch=firstCh; ch<=lastCh; ch++) {
            last_rstgrp_num[ch] = 0;
            for (j=0; j<LN2; j++)
                reset_pred_state(&psp[ch][j]);
        }
    }
    return 0;
}
