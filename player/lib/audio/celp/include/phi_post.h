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
/*      INCLUDE_FILE:   PHI_POST.H                                      */
/*      PACKAGE:        WDBxx                                           */
/*      COMPONENT:      Post-processing Module                          */
/*                                                                      */
/*======================================================================*/

#ifndef _phi_post_h_
#define _phi_post_h_

#include "phi_priv.h"

#ifdef __cplusplus
extern "C" {
#endif

/*======================================================================*/
/* Function Prototype: PHI_InitPostProcessor                            */
/*======================================================================*/
void                            /* Return Value: Void                   */
PHI_InitPostProcessor
(
const long lpc_order,           /* In: Order of LPC                     */
PHI_PRIV_TYPE *PHI_Priv         /* In/Out: private data (instance context)*/
);

/*======================================================================*/
/* Function Prototype: PHI_ClosePostProcessor                           */
/*======================================================================*/
void                            /* Return Value: Void                   */
PHI_ClosePostProcessor
(
PHI_PRIV_TYPE *PHI_Priv         /* In/Out: private data (instance context)*/
);

/*======================================================================*/
/* Function Prototype: celp_postprocessing                              */
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
);

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _phi_post_h_ */


/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 08-08-96 R. Taori  Initial Version                                   */






