/* 
 *  encode_x86.S
 *
 *     Copyright (C) Peter Schlaile - February 2001
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

.data
ALLONE:		.word 1,1,1,1
VLCADDMASK:	.byte 255,0,0,0,255,0,0,0
		
.text

.global vlc_encode_block_mmx
vlc_encode_block_mmx:	
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp

	xorl	%eax, %eax
	xorl	%edx, %edx
	movl	4+4*4(%esp), %edi                # src
	movl	4+4*4+4(%esp), %edx              # &dst
	movl	(%edx), %edx
	addl	$2, %edi

	movl	$63, %ecx

	movl	vlc_encode_lookup, %esi

	pxor	%mm0, %mm0
	pxor	%mm2, %mm2
	movq	VLCADDMASK, %mm1
	xorl	%ebp, %ebp
	subl	$8, %edx
vlc_encode_block_mmx_loop:
	pand	%mm1, %mm0
	movw	(%edi), %ax
	addl	$8, %edx
	paddd	%mm0, %mm2
	cmpw	$0, %ax
	jz	vlc_encode_block_amp_zero
	addw	$255, %ax
	addl	$2, %edi
	movq	(%esi, %eax, 8), %mm0
	movq	%mm0, (%edx)
	decl	%ecx
	jnz	vlc_encode_block_mmx_loop
	pand	%mm1, %mm0
	paddd	%mm0, %mm2
	jmp     vlc_encode_block_out
	
vlc_encode_block_amp_zero:
        movl    %ecx, %ebp
        incl    %ecx
        repz    scasw
        jecxz   vlc_encode_block_out
        movw    -2(%edi), %ax
        subl    %ecx, %ebp
	addw	$255, %ax
        shll    $9, %ebp
	orl	%ebp, %eax
	
	movq	(%esi, %eax, 8), %mm0
	movq	%mm0, (%edx)
	
	decl	%ecx
	jnz	vlc_encode_block_mmx_loop

	pand	%mm1, %mm0
	paddd	%mm0, %mm2
	
vlc_encode_block_out:
	movq	%mm2, %mm0
	psrlq	$32, %mm0
	paddd	%mm0, %mm2

	movl	4+4*4+4(%esp), %ebx
	movl	%edx, (%ebx)
	
	movd	%mm2, %eax
	
	popl	%ebp
	popl	%edi	
	popl	%esi
	popl	%ebx
	ret	

.global vlc_num_bits_block_x86
vlc_num_bits_block_x86:	
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp

	xorl	%eax, %eax
	xorl	%edx, %edx
	xorl	%ebx, %ebx
	xorl	%ebp, %ebp
	
	movl	4+4*4(%esp), %edi                # src
	addl	$2, %edi

	movl	$63, %ecx
	movl	vlc_num_bits_lookup, %esi
	
vlc_num_bits_block_x86_loop:
	movw	(%edi), %ax
	addl	%ebx, %edx
	cmpw	$0, %ax
	jz	vlc_num_bits_block_amp_zero
	addw	$255, %ax
	addl	$2, %edi
	movb	(%esi, %eax), %bl
	
	decl	%ecx
	jnz	vlc_num_bits_block_x86_loop
	addl	%ebx, %edx
	jmp	vlc_num_bits_block_out
	
vlc_num_bits_block_amp_zero:
        movl    %ecx, %ebp
        incl    %ecx
        repz    scasw
        jecxz   vlc_num_bits_block_out
	
        subl    %ecx, %ebp
        movw    -2(%edi), %ax
        shll    $9, %ebp
	addw	$255, %ax
	orl	%ebp, %eax
	movb	(%esi, %eax), %bl
	
	decl	%ecx
	jnz	vlc_num_bits_block_x86_loop
	addl	%ebx, %edx
	
vlc_num_bits_block_out:
	movl	%edx, %eax
		
	popl	%ebp
	popl	%edi	
	popl	%esi
	popl	%ebx
	ret	

.global vlc_encode_block_pass_1_x86	
vlc_encode_block_pass_1_x86:	
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp

	subl	$4, %esp
	
	movl	1*4+5*4(%esp), %esi                # start
	movl	(%esi), %esi
	movl	2*4+5*4(%esp), %edi		   # end
	movl	3*4+5*4(%esp), %eax                # bit_budget
	movl	(%eax), %eax
	movl	%eax, (%esp)
	movl	4*4+5*4(%esp), %ebp		   # bit_offset
	movl	(%ebp), %ebp
	/*      5*4+5*4(%esp)                      # vsbuffer */
	xorl	%ecx, %ecx
	xorl	%edx, %edx
	
