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
	File:		RTSPRequestInterface.h

	Contains:	Provides a simple API for modules to access request information and
				manipulate (and possibly send) the client response.
				
				Implements the RTSP Request dictionary for QTSS API.
	
	
*/


#ifndef __RTSPREQUESTINTERFACE_H__
#define __RTSPREQUESTINTERFACE_H__

//INCLUDES:
#include "QTSS.h"
#include "QTSSDictionary.h"

#include "StrPtrLen.h"
#include "RTSPSessionInterface.h"
#include "RTSPResponseStream.h"
#include "RTSPProtocol.h"
#include "QTSSMessages.h"



class RTSPRequestInterface : public QTSSDictionary
{
	public:

		//Initialize
		//Call initialize before instantiating this class. For maximum performance, this class builds
		//any response header it can at startup time.
		static void 		Initialize();
		
		//CONSTRUCTOR:
		RTSPRequestInterface(RTSPSessionInterface *session);
		virtual ~RTSPRequestInterface()
			{ if (fMovieFolderPtr != &fMovieFolderPath[0]) delete [] fMovieFolderPtr; delete [] fFullPath; }
		
		//FUNCTIONS FOR SENDING OUTPUT:
		
		//Adds a new header to this object's list of headers to be sent out.
		//Note that this is only needed for "special purpose" headers. The Server,
		//CSeq, SessionID, and Connection headers are taken care of automatically
		void	AppendHeader(QTSS_RTSPHeader inHeader, StrPtrLen* inValue);


		
		//These functions format headers with complex formats
		
		// The transport header constructed by this function mimics the one sent
		// by the client, with the addition of server port & interleaved sub headers
		void 	AppendTransportHeader(StrPtrLen* serverPortA,
										StrPtrLen* serverPortB,
										StrPtrLen* channelA,
										StrPtrLen* channelB,
										StrPtrLen* serverIPAddr = NULL);
		void 	AppendContentBaseHeader(StrPtrLen* theURL);
		void 	AppendRTPInfoHeader(QTSS_RTSPHeader inHeader,
									StrPtrLen* url, StrPtrLen* seqNumber,
									StrPtrLen* ssrc, StrPtrLen* rtpTime);

		void 	AppendDateAndExpires();
		void	AppendSessionHeaderWithTimeout( StrPtrLen* inSessionID, StrPtrLen* inTimeout );

		// MODIFIERS
		
		void SetKeepAlive(Bool16 newVal)				{ fResponseKeepAlive = newVal; }
		
		//SendHeader:
		//Sends the RTSP headers, in their current state, to the client.
		void SendHeader();
		
		// QTSS STREAM FUNCTIONS
		
		// THE FIRST ENTRY OF THE IOVEC MUST BE BLANK!!!
		virtual QTSS_Error WriteV(iovec* inVec, UInt32 inNumVectors, UInt32 inTotalLength, UInt32* outLenWritten);
		
		//Write
		//A "buffered send" that can be used for sending small chunks of data at a time.
		virtual QTSS_Error Write(void* inBuffer, UInt32 inLength, UInt32* outLenWritten, UInt32 inFlags);
		
		// Flushes all currently buffered data to the network. This either returns
		// QTSS_NoErr or EWOULDBLOCK. If it returns EWOULDBLOCK, you should wait for
		// a EV_WR on the socket, and call flush again.
		virtual QTSS_Error 	Flush() { return fOutputStream->Flush(); }

		// Reads data off the stream. Same behavior as calling RTSPSessionInterface::Read
		virtual QTSS_Error Read(void* ioBuffer, UInt32 inLength, UInt32* outLenRead)
			{ return fSession->Read(ioBuffer, inLength, outLenRead); }
			
		// Requests an event. Same behavior as calling RTSPSessionInterface::RequestEvent
		virtual QTSS_Error RequestEvent(QTSS_EventType inEventMask)
			{ return fSession->RequestEvent(inEventMask); }
		
		
		//ACCESS FUNCTIONS:
		
		// These functions are shortcuts that objects internal to the server
		// use to get access to RTSP request information. Pretty much all
		// of this stuff is also available as QTSS API attributes.
		
		QTSS_RTSPMethod 			GetMethod() const		{ return fMethod; }
		QTSS_RTSPStatusCode 		GetStatus() const		{ return fStatus; }
		Bool16						GetResponseKeepAlive() const { return fResponseKeepAlive; }
		void						SetResponseKeepAlive(Bool16 keepAlive)  { fResponseKeepAlive = keepAlive; }
		
		//will be -1 unless there was a Range header. May have one or two values
		Float64						GetStartTime()		{ return fStartTime; }
		Float64						GetStopTime()		{ return fStopTime; }
		
