/* 
 *  rgbtoyuv.S
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

	
# The loop processes interleaved RGB values for 8 pixels.
# The notation in the comments which describe the data locate
# the first byte on the right. For example in a register containing 
# G2R2B1G1R1B0G0R0, R0 is in the position of the lease significant
# byte and G2 is in the position of the most significant byte. 
# The output is to separate Y, U, and V buffers. Input are bytes,
# output are words 	
	
#define CONSTSHIFT 15
#define PRECISION  1
#define FIXPSHIFT  CONSTSHIFT-PRECISION
	
#define	DV_WIDTH_SHORT      720*2
#define	DV_WIDTH_BYTE       720
#define DV_WIDTH_SHORT_HALF 720
#define DV_WIDTH_BYTE_HALF  360	
		
.global rgbtoycb_mmx
# .global yuvtoycb_mmx

.data

.align 8
ZEROSX: .word   0,0,0,0
ZEROS:  .long   0,0

ALLONE:	.word	1,1,1,1
	
OFFSETDX:       .word   0,64,0,64       #offset used before shift 
OFFSETD:        .long   0,0
OFFSETWX:       .word   128,0,128,0     #offset used before pack 32
OFFSETW:        .long   0,0
OFFSETBX:       .word   128,128,128,128
OFFSETB:        .long   0,0
OFFSETY:	.word	(-128+16) << PRECISION 
		.word   (-128+16) << PRECISION 
		.word   (-128+16) << PRECISION
		.word   (-128+16) << PRECISION
	
TEMP0:          .long   0,0
TEMPY:  .long   0,0
TEMPU:          .long   0,0
TEMPV:  .long   0,0

#if 0 /* Original YUV */
YR0GRX: .word   9798,19235,0,9798
YBG0BX: .word   3736,0,19235,3736
YR0GR:  .long   0,0
YBG0B:  .long   0,0
UR0GRX: .word   -4784,-9437,0,-4784
UBG0BX: .word   14221,0,-9437,14221
UR0GR:  .long   0,0
UBG0B:  .long   0,0
VR0GRX: .word   20218,-16941,0,20218
VBG0BX: .word   -3277,0,-16941,-3277
VR0GR:  .long   0,0
VBG0B:  .long   0,0

YR0GRX: .word   8420,16529,0,8420
YBG0BX: .word   3203,0,16529,3203
YR0GR:  .long   0,0
YBG0B:  .long   0,0
UR0GRX: .word   14391,-12055,0,14391
UBG0BX: .word   -2336,0,-12055,-2336
UR0GR:  .long   0,0
UBG0B:  .long   0,0
VR0GRX: .word   -4857,-9534,0,-4857
VBG0BX: .word   14391,0,-9534,14391
VR0GR:  .long   0,0
VBG0B:  .long   0,0
	
#else
YR0GRX: .word   8414,16519,0,8414
YBG0BX: .word   3208,0,16519,3208
YR0GR:  .long   0,0
YBG0B:  .long   0,0
UR0GRX: .word   14392,-12061,0,14392
UBG0BX: .word   -2332,0,-12061,-2332
UR0GR:  .long   0,0
UBG0B:  .long   0,0
VR0GRX: .word   -4864,-9528,0,-4864
VBG0BX: .word   14392,0,-9528,14392
VR0GR:  .long   0,0
VBG0B:  .long   0,0
	
#endif	
	
.text

#define _inPtr     8
#define _rows      12
#define _columns   16
#define _outyPtr   20
#define _outuPtr   24
#define _outvPtr   28

rgbtoycb_mmx:

	pushl   %ebp
	movl    %esp, %ebp
	pushl   %eax
	pushl   %ebx
	pushl   %ecx
	pushl   %edx
	pushl   %esi
	pushl   %edi

	leal    ZEROSX, %eax    #This section gets around a bug
	movq    (%eax), %mm0    #unlikely to persist
	movq    %mm0, ZEROS
	leal    OFFSETDX, %eax
	movq    (%eax), %mm0
	movq    %mm0, OFFSETD
	leal    OFFSETWX, %eax
	movq    (%eax), %mm0
	movq    %mm0, OFFSETW
	leal    OFFSETBX, %eax
	movq    (%eax), %mm0
	movq    %mm0, OFFSETB
	leal    YR0GRX, %eax
	movq    (%eax), %mm0
	movq    %mm0, YR0GR
	leal    YBG0BX, %eax
	movq    (%eax), %mm0
	movq    %mm0, YBG0B
	leal    UR0GRX, %eax
	movq    (%eax), %mm0
	movq    %mm0, UR0GR
	leal    UBG0BX, %eax
	movq    (%eax), %mm0
	movq    %mm0, UBG0B
	leal    VR0GRX, %eax
	movq    (%eax), %mm0
	movq    %mm0, VR0GR
	leal    VBG0BX, %eax
	movq    (%eax), %mm0
	movq    %mm0, VBG0B
	
	movl    _rows(%ebp), %eax
	movl    _columns(%ebp), %ebx
	mull    %ebx            #number pixels
	shrl    $3, %eax        #number of loops
	movl    %eax, %edi      #loop counter in edi
	movl    _inPtr(%ebp), %eax
	movl    _outyPtr(%ebp), %ebx
	movl    _outuPtr(%ebp), %ecx
	movl    _outvPtr(%ebp), %edx
