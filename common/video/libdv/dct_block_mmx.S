/* 
 *  dct_block_mmx.S
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

# esi - input and output data pointer
# the input data each 16 bit element in the 8x8 matrix is left aligned:
# the output data is tranposed and each 16 bit element in the 8x8 matrix is left aligned:
# e.g. in 11...1110000 format
# israelh. 11/11/97 removed emms. moved to stub
# MMX implementation. Using MMX transpose 

#define YUV_PRECISION  1
	
#define NSHIFT     15
#define PRESHIFT   1
#define WA4_SHIFT  (NSHIFT-1)
#define WA5_SHIFT  (NSHIFT+1)


.data

.align 8	
			
WA1: .word 23171,23171,23171,23171  /* 0.70711 * 32768 */
WA2: .word 17734,17734,17734,17734  /* 0.54120 * 32768 */
WA3: .word 23171,23171,23171,23171  /* 0.70711 * 32768 */
WA4: .word 21407,21407,21407,21407  /* 1.30658 * 16384 */
WA5: .word 25079,25079,25079,25079  /* 0.38268 * 65536 */

scratch1:       .quad 0
scratch2:       .quad 0
scratch3:       .quad 0
scratch4:       .quad 0

.text

.align 8	
.global dct_88_block_mmx
dct_88_block_mmx:

	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi

	movl    8(%ebp), %esi          # source

# column 0
	movq 16*0(%esi), %mm0          # v0
	movq 16*7(%esi), %mm1          # v7
	movq  %mm0, %mm2               # duplicate v0 
	paddw %mm1, %mm0               # v00: v0+v7  
	psubw %mm1, %mm2               # v07: v0-v7  

	movq 16*1(%esi), %mm1          # v1
	movq 16*6(%esi), %mm3          # v6
	movq  %mm1, %mm4               # duplicate v1 
	paddw %mm3, %mm1               # v01: v1+v6  
	psubw %mm3, %mm4               # v06: v1-v6  

	movq 16*2(%esi), %mm3          # v2
	movq 16*5(%esi), %mm5          # v5
	movq  %mm3, %mm6               # duplicate v2 
	paddw %mm5, %mm3               # v02: v2+v5  
	psubw %mm5, %mm6               # v05: v2-v5  

	movq 16*3(%esi), %mm5          # v3
	movq 16*4(%esi), %mm7          # v4
	movq  %mm7, scratch1           # scratch1: v4   ; 
	movq  %mm5, %mm7               # duplicate v3 
	paddw scratch1, %mm5           # v03: v3+v4  
	psubw scratch1, %mm7           # v04: v3-v4  
	movq  %mm5, scratch2           # scratch2: v03
	movq  %mm0, %mm5               # mm5: v00

	paddw scratch2, %mm0           # v10: v00+v03   
	psubw scratch2, %mm5           # v13: v00-v03   
	movq  %mm3, scratch3           # scratch3: v02
	movq  %mm1, %mm3               # duplicate v01

	paddw scratch3, %mm1          # v11: v01+v02
	psubw scratch3, %mm3          # v12: v01-v02

	movq  %mm6, scratch4           # scratch4: v05
	movq  %mm0, %mm6               # duplicate v10

	paddw %mm1, %mm0              # v10+v11
	psubw %mm1, %mm6              # v10-v11

	movq  %mm0, (%esi)             # out0: v10+v11 
	movq  %mm6, 16*4(%esi)         # out4: v10-v11 

	movq  %mm4, %mm0               # mm0: v06
	paddw scratch4, %mm4          # v15: v05+v06 
	paddw  %mm2, %mm0             # v16: v07+v06

	pmulhw WA3, %mm4               # v35~: WA3*v15
	psllw  $1, %mm4                # v35: compensate the coeefient scale

	movq   %mm4, %mm6              # duplicate v35
	paddw  %mm2, %mm4             # v45: v07+v35
	psubw  %mm6, %mm2             # v47: v07-v35

	paddw  %mm5, %mm3             # v22: v12+v13

	pmulhw WA1, %mm3               # v32~: WA1*v22
	psllw  $16-NSHIFT, %mm3        # v32: compensate the coeefient scale
	movq   %mm5, %mm6              # duplicate v13

	paddw  %mm3, %mm5             # v13+v32
	psubw  %mm3, %mm6             # v13-v32

	movq  %mm5, 16*2(%esi)         # out2: v13+v32 
	movq  %mm6, 16*6(%esi)         # out6: v13-v32 


	paddw  scratch4, %mm7         # v14n: v04+v05
	movq   %mm0, %mm5              # duplicate v16

	psubw  %mm7, %mm0             # va1: v16-v14n
	pmulhw WA5, %mm0               # va0~:  va1*WA5
	pmulhw WA4, %mm5               # v36~~: v16*WA4
	pmulhw WA2, %mm7               # v34~~: v14n*WA2
	psllw  $16-WA4_SHIFT, %mm5     # v36: compensate the coeefient scale 
	psllw  $16-NSHIFT, %mm7        # v34: compensate the coeefient scale

	psubw  %mm0, %mm5              # v36~: v36~~-va0~
	psubw  %mm0, %mm7              # v34~: v34~~-va0~

	movq   %mm4, %mm0              # duplicate v45
	paddw  %mm5, %mm4              # v45+v36
	psubw  %mm5, %mm0              # v45-v36

	movq  %mm4, 16*1(%esi)         # out1: v45+v36 
	movq  %mm0, 16*7(%esi)         # out7: v45-v36 

	movq   %mm2, %mm5              # duplicate v47
	paddw  %mm7, %mm2              # v47+v34
	psubw  %mm7, %mm5              # v47-v34

	movq  %mm2, 16*5(%esi)         # out5: v47+v34 
	movq  %mm5, 16*3(%esi)         # out3: v47-v34 


