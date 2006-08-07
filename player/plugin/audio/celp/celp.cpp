/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2000 - 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Massimo Villari        mvillari@cisco.com
 */
#include "celp.h"
#include "bitstreamStruct.h"

#include "dec_lpc.h"
#include "celp_decoder.h"
//#include "../include/audio.h"
//#include "../include/austream.h"

#include <mp4util/mpeg4_audio_config.h>
#include <mp4util/mpeg4_sdp.h>
#include <mp4v2/mp4.h>
#include <mp4av/mp4av.h>

#define DEBUG_SYNC 2
#define bit2byte(a) (((a)+8-1)/8)

const char *celplib="celp";

/*
 * Create CELP Codec class
 */


static codec_data_t *celp_codec_create (const char *stream_type,
					const char *compressor, 
					int type, 
					int profile, 
					format_list_t *media_fmt,
					audio_info_t *audio,
					const uint8_t *userdata,
					uint32_t userdata_size,
					audio_vft_t *vft,
					void *ifptr)

{


  int i;
  celp_codec_t *celp;
  celp = (celp_codec_t *)malloc(sizeof(celp_codec_t));
  memset(celp, 0, sizeof(celp_codec_t));
  
#if 1 	 
  celp->m_vft = vft;
  celp->m_ifptr = ifptr;
  fmtp_parse_t *fmtp = NULL;

  BsInit(0, 0, 0);
  
  // Start setting up CELP stuff...
  
  celp->m_record_sync_time = 1;
  
  celp->m_celp_inited = 0;
  celp->m_audio_inited = 0;
  //celp->m_temp_buff = (float *)malloc(4096);

  
  // Use media_fmt to indicate that we're streaming.
  if (media_fmt != NULL) {
    // haven't checked for null buffer
    // This is not necessarilly right - it is, for the most part, but
    // we should be reading the fmtp statement, and looking at the config.
    // (like we do below in the userdata section...
    celp->m_freq = media_fmt->rtpmap_clock_rate;
    fmtp = parse_fmtp_for_mpeg4(media_fmt->fmt_param, vft->log_msg);
    if (fmtp != NULL) {
      userdata = fmtp->config_binary;
      userdata_size = fmtp->config_binary_len;
    }
  } else {
    if (audio != NULL) {
      celp->m_freq = audio->freq;
    } else {
      celp->m_freq = 44100;
    }
  }
  //celp->m_chans = 1; // this may be wrong - the isma spec, Appendix A.1.1 of
  // Appendix H says the default is 1 channel...
  //celp->m_output_frame_size = 2048;
  // celp->m_object_type = 8;CELP  AACMAIN;
  mpeg4_audio_config_t audio_config;
  if (userdata != NULL || fmtp != NULL) {
    
    celp_message(LOG_DEBUG, celplib, "config len %d %02x %02x %02x %02x", 
		 userdata_size, userdata[0], userdata[1], userdata[2], 
		 userdata[3]);
    decode_mpeg4_audio_config(userdata, userdata_size, &audio_config, false);
    celp->m_object_type = audio_config.audio_object_type;
    celp->m_freq = audio_config.frequency;
    celp->m_chans = audio_config.channels;
  }


	

  // write 
  BsBitBuffer *bitHeader;
  BsBitStream	*hdrStream;


  bitHeader=BsAllocBuffer(userdata_size * 8);

  //wmay removed
  bitHeader->numBit=userdata_size*8;
  bitHeader->size=userdata_size*8;

  memcpy(bitHeader->data,userdata,userdata_size);


  hdrStream = BsOpenBufferRead(bitHeader);

  BsGetSkip (hdrStream,userdata_size*8-audio_config.codec.celp.NumOfBitsInBuffer);
  BsBitBuffer *bBuffer=BsAllocBuffer(userdata_size*8);
  BsGetBuffer (hdrStream, bBuffer,audio_config.codec.celp.NumOfBitsInBuffer);
  int delayNumSample;


  DecLpcInit(celp->m_chans,celp->m_freq,0,NULL,
	     bBuffer ,&celp->m_output_frame_size,&delayNumSample);


  celp->m_samples_per_frame = celp->m_output_frame_size;
  celp->m_msec_per_frame *= TO_U64(1000);
  celp->m_msec_per_frame /= celp->m_freq;
  celp->m_last=userdata_size;
	
  BsFreeBuffer (bitHeader);

  BsFreeBuffer (bBuffer);

	
  celp->m_sampleBuf=(float**)malloc(celp->m_chans*sizeof(float*));
  for(i=0;i<celp->m_chans;i++)
    // wmay - added 2 times - return for frame size was samples, not bytes
    celp->m_sampleBuf[i]=(float*)malloc(2*celp->m_output_frame_size*sizeof(float));

  celp->m_bufs = 
    (uint16_t *)malloc(sizeof(uint16_t) * 2 * celp->m_chans * celp->m_output_frame_size);
  //celp->audiFile = AudioOpenWrite("out1.au",".au",
  //		  celp->m_chans,celp->m_freq);




  celp_message(LOG_INFO, celplib,"CELP object type is %d", celp->m_object_type);
  //celp_message(LOG_INFO, celplib,"CELP channel are %d", celp->m_chans );
  celp_message(LOG_INFO, celplib, "Setting freq to %d", celp->m_freq);
  celp_message(LOG_INFO, celplib, "output frame size is %d", celp->m_output_frame_size);
  
 


#if DUMP_OUTPUT_TO_FILE
  celp->m_outfile = fopen("temp.raw", "w");
#endif
  if (fmtp != NULL) {
    free_fmtp_parse(fmtp);
  }
#endif	 
  celp->m_vft->audio_configure(celp->m_ifptr,
			       celp->m_freq, 
			       celp->m_chans, 
			       AUDIO_FMT_S16,
			       celp->m_output_frame_size);

  return (codec_data_t *)celp;


}

