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

; MMX version of frame doubler

; the function arguments
%define PSRC		ebp+8
%define PDST		ebp+12
%define WIDTH 		ebp+16
%define HEIGHT		ebp+20

; our register variables
; eax reserved for temporary calculations
%define RWIDTH		ebx
%define ROWNUM		ecx
%define COLNUM		edx
%define PSRCROW		esi
%define PDSTROW		edi

BITS 32

GLOBAL FrameDoublerMmx

SECTION .data
ALIGN 8

lastColByte
	dd 0xFF000000
	dd 0x00000000

SECTION .text

FrameDoublerMmx:
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
	mov COLNUM, 0;

colLoop:

	; compute first row
	movq mm0, [PSRCROW];	mm0 = row[i][j] = p7 p6 p5 p4 p3 p2 p1 p0

	movq mm1, mm0;			mm1 = mm0
	punpcklbw mm1, mm0;		mm1 = p3 p3 p2 p2 p1 p1 p0 p0

	movq mm2, mm0;			mm2 = mm0
	psrlq mm2, 8;			mm2 = 00 p7 p6 p5 p4 p3 p2 p1

	punpcklbw mm0, mm2;		mm2 = p4 p3 p3 p2 p2 p1 p1 p0

	pavgb mm1, mm0;			mm1 = p0 (p0+p1+1)>>1 p1 (p1+p2+1)>>1 ...

	movq [PDSTROW], mm1;	store result

	add PSRCROW, RWIDTH;	increment src pointer by width
	add PDSTROW, RWIDTH;	increment dst pointer by 2 * width
	add PDSTROW, RWIDTH;

	mov ROWNUM, [HEIGHT];
	sub ROWNUM, 1;

rowLoop:
	; mm1 contains previous row, row[i-1][j]

	movq mm0, [PSRCROW];	mm0 = row[i][j] = p7 p6 p5 p4 p3 p2 p1 p0

	movq mm3, mm0;			mm3 = mm0
	punpcklbw mm3, mm0;		mm3 = p3 p3 p2 p2 p1 p1 p0 p0

	movq mm2, mm0;			mm2 = mm0
	psrlq mm2, 8;			mm2 = 00 p7 p6 p5 p4 p3 p2 p1

	punpcklbw mm0, mm2;		mm2 = p4 p3 p3 p2 p2 p1 p1 p0

	; compute expanded row, average of mm0 and mm3
	pavgb mm3, mm0;			mm3 = (p4+p3+1)>>1 p3 (p3+p2+1)>>1 p2 ...

	; compute interpolated row, average of mm1 and mm3
	pavgb mm1, mm3;			
	
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
	add COLNUM, 4;
	add PSRCROW, COLNUM;
	add PDSTROW, COLNUM;
	add PDSTROW, COLNUM;
	
	mov eax, RWIDTH;
	sub eax, 4;
	cmp COLNUM, eax;
	jnz near colLoop;

; last column needs special handling
lastCol:
	; compute first row

	movd mm0, [PSRCROW];	mm0 = row[0][j] = 00 00 00 00 p3 p2 p1 p0 

	movq mm1, mm0;			mm1 = mm0
	punpcklbw mm1, mm0;		mm1 = p3 p3 p2 p2 p1 p1 p0 p0

	movq mm2, mm0;			mm2 = mm0
	psrlq mm2, 8;			mm2 = 00 00 00 00 00 p3 p2 p1

	movq mm4, mm0;			mm4 = mm0
	pand mm4, [lastColByte]; mm4 = 00 00 00 00 p3 00 00 00
	por mm2, mm4;			mm2 = 00 00 00 00 p3 p3 p2 p1

	punpcklbw mm0, mm2;		mm0 = p3 p3 p3 p2 p2 p1 p1 p0

	pavgb mm1, mm0;			mm1 = p3 p3 (p3+p2+1)>>1 p2 (p2+p1+1)>>1 p1 ...

	movq [PDSTROW], mm1;	store result

	add PSRCROW, RWIDTH;	increment src pointer by width
	add PDSTROW, RWIDTH;	increment dst pointer by 2 * width
	add PDSTROW, RWIDTH;

	mov ROWNUM, [HEIGHT];
	sub ROWNUM, 1;

lastColRowLoop:
	; mm1 contains previous row, row[i-1][j]

	movd mm0, [PSRCROW];	mm0 = row[0][j] = 00 00 00 00 p3 p2 p1 p0 

	movq mm3, mm0;			mm3 = mm0
	punpcklbw mm3, mm0;		mm1 = p3 p3 p2 p2 p1 p1 p0 p0

	movq mm2, mm0;			mm2 = mm0
	psrlq mm2, 8;			mm2 = 00 00 00 00 00 p3 p2 p1

	movq mm4, mm0;			mm4 = mm0
	pand mm4, [lastColByte]; mm4 = 00 00 00 00 p3 00 00 00
	por mm2, mm4;			mm2 = 00 00 00 00 p3 p3 p2 p1

	punpcklbw mm0, mm2;		mm0 = p3 p3 p3 p2 p2 p1 p1 p0

	; compute expanded row, average of mm0 and mm3
	pavgb mm3, mm0;			mm1 = p3 p3 (p3+p2+1)>>1 p2 (p2+p1+1)>>1 p1 ...

	; compute interpolated row, average of mm1 and mm3
	pavgb mm1, mm3;			
	
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

