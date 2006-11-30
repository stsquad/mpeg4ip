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
 *              Bill May        wmay@cisco.com
 */
/*
 * qtime_file.cpp - provides generic class for quicktime file access control.
 * file access is then used by quicktime audio and video bytestreams.
 */
#include "mpeg4ip.h"
#include "player_session.h"
#include "player_media.h"
#include "player_util.h"
#include "media_utils.h"
#include <mp4.h>
#include "mp4_bytestream.h"
#include "mp4_file.h"
#include <mp4util/mpeg4_audio_config.h>
#include "our_config_file.h"
#include "codec_plugin_private.h"
#include <ismacryplib.h>

/*
 * Create the media for the quicktime file, and set up some session stuff.
 */
static void close_mp4_file (void *data)
{
  CMp4File *Mp4File1 = (CMp4File *)data;
  if (Mp4File1 != NULL) {
    delete Mp4File1;
    Mp4File1 = NULL;
  }
}

int create_media_for_mp4_file (CPlayerSession *psptr, 
			       const char *name,
			       int have_audio_driver,
			       control_callback_vft_t *cc_vft)
{
  MP4FileHandle fh;
  CMp4File *Mp4File1;

  fh = MP4Read(name, MP4_DETAILS_ERROR); // | MP4_DETAILS_READ | MP4_DETAILS_SAMPLE);
  if (!MP4_IS_VALID_FILE_HANDLE(fh)) {
    psptr->set_message("`%s\' is not an mp4 file", name);
    return -1;
  }

  Mp4File1 = new CMp4File(fh);
  // quicktime is searchable...
  psptr->set_media_close_callback(close_mp4_file, (void *)Mp4File1);
  psptr->session_set_seekable(1);

  int ret;
  ret = Mp4File1->create_media(psptr, 
			       have_audio_driver,
			       cc_vft);
  if (ret <= 0) return ret;

  uint offset = 0;

  char errmsg[512];
  uint32_t errlen = sizeof(errmsg) - 1;
  errmsg[0] = '\0';
  if (Mp4File1->get_illegal_video_codec() != 0) {
    offset = snprintf(errmsg, errlen, "Unknown or unused Video tracks ");
  }

  if (have_audio_driver == 0) {
    offset += snprintf(errmsg + offset, errlen - offset, 
		       "%sNo Audio driver - can't play audio",
		       offset == 0 ? "" : "and ");
  } else if (Mp4File1->get_illegal_audio_codec() != 0) {
    snprintf(errmsg + offset, errlen - offset, 
	     "%sUnknown or unused audio tracks", 
	     offset == 0 ? "" : "and ");
  }
  psptr->set_message(errmsg);
  return (1);
}

CMp4File::CMp4File (MP4FileHandle filehandle)
{
  m_mp4file = filehandle;
  m_file_mutex = SDL_CreateMutex();
  m_illegal_audio_codec = 0;
  m_illegal_video_codec = 0;
  m_illegal_text_codec = 0;
  m_have_audio = false;
}

CMp4File::~CMp4File (void)
{
  MP4Close(m_mp4file);
  m_mp4file = NULL;
  if (m_file_mutex) {
    SDL_DestroyMutex(m_file_mutex);
    m_file_mutex = NULL;
  }
}

