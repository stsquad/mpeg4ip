;/**************************************************************************
; *
; *	XVID MPEG-4 VIDEO CODEC
; *	mmx 8bit<->16bit transfers
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
; * 07.01.2002	merge functions from compensate_mmx; rename functions
; *	07.11.2001	initial version; (c)2001 peter ross <pross@cs.rmit.edu.au>
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


section .text


;===========================================================================
;
; void transfer_8to16copy_mmx(int16_t * const dst,
;							const uint8_t * const src,
;							uint32_t stride);
;
;===========================================================================

align 16
cglobal transfer_8to16copy_mmx
transfer_8to16copy_mmx

		push	esi
		push	edi

		mov	edi, [esp + 8 + 4]		; dst
		mov	esi, [esp + 8 + 8]		; src
		mov ecx, [esp + 8 + 12]		; stride

		pxor mm7, mm7				; mm7 = zero
			
		mov eax, 8

.loop
		movq mm0, [esi]
		movq mm1, mm0
		punpcklbw mm0, mm7		; mm01 = unpack([src])
		punpckhbw mm1, mm7
		
		movq [edi], mm0			; [dst] = mm01
		movq [edi + 8], mm1

		add edi, 16
		add esi, ecx
		dec eax
		jnz .loop

		pop edi
		pop esi

		ret



;===========================================================================
;
; void transfer_16to8copy_mmx(uint8_t * const dst,
;							const int16_t * const src,
;							uint32_t stride);
;
;===========================================================================

align 16
cglobal transfer_16to8copy_mmx
transfer_16to8copy_mmx

		push	esi
		push	edi

		mov	edi, [esp + 8 + 4]		; dst
		mov	esi, [esp + 8 + 8]		; src
		mov ecx, [esp + 8 + 12]		; stride

		mov eax, 8

.loop
		movq mm0, [esi]
		packuswb mm0, [esi + 8]		; mm0 = pack([src])
		
		movq [edi], mm0				; [dst] = mm0

		add esi, 16
		add edi, ecx
		dec eax
		jnz .loop

		pop edi
		pop esi

		ret


;===========================================================================
;
; void transfer_8to16sub_mmx(int16_t * const dct,
;				uint8_t * const cur,
;				const uint8_t * const ref,
;				const uint32_t stride);
;
;===========================================================================
;/**************************************************************************
; *
; *	History:
; *
; * 27.12.2001	renamed from 'compensate' to 'transfer_8to16sub'
; * 02.12.2001  loop unrolled, code runs 10% faster now (Isibaar)
; * 30.11.2001  16 pixels are processed per iteration (Isibaar)
; * 30.11.2001	.text missing
; *	06.11.2001	inital version; (c)2001 peter ross <pross@cs.rmit.edu.au>
; *
; *************************************************************************/