# column 1

	addl $8, %esi                  # point to the next 4 columns. 
		                       # it can be done by adding 8 to 
		                       # immediates but this is nicer

	movq (%esi), %mm0                  # v0
	movq 16*7(%esi), %mm1              # v7
	movq  %mm0, %mm2               # duplicate v0 
	paddw %mm1, %mm0               # v00: v0+v7  
	psubw %mm1, %mm2               # v07: v0-v7  

	movq 16(%esi), %mm1                # v1
	movq 16*6(%esi), %mm3              # v6
	movq  %mm1, %mm4               # duplicate v1 
	paddw %mm3, %mm1               # v01: v1+v6  
	psubw %mm3, %mm4               # v06: v1-v6  

	movq 16*2(%esi), %mm3         # v2
	movq 16*5(%esi), %mm5              # v5
	movq  %mm3, %mm6               # duplicate v2 
	paddw %mm5, %mm3               # v02: v2+v5  
	psubw %mm5, %mm6               # v05: v2-v5  

	movq 16*3(%esi), %mm5              # v3
	movq 16*4(%esi), %mm7              # v4
	movq  %mm7, scratch1                    # scratch1: v4   ; 
	movq  %mm5, %mm7               # duplicate v3 
	paddw scratch1, %mm5           # v03: v3+v4  
	psubw scratch1, %mm7           # v04: v3-v4  
	movq  %mm5, scratch2        # scratch2: v03
	movq  %mm0, %mm5               # mm5: v00

	paddw scratch2, %mm0           # v10: v00+v03   
	psubw scratch2, %mm5           # v13: v00-v03   
	movq  %mm3, scratch3         # scratc3: v02
	movq  %mm1, %mm3               # duplicate v01

	paddw scratch3, %mm1           # v11: v01+v02
	psubw scratch3, %mm3           # v12: v01-v02

	movq  %mm6, scratch4         # scratc4: v05
	movq  %mm0, %mm6               # duplicate v10

	paddw %mm1, %mm0                            # v10+v11
	psubw %mm1, %mm6                        # v10-v11

	movq  %mm0, (%esi)         # out0: v10+v11 
	movq  %mm6, 16*4(%esi)          # out4: v10-v11 

	movq  %mm4, %mm0             # mm0: v06
	paddw scratch4, %mm4         # v15: v05+v06 
	paddw  %mm2, %mm0                       # v16: v07+v06

	pmulhw WA3, %mm4           # v35~: WA3*v15
	psllw  $16-NSHIFT, %mm4       # v35: compensate the coeefient scale

	movq   %mm4, %mm6            # duplicate v35
	paddw  %mm2, %mm4            # v45: v07+v35
	psubw  %mm6, %mm2            # v47: v07-v35

	paddw  %mm5, %mm3            # v22: v12+v13

	pmulhw WA1, %mm3           # v32~: WA3*v15
	psllw  $16-NSHIFT, %mm3       # v32: compensate the coeefient scale
	movq   %mm5, %mm6            # duplicate v13

	paddw  %mm3, %mm5            # v13+v32
	psubw  %mm3, %mm6            # v13-v32

	movq  %mm5, 16*2(%esi)          # out2: v13+v32 
	movq  %mm6, 16*6(%esi)          # out6: v13-v32 

	paddw  scratch4, %mm7                           # v14n: v04+v05
	movq   %mm0, %mm5                               # duplicate v16

	psubw  %mm7, %mm0                               # va1: v16-v14n
	pmulhw WA2, %mm7                # v34~~: v14n*WA2
	pmulhw WA5, %mm0                # va0~:  va1*WA5
	pmulhw WA4, %mm5                        # v36~~: v16*WA4
	psllw  $16-NSHIFT, %mm7
	psllw  $16-WA4_SHIFT, %mm5      # v36: compensate the coeffient 
		# scale note that WA4 is shifted 1 bit less than the others
	
	psubw  %mm0, %mm5   # v36~: v36~~-va0~
	psubw  %mm0, %mm7   # v34~: v34~~-va0~

	movq   %mm4, %mm0    # duplicate v45
	paddw  %mm5, %mm4    # v45+v36
	psubw  %mm5, %mm0    # v45-v36

	movq  %mm4, 16*1(%esi)          # out1: v45+v36 
	movq  %mm0, 16*7(%esi)          # out7: v45-v36 

	movq   %mm2, %mm5    # duplicate v47
	paddw  %mm7, %mm2    # v47+v34
	psubw  %mm7, %mm5    # v47-v34

	movq  %mm2, 16*5(%esi)          # out5: v47+v34 
	movq  %mm5, 16*3(%esi)          # out3: v47-v34 

	popl    %esi
	popl %ebp

	ret     $0

