/*
 *  parse.c
 *
 *     Copyright (C) Charles 'Buck' Krasic - April 2000
 *     Copyright (C) Erik Walthinsen - April 2000
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

#include <dv_types.h>

#include <stdio.h>
#include <string.h>

#include "util.h"
#include "dv.h"
#include "bitstream.h"
#include "vlc.h"
#include "parse.h"
#include "audio.h"

#define STRICT_SYNTAX 0
#define VLC_BOUNDS_CHECK 0

#define PARSE_VLC_TRACE 0

#ifdef __GNUC__
#if PARSE_VLC_TRACE
#define vlc_trace(format,args...) fprintf(stdout,format,##args)
#else 
#define vlc_trace(format,args...) 
#endif  /* PARSE_VLC_TRACE */
#else
static inline void vlc_trace(char *format, ...)
{
#if PARSE_VLC_TRACE
  va_list argp;
  va_start(argp, format);
  vfprintf(stdout, format, argp);
  va_end(argp);
#endif /* PARSE_VLC_TRACE */
}
#endif /* __GNUC__ */

/* Assign coefficient in zigzag order without indexing multiply */
#define ZERO_MULT_ZIGZAG 1
#if ZERO_MULT_ZIGZAG
#define SET_COEFF(COEFFS,REORDER,VALUE) (*((dv_coeff_t *)(((guint8 *)(COEFFS)) + *(REORDER)++)) = (VALUE))
#else
#define SET_COEFF(COEFFS,REORDER,VALUE) COEFFS[*REORDER++] = VALUE
#endif

gint    dv_parse_bit_start[6] = { 4*8+12,  18*8+12, 32*8+12, 46*8+12, 60*8+12, 70*8+12 };
gint    dv_parse_bit_end[6]   = { 18*8,    32*8,    46*8,    60*8,    70*8,    80*8 };

gint     dv_super_map_vertical[5] = { 2, 6, 8, 0, 4 };
gint     dv_super_map_horizontal[5] = { 2, 1, 3, 0, 4 };

static gint8  dv_88_reorder_prime[64] = {
0, 1, 8, 16, 9, 2, 3, 10,		17, 24, 32, 25, 18, 11, 4, 5,
12, 19, 26, 33, 40, 48, 41, 34,		27, 20, 13, 6, 7, 14, 21, 28,
35, 42, 49, 56, 57, 50, 43, 36,		29, 22, 15, 23, 30, 37, 44, 51,
58, 59, 52, 45, 38, 31, 39, 46,		53, 60, 61, 54, 47, 55, 62, 63 
};

gint8  dv_reorder[2][64] = {
  { 0 },
  {
    0, 32, 1, 33, 8, 40, 2, 34,		9, 41, 16, 48, 24, 56, 17, 49,
    10, 42, 3, 35, 4, 36, 11, 43,		18, 50, 25, 57, 26, 58, 19, 51,
    12, 44, 5, 37, 6, 38, 13, 45,		20, 52, 27, 59, 28, 60, 21, 53,
    14, 46, 7, 39, 15, 47, 22, 54,		29, 61, 30, 62, 23, 55, 31, 63 }
};

#if HAVE_LIBPOPT
static void
dv_video_popt_callback(poptContext con, enum poptCallbackReason reason, 
		       const struct poptOption * opt, const char * arg, const void * data)
{
  dv_video_t *video = (dv_video_t *)data;
  
  switch(video->arg_block_quality) {
  case 1:
    video->quality |= DV_QUALITY_DC;
    break;
  case 2:
    video->quality |= DV_QUALITY_AC_1;
    break;
  case 3:
    video->quality |= DV_QUALITY_AC_2;
    break;
  default:
    dv_opt_usage(con, video->option_table, DV_VIDEO_OPT_BLOCK_QUALITY);
    break;
  } /* switch  */
  if(!video->arg_monochrome) {
    video->quality |= DV_QUALITY_COLOR;
  } /* if */
  
} /* dv_video_popt_callback  */
#endif /* HAVE_LIBPOPT */

