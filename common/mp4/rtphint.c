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

#include "quicktime.h"


quicktime_rtp_sample_t* quicktime_get_hint_sample(u_char* hintBuf)
{
	return (quicktime_rtp_sample_t*)hintBuf;
}

quicktime_rtp_packet_entry_t* quicktime_get_hint_last_packet_entry(
	u_char* hintBuf)
{
	quicktime_rtp_sample_t* sample = quicktime_get_hint_sample(hintBuf);
	u_int16_t numPacketEntries = ntohs(sample->entryCount);
	u_char* bufPtr = hintBuf + sizeof(quicktime_rtp_sample_t);	
	int i;

	if (numPacketEntries == 0) {
		return NULL;
	}

	for (i = 0; i < numPacketEntries - 1; i++) {
		bufPtr += quicktime_get_packet_size(bufPtr);
	}

	return (quicktime_rtp_packet_entry_t*)bufPtr;
}

void quicktime_init_hint_sample(u_char* hintBuf, u_int* pHintBufSize)
{
	quicktime_rtp_sample_t* sample = quicktime_get_hint_sample(hintBuf);

	sample->entryCount = htons(0);
	sample->reserved = htons(0);

	(*pHintBufSize) = sizeof(quicktime_rtp_sample_t);
}

void quicktime_add_hint_packet(u_char* hintBuf, u_int* pHintBufSize, 
	u_int payloadNumber, u_int rtpSeqNum)
{
	quicktime_rtp_sample_t* sample = quicktime_get_hint_sample(hintBuf);
	quicktime_rtp_packet_entry_t* packetEntry = 
		(quicktime_rtp_packet_entry_t*)(hintBuf + (*pHintBufSize));

	sample->entryCount = htons(ntohs(sample->entryCount) + 1);

	packetEntry->relativeXmitTime = htonl(0);
	packetEntry->headerInfo = htons(payloadNumber);
	packetEntry->seqNum = htons(rtpSeqNum);
	packetEntry->flags = htons(0);
	packetEntry->numDataEntries = htons(0);

	(*pHintBufSize) += sizeof(quicktime_rtp_packet_entry_t);
}

void quicktime_set_rtp_hint_timestamp_offset(u_char* hintBuf, 
	u_int* pHintBufSize, int offset)
{
	quicktime_rtp_packet_entry_t* packetEntry = 
		quicktime_get_hint_last_packet_entry(hintBuf);
	u_int32_t* pTlvTableSize = (u_int32_t*)
		((u_char*)packetEntry + sizeof(quicktime_rtp_packet_entry_t));
	quicktime_rtp_tlv_entry_t* tlvEntry = (quicktime_rtp_tlv_entry_t*)
		((u_char*)pTlvTableSize + sizeof(u_int32_t));

	/* nothing to do */
	if (offset == 0) {
		return;
	}

	if (ntohs(packetEntry->numDataEntries)) {
#ifdef DEBUG
		printf("OOPS, have already added data to this hint packet\n");
#endif
		return;
	}

	packetEntry->flags |= htons(1 << 2); 	/* signal presence of TLV */

	*pTlvTableSize = htonl(16);

	/* the TLV entry */
	tlvEntry->size = htonl(12);
	memcpy(tlvEntry->id, "rtpo", sizeof(tlvEntry->id));
	*((int32_t*)&tlvEntry->data) = htonl(offset);

	(*pHintBufSize) += 16;
}

void quicktime_add_hint_immed_data(u_char* hintBuf, u_int* pHintBufSize, 
					u_char* data, u_int length)
{
	quicktime_rtp_packet_entry_t* packetEntry = 
		quicktime_get_hint_last_packet_entry(hintBuf);
	quicktime_rtp_immed_data_entry_t* dataEntry = 
		(quicktime_rtp_immed_data_entry_t*)(hintBuf + (*pHintBufSize));

	dataEntry->source = 1;
	dataEntry->length = MIN(length, sizeof(dataEntry->data));
	memcpy(dataEntry->data, data, dataEntry->length);

	packetEntry->numDataEntries = htons(ntohs(packetEntry->numDataEntries) + 1);
	(*pHintBufSize) += sizeof(quicktime_rtp_immed_data_entry_t);
}

void quicktime_add_hint_sample_data(u_char* hintBuf, u_int* pHintBufSize, 
					u_int fromSampleNum, u_int offset, u_int length)
{
	quicktime_rtp_packet_entry_t* packetEntry = 
		quicktime_get_hint_last_packet_entry(hintBuf);
	quicktime_rtp_sample_data_entry_t* dataEntry = 
		(quicktime_rtp_sample_data_entry_t*)(hintBuf + (*pHintBufSize));

	dataEntry->source = 2;
	dataEntry->trackId = 0;
	dataEntry->length = htons(length);
	dataEntry->fromSampleNum = htonl(fromSampleNum);
	dataEntry->offset = htonl(offset);
	dataEntry->bytesPerCompressionBlock = htons(0);
	dataEntry->samplesPerCompressionBlock = htons(0);

	packetEntry->numDataEntries = htons(ntohs(packetEntry->numDataEntries) + 1);
	(*pHintBufSize) += sizeof(quicktime_rtp_sample_data_entry_t);
}

