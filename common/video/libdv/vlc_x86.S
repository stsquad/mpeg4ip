	#include "asmoff.h"
.text
	.align 4
.globl dv_decode_vlc 
	.type	 dv_decode_vlc,@function
dv_decode_vlc:
	pushl %ebx

	/* Args are at 8(%esp). */
	movl  8(%esp),%eax		/* %eax is bits */
	movl  12(%esp),%ebx		/* %ebx is maxbits */

	movl  dv_vlc_class_index_mask(,%ebx,4),%edx
	andl  %eax,%edx
	movl  dv_vlc_class_index_rshift(,%ebx,4),%ecx
	sarl  %cl,%edx
	movl  dv_vlc_classes(,%ebx,4),%ecx
	movsbl  (%ecx,%edx,1),%edx	/* %edx is class */
			
	movl  dv_vlc_index_mask(,%edx,4),%ebx
	movl  dv_vlc_index_rshift(,%edx,4),%ecx
	andl  %eax,%ebx
	sarl  %cl,%ebx

	movl  dv_vlc_lookups(,%edx,4),%edx
	movl  (%edx,%ebx,4),%edx

	/* Now %edx holds result, like this:
	   bits 0-7   run
	   bits 8-15  len
	   bits 16-31 amp
	*/
	/* code needs to do this with result:
	   if ((amp > 0) &&
	     if (bits & sign_mask[result->len])
	         amp = -amp;
	   }
	*/

	/* Form a mask from (bits & sign_mask[result->len]) */
	movl  %edx,%ecx
	sarl  $8,%ecx
	andl  $0xff,%ecx
	movl  sign_mask(,%ecx,4),%ebx
	andl  %ebx,%eax
	negl  %eax
	sarl  $31,%eax

	movl  %edx,%ebx
	sarl  $31,%ebx
	xorl  $0xffffffff,%ebx
	andl  $0xffff0000,%ebx

	andl  %ebx,%eax

	/* Now %eax is 0xffff0000 if we want to negate %edx, zero otherwise */
	xorl  %eax,%edx
	subl  %eax,%edx

	/*
	if (maxbits < result->len)
	    *result = broken;
	Note that the 'broken' pattern is all ones (i.e. 0xffffffff)
	*/
	movl  12(%esp),%ebx		/* %ebx is maxbits */
	subl  %ecx,%ebx
	sbbl  %ebx,%ebx
	orl   %ebx,%edx

	movl  16(%esp),%eax
	movl  %edx,(%eax)
	
	popl  %ebx
	ret
	
.text
	.align 4
.globl __dv_decode_vlc 
	.type	 __dv_decode_vlc,@function
__dv_decode_vlc:
	pushl %ebx

	/* Args are at 8(%esp). */
	movl  8(%esp),%eax		/* %eax is bits */
	
	movl  %eax,%edx			/* %edx is class */
	andl  $0xfe00,%edx
	sarl  $9,%edx
	movsbl dv_vlc_class_lookup5(%edx),%edx
	
	movl  dv_vlc_index_mask(,%edx,4),%ebx
	movl  dv_vlc_index_rshift(,%edx,4),%ecx
	andl  %eax,%ebx
	sarl  %cl,%ebx

	movl  dv_vlc_lookups(,%edx,4),%edx
	movl  (%edx,%ebx,4),%edx

	/* Now %edx holds result, like this:
	   bits 0-7   run
	   bits 8-15  len
	   bits 16-31 amp
	*/
	/* code needs to do this with result:
	   if ((amp > 0) &&
	     if ((bits >> sign_rshift[result->len]) & 1)
	         amp = -amp;
	   }
	*/
	/* if (amp < 0) %edx is 0, else 0xffff0000. */
	movl  %edx,%ecx
	sarl  $8,%ecx
	andl  $0xff,%ecx
	movl  sign_mask(,%ecx,4),%ecx
	andl  %ecx,%eax
	negl  %eax
	sarl  $31,%eax

	movl  %edx,%ebx
	sarl  $31,%ebx
	xorl  $0xffffffff,%ebx
	andl  $0xffff0000,%ebx

	andl  %ebx,%eax
	
	xorl  %eax,%edx
	subl  %eax,%edx

	movl  12(%esp),%eax
	movl  %edx,(%eax)
	
	popl  %ebx
	ret

/*	
void dv_parse_ac_coeffs_pass0(bitstream_t *bs,
			      dv_macroblock_t *mb,
			      dv_block_t *bl)
*/
.text
	.align	4
.globl	dv_parse_ac_coeffs_pass0
dv_parse_ac_coeffs_pass0:
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	pushl	%ebp

