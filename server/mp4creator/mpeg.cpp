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
 * Copyright (C) Cisco Systems Inc. 2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May  wmay@cisco.com
 *              Alix Marchandise-Franquet alix@cisco.com
 */
#include "mp4creator.h"
#include "mpeg2_ps.h"

static MP4TrackId VideoCreate (MP4FileHandle mp4file, 
			       mpeg2ps_t *file, 
			       int vstream,
			       bool doEncrypt)
{
  double frame_rate = mpeg2ps_get_video_stream_framerate(file, vstream);
  uint8_t video_type;
  uint16_t w, h;
  MP4TrackId id;
  ismacryp_session_id_t ismaCrypSId;
  mp4v2_ismacrypParams *icPp =  (mp4v2_ismacrypParams *) malloc(sizeof(mp4v2_ismacrypParams));
  memset(icPp, 0, sizeof(mp4v2_ismacrypParams));


#ifdef _WIN32
  MP4Duration mp4FrameDuration;
  mp4FrameDuration = 
    (MP4Duration)((double)Mp4TimeScale / frame_rate);
#else
  MP4Duration mp4FrameDuration = 
    (MP4Duration)(Mp4TimeScale / frame_rate);
#endif

  h = mpeg2ps_get_video_stream_height(file, vstream);
  w = mpeg2ps_get_video_stream_width(file, vstream);

  video_type = mpeg2ps_get_video_stream_mp4_type(file, vstream);

  if (doEncrypt) {
    // initialize session
    if (ismacrypInitSession(&ismaCrypSId,KeyTypeVideo) != 0) {
      fprintf(stderr, "%s: could not initialize the ISMAcryp session\n",
	      ProgName);
      return MP4_INVALID_TRACK_ID;
    }
    if (ismacrypGetScheme(ismaCrypSId, &(icPp->scheme_type)) != ismacryp_rc_ok) {
       fprintf(stderr, "%s: could not get ismacryp scheme type. sid %d\n", 
               ProgName, ismaCrypSId);
       ismacrypEndSession(ismaCrypSId);
       return MP4_INVALID_TRACK_ID;
    }
    if (ismacrypGetSchemeVersion(ismaCrypSId, &(icPp->scheme_version)) != ismacryp_rc_ok) {
       fprintf(stderr, "%s: could not get ismacryp scheme ver. sid %d\n",
               ProgName, ismaCrypSId);
       ismacrypEndSession(ismaCrypSId);
       return MP4_INVALID_TRACK_ID;
    }
    if (ismacrypGetKMSUri(ismaCrypSId, &(icPp->kms_uri)) != ismacryp_rc_ok) {
       fprintf(stderr, "%s: could not get ismacryp kms uri. sid %d\n",
               ProgName, ismaCrypSId);
       CHECK_AND_FREE(icPp->kms_uri);
       ismacrypEndSession(ismaCrypSId);
       return MP4_INVALID_TRACK_ID;
    }
    if ( ismacrypGetSelectiveEncryption(ismaCrypSId, &(icPp->selective_enc)) != ismacryp_rc_ok ) {
       fprintf(stderr, "%s: could not get ismacryp selec enc. sid %d\n",
               ProgName, ismaCrypSId);
       ismacrypEndSession(ismaCrypSId);
       return MP4_INVALID_TRACK_ID;
    }
    if (ismacrypGetKeyIndicatorLength(ismaCrypSId, &(icPp->key_ind_len)) != ismacryp_rc_ok) {
       fprintf(stderr, "%s: could not get ismacryp key ind len. sid %d\n",
               ProgName, ismaCrypSId);
       ismacrypEndSession(ismaCrypSId);
       return MP4_INVALID_TRACK_ID;
    }
    if (ismacrypGetIVLength(ismaCrypSId, &(icPp->iv_len)) != ismacryp_rc_ok) {
       fprintf(stderr, "%s: could not get ismacryp iv len. sid %d\n",
               ProgName, ismaCrypSId);
       ismacrypEndSession(ismaCrypSId);
       return MP4_INVALID_TRACK_ID;
    }
    id = MP4AddEncVideoTrack(mp4file, 
			     Mp4TimeScale, 
			     mp4FrameDuration,
			     w, 
			     h,
                             icPp,
			     video_type);
  } else {
    id = MP4AddVideoTrack(mp4file, 
			  Mp4TimeScale, 
			  mp4FrameDuration,
			  w, 
			  h, 
			  video_type);
  }

  //printf("duration "U64" w %d h %d type %x\n", mp4FrameDuration, w, h, video_type);
  if (MP4GetNumberOfTracks(mp4file, MP4_VIDEO_TRACK_TYPE) == 1) {
    MP4SetVideoProfileLevel(mp4file, 0xfe); // undefined profile
  }

  if (id == MP4_INVALID_TRACK_ID) {
    fprintf(stderr, "%s:Couldn't add video track %d", ProgName, vstream);
    return MP4_INVALID_TRACK_ID;
  }
  uint8_t *buf;
  uint32_t blen;
  uint32_t frames = 1;
#if 0
  printf("Processing %lu video frames\n", frames_max);
#endif
  uint32_t refFrame = 1;
  uint8_t frame_type;
  while (mpeg2ps_get_video_frame(file, 
				 vstream,
				 &buf, 
				 &blen, 
				 &frame_type,
				 TS_90000,
				 NULL)) {
    if (buf[blen - 4] == 0 &&
	buf[blen - 3] == 0 &&
	buf[blen - 2] == 1) blen -= 4;
    
    // encrypt the sample if neeed
    if (doEncrypt) {
      u_int8_t* encSampleData = NULL;
      u_int32_t encSampleLen = 0;
      if (ismacrypEncryptSampleAddHeader(ismaCrypSId, blen, buf,
				&encSampleLen, &encSampleData) != 0) {
	fprintf(stderr,	
		"%s: can't encrypt video sample and add header %u\n", ProgName, id);
      }
      MP4WriteSample(mp4file, id, encSampleData, encSampleLen, 
		     mp4FrameDuration, 0, 
		     frame_type == 1 ? true : false);
      if (encSampleData != NULL) {
	free(encSampleData);
      }
    } else {
      MP4WriteSample(mp4file, id, buf, blen, mp4FrameDuration, 0, 
		     frame_type == 1 ? true : false);
#if 0
      printf("frame %d len %d duration "U64" ftype %d\n",
	     frames, blen, mp4FrameDuration, frame_type);
#endif
    }
    if (frame_type != 3) {
      // I or P frame
      MP4SetSampleRenderingOffset(mp4file, id, refFrame, 
				  (frames - refFrame) * mp4FrameDuration);
      refFrame = frames;
    }
    frames++;
#if 0
    if ((frames % 100) == 0) printf("%d frames\n", frames);
#endif
  }

  // if encrypting, terminate the ismacryp session
  if (doEncrypt) {
    if (ismacrypEndSession(ismaCrypSId) != 0) {
      fprintf(stderr, 
	      "%s: could not end the ISMAcryp session\n",
	      ProgName);
      return MP4_INVALID_TRACK_ID;
    }
  }
  return id;
}

