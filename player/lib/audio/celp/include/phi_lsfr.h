/*====================================================================*/
/*         MPEG-4 Audio (ISO/IEC 14496-3) Copyright Header            */
/*====================================================================*/
/*
This software module was originally developed by P. Kabal, McGill University,
in the course of development of the MPEG-4 Audio (ISO/IEC 14496-3). This
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
/*      INCLUDE_FILE:   PHI_LSFR.H                                      */
/*                                                                      */
/*======================================================================*/

#ifndef _phi_lsfr_h_
#define _phi_lsfr_h_


#ifdef __cplusplus
extern "C" {
#endif

void PHI_lsf2pc
(
long order,            /* input : predictor order                     */
const float lsf[],     /* input : line-spectral freqs [0,pi]          */
float pc[]             /* output: predictor coeff: a_1,a_2...a_order  */
); 


void PHI_pc2lsf(		/* ret 1 on succ, 0 on failure */
long np,			    /* input : order = # of freq to cal. */
const float pc[],	    /* input : the np predictor coeff. */
float lsf[] 		    /* output: the np line spectral freq. */
);

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _phi_lsfr_h_ */

