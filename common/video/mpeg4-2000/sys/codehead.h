/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: March, 1996)

and edited by
	Yoshihiro Kikuchi (TOSHIBA CORPORATION)
	Takeshi Nagai (TOSHIBA CORPORATION)
	Toshiaki Watanabe (TOSHIBA CORPORATION)
	Noboru Yamaguchi (TOSHIBA CORPORATION)

and also edited by
	Dick van Smirren (D.vanSmirren@research.kpn.com), KPN Research
	Cor Quist (C.P.Quist@research.kpn.com), KPN Research
	(date: July, 1998)

and also edited by
    Mathias Wien (wien@ient.rwth-aachen.de) RWTH Aachen / Robert BOSCH GmbH

in the course of development of the MPEG-4 Video (ISO/IEC 14496-2). 
This software module is an implementation of a part of one or more MPEG-4 Video tools 
as specified by the MPEG-4 Video. 
ISO/IEC gives users of the MPEG-4 Video free license to this software module or modifications 
thereof for use in hardware or software products claiming conformance to the MPEG-4 Video. 
Those intending to use this software module in hardware or software products are advised that its use may infringe existing patents. 
The original developer of this software module and his/her company, 
the subsequent editors and their companies, 
and ISO/IEC have no liability for use of this software module or modifications thereof in an implementation. 
Copyright is not released for non MPEG-4 Video conforming products. 
Microsoft retains full right to use the code for his/her own purpose, 
assign or donate the code to a third party and to inhibit third parties from using the code for non <MPEG standard> conforming products. 
This copyright notice must be included in all copies or derivative works. 

Copyright (c) 1996, 1997.

Module Name:

	codehead.h

Abstract:
	define number of bits and some information for encoder/decoder

Revision History:
	Sept. 29, 1997: add Video Packet overhead by Toshiba
    Feb.16,  1999 : add Quarter Sample 
                    Mathias Wien (wien@ient.rwth-aachen.de) 

*************************************************************************/

#ifndef __CODEHEAD_H_ 
#define __CODEHEAD_H_

#define MARKER_BIT						1

#define START_CODE_PREFIX				1
#define NUMBITS_START_CODE_PREFIX		24
//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
#define NUMBITS_START_CODE_SUFFIX		8
//	End Toshiba(1998-1-16:DP+RVLC)

// session overhead information
#define USER_DATA_START_CODE			0xB2
#define VSS_START_CODE					0xB0	// 8-bit
#define VSS_END_CODE					0xB1	// 8-bit
#define NUMBITS_VSS_PROFILE				8
#define VSO_START_CODE					0xB5	// 8-bit
#define VSO_VERID						1
#define VSO_TYPE						1
#define NUMBITS_VSO_VERID				4
#define NUMBITS_VSO_PRIORITY			3
#define NUMBITS_VSO_TYPE				4

// VO overhead information
#define NUMBITS_VO_START_CODE			3
#define VO_START_CODE					0
#define NUMBITS_VO_ID					5

// VOL overhead information
#define NUMBITS_SHORT_HEADER_START_CODE 22 // Added by KPN for short headers (1998-02-07, DS)
#define SHORT_VIDEO_START_MARKER        32 // Added by KPN for short headers (1998-02-07, DS)
#define NUMBITS_VOL_START_CODE			4
#define VOL_START_CODE					2
#define NUMBITS_VOL_ID					4
#define NUMBITS_VOL_SHAPE				2
#define NUMBITS_TIME_RESOLUTION			16
#define NUMBITS_VOL_FCODE				3
#define NUMBITS_SEP_MOTION_TEXTURE		1
#define NUMBITS_QMATRIX					8

// GOV overhead information
#define GOV_START_CODE					0xB3
#define NUMBITS_GOV_START_CODE			8
#define NUMBITS_GOV_TIMECODE_HOUR		5
#define NUMBITS_GOV_TIMECODE_MIN		6
#define NUMBITS_GOV_TIMECODE_SEC		6
#define GOV_CLOSED						0
#define NUMBITS_GOV_CLOSED				1
#define GOV_BROKEN_LINK					0
#define NUMBITS_GOV_BROKEN_LINK			1

