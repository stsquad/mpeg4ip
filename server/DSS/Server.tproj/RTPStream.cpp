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
	File:		RTPStream.cpp

	Contains:	Implementation of RTPStream class. 
					
	

*/



#include <stdlib.h>

#include "RTPStream.h"

#include "QTSSModuleUtils.h"
#include "QTSServerInterface.h"
#include "OS.h"

#include "RTCPPacket.h"
#include "RTCPAPPPacket.h"
#include "RTCPAckPacket.h"
#include "RTCPSRPacket.h"
#include "SocketUtils.h"
#include <errno.h>

#if DEBUG
#define RTP_TCP_STREAM_DEBUG 0
#else
#define RTP_TCP_STREAM_DEBUG 0
#endif

QTSSAttrInfoDict::AttrInfo	RTPStream::sAttributes[] = 
{   /*fields:   fAttrName, fFuncPtr, fAttrDataType, fAttrPermission */
	/* 0  */ { "qtssRTPStrTrackID", 				NULL, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite },
	/* 1  */ { "qtssRTPStrSSRC", 					NULL, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModePreempSafe }, 
	/* 2  */ { "qtssRTPStrPayloadName", 			NULL, 	qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite   },
	/* 3  */ { "qtssRTPStrPayloadType", 			NULL, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite   },
	/* 4  */ { "qtssRTPStrFirstSeqNumber",			NULL, 	qtssAttrDataTypeSInt16,	qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite   },
	/* 5  */ { "qtssRTPStrFirstTimestamp", 			NULL, 	qtssAttrDataTypeSInt32,	qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite   },
	/* 6  */ { "qtssRTPStrTimescale", 				NULL, 	qtssAttrDataTypeSInt32,	qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite   },
	/* 7  */ { "qtssRTPStrQualityLevel", 			NULL, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite   },
	/* 8  */ { "qtssRTPStrNumQualityLevels", 		NULL, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite   },
	/* 9  */ { "qtssRTPStrBufferDelayInSecs", 		NULL, 	qtssAttrDataTypeFloat32,	qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite   },
	
	/* 10 */ { "qtssRTPStrFractionLostPackets", 	NULL, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 11 */ { "qtssRTPStrTotalLostPackets", 		NULL, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 12 */ { "qtssRTPStrJitter", 					NULL, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 13 */ { "qtssRTPStrRecvBitRate", 			NULL, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 14 */ { "qtssRTPStrAvgLateMilliseconds", 	NULL, 	qtssAttrDataTypeUInt16,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 15 */ { "qtssRTPStrPercentPacketsLost", 		NULL, 	qtssAttrDataTypeUInt16,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 16 */ { "qtssRTPStrAvgBufDelayInMsec", 		NULL, 	qtssAttrDataTypeUInt16,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 17 */ { "qtssRTPStrGettingBetter", 			NULL, 	qtssAttrDataTypeUInt16,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 18 */ { "qtssRTPStrGettingWorse", 			NULL, 	qtssAttrDataTypeUInt16,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 19 */ { "qtssRTPStrNumEyes", 				NULL, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 20 */ { "qtssRTPStrNumEyesActive", 			NULL, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 21 */ { "qtssRTPStrNumEyesPaused", 			NULL, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 22 */ { "qtssRTPStrTotPacketsRecv", 			NULL, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 23 */ { "qtssRTPStrTotPacketsDropped", 		NULL, 	qtssAttrDataTypeUInt16,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 24 */ { "qtssRTPStrTotPacketsLost", 			NULL, 	qtssAttrDataTypeUInt16,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 25 */ { "qtssRTPStrClientBufFill", 			NULL, 	qtssAttrDataTypeUInt16,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 26 */ { "qtssRTPStrFrameRate", 				NULL, 	qtssAttrDataTypeUInt16,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 27 */ { "qtssRTPStrExpFrameRate", 			NULL, 	qtssAttrDataTypeUInt16,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 28 */ { "qtssRTPStrAudioDryCount", 			NULL, 	qtssAttrDataTypeUInt16,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 29 */ { "qtssRTPStrIsTCP", 					NULL, 	qtssAttrDataTypeBool16,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 30 */ { "qtssRTPStrStreamRef", 				NULL, 	qtssAttrDataTypeQTSS_StreamRef,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 31 */ { "qtssRTPStrCurrentPacketDelay", 		NULL, 	qtssAttrDataTypeSInt32,	qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite  },
	/* 32 */ { "qtssRTPStrTransportType", 			NULL, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 33 */ { "qtssRTPStrStalePacketsDropped", 	NULL, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 34 */ { "qtssRTPStrOverbufferTimeInSec", 	NULL, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 35 */ { "qtssRTPStrCurPacketsLostInRTCPInterval", 	NULL, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 36 */ { "qtssRTPStrPacketCountInRTCPInterval", 		NULL, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModePreempSafe  }

};

StrPtrLen	RTPStream::sChannelNums[] =
{
	StrPtrLen("0"),
	StrPtrLen("1"),
	StrPtrLen("2"),
	StrPtrLen("3"),
	StrPtrLen("4"),
	StrPtrLen("5"),
	StrPtrLen("6"),
	StrPtrLen("7"),
	StrPtrLen("8"),
	StrPtrLen("9")
};

QTSS_ModuleState RTPStream::sRTCPProcessModuleState = { NULL, 0, NULL, false };

void	RTPStream::Initialize()
{
	for (int x = 0; x < qtssRTPStrNumParams; x++)
		QTSSDictionaryMap::GetMap(QTSSDictionaryMap::kRTPStreamDictIndex)->
			SetAttribute(x, sAttributes[x].fAttrName, sAttributes[x].fFuncPtr,
				sAttributes[x].fAttrDataType, sAttributes[x].fAttrPermission);
}

RTPStream::RTPStream(UInt32 inSSRC, RTPSessionInterface* inSession)
: 	QTSSDictionary(QTSSDictionaryMap::GetMap(QTSSDictionaryMap::kRTPStreamDictIndex), NULL),
	fSockets(NULL),
	fSession(inSession),
	fBytesSentThisInterval(0),
	fDisplayCount(0),
	fSawFirstPacket(false),
	fTracker(NULL),
	fRemoteAddr(0),
	fRemoteRTPPort(0),
	fRemoteRTCPPort(0),
	fLastSenderReportTime(0),
	fPacketCount(0),
	fLastPacketCount(0),
	fPacketCountInRTCPInterval(0),
	fByteCount(0),
	fTrackID(0),
	fSsrc(inSSRC),
	fSsrcStringPtr(fSsrcString, 0),
	fPayloadType(qtssUnknownPayloadType),
	fFirstSeqNumber(0),
	fFirstTimeStamp(0),
	fTimescale(0),
	fStreamURLPtr(fStreamURL, 0),
	fQualityLevel(0),
	fNumQualityLevels(0),
	fLastRTPTimestamp(0),
	
	fFractionLostPackets(0),
	fTotalLostPackets(0),
	fPriorTotalLostPackets(0),
	fJitter(0),
	fReceiverBitRate(0),
	fAvgLateMsec(0),
	fPercentPacketsLost(0),
	fAvgBufDelayMsec(0),
	fIsGettingBetter(0),
	fIsGettingWorse(0),
	fNumEyes(0),
	fNumEyesActive(0),
	fNumEyesPaused(0),
	fTotalPacketsRecv(0),
	fTotalPacketsDropped(0),
	fTotalPacketsLost(0),
	fPriorTotalPacketsLost(0),
	fCurPacketsLostInRTCPInterval(0),
	fClientBufferFill(0),
	fFrameRate(0),
	fExpectedFrameRate(0),
	fAudioDryCount(0),
	fIsTCP(false),
	fTransportType(qtssRTPTransportTypeUDP),
	fCurrentPacketDelay(0),
	fTurnThinningOffDelay(0),
	fIncreaseThinningDelay(0),
	fDropAllPacketsForThisStreamDelay(0),
	fStalePacketsDropped(0),
	fTimeStreamCaughtUp(0),
	fBufferDelay(3.0),
	fLateToleranceInSec(0),
	fStreamRef(this),
	fOverbufferTimeInSec(0),
	fRTPChannel(0),
	fRTCPChannel(0)
{
#if DEBUG
	fNumPacketsDroppedOnTCPFlowControl = 0;
	fFlowControlStartedMsec = 0;
	fFlowControlDurationMsec = 0;
#endif
	//format the ssrc as a string
	::sprintf(fSsrcString, "%ld", fSsrc);
	fSsrcStringPtr.Len = ::strlen(fSsrcString);
	Assert(fSsrcStringPtr.Len < kMaxSsrcSizeInBytes);

	// SETUP DICTIONARY ATTRIBUTES
	
	this->SetVal(qtssRTPStrTrackID, 			&fTrackID, 				sizeof(fTrackID));
	this->SetVal(qtssRTPStrSSRC, 				&fSsrc, 				sizeof(fSsrc));
	this->SetEmptyVal(qtssRTPStrPayloadName, 	&fPayloadNameBuf, 		kDefaultPayloadBufSize);
	this->SetVal(qtssRTPStrPayloadType, 		&fPayloadType, 			sizeof(fPayloadType));
	this->SetVal(qtssRTPStrFirstSeqNumber, 		&fFirstSeqNumber, 		sizeof(fFirstSeqNumber));
	this->SetVal(qtssRTPStrFirstTimestamp, 		&fFirstTimeStamp, 		sizeof(fFirstTimeStamp));
	this->SetVal(qtssRTPStrTimescale, 			&fTimescale, 			sizeof(fTimescale));
	this->SetVal(qtssRTPStrQualityLevel, 		&fQualityLevel, 		sizeof(fQualityLevel));
	this->SetVal(qtssRTPStrNumQualityLevels, 	&fNumQualityLevels, 	sizeof(fNumQualityLevels));
	this->SetVal(qtssRTPStrBufferDelayInSecs, 	&fBufferDelay, 			sizeof(fBufferDelay));
	this->SetVal(qtssRTPStrFractionLostPackets,	&fFractionLostPackets, 	sizeof(fFractionLostPackets));
	this->SetVal(qtssRTPStrTotalLostPackets, 	&fTotalLostPackets, 	sizeof(fTotalLostPackets));
	this->SetVal(qtssRTPStrJitter, 				&fJitter, 				sizeof(fJitter));
	this->SetVal(qtssRTPStrRecvBitRate, 		&fReceiverBitRate, 		sizeof(fReceiverBitRate));
	this->SetVal(qtssRTPStrAvgLateMilliseconds, &fAvgLateMsec, 			sizeof(fAvgLateMsec));
	this->SetVal(qtssRTPStrPercentPacketsLost, 	&fPercentPacketsLost, 	sizeof(fPercentPacketsLost));
	this->SetVal(qtssRTPStrAvgBufDelayInMsec, 	&fAvgBufDelayMsec, 		sizeof(fAvgBufDelayMsec));
	this->SetVal(qtssRTPStrGettingBetter, 		&fIsGettingBetter, 		sizeof(fIsGettingBetter));
	this->SetVal(qtssRTPStrGettingWorse, 		&fIsGettingWorse, 		sizeof(fIsGettingWorse));
	this->SetVal(qtssRTPStrNumEyes, 			&fNumEyes, 				sizeof(fNumEyes));
	this->SetVal(qtssRTPStrNumEyesActive, 		&fNumEyesActive, 		sizeof(fNumEyesActive));
	this->SetVal(qtssRTPStrNumEyesPaused, 		&fNumEyesPaused, 		sizeof(fNumEyesPaused));
	this->SetVal(qtssRTPStrTotPacketsRecv, 		&fTotalPacketsRecv, 	sizeof(fTotalPacketsRecv));
	this->SetVal(qtssRTPStrTotPacketsDropped, 	&fTotalPacketsDropped, 	sizeof(fTotalPacketsDropped));
	this->SetVal(qtssRTPStrTotPacketsLost, 		&fTotalPacketsLost, 	sizeof(fTotalPacketsLost));
	this->SetVal(qtssRTPStrClientBufFill, 		&fClientBufferFill, 	sizeof(fClientBufferFill));
	this->SetVal(qtssRTPStrFrameRate, 			&fFrameRate, 			sizeof(fFrameRate));
	this->SetVal(qtssRTPStrExpFrameRate, 		&fExpectedFrameRate, 	sizeof(fExpectedFrameRate));
	this->SetVal(qtssRTPStrAudioDryCount, 		&fAudioDryCount, 		sizeof(fAudioDryCount));
	this->SetVal(qtssRTPStrIsTCP, 				&fIsTCP, 				sizeof(fIsTCP));
	this->SetVal(qtssRTPStrCurrentPacketDelay,	&fCurrentPacketDelay, 	sizeof(fCurrentPacketDelay));
	this->SetVal(qtssRTPStrStreamRef, 			&fStreamRef, 			sizeof(fStreamRef));
	this->SetVal(qtssRTPStrTransportType, 		&fTransportType, 		sizeof(fTransportType));
	this->SetVal(qtssRTPStrStalePacketsDropped, &fStalePacketsDropped, 	sizeof(fStalePacketsDropped));
	this->SetVal(qtssRTPStrOverbufferTimeInSec, &fOverbufferTimeInSec, 	sizeof(fOverbufferTimeInSec));
	this->SetVal(qtssRTPStrCurPacketsLostInRTCPInterval , 		&fCurPacketsLostInRTCPInterval , 		sizeof(fPacketCountInRTCPInterval));
	this->SetVal(qtssRTPStrPacketCountInRTCPInterval, 		&fPacketCountInRTCPInterval, 		sizeof(fPacketCountInRTCPInterval));
}

RTPStream::~RTPStream()
{
	QTSS_Error err = QTSS_NoErr;
	if (fSockets != NULL)
	{
		// If there is an UDP socket pair associated with this stream, make sure to free it up
		Assert(fSockets->GetSocketB()->GetDemuxer() != NULL);
		fSockets->GetSocketB()->GetDemuxer()->
			UnregisterTask(fRemoteAddr, fRemoteRTCPPort, this);
		Assert(err == QTSS_NoErr);
	
		QTSServerInterface::GetServer()->GetSocketPool()->ReleaseUDPSocketPair(fSockets);
	}
	
	//
	// If there is a bandwidth tracker, delete it.
	delete fTracker;
	
#if RTP_PACKET_RESENDER_DEBUGGING
	fResender.LogClose(fFlowControlDurationMsec);
	//printf("Flow control duration msec: %qd. Max outstanding packets: %d\n", fFlowControlDurationMsec, fResender.GetMaxPacketsInList());
#endif

#if RTP_TCP_STREAM_DEBUG
	if ( fIsTCP )
		::printf( "DEBUG: ~RTPStream %li sends got EAGAIN'd.\n", (long)fNumPacketsDroppedOnTCPFlowControl );
#endif
}

void RTPStream::SetThinningParams()
{
	SInt32 lateToleranceMsec = (SInt32)(fLateToleranceInSec * 1000);
	
	QTSServerPrefs* thePrefs = QTSServerInterface::GetServer()->GetPrefs();
	fTurnThinningOffDelay = thePrefs->GetTCPMinThinDelayToleranceInMSec() + lateToleranceMsec;
	fIncreaseThinningDelay = thePrefs->GetTCPMaxThinDelayToleranceInMSec() + lateToleranceMsec;
	if (fPayloadType == qtssVideoPayloadType)
		fDropAllPacketsForThisStreamDelay = thePrefs->GetTCPVideoDelayToleranceInMSec() + lateToleranceMsec;
	else
		fDropAllPacketsForThisStreamDelay = thePrefs->GetTCPAudioDelayToleranceInMSec() + lateToleranceMsec;
}

QTSS_Error RTPStream::Setup(RTSPRequestInterface* request, QTSS_AddStreamFlags inFlags)
{
	//Get the URL for this track
	fStreamURLPtr.Len = kMaxStreamURLSizeInBytes;
	if (request->GetValue(qtssRTSPReqFileName, 0, fStreamURLPtr.Ptr, &fStreamURLPtr.Len) != QTSS_NoErr)
		return QTSSModuleUtils::SendErrorResponse(request, qtssClientBadRequest, qtssMsgFileNameTooLong);
	fStreamURL[fStreamURLPtr.Len] = '\0';//just in case someone wants to use string routines
	
	//
	// Store the late-tolerance value that came out of hte x-RTP-Options header,
	// so that when it comes time to determine our thinning params (when we PLAY),
	// we will know this
	fLateToleranceInSec = request->GetLateToleranceInSec();
	if (fLateToleranceInSec == -1)
		fLateToleranceInSec = 0;
	
	//
	// Setup the transport type
	fTransportType = request->GetTransportType();
	
	//
	// Only do retransmits if retransmits are enabled
	if ((fTransportType == qtssRTPTransportTypeReliableUDP) &&
		(!QTSServerInterface::GetServer()->GetPrefs()->IsReliableUDPEnabled()))
		fTransportType = qtssRTPTransportTypeUDP;
		
	//
	// Set the max overbuffer time. Only allow overbuffering
	// over reliable transports
	fOverbufferTimeInSec = QTSServerInterface::GetServer()->GetPrefs()->GetMaxOverBufferTimeInSec();
	if (fTransportType == qtssRTPTransportTypeUDP)
		fOverbufferTimeInSec = 0;
	
	// Check to see if this RTP stream should be sent over TCP.
	if (fTransportType == qtssRTPTransportTypeTCP)
	{
		fIsTCP = true;
		
		request->GetSession()->InterleaveSetup();
		
		// If it is, get 2 channel numbers from the RTSP session.
		fRTPChannel = request->GetSession()->GetChannelNumber();
		fRTCPChannel = request->GetSession()->GetChannelNumber();
		
		// If we are interleaving, this is all we need to do to setup.
		return QTSS_NoErr;
	}
	//Get and store the remote addresses provided by the client. The remote addr is the
	//same as the RTSP client's IP address, unless an alternate was specified in the
	//transport header.
	fRemoteAddr = request->GetSession()->GetSocket()->GetRemoteAddr();
	if (request->GetDestAddr() != INADDR_ANY)
	{
		// Sending data to other addresses could be used in malicious ways, therefore
		// it is up to the module as to whether this sort of request might be allowed
		if (!(inFlags & qtssASFlagsAllowDestination))
			return QTSSModuleUtils::SendErrorResponse(request, qtssClientBadRequest, qtssMsgAltDestNotAllowed);
		fRemoteAddr = request->GetDestAddr();
	}
	fRemoteRTPPort = request->GetClientPortA();
	fRemoteRTCPPort = request->GetClientPortB();

	if ((fRemoteRTPPort == 0) || (fRemoteRTCPPort == 0))
		return QTSSModuleUtils::SendErrorResponse(request, qtssClientBadRequest, qtssMsgNoClientPortInTransport);		
	
	//make sure that the client is advertising an even-numbered RTP port,
	//and that the RTCP port is actually one greater than the RTP port
	if ((fRemoteRTPPort & 1) != 0)
		return QTSSModuleUtils::SendErrorResponse(request, qtssClientBadRequest, qtssMsgRTPPortMustBeEven);		
		
	if (fRemoteRTCPPort != (fRemoteRTPPort + 1))
		return QTSSModuleUtils::SendErrorResponse(request, qtssClientBadRequest, qtssMsgRTCPPortMustBeOneBigger);		
	
	// Find the right source address for this stream. If it isn't specified in the
	// RTSP request, assume it is the same interface as for the RTSP request.
	UInt32 sourceAddr = request->GetSession()->GetSocket()->GetLocalAddr();
	if ((request->GetSourceAddr() != INADDR_ANY) && (SocketUtils::IsLocalIPAddr(request->GetSourceAddr())))
		sourceAddr = request->GetSourceAddr();

	// If the destination address is multicast, we need to setup multicast socket options
	// on the sockets. Because these options may be different for each stream, we need
	// a dedicated set of sockets
	if (SocketUtils::IsMulticastIPAddr(fRemoteAddr))
	{
		fSockets = QTSServerInterface::GetServer()->GetSocketPool()->CreateUDPSocketPair(sourceAddr, 0);
		
		if (fSockets != NULL)
		{
			//Set options on both sockets. Not really sure why we need to specify an
			//outgoing interface, because these sockets are already bound to an interface!
			QTSS_Error err = fSockets->GetSocketA()->SetTtl(request->GetTtl());
			if (err == QTSS_NoErr)
				err = fSockets->GetSocketB()->SetTtl(request->GetTtl());
			if (err == QTSS_NoErr)
				err = fSockets->GetSocketA()->SetMulticastInterface(fSockets->GetSocketA()->GetLocalAddr());
			if (err == QTSS_NoErr)
				err = fSockets->GetSocketB()->SetMulticastInterface(fSockets->GetSocketB()->GetLocalAddr());
			if (err != QTSS_NoErr)
				return QTSSModuleUtils::SendErrorResponse(request, qtssServerInternal, qtssMsgCantSetupMulticast);		
		}
	}
	else
		fSockets = QTSServerInterface::GetServer()->GetSocketPool()->GetUDPSocketPair(sourceAddr, 0, fRemoteAddr, 
																						fRemoteRTCPPort);

	if (fSockets == NULL)
		return QTSSModuleUtils::SendErrorResponse(request, qtssServerInternal, qtssMsgOutOfPorts);		
		
	else if (fTransportType == qtssRTPTransportTypeReliableUDP)
	{
		//
		// If we are using Reliable UDP, setup the UDP Resender
		UInt32 theWindowSize = request->GetWindowSize();
		if (theWindowSize == 0)
			theWindowSize = 1024 * QTSServerInterface::GetServer()->GetPrefs()->GetDefaultWindowSizeInK();
		
		//
		// FIXME - we probably want to get rid of this slow start flag in the API
		Bool16 useSlowStart = !(inFlags & qtssASFlagsDontUseSlowStart);
		if (!QTSServerInterface::GetServer()->GetPrefs()->IsSlowStartEnabled())
			useSlowStart = false;
		
		RTPBandwidthTracker* theTracker = fSession->GetBandwidthTracker();
		if (!QTSServerInterface::GetServer()->GetPrefs()->ShareReliableRTPWindowStats())
			theTracker = fTracker = NEW RTPBandwidthTracker(useSlowStart);
			
		theTracker->SetWindowSize(theWindowSize);
		fResender.SetBandwidthTracker( theTracker );
		fResender.SetDestination( fSockets->GetSocketA(), fRemoteAddr, fRemoteRTPPort );

#if RTP_PACKET_RESENDER_DEBUGGING
		if (QTSServerInterface::GetServer()->GetPrefs()->IsAckLoggingEnabled())
		{
			char		url[256];

			fResender.SetLog( fSession->GetSessionID() );
		
			StrPtrLen	*presoURL = fSession->GetValue(qtssCliSesPresentationURL);
			UInt32		clientAddr = request->GetSession()->GetSocket()->GetRemoteAddr();
			memcpy( url, presoURL->Ptr, presoURL->Len );
			url[presoURL->Len] = 0;
			printf( "RTPStream::Setup for %s will use ACKS, ip addr: %li.%li.%li.%li\n", url, (clientAddr & 0xff000000) >> 24
																								 , (clientAddr & 0x00ff0000) >> 16
																								 , (clientAddr & 0x0000ff00) >> 8
																								 , (clientAddr & 0x000000ff)
																								  );
		}
#endif
	}
	
	//finally, register with the demuxer to get RTCP packets from the proper address
	Assert(fSockets->GetSocketB()->GetDemuxer() != NULL);
	QTSS_Error err = fSockets->GetSocketB()->GetDemuxer()->RegisterTask(fRemoteAddr, fRemoteRTCPPort, this);
	//errors should only be returned if there is a routing problem, there should be none
	Assert(err == QTSS_NoErr);
	return QTSS_NoErr;
}

void RTPStream::SendSetupResponse( RTSPRequestInterface* inRequest )
{
	if (fSession->DoSessionSetupResponse(inRequest) != QTSS_NoErr)
		return;
		
	inRequest->AppendDateAndExpires();
	this->AppendTransport(inRequest);
	
	//
	// Append the x-RTP-Options header if there was a late-tolerance field
	if (inRequest->GetLateToleranceStr()->Len > 0)
		inRequest->AppendHeader(qtssXRTPOptionsHeader, inRequest->GetLateToleranceStr());
	
	//
	// Append the retransmit header if the client sent it
	StrPtrLen* theRetrHdr = inRequest->GetHeaderDictionary()->GetValue(qtssXRetransmitHeader);
	if ((theRetrHdr->Len > 0) && (fTransportType == qtssRTPTransportTypeReliableUDP))
		inRequest->AppendHeader(qtssXRetransmitHeader, theRetrHdr);
		
	inRequest->SendHeader();
}

void RTPStream::AppendTransport(RTSPRequestInterface* request)
{
	// We are either going to append the RTP / RTCP port numbers (UDP),
	// or the channel numbers (TCP, interleaved)
	if (!fIsTCP)
	{
		//
		// With UDP retransmits its important the client starts sending RTCPs
		// to the right address right away. The sure-firest way to get the client
		// to do this is to put the src address in the transport. So now we do that always.
		//
		// Old code
		//
		// If we are supposed to append the source IP address, set that up
		// StrPtrLen* theSrcIPAddress = NULL;
		// if (QTSServerInterface::GetServer()->GetPrefs()->GetAppendSrcAddrInTransport())
		//	theSrcIPAddress = fSockets->GetSocketA()->GetLocalAddrStr();

		//
		// New code
		char srcIPAddrBuf[20];
		StrPtrLen theSrcIPAddress(srcIPAddrBuf, 20);
		QTSServerInterface::GetServer()->GetPrefs()->GetTransportSrcAddr(&theSrcIPAddress);
		if (theSrcIPAddress.Len == 0)		
			theSrcIPAddress = *fSockets->GetSocketA()->GetLocalAddrStr();

		// Append UDP socket port numbers.
		UDPSocket* theRTPSocket = fSockets->GetSocketA();
		UDPSocket* theRTCPSocket = fSockets->GetSocketB();
		request->AppendTransportHeader(theRTPSocket->GetLocalPortStr(), theRTCPSocket->GetLocalPortStr(), NULL, NULL, &theSrcIPAddress);
	}
	else if (fRTCPChannel < kNumPrebuiltChNums)
		// We keep a certain number of channel number strings prebuilt, so most of the time
		// we won't have to call sprintf
		request->AppendTransportHeader(NULL, NULL, &sChannelNums[fRTPChannel],  &sChannelNums[fRTCPChannel]);
	else
	{
		// If these channel numbers fall outside prebuilt range, we will have to call sprintf.
		char rtpChannelBuf[10];
		char rtcpChannelBuf[10];
		::sprintf(rtpChannelBuf, "%d", fRTPChannel);		
		::sprintf(rtcpChannelBuf, "%d", fRTCPChannel);
		
		StrPtrLen rtpChannel(rtpChannelBuf);
		StrPtrLen rtcpChannel(rtcpChannelBuf);

		request->AppendTransportHeader(NULL, NULL, &rtpChannel, &rtcpChannel);
	}
}

void	RTPStream::AppendRTPInfo(QTSS_RTSPHeader inHeader, RTSPRequestInterface* request, UInt32 inFlags)
{
	//format strings for the various numbers we need to send back to the client
	char rtpTimeBuf[20];
	StrPtrLen rtpTimeBufPtr;
	if (inFlags & qtssPlayRespWriteTrackInfo)
	{
		::sprintf(rtpTimeBuf, "%ld", fFirstTimeStamp);
		rtpTimeBufPtr.Set(rtpTimeBuf, ::strlen(rtpTimeBuf));
		Assert(rtpTimeBufPtr.Len < 20);
	}	
	
	char seqNumberBuf[20];
	StrPtrLen seqNumberBufPtr;
	if (inFlags & qtssPlayRespWriteTrackInfo)
	{
		::sprintf(seqNumberBuf, "%d", fFirstSeqNumber);
		seqNumberBufPtr.Set(seqNumberBuf, ::strlen(seqNumberBuf));
		Assert(seqNumberBufPtr.Len < 20);
	}
	
	//only advertise our ssrc to the client if we are in fact sending RTCP
	//sender reports. Otherwise, this ssrc is invalid!
	UInt32 theSsrcStringLen = fSsrcStringPtr.Len;
	if (!(inFlags & qtssPlayRespWriteTrackInfo))
		fSsrcStringPtr.Len = 0;
	
	request->AppendRTPInfoHeader(inHeader, &fStreamURLPtr, &seqNumberBufPtr,
									&fSsrcStringPtr, &rtpTimeBufPtr);
	fSsrcStringPtr.Len = theSsrcStringLen;
}

/*********************************
/
/	InterleavedWrite
/
/	Write the given RTP packet out on the RTSP channel in interleaved format.
/	update quality levels and statistics
/	on success refresh the RTP session timeout to keep it alive
/
*/

QTSS_Error	RTPStream::InterleavedWrite(void* inBuffer, UInt32 inLen, UInt32* outLenWritten, unsigned char channel)
{
	
	// Check to see if fSession is NULL. This may happen if we are about to go away.
	// (Look at RTPSession::Teardown)
	// return EAGAIN because it is known to be well tolerated.
	Assert(fSession != NULL);
	if (!fSession->GetSessionMutex()->TryLock())
		return EAGAIN;
		
	if (fSession->GetRTSPSession() == NULL)
	{
		fSession->GetSessionMutex()->Unlock();
		return EAGAIN;
	}
		
	QTSS_Error err = fSession->GetRTSPSession()->InterleavedWrite( inBuffer, inLen, outLenWritten, channel);
	
	// Make sure to unlock the mutex
	fSession->GetSessionMutex()->Unlock();

#if DEBUG
	if ( err == EAGAIN )
	{
		fNumPacketsDroppedOnTCPFlowControl++;
	}
#endif		

	// reset the timeouts when the connection is still alive
	// wehn transmitting over HTTP, we're not going to get
	// RTCPs that would normally Refresh the session time.
	if ( err == QTSS_NoErr )
		fSession->RefreshTimeout(); // RTSP session gets refreshed internally in WriteV

	#if RTP_TCP_STREAM_DEBUG
	//printf( "DEBUG: RTPStream fCurrentPacketDelay %li, fQualityLevel %i\n", (long)fCurrentPacketDelay, (int)fQualityLevel );
	#endif

	return err;
	
}

void 	RTPStream::SendRetransmits()
{
	if ( fTransportType == qtssRTPTransportTypeReliableUDP )
		fResender.ResendDueEntries();
}

QTSS_Error RTPStream::ReliableRTPWrite(void* inBuffer, UInt32 inLen)
{
	QTSS_Error err = QTSS_NoErr;

	// Here we are using the session mutex to protect the Resender, which
	// is not preemptive safe and will be accessed by several threads
	// possibly - the IncomingRTCPTask, and whatever thread is calling QTSS_Write.
	Assert(fSession != NULL);
	if (!fSession->GetSessionMutex()->TryLock())
		return EAGAIN;

	// this must ALSO be called in response to a packet timeout
	// event that can be resecheduled as necessary by the fResender
	// for -hacking- purposes we'l do it just as we write packets,
	// but we won't be able to play low bit-rate movies ( like MIDI )
	// until this is a schedulable task
	
	// Send retransmits for all streams on this session
	RTPStream** retransStream = NULL;
	UInt32 retransStreamLen = 0;

	//
	// Send retransmits if we need to
	for (int streamIter = 0; fSession->GetValuePtr(qtssCliSesStreamObjects, streamIter, (void**)&retransStream, &retransStreamLen) == QTSS_NoErr; streamIter++)
	{
		//printf("Resending packets for stream: %d\n",(*retransStream)->fTrackID);
		//printf("RTPStream::ReliableRTPWrite. Calling ResendDueEntries\n");
		(*retransStream)->fResender.ResendDueEntries();
	}
	
	if ( !fSawFirstPacket )
	{
		fSawFirstPacket = true;
		fStreamCumDuration = 0;
		fStreamCumDuration = OS::Milliseconds() - fSession->GetPlayTime();
		//fInfoDisplayTimer.ResetToDuration( 1000 - fStreamCumDuration % 1000 );
	}
	
#if RTP_PACKET_RESENDER_DEBUGGING
	fResender.SetDebugInfo(fTrackID, fRemoteRTCPPort, fCurrentPacketDelay);
	fBytesSentThisInterval = fResender.SpillGuts(fBytesSentThisInterval);
#endif

	if ( fResender.IsFlowControlled() )
	{
		//printf("Flow controlled\n");
#if DEBUG
		if (fFlowControlStartedMsec == 0)
		{
			//printf("Flow control start\n");
			fFlowControlStartedMsec = OS::Milliseconds();
		}
#endif
		err = QTSS_WouldBlock;
	}
	else
	{
#if DEBUG	
		if (fFlowControlStartedMsec != 0)
		{
			fFlowControlDurationMsec += OS::Milliseconds() - fFlowControlStartedMsec;
			fFlowControlStartedMsec = 0;
		}
#endif
		
		//
		// Assign a lifetime to the packet using the current delay of the packet and
		// the time until this packet becomes stale.
		fBytesSentThisInterval += inLen;
		fResender.AddPacket( inBuffer, inLen, fDropAllPacketsForThisStreamDelay - fCurrentPacketDelay );

		(void)fSockets->GetSocketA()->SendTo(fRemoteAddr, fRemoteRTPPort, inBuffer, inLen);
	}

	// Make sure to unlock the mutex
	fSession->GetSessionMutex()->Unlock();

	return err;
}


QTSS_Error	RTPStream::Write(void* inBuffer, UInt32 inLen, UInt32* outLenWritten, UInt32 inFlags)
{
	QTSS_Error err = QTSS_NoErr;
	
	if (inFlags == qtssWriteFlagsIsRTCP)
	{
		if ( fTransportType == qtssRTPTransportTypeTCP )// write out in interleave format on the RTSP TCP channel
			err = this->InterleavedWrite( inBuffer, inLen, outLenWritten, fRTCPChannel );
		else if ( inLen > 0 )
			(void)fSockets->GetSocketB()->SendTo(fRemoteAddr, fRemoteRTCPPort, inBuffer, inLen);
	}
	//
	// If this packet is just too old, drop it and make the module think
	// that it has been sent.
	else if (fCurrentPacketDelay > fDropAllPacketsForThisStreamDelay)
		fStalePacketsDropped++;

#if DEBUG
	//
	// Creating loss artificially. Drop a certain % of packets based on a pref
	UInt32 theRand = (::rand() % 100) + 1;
	if (theRand < QTSServerInterface::GetServer()->GetPrefs()->GetRTPPacketDropPercent())
		return QTSS_NoErr;
#endif

	else if (inFlags == qtssWriteFlagsIsRTP)
	{
		if ( fTransportType == qtssRTPTransportTypeTCP )	// write out in interleave format on the RTSP TCP channel
			err = this->InterleavedWrite( inBuffer, inLen, outLenWritten, fRTPChannel );		
		else if ( fTransportType == qtssRTPTransportTypeReliableUDP )
			err = this->ReliableRTPWrite( inBuffer, inLen );
		else if ( inLen > 0 )
			(void)fSockets->GetSocketA()->SendTo(fRemoteAddr, fRemoteRTPPort, inBuffer, inLen);
	
		if ( err == QTSS_NoErr && inLen > 0 )
		{
			SInt64 theTime = OS::Milliseconds();

			//
			// Set the quality level based on the current packet delay.
			// This should never really execute for normal UDP sending, which is good
			// because the QTSSFlowControlModule handles thinning for that transport.
			if ( fCurrentPacketDelay < fTurnThinningOffDelay )
			{
				if (fTimeStreamCaughtUp == 0)
					fTimeStreamCaughtUp = theTime;
					
				
				if (((theTime - fTimeStreamCaughtUp) >
					QTSServerInterface::GetServer()->GetPrefs()->GetTCPThickIntervalInSec()) &&
					(fQualityLevel > 0))
				{
					fTimeStreamCaughtUp = 0;
					fQualityLevel--;
				}
			}
			else
				fTimeStreamCaughtUp = 0;
				
			if ( fCurrentPacketDelay > fIncreaseThinningDelay )
			{
				fQualityLevel++;
				if (fQualityLevel > fNumQualityLevels)
					fQualityLevel = fNumQualityLevels;
			}
			
			// Update statistics if we were actually able to send the data (don't
			// update if the socket is flow controlled or some such thing)
			
			fSession->UpdatePacketsSent(1);
			fSession->UpdateBytesSent(inLen);
			QTSServerInterface::GetServer()->IncrementTotalRTPBytes(inLen);
			QTSServerInterface::GetServer()->IncrementTotalPackets();
			
			// Record the RTP timestamp for RTCPs
			UInt32* timeStampP = (UInt32*)inBuffer;
			fLastRTPTimestamp = ntohl(timeStampP[1]);
			
			//stream statistics
			fPacketCount++;
			fByteCount += inLen;

			// Send an RTCP sender report if it's time. Again, we only want to send an
			// RTCP if the RTP packet was sent sucessfully
			if ((fSession->GetPlayFlags() & qtssPlayFlagsSendRTCP) &&
				(theTime > (fLastSenderReportTime + (kSenderReportIntervalInSecs * 1000))))
			{
				fLastSenderReportTime = theTime;
				this->SendRTCPSR(theTime);
			}
		}
	}
	else
		return QTSS_BadArgument;//qtssWriteFlagsIsRTCP or qtssWriteFlagsIsRTP wasn't specified

	if (outLenWritten != NULL)
		*outLenWritten = inLen;
	return err;
}

void RTPStream::SendRTCPSR(const SInt64& inTime, Bool16 inAppendBye)
{
	RTCPSRPacket* theSR = fSession->GetSRPacket();
	theSR->SetSSRC(fSsrc);
	theSR->SetNTPTimestamp(fSession->GetNTPPlayTime() +
							OS::TimeMilli_To_Fixed64Secs(inTime - fSession->GetPlayTime()));
	theSR->SetRTPTimestamp(fLastRTPTimestamp);
	theSR->SetPacketCount(fPacketCount);
	theSR->SetByteCount(fByteCount);
	
	UInt32 thePacketLen = theSR->GetSRPacketLen();
	if (inAppendBye)
		thePacketLen = theSR->GetSRWithByePacketLen();
		
	if ( fTransportType == qtssRTPTransportTypeTCP )	// write out in interleave format on the RTSP TCP channel
	{	UInt32	wasWritten;
	
		(void)this->InterleavedWrite( theSR->GetSRPacket(), thePacketLen, &wasWritten, fRTCPChannel );
	}
	else
		(void)fSockets->GetSocketB()->SendTo(fRemoteAddr, fRemoteRTCPPort,
										theSR->GetSRPacket(), thePacketLen);
}


void RTPStream::ProcessPacket(StrPtrLen* inPacket)
{
	StrPtrLen currentPtr(*inPacket);
	SInt64 curTime = OS::Milliseconds();
	
	while ( currentPtr.Len > 0 )
	{
		/*
			Due to the variable-type nature of RTCP packets, this is a bit unusual...
			We initially treat the packet as a generic RTCPPacket in order to determine its'
			actual packet type.  Once that is figgered out, we treat it as its' actual packet type
		*/
		RTCPPacket rtcpPacket;
		if (!rtcpPacket.ParsePacket((UInt8*)currentPtr.Ptr, currentPtr.Len))
			return;//abort if we discover a malformed RTCP packet
		
		switch (rtcpPacket.GetPacketType())
		{
			case RTCPPacket::kReceiverPacketType:
			{		
				RTCPReceiverPacket receiverPacket;
				if (!receiverPacket.ParseReceiverReport((UInt8*)currentPtr.Ptr, currentPtr.Len))
					return;//abort if we discover a malformed receiver report

				fFractionLostPackets = receiverPacket.GetCumulativeFractionLostPackets();
				fJitter = receiverPacket.GetCumulativeJitter();
				
				UInt32 curTotalLostPackets = receiverPacket.GetCumulativeTotalLostPackets();
				//zero means either no loss or loss info wasn't included in receiver report
						
				if (curTotalLostPackets != 0)
				{	
					fPriorTotalLostPackets = fTotalLostPackets;
					fTotalLostPackets = curTotalLostPackets;
					//increment the server total by the new delta
					QTSServerInterface::GetServer()->IncrementTotalRTPPacketsLost(fTotalLostPackets - fPriorTotalLostPackets);

					if (fTotalLostPackets > fPriorTotalLostPackets)
						fCurPacketsLostInRTCPInterval = (fTotalLostPackets - fPriorTotalLostPackets);
					else
						fPriorTotalLostPackets = fTotalLostPackets;
				}
				else
					fCurPacketsLostInRTCPInterval = 0;
								
				fPacketCountInRTCPInterval = fPacketCount - fLastPacketCount;
				fLastPacketCount = fPacketCount;

#ifdef DEBUG_RTCP_PACKETS
				receiverPacket.Dump();
#endif
			}
			break;
			
			case RTCPPacket::kAPPPacketType:
			{	
				//
				// Check and see if this is an Ack packet. If it is, update the UDP Resender
				RTCPAckPacket theAckPacket;
				if (theAckPacket.ParseAckPacket(rtcpPacket.GetPacketBuffer(), (rtcpPacket.GetPacketLength() * 4) + RTCPPacket::kRTCPHeaderSizeInBytes ))
				{
					//
					// Only check for ack packets if we are using Reliable UDP
					if (fTransportType == qtssRTPTransportTypeReliableUDP)
					{
						UInt16 theSeqNum = theAckPacket.GetAckSeqNum();
						fResender.AckPacket(theSeqNum, curTime);
						//printf("Got ack: %d\n",theSeqNum);
						
						for (UInt32 maskCount = 0; maskCount < theAckPacket.GetAckMaskSizeInBits(); maskCount++)
						{
							if (theAckPacket.IsNthBitEnabled(maskCount))
							{
								fResender.AckPacket(theSeqNum + maskCount + 1, curTime);
								//printf("Got ack in mask: %d\n",theSeqNum + maskCount + 1);
							}
						}
					}
				}
				else
				{	
					//
					// If it isn't an ACK, assume its the qtss APP packet
					RTCPCompressedQTSSPacket compressedQTSSPacket;
					if (!compressedQTSSPacket.ParseCompressedQTSSPacket((UInt8*)currentPtr.Ptr, currentPtr.Len))
						return;//abort if we discover a malformed app packet

					fPriorTotalPacketsLost = fTotalPacketsLost; // used to calculate the totalpacket received during the receiver report interval
					fReceiverBitRate = 		compressedQTSSPacket.GetReceiverBitRate();
					fAvgLateMsec = 			compressedQTSSPacket.GetAverageLateMilliseconds();
					
					fPercentPacketsLost = 	compressedQTSSPacket.GetPercentPacketsLost();
					fAvgBufDelayMsec = 		compressedQTSSPacket.GetAverageBufferDelayMilliseconds();
					fIsGettingBetter = (UInt16)compressedQTSSPacket.GetIsGettingBetter();
					fIsGettingWorse = (UInt16)compressedQTSSPacket.GetIsGettingWorse();
					fNumEyes = 				compressedQTSSPacket.GetNumEyes();
					fNumEyesActive = 		compressedQTSSPacket.GetNumEyesActive();
					fNumEyesPaused = 		compressedQTSSPacket.GetNumEyesPaused();
					fTotalPacketsRecv = 	compressedQTSSPacket.GetTotalPacketReceived();
					fTotalPacketsDropped = 	compressedQTSSPacket.GetTotalPacketsDropped();
					fTotalPacketsLost = 	compressedQTSSPacket.GetTotalPacketsLost();
					fClientBufferFill = 	compressedQTSSPacket.GetClientBufferFill();
					fFrameRate = 			compressedQTSSPacket.GetFrameRate();
					fExpectedFrameRate = 	compressedQTSSPacket.GetExpectedFrameRate();
					fAudioDryCount = 		compressedQTSSPacket.GetAudioDryCount();
					
#ifdef DEBUG_RTCP_PACKETS
				compressedQTSSPacket.Dump();
#endif
				}
			}
			break;
			
			case RTCPPacket::kSDESPacketType:
			{
#ifdef DEBUG_RTCP_PACKETS
				SourceDescriptionPacket sedsPacket;
				if (!sedsPacket.ParsePacket((UInt8*)currentPtr.Ptr, currentPtr.Len))
					return;//abort if we discover a malformed app packet

				sedsPacket.Dump();
#endif
			}
			break;
			
			//default:
			//	WarnV(false, "Unknown RTCP Packet Type");
			break;
		
		}
		
		
		currentPtr.Ptr += (rtcpPacket.GetPacketLength() * 4 ) + 4;
		currentPtr.Len -= (rtcpPacket.GetPacketLength() * 4 ) + 4;
	}

	// Invoke the RTCP modules, allowing them to process this packet
	QTSS_RoleParams theParams;
	theParams.rtcpProcessParams.inRTPStream = this;
	theParams.rtcpProcessParams.inClientSession = fSession;
	theParams.rtcpProcessParams.inRTCPPacketData = inPacket->Ptr;
	theParams.rtcpProcessParams.inRTCPPacketDataLen = inPacket->Len;
	
	// We don't allow async events from this role, so just set an empty module state.
	OSThread::GetCurrent()->SetThreadData(&sRTCPProcessModuleState);
	
	// Modules are guarenteed atomic access to the session. Also, the RTSP Session accessed
	// below could go away at any time. So we need to lock the RTP session mutex.
	// *BUT*, when this function is called the caller already has the UDP socket pool &
	// UDP Demuxer mutexes. Blocking on grabbing this mutex could cause a deadlock.
	// So, dump this RTCP packet if we can't get the mutex.
	if (!fSession->GetSessionMutex()->TryLock())
		return;

	//no matter what happens (whether or not this is a valid packet) reset the timeouts
	fSession->RefreshTimeout();
	if (fSession->GetRTSPSession() != NULL)
		fSession->GetRTSPSession()->RefreshTimeout();
	
	// Invoke RTCP processing modules
	for (UInt32 x = 0; x < QTSServerInterface::GetNumModulesInRole(QTSSModule::kRTCPProcessRole); x++)
		(void)QTSServerInterface::GetModule(QTSSModule::kRTCPProcessRole, x)->CallDispatch(QTSS_RTCPProcess_Role, &theParams);

	fSession->GetSessionMutex()->Unlock();
}
