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
; *  quantize4.asm, MMX optimized MPEG quantization/dequantization             *
; *                                                                            *
; *  Copyright (C) 2002 - Peter Ross <pross@cs.rmit.edu.au>                    *
; *  Copyright (C) 2002 - Michael Militzer <isibaar@xvid.org>                  *
; *                                                                            *
; *  For more information visit the XviD homepage: http://www.xvid.org         *
; *                                                                            *
; ******************************************************************************/
;
;/******************************************************************************
; *                                                                            *
; *  Revision history:                                                         *
; *                                                                            *
; *  22.01.2002 initial version                                                *
; *                                                                            *
; ******************************************************************************/

; data/text alignment
%define ALIGN 8

%define SATURATE

bits 32

section .data

%macro cglobal 1 
	%ifdef PREFIX
		global _%1 
		%define %1 _%1
	%else
		global %1
	%endif
%endmacro

%macro cextern 1 
	%ifdef PREFIX
		extern _%1 
		%define %1 _%1
	%else
		extern %1
	%endif
%endmacro

mmx_one times 4	dw	 1

;===========================================================================
;
; divide by 2Q table 
;
;===========================================================================

%macro MMX_DIV  1
times 4 dw  (1 << 17) / (%1 * 2) + 1
%endmacro

align ALIGN
mmx_div
		MMX_DIV 1
		MMX_DIV 2
		MMX_DIV 3
		MMX_DIV 4
		MMX_DIV 5
		MMX_DIV 6
		MMX_DIV 7
		MMX_DIV 8
		MMX_DIV 9
		MMX_DIV 10
		MMX_DIV 11
		MMX_DIV 12
		MMX_DIV 13
		MMX_DIV 14
		MMX_DIV 15
		MMX_DIV 16
		MMX_DIV 17
		MMX_DIV 18
		MMX_DIV 19
		MMX_DIV 20
		MMX_DIV 21
		MMX_DIV 22
		MMX_DIV 23
		MMX_DIV 24
		MMX_DIV 25
		MMX_DIV 26
		MMX_DIV 27
		MMX_DIV 28
		MMX_DIV 29
		MMX_DIV 30
		MMX_DIV 31


;===========================================================================
;
; intra matrix 
;
;===========================================================================

cextern intra_matrix
cextern intra_matrix_fix

;===========================================================================
;
; inter matrix
;
;===========================================================================

cextern inter_matrix
cextern inter_matrix_fix


%define VM18P 3
%define VM18Q 4


;===========================================================================
;
; quantd table 
;
;===========================================================================

%macro MMX_QUANTD  1
times 4 dw ((VM18P*%1) + (VM18Q/2)) / VM18Q
%endmacro

quantd
		MMX_QUANTD 1 
		MMX_QUANTD 2 
		MMX_QUANTD 3 
		MMX_QUANTD 4 
		MMX_QUANTD 5 
		MMX_QUANTD 6 
		MMX_QUANTD 7 
		MMX_QUANTD 8 
		MMX_QUANTD 9 
		MMX_QUANTD 10 
		MMX_QUANTD 11
		MMX_QUANTD 12
		MMX_QUANTD 13
		MMX_QUANTD 14
		MMX_QUANTD 15
		MMX_QUANTD 16
		MMX_QUANTD 17
		MMX_QUANTD 18
		MMX_QUANTD 19
		MMX_QUANTD 20
		MMX_QUANTD 21
		MMX_QUANTD 22
		MMX_QUANTD 23
		MMX_QUANTD 24
		MMX_QUANTD 25
		MMX_QUANTD 26
		MMX_QUANTD 27
		MMX_QUANTD 28
		MMX_QUANTD 29
		MMX_QUANTD 30
		MMX_QUANTD 31


;===========================================================================
;
; multiple by 2Q table
;
;===========================================================================

%macro MMX_MUL_QUANT  1
times 4   dw  %1
%endmacro

