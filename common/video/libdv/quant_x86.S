/* 
 *  quant_x86.S
 *
 *     Copyright (C) James Bowman - May 2000
 *     Copyright (C) Peter Schlaile - Jan 2001
 *
 *  This file is part of libdv, a free DV (IEC 61834/SMPTE 314M)
 *  codec.
 *
 *  libdv is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your
 *  option) any later version.
 *   
 *  libdv is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 *  The libdv homepage is http://libdv.sourceforge.net/.  
 */


/*

The pattern for dv_88_areas looks like this:	
    0  0  1  1  1  2  2   
 0  0  1  1  1  2  2  2   
 0  1  1  1  2  2  2  3   
 1  1  1  2  2  2  3  3   
 1  1  2  2  2  3  3  3   
 1  2  2  2  3  3  3  3   
 2  2  2  3  3  3  3  3   
 2  2  3  3  3  3  3  3

Note
[1] matrix element [0][0] is untouched.
[2] all values in the same diagonal are equal
	
This implementation works by loading the four shift values in turn,
and shifting all the appropriate array elements.
	
*/

#include "asmoff.h"
	
/*
void quant_88_inverse(dv_coeff_t *block,int qno,int class)
*/

.text
	.align	4
.globl	quant_88_inverse_x86
quant_88_inverse_x86:	
	pushl	%ebx
	pushl	%esi

#define ARGn(N)  (12+(4*(N)))(%esp)

	/*
		eax	
		ebx	extra
		ecx
		edx	pq
		esi	block
	*/
	
	/*  pq = dv_quant_shifts[qno + dv_quant_offset[class]]; */
	movl	ARGn(1),%eax	/* qno */
	movl	ARGn(2),%ebx	/* class */
	movzbl	dv_quant_offset(%ebx),%ecx
	addl	%ecx,%eax
	leal	dv_quant_shifts(,%eax,4),%edx	/* edx is pq */

	/* extra = (class == 3); */
				/*  0   1   2   3 */
	subl	$3,%ebx		/* -3  -2  -1   0 */
	sarl	$31,%ebx	/* -1  -1  -1   0 */
	incl	%ebx		/*  0   0   0   1 */
	addl	$DV_WEIGHT_BIAS,%ebx
				/* ebx is extra */

	movl	ARGn(0),%esi	/* esi is block */

	/* Pick up each of the factors into %ecx, then shift the
	appropriate coefficients.  The pattern here is taken from
	dv_88_areas; it's arranged by diagonals for clarity. */

#define ADDR(row,col)	(2*(8*row+col))(%esi)
#define MSHIFT(row,col)	\
	shlw	%cl,ADDR(row,col)

	movl	$DV_WEIGHT_BIAS,%ecx
	MSHIFT(0,0)
	
	/* 0 */
	movzbl	(%edx),%ecx
	addl	%ebx,%ecx
	MSHIFT(0,1)
	MSHIFT(1,0)
	
	MSHIFT(0,2)
	MSHIFT(1,1)
	MSHIFT(2,0)

	/* 1 */	
	movzbl	1(%edx),%ecx
	addl	%ebx,%ecx
	MSHIFT(0,3)
	MSHIFT(1,2)
	MSHIFT(2,1)
	MSHIFT(3,0)

	MSHIFT(0,4)
	MSHIFT(1,3)
	MSHIFT(2,2)
	MSHIFT(3,1)
	MSHIFT(4,0)

	MSHIFT(0,5)
	MSHIFT(1,4)
	MSHIFT(2,3)
	MSHIFT(3,2)
	MSHIFT(4,1)
	MSHIFT(5,0)
		
	/* 2 */	
	movzbl	2(%edx),%ecx
	addl	%ebx,%ecx
	MSHIFT(0,6)
	MSHIFT(1,5)
	MSHIFT(2,4)
	MSHIFT(3,3)
	MSHIFT(4,2)
	MSHIFT(5,1)
	MSHIFT(6,0)
	
	MSHIFT(0,7)
	MSHIFT(1,6)
	MSHIFT(2,5)
	MSHIFT(3,4)
	MSHIFT(4,3)
	MSHIFT(5,2)
	MSHIFT(6,1)
	MSHIFT(7,0)
	
	MSHIFT(1,7)
	MSHIFT(2,6)
	MSHIFT(3,5)
	MSHIFT(4,4)
	MSHIFT(5,3)
	MSHIFT(6,2)
	MSHIFT(7,1)

	/* 3 */	
	movzbl	3(%edx),%ecx
	addl	%ebx,%ecx
	MSHIFT(2,7)
	MSHIFT(3,6)
	MSHIFT(4,5)
	MSHIFT(5,4)
	MSHIFT(6,3)
	MSHIFT(7,2)
	
	MSHIFT(3,7)
	MSHIFT(4,6)
	MSHIFT(5,5)
	MSHIFT(6,4)
	MSHIFT(7,3)
	
	MSHIFT(4,7)
	MSHIFT(5,6)
	MSHIFT(6,5)
	MSHIFT(7,4)
	
	MSHIFT(5,7)
	MSHIFT(6,6)
	MSHIFT(7,5)
	
	MSHIFT(6,7)
	MSHIFT(7,6)
	
	MSHIFT(7,7)
		
