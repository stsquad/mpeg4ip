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

#include <mp4av_common.h>

class CBitBuffer{
public:
	CBitBuffer(u_int8_t* pBytes, u_int32_t numBytes) {
		m_pBytes = pBytes;
		m_numBytes = numBytes;
		m_byteIndex = 0;
		m_bitIndex = 0;
	}

	u_int32_t GetBits(u_int8_t numBits);

protected:
	u_int8_t*	m_pBytes;
	u_int32_t	m_numBytes;
	u_int32_t	m_byteIndex;
	u_int8_t	m_bitIndex;
}; 

u_int32_t CBitBuffer::GetBits(u_int8_t numBits)
{
	ASSERT(m_pBytes);
	ASSERT(numBits > 0);
	ASSERT(numBits <= 32);

	u_int32_t bits = 0;

	for (u_int8_t i = numBits; i > 0; i--) {
		if (m_bitIndex == 8) {
			m_byteIndex++;
			if (m_byteIndex >= m_numBytes) {
				throw;
			}
			m_bitIndex = 0;
		}
		bits = (bits << 1) 
			| ((m_pBytes[m_byteIndex] >> (7 - m_bitIndex)) & 1);
		m_bitIndex++;
	}

	return bits;
}

bool MP4AV_Mpeg4ParseVosh(
	u_int8_t* pVoshBuf, 
	u_int32_t voshSize,
	u_int8_t* pProfileLevel)
{
	CBitBuffer vosh(pVoshBuf, voshSize);

	try {
		vosh.GetBits(32);				// start code
		*pProfileLevel = vosh.GetBits(8);
	}
	catch (...) {
		return false;
	}

	return true;
}

bool MP4AV_Mpeg4ParseVol(
	u_int8_t* pVolBuf, 
	u_int32_t volSize,
	u_int8_t* pTimeBits, 
	u_int16_t* pTimeTicks, 
	u_int16_t* pFrameDuration, 
	u_int16_t* pFrameWidth, 
	u_int16_t* pFrameHeight)
{
	CBitBuffer vol(pVolBuf, volSize);

	try {
		vol.GetBits(32);				// start code

		vol.GetBits(1);					// random accessible vol
		vol.GetBits(8);					// object type id
		u_int8_t verid = 0;
		if (vol.GetBits(1)) {			// is object layer id
			verid = vol.GetBits(4);			// object layer verid
			vol.GetBits(3);					// object layer priority
		}
		if (vol.GetBits(4) == 0xF) { 	// aspect ratio info
			vol.GetBits(8);					// par width
			vol.GetBits(8);					// par height
		}
		if (vol.GetBits(1)) {			// vol control parameters
			vol.GetBits(2);					// chroma format
			vol.GetBits(1);					// low delay
			if (vol.GetBits(1)) {			// vbv parameters
				vol.GetBits(15);				// first half bit rate
				vol.GetBits(1);					// marker bit
				vol.GetBits(15);				// latter half bit rate
				vol.GetBits(1);					// marker bit
				vol.GetBits(15);				// first half vbv buffer size
				vol.GetBits(1);					// marker bit
				vol.GetBits(3);					// latter half vbv buffer size
				vol.GetBits(11);				// first half vbv occupancy
				vol.GetBits(1);					// marker bit
				vol.GetBits(15);				// latter half vbv occupancy
				vol.GetBits(1);					// marker bit
			}
		}
		u_int8_t shape = vol.GetBits(2); // object layer shape
		if (shape == 3 /* GRAYSCALE */ && verid != 1) {
			vol.GetBits(4);					// object layer shape extension
		}
		vol.GetBits(1);					// marker bit
		*pTimeTicks = vol.GetBits(16);		// vop time increment resolution 

		u_int8_t i;
		u_int32_t powerOf2 = 1;
		for (i = 0; i < 16; i++) {
			if (*pTimeTicks < powerOf2) {
				break;
			}
			powerOf2 <<= 1;
		}
		*pTimeBits = i;

		vol.GetBits(1);					// marker bit
		if (vol.GetBits(1)) {			// fixed vop rate
			// fixed vop time increment
			*pFrameDuration = vol.GetBits(*pTimeBits); 
		} else {
			*pFrameDuration = 0;
		}
		if (shape == 0 /* RECTANGULAR */) {
			vol.GetBits(1);					// marker bit
			*pFrameWidth = vol.GetBits(13);	// object layer width
			vol.GetBits(1);					// marker bit
			*pFrameHeight = vol.GetBits(13);// object layer height
			vol.GetBits(1);					// marker bit
		} else {
			*pFrameWidth = 0;
			*pFrameHeight = 0;
		}
		// there's more, but we don't need it
	}
	catch (...) {
		return false;
	}

	return true;
}

