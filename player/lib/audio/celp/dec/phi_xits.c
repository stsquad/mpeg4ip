/*====================================================================*/
/*         MPEG-4 Audio (ISO/IEC 14496-3) Copyright Header            */
/*====================================================================*/
/*
This software module was originally developed by Rakesh Taori and Andy
Gerrits (Philips Research Laboratories, Eindhoven, The Netherlands) in
the course of development of the MPEG-4 Audio (ISO/IEC 14496-3). This
software module is an implementation of a part of one or more MPEG-4
Audio (ISO/IEC 14496-3) tools as specified by the MPEG-4 Audio
(ISO/IEC 14496-3). ISO/IEC gives users of the MPEG-4 Audio (ISO/IEC
14496-3) free license to this software module or modifications thereof
for use in hardware or software products claiming conformance to the
MPEG-4 Audio (ISO/IEC 14496-3). Those intending to use this software
module in hardware or software products are advised that its use may
infringe existing patents. The original developer of this software
module and his/her company, the subsequent editors and their
companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Copyright is not
released for non MPEG-4 Audio (ISO/IEC 14496-3) conforming products.
CN1 retains full right to use the code for his/her own purpose, assign
or donate the code to a third party and to inhibit third parties from
using the code for non MPEG-4 Audio (ISO/IEC 14496-3) conforming
products.  This copyright notice must be included in all copies or
derivative works. Copyright 1996.
*/
/*====================================================================*/
/*======================================================================*/
/*                                                                      */
/*      SOURCE_FILE:    PHI_XITS.C                                      */
/*      PACKAGE:        WDBxx                                           */
/*      COMPONENT:      Subroutines for Excitation Modules              */
/*                                                                      */
/*======================================================================*/

/*======================================================================*/
/*      I N C L U D E S                                                 */
/*======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <assert.h>
#include "phi_excq.h"
#include "phi_xits.h"
#include "bitstream.h"
/*======================================================================*/
/* Function Definition:  perceptual_weighting                           */
/*======================================================================*/
void 
PHI_perceptual_weighting
(
long  nos,              /* In:     Number of samples to be processed    */
float *vi,              /* In:     Array of input samps to be processe  */
float *vo,              /* Out:    Perceptually Weighted Output Speech  */ 
long  order,            /* In:     LPC-Order of the weighting filter    */ 
float *a_gamma,         /* In:     Array of the Weighting filter coeffs */
float *vp1              /* In/Out: The delay line states                */
)
{
    int i, j;           /* Local: Loop indices                          */

    for (i = 0; i < (int)nos; i ++)
    {
        register float temp_vo = vi[i];
        
        for (j = 0; j < (int)order; j ++)
        {
            temp_vo += (vp1[j] * a_gamma[j]);
        }
        vo[i] = temp_vo;

        for (j = (int)order - 1; j > 0; j --)
        {
            vp1[j] = vp1[j - 1];
        }
        vp1[0] = vo[i];
    }
    return;
}


/*
------------------------------------------------------------------------
  zero input response 
------------------------------------------------------------------------
*/

void 
PHI_calc_zero_input_response
(
long  nos,               /* In:     Number of samples to be processed   */
float *vo,               /* Out:    The zero-input response             */ 
long  order,             /* In:     Order of the Weighting filter       */
float *a_gamma,          /* In:     Coefficients of the weighting filter*/
float *vp                /* In:     Filter states                       */
)
{
    float *vpp;         /* Local: To copy the filter states             */
    int i, j;           /* Local: Loop indices                          */

    /*==================================================================*/
    /* Allocate temp states                                             */
    /*==================================================================*/
    if ((vpp = (float *)malloc((unsigned int)order * sizeof(float))) == NULL )
    {
       fprintf(stderr, "\n Malloc Failure in Block: Excitation Analysis \n");
       exit(1);
    }
  
    /*==================================================================*/
    /* Initiallise temp states                                          */
    /*==================================================================*/
    for (i = 0; i < (int)order; i ++)
    {
        vpp[i] = vp[i];
    }
        
    /*==================================================================*/
    /* Compute zero-input response                                      */
    /*==================================================================*/
    for (i = 0; i < (int)nos; i ++)
    {
        register float temp_vo = (float)0.0;
        
        for (j = 0; j < (int)order; j ++)
        {
            temp_vo += a_gamma[j] * vpp[j];
        }
        vo[i] = temp_vo;

        for (j =(int)order  - 1; j > 0; j --)
        {
            vpp[j] = vpp[j - 1];
        }
        vpp[0] = vo[i];
    }
    
    /*==================================================================*/
    /* Free temp states                                                 */
    /*==================================================================*/
   FREE(vpp);
    
    return;
}