/* do mmx postscaling and reordering... */
		
.align 8	
.global dct_block_mmx_postscale_88
dct_block_mmx_postscale_88:

	pushl	 %ebp
	movl	 %esp,%ebp
	pushl	 %esi
	pushl	 %edi
	pushl	 %ebx

	movl	 8(%ebp),%esi		# source matrix 
	movl    12(%ebp), %edi          # postscale matrix 

	subl	$128+2, %esp
	
	movq	0*8(%esi), %mm0
	movq	1*8(%esi), %mm1
	movq	2*8(%esi), %mm2
	movq	3*8(%esi), %mm3

	movq	%mm0, %mm4
	movq	%mm1, %mm5
	movq	%mm2, %mm6
	movq	%mm3, %mm7
	psraw	$0xf, %mm4
	psraw	$0xf, %mm5
	psraw	$0xf, %mm6
	psraw	$0xf, %mm7

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
		
	pmulhw	0*8(%edi), %mm0
	pmulhw	1*8(%edi), %mm1
	pmulhw	2*8(%edi), %mm2
	pmulhw	3*8(%edi), %mm3
	
	psraw   $YUV_PRECISION, %mm0
	psraw   $YUV_PRECISION, %mm1
	psraw   $YUV_PRECISION, %mm2
	psraw   $YUV_PRECISION, %mm3

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
	
	movq	%mm0, 0*8(%esi)
	movq	%mm1, 1*8(%esi)
	movq	%mm2, 2*8(%esi)
	movq	%mm3, 3*8(%esi)

	movq	4*8(%esi), %mm0
	movq	5*8(%esi), %mm1
	movq	6*8(%esi), %mm2
	movq	7*8(%esi), %mm3

	movq	%mm0, %mm4
	movq	%mm1, %mm5

	movl	0*2(%esi), %eax
	movw	2*2(%esi), %bx

	movq	%mm2, %mm6
	movq	%mm3, %mm7

	movl	%eax, 1*2(%esp)
	movw	%bx, 6*2(%esp)

	psraw	$0xf, %mm4
	psraw	$0xf, %mm5

	movw	3*2(%esi), %ax
	movw	4*2(%esi), %bx
	
	psraw	$0xf, %mm6
	psraw	$0xf, %mm7

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1

	movw	%ax, 7*2(%esp)
	movw	%bx, 15*2(%esp)

	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	movw	5*2(%esi), %ax
	movw	6*2(%esi), %bx
	
	psubw	%mm4, %mm0
	psubw	%mm5, %mm1

	movw	%ax, 16*2(%esp)
	movw	%bx, 28*2(%esp)
	
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3

	movw	7*2(%esi), %ax
	movw	8*2(%esi), %bx
	
	pmulhw	4*8(%edi), %mm0
	pmulhw	5*8(%edi), %mm1

	movw	%ax, 29*2(%esp)
	movw	%bx, 3*2(%esp)
	
	pmulhw	6*8(%edi), %mm2
	pmulhw	7*8(%edi), %mm3

	movw	9*2(%esi), %ax
	movw	10*2(%esi), %bx
	
	psraw   $YUV_PRECISION, %mm0
	psraw   $YUV_PRECISION, %mm1

	movw	%ax, 5*2(%esp)
	movw	%bx, 8*2(%esp)
	
	psraw   $YUV_PRECISION, %mm2
	psraw   $YUV_PRECISION, %mm3

	movw	11*2(%esi), %ax
	movw	12*2(%esi), %bx
	
	pxor	%mm4, %mm0
	pxor	%mm5, %mm1

	movw	%ax, 14*2(%esp)
	movw	%bx, 17*2(%esp)
		
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	movw	13*2(%esi), %ax
	movw	14*2(%esi), %bx
		
	psubw	%mm4, %mm0
	psubw	%mm5, %mm1

	movw	%ax, 27*2(%esp)
	movw	%bx, 30*2(%esp)

	psubw	%mm6, %mm2
	psubw	%mm7, %mm3

	movq	%mm0, 4*8(%esi)
	movq	%mm1, 5*8(%esi)
	movq	%mm2, 6*8(%esi)
	movq	%mm3, 7*8(%esi)
	
	movq	8*8(%esi), %mm0
	movq	9*8(%esi), %mm1

	movw	15*2(%esi), %ax
	movw	16*2(%esi), %bx
	
	movq	10*8(%esi), %mm2
	movq	11*8(%esi), %mm3

	movw	%ax, 43*2(%esp)
	movw	%bx, 4*2(%esp)
	
	movq	%mm0, %mm4
	movq	%mm1, %mm5

	movw	17*2(%esi), %ax
	movw	18*2(%esi), %bx
	
	movq	%mm2, %mm6
	movq	%mm3, %mm7

	movw	%ax, 9*2(%esp)
	movw	%bx, 13*2(%esp)
	
	psraw	$0xf, %mm4
	psraw	$0xf, %mm5

	movw	19*2(%esi), %ax
	movw	20*2(%esi), %bx
		
	psraw	$0xf, %mm6
	psraw	$0xf, %mm7

	movw	%ax, 18*2(%esp)
	movw	%bx, 26*2(%esp)
		
	pxor	%mm4, %mm0
	pxor	%mm5, %mm1

	movw	21*2(%esi), %ax
	movw	22*2(%esi), %bx
	
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	movw	%ax, 31*2(%esp)
	movw	%bx, 42*2(%esp)

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1

	movw	23*2(%esi), %ax
	movw	24*2(%esi), %bx
	
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3

	movw	%ax, 44*2(%esp)
	movw	%bx, 10*2(%esp)
	
	pmulhw	8*8(%edi), %mm0
	pmulhw	9*8(%edi), %mm1

	movw	25*2(%esi), %ax
	movw	26*2(%esi), %bx
	
	pmulhw	10*8(%edi), %mm2
	pmulhw	11*8(%edi), %mm3

	psraw   $YUV_PRECISION, %mm0
	psraw   $YUV_PRECISION, %mm1

	movw	%ax, 12*2(%esp)
	movw	%bx, 19*2(%esp)
	
	psraw   $YUV_PRECISION, %mm2
	psraw   $YUV_PRECISION, %mm3

	movw	27*2(%esi), %ax
	movw	28*2(%esi), %bx
	
	pxor	%mm4, %mm0
	pxor	%mm5, %mm1

	movw	%ax, 25*2(%esp)
	movw	%bx, 32*2(%esp)
	
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	movw	29*2(%esi), %ax
	movw	30*2(%esi), %bx
	
	psubw	%mm4, %mm0
	psubw	%mm5, %mm1

	movw	%ax, 41*2(%esp)
	movw	%bx, 45*2(%esp)
	
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3

	movq	%mm0, 8*8(%esi)
	movq	%mm1, 9*8(%esi)
	movq	%mm2, 10*8(%esi)
	movq	%mm3, 11*8(%esi)

	
	movq	12*8(%esi), %mm0
	movq	13*8(%esi), %mm1
	movq	14*8(%esi), %mm2
	movq	15*8(%esi), %mm3

	movq	%mm0, %mm4
	movq	%mm1, %mm5

	movw	31*2(%esi), %ax
	movw	32*2(%esi), %bx
	
	movq	%mm2, %mm6
	movq	%mm3, %mm7

	movw	%ax, 54*2(%esp)
	movw	%bx, 11*2(%esp)
	
	psraw	$0xf, %mm4
	psraw	$0xf, %mm5

	movw	33*2(%esi), %ax
	movw	34*2(%esi), %bx
	
	psraw	$0xf, %mm6
	psraw	$0xf, %mm7

	movw	%ax, 20*2(%esp)
	movw	%bx, 24*2(%esp)
		
	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	
	movw	35*2(%esi), %ax
	movw	36*2(%esi), %bx
		
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3
	
	movw	%ax, 33*2(%esp)
	movw	%bx, 40*2(%esp)
	
	psubw	%mm4, %mm0
	psubw	%mm5, %mm1

	movw	37*2(%esi), %ax
	movw	38*2(%esi), %bx
	
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3

	movw	%ax, 46*2(%esp)
	movw	%bx, 53*2(%esp)
			
	pmulhw	12*8(%edi), %mm0
	pmulhw	13*8(%edi), %mm1

	movw	39*2(%esi), %ax
	movw	40*2(%esi), %bx
	
	pmulhw	14*8(%edi), %mm2
	pmulhw	15*8(%edi), %mm3
		
	movw	%ax, 55*2(%esp)
	movw	%bx, 21*2(%esp)
		
	psraw   $YUV_PRECISION, %mm0
	psraw   $YUV_PRECISION, %mm1

	movw	41*2(%esi), %ax
	movw	42*2(%esi), %bx
	
	psraw   $YUV_PRECISION, %mm2
	psraw   $YUV_PRECISION, %mm3

	movw	%ax, 23*2(%esp)
	movw	%bx, 34*2(%esp)

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1

	movw	43*2(%esi), %ax
	movw	44*2(%esi), %bx
	
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3
	
	movw	%ax, 39*2(%esp)
	movw	%bx, 47*2(%esp)
	
	psubw	%mm4, %mm0
	psubw	%mm5, %mm1

	movw	45*2(%esi), %ax
	movw	46*2(%esi), %bx
	
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3

	movw	%ax, 52*2(%esp)
	movw	%bx, 56*2(%esp)
	
	movq	%mm0, 12*8(%esi)
	movq	%mm1, 13*8(%esi)

	movw	47*2(%esi), %ax
	movw	48*2(%esi), %bx
	
	movq	%mm2, 14*8(%esi)
	movq	%mm3, 15*8(%esi)
	
	movw	%ax, 61*2(%esp)
	movw	%bx, 22*2(%esp)
	
	movw	49*2(%esi), %ax
	movw	50*2(%esi), %bx

	
	movw	%ax, 35*2(%esp)
	movw	%bx, 38*2(%esp)

	
	movw	51*2(%esi), %ax
	movw	52*2(%esi), %bx
	
	movw	%ax, 48*2(%esp)
	movw	%bx, 51*2(%esp)

	
	movw	53*2(%esi), %ax
	movw	54*2(%esi), %bx
		
	movw	%ax, 57*2(%esp)
	movw	%bx, 60*2(%esp)

	movw	55*2(%esi), %ax
	movw	56*2(%esi), %bx


	movw	%ax, 62*2(%esp)
	movw	%bx, 36*2(%esp)
	
	movw	57*2(%esi), %ax
	movw	58*2(%esi), %bx
	
	movw	%ax, 37*2(%esp)
	movw	%bx, 49*2(%esp)
	
	movw	59*2(%esi), %ax
	movw	60*2(%esi), %bx

	movw	%ax, 50*2(%esp)
	movw	%bx, 58*2(%esp)
	
	movw	61*2(%esi), %ax
	movl	62*2(%esi), %ebx
	
	movw	%ax, 59*2(%esp)
	movl	%ebx, 63*2(%esp)

	addl	$2, %esp
	
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
	
	popl	 %ebx			
	popl	 %edi
	popl	 %esi
	popl	 %ebp
	ret

