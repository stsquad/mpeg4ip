;/**************************************************************************
; *
; *	XVID MPEG-4 VIDEO CODEC
; *	mmx yuv planar to yuyv/uyvy
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
; *	05.12.2001	initial version; (c)2001 peter ross <pross@cs.rmit.edu.au>
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


section .text


;===========================================================================
;
; void yuv_to_uyvy_mmx(
;				uint8_t * dst,
;				int dst_stride,
;				uint8_t * y_src,
;				uint8_t * u_src,
;				uint8_t * v_src,
;				int y_stride,
;				int uv_stride,
;				int width,
;				int height);
;
;	width must be multiple of 8
;	~10% faster than plain c
;
;===========================================================================

align 16
cglobal yuv_to_yuyv_mmx
yuv_to_yuyv_mmx

		push ebx
		push ecx
		push esi
		push edi			
		push ebp		; STACK BASE = 20

		; global constants

		mov ebx, [esp + 20 + 32]	; width
		mov eax, [esp + 20 + 8]		; dst_stride
		sub eax, ebx				; 
		add eax, eax				; eax = 2*(dst_stride - width)
		push eax					; [esp + 4] = dst_dif
						; STACK BASE = 24

		shr ebx, 3					; ebx = width / 8
		mov edi, [esp + 24 + 4]		; dst


		; --------- flip -------------

		mov	ebp, [esp + 24 + 36]
		test ebp, ebp
		jl .flip

		mov esi, [esp + 24 + 12]	; y_src
		mov ecx, [esp + 24 + 16]	; u_src
		mov edx, [esp + 24 + 20]	; v_src
		shr ebp, 1					; y = height / 2
		jmp short .yloop


.flip
		neg ebp			; height = -height
		
		mov	eax, [esp + 24 + 24]	; y_stride
		lea	edx, [ebp - 1]			; edx = height - 1
		mul	edx
		mov esi, [esp + 24 + 12]	; y_src
		add esi, eax				; y_src += (height - 1) * y_stride

		shr ebp, 1					; y = height / 2
		mov	eax, [esp + 24 + 28]	; uv_stride
		lea	edx, [ebp - 1]			; edx = height/2 - 1
		mul	edx

		mov ecx, [esp + 24 + 16]	; u_src
		mov edx, [esp + 24 + 20]	; v_src
		add ecx, eax				; u_src += (height/2 - 1) * uv_stride
		add edx, eax				; v_src += (height/2 - 1) * uv_stride

		neg	dword [esp + 24 + 24]	; y_stride = -y_stride
		neg dword [esp + 24 + 28]	; uv_stride = -uv_stride
	
.yloop
		xor eax, eax			; x = 0;

.xloop1
				movd mm0, [ecx+4*eax]		; [    |uuuu]
				movd mm1, [edx+4*eax]		; [    |vvvv]
				movq mm2, [esi+8*eax]		; [yyyy|yyyy]

				punpcklbw mm0, mm1			; [vuvu|vuvu]
				movq mm3, mm2
				punpcklbw mm2, mm0			; [vyuy|vyuy]
				punpckhbw mm3, mm0			; [vyuy|vyuy]
				movq [edi], mm2
				movq [edi+8], mm3

				inc eax
				add edi, 16
						
				cmp eax, ebx
				jb	.xloop1

		add edi, [esp + 0]		; dst += dst_dif
		add esi, [esp + 24 + 24]	; y_src += y_stride
		
		xor eax, eax

.xloop2
				movd mm0, [ecx+4*eax]		; [    |uuuu]
				movd mm1, [edx+4*eax]		; [    |vvvv]
				movq mm2, [esi+8*eax]		; [yyyy|yyyy]

				punpcklbw mm0, mm1			; [vuvu|vuvu]
				movq mm3, mm2
				punpcklbw mm2, mm0			; [vyuy|vyuy]
				punpckhbw mm3, mm0			; [vyuy|vyuy]
				movq [edi], mm2
				movq [edi+8], mm3

				inc eax
				add edi, 16
										
				cmp eax, ebx
				jb	.xloop2

		add edi, [esp + 0]			; dst += dst_dif
		add esi, [esp + 24 + 24]	; y_src += y_stride
		add ecx, [esp + 24 + 28]	; u_src += uv_stride
		add edx, [esp + 24 + 28]	; v_src += uv_stride

		dec	ebp				; y--
		jnz	near .yloop

		emms

		add esp, 4
		pop ebp
		pop edi
		pop esi
		pop ecx
		pop ebx

		ret





;===========================================================================
;
; void yuv_to_uyvy_mmx(
;				uint8_t * dst,
;				int dst_stride,
;				uint8_t * y_src,
;				uint8_t * u_src,
;				uint8_t * v_src,
;				int y_stride,
;				int uv_stride,
;				int width,
;				int height);
;
;	width must be multiple of 8
;	~10% faster than plain c
;
;===========================================================================

align 16
cglobal yuv_to_uyvy_mmx
yuv_to_uyvy_mmx

		push ebx
		push ecx
		push esi
		push edi			
		push ebp		; STACK BASE = 20

		; global constants

		mov ebx, [esp + 20 + 32]	; width
		mov eax, [esp + 20 + 8]		; dst_stride
		sub eax, ebx				; 
		add eax, eax				; eax = 2*(dst_stride - width)
		push eax					; [esp + 4] = dst_dif
						; STACK BASE = 24

		shr ebx, 3					; ebx = width / 8
		mov edi, [esp + 24 + 4]		; dst


		; --------- flip -------------

		mov	ebp, [esp + 24 + 36]
		test ebp, ebp
		jl .flip

		mov esi, [esp + 24 + 12]	; y_src
		mov ecx, [esp + 24 + 16]	; u_src
		mov edx, [esp + 24 + 20]	; v_src
		shr ebp, 1					; y = height / 2
		jmp short .yloop


.flip
		neg ebp			; height = -height
		
		mov	eax, [esp + 24 + 24]	; y_stride
		lea	edx, [ebp - 1]			; edx = height - 1
		mul	edx
		mov esi, [esp + 24 + 12]	; y_src
		add esi, eax				; y_src += (height - 1) * y_stride

		shr ebp, 1					; y = height / 2
		mov	eax, [esp + 24 + 28]	; uv_stride
		lea	edx, [ebp - 1]			; edx = height/2 - 1
		mul	edx

		mov ecx, [esp + 24 + 16]	; u_src
		mov edx, [esp + 24 + 20]	; v_src
		add ecx, eax				; u_src += (height/2 - 1) * uv_stride
		add edx, eax				; v_src += (height/2 - 1) * uv_stride

		neg	dword [esp + 24 + 24]	; y_stride = -y_stride
		neg dword [esp + 24 + 28]	; uv_stride = -uv_stride
	
.yloop
		xor eax, eax			; x = 0;

.xloop1
				movd mm0, [ecx+4*eax]		; [    |uuuu]
				movd mm1, [edx+4*eax]		; [    |vvvv]
				movq mm2, [esi+8*eax]		; [yyyy|yyyy]

				punpcklbw mm0, mm1			; [vuvu|vuvu]
				movq mm1, mm0
				punpcklbw mm0, mm2			; [yvyu|yvyu]
				punpckhbw mm1, mm2			; [yvyu|yvyu]
				movq [edi], mm0
				movq [edi+8], mm1

				inc eax
				add edi, 16
						
				cmp eax, ebx
				jb	.xloop1

		add edi, [esp + 0]		; dst += dst_dif
		add esi, [esp + 24 + 24]	; y_src += y_stride
		
		xor eax, eax

.xloop2
				movd mm0, [ecx+4*eax]		; [    |uuuu]
				movd mm1, [edx+4*eax]		; [    |vvvv]
				movq mm2, [esi+8*eax]		; [yyyy|yyyy]

				punpcklbw mm0, mm1			; [vuvu|vuvu]
				movq mm1, mm0
				punpcklbw mm0, mm2			; [yvyu|yvyu]
				punpckhbw mm1, mm2			; [yvyu|yvyu]

				movq [edi], mm0
				movq [edi+8], mm1

				inc eax
				add edi, 16
										
				cmp eax, ebx
				jb	.xloop2

		add edi, [esp + 0]			; dst += dst_dif
		add esi, [esp + 24 + 24]	; y_src += y_stride
		add ecx, [esp + 24 + 28]	; u_src += uv_stride
		add edx, [esp + 24 + 28]	; v_src += uv_stride

		dec	ebp				; y--
		jnz	near .yloop

		emms

		add esp, 4
		pop ebp
		pop edi
		pop esi
		pop ecx
		pop ebx

		ret