dv_video_t *
dv_video_new(void)
{
  dv_video_t *result;
  
  result = (dv_video_t *)calloc(1,sizeof(dv_video_t));
  if(!result) goto noopt;

  result->arg_block_quality = 3; /* Default is best quality  */

#if HAVE_LIBPOPT
  result->option_table[DV_VIDEO_OPT_BLOCK_QUALITY] = (struct poptOption){
    longName:   "quality", 
    shortName:  'q', 
    argInfo:    POPT_ARG_INT, 
    arg:        &result->arg_block_quality,
    argDescrip: "(1|2|3)",
    descrip:    "video quality level (coeff. parsing):"
    "  1=DC and no ACs,"
    " 2=DC and single-pass for ACs ,"
    " 3=DC and multi-pass for ACs [default]",
  }; /* block quality */

  result->option_table[DV_VIDEO_OPT_MONOCHROME] = (struct poptOption){
    longName:  "monochrome", 
    shortName: 'm', 
    arg:       &result->arg_monochrome,
    descrip:   "skip decoding of color blocks",
  }; /* monochrome */

  result->option_table[DV_VIDEO_OPT_CALLBACK] = (struct poptOption){
    argInfo: POPT_ARG_CALLBACK|POPT_CBFLAG_POST,
    arg:     dv_video_popt_callback,
    descrip: (char *)result, /* data passed to callback */
  }; /* callback */
#else
  result->quality = DV_QUALITY_BEST;
#endif /* HAVE_LIBPOPT */

 noopt:
  return(result);
} /* dv_video_new */

void dv_parse_init(void) {
  gint i;
  for(i=0;i<64;i++) {
#if !ARCH_X86
    dv_reorder[DV_DCT_88][i] = ((dv_88_reorder_prime[i] / 8) * 8) + (dv_88_reorder_prime[i] % 8);
#else
    dv_reorder[DV_DCT_88][i] = ((dv_88_reorder_prime[i] % 8) * 8) + (dv_88_reorder_prime[i] / 8);
#endif
  } /* for  */
  for(i=0;i<64;i++) {
#if ZERO_MULT_ZIGZAG
    dv_reorder[DV_DCT_88][i] = (dv_reorder[DV_DCT_88][i]) * sizeof(dv_coeff_t);
    dv_reorder[DV_DCT_248][i] = (dv_reorder[DV_DCT_248][i]) * sizeof(dv_coeff_t);
#endif
  } /* for */
} /* dv_parse_init */

/* Scan the blocks of a macroblock.  We're looking to find the next */
/* block from which unused space was borrowed */
static inline
gboolean dv_find_mb_unused_bits(dv_macroblock_t *mb, dv_block_t **lender) {
  gint b;

  for(b=0; b<6; b++) {
    if((mb->b[b].eob) &&    /* an incomplete block can only "borrow" bits
			* from other blocks that are themselves
                        * already completely decoded */
       (mb->b[b].end > mb->b[b].offset) && /* the lender must have unused bits */
       (!mb->b[b].mark)) {  /* the lender musn't already be lending... */
      mb->b[b].mark = TRUE;
      *lender = &mb->b[b];
      return(TRUE);
    } /* if  */
  } /* for b */
  return(FALSE);
} /* dv_find_mb_unused_bits */

/* After parsing vlcs from borrowed space, we must clear the trail of 
 * marks we used to track lenders.  found_vlc indicates whether the 
 * scanning process successfully found a complete vlc.  If it did,
 * then we update all blocks that lent bits as having no bits left. 
 * If so, the last block gets fixed in the caller.   */
static void dv_clear_mb_marks(dv_macroblock_t *mb, gboolean found_vlc) { 
  dv_block_t *bl; gint b;

  for(b=0,bl=mb->b;
      b<6;
      b++,bl++) {
    if(bl->mark) {
      bl->mark = FALSE;
      if(found_vlc) bl->offset = bl->end;
    }
  }
} /* dv__clear_mb_marks */

/* For pass 3, we scan all blocks of a video segment for unused bits  */
static gboolean dv_find_vs_unused_bits(dv_videosegment_t *seg, dv_block_t **lender) {
  dv_macroblock_t *mb;
  gint m;
  
  for(m=0,mb=seg->mb;
      m<5;
      m++,mb++) {
    if((mb->eob_count == 6) && /* We only borrow bits from macroblocks that are themselves complete */
       dv_find_mb_unused_bits(mb,lender)) 
      return(TRUE);
  } /* for */
  return(FALSE);
} /* dv_find_vs_unused_bits */

