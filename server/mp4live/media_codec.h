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

#ifndef __MEDIA_CODEC_H__
#define __MEDIA_CODEC_H__

#include "media_frame.h"
#include "media_feeder.h"
#include "media_sink.h"
#include "config_list.h"
#include "rtp_transmitter.h"
#include "media_stream.h"

// forward declarations
class CLiveConfig;
class CRtpTransmitter;
class CMediaStream;

class CMediaCodec : public CMediaFeeder, public CMediaSink {
public:
	CMediaCodec(CConfigEntry *cfg,
		    CMediaCodec *next = NULL,
		    bool realTime = true) { 
		m_pConfig = cfg;
		m_nextCodec = next;
		m_realTime = realTime;
		m_rtp_sink = NULL;
	}

	~CMediaCodec(void) {
	  if (m_rtp_sink != NULL) {
	    delete m_rtp_sink;
	    m_rtp_sink = NULL;
	  }
	};
	virtual MediaType GetFrameType(void) = 0;

	CMediaCodec *GetNext(void) { return m_nextCodec; };
	const char *GetProfileName(void) {
	  return m_pConfig->GetName();
	};
	virtual void AddRtpDestination(CMediaStream *stream,
				       uint16_t mtu,
				       bool disable_ts_offset, 
				       uint16_t max_ttl,
				       in_port_t srcPort = 0) = 0;
protected:
	virtual void StopEncoder(void) = 0;
	CMediaCodec*    m_nextCodec;
	CConfigEntry *m_pConfig;
	bool m_realTime;
	CRtpTransmitter *m_rtp_sink;
	virtual CRtpTransmitter *CreateRtpTransmitter(uint16_t mtu,
						      bool disable_ts_offset) = 0;
	void AddRtpDestInt(uint16_t mtu,
			   bool disable_ts_offset, 
			   uint16_t max_ttl,
			   const char *dest_addr, 
			   in_port_t destPort,
			   in_port_t srcPort) {
	  if (m_rtp_sink == NULL) {
	    m_rtp_sink = CreateRtpTransmitter(mtu, disable_ts_offset);
	    m_rtp_sink->StartThread();
	    AddSink(m_rtp_sink);
	  }
	  m_rtp_sink->AddRtpDestination(dest_addr, 
					destPort,
					srcPort,
					max_ttl);
	};

};

#endif /* __MEDIA_CODEC_H__ */

