;
; The contents of this file are subject to the Mozilla Public
; License Version 1.1 (the "License"); you may not use this file
; except in compliance with the License. You may obtain a copy of
; the License at http://www.mozilla.org/MPL/
; 
; Software distributed under the License is distributed on an "AS
; IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
; implied. See the License for the specific language governing
; rights and limitations under the License.
; 
; The Original Code is MPEG4IP.
; 
; The Initial Developer of the Original Code is Cisco Systems Inc.
; Portions created by Cisco Systems Inc. are
; Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
; 
; Contributor(s): 
;		Dave Mackie			dmackie@cisco.com
;

; MMX version of image interpolater

; the function arguments
%define PSRC		ebp+8
%define PDST		ebp+12
%define WIDTH 		ebp+16
%define HEIGHT		ebp+20
%define ROUNDING	ebp+24

; our register variables
; eax reserved for temporary calculations
%define RWIDTH		ebx
%define ROWNUM		ecx
%define COLNUM		edx
%define PSRCROW		esi
%define PDSTROW		edi
%define ROUNDER		mm7

BITS 32

GLOBAL InterpolateImageMmx

SECTION .data
ALIGN 8

wordsZero
	dd 0x00000000
	dd 0x00000000
wordsOne
	dd 0x00010001
	dd 0x00010001
lastColDWord
	dd 0x00000000
	dd 0xFFFFFFFF

SECTION .text

InterpolateImageMmx:
	push ebp;
	mov ebp, esp;

	push eax;
	push ebx;
	push ecx;
	push edx;
	push esi;
	push edi;

	mov PSRCROW, [PSRC];
	mov PDSTROW, [PDST];
	mov RWIDTH, [WIDTH];
	imul RWIDTH, 2;			arg is in pixels, we want bytes
	mov COLNUM, 0;

	cmp DWORD [ROUNDING], 0;
	jnz near roundUp;
	movq ROUNDER, [wordsZero];
	jmp near colLoop;
roundUp:
	movq ROUNDER, [wordsOne];

colLoop:

	; compute first row
	movq mm0, [PSRCROW];	mm0 = row[i][j] = p3h p3l p2h p2l p1h p1l p0h p0l

	movq mm1, mm0;			mm1 = mm0
	punpcklwd mm1, mm0;		mm1 = p1h p1l p1h p1l p0h p0l p0h p0l

	movq mm2, mm0;			mm2 = mm0
	psrlq mm2, 16;			mm2 = 00 00 p3h p3l p2h p2l p1h p1l

	punpcklwd mm0, mm2;		mm0 = p2h p2l p1h p1l p1h p1l p0h p0l

	paddusw mm1, ROUNDER;	add in rounding control to each word

	pavgw mm1, mm0;			mm1 = average of mm0 and mm1 words

	movq [PDSTROW], mm1;	store result

	add PSRCROW, RWIDTH;	increment src pointer by width
	add PDSTROW, RWIDTH;	increment dst pointer by 2 * width
	add PDSTROW, RWIDTH;

	mov ROWNUM, [HEIGHT];
	sub ROWNUM, 1;

rowLoop:
	; mm1 contains previous row, row[i-1][j]

	movq mm0, [PSRCROW];	mm0 = row[i][j] = p3h p3l p2h p2l p1h p1l p0h p0l

	movq mm3, mm0;			mm3 = mm0
	punpcklwd mm3, mm0;		mm3 = p1h p1l p1h p1l p0h p0l p0h p0l

	movq mm2, mm0;			mm2 = mm0
	psrlq mm2, 16;			mm2 = 00 00 p3h p3l p2h p2l p1h p1l

	punpcklwd mm0, mm2;		mm0 = p2h p2l p1h p1l p1h p1l p0h p0l

	paddusw mm3, ROUNDER;	add in rounding control to each word

	; compute expanded row
	pavgw mm3, mm0;			mm3 = average of mm0 and mm3 words

	paddusw mm3, ROUNDER;	add in rounding control to each word

	; compute interpolated row
	pavgb mm1, mm3;			mm1 = average of mm1 and mm3 words			

	movq [PDSTROW], mm1;	store interpolated row

	add PSRCROW, RWIDTH;	increment src pointer by width

	add PDSTROW, RWIDTH;	increment dst pointer by 2 * width
	add PDSTROW, RWIDTH;

	movq [PDSTROW], mm3; 	store expanded row

	add PDSTROW, RWIDTH;	increment dst pointer by 2 * width
	add PDSTROW, RWIDTH;

	movq mm1, mm3;			current row now becomes previous row

	sub ROWNUM, 1;
	jnz rowLoop;

	movq [PDSTROW], mm3;	store last dst row

	; compute ptrs for next column
	mov PSRCROW, [PSRC];
	mov PDSTROW, [PDST];
	add COLNUM, 2;
	add PSRCROW, COLNUM;
	add PDSTROW, COLNUM;
	add PDSTROW, COLNUM;
	
	mov eax, [WIDTH];
	sub eax, 2;
	cmp COLNUM, eax;
	jnz near colLoop;

