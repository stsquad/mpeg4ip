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
 */
#include "mp4creator.h"
#include "libmpeg3.h"

static MP4TrackId VideoCreate (MP4FileHandle mp4file, 
			       mpeg3_t *file, 
			       int vstream,
			       bool doEncrypt)
{
  double frame_rate = mpeg3_frame_rate(file, vstream);
  uint8_t video_type;
  uint16_t w, h;
  long frames_max;
  MP4TrackId id;
#ifdef _WIN32
  MP4Duration mp4FrameDuration;
  mp4FrameDuration = 
    (MP4Duration)((double)Mp4TimeScale / frame_rate);
#else
  MP4Duration mp4FrameDuration = 
    (MP4Duration)(Mp4TimeScale / frame_rate);
#endif

  h = mpeg3_video_height(file, vstream);
  w = mpeg3_video_width(file, vstream);
  video_type = mpeg3_video_layer(file, vstream) == 2 ?
    MP4_MPEG2_MAIN_VIDEO_TYPE : MP4_MPEG1_VIDEO_TYPE;

  if (doEncrypt) {
#ifdef ISMACRYP
    id = MP4AddEncVideoTrack(mp4file, 
			     Mp4TimeScale, 
			     mp4FrameDuration,
			     w, 
			     h, 
			     video_type);
#else
    id = MP4_INVALID_TRACK_ID;
    fprintf(stderr,
	    "%s: enable ismacrypt to encrypt (--enable-ismacrypt=<path>)\n",
	    ProgName);
#endif
  } else {
    id = MP4AddVideoTrack(mp4file, 
			  Mp4TimeScale, 
			  mp4FrameDuration,
			  w, 
			  h, 
			  video_type);
  }

  //printf("duration %llu w %d h %d type %x\n", mp4FrameDuration, w, h, video_type);
  if (MP4GetNumberOfTracks(mp4file, MP4_VIDEO_TRACK_TYPE) == 1) {
    MP4SetVideoProfileLevel(mp4file, 0xfe); // undefined profile
  }

  if (id == MP4_INVALID_TRACK_ID) {
    fprintf(stderr, "%s:Couldn't add video track %d", ProgName, vstream);
    return MP4_INVALID_TRACK_ID;
  }
  frames_max = mpeg3_video_frames(file, vstream);
  uint8_t *buf;
  long blen;
  uint32_t frames = 1;
#if 0
  printf("Processing %lu video frames\n", frames_max);
#endif
  uint32_t refFrame = 1;

  while (mpeg3_read_video_chunk_resize(file, 
				      (unsigned char **)&buf, 
				      &blen, 
				      vstream) == 0) {
    int ret;
    int frame_type;
    ret = MP4AV_Mpeg3FindGopOrPictHdr(buf, blen, &frame_type);
    // we sometimes have the next start code...
    if (buf[blen - 4] == 0 &&
	buf[blen - 3] == 0 &&
	buf[blen - 2] == 1) blen -= 4;
    
#ifdef ISMACRYP
    // encrypt the sample if neeed
    if (doEncrypt) {
      if (ismacrypEncryptSample(ismaCryptSId, blen, buf) != 0) {
	fprintf(stderr,	
		"%s: can't encrypt video sample %u\n", ProgName, id);
      }
    }
#endif
    MP4WriteSample(mp4file, id, buf, blen, mp4FrameDuration, 0, 
		   ret >= 0 ? true : false);
    mpeg3_read_video_chunk_cleanup(file, vstream);
    if (ret == 0 || frame_type != 3) {
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
  return id;
}

static MP4TrackId AudioCreate (MP4FileHandle mp4file, 
			       mpeg3_t *file, 
			       int astream,
			       bool doEncrypt)
{
  uint16_t freq;
  int type;
  MP4TrackId id;
  uint16_t samples_per_frame;
  uint8_t *buf = NULL;
  uint32_t blen = 0;
  uint32_t blenmax = 0;
  uint32_t frame_num = 1;

  type = mpeg3_get_audio_format(file, astream);

  if (type != AUDIO_MPEG) {
    fprintf(stderr, "Unsupported audio format %d in audio stream %d\n", 
	    type, astream);
    return MP4_INVALID_TRACK_ID;
  }

  freq = mpeg3_sample_rate(file, astream);
  samples_per_frame = mpeg3_audio_samples_per_frame(file, astream);

  if (mpeg3_read_audio_frame(file, 
			 (unsigned char **)&buf, 
			 &blen,
			 &blenmax, 
			 astream) < 0) {
    fprintf(stderr, "No audio tracks in audio stream %d\n", astream);
    return MP4_INVALID_TRACK_ID;
  }
  MP4AV_Mp3Header hdr;
  u_int8_t mpegVersion;
  
  hdr = MP4AV_Mp3HeaderFromBytes(buf);
  mpegVersion = MP4AV_Mp3GetHdrVersion(hdr);

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
#ifdef ISMACRYP
    id = MP4AddEncAudioTrack(mp4file, 
			     90000, 
			     duration,
			     audioType);
#else
    id = MP4_INVALID_TRACK_ID;
    fprintf(stderr,
	    "%s: enable ismacrypt to encrypt (--enable-ismacrypt=<path>)\n",
	    ProgName);
#endif
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
#ifdef ISMACRYP
    // encrypt if needed
     if (doEncrypt) {
      if (ismacrypEncryptSample(ismaCryptSId, blen, buf) != 0) {
	fprintf(stderr,	
		"%s: can't encrypt audio sample %u\n", ProgName, id);
      }
    }
#endif
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
  }  while (mpeg3_read_audio_frame(file, 
				   (unsigned char **)&buf, 
				   &blen,
				   &blenmax, 
				   astream) >= 0);
  return id;
}

MP4TrackId *MpegCreator (MP4FileHandle mp4file, const char *fname, bool doEncrypt)
{

  mpeg3_t *file;
  int video_streams, audio_streams;
  int ix;
  MP4TrackId *pTrackId;

  if (mpeg3_check_sig(fname) != 1) {
    return (NULL);
  }

  file = mpeg3_open(fname);
  video_streams = mpeg3_total_vstreams(file);
  audio_streams = mpeg3_total_astreams(file);

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
