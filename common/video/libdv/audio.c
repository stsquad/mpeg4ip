/* 
 *  audio.c
 *
 *     Copyright (C) Charles 'Buck' Krasic - January 2001
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
#include <math.h>

#include "util.h"
#include "audio.h"

/*
 * DV audio data is shuffled within the frame data.  The unshuffle 
 * tables are indexed by DIF sequence number, then audio DIF block number.
 * The first audio channel (pair) is in the first half of DIF sequences, and the
 * second audio channel (pair) is in the second half.
 *
 * DV audio can be sampled at 48, 44.1, and 32 kHz.  Normally, samples
 * are quantized to 16 bits, with 2 channels.  In 32 kHz mode, it is
 * possible to support 4 channels by non-linearly quantizing to 12
 * bits.
 *
 * The code here always returns 16 bit samples.  In the case of 12
 * bit, we upsample to 16 according to the DV standard defined
 * mapping.
 *
 * DV audio can be "locked" or "unlocked".  In unlocked mode, the
 * number of audio samples per video frame can vary somewhat.  Header
 * info in the audio sections (AAUX AS) is used to tell the decoder on
 * a frame be frame basis how many samples are present. 
 * */

static int dv_audio_unshuffle_60[5][9] = {
  { 0, 15, 30, 10, 25, 40,  5, 20, 35 },
  { 3, 18, 33, 13, 28, 43,  8, 23, 38 },
  { 6, 21, 36,  1, 16, 31, 11, 26, 41 },
  { 9, 24, 39,  4, 19, 34, 14, 29, 44 },
  {12, 27, 42,  7, 22, 37,  2, 17, 32 },
};

static int dv_audio_unshuffle_50[6][9] = {
  {  0, 18, 36, 13, 31, 49,  8, 26, 44 },
  {  3, 21, 39, 16, 34, 52, 11, 29, 47 },
  {  6, 24, 42,  1, 19, 37, 14, 32, 50 }, 
  {  9, 27, 45,  4, 22, 40, 17, 35, 53 }, 
  { 12, 30, 48,  7, 24, 43,  2, 20, 38 },
  { 15, 33, 51, 10, 28, 46,  5, 23, 41 },
};

/* Minumum number of samples, indexed by system (PAL/NTSC) and
   sampling rate (32 kHz, 44.1 kHz, 48 kHz) */

static gint min_samples[2][3] = {
  { 1580, 1452, 1053 }, /* 60 fields (NTSC) */
  { 1896, 1742, 1264 }, /* 50 fields (PAL) */
};

static gint max_samples[2][3] = {
  { 1620, 1489, 1080 }, /* 60 fields (NTSC) */
  { 1944, 1786, 1296 }, /* 50 fields (PAL) */
};

static gint frequency[3] = {
  48000, 44100, 32000
};


#ifdef __GNUC__
static gint quantization[8] = {
  [0] 16,
  [1] 12,
  [2] 20,
  [3 ... 7] = -1,
};
#else /* ! __GNUC__ */
static gint quantization[8] = {
  16,
  12,
  20,
  -1,
  -1,
  -1,
  -1,
  -1,
};
#endif /* ! __GNUC__ */



#if HAVE_LIBPOPT
static void
dv_audio_popt_callback(poptContext con, enum poptCallbackReason reason, 
		       const struct poptOption * opt, const char * arg, const void * data)
{
  dv_audio_t *audio = (dv_audio_t *)data;

  if((audio->arg_audio_frequency < 0) || (audio->arg_audio_frequency > 3)) {
    dv_opt_usage(con, audio->option_table, DV_AUDIO_OPT_FREQUENCY);
  } /* if */
  
  if((audio->arg_audio_quantization < 0) || (audio->arg_audio_quantization > 2)) {
    dv_opt_usage(con, audio->option_table, DV_AUDIO_OPT_QUANTIZATION);
  } /* if */

  if((audio->arg_audio_emphasis < 0) || (audio->arg_audio_emphasis > 2)) {
    dv_opt_usage(con, audio->option_table, DV_AUDIO_OPT_EMPHASIS);
  } /* if */

} /* dv_audio_popt_callback  */
#endif /* HAVE_LIBPOPT */