#undef ARGn
	popl	%esi
	popl	%ebx
	ret

	.align	4
.globl	quant_x86
quant_x86:	
	pushl	%ebx
	pushl	%ecx
	pushl	%edx	
	pushl	%esi
	

#define ARGn(N)  (20+(4*(N)))(%esp)

	/*
		eax	
		ebx	extra
		ecx
		edx	pq
		esi	block
	*/
	
	/*  pq = dv_quant_shifts[qno + dv_quant_offset[class]]; */
	movl	ARGn(1),%eax	/* qno */
	movl	ARGn(2),%ebx	/* class */

	movzbl	dv_quant_offset(%ebx),%ecx
	addl	%ecx,%eax
	leal	dv_quant_shifts(,%eax,4),%edx	/* edx is pq */

	/* extra = (class == 3); */
				/*  0   1   2   3 */
	subl	$3,%ebx		/* -3  -2  -1   0 */
	sarl	$31,%ebx	/* -1  -1  -1   0 */
	incl	%ebx		/*  0   0   0   1 */
				/* ebx is extra */

	movl	ARGn(0),%esi	/* esi is block */

	/* Since we already reordered the coefficients, it's easy:
	   Shift between OFS0 and OFS1 with the first pq value
	         between OFS1 and OFS2 with the second pq value etc.
	   Since we really want to divide, we have to compensate for
	   negative values.

	   The remaining thing is pipe-line optimization
	   which results in obfuscating MMX code...
	 */

	# sarw	%cl,ADDR(row,col)

