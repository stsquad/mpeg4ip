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
#ifdef ISMACRYP
#include <ismacryplib.h>
#endif

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
			       char *errmsg,
			       uint32_t errlen,
			       int have_audio_driver,
			       control_callback_vft_t *cc_vft)
{
  MP4FileHandle fh;
  CMp4File *Mp4File1;

  fh = MP4Read(name, MP4_DETAILS_ERROR);
  if (!MP4_IS_VALID_FILE_HANDLE(fh)) {
    snprintf(errmsg, errlen, "`%s\' is not an mp4 file", name);
    return -1;
  }

  Mp4File1 = new CMp4File(fh);
  // quicktime is searchable...
  psptr->set_media_close_callback(close_mp4_file, (void *)Mp4File1);
  psptr->session_set_seekable(1);

  int ret;
  ret = Mp4File1->create_media(psptr, 
				 errmsg, 
				 errlen, 
				 have_audio_driver,
				 cc_vft);
  if (ret <= 0) return ret;

  int offset = 0;

  if (Mp4File1->get_illegal_video_codec() != 0) {
    offset = snprintf(errmsg, errlen, "Unknown or unused Video tracks ");
  }

  if (Mp4File1->have_audio() != 0 && have_audio_driver == 0) {
    offset += snprintf(errmsg + offset, errlen - offset, 
		       "%sNo Audio driver - can't play audio",
		       offset == 0 ? "" : "and ");
  } else if (Mp4File1->get_illegal_audio_codec() != 0) {
    snprintf(errmsg + offset, errlen - offset, 
	     "%sUnknown or unused audio tracks", 
	     offset == 0 ? "" : "and ");
  }
  return (1);
}

