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
	File:		RTPSessionInterface.h

	Contains:	Implementation of object defined in .h
				


	

*/

#include "RTPSessionInterface.h"
#include "QTSServerInterface.h"
#include "RTSPRequestInterface.h"
#include "QTSS.h"
#include "OS.h"
#include "RTPStream.h"

unsigned int 			RTPSessionInterface::sRTPSessionIDCounter = 0;


QTSSAttrInfoDict::AttrInfo	RTPSessionInterface::sAttributes[] = 
{   /*fields:   fAttrName, fFuncPtr, fAttrDataType, fAttrPermission */
	/* 0  */ { "qtssCliSesStreamObjects", 			NULL, 	qtssAttrDataTypeQTSS_Object,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 1  */ { "qtssCliSesCreateTimeInMsec", 		NULL, 	qtssAttrDataTypeTimeVal,		qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 2  */ { "qtssCliSesFirstPlayTimeInMsec", 	NULL, 	qtssAttrDataTypeTimeVal,		qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 3  */ { "qtssCliSesPlayTimeInMsec", 			NULL, 	qtssAttrDataTypeTimeVal,		qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 4  */ { "qtssCliSesAdjustedPlayTimeInMsec",	NULL, 	qtssAttrDataTypeTimeVal,		qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 5  */ { "qtssCliSesRTPBytesSent", 			NULL, 	qtssAttrDataTypeUInt32,			qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 6  */ { "qtssCliSesRTPPacketsSent", 			NULL, 	qtssAttrDataTypeUInt32,			qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 7  */ { "qtssCliSesState", 					NULL, 	qtssAttrDataTypeUInt32,			qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 8  */ { "qtssCliSesPresentationURL", 		NULL, 	qtssAttrDataTypeCharArray,		qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 9  */ { "qtssCliSesFirstUserAgent", 			NULL, 	qtssAttrDataTypeCharArray,		qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 10 */ { "qtssCliStrMovieDurationInSecs", 	NULL, 	qtssAttrDataTypeFloat64,		qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite },
	/* 11 */ { "qtssCliStrMovieSizeInBytes", 		NULL, 	qtssAttrDataTypeUInt64,			qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite },
	/* 12 */ { "qtssCliSesMovieAverageBitRate", 	NULL, 	qtssAttrDataTypeUInt32,			qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite },
	/* 13 */ { "qtssCliSesLastRTSPSession", 		NULL, 	qtssAttrDataTypeQTSS_Object,	qtssAttrModeRead | qtssAttrModePreempSafe }	,
	/* 14 */ { "qtssCliSesFullURL", 				NULL, 	qtssAttrDataTypeCharArray,		qtssAttrModeRead | qtssAttrModePreempSafe }	,
	/* 15 */ { "qtssCliSesHostName", 				NULL, 	qtssAttrDataTypeCharArray,		qtssAttrModeRead | qtssAttrModePreempSafe },

	/* 16 */ { "qtssCliRTSPSessRemoteAddrStr", 		NULL, 	qtssAttrDataTypeCharArray,		qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 17 */ { "qtssCliRTSPSessLocalDNS", 			NULL, 	qtssAttrDataTypeCharArray,		qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 18 */ { "qtssCliRTSPSessLocalAddrStr", 		NULL, 	qtssAttrDataTypeCharArray,		qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 19 */ { "qtssCliRTSPSesUserName", 			NULL, 	qtssAttrDataTypeCharArray,		qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 20 */ { "qtssCliRTSPSesUserPassword", 		NULL, 	qtssAttrDataTypeCharArray,		qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 21 */ { "qtssCliRTSPSesURLRealm", 			NULL, 	qtssAttrDataTypeCharArray,		qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 22 */ { "qtssCliRTSPReqRealStatusCode", 		NULL, 	qtssAttrDataTypeUInt32,			qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 23 */ { "qtssCliTeardownReason", 			NULL, 	qtssAttrDataTypeUInt32,			qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite },
	/* 24 */ { "qtssCliSesReqQueryString",			NULL, 	qtssAttrDataTypeCharArray,		qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 25 */ { "qtssCliRTSPReqRespMsg", 			NULL, 	qtssAttrDataTypeCharArray,		qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 26 */ { "qtssCliSesID", 						NULL, 	qtssAttrDataTypeUInt32,			qtssAttrModeRead | qtssAttrModePreempSafe },
	
	/* 27 */ { "qtssCliSesBytesSent", 				NULL, 				qtssAttrDataTypeUInt32,			qtssAttrModeRead },
	/* 28 */ { "qtssCliSesCurrentBitRate", 			BitRate, 			qtssAttrDataTypeUInt32,			qtssAttrModeRead },
	/* 29 */ { "qtssCliSesPacketLossPercent", 		PacketLossPercent, 	qtssAttrDataTypeFloat32,		qtssAttrModeRead },
	/* 30 */ { "qtssCliSesTimeConnectedinMsec", 	TimeConnected, 		qtssAttrDataTypeSInt64,			qtssAttrModeRead },	
	/* 31 */ { "qtssCliSesCounterID", 				NULL, 	qtssAttrDataTypeUInt32,			qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 32 */ { "qtssCliSesCurSpeed", 				NULL, 	qtssAttrDataTypeFloat32,		qtssAttrModeRead | qtssAttrModeWrite | qtssAttrModePreempSafe },
	/* 33 */ { "qtssCliSesStartTime", 				NULL, 	qtssAttrDataTypeFloat64,		qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 34 */ { "qtssCliSesStopTime", 				NULL, 	qtssAttrDataTypeFloat64,		qtssAttrModeRead | qtssAttrModePreempSafe }
};

void	RTPSessionInterface::Initialize()
{
	for (UInt32 x = 0; x < qtssCliSesNumParams; x++)
		QTSSDictionaryMap::GetMap(QTSSDictionaryMap::kClientSessionDictIndex)->
			SetAttribute(x, sAttributes[x].fAttrName, sAttributes[x].fFuncPtr,
				sAttributes[x].fAttrDataType, sAttributes[x].fAttrPermission);
}

RTPSessionInterface::RTPSessionInterface()
: 	QTSSDictionary(QTSSDictionaryMap::GetMap(QTSSDictionaryMap::kClientSessionDictIndex), NULL),
	fIsFirstPlay(true),
	fFirstPlayTime(0),
	fPlayTime(0),
	fAdjustedPlayTime(0),
	fNTPPlayTime(0),
	fNextSendPacketsTime(0),
	fState(qtssPausedState),
	fPlayFlags(0),
	fSpeed(1),
	fStartTime(-1),
	fStopTime(-1),
	fRTSPSessionID(fRTSPSessionIDBuf, 0),
	fRTSPSession(NULL),
	fLastRTSPReqRealStatusCode(200),
	fTimeoutTask(this, QTSServerInterface::GetServer()->GetPrefs()->GetRTPTimeoutInSecs() * 1000),
	fNumQualityLevels(0),
	fBytesSent(0),
	fPacketsSent(0),
	fPacketLossPercent(0.0),
	fTimeConnected(0),
	fLastBitRateTime(0),
	fLastBitsSent(0),
	fMovieDuration(0),
	fMovieSizeInBytes(0),
	fMovieAverageBitRate(0),
	fMovieCurrentBitRate(0),
	fTeardownReason(0),
	fUniqueID(0),
	fTracker(QTSServerInterface::GetServer()->GetPrefs()->IsSlowStartEnabled())
{
	//don't actually setup the fTimeoutTask until the session has been bound!
	//(we don't want to get timeouts before the session gets bound)
	fTimeout = QTSServerInterface::GetServer()->GetPrefs()->GetRTPTimeoutInSecs() * 1000;
	fUniqueID = (UInt32)atomic_add(&sRTPSessionIDCounter, 1);
	
	//mark the session create time
	fSessionCreateTime = OS::Milliseconds();

	// Setup all dictionary attribute values
	
	// Make sure the dictionary knows about our preallocated memory for the RTP stream array
	this->SetEmptyVal(qtssCliSesFirstUserAgent, &fUserAgentBuffer[0], kUserAgentBufSize);
	this->SetEmptyVal(qtssCliSesStreamObjects, &fStreamBuffer[0], kStreamBufSize);
	this->SetEmptyVal(qtssCliSesPresentationURL, &fPresentationURL[0], kPresentationURLSize);
	this->SetEmptyVal(qtssCliSesFullURL, &fFullRequestURL[0], kRequestHostNameBufferSize);
	this->SetEmptyVal(qtssCliSesHostName, &fRequestHostName[0], kFullRequestURLBufferSize);

	this->SetVal(qtssCliSesCreateTimeInMsec, 	&fSessionCreateTime, sizeof(fSessionCreateTime));
	this->SetVal(qtssCliSesFirstPlayTimeInMsec, &fFirstPlayTime, sizeof(fFirstPlayTime));
	this->SetVal(qtssCliSesPlayTimeInMsec, 		&fPlayTime, sizeof(fPlayTime));
	this->SetVal(qtssCliSesAdjustedPlayTimeInMsec, &fAdjustedPlayTime, sizeof(fAdjustedPlayTime));
	this->SetVal(qtssCliSesRTPBytesSent, 		&fBytesSent, sizeof(fBytesSent));
	this->SetVal(qtssCliSesRTPPacketsSent, 		&fPacketsSent, sizeof(fPacketsSent));
	this->SetVal(qtssCliSesState, 				&fState, sizeof(fState));
	this->SetVal(qtssCliSesMovieDurationInSecs, &fMovieDuration, sizeof(fMovieDuration));
	this->SetVal(qtssCliSesMovieSizeInBytes,	&fMovieSizeInBytes, sizeof(fMovieSizeInBytes));
	this->SetVal(qtssCliSesLastRTSPSession,		&fRTSPSession, sizeof(fRTSPSession));
	this->SetVal(qtssCliSesMovieAverageBitRate,	&fMovieAverageBitRate, sizeof(fMovieAverageBitRate));
	this->SetVal(qtssCliSesID,	&fRTSPSessionIDBuf, sizeof(UInt32) );
	this->SetVal(qtssCliSesBytesSent,	&fBytesSent, sizeof(fBytesSent) );
	this->SetEmptyVal(qtssCliRTSPSessRemoteAddrStr, &fRTSPSessRemoteAddrStr[0], kIPAddrStrBufSize );
	this->SetEmptyVal(qtssCliRTSPSessLocalDNS, &fRTSPSessLocalDNS[0], kLocalDNSBufSize);
	this->SetEmptyVal(qtssCliRTSPSessLocalAddrStr, &fRTSPSessLocalAddrStr[0], kIPAddrStrBufSize);

	this->SetEmptyVal(qtssCliRTSPSesUserName, &fUserNameBuf[0],RTSPSessionInterface::kMaxUserNameLen);
	this->SetEmptyVal(qtssCliRTSPSesUserPassword, &fUserPasswordBuf[0], RTSPSessionInterface::kMaxUserPasswordLen);
	this->SetEmptyVal(qtssCliRTSPSesURLRealm, &fUserRealmBuf[0], RTSPSessionInterface::kMaxUserRealmLen);

	this->SetVal(qtssCliRTSPReqRealStatusCode, &fLastRTSPReqRealStatusCode, sizeof(fLastRTSPReqRealStatusCode));

	this->SetVal(qtssCliTeardownReason, &fTeardownReason, sizeof(fTeardownReason));
	this->SetVal(qtssCliSesCounterID, &fUniqueID, sizeof(fUniqueID));
	this->SetVal(qtssCliSesCurSpeed, &fSpeed, sizeof(fSpeed));
	this->SetVal(qtssCliSesStartTime, &fStartTime, sizeof(fStartTime));
	this->SetVal(qtssCliSesStopTime, &fStopTime, sizeof(fStopTime));
}

void RTPSessionInterface::UpdateRTSPSession(RTSPSessionInterface* inNewRTSPSession)
{
	if (inNewRTSPSession != fRTSPSession)
	{
		// If there was an old session, let it know that we are done
		if (fRTSPSession != NULL)
			fRTSPSession->DecrementObjectHolderCount();
		
		// Increment this count to prevent the RTSP session from being deleted
		fRTSPSession = inNewRTSPSession;
		fRTSPSession->IncrementObjectHolderCount();
	}
}

QTSS_Error RTPSessionInterface::DoSessionSetupResponse(RTSPRequestInterface* inRequest)
{
	// This function appends a session header to the SETUP response, and
	// checks to see if it is a 304 Not Modified. If it is, it sends the entire
	// response and returns an error
	if ( QTSServerInterface::GetServer()->GetPrefs()->GetRTSPTimeoutInSecs() > 0 )	// adv the timeout
		inRequest->AppendSessionHeaderWithTimeout( &fRTSPSessionID, QTSServerInterface::GetServer()->GetPrefs()->GetRTSPTimeoutAsString() );
	else
		inRequest->AppendSessionHeaderWithTimeout( &fRTSPSessionID, NULL );	// no timeout in resp.
	
	if (inRequest->GetStatus() == qtssRedirectNotModified)
	{
		(void)inRequest->SendHeader();
		return QTSS_RequestFailed;
	}
	return QTSS_NoErr;
}

Bool16 RTPSessionInterface::BitRate(QTSS_FunctionParams* funcParamsPtr)
{

	// This param retrieval function must be invoked each time it is called,
	// because the number of sockets can be continually changing
	if (qtssGetValuePtrEnterFunc != funcParamsPtr->selector)
		return false;
		
	RTPSessionInterface* theSession = (RTPSessionInterface*)funcParamsPtr->object;
	SInt64 currentTime = OS::Milliseconds();
	SInt64 lastTime = theSession->fLastBitRateTime;
	if (lastTime == 0)
		lastTime = theSession->GetSessionCreateTime();
	if (lastTime + 1000 < currentTime)
	{
		UInt32 timeConnected = (UInt32) (currentTime - lastTime);
		UInt32 currentBitsSent = theSession->fBytesSent * 8;
		UInt32 bitsSent = currentBitsSent - theSession->fLastBitsSent;
		theSession->fLastBitRateTime = currentTime;
		theSession->fLastBitsSent = currentBitsSent;
		theSession->fMovieCurrentBitRate = bitsSent/ (timeConnected / 1000);	
	}

	// Return the result
	funcParamsPtr->io.bufferPtr =  &theSession->fMovieCurrentBitRate;
	funcParamsPtr->io.valueLen = sizeof(theSession->fMovieCurrentBitRate);
	
	return true;
}

Bool16 RTPSessionInterface::TimeConnected(QTSS_FunctionParams* funcParamsPtr)
{

	// This param retrieval function must be invoked each time it is called,
	// because the number of sockets can be continually changing
	if (qtssGetValuePtrEnterFunc != funcParamsPtr->selector)
		return false;
		
	RTPSessionInterface* theSession = (RTPSessionInterface*)funcParamsPtr->object;
	theSession->fTimeConnected = (OS::Milliseconds() - theSession->GetSessionCreateTime());
	// Return the result
	funcParamsPtr->io.bufferPtr =  &theSession->fTimeConnected;
	funcParamsPtr->io.valueLen = sizeof(theSession->fTimeConnected);
	
	return true;
}

Bool16 RTPSessionInterface::PacketLossPercent(QTSS_FunctionParams* funcParamsPtr)
{

	// This param retrieval function must be invoked each time it is called,
	// because the number of sockets can be continually changing
	if (qtssGetValuePtrEnterFunc != funcParamsPtr->selector)
		return false;
		
	RTPSessionInterface* theSession = (RTPSessionInterface*)funcParamsPtr->object;
	RTPStream* theStream = NULL;
	UInt32 theLen = sizeof(UInt32);
			
	SInt64 packetsLost = 0;
	SInt64 packetsReceived = 0;
	
	for (int x = 0; theSession->GetValue(qtssCliSesStreamObjects, x, (void**)&theStream, &theLen) == QTSS_NoErr; x++)
	{		
		if (theStream != NULL)
		{
			UInt32 streamCurPacketsLost = 0;
			theLen = sizeof(UInt32);
			(void) (theStream)->GetValue(qtssRTPStrCurPacketsLostInRTCPInterval,0, &streamCurPacketsLost, &theLen);
			//printf("stream = %d streamCurPacketsLost = %lu \n",x, streamCurPacketsLost);
			
			UInt32 streamCurPackets = 0;
			theLen = sizeof(UInt32);
			(void) (theStream)->GetValue(qtssRTPStrPacketCountInRTCPInterval,0, &streamCurPackets, &theLen);
			//printf("stream = %d streamCurPackets = %lu \n",x, streamCurPackets);
				
			packetsReceived += (SInt64)  streamCurPackets;
			packetsLost += (SInt64) streamCurPacketsLost;
			//printf("stream calculated loss = %f \n",x, (Float32) streamCurPacketsLost / (Float32) streamCurPackets);
			
		}
		theStream = NULL;
		theLen = sizeof(UInt32);
	}
	if (packetsReceived > 0)
	{	theSession->fPacketLossPercent =(( ((Float32) packetsLost / (Float32) packetsReceived) * 100.0) );
	}
	else
		theSession->fPacketLossPercent = 0.0;
	//printf("Session loss percent packetsLost = %qd packetsReceived= %qd theSession->fPacketLossPercent=%f\n",packetsLost,packetsReceived,theSession->fPacketLossPercent);
		
	// Return the result
	funcParamsPtr->io.bufferPtr =  &theSession->fPacketLossPercent;
	funcParamsPtr->io.valueLen = sizeof(theSession->fPacketLossPercent);
	
	return true;
}
