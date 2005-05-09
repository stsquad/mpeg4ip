/* $Id: default.h,v 1.1 2005/05/09 21:29:45 wmaycisco Exp $ */
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

#ifndef _DEFAULT_H_
#define _DEFAULT_H_

#ifdef DEFINE_GLOBALS
#define EXTERN
#else
#define EXTERN extern
#endif



/* Default Analysis filters -- odd symmetric */
EXTERN Short DefaultAnalysisLPInt[]
#ifdef DEFINE_GLOBALS
= {3, -6, -16, 38, 90, 38, -16, -6, 3}
#endif
;

EXTERN Short DefaultAnalysisHPInt[]
#ifdef DEFINE_GLOBALS
= {-32, 64, -32}
#endif
;

EXTERN FILTER DefaultAnalysisFilterInt
#ifdef DEFINE_GLOBALS
={
  DWT_ODD_SYMMETRIC,
  DWT_INT_TYPE,
  3,
  9,
  DefaultAnalysisHPInt,
  DefaultAnalysisLPInt,
  128
}
#endif
;

EXTERN Double DefaultAnalysisLPDbl[]
#ifdef DEFINE_GLOBALS
= { 0.03314563036812,  -0.06629126073624,  
    -0.17677669529665,   0.41984465132952, 
    0.99436891104360,   0.41984465132952,  
    -0.17677669529665,  -0.06629126073624, 
    0.03314563036812 }
#endif
;

EXTERN Double DefaultAnalysisHPDbl[]
#ifdef DEFINE_GLOBALS
= { -0.35355339059327,   0.70710678118655,  
    -0.35355339059327 }
#endif
;

EXTERN FILTER DefaultAnalysisFilterDbl
#ifdef DEFINE_GLOBALS
={
  DWT_ODD_SYMMETRIC,
  DWT_DBL_TYPE,
  3,
  9,
  DefaultAnalysisHPDbl,
  DefaultAnalysisLPDbl,
  1
}
#endif
;

/* Default Synthesis filters - odd symmtric*/
EXTERN Short DefaultSynthesisHPInt[]
#ifdef DEFINE_GLOBALS
= {3, 6, -16, -38, 90, -38, -16, 6, 3}
#endif
;

EXTERN Short DefaultSynthesisLPInt[]
#ifdef DEFINE_GLOBALS
= {32, 64, 32}
#endif
;

EXTERN FILTER DefaultSynthesisFilterInt
#ifdef DEFINE_GLOBALS
={
  DWT_ODD_SYMMETRIC,
  DWT_INT_TYPE,
  9,
  3,
  DefaultSynthesisHPInt,
  DefaultSynthesisLPInt,
  128
}
#endif
;

EXTERN Double DefaultSynthesisHPDbl[]
#ifdef DEFINE_GLOBALS
={ 0.03314563036812,  0.06629126073624, 
   -0.17677669529665,  -0.41984465132952, 
   0.99436891104360,  -0.41984465132952, 
   -0.17677669529665,  0.06629126073624, 
   0.03314563036812 }
#endif
;

EXTERN Double DefaultSynthesisLPDbl[]
#ifdef DEFINE_GLOBALS
= { 0.35355339059327,   0.70710678118655,  
    0.35355339059327 }
#endif
;


EXTERN FILTER DefaultSynthesisFilterDbl
#ifdef DEFINE_GLOBALS
={
  DWT_ODD_SYMMETRIC,
  DWT_DBL_TYPE,
  9,
  3,
  DefaultSynthesisHPDbl,
  DefaultSynthesisLPDbl,
  1
}
#endif
;

/* Default Even Symmetric Analysis filters */
EXTERN Short DefaultEvenAnalysisLPInt[]
#ifdef DEFINE_GLOBALS
= {-5, 15, 19, -97, -26, 350, 350, -26, 
   -97, 19, 15, -5}
#endif
;

EXTERN Short DefaultEvenAnalysisHPInt[]
#ifdef DEFINE_GLOBALS
= {64, -192, 192, -64}
#endif
;

EXTERN FILTER DefaultEvenAnalysisFilterInt
#ifdef DEFINE_GLOBALS
={
  DWT_EVEN_SYMMETRIC,
  DWT_INT_TYPE,
  4,
  12,
  DefaultEvenAnalysisHPInt,
  DefaultEvenAnalysisLPInt,
  512
}
#endif
;

