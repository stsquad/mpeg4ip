;/******************************************************************************
; *                                                                            *
; *  This file is part of XviD, a free MPEG-4 video encoder/decoder            *
; *                                                                            *
; *  XviD is an implementation of a part of one or more MPEG-4 Video tools     *
; *  as specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
; *  software module in hardware or software products are advised that its     *
; *  use may infringe existing patents or copyrights, and any such use         *
; *  would be at such party's own risk.  The original developer of this        *
; *  software module and his/her company, and subsequent editors and their     *
; *  companies, will have no liability for use of this software or             *
; *  modifications or derivatives thereof.                                     *
; *                                                                            *
; *  XviD is free software; you can redistribute it and/or modify it           *
; *  under the terms of the GNU General Public License as published by         *
; *  the Free Software Foundation; either version 2 of the License, or         *
; *  (at your option) any later version.                                       *
; *                                                                            *
; *  XviD is distributed in the hope that it will be useful, but               *
; *  WITHOUT ANY WARRANTY; without even the implied warranty of                *
; *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
; *  GNU General Public License for more details.                              *
; *                                                                            *
; *  You should have received a copy of the GNU General Public License         *
; *  along with this program; if not, write to the Free Software               *
; *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA  *
; *                                                                            *
; ******************************************************************************/
;
;/******************************************************************************
; *                                                                            *
; *  fdct_mmx.asm, MMX optimized forward DCT                                   *
; *                                                                            *
; *  Initial, but incomplete version provided by Intel at AppNote AP-922       *
; *  http://developer.intel.com/vtune/cbts/strmsimd/922down.htm                *
; *  Copyright (C) 1999 Intel Corporation,                                     *
; *                                                                            *
; *  completed and corrected in fdctmm32.c/fdctmm32.doc,                       *
; *  http://members.tripod.com/~liaor                                          *
; *  Copyright (C) 2000 - Royce Shih-Wea Liao <liaor@iname.com>,               *
; *                                                                            *
; *  ported to NASM and some minor changes                                     * 
; *  Copyright (C) 2001 - Michael Militzer <isibaar@xvid.org>                  *
; *                                                                            *
; *  For more information visit the XviD homepage: http://www.xvid.org         *
; *                                                                            *
; ******************************************************************************/
;
;/******************************************************************************
; *                                                                            *
; *  Revision history:                                                         *
; *                                                                            *
; *  04.11.2001 loop unrolled (Isibaar) 									   *
; *  02.11.2001 initial version  (Isibaar)                                     *
; *                                                                            *
; ******************************************************************************/


BITS 32

%macro cglobal 1 
	%ifdef PREFIX
		global _%1 
		%define %1 _%1
	%else
		global %1
	%endif
%endmacro

%define INP eax
%define TABLE ebx
%define TABLEF ebx
%define OUT ecx
%define round_frw_row edx

%define INP_1 eax + 16
%define INP_2 eax + 32
%define INP_3 eax + 48
%define INP_4 eax + 64
%define INP_5 eax + 80
%define INP_6 eax + 96
%define INP_7 eax + 112

%define OUT_1 ecx + 16
%define OUT_2 ecx + 32
%define OUT_3 ecx + 48
%define OUT_4 ecx + 64
%define OUT_5 ecx + 80
%define OUT_6 ecx + 96
%define OUT_7 ecx + 112
%define OUT_8 ecx + 128

%define TABLE_1 ebx + 64
%define TABLE_2 ebx + 128
%define TABLE_3 ebx + 192
%define TABLE_4 ebx + 256
%define TABLE_5 ebx + 320
%define TABLE_6 ebx + 384
%define TABLE_7 ebx + 448

%define x0 INP + 0*16
%define x1 INP + 1*16
%define x2 INP + 2*16
%define x3 INP + 3*16
%define x4 INP + 4*16
%define x5 INP + 5*16
%define x6 INP + 6*16
%define x7 INP + 7*16
%define y0 OUT + 0*16
%define y1 OUT + 1*16
%define y2 OUT + 2*16
%define y3 OUT + 3*16
%define y4 OUT + 4*16
%define y5 OUT + 5*16
%define y6 OUT + 6*16
%define y7 OUT + 7*16

%define tg_1_16 (TABLEF + 0)
%define tg_2_16 (TABLEF + 8)
%define tg_3_16 (TABLEF + 16)
%define cos_4_16 (TABLEF + 24)
%define ocos_4_16 (TABLEF + 32)


SECTION .data

ALIGN 16

BITS_FRW_ACC equ 3							; 2 or 3 for accuracy
SHIFT_FRW_COL equ BITS_FRW_ACC
SHIFT_FRW_ROW equ (BITS_FRW_ACC + 17)
RND_FRW_ROW equ (1 << (SHIFT_FRW_ROW-1))

SHIFT_FRW_ROW_CLIP2	equ (4)
SHIFT_FRW_ROW_CLIP1	equ (SHIFT_FRW_ROW - SHIFT_FRW_ROW_CLIP2)

one_corr		 dw 1, 1, 1, 1


r_frw_row		 dd RND_FRW_ROW, RND_FRW_ROW


tg_all_16		 dw 13036, 13036, 13036, 13036,		; tg * (2<<16) + 0.5
				 dw 27146, 27146, 27146, 27146,		; tg * (2<<16) + 0.5
				 dw -21746, -21746, -21746, -21746,	; tg * (2<<16) + 0.5
				 dw -19195, -19195, -19195, -19195,	; cos * (2<<16) + 0.5
				 dw 23170, 23170, 23170, 23170      ; cos * (2<<15) + 0.5