rgbtoycb_mmx_loop: 
	movq    (%eax), %mm1    #load G2R2B1G1R1B0G0R0
	pxor    %mm6, %mm6      #0 -> mm6

	movq    %mm1, %mm0      #G2R2B1G1R1B0G0R0 -> mm0
	psrlq   $16, %mm1       #00G2R2B1G1R1B0-> mm1

	punpcklbw %mm6, %mm0     #R1B0G0R0 -> mm0
	movq    %mm1, %mm7      #00G2R2B1G1R1B0-> mm7

	punpcklbw %mm6, %mm1     #B1G1R1B0 -> mm1
	movq    %mm0, %mm2      #R1B0G0R0 -> mm2

	pmaddwd YR0GR, %mm0     #yrR1,ygG0+yrR0 -> mm0
	movq    %mm1, %mm3      #B1G1R1B0 -> mm3

	pmaddwd YBG0B, %mm1     #ybB1+ygG1,ybB0 -> mm1
	movq    %mm2, %mm4      #R1B0G0R0 -> mm4

	pmaddwd UR0GR, %mm2     #urR1,ugG0+urR0 -> mm2
	movq    %mm3, %mm5      #B1G1R1B0 -> mm5

	pmaddwd UBG0B, %mm3     #ubB1+ugG1,ubB0 -> mm3
	punpckhbw       %mm6, %mm7 #    00G2R2 -> mm7

	pmaddwd VR0GR, %mm4     #vrR1,vgG0+vrR0 -> mm4
	paddd   %mm1, %mm0      #Y1Y0 -> mm0

	pmaddwd VBG0B, %mm5     #vbB1+vgG1,vbB0 -> mm5

	movq    8(%eax), %mm1   #R5B4G4R4B3G3R3B2 -> mm1
	paddd   %mm3, %mm2      #U1U0 -> mm2

	movq    %mm1, %mm6      #R5B4G4R4B3G3R3B2 -> mm6

	punpcklbw       ZEROS, %mm1     #B3G3R3B2 -> mm1
	paddd   %mm5, %mm4      #V1V0 -> mm4

	movq    %mm1, %mm5      #B3G3R3B2 -> mm5
	psllq   $32, %mm1       #R3B200 -> mm1

	paddd   %mm7, %mm1      #R3B200+00G2R2=R3B2G2R2->mm1

	punpckhbw       ZEROS, %mm6     #R5B4G4R3 -> mm6
	movq    %mm1, %mm3      #R3B2G2R2 -> mm3

	pmaddwd YR0GR, %mm1     #yrR3,ygG2+yrR2 -> mm1
	movq    %mm5, %mm7      #B3G3R3B2 -> mm7

	pmaddwd YBG0B, %mm5     #ybB3+ygG3,ybB2 -> mm5
	psrad   $FIXPSHIFT, %mm0       #32-bit scaled Y1Y0 -> mm0

	movq    %mm6, TEMP0     #R5B4G4R4 -> TEMP0
	movq    %mm3, %mm6      #R3B2G2R2 -> mm6
	pmaddwd UR0GR, %mm6     #urR3,ugG2+urR2 -> mm6
	psrad   $FIXPSHIFT, %mm2       #32-bit scaled U1U0 -> mm2

	paddd   %mm5, %mm1      #Y3Y2 -> mm1
	movq    %mm7, %mm5      #B3G3R3B2 -> mm5
	pmaddwd UBG0B, %mm7     #ubB3+ugG3,ubB2
	psrad   $FIXPSHIFT, %mm1 #32-bit scaled Y3Y2 -> mm1

	pmaddwd VR0GR, %mm3     #vrR3,vgG2+vgR2
	packssdw        %mm1, %mm0      #Y3Y2Y1Y0 -> mm0

	pmaddwd VBG0B, %mm5     #vbB3+vgG3,vbB2 -> mm5
	psrad   $FIXPSHIFT, %mm4       #32-bit scaled V1V0 -> mm4

	movq    16(%eax), %mm1  #B7G7R7B6G6R6B5G5 -> mm7
	paddd   %mm7, %mm6      #U3U2 -> mm6    

	movq    %mm1, %mm7      #B7G7R7B6G6R6B5G5 -> mm1
	psrad   $FIXPSHIFT, %mm6       #32-bit scaled U3U2 -> mm6

	paddd   %mm5, %mm3      #V3V2 -> mm3
	psllq   $16, %mm7       #R7B6G6R6B5G500 -> mm7

	movq    %mm7, %mm5      #R7B6G6R6B5G500 -> mm5
	psrad   $FIXPSHIFT, %mm3       #32-bit scaled V3V2 -> mm3

	paddw	OFFSETY, %mm0
	movq    %mm0, (%ebx)     #store Y3Y2Y1Y0 
	packssdw %mm6, %mm2      #32-bit scaled U3U2U1U0 -> mm2

	movq    TEMP0, %mm0     #R5B4G4R4 -> mm0
	addl	$8, %ebx
	
	punpcklbw       ZEROS, %mm7     #B5G500 -> mm7
	movq    %mm0, %mm6      #R5B4G4R4 -> mm6

	movq    %mm2, TEMPU     #32-bit scaled U3U2U1U0 -> TEMPU
	psrlq   $32, %mm0       #00R5B4 -> mm0

	paddw   %mm0, %mm7      #B5G5R5B4 -> mm7
	movq    %mm6, %mm2      #B5B4G4R4 -> mm2

	pmaddwd YR0GR, %mm2     #yrR5,ygG4+yrR4 -> mm2
	movq    %mm7, %mm0      #B5G5R5B4 -> mm0

	pmaddwd YBG0B, %mm7     #ybB5+ygG5,ybB4 -> mm7
	packssdw        %mm3, %mm4      #32-bit scaled V3V2V1V0 -> mm4

	addl    $24, %eax       #increment RGB count

	movq    %mm4, TEMPV     #(V3V2V1V0)/256 -> mm4
	movq    %mm6, %mm4      #B5B4G4R4 -> mm4

	pmaddwd UR0GR, %mm6     #urR5,ugG4+urR4
	movq    %mm0, %mm3      #B5G5R5B4 -> mm0

	pmaddwd UBG0B, %mm0     #ubB5+ugG5,ubB4
	paddd   %mm7, %mm2      #Y5Y4 -> mm2

	pmaddwd         VR0GR, %mm4     #vrR5,vgG4+vrR4 -> mm4
	pxor    %mm7, %mm7      #0 -> mm7

	pmaddwd VBG0B, %mm3     #vbB5+vgG5,vbB4 -> mm3
	punpckhbw       %mm7, %mm1      #B7G7R7B6 -> mm1

	paddd   %mm6, %mm0      #U5U4 -> mm0
	movq    %mm1, %mm6      #B7G7R7B6 -> mm6

	pmaddwd YBG0B, %mm6     #ybB7+ygG7,ybB6 -> mm6
	punpckhbw       %mm7, %mm5      #R7B6G6R6 -> mm5

	movq    %mm5, %mm7      #R7B6G6R6 -> mm7
	paddd   %mm4, %mm3      #V5V4 -> mm3

	pmaddwd YR0GR, %mm5     #yrR7,ygG6+yrR6 -> mm5
	movq    %mm1, %mm4      #B7G7R7B6 -> mm4

	pmaddwd UBG0B, %mm4     #ubB7+ugG7,ubB6 -> mm4
	psrad   $FIXPSHIFT, %mm0       #32-bit scaled U5U4 -> mm0

	psrad   $FIXPSHIFT, %mm2       #32-bit scaled Y5Y4 -> mm2

	paddd   %mm5, %mm6      #Y7Y6 -> mm6
	movq    %mm7, %mm5      #R7B6G6R6 -> mm5

	pmaddwd UR0GR, %mm7     #urR7,ugG6+ugR6 -> mm7
	psrad   $FIXPSHIFT, %mm3       #32-bit scaled V5V4 -> mm3

	pmaddwd VBG0B, %mm1     #vbB7+vgG7,vbB6 -> mm1
	psrad   $FIXPSHIFT, %mm6 #32-bit scaled Y7Y6 -> mm6

	packssdw %mm6, %mm2     #Y7Y6Y5Y4 -> mm2

	pmaddwd VR0GR, %mm5     #vrR7,vgG6+vrR6 -> mm5
	paddd   %mm4, %mm7      #U7U6 -> mm7    

	psrad   $FIXPSHIFT, %mm7       #32-bit scaled U7U6 -> mm7
	paddw	OFFSETY, %mm2
	movq	%mm2, (%ebx)    #store Y7Y6Y5Y4 

	movq	ALLONE, %mm6
	packssdw %mm7, %mm0     #32-bit scaled U7U6U5U4 -> mm0

	movq    TEMPU, %mm4     #32-bit scaled U3U2U1U0 -> mm4
	pmaddwd	%mm6, %mm0      #U7U6U5U4 averaged -> (U7U6)(U5U4)=UU3 UU2->mm0
	
	pmaddwd	%mm6, %mm4      #U3U2U1U0 averaged -> (U3U2)(U1U0)=UU1 UU0->mm4

	paddd   %mm5, %mm1      #V7V6 -> mm1
	packssdw %mm0, %mm4     #UU3UU2UU1UU0 -> mm4

	psrad   $FIXPSHIFT, %mm1       #32-bit scaled V7V6 -> mm1
	psraw	$1, %mm4 	#divide UU3 UU2 UU1 UU0 by 2 -> mm4
		
	movq    TEMPV, %mm5     #32-bit scaled V3V2V1V0 -> mm5

	movq	%mm4, (%ecx)    # store U	

	pmaddwd %mm6, %mm5	#V3V2V1V0 averaged -> VV1 VV0 -> mm5
	packssdw %mm1, %mm3     #V7V6V5V4 -> mm3

	pmaddwd %mm6, %mm3	#V7V6V5V4 averaged -> VV3 VV2 -> mm3

	packssdw %mm3, %mm5     # VV3 VV2 VV1 VV0 -> mm5
	psraw	$1, %mm5

	addl    $8, %ebx        #increment Y count
	addl    $8, %ecx        #increment U count

	movq    %mm5, (%edx)    #store V

	addl    $8, %edx        #increment V count
	
	decl    %edi            #decrement loop counter
	jnz     rgbtoycb_mmx_loop  #do 24 more bytes if not 0

	popl    %edi
	popl    %esi
	popl    %edx
	popl    %ecx
	popl    %ebx
	popl    %eax
	popl    %ebp

	ret     