static gint 
dv_audio_samples_per_frame(dv_aaux_as_t *dv_aaux_as, gint freq) {
  gint result = -1;
  gint col;
  
  switch(freq) {
  case 48000:
    col = 0;
    break;
  case 44100:
    col = 1;
    break;
  case 32000:
    col = 2;
    break;
  default:
    goto unknown_freq;
    break;
  }
  if(!(dv_aaux_as->pc3.system < 2)) goto unknown_format;

  result = dv_aaux_as->pc1.af_size + min_samples[dv_aaux_as->pc3.system][col];
 done:
  return(result);

 unknown_format:
  fprintf(stderr, "libdv(%s):  badly formed AAUX AS data [pc3.system:%d, pc4.smp:%d]\n",
	  __FUNCTION__, dv_aaux_as->pc3.system, dv_aaux_as->pc4.smp);
  goto done;

 unknown_freq:
  fprintf(stderr, "libdv(%s):  frequency %d not supported\n",
	  __FUNCTION__, freq);
  goto done;
} /* dv_audio_samples_per_frame */

/* Take a DV 12bit audio sample upto 16 bits. 
 * See IEC 61834-2, Figure 16, on p. 129 */

static __inline__ gint32 
dv_upsample(gint32 sample) {
  gint32 shift, result;
  
  shift = (sample & 0xf00) >> 8;
  switch(shift) {

#ifdef __GNUC__
  case 0x2 ... 0x7:
    shift--;
    result = (sample - (256 * shift)) << shift;
    break;
  case 0x8 ... 0xd:
    shift = 0xe - shift;
    result = ((sample + ((256 * shift) + 1)) << shift) - 1;
    break;
#else /* ! __GNUC__ */
  case 0x2:
  case 0x3:
  case 0x4:
  case 0x5:
  case 0x6:
  case 0x7:
    shift--;
    result = (sample - (256 * shift)) << shift;
    break;
  case 0x8:
  case 0x9:
  case 0xa:
  case 0xb:
  case 0xc:
  case 0xd:
    shift = 0xe - shift;
    result = ((sample + ((256 * shift) + 1)) << shift) - 1;
    break;
#endif /* ! __GNUC__ */

  default:
    result = sample;
    break;
  } /* switch */
  return(result);
} /* dv_upsample */

dv_audio_t *
dv_audio_new(void)
{
  dv_audio_t *result;
  
  if(!(result = (dv_audio_t *)calloc(1,sizeof(dv_audio_t)))) goto no_mem;

#if HAVE_LIBPOPT
  result->option_table[DV_AUDIO_OPT_FREQUENCY] = (struct poptOption){ 
    longName:   "frequency", 
    shortName:  'f', 
    argInfo:    POPT_ARG_INT, 
    arg:        &result->arg_audio_frequency,
    descrip:    "audio frequency: 0=autodetect [default], 1=32 kHz, 2=44.1 kHz, 3=48 kHz",
    argDescrip: "(0|1|2|3)"
  }; /* freq */

  result->option_table[DV_AUDIO_OPT_QUANTIZATION] = (struct poptOption){ 
    longName:   "quantization", 
    shortName:  'Q', 
    argInfo:    POPT_ARG_INT, 
    arg:        &result->arg_audio_quantization,
    descrip:    "audio quantization: 0=autodetect [default], 1=12 bit, 2=16bit",
    argDescrip: "(0|1|2)"
  }; /* quant */

  result->option_table[DV_AUDIO_OPT_EMPHASIS] = (struct poptOption){ 
    longName:   "emphasis", 
    shortName:  'e', 
    argInfo:    POPT_ARG_INT, 
    arg:        &result->arg_audio_emphasis,
    descrip:    "first-order preemphasis of 50/15 us: 0=autodetect [default], 1=on, 2=off",
    argDescrip: "(0|1|2)"
  }; /* quant */

  result->option_table[DV_AUDIO_OPT_CALLBACK] = (struct poptOption){
    argInfo: POPT_ARG_CALLBACK|POPT_CBFLAG_POST,
    arg:     dv_audio_popt_callback,
    descrip: (char *)result, /* data passed to callback */
  }; /* callback */

#endif /* HAVE_LIBPOPT */
  return(result);

 no_mem:
  return(result);
} /* dv_audio_new */