.align 8	
.global dct_248_block_mmx
dct_248_block_mmx:

	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi
	pushl   %edi

	movl    8(%ebp), %esi          # source

# column 0

	movq 16*0(%esi), %mm0          # v00
	movq 16*2(%esi), %mm1          # v01
	movq 16*4(%esi), %mm2          # v02
	movq 16*6(%esi), %mm3          # v03

	movq %mm0, %mm4                # v00
	movq %mm1, %mm5                # v01
	
	paddw %mm3, %mm0               # v10: v00+v03
	psubw %mm3, %mm4	       # v13: v00-v03
	
	paddw %mm2, %mm1	       # v11: v01+v02
	psubw %mm2, %mm5	       # v12: v01-v02
	
	movq %mm0, %mm3                # v10

	paddw %mm4, %mm5               # v22: v12+v13	
	paddw %mm1, %mm0	       # v20: v10+v11
	psubw %mm1, %mm3	       # v21: v10-v11

	pmulhw WA1, %mm5               # v32~: WA1*v22
	movq  %mm4, %mm2	
	psllw  $16-NSHIFT, %mm5        # v32: compensate the coeffient scale

	movq  %mm0, 16*0(%esi)
	
	psubw %mm5, %mm2	       # v43: v13-v32	
	paddw %mm4, %mm5	       # v42: v32+v13

	movq  %mm2, 16*6(%esi)
	movq  %mm3, 16*4(%esi)
	movq  %mm5, 16*2(%esi)

