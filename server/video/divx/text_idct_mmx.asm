;/**************************************************************************
; *                                                                        *
; * This code is developed by John Funnell.  This software is an           *
; * an implementation of a part of one or more MPEG-4 Video tools as       *
; * specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
; * software module in hardware or software products are advised that its  *
; * use may infringe existing patents or copyrights, and any such use      *
; * would be at such party's own risk.  The original developer of this     *
; * software module and his/her company, and subsequent editors and their  *
; * companies (including Project Mayo), will have no liability for use of  *
; * this software or modifications or derivatives thereof.                 *
; *                                                                        *
; * Project Mayo gives users of the Codec a license to this software       *
; * module or modifications thereof for use in hardware or software        *
; * products claiming conformance to the MPEG-4 Video Standard as          *
; * described in the Open DivX license.                                    *
; *                                                                        *
; * The complete Open DivX license can be found at                         *
; * http://www.projectmayo.com/opendivx/license.php .                      *
; *                                                                        *
; **************************************************************************/
;
;/**************************************************************************
; *
; *  text_idct_mmx.c
; *
; *  Copyright (C) 2001  Project Mayo
; *
; *  John Funnell
; *
; *  DivX Advance Research Center <darc@projectmayo.com>
; *
; **************************************************************************/
;
;/* This file contains idct functions with MMX optimization.               */
;/* Please see acknowledgement below.                                      */
;
;/*
;
; MMX iDCT
;
; Originally provided by Intel at AP-922
; http://developer.intel.com/vtune/cbts/strmsimd/922down.htm
; (See more app notes at http://developer.intel.com/vtune/cbts/strmsimd/appnotes.htm)
; but in a limited edition.
; New macro implements a column part for precise iDCT
; The routine precision now satisfies IEEE standard 1180-1990. 
;
; Copyright (c) 2000-2001 Peter Gubanov <peter@elecard.net.ru>
; Rounding trick Copyright (c) 2000 Michel Lespinasse <walken@zoy.org>
;
; http://www.elecard.com/peter/idct.html
; http://www.linuxvideo.org/mpeg2dec/
;
; Conversion to MS-style inline C format (c) 2001 Project Mayo Inc. John Funnell
;
; Version v1.0.0 10 January 2001
;
;*/

%define BITS_INV_ACC      5                             ;/* 4 or 5 for IEEE */
%define SHIFT_INV_ROW     (16 - BITS_INV_ACC)
%define SHIFT_INV_COL     (1 + BITS_INV_ACC)
%define RND_INV_ROW       (1024 * (6 - BITS_INV_ACC))   ;/* 1 << (SHIFT_INV_ROW-1) */
%define RND_INV_COL       (16 * (BITS_INV_ACC - 3))     ;/* 1 << (SHIFT_INV_COL-1) */
%define RND_INV_CORR      (RND_INV_COL - 1)             ;/* correction -1.0 and round */
%define BITS_FRW_ACC      3                             ;/* 2 or 3 for accuracy */
%define SHIFT_FRW_COL     BITS_FRW_ACC
%define SHIFT_FRW_ROW     (BITS_FRW_ACC + 17)
%define RND_FRW_ROW       (262144 * (BITS_FRW_ACC - 1)) ;/* 1 << (SHIFT_FRW_ROW-1) */

%define INP      eax
%define ROUNDER  ebx
%define TABLE    ecx
%define OUTP     edx

BITS 32

GLOBAL idct_mmx
	
SECTION .data
ALIGN 8

;static const uint64_t   tg_1_16 = SHORT4_TO_QWORD( 13036,  13036,  13036,  13036 );
tg_1_16 dw 13036, 13036, 13036, 13036

;static const uint64_t   tg_2_16 = SHORT4_TO_QWORD( 27146,  27146,  27146,  27146 );
tg_2_16 dw 27146, 27146, 27146, 27146

;static const uint64_t   tg_3_16 = SHORT4_TO_QWORD(-21746, -21746, -21746, -21746 );
tg_3_16 dw -27146, -27146, -27146, -27146