#define ARGn(N)  (20+(4*(N)))(%esp)

	/*
	eax	scratch
	ebx	bl->reorder
	ecx
	edx
	esi	bs->buf
	edi	bl->offset
	ebp	bl
	*/
	movl    ARGn(2),%ebp
	movl	ARGn(0),%esi
	movl	bitstream_t_buf(%esi),%esi
	movl	dv_block_t_offset(%ebp),%edi
	movl	dv_block_t_reorder(%ebp),%ebx

	/* I think it would be better to zero out the coeffs as we're
	copying them into the framebuffer.  But that optimization is
	for another day. */
	
	movq    dv_block_t_coeffs(%ebp),%mm1
	pxor    %mm0,%mm0
	pand    const_f_0_0_0,%mm1
	movq    %mm1,dv_block_t_coeffs(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 8)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 16)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 24)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 32)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 40)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 48)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 56)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 64)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 72)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 80)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 88)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 96)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 104)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 112)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 120)(%ebp)
	
readloop:
	movl	%edi,%ecx
	shrl	$3,%ecx
	movzbl  (%esi,%ecx,1),%eax
	movzbl  1(%esi,%ecx,1),%edx
	movzbl  2(%esi,%ecx,1),%ecx
	shll	$16,%eax
	shll	$8,%edx
	orl	%ecx,%eax
	orl	%edx,%eax
	movl	%edi,%edx
	andl	$7,%edx
	movl	$8,%ecx
	subl	%edx,%ecx
	shrl    %cl,%eax
	
	movl	dv_block_t_end(%ebp),%edx
	subl	%edi,%edx	/* edx is bits_left */
	cmpl	$16,%edx
	
	jl	slowpath

	/* ecx is most significant 7 bits */
	movl	%eax,%ecx
	andl    $0xfe00,%ecx
	sarl    $9,%ecx

	/* Attempt to use the shortcut first.  If it hits, then
	   this vlc term has been decoded. */
	movl	dv_vlc_class1_shortcut(,%ecx,4),%edx
	test	$0x80,%edx
	je	done_decode

	/* */
	
	/* fast path:	 use inlined version of __dv_decode_vlc */
	/* ---------------------- */
	movl	%ebx,dv_block_t_reorder(%ebp)

	/* %eax is bits */
	
	movsbl dv_vlc_class_lookup5(%ecx),%ecx

	movl  dv_vlc_index_mask(,%ecx,4),%ebx
	movl  dv_vlc_lookups(,%ecx,4),%edx
	movl  dv_vlc_index_rshift(,%ecx,4),%ecx
	andl  %eax,%ebx
	sarl  %cl,%ebx

	movl  (%edx,%ebx,4),%edx

	/* Now %edx holds result, like this:
	   bits 0-7   run
	   bits 8-15  len
	   bits 16-31 amp
	*/
	test	$0x80,%edx	/* If (vlc.run < 0) break */
	jne	escape1
	/* code needs to do this with result:
	   if ((amp > 0) &&
	     if ((bits >> sign_rshift[result->len]) & 1)
	         amp = -amp;
	   }
	*/
	/* if (amp < 0) %edx is 0, else 0xffff0000. */
	movl  %edx,%ecx
	sarl  $8,%ecx
	andl  $0xff,%ecx
	movl  sign_mask(,%ecx,4),%ecx
	andl  %ecx,%eax
	negl  %eax
	sarl  $31,%eax

	movl  %edx,%ebx
	sarl  $31,%ebx
	xorl  $0xffffffff,%ebx
	andl  $0xffff0000,%ebx
	andl  %ebx,%eax

	xorl  %eax,%edx
	subl  %eax,%edx

	movl	dv_block_t_reorder(%ebp),%ebx
	/* ---------------------- */
	
done_decode:
	/* bl->offset += vlc.len */
	movl	%edx,%eax
	shrl	$8,%eax
	andl	$255,%eax
	addl	%eax,%edi

	/* bl->reorder += vlc.run */
	movl	%edx,%eax
	andl	$255,%eax
	addl	%eax,%ebx
	
	/* SET_COEFF(bl->coeffs, bl->reorder, vlc.amp); */
	movzbl	(%ebx),%eax
	incl	%ebx

	shrl	$16,%edx
	movw	%dx,(dv_block_t_coeffs)(%ebp,%eax,1)
	
	jmp	readloop

escape1:
	movl	dv_block_t_reorder(%ebp),%ebx