/* For pass 3, the trail of lenders can span the whole video segment */
static void dv_clear_vs_marks(dv_videosegment_t *seg,gboolean found_vlc) {
  dv_macroblock_t *mb;
  gint m;
  
  for(m=0,mb=seg->mb;
      m<5;
      m++,mb++) 
    dv_clear_mb_marks(mb,found_vlc);
} /* dv_clear_vs_marks */

/* For passes 2 and 3, vlc data that didn't fit in the area of a block
 * are put in space borrowed from other blocks.  Pass 2 borrows from
 * blocks of the same macroblock.  Pass 3 uses space from blocks of
 * other macroblocks of the videosegment. */
static gint dv_find_spilled_vlc(dv_videosegment_t *seg, dv_macroblock_t *mb, dv_block_t **bl_lender, gint pass) {
  dv_vlc_t vlc;
  dv_block_t *bl_new_lender;
  gboolean found_vlc, found_bits;
  gint bits_left, found_bits_left;
  gint save_offset = 0;
  gint broken_vlc = 0;
  bitstream_t *bs;

  bs = seg->bs;
  if((bits_left = (*bl_lender)->end - (*bl_lender)->offset))
    broken_vlc=bitstream_get(bs,bits_left);
  found_vlc = FALSE;
  found_bits = dv_find_mb_unused_bits(mb,&bl_new_lender);
  if(!found_bits && pass != 1)
    found_bits = dv_find_vs_unused_bits(seg,&bl_new_lender);
  while(found_bits && (!found_vlc)) {
    bitstream_seek_set(bs, bl_new_lender->offset);
    found_bits_left = bl_new_lender->end - bl_new_lender->offset;
    if(bits_left) /* prepend broken vlc if there is one */
      bitstream_unget(bs, broken_vlc, bits_left);
    dv_peek_vlc(bs, found_bits_left + bits_left, &vlc);
    if(vlc.len >= 0) {
      /* found a vlc, set things up to return to the main coeff loop */
      save_offset = bl_new_lender->offset - bits_left;
      found_vlc = TRUE;
    } else if(vlc.len == VLC_NOBITS) {
      /* still no vlc */
      bits_left += found_bits_left;
      broken_vlc = bitstream_get(bs, bits_left);
      if(pass == 1) 
	found_bits = dv_find_mb_unused_bits(mb,&bl_new_lender);
      else if(!(found_bits = dv_find_mb_unused_bits(mb,&bl_new_lender)))
	found_bits = dv_find_vs_unused_bits(seg,&bl_new_lender);
    } else {
      if(pass == 1) dv_clear_mb_marks(mb,found_vlc); 
      else dv_clear_vs_marks(seg,found_vlc);
      return(-1);
    } /* else */
  } /* while */
  if(pass == 1) dv_clear_mb_marks(mb,found_vlc); 
  else dv_clear_vs_marks(seg,found_vlc);
  if(found_vlc) {
    bl_new_lender->offset = save_offset; /* fixup offset clobbered by clear marks  */
    *bl_lender = bl_new_lender;
  }
  return(found_vlc);
} /* dv_find_spilled_vlc */


