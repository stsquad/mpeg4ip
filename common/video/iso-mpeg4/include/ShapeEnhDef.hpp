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
/*   Scalable Shape Coding was provided by:                                 */
/*         Shipeng Li (Sarnoff Corporation),                                */
/*         Dae-Sung Cho (Samsung AIT),					    */
/*         Se Hoon Son (Samsung AIT)	                                    */
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
/*****************************************************************************
 *
 * This software module was originally developed by
 *
 *	Dae-Sung Cho (Samsung AIT)
 *	Se Hoon Son (Samsung AIT)
 *
 * and edited by
 *
 *	Dae-Sung Cho (Samsung AIT)
 *
 * in the course of development of the MPEG-4 Video (ISO/IEC 14496-2) standard.
 * This software module is an implementation of a part of one or more MPEG-4
 * Video (ISO/IEC 14496-2) tools as specified by the MPEG-4 Video (ISO/IEC
 * 14496-2) standard.
 *
 * ISO/IEC gives users of the MPEG-4 Video (ISO/IEC 14496-2) standard free
 * license to this software module or modifications thereof for use in hardware
 * or software products claiming conformance to the MPEG-4 Video (ISO/IEC
 * 14496-2) standard.
 *
 * Those intending to use this software module in hardware or software products
 * are advised that its use may infringe existing patents. The original
 * developer of this software module and his/her company, the subsequent
 * editors and their companies, and ISO/IEC have no liability for use of this
 * software module or modifications thereof in an implementation. Copyright is
 * not released for non MPEG-4 Video (ISO/IEC 14496-2) Standard conforming
 * products.
 *
 * Samsung AIT (SAIT) retains full right to use the code for his/her own
 * purpose, assign or donate the code to a third party and to inhibit third
 * parties from using the code for non MPEG-4 Video (ISO/IEC 14496-2) Standard
 * conforming products. This copyright notice must be included in all copies or
 * derivative works.
 *
 * Copyright (c) 1997
 *
 *****************************************************************************/
#ifndef _SISC_DEF_H_
#define _SISC_DEF_H_
#include        <stdio.h>
#include        <math.h>

#define	H_SAMPLING	0
#define	V_SAMPLING	1
#define	SI_COD_TYPE	256

#define	BORDER		1
#define	MBORDER		2
#define  TEXTURE_SPATIAL_START_CODE (0x1BF)
#define TEXTURE_SHAPE_START_CODE (0x1C2) /*(0x1C1)*/	// SAIT_PDAM: added by Samsung AIT
#define MARKER_BIT 1
/*  Probability Tables for Shape Coding (SI)  */

/* Inserted by shson*/
/* Probability model for bab type (0-odd, 1-even) */
// modified for FDAM1 by Samsung AIT on 2000/02/03
//static  unsigned int scalable_bab_type_prob[2]={57203,44651};
#endif
