/* 
 *  dv.c
 *
 *     Copyright (C) Charles 'Buck' Krasic - November 2000
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dv_types.h"
#include "dv.h"
#include "encode.h"
#include "util.h"
#include "dct.h"
#include "audio.h"
#include "idct_248.h"
#include "quant.h"
#include "weighting.h"
#include "vlc.h"
#include "parse.h"
#include "place.h"
#include "rgb.h"
#include "YUY2.h"
#include "YV12.h"
#if ARCH_X86
#include "mmx.h"
#endif

#if YUV_420_USE_YV12
#define DV_MB420_YUV(a,b,c)     dv_mb420_YV12    (a,b,c)
#define DV_MB420_YUV_MMX(a,b,c) dv_mb420_YV12_mmx(a,b,c)
#else
#define DV_MB420_YUV(a,b,c)     dv_mb420_YUY2    (a,b,c)
#define DV_MB420_YUV_MMX(a,b,c) dv_mb420_YUY2_mmx(a,b,c)
#endif 

gboolean dv_use_mmx;

#if HAVE_LIBPOPT
static void
dv_decoder_popt_callback(poptContext con, enum poptCallbackReason reason, 
		       const struct poptOption * opt, const char * arg, const void * data)
{
  dv_decoder_t *decoder = (dv_decoder_t *)data;

  if((decoder->arg_video_system < 0) || (decoder->arg_video_system > 3)) {
    dv_opt_usage(con, decoder->option_table, DV_DECODER_OPT_SYSTEM);
  } /* if */
  
} /* dv_decoder_popt_callback  */
#endif /* HAVE_LIBPOPT */

dv_decoder_t * 
dv_decoder_new(void) {
  dv_decoder_t *result;
  
  result = (dv_decoder_t *)calloc(1,sizeof(dv_decoder_t));
  if(!result) goto no_mem;
  
  result->video = dv_video_new();
  if(!result->video) goto no_video;

  result->audio = dv_audio_new();
  if(!result->audio) goto no_audio;

#if HAVE_LIBPOPT
  result->option_table[DV_DECODER_OPT_SYSTEM] = (struct poptOption) {
    longName: "video-system", 
    shortName: 'V', 
    argInfo: POPT_ARG_INT, 
    descrip: "video standard:" 
    "0=autoselect [default]," 
    " 1=525/60 4:1:1 (NTSC),"
    " 2=625/50 4:2:0 (PAL,IEC 61834 DV),"
    " 3=625/50 4:1:1 (PAL,SMPTE 314M DV)",
    argDescrip: "(0|1|2|3)",
    arg: &result->arg_video_system,
  }; /* system */

  result->option_table[DV_DECODER_OPT_VIDEO_INCLUDE] = (struct poptOption) {
    argInfo: POPT_ARG_INCLUDE_TABLE,
    descrip: "Video decode options",
    arg: &result->video->option_table[0],
  }; /* video include */

  result->option_table[DV_DECODER_OPT_AUDIO_INCLUDE] = (struct poptOption) {
    argInfo: POPT_ARG_INCLUDE_TABLE,
    descrip: "Audio decode options",
    arg: result->audio->option_table,
  }; /* audio include */

  result->option_table[DV_DECODER_OPT_CALLBACK] = (struct poptOption){
    argInfo: POPT_ARG_CALLBACK|POPT_CBFLAG_POST,
    arg:     dv_decoder_popt_callback,
    descrip: (char *)result, /* data passed to callback */
  }; /* callback */

#endif /* HAVE_LIBPOPT */

  return(result);

 no_audio:
  free(result->video);
 no_video:
  free(result);
  result=NULL;
 no_mem:
  return(result);
} /* dv_decoder_new */

