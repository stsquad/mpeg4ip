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
/*      INCLUDE_FILE:   PHI_XITS.H                                      */
/*      PACKAGE:        WDBxx                                           */
/*      COMPONENT:      Subroutines for Excitation Modules              */
/*                                                                      */
/*======================================================================*/

#ifndef _phi_xits_h_
#define _phi_xits_h_

#ifdef __cplusplus
extern "C" {
#endif

/*======================================================================*/
/* Function Prototype: perceptual_weighting                             */
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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
long  *amp             /* Out:   Pulse amplitudes  +/- 1               */ 
);


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
);

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
long  *pos               /* In:    Index to non-zero codevector          */
);

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
);

/*
----------------------------------------------------------------------------
  fixed codebook search
----------------------------------------------------------------------------
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
);

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
);

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
const  long    index           /* In:   Coded Fixed codebook index             */
);

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
);

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
);

/*
----------------------------------------------------------------------------
  computes excitation of the adaptive codebook
----------------------------------------------------------------------------
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
);

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
);

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
);

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
);

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
);


#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _phi_xits_h_ */

/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 17-04-96 R. Taori  Initial Version                                   */
/* 13-08-96 R. Taori  Added 2 subroutines CompAmpArray CompPosArray     */
/*                    Modified generate_cbf to reflect the above        */