gint dv_parse_ac_coeffs(dv_videosegment_t *seg) {
  dv_vlc_t         vlc;
  gint             m, b, pass;
  gint             bits_left;
  gboolean         vlc_error;
  gint8           **reorder, *reorder_sentinel;
  dv_coeff_t      *coeffs;
  dv_macroblock_t *mb;
  dv_block_t      *bl, *bl_bit_source;
  bitstream_t     *bs;

  bs = seg->bs;
  /* Phase 2:  do the 3 pass AC vlc decode */
  vlc_error = FALSE;
  for (pass=1;pass<3;pass++) {
    vlc_trace("P%d",pass);
    if((pass == 2) && vlc_error) break;
    for (m=0,mb=seg->mb;
	 m<5;
	 m++,mb++) {
      /* if(vlc_error) goto abort_segment; */
      vlc_trace("\nM%d",m);
      if((pass == 1) && mb->vlc_error) continue;
      for (b=0,bl=mb->b;
	   b<6;
	   b++,bl++) {
	if(bl->eob) continue;
	coeffs = bl->coeffs;
	vlc_trace("\nB%d",b);
	bitstream_seek_set(bs,bl->offset);
	reorder = &bl->reorder;
	reorder_sentinel = bl->reorder_sentinel;
	bl_bit_source = bl;
	/* Main coeffient parsing loop */
	while(1) {
	  bits_left = bl_bit_source->end - bl_bit_source->offset;
	  dv_peek_vlc(bs,bits_left,&vlc);
	  if(vlc.run < 0) goto bh;
	  /* complete, valid vlc found */
	  vlc_trace("(%d,%d,%d)",vlc.run,vlc.amp,vlc.len);
	  bl_bit_source->offset += vlc.len;
	  bitstream_flush(bs,vlc.len);
	  *reorder += vlc.run;
	  if((*reorder) < reorder_sentinel) {
	    SET_COEFF(coeffs,(*reorder),vlc.amp);
	  } else {
	    vlc_trace("! vlc code overran coeff block");
	    goto vlc_error;
	  } /* else  */
	  continue;
	bh:
	  if(vlc.amp == 0) {
	    /* found eob vlc */
	    vlc_trace("#");
	    /* EOb -- zero out the rest of block */
	    *reorder = reorder_sentinel;
	    bl_bit_source->offset += 4;
	    bitstream_flush(bs,4);
	    bl->eob = 1;
	    mb->eob_count++;
	    break;
	  } else if(vlc.len == VLC_NOBITS) {
	    bl_bit_source->mark = TRUE;
	    switch(dv_find_spilled_vlc(seg,mb,&bl_bit_source,pass)) {
	    case 1: /* found: keep parsing */
	      break;
	    case 0: /* not found: move on to next block */
	      if(pass==1) goto mb_done;
	      else goto seg_done;
	      break;
	    case -1: 
	      goto vlc_error;
	      break;
	    } /* switch */
	  } else if(vlc.len == VLC_ERROR) {
	    goto vlc_error;
	  } /* else (used == ?)  */
	} /* while !bl->eob */
	goto bl_done;
  vlc_error:
	vlc_trace("X\n");
	vlc_error = TRUE;
	if(pass == 1) goto mb_done;
	else goto abort_segment;
  bl_done:
#if PARSE_VLC_TRACE
	if((bits_left = bl->end - bl->offset)) {
	  gint x;
	  bitstream_seek_set(bs,bl->offset);
	  vlc_trace("\n\tunused bits:\n\t");
	  for(x=bits_left-1;x>=0;x--) 
	    vlc_trace("%s", (bitstream_get(bs,1)) ? "1" : "0");
	} else { vlc_trace("\n\tno unused bits"); }
#endif /* PARSE_VLC_TRACE */
	; } /* for b  */
  mb_done: 
      ; } /* for m  */
    vlc_trace("\n");
  } /* for pass  */
  vlc_trace("Segment OK.  ");
  goto seg_done;
 abort_segment:
  vlc_trace("Segment aborted. ");
 seg_done:
#if 0
  /* zero out remaining coeffs  */
  x = 0;
  for(m=0,mb=seg->mb;
      m<5;
      m++,mb++) {
    for (b=0,bl=mb->b;
	 b<6;
	 b++,bl++) {
      coeffs = bl->coeffs;
      reorder = &bl->reorder;
      reorder_sentinel = bl->reorder_sentinel;
      while((*reorder) < reorder_sentinel) { 
	SET_COEFF(coeffs,(*reorder),0);
	x++;
      } /* while */
    } /* for b  */
  } /* for m  */
  vlc_trace("%d coeffs lost\n", x);
  return(x);
#else
  return(0);
#endif
#if 0
 fatal_vlc_error:
  vlc_trace("\n\nOffset %d\n", offset);
  vlc_trace("DIF Block:\n");
  for(x=0; x<dv_parse_bytes[b]; x++) {
    if(!(x%10)) vlc_trace("\n");
    print_byte(data[(m*80)+dv_parse_blockstart[b]+x]);
    vlc_trace(" ");
  } /* for  */
  vlc_trace("\n");
  exit(0);
#endif
} /* dv_parse_ac_coeffs */

