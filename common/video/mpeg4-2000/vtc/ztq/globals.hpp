/* $Id: globals.hpp,v 1.2 2003/08/01 22:48:43 wmaycisco Exp $ */
/****************************************************************************/
/*   MPEG4 Visual Texture Coding (VTC) Mode Software                        */
/*                                                                          */
/*   This software was jointly developed by the following participants:     */
/*                                                                          */
/*   Single-quant,  multi-quant and flow control                            */
/*   are provided by  Sarnoff Corporation                                   */
/*     Iraj Sodagar   (iraj@sarnoff.com)                                    */
/*     Hung-Ju Lee    (hjlee@sarnoff.com)                                   */
/*     Paul Hatrack   (hatrack@sarnoff.com)                                 */
/*     Shipeng Li     (shipeng@sarnoff.com)                                 */
/*     Bing-Bing Chai (bchai@sarnoff.com)                                   */
/*     B.S. Srinivas  (bsrinivas@sarnoff.com)                               */
/*                                                                          */
/*   Bi-level is provided by Texas Instruments                              */
/*     Jie Liang      (liang@ti.com)                                        */
/*                                                                          */
/*   Shape Coding is provided by  OKI Electric Industry Co., Ltd.           */
/*     Zhixiong Wu    (sgo@hlabs.oki.co.jp)                                 */
/*     Yoshihiro Ueda (yueda@hlabs.oki.co.jp)                               */
/*     Toshifumi Kanamaru (kanamaru@hlabs.oki.co.jp)                        */
/*                                                                          */
/*   OKI, Sharp, Sarnoff, TI and Microsoft contributed to bitstream         */
/*   exchange and bug fixing.                                               */
/*                                                                          */
/*                                                                          */
/* In the course of development of the MPEG-4 standard, this software       */
/* module is an implementation of a part of one or more MPEG-4 tools as     */
/* specified by the MPEG-4 standard.                                        */
/*                                                                          */
/* The copyright of this software belongs to ISO/IEC. ISO/IEC gives use     */
/* of the MPEG-4 standard free license to use this  software module or      */
/* modifications thereof for hardware or software products claiming         */
/* conformance to the MPEG-4 standard.                                      */
/*                                                                          */
/* Those intending to use this software module in hardware or software      */
/* products are advised that use may infringe existing  patents. The        */
/* original developers of this software module and their companies, the     */
/* subsequent editors and their companies, and ISO/IEC have no liability    */
/* and ISO/IEC have no liability for use of this software module or         */
/* modification thereof in an implementation.                               */
/*                                                                          */
/* Permission is granted to MPEG members to use, copy, modify,              */
/* and distribute the software modules ( or portions thereof )              */
/* for standardization activity within ISO/IEC JTC1/SC29/WG11.              */
/*                                                                          */
/* Copyright 1995, 1996, 1997, 1998 ISO/IEC                                 */
/****************************************************************************/

/************************************************************/
/*     Sarnoff Very Low Bit Rate Still Image Coder          */
/*     Copyright 1995, 1996, 1997, 1998 Sarnoff Corporation */
/************************************************************/



#ifndef GLOBALS_H
#define GLOBALS_H
#include "basic.hpp"

#ifdef DEFINE_GLOBALS
#define EXTERN
#else
#define EXTERN  extern
#endif

#ifndef DATA_STRUCT_H
#include "dataStruct.hpp"
#endif 

#ifndef MSG_H
#include "msg.hpp"
#endif 

/* Definition of bitstream(tentative solution) */
//#define VERSION	(2)

/* main data structure */
EXTERN WVT_CODEC mzte_codec;

/* for displaying state information */
EXTERN Char *mapStateToText[]
#ifdef DEFINE_GLOBALS
 = {"S_DC", "S_INIT", "S_ZTR", "S_ZTR_D", "S_IZ", "S_VZTR", "S_VAL", "S_LINIT",
 "S_LZTR", "S_LZTR_D", "S_LVZTR"}
#endif
;

/* for displaying Type information */
EXTERN Char *mapTypeToText[]
#ifdef DEFINE_GLOBALS
 =  {"IZ", "VAL", "ZTR", "VZTR", "ZTR_D", "VLEAF", "ZLEAF",
     "UNTYPED"}
#endif
;

/* for displaying arithmetic probability model information */
EXTERN Char *mapArithModelToText[]
#ifdef DEFINE_GLOBALS
= {"ACM_NONE", "ACM_SKIP", "ACM_ROOT", "ACM_VALZ", "ACM_VALNZ", "ACM_RESID",
  "ACM_DC"}
#endif
;

/* Maps the state to the arithmetic codeing probability model. */
EXTERN Int stateToProbModel[] 
#ifdef DEFINE_GLOBALS
 = {ACM_ROOT, ACM_VALNZ, ACM_VALZ, ACM_RESID, ACM_RESID, ACM_RESID, ACM_DC}
#endif
;


/* The filename (sans suffix) where the coefficient info. during 
   the encoding phase is written. */
EXTERN Char *mapFileEnc
#ifdef DEFINE_GLOBALS
="mapEnc"
#endif
;

/* The filename (sans suffix) where the coefficient info. during 
   the decoding phase is written. */
EXTERN Char *mapFileDec
#ifdef DEFINE_GLOBALS
="mapDec"
#endif
;