;static const uint64_t ocos_4_16 = SHORT4_TO_QWORD( 23170,  23170,  23170,  23170 );
ocos_4_16 dw 23170, 23170, 23170, 23170


;/* rounding value that is added before the row transform's rounding */
%if SHIFT_INV_ROW == 12
;static const uint64_t rounder[8] = {
;	INT2_TO_QWORD( 65536, 65536), /* rounder_0 */       
;	INT2_TO_QWORD(  7195,  7195), /* rounder_1 */ 
;	INT2_TO_QWORD(  4520,  4520), /* rounder_2 */ 
;	INT2_TO_QWORD(  2407,  2407), /* rounder_3 */ 
;	INT2_TO_QWORD(     0,     0), /* rounder_4 */ 
;	INT2_TO_QWORD(   240,   240), /* rounder_5 */
;	INT2_TO_QWORD(  1024,  1024), /* rounder_6 */
;	INT2_TO_QWORD(  1024,  1024)  /* rounder_7 */
;};
rounder
	dw 65536, 65536,
	dw 7195, 7195,
	dw 4520, 4520,
	dw 2407, 2407,
	dw 0, 0,
	dw 240, 240,
	dw 1024, 1024,
	dw 1024, 1024,
%elif SHIFT_INV_ROW == 11
;static const uint64_t rounder[2*8] = {
;	INT2_TO_QWORD( 65536, 65536), /* rounder_0 */
;	INT2_TO_QWORD(  3597,  3597), /* rounder_1 */
;	INT2_TO_QWORD(  2260,  2260), /* rounder_2 */
;	INT2_TO_QWORD(  1203,  1203), /* rounder_3 */
;	INT2_TO_QWORD( 	 0,	  0), /* rounder_4 */
;	INT2_TO_QWORD(   120,	120), /* rounder_5 */
;	INT2_TO_QWORD(   512,	512), /* rounder_6 */
;	INT2_TO_QWORD(   512,	512)  /* rounder_7 */
;};
rounder
	dw 65536, 65536,
	dw 3597, 3597,
	dw 2260, 2260,
	dw 1203, 1203,
	dw 0, 0,
	dw 120, 120,
	dw 512, 512,
	dw 512, 512,
%endif


;/*
;=============================================================================
;
; The first stage iDCT 8x8 - inverse DCTs of rows
;
;-----------------------------------------------------------------------------
; The 8-point inverse DCT direct algorithm
;-----------------------------------------------------------------------------
;
; static const short w[32] = {
; FIX(cos_4_16), FIX(cos_2_16), FIX(cos_4_16), FIX(cos_6_16),
; FIX(cos_4_16), FIX(cos_6_16), -FIX(cos_4_16), -FIX(cos_2_16),
; FIX(cos_4_16), -FIX(cos_6_16), -FIX(cos_4_16), FIX(cos_2_16),
; FIX(cos_4_16), -FIX(cos_2_16), FIX(cos_4_16), -FIX(cos_6_16),
; FIX(cos_1_16), FIX(cos_3_16), FIX(cos_5_16), FIX(cos_7_16),
; FIX(cos_3_16), -FIX(cos_7_16), -FIX(cos_1_16), -FIX(cos_5_16),
; FIX(cos_5_16), -FIX(cos_1_16), FIX(cos_7_16), FIX(cos_3_16),
; FIX(cos_7_16), -FIX(cos_5_16), FIX(cos_3_16), -FIX(cos_1_16) };
;
; #define DCT_8_INV_ROW(x, y)
;{
; int a0, a1, a2, a3, b0, b1, b2, b3;
;
; a0 =x[0]*w[0]+x[2]*w[1]+x[4]*w[2]+x[6]*w[3];
; a1 =x[0]*w[4]+x[2]*w[5]+x[4]*w[6]+x[6]*w[7];
; a2 = x[0] * w[ 8] + x[2] * w[ 9] + x[4] * w[10] + x[6] * w[11];
; a3 = x[0] * w[12] + x[2] * w[13] + x[4] * w[14] + x[6] * w[15];
; b0 = x[1] * w[16] + x[3] * w[17] + x[5] * w[18] + x[7] * w[19];
; b1 = x[1] * w[20] + x[3] * w[21] + x[5] * w[22] + x[7] * w[23];
; b2 = x[1] * w[24] + x[3] * w[25] + x[5] * w[26] + x[7] * w[27];
; b3 = x[1] * w[28] + x[3] * w[29] + x[5] * w[30] + x[7] * w[31];
;
; y[0] = SHIFT_ROUND ( a0 + b0 );
; y[1] = SHIFT_ROUND ( a1 + b1 );
; y[2] = SHIFT_ROUND ( a2 + b2 );
; y[3] = SHIFT_ROUND ( a3 + b3 );
; y[4] = SHIFT_ROUND ( a3 - b3 );
; y[5] = SHIFT_ROUND ( a2 - b2 );
; y[6] = SHIFT_ROUND ( a1 - b1 );
; y[7] = SHIFT_ROUND ( a0 - b0 );
;}
;
;-----------------------------------------------------------------------------
;
; In this implementation the outputs of the iDCT-1D are multiplied
; for rows 0,4 - by cos_4_16,
; for rows 1,7 - by cos_1_16,
; for rows 2,6 - by cos_2_16,
; for rows 3,5 - by cos_3_16
; and are shifted to the left for better accuracy
;
; For the constants used,
; FIX(float_const) = (short) (float_const * (1<<15) + 0.5)
;
;=============================================================================
;*/