mmx_mul_quant
	MMX_MUL_QUANT 1
	MMX_MUL_QUANT 2
	MMX_MUL_QUANT 3
	MMX_MUL_QUANT 4
	MMX_MUL_QUANT 5
	MMX_MUL_QUANT 6
	MMX_MUL_QUANT 7
	MMX_MUL_QUANT 8
	MMX_MUL_QUANT 9
	MMX_MUL_QUANT 10
	MMX_MUL_QUANT 11
	MMX_MUL_QUANT 12
	MMX_MUL_QUANT 13
	MMX_MUL_QUANT 14
	MMX_MUL_QUANT 15
	MMX_MUL_QUANT 16
	MMX_MUL_QUANT 17
	MMX_MUL_QUANT 18
	MMX_MUL_QUANT 19
	MMX_MUL_QUANT 20
	MMX_MUL_QUANT 21
	MMX_MUL_QUANT 22
	MMX_MUL_QUANT 23
	MMX_MUL_QUANT 24
	MMX_MUL_QUANT 25
	MMX_MUL_QUANT 26
	MMX_MUL_QUANT 27
	MMX_MUL_QUANT 28
	MMX_MUL_QUANT 29
	MMX_MUL_QUANT 30
	MMX_MUL_QUANT 31

;===========================================================================
;
; saturation limits 
;
;===========================================================================

align 16
mmx_32768_minus_2048                times 4 dw (32768-2048)
mmx_32767_minus_2047                times 4 dw (32767-2047)

section .text

;===========================================================================
;
; void quant_intra4_mmx(int16_t * coeff, 
;					const int16_t const * data,
;					const uint32_t quant,
;					const uint32_t dcscalar);
;
;===========================================================================

align ALIGN
cglobal quant4_intra_mmx
quant4_intra_mmx

		push	ecx
		push	esi
		push	edi

		mov	edi, [esp + 12 + 4]		; coeff
		mov	esi, [esp + 12 + 8]		; data
		mov	eax, [esp + 12 + 12]	; quant

		movq    mm5, [quantd + eax * 8 - 8] ; quantd -> mm5

		xor ecx, ecx
		cmp	al, 1
		jz	near .q1loop

		cmp	al, 2
		jz	near .q2loop

		movq	mm7, [mmx_div + eax * 8 - 8] ; multipliers[quant] -> mm7
		 
align ALIGN
.loop
		movq	mm0, [esi + 8*ecx]		; mm0 = [1st]
		movq	mm3, [esi + 8*ecx + 8]	; 
		
		pxor	mm1, mm1		; mm1 = 0
		pxor	mm4, mm4
		
		pcmpgtw	mm1, mm0		; mm1 = (0 > mm0)
		pcmpgtw	mm4, mm3
		
		pxor	mm0, mm1		; mm0 = |mm0|
		pxor	mm3, mm4		;
		psubw	mm0, mm1		; displace
		psubw	mm3, mm4		;

		psllw   mm0, 4			; level << 4
		psllw   mm3, 4			;
		
		movq    mm2, [intra_matrix + 8*ecx]
		psrlw   mm2, 1			; intra_matrix[i]>>1
		paddw   mm0, mm2		
		
		movq    mm2, [intra_matrix_fix + ecx*8]
		pmulhw  mm0, mm2		; (level<<4 + intra_matrix[i]>>1) / intra_matrix[i]

		movq    mm2, [intra_matrix + 8*ecx + 8]
		psrlw   mm2, 1
		paddw   mm3, mm2

		movq    mm2, [intra_matrix_fix + ecx*8 + 8]
		pmulhw  mm3, mm2

	    paddw   mm0, mm5		; + quantd
		paddw   mm3, mm5

		pmulhw	mm0, mm7		; mm0 = (mm0 / 2Q) >> 16
		pmulhw	mm3, mm7		;
		psrlw   mm0, 1			; additional shift by 1 => 16 + 1 = 17
		psrlw   mm3, 1
		
		pxor	mm0, mm1		; mm0 *= sign(mm0)
		pxor	mm3, mm4		;
		psubw	mm0, mm1		; undisplace
		psubw	mm3, mm4		;

		movq	[edi + 8*ecx], mm0
		movq	[edi + 8*ecx + 8], mm3
		
		add ecx,2
		cmp ecx,16
		jnz 	near .loop 