void 
dv_init(void) {
  static gboolean done=FALSE;
  if(done) goto init_done;
#if ARCH_X86
  dv_use_mmx = mmx_ok(); 
#endif
  weight_init();
  dct_init();
  dv_dct_248_init();
  dv_construct_vlc_table();
  dv_parse_init();
  dv_place_init();
  dv_quant_init();
  dv_rgb_init();
  dv_YUY2_init();
  dv_YV12_init();
  init_vlc_test_lookup();
  init_vlc_encode_lookup();
  init_qno_start();
  prepare_reorder_tables();

  done=TRUE;
 init_done:
  return;
} /* dv_init */

static inline void 
dv_decode_macroblock(dv_decoder_t *dv, dv_macroblock_t *mb, guint quality) {
  gint i;
  for (i=0;
       i<((quality & DV_QUALITY_COLOR) ? 6 : 4);
       i++) {
    if (mb->b[i].dct_mode == DV_DCT_248) {
	dv_248_coeff_t co248[64];

      quant_248_inverse (mb->b[i].coeffs, mb->qno, mb->b[i].class_no, co248);
      dv_idct_248 (co248, mb->b[i].coeffs);
    } else {
#if ARCH_X86
      quant_88_inverse_x86(mb->b[i].coeffs,mb->qno,mb->b[i].class_no);
      idct_88(mb->b[i].coeffs);
#else /* ARCH_X86 */
      quant_88_inverse(mb->b[i].coeffs,mb->qno,mb->b[i].class_no);
      weight_88_inverse(mb->b[i].coeffs);
      idct_88(mb->b[i].coeffs);
#endif /* ARCH_X86 */
    } /* else */
  } /* for b */
} /* dv_decode_macroblock */

void 
dv_decode_video_segment(dv_decoder_t *dv, dv_videosegment_t *seg, guint quality) {
  dv_macroblock_t *mb;
  dv_block_t *bl;
  gint m, b;
  for (m=0,mb = seg->mb;
       m<5;
       m++,mb++) {
    for (b=0,bl = mb->b;
	 b<((quality & DV_QUALITY_COLOR) ? 6 : 4);
	 b++,bl++) {
      if (bl->dct_mode == DV_DCT_248) {
	  dv_248_coeff_t co248[64];

	quant_248_inverse (mb->b[b].coeffs, mb->qno, mb->b[b].class_no, co248);
	dv_idct_248 (co248, mb->b[b].coeffs);
      } else {
#if ARCH_X86
	quant_88_inverse_x86(bl->coeffs,mb->qno,bl->class_no);
	weight_88_inverse(bl->coeffs);
	idct_88(bl->coeffs);
#else /* ARCH_X86 */
	quant_88_inverse(bl->coeffs,mb->qno,bl->class_no);
	weight_88_inverse(bl->coeffs);
	idct_88(bl->coeffs);
#endif /* ARCH_X86 */
      } /* else */
    } /* for b */
  } /* for mb */
} /* dv_decode_video_segment */

static inline void
dv_render_macroblock_rgb(dv_decoder_t *dv, dv_macroblock_t *mb, guchar **pixels, gint *pitches ) {
  if(dv->sampling == e_dv_sample_411) {
    if(mb->x >= 704) {
      dv_mb411_right_rgb(mb, pixels, pitches); /* Right edge are 16x16 */
    } else {
      dv_mb411_rgb(mb, pixels, pitches);
    } /* else */
  } else {
    dv_mb420_rgb(mb, pixels, pitches);
  } /* else */
} /* dv_render_macroblock_rgb */

void
dv_render_video_segment_rgb(dv_decoder_t *dv, dv_videosegment_t *seg, guchar **pixels, gint *pitches ) {
  dv_macroblock_t *mb;
  gint m;
  for (m=0,mb = seg->mb;
       m<5;
       m++,mb++) {
    if(dv->sampling == e_dv_sample_411) {
      if(mb->x >= 704) {
	dv_mb411_right_rgb(mb, pixels, pitches); /* Right edge are 16x16 */
      } else {
	dv_mb411_rgb(mb, pixels, pitches);
      } /* else */
    } else {
      dv_mb420_rgb(mb, pixels, pitches);
    } /* else */
  } /* for    */
} /* dv_render_video_segment_rgb */

