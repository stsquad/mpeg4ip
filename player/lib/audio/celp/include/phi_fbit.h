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
/*      INCLUDE_FILE:   PHI_FBIT.H                                      */
/*      PACKAGE:        WDBxx                                           */
/*      COMPONENT:      Frame bit allocation table                      */
/*                                                                      */
/*======================================================================*/

#ifndef _phi_fbit_h_
#define _phi_fbit_h_

#ifdef __cplusplus
extern "C" {
#endif

/*======================================================================*/
/* Function Prototype: PHI_init_bit_allocation                          */
/*======================================================================*/

long  * PHI_init_bit_allocation(
    const long SampleRateMode,        /* In: SampleRate Mode           */
    const long RPE_configuration,      /* In:  Bit Rate Configuration    */
    const long QuantizationMode,      /* In: Type of Quantization  	    */
    const long LosslessCodingMode,    /* In: Lossless Coding Mode  	    */   
    const long FineRateControl,       /* In:  FRC flag                  */
    const long num_lpc_indices,       /* In:  Number of LPC indices     */  
    const long n_subframes,           /* In:  Number of subframes       */
    const long num_shape_cbks,        /* In:  Number of Shape Codebooks */
    const long num_gain_cbks          /* In:  Number of Gain Codebooks  */
);
 
/*======================================================================*/
/* Function Prototype: PHI_free_bit_allocation                          */
/*======================================================================*/
void PHI_free_bit_allocation(
    long  *frame_bit_allocation
);


#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _phi_fbit_h_ */
   
/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 08-10-96 A. Gerrits  Initial Version                                 */