/* Variables to keep track of quantization values. Used when calaculating
   number of residual levels */
EXTERN Int *prevQList[3];
EXTERN Int *prevQList2[3];
EXTERN Int *scaleLev[3];


/* Probability model orders for arithmetic coder */
#define AC_MODEL_ZEROTH 0
#define AC_MODEL_FIRST  1
#define AC_MODEL_MIXED  99

/* Some arithmetic functions - be careful about using expressions
   in these as they may be calculated twice.
*/
#ifndef ABS
#define ABS(x) (((x)<0) ? -(x) : (x))
#endif
#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

/*----- Shorthand for accessing global information -----*/
#define USER_Q(c) (mzte_codec.m_SPlayer[c].SNRlayer.snr_image.quant)
#define ROOT_MAX(c) (mzte_codec.m_SPlayer[c].SNRlayer.snr_image.root_max)
#define VALZ_MAX(c) (mzte_codec.m_SPlayer[c].SNRlayer.snr_image.valz_max)
#define VALNZ_MAX(c) (mzte_codec.m_SPlayer[c].SNRlayer.snr_image.valnz_max)
#define RESID_MAX(c) (mzte_codec.m_SPlayer[c].SNRlayer.snr_image.residual_max)

// hjlee 0901
#define WVTDECOMP_MAX(c,l) \
  (mzte_codec.m_SPlayer[c].SNRlayer.snr_image.wvtDecompMax[l])
#define RESID_MAX(c) (mzte_codec.m_SPlayer[c].SNRlayer.snr_image.residual_max)
#define WVTDECOMP_NUMBITPLANES(c,l) \
  (mzte_codec.m_SPlayer[c].SNRlayer.snr_image.wvtDecompNumBitPlanes[l])
#define WVTDECOMP_RES_NUMBITPLANES(c) \
  (mzte_codec.m_SPlayer[c].SNRlayer.snr_image.wvtDecompResNumBitPlanes)

#define ALL_ZERO(c) (mzte_codec.m_SPlayer[c].SNRlayer.snr_image.allzero)
#define COEFF_VAL(x,y,c) \
  (mzte_codec.m_SPlayer[c].coeffinfo[y][x].quantized_value)
#define COEFF_ORGVAL(x,y,c) (mzte_codec.m_SPlayer[c].coeffinfo[y][x].wvt_coeff)
#define COEFF_RECVAL(x,y,c) (mzte_codec.m_SPlayer[c].coeffinfo[y][x].rec_coeff)
#define COEFF_QSTATE(x,y,c) (mzte_codec.m_SPlayer[c].coeffinfo[y][x].qState)
#define COEFF_STATE(x,y,c) (mzte_codec.m_SPlayer[c].coeffinfo[y][x].state)
#define COEFF_TYPE(x,y,c) (mzte_codec.m_SPlayer[c].coeffinfo[y][x].type)
#define COEFF_SKIP(x,y,c) (mzte_codec.m_SPlayer[c].coeffinfo[y][x].skip)
#define COEFF_MASK(x,y,c) (mzte_codec.m_SPlayer[c].coeffinfo[y][x].mask)
#define snrStartCode (mzte_codec.m_bStartCodeEnable)



#define IS_RESID(x,y,c) (COEFF_STATE(x,y,c)==S_VZTR  || \
			 COEFF_STATE(x,y,c)==S_VAL   || \
			 COEFF_STATE(x,y,c)==S_LVZTR)

#define IS_LEAF(x,y,c) (COEFF_STATE(x,y,c)==S_LINIT  || \
			COEFF_STATE(x,y,c)==S_LZTR   || \
			COEFF_STATE(x,y,c)==S_LZTR_D || \
			COEFF_STATE(x,y,c)==S_LVZTR)

#define IS_STATE_LEAF(state) ((state)==S_LINIT  || \
			      (state)==S_LZTR   || \
			      (state)==S_LZTR_D || \
			      (state)==S_LVZTR)



/* Do Y, not U or V condition. */
#define NCOL ((mzte_codec.m_iCurSpatialLev==0          \
	       && (mzte_codec.m_lastWvtDecompInSpaLayer[0][1]<0      \
		   || mzte_codec.m_lastWvtDecompInSpaLayer[0][2]<0)) \
	      ? 1 :  mzte_codec.m_iColors)

#define FIRST_SPA_LEV(numSpa, wvtDecomp, col) \
     ((mzte_codec.m_lastWvtDecompInSpaLayer[0][col]<0) ? 1 : 0)

//Added by Sarnoff for error resilience, 3/5/99
#ifndef _ERROR_RESI_
extern Int TU_first, TU_last, TU_no, TU_max, errSignal, targetPacketLength, 
 TU_max_dc, packet_size, prev_TU_first, prev_TU_last, prev_TU_err,
 overhead, errWarnSignal, errMagSignal, LTU, CTU_no, prev_segs_size;
extern Int start_h, start_w, band_height, band_width, TU_height, TU_spa_loc, 
 TU_color, TU_band, prev_LTU, packet_band, packet_TU_first, packet_top,
 packet_left;
extern Int wvt_level, level_h, level_w, subband_loc;
#endif
//end added by Sarnoff for error resilience, 3/5/99

#endif 