static inline void
dv_render_macroblock_bgr0(dv_decoder_t *dv, dv_macroblock_t *mb, guchar **pixels, gint *pitches ) {
  if(dv->sampling == e_dv_sample_411) {
    if(mb->x >= 704) {
      dv_mb411_right_bgr0(mb, pixels, pitches); /* Right edge are 16x16 */
    } else {
      dv_mb411_bgr0(mb, pixels, pitches);
    } /* else */
  } else {
    dv_mb420_bgr0(mb, pixels, pitches);
  } /* else */
} /* dv_render_macroblock_bgr0 */

void
dv_render_video_segment_bgr0(dv_decoder_t *dv, dv_videosegment_t *seg, guchar **pixels, gint *pitches ) {
  dv_macroblock_t *mb;
  gint m;
  for (m=0,mb = seg->mb;
       m<5;
       m++,mb++) {
    if(dv->sampling == e_dv_sample_411) {
      if(mb->x >= 704) {
	dv_mb411_right_bgr0(mb, pixels, pitches); /* Right edge are 16x16 */
      } else {
	dv_mb411_bgr0(mb, pixels, pitches);
      } /* else */
    } else {
      dv_mb420_bgr0(mb, pixels, pitches);
    } /* else */
  } /* for    */
} /* dv_render_video_segment_bgr0 */

#if ARCH_X86

static inline void
dv_render_macroblock_yuv(dv_decoder_t *dv, dv_macroblock_t *mb, guchar **pixels, gint *pitches) {
  if(dv_use_mmx) {
    if(dv->sampling == e_dv_sample_411) {
      if(mb->x >= 704) {
	dv_mb411_right_YUY2_mmx(mb, pixels, pitches); /* Right edge are 420! */
      } else {
	dv_mb411_YUY2_mmx(mb, pixels, pitches);
      } /* else */
    } else {
      DV_MB420_YUV_MMX(mb, pixels, pitches);
    } /* else */
  } else {
    if(dv->sampling == e_dv_sample_411) {
      if(mb->x >= 704) {
	dv_mb411_right_YUY2(mb, pixels, pitches); /* Right edge are 420! */
      } else {
	dv_mb411_YUY2(mb, pixels, pitches);
      } /* else */
    } else {
      DV_MB420_YUV(mb, pixels, pitches);
    } /* else */
  } /* else */
} /* dv_render_macroblock_yuv */

void
dv_render_video_segment_yuv(dv_decoder_t *dv, dv_videosegment_t *seg, guchar **pixels, gint *pitches) {
  dv_macroblock_t *mb;
  gint m;
  for (m=0,mb = seg->mb;
       m<5;
       m++,mb++) {
    if(dv_use_mmx) {
      if(dv->sampling == e_dv_sample_411) {
	if(mb->x >= 704) {
	  dv_mb411_right_YUY2_mmx(mb, pixels, pitches); /* Right edge are 420! */
	} else {
	  dv_mb411_YUY2_mmx(mb, pixels, pitches);
	} /* else */
      } else {
	DV_MB420_YUV_MMX(mb, pixels, pitches);
      } /* else */
    } else {
      if(dv->sampling == e_dv_sample_411) {
	if(mb->x >= 704) {
	  dv_mb411_right_YUY2(mb, pixels, pitches); /* Right edge are 420! */
	} else {
	  dv_mb411_YUY2(mb, pixels, pitches);
	} /* else */
      } else {
	DV_MB420_YUV(mb, pixels, pitches);
      } /* else */
    } /* else */
  } /* for    */
} /* dv_render_video_segment_yuv */

#else /* ARCH_X86 */