EXTERN Double DefaultEvenAnalysisLPDbl[]
#ifdef DEFINE_GLOBALS
= { -0.01381067932005,   0.04143203796015,   
    0.05248058141619,  -0.26792717880897,   
    -0.07181553246426,   0.96674755240348,  
    0.96674755240348,  -0.07181553246426,  
    -0.26792717880897,   0.05248058141619, 
    0.04143203796015,  -0.01381067932005}
#endif
;

EXTERN Double DefaultEvenAnalysisHPDbl[]
#ifdef DEFINE_GLOBALS
= { 0.17677669529664,  -0.53033008588991,   
    0.53033008588991,  -0.17677669529664}
#endif
;

EXTERN FILTER DefaultEvenAnalysisFilterDbl
#ifdef DEFINE_GLOBALS
={
  DWT_EVEN_SYMMETRIC,
  DWT_DBL_TYPE,
  4,
  12,
  DefaultEvenAnalysisHPDbl,
  DefaultEvenAnalysisLPDbl,
  1
}
#endif
;

/* Default Even Symmetric Synthesis filters */
EXTERN Short DefaultEvenSynthesisHPInt[]
#ifdef DEFINE_GLOBALS
= {5, 15, -19, -97, 26, 350, -350, -26, 
   97, 19, -15, -5}
#endif
;

EXTERN Short DefaultEvenSynthesisLPInt[]
#ifdef DEFINE_GLOBALS
= {64, 192, 192, 64}
#endif
;

EXTERN FILTER DefaultEvenSynthesisFilterInt
#ifdef DEFINE_GLOBALS
={
  DWT_EVEN_SYMMETRIC,
  DWT_INT_TYPE,
  12,
  4,
  DefaultEvenSynthesisHPInt,
  DefaultEvenSynthesisLPInt,
  512
}
#endif
;

EXTERN Double DefaultEvenSynthesisHPDbl[]
#ifdef DEFINE_GLOBALS
={ 0.01381067932005,   0.04143203796015,
   -0.05248058141619,  -0.26792717880897,  
   0.07181553246426,   0.96674755240348,  
   -0.96674755240348,  -0.07181553246426,    
   0.26792717880897,   0.05248058141619,  
   -0.04143203796015,  -0.01381067932005}
#endif
;


EXTERN Double DefaultEvenSynthesisLPDbl[]
#ifdef DEFINE_GLOBALS
= { 0.17677669529664,   0.53033008588991,  
    0.53033008588991,   0.17677669529664}
#endif
;


EXTERN FILTER DefaultEvenSynthesisFilterDbl
#ifdef DEFINE_GLOBALS
={
  DWT_EVEN_SYMMETRIC,
  DWT_DBL_TYPE,
  12,
  4,
  DefaultEvenSynthesisHPDbl,
  DefaultEvenSynthesisLPDbl,
  1
}
#endif
;


/* -------  the following are symmetric filters available from SOL -------- */


/********/
/* Haar */
/********/
EXTERN Double haar_lo[]
#ifdef DEFINE_GLOBALS
 = { .707107, .707107 }
#endif
;

EXTERN Double haar_hi[]
#ifdef DEFINE_GLOBALS
 = { -.707107, .707107 }
#endif
;

EXTERN Double haar_hi_syn[]
#ifdef DEFINE_GLOBALS
 = { .707107, -.707107 }
#endif
;

EXTERN FILTER HaarAna
#ifdef DEFINE_GLOBALS
={
  DWT_EVEN_SYMMETRIC,
  DWT_DBL_TYPE,
  2,
  2,
  haar_hi,
  haar_lo,
  1
}
#endif
;

EXTERN FILTER HaarSyn
#ifdef DEFINE_GLOBALS
={
  DWT_EVEN_SYMMETRIC,
  DWT_DBL_TYPE,
  2,
  2,
  haar_hi_syn,
  haar_lo,
  1
}
#endif
;
  

/****************************/
/* QMF9, Floating precision */
/****************************/
EXTERN Double qmf9_lo[]
#ifdef DEFINE_GLOBALS
 = { 0.028213, -0.060386, -0.073878, 0.41394, 0.798422,
     0.41394, -0.073878, -0.060386, 0.028213}