void quicktime_set_hint_Mbit(u_char* hintBuf)
{
	quicktime_rtp_packet_entry_t* packetEntry = 
		quicktime_get_hint_last_packet_entry(hintBuf);
	packetEntry->headerInfo |= htons(1 << 7);
}

void quicktime_set_hint_Bframe(u_char* hintBuf)
{
	quicktime_rtp_packet_entry_t* packetEntry = 
		quicktime_get_hint_last_packet_entry(hintBuf);
	packetEntry->flags |= htons(1 << 1); 
}

void quicktime_set_hint_repeat(u_char* hintBuf)
{
	quicktime_rtp_packet_entry_t* packetEntry = 
		quicktime_get_hint_last_packet_entry(hintBuf);
	packetEntry->flags |= htons(1 << 0);
}
	
void quicktime_set_hint_repeat_offset(u_char* hintBuf, u_int32_t offset)
{
	quicktime_rtp_packet_entry_t* packetEntry = 
		quicktime_get_hint_last_packet_entry(hintBuf);
	packetEntry->relativeXmitTime = offset;
}

int quicktime_dump_hint_sample(u_char* hintBuf)
{
	quicktime_rtp_sample_t* sample = quicktime_get_hint_sample(hintBuf);
	int i, numPacketEntries;
	u_char* bufPtr;

	fprintf(stdout, " entryCount %u\n", ntohs(sample->entryCount));
	fprintf(stdout, " reserved %u\n", ntohs(sample->reserved));

	/* entryCount of quicktime_rtp_packet_entry_t follow */

	numPacketEntries = ntohs(sample->entryCount);
	bufPtr = hintBuf + sizeof(quicktime_rtp_sample_t);	

	for (i = 0; i < numPacketEntries; i++) {
		fprintf(stdout, " packet %u\n", i + 1);
		bufPtr += quicktime_dump_hint_packet(bufPtr);
	}

	/* return number of bytes dumped */
	return (bufPtr - hintBuf);
}

int quicktime_dump_hint_packet(u_char* hintBuf)
{
	quicktime_rtp_packet_entry_t* packetEntry = 
		(quicktime_rtp_packet_entry_t*)hintBuf;
	u_char* bufPtr;
	int i;

	fprintf(stdout, "  relativeXmitTime %lu\n", ntohl(packetEntry->relativeXmitTime));
	fprintf(stdout, "  headerInfo %x\n", packetEntry->headerInfo);
	fprintf(stdout, "  seqNum %u\n", ntohs(packetEntry->seqNum));
	fprintf(stdout, "  flags %x\n", ntohs(packetEntry->flags));
	fprintf(stdout, "  numDataEntries %u\n", ntohs(packetEntry->numDataEntries));

	bufPtr = hintBuf + sizeof(quicktime_rtp_packet_entry_t);

	/* if X bit is set in flags, TLV entries exist */
	if (packetEntry->flags & htons(1 << 2)) {
		u_char* tlvTableStart = bufPtr;

		u_int32_t tlvTableSize = ntohl(*(u_int32_t*)bufPtr);
		bufPtr += sizeof(u_int32_t);

		fprintf(stdout, "  tlvTableSize %u\n", tlvTableSize);

		i = 1;
		do {
			fprintf(stdout, "  tlvEntry %u\n", i);
			bufPtr += quicktime_dump_hint_tlv(bufPtr);
			i++;
		} while (bufPtr < tlvTableStart + tlvTableSize);
	}

	/* numDataEntries of qt_rtp_data_entry follow */
	for (i = 0; i < ntohs(packetEntry->numDataEntries); i++) {
		fprintf(stdout, "  dataEntry %u\n", i + 1);
		bufPtr += quicktime_dump_hint_data(bufPtr);
	}

	/* return number of bytes dumped */
	return (bufPtr - hintBuf);
}

int quicktime_dump_hint_tlv(u_char* hintBuf)
{
	quicktime_rtp_tlv_entry_t* tlvEntry = 
		(quicktime_rtp_tlv_entry_t*)hintBuf;

	fprintf(stdout, "   id %.4s\n", tlvEntry->id);
	if (!memcmp(tlvEntry->id, "rtpo", 4)) {
		fprintf(stdout, "   offset %d\n", ntohl(*(u_int32_t*)&tlvEntry->data));
	}

	return ntohl(tlvEntry->size);
}