int CMp4File::create_video(CPlayerSession *psptr, 
			   video_query_t *vq, 
			   uint video_offset,
			   uint &start_desc)
{
  uint ix;
  CPlayerMedia *mptr;
  codec_plugin_t *plugin;
  const char *file_kms_uri;
  const char *inuse_kms_uri;

  for (ix = 0; ix < video_offset; ix++) {
    if (vq[ix].enabled != 0) {

      mptr = new CPlayerMedia(psptr, VIDEO_SYNC);
      if (mptr == NULL) {
	return (-1);
      }
						  
      video_info_t *vinfo;
      vinfo = (video_info_t *)malloc(sizeof(video_info_t));
      vinfo->height = vq[ix].h;
      vinfo->width = vq[ix].w;
      plugin = check_for_video_codec(vq[ix].stream_type,
				     vq[ix].compressor,
				     NULL,
				     vq[ix].type,
				     vq[ix].profile,
				     vq[ix].config, 
				     vq[ix].config_len,
				     &config);

      int ret = mptr->create_video_plugin(plugin, 
					  vq[ix].stream_type,
					  vq[ix].compressor,
					  vq[ix].type,
					  vq[ix].profile,
					  NULL, // sdp info
					  vinfo, // video info
					  vq[ix].config,
					  vq[ix].config_len);

      if (ret < 0) {
	mp4f_message(LOG_ERR, "Failed to create plugin data");
	psptr->set_message("Failed to start plugin");
	delete mptr;
	return -1;
      }

      CMp4VideoByteStream *vbyte = NULL;
      uint64_t IVLength;
     
      /* check if clear-text or ismacryp.
       * in this context original format is encv 
       * and compressor specifies which codec
       */
      uint32_t verb = MP4GetVerbosity(m_mp4file);
      MP4SetVerbosity(m_mp4file, verb & ~(MP4_DETAILS_ERROR));
      if (strcasecmp(vq[ix].original_fmt, "avc1") == 0) {
	vbyte = new CMp4H264VideoByteStream(this, vq[ix].track_id);
      } else if (strcasecmp(vq[ix].original_fmt, "encv") == 0) {
        MP4GetTrackIntegerProperty(m_mp4file,
				   vq[ix].track_id, 
				   "mdia.minf.stbl.stsd.encv.sinf.schi.iSFM.IV-length", 
				   &IVLength);

      	if (strcasecmp(vq[ix].compressor, "mp4v") == 0) {
	vbyte = new CMp4EncVideoByteStream(this, vq[ix].track_id,IVLength);
	}
      	else if (((strcasecmp(vq[ix].compressor, "avc1") == 0) 
      	|| (strcasecmp(vq[ix].compressor, "264b")) == 0)) {
		vbyte = new CMp4EncH264VideoByteStream(this, vq[ix].track_id,IVLength);

		// check if file kms uri matches in-use kms uri
		// advisory only 
		MP4GetTrackStringProperty(m_mp4file, vq[ix].track_id, 
			"mdia.minf.stbl.stsd.encv.sinf.schi.iKMS.kms_URI", 
			&file_kms_uri);
		inuse_kms_uri = 
			((CMp4EncH264VideoByteStream *)vbyte)->get_inuse_kms_uri();
		if (file_kms_uri && inuse_kms_uri) {
			if (strcmp(file_kms_uri, inuse_kms_uri)) {
				mp4f_message(LOG_DEBUG, 
				"KMS in file (%s) does not match KMS in use (%s)\n", 
				file_kms_uri, inuse_kms_uri);
      		}
		}
	}
      } else {
	vbyte = new CMp4VideoByteStream(this, vq[ix].track_id);
      }
      MP4SetVerbosity(m_mp4file, verb);

      if (vbyte == NULL) {
	delete mptr;
	return (-1);
      }

      ret = mptr->create_media("video", vbyte);
      if (ret != 0) {
	return (-1);
      }
      MP4SetVerbosity(m_mp4file, verb & ~(MP4_DETAILS_ERROR));
      char *mp4info = MP4Info(m_mp4file, vq[ix].track_id);
      MP4SetVerbosity(m_mp4file, verb);
      char *temp = mp4info;
      while (*temp != '\0') {
	if (isspace(*temp)) *temp = ' ';
	if (!isprint(*temp)) *temp = '*';
	temp++;
      }
      psptr->set_session_desc(start_desc, mp4info);
      free(mp4info);
      start_desc++;
    } else {
      if (vq[ix].config != NULL) free((void *)vq[ix].config);
    }
  }
  return 0;
}

