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

#ifndef __RTPHINT_INCLUDED__
#define __RTPHINT_INCLUDED__

typedef struct {
	u_int16_t entryCount;
	u_int16_t reserved;
	/* entryCount of quicktime_rtp_packet_entry follow */
} quicktime_rtp_sample_t;

typedef struct {
	u_int32_t relativeXmitTime;
	u_int16_t headerInfo;
	u_int16_t seqNum;
	u_int16_t flags;
	u_int16_t numDataEntries;

	/* 
	 * if X bit is set in flags,
	 *   u_int32_t tlvTableSize 
	 *   tlvTableEntries
	 */

	/* 
	 * numDataEntries of quicktime_rtp_data_entry follows 
	 */

} quicktime_rtp_packet_entry_t;

typedef struct {
	u_int32_t size;
	char id[4];
	/* size - 8 bytes follow */
	u_char data;	/* convience placeholder */
	/* N.B. don't use sizeof(quicktime_rtp_tlv_entry_t) */
} quicktime_rtp_tlv_entry_t;

typedef struct {
	u_int8_t source;
	u_int8_t padding[15];
} quicktime_rtp_null_data_entry_t;

typedef struct {
	u_int8_t source;
	u_int8_t length;
	u_int8_t data[14];
} quicktime_rtp_immed_data_entry_t;

typedef struct {
	u_int8_t source;
	u_int8_t trackId;
	u_int16_t length;
	u_int32_t fromSampleNum;
	u_int32_t offset;
	u_int16_t bytesPerCompressionBlock;
	u_int16_t samplesPerCompressionBlock;
} quicktime_rtp_sample_data_entry_t;

typedef union {
	quicktime_rtp_null_data_entry_t		null;
	quicktime_rtp_immed_data_entry_t	immed;
	quicktime_rtp_sample_data_entry_t	sample;
} quicktime_rtp_data_entry_t;

typedef struct {
	u_int32_t trpy;
	u_int32_t nump;
	u_int32_t tpyl;
	u_int32_t dmed;
	u_int32_t dimm;
	u_int32_t drep;
	int32_t tmin;
	int32_t tmax;
	u_int32_t pmax;
} quicktime_hint_info_t;

#define RTP_HEADER_STD_SIZE 12

#endif /* __RTPHINT_INCLUDED__ */
