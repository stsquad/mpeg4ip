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
/*      INCLUDE_FILE:   PHI_APRE.H                                      */
/*      PACKAGE:        WDBxx                                           */
/*      COMPONENT:      Adaptive Codebook Preselection Module           */
/*                                                                      */
/*======================================================================*/


#ifndef _phi_apre_h_
#define _phi_apre_h_


#ifdef __cplusplus
extern "C" {
#endif

/*======================================================================*/
/* Function Prototype: PHI_allocate_energy_table                        */
/*======================================================================*/
void PHI_allocate_energy_table
(
    const long sbfrm_size,          /* In: Subframe size in samples     */
    const long acbk_size,           /* In: Length of Adaptive Codebook  */
    const long sac_search_step      /* In: Try every $1-th sample       */
    
);
    
/*======================================================================*/
/* Function Prototype: PHI_cba_preselection                             */
/*======================================================================*/
void PHI_cba_preselection
(
const long  nos,	      /* In:   Number of samples to be processed	*/
const long  max_lag,      /* In:   Maximum Permitted Adapt cbk Lag 	    */
const long  min_lag,      /* In:   Minimum Permitted Adapt cbk Lag 	    */
const long  n_sbfrm_frm,  /* In:   Number of subframes in a frame  	    */
const long  n_lags,       /* In:   Number of lag candidates			    */
const float ca[],	      /* In:   Segment from adaptive codebook  	    */
const float ta[],	      /* In:   Backward filtered target signal 	    */
const float a,  	      /* In:   Lpc Coefficients (weighted) 		    */
long  pi[], 		      /* Out:  Result preselection: Lag candidates  */
const long  n		      /* In:   Subframe Counter (we need this) 	    */
            		      /*	   For explanation :see philips proposal*/
);

/*======================================================================*/
/* Function Prototype: PHI_free_energy_table                            */
/*======================================================================*/
void 
PHI_free_energy_table
(
    const long sbfrm_size,          /* In: Subframe size in samples     */
    const long acbk_size            /* In: Length of Adaptive Codebook  */
);

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _phi_apre_h_ */

/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 17-04-96 R. Taori  Initial Version                                   */
