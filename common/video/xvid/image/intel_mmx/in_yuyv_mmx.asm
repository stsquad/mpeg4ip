;/**************************************************************************
; *
; *	XVID MPEG-4 VIDEO CODEC
; *	mmx yuyv/uyvy to yuv planar
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
; *	30.11.2001	initial version; (c)2001 peter ross <pross@cs.rmit.edu.au>
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


section .data


;===========================================================================
; masks for extracting yuv components
;===========================================================================
;			y     u     y     v     y     u     y     v

mask1	db	0xff, 0,    0xff, 0,    0xff, 0,    0xff, 0
mask2	db	0,    0xff, 0,    0xff, 0,    0xff, 0,    0xff


section .text

;===========================================================================
;
;	void yuyv_to_yuv_mmx(uint8_t * const y_out,
;						uint8_t * const u_out,
;						uint8_t * const v_out,
;						const uint8_t * const src,
;						const uint32_t width,
;						const uint32_t height,
;						const uint32_t stride);
;
;	width must be multiple of 8
;	does not flip
;	~30% faster than plain c
;
;===========================================================================

align 16
cglobal yuyv_to_yuv_mmx
yuyv_to_yuv_mmx

		push ebx
		push ecx
		push esi
		push edi			
		push ebp		; STACK BASE = 20

		; some global consants

		mov ecx, [esp + 20 + 20]	; width
		mov eax, [esp + 20 + 28]	; stride
		sub eax, ecx				; eax = stride - width
		mov edx, eax
		add edx, [esp + 20 + 28]	; edx = y_dif + stride
		push edx					; [esp + 12] = y_dif

		shr eax, 1
		push eax					; [esp + 8] = uv_dif

		shr ecx, 3
		push ecx					; [esp + 4] = width/8

		sub	esp, 4					; [esp + 0] = tmp_height_counter
						; STACK_BASE = 36
		
		movq mm6, [mask1]
		movq mm7, [mask2]

		mov edi, [esp + 36 + 4]		; y_out
		mov ebx, [esp + 36 + 8]		; u_out
		mov edx, [esp + 36 + 12]	; v_out
		mov esi, [esp + 36 + 16]	; src

		mov eax, [esp + 36 + 20]
		mov ebp, [esp + 36 + 24]
		mov ecx, [esp + 36 + 28]	; ecx = stride
		shr ebp, 1					; ebp = height /= 2
		add eax, eax				; eax = 2 * width

.yloop
		mov [esp], ebp
		mov ebp, [esp + 4]			; width/8
.xloop
		movq mm2, [esi]				; y 1st row
		movq mm3, [esi + 8]
		movq mm0, mm2
		movq mm1, mm3
		pand mm2, mm6 ; mask1
		pand mm3, mm6 ; mask1
		pand mm0, mm7 ; mask2
		pand mm1, mm7 ; mask2
		packuswb mm2, mm3
		psrlq mm0, 8
		psrlq mm1, 8
		movq [edi], mm2

		movq mm4, [esi + eax]		; y 2nd row
		movq mm5, [esi + eax + 8]
		movq mm2, mm4
		movq mm3, mm5
		pand mm4, mm6 ; mask1
		pand mm5, mm6 ; mask1
		pand mm2, mm7 ; mask2
		pand mm3, mm7 ; mask2
		packuswb mm4, mm5
		psrlq mm2, 8
		psrlq mm3, 8
		movq [edi + ecx], mm4

		paddw mm0, mm2			; uv avg 1st & 2nd
		paddw mm1, mm3
		psrlw mm0, 1
		psrlw mm1, 1
		packuswb mm0, mm1
		movq mm2, mm0
		pand mm0, mm6 ; mask1
		pand mm2, mm7 ; mask2
		packuswb mm0, mm0
		psrlq mm2, 8
		movd [ebx], mm0
		packuswb mm2, mm2
		movd [edx], mm2

		add	esi, 16
		add	edi, 8
		add	ebx, 4
		add	edx, 4
		dec ebp
		jnz near .xloop

		mov ebp, [esp]

		add esi, eax			; += width2
		add edi, [esp + 12]		; += y_dif + stride
		add ebx, [esp + 8]		; += uv_dif
		add edx, [esp + 8]		; += uv_dif

		dec ebp
		jnz near .yloop

		emms

		add esp, 16
		pop ebp
		pop edi
		pop esi
		pop ecx
		pop ebx

		ret


;===========================================================================
;
;	void uyvy_to_yuv_mmx(uint8_t * const y_out,
;						uint8_t * const u_out,
;						uint8_t * const v_out,
;						const uint8_t * const src,
;						const uint32_t width,
;						const uint32_t height,
;						const uint32_t stride);
;
;	width must be multiple of 8
;	does not flip
;	~30% faster than plain c
;
;===========================================================================

align 16
cglobal uyvy_to_yuv_mmx
uyvy_to_yuv_mmx

		push ebx
		push ecx
		push esi
		push edi			
		push ebp		; STACK BASE = 20

		; some global consants

		mov ecx, [esp + 20 + 20]	; width
		mov eax, [esp + 20 + 28]	; stride
		sub eax, ecx				; eax = stride - width
		mov edx, eax
		add edx, [esp + 20 + 28]	; edx = y_dif + stride
		push edx					; [esp + 12] = y_dif

		shr eax, 1
		push eax					; [esp + 8] = uv_dif

		shr ecx, 3
		push ecx					; [esp + 4] = width/8

		sub	esp, 4					; [esp + 0] = tmp_height_counter
						; STACK_BASE = 36
		
		movq mm6, [mask1]
		movq mm7, [mask2]

		mov edi, [esp + 36 + 4]		; y_out
		mov ebx, [esp + 36 + 8]		; u_out
		mov edx, [esp + 36 + 12]	; v_out
		mov esi, [esp + 36 + 16]	; src

		mov eax, [esp + 36 + 20]
		mov ebp, [esp + 36 + 24]
		mov ecx, [esp + 36 + 28]	; ecx = stride
		shr ebp, 1					; ebp = height /= 2
		add eax, eax				; eax = 2 * width

.yloop
		mov [esp], ebp
		mov ebp, [esp + 4]			; width/8
.xloop
		movq mm2, [esi]				; y 1st row
		movq mm3, [esi + 8]
		movq mm0, mm2
		movq mm1, mm3
		pand mm2, mm7 ; mask2
		pand mm3, mm7 ; mask2
		psrlq mm2, 8
		psrlq mm3, 8
		pand mm0, mm6 ; mask1
		pand mm1, mm6 ; mask1
		packuswb mm2, mm3
		movq [edi], mm2

		movq mm4, [esi + eax]		; y 2nd row
		movq mm5, [esi + eax + 8]
		movq mm2, mm4
		movq mm3, mm5
		pand mm4, mm7 ; mask2
		pand mm5, mm7 ; mask2
		psrlq mm4, 8
		psrlq mm5, 8
		pand mm2, mm6 ; mask1
		pand mm3, mm6 ; mask1
		packuswb mm4, mm5
		movq [edi + ecx], mm4

		paddw mm0, mm2			; uv avg 1st & 2nd
		paddw mm1, mm3
		psrlw mm0, 1
		psrlw mm1, 1
		packuswb mm0, mm1
		movq mm2, mm0
		pand mm0, mm6 ; mask1
		pand mm2, mm7 ; mask2
		packuswb mm0, mm0
		psrlq mm2, 8
		movd [ebx], mm0
		packuswb mm2, mm2
		movd [edx], mm2

		add	esi, 16
		add	edi, 8
		add	ebx, 4
		add	edx, 4
		dec ebp
		jnz near .xloop

		mov ebp, [esp]

		add esi, eax			; += width2
		add edi, [esp + 12]		; += y_dif + stride
		add ebx, [esp + 8]		; += uv_dif
		add edx, [esp + 8]		; += uv_dif

		dec ebp
		jnz near .yloop

		emms

		add esp, 16
		pop ebp
		pop edi
		pop esi
		pop ecx
		pop ebx

		ret
