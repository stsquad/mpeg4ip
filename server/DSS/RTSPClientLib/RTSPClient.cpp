/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
	File:		RTSPClient.cpp

	Contains:	.  
					
	
*/

#include "RTSPClient.h"
#include "StringParser.h"
#include "OSMemory.h"

#include <errno.h>

static char* sEmptyString = "";
char* RTSPClient::sUserAgent = "None";

RTSPClient::RTSPClient(ClientSocket* inSocket, Bool16 verbosePrinting)
:	fSocket(inSocket),
	fVerbose(verbosePrinting),
	fCSeq(1),
	fStatus(0),
	fSessionID(sEmptyString),
	fServerPort(0),
	fContentLength(0),
	fSetupHeaders(NULL),
	fNumChannelElements(kMinNumChannelElements),
	fNumSSRCElements(0),
	fSSRCMapSize(kMinNumChannelElements),
	fNumFieldIDElements(0),
	fFieldIDMapSize(kMinNumChannelElements),
	fPacketBuffer(NULL),
	fPacketBufferOffset(0),
	fPacketOutstanding(false),
	fRecvContentBuffer(NULL),
	fSendBufferLen(0),
	fContentRecvLen(0),
	fHeaderRecvLen(0),
	fSetupTrackID(0),
	fTransactionStarted(false),
	fReceiveInProgress(false),
	fReceivedResponse(false)
{
	fChannelTrackMap = NEW ChannelMapElem[kMinNumChannelElements];
	::memset(fChannelTrackMap, 0, sizeof(ChannelMapElem) * kMinNumChannelElements);
	fSSRCMap = NEW SSRCMapElem[kMinNumChannelElements];
	::memset(fSSRCMap, 0, sizeof(SSRCMapElem) * kMinNumChannelElements);
	fFieldIDMap = NEW FieldIDArrayElem[kMinNumChannelElements];
	::memset(fFieldIDMap, 0, sizeof(FieldIDArrayElem) * kMinNumChannelElements);

	fSetupHeaders = NEW char[2];
	fSetupHeaders[0] = '\0';
}

RTSPClient::~RTSPClient()
{
	delete [] fRecvContentBuffer;
	delete [] fURL.Ptr;
	if (fSessionID.Ptr != sEmptyString)
		delete [] fSessionID.Ptr;
		
	delete [] fSetupHeaders;
	delete [] fChannelTrackMap;
	delete [] fSSRCMap;
	delete [] fFieldIDMap;
}

void RTSPClient::Set(const StrPtrLen& inURL)
{
	delete [] fURL.Ptr;
	fURL.Ptr = NEW char[inURL.Len + 1];
	fURL.Len = inURL.Len;
	::memcpy(fURL.Ptr, inURL.Ptr, inURL.Len);
	fURL.Ptr[fURL.Len] = '\0';
}

void RTSPClient::SetSetupParams(Float32 inLateTolerance, char* inMetaInfoFields)
{
	delete [] fSetupHeaders;
	fSetupHeaders = NEW char[256];
	fSetupHeaders[0] = '\0';
	
	if (inLateTolerance != 0)
		::sprintf(fSetupHeaders, "x-RTP-Options: late-tolerance=%f\r\n", inLateTolerance);
	if ((inMetaInfoFields != NULL) && (::strlen(inMetaInfoFields) > 0))
		::sprintf(fSetupHeaders + ::strlen(fSetupHeaders), "x-RTP-Meta-Info: %s\r\n", inMetaInfoFields);
}

OS_Error RTSPClient::SendDescribe(Bool16 inAppendJunkData)
{
	if (!fTransactionStarted)
	{
		if (inAppendJunkData)
		{
			::sprintf(fSendBuffer, "DESCRIBE %s RTSP/1.0\r\nCSeq: %lu\r\nAccept: application.sdp\r\nContent-Length: 200\r\nUser-agent: %s\r\n\r\n", fURL.Ptr, fCSeq, sUserAgent);
			UInt32 theBufLen = ::strlen(fSendBuffer);
			Assert((theBufLen + 200) < kReqBufSize)
			for (UInt32 x = theBufLen; x < (theBufLen + 200); x++)
				fSendBuffer[x] = 'd';
			fSendBuffer[theBufLen + 200] = '\0';
		}
		else
		::sprintf(fSendBuffer, "DESCRIBE %s RTSP/1.0\r\nCSeq: %lu\r\nAccept: application.sdp\r\nUser-agent: %s\r\n\r\n", fURL.Ptr, fCSeq, sUserAgent);
	}
	return this->DoTransaction();
}