static MP4TrackId AudioCreate (MP4FileHandle mp4file, 
			       mpeg2ps_t *file, 
			       int astream,
			       bool doEncrypt)
{
  uint16_t freq;
  int type;
  MP4TrackId id;
  uint16_t samples_per_frame;
  uint8_t *buf = NULL;
  uint32_t blen = 0;
  uint32_t frame_num = 1;
  ismacryp_session_id_t ismaCrypSId;
  mp4v2_ismacrypParams *icPp =  (mp4v2_ismacrypParams *) malloc(sizeof(mp4v2_ismacrypParams));
  MP4AV_Mp3Header hdr;
  u_int8_t mpegVersion;
  memset(icPp, 0, sizeof(mp4v2_ismacrypParams));

  type = mpeg2ps_get_audio_stream_type(file, astream);

  if (type != MPEG_AUDIO_MPEG) {
    fprintf(stderr, "Unsupported audio format %d in audio stream %d\n", 
	    type, astream);
    return MP4_INVALID_TRACK_ID;
  }

  freq = mpeg2ps_get_audio_stream_sample_freq(file, astream);

  if (mpeg2ps_get_audio_frame(file, 
			      astream,
			      &buf, 
			      &blen,
			      TS_90000,
			      NULL, 
			      NULL) == false) {
    fprintf(stderr, "No audio tracks in audio stream %d\n", astream);
    return MP4_INVALID_TRACK_ID;
  }
  
  hdr = MP4AV_Mp3HeaderFromBytes(buf);
  mpegVersion = MP4AV_Mp3GetHdrVersion(hdr);
  samples_per_frame = MP4AV_Mp3GetHdrSamplingWindow(hdr);

  u_int8_t audioType = MP4AV_Mp3ToMp4AudioType(mpegVersion);
  
  if (audioType == MP4_INVALID_AUDIO_TYPE
      || samples_per_frame == 0) {
    fprintf(stderr,	
	    "%s: data in file doesn't appear to be valid audio\n",
	    ProgName);
    return MP4_INVALID_TRACK_ID;
  }

  MP4Duration duration = (90000 * samples_per_frame) / freq;

  if (doEncrypt) {
    // initialize the ismacryp session
    if (ismacrypInitSession(&ismaCrypSId,KeyTypeAudio) != 0) {
      fprintf(stderr, 
	      "%s: could not initialize the ISMAcryp session\n",
	      ProgName);
      return MP4_INVALID_TRACK_ID;
    }
    if (ismacrypGetScheme(ismaCrypSId, &(icPp->scheme_type)) != ismacryp_rc_ok) {
       fprintf(stderr, "%s: could not get ismacryp scheme type. sid %d\n", 
               ProgName, ismaCrypSId);
       ismacrypEndSession(ismaCrypSId);
       return MP4_INVALID_TRACK_ID;
    }
    if (ismacrypGetSchemeVersion(ismaCrypSId, &(icPp->scheme_version)) != ismacryp_rc_ok) {
       fprintf(stderr, "%s: could not get ismacryp scheme ver. sid %d\n",
               ProgName, ismaCrypSId);
       ismacrypEndSession(ismaCrypSId);
       return MP4_INVALID_TRACK_ID;
    }
    if (ismacrypGetKMSUri(ismaCrypSId, &(icPp->kms_uri)) != ismacryp_rc_ok) {
       fprintf(stderr, "%s: could not get ismacryp kms uri. sid %d\n",
               ProgName, ismaCrypSId);
       CHECK_AND_FREE(icPp->kms_uri);
       ismacrypEndSession(ismaCrypSId);
       return MP4_INVALID_TRACK_ID;
    }
    if ( ismacrypGetSelectiveEncryption(ismaCrypSId, &(icPp->selective_enc)) != ismacryp_rc_ok ) {
       fprintf(stderr, "%s: could not get ismacryp selec enc. sid %d\n",
               ProgName, ismaCrypSId);
       ismacrypEndSession(ismaCrypSId);
       return MP4_INVALID_TRACK_ID;
    }
    if (ismacrypGetKeyIndicatorLength(ismaCrypSId, &(icPp->key_ind_len)) != ismacryp_rc_ok) {
       fprintf(stderr, "%s: could not get ismacryp key ind len. sid %d\n",
               ProgName, ismaCrypSId);
       ismacrypEndSession(ismaCrypSId);
       return MP4_INVALID_TRACK_ID;
    }
    if (ismacrypGetIVLength(ismaCrypSId, &(icPp->iv_len)) != ismacryp_rc_ok) {
       fprintf(stderr, "%s: could not get ismacryp iv len. sid %d\n",
               ProgName, ismaCrypSId);
       ismacrypEndSession(ismaCrypSId);
       return MP4_INVALID_TRACK_ID;
    }
    id = MP4AddEncAudioTrack(mp4file, 
			     90000, 
			     duration,
                             icPp,
			     audioType);
  } else {
    id = MP4AddAudioTrack(mp4file, 
			  90000, 
			  duration,
			  audioType);
  }
  
  if (id == MP4_INVALID_TRACK_ID) {
    fprintf(stderr, 
	    "%s: can't create audio track from stream %d\n", 
	    ProgName, astream);
    return MP4_INVALID_TRACK_ID;
  }

  if (MP4GetNumberOfTracks(mp4file, MP4_AUDIO_TRACK_TYPE) == 1) {
    MP4SetAudioProfileLevel(mp4file, 0xFE);
  }

  do {
    // encrypt if needed
     if (doEncrypt) {
       u_int8_t* encSampleData = NULL;
       u_int32_t encSampleLen = 0;
       if (ismacrypEncryptSampleAddHeader(ismaCrypSId, blen, buf,
					  &encSampleLen, &encSampleData) != 0) {
	 fprintf(stderr,	
		 "%s: can't encrypt audio sample and add header %u\n", ProgName, id);
       }
       // now write the sample
       if (!MP4WriteSample(mp4file, id, encSampleData, encSampleLen)) {
	 fprintf(stderr, "%s: can't write audio track %u, stream %d",
		 ProgName, frame_num, astream);
	 MP4DeleteTrack(mp4file, id);
	 return MP4_INVALID_TRACK_ID;
       }
       if (encSampleData != NULL) {
	 free(encSampleData);
       }
    }
     // now write the sample
    if (!MP4WriteSample(mp4file, id, buf, blen)) {
      fprintf(stderr, "%s: can't write audio track %u, stream %d",
	      ProgName, frame_num, astream);
      MP4DeleteTrack(mp4file, id);
      return MP4_INVALID_TRACK_ID;
    }
    frame_num++;
#if 0
    if ((frame_num % 100) == 0) printf("Audio frame %d\n", frame_num);
#endif
  }  while (mpeg2ps_get_audio_frame(file, 
				    astream, 
				    &buf, 
				    &blen,
				    TS_90000,
				    NULL, NULL));
  
  // if encrypting, terminate the ismacryp session
  if (doEncrypt) {
    if (ismacrypEndSession(ismaCrypSId) != 0) {
      fprintf(stderr, 
	      "%s: could not end the ISMAcryp session\n",
	      ProgName);
      return MP4_INVALID_TRACK_ID;
    }
  }

  return id;
}

MP4TrackId *MpegCreator (MP4FileHandle mp4file, const char *fname, bool doEncrypt)
{

  mpeg2ps_t *file;
  int video_streams, audio_streams;
  int ix;
  MP4TrackId *pTrackId;

  file = mpeg2ps_init(fname);
  video_streams = mpeg2ps_get_video_stream_count(file);
  audio_streams = mpeg2ps_get_audio_stream_count(file);

  pTrackId = 
    (MP4TrackId *)malloc(sizeof(MP4TrackId) * (audio_streams + video_streams + 1));
 
  for (ix = 0; ix < video_streams + audio_streams + 1; ix++) {
    pTrackId[ix] = MP4_INVALID_TRACK_ID;
  }

  for (ix = 0; ix < video_streams; ix++) {
    pTrackId[ix] = VideoCreate(mp4file, file, ix, doEncrypt);
  }
  for (ix = 0; ix < audio_streams; ix++) {
    pTrackId[ix + video_streams] = AudioCreate(mp4file, file, ix, doEncrypt);
  }
  return pTrackId;
}
