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

#ifndef _phi_nec_lpc_h_
#define _phi_nec_lpc_h_


#ifdef __cplusplus
extern "C" {
#endif

/*======================================================================*/
/*   Function Prototype: celp_lpc_analysis_bws                          */
/*======================================================================*/
void
celp_lpc_analysis_bws
(
float PP_InputSignal[],         /* In:  Input Signal                    */
float lpc_coefficients[],       /* Out: LPC Coefficients[0..lpc_order-1]*/
float *first_order_lpc_par,     /* Out: a_parameter for 1st-order fit   */
long  frame_size,               /* In:  Number of samples in frame      */
long  window_offsets[],         /* In:  offset for window w.r.t curr. fr*/
long  window_sizes[],           /* In:  LPC Analysis-Window Size        */
long  lpc_order,                /* In:  Order of LPC                    */
long  n_lpc_analysis            /* In:  Number of LP analysis/frame     */
); 

/*======================================================================*/
/*   Function Prototype: NEC_InitLpcAnalysisEncoder                     */
/*======================================================================*/
void
NEC_InitLpcAnalysisEncoder
(
long  win_size[],               /* In:  LPC Analysis-Window Size        */
long  n_lpc_analysis,           /* In:  Number of LP analysis/frame     */
long  order,                    /* In:  Order of LPC                    */
float gamma_be,                 /* In:  Bandwidth Expansion Coefficient */
long  bit_rate                  /* In:  Bit Rate                        */
);

/*======================================================================*/
/* Function Prototype: NEC_FreeLpcAnalysisEncoder                       */
/*======================================================================*/
void
NEC_FreeLpcAnalysisEncoder
(
long n_lpc_analysis             /* In: Number of LP analysis/frame */
);

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _phi_nec_lpc_h_ */

/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 17-04-96 R. Taori  Initial Version                                   */
/* 30-07-96 R. Taori  Modified interface  to meet the MPEG-4 requirement*/
/* 30-08-96 R. Taori  Prefixed "PHI_" to several subroutines(MPEG req.) */
/* 07-11-96 N. Tanaka (Panasonic)                                       */
/*                    Added several modules for narrowband coder (PAN_) */
