#ifndef _PHI_PRIV_H_
#define _PHI_PRIV_H_

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
/*      INCLUDE_FILE:   PHI_PRIV.H                                    */
/*      COMPONENT:      Private data used within Philips CELP decoder */
/*                                                                    */
/*====================================================================*/

#define	NEC_LSPPRDCT_ORDER	4
#define NEC_MAX_LSPVQ_ORDER	20

/*======================================================================*/
/*   Type definition: PHI_PRIV_TYPE		                        */
/*   Structure stores context for one decoded stream                    */
/*======================================================================*/
typedef struct
{
/*--- phi_bs2p.c ---*/
    /*    none	*/
#ifdef KEEP_ESLE_REMOVE_PRIV
    int	  PHI_decoded_bits;
#endif /* KEEP_ESLE_REMOVE_PRIV */
/*--- phi_fbit.c ---*/
    /*    none	*/
/*--- phi_gxit.c ---*/
    float *PHI_adaptive_cbk;		/* Adaptive Codebook			*/
    long  PHI_sfrm_ctr;			/* counter:indicates current subframe	*/
    float PHI_prev_fcbk_gain;		/* Fixed Cbk Gain of previous frame	*/ 
    long  PHI_D;
    long  PHI_Np;
/*--- phi_lpc.c ---*/
    float *PHI_mem_i;			/* Filter States of LPC Analysis Filter	*/
    float *PHI_mem_s;			/* Filter States of LPC Synthesis Filter*/
    float *PHI_prev_lar;		/* Previous log-area ratios		*/
    float *PHI_current_lar;		/* Current log-area ratios		*/
    long  *PHI_prev_indices;		/* Previous Indices			*/
    long  PHI_prev_int_flag;		/* Previous Interpolation Flag		*/
    long  PHI_prev_lpc_flag;		/* Previous LPC sent Flag		*/
    float *PHI_dec_prev_lar;		/* Previous log-area ratios		*/
    float *PHI_dec_current_lar;		/* Current log-area ratios		*/
    long  PHI_dec_prev_interpolation_flag; /* Previous Interpolation Flag	*/
    long  PHI_dec_int_switch;		/* 0: interpolation on LARs		*/
    /*----------------------------------------------------------------------*/
    /*    Variables for bit rate control in VQ mode                         */
    /* ---------------------------------------------------------------------*/
    float *next_uq_lsf_16;
    float *current_uq_lsf_16;
    float *previous_uq_lsf_16;
    float *next_q_lsf_16;
    float *current_q_lsf_16;
    float *previous_q_lsf_16;
    float *previous_q_lsf_int_16;
    float blsp_previous[NEC_LSPPRDCT_ORDER][NEC_MAX_LSPVQ_ORDER];
    float *next_uq_lsf_8;
    float *current_uq_lsf_8;
    float *previous_uq_lsf_8;
    float *next_q_lsf_8;
    float *current_q_lsf_8;
    float *previous_q_lsf_8;
    float *previous_q_lsf_int_8;
    float blsp_dec[NEC_LSPPRDCT_ORDER][NEC_MAX_LSPVQ_ORDER];
    float *next_q_lsf_16_dec;
    float *current_q_lsf_16_dec;
    float *previous_q_lsf_16_dec;
    float *next_q_lsf_8_dec;
    float *current_q_lsf_8_dec;
    float *previous_q_lsf_8_dec;
    float nec_lsp_minwidth;
    /*----------------------------------------------------------------------*/
    /*    Variables for dynamic threshold                                   */
    /* ---------------------------------------------------------------------*/
    long PHI_FRAMES; 
    long PHI_desired_bit_rate;
    long PHI_actual_bits;
    long PHI_frames_sent;
    float PHI_dyn_lpc_thresh;
    long PHI_MAX_BITS;
    long PHI_MIN_BITS;
    float PHI_FR;
    float PHI_stop_threshold;
    /*----------------------------------------------------------------------*/
    /*    Variables for statistics only                                     */
    /* ---------------------------------------------------------------------*/
    float frames;
    long  llc_bits;
/*--- phi_lsfr.c ---*/
    /*    none	*/
/*--- phi_post.c ---*/
    float *PHI_P1_states;	/* Post-Filter  delay line (inverse filter)	*/
    float *PHI_P2_states;	/* Post-Filter  delay line (synthesis filter)	*/
    float PHI_P3_state;		/* state for the tilt correction filter		*/
    float PHI_Gpf;		/* Adaptive Gain				*/
    float *PHI_g1;		/* Gamma Array for PostFilter (Analysis Path)	*/
    float *PHI_g2;		/* Gamma Array for PostFilter (Synthesis Path)	*/                           
/*--- phi_xits.c ---*/
/* PRIV not used in decoder, only in encoder
    float gp; 
*/
} PHI_PRIV_TYPE;

/*======================================================================*/
/*   Function Prototype: PHI_Init_Private_Data				*/
/*   Initialise private data                                            */
/*======================================================================*/
#ifdef __cplusplus
extern "C" {
#endif

void	PHI_Init_Private_Data(PHI_PRIV_TYPE	*PHI_Priv);

#ifdef __cplusplus
}
#endif


/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 13-03-98  P. Dillen                Initial Version                   */
#endif /* _PHI_PRIV_H_ */