vlc_encode_block_pass_1_x86_loop:	
	lodsl
	movb	%al, %cl
	
	subl	%ecx, (%esp)             # bit_budget -= len
	jl	vlc_encode_block_pass1_x86_out

	movl	%ebp, %ebx               # bit_offset
	negl	%ecx                     # -len
	
	andl	$7, %ebx                 # bit_offset & 7
	addl	$32, %ecx                # 32-len
	
	movb	%al, %dl                 # len
	subl	%ebx, %ecx               # 32-len-(bit_offset & 7)
	
	shrl	$8, %eax                 # value
	movl	%ebp, %ebx               # bit_offset
	
	shll	%cl, %eax                # value <<= 32-len-(bit_offset & 7)
	shrl	$3, %ebx                 # bit_offset >> 3
	
	bswap	%eax
	addl	5*4+5*4(%esp), %ebx      # vsbuffer + bit_offset >> 3
	
	addl	%edx, %ebp               # bit_offset += len
	orl	%eax, (%ebx)             # store value
		
	cmpl	%esi, %edi
	jnz	vlc_encode_block_pass_1_x86_loop
	
	xorl	%ecx, %ecx
	addl	$4, %esi
	
vlc_encode_block_pass1_x86_out:
	subl	$4, %esi
	addl	(%esp), %ecx            # bit_budget

	movl	1*4+5*4(%esp), %eax     # start
	movl	%esi, (%eax)
	
	movl	3*4+5*4(%esp), %eax     # bit_budget
	movl	%ecx, (%eax)

	movl	4*4+5*4(%esp), %eax     # bit_offset
	movl	%ebp, (%eax)

	addl	$4, %esp
	
	popl	%ebp	
	popl	%edi
	popl	%esi	
	popl	%ebx	
	ret		
		
.global classify_mmx
classify_mmx:

	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi

	movl	12(%ebp), %esi
	movq	(%esi), %mm7            # amp_ofs
	movl	16(%ebp), %esi
	movq	(%esi), %mm6            # amp_cmp

	movl    8(%ebp), %esi          # source

	movq	%mm7, %mm5
	movq	%mm6, %mm4
	
	pxor	%mm3, %mm3
	pxor	%mm2, %mm2

	movq	0*8(%esi), %mm0
	movq	1*8(%esi), %mm1
	
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 

	movq	2*8(%esi), %mm0
	movq	3*8(%esi), %mm1
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 

	movq	4*8(%esi), %mm0
	movq	5*8(%esi), %mm1
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 

	movq	6*8(%esi), %mm0
	movq	7*8(%esi), %mm1
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 

	movq	8*8(%esi), %mm0
	movq	9*8(%esi), %mm1
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 

	movq	10*8(%esi), %mm0
	movq	11*8(%esi), %mm1
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 

	movq	12*8(%esi), %mm0
	movq	13*8(%esi), %mm1
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 

	movq	14*8(%esi), %mm0
	movq	15*8(%esi), %mm1
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 
							
	paddw	%mm2, %mm3
	packsswb %mm3, %mm3

	movd	%mm3, %eax

	pop	%esi
	pop	%ebp
	ret

/* FIXME: reorder_block_mmx isn't really _that_ faster than the C version... 
	 don't know why... */
	