void celp_close (codec_data_t *ptr)
{
  
	
	
  int i;
  if (ptr == NULL) {
    printf("\nin celp close\n");
    
    return;
  }
 
  celp_codec_t *celp = (celp_codec_t *)ptr;
 
		
  if(celp->m_bufs) {
    free(celp->m_bufs);
    celp->m_bufs=NULL;

  }
  //AudioClose(celp->audiFile);
  
	
  // if (celp->m_temp_buff) {
  //    free(celp->m_temp_buff);
  //    celp->m_temp_buff = NULL;
  //  }


  if(celp->m_sampleBuf){
    for(i=0; i<celp->m_chans;i++) {
      free(celp->m_sampleBuf[i]);
      celp->m_sampleBuf[i] = NULL;
    }

    free(celp->m_sampleBuf);
    celp->m_sampleBuf = NULL;
  }

  DecLpcFree();

#if DUMP_OUTPUT_TO_FILE
  fclose(celp->m_outfile);
#endif
  free(celp);
  
		
} 

/*
 * Handle pause - basically re-init the codec
 */
static void celp_do_pause (codec_data_t *ifptr)
{
  

  celp_codec_t *celp = (celp_codec_t *)ifptr;
  celp->m_record_sync_time = 1;
  celp->m_audio_inited = 0;
  celp->m_celp_inited = 0;
	
}

/*
 * Decode task call for CELP
 */