void dv_parse_ac_coeffs_pass0(bitstream_t *bs,
			      dv_macroblock_t *mb,
			      dv_block_t *bl);

#if ! ARCH_X86
__inline__ void dv_parse_ac_coeffs_pass0(bitstream_t *bs,
						dv_macroblock_t *mb,
						dv_block_t *bl) {
  dv_vlc_t         vlc;
  gint             bits_left;
  guint32 bits;

  /* vlc_trace("\nB%d",b); */
  /* Main coeffient parsing loop */
  memset(&bl->coeffs[1],'\0',sizeof(bl->coeffs)-sizeof(bl->coeffs[0]));
  while(1) {
    bits_left = bl->end - bl->offset;
    bits = bitstream_show(bs,16);
    if(bits_left >= 16) 
      __dv_decode_vlc(bits, &vlc);
    else
      dv_decode_vlc(bits, bits_left,&vlc);
    if(vlc.run < 0) break;
    /* complete, valid vlc found */
    vlc_trace("(%d,%d,%d)",vlc.run,vlc.amp,vlc.len);
    bl->offset += vlc.len;
    bitstream_flush(bs,vlc.len);
    bl->reorder += vlc.run;
    SET_COEFF(bl->coeffs,bl->reorder,vlc.amp);
  } /* while */
  if(vlc.amp == 0) {
    /* found eob vlc */
    vlc_trace("#");
    /* EOb -- zero out the rest of block */
    bl->reorder = bl->reorder_sentinel;
    bl->offset += 4;
    bitstream_flush(bs,4);
    bl->eob = 1;
    mb->eob_count++;
  } else if(vlc.len == VLC_ERROR) {
    vlc_trace("X\n");
    mb->vlc_error = TRUE;
  } /* else */
#if PARSE_VLC_TRACE
  if((bits_left = bl->end - bl->offset)) {
    gint x;
    bitstream_seek_set(bs,bl->offset);
    vlc_trace("\n\tunused bits:\n\t");
    for(x=bits_left-1;x>=0;x--) 
      vlc_trace("%s", (bitstream_get(bs,1)) ? "1" : "0");
  } else { vlc_trace("\n\tno unused bits"); }
#endif /* PARSE_VLC_TRACE */
  vlc_trace("\n");
} /* dv_parse_ac_coeffs_pass0 */
#endif

/* DV requires vlc decode of AC coefficients for each block in three passes:
 *    Pass1 : decode coefficient vlc bits from their own block's area
 *    Pass2 : decode coefficient vlc bits spilled into areas of other blocks of the same macroblock
 *    Pass3 : decode coefficient vlc bits spilled into other macroblocks of the same video segment
 *
 * What to do about VLC errors?  This is tricky.
 *
 * The most conservative is to assume that any further spilled data is suspect.
 * So, on pass 1, a VLC error means do the rest, and skip passes 2 & 3
 * On passes 2 & 3, just abort.  This seems to drop a lot more coefficients, 21647 
 * in a single frame, that more tolerant aproaches.
 *
 *  */
