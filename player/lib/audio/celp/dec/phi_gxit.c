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
/*      SOURCE_FILE:    PHI_GXIT.C                                      */
/*      PACKAGE:        WDBxx                                           */
/*      COMPONENT:      Excitation Generation Module                    */
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

#include "buffersHandle.h"       /* handler, defines, enums */

#include "phi_cons.h"
#include "bitstream.h"
#include "lpc_common.h"
#include "phi_gxit.h"
#include "phi_xits.h"

/*======================================================================*/
/*       L O C A L    D A T A   D E C L A R A T I O N                   */
/*======================================================================*/




/*======================================================================*/
/* Function Definition:  PHI_init_excitation_generation                 */
/*======================================================================*/
void 
PHI_init_excitation_generation
(
const long max_lag,     /* In:Maximum permitted lag in the adaptive cbk */
const long sbfrm_size,  /* In:Size of subframe in samples               */
const long RPE_configuration, /* In:Confguration                        */
PHI_PRIV_TYPE * PHI_Priv /* In/Out: private data (instance context)    */
)
{
    int i;
    
    /* -----------------------------------------------------------------*/
    /* Allocate memory for the adaptive Codebook                        */
    /* -----------------------------------------------------------------*/
    if(( PHI_Priv->PHI_adaptive_cbk = (float *)malloc((unsigned int)max_lag * sizeof(float))) == NULL )
    {
		printf("MALLOC FAILURE in init_abs_excitation_analysis \n");
		exit(1);
    }
    
    /* -----------------------------------------------------------------*/
    /* Initialise the adaptive codebook                                 */
    /* -----------------------------------------------------------------*/
    for (i = 0; i < (int)max_lag; i ++)
    {
         PHI_Priv->PHI_adaptive_cbk[i] = (float)0.0;
    }
    
    /* -----------------------------------------------------------------*/
    /* Set the excitation Parameters                                    */
    /* -----------------------------------------------------------------*/
        
    PHI_Priv->PHI_D = 1;
    
    if ((RPE_configuration == 0) || (RPE_configuration == 1))
    {
         PHI_Priv->PHI_D = 8;
    }

    if (RPE_configuration == 2)
    {
         PHI_Priv->PHI_D = 5;
    }

    if (RPE_configuration == 3)
    {
         PHI_Priv->PHI_D = 4;
    }
    
    PHI_Priv->PHI_Np = sbfrm_size/PHI_Priv->PHI_D;
}