escape:
	/* if (vlc.amp == 0) */
	test	$0xffff0000,%edx
	jne	ampnonzero
	/* bl->reorder = bl->reorder_sentinel; */
	movl	dv_block_t_reorder_sentinel(%ebp),%ebx
	addl	$4,%edi
	movl	$1,dv_block_t_eob(%ebp)
	movl    ARGn(1),%edx
	incl	dv_macroblock_t_eob_count(%edx)
	jmp	alldone
ampnonzero:
	andl	$0x0000ff00,%edx
	cmpl	$0x0000fe00,%edx	/* VLC_ERROR */
	jne	alldone
	movl    ARGn(1),%edx
	movl	$1,dv_macroblock_t_vlc_error(%edx); 
alldone:
	movl	%ebx,dv_block_t_reorder(%ebp)
	movl	%edi,dv_block_t_offset(%ebp)
	
	popl	%ebp
	popl	%esi
	popl	%edi
	popl	%ebx

	ret

slowpath:
	/* slow path:	 use dv_decode_vlc */;
	pushl	$vlc		/* last parameter is &vlc */
	pushl	%edx		/* bits_left */
	pushl	%eax		/* bits */
	call	dv_decode_vlc
	addl	$12,%esp
	test	$0x80,%edx	/* If (vlc.run < 0) break */
	jne	escape
	
	jmp	done_decode
	
show16:
	movl	%edi,%ecx
	movl	%edi,%edx
	shrl	$3,%ecx
	andl	$7,%edx
	movzbl  (%esi,%ecx,1),%eax
	movzbl  1(%esi,%ecx,1),%ebx
	movzbl  2(%esi,%ecx,1),%ecx
	shll	$16,%eax
	shll	$8,%ebx
	orl	%ecx,%eax
	orl	%ebx,%eax
	movl	$8,%ecx
	subl	%edx,%ecx
	shrl    %cl,%eax
	ret
#undef ARGn


	/*
	gint dv_parse_video_segment(dv_videosegment_t *seg, guint quality) {
	*/
	.globl dv_parse_video_segment
dv_parse_video_segment:
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	pushl	%ebp

#define ARGn(N)  (20+(4*(N)))(%esp)

	movl	ARGn(1),%eax			/* quality */
	movl	$4,%ebx
	testl	$DV_QUALITY_COLOR,%eax
	jz	its_mono
	movl	$6,%ebx
its_mono:
	movl	%ebx,n_blocks
	
	/*
	 *	ebx	seg/b
	 *
	 *	
	 *      esi	bs->buf
	 *	edi	mb
	 *	ebp	bl
	 */
	movl	ARGn(0),%ebx
	movl	dv_videosegment_t_bs(%ebx),%esi
	movl	bitstream_t_buf(%esi),%esi
	leal	dv_videosegment_t_mb(%ebx),%edi

	movl	$0,%eax
	movl	$0,%ecx
macloop:
	movl	%eax,m
	movl	%ecx,mb_start

	movl	ARGn(0),%ebx
	
	/* bitstream_seek_set(bs,mb_start+28); */
	/* mb->qno = bitstream_get(bs,4); */
	movl	%ecx,%edx
	shr	$3,%edx
	movzbl	3(%esi,%edx,1),%edx
	andl	$0xf,%edx
	movl	%edx,dv_macroblock_t_qno(%edi)

	/* mb->vlc_error = 0;
           mb->eob_count = 0; */
	xorl	%edx,%edx
	movl	%edx,dv_macroblock_t_vlc_error(%edi)
	movl	%edx,dv_macroblock_t_eob_count(%edi)

	/* mb->i = (seg->i + dv_super_map_vertical[m]) % (seg->isPAL?12:10); */
	movl	dv_super_map_vertical(,%eax,4),%edx
	movl	dv_videosegment_t_i(%ebx),%ecx
	addl	%ecx,%edx

skarly:	
	movl	dv_videosegment_t_isPAL(%ebx),%ecx
	addl	$-1,%ecx
	sbbl	%ecx,%ecx
	andl	$1,%ecx
	shll	$5,%ecx		/* ecx = (isPAL ? 32 : 0) */

	movzbl	mod_10(%edx,%ecx,1),%edx	/* uses mod_12 for PAL */
	movl	%edx,dv_macroblock_t_i(%edi)

	/*  mb->j = dv_super_map_horizontal[m]; */	
	movl	dv_super_map_horizontal(,%eax,4),%edx
	movl	%edx,dv_macroblock_t_j(%edi)

	/* mb->k = seg->k; */
	movl	dv_videosegment_t_k(%ebx),%edx
	movl	%edx,dv_macroblock_t_k(%edi)

	movl	$0,%ebx
	lea	dv_macroblock_t_b(%edi),%ebp
	