.global ppm_copy_y_block_mmx
ppm_copy_y_block_mmx:

	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi
	pushl	%edi
	
	movl    8(%ebp), %edi          # dest
	movl    12(%ebp), %esi         # src
	
	movq	(%esi), %mm0
	movq	8(%esi), %mm1
	movq	%mm0, 0*8(%edi)
	movq	%mm1, 1*8(%edi) 
	movq	DV_WIDTH_SHORT(%esi), %mm2
	movq	DV_WIDTH_SHORT+8(%esi), %mm3
	movq	%mm2, 2*8(%edi)
	movq	%mm3, 3*8(%edi) 
	movq	DV_WIDTH_SHORT*2(%esi), %mm4
	movq	DV_WIDTH_SHORT*2+8(%esi), %mm5
	movq	%mm4, 4*8(%edi)
	movq	%mm5, 5*8(%edi) 
	movq	DV_WIDTH_SHORT*3(%esi), %mm6
	movq	DV_WIDTH_SHORT*3+8(%esi), %mm7
	movq	%mm6, 6*8(%edi)
	movq	%mm7, 7*8(%edi) 

	movq	DV_WIDTH_SHORT*4(%esi), %mm0
	movq	DV_WIDTH_SHORT*4+8(%esi), %mm1
	movq	%mm0, 8*8(%edi)
	movq	%mm1, 9*8(%edi) 
	movq	DV_WIDTH_SHORT*5(%esi), %mm2
	movq	DV_WIDTH_SHORT*5+8(%esi), %mm3
	movq	%mm2, 10*8(%edi)
	movq	%mm3, 11*8(%edi) 
	movq	DV_WIDTH_SHORT*6(%esi), %mm4
	movq	DV_WIDTH_SHORT*6+8(%esi), %mm5
	movq	%mm4, 12*8(%edi)
	movq	%mm5, 13*8(%edi) 
	movq	DV_WIDTH_SHORT*7(%esi), %mm6
	movq	DV_WIDTH_SHORT*7+8(%esi), %mm7
	movq	%mm6, 14*8(%edi)
	movq	%mm7, 15*8(%edi) 
	
	pop	%edi
	pop	%esi
	pop	%ebp
	ret