#if ! ARCH_X86
gint dv_parse_video_segment(dv_videosegment_t *seg, guint quality) {
  gint             m, b;
  gint             mb_start;
  gint             dc;
  dv_macroblock_t *mb;
  dv_block_t      *bl;
  bitstream_t     *bs;
  guint n_blocks;

  vlc_trace("S[%d,%d]\n", seg->i,seg->k);
  /* Phase 1:  initialize data structures, and get the DC */
  bs = seg->bs;

  if (quality & DV_QUALITY_COLOR)
    n_blocks = 6;
  else
    n_blocks = 4;

  for(m=0,mb=seg->mb,mb_start=0;
      m<5;
      m++,mb++,mb_start+=(80*8)) {
#if STRICT_SYNTAX
    bitstream_seek_set(bs,mb_start);
    g_return_val_if_fail(((bitstream_get(bs,3) == DV_SCT_VIDEO)),6*64); /* SCT */
    bitstream_flush(bs,5);

    g_return_val_if_fail((bitstream_get(bs,4) == seg->i),6*64);
    g_return_val_if_fail((bitstream_get(bs,1) == DV_FSC_0),6*64);
    bitstream_flush(bs,3);

    bitstream_flush(bs,8); /* DBN -- could check this */
    mb->sta = bitstream_get(bs,4);
#else
    bitstream_seek_set(bs,mb_start+28);
#endif
    /* first get the macroblock-wide data */
    mb->qno = bitstream_get(bs,4);
    mb->vlc_error = 0;
    mb->eob_count = 0;
    mb->i = (seg->i + dv_super_map_vertical[m]) % (seg->isPAL?12:10);
    mb->j = dv_super_map_horizontal[m];
    mb->k = seg->k;
    if ((quality & DV_QUALITY_AC_MASK) == DV_QUALITY_DC) {
      /* DC only */
      for(b=0,bl=mb->b;
	  b < n_blocks;
	  b++,bl++) {
	memset(bl->coeffs, 0, sizeof(bl->coeffs));
	dc = bitstream_get(bs,9);  /* DC coefficient (twos complement) */
	if(dc > 255) dc -= 512;
	bl->coeffs[0] = dc;
	vlc_trace("DC [%d,%d,%d,%d] = %d\n",mb->i,mb->j,mb->k,b,dc);
	bl->dct_mode = bitstream_get(bs,1);
	bl->class_no = bitstream_get(bs,2);
	bitstream_seek_set(bs, mb_start + dv_parse_bit_end[b]);
      } /* for b */
    } else {
      /* quality is DV_QUALITY_AC_1 or DV_QUALITY_AC_2 */
      for(b=0,bl=mb->b;
	  b < n_blocks;
	  b++,bl++) {
	/* Pass 0, read bits from individual blocks  */
	/* Get DC coeff, mode, and class from start of block */
	dc = bitstream_get(bs,9);  /* DC coefficient (twos complement) */
	if(dc > 255) dc -= 512;
	bl->coeffs[0] = dc;
	vlc_trace("DC [%d,%d,%d,%d] = %d\n",mb->i,mb->j,mb->k,b,dc);
	bl->dct_mode = bitstream_get(bs,1);
	bl->class_no = bitstream_get(bs,2);
	bl->eob=0;
	bl->offset= mb_start + dv_parse_bit_start[b];
	bl->end= mb_start + dv_parse_bit_end[b];
	bl->reorder = &dv_reorder[bl->dct_mode][1];
	bl->reorder_sentinel = bl->reorder + 63;
	dv_parse_ac_coeffs_pass0(bs,mb,bl);
	bitstream_seek_set(bs,bl->end);
      } /* for b */
    } /* if quality */
  } /* for m */
  /* Phase 2:  do the 3 pass AC vlc decode */
  if ((quality & DV_QUALITY_AC_MASK) == DV_QUALITY_AC_2)
    return(dv_parse_ac_coeffs(seg));
  else
    return 0;
} /* dv_parse_video_segment  */
#endif

static void
dv_parse_vaux (dv_decoder_t *dv, guchar *buffer) {
  gint	i, j;

  /* 
   * reset vaux structure first
   */
  dv -> vaux_next = 0;
  memset (dv -> vaux_pack, 0xff, sizeof (dv -> vaux_pack));
  /* 
   * so search thru all 3 vaux packs
   */
  for (i = 0, buffer += 240; i < 3; ++i, buffer += 80) {
    /*
     * each pack may contain up to 15 packets
     */
    for (j = 0; j < 15; j++) {
      if (buffer [3 + (j * 5)] != 0xff && dv -> vaux_next < 45) {
	dv -> vaux_pack [buffer [3 + (j * 5)]] = dv -> vaux_next;
	memcpy (dv -> vaux_data [dv -> vaux_next], &buffer [3 + 1 + (j * 5)], 4);
	dv -> vaux_next++;
#if 0
	fprintf (stderr,
		 " pack (%02x) (%02x,%02x,%02x,%02x)\n",
		 buffer [3 + 0 + (j * 5)],
		 buffer [3 + 1 + (j * 5)],
		 buffer [3 + 2 + (j * 5)],
		 buffer [3 + 3 + (j * 5)],
		 buffer [3 + 4 + (j * 5)]);
#endif
      }
    }
  }
}