int CMp4File::create_audio(CPlayerSession *psptr, 
			   audio_query_t *aq, 
			   uint audio_offset,
			   uint &start_desc)
{
  uint ix;
  uint64_t IVLength;
  CPlayerMedia *mptr;
  codec_plugin_t *plugin;
  for (ix = 0; ix < audio_offset; ix++) {
    if (aq[ix].enabled != 0) {
      CMp4AudioByteStream *abyte;
      mptr = new CPlayerMedia(psptr, AUDIO_SYNC);
      if (mptr == NULL) {
	return (-1);
      }

      /* check if ismacryp */
      uint32_t verb = MP4GetVerbosity(m_mp4file);
      MP4SetVerbosity(m_mp4file, verb & ~(MP4_DETAILS_ERROR));
      if (MP4IsIsmaCrypMediaTrack(m_mp4file, aq[ix].track_id)) {
        MP4GetTrackIntegerProperty(m_mp4file,
                    aq[ix].track_id, "mdia.minf.stbl.stsd.enca.sinf.schi.iSFM.IV-length", &IVLength);
	abyte = new CMp4EncAudioByteStream(this, aq[ix].track_id, IVLength);
      } else {
	abyte = new CMp4AudioByteStream(this, aq[ix].track_id);
      }
      MP4SetVerbosity(m_mp4file, verb);

      audio_info_t *ainfo;
      ainfo = (audio_info_t *)malloc(sizeof(audio_info_t));
      memset(ainfo, 0, sizeof(*ainfo));

      ainfo->freq = aq[ix].sampling_freq;
      if (aq[ix].chans != -1) {
	ainfo->chans = aq[ix].chans;
      }
      if ((aq[ix].type == MP4_PCM16_LITTLE_ENDIAN_AUDIO_TYPE) ||
	  (aq[ix].type == MP4_PCM16_BIG_ENDIAN_AUDIO_TYPE)) {
	ainfo->bitspersample = 16;
      }

      int ret;
      plugin = check_for_audio_codec(STREAM_TYPE_MP4_FILE,
				     aq[ix].compressor, // media_data field
				     NULL,
				     aq[ix].type,
				     aq[ix].profile,
				     aq[ix].config,
				     aq[ix].config_len,
				     &config);

      ret = mptr->create_audio_plugin(plugin,
				      STREAM_TYPE_MP4_FILE,
				      aq[ix].compressor,
				      aq[ix].type, 
				      aq[ix].profile,
				      NULL, // sdp info
				      ainfo, // audio info
				      aq[ix].config,
				      aq[ix].config_len);
      if (ret < 0) {
	mp4f_message(LOG_ERR, "Couldn't create audio from plugin %s", 
		     plugin->c_name);
	psptr->set_message("Couldn't start audio plugin %s", 
			   plugin->c_name);
	delete mptr;
	delete abyte;
	return -1;
      }

      ret = mptr->create_media("audio", abyte);
      if (ret != 0) {
	return (-1);
      }
      MP4SetVerbosity(m_mp4file, verb & ~(MP4_DETAILS_ERROR));
      char *mp4info = MP4Info(m_mp4file, aq[ix].track_id);
      MP4SetVerbosity(m_mp4file, verb);
      char *temp = mp4info;
      while (*temp != '\0') {
	if (isspace(*temp)) *temp = ' ';
	if (!isprint(*temp)) *temp = '*';
	temp++;
      }
      psptr->set_session_desc(start_desc, mp4info);
      free(mp4info);
      start_desc++;
    } else {
      if (aq[ix].config != NULL) free((void *)aq[ix].config);
    }
  }

  return 0;
}

int CMp4File::create_text(CPlayerSession *psptr, 
			   text_query_t *tq, 
			   uint text_offset,
			   uint &start_desc)
{
  uint ix;
  //uint64_t IVLength;
  CPlayerMedia *mptr;
  codec_plugin_t *plugin;
  uint32_t verb = MP4GetVerbosity(m_mp4file);
  for (ix = 0; ix < text_offset; ix++) {
    if (tq[ix].enabled != 0) {
      CMp4TextByteStream *tbyte;
      mptr = new CPlayerMedia(psptr, TIMED_TEXT_SYNC);
      if (mptr == NULL) {
	return (-1);
      }

      /* check if ismacryp */
#if 0
      uint32_t verb = MP4GetVerbosity(m_mp4file);
      MP4SetVerbosity(m_mp4file, verb & ~(MP4_DETAILS_ERROR));
      if (MP4IsIsmaCrypMediaTrack(m_mp4file, aq[ix].track_id)) {
        IVLength = MP4GetTrackIntegerProperty(m_mp4file,
                    aq[ix].track_id, "mdia.minf.stbl.stsd.enca.sinf.schi.iSFM.IV-length");
	abyte = new CMp4EncAudioByteStream(this, aq[ix].track_id, IVLength);
      } else {
	abyte = new CMp4AudioByteStream(this, aq[ix].track_id);
      }
      MP4SetVerbosity(m_mp4file, verb);
#else
      tbyte = new CMp4TextByteStream(this, tq[ix].track_id,
				     MP4GetHrefTrackBaseUrl(m_mp4file,
							    tq[ix].track_id));
#endif

      int ret;
      plugin = check_for_text_codec(tq[ix].stream_type,
				    tq[ix].compressor,
				    NULL,
				    NULL,
				    0, 
				    &config);

      ret = mptr->create_text_plugin(plugin,
				     STREAM_TYPE_MP4_FILE,
				     tq[ix].compressor,
				     NULL, // sdp info
				     NULL, 
				     0);
      if (ret < 0) {
	mp4f_message(LOG_ERR, "Couldn't create text from plugin %s", 
		     plugin->c_name);
	psptr->set_message("Couldn't start text plugin %s", 
			   plugin->c_name);
	delete mptr;
	delete tbyte;
	return -1;
      }

      ret = mptr->create_media("text", tbyte);
      if (ret != 0) {
	return (-1);
      }
      MP4SetVerbosity(m_mp4file, verb & ~(MP4_DETAILS_ERROR));
      char *mp4info = MP4Info(m_mp4file, tq[ix].track_id);
      MP4SetVerbosity(m_mp4file, verb);
      char *temp = mp4info;
      while (*temp != '\0') {
	if (isspace(*temp)) *temp = ' ';
	if (!isprint(*temp)) *temp = '*';
	temp++;
      }
      psptr->set_session_desc(start_desc, mp4info);
      free(mp4info);
      start_desc++;
    } else {
      CHECK_AND_FREE(tq[ix].config);
    }
  }

  return 0;
}