align 16
cglobal transfer_8to16sub_mmx
transfer_8to16sub_mmx
		push	esi
		push	edi
		push    ebx

		mov	edi, [esp + 12 + 4]		; dct [out]
		mov	edx, [esp + 12 + 8]		; cur [in/out]
		mov	esi, [esp + 12 + 12]		; ref [in]
		mov ecx, [esp + 12 + 16]		; stride [in]

		mov eax, edx				; cur -> eax
		mov ebx, esi				; ref -> ebx
		add eax, ecx				; cur + stride
		add ebx, ecx				; ref + stride
		
		shl ecx, 1
				
		pxor mm7, mm7			; mm7 = zero

		movq mm0, [edx]			; mm01 = [cur]
		movq mm1, mm0
		
		punpcklbw mm0, mm7
		punpckhbw mm1, mm7
		
		movq mm4, [eax]
		movq mm5, mm4

		punpcklbw mm4, mm7
		punpckhbw mm5, mm7

		movq mm2, [esi]			; mm23 = [ref]
		movq mm3, mm2

		movq mm6, [ebx]

		movq [edx], mm2			; [cur] = [ref]
		movq [eax], mm6

		punpcklbw mm2, mm7
		punpckhbw mm3, mm7

		psubsw mm0, mm2			; mm01 -= mm23

		movq mm2, mm6

		punpcklbw mm2, mm7
		punpckhbw mm6, mm7

		psubsw mm1, mm3

		psubsw mm4, mm2
		psubsw mm5, mm6

		movq [edi], mm0			; dct[] = mm01
		movq [edi + 8], mm1
		movq [edi + 16], mm4
		movq [edi + 24], mm5

		add edx, ecx
		add esi, ecx
		add eax, ecx
		add ebx, ecx

		movq mm0, [edx]			; mm01 = [cur]
		movq mm1, mm0
		
		punpcklbw mm0, mm7
		punpckhbw mm1, mm7
		
		movq mm4, [eax]
		movq mm5, mm4

		punpcklbw mm4, mm7
		punpckhbw mm5, mm7

		movq mm2, [esi]			; mm23 = [ref]
		movq mm3, mm2

		movq mm6, [ebx]

		movq [edx], mm2			; [cur] = [ref]
		movq [eax], mm6

		punpcklbw mm2, mm7
		punpckhbw mm3, mm7

		psubsw mm0, mm2			; mm01 -= mm23

		movq mm2, mm6

		punpcklbw mm2, mm7
		punpckhbw mm6, mm7

		psubsw mm1, mm3

		psubsw mm4, mm2
		psubsw mm5, mm6

		movq [edi + 32], mm0			; dct[] = mm01
		movq [edi + 40], mm1
		movq [edi + 48], mm4
		movq [edi + 56], mm5

		add edx, ecx
		add esi, ecx
		add eax, ecx
		add ebx, ecx

		movq mm0, [edx]			; mm01 = [cur]
		movq mm1, mm0
		
		punpcklbw mm0, mm7
		punpckhbw mm1, mm7
		
		movq mm4, [eax]
		movq mm5, mm4

		punpcklbw mm4, mm7
		punpckhbw mm5, mm7

		movq mm2, [esi]			; mm23 = [ref]
		movq mm3, mm2

		movq mm6, [ebx]

		movq [edx], mm2			; [cur] = [ref]
		movq [eax], mm6

		punpcklbw mm2, mm7
		punpckhbw mm3, mm7

		psubsw mm0, mm2			; mm01 -= mm23

		movq mm2, mm6

		punpcklbw mm2, mm7
		punpckhbw mm6, mm7

		psubsw mm1, mm3

		psubsw mm4, mm2
		psubsw mm5, mm6

		movq [edi + 64], mm0			; dct[] = mm01
		movq [edi + 72], mm1
		movq [edi + 80], mm4
		movq [edi + 88], mm5

		add edx, ecx
		add esi, ecx
		add eax, ecx
		add ebx, ecx

		movq mm0, [edx]			; mm01 = [cur]
		movq mm1, mm0
		
		punpcklbw mm0, mm7
		punpckhbw mm1, mm7
		
		movq mm4, [eax]
		movq mm5, mm4

		punpcklbw mm4, mm7
		punpckhbw mm5, mm7

		movq mm2, [esi]			; mm23 = [ref]
		movq mm3, mm2

		movq mm6, [ebx]

		movq [edx], mm2			; [cur] = [ref]
		movq [eax], mm6

		punpcklbw mm2, mm7
		punpckhbw mm3, mm7

		psubsw mm0, mm2			; mm01 -= mm23

		movq mm2, mm6

		punpcklbw mm2, mm7
		punpckhbw mm6, mm7

		psubsw mm1, mm3

		psubsw mm4, mm2
		psubsw mm5, mm6

		movq [edi + 96], mm0			; dct[] = mm01
		movq [edi + 104], mm1
		movq [edi + 112], mm4
		movq [edi + 120], mm5

		pop ebx
		pop edi
		pop esi

		ret



;===========================================================================
;
; void transfer_16to8add_mmx(uint8_t * const dst,
;						const int16_t * const src,
;						uint32_t stride);
;
;===========================================================================

align 16
cglobal transfer_16to8add_mmx
transfer_16to8add_mmx

		push	esi
		push	edi

		mov	edi, [esp + 8 + 4]		; dst
		mov	esi, [esp + 8 + 8]		; src
		mov ecx, [esp + 8 + 12]		; stride

		pxor mm7, mm7

		mov eax, 8

.loop
		movq mm0, [edi]			
		movq mm1, mm0
		punpcklbw mm0, mm7		; mm23 = unpack([dst])
		punpckhbw mm1, mm7

		movq mm2, [esi]			; mm01 = [src]
		movq mm3, [esi + 8]

		paddsw mm0, mm2			; mm01 += mm23
		paddsw mm1, mm3

		packuswb mm0, mm1		; [dst] = pack(mm01)
		movq [edi], mm0

		add esi, 16
		add edi, ecx
		dec eax
		jnz .loop

		pop edi
		pop esi

		ret


;===========================================================================
;
; void transfer8x8_copy_mmx(uint8_t * const dst,
;					const uint8_t * const src,
;					const uint32_t stride);
;
;
;===========================================================================

align 16
cglobal transfer8x8_copy_mmx
transfer8x8_copy_mmx
		push	esi
		push	edi

		mov	edi, [esp + 8 + 4]		; dst [out]
		mov	esi, [esp + 8 + 8]		; src [in]
		mov eax, [esp + 8 + 12]		; stride [in]

		movq mm0, [esi]
		movq mm1, [esi+eax]
		movq [edi], mm0
		movq [edi+eax], mm1
		
		add esi, eax
		add edi, eax
		add esi, eax
		add edi, eax

		movq mm0, [esi]
		movq mm1, [esi+eax]
		movq [edi], mm0
		movq [edi+eax], mm1
		
		add esi, eax
		add edi, eax
		add esi, eax
		add edi, eax

		movq mm0, [esi]
		movq mm1, [esi+eax]
		movq [edi], mm0
		movq [edi+eax], mm1
		
		add esi, eax
		add edi, eax
		add esi, eax
		add edi, eax

		movq mm0, [esi]
		movq mm1, [esi+eax]
		movq [edi], mm0
		movq [edi+eax], mm1
		
		add esi, eax
		add edi, eax
		add esi, eax
		add edi, eax

		pop edi
		pop esi

		ret