/*
----------------------------------------------------------------------------
  weighted target signal
----------------------------------------------------------------------------
*/

void 
PHI_calc_weighted_target
(
long nos,               /* In:     Number of samples to be processed    */
float *v1,              /* In:     Array of Perceptually weighted speech*/ 
float *v2,              /* In:     The zero-input response              */ 
float *vd               /* Out:    Real Target signal for current frame */
)
{
    int i;              /* Local:  Loop index                           */

    for (i = 0; i < (int)nos; i ++)
        vd[i] = v1[i] - v2[i];

    return;
}

/*
----------------------------------------------------------------------------
  impulse response
----------------------------------------------------------------------------
*/

void 
PHI_calc_impulse_response
(
long  nos,              /* In:     Number of samples to be processed    */
float *h,               /* Out:    Impulse response of the filter       */
long  order,            /* In:     Order of the filter                  */ 
float *a_gamma          /* In:     Array of the filter coefficients     */
)
{
    float *vpp;         /* Local: Local filter states                   */
    int i, j;           /* Local: Loop indices                          */

    /*==================================================================*/
    /* Allocate temp states                                             */
    /*==================================================================*/
    if ((vpp = (float *)malloc((unsigned int)order * sizeof(float)))== NULL )
    {
       fprintf(stderr, "\n Malloc Failure in Block:Excitation Anlaysis \n");
       exit(1);
    }
  
    /*==================================================================*/
    /* Initiallise temp states                                          */
    /*==================================================================*/
    for (i = 0; i < (int)order; i ++)
    {
        vpp[i] = (float)0.0;
    }

    h[0] = (float)1.0;
    for (i = 1; i < (int)nos; i ++)
        h[i] = (float)0.0;

    /*==================================================================*/
    /* Compute Impulse response                                         */
    /*==================================================================*/
    for (i = 0; i < (int)nos; i ++)
    {
        register float temp_h = h[i];
        
        for (j = 0; j < (int)order; j ++)
        {
            temp_h += a_gamma[j] * vpp[j];
        }
        h[i] = temp_h;

        for (j = (int)order - 1; j > 0; j --)
        {
            vpp[j] = vpp[j - 1];
        }
        vpp[0] = h[i];
    }
    
    /*==================================================================*/
    /*FREE temp states                                                 */
    /*==================================================================*/
   FREE(vpp);
    
    return;
}


/*
------------------------------------------------------------------------
  backward filtering 
------------------------------------------------------------------------
*/

void 
PHI_backward_filtering
(
long  nos,              /* In:     Number of samples to be processed    */
float *vi,              /* In:     Array of samples to be filtered      */ 
float *vo,              /* Out:    Array of filtered samples            */ 
float *h                /* In:     The filter coefficients              */
)
{
    int i, j;           /* Local:  Loop indices                         */

    for (i = 0; i < (int)nos; i ++)
    {
        register float temp_vo = (float)0.0;
        
        for (j = 0; j <= i; j ++)
        {
            temp_vo += h[i - j] * vi[(int)nos - 1 - j];
        }
        vo[(int)nos - 1 - i] = temp_vo;
    }

    return;
}

/*
----------------------------------------------------------------------------
  adaptive codebook search
----------------------------------------------------------------------------
*/

