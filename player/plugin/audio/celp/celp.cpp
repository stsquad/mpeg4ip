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
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
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

#define DEBUG_SYNC 2
#define bit2byte(a) (((a)+8-1)/8)

const char *celplib="celp";

/*
 * Create CELP Codec class
 */


static codec_data_t *celp_codec_create (format_list_t *media_fmt,
				       audio_info_t *audio,
				       const unsigned char *userdata,
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
  
  // Start setting up CELP stuff...
  celp->m_bStream= (BsBitStream *) malloc(sizeof(BsBitStream));
  celp->m_bStream->buffer[0]=(BsBitBuffer*)malloc(sizeof(BsBitBuffer));
  
  
  celp->m_bBuffer=BsAllocBuffer(userdata_size);
  
  celp->m_resync_with_header = 1;
  celp->m_record_sync_time = 1;
  
  celp->m_celp_inited = 0;
  celp->m_audio_inited = 0;
  celp->m_bufs = (uint16_t *)malloc(4096);
  //celp->m_temp_buff = (float *)malloc(4096);

  
  // Use media_fmt to indicate that we're streaming.
  if (media_fmt != NULL) {
    // haven't checked for null buffer
    // This is not necessarilly right - it is, for the most part, but
    // we should be reading the fmtp statement, and looking at the config.
    // (like we do below in the userdata section...
    celp->m_freq = media_fmt->rtpmap->clock_rate;
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
    
	
    decode_mpeg4_audio_config(userdata, userdata_size, &audio_config);
    celp->m_object_type = audio_config.audio_object_type;
    celp->m_freq = audio_config.frequency;
    celp->m_chans = audio_config.channels;
    
  }


	

// write 
BsBitBuffer *bitHeader;
BsBitStream	*hdrStream;


bitHeader=BsAllocBuffer(userdata_size*8);


bitHeader->numBit=userdata_size*8;
bitHeader->size=userdata_size*8;

memcpy(  bitHeader->data,userdata,userdata_size*8);


hdrStream = BsOpenBufferRead(bitHeader);

BsGetSkip (hdrStream,userdata_size*8-audio_config.codec.celp.NumOfBitsInBuffer);
celp->m_bBuffer=BsAllocBuffer(userdata_size*8);
BsGetBuffer (hdrStream, celp->m_bBuffer,audio_config.codec.celp.NumOfBitsInBuffer);
int delayNumSample;


DecLpcInit(celp->m_chans,celp->m_freq,0,NULL,
				celp->m_bBuffer ,&celp->m_output_frame_size,&delayNumSample);



celp->m_msec_per_frame *= M_LLU;
celp->m_msec_per_frame /= celp->m_freq;
celp->m_last=userdata_size;
	
BsFreeBuffer (bitHeader);

BsFreeBuffer (celp->m_bBuffer);

	
celp->m_sampleBuf=(float**)malloc(celp->m_chans*sizeof(float*));
	for(i=0;i<celp->m_chans;i++)
		celp->m_sampleBuf[i]=(float*)malloc(celp->m_output_frame_size*sizeof(float));

//celp->audiFile = AudioOpenWrite("out1.au",".au",
//		  celp->m_chans,celp->m_freq);




  celp_message(LOG_INFO, celplib,"CELP object type is %d", celp->m_object_type);
  //celp_message(LOG_INFO, celplib,"CELP channel are %d", celp->m_chans );
  celp_message(LOG_INFO, celplib, "Setting freq to %d", celp->m_freq);
  
 


#if DUMP_OUTPUT_TO_FILE
  celp->m_outfile = fopen("temp.raw", "w");
#endif
  if (fmtp != NULL) {
    free_fmtp_parse(fmtp);
  }
#endif	 
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
		for(i=0; i<celp->m_chans;i++) free(celp->m_sampleBuf[i]);
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
  celp->m_resync_with_header = 1;
  celp->m_record_sync_time = 1;
  celp->m_audio_inited = 0;
  celp->m_celp_inited = 0;
  if (celp->m_bufs == NULL) 
    celp->m_bufs = (uint16_t*)malloc(4096);
	
}

/*
 * Decode task call for CELP
 */
