;/**************************************************************************
; *
; *	XVID MPEG-4 VIDEO CODEC
; *	mmx/xmm sum of absolute difference
; *
; *	This program is an implementation of a part of one or more MPEG-4
; *	Video tools as specified in ISO/IEC 14496-2 standard.  Those intending
; *	to use this software module in hardware or software products are
; *	advised that its use may infringe existing patents or copyrights, and
; *	any such use would be at such party's own risk.  The original
; *	developer of this software module and his/her company, and subsequent
; *	editors and their companies, will have no liability for use of this
; *	software or modifications or derivatives thereof.
; *
; *	This program is free software; you can redistribute it and/or modify
; *	it under the terms of the GNU General Public License as published by
; *	the Free Software Foundation; either version 2 of the License, or
; *	(at your option) any later version.
; *
; *	This program is distributed in the hope that it will be useful,
; *	but WITHOUT ANY WARRANTY; without even the implied warranty of
; *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; *	GNU General Public License for more details.
; *
; *	You should have received a copy of the GNU General Public License
; *	along with this program; if not, write to the Free Software
; *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
; *
; *************************************************************************/

;/**************************************************************************
; *
; *	History:
; *
; * 17.11.2001  bugfix and small improvement for dev16_xmm,
; *             removed terminate early in sad16_xmm 
; *	12.11.2001	inital version; (c)2001 peter ross <pross@cs.rmit.edu.au>
; *
; *************************************************************************/


bits 32

%macro cglobal 1 
	%ifdef PREFIX
		global _%1 
		%define %1 _%1
	%else
		global %1
	%endif
%endmacro

section .data


align 16
mmx_one	times 4	dw 1


section .text


;===========================================================================
;
; uint32_t sad16_mmx(const uint8_t * const cur,
;					const uint8_t * const ref,
;					const uint32_t stride,
;					const uint32_t best_sad);
;
; (early termination ignore; slows this down)
;
;===========================================================================

align 16
cglobal sad16_mmx
sad16_mmx
		push esi
		push edi

		mov esi, [esp + 8 + 4]	; ref
		mov edi, [esp + 8 + 8]	; cur
		mov ecx, [esp + 8 + 12]	; stride
		mov	edx, 16
		
		pxor mm6, mm6			; mm6 = sum = 0
		pxor mm7, mm7			; mm7 = 0
.loop
		movq mm0, [esi]	; ref
		movq mm1, [edi]	; cur

		movq mm2, [esi+8]	; ref2
		movq mm3, [edi+8]	; cur2

		movq mm4, mm0 
		movq mm5, mm2

		psubusb mm0, mm1
		psubusb mm2, mm3
		
		psubusb mm1, mm4
		psubusb mm3, mm5

		por mm0, mm1			; mm0 = |ref - cur|
		por mm2, mm3			; mm2 = |ref2 - cur2|

		movq mm1,mm0
		movq mm3,mm2

		punpcklbw mm0,mm7
		punpcklbw mm2,mm7

		punpckhbw mm1,mm7
		punpckhbw mm3,mm7

		paddusw mm0,mm1
		paddusw mm2,mm3
		
		paddusw mm6,mm0		; sum += mm01
		paddusw mm6,mm2		; sum += mm23

		add	esi, ecx
		add	edi, ecx
		dec	edx
		jnz	.loop

		pmaddwd mm6, [mmx_one]	; merge sum
		movq mm7, mm6
		psrlq mm7, 32 
		paddd mm6, mm7
		movd eax, mm6

		pop 	edi
		pop 	esi

		ret


;===========================================================================
;
; uint32_t sad16_xmm(const uint8_t * const cur,
;					const uint8_t * const ref,
;					const uint32_t stride,
;					const uint32_t best_sad);
;
; experimental!
;
;===========================================================================

align 16
cglobal sad16_xmm
sad16_xmm
		push esi
		push edi
		push ebx

		mov esi, [esp + 12 + 4]		; ref
		mov edi, [esp + 12 + 8]		; cur
		mov ecx, [esp + 12 + 12]	; stride
		mov ebx, [esp + 12 + 16]	; best_sad
		mov	edx, 16
		
		pxor mm6, mm6			; mm6 = sum = 0