.global pgm_copy_y_block_mmx
pgm_copy_y_block_mmx:

	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi
	pushl	%edi
	
	movl    8(%ebp), %edi          # dest
	movl    12(%ebp), %esi         # src

	movq	OFFSETY, %mm7
	pxor	%mm6, %mm6
	
	movq	(%esi), %mm0
	movq	DV_WIDTH_BYTE(%esi), %mm1

	movq	%mm0, %mm2
	movq	%mm1, %mm3

	punpcklbw %mm6, %mm0
	punpcklbw %mm6, %mm1

	punpckhbw %mm6, %mm2
	punpckhbw %mm6, %mm3
	
#if PRECISION > 0	
	psllw	$PRECISION, %mm0
	psllw	$PRECISION, %mm1
	psllw	$PRECISION, %mm2
	psllw	$PRECISION, %mm3
#endif
		
	paddw	%mm7, %mm0
	paddw	%mm7, %mm1
	paddw	%mm7, %mm2
	paddw	%mm7, %mm3

	movq	%mm0, (%edi)
	movq	%mm2, 8(%edi)
	movq	%mm1, 16(%edi)
	movq	%mm3, 24(%edi)

	addl	$2*DV_WIDTH_BYTE, %esi
	addl	$32, %edi

	movq	(%esi), %mm0
	movq	DV_WIDTH_BYTE(%esi), %mm1

	movq	%mm0, %mm2
	movq	%mm1, %mm3

	punpcklbw %mm6, %mm0
	punpcklbw %mm6, %mm1

	punpckhbw %mm6, %mm2
	punpckhbw %mm6, %mm3
	
#if PRECISION > 0	
	psllw	$PRECISION, %mm0
	psllw	$PRECISION, %mm1
	psllw	$PRECISION, %mm2
	psllw	$PRECISION, %mm3
#endif

	paddw	%mm7, %mm0
	paddw	%mm7, %mm1
	paddw	%mm7, %mm2
	paddw	%mm7, %mm3
		
	movq	%mm0, (%edi)
	movq	%mm2, 8(%edi)
	movq	%mm1, 16(%edi)
	movq	%mm3, 24(%edi)

	addl	$2*DV_WIDTH_BYTE, %esi
	addl	$32, %edi

	movq	(%esi), %mm0
	movq	DV_WIDTH_BYTE(%esi), %mm1

	movq	%mm0, %mm2
	movq	%mm1, %mm3

	punpcklbw %mm6, %mm0
	punpcklbw %mm6, %mm1

	punpckhbw %mm6, %mm2
	punpckhbw %mm6, %mm3
	
#if PRECISION > 0	
	psllw	$PRECISION, %mm0
	psllw	$PRECISION, %mm1
	psllw	$PRECISION, %mm2
	psllw	$PRECISION, %mm3
#endif
	paddw	%mm7, %mm0
	paddw	%mm7, %mm1
	paddw	%mm7, %mm2
	paddw	%mm7, %mm3
		
	movq	%mm0, (%edi)
	movq	%mm2, 8(%edi)
	movq	%mm1, 16(%edi)
	movq	%mm3, 24(%edi)

	addl	$2*DV_WIDTH_BYTE, %esi
	addl	$32, %edi

	movq	(%esi), %mm0
	movq	DV_WIDTH_BYTE(%esi), %mm1

	movq	%mm0, %mm2
	movq	%mm1, %mm3

	punpcklbw %mm6, %mm0
	punpcklbw %mm6, %mm1

	punpckhbw %mm6, %mm2
	punpckhbw %mm6, %mm3
	
#if PRECISION > 0	
	psllw	$PRECISION, %mm0
	psllw	$PRECISION, %mm1
	psllw	$PRECISION, %mm2
	psllw	$PRECISION, %mm3
#endif
	paddw	%mm7, %mm0
	paddw	%mm7, %mm1
	paddw	%mm7, %mm2
	paddw	%mm7, %mm3
		
	movq	%mm0, (%edi)
	movq	%mm2, 8(%edi)
	movq	%mm1, 16(%edi)
	movq	%mm3, 24(%edi)

	pop	%edi
	pop	%esi
	pop	%ebp
	ret

.global video_copy_y_block_mmx
video_copy_y_block_mmx:

	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi
	pushl	%edi
	
	movl    8(%ebp), %edi          # dest
	movl    12(%ebp), %esi         # src

	movq	OFFSETBX, %mm7
	pxor	%mm6, %mm6
	
	movq	(%esi), %mm0
	movq	DV_WIDTH_BYTE(%esi), %mm1

	movq	%mm0, %mm2
	movq	%mm1, %mm3

	punpcklbw %mm6, %mm0
	punpcklbw %mm6, %mm1

	punpckhbw %mm6, %mm2
	punpckhbw %mm6, %mm3
	
	psubw	%mm7, %mm0
	psubw	%mm7, %mm1
	psubw	%mm7, %mm2
	psubw	%mm7, %mm3

