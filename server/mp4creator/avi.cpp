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
 * Copyright (C) Cisco Systems Inc. 2000-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

/* 
 * Notes:
 *  - file formatted with tabstops == 4 spaces 
 */

#include <mp4creator.h>
#include <avilib.h>
static void set_es_config (MP4FileHandle mp4File, 
			   MP4TrackId trackId, 
			   uint8_t *pBuffer, 
			   uint32_t len)
{
  uint8_t *outbuf = (uint8_t *)malloc(len);
  uint8_t *in, *out;
  uint32_t ix;
  uint32_t code = 0;
  uint32_t outlen;

  memcpy(outbuf, pBuffer, 4);
  outlen = 4;
  len -= 4;
  in = pBuffer + 4;
  out = outbuf + 4;
  bool skip = false;

  for (ix = 0; ix < len; ix++) {
    code <<= 8;
    code |= *in;
    if (code == ((MP4AV_MPEG4_SYNC << 8) | MP4AV_MPEG4_USER_DATA_START)) {
      skip = true;
    } else {
      if ((code & 0xffffff00) == (MP4AV_MPEG4_SYNC << 8)) {
	skip = false;
      }
      if (skip == false) {
	*out++ = *in;
	outlen++;
      }
    }
    in++;
  }
  if (skip) {
    outlen -= 3; // we wrote 00 00 01 before skip
  }
      
  MP4SetTrackESConfiguration(mp4File, trackId,
			     outbuf, outlen);
}