// sprite data
#ifdef __VERIFICATION_MODEL_
#define NUMBITS_SPRITE_USAGE			2
#else
#define NUMBITS_SPRITE_USAGE			1
#endif //__VERIFICATION_MODEL_
#define NUMBITS_SPRITE_HDIM				13
#define NUMBITS_SPRITE_VDIM				13
#define NUMBITS_SPRITE_LEFT_EDGE		13
#define NUMBITS_SPRITE_TOP_EDGE			13
#define NUMBITS_NUM_SPRITE_POINTS		6
#define NUMBITS_WARPING_ACCURACY		2
#define SPRITE_MV_ESC					2
#define NUMBITS_SPRITE_MV_ESC			13
#define NUMBITS_SPRITE_MB_OFFSET		9  //low latency stuff
#define NUMBITS_SPRITE_XMIT_MODE		2  //low latency stuff

// VOP overhead information
#define VOP_START_CODE					0xB6
#define NUMBITS_VOP_START_CODE			8
#define NUMBITS_VOP_TIMEINCR			10
#define NUMBITS_VOP_HORIZONTAL_SPA_REF	13
#define NUMBITS_VOP_VERTICAL_SPA_REF	13
#define NUMBITS_VOP_WIDTH				13
#define NUMBITS_VOP_HEIGHT				13
#define NUMBITS_VOP_PRED_TYPE			2
#define NUMBITS_VOP_QUANTIZER			5
#define NUMBITS_VOP_ALPHA_QUANTIZER		6
#define NUMBITS_VOP_FCODE				3

// Video Packet	overhead, added by Toshiba
#define	NUMBITS_VP_RESYNC_MARKER		17
#define	RESYNC_MARKER					0x1
#define	NUMBITS_VP_QUANTIZER			NUMBITS_VOP_QUANTIZER
#define	NUMBITS_VP_HEC					1
#define	NUMBITS_VP_PRED_TYPE			NUMBITS_VOP_PRED_TYPE
#define	NUMBITS_VP_INTRA_DC_SWITCH_THR	3

// for Data Partitioning By Toshiba(1998-1-16:DP+RVLC)
#define	NUMBITS_DP_MOTION_MARKER		17
#define	MOTION_MARKER				0x1F001
#define	NUMBITS_DP_DC_MARKER			19
#define	DC_MARKER				0x6B001
// End toshiba(1998-1-16:DP+RVLC)

// for MB ovrehead information
#define NUMBITS_MB_SKIP					1

// for motion estimation
/* changed by mwi 28JUL98 for Quarter Sample*/
/* #define EXPANDY_REFVOP					32 */
/* #define EXPANDUV_REFVOP					16 */
/* #define EXPANDY_REF_FRAME				48 */
/* #define EXPANDUV_REF_FRAME				24 */
/* #define EXPANDY_REF_FRAMEx2				96 */
/* #define EXPANDUV_REF_FRAMEx2			48 */
#define EXPANDY_REFVOP					16
#define EXPANDUV_REFVOP					8

//OBSS_SAIT_FIX000524
#define EXPANDY_REF_FRAME				32 /* 16 */
#define EXPANDUV_REF_FRAME				16 /* 8  */
#define EXPANDY_REF_FRAMEx2				64 /* 32 */
#define EXPANDUV_REF_FRAMEx2			32 /* 16 */
//#ifdef _OBSS_
//#define EXPANDY_REF_FRAME				256			
//#define EXPANDUV_REF_FRAME				128			
//#define EXPANDY_REF_FRAMEx2				512			
//#define EXPANDUV_REF_FRAMEx2			256		
//#else  _OBSS_
//#define EXPANDY_REF_FRAME				32 /* 16 */
//#define EXPANDUV_REF_FRAME				16 /* 8  */
//#define EXPANDY_REF_FRAMEx2				64 /* 32 */
//#define EXPANDUV_REF_FRAMEx2			32 /* 16 */
//#endif _OBSS_
//~OBSS_SAIT_FIX000524

//#define MAX_DISP						3
#define ADD_DISP						2

// Block DCT parameters
#define NUMBITS_ESC_RUN 6
#define NUMBITS_ESC_LEVEL 8

//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
#define NUMBITS_RVLC_ESC_RUN 6
//#define NUMBITS_RVLC_ESC_LEVEL 7
//	End Toshiba(1998-1-16:DP+RVLC)

// Shape coding
#define GRAY_ALPHA_THRESHOLD 64
#define MC_BAB_SIZE 18
#define BAB_SIZE 20
#define BAB_BORDER 2
#define MC_BAB_BORDER 1
#define BAB_BORDER_BOTH 4
#define TOTAL_BAB_SIZE 20

#endif // __CODEHEAD_H_