void 
PHI_cba_search
(
long   nos,             /* In:     Number of samples to be processed    */
long   max_lag,         /* In:     Maximum Permitted Adapt cbk Lag      */
long   min_lag,         /* In:     Minimum Permitted Adapt cbk Lag      */
float  *cb,             /* In:     Segment of Adaptive Codebook         */
long   *pi,             /* In:     Array of preselected-lag indices     */
long   n_lags,          /* In:     Number of lag candidates to be tried */
float  *h,              /* In:     Impulse response of synthesis filter */ 
float  *t,              /* In:     The real target signal               */ 
float  *g,              /* Out:    Adapt-cbk gain(Quantised but uncoded)*/
long   *vid,            /* Out:    The final adaptive cbk lag index     */ 
long   *gid             /* Out:    The gain index                       */
)
{
    float *y, *yp;         
    float num, den;             
    float rs, r;
    int   ks;
    int   i, j, k;
    int   m = 0;
    int   prevm;

    /*==================================================================*/
    /* Allocate temp states                                             */
    /*==================================================================*/
    if ( ((y = (float *)malloc((unsigned int)nos * sizeof(float))) == NULL )||
         ((yp = (float *)malloc((unsigned int)nos * sizeof(float))) == NULL ))
    {
       fprintf(stderr, "\n Malloc Failure in Block: Excitation Anlaysis \n");
       exit(1);
    }

    rs = -FLT_MAX;

    /*==================================================================*/
    /* Codebook Vector Search                                           */
    /*==================================================================*/
    for (k = 0; k < (int)n_lags; k ++)
    {
        int end_correction;
        
        /* =============================================================*/
        /* Compute Lag index                                            */
        /* =============================================================*/
        prevm = m;
        m = (int)(max_lag - min_lag - pi[k]);
        
        /* =============================================================*/
        /* Check if we can use end_correction                           */
        /* =============================================================*/
        if ( (k > 0) && (( prevm - m ) == 1))
        {
            end_correction = 1;
        }
        else
        {
            end_correction = 0;
        }
        /* =============================================================*/
        /* If end_correction can be used, use complexity reduced method */
        /* =============================================================*/
        if (end_correction)
        {
            y[0] = h[0] * cb[m];
            for (i = 1; i < (int)nos; i ++)
            {
                y[i] = yp[i - 1] + h[i] * cb[m];
            }
        }
        else
        {
            for (i = 0; i < (int)nos; i ++)
            {
                register float temp_y = (float)0.0;
                for (j = 0; j <= i; j ++)
                {
                    temp_y += h[i - j] * cb[m + j];
                }    
                y[i] = temp_y;
            }
        }
        
        /* =============================================================*/
        /* Copy the current to y to previous y (for the next iteration) */
        /* =============================================================*/
        for (i = 0; i < (int)nos; i ++)
        {
            yp[i] = y[i];
        }

        /* =============================================================*/
        /* Compute normalised cross correlation coefficient             */
        /* =============================================================*/
        num = (float)0.0;
        den = FLT_MIN;
        for (i = 0; i < (int)nos; i ++)
        {
            num += t[i] * y[i];
            den += y[i] * y[i];
        }
        r = num * num / den;

        /* =============================================================*/
        /* Check if the curent lag is the best candidate                */
        /* =============================================================*/
        if (r > rs)
        {
            ks = k;
            rs = r;
            *g = num / den;
        }
    }
    *vid = pi[ks];

    /*==================================================================*/
    /* Gain Quantisation                                                */
    /*==================================================================*/

    i = *g < (float)0.0 ? -1 : 1;
    *g = (float)fabs((double)*g);

    for (j = 0; *g > (float)tbl_cba_dir[j].dec && j < QLa - 1; j ++);
    *g = (float) i * (float)tbl_cba_dir[j].rep;
    *gid = i == 1 ? (long)j : (((long)(-j - 1)) & 63);

    /*==================================================================*/
    /*FREE temp states                                                 */
    /*==================================================================*/
   FREE(y);
   FREE(yp);

    return;
}


/*
----------------------------------------------------------------------------
  computes residual signal after adaptive codebook
----------------------------------------------------------------------------
*/
    