static int celp_decode (codec_data_t *ptr,
		       uint64_t ts,
		       int from_rtp,
		       int *sync_frame,
		       unsigned char *buffer,
		       uint32_t buflen)
{
 
	
  
#if 1	
  int usedNumBit;	
  celp_codec_t *celp = (celp_codec_t *)ptr;
  
  //  struct timezone tz;

  if (celp->m_record_sync_time) {
    celp->m_current_frame = 0;
    celp->m_record_sync_time = 0;
    celp->m_current_time = ts;
    celp->m_last_rtp_ts = ts;
  } else {
    if (celp->m_last_rtp_ts == ts) {
      celp->m_current_time += celp->m_msec_per_frame;
      celp->m_current_frame++;
    } else {
      celp->m_last_rtp_ts = ts;
      celp->m_current_time = ts;
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
    
	
    celp->m_bBuffer=BsAllocBuffer(buflen*8);
	
	
	/*
       * If not initialized, do so.  
     */
    
	//
	celp->m_celp_inited = 1;
	
	}
	

	//printf("buflen:%d\n",buflen);

	/*
    unsigned char *buff;
	
    /* 
     * Get an audio buffer
     * /
	
    if (celp->m_audio_inited == 0) {
      buff = celp->m_temp_buff;
    } else {
      buff = celp->m_vft->audio_get_buffer(celp->m_ifptr);
    }
    if (buff == NULL) {
      //player_debug_message("Can't get buffer in celp");
      return (0);
    }

    */
	
	
	int dim=celp->m_output_frame_size;

	//if ( ((celp->m_last-buflen)/celp->m_last) < 0.2) return (0);
	if ( buflen<23) return (0);

	celp->m_bBuffer->data=buffer;
	celp->m_bBuffer->numBit=buflen*8;
	celp->m_bBuffer->size=buflen*8;
	
	
	
	DecLpcFrame(celp->m_bBuffer,celp->m_sampleBuf,&usedNumBit);
	
	//AudioWriteData(celp->audiFile,celp->m_sampleBuf,celp->m_output_frame_size);
	
	int tempchans = celp->m_chans;	  

	
	
	int k,j;
	
	
	
	for(j=0;j<celp->m_chans;j++){
	for(k=0;k<celp->m_output_frame_size;k++){
			
		    
			celp->m_bufs[k+(j*celp->m_output_frame_size)]=(uint16_t)celp->m_sampleBuf[j][k];
			
	}
			
	
	}
	
	

	celp->m_vft->audio_configure(celp->m_ifptr,
				     celp->m_freq, 
				     celp->m_chans, 
				     AUDIO_S16SYS,// AUDIO_S8,//
				     celp->m_output_frame_size);

	unsigned char *now = celp->m_vft->audio_get_buffer(celp->m_ifptr);
	if (now != NULL) {
	  memcpy(now, celp->m_bufs , tempchans * (dim) * sizeof(int16_t));
	}
	
	celp->m_audio_inited = 1;
	


	
      /*
       * good result - give it to audio sync class
       */
	  //free(buff);

	
#if DUMP_OUTPUT_TO_FILE
      fwrite(buff, celp->m_output_frame_size * 4, 1, celp->m_outfile);
#endif
      celp->m_vft->audio_filled_buffer(celp->m_ifptr,
				      celp->m_current_time, 
				      celp->m_resync_with_header);
      if (celp->m_resync_with_header == 1) {
	celp->m_resync_with_header = 0;
#ifdef DEBUG_SYNC
	celp_message(LOG_DEBUG, celplib, "Back to good at "LLU, celp->m_current_time);
#endif
      }
      




	return bit2byte(usedNumBit);


#else 
	return 2; 
#endif
}

static const char *celp_compressors[] = {
  "celp ",
  "mp4a",
  NULL
};

static int celp_codec_check (lib_message_func_t message,
			    const char *compressor,
			    int type,
			    int profile,
			    format_list_t *fptr, 
			    const unsigned char *userdata,
			    uint32_t userdata_size)
{
  fmtp_parse_t *fmtp = NULL;
  if (compressor != NULL && 
      strcasecmp(compressor, "MP4 FILE") == 0 &&
      type != -1) {
    switch (type) {
    case MP4_MPEG4_AUDIO_TYPE:
      break;
    default:
      return -1;
    }
  }
  if (fptr != NULL && 
      fptr->rtpmap != NULL &&
      fptr->rtpmap->encode_name != NULL) {
    if (strcasecmp(fptr->rtpmap->encode_name, "mpeg4-generic") != 0) {
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
    decode_mpeg4_audio_config(userdata, userdata_size, &audio_config);
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

AUDIO_CODEC_WITH_RAW_FILE_PLUGIN("celp",
				 celp_codec_create,
				 celp_do_pause,
				 celp_decode,
				 celp_close,
				 celp_codec_check,
				 celp_file_check,
				 celp_file_next_frame,
				 celp_file_used_for_frame,
				 celp_raw_file_seek_to,
				 celp_file_eof);
/* end file aa.cpp */


