;/**************************************************************************
; *
; *	XVID MPEG-4 VIDEO CODEC
; *	mmx colorspace conversions
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
; * 24.11.2001  added cglobal macro (Isibaar)
; *	23.11.2001	initial version; (c)2001 peter ross <pross@cs.rmit.edu.au>
; *
; *************************************************************************/

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

align 16


;===========================================================================
; yuv constants
;===========================================================================

%define Y_R		0.257
%define Y_G		0.504
%define Y_B		0.098
%define Y_ADD	16

%define U_R		0.148
%define U_G		0.291
%define U_B		0.439
%define U_ADD	128

%define V_R		0.439
%define V_G		0.368
%define V_B		0.071
%define V_ADD	128


;===========================================================================
; multiplication matrices
;===========================================================================

; %define SCALEBITS 8

y_mul	dw		25		; FIX(Y_B)
		dw		129		; FIX(Y_G)
		dw		66		; FIX(Y_R)
		dw		0

u_mul	dw		112		; FIX(U_B)
		dw		-74		; FIX(U_G)
		dw		-38		; FIX(U_R)
		dw		0

v_mul	dw		-18		; FIX(V_B)
		dw		-94		; FIX(V_G)
		dw		112		; FIX(V_R)
		dw		0



section .text

;===========================================================================
;
;	void yuyv_to_rgb24_mmx(uint8_t * const y_out,
;						uint8_t * const u_out,
;						uint8_t * const v_out,
;						const uint8_t * const src,
;						const uint32_t width,
;						const uint32_t height,
;						const uint32_t stride)
;
; always flips
;
;===========================================================================

align 16
cglobal rgb24_to_yuv_mmx
rgb24_to_yuv_mmx

		push ebx
		push ecx
		push esi
		push edi			
		push ebp			; STACK BASE = 20

		; global consants

		mov eax, [esp + 20 + 28]	; stride
		mov ecx, [esp + 20 + 20]	; width
		mov ebx, eax
		sub ebx, ecx				
		shr ebx, 1					; ebx = (stride-width) / 2;
		push ebx					; [esp + 20] = uv_dif
							; STACK BASE = 24

		add eax, eax
		sub eax, ecx				; eax = 2*stride - width
		push eax					; [esp + 16] = y_dif
							; STACK BASE = 28

		mov ebx, ecx				; 
		shr ebx, 1					;
		push ebx					; [esp + 12] = width/2
							; STACK BASE = 32

		mov edx, ecx
		add ecx, edx
		add ecx, edx				; ecx = 3*width   (use 4 for rgb32)
		push ecx					; [esp + 8] = width3
							; STACK BASE = 36

		mov edx, ecx
		add edx, ecx
		add edx, ecx				; edx = 3*width3
		push edx					; [esp + 4] = src_dif
							; STACK BASE = 40

		mov esi, [esp + 40 + 16]	; src
		mov ebp, [esp + 40 + 24]	; eax = height
		mov eax, ebp
		sub eax, 2
		mul ecx
		add esi, eax				; src += (height-2) * width3

		mov edi, [esp + 40 + 4]		; y_out
		mov ecx, [esp + 40 + 8]		; u_out
		mov edx, [esp + 40 + 12]	; v_out
		movq mm7, [y_mul]		

		shr ebp, 1				; ebp = height / 2
		push ebp				; [esp+0] = tmp
								; STACK BASE = 44

.yloop
		mov ebp, [esp + 12]		; ebp = width /2 