int quicktime_dump_hint_data(u_char* hintBuf)
{
	quicktime_rtp_data_entry_t* dataEntry = 
		(quicktime_rtp_data_entry_t*)hintBuf;

	fprintf(stdout, "   source %u\n", dataEntry->null.source);

	if (dataEntry->null.source == 1) {
		u_int i;

		fprintf(stdout, "   length %u\n", dataEntry->immed.length);
		fprintf(stdout, "   data ");
		for (i = 0; i < MIN(dataEntry->immed.length, sizeof(dataEntry->immed.data)); i++) {
			fprintf(stdout, "%x ", dataEntry->immed.data[i]);
		}
		fprintf(stdout, "\n");
	} else if (dataEntry->null.source == 2) {
		fprintf(stdout, "   trackId %u\n", dataEntry->sample.trackId);
		fprintf(stdout, "   length %u\n", ntohs(dataEntry->sample.length));
		fprintf(stdout, "   fromSampleNum %u\n", ntohl(dataEntry->sample.fromSampleNum));
		fprintf(stdout, "   offset %u\n", ntohl(dataEntry->sample.offset));
		fprintf(stdout, "   bytesPerCompressionBlock %u\n", ntohs(dataEntry->sample.bytesPerCompressionBlock));
		fprintf(stdout, "   samplesPerCompressionBlock %u\n", ntohs(dataEntry->sample.samplesPerCompressionBlock));
	} 

	/* return number of bytes dumped */
	return sizeof(quicktime_rtp_data_entry_t); 
}

int quicktime_get_hint_size(u_char* hintBuf)
{
	quicktime_rtp_sample_t* sample = quicktime_get_hint_sample(hintBuf);
	u_int16_t numPacketEntries = ntohs(sample->entryCount);
	u_char* bufPtr = hintBuf + sizeof(quicktime_rtp_sample_t);	
	int i;

	for (i = 0; i < numPacketEntries; i++) {
		bufPtr += quicktime_get_packet_size(bufPtr);
	}

	return (bufPtr - hintBuf);
}

int quicktime_get_packet_entry_size(u_char* hintBuf)
{
	quicktime_rtp_packet_entry_t* packetEntry = 
		(quicktime_rtp_packet_entry_t*)hintBuf;
	u_char* bufPtr = hintBuf + sizeof(quicktime_rtp_packet_entry_t);

	if (packetEntry->flags & htons(1 << 2)) {
		u_int32_t tlvTableSize = ntohl(*(u_int32_t*)bufPtr);
		bufPtr += tlvTableSize;
	}

	return (bufPtr - hintBuf);
}

int quicktime_get_packet_size(u_char* hintBuf)
{
	quicktime_rtp_packet_entry_t* packetEntry = 
		(quicktime_rtp_packet_entry_t*)hintBuf;
	u_int numDataEntries = ntohs(packetEntry->numDataEntries);

	return quicktime_get_packet_entry_size(hintBuf) 
		+ (numDataEntries * sizeof(quicktime_rtp_data_entry_t));
}

int quicktime_get_hint_info(u_char* hintBuf, u_int hintBufSize, quicktime_hint_info_t* pHintInfo)
{
	quicktime_rtp_sample_t* sample = quicktime_get_hint_sample(hintBuf);
	u_int16_t numPacketEntries = ntohs(sample->entryCount);
	u_char* bufPtr = hintBuf + sizeof(quicktime_rtp_sample_t);	
	int i, j;

	memset(pHintInfo, 0, sizeof(quicktime_hint_info_t));

	pHintInfo->nump = numPacketEntries;

	for (i = 0; i < numPacketEntries; i++) {
		quicktime_rtp_packet_entry_t* packetEntry = 
			(quicktime_rtp_packet_entry_t*)bufPtr;
		u_int numDataEntries = ntohs(packetEntry->numDataEntries);
		u_int32_t rtpPacketLength = 0;

		pHintInfo->tmin = MIN(pHintInfo->tmin,
			(int32_t)ntohl(packetEntry->relativeXmitTime));
		pHintInfo->tmax = MAX(pHintInfo->tmax,
			(int32_t)ntohl(packetEntry->relativeXmitTime));

		bufPtr += quicktime_get_packet_entry_size(bufPtr);

		for (j = 0; j < numDataEntries; j++) {
			quicktime_rtp_data_entry_t* dataEntry =
				(quicktime_rtp_data_entry_t*)bufPtr; 
			u_int16_t dataLength = 0;

			if (dataEntry->null.source == 1) {
				/* immediate data */
				dataLength = dataEntry->immed.length;
				pHintInfo->dimm += dataLength;
			} else if (dataEntry->null.source == 2) {
				/* sample data */
				dataLength = ntohs(dataEntry->sample.length);
				pHintInfo->dmed += dataLength;
			}
			rtpPacketLength += dataLength;

			bufPtr += sizeof(quicktime_rtp_data_entry_t);
		}

		pHintInfo->trpy += RTP_HEADER_STD_SIZE + rtpPacketLength;
		pHintInfo->tpyl += rtpPacketLength;
		if (ntohs(packetEntry->flags) & 0x80) {
			/* repeated data */
			pHintInfo->drep += rtpPacketLength;
		}
		pHintInfo->pmax = MAX(pHintInfo->pmax, 
			RTP_HEADER_STD_SIZE + rtpPacketLength);
	}
}