void 
PHI_calc_cba_residual
(
long  nos,             /* In:     Number of samples to be processed    */
float *vi,             /* In:     Succesful excitation candidate       */ 
float gain,            /* In:     Gain of the adaptive codebook        */ 
float *h,              /* In:     Impulse response of synthesis filt   */
float *t,              /* In:     The real target signal               */
float *e               /* Out:    Adapt-cbk residual: Target for f-cbk */
)
{
    float v;                                
    int i, j;
    
    for (i = 0; i < (int)nos; i ++)
    {
        v = (float)0.0;
        for (j = 0; j <= i; j ++)
        {
            v += h[i - j] * vi[j];
        }
        e[i] = t[i] - gain * v;
    }

    return;
}

/*
----------------------------------------------------------------------------
  determines phase of the local rpe codebook vectors
----------------------------------------------------------------------------
*/

void 
PHI_calc_cbf_phase
(
long  pulse_spacing,    /* In:    Regular Spacing Between Pulses        */
long  nos,              /* In:    Number of samples to be processed     */
float *tf,              /* In:    Backward filtered target signal       */ 
long  *p                /* Out:   Phase of the local RPE codebook vector*/
)
{
    float vs, v;
    int i, j;

    vs = -FLT_MAX;
    *p = 0;
    
    for (i = 0; i < (int)pulse_spacing; i ++)
    {
        for (j = i, v = (float)0.0; j < (int)nos; j += (int)pulse_spacing)
        {
            v += (float)fabs((double)tf[j]);
        }

        if (v > vs)
        {
            vs = v;
            *p = (long)i;
        }
    }

    return;
}

/*
----------------------------------------------------------------------------
  computes rpe pulse amplitude (Version 2:only computes amplitude)
----------------------------------------------------------------------------
*/

void 
PHI_CompAmpArray
(
long  num_of_pulses,    /* In:    Number of pulses in the sequence      */
long  pulse_spacing,    /* In:    Regular Spacing Between Pulses        */
float *tf,              /* In:    Backward filtered target signal       */ 
long  p,                /* In:    Phase of the RPE codebook             */
long  *amp              /* Out:   Pulse amplitudes  +/- 1               */ 
)
{

    int i, k;
    float v;
    
    for (k = (int)p, i = 0; i < (int)num_of_pulses; k += (int)pulse_spacing, i ++)
    {
        v = tf[k];
        
        if (v == (float)0.0)
        {
            amp[i] = 0; /* THIS SHOULD BE ZERO!!! */
        }
        else
        {
            amp[i] = v > (float)0.0 ? (long)1 : (long)-1;
        }
    }
    return;
}

/*
----------------------------------------------------------------------------
  computes pos array {extension to Vienna code which only fixed 1 amp
----------------------------------------------------------------------------
*/
void 
PHI_CompPosArray
(
long  num_of_pulses,    /* In:    Number of pulses in the sequence      */
long  pulse_spacing,    /* In:    Regular Spacing Between Pulses        */
long  num_fxd_amps,     /* IN:    Number of fixed amplitudes            */
float *tf,              /* In:    Backward filtered target signal       */ 
long  p,                /* In:    Phase of the RPE codebook             */
long  *pos              /* Out:   Pulse amplitudes  +/- 1               */ 
)
{
    float *d_tmp;
    int i, nr_nonzero_samples;
    
    /*==================================================================*/
    /* Allocate d_tmp                                             */
    /*==================================================================*/
    if ((d_tmp = (float *)malloc((unsigned int)num_of_pulses * sizeof(float)))== NULL )
    {
       fprintf(stderr, "\n Malloc Failure in CompPosArray:Excitation Anlaysis \n");
       exit(1);
    }
    for (i = nr_nonzero_samples = 0; i < (int)num_of_pulses; i ++)
    {
        pos[i] = 0;
        d_tmp[i] = (float)fabs((double)tf[(int)p + i * (int)pulse_spacing]);
	if (d_tmp[i] > 0)
	    nr_nonzero_samples++;
    }
    if (nr_nonzero_samples >= (int)num_fxd_amps)
    {
        for (i = 0; i < (int)num_fxd_amps; i ++)
        {
           float tmp   = (float)0.0;
           int   p_tmp = 0;
           int   l;

           for(l = 0; l < (int)num_of_pulses; l++)
           {
                   if (d_tmp[l] > tmp)
                   {
                   tmp = d_tmp[l];
                   p_tmp = l;
                   }
           }
           pos[p_tmp] = 1;
           d_tmp[p_tmp] = (float)0;
        }
    }
    else
    {
        int   l;

	for(i = 0; i < (int)num_of_pulses; i++)
	{
	    if (d_tmp[i] > 0)
	    {
                pos[i] = 1;
	    }
	}
	for(i = 0, l = nr_nonzero_samples; l < num_fxd_amps; i++)
	{
	    if (d_tmp[i] == 0)
	    {
                pos[i] = 1;
		l++;
	    }
	}
    }
   FREE(d_tmp);
    return;
}