;/*
;=============================================================================
; code for MMX
;=============================================================================
; Table for rows 0,4 - constants are multiplied by cos_4_16
;*/
;static const uint64_t tab_i_04[8*8] = {
tab_i_04

;/* Table for rows 0,4 - constants are multiplied by cos_4_16 */
; 	SHORT4_TO_QWORD( 16384,  16384,  16384, -16384 ),   /* movq-> w06 w04 w02 w00 */
;	SHORT4_TO_QWORD( 21407,   8867,   8867, -21407 ),   /*        w07 w05 w03 w01 */
;	SHORT4_TO_QWORD( 16384, -16384,  16384,  16384 ),   /*        w14 w12 w10 w08 */
;	SHORT4_TO_QWORD( -8867,  21407, -21407,  -8867 ),   /*        w15 w13 w11 w09 */
;	SHORT4_TO_QWORD( 22725,  12873,  19266, -22725 ),   /*        w22 w20 w18 w16 */
;	SHORT4_TO_QWORD( 19266,   4520,  -4520, -12873 ),   /*        w23 w21 w19 w17 */
;	SHORT4_TO_QWORD( 12873,   4520,   4520,  19266 ),   /*        w30 w28 w26 w24 */
;	SHORT4_TO_QWORD(-22725,  19266, -12873, -22725 ),   /*        w31 w29 w27 w25 */
	dw 16384,  16384,  16384, -16384
	dw 21407,   8867,   8867, -21407
	dw 16384, -16384,  16384,  16384
	dw -8867,  21407, -21407,  -8867
	dw 22725,  12873,  19266, -22725
	dw 19266,   4520,  -4520, -12873
	dw 12873,   4520,   4520,  19266
	dw 22725,  19266, -12873, -22725

;/* Table for rows 1,7 - constants are multiplied by cos_1_16 */
;	SHORT4_TO_QWORD( 22725,  22725,  22725, -22725 ),   /* movq-> w06 w04 w02 w00 */
;	SHORT4_TO_QWORD( 29692,  12299,  12299, -29692 ),   /*        w07 w05 w03 w01 */
;	SHORT4_TO_QWORD( 22725, -22725,  22725,  22725 ),   /*        w14 w12 w10 w08 */
;	SHORT4_TO_QWORD(-12299,  29692, -29692, -12299 ),   /*        w15 w13 w11 w09 */
;	SHORT4_TO_QWORD( 31521,  17855,  26722, -31521 ),   /*        w22 w20 w18 w16 */
;	SHORT4_TO_QWORD( 26722,   6270,  -6270, -17855 ),   /*        w23 w21 w19 w17 */
;	SHORT4_TO_QWORD( 17855,   6270,   6270,  26722 ),   /*        w30 w28 w26 w24 */
;	SHORT4_TO_QWORD(-31521,  26722, -17855, -31521 ),   /*        w31 w29 w27 w25 */
 	dw 22725,  22725,  22725, -22725 
 	dw 29692,  12299,  12299, -29692 
 	dw 22725, -22725,  22725,  22725 
 	dw -12299,  29692, -29692, -12299 
 	dw 31521,  17855,  26722, -31521 
 	dw 26722,   6270,  -6270, -17855 
 	dw 17855,   6270,   6270,  26722 
 	dw -31521,  26722, -17855, -31521 

;/* Table for rows 2,6 - constants are multiplied by cos_2_16 */
;	SHORT4_TO_QWORD( 21407,  21407,  21407, -21407 ),   /* movq-> w06 w04 w02 w00 */
;	SHORT4_TO_QWORD( 27969,  11585,  11585, -27969 ),   /*        w07 w05 w03 w01 */
;	SHORT4_TO_QWORD( 21407, -21407,  21407,  21407 ),   /*        w14 w12 w10 w08 */
;	SHORT4_TO_QWORD(-11585,  27969, -27969, -11585 ),   /*        w15 w13 w11 w09 */
;	SHORT4_TO_QWORD( 29692,  16819,  25172, -29692 ),   /*        w22 w20 w18 w16 */
;	SHORT4_TO_QWORD( 25172,   5906,  -5906, -16819 ),   /*        w23 w21 w19 w17 */
;	SHORT4_TO_QWORD( 16819,   5906,   5906,  25172 ),   /*        w30 w28 w26 w24 */
;	SHORT4_TO_QWORD(-29692,  25172, -16819, -29692 ),   /*        w31 w29 w27 w25 */
	dw  21407,  21407,  21407, -21407 
	dw  27969,  11585,  11585, -27969 
	dw  21407, -21407,  21407,  21407 
	dw -11585,  27969, -27969, -11585 
	dw  29692,  16819,  25172, -29692 
	dw  25172,   5906,  -5906, -16819 
	dw  16819,   5906,   5906,  25172 
	dw -29692,  25172, -16819, -29692 

;/* Table for rows 3,5 - constants are multiplied by cos_3_16 */
;	SHORT4_TO_QWORD( 19266,  19266,  19266, -19266 ),   /* movq-> w06 w04 w02 w00 */
;	SHORT4_TO_QWORD( 25172,  10426,  10426, -25172 ),   /*        w07 w05 w03 w01 */
;	SHORT4_TO_QWORD( 19266, -19266,  19266,  19266 ),   /*        w14 w12 w10 w08 */
;	SHORT4_TO_QWORD(-10426,  25172, -25172, -10426 ),   /*        w15 w13 w11 w09 */
;	SHORT4_TO_QWORD( 26722,  15137,  22654, -26722 ),   /*        w22 w20 w18 w16 */
;	SHORT4_TO_QWORD( 22654,   5315,  -5315, -15137 ),   /*        w23 w21 w19 w17 */
;	SHORT4_TO_QWORD( 15137,   5315,   5315,  22654 ),   /*        w30 w28 w26 w24 */
;	SHORT4_TO_QWORD(-26722,  22654, -15137, -26722 ),   /*        w31 w29 w27 w25 */
	dw  19266,  19266,  19266, -19266 
	dw  25172,  10426,  10426, -25172 
	dw  19266, -19266,  19266,  19266 
	dw -10426,  25172, -25172, -10426 
	dw  26722,  15137,  22654, -26722 
	dw  22654,   5315,  -5315, -15137 
	dw  15137,   5315,   5315,  22654 
	dw -26722,  22654, -15137, -26722 

;/* Table for rows 0,4 - constants are multiplied by cos_4_16 */
; 	SHORT4_TO_QWORD( 16384,  16384,  16384, -16384 ),   /* movq-> w06 w04 w02 w00 */
;	SHORT4_TO_QWORD( 21407,   8867,   8867, -21407 ),   /*        w07 w05 w03 w01 */
;	SHORT4_TO_QWORD( 16384, -16384,  16384,  16384 ),   /*        w14 w12 w10 w08 */
;	SHORT4_TO_QWORD(-8867,  21407, -21407,  -8867  ),   /*        w15 w13 w11 w09 */
;	SHORT4_TO_QWORD( 22725,  12873,  19266, -22725 ),   /*        w22 w20 w18 w16 */
;	SHORT4_TO_QWORD( 19266,   4520,  -4520, -12873 ),   /*        w23 w21 w19 w17 */
;	SHORT4_TO_QWORD( 12873,   4520,   4520,  19266 ),   /*        w30 w28 w26 w24 */
;	SHORT4_TO_QWORD(-22725,  19266, -12873, -22725 ),   /*        w31 w29 w27 w25 */
  	dw  16384,  16384,  16384, -16384 
 	dw  21407,   8867,   8867, -21407 
 	dw  16384, -16384,  16384,  16384 
 	dw -8867,  21407, -21407,  -8867  
 	dw  22725,  12873,  19266, -22725 
 	dw  19266,   4520,  -4520, -12873 
 	dw  12873,   4520,   4520,  19266 
 	dw -22725,  19266, -12873, -22725 

;/* Table for rows 3,5 - constants are multiplied by cos_3_16 */
;	SHORT4_TO_QWORD( 19266,  19266,  19266, -19266 ),   /* movq-> w06 w04 w02 w00 */
;	SHORT4_TO_QWORD( 25172,  10426,  10426, -25172 ),   /*        w07 w05 w03 w01 */
;	SHORT4_TO_QWORD( 19266, -19266,  19266,  19266 ),   /*        w14 w12 w10 w08 */
;	SHORT4_TO_QWORD(-10426,  25172, -25172, -10426 ),   /*        w15 w13 w11 w09 */
;	SHORT4_TO_QWORD( 26722,  15137,  22654, -26722 ),   /*        w22 w20 w18 w16 */
;	SHORT4_TO_QWORD( 22654,   5315,  -5315, -15137 ),   /*        w23 w21 w19 w17 */
;	SHORT4_TO_QWORD( 15137,   5315,   5315,  22654 ),   /*        w30 w28 w26 w24 */
;	SHORT4_TO_QWORD(-26722,  22654, -15137, -26722 ),   /*        w31 w29 w27 w25 */
 	dw  19266,  19266,  19266, -19266 
 	dw  25172,  10426,  10426, -25172 
 	dw  19266, -19266,  19266,  19266 
 	dw -10426,  25172, -25172, -10426 
 	dw  26722,  15137,  22654, -26722 
 	dw  22654,   5315,  -5315, -15137 
 	dw  15137,   5315,   5315,  22654 
 	dw -26722,  22654, -15137, -26722 

;/* Table for rows 2,6 - constants are multiplied by cos_2_16 */
;	SHORT4_TO_QWORD( 21407,  21407,  21407, -21407 ),   /* movq-> w06 w04 w02 w00 */
;	SHORT4_TO_QWORD( 27969,  11585,  11585, -27969 ),   /*        w07 w05 w03 w01 */
;	SHORT4_TO_QWORD( 21407, -21407,  21407,  21407 ),   /*        w14 w12 w10 w08 */
;	SHORT4_TO_QWORD(-11585,  27969, -27969, -11585 ),   /*        w15 w13 w11 w09 */
;	SHORT4_TO_QWORD( 29692,  16819,  25172, -29692 ),   /*        w22 w20 w18 w16 */
;	SHORT4_TO_QWORD( 25172,   5906,  -5906, -16819 ),   /*        w23 w21 w19 w17 */
;	SHORT4_TO_QWORD( 16819,   5906,   5906,  25172 ),   /*        w30 w28 w26 w24 */
;	SHORT4_TO_QWORD(-29692,  25172, -16819, -29692 ),   /*        w31 w29 w27 w25 */
 	dw  21407,  21407,  21407, -21407 
 	dw  27969,  11585,  11585, -27969 
 	dw  21407, -21407,  21407,  21407 
 	dw -11585,  27969, -27969, -11585 
 	dw  29692,  16819,  25172, -29692 
 	dw  25172,   5906,  -5906, -16819 
 	dw  16819,   5906,   5906,  25172 
 	dw -29692,  25172, -16819, -29692 

;/* Table for rows 1,7 - constants are multiplied by cos_1_16 */
;	SHORT4_TO_QWORD( 22725,  22725,  22725, -22725 ),   /* movq-> w06 w04 w02 w00 */
;	SHORT4_TO_QWORD( 29692,  12299,  12299, -29692 ),   /*        w07 w05 w03 w01 */
;	SHORT4_TO_QWORD( 22725, -22725,  22725,  22725 ),   /*        w14 w12 w10 w08 */
;	SHORT4_TO_QWORD(-12299,  29692, -29692, -12299 ),   /*        w15 w13 w11 w09 */
;	SHORT4_TO_QWORD( 31521,  17855,  26722, -31521 ),   /*        w22 w20 w18 w16 */
;	SHORT4_TO_QWORD( 26722,   6270,  -6270, -17855 ),   /*        w23 w21 w19 w17 */
;	SHORT4_TO_QWORD( 17855,   6270,   6270,  26722 ),   /*        w30 w28 w26 w24 */
;	SHORT4_TO_QWORD(-31521,  26722, -17855, -31521 ),   /*        w31 w29 w27 w25 */
 	dw  22725,  22725,  22725, -22725 
 	dw  29692,  12299,  12299, -29692 
 	dw  22725, -22725,  22725,  22725 
 	dw -12299,  29692, -29692, -12299 
 	dw  31521,  17855,  26722, -31521 
 	dw  26722,   6270,  -6270, -17855 
 	dw  17855,   6270,   6270,  26722 
 	dw -31521,  26722, -17855, -31521 

;};