int CMp4File::create_media (CPlayerSession *psptr,
			    int have_audio_driver,
			    control_callback_vft_t *cc_vft)
{
  uint video_count, video_offset;
  uint text_count, text_offset;
  uint audio_count, audio_offset;
  MP4TrackId trackId;
  video_query_t *vq;
  audio_query_t *aq;
  text_query_t *tq;
  uint ix;
  codec_plugin_t *plugin;
  int ret_value = 0;
  uint8_t *foo;
  u_int32_t bufsize;
  char original_fmt[8];
  
  uint32_t verb = MP4GetVerbosity(m_mp4file);
  MP4SetVerbosity(m_mp4file, verb & ~(MP4_DETAILS_ERROR));
  video_count = MP4GetNumberOfTracks(m_mp4file, MP4_VIDEO_TRACK_TYPE);
  audio_count = MP4GetNumberOfTracks(m_mp4file, MP4_AUDIO_TRACK_TYPE);
  text_count = MP4GetNumberOfTracks(m_mp4file, MP4_CNTL_TRACK_TYPE);
  mp4f_message(LOG_DEBUG, "cntl tracks %u", text_count);
  MP4SetVerbosity(m_mp4file, verb);

  if (video_count == 0 && audio_count == 0 && text_count == 0) {
    psptr->set_message("No audio, video or control tracks in file");
    return -1;
  }

  if (video_count > 0) {
    vq = (video_query_t *)malloc(sizeof(video_query_t) * video_count);
    memset(vq, 0, sizeof(video_query_t) * video_count);
  } else {
    vq = NULL;
  }
  if (have_audio_driver && audio_count > 0) {
    aq = (audio_query_t *)malloc(sizeof(audio_query_t) * audio_count);
    memset(aq, 0, sizeof(audio_query_t) * audio_count);
  } else {
    aq = NULL;
  }

  if (text_count > 0) {
    tq = (text_query_t *)malloc(sizeof(text_query_t) * text_count);
    memset(tq, 0, sizeof(text_query_t) * text_count);
  } else {
    tq = NULL;
  }
  for (ix = 0, video_offset = 0; ix < video_count; ix++) {
    trackId = MP4FindTrackId(m_mp4file, ix, MP4_VIDEO_TRACK_TYPE);
    const char *media_data_name;
    media_data_name = MP4GetTrackMediaDataName(m_mp4file, trackId);
    mp4f_message(LOG_DEBUG, "MP4 - got video track 4cc %s", 
	media_data_name);
    vq[video_offset].track_id = trackId;
    vq[video_offset].stream_type = STREAM_TYPE_MP4_FILE;
    vq[video_offset].compressor = media_data_name;
    vq[video_offset].original_fmt = media_data_name;

    if (strcasecmp(media_data_name, "encv") == 0) {
    	MP4GetTrackMediaDataOriginalFormat(m_mp4file, trackId, 
		original_fmt, sizeof(original_fmt));
    	mp4f_message(LOG_DEBUG, "MP4 - got video track original format 4cc %s", 
		original_fmt);
	// ok to do this since both vq and original_fmt are volatile in this scope
    	vq[video_offset].compressor = original_fmt;
	// in this case, the original_fmt is going to be encv, the
	// compressor avc1 or mp4v
    }

    if (strcasecmp(vq[video_offset].compressor, "mp4v") == 0) { 
      uint8_t video_type = MP4GetTrackEsdsObjectTypeId(m_mp4file, trackId);
      uint8_t profileID = MP4GetVideoProfileLevel(m_mp4file, trackId);
      mp4f_message(LOG_DEBUG, "MP4 - got track %x profile ID %d", 
		 trackId, profileID);
      MP4SetVerbosity(m_mp4file, verb & ~(MP4_DETAILS_ERROR));
      MP4GetTrackESConfiguration(m_mp4file, trackId, &foo, &bufsize);
      MP4SetVerbosity(m_mp4file, verb);
      vq[video_offset].type = video_type;
      vq[video_offset].profile = profileID;
      vq[video_offset].fptr = NULL;
      vq[video_offset].config = foo;
      vq[video_offset].config_len = bufsize;
    } 
    // avc1 is unaltered h264, 264b is original format for ismacrypted avc1
    else if ((strcasecmp(vq[video_offset].compressor, "avc1") == 0) || 
    	(strcasecmp(vq[video_offset].compressor, "264b") == 0)) { 
      uint8_t profile, level;
      uint8_t **seqheader, **pictheader;
      uint32_t *pictheadersize, *seqheadersize;
      uint32_t ix;


      MP4GetTrackH264ProfileLevel(m_mp4file, trackId, &profile, &level);
      MP4GetTrackH264SeqPictHeaders(m_mp4file, trackId, 
				    &seqheader, &seqheadersize,
				    &pictheader, &pictheadersize);
      bufsize = 0;
      for (ix = 0; seqheadersize[ix] != 0; ix++) {
	bufsize += seqheadersize[ix] + 4;
      }
      for (ix = 0; pictheadersize[ix] != 0; ix++) {
	bufsize += pictheadersize[ix] + 4;
      }
      foo = (uint8_t *)malloc(bufsize + 4);
      memset(foo, 0, bufsize + 4);
      uint32_t copied = 0;
      // headers do not have the byte stream start code stored in the file
      for (ix = 0; seqheadersize[ix] != 0; ix++) {
	foo[copied] = 0;
	foo[copied + 1] = 0;
	foo[copied + 2] = 0;
	foo[copied + 3] = 1;
	copied += 4; // add header
	memcpy(foo + copied, 
	       seqheader[ix], 
	       seqheadersize[ix]);
	copied += seqheadersize[ix];
	free(seqheader[ix]);
      }
      free(seqheader);
      free(seqheadersize);
      for (ix = 0; pictheadersize[ix] != 0; ix++) {
	foo[copied] = 0;
	foo[copied + 1] = 0;
	foo[copied + 2] = 0;
	foo[copied + 3] = 1;
	copied += 4; // add header
	memcpy(foo + copied, 
	       pictheader[ix], 
	       pictheadersize[ix]);
	copied += pictheadersize[ix];
	free(pictheader[ix]);
      }
      free(pictheader);
      free(pictheadersize);
	
      vq[video_offset].type = level;
      vq[video_offset].profile = profile;
      vq[video_offset].fptr = NULL;
      vq[video_offset].config = foo;
      vq[video_offset].config_len = bufsize;


    } else {
      MP4GetTrackVideoMetadata(m_mp4file, trackId, &foo, &bufsize);
      vq[video_offset].config = foo;
      vq[video_offset].config_len = bufsize;
    }

    plugin = check_for_video_codec(vq[video_offset].stream_type,
				   vq[video_offset].compressor,
				   NULL,
				   vq[video_offset].type,
				   vq[video_offset].profile,
				   vq[video_offset].config,
				   vq[video_offset].config_len,
				   &config);
    if (plugin == NULL) {
      psptr->set_message("Can't find plugin for video %s (%s) type %d, profile %d",
			 vq[video_offset].compressor,
			 vq[video_offset].original_fmt,
			 vq[video_offset].type, 
			 vq[video_offset].profile);
      m_illegal_video_codec++;
      ret_value = 1;
      // possibly memleak for foo here
    } else {
      vq[video_offset].h = MP4GetTrackVideoHeight(m_mp4file, trackId);
      vq[video_offset].w = MP4GetTrackVideoWidth(m_mp4file, trackId);
      vq[video_offset].frame_rate = MP4GetTrackVideoFrameRate(m_mp4file, trackId);
      vq[video_offset].enabled = 0;
      vq[video_offset].reference = NULL;
      video_offset++;
    }
  }

  audio_offset = 0;
  if (have_audio_driver) {
    for (ix = 0; ix < audio_count; ix++) {
      trackId = MP4FindTrackId(m_mp4file, ix, MP4_AUDIO_TRACK_TYPE);
      const char *media_data_name;
      media_data_name = MP4GetTrackMediaDataName(m_mp4file, trackId);

      aq[audio_offset].track_id = trackId;
      aq[audio_offset].stream_type = STREAM_TYPE_MP4_FILE;
      aq[audio_offset].compressor = media_data_name;
      if (strcasecmp(media_data_name, "mp4a") == 0 ||
	  strcasecmp(media_data_name, "enca") == 0) {
	uint8_t *userdata = NULL;
	u_int32_t userdata_size;
	aq[audio_offset].type = MP4GetTrackEsdsObjectTypeId(m_mp4file, trackId);
	MP4SetVerbosity(m_mp4file, verb & ~(MP4_DETAILS_ERROR));
	aq[audio_offset].profile = MP4GetAudioProfileLevel(m_mp4file);
	MP4GetTrackESConfiguration(m_mp4file, 
				   trackId, 
				   &userdata, 
				   &userdata_size);
	MP4SetVerbosity(m_mp4file, verb);
	aq[audio_offset].config = userdata;
	aq[audio_offset].config_len = userdata_size;
      }
      plugin = check_for_audio_codec(aq[audio_offset].stream_type,
				     aq[audio_offset].compressor,
				     NULL,
				     aq[audio_offset].type,
				     aq[audio_offset].profile,
				     aq[audio_offset].config,
				     aq[audio_offset].config_len,
				     &config);
      if (plugin != NULL) {
	aq[audio_offset].fptr = NULL;
	aq[audio_offset].sampling_freq = 
	  MP4GetTrackTimeScale(m_mp4file, trackId);
	MP4SetVerbosity(m_mp4file, verb & ~(MP4_DETAILS_ERROR));
	aq[audio_offset].chans = MP4GetTrackAudioChannels(m_mp4file, trackId);
	MP4SetVerbosity(m_mp4file, verb);
	aq[audio_offset].enabled = 0;
	aq[audio_offset].reference = NULL;
	audio_offset++;
	m_have_audio = true;
      } else {
	m_illegal_audio_codec++;
	ret_value = 1;
      }
    }
  } else {
    if (audio_count)
      ret_value = 1;
  }
  text_offset = 0;
  for (ix = 0; ix < text_count; ix++) {
    trackId = MP4FindTrackId(m_mp4file, ix, MP4_CNTL_TRACK_TYPE);
    const char *media_data_name;
    media_data_name = MP4GetTrackMediaDataName(m_mp4file, trackId);

    tq[text_offset].track_id = trackId;
    tq[text_offset].stream_type = STREAM_TYPE_MP4_FILE;
    tq[text_offset].compressor = media_data_name;
    plugin = check_for_text_codec(tq[text_offset].stream_type,
				  tq[text_offset].compressor,
				  NULL,
				  NULL,
				  0, 
				  &config);
    if (plugin != NULL) {
      tq[text_offset].fptr = NULL;
      tq[text_offset].enabled = 0;
      tq[text_offset].reference = NULL;
      text_offset++;
    } else {
      m_illegal_text_codec++;
      ret_value = 1;
    }
  }

  if (video_offset == 0 && audio_offset == 0 && text_offset == 0) {
    psptr->set_message("No playable codecs in mp4 file");
    return -1;
  }
  if (cc_vft && cc_vft->media_list_query != NULL) {
    (cc_vft->media_list_query)(psptr, video_offset, vq, audio_offset, aq, text_offset, tq);
  } else {
    if (video_offset > 0) {
      vq[0].enabled = 1;
    }
    if (audio_offset > 0) {
      aq[0].enabled = 1;
    }
    if (text_offset > 0) {
      tq[0].enabled = 1;
    }
  }

  int vidret, audret, textret;
  uint start_desc = 1;
  vidret = create_video(psptr, vq, video_offset, start_desc);
  free(vq);

  if (vidret < 0) {
    free(aq);
    free(tq);
    return -1;
  }
 
  audret = create_audio(psptr, aq, audio_offset, start_desc);
  free(aq);

  textret = create_text(psptr, tq, text_offset, start_desc);
  free(tq);

  if (audret < 0 || textret < 0) ret_value = -1;

  char *name;
  verb = MP4GetVerbosity(m_mp4file);
  MP4SetVerbosity(m_mp4file, verb & ~(MP4_DETAILS_ERROR));
  if (MP4GetMetadataName(m_mp4file, &name) &&
      name != NULL) {
    psptr->set_session_desc(0, name);
    free(name);
  }
  MP4SetVerbosity(m_mp4file, verb);
  
  return (ret_value);
}

/* end file mp4_file.cpp */