.loop
		movq mm0, [esi]			; ref
		movq mm2, [esi+8]		; ref2

		psadbw mm0, [edi]		; mm0 = |ref - cur|
		psadbw mm2, [edi+8]		; mm0 = |ref2 - cur2|
		
		paddusw mm6,mm0			; sum += mm01
		paddusw mm6,mm2			; sum += mm23

		add	esi, ecx
		add	edi, ecx
		dec	edx
		jnz	.loop

		movd eax, mm6

.ret	pop		ebx
		pop 	edi
		pop 	esi

		ret



;===========================================================================
;
; uint32_t sad8_mmx(const uint8_t * const cur,
;					const uint8_t * const ref,
;					const uint32_t stride);
;
;===========================================================================
align 16
cglobal sad8_mmx
sad8_mmx
		push 	esi
		push 	edi

		mov 	esi, [esp + 8 + 4]	; ref
		mov 	edi, [esp + 8 + 8]	; cur
		mov 	ecx, [esp + 8 + 12]	; stride
		mov	eax, 4
		
		pxor mm6, mm6			; mm6 = sum = 0
		pxor mm7, mm7			; mm7 = 0
.loop
		movq mm0, [esi]	; ref
		movq mm1, [edi]	; cur

		movq mm2, [esi+ecx]	; ref2
		movq mm3, [edi+ecx]	; cur2

		movq mm4, mm0 
		movq mm5, mm2

		psubusb mm0, mm1
		psubusb mm2, mm3
		
		psubusb mm1, mm4
		psubusb mm3, mm5

		por mm0, mm1			; mm0 = |ref - cur|
		por mm2, mm3			; mm2 = |ref2 - cur2|

		movq mm1,mm0
		movq mm3,mm2

		punpcklbw mm0,mm7
		punpcklbw mm2,mm7

		punpckhbw mm1,mm7
		punpckhbw mm3,mm7

		paddusw mm0,mm1
		paddusw mm2,mm3
		
		paddusw mm6,mm0		; sum += mm01
		paddusw mm6,mm2		; sum += mm23

		add	esi, ecx
		add	edi, ecx
		add	esi, ecx
		add	edi, ecx
		dec	eax
		jnz	.loop

		pmaddwd mm6, [mmx_one]	; merge sum
		movq mm7, mm6
		psrlq mm7, 32 
		paddd mm6, mm7
		movd eax, mm6

		pop 	edi
		pop 	esi

		ret




;===========================================================================
;
; uint32_t sad8_xmm(const uint8_t * const cur,
;					const uint8_t * const ref,
;					const uint32_t stride);
;
; experimental!
;
;===========================================================================
align 16
cglobal sad8_xmm
sad8_xmm
		push 	esi
		push 	edi

		mov 	esi, [esp + 8 + 4]	; ref
		mov 	edi, [esp + 8 + 8]	; cur
		mov 	ecx, [esp + 8 + 12]	; stride
		mov     edx, ecx
		shl     edx, 1

		mov	eax, 4
		
		pxor mm6, mm6			; mm6 = sum = 0
.loop
		movq mm0, [esi]			; ref
		movq mm2, [esi+ecx]		; ref2

		psadbw mm0, [edi]		; mm0 = |ref - cur|
		psadbw mm2, [edi+ecx]	; mm0 = |ref2 - cur2|
		
		paddusw mm6,mm0			; sum += mm01
		paddusw mm6,mm2			; sum += mm23

		add	esi, edx
		add	edi, edx
		dec	eax
		jnz	.loop

		movd eax, mm6

		pop 	edi
		pop 	esi

		ret



;===========================================================================
;
; uint32_t dev16_mmx(const uint8_t * const cur,
;					const uint32_t stride);
;
;===========================================================================

align 16
cglobal dev16_mmx
dev16_mmx

		push 	esi
		push 	edi

		pxor mm4, mm4			; mm23 = sum = 0
		pxor mm5, mm5

		mov 	esi, [esp + 8 + 4]	; cur
		mov 	ecx, [esp + 8 + 8]	; stride
		mov     edi, esi

		mov	eax, 16
		pxor mm7, mm7			; mm7 = 0