.done	
		; caclulate  data[0] // (int32_t)dcscalar)

		mov 	ecx, [esp + 12 + 16]	; dcscalar
		mov 	edx, ecx
		movsx 	eax, word [esi]	; data[0]
		shr 	edx, 1			; edx = dcscalar /2
		cmp		eax, 0
		jg		.gtzero

		sub		eax, edx
		jmp		short .mul
.gtzero
		add		eax, edx
.mul
		cdq 				; expand eax -> edx:eax
		idiv	ecx			; eax = edx:eax / dcscalar
		
		mov	[edi], ax		; coeff[0] = ax

		pop	edi
		pop	esi
		pop	ecx

		ret				

align ALIGN
.q1loop
		movq	mm0, [esi + 8*ecx]		; mm0 = [1st]
		movq	mm3, [esi + 8*ecx + 8]	; 

		pxor	mm1, mm1		; mm1 = 0
		pxor	mm4, mm4		;

		pcmpgtw	mm1, mm0		; mm1 = (0 > mm0)
		pcmpgtw	mm4, mm3		;
		 
		pxor	mm0, mm1		; mm0 = |mm0|
		pxor	mm3, mm4		; 
		psubw	mm0, mm1		; displace
		psubw	mm3, mm4		; 

		psllw   mm0, 4
		psllw   mm3, 4
		
		movq    mm2, [intra_matrix + 8*ecx]
		psrlw   mm2, 1
		paddw   mm0, mm2
		
		movq    mm2, [intra_matrix_fix + ecx*8]
		pmulhw  mm0, mm2		; (level<<4 + intra_matrix[i]>>1) / intra_matrix[i]

		movq    mm2, [intra_matrix + 8*ecx + 8]
		psrlw   mm2, 1
		paddw   mm3, mm2

		movq    mm2, [intra_matrix_fix + ecx*8 + 8]
		pmulhw  mm3, mm2

        paddw   mm0, mm5
		paddw   mm3, mm5

		psrlw	mm0, 1			; mm0 >>= 1   (/2)
		psrlw	mm3, 1			;
		
		pxor	mm0, mm1		; mm0 *= sign(mm0)
		pxor	mm3, mm4        ;
		psubw	mm0, mm1		; undisplace
		psubw	mm3, mm4		;
		
		movq	[edi + 8*ecx], mm0
		movq	[edi + 8*ecx + 8], mm3

		add ecx,2
		cmp ecx,16
		jnz	near .q1loop
		jmp	near .done


align ALIGN
.q2loop
		movq	mm0, [esi + 8*ecx]		; mm0 = [1st]
		movq	mm3, [esi + 8*ecx + 8]	; 

		pxor	mm1, mm1		; mm1 = 0
		pxor	mm4, mm4		;

		pcmpgtw	mm1, mm0		; mm1 = (0 > mm0)
		pcmpgtw	mm4, mm3		;
		 
		pxor	mm0, mm1		; mm0 = |mm0|
		pxor	mm3, mm4		; 
		psubw	mm0, mm1		; displace
		psubw	mm3, mm4		; 

		psllw   mm0, 4
		psllw   mm3, 4
		
		movq    mm2, [intra_matrix + 8*ecx]
		psrlw   mm2, 1
		paddw   mm0, mm2
		
		movq    mm2, [intra_matrix_fix + ecx*8]
		pmulhw  mm0, mm2		; (level<<4 + intra_matrix[i]>>1) / intra_matrix[i]

		movq    mm2, [intra_matrix + 8*ecx + 8]
		psrlw   mm2, 1
		paddw   mm3, mm2

		movq    mm2, [intra_matrix_fix + ecx*8 + 8]
		pmulhw  mm3, mm2

        paddw   mm0, mm5
		paddw   mm3, mm5

		psrlw	mm0, 2			; mm0 >>= 1   (/4)
		psrlw	mm3, 2			;
		
		pxor	mm0, mm1		; mm0 *= sign(mm0)
		pxor	mm3, mm4        ;
		psubw	mm0, mm1		; undisplace
		psubw	mm3, mm4		;
		
		movq	[edi + 8*ecx], mm0
		movq	[edi + 8*ecx + 8], mm3

		add ecx,2
		cmp ecx,16
		jnz	near .q2loop
		jmp	near .done


;===========================================================================
;
; uint32_t quant4_inter_mmx(int16_t * coeff,
;					const int16_t const * data,
;					const uint32_t quant);
;
;===========================================================================

