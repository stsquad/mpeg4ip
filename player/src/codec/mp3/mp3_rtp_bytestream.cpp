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
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * mp3_rtp_bytestream.h - provides an RTP bytestream for the codecs
 * to access
 */
#include "mp3_rtp_bytestream.h"

CMP3RtpByteStream::CMP3RtpByteStream (unsigned int rtp_proto,
				      int ondemand,
				      uint64_t tps,
				      rtp_packet **head, 
				      rtp_packet **tail,
				      int rtpinfo_received,
				      uint32_t rtp_rtptime,
				      int rtcp_received,
				      uint32_t ntp_frac,
				      uint32_t ntp_sec,
				      uint32_t rtp_ts) :
  CRtpByteStream("mp3", 
		 rtp_proto,
		     ondemand,
		     tps,
		     head, 
		     tail,
		     rtpinfo_received,
		     rtp_rtptime,
		     rtcp_received,
		     ntp_frac,
		     ntp_sec,
		     rtp_ts)
{
  set_skip_on_advance(4);
  init();
}

CMP3RtpByteStream::~CMP3RtpByteStream(void)
{
}

int CMP3RtpByteStream::have_no_data (void)
{
  if (m_head == NULL) return TRUE;
  return FALSE;
}

