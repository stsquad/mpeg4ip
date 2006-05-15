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
 *		Bill May 		wmay@cisco.com
 *		Peter Maersk-Moller	peter@maersk-moller.net
 */

#ifndef __RTP_TRANSMITTER_H__
#define __RTP_TRANSMITTER_H__

#include <rtp/rtp.h>
#include "srtp/liblibsrtp.h"
#include "media_sink.h"

typedef struct mp4live_rtp_params_t {
  rtp_stream_params_t rtp_params;
  srtp_if_params_t srtp_params;
  bool use_srtp;
  uint auth_len;
} mp4live_rtp_params_t;

// *****************************************************************************
// Flags set by audio_queue_frame()
// Unless DROP_IT or IS_JUMBO is set, then frame will be added to queue before
// the flag SEND_NOW is checked. This is how it should be understood:
//
//      if DROP_IT return;
//      if SEND_FIRST { send(queue); empty(queue); }
//      if IS_JUMBO { sendjumbo(frame); return; }
//      add_to_queue(frame);
//      if SEND_NOW { send(queue); empty(queue); }
//
// *****************************************************************************
#define NO_OP           0               // No op except just add frame to queue
#define DROP_IT         1               // Drop frame on the spot
#define SEND_FIRST      2               // Send queue first before anything else
#define IS_JUMBO        4               // Send jumbo and return
#define SEND_NOW        8               // Send queue after frame has been added
// *****************************************************************************

class CRtpDestination
{
 public:
  CRtpDestination(mp4live_rtp_params_t *rtp_params, 
		  uint8_t payloadNumber);
  CRtpDestination(struct rtp *session, uint8_t payloadNumber);
  virtual ~CRtpDestination();
  void start(void);
  void send_rtcp(u_int32_t rtpTimestamp,
		 u_int64_t ntpTimestamp);
  int send_iov(struct iovec *piov,
	       uint iovCount,
	       u_int32_t rtpTimestamp,
	       int mbit);
  CRtpDestination *get_next(void) { return m_next;};
  void set_next (CRtpDestination *p) { m_next = p; };
  void add_reference (void) {
    SDL_LockMutex(m_ref_mutex);
    m_reference++;
    SDL_UnlockMutex(m_ref_mutex);
  };
  bool remove_reference (void) {
    bool ret;
    SDL_LockMutex(m_ref_mutex);
    if (m_reference > 0) 
      m_reference--;
    ret = m_reference == 0;
    SDL_UnlockMutex(m_ref_mutex);
    return ret;
  };
  bool Matches (const char *dest_addr, 
		in_port_t destPort) {
    if (destPort != m_rtp_params->rtp_params.rtp_tx_port) return false;
    return strcasecmp(dest_addr, m_rtp_params->rtp_params.rtp_addr) == 0;
  };
  in_port_t get_source_port (void) { return m_rtp_params->rtp_params.rtp_rx_port; };

  virtual const char* name() {
    return "CRtpTransmitter";
  }
 protected:
  SDL_mutex *m_ref_mutex;
  CRtpDestination *m_next;
  mp4live_rtp_params_t *m_rtp_params;
  //  uint32_t m_timeScale;
  uint8_t m_payloadNumber;
  struct rtp *m_rtpSession;
  srtp_if_t *m_srtpSession;
  uint32_t m_reference;
  bool m_destroy_rtp_session;
};

typedef void (*rtp_transmitter_f)(CMediaFrame *pak, CRtpDestination *list,
					uint32_t rtpTimestamp, uint16_t mtu);

typedef int (*audio_queue_frame_f)(u_int32_t **frameno,
					u_int32_t frameLength,
					u_int8_t audioQueueCount,
					u_int16_t audioQueueSize,
					u_int32_t rtp_payload_size);
typedef int (*audio_set_rtp_payload_f)(CMediaFrame** m_audioQueue,
				       int queue_cnt,
				       struct iovec *iov,
				       void *ud,
				       bool *mbit);

typedef bool (*audio_set_rtp_header_f)(struct iovec *iov,
				       int queue_cnt,
				       void *ud, 
				       bool *mbit);

typedef bool (*audio_set_rtp_jumbo_frame_f)(struct iovec *iov,
					    uint32_t dataOffset,
					    uint32_t bufferLen,
					    uint32_t rtpPayloadMax,
					    bool &mbit,
					    void *ud);
#define DEFAULT_RTCP_BW (100.0)
class CRtpTransmitter : public CMediaSink {
public:
  CRtpTransmitter(uint16_t mtu, 
		  bool disable_ts_offset = false);
	virtual ~CRtpTransmitter();
	static void SeedRandom(void) {
		static bool once = false;
		if (!once) {
			srandom(time(NULL));
			once = true;
		}
	}

	static u_int32_t GetRandomMcastAddress(void) {
		SeedRandom();

		// pick a random number in the multicast range
		u_int32_t mcast = ((random() & 0x0FFFFFFF) | 0xE0000000);

		// screen out undesirable values
		// introduces small biases in the results

		// stay away from 224.0.0.x
		if ((mcast & 0x0FFFFF00) == 0) {
			mcast |= 0x00000100;	// move to 224.0.1
		} 

		// stay out of SSM range 232.x.x.x
		// user should explictly select this if they want SSM
		if ((mcast & 0xFF000000) == 232) {
			mcast |= 0x01000000;	// move to 233
		}

		// stay away from .0 .1 and .255
		if ((mcast & 0xFF) == 0 || (mcast & 0xFF) == 1 
		  || (mcast & 0xFF) == 255) {
			mcast = (mcast & 0xFFFFFFF0) | 0x4;	// move to .4 or .244
		}

		return htonl(mcast);
	}