# column 1

	addl $8, %esi # point to the next 4 columns. 
	              # it can be done by adding 8 to immediates 
	              # but this is nicer

	movq 16*0(%esi), %mm0          # v00
	movq 16*2(%esi), %mm1          # v01
	movq 16*4(%esi), %mm2          # v02
	movq 16*6(%esi), %mm3          # v03

	movq %mm0, %mm4
	movq %mm1, %mm5
	
	paddw %mm3, %mm0               # v10: v00+v03
	psubw %mm3, %mm4	       # v13: v00-v03
	
	paddw %mm2, %mm1	       # v11: v01+v02
	psubw %mm2, %mm5	       # v12: v01-v02
	
	movq %mm0, %mm3

	paddw %mm4, %mm5               # v22: v12+v13	
	paddw %mm1, %mm0	       # v20: v10+v11
	psubw %mm1, %mm3	       # v21: v10-v11

	pmulhw WA1, %mm5               # v32~: WA1*v22
	movq  %mm4, %mm2	
	psllw  $16-NSHIFT, %mm5        # v32: compensate the coeffient scale

	movq  %mm0, 16*0(%esi)
	
	psubw %mm5, %mm2	       # v43: v13-v32	
	paddw %mm4, %mm5	       # v42: v32+v13

	movq  %mm2, 16*6(%esi)
	movq  %mm3, 16*4(%esi)
	movq  %mm5, 16*2(%esi)