; last column needs special handling
lastCol:
	; compute first row

	movd mm0, [PSRCROW];	mm0 = row[0][j] = 00 00 00 00 p1h p1l p0h p0l 

	movq mm1, mm0;			mm1 = mm0
	punpcklwd mm1, mm0;		mm1 = p1h p1l p1h p1l p0h p0l p0h p0l

	movq mm2, mm0;			mm2 = mm0
	movq mm4, mm1;			mm4 = mm1
	pand mm4, [lastColDWord]; mm4 = p1h p1l p1h p1l 00 00 00 00
	por mm2, mm4;			mm2 = p1h p1l p1h p1l p1h p1l p0h p0l

	punpcklwd mm0, mm2;		mm0 = p2h p2l p1h p1l p1h p1l p0h p0l

	paddusw mm1, ROUNDER;	add in rounding control to each word

	pavgw mm1, mm0;			mm1 = average of mm0 and mm1 words

	movq [PDSTROW], mm1;	store result

	add PSRCROW, RWIDTH;	increment src pointer by width
	add PDSTROW, RWIDTH;	increment dst pointer by 2 * width
	add PDSTROW, RWIDTH;

	mov ROWNUM, [HEIGHT];
	sub ROWNUM, 1;

lastColRowLoop:
	; mm1 contains previous row, row[i-1][j]

	movd mm0, [PSRCROW];	mm0 = row[0][j] = 00 00 00 00 p1h p1l p0h p0l 

	movq mm3, mm0;			mm3 = mm0
	punpcklwd mm3, mm0;		mm3 = p1h p1l p1h p1l p0h p0l p0h p0l

	movq mm2, mm0;			mm2 = mm0
	movq mm4, mm1;			mm4 = mm1
	pand mm4, [lastColDWord]; mm4 = p1h p1l p1h p1l 00 00 00 00
	por mm2, mm4;			mm2 = p1h p1l p1h p1l p1h p1l p0h p0l

	punpcklwd mm0, mm2;		mm0 = p1h p1l p1h p1l p1h p1l p0h p0l

	paddusw mm3, ROUNDER;	add in rounding control to each word

	; compute expanded row
	pavgw mm3, mm0;			mm3 = average of mm0 and mm3 words

	paddusw mm3, ROUNDER;	add in rounding control to each word

	; compute interpolated row
	pavgb mm1, mm3;			mm1 = average of mm1 and mm3 words			

	movq [PDSTROW], mm1;	store interpolated row

	add PSRCROW, RWIDTH;	increment src pointer by width

	add PDSTROW, RWIDTH;	increment dst pointer by 2 * width
	add PDSTROW, RWIDTH;

	movq [PDSTROW], mm3; 	store expanded row

	add PDSTROW, RWIDTH;	increment dst pointer by 2 * width
	add PDSTROW, RWIDTH;

	movq mm1, mm3;			current row now becomes previous row

	sub ROWNUM, 1;
	jnz lastColRowLoop;

	movq [PDSTROW], mm3;	store last dst row

done:
	pop edi;
	pop esi;
	pop edx;
	pop ecx;
	pop ebx;
	pop eax;

	emms;
	pop ebp;
	ret;