#endif
;

EXTERN Double qmf9_hi[]
#ifdef DEFINE_GLOBALS
= { 0.028213, 0.060386, -0.073878, -0.41394, 0.798422,
    -0.41394, -0.073878, 0.060386, 0.028213}
#endif
;


EXTERN FILTER qmf9Ana
#ifdef DEFINE_GLOBALS
={
  DWT_ODD_SYMMETRIC,
  DWT_DBL_TYPE,
  9,
  9,
  qmf9_hi,
  qmf9_lo,
  1
}
#endif
;

EXTERN FILTER  qmf9Syn
#ifdef DEFINE_GLOBALS
={
  DWT_ODD_SYMMETRIC,
  DWT_DBL_TYPE,
  9,
  9,
  qmf9_hi,
  qmf9_lo,
  1
}
#endif
;


/**************************/
/* QMF9, Double precision */
/**************************/
EXTERN Double qmf9a_lo[]
#ifdef DEFINE_GLOBALS
 = { 0.02821356056934, -0.06040106124895, 
     -0.07387851649837, 0.41395445184223,  
     0.79843669304460,  0.41395445184223, 
     -0.07387851649837, -0.06040106124895,
     0.02821356056934}
#endif
;

EXTERN Double qmf9a_hi[]
#ifdef DEFINE_GLOBALS
 = { 0.02821356056934,  0.06040106124895, 
     -0.07387851649837, -0.41395445184223,  
     0.79843669304460,  -0.41395445184223, 
     -0.07387851649837, 0.06040106124895,  
     0.02821356056934}
#endif
;

EXTERN FILTER qmf9aAna
#ifdef DEFINE_GLOBALS
={
  DWT_ODD_SYMMETRIC,
  DWT_DBL_TYPE,
  9,
  9,
  qmf9a_hi,
  qmf9a_lo,
  1
}
#endif
;

EXTERN FILTER  qmf9aSyn
#ifdef DEFINE_GLOBALS
={
  DWT_ODD_SYMMETRIC,
  DWT_DBL_TYPE,
  9,
  9,
  qmf9a_hi,
  qmf9a_lo,
  1
}
#endif
;




/*********/
/* fpr53 */
/*********/
EXTERN Double fpr53_lo_ana[]
#ifdef DEFINE_GLOBALS
 = {-0.1767766953,0.3535533906,1.060660172,
    0.3535533906,-0.1767766953}
#endif
;

EXTERN Double fpr53_hi_ana[]
#ifdef DEFINE_GLOBALS
 = { -0.3535533906,0.7071067812,-0.3535533906 }
#endif
;

EXTERN Double fpr53_lo_syn[]
#ifdef DEFINE_GLOBALS
 = { 0.3535533906,0.7071067812,0.3535533906 }
#endif
;

EXTERN Double fpr53_hi_syn[]
#ifdef DEFINE_GLOBALS
 = {-0.1767766953,-0.3535533906,1.060660172,
    -0.3535533906,-0.1767766953}
#endif
;

EXTERN FILTER fpr53Ana 
#ifdef DEFINE_GLOBALS
={
  DWT_ODD_SYMMETRIC,
  DWT_DBL_TYPE,
  3,
  5,
  fpr53_hi_ana,
  fpr53_lo_ana,
  1
}
#endif
;

EXTERN FILTER fpr53Syn 
#ifdef DEFINE_GLOBALS
={
  DWT_ODD_SYMMETRIC,
  DWT_DBL_TYPE,
  5,
  3,
  fpr53_hi_syn,
  fpr53_lo_syn,
  1
}
#endif
;



/**********/
/* fpr53a */
/**********/
EXTERN Double fpr53a_lo_ana[]
#ifdef DEFINE_GLOBALS
 = {-0.15713836263467, 0.31427672526933, 
    0.94283017598577, 0.31427672526933, 
    -0.15713836263467}
#endif
;

EXTERN Double fpr53a_hi_ana[]
#ifdef DEFINE_GLOBALS
 = {-0.35355339059327, 0.70710678118655, 
    -0.35355339059327}
#endif
;