# column 0 ... second line		

	addl $8, %esi

	movq 16*0(%esi), %mm0          # v00
	movq 16*2(%esi), %mm1          # v01
	movq 16*4(%esi), %mm2          # v02
	movq 16*6(%esi), %mm3          # v03

	movq %mm0, %mm4
	movq %mm1, %mm5
	
	paddw %mm3, %mm0               # v10: v00+v03
	psubw %mm3, %mm4	       # v13: v00-v03
	
	paddw %mm2, %mm1	       # v11: v01+v02
	psubw %mm2, %mm5	       # v12: v01-v02
	
	movq %mm0, %mm3

	paddw %mm4, %mm5               # v22: v12+v13	
	paddw %mm1, %mm0	       # v20: v10+v11
	psubw %mm1, %mm3	       # v21: v10-v11

	pmulhw WA1, %mm5               # v32~: WA1*v22
	movq  %mm4, %mm2	
	psllw  $16-NSHIFT, %mm5        # v32: compensate the coeffient scale

	movq  %mm0, 16*0(%esi)
	
	psubw %mm5, %mm2	       # v43: v13-v32	
	paddw %mm4, %mm5	       # v42: v32+v13

	movq  %mm2, 16*6(%esi)
	movq  %mm3, 16*4(%esi)
	movq  %mm5, 16*2(%esi)