#define OFS0 (1)
#define OFS1 (1+2+3)
#define OFS2 (1+2+3+4+5+6)
#define OFS3 (1+2+3+4+5+6+7+8+7)

	/* 0 */                     
	movzbl	(%edx),%ecx
	movq	OFS0*2(%esi), %mm2
	addl	%ebx, %ecx
	movq	(OFS0+4)*2(%esi), %mm4
	movd	%ecx, %mm7
	
	movq	%mm2, %mm3
	movq	%mm4, %mm5
	psraw	$0x0f, %mm2
	psraw	$0x0f, %mm4
	pxor	%mm2, %mm3
	pxor	%mm4, %mm5
	psubw	%mm2, %mm3
	psubw	%mm4, %mm5
	psraw	%mm7, %mm3
	psraw	%mm7, %mm5
	pxor	%mm2, %mm3
	pxor	%mm4, %mm5
	psubw	%mm2, %mm3
	psubw	%mm4, %mm5
	
	movq	(OFS1*2)(%esi), %mm2
	movq	%mm3, OFS0*2(%esi)
	movq	%mm5, (OFS0+4)*2(%esi)
	
	/* 1 */	
	movzbl	1(%edx),%ecx
	movq	(OFS1+4)*2(%esi), %mm4
	addl	%ebx, %ecx
	movq	%mm2, %mm3
	movd	%ecx, %mm7
	movq	%mm4, %mm5
	psraw	$0x0f, %mm2
	psraw	$0x0f, %mm4
	pxor	%mm2, %mm3
	pxor	%mm4, %mm5
	psubw	%mm2, %mm3
	psubw	%mm4, %mm5
	psraw	%mm7, %mm3
	psraw	%mm7, %mm5
	pxor	%mm2, %mm3
	pxor	%mm4, %mm5
	psubw	%mm2, %mm3
	psubw	%mm4, %mm5
	
	movq	%mm3, OFS1*2(%esi)
	movq	(OFS1+8)*2(%esi), %mm2
	movq	%mm5, (OFS1+4)*2(%esi)
	movq	(OFS1+12)*2(%esi), %mm4	
	movq	%mm2, %mm3
	movq	%mm4, %mm5
	psraw	$0x0f, %mm2
	psraw	$0x0f, %mm4
	pxor	%mm2, %mm3
	pxor	%mm4, %mm5
	psubw	%mm2, %mm3
	psubw	%mm4, %mm5
	psraw	%mm7, %mm3
	psraw	%mm7, %mm5
	pxor	%mm2, %mm3
	pxor	%mm4, %mm5
	psubw	%mm2, %mm3
	psubw	%mm4, %mm5
	
	movq	OFS2*2(%esi), %mm0
	movq	%mm3, (OFS1+8)*2(%esi)
	movq	%mm5, (OFS1+12)*2(%esi)
	
	/* 2 */	
	movzbl	2(%edx),%ecx
	movq	(OFS2+4)*2(%esi), %mm2
	addl	%ebx, %ecx
	movq	(OFS2+8)*2(%esi), %mm4
	movd	%ecx, %mm7

	movq	%mm0, %mm1
	movq	%mm2, %mm3
	movq	%mm4, %mm5
	psraw	$0x0f, %mm0
	psraw	$0x0f, %mm2
	psraw	$0x0f, %mm4
	pxor	%mm0, %mm1
	pxor	%mm2, %mm3
	pxor	%mm4, %mm5
	psubw	%mm0, %mm1
	psubw	%mm2, %mm3
	psubw	%mm4, %mm5
	psraw	%mm7, %mm1
	psraw	%mm7, %mm3
	psraw	%mm7, %mm5
	pxor	%mm0, %mm1
	pxor	%mm2, %mm3
	pxor	%mm4, %mm5
	psubw	%mm0, %mm1
	psubw	%mm2, %mm3
	psubw	%mm4, %mm5
	
	movq	%mm1, OFS2*2(%esi)
	movq	%mm3, (OFS2+4)*2(%esi)
	movq	%mm5, (OFS2+8)*2(%esi)
	
	movq	(OFS2+12)*2(%esi), %mm0
	movq	(OFS2+16)*2(%esi), %mm2
	movq	(OFS2+20)*2(%esi), %mm4

	movq	%mm0, %mm1
	movq	%mm2, %mm3
	movq	%mm4, %mm5
	psraw	$0x0f, %mm0
	psraw	$0x0f, %mm2
	psraw	$0x0f, %mm4
	pxor	%mm0, %mm1
	pxor	%mm2, %mm3
	pxor	%mm4, %mm5
	psubw	%mm0, %mm1
	psubw	%mm2, %mm3
	psubw	%mm4, %mm5
	psraw	%mm7, %mm1
	psraw	%mm7, %mm3
	psraw	%mm7, %mm5
	pxor	%mm0, %mm1
	pxor	%mm2, %mm3
	pxor	%mm4, %mm5
	psubw	%mm0, %mm1
	psubw	%mm2, %mm3
	psubw	%mm4, %mm5

	movq	OFS3*2(%esi), %mm0

	movq	%mm1, (OFS2+12)*2(%esi)
	movq	%mm3, (OFS2+16)*2(%esi)
	movq	%mm5, (OFS2+20)*2(%esi)

	/* 3 */	
	movzbl	3(%edx),%ecx
	movq	(OFS3+4)*2(%esi), %mm2
	addl	%ebx, %ecx
	movq	(OFS3+8)*2(%esi), %mm4
	movd	%ecx, %mm7

	movq	%mm0, %mm1
	movq	%mm2, %mm3
	movq	%mm4, %mm5
	psraw	$0x0f, %mm0
	psraw	$0x0f, %mm2
	psraw	$0x0f, %mm4
	pxor	%mm0, %mm1
	pxor	%mm2, %mm3
	pxor	%mm4, %mm5
	psubw	%mm0, %mm1
	psubw	%mm2, %mm3
	psubw	%mm4, %mm5
	psraw	%mm7, %mm1
	psraw	%mm7, %mm3
	psraw	%mm7, %mm5
	pxor	%mm0, %mm1
	pxor	%mm2, %mm3
	pxor	%mm4, %mm5
	psubw	%mm0, %mm1
	psubw	%mm2, %mm3
	psubw	%mm4, %mm5
	
	movq	%mm1, OFS3*2(%esi)
	movq	%mm3, (OFS3+4)*2(%esi)
	movq	%mm5, (OFS3+8)*2(%esi)

	movq	(OFS3+12)*2(%esi), %mm2
	movq	(OFS3+16)*2(%esi), %mm4

	movq	%mm2, %mm3
	movq	%mm4, %mm5
	psraw	$0x0f, %mm2
	psraw	$0x0f, %mm4
	pxor	%mm2, %mm3
	pxor	%mm4, %mm5
	psubw	%mm2, %mm3
	psubw	%mm4, %mm5
	psraw	%mm7, %mm3
	psraw	%mm7, %mm5
	pxor	%mm2, %mm3
	pxor	%mm4, %mm5
	psubw	%mm2, %mm3
	psubw	%mm4, %mm5
	
	movq	%mm3, (OFS3+12)*2(%esi)
	movq	%mm5, (OFS3+16)*2(%esi)

	movw	(OFS3+20)*2(%esi), %ax
	movw	%ax, %bx
	sarw	$0xf, %bx
	xorw	%bx, %ax
	subw	%bx, %ax
	sarw	%cl, %ax
	xorw	%bx, %ax
	subw	%bx, %ax
	movw	%ax, (OFS3+20)*2(%esi)
	
	popl	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx
	ret