.xloop
			; y_out

			mov ebx, [esp + 8]			; ebx = width3

			pxor mm4, mm4
			pxor mm5, mm5
			movd mm0, [esi]			; src[0...]
			movd mm2, [esi+ebx]		; src[width3...]
			punpcklbw mm0, mm4		; [  |b |g |r ]
			punpcklbw mm2, mm5		; [  |b |g |r ]
			movq mm6, mm0			; = [  |b4|g4|r4]
			paddw mm6, mm2			; +[  |b4|g4|r4]
			pmaddwd mm0, mm7		; *= Y_MUL
			pmaddwd mm2, mm7		; *= Y_MUL
			movq mm4, mm0			; [r]
			movq mm5, mm2			; [r]
			psrlq mm4, 32			; +[g]
			psrlq mm5, 32			; +[g]
			paddd mm0, mm4			; +[b]
			paddd mm2, mm5			; +[b]

			pxor mm4, mm4
			pxor mm5, mm5
			movd mm1, [esi+3]		; src[4...]
			movd mm3, [esi+ebx+3]	; src[width3+4...]
			punpcklbw mm1, mm4		; [  |b |g |r ]
			punpcklbw mm3, mm5		; [  |b |g |r ]
			paddw mm6, mm1			; +[  |b4|g4|r4]
			paddw mm6, mm3			; +[  |b4|g4|r4]
			pmaddwd mm1, mm7		; *= Y_MUL
			pmaddwd mm3, mm7		; *= Y_MUL
			movq mm4, mm1			; [r]
			movq mm5, mm3			; [r]
			psrlq mm4, 32			; +[g]
			psrlq mm5, 32			; +[g]
			paddd mm1, mm4			; +[b]
			paddd mm3, mm5			; +[b]

			mov ebx, [esp + 44 + 28]	; stride

			movd eax, mm0
			shr eax, 8
			add eax, Y_ADD
			mov [edi + ebx], al

			movd eax, mm1
			shr eax, 8
			add eax, Y_ADD
			mov [edi + ebx + 1], al

			movd eax, mm2
			shr eax, 8
			add eax, Y_ADD
			mov [edi], al

			movd eax, mm3
			shr eax, 8
			add eax, Y_ADD
			mov [edi + 1], al

			; u_out, v_out

			movq mm0, mm6			; = [  |b4|g4|r4]
			pmaddwd mm6, [v_mul]		; *= V_MUL
			pmaddwd mm0, [u_mul]		; *= U_MUL
			movq mm1, mm0
			movq mm2, mm6
			psrlq mm1, 32
			psrlq mm2, 32
			paddd mm0, mm1
			paddd mm2, mm6

			movd eax, mm0
			shr eax, 10
			add eax, U_ADD
			mov [ecx], al

			movd eax, mm2
			shr eax, 10
			add eax, V_ADD
			mov [edx], al

			add esi, 2 * 3			; (use 4 for rgb32)
			add edi, 2
			inc ecx
			inc edx

			dec ebp
			jnz near .xloop

		sub esi, [esp + 4]			; src  -= src_dif
		add edi, [esp + 16]			; y_out += y_dif
		add ecx, [esp + 20]			; u_out += uv_dif
		add edx, [esp + 20]			; v_out += uv_dif

		dec dword [esp+0]
		jnz near .yloop

		emms

		add esp, 24
		pop ebp
		pop edi
		pop esi
		pop ecx
		pop ebx

		ret



;===========================================================================
;
;	void yuyv_to_rgb32_mmx(uint8_t * const y_out,
;						uint8_t * const u_out,
;						uint8_t * const v_out,
;						const uint8_t * const src,
;						const uint32_t width,
;						const uint32_t height,
;						const uint32_t stride)
;
; always flips
;
;===========================================================================

align 16
cglobal rgb32_to_yuv_mmx
rgb32_to_yuv_mmx

		push ebx
		push ecx
		push esi
		push edi			
		push ebp			; STACK BASE = 20

		; global consants

		mov eax, [esp + 20 + 28]	; stride
		mov ecx, [esp + 20 + 20]	; width
		mov ebx, eax
		sub ebx, ecx				
		shr ebx, 1					; ebx = (stride-width) / 2;
		push ebx					; [esp + 20] = uv_dif
							; STACK BASE = 24

		add eax, eax
		sub eax, ecx				; eax = 2*stride - width
		push eax					; [esp + 16] = y_dif
							; STACK BASE = 28

		mov ebx, ecx				; 
		shr ebx, 1					;
		push ebx					; [esp + 12] = width/2
							; STACK BASE = 32

		mov edx, ecx
		shl ecx, 2					; ecx = 4*width   (use 4 for rgb32)
		push ecx					; [esp + 8] = width4
							; STACK BASE = 36

		mov edx, ecx
		add edx, ecx
		add edx, ecx				; edx = 3*width4
		push edx					; [esp + 4] = src_dif
							; STACK BASE = 40

		mov esi, [esp + 40 + 16]	; src
		mov ebp, [esp + 40 + 24]	; eax = height
		mov eax, ebp
		sub eax, 2
		mul ecx
		add esi, eax				; src += (height-2) * width4

		mov edi, [esp + 40 + 4]		; y_out
		mov ecx, [esp + 40 + 8]		; u_out
		mov edx, [esp + 40 + 12]	; v_out
		movq mm7, [y_mul]		

		shr ebp, 1				; ebp = height / 2
		push ebp				; [esp+0] = tmp
								; STACK BASE = 44

