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
/*      INCLUDE_FILE:   PHI_LPC.H                                       */
/*      PACKAGE:        WDBxx                                           */
/*      COMPONENT:      Prototypes of Linear Prediction Subroutines     */
/*                                                                      */
/*======================================================================*/

#ifndef _phi_lpc_h_
#define _phi_lpc_h_

#include "phi_priv.h"	/* PRIV */


#ifdef __cplusplus
extern "C" {
#endif

/*======================================================================*/
/*   Function Prototype: celp_lpc_analysis                              */
/*======================================================================*/
void celp_lpc_analysis
(
float PP_InputSignal[],         /* In:  Input Signal                    */
float lpc_coefficients[],       /* Out: LPC Coefficients[0..lpc_order-1]*/
float *first_order_lpc_par,     /* Out: a_parameter for 1st-order fit   */
long  frame_size,               /* In:  Number of samples in frame      */
long  window_offsets[],         /* In:  offset for window w.r.t curr. fr*/
long  window_sizes[],           /* In:  LPC Analysis-Window Size        */
float *windows[],               /* In:  Array of LPC Analysis windows   */
float gamma_be[],               /* In:  Bandwidth expansion coefficients*/
long  lpc_order,                /* In:  Order of LPC                    */
long  n_lpc_analysis            /* In:  Number of LP analysis/frame     */
); 

/*======================================================================*/
/*   Function Prototype: VQ_celp_lpc_decode                             */
/*======================================================================*/
void VQ_celp_lpc_decode
(
unsigned long  lpc_indices[],  /* In: Received Packed LPC Codes         */
float int_Qlpc_coefficients[], /* Out: Qaunt/interpolated a-pars        */
long  lpc_order,               /* In:  Order of LPC                     */
long  num_lpc_indices,         /* In:  Number of packes LPC codes       */
long  n_subframes,             /* In:  Number of subframes              */
unsigned long interpolation_flag, /* In:  Was interpolation done?          */
long  Wideband_VQ,             /* In:  Wideband VQ switch               */
PHI_PRIV_TYPE *PHI_Priv		/* In/Out: PHI private data (instance context) */
);

/*======================================================================*/
/*   Function Prototype: VQ_celp_lpc_quantizer                          */
/*======================================================================*/
void VQ_celp_lpc_quantizer
(
float lpc_coefficients[],      /* In:  Current unquantised a-pars       */
float lpc_coefficients_8[],    /* In:  Current unquantised a-pars(8 kHz)*/
float int_Qlpc_coefficients[], /* Out: Qaunt/interpolated a-pars        */
long  lpc_indices[],           /* Out: Codes thar are transmitted       */
long  lpc_order,               /* In:  Order of LPC                     */
long  num_lpc_indices,         /* In:  Number of packes LPC codes       */
long  n_lpc_analysis,          /* In:  Number of LPC/frame              */
long  n_subframes,             /* In:  Number of subframes              */
long  *interpolation_flag,     /* Out: Interpolation Flag               */
long  *send_lpc_flag,          /* Out: Send LPC flag                    */
long  Wideband_VQ,
PHI_PRIV_TYPE *PHI_Priv	       /* In/Out: PHI private data (instance context) */
);

/*======================================================================*/
/*   Function Prototype: celp_lpc_analysis_filter                       */
/*======================================================================*/
void celp_lpc_analysis_filter
(
float PP_InputSignal[],         /* In:  Input Signal [0..sbfrm_size-1]  */
float lpc_residual[],           /* Out: LPC residual [0..sbfrm_size-1]  */
float int_Qlpc_coefficients[],  /* In:  LPC Coefficients[0..lpc_order-1]*/
long  lpc_order,                /* In:  Order of LPC                    */
long  sbfrm_size,               /* In:  Number of samples in subframe   */
PHI_PRIV_TYPE *PHI_Priv	        /* In/Out: PHI private data (instance context) */
);

/*======================================================================*/
/*   Function Prototype: celp_lpc_synthesis_filter                      */
/*======================================================================*/
void celp_lpc_synthesis_filter
(
float excitation[],             /* In:  Input Signal [0..sbfrm_size-1]  */
float synth_signal[],           /* Out: LPC residual [0..sbfrm_size-1]  */
float int_Qlpc_coefficients[],  /* In:  LPC Coefficients[0..lpc_order-1]*/
long  lpc_order,                /* In:  Order of LPC                    */
long  sbfrm_size,               /* In:  Number of samples in subframe   */
PHI_PRIV_TYPE *PHI_Priv	        /* In/Out: PHI private data (instance context) */
);

/*======================================================================*/
/*   Function Prototype: celp_weighting_module                          */
/*======================================================================*/
void celp_weighting_module
(
float lpc_coefficients[],       /* In:  LPC Coefficients[0..lpc_order-1]*/
long  lpc_order,                /* In:  Order of LPC                    */
float Wnum_coeff[],             /* Out: num. coeffs[0..Wnum_order-1]    */
float Wden_coeff[],             /* Out: den. coeffs[0..Wden_order-1]    */
float gamma_num,                /* In:  Weighting factor: numerator     */
float gamma_den                 /* In:  Weighting factor: denominator   */
);
   
/*======================================================================*/
/*   Function Prototype: PHI_InitLpcAnalysisEncoder                     */
/*======================================================================*/
void PHI_InitLpcAnalysisEncoder
(
long  win_size[],               /* In:  LPC Analysis-Window Size        */
long  n_lpc_analysis,           /* In:  Numberof LPC Analysis Frame     */
long  order,                    /* In:  Order of LPC                    */
long  order_8,                  /* In:  Order of LPC                    */
float gamma_be,                 /* In:  Bandwidth Expansion Coefficient */
long  bit_rate,                 /* In:  Bit Rate                        */
long  sampling_frequency,       /* In:  Sampling Frequency              */
long  SampleRateMode,           /* In:  SampleRateMode                  */
long  frame_size,               /* In:  Frame Size                      */
long  num_lpc_indices,          /* In:  Number of LPC indices           */  
long  n_subframes,              /* In:  Number of subframes             */
long  num_shape_cbks,           /* In:  Number of Shape Codebooks       */
long  num_gain_cbks,            /* In:  Number of Gain Codebooks        */
long  frame_bit_allocation[],   /* In:  Frame bit allocation            */
long  num_indices,
long  QuantizationMode,
PHI_PRIV_TYPE *PHI_Priv		/* In/Out: PHI private data (instance context) */
);

/*======================================================================*/
/*   Function Prototype: PAN_InitLpcAnalysisEncoder                     */
/*======================================================================*/
void PAN_InitLpcAnalysisEncoder
(
long  win_size[],               /* In:  LPC Analysis-Window Size        */
long  n_lpc_analysis,           /* In:  Number of LP analysis/frame     */
long  order,                    /* In:  Order of LPC                    */
float gamma_be,                 /* In:  Bandwidth Expansion Coefficient */
long  bit_rate ,                /* In:  Bit Rate                        */
PHI_PRIV_TYPE *PHI_Priv		/* In/Out: PHI private data (instance context) */
);

/*======================================================================*/
/*   Function Prototype: PHI_InitLpcAnalysisDecoder                     */
/*======================================================================*/
void PHI_InitLpcAnalysisDecoder
(
long  order,                    /* In:  Order of LPC                    */
long  order_8,                  /* In:  Order of LPC                    */
PHI_PRIV_TYPE *PHI_Priv		/* In/Out: PHI private data (instance context) */
);

/*======================================================================*/
/* Function Prototype: PHI_FreeLpcAnalysisEncoder                       */
/*======================================================================*/
void PHI_FreeLpcAnalysisEncoder
(
long n_lpc_analysis,             /* In: Number of LP analysis/frame */
PHI_PRIV_TYPE *PHI_Priv		/* In/Out: PHI private data (instance context) */
);

/*======================================================================*/
/* Function Prototype: PAN_FreeLpcAnalysisEncoder                       */
/*======================================================================*/
void PAN_FreeLpcAnalysisEncoder
(
long n_lpc_analysis,             /* In: Number of LP analysis/frame */
PHI_PRIV_TYPE *PHI_Priv		/* In/Out: PHI private data (instance context) */
);

/*======================================================================*/
/* Function Prototype: PHI_FreeLpcAnalysisDecoder                       */
/*======================================================================*/
void PHI_FreeLpcAnalysisDecoder
(
PHI_PRIV_TYPE *PHI_Priv		/* In/Out: PHI private data (instance context) */
);

/*======================================================================*/
/* Function Prototype: PHI_Adjust_bit_rate                              */
/*======================================================================*/
long PHI_Adjust_bit_rate(
  const long offset);

/*======================================================================*/
/*   Function Prototype: PHI_Interpolation                              */
/*======================================================================*/
void PHI_Interpolation
(
    const long flag,                  /* In: Interpoaltion     flag  */
    PHI_PRIV_TYPE *PHI_Priv	      /* In/Out: PHI private data (instance context) */
);

/*======================================================================*/
/*   Function Prototype: celp_lpc_analysis_lag                          */
/*======================================================================*/

void celp_lpc_analysis_lag
(
float PP_InputSignal[],         /* In:  Input Signal                    */
float lpc_coefficients[],       /* Out: LPC Coefficients[0..lpc_order-1]*/
float *first_order_lpc_par,     /* Out: a_parameter for 1st-order fit   */
long  frame_size,               /* In:  Number of samples in frame      */
long  window_offsets[],         /* In:  offset for window w.r.t curr. fr*/
long  window_sizes[],           /* In:  LPC Analysis-Window Size        */
float *windows[],               /* In:  Array of LPC Analysis windows   */
float gamma_be[],               /* In:  Bandwidth expansion coefficients*/
long  lpc_order,                /* In:  Order of LPC                    */
long  n_lpc_analysis            /* In:  Number of LP analysis/frame     */
);


/*======================================================================*/
/*   Function Prototype: PHI_lpc_analysis_lag                           */
/*======================================================================*/

void PHI_lpc_analysis_lag
(
float PP_InputSignal[],         /* In:  Input Signal                    */
float lpc_coefficients[],       /* Out: LPC Coefficients[0..lpc_order-1]*/
float *first_order_lpc_par,     /* Out: a_parameter for 1st-order fit   */
long  frame_size,               /* In:  Number of samples in frame      */
float HamWin[],                 /* In:  Hamming Window                  */
long  window_offset,            /* In:  offset for window w.r.t curr. fr*/
long  window_size,              /* In:  LPC Analysis-Window Size        */
float gamma_be[],               /* In:  Bandwidth expansion coeffs.     */
long  lpc_order                 /* In:  Order of LPC                    */
);

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _phi_lpc_h_ */

/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 17-04-96 R. Taori  Initial Version                                   */
/* 30-07-96 R. Taori  Modified interface  to meet the MPEG-4 requirement*/
/* 30-08-96 R. Taori  Prefixed "PHI_" to several subroutines(MPEG req.) */
/* 07-11-96 N. Tanaka (Panasonic)                                       */
/*                    Added several modules for narrowband coder (PAN_) */