.loop1
		movq mm0, [esi]
		movq mm2, [esi + 8]

		movq mm1, mm0
		movq mm3, mm2

		punpcklbw mm0, mm7
		punpcklbw mm2, mm7

		punpckhbw mm1, mm7		
		punpckhbw mm3, mm7		

		paddw mm0, mm1
		paddw mm2, mm3

		paddw mm4, mm0
		paddw mm5, mm2

		add	esi, ecx
		dec	eax
		jnz	.loop1

		paddusw	mm4, mm5
		pmaddwd mm4, [mmx_one]	; merge sum
		movq mm5, mm4
		psrlq mm5, 32 
		paddd mm4, mm5

		psllq mm4, 32			; blank upper dword
		psrlq mm4, 32 + 8		; mm4 /= (16*16)

		punpckldq mm4, mm4		
		packssdw mm4, mm4		; mm4 = mean

		pxor mm6, mm6			; mm6 = dev = 0
		mov	eax, 16
.loop2
		movq mm0, [edi]
		movq mm2, [edi + 8]

		movq mm1, mm0
		movq mm3, mm2

		punpcklbw mm0, mm7
		punpcklbw mm2, mm7

		punpckhbw mm1, mm7		; mm01 = cur
		punpckhbw mm3, mm7		; mm23 = cur2

		movq mm5, mm4			;
		psubusw mm5, mm0		;
		psubusw mm0, mm4		;
		por mm0, mm5			;
		movq mm5, mm4			;
		psubusw mm5, mm1		;
		psubusw mm1, mm4		;
		por mm1, mm5			; mm01 = |mm01 - mm4|


		movq mm5, mm4			;
		psubusw mm5, mm2		;
		psubusw mm2, mm4		;
		por mm2, mm5			;

		movq mm5, mm4			;
		psubusw mm5, mm3		;
		psubusw mm3, mm4		;
		por mm3, mm5			; mm23 = |mm23 - mm4|

		paddw mm0, mm1
		paddw mm2, mm3

		paddw mm6, mm0
		paddw mm6, mm2			; dev += mm01 + mm23

		add	edi, ecx
		dec	eax
		jnz	.loop2

		pmaddwd mm6, [mmx_one]	; merge dev
		movq mm7, mm6
		psrlq mm7, 32 
		paddd mm6, mm7
		movd eax, mm6

		pop 	edi
		pop 	esi

		ret



;===========================================================================
;
; uint32_t dev16_xmm(const uint8_t * const cur,
;					const uint32_t stride);
;
; experimental!
;
;===========================================================================

align 16
cglobal dev16_xmm
dev16_xmm

		push 	esi
		push 	edi

		pxor mm4, mm4			; mm23 = sum = 0

		mov 	esi, [esp + 8 + 4]	; cur
		mov 	ecx, [esp + 8 + 8]	; stride
		mov     edi, esi

		mov	eax, 16
		pxor mm7, mm7			; mm7 = 0
.loop1
		movq mm0, [esi]
		movq mm2, [esi + 8]

		psadbw mm0, mm7			; abs(cur0 - 0) + abs(cur1 - 0) + ... + abs(cur7 - 0) -> mm0
		psadbw mm2, mm7			; abs(cur8 - 0) + abs(cur9 - 0) + ... + abs(cur15 - 0) -> mm2

		paddw mm4,mm0			; mean += mm0
		paddw mm4,mm2			; mean += mm2

		add	esi, ecx
		dec	eax
		jnz	.loop1

		movq mm5, mm4
		psllq mm5, 32 
		paddd mm4, mm5

		psrld mm4, 8
		packssdw mm4, mm4
		packuswb mm4, mm4

		pxor mm6, mm6			; mm6 = dev = 0
		mov	eax, 16
.loop2
		movq mm0, [edi]
		movq mm2, [edi + 8]

		psadbw mm0, mm4			; mm0 = |cur - mean|
		psadbw mm2, mm4			; mm0 = |cur2 - mean|
	
		paddw mm6,mm0			; dev += mm01
		paddw mm6,mm2			; dev += mm23

		add	edi, ecx
		dec	eax
		jnz	.loop2

		movq mm7, mm6
		psllq mm7, 32 
		paddd mm6, mm7
		movd eax, mm6

		pop 	edi
		pop 	esi

		ret