.yloop
		mov ebp, [esp + 12]		; ebp = width /2 

.xloop
			; y_out

			mov ebx, [esp + 8]			; ebx = width4

			pxor mm4, mm4
			movq mm0, [esi]			; src[4...       |0...     ]
			movq mm2, [esi+ebx]		; src[width4+4...|width4...]
			movq mm1, mm0
			movq mm3, mm2
			punpcklbw mm0, mm4		; [  |b |g |r ]
			punpcklbw mm2, mm4		; [  |b |g |r ]
			punpckhbw mm1, mm4		; [  |b |g |r ]
			punpckhbw mm3, mm4		; [  |b |g |r ]

			movq mm6, mm0			; = [  |b4|g4|r4]
			paddw mm6, mm2			; +[  |b4|g4|r4]
			pmaddwd mm0, mm7		; *= Y_MUL
			pmaddwd mm2, mm7		; *= Y_MUL
			movq mm4, mm0			; [r]
			movq mm5, mm2			; [r]
			psrlq mm4, 32			; +[g]
			psrlq mm5, 32			; +[g]
			paddd mm0, mm4			; +[b]
			paddd mm2, mm5			; +[b]

			paddw mm6, mm1			; +[  |b4|g4|r4]
			paddw mm6, mm3			; +[  |b4|g4|r4]
			pmaddwd mm1, mm7		; *= Y_MUL
			pmaddwd mm3, mm7		; *= Y_MUL
			movq mm4, mm1			; [r]
			movq mm5, mm3			; [r]
			psrlq mm4, 32			; +[g]
			psrlq mm5, 32			; +[g]
			paddd mm1, mm4			; +[b]
			paddd mm3, mm5			; +[b]

			mov ebx, [esp + 44 + 28]	; stride

			movd eax, mm0
			shr eax, 8
			add eax, Y_ADD
			mov [edi + ebx], al

			movd eax, mm1
			shr eax, 8
			add eax, Y_ADD
			mov [edi + ebx + 1], al

			movd eax, mm2
			shr eax, 8
			add eax, Y_ADD
			mov [edi], al

			movd eax, mm3
			shr eax, 8
			add eax, Y_ADD
			mov [edi + 1], al

			; u_out, v_out

			movq mm0, mm6			; = [  |b4|g4|r4]
			pmaddwd mm6, [v_mul]		; *= V_MUL
			pmaddwd mm0, [u_mul]		; *= U_MUL
			movq mm1, mm0
			movq mm2, mm6
			psrlq mm1, 32
			psrlq mm2, 32
			paddd mm0, mm1
			paddd mm2, mm6

			movd eax, mm0
			shr eax, 10
			add eax, U_ADD
			mov [ecx], al

			movd eax, mm2
			shr eax, 10
			add eax, V_ADD
			mov [edx], al

			add esi, 2 * 4			; (use 4 for rgb32)
			add edi, 2
			inc ecx
			inc edx

			dec ebp
			jnz near .xloop

		sub esi, [esp + 4]			; src  -= src_dif
		add edi, [esp + 16]			; y_out += y_dif
		add ecx, [esp + 20]			; u_out += uv_dif
		add edx, [esp + 20]			; v_out += uv_dif

		dec dword [esp+0]
		jnz near .yloop

		emms

		add esp, 24
		pop ebp
		pop edi
		pop esi
		pop ecx
		pop ebx

		ret
