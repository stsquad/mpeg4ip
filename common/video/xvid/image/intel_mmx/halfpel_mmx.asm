;/**************************************************************************
; *
; *	XVID MPEG-4 VIDEO CODEC
; *	mmx/3dnow halfpel interpolation
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
; *	16.11.2001	reordered instructions
; *	05.11.2001	inital version; (c)2001 peter ross <pross@cs.rmit.edu.au>
; *				note: assumes src is allocated atleast width*height+1 pixels
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


;===========================================================================
; (1 - r) rounding table
;===========================================================================

rounding1_mmx
times 4 dw 1
times 4 dw 0



;===========================================================================
; (2 - r) rounding table  
;===========================================================================

rounding2_mmx
times 4 dw 2
times 4 dw 1



section .text


;===========================================================================
;
; void interpolate_halfpel_h_3dn(uint8_t * const dst,
;						const uint8_t * const src,
;						const uint32_t width,
;						const uint32_t height,
;						const uint32_t rounding);
;
;===========================================================================

align 16
cglobal interpolate_halfpel_h_3dn
interpolate_halfpel_h_3dn
		push	esi
		push	edi

		mov	eax, [esp + 8 + 20]		; rounding
		
		or al,al
		jnz interpolate_halfpel_h_mmx.start
		
		; pavgusb only works when rounding = 0

		mov	edi, [esp + 8 + 4]		; dst
		mov	esi, [esp + 8 + 8]		; src

		mov eax, [esp + 8 + 12]
		mul dword [esp + 8 + 16]	
		shr eax, 4					; eax = width * height / 16
		
.loop
		movq mm0, [esi]
		movq mm1, [esi + 8]
		pavgusb mm0, [esi + 1]		; mm0 = avg([src], [src+1])
		pavgusb mm1, [esi + 9]		; mm1 = avg([src+8], [src+9])
		movq [edi], mm0
		movq [edi + 8], mm1

		add esi, 16
		add edi, 16
		dec eax
		jnz .loop

		pop edi
		pop esi

		ret



;===========================================================================
;
; void interpolate_halfpel_h_mmx(uint8_t * const dst,
;						const uint8_t * const src,
;						const uint32_t width,
;						const uint32_t height,
;						const uint32_t rounding);
;
;===========================================================================

align 16
cglobal interpolate_halfpel_h_mmx
interpolate_halfpel_h_mmx

		push	esi
		push	edi

		mov	eax, [esp + 8 + 20]		; rounding

interpolate_halfpel_h_mmx.start
		movq mm7, [rounding1_mmx + eax * 8]

		mov	edi, [esp + 8 + 4]		; dst
		mov	esi, [esp + 8 + 8]		; src

		pxor	mm6, mm6		; zero
		
		mov eax, [esp + 8 + 12]		
		mul dword [esp + 8 + 16]	
		shr eax, 3				; eax = width * height / 8
		
.loop
		movq mm0, [esi]
		movq mm2, [esi + 1]
		movq mm1, mm0
		movq mm3, mm2
		punpcklbw mm0, mm6		; mm01 = [src]
		punpcklbw mm2, mm6		; mm23 = [src + 1]
		punpckhbw mm1, mm6
		punpckhbw mm3, mm6

		paddusw mm0, mm2		; mm01 += mm23
		paddusw mm1, mm3
		paddusw mm0, mm7		; mm01 += rounding
		paddusw mm1, mm7
		
		psrlw mm0, 1			; mm01 >>= 1
		psrlw mm1, 1

		packuswb mm0, mm1
		movq [edi], mm0			; [dst] = mm01

		add esi, 8
		add edi, 8
		dec eax
		jnz .loop

		pop edi
		pop esi

		ret



;===========================================================================
;
; void interpolate_halfpel_v_3dn(uint8_t * const dst,
;						const uint8_t * const src,
;						const uint32_t width,
;						const uint32_t height, 
;						const uint32_t rounding);
;
;===========================================================================

align 16
cglobal interpolate_halfpel_v_3dn
interpolate_halfpel_v_3dn

		push	esi
		push	edi

		mov	eax, [esp + 8 + 20]				; rounding
		
		or al,al
		jnz interpolate_halfpel_v_mmx.start	
		
		; pavgusb only works when rounding = 0

		mov	edi, [esp + 8 + 4]		; dst
		mov	esi, [esp + 8 + 8]		; src

		mov eax, [esp + 8 + 16]
		dec eax						; eax = height - 1 
		mov ecx, [esp + 8 + 12]		; width
		mul ecx
		shr eax, 4		; eax = width * (height-1) / 16

.loop
		movq mm0, [esi]
		movq mm1, [esi + 8]
		pavgusb mm0, [esi + ecx]		; mm0 = avg([src], [src+width])
		pavgusb mm1, [esi + ecx + 8]	; mm1 = avg([src+8], [src+width+8])
		movq [edi], mm0
		movq [edi + 8], mm1

		add esi, 16
		add edi, 16
		dec eax
		jnz .loop

		pop edi
		pop esi

		ret



;===========================================================================
;
; void interpolate_halfpel_v_mmx(uint8_t * const dst,
;						const uint8_t * const src,
;						const uint32_t width,
;						const uint32_t height, 
;						const uint32_t rounding);
;
;===========================================================================

align 16
cglobal interpolate_halfpel_v_mmx
interpolate_halfpel_v_mmx

		push	esi
		push	edi

		mov	eax, [esp + 8 + 20]				; rounding

interpolate_halfpel_v_mmx.start
		movq mm7, [rounding1_mmx + eax * 8]

		mov	edi, [esp + 8 + 4]		; dst
		mov	esi, [esp + 8 + 8]		; src

		pxor	mm6, mm6		; zero
		
		mov eax, [esp + 8 + 16]
		dec eax						; eax = height - 1 
		mov ecx, [esp + 8 + 12]		; width
		mul ecx
		shr eax, 3		; eax = width * (height-1) / 8

.loop
		movq mm0, [esi]
		movq mm2, [esi + ecx]

		movq mm1, mm0
		movq mm3, mm2

		punpcklbw mm0, mm6		; mm01 = [src]
		punpcklbw mm2, mm6		; mm23 = [src + width]
		punpckhbw mm1, mm6
		punpckhbw mm3, mm6

		paddusw mm0, mm2		; mm01 += mm23
		paddusw mm1, mm3
		paddusw mm0, mm7		; mm01 += rounding
		paddusw mm1, mm7
		
		psrlw mm0, 1			; mm01 >>= 1
		psrlw mm1, 1

		packuswb mm0, mm1
		movq [edi], mm0			; [dst] = mm01

		add esi, 8
		add edi, 8
		dec eax
		jnz .loop

		pop edi
		pop esi

		ret



;===========================================================================
;
; void interpolate_halfpel_hv_mmx(uint8_t * const dst,
;						const uint8_t * const src,
;						const uint32_t width,
;						const uint32_t height, 
;						const uint32_t rounding);
;
; 3dnow(pavgusb) is no help to us here!
;
;===========================================================================

align 16
cglobal interpolate_halfpel_hv_mmx
interpolate_halfpel_hv_mmx

		push	esi
		push	edi

		mov	eax, [esp + 8 + 20]		; rounding
		movq mm7, [rounding2_mmx + eax * 8]

		mov	edi, [esp + 8 + 4]		; dst
		mov	esi, [esp + 8 + 8]		; src

		pxor	mm6, mm6		; zero
		
		mov eax, [esp + 8 + 16]
		dec eax						; eax = height - 1 
		mov ecx, [esp + 8 + 12]		; width
		mul ecx
		shr eax, 3					; eax = width * (height - 1) / 8

.loop
		; current row

		movq mm0, [esi]
		movq mm2, [esi + 1]

		movq mm1, mm0
		movq mm3, mm2

		punpcklbw mm0, mm6		; mm01 = [src]
		punpcklbw mm2, mm6		; mm23 = [src + 1]
		punpckhbw mm1, mm6
		punpckhbw mm3, mm6

		paddusw mm0, mm2		; mm01 += mm23
		paddusw mm1, mm3

		; next row

		movq mm4, [esi + ecx]
		movq mm2, [esi + ecx + 1]
		
		movq mm5, mm4
		movq mm3, mm2
		
		punpcklbw mm4, mm6		; mm45 = [src + width]
		punpcklbw mm2, mm6		; mm23 = [src + width + 1]
		punpckhbw mm5, mm6
		punpckhbw mm3, mm6

		paddusw mm4, mm2		; mm45 += mm23
		paddusw mm5, mm3

		; add current + next row

		paddusw mm0, mm4		; mm01 += mm45
		paddusw mm1, mm5
		paddusw mm0, mm7		; mm01 += rounding2
		paddusw mm1, mm7
		
		psrlw mm0, 2			; mm01 >>= 2
		psrlw mm1, 2

		packuswb mm0, mm1
		movq [edi], mm0			; [dst] = mm01

		add esi, 8
		add edi, 8
		dec eax
		jnz .loop

		pop edi
		pop esi

		ret