/*
----------------------------------------------------------------------------
  generates local fixed codebook
----------------------------------------------------------------------------
*/

void 
PHI_generate_cbf
(
long  num_of_pulses,    /* In:    Number of pulses in the sequence      */
long  pulse_spacing,    /* In:    Regular Spacing Between Pulses        */
long  num_fcbk_vecs,    /* In:    #Vectors in the fixed code book       */
long  nos,              /* In:    Number of samples to be processed     */
long  **cb,             /* Out:   Generated Local Fixed Codebook        */ 
long  p,                /* In:    Phase of the codebook vector          */
long  *amp,             /* In:    Pulse Amplitudes                      */ 
long  *pos              /* In:    Array saying which are fixed amps     */
)
{
    int i, j, k, m, n, index;
    for (i = 0; i < (int)num_fcbk_vecs; i ++)
        for (j = 0; j < (int)nos; j ++)
            cb[i][j] = 0;

    for (index = (int)p, i = 0; i < (int)num_of_pulses; index += (int)pulse_spacing, i ++)
    {
        cb[0][index] = amp[i];
    }
    
    for (index = (int)p,i = 0, m = 1; i < (int)num_of_pulses; index += (int)pulse_spacing, i ++)
    {
        if (!pos[i])
        {
            for (j = 0, k = m; j < k; j ++)
            {
                for (n = (int)p; n < (int)nos; n += (int)pulse_spacing)
                {
                    cb[m][n] = cb[j][n];
                }
                cb[m ++][index] = 0;
            }
        }
    }
    /* -----------------------------------------------------------------*/
    /* To test the codebook content                                     */
    /* -----------------------------------------------------------------*/
    /*
    for (i = 0; i < (int)num_fcbk_vecs; i ++)
    {
        for (j = (int)p; j < (int)nos; j += (int)pulse_spacing)
            printf("%3d  ", cb[i][j]);
            
        printf("\n");
    }
    printf("\n");
    */
    /* -----------------------------------------------------------------*/
    return;
}

/*
----------------------------------------------------------------------------
  preselection of fixed codebook indices (Reduction from 16 to 5 WDBxx)
----------------------------------------------------------------------------
*/