align ALIGN
cglobal quant4_inter_mmx
		quant4_inter_mmx

		push	ecx
		push	esi
		push	edi

		mov	edi, [esp + 12 + 4]		; coeff
		mov	esi, [esp + 12 + 8]		; data
		mov	eax, [esp + 12 + 12]	; quant

		xor ecx, ecx

		pxor mm5, mm5					; sum

		cmp	al, 1
		jz  near .q1loop

		cmp	al, 2
		jz  near .q2loop

		movq	mm7, [mmx_div + eax * 8 - 8]	; divider

align ALIGN
.loop
		movq	mm0, [esi + 8*ecx]		; mm0 = [1st]
		movq	mm3, [esi + 8*ecx + 8]	; 
		pxor	mm1, mm1		; mm1 = 0
		pxor	mm4, mm4		;
		pcmpgtw	mm1, mm0		; mm1 = (0 > mm0)
		pcmpgtw	mm4, mm3		; 
		pxor	mm0, mm1		; mm0 = |mm0|
		pxor	mm3, mm4		; 
		psubw	mm0, mm1		; displace
		psubw	mm3, mm4		;

		psllw   mm0, 4
		psllw   mm3, 4
		
		movq    mm2, [inter_matrix + 8*ecx]
		psrlw   mm2, 1
		paddw   mm0, mm2
		
		movq    mm2, [inter_matrix_fix + ecx*8]
		pmulhw  mm0, mm2		; (level<<4 + inter_matrix[i]>>1) / inter_matrix[i]

		movq    mm2, [inter_matrix + 8*ecx + 8]
		psrlw   mm2, 1
		paddw   mm3, mm2

		movq    mm2, [inter_matrix_fix + ecx*8 + 8]
		pmulhw  mm3, mm2

		pmulhw	mm0, mm7		; mm0 = (mm0 / 2Q) >> 16
		pmulhw	mm3, mm7		; 
		psrlw   mm0, 1			; additional shift by 1 => 16 + 1 = 17
		psrlw   mm3, 1
		
		paddw	mm5, mm0		; sum += mm0
		pxor	mm0, mm1		; mm0 *= sign(mm0)
		paddw	mm5, mm3		;
		pxor	mm3, mm4		;
		psubw	mm0, mm1		; undisplace
		psubw	mm3, mm4
		movq	[edi + 8*ecx], mm0
		movq	[edi + 8*ecx + 8], mm3

		add ecx, 2	
		cmp ecx, 16
		jnz near .loop

.done
		pmaddwd mm5, [mmx_one]
		movq    mm0, mm5
		psrlq   mm5, 32
		paddd   mm0, mm5
		movd	eax, mm0		; return sum

		pop	edi
		pop	esi
		pop ecx

		ret

align ALIGN
.q1loop
		movq	mm0, [esi + 8*ecx]		; mm0 = [1st]
		movq	mm3, [esi + 8*ecx+ 8]
				; 
		pxor	mm1, mm1		; mm1 = 0
		pxor	mm4, mm4		;

		pcmpgtw	mm1, mm0		; mm1 = (0 > mm0)
		pcmpgtw	mm4, mm3		;

		pxor	mm0, mm1		; mm0 = |mm0|
		pxor	mm3, mm4		; 
		psubw	mm0, mm1		; displace
		psubw	mm3, mm4		;
		
		psllw   mm0, 4
		psllw   mm3, 4
		
		movq    mm2, [inter_matrix + 8*ecx]
		psrlw   mm2, 1
		paddw   mm0, mm2
		
		movq    mm2, [inter_matrix_fix + ecx*8]
		pmulhw  mm0, mm2		; (level<<4 + inter_matrix[i]>>1) / inter_matrix[i]

		movq    mm2, [inter_matrix + 8*ecx + 8]
		psrlw   mm2, 1
		paddw   mm3, mm2

		movq    mm2, [inter_matrix_fix + ecx*8 + 8]
		pmulhw  mm3, mm2
 
		psrlw	mm0, 1			; mm0 >>= 1   (/2)
		psrlw	mm3, 1			;

		paddw	mm5, mm0		; sum += mm0
		pxor	mm0, mm1		; mm0 *= sign(mm0)
		paddw	mm5, mm3		;
		pxor	mm3, mm4		;
		psubw	mm0, mm1		; undisplace
		psubw	mm3, mm4

		movq	[edi + 8*ecx], mm0
		movq	[edi + 8*ecx + 8], mm3
		
		add ecx,2
		cmp ecx,16
		jnz	near .q1loop

		jmp	.done