tab_frw_01234567 
				 ; row0
				 dw 16384, 16384, 21407, -8867,     ; w09 w01 w08 w00
				 dw 16384, 16384, 8867, -21407,     ; w13 w05 w12 w04
                 dw 16384, -16384, 8867, 21407,     ; w11 w03 w10 w02
                 dw -16384, 16384, -21407, -8867,   ; w15 w07 w14 w06
                 dw 22725, 12873, 19266, -22725,    ; w22 w20 w18 w16
                 dw 19266, 4520, -4520, -12873,     ; w23 w21 w19 w17
                 dw 12873, 4520, 4520, 19266,       ; w30 w28 w26 w24
                 dw -22725, 19266, -12873, -22725,  ; w31 w29 w27 w25

				 ; row1
                 dw 22725, 22725, 29692, -12299,    ; w09 w01 w08 w00
                 dw 22725, 22725, 12299, -29692,    ; w13 w05 w12 w04
                 dw 22725, -22725, 12299, 29692,    ; w11 w03 w10 w02
                 dw -22725, 22725, -29692, -12299,  ; w15 w07 w14 w06
                 dw 31521, 17855, 26722, -31521,    ; w22 w20 w18 w16
                 dw 26722, 6270, -6270, -17855,     ; w23 w21 w19 w17
                 dw 17855, 6270, 6270, 26722,       ; w30 w28 w26 w24
                 dw -31521, 26722, -17855, -31521,  ; w31 w29 w27 w25

				 ; row2
                 dw 21407, 21407, 27969, -11585,    ; w09 w01 w08 w00
                 dw 21407, 21407, 11585, -27969,    ; w13 w05 w12 w04
                 dw 21407, -21407, 11585, 27969,    ; w11 w03 w10 w02
                 dw -21407, 21407, -27969, -11585,	; w15 w07 w14 w06
                 dw 29692, 16819, 25172, -29692,    ; w22 w20 w18 w16
                 dw 25172, 5906, -5906, -16819,     ; w23 w21 w19 w17
                 dw 16819, 5906, 5906, 25172,       ; w30 w28 w26 w24
                 dw -29692, 25172, -16819, -29692,  ; w31 w29 w27 w25

				 ; row3
                 dw 19266, 19266, 25172, -10426,    ; w09 w01 w08 w00
                 dw 19266, 19266, 10426, -25172,    ; w13 w05 w12 w04
                 dw 19266, -19266, 10426, 25172,    ; w11 w03 w10 w02
                 dw -19266, 19266, -25172, -10426,  ; w15 w07 w14 w06 
                 dw 26722, 15137, 22654, -26722,    ; w22 w20 w18 w16
                 dw 22654, 5315, -5315, -15137,     ; w23 w21 w19 w17
                 dw 15137, 5315, 5315, 22654,       ; w30 w28 w26 w24
                 dw -26722, 22654, -15137, -26722,  ; w31 w29 w27 w25

				 ; row4
                 dw 16384, 16384, 21407, -8867,     ; w09 w01 w08 w00
                 dw 16384, 16384, 8867, -21407,     ; w13 w05 w12 w04
                 dw 16384, -16384, 8867, 21407,     ; w11 w03 w10 w02
                 dw -16384, 16384, -21407, -8867,   ; w15 w07 w14 w06
                 dw 22725, 12873, 19266, -22725,    ; w22 w20 w18 w16
                 dw 19266, 4520, -4520, -12873,     ; w23 w21 w19 w17
                 dw 12873, 4520, 4520, 19266,       ; w30 w28 w26 w24
                 dw -22725, 19266, -12873, -22725,  ; w31 w29 w27 w25 

				 ; row5
                 dw 19266, 19266, 25172, -10426,    ; w09 w01 w08 w00
                 dw 19266, 19266, 10426, -25172,    ; w13 w05 w12 w04
                 dw 19266, -19266, 10426, 25172,    ; w11 w03 w10 w02
                 dw -19266, 19266, -25172, -10426,  ; w15 w07 w14 w06
                 dw 26722, 15137, 22654, -26722,    ; w22 w20 w18 w16
                 dw 22654, 5315, -5315, -15137,     ; w23 w21 w19 w17
                 dw 15137, 5315, 5315, 22654,       ; w30 w28 w26 w24
                 dw -26722, 22654, -15137, -26722,  ; w31 w29 w27 w25

				 ; row6
                 dw 21407, 21407, 27969, -11585,    ; w09 w01 w08 w00
                 dw 21407, 21407, 11585, -27969,    ; w13 w05 w12 w04
                 dw 21407, -21407, 11585, 27969,    ; w11 w03 w10 w02
                 dw -21407, 21407, -27969, -11585,  ; w15 w07 w14 w06
                 dw 29692, 16819, 25172, -29692,    ; w22 w20 w18 w16
                 dw 25172, 5906, -5906, -16819,     ; w23 w21 w19 w17
                 dw 16819, 5906, 5906, 25172,       ; w30 w28 w26 w24
                 dw -29692, 25172, -16819, -29692,  ; w31 w29 w27 w25

				 ; row7
                 dw 22725, 22725, 29692, -12299,    ; w09 w01 w08 w00
                 dw 22725, 22725, 12299, -29692,    ; w13 w05 w12 w04
                 dw 22725, -22725, 12299, 29692,    ; w11 w03 w10 w02
                 dw -22725, 22725, -29692, -12299,  ; w15 w07 w14 w06
                 dw 31521, 17855, 26722, -31521,    ; w22 w20 w18 w16
                 dw 26722, 6270, -6270, -17855,     ; w23 w21 w19 w17
                 dw 17855, 6270, 6270, 26722,       ; w30 w28 w26 w24
                 dw -31521, 26722, -17855, -31521   ; w31 w29 w27 w25


SECTION .text

ALIGN 16

