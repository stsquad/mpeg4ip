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
/*      INCLUDE_FILE:   PHI_AXIT.H                                      */
/*      PACKAGE:        WDBxx                                           */
/*      COMPONENT:      Excitation Analysis Modules                     */
/*                                                                      */
/*======================================================================*/

#ifndef _phi_axit_h_
#define _phi_axit_h_


#ifdef __cplusplus
extern "C" {
#endif

/*======================================================================*/
/* Function Prototype : PHI_init_excitation_analysis                    */
/*======================================================================*/
void 
PHI_init_excitation_analysis
(
const long max_lag,   /* In:Maximum permitted lag in the adaptive cbk */ 
const long lpc_order, /* In:The LPC order                             */
const long sbfrm_size,/* In:Size of subframe in samples               */
const long RPE_configuration /* In:Confguration                        */
);

/*======================================================================*/
/* Function Prototype : celp_excitation_analysis                        */
/*======================================================================*/
void celp_excitation_analysis
(
                                  /* -----------------------------------*/
                                  /* INPUT PARAMETERS			        */
                                  /* -----------------------------------*/
float PP_InputSignal[], 		  /* Preprocessed Input signal          */
float lpc_residual[],			  /* Inverse Filtered Signal	        */
float int_Qlpc_coefficients[],    /* Interpolated LPC Coeffs	        */
long  lpc_order,				  /* Order of LPC				        */		
float Wnum_coeff[], 			  /* Weighting Filter: Numerator        */
float Wden_coeff[],               /* Weighting Filter: Denominator      */
float first_order_lpc_par,        /* apar corresponding to 1st-order fit*/
long  lag_candidates[],		      /* Array of Lag candidates            */
long  n_lag_candidates,           /* Number of lag candidates			*/
long  frame_size,		          /* Number of samples in the frame     */
long  sbfrm_size,		          /* Number of samples in the subframe  */
long  n_subframes,                /* Number of subframes				*/
long  signal_mode,                /* Configuration Input				*/
long  frame_bit_allocation[],     /* Configuration Input 		        */
                                  /* -----------------------------------*/
                                  /* OUTPUT PARAMETERS                  */
                                  /* -----------------------------------*/
long  shape_indices[],            /* Adaptive and Fixed codebook lags   */
long  gain_indices[],             /* Adaptive and Fixed codebook gains  */
long  num_shape_cbks,             /* Number of shape codebooks          */
long  num_gain_cbks,              /* Number of gain codebooks           */
long  *rms_index,                 /* RMS Value ????                     */
float decoded_excitation[]        /* Synthesised Signal                 */
);

/*======================================================================*/
/* Function Prototype : PHI_close_excitation_analysis                   */
/*======================================================================*/
void PHI_close_excitation_analysis(void);
   
#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _phi_axit_h_ */
   
/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 17-04-96 R. Taori  Initial Version                                   */
/* 30-06-96 R. Taori  Modified interface  to meet the MPEG-4 requirement*/
/* 20-08-96 R. Taori  Modified interface to accomodate Tampere results  */
   