align ALIGN
.q2loop
		movq	mm0, [esi + 8*ecx]		; mm0 = [1st]
		movq	mm3, [esi + 8*ecx+ 8]
				; 
		pxor	mm1, mm1		; mm1 = 0
		pxor	mm4, mm4		;

		pcmpgtw	mm1, mm0		; mm1 = (0 > mm0)
		pcmpgtw	mm4, mm3		;

		pxor	mm0, mm1		; mm0 = |mm0|
		pxor	mm3, mm4		; 
		psubw	mm0, mm1		; displace
		psubw	mm3, mm4		;
		
		psllw   mm0, 4
		psllw   mm3, 4
		
		movq    mm2, [inter_matrix + 8*ecx]
		psrlw   mm2, 1
		paddw   mm0, mm2
		
		movq    mm2, [inter_matrix_fix + ecx*8]
		pmulhw  mm0, mm2		; (level<<4 + inter_matrix[i]>>1) / inter_matrix[i]

		movq    mm2, [inter_matrix + 8*ecx + 8]
		psrlw   mm2, 1
		paddw   mm3, mm2

		movq    mm2, [inter_matrix_fix + ecx*8 + 8]
		pmulhw  mm3, mm2
 
		psrlw	mm0, 2			; mm0 >>= 1   (/2)
		psrlw	mm3, 2			;

		paddw	mm5, mm0		; sum += mm0
		pxor	mm0, mm1		; mm0 *= sign(mm0)
		paddw	mm5, mm3		;
		pxor	mm3, mm4		;
		psubw	mm0, mm1		; undisplace
		psubw	mm3, mm4

		movq	[edi + 8*ecx], mm0
		movq	[edi + 8*ecx + 8], mm3
		
		add ecx,2
		cmp ecx,16
		jnz	near .q2loop

		jmp	.done


;===========================================================================
;
; void dequant4_intra_mmx(int16_t *data,
;                    const int16_t const *coeff,
;                    const uint32_t quant,
;                    const uint32_t dcscalar);
;
;===========================================================================

align 16
cglobal dequant4_intra_mmx
dequant4_intra_mmx

        push    esi
        push    edi

        mov    edi, [esp + 8 + 4]        ; data
        mov    esi, [esp + 8 + 8]        ; coeff
        mov    eax, [esp + 8 + 12]        ; quant
        
        movq mm7, [mmx_mul_quant  + eax*8 - 8]
    
        xor eax, eax


align 16        
.loop
        movq    mm0, [esi + 8*eax]        ; mm0 = [coeff]
        
        pxor    mm1, mm1                ; mm1 = 0
        pcmpeqw    mm1, mm0                ; mm1 = (0 == mm0)

        pxor    mm2, mm2                ; mm2 = 0
        pcmpgtw    mm2, mm0                ; mm2 = (0 > mm0)
        pxor    mm0, mm2                ; mm0 = |mm0|
        psubw    mm0, mm2                ; displace

        pmullw    mm0, mm7                ; mm0 *= quant
        
        movq    mm3, [intra_matrix + 8*eax]

        movq  mm4, mm0                    ;
        pmullw    mm0, mm3                ; mm0 = low(mm0 * mm3)
        pmulhw    mm3, mm4                ; mm3 = high(mm0 * mm3)

        movq    mm4, mm0                ; mm0,mm4 = unpack(mm3, mm0)
        punpcklwd mm0, mm3                ;
        punpckhwd mm4, mm3                ;
        psrld mm0, 3                    ; mm0,mm4 /= 8
        psrld mm4, 3                    ;
        packssdw mm0, mm4                ; mm0 = pack(mm4, mm0)

        pxor    mm0, mm2                ; mm0 *= sign(mm0)
        psubw    mm0, mm2                ; undisplace
        pandn    mm1, mm0                ; mm1 = ~(iszero) & mm0

