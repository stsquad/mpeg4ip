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
/*      INCLUDE_FILE:   PHI_LPCQ.H                                      */
/*      PACKAGE:        WDBxx                                           */
/*      COMPONENT:      Tables for LPC Coeffs Quantization              */
/*                                                                      */
/*======================================================================*/

#ifndef _phi_lpcq_h_
#define _phi_lpcq_h_

#ifdef __cplusplus
extern "C" {
#endif

#define SCALING_BITS_rfc     16L    /* scaling bits for rfc's           */
#define SCALE_rfc     (1L << SCALING_BITS_rfc)     /* scal. of rfc's         */

/*======================================================================*/
/*     TABLE FOR Reflection Coefficients QUANTIZATION                   */
/* UNIFORM LAR QUANTISATION ==> NONLINEAR LOOK_UP TABLE for Refl. coeffs*/
/*======================================================================*/
float  PHI_tbl_rfc_level[2][49] =
{
    {
    (float)-0.9882, 
    (float)-0.9848, 
    (float)-0.9806, 
    (float)-0.9751, 
    (float)-0.9682, 
    (float)-0.9593, 
    (float)-0.9481, 
    (float)-0.9338, 
    (float)-0.9158, 
    (float)-0.8932, 
    (float)-0.8649, 
    (float)-0.8298, 
    (float)-0.7866, 
    (float)-0.7341, 
    (float)-0.6710, 
    (float)-0.5964, 
    (float)-0.5098, 
    (float)-0.4116, 
    (float)-0.3027, 
    (float)-0.1853, 
    (float)-0.0624, 
    (float)0.0624, 
    (float)0.1853, 
    (float)0.3027, 
    (float)0.4116, 
    (float)0.5098, 
    (float)0.5964, 
    (float)0.6710, 
    (float)0.7341, 
    (float)0.7866, 
    (float)0.8298, 
    (float)0.8649, 
    (float)0.8932, 
    (float)0.9158, 
    (float)0.9338, 
    (float)0.9481, 
    (float)0.9593, 
    (float)0.9682, 
    (float)0.9751, 
    (float)0.9806, 
    (float)0.9848, 
    (float)0.9882, 
    (float)0.9908, 
    (float)0.9928, 
    (float)0.9944, 
    (float)0.9956, 
    (float)0.9966, 
    (float)0.9973, 
    (float)0.9999
     },
     
     {
    (long) (-0.9896 * SCALE_rfc - 0.5) / (double) SCALE_rfc,
    (long) (-0.9866 * SCALE_rfc - 0.5) / (double) SCALE_rfc,
    (long) (-0.9828 * SCALE_rfc - 0.5) / (double) SCALE_rfc,
    (long) (-0.9780 * SCALE_rfc - 0.5) / (double) SCALE_rfc,
    (long) (-0.9719 * SCALE_rfc - 0.5) / (double) SCALE_rfc,
    (long) (-0.9640 * SCALE_rfc - 0.5) / (double) SCALE_rfc,
    (long) (-0.9540 * SCALE_rfc - 0.5) / (double) SCALE_rfc,
    (long) (-0.9414 * SCALE_rfc - 0.5) / (double) SCALE_rfc,
    (long) (-0.9253 * SCALE_rfc - 0.5) / (double) SCALE_rfc,
    (long) (-0.9051 * SCALE_rfc - 0.5) / (double) SCALE_rfc,
    (long) (-0.8798 * SCALE_rfc - 0.5) / (double) SCALE_rfc,
    (long) (-0.8483 * SCALE_rfc - 0.5) / (double) SCALE_rfc,
    (long) (-0.8093 * SCALE_rfc - 0.5) / (double) SCALE_rfc,
    (long) (-0.7616 * SCALE_rfc - 0.5) / (double) SCALE_rfc,
    (long) (-0.7039 * SCALE_rfc - 0.5) / (double) SCALE_rfc,
    (long) (-0.6351 * SCALE_rfc - 0.5) / (double) SCALE_rfc,
    (long) (-0.5546 * SCALE_rfc - 0.5) / (double) SCALE_rfc,
    (long) (-0.4621 * SCALE_rfc - 0.5) / (double) SCALE_rfc,
    (long) (-0.3584 * SCALE_rfc - 0.5) / (double) SCALE_rfc,
    (long) (-0.2449 * SCALE_rfc - 0.5) / (double) SCALE_rfc,
    (long) (-0.1244 * SCALE_rfc - 0.5) / (double) SCALE_rfc,
    (long) (0.0000),
    (long) (0.1244 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.2449 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.3584 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.4621 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.5546 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.6351 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.7039 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.7616 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.8093 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.8483 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.8798 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.9051 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.9253 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.9414 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.9540 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.9640 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.9719 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.9780 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.9828 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.9866 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.9896 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.9919 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.9937 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.9951 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.9961 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.9970 * SCALE_rfc + 0.5) / (double) SCALE_rfc,
    (long) (0.9977 * SCALE_rfc + 0.5) / (double) SCALE_rfc
	}
};

/*======================================================================*/
/*     TABLE FOR Range of reflection coeffs                             */
/*======================================================================*/
short PHI_tbl_rfc_range[2][20] =
{
    {
    13,
     0,
    16,
    12,
    16,
    13,
    16,
    14,
    18,
    16,
    18,
    17,
    19,
    17,
    19,
    18,
    19,
    17,
    19,
    18,
    },

    {
	36,
	28,
	15,
	14,
	13,
	13,
	12,
	11,
	9,
	8,
	8,
	7,
	7,
	8,
	7,
	6,
	6,
	7,
	6,
	6    
    }
};

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _phi_lpcq_h_ */

/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 17-04-96 R. Taori  Initial Version                                   */