;uint64_t tempMatrix[64/4];
tempMatrix times 32 dd 0 


SECTION .text

; void idct_mmx(int16_t* block)
idct_mmx:
	push ebp
	mov ebp, esp

	push eax
	push ebx 
	push ecx
	push edx 
	push edi

	mov INP, [ebp+8]
	lea OUTP,  [tempMatrix]
	lea TABLE, [tab_i_04]
	lea ROUNDER, [rounder]

	mov edi, -8;                 ; i=-8

	align 16

rowloop:

	movq mm0, [INP] 		; 0	; x3 x2 x1 x0

	movq mm1, [INP+8]		; 1	; x7 x6 x5 x4
	movq mm2, mm0 			; 2	; x3 x2 x1 x0

	movq mm3, [TABLE]		; 3	; w06 w04 w02 w00
	punpcklwd mm0, mm1 			; x5 x1 x4 x0

	movq mm5, mm0 			; 5	; x5 x1 x4 x0
	punpckldq mm0, mm0 			; x4 x0 x4 x0

	movq mm4, [TABLE+8] 	; 4	; w07 w05 w03 w01
	punpckhwd mm2, mm1		; 1	; x7 x3 x6 x2

	pmaddwd mm3, mm0 			; x4*w06+x0*w04 x4*w02+x0*w00
	movq mm6, mm2 			; 6 	; x7 x3 x6 x2

	movq mm1, [TABLE+32] 	; 1 	; w22 w20 w18 w16
	punpckldq mm2, mm2 			; x6 x2 x6 x2

	pmaddwd mm4, mm2 			; x6*w07+x2*w05 x6*w03+x2*w01
	punpckhdq mm5, mm5 			; x5 x1 x5 x1

	pmaddwd mm0, [TABLE+16] 		; x4*w14+x0*w12 x4*w10+x0*w08
	punpckhdq mm6, mm6 			; x7 x3 x7 x3

	movq mm7, [TABLE+40] 	; 7 	; w23 w21 w19 w17
	pmaddwd mm1, mm5 			; x5*w22+x1*w20 x5*w18+x1*w16

	paddd mm3, [ROUNDER] 		; +rounder
	pmaddwd mm7, mm6 			; x7*w23+x3*w21 x7*w19+x3*w17

	pmaddwd mm2, [TABLE+24] 		; x6*w15+x2*w13 x6*w11+x2*w09
	paddd mm3, mm4 			; 4 	; a1=sum(even1) a0=sum(even0)

	pmaddwd mm5, [TABLE+48] 		; x5*w30+x1*w28 x5*w26+x1*w24
	movq mm4, mm3 			; 4 	; a1 a0

	pmaddwd mm6, [TABLE+56] 		; x7*w31+x3*w29 x7*w27+x3*w25
	paddd mm1, mm7 			; 7 	; b1=sum(odd1) b0=sum(odd0)

	paddd mm0, [ROUNDER]		; +rounder
	psubd mm3, mm1 				; a1-b1 a0-b0

	psrad mm3, SHIFT_INV_ROW 		; y6=a1-b1 y7=a0-b0
	paddd mm1, mm4 			; 4 	; a1+b1 a0+b0

	paddd mm0, mm2 			; 2 	; a3=sum(even3) a2=sum(even2)
	psrad mm1, SHIFT_INV_ROW 		; y1=a1+b1 y0=a0+b0

	paddd mm5, mm6 			; 6 	; b3=sum(odd3) b2=sum(odd2)
	movq mm4, mm0 			; 4 	; a3 a2

	paddd mm0, mm5 				; a3+b3 a2+b2
	psubd mm4, mm5 			; 5 	; a3-b3 a2-b2

	psrad mm0, SHIFT_INV_ROW 		; y3=a3+b3 y2=a2+b2
	psrad mm4, SHIFT_INV_ROW 		; y4=a3-b3 y5=a2-b2

	packssdw mm1, mm0 		; 0 	; y3 y2 y1 y0
	packssdw mm4, mm3 		; 3 	; y6 y7 y4 y5

	movq mm7, mm4 			; 7 	; y6 y7 y4 y5
	psrld mm4, 16 				; 0 y6 0 y4

	pslld mm7, 16 				; y7 0 y5 0
	movq [OUTP], mm1 		; 1 	; save y3 y2 y1 y0
                             	
	por mm7, mm4 			; 4 	; y7 y6 y5 y4
	movq [OUTP+8], mm7 		; 7 	; save y7 y6 y5 y4


	add INP,  16                    ; add 1 row to input pointer
	add ROUNDER, 8                  ; go to next rounding values
	add OUTP, 16                    ; add 1 row to output pointer
	add TABLE,64                    ; move to next section of table
		
	add edi, 1
	jne near rowloop;
	
	