bool MP4AV_Mpeg4ParseGov(
	u_int8_t* pGovBuf, 
	u_int32_t govSize,
	u_int8_t* pHours, 
	u_int8_t* pMinutes, 
	u_int8_t* pSeconds)
{
	CBitBuffer gov(pGovBuf, govSize);

	try {
		gov.GetBits(32);	// start code
		*pHours = gov.GetBits(5);
		*pMinutes = gov.GetBits(6);
		gov.GetBits(1);		// marker bit
		*pSeconds = gov.GetBits(6);
	}
	catch (...) {
		return false;
	}

	return true;
}

bool MP4AV_Mpeg4ParseVop(
	u_int8_t* pVopBuf, 
	u_int32_t vopSize,
	u_char* pVopType, 
	u_int8_t timeBits, 
	u_int16_t timeTicks, 
	u_int32_t* pVopTimeIncrement)
{
	CBitBuffer vop(pVopBuf, vopSize);

	try {
		vop.GetBits(32);	// skip start code

		switch (vop.GetBits(2)) {
		case 0:
			/* Intra */
			*pVopType = 'I';
			break;
		case 1:
			/* Predictive */
			*pVopType = 'P';
			break;
		case 2:
			/* Bidirectional Predictive */
			*pVopType = 'B';
			break;
		case 3:
			/* Sprite */
			*pVopType = 'S';
			break;
		}

		if (!pVopTimeIncrement) {
			return true;
		}

		u_int8_t numSecs = 0;
		while (vop.GetBits(1) != 0) {
			numSecs++;
		}
		vop.GetBits(1);		// skip marker
		u_int16_t numTicks = vop.GetBits(timeBits);
		*pVopTimeIncrement = (numSecs * timeTicks) + numTicks; 
	}
	catch (...) {
		return false;
	}

	return true;
}

// Map from ISO IEC 14496-2:2000 Appendix G 
// to ISO IEC 14496-1:2001 8.6.4.2 Table 6
u_int8_t MP4AV_Mpeg4VideoToSystemsProfileLevel(u_int8_t videoProfileLevel)
{
	switch (videoProfileLevel) {
	// Simple Profile
	case 0x01: // L1
		return 0x03;
	case 0x02: // L2
		return 0x02;
	case 0x03: // L3
		return 0x01;
	// Simple Scalable Profile
	case 0x11: // L1
		return 0x05;
	case 0x12: // L2
		return 0x04;
	// Core Profile
	case 0x21: // L1
		return 0x07;
	case 0x22: // L2
		return 0x06;
	// Main Profile
	case 0x32: // L2
		return 0x0A;
	case 0x33: // L3
		return 0x09;
	case 0x34: // L4
		return 0x08;
	// N-bit Profile
	case 0x42: // L2
		return 0x0B;
	// Scalable Texture
	case 0x51: // L1
		return 0x12;
	case 0x52: // L2
		return 0x11;
	case 0x53: // L3
		return 0x10;
	// Simple Face Animation Profile
	case 0x61: // L1
		return 0x14;
	case 0x62: // L2
		return 0x13;
	// Simple FBA Profile
	case 0x63: // L1
	case 0x64: // L2
		return 0xFE;
	// Basic Animated Texture Profile
	case 0x71: // L1
		return 0x0F;
	case 0x72: // L2
		return 0x0E;
	// Hybrid Profile
	case 0x81: // L1
		return 0x0D;
	case 0x82: // L2
		return 0x0C;
	// Advanced Real Time Simple Profile
	case 0x91: // L1
	case 0x92: // L2
	case 0x93: // L3
	case 0x94: // L4
	// Core Scalable Profile
	case 0xA1: // L1
	case 0xA2: // L2
	case 0xA3: // L3
	// Advanced Coding Efficiency Profle
	case 0xB1: // L1
	case 0xB2: // L2
	case 0xB3: // L3
	case 0xB4: // L4
	// Advanced Core Profile
	case 0xC1: // L1
	case 0xC2: // L2
	// Advanced Scalable Texture Profile
	case 0xD1: // L1
	case 0xD2: // L2
	case 0xD3: // L3
	// from draft amendments
	// Simple Studio
	case 0xE1: // L1
	case 0xE2: // L2
	case 0xE3: // L3
	case 0xE4: // L4
	// Core Studio Profile
	case 0xE5: // L1
	case 0xE6: // L2
	case 0xE7: // L3
	case 0xE8: // L4
	// Advanced Simple Profile
	case 0xF1: // L0
	case 0xF2: // L1
	case 0xF3: // L2
	case 0xF4: // L3
	case 0xF5: // L4
	// Fine Granularity Scalable Profile
	case 0xF6: // L0
	case 0xF7: // L1
	case 0xF8: // L2
	case 0xF9: // L3
	case 0xFA: // L4
	default:
		return 0xFE;
	}
}

u_char MP4AV_Mpeg4GetVopType(u_int8_t* pVopBuf, u_int32_t vopSize)
{
	u_char vopType;

	if (MP4AV_Mpeg4ParseVop(pVopBuf, vopSize, &vopType, 0, 0, NULL)) {
		return vopType;
	}

	return 0;
}

