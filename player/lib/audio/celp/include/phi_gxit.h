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
/*      INCLUDE_FILE:   PHI_GXIT.H                                      */
/*      PACKAGE:        WDBxx                                           */
/*      COMPONENT:      Excitation Generation Module                    */
/*                                                                      */
/*======================================================================*/

#ifndef _phi_gxit_h_
#define _phi_gxit_h_

#include "phi_priv.h"

#ifdef __cplusplus
extern "C" {
#endif

/*======================================================================*/
/* Function Prototype:  PHI_init_excitation_generation                  */
/*======================================================================*/
void 
PHI_init_excitation_generation
(
const long max_lag,     /* In:Maximum permitted lag in the adaptive cbk */
const long sbfrm_size,  /* In:Size of subframe in samples               */
const long RPE_configuration, /* In:Confguration                        */
PHI_PRIV_TYPE * PHI_Priv /* In/Out: private data (instance context)    */
);


/*======================================================================*/
/* Function  Prototype: celp_excitation_generation                      */
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
);

/*======================================================================*/
/* Function Prototype: PHI_close_excitation_generation                  */
/*======================================================================*/
void PHI_close_excitation_generation(
     PHI_PRIV_TYPE * PHI_Priv        /* In: private data (instance context)*/
);

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _phi_gxit_h_ */

/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 17-04-96 R. Taori  Initial Version                                   */
/* 30-08-96 R. Taori  Modified interface to meet Tampere requirements   */




