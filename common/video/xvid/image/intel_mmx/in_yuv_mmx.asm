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
; *  yuv_to_yuv.asm, MMX optimized color conversion                            *
; *                                                                            *
; *  Copyright (C) 2001 - Michael Militzer <isibaar@xvid.org>,                 *
; *                                                                            *
; *  For more information visit the XviD homepage: http://www.xvid.org         *
; *                                                                            *
; ******************************************************************************/
;
;/******************************************************************************
; *                                                                            *
; *  Revision history:                                                         *
; *                                                                            *
; *  24.11.2001 initial version  (Isibaar)                                     *
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

SECTION .data
remainder dw 0

SECTION .text

ALIGN 64

; Attention: This code assumes that width is a multiple of 16
; This function probably also runs on PentiumII class cpu's

;;void yuv_to_yuv_xmm(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out, uint8_t *src,
;;				   int width, int height, int stride);
cglobal yuv_to_yuv_xmm
yuv_to_yuv_xmm:
	
    push ebx
    push esi
    push edi
	push ebp

    mov eax, [esp + 40]		; height -> eax
    mov ebx, [esp + 44]		; stride -> ebx
    mov esi, [esp + 32] 	; src -> esi 
    mov edi, [esp + 20] 	; y_out -> edi 
    mov ecx, [esp + 36] 	; width -> ecx

    sub ebx, ecx			; stride - width -> ebx

	mov edx, ecx
	mov ebp, ecx
	shr edx, 6				
	mov ecx, edx			; 64 bytes copied per iteration
	shl edx, 6
	sub ebp, edx			; remainder -> ebp
	shr ebp, 4				; 16 bytes per iteration
	add ebp, 1			
	mov [remainder], ebp

	mov edx, ecx			

.y_inner_loop:
	prefetchnta [esi + 64]	; non temporal prefetch 
    prefetchnta [esi + 96] 

    movq mm1, [esi]			; read from src 
    movq mm2, [esi + 8] 
    movq mm3, [esi + 16] 
    movq mm4, [esi + 24] 
    movq mm5, [esi + 32] 
    movq mm6, [esi + 40] 
    movq mm7, [esi + 48] 
    movq mm0, [esi + 56] 

    movntq [edi], mm1		; write to y_out 
    movntq [edi + 8], mm2 
    movntq [edi + 16], mm3 
    movntq [edi + 24], mm4 
    movntq [edi + 32], mm5 
    movntq [edi + 40], mm6 
    movntq [edi + 48], mm7 
    movntq [edi + 56], mm0 

    add esi, 64
    add edi, 64 
    dec ecx
    jnz .y_inner_loop    
	
	dec ebp
	jz .y_outer_loop

.y_remainder_loop:
    movq mm1, [esi]			; read from src 
    movq mm2, [esi + 8] 

    movntq [edi], mm1		; write to y_out 
    movntq [edi + 8], mm2 

    add esi, 16
    add edi, 16 
	dec ebp
	jnz .y_remainder_loop
	
.y_outer_loop:
	mov ebp, [remainder]	
    mov ecx, edx
    add edi, ebx
    
    dec eax
	jnz near .y_inner_loop

    mov eax, [esp + 40]		; height -> eax
    mov ebx, [esp + 44]		; stride -> ebx
    mov ecx, [esp + 36]	 	; width -> ecx
    mov edi, [esp + 24] 	; u_out -> edi 

	shr ecx, 1				; width / 2 -> ecx
	shr ebx, 1				; stride / 2 -> ebx
	shr eax, 1				; height / 2 -> eax

    sub ebx, ecx			; stride / 2 - width / 2 -> ebx

	mov edx, ecx
	mov ebp, ecx
	shr edx, 6				
	mov ecx, edx			; 64 bytes copied per iteration
	shl edx, 6
	sub ebp, edx			; remainder -> ebp
	shr ebp, 3				; 8 bytes per iteration
	add ebp, 1			
	mov [remainder], ebp
	
	mov edx, ecx			

.u_inner_loop:
	prefetchnta [esi + 64]	; non temporal prefetch 
    prefetchnta [esi + 96] 

    movq mm1, [esi]			; read from src 
    movq mm2, [esi + 8] 
    movq mm3, [esi + 16] 
    movq mm4, [esi + 24] 
    movq mm5, [esi + 32] 
    movq mm6, [esi + 40] 
    movq mm7, [esi + 48] 
    movq mm0, [esi + 56] 

    movntq [edi], mm1		; write to u_out 
    movntq [edi + 8], mm2 
    movntq [edi + 16], mm3 
    movntq [edi + 24], mm4 
    movntq [edi + 32], mm5 
    movntq [edi + 40], mm6 
    movntq [edi + 48], mm7 
    movntq [edi + 56], mm0 


    add esi, 64
    add edi, 64 
    dec ecx
    jnz .u_inner_loop    

	dec ebp
	jz .u_outer_loop

.u_remainder_loop:
    movq mm1, [esi]			; read from src 
    movntq [edi], mm1		; write to y_out 

    add esi, 8
    add edi, 8 
	dec ebp
	jnz .u_remainder_loop

.u_outer_loop:
	mov ebp, [remainder]	
    mov ecx, edx
    add edi, ebx
    
    dec eax
	jnz .u_inner_loop

    mov eax, [esp + 40]		; height -> eax
    mov ecx, [esp + 36] 	; width -> ecx
    mov edi, [esp + 28] 	; v_out -> edi 

	shr ecx, 1				; width / 2 -> ecx
	shr eax, 1				; height / 2 -> eax

	mov edx, ecx
	mov ebp, ecx
	shr edx, 6				
	mov ecx, edx			; 64 bytes copied per iteration
	shl edx, 6
	sub ebp, edx			; remainder -> ebp
	shr ebp, 3				; 8 bytes per iteration
	add ebp, 1			
	mov [remainder], ebp
	
	mov edx, ecx			

.v_inner_loop:
	prefetchnta [esi + 64]	; non temporal prefetch 
    prefetchnta [esi + 96] 

    movq mm1, [esi]			; read from src 
    movq mm2, [esi + 8] 
    movq mm3, [esi + 16] 
    movq mm4, [esi + 24] 
    movq mm5, [esi + 32] 
    movq mm6, [esi + 40] 
    movq mm7, [esi + 48] 
    movq mm0, [esi + 56] 

    movntq [edi], mm1		; write to u_out 
    movntq [edi + 8], mm2 
    movntq [edi + 16], mm3 
    movntq [edi + 24], mm4 
    movntq [edi + 32], mm5 
    movntq [edi + 40], mm6 
    movntq [edi + 48], mm7 
    movntq [edi + 56], mm0 


    add esi, 64
    add edi, 64 
    dec ecx
    jnz .v_inner_loop    

	dec ebp
	jz .v_outer_loop

.v_remainder_loop:
    movq mm1, [esi]			; read from src 
    movntq [edi], mm1		; write to y_out 

    add esi, 8
    add edi, 8 
	dec ebp
	jnz .v_remainder_loop

.v_outer_loop:
	mov ebp, [remainder]	
    mov ecx, edx
    add edi, ebx
    
    dec eax
	jnz .v_inner_loop

	pop ebp
    pop edi
    pop esi
    pop ebx
    
    emms

    ret



; Attention: This code assumes that width is a multiple of 16
;;void yuv_to_yuv_mmx(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out, uint8_t *src,
;;				   int width, int height, int stride);
cglobal yuv_to_yuv_mmx
yuv_to_yuv_mmx:
	
    push ebx
    push esi
    push edi
	push ebp

    mov eax, [esp + 40]		; height -> eax
    mov ebx, [esp + 44]		; stride -> ebx
    mov esi, [esp + 32] 	; src -> esi 
    mov edi, [esp + 20] 	; y_out -> edi 
    mov ecx, [esp + 36] 	; width -> ecx

    sub ebx, ecx			; stride - width -> ebx

	mov edx, ecx
	mov ebp, ecx
	shr edx, 6				
	mov ecx, edx			; 64 bytes copied per iteration
	shl edx, 6
	sub ebp, edx			; remainder -> ebp
	shr ebp, 4				; 16 bytes per iteration
	add ebp, 1			
	mov [remainder], ebp

	mov edx, ecx			

.y_inner_loop:
    movq mm1, [esi]			; read from src 
    movq mm2, [esi + 8] 
    movq mm3, [esi + 16] 
    movq mm4, [esi + 24] 
    movq mm5, [esi + 32] 
    movq mm6, [esi + 40] 
    movq mm7, [esi + 48] 
    movq mm0, [esi + 56] 

    movq [edi], mm1			; write to y_out 
    movq [edi + 8], mm2 
    movq [edi + 16], mm3 
    movq [edi + 24], mm4 
    movq [edi + 32], mm5 
    movq [edi + 40], mm6 
    movq [edi + 48], mm7 
    movq [edi + 56], mm0 

    add esi, 64
    add edi, 64 
    dec ecx
    jnz .y_inner_loop    
	
	dec ebp
	jz .y_outer_loop

.y_remainder_loop:
    movq mm1, [esi]			; read from src 
    movq mm2, [esi + 8] 

    movq [edi], mm1			; write to y_out 
    movq [edi + 8], mm2 

    add esi, 16
    add edi, 16 
	dec ebp
	jnz .y_remainder_loop
	
.y_outer_loop:
	mov ebp, [remainder]	
    mov ecx, edx
    add edi, ebx
    
    dec eax
	jnz near .y_inner_loop

    mov eax, [esp + 40]		; height -> eax
    mov ebx, [esp + 44]		; stride -> ebx
    mov ecx, [esp + 36]	 	; width -> ecx
    mov edi, [esp + 24] 	; u_out -> edi 

	shr ecx, 1				; width / 2 -> ecx
	shr ebx, 1				; stride / 2 -> ebx
	shr eax, 1				; height / 2 -> eax

    sub ebx, ecx			; stride / 2 - width / 2 -> ebx

	mov edx, ecx
	mov ebp, ecx
	shr edx, 6				
	mov ecx, edx			; 64 bytes copied per iteration
	shl edx, 6
	sub ebp, edx			; remainder -> ebp
	shr ebp, 3				; 8 bytes per iteration
	add ebp, 1			
	mov [remainder], ebp
	
	mov edx, ecx			

.u_inner_loop:
    movq mm1, [esi]			; read from src 
    movq mm2, [esi + 8] 
    movq mm3, [esi + 16] 
    movq mm4, [esi + 24] 
    movq mm5, [esi + 32] 
    movq mm6, [esi + 40] 
    movq mm7, [esi + 48] 
    movq mm0, [esi + 56] 

    movq [edi], mm1		; write to u_out 
    movq [edi + 8], mm2 
    movq [edi + 16], mm3 
    movq [edi + 24], mm4 
    movq [edi + 32], mm5 
    movq [edi + 40], mm6 
    movq [edi + 48], mm7 
    movq [edi + 56], mm0 


    add esi, 64
    add edi, 64 
    dec ecx
    jnz .u_inner_loop    

	dec ebp
	jz .u_outer_loop

.u_remainder_loop:
    movq mm1, [esi]			; read from src 
    movq [edi], mm1		; write to y_out 

    add esi, 8
    add edi, 8 
	dec ebp
	jnz .u_remainder_loop

.u_outer_loop:
	mov ebp, [remainder]	
    mov ecx, edx
    add edi, ebx
    
    dec eax
	jnz .u_inner_loop

    mov eax, [esp + 40]		; height -> eax
    mov ecx, [esp + 36] 	; width -> ecx
    mov edi, [esp + 28] 	; v_out -> edi 

	shr ecx, 1				; width / 2 -> ecx
	shr eax, 1				; height / 2 -> eax

	mov edx, ecx
	mov ebp, ecx
	shr edx, 6				
	mov ecx, edx			; 64 bytes copied per iteration
	shl edx, 6
	sub ebp, edx			; remainder -> ebp
	shr ebp, 3				; 8 bytes per iteration
	add ebp, 1			
	mov [remainder], ebp
	
	mov edx, ecx			

.v_inner_loop:
    movq mm1, [esi]			; read from src 
    movq mm2, [esi + 8] 
    movq mm3, [esi + 16] 
    movq mm4, [esi + 24] 
    movq mm5, [esi + 32] 
    movq mm6, [esi + 40] 
    movq mm7, [esi + 48] 
    movq mm0, [esi + 56] 

    movq [edi], mm1		; write to u_out 
    movq [edi + 8], mm2 
    movq [edi + 16], mm3 
    movq [edi + 24], mm4 
    movq [edi + 32], mm5 
    movq [edi + 40], mm6 
    movq [edi + 48], mm7 
    movq [edi + 56], mm0 


    add esi, 64
    add edi, 64 
    dec ecx
    jnz .v_inner_loop    

	dec ebp
	jz .v_outer_loop

.v_remainder_loop:
    movq mm1, [esi]			; read from src 
    movq [edi], mm1		; write to y_out 

    add esi, 8
    add edi, 8 
	dec ebp
	jnz .v_remainder_loop

.v_outer_loop:
	mov ebp, [remainder]	
    mov ecx, edx
    add edi, ebx
    
    dec eax
	jnz .v_inner_loop

	pop ebp
    pop edi
    pop esi
    pop ebx
    
    emms

    ret