#if PRECISION > 0	
	psllw	$PRECISION, %mm0
	psllw	$PRECISION, %mm1
	psllw	$PRECISION, %mm2
	psllw	$PRECISION, %mm3
#endif
		
	movq	%mm0, (%edi)
	movq	%mm2, 8(%edi)
	movq	%mm1, 16(%edi)
	movq	%mm3, 24(%edi)

	addl	$2*DV_WIDTH_BYTE, %esi
	addl	$32, %edi

	movq	(%esi), %mm0
	movq	DV_WIDTH_BYTE(%esi), %mm1

	movq	%mm0, %mm2
	movq	%mm1, %mm3

	punpcklbw %mm6, %mm0
	punpcklbw %mm6, %mm1

	punpckhbw %mm6, %mm2
	punpckhbw %mm6, %mm3
	
	psubw	%mm7, %mm0
	psubw	%mm7, %mm1
	psubw	%mm7, %mm2
	psubw	%mm7, %mm3

#if PRECISION > 0	
	psllw	$PRECISION, %mm0
	psllw	$PRECISION, %mm1
	psllw	$PRECISION, %mm2
	psllw	$PRECISION, %mm3
#endif
		
	movq	%mm0, (%edi)
	movq	%mm2, 8(%edi)
	movq	%mm1, 16(%edi)
	movq	%mm3, 24(%edi)

	addl	$2*DV_WIDTH_BYTE, %esi
	addl	$32, %edi

	movq	(%esi), %mm0
	movq	DV_WIDTH_BYTE(%esi), %mm1

	movq	%mm0, %mm2
	movq	%mm1, %mm3

	punpcklbw %mm6, %mm0
	punpcklbw %mm6, %mm1

	punpckhbw %mm6, %mm2
	punpckhbw %mm6, %mm3
	
	psubw	%mm7, %mm0
	psubw	%mm7, %mm1
	psubw	%mm7, %mm2
	psubw	%mm7, %mm3

#if PRECISION > 0	
	psllw	$PRECISION, %mm0
	psllw	$PRECISION, %mm1
	psllw	$PRECISION, %mm2
	psllw	$PRECISION, %mm3
#endif
		
	movq	%mm0, (%edi)
	movq	%mm2, 8(%edi)
	movq	%mm1, 16(%edi)
	movq	%mm3, 24(%edi)

	addl	$2*DV_WIDTH_BYTE, %esi
	addl	$32, %edi

	movq	(%esi), %mm0
	movq	DV_WIDTH_BYTE(%esi), %mm1

	movq	%mm0, %mm2
	movq	%mm1, %mm3

	punpcklbw %mm6, %mm0
	punpcklbw %mm6, %mm1

	punpckhbw %mm6, %mm2
	punpckhbw %mm6, %mm3
	
	psubw	%mm7, %mm0
	psubw	%mm7, %mm1
	psubw	%mm7, %mm2
	psubw	%mm7, %mm3

#if PRECISION > 0	
	psllw	$PRECISION, %mm0
	psllw	$PRECISION, %mm1
	psllw	$PRECISION, %mm2
	psllw	$PRECISION, %mm3
#endif
		
	movq	%mm0, (%edi)
	movq	%mm2, 8(%edi)
	movq	%mm1, 16(%edi)
	movq	%mm3, 24(%edi)

	pop	%edi
	pop	%esi
	pop	%ebp
	ret
	
		
.global ppm_copy_pal_c_block_mmx
ppm_copy_pal_c_block_mmx:
				
	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi
	pushl	%edi
	pushl	%ebx
	
	movl    8(%ebp), %edi          # dest
	movl    12(%ebp), %esi         # src

	movq	          (%esi), %mm0
	movq	DV_WIDTH_SHORT_HALF(%esi), %mm1
	movq               8(%esi), %mm2
	movq	DV_WIDTH_SHORT_HALF+8(%esi), %mm3

	paddw	%mm0, %mm1
	paddw	%mm2, %mm3
	psraw	$1, %mm1
	psraw	$1, %mm3

	movq	%mm1, 0*8(%edi)
	movq	%mm3, 1*8(%edi)

	addl	$DV_WIDTH_SHORT, %esi
	addl	$16, %edi

	movq	          (%esi), %mm0
	movq	DV_WIDTH_SHORT_HALF(%esi), %mm1
	movq               8(%esi), %mm2
	movq	DV_WIDTH_SHORT_HALF+8(%esi), %mm3

	paddw	%mm0, %mm1
	paddw	%mm2, %mm3
	psraw	$1, %mm1
	psraw	$1, %mm3

	movq	%mm1, 0*8(%edi)
	movq	%mm3, 1*8(%edi)

	addl	$DV_WIDTH_SHORT, %esi
	addl	$16, %edi

	movq	          (%esi), %mm0
	movq	DV_WIDTH_SHORT_HALF(%esi), %mm1
	movq               8(%esi), %mm2
	movq	DV_WIDTH_SHORT_HALF+8(%esi), %mm3

	paddw	%mm0, %mm1
	paddw	%mm2, %mm3
	psraw	$1, %mm1
	psraw	$1, %mm3

	movq	%mm1, 0*8(%edi)
	movq	%mm3, 1*8(%edi)

	addl	$DV_WIDTH_SHORT, %esi
	addl	$16, %edi

	movq	          (%esi), %mm0
	movq	DV_WIDTH_SHORT_HALF(%esi), %mm1
	movq               8(%esi), %mm2
	movq	DV_WIDTH_SHORT_HALF+8(%esi), %mm3

	paddw	%mm0, %mm1
	paddw	%mm2, %mm3
	psraw	$1, %mm1
	psraw	$1, %mm3

	movq	%mm1, 0*8(%edi)
	movq	%mm3, 1*8(%edi)

	addl	$DV_WIDTH_SHORT, %esi
	addl	$16, %edi

	movq	          (%esi), %mm0
	movq	DV_WIDTH_SHORT_HALF(%esi), %mm1
	movq               8(%esi), %mm2
	movq	DV_WIDTH_SHORT_HALF+8(%esi), %mm3

	paddw	%mm0, %mm1
	paddw	%mm2, %mm3
	psraw	$1, %mm1
	psraw	$1, %mm3

	movq	%mm1, 0*8(%edi)
	movq	%mm3, 1*8(%edi)

	addl	$DV_WIDTH_SHORT, %esi
	addl	$16, %edi

	movq	          (%esi), %mm0
	movq	DV_WIDTH_SHORT_HALF(%esi), %mm1
	movq               8(%esi), %mm2
	movq	DV_WIDTH_SHORT_HALF+8(%esi), %mm3

	paddw	%mm0, %mm1
	paddw	%mm2, %mm3
	psraw	$1, %mm1
	psraw	$1, %mm3

	movq	%mm1, 0*8(%edi)
	movq	%mm3, 1*8(%edi)

	addl	$DV_WIDTH_SHORT, %esi
	addl	$16, %edi

	movq	          (%esi), %mm0
	movq	DV_WIDTH_SHORT_HALF(%esi), %mm1
	movq               8(%esi), %mm2
	movq	DV_WIDTH_SHORT_HALF+8(%esi), %mm3

	paddw	%mm0, %mm1
	paddw	%mm2, %mm3
	psraw	$1, %mm1
	psraw	$1, %mm3

	movq	%mm1, 0*8(%edi)
	movq	%mm3, 1*8(%edi)

	addl	$DV_WIDTH_SHORT, %esi
	addl	$16, %edi

	movq	          (%esi), %mm0
	movq	DV_WIDTH_SHORT_HALF(%esi), %mm1
	movq               8(%esi), %mm2
	movq	DV_WIDTH_SHORT_HALF+8(%esi), %mm3

	paddw	%mm0, %mm1
	paddw	%mm2, %mm3
	psraw	$1, %mm1
	psraw	$1, %mm3

	movq	%mm1, 0*8(%edi)
	movq	%mm3, 1*8(%edi)

	pop	%ebx
	pop	%edi
	pop	%esi
	pop	%ebp
	ret