OS_Error RTSPClient::SendReliableUDPSetup(UInt32 inTrackID, UInt16 inClientPort)
{
	fSetupTrackID = inTrackID; // Needed when SETUP response is received.
	
	if (!fTransactionStarted)
			::sprintf(fSendBuffer, "SETUP %s/trackID=%lu RTSP/1.0\r\nCSeq: %lu\r\n%sTransport: RTP/AVP;unicast;client_port=%u-%u\r\nx-Retransmit: our-retransmit\r\n%sUser-agent: %s\r\n\r\n", fURL.Ptr, inTrackID, fCSeq, fSessionID.Ptr, inClientPort, inClientPort + 1, fSetupHeaders, sUserAgent);

	return this->DoTransaction();
}

OS_Error RTSPClient::SendUDPSetup(UInt32 inTrackID, UInt16 inClientPort)
{
	fSetupTrackID = inTrackID; // Needed when SETUP response is received.
	
	if (!fTransactionStarted)
		::sprintf(fSendBuffer, "SETUP %s/trackID=%lu RTSP/1.0\r\nCSeq: %lu\r\n%sTransport: RTP/AVP;unicast;client_port=%u-%u\r\n%sUser-agent: %s\r\n\r\n", fURL.Ptr, inTrackID, fCSeq, fSessionID.Ptr, inClientPort, inClientPort + 1, fSetupHeaders, sUserAgent);

	return this->DoTransaction();
}

OS_Error RTSPClient::SendTCPSetup(UInt32 inTrackID)
{
	fSetupTrackID = inTrackID; // Needed when SETUP response is received.
	
	if (!fTransactionStarted)
			::sprintf(fSendBuffer, "SETUP %s/trackID=%lu RTSP/1.0\r\nCSeq: %lu\r\n%sTransport: RTP/AVP/TCP\r\n%sUser-agent: %s\r\n\r\n", fURL.Ptr, inTrackID, fCSeq, fSessionID.Ptr, fSetupHeaders, sUserAgent);

	return this->DoTransaction();

}

OS_Error RTSPClient::SendPlay(UInt32 inStartPlayTimeInSec, Float32 inSpeed)
{
	char speedBuf[128];
	speedBuf[0] = '\0';
	
	if (inSpeed != 1)
		::sprintf(speedBuf, "Speed: %f5.2\r\n", inSpeed);
		
	if (!fTransactionStarted)
		::sprintf(fSendBuffer, "PLAY %s RTSP/1.0\r\nCSeq: %lu\r\n%sRange: npt=%lu.0-\r\n%sUser-agent: %s\r\n\r\n", fURL.Ptr, fCSeq, fSessionID.Ptr, inStartPlayTimeInSec, speedBuf, sUserAgent);
	
	return this->DoTransaction();
}

OS_Error RTSPClient::SendTeardown()
{
	if (!fTransactionStarted)
		::sprintf(fSendBuffer, "TEARDOWN %s RTSP/1.0\r\nCSeq: %lu\r\n%sUser-agent: %s\r\n\r\n", fURL.Ptr, fCSeq, fSessionID.Ptr, sUserAgent);
	
	return this->DoTransaction();
}