;/* column code starts here... */

	lea INP,   [tempMatrix] 
	mov OUTP,  [ebp+8]
	mov edi, -2;                 ; i=-2

	align 16
colloop:

	movq	mm0, [tg_3_16]

	movq	mm3, [INP+16*3]
	movq	mm1, mm0			; tg_3_16

	movq	mm5, [INP+16*5]
	pmulhw	mm0, mm3			; x3*(tg_3_16-1)

	movq	mm4, [tg_1_16]
	pmulhw	mm1, mm5			; x5*(tg_3_16-1)

	movq	mm7, [INP+16*7]
	movq	mm2, mm4			; tg_1_16

	movq	mm6, [INP+16*1]
	pmulhw	mm4, mm7			; x7*tg_1_16

	paddsw	mm0, mm3			; x3*tg_3_16
	pmulhw	mm2, mm6			; x1*tg_1_16

	paddsw	mm1, mm3			; x3+x5*(tg_3_16-1)
	psubsw	mm0, mm5			; x3*tg_3_16-x5 = tm35

	movq	mm3, [ocos_4_16]
	paddsw	mm1, mm5			; x3+x5*tg_3_16 = tp35

	paddsw	mm4, mm6			; x1+tg_1_16*x7 = tp17
	psubsw	mm2, mm7			; x1*tg_1_16-x7 = tm17

	movq	mm5, mm4			; tp17
	movq	mm6, mm2			; tm17

	paddsw	mm5, mm1			; tp17+tp35 = b0
	psubsw	mm6, mm0			; tm17-tm35 = b3

	psubsw	mm4, mm1			; tp17-tp35 = t1
	paddsw	mm2, mm0			; tm17+tm35 = t2

	movq	mm7, [tg_2_16]
	movq	mm1, mm4			; t1

