/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Copyright (c) 1999 Apple Computer, Inc.  All Rights Reserved.
 * The contents of this file constitute Original Code as defined in and are 
 * subject to the Apple Public Source License Version 1.1 (the "License").  
 * You may not use this file except in compliance with the License.  Please 
 * obtain a copy of the License at http://www.apple.com/publicsource and 
 * read it before using this file.
 * 
 * This Original Code and all software distributed under the License are 
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the License for 
 * the specific language governing rights and limitations under the 
 * License.
 * 
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
	File:		RTSPClient.h

	
	
*/

#ifndef __RTSP_CLIENT_H__
#define __RTSP_CLIENT_H__

#include "OSHeaders.h"
#include "StrPtrLen.h"
#include "TCPSocket.h"
#include "ClientSocket.h"
#include "RTPMetaInfoPacket.h"

class RTSPClient
{
	public:
	
		//
		// Before using this class, you must set the User Agent this way.
		static void		SetUserAgentStr(char* inUserAgent) { sUserAgent = inUserAgent; }
	
		// verbosePrinting = print out all requests and responses
		RTSPClient(ClientSocket* inSocket, Bool16 verbosePrinting);
		~RTSPClient();
		
		// This must be called before any other function. Sets up very important info.
		void		Set(const StrPtrLen& inURL);
		
		//
		// This function allows you to add some special-purpose headers to your
		// SETUP request if you want. These are mainly used for the caching proxy protocol,
		// though there may be other uses.
		//
		// inLateTolerance: default = 0 (don't care)
		// inMetaInfoFields: default = NULL (don't want RTP-Meta-Info packets
		// inSpeed: default = 1 (normal speed)
		void		SetSetupParams(Float32 inLateTolerance, char* inMetaInfoFields);
		
		// Send specified RTSP request to server, wait for complete response.
		// These return EAGAIN if transaction is in progress, OS_NoErr if transaction
		// was successful, EINPROGRESS if connection is in progress, or some other
		// error if transaction failed entirely.
		OS_Error	SendDescribe(Bool16 inAppendJunkData = false);
		
		OS_Error 	SendReliableUDPSetup(UInt32 inTrackID, UInt16 inClientPort);
		OS_Error 	SendUDPSetup(UInt32 inTrackID, UInt16 inClientPort);
		OS_Error 	SendTCPSetup(UInt32 inTrackID);
		OS_Error	SendPlay(UInt32 inStartPlayTimeInSec, Float32 inSpeed = 1);
		OS_Error	SendTeardown();
		
		//
		// If any of the tracks are being interleaved, this fetches a media packet from
		// the control connection. This function assumes that SendPlay has already completed
		// successfully and media packets are being sent.
		OS_Error	GetMediaPacket(	UInt32* outTrackID, Bool16* outIsRTCP,
									char** outBuffer, UInt32* outLen);
		
		// ACCESSORS

		StrPtrLen*	GetURL()				{ return &fURL; }
		UInt32		GetStatus() 			{ return fStatus; }
		StrPtrLen*	GetSessionID() 			{ return &fSessionID; }
		UInt16		GetServerPort() 		{ return fServerPort; }
		UInt32		GetContentLength() 		{ return fContentLength; }
		char*		GetContentBody() 		{ return fRecvContentBuffer; }
		ClientSocket*	GetSocket()			{ return fSocket; }

		//
		// If available, returns the SSRC associated with the track in the PLAY response.
		// Returns 0 if SSRC is not available.
		UInt32		GetSSRCByTrack(UInt32 inTrackID);
	
		//
		// If available, returns the RTP-Meta-Info field ID array for
		// a given track. For more details, see RTPMetaInfoPacket.h
		RTPMetaInfoPacket::FieldID*	GetFieldIDArrayByTrack(UInt32 inTrackID);
		
	private:
	
		static char*	sUserAgent;
	
		// Helper methods
		OS_Error 	DoTransaction();
		void		ParseInterleaveSubHeader(StrPtrLen* inSubHeader);
		void		ParseRTPInfoHeader(StrPtrLen* inHeader);
		void		ParseRTPMetaInfoHeader(StrPtrLen* inHeader);

		// Call this to receive an RTSP response from the server.
		// Returns EAGAIN until a complete response has been received.
		OS_Error 	ReceiveResponse();
		
		ClientSocket*	fSocket;
		Bool16			fVerbose;

		// Information we need to send the request
		StrPtrLen	fURL;
		UInt32		fCSeq;
		
		// Response data we get back
		UInt32		fStatus;
		StrPtrLen	fSessionID;
		UInt16		fServerPort;
		UInt32		fContentLength;
		StrPtrLen	fRTPInfoHeader;
		
		// Special purpose SETUP params
		char*			fSetupHeaders;
		
		// If we are interleaving, this maps channel numbers to track IDs
		struct ChannelMapElem
		{
			UInt32	fTrackID;
			Bool16	fIsRTCP;
		};
		ChannelMapElem*	fChannelTrackMap;
		UInt8			fNumChannelElements;
		
		// For storing SSRCs
		struct SSRCMapElem
		{
			UInt32 fTrackID;
			UInt32 fSSRC;
		};
		SSRCMapElem*	fSSRCMap;
		UInt32			fNumSSRCElements;
		UInt32			fSSRCMapSize;

		// For storing FieldID arrays
		struct FieldIDArrayElem
		{
			UInt32 fTrackID;
			RTPMetaInfoPacket::FieldID fFieldIDs[RTPMetaInfoPacket::kNumFields];
		};
		FieldIDArrayElem*	fFieldIDMap;
		UInt32				fNumFieldIDElements;
		UInt32				fFieldIDMapSize;
		
		// If we are interleaving, we need this stuff to support the GetMediaPacket function
		char*			fPacketBuffer;
		UInt32			fPacketBufferOffset;
		Bool16			fPacketOutstanding;
		
		enum
		{
			kMinNumChannelElements = 5,
			kReqBufSize = 512,
		};
		
		// Data buffers
		char		fSendBuffer[kReqBufSize + 1]; 	// for sending requests
		char		fRecvHeaderBuffer[kReqBufSize + 1];// for receiving response headers
		char*		fRecvContentBuffer;				// for receiving response body
		UInt32		fSendBufferLen;
		
		// Tracking the state of our receives
		UInt32		fContentRecvLen;
		UInt32		fHeaderRecvLen;
		UInt32		fSetupTrackID;
		
		// States
		Bool16		fTransactionStarted;
		Bool16		fReceiveInProgress;
		Bool16		fReceivedResponse;
		
		UInt32		fEventMask;
};

#endif //__CLIENT_SESSION_H__