.global pgm_copy_pal_c_block_mmx
pgm_copy_pal_c_block_mmx:
				
	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi
	pushl	%edi
	pushl	%ebx
	
	movl    8(%ebp), %edi          # dest
	movl    12(%ebp), %esi         # src


	movq	OFFSETBX, %mm7
	pxor	%mm6, %mm6

	
	movq	(%esi), %mm0
	movq	DV_WIDTH_BYTE(%esi), %mm1

	movq	%mm0, %mm2
	movq	%mm1, %mm3

	punpcklbw %mm6, %mm0
	punpcklbw %mm6, %mm1

	punpckhbw %mm6, %mm2
	punpckhbw %mm6, %mm3
	
	psubw	%mm7, %mm0
	psubw	%mm7, %mm1
	psubw	%mm7, %mm2
	psubw	%mm7, %mm3

#if PRECISION > 0	
	psllw	$PRECISION, %mm0
	psllw	$PRECISION, %mm1
	psllw	$PRECISION, %mm2
	psllw	$PRECISION, %mm3
#endif
		
	movq	%mm0, (%edi)
	movq	%mm2, 8(%edi)
	movq	%mm1, 16(%edi)
	movq	%mm3, 24(%edi)

	addl	$2*DV_WIDTH_BYTE, %esi
	addl	$32, %edi

	movq	(%esi), %mm0
	movq	DV_WIDTH_BYTE(%esi), %mm1

	movq	%mm0, %mm2
	movq	%mm1, %mm3

	punpcklbw %mm6, %mm0
	punpcklbw %mm6, %mm1

	punpckhbw %mm6, %mm2
	punpckhbw %mm6, %mm3
	
	psubw	%mm7, %mm0
	psubw	%mm7, %mm1
	psubw	%mm7, %mm2
	psubw	%mm7, %mm3

#if PRECISION > 0	
	psllw	$PRECISION, %mm0
	psllw	$PRECISION, %mm1
	psllw	$PRECISION, %mm2
	psllw	$PRECISION, %mm3
#endif
		
	movq	%mm0, (%edi)
	movq	%mm2, 8(%edi)
	movq	%mm1, 16(%edi)
	movq	%mm3, 24(%edi)

	addl	$2*DV_WIDTH_BYTE, %esi
	addl	$32, %edi

	movq	(%esi), %mm0
	movq	DV_WIDTH_BYTE(%esi), %mm1

	movq	%mm0, %mm2
	movq	%mm1, %mm3

	punpcklbw %mm6, %mm0
	punpcklbw %mm6, %mm1

	punpckhbw %mm6, %mm2
	punpckhbw %mm6, %mm3
	
	psubw	%mm7, %mm0
	psubw	%mm7, %mm1
	psubw	%mm7, %mm2
	psubw	%mm7, %mm3

#if PRECISION > 0	
	psllw	$PRECISION, %mm0
	psllw	$PRECISION, %mm1
	psllw	$PRECISION, %mm2
	psllw	$PRECISION, %mm3
#endif
		
	movq	%mm0, (%edi)
	movq	%mm2, 8(%edi)
	movq	%mm1, 16(%edi)
	movq	%mm3, 24(%edi)

	addl	$2*DV_WIDTH_BYTE, %esi
	addl	$32, %edi

	movq	(%esi), %mm0
	movq	DV_WIDTH_BYTE(%esi), %mm1

	movq	%mm0, %mm2
	movq	%mm1, %mm3

	punpcklbw %mm6, %mm0
	punpcklbw %mm6, %mm1

	punpckhbw %mm6, %mm2
	punpckhbw %mm6, %mm3
	
	psubw	%mm7, %mm0
	psubw	%mm7, %mm1
	psubw	%mm7, %mm2
	psubw	%mm7, %mm3