OS_Error	RTSPClient::GetMediaPacket(UInt32* outTrackID, Bool16* outIsRTCP, char** outBuffer, UInt32* outLen)
{
	static const UInt32 kPacketHeaderLen = 4;
	static const UInt32 kMaxPacketSize = 2048;
	
	// We need to buffer until we get a full packet.
	if (fPacketBuffer == NULL)
		fPacketBuffer = NEW char[kMaxPacketSize];
		
	if (fPacketOutstanding)
	{
		// The previous call to this function returned a packet successfully. We've been holding
		// onto that packet data until now... Now we can blow it away.
		UInt16* thePacketLenP = (UInt16*)fPacketBuffer;
		UInt16 thePacketLen = ntohs(thePacketLenP[1]);

		// Move the leftover data (part of the next packet) to the beginning of the buffer
		Assert(fPacketBufferOffset >= (thePacketLen + kPacketHeaderLen));
		fPacketBufferOffset -= thePacketLen + kPacketHeaderLen;
		::memmove(fPacketBuffer, &fPacketBuffer[thePacketLen + kPacketHeaderLen], fPacketBufferOffset);
		
		fPacketOutstanding = false;
	}
	
	UInt32 theRecvLen = 0;
	OS_Error theErr = fSocket->Read(&fPacketBuffer[fPacketBufferOffset], kMaxPacketSize - fPacketBufferOffset, &theRecvLen);
	if (theErr != OS_NoErr)
		return theErr;

	fPacketBufferOffset += theRecvLen;

	if (fPacketBufferOffset > kPacketHeaderLen)
	{
		UInt16* thePacketLenP = (UInt16*)fPacketBuffer;
		UInt16 thePacketLen = ntohs(thePacketLenP[1]);
		
		if (fPacketBufferOffset >= (thePacketLen + kPacketHeaderLen))
		{
			// We have a complete packet. Return it to the caller.
			Assert(fPacketBuffer[1] < fNumChannelElements); // This is really not a safe assert, but anyway...
			*outTrackID = fChannelTrackMap[fPacketBuffer[1]].fTrackID;
			*outIsRTCP = fChannelTrackMap[fPacketBuffer[1]].fIsRTCP;
			*outLen = thePacketLen;
			
			// Next time we call this function, we will blow away the packet, but until then
			// we leave it untouched.
			fPacketOutstanding = true;
			*outBuffer = &fPacketBuffer[kPacketHeaderLen];
			return OS_NoErr;
		}
	}
	return NULL;
}

UInt32	RTSPClient::GetSSRCByTrack(UInt32 inTrackID)
{
	for (UInt32 x = 0; x < fNumSSRCElements; x++)
	{
		if (inTrackID == fSSRCMap[x].fTrackID)
			return fSSRCMap[x].fSSRC;
	}
	return 0;
}

RTPMetaInfoPacket::FieldID*	RTSPClient::GetFieldIDArrayByTrack(UInt32 inTrackID)
{
	for (UInt32 x = 0; x < fNumFieldIDElements; x++)
	{
		if (inTrackID == fFieldIDMap[x].fTrackID)
			return fFieldIDMap[x].fFieldIDs;
	}
	return NULL;
}


OS_Error RTSPClient::DoTransaction()
{
	OS_Error theErr = OS_NoErr;
	
	if (!fTransactionStarted)
	{
		fCSeq++;
		if (fVerbose)
			printf("%s", fSendBuffer);
	}
	
	if (!fReceiveInProgress)
	{
		fTransactionStarted = true;
		
		theErr = fSocket->Send(fSendBuffer, ::strlen(fSendBuffer));
		if (theErr != OS_NoErr)
			return theErr;
			
		// Done sending request, we're moving onto receiving the response
		fReceiveInProgress = true;
	}

	if (fReceiveInProgress)
	{
		Assert(theErr == OS_NoErr);
		theErr = this->ReceiveResponse();
	}
	return theErr;
}