void 
PHI_cbf_preselection
(
long  pulse_spacing,    /* In:   Regular Spacing Between Pulses         */
long  num_fcbk_cands,   /* In:   #Preselected candidates for fixed cbk  */
long  num_fcbk_vecs,    /* In:   #Vectors in the fixed code book        */
long  nos,              /* In:    Number of samples to be processed     */
long  **cb,             /* In:    Local fixed codebook,(Nf-1) by (nos-1)*/  
long  p,                /* In:    Phase  of the RPE codebook            */
float *tf,              /* In:    Backward-filtered target signal       */ 
float a,                /* In:    LPC coeffs of the preselection filter */
long  *pi               /* Out:   Result: Preselected Codebook Indices  */
)
{
    float t1, t2, e;                           
    float *rfp;                           
    long is;
    int i, j;

    /*==================================================================*/
    /* Allocate memory for rfp                                          */
    /*==================================================================*/
    if ((rfp = (float *)malloc((unsigned int)num_fcbk_vecs * sizeof(float)))== NULL )
    {
       fprintf(stderr, "\n Malloc Failure in Block:Excitation Anlaysis \n");
       exit(1);
    }
  
    /*==================================================================*/
    /* computes rfp ratios                                              */
    /*==================================================================*/
    for (i = 0; i < (int)num_fcbk_vecs; i ++)
    {
        t1 = t2 = (float)0.0;
        e = FLT_MIN;

        for (j = 0; j < (int)nos; j ++)
        {
            t1  = t1 * a + (float) cb[i][j];
            e  += t1 * t1;
        }

        for (j = (int)p; j < (int)nos; j += (int)pulse_spacing)
        {
            t2 += (float) cb[i][j] * tf[j];
        }

        rfp[i] = (t2 * t2) / e;
    }

    /*==================================================================*/
    /* select Pf indices with largest rfp's                             */
    /*==================================================================*/
    for (j = 0; j < (int)num_fcbk_cands; j ++)
    {
        e = -FLT_MAX;

        for (i = 0; i < (int)num_fcbk_vecs; i ++)
            if (rfp[i] > e)
            {
                e = rfp[i];
                is = (long)i;
            }
        assert(is < num_fcbk_vecs);
        pi[j] = is;

        rfp[is] = -FLT_MAX;
    }
    
    /*==================================================================*/
    /*FREE rfp                                                         */
    /*==================================================================*/
   FREE(rfp);
    return;
}

/*
------------------------------------------------------------------------
  fixed codebook search
------------------------------------------------------------------------
*/

void 
PHI_cbf_search(
long  num_of_pulses,    /* In:  Number of pulses in the sequence        */
long  pulse_spacing,    /* In:  Regular Spacing Between Pulses          */
long  num_fcbk_cands,   /* In:  #Preselected candidates for fixed cbk   */
long  nos,              /* In:  Number of samples to be processed       */
long  **cb,             /* In:  Local fixed codebook                    */
long  p,                /* In:  Phase of the fixed codebook, 0 to D-1   */
long  *pi,              /* In:  Preselected codebook indices, p[0..Pf-1]*/
float *h,               /* In:  Synthesis filter impulse response,      */
float *e,               /* In:  Target residual signal, e[0..nos-1]     */
float *gain,            /* Out: Selected Fixed Codebook gain (Uncoded)  */ 
long  *gid,             /* Out: Selected Fixed Codebook gain index      */
long  *amp,             /* Out: S rpe pulse amplitudes, amp[0..Np-1]    */
long  n                 /* In:  Subframe index, 0 to n_subframes-1      */
)
{
    static float gp ;                            
    float *y;                            
    float num, den;
    float g, r, rs;
    int ks;
    int i, j, k, m;
        

    rs = -FLT_MAX;

    /*==================================================================*/
    /* Allocate temp states                                             */
    /*==================================================================*/
    if ((y = (float *)malloc((unsigned int)nos * sizeof(float))) == NULL )
    {
       fprintf(stderr, "\n Malloc Failure in Block: Excitation Anlaysis \n");
       exit(1);
    }
    for (k = 0; k < (int)num_fcbk_cands; k ++)
    {
        for (i = 0; i < (int)nos; i ++)
        {
            register float temp_y = (float)0.0;
            for (j = (int)p; j <= i; j += (int)pulse_spacing)
            {
                temp_y += h[i - j] * (float) cb[pi[k]][j];
            }
            y[i] = temp_y;
        }

        num = (float)0.0;
        den = FLT_MIN;
        for (i = 0; i < (int)nos; i ++)
        {
            num += e[i] * y[i];
            den += y[i] * y[i];
        }
        g = num / den;
        
        if (!n)
        {
            for (m = 0; g > (float)tbl_cbf_dir[m].dec && m < QLf - 1; m ++);
            g = (float)tbl_cbf_dir[m].rep;
        }
        else
        {
            g /= gp;
            for (m = 0; g > (float)tbl_cbf_dif[m].dec && m < QLfd - 1; m ++);
            g = gp * (float)tbl_cbf_dif[m].rep;
        }

        r = (float)2.0 * g * num - g * g * den;
        if (r > rs)
        {
            rs = r;
            ks = k;
            *gid = (long)m;
            *gain = g;
        }
    }
    
    for (k = (int)p, i = 0; i < (int)num_of_pulses; k += (int)pulse_spacing, i ++)
    {
        amp[i] = cb[pi[ks]][k];
    }
    gp = *gain;

    /*==================================================================*/
    /*FREE temp states                                                 */
    /*==================================================================*/
   FREE(y);

    return;
}