# column 1 ... second line		

	addl $8, %esi
	
	movq 16*0(%esi), %mm0          # v00
	movq 16*2(%esi), %mm1          # v01
	movq 16*4(%esi), %mm2          # v02
	movq 16*6(%esi), %mm3          # v03

	movq %mm0, %mm4
	movq %mm1, %mm5
	
	paddw %mm3, %mm0               # v10: v00+v03
	psubw %mm3, %mm4	       # v13: v00-v03
	
	paddw %mm2, %mm1	       # v11: v01+v02
	psubw %mm2, %mm5	       # v12: v01-v02
	
	movq %mm0, %mm3

	paddw %mm4, %mm5               # v22: v12+v13	
	paddw %mm1, %mm0	       # v20: v10+v11
	psubw %mm1, %mm3	       # v21: v10-v11

	pmulhw WA1, %mm5               # v32~: WA1*v22
	movq  %mm4, %mm2	
	psllw  $16-NSHIFT, %mm5        # v32: compensate the coeffient scale

	movq  %mm0, 16*0(%esi)
	
	psubw %mm5, %mm2	       # v43: v13-v32	
	paddw %mm4, %mm5	       # v42: v32+v13

	movq  %mm2, 16*6(%esi)
	movq  %mm3, 16*4(%esi)
	movq  %mm5, 16*2(%esi)
	
	popl	 %edi
	popl	 %esi
	popl	 %ebp
	ret

.align 8	
.global dct_248_block_mmx_post_sum
dct_248_block_mmx_post_sum:

	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi

	movl    8(%ebp), %esi          # source

	movq	16*0(%esi), %mm0
	movq	16*1(%esi), %mm1	
	movq	16*2(%esi), %mm2
	movq	16*3(%esi), %mm3	
	movq	16*4(%esi), %mm4
	movq	16*5(%esi), %mm5	
	movq	16*6(%esi), %mm6	
	movq	16*7(%esi), %mm7
	
	psubw	%mm1, %mm0
	paddw	16*0(%esi), %mm1 
	movq	%mm0, 16*4(%esi)
	movq	%mm1, 16*0(%esi)

	movq	%mm4, %mm1
	movq	%mm2, %mm0
	paddw	%mm5, %mm1
	psubw	%mm5, %mm4		
	paddw	%mm3, %mm0
	psubw	%mm3, %mm2

	movq	%mm0, 16*1(%esi)
	movq	%mm2, 16*5(%esi)			
	movq	%mm1, 16*2(%esi)
	movq	%mm4, 16*6(%esi)			
		
	paddw	%mm6, %mm7
	psubw	16*7(%esi), %mm6

	movq	%mm7, 16*3(%esi)
	movq	%mm6, 16*7(%esi)		
				
	addl	$8, %esi

	movq	16*0(%esi), %mm0
	movq	16*1(%esi), %mm1	
	movq	16*2(%esi), %mm2
	movq	16*3(%esi), %mm3	
	movq	16*4(%esi), %mm4
	movq	16*5(%esi), %mm5	
	movq	16*6(%esi), %mm6	
	movq	16*7(%esi), %mm7
	
	psubw	%mm1, %mm0
	paddw	16*0(%esi), %mm1
	movq	%mm0, 16*4(%esi)
	movq	%mm1, 16*0(%esi)

	movq	%mm4, %mm1
	movq	%mm2, %mm0
	paddw	%mm5, %mm1
	psubw	%mm5, %mm4		
	paddw	%mm3, %mm0
	psubw	%mm3, %mm2

	movq	%mm0, 16*1(%esi)
	movq	%mm2, 16*5(%esi)			
	movq	%mm1, 16*2(%esi)
	movq	%mm4, 16*6(%esi)			
		
	paddw	%mm6, %mm7
	psubw	16*7(%esi), %mm6

	movq	%mm7, 16*3(%esi)
	movq	%mm6, 16*7(%esi)		
	
	popl	%esi
	popl	%ebp
	ret		