#if PRECISION > 0	
	psllw	$PRECISION, %mm0
	psllw	$PRECISION, %mm1
	psllw	$PRECISION, %mm2
	psllw	$PRECISION, %mm3
#endif
		
	movq	%mm0, (%edi)
	movq	%mm2, 8(%edi)
	movq	%mm1, 16(%edi)
	movq	%mm3, 24(%edi)

	pop	%ebx
	pop	%edi
	pop	%esi
	pop	%ebp
	ret

.global video_copy_pal_c_block_mmx
video_copy_pal_c_block_mmx:
				
	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi
	pushl	%edi
	pushl	%ebx
	
	movl    8(%ebp), %edi          # dest
	movl    12(%ebp), %esi         # src

	movq	OFFSETBX, %mm7
	paddw	%mm7, %mm7
	pxor	%mm6, %mm6

	movl	$4, %ebx
	
video_copy_pal_c_block_mmx_loop:	
	
	movq	(%esi), %mm0
	movq    DV_WIDTH_BYTE_HALF(%esi), %mm2

	movq	%mm0, %mm1
	movq	%mm2, %mm3

	punpcklbw %mm6, %mm0
	punpcklbw %mm6, %mm2

	punpckhbw %mm6, %mm1
	punpckhbw %mm6, %mm3
	
	paddw	%mm0, %mm2
	paddw	%mm1, %mm3

	psubw	%mm7, %mm2
	psubw	%mm7, %mm3
	
#if PRECISION == 0	
	psraw	$1, %mm2
	psraw	$1, %mm3
#else	
#if PRECISION > 1
	psllw	$PRECISION-1, %mm2
	psllw	$PRECISION-1, %mm3
#endif
#endif
	movq	%mm2, 0*8(%edi)
	movq	%mm3, 1*8(%edi)

	addl	$DV_WIDTH_BYTE, %esi
	addl	$16, %edi

	movq	(%esi), %mm0
	movq    DV_WIDTH_BYTE_HALF(%esi), %mm2

	movq	%mm0, %mm1
	movq	%mm2, %mm3

	punpcklbw %mm6, %mm0
	punpcklbw %mm6, %mm2

	punpckhbw %mm6, %mm1
	punpckhbw %mm6, %mm3
	
	paddw	%mm0, %mm2
	paddw	%mm1, %mm3

	psubw	%mm7, %mm2
	psubw	%mm7, %mm3
	
#if PRECISION == 0	
	psraw	$1, %mm2
	psraw	$1, %mm3
#else	
#if PRECISION > 1
	psllw	$PRECISION-1, %mm2
	psllw	$PRECISION-1, %mm3
#endif
#endif
	movq	%mm2, 0*8(%edi)
	movq	%mm3, 1*8(%edi)

	addl	$DV_WIDTH_BYTE, %esi
	addl	$16, %edi

	decl	%ebx
	jnz	video_copy_pal_c_block_mmx_loop

	pop	%ebx
	pop	%edi
	pop	%esi
	pop	%ebp
	ret
	
.global ppm_copy_ntsc_c_block_mmx
ppm_copy_ntsc_c_block_mmx:
				
	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi
	pushl	%edi
	pushl	%ebx
	
	movl    8(%ebp), %edi          # dest
	movl    12(%ebp), %esi         # src

	movl	$4, %ebx	

ppm_copy_ntsc_c_block_mmx_loop:	
	
	movq	(%esi), %mm0
	movq    8(%esi), %mm1
	movq	16(%esi), %mm2
	movq	24(%esi), %mm3

	paddw	%mm0, %mm1
	paddw	%mm2, %mm3
	psraw	$1, %mm1
	psraw	$1, %mm3

	movq	%mm1, 0*8(%edi)
	movq	%mm3, 1*8(%edi)

	addl	$DV_WIDTH_SHORT_HALF, %esi
	addl	$16, %edi

	movq	(%esi), %mm0
	movq    8(%esi), %mm1
	movq	16(%esi), %mm2
	movq	24(%esi), %mm3

	paddw	%mm0, %mm1
	paddw	%mm2, %mm3
	psraw	$1, %mm1
	psraw	$1, %mm3

	movq	%mm1, 0*8(%edi)
	movq	%mm3, 1*8(%edi)

	addl	$DV_WIDTH_SHORT_HALF, %esi
	addl	$16, %edi

	decl	%ebx
	jnz	ppm_copy_ntsc_c_block_mmx_loop
				
	pop	%ebx
	pop	%edi
	pop	%esi
	pop	%ebp
	ret

.global pgm_copy_ntsc_c_block_mmx
pgm_copy_ntsc_c_block_mmx:
				
	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi
	pushl	%edi
	
	movl    8(%ebp), %edi          # dest
	movl    12(%ebp), %esi         # src

	movq	OFFSETBX, %mm7
	paddw	%mm7, %mm7
	pxor	%mm6, %mm6

	movq	(%esi), %mm0
	movq    8(%esi), %mm2

	movq	%mm0, %mm1
	movq	%mm2, %mm3

	punpcklbw %mm6, %mm0
	punpcklbw %mm6, %mm2

	punpckhbw %mm6, %mm1
	punpckhbw %mm6, %mm3
	
	paddw	%mm0, %mm1
	paddw	%mm2, %mm3

	psubw	%mm7, %mm1
	psubw	%mm7, %mm3
	
#if PRECISION == 0	
	psraw	$1, %mm1
	psraw	$1, %mm3
#else	
#if PRECISION > 1
	psllw	$PRECISION-1, %mm1
	psllw	$PRECISION-1, %mm3