;	movq	[SCRATCH+0], mm5	; save b0
	movq	[OUTP+3*16], mm5	; save b0
	paddsw	mm1, mm2			; t1+t2

;	movq	[SCRATCH+8], mm6	; save b3
	movq	[OUTP+5*16], mm6	; save b3
	psubsw	mm4, mm2			; t1-t2

	movq	mm5, [INP+2*16]
	movq	mm0, mm7			; tg_2_16

	movq	mm6, [INP+6*16]
	pmulhw	mm0, mm5			; x2*tg_2_16

	pmulhw	mm7, mm6			; x6*tg_2_16
; slot
	pmulhw	mm1, mm3			; ocos_4_16*(t1+t2) = b1/2
; slot
	movq	mm2, [INP+0*16]
	pmulhw	mm4, mm3			; ocos_4_16*(t1-t2) = b2/2

	psubsw	mm0, mm6			; t2*tg_2_16-x6 = tm26
	movq	mm3, mm2			; x0

	movq	mm6, [INP+4*16]
	paddsw	mm7, mm5			; x2+x6*tg_2_16 = tp26

	paddsw	mm2, mm6			; x0+x4 = tp04
	psubsw	mm3, mm6			; x0-x4 = tm04

	movq	mm5, mm2			; tp04
	movq	mm6, mm3			; tm04

	psubsw	mm2, mm7			; tp04-tp26 = a3
	paddsw	mm3, mm0			; tm04+tm26 = a1

	paddsw mm1, mm1				; b1
	paddsw mm4, mm4				; b2

	paddsw	mm5, mm7			; tp04+tp26 = a0
	psubsw	mm6, mm0			; tm04-tm26 = a2

	movq	mm7, mm3			; a1
	movq	mm0, mm6			; a2

	paddsw	mm3, mm1			; a1+b1
	paddsw	mm6, mm4			; a2+b2

	psraw	mm3, SHIFT_INV_COL		; dst1
	psubsw	mm7, mm1			; a1-b1

	psraw	mm6, SHIFT_INV_COL		; dst2
	psubsw	mm0, mm4			; a2-b2