/*======================================================================*/
/* Function Definition: celp_excitation_generation                      */
/*======================================================================*/
void
celp_excitation_generation
(
                                /* -------------------------------------*/
                                /* INPUT PARAMETERS                     */
                                /* -------------------------------------*/
unsigned long  shape_indices[], /* Lag indices for Adaptive & Fixed cbks*/
unsigned long  gain_indices[],  /* Gains for Adaptive & Fixed cbks      */
long  num_shape_cbks,           /* Number of shape codebooks            */
long  num_gain_cbks,            /* Number of gain codebooks             */
unsigned long  rms_index,       /* NOT USED HERE: RMS value subframe ?? */
float int_Qlpc_coefficients[],  /* Interpolated LPC coeffs of subframe  */
long  lpc_order,                /* Order of LPC                         */
long  sbfrm_size,               /* In: Number of samples in the subframe*/
long  n_subframes,              /* In: Number of subframes              */
unsigned long  signal_mode,     /* In: Configuration Input              */
long  frame_bit_allocation[],   /* In: Configuration Input              */
                                /* -------------------------------------*/
                                /* OUTPUT PARAMETERS                    */
                                /* -------------------------------------*/
float excitation[],             /* Out: Excitation Signal               */
long  *acb_delay,               /* NOT USED HERE: Pitch for post filter?*/
float *adaptive_gain,            /* NOT USED HERE: ????                  */
PHI_PRIV_TYPE * PHI_Priv        /* In/Out: private data (instance context)*/
)
{
    float *acbk_contrib;
    float *fcbk_contrib;
    long  *amp;
    float acbk_gain;
    float fcbk_gain;
    long  phase;
    
    /*==================================================================*/
    /* Memory allocation for temporary arrays                           */
    /*==================================================================*/
    if
    (  
    (( fcbk_contrib = (float *)malloc((unsigned int)sbfrm_size * sizeof(float)))== NULL ) || 
    (( acbk_contrib = (float *)malloc((unsigned int)sbfrm_size * sizeof(float)))== NULL ) ||
    (( amp          = (long *) malloc((unsigned int)PHI_Priv->PHI_Np         * sizeof(long )))== NULL )
   ) 
    {
       printf("ERROR: Malloc Failure in Block: Excitation Generation \n");
       exit(1);
    }

    /*==================================================================*/
    /* Make sure the number of shape and gain codebooks is correct      */
    /*==================================================================*/
    if (num_shape_cbks != 2)
    { 
        fprintf(stderr, "Wrong number of shape codebooks in Block: Excitation Generation\n");
        exit(1);
    }    
    if (num_gain_cbks != 2)
    { 
        fprintf(stderr, "Wrong number of gain codebooks in Block: Excitation Generation\n");
        exit(1);
    }    

    /*==================================================================*/
    /* We need to know which subframe we are in                         */
    /*==================================================================*/
    if (PHI_Priv->PHI_sfrm_ctr % n_subframes == 0)
    {
        PHI_Priv->PHI_sfrm_ctr = 0;
    }
   
    /*==================================================================*/
    /* Adptive-Codebook-Gain Decoding                                   */
    /*==================================================================*/
    PHI_DecodeAcbkGain((long)gain_indices[0], &acbk_gain);

    /*==================================================================*/
    /* Fixed-Codebook-Gain Decoding (This is dependent on the subframe) */
    /*==================================================================*/
    PHI_DecodeFcbkGain((long)gain_indices[1],  PHI_Priv->PHI_sfrm_ctr, PHI_Priv->PHI_prev_fcbk_gain, &fcbk_gain);
    PHI_Priv->PHI_prev_fcbk_gain = fcbk_gain;
   
    /*==================================================================*/
    /* Decode the phase and amplitudes of the RPE sequence              */
    /*==================================================================*/
    PHI_decode_cbf_amplitude_phase(PHI_Priv->PHI_Np, PHI_Priv->PHI_D, amp, &phase, (long)shape_indices[1]);

    /*==================================================================*/
    /* Compute the contribution of the Adaptive Codebook                */
    /*==================================================================*/
    PHI_calc_cba_excitation(sbfrm_size, Lmax, Lmin, PHI_Priv->PHI_adaptive_cbk, (long)shape_indices[0], acbk_contrib);
    
    /*==================================================================*/
    /* Compute the contribution of the Fixed Codebook                   */
    /*==================================================================*/
    PHI_calc_cbf_excitation(sbfrm_size, PHI_Priv->PHI_Np, PHI_Priv->PHI_D, amp, phase, fcbk_contrib);
   
    /*==================================================================*/
    /* Sum up the contributions of the codebooks to get the excitation  */
    /*==================================================================*/
    PHI_sum_excitations(sbfrm_size, acbk_gain, acbk_contrib, fcbk_gain, fcbk_contrib, excitation);

    /*==================================================================*/
    /* Update Adaptive Codebook                                         */
    /*==================================================================*/
    PHI_update_cba_memory(sbfrm_size, Lmax, PHI_Priv->PHI_adaptive_cbk, excitation);

    /*==================================================================*/
    /*   Update the subframe counter                                    */
    /*==================================================================*/
    PHI_Priv->PHI_sfrm_ctr++;

    /*==================================================================*/
    /*FREE temp states                                                 */
    /*==================================================================*/
   FREE(amp);
   FREE(acbk_contrib);
   FREE(fcbk_contrib);
    
    /*==================================================================*/
    /* Return to main                                                   */
    /*==================================================================*/
    return;
}

/*======================================================================*/
/* Function Definition: PHI_close_excitation_generation                 */
/*======================================================================*/
void PHI_close_excitation_generation(
     PHI_PRIV_TYPE * PHI_Priv        /* In: private data (instance context)*/
)
{
   FREE (PHI_Priv->PHI_adaptive_cbk);
}

/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 17-04-96 R. Taori  Initial Version                                   */
/* 30-08-96 R. Taori  Modified interface to meet Tampere requirements   */