static inline void
dv_render_macroblock_yuv(dv_decoder_t *dv, dv_macroblock_t *mb, guchar **pixels, gint *pitches) {
  if(dv->sampling == e_dv_sample_411) {
    if(mb->x >= 704) {
      dv_mb411_right_YUY2(mb, pixels, pitches); /* Right edge are 420! */
    } else {
      dv_mb411_YUY2(mb, pixels, pitches);
    } /* else */
  } else {
    DV_MB420_YUV(mb, pixels, pitches);
  } /* else */
} /* dv_render_macroblock_yuv */

void
dv_render_video_segment_yuv(dv_decoder_t *dv, dv_videosegment_t *seg, guchar **pixels, gint *pitches) {
  dv_macroblock_t *mb;
  gint m;
  for (m=0,mb = seg->mb;
       m<5;
       m++,mb++) {
    if(dv->sampling == e_dv_sample_411) {
      if(mb->x >= 704) {
	dv_mb411_right_YUY2(mb, pixels, pitches); /* Right edge are 420! */
      } else {
	dv_mb411_YUY2(mb, pixels, pitches);
      } /* else */
    } else {
      DV_MB420_YUV(mb, pixels, pitches);
    } /* else */
  } /* for    */
} /* dv_render_video_segment_yuv */

#endif /* ! ARCH_X86 */

static gint32 ranges[6][2];

void dv_check_coeff_ranges(dv_macroblock_t *mb) {
  dv_block_t *bl;
  gint b, i;
  for (b=0,bl = mb->b;
       b<6;
       b++,bl++) {
    for(i=0;i<64;i++) {
      ranges[b][0] = MIN(ranges[b][0],bl->coeffs[i]);
      ranges[b][1] = MAX(ranges[b][1],bl->coeffs[i]);
    }
  }
}

void
dv_decode_full_frame(dv_decoder_t *dv, guchar *buffer, 
		     dv_color_space_t color_space, guchar **pixels, gint *pitches) {

  static dv_videosegment_t vs;
  dv_videosegment_t *seg = &vs;
  dv_macroblock_t *mb;
  gint ds, v, m;
  guint offset = 0, dif = 0, audio=0;

  if(!seg->bs) {
    seg->bs = bitstream_init();
    if(!seg->bs) 
      goto no_mem;
  } /* if */
  seg->isPAL = (dv->system == e_dv_system_625_50);

  /* each DV frame consists of a sequence of DIF segments  */
  for (ds=0; ds < dv->num_dif_seqs; ds++) {
    /* Each DIF segment conists of 150 dif blocks, 135 of which are video blocks */
    /* A video segment consists of 5 video blocks, where each video
       block contains one compressed macroblock.  DV bit allocation
       for the VLC stage can spill bits between blocks in the same
       video segment.  So parsing needs the whole segment to decode
       the VLC data */
    dif += 6;
    audio=0;
    /* Loop through video segments  */
    for (v=0;v<27;v++) {
      /* skip audio block - interleaved before every 3rd video segment */
      if(!(v % 3)) {
	/*dv_dump_aaux_as(buffer+(dif*80), ds, audio); */
	dif++; 
	audio++;
      } /* if */
      /* stage 1: parse and VLC decode 5 macroblocks that make up a video segment */
      offset = dif * 80;
      bitstream_new_buffer(seg->bs, buffer + offset, 80*5); 
      dv_parse_video_segment(seg, dv->quality);
      /* stage 2: dequant/unweight/iDCT blocks, and place the macroblocks */
      dif+=5;
      seg->i = ds;
      seg->k = v;
      switch(color_space) {
      case e_dv_color_yuv:
	for (m=0,mb = seg->mb;
	     m<5;
	     m++,mb++) {
	  dv_decode_macroblock(dv, mb, dv->quality);
	  dv_place_macroblock(dv, seg, mb, m);
	  dv_render_macroblock_yuv(dv, mb, pixels, pitches);
	} /* for m */
	break;
      case e_dv_color_bgr0:
	for (m=0,mb = seg->mb;
	     m<5;
	     m++,mb++) {
	  dv_decode_macroblock(dv, mb, dv->quality);
	  dv_place_macroblock(dv, seg, mb, m);
	  dv_render_macroblock_bgr0(dv, mb, pixels, pitches);
	} /* for m */
        break;
      case e_dv_color_rgb:
	for (m=0,mb = seg->mb;
	     m<5;
	     m++,mb++) {
	  dv_decode_macroblock(dv, mb, dv->quality);
	  dv_place_macroblock(dv, seg, mb, m);
#if RANGE_CHECKING
	  dv_check_coeff_ranges(mb);
#endif
	  dv_render_macroblock_rgb(dv, mb, pixels, pitches);
	} /* for m */
	break;
      } /* switch */
    } /* for v */
  } /* ds */
#if RANGE_CHECKING
  for(i=0;i<6;i++) {
    fprintf(stderr, "range[%d] min %d max %d\n", i, ranges[i][0], ranges[i][1]);
  }
#endif
  return;
 no_mem:
  fprintf(stderr,"no memory for bitstream!\n");
  exit(-1);
} /* dv_decode_full_frame  */