;	movq	mm1, [SCRATCH+0]	; load b0
	movq	mm1, [OUTP+3*16]	; load b0
	psraw	mm7, SHIFT_INV_COL		; dst6

	movq	mm4, mm5			; a0
	psraw	mm0, SHIFT_INV_COL		; dst5

	movq	[OUTP+1*16], mm3
	paddsw	mm5, mm1			; a0+b0

	movq	[OUTP+2*16], mm6
	psubsw	mm4, mm1			; a0-b0

;	movq	mm3, [SCRATCH+8]	; load b3
	movq	mm3, [OUTP+5*16]	; load b3
	psraw	mm5, SHIFT_INV_COL		; dst0

	movq	mm6, mm2			; a3
	psraw	mm4, SHIFT_INV_COL		; dst7

	movq	[OUTP+5*16], mm0
	paddsw	mm2, mm3			; a3+b3

	movq	[OUTP+6*16], mm7
	psubsw	mm6, mm3			; a3-b3

	movq	[OUTP+0*16], mm5
	psraw	mm2, SHIFT_INV_COL		; dst3

	movq	[OUTP+7*16], mm4
	psraw	mm6, SHIFT_INV_COL		; dst4

	movq	[OUTP+3*16], mm2

	movq	[OUTP+4*16], mm6

	lea INP,  [tempMatrix] 
	mov OUTP, [ebp+8]
	add INP,  8                    ; add 4 cols to input pointer
	add OUTP, 8                    ; add 4 cols to output pointer
		
	add edi, 1
	jne near colloop

	pop edi
	pop edx 
	pop ecx
	pop ebx
	pop eax
	
	emms 
	pop ebp
	ret