EXTERN Double fpr53a_lo_syn[]
#ifdef DEFINE_GLOBALS
 ={ 0.39773864863086, 0.79547729726172, 
    0.39773864863086}
#endif
;

EXTERN Double fpr53a_hi_syn[]
#ifdef DEFINE_GLOBALS
 = {-0.17677669530336, -0.35355339060673,  
    1.06066017202018, -0.35355339060673, 
    -0.17677669530336}
#endif
;

EXTERN FILTER fpr53aAna 
#ifdef DEFINE_GLOBALS
={
  DWT_ODD_SYMMETRIC,
  DWT_DBL_TYPE,
  3,
  5,
  fpr53a_hi_ana,
  fpr53a_lo_ana,
  1
}
#endif
;

EXTERN FILTER fpr53aSyn
#ifdef DEFINE_GLOBALS
 ={
  DWT_ODD_SYMMETRIC,
  DWT_DBL_TYPE,
  5,
  3,
  fpr53a_hi_syn,
  fpr53a_lo_syn,
  1
}
#endif
;




/*********/
/* ASF93 */
/*********/
EXTERN Double asf93_lo_ana[]
#ifdef DEFINE_GLOBALS
= { 0.02828252799447, -0.05656505598894, 
    -0.15084014930386, 0.35824535459664, 
    0.84847583983416, 0.35824535459664, 
    -0.15084014930386, -0.05656505598894, 
    0.02828252799447}
#endif
;

EXTERN Double asf93_hi_ana[]
#ifdef DEFINE_GLOBALS
= {-0.41434591710792, 0.82869183421585, 
   -0.41434591710792}
#endif
;

EXTERN Double asf93_lo_syn[]
#ifdef DEFINE_GLOBALS
= { 0.41434591710792, 0.82869183421585, 
    0.41434591710792}
#endif
;

EXTERN Double asf93_hi_syn[]
#ifdef DEFINE_GLOBALS
= { 0.02828252799447, 0.05656505598894, 
    -0.15084014930386, -0.35824535459664, 
    0.84847583983416, -0.35824535459664, 
    -0.15084014930386, 0.05656505598894, 
    0.02828252799447}
#endif
;

EXTERN FILTER asd93Ana 
#ifdef DEFINE_GLOBALS
={
  DWT_ODD_SYMMETRIC,
  DWT_DBL_TYPE,
  3,
  9,
  asf93_hi_ana,
  asf93_lo_ana,
  1
}
#endif
;

EXTERN FILTER asd93Syn
#ifdef DEFINE_GLOBALS
 ={
  DWT_ODD_SYMMETRIC,
  DWT_DBL_TYPE,
  9,
  3,
  asf93_hi_syn,
  asf93_lo_syn,
  1
}
#endif
;

/*********/
/* WAV97 */
/*********/

EXTERN Double wav97_lo_ana[]
#ifdef DEFINE_GLOBALS
= { .037829, -.023849, -.110624, .377403, .852699, 
    .377403, -.110624, -.023849, .037829}   
#endif
;

EXTERN Double wav97_hi_ana[]
#ifdef DEFINE_GLOBALS
= {.064539, -.040690, -.418092, .788485, 
    -.418092, -.040690, .064539}  
#endif
;

EXTERN Double wav97_lo_syn[]
#ifdef DEFINE_GLOBALS
= {-.064539, -.040690, .418092, .788485, 
    .418092, -.040690, -.064539}
#endif
;

EXTERN Double wav97_hi_syn[]
#ifdef DEFINE_GLOBALS
= { .037829, .023849, -.110624, -.377403, .852699,
    -.377403, -.110624, .023849, .037829}
#endif
;

EXTERN FILTER wav97Ana 
#ifdef DEFINE_GLOBALS
={
  DWT_ODD_SYMMETRIC,
  DWT_DBL_TYPE,
  7,
  9,
  wav97_hi_ana,
  wav97_lo_ana,
  1
}
#endif
;

EXTERN FILTER wav97Syn
#ifdef DEFINE_GLOBALS
 ={
  DWT_ODD_SYMMETRIC,
  DWT_DBL_TYPE,
  9,
  7,
  wav97_hi_syn,
  wav97_lo_syn,
  1
}
#endif
;


#endif 

