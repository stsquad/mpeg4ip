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
/*      SOURCE_FILE:    PHI_POST.C                                      */
/*      PACKAGE:        WDBxx                                           */
/*      COMPONENT:      Post-processing Module                          */
/*                                                                      */
/*======================================================================*/

/*======================================================================*/
/*      I N C L U D E S                                                 */
/*======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "phi_post.h"
#include "bitstream.h"
/*======================================================================*/
/*     L O C A L     D A T A     D E F I N I T I O N S                  */
/*======================================================================*/




/*======================================================================*/
/* Function Definition: PHI_InitPostProcessor                           */
/*======================================================================*/
void                            /* Return Value: Void                   */
PHI_InitPostProcessor
(
const long lpc_order,            /* In: Order of LPC                     */
PHI_PRIV_TYPE *PHI_Priv         /* In/Out: private data (instance context)*/
)
{
    int k;
    
    /* -----------------------------------------------------------------*/
    /* Create Arrays for delay line(s)                                  */
    /* -----------------------------------------------------------------*/
    if
    (
    (( PHI_Priv->PHI_g1 = (float *)malloc((unsigned int)lpc_order * sizeof(float))) == NULL ) ||
    (( PHI_Priv->PHI_g2 = (float *)malloc((unsigned int)lpc_order * sizeof(float))) == NULL ) ||
    (( PHI_Priv->PHI_P1_states = (float *)malloc((unsigned int)lpc_order * sizeof(float))) == NULL )||
    (( PHI_Priv->PHI_P2_states = (float *)malloc((unsigned int)lpc_order * sizeof(float))) == NULL )
    )
    {
        printf("MALLOC FAILURE in Routine InitPostProcessor \n");
        exit(1);
    }
    /* -----------------------------------------------------------------*/
    /* Initialise Arrays for delay line(s)                              */
    /* -----------------------------------------------------------------*/
    for (k = 0; k < (int)lpc_order; k++)
    {
         PHI_Priv->PHI_P1_states[k] = PHI_Priv->PHI_P2_states[k] = (float)0.0;
    }
    PHI_Priv->PHI_Gpf = PHI_Priv->PHI_P3_state = (float)0.0;
    
    /* -----------------------------------------------------------------*/
    /* Initialise Gamma Arrys of the postfilter                         */
    /* -----------------------------------------------------------------*/
    for(PHI_Priv->PHI_g1[0] = (float)0.65, PHI_Priv->PHI_g2[0] = (float)0.75, k = 1; k < (int)lpc_order; k++)
    {
        PHI_Priv->PHI_g1[k]  = (float)0.65 * PHI_Priv->PHI_g1[k-1];
        PHI_Priv->PHI_g2[k]  = (float)0.75 * PHI_Priv->PHI_g2[k-1];
    }
   
}

/*======================================================================*/
/* Function Definition: PHI_ClosePostProcessor                          */
/*======================================================================*/
void                            /* Return Value: Void                   */
PHI_ClosePostProcessor
(
PHI_PRIV_TYPE *PHI_Priv         /* In/Out: private data (instance context)*/
)
{
   FREE (PHI_Priv->PHI_P1_states);
   FREE (PHI_Priv->PHI_P2_states);
   FREE (PHI_Priv->PHI_g1);
   FREE (PHI_Priv->PHI_g2);
}


/*======================================================================*/
/* Function Definition: celp_postprocessing                             */
/*======================================================================*/
void                            /* Return Value: Void                   */
celp_postprocessing
(
const float synth_signal[],          /*In:  Input sig [0..sbfrm_size -1]*/
      float PP_synth_signal[],       /*Out: Postprocessed ooutput       */
const float int_Qlpc_coefficients[], /*In:  Decoded LPC coefficients    */  
const long  lpc_order,               /*In:  Order of LPC                */
const long  sbfrm_size,              /*In:  #Samps to be processed      */
const long  acb_delay,               /*In:  Pitch-like information      */
const float adaptive_gain,            /*In:  DON'T KNOW WHY IT IS NEEDED */
PHI_PRIV_TYPE *PHI_Priv         /* In/Out: private data (instance context)*/
)
{
    int   nnn     = (int)sbfrm_size;
    const float *in_ptr = synth_signal;
    float *out    = PP_synth_signal; 
    int k;
    
    /* -------------------------------------------------------------*/
    /* Begin Post-processing                                        */
    /* -------------------------------------------------------------*/
    do
    {
        float tmp          = *in_ptr;
        const float *ap_q  = int_Qlpc_coefficients;
        float *PHI_g1_ptr  = PHI_Priv->PHI_g1;
        float *PHI_g2_ptr  = PHI_Priv->PHI_g2;

        for(k = 0; k < (int)lpc_order; k++)
        {
            tmp = tmp - (PHI_Priv->PHI_P1_states[k] * *PHI_g1_ptr++ - PHI_Priv->PHI_P2_states[k] * *PHI_g2_ptr++) * *ap_q++;
        }
        *out++ = PHI_Priv->PHI_Gpf * (tmp - (float)0.3 * PHI_Priv->PHI_P3_state);
        PHI_Priv->PHI_P3_state = tmp;
        for(k = (int)lpc_order - 1; k > 0; k--)
        {
            PHI_Priv->PHI_P1_states[k] = PHI_Priv->PHI_P1_states[k-1];
            PHI_Priv->PHI_P2_states[k] = PHI_Priv->PHI_P2_states[k-1];
        }
        PHI_Priv->PHI_P1_states[0] = *in_ptr++;
        PHI_Priv->PHI_P2_states[0] = tmp;
    }
    while(--nnn);

    /* -------------------------------------------------------------*/
    /* Update PHI_Gpf  Borrowed from Friedhelm's report -II pp.29   */
    /* -------------------------------------------------------------*/
    {
        float pwr1 = (float)0.0;
        float pwr2 = (float)0.0;
        float r    = (float)1.0;
        int   nnn;

        for(nnn = 0; nnn < (int)sbfrm_size; nnn++)
        {
            float ss = synth_signal[nnn];
            float sy = PP_synth_signal[nnn];
            
            if (fabs((double)(ss + sy)) > 1e-17)
            {
                pwr1 += (ss * ss);
                pwr2 += (sy * sy);
            }
        }
        if (pwr2 > (float)0.0)
        {
            r = (float)sqrt((double)pwr1/(double)pwr2);
        }
        PHI_Priv->PHI_Gpf = (((float)1.0 - (float)0.0625) * PHI_Priv->PHI_Gpf) + ((float)0.0625 * r);
    }
}
/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 08-08-96 R. Taori  Initial Version                                   */