OS_Error RTSPClient::ReceiveResponse()
{
	OS_Error theErr = OS_NoErr;
	
	while (!fReceivedResponse)
	{
		UInt32 theRecvLen = 0;
		theErr = fSocket->Read(&fRecvHeaderBuffer[fHeaderRecvLen], kReqBufSize - fHeaderRecvLen, &theRecvLen);
		if (theErr != OS_NoErr)
			return theErr;

		fHeaderRecvLen += theRecvLen;
		
		// Check to see if we've gotten a complete header, and if the header has even started
		fRecvHeaderBuffer[fHeaderRecvLen] = '\0';
		
		// The response may not start with the response if we are interleaving media data,
		// in which case there may be leftover media data in the stream. If we encounter any
		// of this cruft, we can just strip it off.
		char* theHeaderStart = ::strstr(fRecvHeaderBuffer, "RTSP");
		if (theHeaderStart == NULL)
		{
			fHeaderRecvLen = 0;
			continue;
		}
		else if (theHeaderStart != fRecvHeaderBuffer)
		{
			fHeaderRecvLen -= theHeaderStart - fRecvHeaderBuffer;
			::memmove(fRecvHeaderBuffer, theHeaderStart, fHeaderRecvLen);
			fRecvHeaderBuffer[fHeaderRecvLen] = '\0';
		}
		
		char* theResponseData = ::strstr(fRecvHeaderBuffer, "\r\n\r\n");
		if (theResponseData != NULL)
		{
			if (fVerbose)
				printf("%s", fRecvHeaderBuffer);
				
			// skip past the \r\n\r\n
			theResponseData += 4;
			
			// We've got a new response
			fReceivedResponse = true;
			
			// Zero out fields that will change with every RTSP response
			fServerPort = 0;
			fStatus = 0;
			fContentLength = 0;
		
			// Parse the response.
			StrPtrLen theData(fRecvHeaderBuffer, (theResponseData - (&fRecvHeaderBuffer[0])));
			StringParser theParser(&theData);
			
			theParser.ConsumeLength(NULL, 9); //skip past RTSP/1.0
			fStatus = theParser.ConsumeInteger(NULL);
			
			StrPtrLen theLastHeader;
			while (theParser.GetDataRemaining() > 0)
			{
				static StrPtrLen sSessionHeader("Session");
				static StrPtrLen sContentLenHeader("Content-length");
				static StrPtrLen sTransportHeader("Transport");
				static StrPtrLen sRTPInfoHeader("RTP-Info");
				static StrPtrLen sRTPMetaInfoHeader("x-RTP-Meta-Info");
				static StrPtrLen sSameAsLastHeader(" ,");
				
				StrPtrLen theKey;
				theParser.GetThruEOL(&theKey);
				
				if (theKey.NumEqualIgnoreCase(sSessionHeader.Ptr, sSessionHeader.Len))
				{
					if (fSessionID.Len == 0)
					{
						// Copy the session ID and store it.
						// First figure out how big the session ID is. We copy
						// everything up until the first ';' returned from the server
						UInt32 keyLen = 0;
						while ((theKey.Ptr[keyLen] != ';') && (theKey.Ptr[keyLen] != '\r') && (theKey.Ptr[keyLen] != '\n'))
							keyLen++;
						
						// Append an EOL so we can stick this thing transparently into the SETUP request
						
						fSessionID.Ptr = NEW char[keyLen + 3];
						fSessionID.Len = keyLen + 2;
						::memcpy(fSessionID.Ptr, theKey.Ptr, keyLen);
						::memcpy(fSessionID.Ptr + keyLen, "\r\n", 2);//Append a EOL
						fSessionID.Ptr[keyLen + 2] = '\0';
					}
				}
				else if (theKey.NumEqualIgnoreCase(sContentLenHeader.Ptr, sContentLenHeader.Len))
				{
					StringParser theCLengthParser(&theKey);
					theCLengthParser.ConsumeUntil(NULL, StringParser::sDigitMask);
					fContentLength = theCLengthParser.ConsumeInteger(NULL);
					
					delete [] fRecvContentBuffer;
					fRecvContentBuffer = NEW char[fContentLength + 1];
					
					// Figure out how much of the content body we've already received
					// in the header buffer
					fContentRecvLen = fHeaderRecvLen - (theResponseData - &fRecvHeaderBuffer[0]);

					// Immediately copy the bit of the content body that we've already
					// read off of the socket.
					::memcpy(fRecvContentBuffer, theResponseData, fContentRecvLen);
					
				}
				else if (theKey.NumEqualIgnoreCase(sTransportHeader.Ptr, sTransportHeader.Len))
				{
					StringParser theTransportParser(&theKey);
					StrPtrLen theSubHeader;

					while (theTransportParser.GetDataRemaining() > 0)
					{
						static StrPtrLen sServerPort("server_port");
						static StrPtrLen sInterleaved("interleaved");

						theTransportParser.GetThru(&theSubHeader, ';');
						if (theSubHeader.NumEqualIgnoreCase(sServerPort.Ptr, sServerPort.Len))
						{
							StringParser thePortParser(&theSubHeader);
							thePortParser.ConsumeUntil(NULL, StringParser::sDigitMask);
							fServerPort = thePortParser.ConsumeInteger(NULL);
						}
						else if (theSubHeader.NumEqualIgnoreCase(sInterleaved.Ptr, sInterleaved.Len))
							this->ParseInterleaveSubHeader(&theSubHeader);							
					}
				}
				else if (theKey.NumEqualIgnoreCase(sRTPInfoHeader.Ptr, sRTPInfoHeader.Len))
					ParseRTPInfoHeader(&theKey);
				else if (theKey.NumEqualIgnoreCase(sRTPMetaInfoHeader.Ptr, sRTPMetaInfoHeader.Len))
					ParseRTPMetaInfoHeader(&theKey);
				else if (theKey.NumEqualIgnoreCase(sSameAsLastHeader.Ptr, sSameAsLastHeader.Len))
				{
					//
					// If the last header was an RTP-Info header
					if (theLastHeader.NumEqualIgnoreCase(sRTPInfoHeader.Ptr, sRTPInfoHeader.Len))
						ParseRTPInfoHeader(&theKey);
				}
				theLastHeader = theKey;
			}
		}
		else if (fHeaderRecvLen == kReqBufSize)
			return ENOBUFS; // This response is too big for us to handle!
	}
	
	while (fContentLength > fContentRecvLen)
	{
		UInt32 theContentRecvLen = 0;
		theErr = fSocket->Read(&fRecvContentBuffer[fContentRecvLen], fContentLength - fContentRecvLen, &theContentRecvLen);
		if (theErr != OS_NoErr)
		{
			fEventMask = EV_RE;
			return theErr;
		}
		fContentRecvLen += theContentRecvLen;		
	}
	
	
	// We're all done, reset all our state information.
	fContentRecvLen = 0;
	fHeaderRecvLen = 0;
	fReceivedResponse = false;
	fReceiveInProgress = false;
	fTransactionStarted = false;
	
	return OS_NoErr;
}