static MP4TrackId VideoCreator(MP4FileHandle mp4File, avi_t* aviFile, bool doEncrypt)
{
  char* videoType = AVI_video_compressor(aviFile);
  u_int8_t videoProfileLevel = MPEG4_SP_L3;
  int32_t numFrames = AVI_video_frames(aviFile);
  int32_t ix;
  int32_t frameSize;
  int32_t maxFrameSize = 0;
  static u_int8_t startCode[3] = { 
    0x00, 0x00, 0x01, 
  };
  u_int32_t vopStart = 0;
  bool foundVOP = false;
  bool foundVOSH = false;
  bool foundVisualObjectStart = false;
  bool foundVO = false;
  bool foundVOL = false;
  u_int8_t* pFrameBuffer;
  uint8_t *ppFrames[2];
  uint8_t *pPrevFrameBuffer;
  uint32_t prevFrameSize;
  ismacryp_session_id_t ismaCrypSId;
  uint8_t use_buffer = 1;
  //MP4Timestamp currentTime;

  int vopType;
  mp4v2_ismacrypParams *icPp =  (mp4v2_ismacrypParams *) malloc(sizeof(mp4v2_ismacrypParams));
  memset(icPp, 0, sizeof(mp4v2_ismacrypParams));

  if (strcasecmp(videoType, "divx")
      && strcasecmp(videoType, "dx50")
      && strcasecmp(videoType, "xvid")) {
    fprintf(stderr,	
	    "%s: video compressor %s not recognized\n",
	    ProgName, videoType);
    return MP4_INVALID_TRACK_ID;
  }

  if (doEncrypt) {
    if (ismacrypInitSession(&ismaCrypSId, KeyTypeAudio) != 0) {
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
      if (icPp->kms_uri != NULL) free(icPp->kms_uri);
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
  }

  double frameRate = AVI_video_frame_rate(aviFile);

  if (frameRate == 0) {
    fprintf(stderr,	
	    "%s: no video frame rate in avi file\n", ProgName);
    return MP4_INVALID_TRACK_ID;
  }

#ifdef _WIN32
  MP4Duration mp4FrameDuration;
  mp4FrameDuration = 
    (MP4Duration)((double)Mp4TimeScale / frameRate);
#else
  MP4Duration mp4FrameDuration = 
    (MP4Duration)(Mp4TimeScale / frameRate);
#endif

  MP4TrackId trackId;
  if (doEncrypt) {
    trackId = MP4AddEncVideoTrack(
				  mp4File,
				  Mp4TimeScale, 
				  mp4FrameDuration, 
				  AVI_video_width(aviFile), 
				  AVI_video_height(aviFile), 
				  icPp,
				  MP4_MPEG4_VIDEO_TYPE);
  } else {
    trackId = MP4AddVideoTrack(
			       mp4File,
			       Mp4TimeScale, 
			       mp4FrameDuration, 
			       AVI_video_width(aviFile), 
			       AVI_video_height(aviFile), 
			       MP4_MPEG4_VIDEO_TYPE);
  }

  if (trackId == MP4_INVALID_TRACK_ID) {
    fprintf(stderr,	
	    "%s: can't create video track\n", ProgName);
    return MP4_INVALID_TRACK_ID;
  }


  // determine maximum video frame size
  AVI_seek_start(aviFile);

  for (ix = 0; ix < numFrames; ix++) {
    frameSize = AVI_frame_size(aviFile, ix);
    if (frameSize > maxFrameSize) {
      maxFrameSize = frameSize;
    }
  }

  // allocate a large enough frame buffer
  ppFrames[0] = (u_int8_t*)malloc(maxFrameSize + 4);
  ppFrames[1] = (u_int8_t*)malloc(maxFrameSize + 4);
  pFrameBuffer = ppFrames[0];

  if (pFrameBuffer == NULL) {
    fprintf(stderr,	
	    "%s: can't allocate memory\n", ProgName);
    MP4DeleteTrack(mp4File, trackId);
    return MP4_INVALID_TRACK_ID;
  }

  AVI_seek_start(aviFile);

  // read first frame, should contain VO/VOL and I-VOP
  frameSize = AVI_read_frame(aviFile, (char*)pFrameBuffer);

  if (frameSize < 0) {
    fprintf(stderr,	
	    "%s: can't read video frame 1: %s\n",
	    ProgName, AVI_strerror());
    MP4DeleteTrack(mp4File, trackId);
    return MP4_INVALID_TRACK_ID;
  }

  // find VOP start code in first sample

  // Note that we go from 4 to framesize, not 0 to framesize - 4
  // this is adding padding at the beginning for the VOSH, if
  // needed
  for (ix = 0; foundVOP == false && ix < frameSize; ix++) {
    if (!memcmp(&pFrameBuffer[ix], startCode, 3)) {
      // everything before the VOP
      // should be configuration info
      if (pFrameBuffer[ix + 3] == MP4AV_MPEG4_VOP_START) {
	vopStart = ix;
	foundVOP = true;

      } else if (pFrameBuffer[ix + 3] == MP4AV_MPEG4_VOL_START) {
	u_int8_t timeBits = 15;
	u_int16_t timeTicks = 30000;
	u_int16_t frameDuration = 3000;
	u_int16_t frameWidth = 0;
	u_int16_t frameHeight = 0;
	if (MP4AV_Mpeg4ParseVol(pFrameBuffer + ix, frameSize - ix, 
				&timeBits, &timeTicks, &frameDuration, 
				&frameWidth, &frameHeight) == false) {
	  fprintf(stderr, "Couldn't parse vol\n");
	}
	if (frameWidth != AVI_video_width(aviFile)) {
	  fprintf(stderr, 
		  "%s:avi file video width does not match VOL header - %d vs. %d\n", 
		  ProgName, frameWidth, AVI_video_width(aviFile));
	}
	if (frameHeight != AVI_video_height(aviFile)) {
	  fprintf(stderr, 
		  "%s:avi file video height does not match VOL header - %d vs. %d\n", 
		  ProgName, frameHeight, AVI_video_height(aviFile));
	}
	foundVOL = true;
	      
      } else if (pFrameBuffer[ix + 3] == MP4AV_MPEG4_VOSH_START) {
	foundVOSH = true;
	MP4AV_Mpeg4ParseVosh(pFrameBuffer + ix, 
			     frameSize - ix, 
			     &videoProfileLevel);
      } else if (pFrameBuffer[ix + 3] == MP4AV_MPEG4_VO_START) {
	foundVisualObjectStart = true;
      } else if (pFrameBuffer[ix + 3] <= 0x1f) {
	foundVO = true;
      }
      ix += 3; // skip past header
    }
  }

  if (foundVOSH == false) {
    fprintf(stderr, 
	    "%s:Warning: no Visual Object Sequence Start (VOSH) header found "
	    "in MPEG-4 video.\n"
	    "This can cause problems with players other than those included\n"
	    "with this package.\n", ProgName);
  } else {
    if (VideoProfileLevelSpecified &&
	videoProfileLevel != VideoProfileLevel) {
      fprintf(stderr, 
	      "%s:You have specified a different video profile level than "
	      "was detected in the VOSH header\n"
	      "The level you specified was %d and %d was read from the VOSH\n",
	      ProgName, VideoProfileLevel, videoProfileLevel);
    }
  }

  if (foundVisualObjectStart == false) {
    fprintf(stderr, 
	    "%s:Warning: no Visual Object start header found in MPEG-4 video.\n"
	    "This can cause problems with players other than those included\n"
	    "with this package.\n", ProgName);
  }
	  
  if (foundVO == false) {
    fprintf(stderr, 
	    "%s:Warning: No VO header found in mpeg-4 video.\n"
	    "This can cause problems with players other than mp4player\n",
	    ProgName);
  }
  if (foundVOL == false) {
    fprintf(stderr, 
	    "%s: fatal: No VOL header found in mpeg-4 video stream\n",
	    ProgName);
    MP4DeleteTrack(mp4File, trackId);
    return MP4_INVALID_TRACK_ID;
  }

  if (MP4GetNumberOfTracks(mp4File, MP4_VIDEO_TRACK_TYPE) == 1) {
    if (VideoProfileLevelSpecified) {
      videoProfileLevel = VideoProfileLevel;
    }
    MP4SetVideoProfileLevel(mp4File, videoProfileLevel);
  }
  set_es_config(mp4File, trackId, pFrameBuffer, vopStart);

  pPrevFrameBuffer = &pFrameBuffer[vopStart];
  prevFrameSize = frameSize - vopStart;


  MP4Duration dur = mp4FrameDuration;

  // process the rest of the frames
  for (ix = 1; ix < numFrames; ix++) {
    pFrameBuffer = ppFrames[use_buffer];

    frameSize = AVI_read_frame(aviFile, (char*)pFrameBuffer);

    if (frameSize < 0) {
      fprintf(stderr,	
	      "%s: can't read video frame %i: %s\n",
	      ProgName, ix + 1, AVI_strerror());
      MP4DeleteTrack(mp4File, trackId);
      return MP4_INVALID_TRACK_ID;
    }

    // we mark random access points in MP4 files
    // note - we will skip any extra information in the
    // header - only VOPs are allowed to be written
    // wmay - 6-2004
    uint8_t *vophdr = MP4AV_Mpeg4FindVop(pFrameBuffer, frameSize);
		
    if (vophdr != NULL) {
      int32_t diff = vophdr - pFrameBuffer;
      frameSize -= diff;
      pFrameBuffer = vophdr;

      vopType = MP4AV_Mpeg4GetVopType(pPrevFrameBuffer, prevFrameSize);

      MP4WriteSample(mp4File, trackId, 
		     pPrevFrameBuffer, prevFrameSize, dur, 0, 
		     vopType  == VOP_TYPE_I);
      dur = mp4FrameDuration;
      pPrevFrameBuffer = vophdr;
      prevFrameSize = frameSize;
      use_buffer = use_buffer == 0 ? 1 : 0;
    } else {
      fprintf(stderr, 
	      "Video frame %d does not have a VOP code - skipping\n",
	      ix + 1);
      dur += mp4FrameDuration;
    }

    // write the frame to the MP4 file
  }

  // write the last frame
  vopType = MP4AV_Mpeg4GetVopType(pPrevFrameBuffer, prevFrameSize);
  MP4WriteSample(mp4File, trackId, 
		 pPrevFrameBuffer, prevFrameSize, dur, 0, 
		 vopType  == VOP_TYPE_I);

  return trackId;
}

static inline u_int32_t BytesToInt32(u_int8_t* pBytes) 
{
  return (pBytes[0] << 24) | (pBytes[1] << 16) 
    | (pBytes[2] << 8) | pBytes[3];
}

static MP4TrackId AudioCreator(MP4FileHandle mp4File, avi_t* aviFile, bool doEncrypt)
{
  int32_t audioType = AVI_audio_format(aviFile);
  ismacryp_session_id_t ismaCrypSId;
  mp4v2_ismacrypParams *icPp =  (mp4v2_ismacrypParams *) malloc(sizeof(mp4v2_ismacrypParams));
  memset(icPp, 0, sizeof(mp4v2_ismacrypParams));

  // Check for MP3 audio type
  if (audioType != 0x55) {
    fprintf(stderr,	
	    "%s: audio compressor 0x%x not recognized\n",
	    ProgName, audioType);
    return MP4_INVALID_TRACK_ID;
  }

  if (doEncrypt) {
    if (ismacrypInitSession(&ismaCrypSId, KeyTypeAudio) != 0) {
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
      if (icPp->kms_uri != NULL) free(icPp->kms_uri);
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
  }

  u_int8_t temp[4];
  u_int32_t mp3header;

  AVI_seek_start(aviFile);

  if (AVI_read_audio(aviFile, (char*)&temp, 4) != 4) {
    fprintf(stderr,	
	    "%s: can't read audio frame 0: %s\n",
	    ProgName, AVI_strerror());
    return MP4_INVALID_TRACK_ID;
  }
  mp3header = BytesToInt32(temp);

  // check mp3header sanity
  if ((mp3header & 0xFFE00000) != 0xFFE00000) {
    fprintf(stderr,	
	    "%s: data in file doesn't appear to be valid mp3 audio\n",
	    ProgName);
    return MP4_INVALID_TRACK_ID;
  }

  u_int16_t samplesPerSecond = 
    MP4AV_Mp3GetHdrSamplingRate(mp3header);
  u_int16_t samplesPerFrame = 
    MP4AV_Mp3GetHdrSamplingWindow(mp3header);
  u_int8_t mp4AudioType = 
    MP4AV_Mp3ToMp4AudioType(MP4AV_Mp3GetHdrVersion(mp3header));

  if (audioType == MP4_INVALID_AUDIO_TYPE
      || samplesPerSecond == 0) {
    fprintf(stderr,	
	    "%s: data in file doesn't appear to be valid mp3 audio\n",
	    ProgName);
    return MP4_INVALID_TRACK_ID;
  }

	
  MP4TrackId trackId;
  if (doEncrypt) {
    trackId = MP4AddEncAudioTrack(mp4File, samplesPerSecond, 
				  samplesPerFrame, 
				  icPp,
				  mp4AudioType);
  } else {
    trackId = MP4AddAudioTrack(mp4File, samplesPerSecond, 
			       samplesPerFrame, mp4AudioType);
  }

  if (trackId == MP4_INVALID_TRACK_ID) {
    fprintf(stderr,	
	    "%s: can't create audio track\n", ProgName);
    return MP4_INVALID_TRACK_ID;
  }

  if (MP4GetNumberOfTracks(mp4File, MP4_AUDIO_TRACK_TYPE) == 1) {
    MP4SetAudioProfileLevel(mp4File, 0xFE);
  }

  int32_t i;
  int32_t aviFrameSize;
  int32_t maxAviFrameSize = 0;

  // determine maximum avi audio chunk size
  // should be at least as large as maximum mp3 frame size
  AVI_seek_start(aviFile);

  i = 0;
  while (AVI_set_audio_frame(aviFile, i, (long*)&aviFrameSize) == 0) {
    if (aviFrameSize > maxAviFrameSize) {
      maxAviFrameSize = aviFrameSize;
    }
    i++;
  }

  // allocate a large enough frame buffer
  u_int8_t* pFrameBuffer = (u_int8_t*)malloc(maxAviFrameSize);

  if (pFrameBuffer == NULL) {
    fprintf(stderr,	
	    "%s: can't allocate memory\n", ProgName);
    MP4DeleteTrack(mp4File, trackId);
    return MP4_INVALID_TRACK_ID;
  }

  AVI_seek_start(aviFile);

  u_int32_t mp3FrameNumber = 1;
  while (true) {
    if (AVI_read_audio(aviFile, (char*)pFrameBuffer, 4) != 4) {
      // EOF presumably
      break;
    }

    mp3header = BytesToInt32(&pFrameBuffer[0]);

    u_int16_t mp3FrameSize = MP4AV_Mp3GetFrameSize(mp3header);

    int bytesRead =
      AVI_read_audio(aviFile, (char*)&pFrameBuffer[4], mp3FrameSize - 4);

    if (bytesRead != mp3FrameSize - 4) {
      fprintf(stderr,	
	      "%s: Warning, incomplete audio frame %u, ending audio track\n",
	      ProgName, mp3FrameNumber);
      break;
    }

    if (!MP4WriteSample(mp4File, trackId, 
			&pFrameBuffer[0], mp3FrameSize)) {
      fprintf(stderr,	
	      "%s: can't write audio frame %u\n", ProgName, mp3FrameNumber);
      MP4DeleteTrack(mp4File, trackId);
      return MP4_INVALID_TRACK_ID;
    }
	
    mp3FrameNumber++;
  }

  return trackId;
}

MP4TrackId* AviCreator(MP4FileHandle mp4File, const char* aviFileName, 
		       bool doEncrypt)
{
  static MP4TrackId trackIds[3];
  u_int8_t numTracks = 0;

  avi_t* aviFile = AVI_open_input_file(aviFileName, true);
  if (aviFile == NULL) {
    fprintf(stderr,	
	    "%s: can't open %s: %s\n",
	    ProgName, aviFileName, AVI_strerror());

  } else {
    if (AVI_video_frames(aviFile) > 0) {
      trackIds[numTracks] = VideoCreator(mp4File, aviFile, 
					 doEncrypt);
      if (trackIds[numTracks] != MP4_INVALID_TRACK_ID) {
	numTracks++;
      }
    }

    if (AVI_audio_bytes(aviFile) > 0) {
      trackIds[numTracks] = AudioCreator(mp4File, aviFile, 
					 doEncrypt);
      if (trackIds[numTracks] != MP4_INVALID_TRACK_ID) {
	numTracks++;
      }
    }

    AVI_close(aviFile);
  }

  trackIds[numTracks] = MP4_INVALID_TRACK_ID;

  return trackIds;
}

