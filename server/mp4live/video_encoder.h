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
 *		Dave Mackie		dmackie@cisco.com
 */

#ifndef __VIDEO_ENCODER_H__
#define __VIDEO_ENCODER_H__

#include "media_codec.h"
#include "media_frame.h"
#include <sdp.h>
#include <mp4.h>
#include "profile_video.h"
#include "media_sink.h"
#include "media_feeder.h"
#include "video_util_resize.h"
#include "encoder_gui_options.h"

class CTimestampPush {
 public:
  CTimestampPush(uint max) {
    m_in = 0;
    m_out = 0;
    m_max = max;
    m_stack = (Timestamp *)malloc(max * sizeof(Timestamp));
  };
  ~CTimestampPush() {
    free(m_stack);
  };
  void Push (Timestamp t) {
    m_stack[m_in] = t;
    m_in++;
    if (m_in >= m_max) m_in = 0;
  };
  Timestamp Pop (void) {
    Timestamp ret;
    ret = m_stack[m_out];
    m_out++;
    if (m_out >= m_max) m_out = 0;
    return ret;
  };
  Timestamp Closest (Timestamp compare, Duration frame_time) {
    uint ix = m_out;
    frame_time /= 2;
    Duration neg = 0 - frame_time;
    do {
      Duration diff = compare - m_stack[ix];
      if (diff < frame_time && diff > neg) {
	return m_stack[ix];
      }
      ix++;
      if (ix >= m_max) ix = 0;
    } while (ix != m_in);
    return 0;
  };
 private:
  Timestamp *m_stack;
  uint m_in, m_out, m_max;
};

typedef enum VIDEO_FILTERS {
  VF_NONE,
  VF_DEINTERLACE,
  VF_FFMPEG_DEINTERLACE_INPLACE,
} VIDEO_FILTERS;

class CVideoEncoder : public CMediaCodec {
public:
  CVideoEncoder(CVideoProfile *vp,
		uint16_t mtu,
		CVideoEncoder *next,
		bool realTime); 
  
  virtual bool Init(void) = 0;
  void StartPreview(void) { m_preview = true; } ;
  void StopPreview(void) { m_preview = false; } ;
 public:
  virtual bool CanGetEsConfig (void) { return false; };
  virtual bool GetEsConfig(uint8_t **ppEsConfig,
			   uint32_t *pEsConfigLen) { return false; };
  void AddRtpDestination(CMediaStream *stream,
			 bool disable_ts_offset, 
			 uint16_t max_ttl,
			 in_port_t srcPort = 0);
  CVideoEncoder *GetNext(void) {
    return (CVideoEncoder *)CMediaCodec::GetNext();
  };
  uint32_t GetEncodedFrames (void) {
    return m_videoDstFrameNumber;
  };

 protected:
  // all the stuff from media
  int ThreadMain(void);
  CVideoProfile *Profile(void) { return (CVideoProfile *)m_pConfig; } ;

  CRtpTransmitter *CreateRtpTransmitter(bool disable_ts_offset) {
    return new CVideoRtpTransmitter(Profile(), m_mtu, disable_ts_offset);
  };

  // encoder specific routines.

  virtual bool EncodeImage(
			   const u_int8_t* pY, 
			   const u_int8_t* pU, 
			   const u_int8_t* pV,
			   u_int32_t yStride, u_int32_t uvStride,
			   bool wantKeyFrame,
			   Duration elapsedDuration,
			   Timestamp srcFrameTimestamp) = 0;
  
  virtual bool GetEncodedImage(
			       u_int8_t** ppBuffer, u_int32_t* pBufferLength,
			       Timestamp *dts, Timestamp *pts) = 0;
  
  virtual bool GetReconstructedImage(
				     u_int8_t* pY, u_int8_t* pU, u_int8_t* pV) = 0;
  virtual media_free_f GetMediaFreeFunction(void) { return NULL; };

  // processing routines
  void ProcessVideoYUVFrame(CMediaFrame *frame);
  void SetVideoSrcSize(
		       u_int16_t srcWidth,
		       u_int16_t srcHeight,
		       u_int16_t srcStride,
		       bool matchAspectRatios);
  
  void DestroyVideoResizer();
  