CMp4File::CMp4File (MP4FileHandle filehandle)
{
  m_mp4file = filehandle;
  m_file_mutex = SDL_CreateMutex();
  m_illegal_audio_codec = 0;
  m_illegal_video_codec = 0;
  m_have_audio = 0;
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
			   int video_offset,
			   char *errmsg, 
			   uint32_t errlen,
			   int &start_desc)
{
  int ix;
  CPlayerMedia *mptr;
  codec_plugin_t *plugin;

  for (ix = 0; ix < video_offset; ix++) {
    if (vq[ix].enabled != 0) {

      mptr = new CPlayerMedia(psptr);
      if (mptr == NULL) {
	return (-1);
      }
      if (MP4_IS_MPEG2_VIDEO_TYPE(vq[ix].type) &&
	  (vq[ix].h == 480 &&
	   vq[ix].w == 352)) {
	psptr->double_screen_width();
      }
						  
      video_info_t *vinfo;
      vinfo = (video_info_t *)malloc(sizeof(video_info_t));
      vinfo->height = vq[ix].h;
      vinfo->width = vq[ix].w;
      plugin = check_for_video_codec("MP4 FILE",
				     NULL,
				     vq[ix].type,
				     vq[ix].profile,
				     vq[ix].config, 
				     vq[ix].config_len);

      int ret = mptr->create_video_plugin(plugin, 
					  "MP4 FILE",
					  vq[ix].type,
					  vq[ix].profile,
					  NULL, // sdp info
					  vinfo, // video info
					  vq[ix].config,
					  vq[ix].config_len);

      if (ret < 0) {
	mp4f_message(LOG_ERR, "Failed to create plugin data");
	snprintf(errmsg, errlen, "Failed to start plugin");
	delete mptr;
	return -1;
      }

      CMp4VideoByteStream *vbyte;

#ifdef ISMACRYP
      /* check if ismacryp */
      printf("checking for encryption\n");
      if (MP4GetTrackIntegerProperty(m_mp4file, vq[ix].track_id,
			    "mdia.minf.stbl.stsd.encv.sinf.frma.data-format") 
	  != (u_int64_t)-1) {
	printf("encrypted video\n");
	vbyte = new CMp4EncVideoByteStream(this, vq[ix].track_id);
	printf("bytestream has been created\n");
	// TODO: add the code to end the session in the right place
      } else {
	printf("not encrypted video\n");
	vbyte = new CMp4VideoByteStream(this, vq[ix].track_id);
      }
#else
      vbyte = new CMp4VideoByteStream(this, vq[ix].track_id);
#endif
      if (vbyte == NULL) {
	delete mptr;
	return (-1);
      }

      ret = mptr->create(vbyte, TRUE, errmsg, errlen);
      if (ret != 0) {
	return (-1);
      }
      char *mp4info = MP4Info(m_mp4file, vq[ix].track_id);
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
			   int audio_offset,
			   char *errmsg,
			   uint32_t errlen,
			   int &start_desc)
{
  int ix;
  CPlayerMedia *mptr;
  codec_plugin_t *plugin;
  for (ix = 0; ix < audio_offset; ix++) {
    if (aq[ix].enabled != 0) {
      CMp4AudioByteStream *abyte;
      mptr = new CPlayerMedia(psptr);
      if (mptr == NULL) {
	return (-1);
      }

#ifdef ISMACRYP
      /* check if ismacryp */
      printf("checking for encryption\n");
      if (MP4GetTrackIntegerProperty(m_mp4file, aq[ix].track_id,
			    "mdia.minf.stbl.stsd.enca.sinf.frma.data-format") 
	  != (u_int64_t)-1) {
	printf("encrypted audio\n");
	abyte = new CMp4EncAudioByteStream(this, aq[ix].track_id);
	printf("bytestream has been created\n");
	// TODO: add the code to end the session in the right place
      } else {
	printf("not encrypted audio\n");
	abyte = new CMp4AudioByteStream(this, aq[ix].track_id);
      }
#else 
      abyte = new CMp4AudioByteStream(this, aq[ix].track_id);
#endif
      audio_info_t *ainfo;
      ainfo = (audio_info_t *)malloc(sizeof(audio_info_t));
      memset(ainfo, 0, sizeof(*ainfo));

      ainfo->freq = aq[ix].sampling_freq;
      if ((aq[ix].type == MP4_PCM16_LITTLE_ENDIAN_AUDIO_TYPE) ||
	  (aq[ix].type == MP4_PCM16_BIG_ENDIAN_AUDIO_TYPE)) {
	ainfo->bitspersample = 16;
      }

      int ret;
      plugin = check_for_audio_codec("MP4 FILE",
				     NULL,
				     aq[ix].type,
				     aq[ix].profile,
				     aq[ix].config,
				     aq[ix].config_len);

      ret = mptr->create_audio_plugin(plugin,
				      "MP4 FILE",
				      aq[ix].type, 
				      aq[ix].profile,
				      NULL, // sdp info
				      ainfo, // audio info
				      aq[ix].config,
				      aq[ix].config_len);
      if (ret < 0) {
	mp4f_message(LOG_ERR, "Couldn't create audio from plugin %s", 
		     plugin->c_name);
	snprintf(errmsg, errlen, "Couldn't start audio plugin %s", 
		 plugin->c_name);
	delete mptr;
	delete abyte;
	return -1;
      }

      ret = mptr->create(abyte, FALSE, errmsg, errlen);
      if (ret != 0) {
	return (-1);
      }
      char *mp4info = MP4Info(m_mp4file, aq[ix].track_id);
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


int CMp4File::create_media (CPlayerSession *psptr,
			    char *errmsg, 
			    uint32_t errlen,
			    int have_audio_driver,
			    control_callback_vft_t *cc_vft)
{
  int video_count, video_offset;
  int audio_count, audio_offset;
  MP4TrackId trackId;
  video_query_t *vq;
  audio_query_t *aq;
  int ix;
  codec_plugin_t *plugin;
  int ret_value = 0;
  
  video_count = 0;
  do {
    trackId = MP4FindTrackId(m_mp4file, video_count, MP4_VIDEO_TRACK_TYPE);

    if (MP4_IS_VALID_TRACK_ID(trackId)) {
      video_count++;
    }
  } while (MP4_IS_VALID_TRACK_ID(trackId));

  audio_count = 0;
  do {
    trackId = MP4FindTrackId(m_mp4file, audio_count, MP4_AUDIO_TRACK_TYPE);
    
    if (MP4_IS_VALID_TRACK_ID(trackId)) {
      audio_count++;
    }
  } while (MP4_IS_VALID_TRACK_ID(trackId));


  if (video_count == 0 && audio_count == 0) {
    snprintf(errmsg, errlen, "No audio or video tracks in file");
    return -1;
  }

  if (video_count > 0) {
    vq = (video_query_t *)malloc(sizeof(video_query_t) * video_count);
  } else {
    vq = NULL;
  }
  if (have_audio_driver && audio_count > 0) {
    aq = (audio_query_t *)malloc(sizeof(audio_query_t) * audio_count);
  } else {
    aq = NULL;
  }

  for (ix = 0, video_offset = 0; ix < video_count; ix++) {
    trackId = MP4FindTrackId(m_mp4file, ix, MP4_VIDEO_TRACK_TYPE);
    uint8_t video_type = MP4GetTrackEsdsObjectTypeId(m_mp4file, trackId);
    uint8_t profileID = MP4GetVideoProfileLevel(m_mp4file);
    mp4f_message(LOG_DEBUG, "MP4 - got track %x profile ID %d", 
		 trackId, profileID);
    uint8_t *foo;
    u_int32_t bufsize;
    MP4GetTrackESConfiguration(m_mp4file, trackId, &foo, &bufsize);
    
    plugin = check_for_video_codec("MP4 FILE",
				   NULL,
				   video_type,
				   profileID,
				   foo, 
				   bufsize);

    if (plugin == NULL) {
      snprintf(errmsg, errlen, 
	       "Can't find plugin for video type %d, profile %d",
	       video_type, profileID);
      m_illegal_video_codec++;
      ret_value = 1;
    } else {
      vq[video_offset].track_id = trackId;
      vq[video_offset].compressor = "MP4 FILE";
      vq[video_offset].type = video_type;
      vq[video_offset].profile = profileID;
      vq[video_offset].fptr = NULL;
      vq[video_offset].h = MP4GetTrackVideoHeight(m_mp4file, trackId);
      vq[video_offset].w = MP4GetTrackVideoWidth(m_mp4file, trackId);
      vq[video_offset].frame_rate = MP4GetTrackVideoFrameRate(m_mp4file, trackId);
      vq[video_offset].config = foo;
      vq[video_offset].config_len = bufsize;
      vq[video_offset].enabled = 0;
      vq[video_offset].reference = NULL;
      video_offset++;
    }
  }

  audio_offset = 0;
  if (have_audio_driver) {
    for (ix = 0; ix < audio_count; ix++) {
      trackId = MP4FindTrackId(m_mp4file, ix, MP4_AUDIO_TRACK_TYPE);
      uint8_t audio_type;
      uint8_t profile;
      uint8_t *userdata = NULL;
      u_int32_t userdata_size;

      audio_type = MP4GetTrackEsdsObjectTypeId(m_mp4file, trackId);
      profile = MP4GetAudioProfileLevel(m_mp4file);
      MP4GetTrackESConfiguration(m_mp4file, 
				 trackId, 
				 &userdata, 
				 &userdata_size);
      plugin = check_for_audio_codec("MP4 FILE",
				     NULL,
				     audio_type,
				     profile,
				     userdata,
				     userdata_size);
      if (plugin != NULL) {
	aq[audio_offset].track_id = trackId;
	aq[audio_offset].compressor = "MP4 FILE";
	aq[audio_offset].type = audio_type;
	aq[audio_offset].profile = profile;
	aq[audio_offset].fptr = NULL;
	aq[audio_offset].config = userdata;
	aq[audio_offset].config_len = userdata_size;
	aq[audio_offset].sampling_freq = 
	  MP4GetTrackTimeScale(m_mp4file, trackId);
	aq[audio_offset].chans = -1;
	aq[audio_offset].enabled = 0;
	aq[audio_offset].reference = NULL;
	audio_offset++;
      } else {
	m_illegal_audio_codec++;
	ret_value = 1;
      }
    }
  } else {
    if (audio_count)
      ret_value = 1;
  }

  if (cc_vft && cc_vft->media_list_query != NULL) {
    (cc_vft->media_list_query)(psptr, video_offset, vq, audio_offset, aq);
  } else {
    if (video_offset > 0) {
      vq[0].enabled = 1;
    }
    if (audio_offset > 0) {
      aq[0].enabled = 1;
    }
  }

  int ret;
  int start_desc = 1;
  ret = create_video(psptr, vq, video_offset, errmsg, errlen,start_desc);
  free(vq);

  if (ret < 0) {
    free(aq);
    return -1;
  }
 
  ret = create_audio(psptr, aq, audio_offset, errmsg, errlen, start_desc);
  free(aq);

  if (ret < 0) ret_value = -1;

  char *name;
  if (MP4GetMetadataName(m_mp4file, &name) &&
      name != NULL) {
    psptr->set_session_desc(0, name);
    free(name);
  }
  
  return (ret_value);
}

/* end file mp4_file.cpp */