/*
----------------------------------------------------------------------------
  encodes rpe pulse amplitudes and phase into one index  
----------------------------------------------------------------------------
*/

void 
PHI_code_cbf_amplitude_phase
(
long num_of_pulses,      /* In:     Number of pulses in the sequence     */
long pulse_spacing,      /* In:     Regular Spacing Between Pulses       */
long *amp,               /* In:   Array of Pulse Amplitudes, amp[0..Np-1]*/
long phase,              /* In:   The Phase of the RPE sequence          */
long *index              /* Out:  Coded Index: Fixed Codebook index      */
)
{
    int i;
    long ac = 0;

    for (i = 0; i < (int)num_of_pulses; i ++)
    {
        ac = (ac * 3) + (amp[i] == -1 ? (long)0 : (amp[i] ==  1 ? (long)1 : (long)2));
    }
    ac *= pulse_spacing;
    
    *index = ac + phase;

    return;
}

/*
----------------------------------------------------------------------------
  Decodes the RPE amplitudes and phase from the cbk-index
----------------------------------------------------------------------------
*/
void
PHI_decode_cbf_amplitude_phase
(
const long    num_of_pulses,  /* In:     Number of pulses in the sequence     */
const long    pulse_spacing,  /* In:     Regular Spacing Between Pulses       */
long  * const amp,            /* Out:  The Array of pulse amplitudes          */
long  * const phase,          /* Out:  The phase of the RPE sequence          */ 
const long    index           /* In:   Coded Fixed codebook index             */
)
{
    int  i;
    long ac;

    /* =================================================================*/
    /* Extract the phase First                                          */
    /* =================================================================*/
    *phase = (index % pulse_spacing);
    ac = index - *phase;
    ac /= pulse_spacing;

    /* =================================================================*/
    /* The remainder is used to extract the amplitude values            */
    /* =================================================================*/
    for (i = (int)num_of_pulses - 1; i >= 0; i--)
    {
        amp[i]  =  (ac % 3);
        ac     /= 3;
        
        /*==============================================================*/
        /* Only 3 amplitude values are permitted                        */
        /*==============================================================*/
        if (amp[i] == 0)
            amp[i] = -1;
        else if (amp[i] == 2)
            amp[i] = 0;
        else if (amp[i] == 1)
            amp[i] = 1;
        else        
        {
            fprintf(stderr, "FATAL ERROR: Unpermitted Amplitude Value \n");
            exit(1);
        }
    }
    return;
}

/*
----------------------------------------------------------------------------
  Decodes the Adaptive-Codebook Gain
----------------------------------------------------------------------------
*/
void
PHI_DecodeAcbkGain
(
long  acbk_gain_index,
float *gain
)
{
    int i, j;
    if (acbk_gain_index > 31)
    {
        acbk_gain_index = -(64 - acbk_gain_index);
    }
    
    if (acbk_gain_index < 0)
    {
        i = -1; 
        j = (-1 * (int)acbk_gain_index) - 1;
    }
    else
    {
        i = 1;
        j = (int)acbk_gain_index;
    }
    *gain = (float)i * (float)tbl_cba_dir[j].rep;
}

/*
----------------------------------------------------------------------------
  Decodes the Fixed-Codebook Gain
----------------------------------------------------------------------------
*/
void
PHI_DecodeFcbkGain
(
long  fcbk_gain_index,
long  ctr,
float prev_gain, 
float *gain
)
{
    if (ctr == 0)
    {
        *gain = (float)tbl_cbf_dir[fcbk_gain_index].rep;
    }
    else
    {
        *gain = (float)tbl_cbf_dif[fcbk_gain_index].rep * prev_gain;
    }
}
/*
------------------------------------------------------------------------
  computes excitation of the adaptive codebook
------------------------------------------------------------------------
*/

