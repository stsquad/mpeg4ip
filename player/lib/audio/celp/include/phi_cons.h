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
/*      INCLUDE_FILE:   WDBXX.H                                       */
/*      PACKAGE:        Wdbxx                                         */
/*      COMPONENT:      Constants Used by the Philips Modules         */
/*                                                                    */
/*====================================================================*/

#ifndef _wdbxx_h_
#define _wdbxx_h_


/* =====================================================================*/
/* SAMPLING RATE                                                        */
/* =====================================================================*/
#define SAMP_RATE      16000
#define MAX_CHANNELS   1

/* =====================================================================*/
/* BIT RATE RELATED CONSTANTS                                           */
/* =====================================================================*/
#define HEADER_SIZE    1

/* =====================================================================*/
/* MATHEMATICAL CONSTANTS                                               */
/* =====================================================================*/
#ifndef PI2
#define PI2            (1.570796327)
#endif
#ifndef PI
#define PI             (3.141592654)
#endif
#define TWOPI          (6.283185307)

/* =====================================================================*/
/* Sampling rate dependent constants                                    */
/* =====================================================================*/
#define ONE_MS         ( SAMP_RATE/1000 )
#define HALF_MS        ( SAMP_RATE/2000 )
#define ONE_N_QUART_MS ( SAMP_RATE/800 )
#define ONE_N_HALF_MS  (ONE_MS + HALF_MS)
#define TWO_MS         (2  * ONE_MS)
#define TWO_N_HALF_MS  (TWO_MS + HALF_MS)
#define FIVE_MS        (5  * ONE_MS)
#define TEN_MS         (10 * ONE_MS)
#define FIFTEEN_MS     (15 * ONE_MS)
#define TWENTY_MS      (20 * ONE_MS)

/* =====================================================================*/
/* LPC  Related Constants for WDBxx                                     */
/* =====================================================================*/

#define ORDER_LPC_8           10
#define ORDER_LPC_16          20

#define N_INDICES_SQ8         4
#define N_INDICES_SQ16        9
#define N_INDICES_SQ8LL       10
#define N_INDICES_SQ16LL      20
#define N_INDICES_VQ8         5
#define N_INDICES_VQ16        10

#define GAMMA_BE              0.9883000000
#define GAMMA_WF              0.8

/* =====================================================================*/
/* Adaptive Codebook Related Constants for WDBxx                        */
/* =====================================================================*/
#define Lmin                  ( TWO_N_HALF_MS )
#define ACBK_SIZE             ( FIFTEEN_MS + ONE_MS )
#define Lmax                  ( Lmin + ACBK_SIZE - 1)
#define Sa                    ((int)3 )
#define Sam                   ((int)(Sa/2))
#define Pa                    ((int)(MAX_N_LAG_CANDIDATES/Sa))
#define NUM_CBKS              2

/* =====================================================================*/
/* Fixed Codebook  Related Constants for WDBxx                          */
/* =====================================================================*/
#define FCBK_PRESELECT_VECS   ((int)5 )
#define FCBK_GAIN_LEVELS      ((int)31 )

#endif  /*  _wdbxx_h_ */
/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 05-05-96  R. Taori                Initial Version                    */
/* 10-09-96  R. Taori & A.Gerrits    Modified for MPEG Conformation     */
/* 26-09-96  R. Taori & A.Gerrits    BIT_RATE and Interpolation level   */
/* 09-10-96  R. Taori & A.Gerrits    Bit Rate dependencies introduced   */
