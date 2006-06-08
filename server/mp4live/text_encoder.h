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
 * Copyright (C) Cisco Systems Inc. 2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May wmay@cisco.com
 */

#ifndef __TEXT_ENCODER_H__
#define __TEXT_ENCODER_H__

#include "media_codec.h"
#include "media_frame.h"
#include <sdp.h>
#include <mp4.h>
#include "encoder_gui_options.h"
#include "profile_text.h"

class CTextEncoder : public CMediaCodec {
 public:
  CTextEncoder(CTextProfile *profile,
	       uint16_t mtu,
	       CTextEncoder *next,
	       bool realTime = true);

  virtual bool Init(void) = 0;

  void AddRtpDestination(CMediaStream *stream,
			 bool disable_ts_offset,
			 uint16_t max_ttl,
			 in_port_t srcPort = 0);

  CTextEncoder *GetNext(void) {
    return (CTextEncoder *)CMediaCodec::GetNext();
  };
 protected:
  int ThreadMain(void);
  CTextProfile *Profile(void) { return (CTextProfile *)m_pConfig; } ;

  void Initialize(void);

  virtual bool EncodeFrame(const char *fptr) = 0;

  virtual bool GetEncodedFrame(void **ppBuffer, 
			       u_int32_t* pBufferLength) = 0;

  CRtpTransmitter *CreateRtpTransmitter(bool disable_ts) {
    return new CTextRtpTransmitter(Profile(), m_mtu, disable_ts);
  };

  void ProcessTextFrame(CMediaFrame *frame);
  void SendFrame(Timestamp t);
  void DoStopText();

  // 
  MediaType		m_textDstType;
  Timestamp		m_textStartTimestamp;
};


CTextEncoder* TextEncoderCreate(CTextProfile *ap, 
				uint16_t mtu,
				CTextEncoder *next,
				bool realTime = true);

media_desc_t *create_text_sdp(CTextProfile *pConfig);

MediaType get_text_mp4_fileinfo(CTextProfile *pConfig, const char **base_url);

void create_mp4_text_hint_track(CTextProfile *pConfig, 
				MP4FileHandle mp4file,
				MP4TrackId trackId,
				uint16_t mtu);


rtp_transmitter_f GetTextRtpTransmitRoutine(CTextProfile *pConfig, 
					    MediaType *pType, 
					    uint8_t *pPayload);

typedef struct text_encoder_table_t {
  char *dialog_selection_name;
  char *text_encoding;
  get_gui_options_list_f get_gui_options;
} text_encoder_table_t;

extern text_encoder_table_t text_encoder_table[];
extern uint32_t text_encoder_table_size;

void AddTextProfileEncoderVariables(CTextProfile *pConfig);

#endif /* __TEXT_ENCODER_H__ */