gboolean 
dv_decode_full_audio(dv_decoder_t *dv, guchar *buffer, gint16 **outbufs)
{
  gint ds, dif, audio_dif, result;
  gint ch;

  dif=0;
  if(!dv_parse_audio_header(dv, buffer)) goto no_audio;

  for (ds=0; ds < dv->num_dif_seqs; ds++) {
    dif += 6;
    for(audio_dif=0; audio_dif<9; audio_dif++) {
      if((result = dv_decode_audio_block(dv->audio, buffer+(dif *80), ds, audio_dif, outbufs))) goto fail;
      dif+=16;
    } /* for  */
  } /* for */

  if(dv->audio->emphasis) {
    for(ch=0; ch< dv->audio->num_channels; ch++) {
      dv_audio_deemphasis(dv->audio, outbufs[ch]);
    } /* for  */
  } /* if */
  
  return(TRUE);
  
 fail:
 no_audio:
  return(FALSE);
  
} /* dv_decode_full_audio */


/*
 * query functions based upon vaux data
 */

int
dv_get_vaux_pack (dv_decoder_t *dv, guint8 pack_id, guint8 *data)
{
  guint8  id;
  if ((id = dv -> vaux_pack [pack_id]) == 0xff) 
    return -1;
  memcpy (data, dv -> vaux_data [id], 4);
  return 0;
} /* dv_get_vaux_pack */

int
dv_frame_is_color (dv_decoder_t *dv)
{
  guint8  id;

  if ((id = dv -> vaux_pack [0x60]) != 0xff) {
    if (dv -> vaux_data [id] [1] & 0x80) {
      return 1;
    }
    return 0;
  }
  return -1;
}

int
dv_system_50_fields (dv_decoder_t *dv)
{
  guint8  id;

  if ((id = dv -> vaux_pack [0x60]) != 0xff) {
    if (dv -> vaux_data [id] [2] & 0x20) {
      return 1;
    }
    return 0;
  }
  return -1;
}

int
dv_format_normal (dv_decoder_t *dv)
{
  guint8  id;

  if ((id = dv -> vaux_pack [0x61]) != 0xff) {
    if (!(dv -> vaux_data [id] [1] & 0x07)) {
      return 1;
    }
    return 0;
  }
  return -1;
}

int
dv_format_wide (dv_decoder_t *dv)
{
  guint8  id;

  if ((id = dv -> vaux_pack [0x61]) != 0xff) {
    if (dv -> vaux_data [id] [1] & 0x07) {
      return 1;
    }
    return 0;
  }
  return -1;
}

int
dv_frame_changed (dv_decoder_t *dv)
{
  guint8  id;

  if ((id = dv -> vaux_pack [0x61]) != 0xff) {
    if (dv -> vaux_data [id] [2] & 0x20) {
      return 1;
    }
    return 0;
  }
  return -1;
}