static int celp_decode (codec_data_t *ptr,
			frame_timestamp_t *pts,
			int from_rtp,
			int *sync_frame,
			uint8_t *buffer,
			uint32_t buflen,
			void *userdata)
{
  int usedNumBit;	
  celp_codec_t *celp = (celp_codec_t *)ptr;

  uint32_t freq_ts;

  freq_ts = pts->audio_freq_timestamp;
  if (pts->audio_freq != celp->m_freq) {
    freq_ts = convert_timescale(freq_ts, pts->audio_freq, celp->m_freq);
  }
  if (celp->m_record_sync_time) {
    celp->m_current_frame = 0;
    celp->m_record_sync_time = 0;
    celp->m_current_time = pts->msec_timestamp;
    celp->m_last_rtp_ts = freq_ts;
    celp->m_current_freq_time = freq_ts;
  } else {
    if (celp->m_last_rtp_ts == pts->audio_freq_timestamp) {
      celp->m_current_frame++;
      celp->m_current_time = celp->m_last_rtp_ts;
      celp->m_current_time += 
	celp->m_samples_per_frame * celp->m_current_frame * TO_U64(1000) / 
	celp->m_freq;
      celp->m_current_freq_time += celp->m_samples_per_frame;
    } else {
      celp->m_last_rtp_ts = celp->m_current_freq_time = freq_ts;
      celp->m_current_time = pts->msec_timestamp;
      celp->m_current_frame = 0;
    }

    // Note - here m_current_time should pretty much always be >= rtpts.  
    // If we're not, we most likely want to stop and resync.  We don't
    // need to keep decoding - just decode this frame and indicate we
    // need a resync... That should handle fast forwards...  We need
    // someway to handle reverses - perhaps if we're more than .5 seconds
    // later...
  }

	

  if (celp->m_celp_inited == 0) {
    
	
    /*
     * If not initialized, do so.  
     */
    
    //
    celp->m_celp_inited = 1;
	
  }
  //printf("buflen:%d\n",buflen);

  //if ( ((celp->m_last-buflen)/celp->m_last) < 0.2) return (0);
  if ( buflen<5) return (-1);
  
  BsBitBuffer local;
  local.data= (unsigned char *)buffer;
  local.numBit=buflen*8;
  local.size=buflen*8;
	
  DecLpcFrame(&local,celp->m_sampleBuf,&usedNumBit);
	
  //AudioWriteData(celp->audiFile,celp->m_sampleBuf,celp->m_output_frame_size);
	
  int chan,sample;

  uint8_t *now = celp->m_vft->audio_get_buffer(celp->m_ifptr,
					       celp->m_current_freq_time,
					       celp->m_current_time);
  if (now != NULL) {
    uint16_t *buf = (uint16_t *)now;
    
    for(chan=0;chan<celp->m_chans;chan++){
      for(sample=0;sample < celp->m_output_frame_size; sample++){
	buf[sample +(chan*celp->m_output_frame_size)]=
	  (uint16_t)celp->m_sampleBuf[chan][sample];
	
      }
    }
  }
	
#if DUMP_OUTPUT_TO_FILE
  fwrite(buff, celp->m_output_frame_size * 4, 1, celp->m_outfile);
#endif
  celp->m_vft->audio_filled_buffer(celp->m_ifptr);
      
  return bit2byte(usedNumBit);
}

static const char *celp_compressors[] = {
  "celp ",
  "mp4a",
  "enca",
  NULL
};

static int celp_codec_check (lib_message_func_t message,
			     const char *stream_type,
			     const char *compressor,
			     int type,
			     int profile,
			     format_list_t *fptr, 
			     const uint8_t *userdata,
			     uint32_t userdata_size,
			     CConfigSet *pConfig)
{
  fmtp_parse_t *fmtp = NULL;
  if (strcasecmp(stream_type, STREAM_TYPE_MP4_FILE) == 0 &&
      type != -1) {
    switch (type) {
    case MP4_MPEG4_AUDIO_TYPE:
      break;
    default:
      return -1;
    }
  }
  if (strcasecmp(stream_type, STREAM_TYPE_RTP) == 0 &&
      fptr != NULL && 
      fptr->rtpmap_name != NULL) {
    if (strcasecmp(fptr->rtpmap_name, "mpeg4-generic") != 0) {
      return -1;
    }
    if (userdata == NULL) {
      fmtp = parse_fmtp_for_mpeg4(fptr->fmt_param, message);
      if (fmtp != NULL) {
	userdata = fmtp->config_binary;
	userdata_size = fmtp->config_binary_len;
      }
    }
  }
  if (userdata != NULL) {
    mpeg4_audio_config_t audio_config;
    decode_mpeg4_audio_config(userdata, userdata_size, &audio_config, false);
    if (fmtp != NULL) free_fmtp_parse(fmtp);
    if (audio_object_type_is_celp(&audio_config) == 0) {
      return -1;
    }
    return 1;
  }
  if (compressor != NULL) {
    const char **lptr = celp_compressors;
    while (*lptr != NULL) {
      if (strcasecmp(*lptr, compressor) == 0) {
	return 1;
      }
      lptr++;
    }
  }
  return -1;
}

AUDIO_CODEC_PLUGIN("celp",
		   celp_codec_create,
		   celp_do_pause,
		   celp_decode,
		   NULL,
		   celp_close,
		   celp_codec_check,
		   NULL, 
		   0);
/* end file aa.cpp */


