/*
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * Copyright (c) 1999-2001 Apple Computer, Inc.  All Rights Reserved. The
 * contents of this file constitute Original Code as defined in and are
 * subject to the Apple Public Source License Version 1.2 (the 'License').
 * You may not use this file except in compliance with the License.  Please
 * obtain a copy of the License at http://www.apple.com/publicsource and
 * read it before using this file.
 *
 * This Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.  Please
 * see the License for the specific language governing rights and
 * limitations under the License.
 *
 *
 * @APPLE_LICENSE_HEADER_END@
 *
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
	/* 31 */ { "qtssRTPStrTransportType", 			NULL, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 32 */ { "qtssRTPStrStalePacketsDropped", 	NULL, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 33 */ { "qtssRTPStrCurrentAckTimeout", 		NULL, 	qtssAttrDataTypeUInt32, qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 34 */ { "qtssRTPStrCurPacketsLostInRTCPInterval", 	NULL, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 35 */ { "qtssRTPStrPacketCountInRTCPInterval", 		NULL, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 36 */ { "qtssRTPStrSvrRTPPort", 				NULL, 	qtssAttrDataTypeUInt16,	qtssAttrModeRead | qtssAttrModePreempSafe  },
	/* 37 */ { "qtssRTPStrClientRTPPort", 			NULL, 	qtssAttrDataTypeUInt16,	qtssAttrModeRead | qtssAttrModePreempSafe  }

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
	fLastQualityChange(0),
	fSockets(NULL),
	fSession(inSession),
	fBytesSentThisInterval(0),
	fDisplayCount(0),
	fSawFirstPacket(false),
	fTracker(NULL),
	fRemoteAddr(0),
	fRemoteRTPPort(0),
	fRemoteRTCPPort(0),
	fLocalRTPPort(0),
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
	//fPriorTotalLostPackets(0),
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
	fClientSSRC(0),
	fIsTCP(false),
	fTransportType(qtssRTPTransportTypeUDP),
	
	fTurnThinningOffDelay_TCP(0),
	fIncreaseThinningDelay_TCP(0),
	fDropAllPacketsForThisStreamDelay_TCP(0),
	fStalePacketsDropped_TCP(0),
	fTimeStreamCaughtUp_TCP(0),
	fLastQualityLevelIncreaseTime_TCP(0),

	fThinAllTheWayDelay(0),
	fOptimalDelay(0),
	fStartThickingDelay(0),
	fQualityCheckInterval(0),
	fDropAllPacketsForThisStreamDelay(0),
	fStalePacketsDropped(0),
	fLastQualityCheckTime(0),
	fLastCurrentPacketDelay(0),
	fJustIncreasedQualityLevel(false),
	fBufferDelay(3.0),
	fLateToleranceInSec(0),
	fStreamRef(this),
	fCurrentAckTimeout(0),
	fMaxSendAheadTimeMSec(0),
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
	this->SetVal(qtssRTPStrStreamRef, 			&fStreamRef, 			sizeof(fStreamRef));
	this->SetVal(qtssRTPStrTransportType, 		&fTransportType, 		sizeof(fTransportType));
	this->SetVal(qtssRTPStrStalePacketsDropped, &fStalePacketsDropped, 	sizeof(fStalePacketsDropped));
	this->SetVal(qtssRTPStrCurrentAckTimeout,	&fCurrentAckTimeout, 	sizeof(fCurrentAckTimeout));
	this->SetVal(qtssRTPStrCurPacketsLostInRTCPInterval , 		&fCurPacketsLostInRTCPInterval , 		sizeof(fPacketCountInRTCPInterval));
	this->SetVal(qtssRTPStrPacketCountInRTCPInterval, 		&fPacketCountInRTCPInterval, 		sizeof(fPacketCountInRTCPInterval));
	this->SetVal(qtssRTPStrSvrRTPPort, 			&fLocalRTPPort, 		sizeof(fLocalRTPPort));
	this->SetVal(qtssRTPStrClientRTPPort, 		&fRemoteRTPPort, 		sizeof(fRemoteRTPPort));
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
	
#if RTP_PACKET_RESENDER_DEBUGGING
	//fResender.LogClose(fFlowControlDurationMsec);
	//printf("Flow control duration msec: %qd. Max outstanding packets: %d\n", fFlowControlDurationMsec, fResender.GetMaxPacketsInList());
#endif

#if RTP_TCP_STREAM_DEBUG
	if ( fIsTCP )
		::printf( "DEBUG: ~RTPStream %li sends got EAGAIN'd.\n", (long)fNumPacketsDroppedOnTCPFlowControl );
#endif
}

static UInt32 sStreamCount = 0;
void RTPStream::DropQualityLevelValue() //HTTP-TCP
{		
	if (fQualityLevel > 0) 
		fQualityLevel--; // This raises the effective quality/bitrate of the stream & movie
	//printf("RTPStream::DropQualityLevelValue=%ld \n",fQualityLevel);
};

void RTPStream::RaiseQualityLevelValue()
{	
	if (fQualityLevel < (SInt32) fNumQualityLevels) 
		fQualityLevel++; // This lowers the effective quality/bitrate of the stream & movie
		
	//printf("RTPStream::RaiseQualityLevelValue=%ld \n",fQualityLevel);

};



void RTPStream::SetTCPThinningParams()
{	
	QTSServerPrefs* thePrefs = QTSServerInterface::GetServer()->GetPrefs();
	fIncreaseThinningDelay_TCP = thePrefs->GetTCPMaxThinDelayToleranceInMSec();
	if (fPayloadType == qtssVideoPayloadType)
		fDropAllPacketsForThisStreamDelay_TCP = thePrefs->GetTCPVideoDelayToleranceInMSec();
	else
		fDropAllPacketsForThisStreamDelay_TCP = thePrefs->GetTCPAudioDelayToleranceInMSec();

	fTurnThinningOffDelay_TCP = fDropAllPacketsForThisStreamDelay_TCP;
	//fMaxSendAheadTimeMSec = 1000 * thePrefs->GetMaxSendAheadTimeInSecs();  // need rate control
	fMaxSendAheadTimeMSec = 2 * 1000;
	//printf("RTPStream::SetTCPThinningParams fIncreaseThinningDelay: %d. fTurnThinningOffDelay: %d. fDropAllPacketsForThisStreamDelay %d\n", fIncreaseThinningDelay_TCP, fTurnThinningOffDelay_TCP, fDropAllPacketsForThisStreamDelay_TCP);
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
	if (fLateToleranceInSec == -1.0)
		fLateToleranceInSec = 1.5;

	//
	// Setup the transport type
	fTransportType = request->GetTransportType();
	
	//
	// Only allow reliable UDP if it is enabled
	if ((fTransportType == qtssRTPTransportTypeReliableUDP) && (!QTSServerInterface::GetServer()->GetPrefs()->IsReliableUDPEnabled()))
		fTransportType = qtssRTPTransportTypeUDP;

        //
        // Check to see if we are inside a valid reliable UDP directory
        if ((fTransportType == qtssRTPTransportTypeReliableUDP) && (!QTSServerInterface::GetServer()->GetPrefs()->IsPathInsideReliableUDPDir(request->GetValue(qtssRTSPReqFilePath))))
            fTransportType = qtssRTPTransportTypeUDP;

	//
	// Check to see if caller is forcing raw UDP transport
	if ((fTransportType == qtssRTPTransportTypeReliableUDP) && (inFlags & qtssASFlagsForceUDPTransport))
		fTransportType = qtssRTPTransportTypeUDP;
		
	//
	// Only do overbuffering if all our tracks are reliable
	if (fTransportType == qtssRTPTransportTypeUDP)
		fSession->GetOverbufferWindow()->SetWindowSize(0);
		
	// Check to see if this RTP stream should be sent over TCP.
	if (fTransportType == qtssRTPTransportTypeTCP)
	{
		fIsTCP = true;
		
		// If it is, get 2 channel numbers from the RTSP session.
		fRTPChannel = request->GetSession()->GetTwoChannelNumbers(fSession->GetValue(qtssCliSesRTSPSessionID));
		fRTCPChannel = fRTPChannel+1;
		
		// If we are interleaving, this is all we need to do to setup.
		return QTSS_NoErr;
	}
	
	//
	// This track is not interleaved, so let the session know that all
	// tracks are not interleaved. This affects our scheduling of packets
	fSession->SetAllTracksInterleaved(false);
	
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
		
		fTracker = fSession->GetBandwidthTracker();
			
		fTracker->SetWindowSize( theWindowSize );
		fResender.SetBandwidthTracker( fTracker );
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
	
	//
	// Record the Server RTP port
	fLocalRTPPort = fSockets->GetSocketA()->GetLocalPort();

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
		inRequest->AppendHeader(qtssXTransportOptionsHeader, inRequest->GetLateToleranceStr());
	
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

		if(request->IsPushRequest())
		{
			char rtpPortStr[10];
			char rtcpPortStr[10];
			::sprintf(rtpPortStr, "%d", request->GetSetUpServerPort());		
			::sprintf(rtcpPortStr, "%d", request->GetSetUpServerPort()+1);
			//printf(" RTPStream::AppendTransport rtpPort=%u rtcpPort=%u \n",request->GetSetUpServerPort(),request->GetSetUpServerPort()+1);
			StrPtrLen rtpSPL(rtpPortStr);
			StrPtrLen rtcpSPL(rtcpPortStr);
			// Append UDP socket port numbers.
			request->AppendTransportHeader(&rtpSPL, &rtcpSPL, NULL, NULL, &theSrcIPAddress);
		}		
		else
		{
			// Append UDP socket port numbers.
			UDPSocket* theRTPSocket = fSockets->GetSocketA();
			UDPSocket* theRTCPSocket = fSockets->GetSocketB();
			request->AppendTransportHeader(theRTPSocket->GetLocalPortStr(), theRTCPSocket->GetLocalPortStr(), NULL, NULL, &theSrcIPAddress);
		}
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

//ReliableRTPWrite must be called from a fSession mutex protected caller
QTSS_Error	RTPStream::InterleavedWrite(void* inBuffer, UInt32 inLen, UInt32* outLenWritten, unsigned char channel)
{
	
	if (fSession->GetRTSPSession() == NULL) // RTSPSession required for interleaved write
	{
		return EAGAIN;
	}

	//char blahblah[2048];
	
	QTSS_Error err = fSession->GetRTSPSession()->InterleavedWrite( inBuffer, inLen, outLenWritten, channel);
	//QTSS_Error err = fSession->GetRTSPSession()->InterleavedWrite( blahblah, 2044, outLenWritten, channel);
#if DEBUG
	//if (outLenWritten != NULL)
	//{
	//	Assert((*outLenWritten == 0) || (*outLenWritten == 2044));
	//}
#endif
	

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

//SendRetransmits must be called from a fSession mutex protected caller
void 	RTPStream::SendRetransmits()
{

	if ( fTransportType == qtssRTPTransportTypeReliableUDP )
		fResender.ResendDueEntries(); 
		
	
}

//ReliableRTPWrite must be called from a fSession mutex protected caller
QTSS_Error RTPStream::ReliableRTPWrite(void* inBuffer, UInt32 inLen, const SInt64& curPacketDelay)
{
	QTSS_Error err = QTSS_NoErr;

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
		if (retransStream != NULL && *retransStream != NULL)
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
	fResender.SetDebugInfo(fTrackID, fRemoteRTCPPort, curPacketDelay);
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
		fResender.AddPacket( inBuffer, inLen, fDropAllPacketsForThisStreamDelay - curPacketDelay );

		(void)fSockets->GetSocketA()->SendTo(fRemoteAddr, fRemoteRTPPort, inBuffer, inLen);
	}


	return err;
}

void RTPStream::SetThinningParams()
{
	if ( fTransportType == qtssRTPTransportTypeTCP )
	{	SetTCPThinningParams();
		return;
	}
	
	QTSServerPrefs* thePrefs = QTSServerInterface::GetServer()->GetPrefs();
	
	SInt32 theBufferTimeInMsec = ((SInt32)fBufferDelay) * 1000;
	
	fDropAllPacketsForThisStreamDelay = theBufferTimeInMsec - thePrefs->GetDropAllPacketsTimeInMsec();
	fThinAllTheWayDelay = theBufferTimeInMsec - thePrefs->GetThinAllTheWayTimeInMsec();
	fOptimalDelay = theBufferTimeInMsec - thePrefs->GetOptimalDelayTimeInMsec();
	fStartThickingDelay = theBufferTimeInMsec - thePrefs->GetStartThickingTimeInMsec();
	fQualityCheckInterval = thePrefs->GetQualityCheckIntervalInMsec();
	fLastQualityCheckTime = 0;
}

Bool16 RTPStream::UpdateQualityLevel(const SInt64& inTransmitTime, const SInt64& inCurrentPacketDelay,
										const SInt64& inCurrentTime, UInt32 inPacketSize)
{
	Assert(fNumQualityLevels > 0);
	
	if (inTransmitTime <= fSession->GetPlayTime())
		return true;
	if (fTransportType == qtssRTPTransportTypeUDP)
		return true;
	
	if ((fLastQualityCheckTime == 0) || (inCurrentPacketDelay > fThinAllTheWayDelay))
	{
		//
		// Reset the interval for checking quality levels
		fLastQualityCheckTime = inCurrentTime;
		fLastCurrentPacketDelay = inCurrentPacketDelay;

		if (inCurrentPacketDelay > fThinAllTheWayDelay ) 
		{
			//
			// If we have fallen behind enough such that we risk trasmitting
			// stale packets to the client, AGGRESSIVELY thin the stream
			fQualityLevel = fNumQualityLevels;
#if RTP_PACKET_RESENDER_DEBUGGING
			fResender.logprintf("Increasing quality level to: %d. Curpackdelay: %qd\n",fQualityLevel, inCurrentPacketDelay);
#endif			
			if (inCurrentPacketDelay > fDropAllPacketsForThisStreamDelay)
			{
				fStalePacketsDropped++;
				return false; // We should not send this packet
			}
		}
	}
	
	if ((inCurrentTime - fLastQualityCheckTime)	> fQualityCheckInterval)
	{
		SInt64 theFutureDelay = inCurrentPacketDelay + (inCurrentPacketDelay - fLastCurrentPacketDelay);
		fLastCurrentPacketDelay = inCurrentPacketDelay;
		fLastQualityCheckTime = inCurrentTime;

		if ((fSession->GetOverbufferWindow()->AvailableSpaceInWindow() - (SInt32)inPacketSize) < 0)
		{
			if (fQualityLevel > 0)
			{
				fQualityLevel--;
#if RTP_PACKET_RESENDER_DEBUGGING
				fResender.logprintf("Overbuffer window full. Decreasing quality level to: %d. Curpackdelay: %qd. Futdelay: %qd\n",fQualityLevel, inCurrentPacketDelay, theFutureDelay);
#endif
			}
		}
		else if (theFutureDelay > fThinAllTheWayDelay)
		{
			fQualityLevel = fNumQualityLevels;
#if RTP_PACKET_RESENDER_DEBUGGING
			fResender.logprintf("Increasing quality level to: %d. Curpackdelay: %qd. Futdelay: %qd\n",fQualityLevel, inCurrentPacketDelay, theFutureDelay);
#endif
		}
		else if ((theFutureDelay > fOptimalDelay) && (fQualityLevel < (SInt32)fNumQualityLevels))
		{
			if ((!fJustIncreasedQualityLevel) || (theFutureDelay > inCurrentPacketDelay))
			{
				fQualityLevel++;
				fJustIncreasedQualityLevel = true;
#if RTP_PACKET_RESENDER_DEBUGGING
				fResender.logprintf("Increasing quality level to: %d. Curpackdelay: %qd. Futdelay: %qd\n",fQualityLevel, inCurrentPacketDelay, theFutureDelay);
#endif
				return true; // So that we don't reset fJustIncreasedQualityLevel
			}
		}
		else if ((theFutureDelay < fStartThickingDelay) && (fQualityLevel > 0))
		{
			fQualityLevel--;
#if RTP_PACKET_RESENDER_DEBUGGING
			fResender.logprintf("Decreasing quality level to: %d. Curpackdelay: %qd. Futdelay: %qd\n",fQualityLevel, inCurrentPacketDelay, theFutureDelay);
#endif
		}
		fJustIncreasedQualityLevel = false;
	}
	return true; // We should send this packet
}

QTSS_Error	RTPStream::Write(void* inBuffer, UInt32 inLen, UInt32* outLenWritten, UInt32 inFlags)
{
	if( fTransportType == qtssRTPTransportTypeTCP )
		return TCPWrite(inBuffer,inLen, outLenWritten,inFlags);

	Assert(fSession != NULL);
	if (!fSession->GetSessionMutex()->TryLock())
		return EAGAIN;


	QTSS_Error err = QTSS_NoErr;
	SInt64 theTime = OS::Milliseconds();
	
	//
	// Data passed into this version of write must be a QTSS_PacketStruct
	QTSS_PacketStruct* thePacket = (QTSS_PacketStruct*)inBuffer;
	thePacket->suggestedWakeupTime = -1;
	SInt64 theCurrentPacketDelay = theTime - thePacket->packetTransmitTime;
	
#if RTP_PACKET_RESENDER_DEBUGGING
	UInt16* theSeqNum = (UInt16*)thePacket->packetData;
#endif

	//
	// Empty the overbuffer window
	fSession->GetOverbufferWindow()->EmptyOutWindow(theTime);

	//
	// Update the bit rate value
	fSession->UpdateCurrentBitRate(theTime);
	
	//
	// Is this the first write in a write burst?
	if (inFlags & qtssWriteFlagsWriteBurstBegin)
		fSession->GetOverbufferWindow()->MarkBeginningOfWriteBurst();
	
	//Assert(thePacket->packetTransmitTime >= fSession->GetPlayTime());
	if (inFlags & qtssWriteFlagsIsRTCP)
	{	
		if ( fTransportType == qtssRTPTransportTypeTCP )// write out in interleave format on the RTSP TCP channel
			err = this->InterleavedWrite( thePacket->packetData, inLen, outLenWritten, fRTCPChannel );
		else if ( inLen > 0 )
			(void)fSockets->GetSocketB()->SendTo(fRemoteAddr, fRemoteRTCPPort, thePacket->packetData, inLen);
	}
	

	else if (inFlags & qtssWriteFlagsIsRTP)
	{
		//
		// Check to see if this packet fits in the overbuffer window
		thePacket->suggestedWakeupTime = fSession->GetOverbufferWindow()->AddPacketToWindow(thePacket->packetTransmitTime, theTime, inLen);
		if (thePacket->suggestedWakeupTime > theTime)
		{
			Assert(thePacket->suggestedWakeupTime >= fSession->GetOverbufferWindow()->GetBucketInterval());
#if RTP_PACKET_RESENDER_DEBUGGING
			fResender.logprintf("Overbuffer window full. Num bytes in overbuffer: %d. Wakeup time: %qd\n",fSession->GetOverbufferWindow()->NumBytesInWindow(), thePacket->packetTransmitTime);
#endif
			//printf("Overbuffer window full. Returning: %qd\n", thePacket->suggestedWakeupTime - theTime);
			
			fSession->GetSessionMutex()->Unlock();// Make sure to unlock the mutex
			return QTSS_WouldBlock;
		}

		//
		// Check to make sure our quality level is correct. This function
		// also tells us whether this packet is just too old to send
		if (this->UpdateQualityLevel(thePacket->packetTransmitTime, theCurrentPacketDelay, theTime, inLen))
		{
			if ( fTransportType == qtssRTPTransportTypeTCP )	// write out in interleave format on the RTSP TCP channel
				err = this->InterleavedWrite( thePacket->packetData, inLen, outLenWritten, fRTPChannel );		
			else if ( fTransportType == qtssRTPTransportTypeReliableUDP )
				err = this->ReliableRTPWrite( thePacket->packetData, inLen, theCurrentPacketDelay );
			else if ( inLen > 0 )
				(void)fSockets->GetSocketA()->SendTo(fRemoteAddr, fRemoteRTPPort, thePacket->packetData, inLen);
		}	
#if RTP_PACKET_RESENDER_DEBUGGING
		if (err != QTSS_NoErr)
			fResender.logprintf("Flow controlled: %qd Overbuffer window: %d. Cur time %qd\n", theCurrentPacketDelay, fSession->GetOverbufferWindow()->NumBytesInWindow(), theTime);
		else
			fResender.logprintf("Sent packet: %d. Overbuffer window: %d Transmit time %qd. Cur time %qd\n", ntohs(theSeqNum[1]), fSession->GetOverbufferWindow()->NumBytesInWindow(), thePacket->packetTransmitTime, theTime);
#endif
		//if (err != QTSS_NoErr)
		//	printf("flow controlled\n");
		if ( err == QTSS_NoErr && inLen > 0 )
		{
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
	{	fSession->GetSessionMutex()->Unlock();// Make sure to unlock the mutex
		return QTSS_BadArgument;//qtssWriteFlagsIsRTCP or qtssWriteFlagsIsRTP wasn't specified
	}
	
	if (outLenWritten != NULL)
		*outLenWritten = inLen;
		
	fSession->GetSessionMutex()->Unlock();// Make sure to unlock the mutex
	return err;
}



QTSS_Error	RTPStream::TCPWrite(void* inBuffer, UInt32 inLen, UInt32* outLenWritten, UInt32 inFlags)
{
	Assert(fSession != NULL);
	Assert( fTransportType == qtssRTPTransportTypeTCP );

	if (!fSession->GetSessionMutex()->TryLock())
		return EAGAIN;

	QTSS_Error err = QTSS_NoErr;
	SInt64 theTime = OS::Milliseconds();

	//
	// Data passed into this version of write must be a QTSS_PacketStruct
	QTSS_PacketStruct* thePacket = (QTSS_PacketStruct*)inBuffer;	
	thePacket->suggestedWakeupTime = theTime + 100;
	
	SInt64 theCurrentPacketDelay = theTime - thePacket->packetTransmitTime; // negative delay means we are farther ahead
	fLastCurrentPacketDelay	=theCurrentPacketDelay;
	// This performs max buffer ahead = fMaxSendAheadTimeMSec (no rate control) during bursts
	if (theCurrentPacketDelay < (SInt64) (-1 * fMaxSendAheadTimeMSec) )// v107 max_advance_send_time pref = 100ms
	{
		err = QTSS_WouldBlock;
		thePacket->suggestedWakeupTime = theTime + (-1 * theCurrentPacketDelay) - fMaxSendAheadTimeMSec + 10;// add 10 ms
		fSession->GetSessionMutex()->Unlock();// Make sure to unlock the mutex
		return err;
	}

		
	if (thePacket->packetTransmitTime >= fSession->GetPlayTime()) do
	{
		if (inFlags & qtssWriteFlagsIsRTCP)
		{	err = this->InterleavedWrite( thePacket->packetData, inLen, outLenWritten, fRTCPChannel );
			break;
		}
		
		if  ( 	(thePacket->packetTransmitTime > fSession->GetPlayTime() + fDropAllPacketsForThisStreamDelay_TCP ) // don't drop packets for 5 seconds at the beginning of the movie
			&&	(theCurrentPacketDelay > fDropAllPacketsForThisStreamDelay_TCP) // value differs depending on whether video or not
			)
		{	
			// If this packet is just too old, drop it and make the module think
			// that it has been sent. But don't drop packets sent before our play time
			fStalePacketsDropped_TCP++;
			break;
		}

		if (inFlags & qtssWriteFlagsIsRTP)
		{
		
			if (fNumQualityLevels > 2)	// HACK: don't thin for reflected streams
			{
				if  (thePacket->packetTransmitTime > fSession->GetPlayTime() + 5000 ) // don't drop packets for 5 seconds at the beginning while tcp is ramping up
				{
					if ( theCurrentPacketDelay < fTurnThinningOffDelay_TCP ) // 800ms behind
					{	
						fQualityLevel = 0; // keep all frames
					}
					else if ( theCurrentPacketDelay < fIncreaseThinningDelay_TCP ) //2500ms behind
					{
						fQualityLevel = 1; // drop b frames
					}
					else 
						fQualityLevel = fNumQualityLevels; // key frames only
				}
			}
		
		
			err = this->InterleavedWrite( thePacket->packetData, inLen, outLenWritten, fRTPChannel );		
			if ( err == QTSS_NoErr && inLen > 0 )
			{
			
				//
				// Update the bit rate value
				fSession->UpdateCurrentBitRate(theTime);
					
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
		{	err = QTSS_BadArgument;//qtssWriteFlagsIsRTCP or qtssWriteFlagsIsRTP wasn't specified
			break;
		}
	
	} while (false);
	
	if (outLenWritten != NULL)
		*outLenWritten = inLen;
		
	fSession->GetSessionMutex()->Unlock();// Make sure to unlock the mutex
	return err;
}


// SendRTCPSR is called by the session as well as the strem
// SendRTCPSR must be called from a fSession mutex protected caller
void RTPStream::SendRTCPSR(const SInt64& inTime, Bool16 inAppendBye)
{
        //
        // This will roll over, after which payloadByteCount will be all messed up.
        // But because it is a 32 bit number, that is bound to happen eventually,
        // and we are limited by the RTCP packet format in that respect, so this is
        // pretty much ok.
	UInt32 payloadByteCount = fByteCount - (12 * fPacketCount);
        
	RTCPSRPacket* theSR = fSession->GetSRPacket();
	theSR->SetSSRC(fSsrc);
	theSR->SetClientSSRC(fClientSSRC);
	theSR->SetNTPTimestamp(fSession->GetNTPPlayTime() +
							OS::TimeMilli_To_Fixed64Secs(inTime - fSession->GetPlayTime()));
	theSR->SetRTPTimestamp(fLastRTPTimestamp);
	theSR->SetPacketCount(fPacketCount);
	theSR->SetByteCount(payloadByteCount);
#if RTP_PACKET_RESENDER_DEBUGGING
	fResender.logprintf("Recommending ack timeout of: %d\n",fSession->GetBandwidthTracker()->RecommendedClientAckTimeout());
#endif
	theSR->SetAckTimeout(fSession->GetBandwidthTracker()->RecommendedClientAckTimeout());
	
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


void RTPStream::ProcessIncomingInterleavedData(UInt8 inChannelNum, RTSPSessionInterface* inRTSPSession, StrPtrLen* inPacket)
{
	if (inChannelNum == fRTPChannel)
	{
		//
		// Currently we don't do anything with incoming RTP packets. Eventually,
		// we might need to make a role to deal with these
	}
	else if (inChannelNum == fRTCPChannel)
		this->ProcessIncomingRTCPPacket(inPacket);
}

void RTPStream::ProcessIncomingRTCPPacket(StrPtrLen* inPacket)
{
	StrPtrLen currentPtr(*inPacket);
	SInt64 curTime = OS::Milliseconds();
	
	// Modules are guarenteed atomic access to the session. Also, the RTSP Session accessed
	// below could go away at any time. So we need to lock the RTP session mutex.
	// *BUT*, when this function is called the caller already has the UDP socket pool &
	// UDP Demuxer mutexes. Blocking on grabbing this mutex could cause a deadlock.
	// So, dump this RTCP packet if we can't get the mutex.
	if (!fSession->GetSessionMutex()->TryLock())
		return;

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

				//
				// Set the Client SSRC based on latest RTCP
				fClientSSRC = rtcpPacket.GetPacketSSRC();

				fFractionLostPackets = receiverPacket.GetCumulativeFractionLostPackets();
				fJitter = receiverPacket.GetCumulativeJitter();
				
				UInt32 curTotalLostPackets = receiverPacket.GetCumulativeTotalLostPackets();
				//zero means either no loss or loss info wasn't included in receiver report
				/*if (curTotalLostPackets != fTotalLostPackets)
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
				*/
				
				// if current value is less than the old value, that means that the packets are out of order
				//	just wait for another packet that arrives in the right order later and for now, do nothing
				if (curTotalLostPackets > fTotalLostPackets)
				{	
					//increment the server total by the new delta
					QTSServerInterface::GetServer()->IncrementTotalRTPPacketsLost(curTotalLostPackets - fTotalLostPackets);
					fTotalLostPackets = curTotalLostPackets;
				}
				else if(curTotalLostPackets == fTotalLostPackets)
				{
					fCurPacketsLostInRTCPInterval = 0;
				}
				
								
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
					
					if (fPercentPacketsLost == 0)
						fCurPacketsLostInRTCPInterval = 0;
					//
					// Update our overbuffer window size to match what the client is telling us
					if (fTransportType != qtssRTPTransportTypeUDP)
						fSession->GetOverbufferWindow()->SetWindowSize(compressedQTSSPacket.GetOverbufferWindowSize());
					
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
	OSThreadDataSetter theSetter(&sRTCPProcessModuleState, NULL);
	
	//no matter what happens (whether or not this is a valid packet) reset the timeouts
	fSession->RefreshTimeout();
	if (fSession->GetRTSPSession() != NULL)
		fSession->GetRTSPSession()->RefreshTimeout();
	
	// Invoke RTCP processing modules
	for (UInt32 x = 0; x < QTSServerInterface::GetNumModulesInRole(QTSSModule::kRTCPProcessRole); x++)
		(void)QTSServerInterface::GetModule(QTSSModule::kRTCPProcessRole, x)->CallDispatch(QTSS_RTCPProcess_Role, &theParams);

	fSession->GetSessionMutex()->Unlock();
}