void	RTSPClient::ParseRTPInfoHeader(StrPtrLen* inHeader)
{
	static StrPtrLen sURL("url");
	StringParser theParser(inHeader);
	theParser.ConsumeUntil(NULL, 'u'); // consume until "url"
	
	if (fNumSSRCElements == fSSRCMapSize)
	{
		SSRCMapElem* theNewMap = NEW SSRCMapElem[fSSRCMapSize * 2];
		::memset(theNewMap, 0, sizeof(SSRCMapElem) * (fSSRCMapSize * 2));
		::memcpy(theNewMap, fSSRCMap, sizeof(SSRCMapElem) * fNumSSRCElements);
		fSSRCMapSize *= 2;
		delete [] fSSRCMap;
		fSSRCMap = theNewMap;
	}	
	
	fSSRCMap[fNumSSRCElements].fTrackID = 0;
	fSSRCMap[fNumSSRCElements].fSSRC = 0;
	
	// Parse out the trackID & the SSRC
	StrPtrLen theRTPInfoSubHeader;
	(void)theParser.GetThru(&theRTPInfoSubHeader, ';');
	
	while (theRTPInfoSubHeader.Len > 0)
	{
		static StrPtrLen sURLSubHeader("url");
		static StrPtrLen sSSRCSubHeader("ssrc");
		
		if (theRTPInfoSubHeader.NumEqualIgnoreCase(sURLSubHeader.Ptr, sURLSubHeader.Len))
		{
			StringParser theURLParser(&theRTPInfoSubHeader);
			theURLParser.ConsumeUntil(NULL, StringParser::sDigitMask);
			fSSRCMap[fNumSSRCElements].fTrackID = theURLParser.ConsumeInteger(NULL);
		}
		else if (theRTPInfoSubHeader.NumEqualIgnoreCase(sSSRCSubHeader.Ptr, sSSRCSubHeader.Len))
		{
			StringParser theURLParser(&theRTPInfoSubHeader);
			theURLParser.ConsumeUntil(NULL, StringParser::sDigitMask);
			fSSRCMap[fNumSSRCElements].fSSRC = theURLParser.ConsumeInteger(NULL);
		}

		// Move onto the next parameter
		(void)theParser.GetThru(&theRTPInfoSubHeader, ';');
	}
	
	fNumSSRCElements++;
}