void 
dv_dump_aaux_as(void *buffer, int ds, int audio_dif) 
{
  dv_aaux_as_t *dv_aaux_as;

  dv_aaux_as = (dv_aaux_as_t *)buffer + 3; /* Is this correct after cast? */

  if(dv_aaux_as->pc0 == 0x50) {
    /* AAUX AS  */

    printf("DS %d, Audio DIF %d, AAUX AS pack: ", ds, audio_dif);

    if(dv_aaux_as->pc1.lf) {
      printf("Unlocked audio");
    } else {
      printf("Locked audio");
    }

    printf(", Sampling ");
    printf("%.1f kHz", (float)frequency[dv_aaux_as->pc4.smp] / 1000.0);

    printf(" (%d samples, %d fields)", 
	   dv_audio_samples_per_frame(dv_aaux_as,frequency[dv_aaux_as->pc4.smp]),
	   (dv_aaux_as->pc3.system ? 50 : 60));

    printf(", Quantization %d bits", quantization[dv_aaux_as->pc4.qu]);
    
    printf(", Emphasis %s\n", (dv_aaux_as->pc4.ef ? "off" : "on"));

  } else {

    fprintf(stderr, "libdv(%s):  Missing AAUX AS PACK!\n", __FUNCTION__);

  } /* else */

} /* dv_dump_aaux_as */

gboolean
dv_parse_audio_header(dv_decoder_t *decoder, guchar *inbuf)
{
  dv_audio_t *audio = decoder->audio;
  dv_aaux_as_t *dv_aaux_as   = (dv_aaux_as_t *) (inbuf + 80*6+80*16*3 + 3);
  dv_aaux_asc_t *dv_aaux_asc = (dv_aaux_asc_t *)(inbuf + 80*6+80*16*4 + 3);
  gboolean normal_speed = FALSE;

  if((dv_aaux_as->pc0 != 0x50) || (dv_aaux_asc->pc0 != 0x51)) goto bad_id;

  audio->max_samples =  max_samples[dv_aaux_as->pc3.system][dv_aaux_as->pc4.smp];
  /* For now we assume that 12bit = 4 channels */
  if(dv_aaux_as->pc4.qu > 1) goto unsupported_quantization;
  /*audio->num_channels = (dv_aaux_as->pc4.qu+1) * 2; // TODO verify this is right with known 4-channel input */

  audio->num_channels = 2; /* TODO verify this is right with known 4-channel input */

  switch(audio->arg_audio_frequency) {
  case 0:
    audio->frequency = frequency[dv_aaux_as->pc4.smp];
    break;
  case 1:
    audio->frequency = 32000;
    break;
  case 2:
    audio->frequency = 44100;
    break;
  case 3:
    audio->frequency = 44100;
    break;
  }  /* switch  */

  switch(audio->arg_audio_quantization) {
  case 0:
    audio->quantization = quantization[dv_aaux_as->pc4.qu];
    break;
  case 1:
    audio->quantization = 12;
    break;
  case 2:
    audio->quantization = 16;
    break;
  }  /* switch  */

  switch(audio->arg_audio_emphasis) {
  case 0:
    if(decoder->std == e_dv_std_iec_61834) {
      audio->emphasis = (dv_aaux_as->pc4.ef == 0);
    } else if(decoder->std == e_dv_std_iec_61834) {
      audio->emphasis = (dv_aaux_asc->pc1.ss == 1);
    } else {
      /* TODO: should never happen... */
    }
    break;
  case 1:
    audio->emphasis = TRUE;
    break;
  case 2:
    audio->emphasis = FALSE;
    break;
  } /* switch  */

  audio->samples_this_frame = dv_audio_samples_per_frame(dv_aaux_as,audio->frequency);

  audio->aaux_as  = *dv_aaux_as;
  audio->aaux_asc = *dv_aaux_asc;

  if(decoder->std == e_dv_std_iec_61834) {
    normal_speed = (dv_aaux_asc->pc3.speed == 0x20);
  } else if(decoder->std == e_dv_std_iec_61834) {
    if(dv_aaux_as->pc3.system) {
      /* PAL */
      normal_speed = (dv_aaux_asc->pc3.speed == 0x64);
    } else {
      /* NTSC */
      normal_speed = (dv_aaux_asc->pc3.speed == 0x78);
    } /* else */
  } /* else */

  return(normal_speed); /* don't do audio if speed is not 1 */

 bad_id:
  return(FALSE);
  
 unsupported_quantization:
  fprintf(stderr, "libdv(%s):  Malformrmed AAUX AS? pc4.qu == %d\n", 
	  __FUNCTION__, audio->aaux_as.pc4.qu);
  return(FALSE);

} /* dv_parse_audio_header */

gboolean
dv_update_num_samples(dv_audio_t *dv_audio, guint8 *inbuf) {

  dv_aaux_as_t *dv_aaux_as = (dv_aaux_as_t *)(inbuf + 80*6+80*16*3 + 3);

  if(dv_aaux_as->pc0 != 0x50) goto bad_id;
  dv_audio->samples_this_frame = dv_audio_samples_per_frame(dv_aaux_as,dv_audio->frequency);
  return(TRUE);

 bad_id:
  return(FALSE);

} /* dv_update_num_samples */

