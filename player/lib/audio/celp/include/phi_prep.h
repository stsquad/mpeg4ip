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
/*      INCLUDE_FILE:   PHI_PREP.H                                      */
/*      PACKAGE:        WDBxx                                           */
/*      COMPONENT:      Preprocessing Module                            */
/*                                                                      */
/*======================================================================*/

#ifndef _phi_prep_h_
#define _phi_prep_h_


#ifdef __cplusplus
extern "C" {
#endif

/*======================================================================*/
/* Function Prototype: celp_preprocessing                               */
/*======================================================================*/
void                            /* Return Value: Void                   */
celp_preprocessing
(
float InputSignal[],            /* In:  Input Signal                    */
float PP_InputSignal[],         /* Out: Preprocessed Input Signal       */   
float *prev_x,                  /* In/Out: Previous x                   */
float *prev_y,                  /* In/Out: Previous y                   */
long  frame_size,               /* In:  Number of samples in frame      */
long  sampling_frequency        /* In:  Sampling Frequency              */
);

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _phi_prep_h_ */

/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 17-04-96 R. Taori  Initial Version                                   */
/* 30-07-96 R. Taori  Modified interface  to meet the MPEG-4 requirement*/