.global reorder_block_mmx
reorder_block_mmx:
				
	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi
	pushl	%edi
	pushl	%ebx
	pushl	%ecx
	pushl	%edx

	movl	8(%ebp), %esi      # source
	movl	12(%ebp), %edi     # reorder_table

	xorl	%ebp, %ebp
	xorl	%eax, %eax
	xorl	%ebx, %ebx
	xorl	%ecx, %ecx
	xorl	%edx, %edx

	subl	$128, %esp
	
reorder_loop:	
	
	movw	 (%esi, %ebp), %ax
	movw	2(%esi, %ebp), %bx

	movw	 (%edi, %ebp), %cx
	movw	2(%edi, %ebp), %dx

	movw	 %ax, (%esp,%ecx)
	movw	 %bx, (%esp,%edx)

	movw	4(%esi, %ebp), %ax
	movw	6(%esi, %ebp), %bx

	movw	4(%edi, %ebp), %cx
	movw	6(%edi, %ebp), %dx

	movw	 %ax, (%esp,%ecx)
	movw	 %bx, (%esp,%edx)

	movw	 8(%esi, %ebp), %ax
	movw	10(%esi, %ebp), %bx

	movw	8(%edi, %ebp), %cx
	movw	10(%edi, %ebp), %dx

	movw	 %ax, (%esp,%ecx)
	movw	 %bx, (%esp,%edx)

	movw	12(%esi, %ebp), %ax
	movw	14(%esi, %ebp), %bx

	movw	12(%edi, %ebp), %cx
	movw	14(%edi, %ebp), %dx

	movw	 %ax, (%esp,%ecx)
	movw	 %bx, (%esp,%edx)

	addl	$16, %ebp
	
	cmpl	$128, %ebp
	jne	reorder_loop
	
	movq	(%esp)  , %mm0
	movq	8(%esp) , %mm1
	movq	16(%esp), %mm2
	movq	24(%esp), %mm3

	movq	%mm0, (%esi)
	movq	%mm1, 8(%esi)
	movq	%mm2, 16(%esi)
	movq	%mm3, 24(%esi)

	movq	32(%esp)   , %mm0
	movq	32+8(%esp) , %mm1
	movq	32+16(%esp), %mm2
	movq	32+24(%esp), %mm3

	movq	%mm0, 32(%esi)
	movq	%mm1, 32+8(%esi)
	movq	%mm2, 32+16(%esi)
	movq	%mm3, 32+24(%esi)

	movq	64(%esp)   , %mm0
	movq	64+8(%esp) , %mm1
	movq	64+16(%esp), %mm2
	movq	64+24(%esp), %mm3

	movq	%mm0, 64(%esi)
	movq	%mm1, 64+8(%esi)
	movq	%mm2, 64+16(%esi)
	movq	%mm3, 64+24(%esi)

	movq	96(%esp)   , %mm0
	movq	96+8(%esp) , %mm1
	movq	96+16(%esp), %mm2
	movq	96+24(%esp), %mm3

	addl	$128, %esp
	
	movq	%mm0, 96(%esi)
	movq	%mm1, 96+8(%esi)
	movq	%mm2, 96+16(%esi)
	movq	%mm3, 96+24(%esi)
	
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%edi
	popl	%esi
	popl	%ebp
	ret