  void DoStopVideo();
  inline Duration VideoDstFramesToDuration(void) {
    double tempd;
    tempd = m_videoDstFrameNumber;
    tempd *= TimestampTicks;
    tempd /= m_videoDstFrameRate;
    return (Duration) tempd;
  };
  
  u_int32_t		m_videoSrcFrameNumber;
  u_int16_t		m_videoSrcWidth;
  u_int16_t		m_videoSrcHeight;
  u_int16_t		m_videoSrcAdjustedHeight;
  float			m_videoSrcAspectRatio;
  u_int32_t		m_videoSrcYUVSize;
  u_int32_t		m_videoSrcYSize;
  u_int16_t		m_videoSrcYStride;
  u_int32_t		m_videoSrcUVSize;
  u_int16_t		m_videoSrcUVStride;
  u_int32_t		m_videoSrcYCrop;
  u_int32_t		m_videoSrcUVCrop;

  // video destination info
  VIDEO_FILTERS         m_videoFilter;
  MediaType		m_videoDstType;
  float			m_videoDstFrameRate;
  Duration		m_videoDstFrameDuration;
  u_int32_t		m_videoDstFrameNumber;
  u_int16_t		m_videoDstWidth;
  u_int16_t		m_videoDstHeight;
  float			m_videoDstAspectRatio;
  u_int32_t		m_videoDstYUVSize;
  u_int32_t		m_videoDstYSize;
  u_int32_t		m_videoDstUVSize;

  // video resizing info
  bool			m_videoMatchAspectRatios;
  image_t*		m_videoSrcYImage;
  image_t*		m_videoDstYImage;
  scaler_t*		m_videoYResizer;
  image_t*		m_videoSrcUVImage;
  image_t*		m_videoDstUVImage;
  scaler_t*		m_videoUVResizer;

  // video encoding info
  bool			m_videoWantKeyFrame;

  // video timing info
  Timestamp		m_videoStartTimestamp;
  //Duration		m_videoEncodingDrift;
  //Duration		m_videoEncodingMaxDrift;
  Duration		m_videoSrcElapsedDuration;
  Duration		m_videoDstElapsedDuration;

  // video previous frame info
  u_int8_t*		m_videoDstPrevImage;
  u_int8_t*		m_videoDstPrevReconstructImage;
  bool m_preview;
};

void VideoProfileCheck(CVideoProfile *vp);
CVideoEncoder* VideoEncoderCreate(CVideoProfile *vp, 
				  uint16_t mtu,
				  CVideoEncoder *next, 
				  bool realTime = true);

void AddVideoProfileEncoderVariables(CVideoProfile *pConfig);

MediaType get_video_mp4_fileinfo(CVideoProfile *pConfig,
				 bool *createIod,
				 bool *isma_compliant,
				 uint8_t *videoProfile,
				 uint8_t **videoConfig,
				 uint32_t *videoConfigLen,
				 uint8_t *mp4_video_type);

media_desc_t *create_video_sdp(CVideoProfile *pConfig,
			       bool *createIod,
			       bool *isma_compliant,
			       bool *is3gp,
			       uint8_t *videoProfile,
			       uint8_t **videooConfig,
			       uint32_t *videoConfigLen,
			       uint8_t *payload_number);

void create_mp4_video_hint_track(CVideoProfile *pConfig,
				  MP4FileHandle mp4file,
				  MP4TrackId trackId,
				 uint16_t mtu);

class CRtpDestination;


rtp_transmitter_f GetVideoRtpTransmitRoutine(CVideoProfile *vp,
					     MediaType *pType,
					     uint8_t *pPayload);


typedef struct video_encoder_table_t {
  const char *encoding_name;
  const char *encoding;
  const char *encoder;
  uint16_t numSizesNTSC;
  uint16_t numSizesPAL;
  uint16_t numSizesSecam;
  uint16_t *widthValuesNTSC;
  uint16_t *widthValuesPAL;
  uint16_t *widthValuesSecam;
  uint16_t *heightValuesNTSC;
  uint16_t *heightValuesPAL;
  uint16_t *heightValuesSecam;
  const char **sizeNamesNTSC;
  const char **sizeNamesPAL;
  const char **sizeNamesSecam;
  get_gui_options_list_f get_gui_options;
} video_encoder_table_t;

extern const video_encoder_table_t video_encoder_table[];
extern const uint32_t video_encoder_table_size;

#endif /* __VIDEO_ENCODER_H__ */