	static u_int16_t GetRandomPortBlock(void) {
		SeedRandom();

		// Get random block of 4 port numbers above 20000
		return (u_int16_t)(20000 + ((random() >> 18) << 2));

	}
	void AddRtpDestination(mp4live_rtp_params_t *rtp_params);
	void AddRtpDestination(struct rtp *rtp_session);
	
	static const uint32_t MSG_RTP_DEST_START = 8192;
	static const uint32_t MSG_RTP_DEST_STOP  = MSG_RTP_DEST_START + 1;

	void StartRtpDestination (const char *destAddr, 
				  in_port_t destPort) {
	  m_myMsgQueue.send_message(MSG_RTP_DEST_START, 
				    destAddr, 
				    strlen(destAddr) + 1,
				    m_myMsgQueueSemaphore,
				    destPort);
	};
	void StopRtpDestination (const char *destAddr, 
				 in_port_t destPort) {
	  m_myMsgQueue.send_message(MSG_RTP_DEST_STOP,
				    destAddr, 
				    strlen(destAddr) + 1,
				    m_myMsgQueueSemaphore,
				    destPort);
	};
protected:
	int ThreadMain(void);

	void DoStartTransmit(void);
	virtual void DoStopTransmit(void);
	virtual void DoSendFrame(CMediaFrame* pFrame) = 0;

	void DoAddRtpDestinationToQueue(CRtpDestination *dest);
	void DoStartRtpDestination(const char *destAddr, in_port_t port);
	void DoStopRtpDestination(const char *destAddr, in_port_t port);

	uint32_t TimestampToRtp (Timestamp t) {
	  if (m_haveStartTimestamp) {
	    return (u_int32_t)(((t - m_startTimestamp) 
				* m_timeScale) / TimestampTicks)
	      + m_rtpTimestampOffset;
	  }
	  m_startTimestamp = t;
	  m_haveStartTimestamp = true;
	  return m_rtpTimestampOffset;
	};

	static const u_int32_t SECS_BETWEEN_1900_1970 = 2208988800U;

	u_int64_t TimestampToNtp(Timestamp t) {
		// low order ntp 32 bits is 2 ^ 32 -1 ticks per sec
		register u_int32_t usecs = t % TimestampTicks;
		uint64_t ret;

		ret = (t / TimestampTicks) + SECS_BETWEEN_1900_1970;
		ret <<= 32;
		ret |= ((usecs << 12) + (usecs << 8) - ((usecs * 3650) >> 6));
		return ret;
	}

	static void RtpCallback(struct rtp *session, rtp_event *e) {
		// Currently we ignore RTCP packets
		// Just do our required housekeeping
		if (e && e->type == RX_RTP) {
			free(e->data);
		}
	}

protected:
	bool m_haveStartTimestamp;
	Timestamp		m_startTimestamp;

	SDL_mutex *m_destListMutex;
	CRtpDestination  *m_rtpDestination;
	MediaType               m_frameType;
	uint32_t m_timeScale;
	uint32_t m_rtpTimestampOffset;
	uint16_t m_mtu;
	uint16_t m_original_mtu;
	u_int8_t m_payloadNumber;

};

class CAudioProfile;
class CAudioRtpTransmitter : public CRtpTransmitter
{
 public:
  CAudioRtpTransmitter(CAudioProfile *ap, 
		       uint16_t mtu, 
		       bool disable_ts_offset);
  ~CAudioRtpTransmitter();
 protected:
  void DoSendFrame(CMediaFrame* pFrame);
  void DoStopTransmit(void);

  void SendAudioFrame(CMediaFrame* pFrame);
  void OldSendAudioFrame(CMediaFrame* pFrame);
  void SendAudioJumboFrame(CMediaFrame* pFrame);
  void SendQueuedAudioFrames(void);

  CAudioProfile *m_audio_profile;
  u_int32_t		m_audioRtpTimestampOffset;
  u_int16_t		m_audioSrcPort;
  u_int8_t		m_audioPayloadBytesPerPacket;
  u_int8_t		m_audioPayloadBytesPerFrame;
  u_int8_t		m_audioiovMaxCount;		// max number of iov segments
  u_int32_t		*m_frameno;			// We need to count the frames
  audio_queue_frame_f	m_audio_queue_frame;		// plugin function for queing
  audio_set_rtp_payload_f	m_audio_set_rtp_payload;	// plugin function for rtp packing
  audio_set_rtp_header_f  m_audio_set_rtp_header;
  audio_set_rtp_jumbo_frame_f m_audio_set_rtp_jumbo_frame;
  void *m_audio_rtp_userdata;
  CMediaFrame**	m_audioQueue;
  u_int8_t		m_audioQueueCount;	// number of frames
  u_int8_t		m_audioQueueMaxCount;	// max number of frames
  u_int16_t		m_audioQueueSize;	// bytes for RTP packet payload
  uint32_t m_nextAudioRtpTimestamp;
};

class CVideoProfile;

class CVideoRtpTransmitter : public CRtpTransmitter
{
 public:
  CVideoRtpTransmitter(CVideoProfile *vp, 
		       uint16_t mtu, bool disable_ts_offset);
 protected:
  void DoSendFrame(CMediaFrame* pFrame);
  //  void DoStopTransmit(void); not needed as of now
  
  rtp_transmitter_f m_videoSendFunc;
};

class CTextProfile;
class CTextRtpTransmitter : public CRtpTransmitter
{
 public:
  CTextRtpTransmitter(CTextProfile *tp, 
		      uint16_t mtu, bool disable_ts_offset);
 protected:
  void DoSendFrame(CMediaFrame *pFrame);
  rtp_transmitter_f m_textSendFunc;
};
  
#endif /* __RTP_TRANSMITTER_H__ */