blkloop:
	/*
		+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
	        |15 |   |   |   |   |   |   |   | 7 | 6 | 5 | 4 |   |   |   | 0 |
	        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
	        |                 dc                |mde| class |               |
	        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
	*/
	/* dc = bitstream_get(bs,9); */
	movl	mb_start,%ecx
	shr	$3,%ecx
	movzbl	blk_start(%ebx),%edx
	addl	%ecx,%edx
	movzbl	(%esi,%edx,1),%eax	/* hi byte */
	movzbl	1(%esi,%edx,1),%ecx	/* lo byte */
	shll	$8,%eax
	orl	%ecx,%eax

        movl    %eax,%edx
        /* if(dc > 255) dc -= 512;
           just do an arithmetric shift right 7bits*/
        sar     $7,%dx                  /* dc in %dx */
        movw    %dx,dv_block_t_coeffs(%ebp)

	/* bl->class_no = bitstream_get(bs,2); */
	movl	%eax,%ecx
	shrl	$4,%ecx
	andl	$3,%ecx
	movl	%ecx,dv_block_t_class_no(%ebp)

	/* bl->eob=0 */
	xorl	%ecx,%ecx
	movl	%ecx,dv_block_t_eob(%ebp)

	/* bl->dct_mode = bitstream_get(bs,1); */
	shrl	$6,%eax
	andl	$1,%eax
	movl	%eax,dv_block_t_dct_mode(%ebp)

	/* bl->reorder = &dv_reorder[bl->dct_mode][1]; */
	shll	$6,%eax
	addl	$(dv_reorder+1),%eax
	movl	%eax,dv_block_t_reorder(%ebp)

	/* bl->reorder_sentinel = bl->reorder + 63; */
	addl	$63,%eax
	movl	%eax,dv_block_t_reorder_sentinel(%ebp)

	/* bl->offset= mb_start + dv_parse_bit_start[b]; */
	movl	mb_start,%ecx
	movl	dv_parse_bit_start(,%ebx,4),%eax
	addl	%ecx,%eax
	movl	%eax,dv_block_t_offset(%ebp)

	/* bl->end= mb_start + dv_parse_bit_end[b]; */
	movl	dv_parse_bit_end(,%ebx,4),%eax
	addl	%ecx,%eax
	movl	%eax,dv_block_t_end(%ebp)

	/* dv_parse_ac_coeffs_pass0(bs,mb,bl); */
	movl	ARGn(1),%ecx	/* quality */
	testl	$DV_QUALITY_AC_MASK,%ecx
	jnz	do_ac_pass
	
	/* no AC pass.  Just zero out the remaining coeffs */
	movq    dv_block_t_coeffs(%ebp),%mm1
	pxor    %mm0,%mm0
	pand    const_f_0_0_0,%mm1
	movq    %mm1,dv_block_t_coeffs(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 8)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 16)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 24)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 32)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 40)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 48)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 56)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 64)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 72)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 80)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 88)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 96)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 104)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 112)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 120)(%ebp)
	jmp	done_ac
	
do_ac_pass:
	movl    ARGn(0),%eax
	movl	dv_videosegment_t_bs(%eax),%eax
	pushl	%ebp
	pushl	%edi
	pushl	%eax
	call	dv_parse_ac_coeffs_pass0
	addl	$12,%esp
done_ac:

	movl	n_blocks,%eax
	addl	$dv_block_t_size,%ebp
	incl	%ebx
	cmpl	%eax,%ebx
	jnz	blkloop

	movl	m,%eax
	movl	mb_start,%ecx
	addl	$(8 * 80),%ecx
	addl	$dv_macroblock_t_size,%edi
	incl	%eax
	cmpl	$5,%eax
	jnz	macloop
	
	movl	ARGn(1),%eax	/* quality */
	
	popl	%ebp
	popl	%esi
	popl	%edi
	popl	%ebx

	emms

	andl	$DV_QUALITY_AC_MASK,%eax
	cmpl	$DV_QUALITY_AC_2,%eax
	jz	dv_parse_ac_coeffs
	movl	$0,%eax
	ret

.data
vlc:
	.long	0
m:
	.long	0
mb_start:
	.long	0
n_blocks:
	.long	0	/* 4 for monochrome, 6 for color */
blk_start:
	.byte	4,18,32,46,60,70
	
	/* mod tables, 32 bytes apart */
mod_10:
	.byte	0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7
	.byte	0,0,0,0,0,0,0,0,0,0,0,0,0,0	/* spacer, see above */
mod_12:
	.byte	0,1,2,3,4,5,6,7,8,9,10,11,0,1,2,3,4,5,6,7,8
	
	.align 16
const_f_0_0_0:
	.short	0xffff,0,0,0