void	RTSPClient::ParseRTPMetaInfoHeader(StrPtrLen* inHeader)
{
	//
	// Reallocate the array if necessary
	if (fNumFieldIDElements == fFieldIDMapSize)
	{
		FieldIDArrayElem* theNewMap = NEW FieldIDArrayElem[fFieldIDMapSize * 2];
		::memset(theNewMap, 0, sizeof(FieldIDArrayElem) * (fFieldIDMapSize * 2));
		::memcpy(theNewMap, fFieldIDMap, sizeof(FieldIDArrayElem) * fNumFieldIDElements);
		fFieldIDMapSize *= 2;
		delete [] fFieldIDMap;
		fFieldIDMap = theNewMap;
	}
	
	//
	// Build the FieldIDArray for this track
	RTPMetaInfoPacket::ConstructFieldIDArrayFromHeader(inHeader, fFieldIDMap[fNumFieldIDElements].fFieldIDs);
	fFieldIDMap[fNumFieldIDElements].fTrackID = fSetupTrackID;
	fNumFieldIDElements++;
}

void	RTSPClient::ParseInterleaveSubHeader(StrPtrLen* inSubHeader)
{
	StringParser theChannelParser(inSubHeader);
	
	// Parse out the channel numbers
	theChannelParser.ConsumeUntil(NULL, StringParser::sDigitMask);
	UInt8 theRTPChannel = theChannelParser.ConsumeInteger(NULL);
	theChannelParser.ConsumeLength(NULL, 1);
	UInt8 theRTCPChannel = theChannelParser.ConsumeInteger(NULL);
	
	UInt8 theMaxChannel = theRTCPChannel;
	if (theRTPChannel > theMaxChannel)
		theMaxChannel = theRTPChannel;
	
	// Reallocate the channel-track array if it is too little
	if (theMaxChannel >= fNumChannelElements)
	{
		ChannelMapElem* theNewMap = NEW ChannelMapElem[theMaxChannel + 1];
		::memset(theNewMap, 0, sizeof(ChannelMapElem) * (theMaxChannel + 1));
		::memcpy(theNewMap, fChannelTrackMap, sizeof(ChannelMapElem) * fNumChannelElements);
		fNumChannelElements = theMaxChannel + 1;
		delete [] fChannelTrackMap;
		fChannelTrackMap = theNewMap;
	}
	
	// Copy the relevent information into the channel-track array.
	fChannelTrackMap[theRTPChannel].fTrackID = fSetupTrackID;
	fChannelTrackMap[theRTPChannel].fIsRTCP = false;
	fChannelTrackMap[theRTCPChannel].fTrackID = fSetupTrackID;
	fChannelTrackMap[theRTCPChannel].fIsRTCP = true;
}

#define _RTSPCLIENTTESTING_ 0

#include "SocketUtils.h"
#include "OS.h"

#if _RTSPCLIENTTESTING_
int main(int argc, char * argv[]) 
{
	OS::Initialize();
	Socket::Initialize();
	SocketUtils::Initialize();

	UInt32 ipAddr = (UInt32)ntohl(::inet_addr("17.221.41.111"));
	UInt16 port = 554;
	StrPtrLen theURL("rtsp://17.221.41.111/mystery.mov");

	RTSPClient theClient(Socket::kNonBlockingSocketType);
	theClient.Set(ipAddr, port, theURL);
	
	OS_Error theErr = EINVAL;
	while (theErr != OS_NoErr)
	{
		theErr = theClient.SendDescribe();
		sleep(1);
	}
	Assert(theClient.GetStatus() == 200);
	theErr = EINVAL;
	while (theErr != OS_NoErr)
	{
		theErr = theClient.SendSetup(3, 6790);
		sleep(1);
	}
	printf("Server port: %d\n", theClient.GetServerPort());
	Assert(theClient.GetStatus() == 200);
	theErr = EINVAL;
	while (theErr != OS_NoErr)
	{
		theErr = theClient.SendSetup(4, 6792);
		sleep(1);
	}
	printf("Server port: %d\n", theClient.GetServerPort());
	Assert(theClient.GetStatus() == 200);
	theErr = EINVAL;
	while (theErr != OS_NoErr)
	{
		theErr = theClient.SendPlay();
		sleep(1);
	}
	Assert(theClient.GetStatus() == 200);
	theErr = EINVAL;
	while (theErr != OS_NoErr)
	{
		theErr = theClient.SendTeardown();
		sleep(1);
	}
	Assert(theClient.GetStatus() == 200);
}
#endif