		//
		// Value of Speed: header in request
		Float32						GetSpeed()			{ return fSpeed; }
		
		//
		// Value of late-tolerance field of x-RTP-Options header
		Float32						GetLateToleranceInSec(){ return fLateTolerance; }
		StrPtrLen*					GetLateToleranceStr(){ return &fLateToleranceStr; }
		
		// these get set if there is a transport header
		UInt16						GetClientPortA()	{ return fClientPortA; }
		UInt16						GetClientPortB()	{ return fClientPortB; }
		UInt32						GetDestAddr()		{ return fDestinationAddr; }
		UInt32						GetSourceAddr()		{ return fSourceAddr; }
		UInt16						GetTtl()			{ return fTtl; }
		QTSS_RTPTransportType		GetTransportType()	{ return fTransportType; }
		UInt32						GetWindowSize()		{ return fWindowSize; }
		UInt32						GetAckTimeout()		{ return fAckTimeout; }
		
			
		Bool16						HasResponseBeenSent()
										{ return fOutputStream->GetBytesWritten() > 0; }
			
		RTSPSessionInterface*		GetSession()		 { return fSession; }
		QTSSDictionary*				GetHeaderDictionary(){ return &fHeaderDictionary; }
		
		Bool16						GetAllowed()				{ return fAllowed; }
		void						SetAllowed(Bool16 allowed)	{ fAllowed = allowed;}

	protected:

		//ALL THIS STUFF HERE IS SETUP BY RTSPREQUEST object (derived)
		
		//REQUEST HEADER DATA
		enum
		{
			kMovieFolderBufSizeInBytes = 256,	//Uint32
			kMaxFilePathSizeInBytes = 256		//Uint32
		};
		
		QTSS_RTSPMethod				fMethod;			//Method of this request
		QTSS_RTSPStatusCode			fStatus;			//Current status of this request
		UInt32						fRealStatusCode;	//Current RTSP status num of this request
		Bool16						fRequestKeepAlive;	//Does the client want keep-alive?
		Bool16						fResponseKeepAlive;	//Are we going to keep-alive?
		RTSPProtocol::RTSPVersion	fVersion;

		Float64						fStartTime;			//Range header info: start time
		Float64						fStopTime;			//Range header info: stop time

		UInt16						fClientPortA;		//This is all info that comes out
		UInt16						fClientPortB;		//of the Transport: header
		UInt16						fTtl;
		UInt32						fDestinationAddr;
		UInt32						fSourceAddr;
		QTSS_RTPTransportType		fTransportType;
		UInt32						fContentLength;
		SInt64						fIfModSinceDate;
		Float32						fSpeed;
		Float32						fLateTolerance;
		StrPtrLen					fLateToleranceStr;
		
		char*						fFullPath;
		StrPtrLen					fFirstTransport;
		
		QTSS_StreamRef				fStreamRef;
		
		//
		// For reliable UDP
		UInt32						fWindowSize;
		UInt32						fAckTimeout;

		//Because of URL decoding issues, we need to make a copy of the file path.
		//Here is a buffer for it.
		char						fFilePath[kMaxFilePathSizeInBytes];
		char						fMovieFolderPath[kMovieFolderBufSizeInBytes];
		char*						fMovieFolderPtr;
		
		QTSSDictionary				fHeaderDictionary;
		
		Bool16						fAllowed;

	private:

		RTSPSessionInterface*	fSession;
		RTSPResponseStream*		fOutputStream;
		

		enum
		{
			kStaticHeaderSizeInBytes = 512	//UInt32
		};
		
		Bool16					fStandardHeadersWritten;
			
		void 					WriteStandardHeaders();
		static void 			PutStatusLine(	StringFormatter* putStream,
												QTSS_RTSPStatusCode status,
												RTSPProtocol::RTSPVersion version);

		//Individual param retrieval functions
		static Bool16 		GetAbsTruncatedPath(QTSS_FunctionParams* funcParamsPtr);
		static Bool16 		GetTruncatedPath(QTSS_FunctionParams* funcParamsPtr);
		static Bool16 		GetFileName(QTSS_FunctionParams* funcParamsPtr);
		static Bool16 		GetFileDigit(QTSS_FunctionParams* funcParamsPtr);
		static Bool16 		GetRealStatusCode(QTSS_FunctionParams* funcParamsPtr);


		//optimized preformatted response header strings
		static char				sPremadeHeader[kStaticHeaderSizeInBytes];
		static StrPtrLen		sPremadeHeaderPtr;
		static StrPtrLen		sColonSpace;
		
		//Dictionary support
		static QTSSAttrInfoDict::AttrInfo	sAttributes[];
};
#endif // __RTSPREQUESTINTERFACE_H__

