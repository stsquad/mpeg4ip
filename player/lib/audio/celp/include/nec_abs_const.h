/*
This software module was originally developed by
Toshiyuki Nomura (NEC Corporation)
and edited by
Naoya Tanaka (Matsushita Communication Ind. Co., Ltd.)
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
/*
 *	MPEG-4 Audio Verification Model (LPC-ABS Core)
 *	
 *	Constants used in the NEC Modules
 *
 *	Ver1.0	96.12.16	T.Nomura(NEC)
 *	Ver2.0	97.03.17	T.Nomura(NEC)
 */

/*------ Bit rate range --------*/
#define	NEC_PAN_BITRATE1	3850
#define	NEC_PAN_BITRATE2	4900
#define	NEC_PAN_BITRATE3	5700
#define	NEC_PAN_BITRATE4	7700
#define	NEC_PAN_BITRATE5	11000
#define	NEC_PAN_BITRATE6	12200

#define	NEC_ENH_BITRATE		2000

/*------ Bitstream syntax --------*/
#define	NEC_BIT_MODE	2
#define	NEC_BIT_RMS	6
#define	NEC_BIT_ACB	8
#define	NEC_BIT_GAIN	6
#define NEC_NUM_SHAPE_CBKS	3
#define NEC_NUM_GAIN_CBKS	1
#define NEC_NUM_OTHER_INDICES	2
#define NEC_NUM_LPC_INDICES_FRQ16	6

#define	NEC_BIT_ENH_POS40_2	4
#define	NEC_BIT_ENH_SGN40_2	2
#define	NEC_BIT_ENH_POS80_4	12
#define	NEC_BIT_ENH_SGN80_4	4
#define	NEC_BIT_ENH_GAIN	4

#define NEC_BIT_ACB_FRQ16	3
#define NEC_BIT_GAIN_FRQ16	11
#define NEC_BIT_LSP1620		32
#define NEC_BIT_LSP1620_0	4
#define NEC_BIT_LSP1620_1	7
#define NEC_BIT_LSP1620_2	4
#define NEC_BIT_LSP1620_3	6
#define NEC_BIT_LSP1620_4	7
#define NEC_BIT_LSP1620_5	4

/*------ Frame --------*/
#define	NEC_FRAME40MS	320
#define	NEC_FRAME30MS	240
#define	NEC_FRAME20MS	160
#define	NEC_FRAME10MS	80
#define NEC_NSF2	2		
#define NEC_NSF3	3
#define NEC_NSF4	4

/*------ LPC Analysis ------*/
#define	NEC_LEN_LPC_ANALYSIS	80	/* LPC analysis interval */
#define	NEC_PAN_NLA		40	/* look ahead for LPC analysis */
#define	NEC_PAN_NLB		80	/* look back for LPC analysis */
#define NEC_FRQ16_NLA		80	/* look ahead for LPC analysis */
#define NEC_FRQ16_NLB		160	/* look back for LPC analysis */
#define	NEC_LPC_ORDER		10	/* LPC analysis order */
#define NEC_LPC_ORDER_FRQ16	20	/* LPC analysis order */
#define NEC_LPC_ORDER_MAXIMUM	20	/* LPC analysis order */

/*------ Pitch Analysis ------*/
#define	NEC_PITCH_MIN		17	/* minimum pitch period */
#define NEC_PITCH_MIN_FRQ16	20
#define	NEC_PITCH_MAX		144	/* maximum pitch period */
#define NEC_PITCH_MAX_FRQ16	295
#define NEC_PITCH_MAX_MAXIMUM	295
#define NEC_PITCH_LIMIT_FRQ16  778
#define	NEC_PITCH_RSLTN		6	/* pitch analysis resolution */
#define	NEC_PITCH_IFTAP		5	/* interpolation tap length */
#define	NEC_PITCH_IFTAP16	10	/* interpolation tap length */
#define NEC_PI			3.141592654
#define	NEC_ACB_BIT	8
#define	NEC_ACB_BIT_FRQ16	3

/*------ RMS ------*/
#define NEC_RMS_MAX_V	15864.0
#define NEC_RMS_MAX_U	7932.0
#define NEC_MU_LAW_V	512.0
#define	NEC_MU_LAW_U	1024.0

/*------ Others ------*/
#define	NEC_MAX_NSF	10	/* maximum number of subframes */
#define NEC_PAI		3.141592
#define NEC_LPF_DELAY   80
#define NEC_GAMMA_BE	0.9951
#define	NEC_GAM_AR	0.60	/* perceptual weighting factor */
#define	NEC_GAM_MA	0.94	/* perceptual weighting factor */

/* LPC analysis using asynmmtric windows */
#define NEC_ASW_LEN1 120
#define NEC_ASW_LEN2 80
#define NEC_ASW_LEN1_FRQ16 200
#define NEC_ASW_LEN2_FRQ16 120

/* for WB MPE mode */
#define NEC_FRAME20MS_FRQ16 320
#define NEC_FRAME10MS_FRQ16 160
#define NEC_SBFRM_SIZE40 40
#define NEC_SBFRM_SIZE80 80

#define NEC_NSF8 8
#define NEC_ENH_BITRATE_FRQ16 4000
#define NEC_ACB_BIT_WB 9
#define	NEC_BIT_GAIN_WB	7

#define NormalDelayMode 0
#define LowDelayMode 1