#endif
#endif
	movq	%mm1, 0*8(%edi)
	movq	%mm3, 1*8(%edi)
	movq	%mm1, 2*8(%edi)
	movq	%mm3, 3*8(%edi)

	addl	$DV_WIDTH_BYTE, %esi
	addl	$32, %edi

	movq	(%esi), %mm0
	movq    8(%esi), %mm2

	movq	%mm0, %mm1
	movq	%mm2, %mm3

	punpcklbw %mm6, %mm0
	punpcklbw %mm6, %mm2

	punpckhbw %mm6, %mm1
	punpckhbw %mm6, %mm3
	
	paddw	%mm0, %mm1
	paddw	%mm2, %mm3

	psubw	%mm7, %mm1
	psubw	%mm7, %mm3
	
#if PRECISION == 0	
	psraw	$1, %mm1
	psraw	$1, %mm3
#else	
#if PRECISION > 1
	psllw	$PRECISION-1, %mm1
	psllw	$PRECISION-1, %mm3
#endif
#endif
	movq	%mm1, 0*8(%edi)
	movq	%mm3, 1*8(%edi)
	movq	%mm1, 2*8(%edi)
	movq	%mm3, 3*8(%edi)

	addl	$DV_WIDTH_BYTE, %esi
	addl	$32, %edi

	movq	(%esi), %mm0
	movq    8(%esi), %mm2

	movq	%mm0, %mm1
	movq	%mm2, %mm3

	punpcklbw %mm6, %mm0
	punpcklbw %mm6, %mm2

	punpckhbw %mm6, %mm1
	punpckhbw %mm6, %mm3
	
	paddw	%mm0, %mm1
	paddw	%mm2, %mm3

	psubw	%mm7, %mm1
	psubw	%mm7, %mm3
	
#if PRECISION == 0	
	psraw	$1, %mm1
	psraw	$1, %mm3
#else	
#if PRECISION > 1
	psllw	$PRECISION-1, %mm1
	psllw	$PRECISION-1, %mm3
#endif
#endif
	movq	%mm1, 0*8(%edi)
	movq	%mm3, 1*8(%edi)
	movq	%mm1, 2*8(%edi)
	movq	%mm3, 3*8(%edi)

	addl	$DV_WIDTH_BYTE, %esi
	addl	$32, %edi

	movq	(%esi), %mm0
	movq    8(%esi), %mm2

	movq	%mm0, %mm1
	movq	%mm2, %mm3

	punpcklbw %mm6, %mm0
	punpcklbw %mm6, %mm2

	punpckhbw %mm6, %mm1
	punpckhbw %mm6, %mm3
	
	paddw	%mm0, %mm1
	paddw	%mm2, %mm3

	psubw	%mm7, %mm1
	psubw	%mm7, %mm3
	
#if PRECISION == 0	
	psraw	$1, %mm1
	psraw	$1, %mm3
#else	
#if PRECISION > 1
	psllw	$PRECISION-1, %mm1
	psllw	$PRECISION-1, %mm3
#endif
#endif
	movq	%mm1, 0*8(%edi)
	movq	%mm3, 1*8(%edi)
	movq	%mm1, 2*8(%edi)
	movq	%mm3, 3*8(%edi)

	pop	%edi
	pop	%esi
	pop	%ebp
	ret

.global video_copy_ntsc_c_block_mmx
video_copy_ntsc_c_block_mmx:
				
	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi
	pushl	%edi
	pushl	%ebx
	
	movl    8(%ebp), %edi          # dest
	movl    12(%ebp), %esi         # src

	movq	OFFSETBX, %mm7
	paddw	%mm7, %mm7
	pxor	%mm6, %mm6

	movl	$4, %ebx
	
video_copy_ntsc_c_block_mmx_loop:	
	
	movq	(%esi), %mm0
	movq    8(%esi), %mm2

	movq	%mm0, %mm1
	movq	%mm2, %mm3

	punpcklbw %mm6, %mm0
	punpcklbw %mm6, %mm2

	punpckhbw %mm6, %mm1
	punpckhbw %mm6, %mm3
	
	paddw	%mm0, %mm1
	paddw	%mm2, %mm3

	psubw	%mm7, %mm1
	psubw	%mm7, %mm3
	
#if PRECISION == 0	
	psraw	$1, %mm1
	psraw	$1, %mm3
#else	
#if PRECISION > 1
	psllw	$PRECISION-1, %mm1
	psllw	$PRECISION-1, %mm3
#endif
#endif
	movq	%mm1, 0*8(%edi)
	movq	%mm3, 1*8(%edi)

	addl	$DV_WIDTH_BYTE_HALF, %esi
	addl	$16, %edi

	movq	(%esi), %mm0
	movq    8(%esi), %mm2

	movq	%mm0, %mm1
	movq	%mm2, %mm3

	punpcklbw %mm6, %mm0
	punpcklbw %mm6, %mm2

	punpckhbw %mm6, %mm1
	punpckhbw %mm6, %mm3
	
	paddw	%mm0, %mm1
	paddw	%mm2, %mm3

	psubw	%mm7, %mm1
	psubw	%mm7, %mm3
	
#if PRECISION == 0	
	psraw	$1, %mm1
	psraw	$1, %mm3
#else	
#if PRECISION > 1
	psllw	$PRECISION-1, %mm1
	psllw	$PRECISION-1, %mm3
#endif
#endif
	movq	%mm1, 0*8(%edi)
	movq	%mm3, 1*8(%edi)

	addl	$DV_WIDTH_BYTE_HALF, %esi
	addl	$16, %edi

	decl	%ebx
	jnz	video_copy_ntsc_c_block_mmx_loop


	pop	%ebx
	pop	%edi
	pop	%esi
	pop	%ebp
	ret