gint
dv_parse_id(bitstream_t *bs,dv_id_t *id) {
  id->sct = bitstream_get(bs,3);
  bitstream_flush(bs,5);
  id->dsn = bitstream_get(bs,4);
  id->fsc = bitstream_get(bs,1);
  bitstream_flush(bs,3);
  id->dbn = bitstream_get(bs,8);
  return 0;
} /* dv_parse_id */

gint
dv_parse_header(dv_decoder_t *dv, guchar *buffer) {
  dv_header_t *header = &dv->header;
  bitstream_t *bs;
  dv_id_t      id;
  gint         prev_system, result = 0;

  if(!(bs = bitstream_init())) goto no_bitstream;
  bitstream_new_buffer(bs,buffer,6*80);
  dv_parse_id(bs,&id);
  if (id.sct != 0) goto parse_error;            /* error, if not header */
  header->dsf = bitstream_get(bs,1);
  if (bitstream_get(bs,1) != 0) goto parse_error; /* error, bit incorrect */
  bitstream_flush(bs,11);
  header->apt = bitstream_get(bs,3);
  header->tf1 = bitstream_get(bs,1);
  bitstream_flush(bs,4);
  header->ap1 = bitstream_get(bs,3);
  header->tf2 = bitstream_get(bs,1);
  bitstream_flush(bs,4);
  header->ap2 = bitstream_get(bs,3);
  header->tf3 = bitstream_get(bs,1);
  bitstream_flush(bs,4);
  header->ap3 = bitstream_get(bs,3);
  bitstream_flush_large(bs,576);		/* skip rest of DIF block */

  /* 
   * parse vaux data now to check if there is a inconsistanciy between
   * header->dsf and vaux data for auto mode
   */
  dv_parse_vaux (dv, buffer);
  switch(dv->arg_video_system) {
  case 0:
    prev_system = dv->system;
    dv->system = (header->dsf || (dv_system_50_fields (dv) == 1)) ?
      e_dv_system_625_50 : e_dv_system_525_60;
    if (prev_system != dv->system) {
      if (dv->system == e_dv_system_625_50)
	result = 1;
      else
	result = 2;
    } /* if */
    dv->std = ((header->apt) ? e_dv_std_smpte_314m : e_dv_std_iec_61834);
    break;
  case 1:
    /* NTSC */
    dv->system = e_dv_system_525_60;
    dv->std = e_dv_std_smpte_314m;  /* arbitrary */
    break;
  case 2:
    /* PAL/IEC 68134 */
    dv->system = e_dv_system_625_50;
    dv->std = e_dv_std_iec_61834;
    break;
  case 3:
    /* PAL/SMPTE 314M */
    dv->system = e_dv_system_625_50;
    dv->std = e_dv_std_smpte_314m;  
    break;
  } /* switch  */

  dv->width = 720;
  dv->sampling = ((dv->system == e_dv_system_625_50) && (dv->std == e_dv_std_iec_61834)) ? e_dv_sample_420 : e_dv_sample_411;
  if(dv->system == e_dv_system_625_50) {
    dv->num_dif_seqs = 12;
    dv->height = 576;
    dv->frame_size = 12 * 150 * 80;
  } else {
    dv->num_dif_seqs = 10;
    dv->height = 480;
    dv->frame_size = 10 * 150 * 80;
  } /* else */

  dv_parse_id(bs,&id);				/* should be SC1 */
  bitstream_flush_large(bs,616);
  dv_parse_id(bs,&id);				/* should be SC2 */
  bitstream_flush_large(bs,616);

  dv_parse_id(bs,&id);				/* should be VA1 */
  bitstream_flush_large(bs,616);
  dv_parse_id(bs,&id);				/* should be VA2 */
  bitstream_flush_large(bs,616);
  dv_parse_id(bs,&id);				/* should be VA3 */
  bitstream_flush_large(bs,616);

  dv_parse_audio_header(dv, buffer);

  return(result);

 parse_error:
 no_bitstream:
  return(-1);
} /* dv_parse_header */