/* This code originates from cdda2wav, by way of Giovanni Iachello <g.iachello@iol.it>
   to Arne Schirmacher <arne@schirmacher.de>. */
void
dv_audio_deemphasis(dv_audio_t *audio, gint16 *outbuf)
{
  gint i;
  /* this implements an attenuation treble shelving filter 
     to undo the effect of pre-emphasis. The filter is of
     a recursive first order */
  /* static */ short lastin[2] = { 0, 0 };
  /* static */ double lastout[2] = { 0.0, 0.0 };
  short *pmm;
  /* See deemphasis.gnuplot */
  double V0	= 0.3365;
  double OMEGAG = (1./19e-6);
  double T	= (1./audio->frequency);
  double H0	= (V0-1.);
  double B	= (V0*tan((OMEGAG * T)/2.0));
  double a1	= ((B - 1.)/(B + 1.));
  double b0 = (1.0 + (1.0 - a1) * H0/2.0);
  double b1 = (a1 + (a1 - 1.0) * H0/2.0);

  /* For 48Khz: a1= -0.659065 b0= 0.449605 b1= -0.108670 
   * For 44.1Khz: a1=-0.62786881719628784282  b0=	0.45995451989513153057 b1=-0.08782333709141937339
   * For 32kHZ ? */

  for (pmm = (short *)outbuf, i=0; 
       i < audio->samples_this_frame; 
       i++) {
    lastout[0] = *pmm * b0 + lastin[0] * b1 - lastout[0] * a1;
    lastin[0] = *pmm;
    *pmm++ = lastout[0] > 0.0 ? lastout[0] + 0.5 : lastout[0] - 0.5;
  } /* for */

} /* dv_audio_deemphasis */


gint 
dv_decode_audio_block(dv_audio_t *dv_audio, guint8 *inbuf, gint ds, gint audio_dif, gint16 **outbufs) 
{
  gint channel, bp, i_base, i, stride;
  gint16 *samples, *ysamples, *zsamples;
  gint16 y,z;
  gint32 msb_y, msb_z, lsb;
  gint half_ds;

#if 0
  if ((inbuf[0] & 0xe0) != 0x60) goto bad_id;
#endif

  half_ds = (dv_audio->aaux_as.pc3.system ? 6 : 5);

  if(ds < half_ds) {
    channel = 0;
  } else {
    channel = 1;
    ds -= half_ds;
  } /* else */

  if(dv_audio->aaux_as.pc3.system) {
    /* PAL */
    i_base = dv_audio_unshuffle_50[ds][audio_dif];
    stride = 54;
  } else {
    /* NTSC */
    i_base = dv_audio_unshuffle_60[ds][audio_dif];
    stride = 45;
  } /* else */

  if(dv_audio->quantization == 16) {

    samples = outbufs[channel];

    for (bp = 8; bp < 80; bp+=2) {

      i = i_base + (bp - 8)/2 * stride;
      samples[i] = ((gint16)inbuf[bp] << 8) | inbuf[bp+1];

    } /* for */

  } else if(dv_audio->quantization == 12) {

    /* See 61834-2 figure 18, and text of section 6.5.3 and 6.7.2 */

    ysamples = outbufs[channel * 2];
    zsamples = outbufs[1 + channel * 2];

    for (bp = 8; bp < 80; bp+=3) {

      i = i_base + (bp - 8)/3 * stride;

      msb_y = inbuf[bp];
      msb_z = inbuf[bp+1];
      lsb   = inbuf[bp+2];  

      y = ((msb_y << 4) & 0xff0) | ((lsb >> 4) & 0xf);
      if(y > 2047) y -= 4096;
      z = ((msb_z << 4) & 0xff0) | (lsb & 0xf);
      if(z > 2047) z -= 4096;

      ysamples[i] = dv_upsample(y); 
      zsamples[i] = dv_upsample(z);
    } /* for  */

  } else {
    goto unsupported_sampling;
  } /* else */

  return(0);

 unsupported_sampling:
  fprintf(stderr, "libdv(%s):  unsupported audio sampling.\n", __FUNCTION__);
  return(-1);

#if 0
 bad_id:
  fprintf(stderr, "libdv(%s):  not an audio block\n", __FUNCTION__);
  return(-1);
#endif
  
} /* dv_decode_audio_block */