.align 8	
.global dct_block_mmx_postscale_248
dct_block_mmx_postscale_248:

	pushl	 %ebp
	movl	 %esp,%ebp
	pushl	 %esi
	pushl	 %edi

	movl	 8(%ebp),%esi		# source matrix 
	movl    12(%ebp), %edi          # postscale matrix 

	movq	0*8(%esi), %mm0
	movq	1*8(%esi), %mm1
	movq	2*8(%esi), %mm2
	movq	3*8(%esi), %mm3

	movq	%mm0, %mm4
	movq	%mm1, %mm5
	movq	%mm2, %mm6
	movq	%mm3, %mm7
	psraw	$0xf, %mm4
	psraw	$0xf, %mm5
	psraw	$0xf, %mm6
	psraw	$0xf, %mm7

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
		
	pmulhw	0*8(%edi), %mm0
	pmulhw	1*8(%edi), %mm1
	pmulhw	2*8(%edi), %mm2
	pmulhw	3*8(%edi), %mm3
	
	psraw   $YUV_PRECISION, %mm0
	psraw   $YUV_PRECISION, %mm1
	psraw   $YUV_PRECISION, %mm2
	psraw   $YUV_PRECISION, %mm3

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
	
	movq	%mm0, 0*8(%esi)
	movq	%mm1, 1*8(%esi)
	movq	%mm2, 2*8(%esi)
	movq	%mm3, 3*8(%esi)

	
	movq	4*8(%esi), %mm0
	movq	5*8(%esi), %mm1
	movq	6*8(%esi), %mm2
	movq	7*8(%esi), %mm3

	movq	%mm0, %mm4
	movq	%mm1, %mm5
	movq	%mm2, %mm6
	movq	%mm3, %mm7
	psraw	$0xf, %mm4
	psraw	$0xf, %mm5
	psraw	$0xf, %mm6
	psraw	$0xf, %mm7

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
		
	pmulhw	4*8(%edi), %mm0
	pmulhw	5*8(%edi), %mm1
	pmulhw	6*8(%edi), %mm2
	pmulhw	7*8(%edi), %mm3
	
	psraw   $YUV_PRECISION, %mm0
	psraw   $YUV_PRECISION, %mm1
	psraw   $YUV_PRECISION, %mm2
	psraw   $YUV_PRECISION, %mm3

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
	
	movq	%mm0, 4*8(%esi)
	movq	%mm1, 5*8(%esi)
	movq	%mm2, 6*8(%esi)
	movq	%mm3, 7*8(%esi)
	

	movq	8*8(%esi), %mm0
	movq	9*8(%esi), %mm1
	movq	10*8(%esi), %mm2
	movq	11*8(%esi), %mm3

	movq	%mm0, %mm4
	movq	%mm1, %mm5
	movq	%mm2, %mm6
	movq	%mm3, %mm7
	psraw	$0xf, %mm4
	psraw	$0xf, %mm5
	psraw	$0xf, %mm6
	psraw	$0xf, %mm7

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
		
	pmulhw	8*8(%edi), %mm0
	pmulhw	9*8(%edi), %mm1
	pmulhw	10*8(%edi), %mm2
	pmulhw	11*8(%edi), %mm3
	
	psraw   $YUV_PRECISION, %mm0
	psraw   $YUV_PRECISION, %mm1
	psraw   $YUV_PRECISION, %mm2
	psraw   $YUV_PRECISION, %mm3

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
	
	movq	%mm0, 8*8(%esi)
	movq	%mm1, 9*8(%esi)
	movq	%mm2, 10*8(%esi)
	movq	%mm3, 11*8(%esi)

		
	movq	12*8(%esi), %mm0
	movq	13*8(%esi), %mm1
	movq	14*8(%esi), %mm2
	movq	15*8(%esi), %mm3

	movq	%mm0, %mm4
	movq	%mm1, %mm5
	movq	%mm2, %mm6
	movq	%mm3, %mm7
	psraw	$0xf, %mm4
	psraw	$0xf, %mm5
	psraw	$0xf, %mm6
	psraw	$0xf, %mm7

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
		
	pmulhw	12*8(%edi), %mm0
	pmulhw	13*8(%edi), %mm1
	pmulhw	14*8(%edi), %mm2
	pmulhw	15*8(%edi), %mm3
	
	psraw   $YUV_PRECISION, %mm0
	psraw   $YUV_PRECISION, %mm1
	psraw   $YUV_PRECISION, %mm2
	psraw   $YUV_PRECISION, %mm3

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
	
	movq	%mm0, 12*8(%esi)
	movq	%mm1, 13*8(%esi)
	movq	%mm2, 14*8(%esi)
	movq	%mm3, 15*8(%esi)
			
	popl	 %edi
	popl	 %esi
	popl	 %ebp
	ret