cglobal fdct_mmx
;;void fdct_mmx(short *block);
fdct_mmx:
	
	push ebx

    mov INP, dword [esp + 8]	; block

    mov TABLEF, tg_all_16
    mov OUT, INP

    movq mm0, [x1]				; 0 ; x1

    movq mm1, [x6]				; 1 ; x6
    movq mm2, mm0				; 2 ; x1

    movq mm3, [x2]				; 3 ; x2
    paddsw mm0, mm1				; t1 = x[1] + x[6]

    movq mm4, [x5]				; 4 ; x5
    psllw mm0, SHIFT_FRW_COL	; t1

    movq mm5, [x0]				; 5 ; x0
    paddsw mm4, mm3				; t2 = x[2] + x[5]

    paddsw mm5, [x7]			; t0 = x[0] + x[7]
    psllw mm4, SHIFT_FRW_COL	; t2

    movq mm6, mm0				; 6 ; t1
    psubsw mm2, mm1				; 1 ; t6 = x[1] - x[6]

    movq mm1, [tg_2_16]			; 1 ; tg_2_16
    psubsw mm0, mm4				; tm12 = t1 - t2

    movq mm7, [x3]				; 7 ; x3
    pmulhw mm1, mm0				; tm12*tg_2_16

    paddsw mm7, [x4]			; t3 = x[3] + x[4]
    psllw mm5, SHIFT_FRW_COL	; t0

    paddsw mm6, mm4				; 4 ; tp12 = t1 + t2
    psllw mm7, SHIFT_FRW_COL	; t3

    movq mm4, mm5				; 4 ; t0
    psubsw mm5, mm7				; tm03 = t0 - t3

    paddsw mm1, mm5				; y2 = tm03 + tm12*tg_2_16
    paddsw mm4, mm7				; 7 ; tp03 = t0 + t3

    por mm1, qword [one_corr]	; correction y2 +0.5
    psllw mm2, SHIFT_FRW_COL+1	; t6

    pmulhw mm5, [tg_2_16]		; tm03*tg_2_16
    movq mm7, mm4				; 7 ; tp03

    psubsw mm3, [x5]			; t5 = x[2] - x[5]
    psubsw mm4, mm6				; y4 = tp03 - tp12

    movq [y2], mm1				; 1 ; save y2
    paddsw mm7, mm6				; 6 ; y0 = tp03 + tp12
     
    movq mm1, [x3]				; 1 ; x3
    psllw mm3, SHIFT_FRW_COL+1	; t5

    psubsw mm1, [x4]			; t4 = x[3] - x[4]
    movq mm6, mm2				; 6 ; t6
    
    movq [y4], mm4				; 4 ; save y4
    paddsw mm2, mm3				; t6 + t5

    pmulhw mm2, [ocos_4_16]		; tp65 = (t6 + t5)*cos_4_16
    psubsw mm6, mm3				; 3 ; t6 - t5

    pmulhw mm6, [ocos_4_16]		; tm65 = (t6 - t5)*cos_4_16
    psubsw mm5, mm0				; 0 ; y6 = tm03*tg_2_16 - tm12

    por mm5, qword [one_corr]	; correction y6 +0.5
    psllw mm1, SHIFT_FRW_COL	; t4

    por mm2, qword [one_corr]	; correction tp65 +0.5
    movq mm4, mm1				; 4 ; t4

    movq mm3, [x0]				; 3 ; x0
    paddsw mm1, mm6				; tp465 = t4 + tm65

    psubsw mm3, [x7]			; t7 = x[0] - x[7]
    psubsw mm4, mm6				; 6 ; tm465 = t4 - tm65

    movq mm0, [tg_1_16]			; 0 ; tg_1_16
    psllw mm3, SHIFT_FRW_COL	; t7

    movq mm6, [tg_3_16]			; 6 ; tg_3_16
    pmulhw mm0, mm1				; tp465*tg_1_16

    movq [y0], mm7				; 7 ; save y0
    pmulhw mm6, mm4				; tm465*tg_3_16

    movq [y6], mm5				; 5 ; save y6
    movq mm7, mm3				; 7 ; t7

    movq mm5, [tg_3_16]			; 5 ; tg_3_16
    psubsw mm7, mm2				; tm765 = t7 - tp65

    paddsw mm3, mm2				; 2 ; tp765 = t7 + tp65
    pmulhw mm5, mm7				; tm765*tg_3_16

    paddsw mm0, mm3				; y1 = tp765 + tp465*tg_1_16
    paddsw mm6, mm4				; tm465*tg_3_16

    pmulhw mm3, [tg_1_16]		; tp765*tg_1_16
 	
    por mm0, qword [one_corr]	; correction y1 +0.5
    paddsw mm5, mm7				; tm765*tg_3_16

    psubsw mm7, mm6				; 6 ; y3 = tm765 - tm465*tg_3_16
    add INP, 0x08

    movq [y1], mm0				; 0 ; save y1
    paddsw mm5, mm4				; 4 ; y5 = tm765*tg_3_16 + tm465

    movq [y3], mm7				; 7 ; save y3
    psubsw mm3, mm1				; 1 ; y7 = tp765*tg_1_16 - tp465

    movq [y5], mm5				; 5 ; save y5

    movq mm0, [x1]				; 0 ; x1

    movq [y7], mm3				; 3 ; save y7 (columns 0-4)

    movq mm1, [x6]				; 1 ; x6
    movq mm2, mm0				; 2 ; x1

    movq mm3, [x2]				; 3 ; x2
    paddsw mm0, mm1				; t1 = x[1] + x[6]

    movq mm4, [x5]				; 4 ; x5
    psllw mm0, SHIFT_FRW_COL	; t1

    movq mm5, [x0]				; 5 ; x0
    paddsw mm4, mm3				; t2 = x[2] + x[5]

    paddsw mm5, [x7]			; t0 = x[0] + x[7]
    psllw mm4, SHIFT_FRW_COL	; t2
	
    movq mm6, mm0				; 6 ; t1
    psubsw mm2, mm1				; 1 ; t6 = x[1] - x[6]

    movq mm1, [tg_2_16]			; 1 ; tg_2_16
    psubsw mm0, mm4				; tm12 = t1 - t2

    movq mm7, [x3]				; 7 ; x3
    pmulhw mm1, mm0				; tm12*tg_2_16

    paddsw mm7, [x4]			; t3 = x[3] + x[4]
    psllw mm5, SHIFT_FRW_COL	; t0

    paddsw mm6, mm4				; 4 ; tp12 = t1 + t2
    psllw mm7, SHIFT_FRW_COL	; t3

    movq mm4, mm5				; 4 ; t0
    psubsw mm5, mm7				; tm03 = t0 - t3

    paddsw mm1, mm5				; y2 = tm03 + tm12*tg_2_16
    paddsw mm4, mm7				; 7 ; tp03 = t0 + t3

    por mm1, qword [one_corr]	; correction y2 +0.5
    psllw mm2, SHIFT_FRW_COL+1	; t6

    pmulhw mm5, [tg_2_16]		; tm03*tg_2_16
    movq mm7, mm4				; 7 ; tp03

    psubsw mm3, [x5]			; t5 = x[2] - x[5]
    psubsw mm4, mm6				; y4 = tp03 - tp12

    movq [y2+8], mm1			; 1 ; save y2
    paddsw mm7, mm6				; 6 ; y0 = tp03 + tp12
     
    movq mm1, [x3]				; 1 ; x3
    psllw mm3, SHIFT_FRW_COL+1 ; t5

    psubsw mm1, [x4]			; t4 = x[3] - x[4]
    movq mm6, mm2				; 6 ; t6
    
    movq [y4+8], mm4			; 4 ; save y4
    paddsw mm2, mm3				; t6 + t5

    pmulhw mm2, [ocos_4_16]		; tp65 = (t6 + t5)*cos_4_16
    psubsw mm6, mm3				; 3 ; t6 - t5

    pmulhw mm6, [ocos_4_16]		; tm65 = (t6 - t5)*cos_4_16
    psubsw mm5, mm0				; 0 ; y6 = tm03*tg_2_16 - tm12

    por mm5, qword [one_corr]	; correction y6 +0.5
    psllw mm1, SHIFT_FRW_COL	; t4

    por mm2, qword [one_corr]	; correction tp65 +0.5
    movq mm4, mm1				; 4 ; t4

    movq mm3, [x0]				; 3 ; x0
    paddsw mm1, mm6				; tp465 = t4 + tm65

    psubsw mm3, [x7]			; t7 = x[0] - x[7]
    psubsw mm4, mm6				; 6 ; tm465 = t4 - tm65

    movq mm0, [tg_1_16]			; 0 ; tg_1_16
    psllw mm3, SHIFT_FRW_COL	; t7

    movq mm6, [tg_3_16]			; 6 ; tg_3_16
    pmulhw mm0, mm1				; tp465*tg_1_16

    movq [y0+8], mm7			; 7 ; save y0
    pmulhw mm6, mm4				; tm465*tg_3_16

    movq [y6+8], mm5			; 5 ; save y6
    movq mm7, mm3				; 7 ; t7

    movq mm5, [tg_3_16]			; 5 ; tg_3_16
    psubsw mm7, mm2				; tm765 = t7 - tp65

    paddsw mm3, mm2				; 2 ; tp765 = t7 + tp65
    pmulhw mm5, mm7				; tm765*tg_3_16

    paddsw mm0, mm3				; y1 = tp765 + tp465*tg_1_16
    paddsw mm6, mm4				; tm465*tg_3_16

    pmulhw mm3, [tg_1_16]		; tp765*tg_1_16
    
	por mm0, qword [one_corr]	; correction y1 +0.5
    paddsw mm5, mm7				; tm765*tg_3_16
	
    psubsw mm7, mm6				; 6 ; y3 = tm765 - tm465*tg_3_16
 
    movq [y1+8], mm0			; 0 ; save y1
    paddsw mm5, mm4				; 4 ; y5 = tm765*tg_3_16 + tm465

    movq [y3+8], mm7			; 7 ; save y3
    psubsw mm3, mm1				; 1 ; y7 = tp765*tg_1_16 - tp465

    movq [y5+8], mm5			; 5 ; save y5

    movq [y7+8], mm3			; 3 ; save y7
	
	mov INP, [esp + 8]			; row 0

	mov TABLEF, tab_frw_01234567	; row 0
	mov OUT, INP

	mov round_frw_row, r_frw_row

    movd mm5, [INP+12]			; mm5 = 7 6

    punpcklwd mm5, [INP+8]

    movq mm2, mm5				; mm2 = 5 7 4 6
    psrlq mm5, 32				; mm5 = _ _ 5 7

    movq mm0, [INP]				; mm0 = 3 2 1 0
    punpcklwd mm5, mm2			; mm5 = 4 5 6 7

    movq mm1, mm0				; mm1 = 3 2 1 0
    paddsw mm0, mm5				; mm0 = [3+4, 2+5, 1+6, 0+7] (xt3, xt2, xt1, xt0)

    psubsw mm1, mm5				; mm1 = [3-4, 2-5, 1-6, 0-7] (xt7, xt6, xt5, xt4)
    movq mm2, mm0				; mm2 = [ xt3 xt2 xt1 xt0 ]

    punpcklwd mm0, mm1			; mm0 = [ xt5 xt1 xt4 xt0 ]

    punpckhwd mm2, mm1			; mm2 = [ xt7 xt3 xt6 xt2 ]
    movq mm1, mm2				; mm1

    movq mm2, mm0				; 2 ; x3 x2 x1 x0

    movq mm3, [TABLE]				; 3 ; w06 w04 w02 w00
    punpcklwd mm0, mm1			; x5 x1 x4 x0

    movq mm5, mm0				; 5 ; x5 x1 x4 x0
    punpckldq mm0, mm0			; x4 x0 x4 x0  [ xt2 xt0 xt2 xt0 ]

    movq mm4, [TABLE+8]			; 4 ; w07 w05 w03 w01
    punpckhwd mm2, mm1			; 1 ; x7 x3 x6 x2

    pmaddwd mm3, mm0			; x4*w06+x0*w04 x4*w02+x0*w00
    movq mm6, mm2				; 6 ; x7 x3 x6 x2

    movq mm1, [TABLE+32]			; 1 ; w22 w20 w18 w16
    punpckldq mm2, mm2			; x6 x2 x6 x2  [ xt3 xt1 xt3 xt1 ]
	
    pmaddwd mm4, mm2			; x6*w07+x2*w05 x6*w03+x2*w01
    punpckhdq mm5, mm5			; x5 x1 x5 x1  [ xt6 xt4 xt6 xt4 ]

    pmaddwd mm0, [TABLE+16]		; x4*w14+x0*w12 x4*w10+x0*w08
    punpckhdq mm6, mm6			; x7 x3 x7 x3  [ xt7 xt5 xt7 xt5 ]

    movq mm7, [TABLE+40]			; 7 ; w23 w21 w19 w17
    pmaddwd mm1, mm5			; x5*w22+x1*w20 x5*w18+x1*w16

    paddd mm3, [round_frw_row]			; +rounder (y2,y0)
    pmaddwd mm7, mm6			; x7*w23+x3*w21 x7*w19+x3*w17

    pmaddwd mm2, [TABLE+24]		; x6*w15+x2*w13 x6*w11+x2*w09
    paddd mm3, mm4				; 4 ; a1=sum(even1) a0=sum(even0)

    pmaddwd mm5, [TABLE+48]		; x5*w30+x1*w28 x5*w26+x1*w24
 
    pmaddwd mm6, [TABLE+56]		; x7*w31+x3*w29 x7*w27+x3*w25
    paddd mm1, mm7				; 7 ; b1=sum(odd1) b0=sum(odd0)

    paddd mm0, [round_frw_row]			; +rounder (y6,y4)
    psrad mm3, SHIFT_FRW_ROW_CLIP1 ; (y2, y0) 

    paddd mm1, [round_frw_row]			; +rounder (y3,y1)
    paddd mm0, mm2				; 2 ; a3=sum(even3) a2=sum(even2)

    paddd mm5, [round_frw_row]			; +rounder (y7,y5)
    psrad mm1, SHIFT_FRW_ROW_CLIP1 ; y1=a1+b1 y0=a0+b0

    paddd mm5, mm6				; 6 ; b3=sum(odd3) b2=sum(odd2)
    psrad mm0, SHIFT_FRW_ROW_CLIP1 ; y3=a3+b3 y2=a2+b2

    psrad mm5, SHIFT_FRW_ROW_CLIP1 ; y4=a3-b3 y5=a2-b2

	packssdw mm3, mm0			; 0 ; y6 y4 y2 y0, saturate {-32768,+32767}

    packssdw mm1, mm5			; 3 ; y7 y5 y3 y1, saturate {-32768,+32767}
    movq mm6, mm3				; mm0 = y6 y4 y2 y0

    punpcklwd mm3, mm1			; y3 y2 y1 y0
    
    punpckhwd mm6, mm1			; y7 y6 y5 y4

    psraw mm3, SHIFT_FRW_ROW_CLIP2 ; descale [y3 y2 y1 y0] to {-2048,+2047}

    psraw mm6, SHIFT_FRW_ROW_CLIP2 ; descale [y7 y6 y5 y4] to {-2048,+2047}

    movq [OUT_1-16], mm3			; 1 ; save y3 y2 y1 y0

    movq [OUT_1-8], mm6			; 7 ; save y7 y6 y5 y4

    movd mm5, [INP_1+12]			; mm5 = 7 6

    punpcklwd mm5, [INP_1+8]

    movq mm2, mm5				; mm2 = 5 7 4 6
    psrlq mm5, 32				; mm5 = _ _ 5 7

    movq mm0, [INP_1]				; mm0 = 3 2 1 0
    punpcklwd mm5, mm2			; mm5 = 4 5 6 7

    movq mm1, mm0				; mm1 = 3 2 1 0
    paddsw mm0, mm5				; mm0 = [3+4, 2+5, 1+6, 0+7] (xt3, xt2, xt1, xt0)

    psubsw mm1, mm5				; mm1 = [3-4, 2-5, 1-6, 0-7] (xt7, xt6, xt5, xt4)
    movq mm2, mm0				; mm2 = [ xt3 xt2 xt1 xt0 ]

    punpcklwd mm0, mm1			; mm0 = [ xt5 xt1 xt4 xt0 ]

    punpckhwd mm2, mm1			; mm2 = [ xt7 xt3 xt6 xt2 ]
    movq mm1, mm2				; mm1

    movq mm2, mm0				; 2 ; x3 x2 x1 x0

    movq mm3, [TABLE_1]				; 3 ; w06 w04 w02 w00
    punpcklwd mm0, mm1			; x5 x1 x4 x0

    movq mm5, mm0				; 5 ; x5 x1 x4 x0
    punpckldq mm0, mm0			; x4 x0 x4 x0  [ xt2 xt0 xt2 xt0 ]

    movq mm4, [TABLE_1+8]			; 4 ; w07 w05 w03 w01
    punpckhwd mm2, mm1			; 1 ; x7 x3 x6 x2

    pmaddwd mm3, mm0			; x4*w06+x0*w04 x4*w02+x0*w00
    movq mm6, mm2				; 6 ; x7 x3 x6 x2

    movq mm1, [TABLE_1+32]			; 1 ; w22 w20 w18 w16
    punpckldq mm2, mm2			; x6 x2 x6 x2  [ xt3 xt1 xt3 xt1 ]
	
    pmaddwd mm4, mm2			; x6*w07+x2*w05 x6*w03+x2*w01
    punpckhdq mm5, mm5			; x5 x1 x5 x1  [ xt6 xt4 xt6 xt4 ]

    pmaddwd mm0, [TABLE_1+16]		; x4*w14+x0*w12 x4*w10+x0*w08
    punpckhdq mm6, mm6			; x7 x3 x7 x3  [ xt7 xt5 xt7 xt5 ]

    movq mm7, [TABLE_1+40]			; 7 ; w23 w21 w19 w17
    pmaddwd mm1, mm5			; x5*w22+x1*w20 x5*w18+x1*w16

    paddd mm3, [round_frw_row]			; +rounder (y2,y0)
    pmaddwd mm7, mm6			; x7*w23+x3*w21 x7*w19+x3*w17

    pmaddwd mm2, [TABLE_1+24]		; x6*w15+x2*w13 x6*w11+x2*w09
    paddd mm3, mm4				; 4 ; a1=sum(even1) a0=sum(even0)

    pmaddwd mm5, [TABLE_1+48]		; x5*w30+x1*w28 x5*w26+x1*w24
 
    pmaddwd mm6, [TABLE_1+56]		; x7*w31+x3*w29 x7*w27+x3*w25
    paddd mm1, mm7				; 7 ; b1=sum(odd1) b0=sum(odd0)

    paddd mm0, [round_frw_row]			; +rounder (y6,y4)
    psrad mm3, SHIFT_FRW_ROW_CLIP1 ; (y2, y0) 

    paddd mm1, [round_frw_row]			; +rounder (y3,y1)
    paddd mm0, mm2				; 2 ; a3=sum(even3) a2=sum(even2)

    paddd mm5, [round_frw_row]			; +rounder (y7,y5)
    psrad mm1, SHIFT_FRW_ROW_CLIP1 ; y1=a1+b1 y0=a0+b0

    paddd mm5, mm6				; 6 ; b3=sum(odd3) b2=sum(odd2)
    psrad mm0, SHIFT_FRW_ROW_CLIP1 ; y3=a3+b3 y2=a2+b2

    psrad mm5, SHIFT_FRW_ROW_CLIP1 ; y4=a3-b3 y5=a2-b2

	packssdw mm3, mm0			; 0 ; y6 y4 y2 y0, saturate {-32768,+32767}

    packssdw mm1, mm5			; 3 ; y7 y5 y3 y1, saturate {-32768,+32767}
    movq mm6, mm3				; mm0 = y6 y4 y2 y0

    punpcklwd mm3, mm1			; y3 y2 y1 y0
    
    punpckhwd mm6, mm1			; y7 y6 y5 y4

    psraw mm3, SHIFT_FRW_ROW_CLIP2 ; descale [y3 y2 y1 y0] to {-2048,+2047}

    psraw mm6, SHIFT_FRW_ROW_CLIP2 ; descale [y7 y6 y5 y4] to {-2048,+2047}

    movq [OUT_2-16], mm3			; 1 ; save y3 y2 y1 y0

    movq [OUT_2-8], mm6			; 7 ; save y7 y6 y5 y4

    movd mm5, [INP_2+12]			; mm5 = 7 6

    punpcklwd mm5, [INP_2+8]

    movq mm2, mm5				; mm2 = 5 7 4 6
    psrlq mm5, 32				; mm5 = _ _ 5 7

    movq mm0, [INP_2]				; mm0 = 3 2 1 0
    punpcklwd mm5, mm2			; mm5 = 4 5 6 7

    movq mm1, mm0				; mm1 = 3 2 1 0
    paddsw mm0, mm5				; mm0 = [3+4, 2+5, 1+6, 0+7] (xt3, xt2, xt1, xt0)

    psubsw mm1, mm5				; mm1 = [3-4, 2-5, 1-6, 0-7] (xt7, xt6, xt5, xt4)
    movq mm2, mm0				; mm2 = [ xt3 xt2 xt1 xt0 ]

    punpcklwd mm0, mm1			; mm0 = [ xt5 xt1 xt4 xt0 ]

    punpckhwd mm2, mm1			; mm2 = [ xt7 xt3 xt6 xt2 ]
    movq mm1, mm2				; mm1

    movq mm2, mm0				; 2 ; x3 x2 x1 x0

    movq mm3, [TABLE_2]				; 3 ; w06 w04 w02 w00
    punpcklwd mm0, mm1			; x5 x1 x4 x0

    movq mm5, mm0				; 5 ; x5 x1 x4 x0
    punpckldq mm0, mm0			; x4 x0 x4 x0  [ xt2 xt0 xt2 xt0 ]

    movq mm4, [TABLE_2+8]			; 4 ; w07 w05 w03 w01
    punpckhwd mm2, mm1			; 1 ; x7 x3 x6 x2

    pmaddwd mm3, mm0			; x4*w06+x0*w04 x4*w02+x0*w00
    movq mm6, mm2				; 6 ; x7 x3 x6 x2

    movq mm1, [TABLE_2+32]			; 1 ; w22 w20 w18 w16
    punpckldq mm2, mm2			; x6 x2 x6 x2  [ xt3 xt1 xt3 xt1 ]
	
    pmaddwd mm4, mm2			; x6*w07+x2*w05 x6*w03+x2*w01
    punpckhdq mm5, mm5			; x5 x1 x5 x1  [ xt6 xt4 xt6 xt4 ]

    pmaddwd mm0, [TABLE_2+16]		; x4*w14+x0*w12 x4*w10+x0*w08
    punpckhdq mm6, mm6			; x7 x3 x7 x3  [ xt7 xt5 xt7 xt5 ]

    movq mm7, [TABLE_2+40]			; 7 ; w23 w21 w19 w17
    pmaddwd mm1, mm5			; x5*w22+x1*w20 x5*w18+x1*w16

    paddd mm3, [round_frw_row]			; +rounder (y2,y0)
    pmaddwd mm7, mm6			; x7*w23+x3*w21 x7*w19+x3*w17

    pmaddwd mm2, [TABLE_2+24]		; x6*w15+x2*w13 x6*w11+x2*w09
    paddd mm3, mm4				; 4 ; a1=sum(even1) a0=sum(even0)

    pmaddwd mm5, [TABLE_2+48]		; x5*w30+x1*w28 x5*w26+x1*w24
 
    pmaddwd mm6, [TABLE_2+56]		; x7*w31+x3*w29 x7*w27+x3*w25
    paddd mm1, mm7				; 7 ; b1=sum(odd1) b0=sum(odd0)

    paddd mm0, [round_frw_row]			; +rounder (y6,y4)
    psrad mm3, SHIFT_FRW_ROW_CLIP1 ; (y2, y0) 

    paddd mm1, [round_frw_row]			; +rounder (y3,y1)
    paddd mm0, mm2				; 2 ; a3=sum(even3) a2=sum(even2)

    paddd mm5, [round_frw_row]			; +rounder (y7,y5)
    psrad mm1, SHIFT_FRW_ROW_CLIP1 ; y1=a1+b1 y0=a0+b0

    paddd mm5, mm6				; 6 ; b3=sum(odd3) b2=sum(odd2)
    psrad mm0, SHIFT_FRW_ROW_CLIP1 ; y3=a3+b3 y2=a2+b2

    psrad mm5, SHIFT_FRW_ROW_CLIP1 ; y4=a3-b3 y5=a2-b2

	packssdw mm3, mm0			; 0 ; y6 y4 y2 y0, saturate {-32768,+32767}

    packssdw mm1, mm5			; 3 ; y7 y5 y3 y1, saturate {-32768,+32767}
    movq mm6, mm3				; mm0 = y6 y4 y2 y0

    punpcklwd mm3, mm1			; y3 y2 y1 y0
    
    punpckhwd mm6, mm1			; y7 y6 y5 y4

    psraw mm3, SHIFT_FRW_ROW_CLIP2 ; descale [y3 y2 y1 y0] to {-2048,+2047}

    psraw mm6, SHIFT_FRW_ROW_CLIP2 ; descale [y7 y6 y5 y4] to {-2048,+2047}

    movq [OUT_3-16], mm3			; 1 ; save y3 y2 y1 y0

    movq [OUT_3-8], mm6			; 7 ; save y7 y6 y5 y4

    movd mm5, [INP_3+12]			; mm5 = 7 6

    punpcklwd mm5, [INP_3+8]

    movq mm2, mm5				; mm2 = 5 7 4 6
    psrlq mm5, 32				; mm5 = _ _ 5 7

    movq mm0, [INP_3]				; mm0 = 3 2 1 0
    punpcklwd mm5, mm2			; mm5 = 4 5 6 7

    movq mm1, mm0				; mm1 = 3 2 1 0
    paddsw mm0, mm5				; mm0 = [3+4, 2+5, 1+6, 0+7] (xt3, xt2, xt1, xt0)

    psubsw mm1, mm5				; mm1 = [3-4, 2-5, 1-6, 0-7] (xt7, xt6, xt5, xt4)
    movq mm2, mm0				; mm2 = [ xt3 xt2 xt1 xt0 ]

    punpcklwd mm0, mm1			; mm0 = [ xt5 xt1 xt4 xt0 ]

    punpckhwd mm2, mm1			; mm2 = [ xt7 xt3 xt6 xt2 ]
    movq mm1, mm2				; mm1

    movq mm2, mm0				; 2 ; x3 x2 x1 x0

    movq mm3, [TABLE_3]				; 3 ; w06 w04 w02 w00
    punpcklwd mm0, mm1			; x5 x1 x4 x0

    movq mm5, mm0				; 5 ; x5 x1 x4 x0
    punpckldq mm0, mm0			; x4 x0 x4 x0  [ xt2 xt0 xt2 xt0 ]

    movq mm4, [TABLE_3+8]			; 4 ; w07 w05 w03 w01
    punpckhwd mm2, mm1			; 1 ; x7 x3 x6 x2

    pmaddwd mm3, mm0			; x4*w06+x0*w04 x4*w02+x0*w00
    movq mm6, mm2				; 6 ; x7 x3 x6 x2

    movq mm1, [TABLE_3+32]			; 1 ; w22 w20 w18 w16
    punpckldq mm2, mm2			; x6 x2 x6 x2  [ xt3 xt1 xt3 xt1 ]
	
    pmaddwd mm4, mm2			; x6*w07+x2*w05 x6*w03+x2*w01
    punpckhdq mm5, mm5			; x5 x1 x5 x1  [ xt6 xt4 xt6 xt4 ]

    pmaddwd mm0, [TABLE_3+16]		; x4*w14+x0*w12 x4*w10+x0*w08
    punpckhdq mm6, mm6			; x7 x3 x7 x3  [ xt7 xt5 xt7 xt5 ]

    movq mm7, [TABLE_3+40]			; 7 ; w23 w21 w19 w17
    pmaddwd mm1, mm5			; x5*w22+x1*w20 x5*w18+x1*w16

    paddd mm3, [round_frw_row]			; +rounder (y2,y0)
    pmaddwd mm7, mm6			; x7*w23+x3*w21 x7*w19+x3*w17

    pmaddwd mm2, [TABLE_3+24]		; x6*w15+x2*w13 x6*w11+x2*w09
    paddd mm3, mm4				; 4 ; a1=sum(even1) a0=sum(even0)

    pmaddwd mm5, [TABLE_3+48]		; x5*w30+x1*w28 x5*w26+x1*w24
 
    pmaddwd mm6, [TABLE_3+56]		; x7*w31+x3*w29 x7*w27+x3*w25
    paddd mm1, mm7				; 7 ; b1=sum(odd1) b0=sum(odd0)

    paddd mm0, [round_frw_row]			; +rounder (y6,y4)
    psrad mm3, SHIFT_FRW_ROW_CLIP1 ; (y2, y0) 

    paddd mm1, [round_frw_row]			; +rounder (y3,y1)
    paddd mm0, mm2				; 2 ; a3=sum(even3) a2=sum(even2)

    paddd mm5, [round_frw_row]			; +rounder (y7,y5)
    psrad mm1, SHIFT_FRW_ROW_CLIP1 ; y1=a1+b1 y0=a0+b0

    paddd mm5, mm6				; 6 ; b3=sum(odd3) b2=sum(odd2)
    psrad mm0, SHIFT_FRW_ROW_CLIP1 ; y3=a3+b3 y2=a2+b2

    psrad mm5, SHIFT_FRW_ROW_CLIP1 ; y4=a3-b3 y5=a2-b2

	packssdw mm3, mm0			; 0 ; y6 y4 y2 y0, saturate {-32768,+32767}

    packssdw mm1, mm5			; 3 ; y7 y5 y3 y1, saturate {-32768,+32767}
    movq mm6, mm3				; mm0 = y6 y4 y2 y0

    punpcklwd mm3, mm1			; y3 y2 y1 y0
    
    punpckhwd mm6, mm1			; y7 y6 y5 y4

    psraw mm3, SHIFT_FRW_ROW_CLIP2 ; descale [y3 y2 y1 y0] to {-2048,+2047}

    psraw mm6, SHIFT_FRW_ROW_CLIP2 ; descale [y7 y6 y5 y4] to {-2048,+2047}

    movq [OUT_4-16], mm3			; 1 ; save y3 y2 y1 y0

    movq [OUT_4-8], mm6			; 7 ; save y7 y6 y5 y4

    movd mm5, [INP_4+12]			; mm5 = 7 6

    punpcklwd mm5, [INP_4+8]

    movq mm2, mm5				; mm2 = 5 7 4 6
    psrlq mm5, 32				; mm5 = _ _ 5 7

    movq mm0, [INP_4]				; mm0 = 3 2 1 0
    punpcklwd mm5, mm2			; mm5 = 4 5 6 7

    movq mm1, mm0				; mm1 = 3 2 1 0
    paddsw mm0, mm5				; mm0 = [3+4, 2+5, 1+6, 0+7] (xt3, xt2, xt1, xt0)

    psubsw mm1, mm5				; mm1 = [3-4, 2-5, 1-6, 0-7] (xt7, xt6, xt5, xt4)
    movq mm2, mm0				; mm2 = [ xt3 xt2 xt1 xt0 ]

    punpcklwd mm0, mm1			; mm0 = [ xt5 xt1 xt4 xt0 ]

    punpckhwd mm2, mm1			; mm2 = [ xt7 xt3 xt6 xt2 ]
    movq mm1, mm2				; mm1

    movq mm2, mm0				; 2 ; x3 x2 x1 x0

    movq mm3, [TABLE_4]				; 3 ; w06 w04 w02 w00
    punpcklwd mm0, mm1			; x5 x1 x4 x0

    movq mm5, mm0				; 5 ; x5 x1 x4 x0
    punpckldq mm0, mm0			; x4 x0 x4 x0  [ xt2 xt0 xt2 xt0 ]

    movq mm4, [TABLE_4+8]			; 4 ; w07 w05 w03 w01
    punpckhwd mm2, mm1			; 1 ; x7 x3 x6 x2

    pmaddwd mm3, mm0			; x4*w06+x0*w04 x4*w02+x0*w00
    movq mm6, mm2				; 6 ; x7 x3 x6 x2

    movq mm1, [TABLE_4+32]			; 1 ; w22 w20 w18 w16
    punpckldq mm2, mm2			; x6 x2 x6 x2  [ xt3 xt1 xt3 xt1 ]
	
    pmaddwd mm4, mm2			; x6*w07+x2*w05 x6*w03+x2*w01
    punpckhdq mm5, mm5			; x5 x1 x5 x1  [ xt6 xt4 xt6 xt4 ]

    pmaddwd mm0, [TABLE_4+16]		; x4*w14+x0*w12 x4*w10+x0*w08
    punpckhdq mm6, mm6			; x7 x3 x7 x3  [ xt7 xt5 xt7 xt5 ]

    movq mm7, [TABLE_4+40]			; 7 ; w23 w21 w19 w17
    pmaddwd mm1, mm5			; x5*w22+x1*w20 x5*w18+x1*w16

    paddd mm3, [round_frw_row]			; +rounder (y2,y0)
    pmaddwd mm7, mm6			; x7*w23+x3*w21 x7*w19+x3*w17

    pmaddwd mm2, [TABLE_4+24]		; x6*w15+x2*w13 x6*w11+x2*w09
    paddd mm3, mm4				; 4 ; a1=sum(even1) a0=sum(even0)

    pmaddwd mm5, [TABLE_4+48]		; x5*w30+x1*w28 x5*w26+x1*w24
 
    pmaddwd mm6, [TABLE_4+56]		; x7*w31+x3*w29 x7*w27+x3*w25
    paddd mm1, mm7				; 7 ; b1=sum(odd1) b0=sum(odd0)

    paddd mm0, [round_frw_row]			; +rounder (y6,y4)
    psrad mm3, SHIFT_FRW_ROW_CLIP1 ; (y2, y0) 

    paddd mm1, [round_frw_row]			; +rounder (y3,y1)
    paddd mm0, mm2				; 2 ; a3=sum(even3) a2=sum(even2)

    paddd mm5, [round_frw_row]			; +rounder (y7,y5)
    psrad mm1, SHIFT_FRW_ROW_CLIP1 ; y1=a1+b1 y0=a0+b0

    paddd mm5, mm6				; 6 ; b3=sum(odd3) b2=sum(odd2)
    psrad mm0, SHIFT_FRW_ROW_CLIP1 ; y3=a3+b3 y2=a2+b2

    psrad mm5, SHIFT_FRW_ROW_CLIP1 ; y4=a3-b3 y5=a2-b2

	packssdw mm3, mm0			; 0 ; y6 y4 y2 y0, saturate {-32768,+32767}

    packssdw mm1, mm5			; 3 ; y7 y5 y3 y1, saturate {-32768,+32767}
    movq mm6, mm3				; mm0 = y6 y4 y2 y0

    punpcklwd mm3, mm1			; y3 y2 y1 y0
    
    punpckhwd mm6, mm1			; y7 y6 y5 y4

    psraw mm3, SHIFT_FRW_ROW_CLIP2 ; descale [y3 y2 y1 y0] to {-2048,+2047}

    psraw mm6, SHIFT_FRW_ROW_CLIP2 ; descale [y7 y6 y5 y4] to {-2048,+2047}

    movq [OUT_5-16], mm3			; 1 ; save y3 y2 y1 y0

    movq [OUT_5-8], mm6			; 7 ; save y7 y6 y5 y4

    movd mm5, [INP_5+12]			; mm5 = 7 6

    punpcklwd mm5, [INP_5+8]

    movq mm2, mm5				; mm2 = 5 7 4 6
    psrlq mm5, 32				; mm5 = _ _ 5 7

    movq mm0, [INP_5]				; mm0 = 3 2 1 0
    punpcklwd mm5, mm2			; mm5 = 4 5 6 7

    movq mm1, mm0				; mm1 = 3 2 1 0
    paddsw mm0, mm5				; mm0 = [3+4, 2+5, 1+6, 0+7] (xt3, xt2, xt1, xt0)

    psubsw mm1, mm5				; mm1 = [3-4, 2-5, 1-6, 0-7] (xt7, xt6, xt5, xt4)
    movq mm2, mm0				; mm2 = [ xt3 xt2 xt1 xt0 ]

    punpcklwd mm0, mm1			; mm0 = [ xt5 xt1 xt4 xt0 ]

    punpckhwd mm2, mm1			; mm2 = [ xt7 xt3 xt6 xt2 ]
    movq mm1, mm2				; mm1

    movq mm2, mm0				; 2 ; x3 x2 x1 x0

    movq mm3, [TABLE_5]				; 3 ; w06 w04 w02 w00
    punpcklwd mm0, mm1			; x5 x1 x4 x0

    movq mm5, mm0				; 5 ; x5 x1 x4 x0
    punpckldq mm0, mm0			; x4 x0 x4 x0  [ xt2 xt0 xt2 xt0 ]

    movq mm4, [TABLE_5+8]			; 4 ; w07 w05 w03 w01
    punpckhwd mm2, mm1			; 1 ; x7 x3 x6 x2

    pmaddwd mm3, mm0			; x4*w06+x0*w04 x4*w02+x0*w00
    movq mm6, mm2				; 6 ; x7 x3 x6 x2

    movq mm1, [TABLE_5+32]			; 1 ; w22 w20 w18 w16
    punpckldq mm2, mm2			; x6 x2 x6 x2  [ xt3 xt1 xt3 xt1 ]
	
    pmaddwd mm4, mm2			; x6*w07+x2*w05 x6*w03+x2*w01
    punpckhdq mm5, mm5			; x5 x1 x5 x1  [ xt6 xt4 xt6 xt4 ]

    pmaddwd mm0, [TABLE_5+16]		; x4*w14+x0*w12 x4*w10+x0*w08
    punpckhdq mm6, mm6			; x7 x3 x7 x3  [ xt7 xt5 xt7 xt5 ]

    movq mm7, [TABLE_5+40]			; 7 ; w23 w21 w19 w17
    pmaddwd mm1, mm5			; x5*w22+x1*w20 x5*w18+x1*w16

    paddd mm3, [round_frw_row]			; +rounder (y2,y0)
    pmaddwd mm7, mm6			; x7*w23+x3*w21 x7*w19+x3*w17

    pmaddwd mm2, [TABLE_5+24]		; x6*w15+x2*w13 x6*w11+x2*w09
    paddd mm3, mm4				; 4 ; a1=sum(even1) a0=sum(even0)

    pmaddwd mm5, [TABLE_5+48]		; x5*w30+x1*w28 x5*w26+x1*w24
 
    pmaddwd mm6, [TABLE_5+56]		; x7*w31+x3*w29 x7*w27+x3*w25
    paddd mm1, mm7				; 7 ; b1=sum(odd1) b0=sum(odd0)

    paddd mm0, [round_frw_row]			; +rounder (y6,y4)
    psrad mm3, SHIFT_FRW_ROW_CLIP1 ; (y2, y0) 

    paddd mm1, [round_frw_row]			; +rounder (y3,y1)
    paddd mm0, mm2				; 2 ; a3=sum(even3) a2=sum(even2)

    paddd mm5, [round_frw_row]			; +rounder (y7,y5)
    psrad mm1, SHIFT_FRW_ROW_CLIP1 ; y1=a1+b1 y0=a0+b0

    paddd mm5, mm6				; 6 ; b3=sum(odd3) b2=sum(odd2)
    psrad mm0, SHIFT_FRW_ROW_CLIP1 ; y3=a3+b3 y2=a2+b2

    psrad mm5, SHIFT_FRW_ROW_CLIP1 ; y4=a3-b3 y5=a2-b2

	packssdw mm3, mm0			; 0 ; y6 y4 y2 y0, saturate {-32768,+32767}

    packssdw mm1, mm5			; 3 ; y7 y5 y3 y1, saturate {-32768,+32767}
    movq mm6, mm3				; mm0 = y6 y4 y2 y0

    punpcklwd mm3, mm1			; y3 y2 y1 y0
    
    punpckhwd mm6, mm1			; y7 y6 y5 y4

    psraw mm3, SHIFT_FRW_ROW_CLIP2 ; descale [y3 y2 y1 y0] to {-2048,+2047}

    psraw mm6, SHIFT_FRW_ROW_CLIP2 ; descale [y7 y6 y5 y4] to {-2048,+2047}

    movq [OUT_6-16], mm3			; 1 ; save y3 y2 y1 y0

    movq [OUT_6-8], mm6			; 7 ; save y7 y6 y5 y4

    movd mm5, [INP_6+12]			; mm5 = 7 6

    punpcklwd mm5, [INP_6+8]

    movq mm2, mm5				; mm2 = 5 7 4 6
    psrlq mm5, 32				; mm5 = _ _ 5 7

    movq mm0, [INP_6]				; mm0 = 3 2 1 0
    punpcklwd mm5, mm2			; mm5 = 4 5 6 7

    movq mm1, mm0				; mm1 = 3 2 1 0
    paddsw mm0, mm5				; mm0 = [3+4, 2+5, 1+6, 0+7] (xt3, xt2, xt1, xt0)

    psubsw mm1, mm5				; mm1 = [3-4, 2-5, 1-6, 0-7] (xt7, xt6, xt5, xt4)
    movq mm2, mm0				; mm2 = [ xt3 xt2 xt1 xt0 ]

    punpcklwd mm0, mm1			; mm0 = [ xt5 xt1 xt4 xt0 ]

    punpckhwd mm2, mm1			; mm2 = [ xt7 xt3 xt6 xt2 ]
    movq mm1, mm2				; mm1

    movq mm2, mm0				; 2 ; x3 x2 x1 x0

    movq mm3, [TABLE_6]				; 3 ; w06 w04 w02 w00
    punpcklwd mm0, mm1			; x5 x1 x4 x0

    movq mm5, mm0				; 5 ; x5 x1 x4 x0
    punpckldq mm0, mm0			; x4 x0 x4 x0  [ xt2 xt0 xt2 xt0 ]

    movq mm4, [TABLE_6+8]			; 4 ; w07 w05 w03 w01
    punpckhwd mm2, mm1			; 1 ; x7 x3 x6 x2

    pmaddwd mm3, mm0			; x4*w06+x0*w04 x4*w02+x0*w00
    movq mm6, mm2				; 6 ; x7 x3 x6 x2

    movq mm1, [TABLE_6+32]			; 1 ; w22 w20 w18 w16
    punpckldq mm2, mm2			; x6 x2 x6 x2  [ xt3 xt1 xt3 xt1 ]
	
    pmaddwd mm4, mm2			; x6*w07+x2*w05 x6*w03+x2*w01
    punpckhdq mm5, mm5			; x5 x1 x5 x1  [ xt6 xt4 xt6 xt4 ]

    pmaddwd mm0, [TABLE_6+16]		; x4*w14+x0*w12 x4*w10+x0*w08
    punpckhdq mm6, mm6			; x7 x3 x7 x3  [ xt7 xt5 xt7 xt5 ]

    movq mm7, [TABLE_6+40]			; 7 ; w23 w21 w19 w17
    pmaddwd mm1, mm5			; x5*w22+x1*w20 x5*w18+x1*w16

    paddd mm3, [round_frw_row]			; +rounder (y2,y0)
    pmaddwd mm7, mm6			; x7*w23+x3*w21 x7*w19+x3*w17

    pmaddwd mm2, [TABLE_6+24]		; x6*w15+x2*w13 x6*w11+x2*w09
    paddd mm3, mm4				; 4 ; a1=sum(even1) a0=sum(even0)

    pmaddwd mm5, [TABLE_6+48]		; x5*w30+x1*w28 x5*w26+x1*w24
 
    pmaddwd mm6, [TABLE_6+56]		; x7*w31+x3*w29 x7*w27+x3*w25
    paddd mm1, mm7				; 7 ; b1=sum(odd1) b0=sum(odd0)

    paddd mm0, [round_frw_row]			; +rounder (y6,y4)
    psrad mm3, SHIFT_FRW_ROW_CLIP1 ; (y2, y0) 

    paddd mm1, [round_frw_row]			; +rounder (y3,y1)
    paddd mm0, mm2				; 2 ; a3=sum(even3) a2=sum(even2)

    paddd mm5, [round_frw_row]			; +rounder (y7,y5)
    psrad mm1, SHIFT_FRW_ROW_CLIP1 ; y1=a1+b1 y0=a0+b0

    paddd mm5, mm6				; 6 ; b3=sum(odd3) b2=sum(odd2)
    psrad mm0, SHIFT_FRW_ROW_CLIP1 ; y3=a3+b3 y2=a2+b2

    psrad mm5, SHIFT_FRW_ROW_CLIP1 ; y4=a3-b3 y5=a2-b2

	packssdw mm3, mm0			; 0 ; y6 y4 y2 y0, saturate {-32768,+32767}

    packssdw mm1, mm5			; 3 ; y7 y5 y3 y1, saturate {-32768,+32767}
    movq mm6, mm3				; mm0 = y6 y4 y2 y0

    punpcklwd mm3, mm1			; y3 y2 y1 y0
    
    punpckhwd mm6, mm1			; y7 y6 y5 y4

    psraw mm3, SHIFT_FRW_ROW_CLIP2 ; descale [y3 y2 y1 y0] to {-2048,+2047}

    psraw mm6, SHIFT_FRW_ROW_CLIP2 ; descale [y7 y6 y5 y4] to {-2048,+2047}

    movq [OUT_7-16], mm3			; 1 ; save y3 y2 y1 y0

    movq [OUT_7-8], mm6			; 7 ; save y7 y6 y5 y4

    movd mm5, [INP_7+12]			; mm5 = 7 6

    punpcklwd mm5, [INP_7+8]

    movq mm2, mm5				; mm2 = 5 7 4 6
    psrlq mm5, 32				; mm5 = _ _ 5 7

    movq mm0, [INP_7]				; mm0 = 3 2 1 0
    punpcklwd mm5, mm2			; mm5 = 4 5 6 7

    movq mm1, mm0				; mm1 = 3 2 1 0
    paddsw mm0, mm5				; mm0 = [3+4, 2+5, 1+6, 0+7] (xt3, xt2, xt1, xt0)

    psubsw mm1, mm5				; mm1 = [3-4, 2-5, 1-6, 0-7] (xt7, xt6, xt5, xt4)
    movq mm2, mm0				; mm2 = [ xt3 xt2 xt1 xt0 ]

    punpcklwd mm0, mm1			; mm0 = [ xt5 xt1 xt4 xt0 ]

    punpckhwd mm2, mm1			; mm2 = [ xt7 xt3 xt6 xt2 ]
    movq mm1, mm2				; mm1

    movq mm2, mm0				; 2 ; x3 x2 x1 x0

    movq mm3, [TABLE_7]				; 3 ; w06 w04 w02 w00
    punpcklwd mm0, mm1			; x5 x1 x4 x0

    movq mm5, mm0				; 5 ; x5 x1 x4 x0
    punpckldq mm0, mm0			; x4 x0 x4 x0  [ xt2 xt0 xt2 xt0 ]

    movq mm4, [TABLE_7+8]			; 4 ; w07 w05 w03 w01
    punpckhwd mm2, mm1			; 1 ; x7 x3 x6 x2

    pmaddwd mm3, mm0			; x4*w06+x0*w04 x4*w02+x0*w00
    movq mm6, mm2				; 6 ; x7 x3 x6 x2

    movq mm1, [TABLE_7+32]			; 1 ; w22 w20 w18 w16
    punpckldq mm2, mm2			; x6 x2 x6 x2  [ xt3 xt1 xt3 xt1 ]
	
    pmaddwd mm4, mm2			; x6*w07+x2*w05 x6*w03+x2*w01
    punpckhdq mm5, mm5			; x5 x1 x5 x1  [ xt6 xt4 xt6 xt4 ]

    pmaddwd mm0, [TABLE_7+16]		; x4*w14+x0*w12 x4*w10+x0*w08
    punpckhdq mm6, mm6			; x7 x3 x7 x3  [ xt7 xt5 xt7 xt5 ]

    movq mm7, [TABLE_7+40]			; 7 ; w23 w21 w19 w17
    pmaddwd mm1, mm5			; x5*w22+x1*w20 x5*w18+x1*w16

    paddd mm3, [round_frw_row]			; +rounder (y2,y0)
    pmaddwd mm7, mm6			; x7*w23+x3*w21 x7*w19+x3*w17

    pmaddwd mm2, [TABLE_7+24]		; x6*w15+x2*w13 x6*w11+x2*w09
    paddd mm3, mm4				; 4 ; a1=sum(even1) a0=sum(even0)

    pmaddwd mm5, [TABLE_7+48]		; x5*w30+x1*w28 x5*w26+x1*w24
 
    pmaddwd mm6, [TABLE_7+56]		; x7*w31+x3*w29 x7*w27+x3*w25
    paddd mm1, mm7				; 7 ; b1=sum(odd1) b0=sum(odd0)

    paddd mm0, [round_frw_row]			; +rounder (y6,y4)
    psrad mm3, SHIFT_FRW_ROW_CLIP1 ; (y2, y0) 

    paddd mm1, [round_frw_row]			; +rounder (y3,y1)
    paddd mm0, mm2				; 2 ; a3=sum(even3) a2=sum(even2)

    paddd mm5, [round_frw_row]			; +rounder (y7,y5)
    psrad mm1, SHIFT_FRW_ROW_CLIP1 ; y1=a1+b1 y0=a0+b0

    paddd mm5, mm6				; 6 ; b3=sum(odd3) b2=sum(odd2)
    psrad mm0, SHIFT_FRW_ROW_CLIP1 ; y3=a3+b3 y2=a2+b2

    psrad mm5, SHIFT_FRW_ROW_CLIP1 ; y4=a3-b3 y5=a2-b2

	packssdw mm3, mm0			; 0 ; y6 y4 y2 y0, saturate {-32768,+32767}

    packssdw mm1, mm5			; 3 ; y7 y5 y3 y1, saturate {-32768,+32767}
    movq mm6, mm3				; mm0 = y6 y4 y2 y0

    punpcklwd mm3, mm1			; y3 y2 y1 y0
    
    punpckhwd mm6, mm1			; y7 y6 y5 y4

    psraw mm3, SHIFT_FRW_ROW_CLIP2 ; descale [y3 y2 y1 y0] to {-2048,+2047}

    psraw mm6, SHIFT_FRW_ROW_CLIP2 ; descale [y7 y6 y5 y4] to {-2048,+2047}

    movq [OUT_8-16], mm3			; 1 ; save y3 y2 y1 y0

    movq [OUT_8-8], mm6			; 7 ; save y7 y6 y5 y4

	pop ebx
	emms
	
	ret