void 
PHI_calc_cba_excitation
(
long   nos,             /* In:     Number of samples to be updated      */
long   max_lag,         /* In:     Maximum Permitted Adapt cbk Lag      */
long   min_lag,         /* In:     Minimum Permitted Adapt cbk Lag      */
float  *cb,             /* In:     The current Adaptive codebook content*/ 
long   idx,             /* In:     The chosen lag candidate             */
float  *v               /* Out:    The Adaptive Codebook contribution   */
)
{
    int i, j;

    i = (int)(max_lag - min_lag - idx);

    for (j = 0; j < (int)nos; j ++)
        v[j] = cb[i + j];

    return;
}


/*
----------------------------------------------------------------------------
  computes excitation of the fixed codebook
----------------------------------------------------------------------------
*/

void 
PHI_calc_cbf_excitation
(
long   nos,             /* In:     Number of samples to be updated      */
long   num_of_pulses,   /* In:     Number of pulses in the sequence     */
long   pulse_spacing,   /* In:     Regular Spacing Between Pulses       */
long   *amp,            /* In:     Aray of RPE pulse amplitudes         */
long   p,               /* In:     Phase of the RPE sequence            */
float  *v               /* Out:    The fixed codebook contribution      */
)
{
    int i,k;

    for (i = 0; i < (int)nos; i ++)
        v[i] = (float)0;

    for (k = 0, i = (int)p; k < (int)num_of_pulses; i += (int)pulse_spacing, k++)
        v[i] = (float)amp[k];

    return;
}

/*
----------------------------------------------------------------------------
  compute the sum of the excitations
----------------------------------------------------------------------------
*/

void 
PHI_sum_excitations 
( 
long  nos,             /* In:     Number of samples to be updated      */
float again,           /* In:     Adaptive Codebook gain               */ 
float *asig,           /* In:     Adaptive Codebook Contribution       */ 
float fgain,           /* In:     Fixed Codebook gain                  */  
float *fsig,           /* In:     Fixed Codebook Contribution          */ 
float *sum_sig         /* Out:    The Excitation sequence              */
)
{
  int k;

  for(k = 0; k < (int)nos; k++)
  {
     sum_sig[k] = again * asig[k] + fgain * fsig[k];
  }
  return;
}


/*
----------------------------------------------------------------------------
  update adaptive codebook with the total excitation computed
----------------------------------------------------------------------------
*/

void 
PHI_update_cba_memory
(
long   nos,             /* In:     Number of samples to be updated      */
long   max_lag,         /* In:     Maximum Adaptive Codebook Lag        */
float *cb,              /* In/Out: Adaptive Codebook                    */ 
float *vi               /* In:     Sum of adaptive and fixed excitaion  */
)
{
    int i;                        

    for (i = (int)nos; i < (int)max_lag; i ++)
        cb[i - (int)nos] = cb[i];

    for (i = 0; i < (int)nos; i ++)
        cb[(int)max_lag - 1 - i] = vi[(int)nos - 1 - i];

    return;
}

/*
---------------------------------------------------------------------------
  update synthesis filter states
----------------------------------------------------------------------------
*/

void 
PHI_update_filter_states
(
long   nos,             /* In:     Number of samples                    */
long   order,           /* In:     Order of the LPC                     */ 
float *vi,              /* In:     Total Codebook contribution          */
float *vp,              /* In/Out: Filter States, vp[0..order-1]        */ 
float *a                /* In:     Lpc Coefficients, a[0..order-1]      */
)
{
    float v;                       
    int i, j;

    for (i = 0; i < (int)nos; i ++)
    {
        v = vi[i];

        for (j = 0; j < (int)order; j ++)
            v += a[j] * vp[j];

        for (j = (int)order - 1; j > 0; j --)
            vp[j] = vp[j - 1];
        vp[0] = v;
    }

    return;
}
/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 17-04-96 R. Taori  Initial Version                                   */
/* 13-08-96 R. Taori  Added 2 subroutines CompAmpArray CompPosArray     */
/*                    Modified generate_cbf to reflect the above        */