%ifdef SATURATE
        movq mm2, [mmx_32767_minus_2047] 
        movq mm6, [mmx_32768_minus_2048] 
        paddsw    mm1, mm2
        psubsw    mm1, mm2
        psubsw    mm1, mm6
        paddsw    mm1, mm6
%endif

        movq    [edi + 8*eax], mm1        ; [data] = mm0

        add eax, 1
        cmp eax, 16
        jnz    near .loop

        mov    ax, [esi]                    ; ax = data[0]
        imul     ax, [esp + 8 + 16]        ; eax = data[0] * dcscalar
        mov    [edi], ax                    ; data[0] = ax

%ifdef SATURATE
        cmp ax, -2048
        jl .set_n2048
        cmp ax, 2047
        jg .set_2047
%endif

        pop    edi
        pop    esi
        ret

%ifdef SATURATE
.set_n2048
        mov    word [edi], -2048
        pop    edi
        pop    esi
        ret
    
.set_2047
        mov    word [edi], 2047
        pop    edi
        pop    esi

		ret
%endif



;===========================================================================
;
; void dequant4_inter_mmx(int16_t * data,
;                    const int16_t * const coeff,
;                    const uint32_t quant);
;
;===========================================================================

align 16
cglobal dequant4_inter_mmx
dequant4_inter_mmx

        push    esi
        push    edi
		
        mov    edi, [esp + 8 + 4]        ; data
        mov    esi, [esp + 8 + 8]        ; coeff
        mov    eax, [esp + 8 + 12]        ; quant
        movq mm7, [mmx_mul_quant  + eax*8 - 8]
        movq mm6, [mmx_one]
        xor eax, eax
        pxor mm5, mm5        ; mismatch sum


align 16        
.loop
        movq    mm0, [esi + 8*eax]                        ; mm0 = [coeff]

        pxor    mm1, mm1                ; mm1 = 0
        pcmpeqw    mm1, mm0                ; mm1 = (0 == mm0)

        pxor    mm2, mm2                ; mm2 = 0
        pcmpgtw    mm2, mm0                ; mm2 = (0 > mm0)
        pxor    mm0, mm2                ; mm0 = |mm0|
        psubw    mm0, mm2                ; displace

        psllw    mm0, 1                ;
        paddsw    mm0, mm6            ; mm0 = 2*mm0 + 1
        pmullw    mm0, mm7            ; mm0 *= quant

        movq    mm3, [inter_matrix + 8*eax]

        movq  mm4, mm0
        pmullw    mm0, mm3            ; mm0 = low(mm0 * mm3)
        pmulhw    mm3, mm4            ; mm3 = high(mm0 * mm3)

        movq    mm4, mm0            ; mm0,mm4 = unpack(mm3, mm0)
        punpcklwd mm0, mm3            ;
        punpckhwd mm4, mm3            ;

        psrad mm0, 4                ; mm0,mm4 /= 16
        psrad mm4, 4                ;
        packssdw mm0, mm4            ; mm0 = pack(mm4, mm0)

        pxor    mm0, mm2            ; mm0 *= sign(mm0)
        psubw    mm0, mm2            ; undisplace
        pandn    mm1, mm0            ; mm1 = ~(iszero) & mm0


;%ifdef SATURATE
        movq mm2, [mmx_32767_minus_2047] 
        movq mm4, [mmx_32768_minus_2048]
        paddsw    mm1, mm2
        psubsw    mm1, mm2
        psubsw    mm1, mm4
        paddsw    mm1, mm4
;%endif

        pxor mm5, mm1        ; mismatch
    
        movq    [edi + 8*eax], mm1        ; [data] = mm0

        add eax, 1
        cmp eax, 16
        jnz    near .loop

        ; mismatch control

        movq mm0, mm5
        movq mm1, mm5
        movq mm2, mm5
        psrlq mm0, 48
        psrlq mm1, 32
        psrlq mm2, 16
        pxor mm5, mm0
        pxor mm5, mm1
        pxor mm5, mm2

        movd eax, mm5
        test eax, 0x1
        jnz .done

        xor word [edi + 2*63], 1

.done    
        pop    edi
        pop    esi

        ret
