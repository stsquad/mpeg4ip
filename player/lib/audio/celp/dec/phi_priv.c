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
/*====================================================================*/
/*                                                                    */
/*      SOURCE_FILE:    PHI_PRIV.C                                    */
/*      COMPONENT:      Private data used within Philips CELP decoder */
/*                                                                    */
/*====================================================================*/
    
/*====================================================================*/
/*      I N C L U D E S                                               */
/*====================================================================*/
#include "phi_priv.h"      

/*======================================================================*/
/*      G L O B A L   F U N C T I O N   D E F I N I T I O N S           */
/*======================================================================*/
/*======================================================================*/
/* Initialise private data                                                        */
/*======================================================================*/
void	PHI_Init_Private_Data(PHI_PRIV_TYPE	*PHI_Priv)
{
/*--- phi_gxit.c ---*/
    PHI_Priv->PHI_sfrm_ctr		= 0;	/* counter:indicates current subframe	*/
    PHI_Priv->PHI_prev_fcbk_gain	= 0.0F;	/* Fixed Cbk Gain of previous frame	*/ 
/*--- phi_lpc.c ---*/
    PHI_Priv->PHI_prev_int_flag		= 1;	/* Previous Interpolation Flag		*/
    PHI_Priv->PHI_prev_lpc_flag		= 0;	/* Previous LPC sent Flag		*/
    PHI_Priv->PHI_dec_prev_interpolation_flag = 0; /* Previous Interpolation Flag	*/
    PHI_Priv->PHI_dec_int_switch	= 0;	/* 0: interpolation on LARs		*/
    PHI_Priv->PHI_desired_bit_rate	= 0;
    PHI_Priv->PHI_actual_bits		= 0;
    PHI_Priv->PHI_frames_sent		= 0;
    PHI_Priv->frames			= 0.0F;
    PHI_Priv->llc_bits			= 0;
}

/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 17-03-98  P. Dillen                Initial Version                   */
