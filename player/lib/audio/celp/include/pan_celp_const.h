/*
This software module was originally developed by
Naoya Tanaka (Matsushita Communication Industrial Co., Ltd.)
and edited by

in the course of development of the
MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.
This software module is an implementation of a part of one or more
MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4 Audio
standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio standards
free license to this software module or modifications thereof for use in
hardware or software products claiming conformance to the MPEG-2 NBC/
MPEG-4 Audio  standards. Those intending to use this software module in
hardware or software products are advised that this use may infringe
existing patents. The original developer of this software module and
his/her company, the subsequent editors and their companies, and ISO/IEC
have no liability for use of this software module or modifications
thereof in an implementation. Copyright is not released for non
MPEG-2 NBC/MPEG-4 Audio conforming products. The original developer
retains full right to use the code for his/her  own purpose, assign or
donate the code to a third party and to inhibit third party from using
the code for non MPEG-2 NBC/MPEG-4 Audio conforming products.
This copyright notice must be included in all copies or derivative works.
Copyright (c)1996.
*/

/*	pan_celp_const.h		*/
/*	Description of constant values used in the CELP core */
/*		06/18/97 Ver 3.01 */

#define     PAN_GAMMA_BE .9902  /* LPC band expansion factor Be=12.5Hz */

/* Perceptual weighting */
#define		PAN_GAM_AR	0.5	/* perceptual weighting factor */
#define		PAN_GAM_MA	0.9	/* perceptual weighting factor */

/* LSP quantizer */
#define PAN_DIM_MAX 10
#define PAN_N_DC_LSP_MAX 8
#define PAN_N_DC_LSP_CELP 2
#define PAN_LSP_AR_R_CELP 0.5
#define PAN_MINGAP_CELP (2./256.)
#define PAN_N_DC_LSP_CELP_W 4
#define PAN_LSP_AR_R_CELP_W 0.5
#define PAN_MINGAP_CELP_W (1./256.)

/* Bit allocation */
#define	PAN_BIT_LSP22_0	4
#define	PAN_BIT_LSP22_1	4
#define	PAN_BIT_LSP22_2	7
#define	PAN_BIT_LSP22_3	6
#define	PAN_BIT_LSP22_4	1
#define PAN_NUM_LPC_INDICES	5

/* Bit allocation (for wideband) */
#define	PAN_BIT_LSP_WL_0 5
#define	PAN_BIT_LSP_WL_1 5
#define	PAN_BIT_LSP_WL_2 7
#define	PAN_BIT_LSP_WL_3 7
#define	PAN_BIT_LSP_WL_4 1
#define	PAN_BIT_LSP_WU_0 4
#define	PAN_BIT_LSP_WU_1 4
#define	PAN_BIT_LSP_WU_2 7
#define	PAN_BIT_LSP_WU_3 5
#define	PAN_BIT_LSP_WU_4 1
#define PAN_NUM_LPC_INDICES_W 10
#define PAN_NUM_LSP_BITS_W 46

/* Others */
#define PAN_N_PCAN_INT 6 /* Dummy */
#define		PAN_PI	3.14159265358979