.global need_dct_248_mmx_rows
need_dct_248_mmx_rows:
	
	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi
	pushl	%edi

	movl	8(%ebp), %esi      # source

	movq	(0*8+0)*2(%esi), %mm0
	movq	(0*8+4)*2(%esi), %mm1
	psubw	(1*8+0)*2(%esi), %mm0
	psubw	(1*8+4)*2(%esi), %mm1
	movq	%mm0, %mm2
	movq	%mm1, %mm3
	psraw	$15, %mm2
	psraw	$15, %mm3
	pxor	%mm2, %mm0
	pxor	%mm3, %mm1
	psubw	%mm2, %mm0
	psubw	%mm3, %mm1
	
	movq	(1*8+0)*2(%esi), %mm4
	movq	(1*8+4)*2(%esi), %mm5
	psubw	(2*8+0)*2(%esi), %mm4
	psubw	(2*8+4)*2(%esi), %mm5
	movq	%mm4, %mm6
	movq	%mm5, %mm7
	psraw	$15, %mm6
	psraw	$15, %mm7
	pxor	%mm6, %mm4
	pxor	%mm7, %mm5
	psubw	%mm6, %mm4
	psubw	%mm7, %mm5

	paddw	%mm4, %mm0
	paddw	%mm5, %mm1

	movq	(2*8+0)*2(%esi), %mm4
	movq	(2*8+4)*2(%esi), %mm5
	psubw	(3*8+0)*2(%esi), %mm4
	psubw	(3*8+4)*2(%esi), %mm5
	movq	%mm4, %mm6
	movq	%mm5, %mm7
	psraw	$15, %mm6
	psraw	$15, %mm7
	pxor	%mm6, %mm4
	pxor	%mm7, %mm5
	psubw	%mm6, %mm4
	psubw	%mm7, %mm5

	paddw	%mm4, %mm0
	paddw	%mm5, %mm1

	movq	(3*8+0)*2(%esi), %mm4
	movq	(3*8+4)*2(%esi), %mm5
	psubw	(4*8+0)*2(%esi), %mm4
	psubw	(4*8+4)*2(%esi), %mm5
	movq	%mm4, %mm6
	movq	%mm5, %mm7
	psraw	$15, %mm6
	psraw	$15, %mm7
	pxor	%mm6, %mm4
	pxor	%mm7, %mm5
	psubw	%mm6, %mm4
	psubw	%mm7, %mm5

	paddw	%mm4, %mm0
	paddw	%mm5, %mm1

	movq	(4*8+0)*2(%esi), %mm4
	movq	(4*8+4)*2(%esi), %mm5
	psubw	(5*8+0)*2(%esi), %mm4
	psubw	(5*8+4)*2(%esi), %mm5
	movq	%mm4, %mm6
	movq	%mm5, %mm7
	psraw	$15, %mm6
	psraw	$15, %mm7
	pxor	%mm6, %mm4
	pxor	%mm7, %mm5
	psubw	%mm6, %mm4
	psubw	%mm7, %mm5

	paddw	%mm4, %mm0
	paddw	%mm5, %mm1

	movq	(5*8+0)*2(%esi), %mm4
	movq	(5*8+4)*2(%esi), %mm5
	psubw	(6*8+0)*2(%esi), %mm4
	psubw	(6*8+4)*2(%esi), %mm5
	movq	%mm4, %mm6
	movq	%mm5, %mm7
	psraw	$15, %mm6
	psraw	$15, %mm7
	pxor	%mm6, %mm4
	pxor	%mm7, %mm5
	psubw	%mm6, %mm4
	psubw	%mm7, %mm5

	paddw	%mm4, %mm0
	paddw	%mm5, %mm1

	movq	(6*8+0)*2(%esi), %mm4
	movq	(6*8+4)*2(%esi), %mm5
	psubw	(7*8+0)*2(%esi), %mm4
	psubw	(7*8+4)*2(%esi), %mm5
	movq	%mm4, %mm6
	movq	%mm5, %mm7
	psraw	$15, %mm6
	psraw	$15, %mm7
	pxor	%mm6, %mm4
	pxor	%mm7, %mm5
	psubw	%mm6, %mm4
	psubw	%mm7, %mm5

	paddw	%mm4, %mm0
	paddw	%mm5, %mm1

	paddw	%mm1, %mm0
	
	pmaddwd	ALLONE, %mm0	
	movq	%mm0, %mm1
	psrlq	$32, %mm1
	paddd	%mm1, %mm0
	
	movd	%mm0, %eax
	
	popl	%edi
	popl	%esi
	popl	%ebp

	ret




