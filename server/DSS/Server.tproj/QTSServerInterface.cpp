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
	File:		QTSServerInterface.cpp

	Contains:	Implementation of object defined in QTSServerInterface.h.
	
	

*/

//INCLUDES:
#include "QTSServerInterface.h"

#include "RTPSessionInterface.h"
#include "OSRef.h"
#include "UDPSocketPool.h"
#include "RTSPProtocol.h"
#include "RTPPacketResender.h"
#ifndef __MacOSXServer__
#ifndef __MacOSX__
#include "revision.h"
#endif
#endif

// STATIC DATA

UInt32					QTSServerInterface::sServerAPIVersion = QTSS_API_VERSION;
QTSServerInterface*		QTSServerInterface::sServer = NULL;
StrPtrLen 				QTSServerInterface::sServerNameStr("QTSS");

// kVersionString from revision.h, include with -i at project level
StrPtrLen 				QTSServerInterface::sServerVersionStr(kVersionString);
StrPtrLen 				QTSServerInterface::sServerPlatformStr(kPlatformNameString);
StrPtrLen 				QTSServerInterface::sServerBuildDateStr(__DATE__ ", "__TIME__);
char					QTSServerInterface::sServerHeader[kMaxServerHeaderLen];
StrPtrLen				QTSServerInterface::sServerHeaderPtr(sServerHeader, kMaxServerHeaderLen);

ResizeableStringFormatter		QTSServerInterface::sPublicHeaderFormatter(NULL, 0);
StrPtrLen						QTSServerInterface::sPublicHeaderStr;

QTSSModule**			QTSServerInterface::sModuleArray[QTSSModule::kNumRoles];
UInt32					QTSServerInterface::sNumModulesInRole[QTSSModule::kNumRoles];
OSQueue					QTSServerInterface::sModuleQueue;
QTSSErrorLogStream		QTSServerInterface::sErrorLogStream;


QTSSAttrInfoDict::AttrInfo	QTSServerInterface::sAttributes[] = 
{   /*fields:   fAttrName, fFuncPtr, fAttrDataType, fAttrPermission */
	/* 0  */ { "qtssServerAPIVersion", 			NULL, 	qtssAttrDataTypeUInt32,		qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 1  */ { "qtssSvrDefaultDNSName", 		NULL, 	qtssAttrDataTypeCharArray,	qtssAttrModeRead },
	/* 2  */ { "qtssSvrDefaultIPAddr", 			NULL, 	qtssAttrDataTypeUInt32,		qtssAttrModeRead },
	/* 3  */ { "qtssSvrServerName", 			NULL, 	qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 4  */ { "qtssRTSPSvrServerVersion", 		NULL, 	qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 5  */ { "qtssRTSPSvrServerBuildDate", 	NULL, 	qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 6  */ { "qtssSvrRTSPPorts", 				NULL, 	qtssAttrDataTypeUInt16,		qtssAttrModeRead },
	/* 7  */ { "qtssSvrRTSPServerHeader", 		NULL, 	qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 8  */ { "qtssRTSPSvrState", 				NULL, 	qtssAttrDataTypeUInt32,		qtssAttrModeRead | qtssAttrModeWrite  },
	/* 9  */ { "qtssSvrIsOutOfDescriptors", 	IsOutOfDescriptors, 	qtssAttrDataTypeBool16,	qtssAttrModeRead },
	/* 10 */ { "qtssRTSPCurrentSessionCount", 	NULL, 	qtssAttrDataTypeUInt32,		qtssAttrModeRead },
	/* 11 */ { "qtssRTSPHTTPCurrentSessionCount",NULL, 	qtssAttrDataTypeUInt32,		qtssAttrModeRead },
	/* 12 */ { "qtssRTPSvrNumUDPSockets", 		GetTotalUDPSockets, 	qtssAttrDataTypeUInt32,	qtssAttrModeRead },
	/* 13 */ { "qtssRTPSvrCurConn", 			NULL, 	qtssAttrDataTypeUInt32,		qtssAttrModeRead },
	/* 14 */ { "qtssRTPSvrTotalConn", 			NULL, 	qtssAttrDataTypeUInt32,		qtssAttrModeRead },
	/* 15 */ { "qtssRTPSvrCurBandwidth", 		NULL, 	qtssAttrDataTypeUInt32,		qtssAttrModeRead },
	/* 16 */ { "qtssRTPSvrTotalBytes", 			NULL, 	qtssAttrDataTypeUInt64,		qtssAttrModeRead },
	/* 17 */ { "qtssRTPSvrAvgBandwidth", 		NULL, 	qtssAttrDataTypeUInt32,		qtssAttrModeRead },
	/* 18 */ { "qtssRTPSvrCurPackets", 			NULL, 	qtssAttrDataTypeUInt32,		qtssAttrModeRead },
	/* 19 */ { "qtssRTPSvrTotalPackets", 		NULL, 	qtssAttrDataTypeUInt64,		qtssAttrModeRead },
	/* 20 */ { "qtssSvrHandledMethods", 		NULL, 	qtssAttrDataTypeUInt32,		qtssAttrModeRead | qtssAttrModeWrite | qtssAttrModePreempSafe  },
	/* 21 */ { "qtssSvrModuleObjects", 			NULL, 	qtssAttrDataTypeQTSS_Object,qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 22 */ { "qtssSvrStartupTime", 			NULL, 	qtssAttrDataTypeTimeVal,	qtssAttrModeRead },
	/* 23 */ { "qtssSvrGMTOffsetInHrs", 		NULL, 	qtssAttrDataTypeSInt32,		qtssAttrModeRead },
	/* 24 */ { "qtssSvrDefaultIPAddrStr", 		NULL, 	qtssAttrDataTypeCharArray,	qtssAttrModeRead },
	/* 25 */ { "qtssSvrPreferences", 			NULL, 	qtssAttrDataTypeQTSS_Object,qtssAttrModeRead },
	/* 26 */ { "qtssSvrMessages", 				NULL, 	qtssAttrDataTypeQTSS_Object,qtssAttrModeRead },
	/* 27 */ { "qtssSvrClientSessions", 		SessionAttribute, 	qtssAttrDataTypeQTSS_Object,qtssAttrModeRead | qtssAttrModePreempSafe},
	/* 28 */ { "qtssSvrCurrentTimeMilliseconds",CurrentUnixTimeMilli, 	qtssAttrDataTypeTimeVal,qtssAttrModeRead},
	/* 29 */ { "qtssSvrCPULoadPercent", 		NULL, 	qtssAttrDataTypeFloat32,	qtssAttrModeRead},
	/* 30 */ { "qtssSvrNumReliableUDPBuffers", 	GetNumUDPBuffers, 	qtssAttrDataTypeUInt32,		qtssAttrModeRead },
	/* 31 */ { "qtssSvrReliableUDPWastageInBytes",GetNumWastedBytes, qtssAttrDataTypeUInt32,		qtssAttrModeRead }
};

void	QTSServerInterface::Initialize()
{
	for (UInt32 x = 0; x < qtssSvrNumParams; x++)
		QTSSDictionaryMap::GetMap(QTSSDictionaryMap::kServerDictIndex)->
			SetAttribute(x, sAttributes[x].fAttrName, sAttributes[x].fFuncPtr,
				sAttributes[x].fAttrDataType, sAttributes[x].fAttrPermission);

	//Write out a premade server header
	StringFormatter serverFormatter(sServerHeaderPtr.Ptr, kMaxServerHeaderLen);
	serverFormatter.Put(RTSPProtocol::GetHeaderString(qtssServerHeader));
	serverFormatter.Put(": ");
	serverFormatter.Put(sServerNameStr);
	serverFormatter.PutChar('/');
	serverFormatter.Put(sServerVersionStr);
	serverFormatter.PutChar('-');
	serverFormatter.Put(sServerPlatformStr);
	sServerHeaderPtr.Len = serverFormatter.GetCurrentOffset();
	Assert(sServerHeaderPtr.Len < kMaxServerHeaderLen);
}

QTSServerInterface::QTSServerInterface()
 : 	QTSSDictionary(QTSSDictionaryMap::GetMap(QTSSDictionaryMap::kServerDictIndex), &fMutex),
	fSocketPool(NULL),
	fRTPMap(NULL),
	fSrvrPrefs(NULL),
	fSrvrMessages(NULL),
 	fServerState(qtssStartingUpState),
 	fDefaultIPAddr(0),
 	fListeners(NULL),
 	fNumListeners(0),
 	fStartupTime_UnixMilli(0),
 	fGMTOffset(0),
 	fNumRTSPSessions(0),
 	fNumRTSPHTTPSessions(0),
	fNumRTPSessions(0),
 	fTotalRTPSessions(0),
 	fTotalRTPBytes(0),
 	fTotalRTPPackets(0),
	fTotalRTPPacketsLost(0),
 	fPeriodicRTPBytes(0),
	fPeriodicRTPPacketsLost(0),
 	fPeriodicRTPPackets(0),
 	fCurrentRTPBandwidthInBits(0),
 	fAvgRTPBandwidthInBits(0),
 	fRTPPacketsPerSecond(0),
 	fCPUPercent(0),
 	fCPUTimeUsedInSec(0),
 	fUDPWastageInBytes(0),
 	fNumUDPBuffers(0)
{
	for (UInt32 y = 0; y < QTSSModule::kNumRoles; y++)
	{
		sModuleArray[y] = NULL;
		sNumModulesInRole[y] = 0;
	}

	this->SetVal(qtssSvrState, 				&fServerState, 				sizeof(fServerState));
	this->SetVal(qtssServerAPIVersion, 		&sServerAPIVersion, 		sizeof(sServerAPIVersion));
	this->SetVal(qtssSvrDefaultIPAddr, 		&fDefaultIPAddr, 			sizeof(fDefaultIPAddr));
	this->SetVal(qtssSvrServerName, 		sServerNameStr.Ptr, 		sServerNameStr.Len);
	this->SetVal(qtssSvrServerVersion, 		sServerVersionStr.Ptr, 		sServerVersionStr.Len);
	this->SetVal(qtssSvrServerBuildDate, 	sServerBuildDateStr.Ptr, 	sServerBuildDateStr.Len);
	this->SetVal(qtssSvrRTSPServerHeader, 	sServerHeaderPtr.Ptr, 		sServerHeaderPtr.Len);
	this->SetVal(qtssRTSPCurrentSessionCount, &fNumRTSPSessions, 		sizeof(fNumRTSPSessions));
	this->SetVal(qtssRTSPHTTPCurrentSessionCount, &fNumRTSPHTTPSessions,sizeof(fNumRTSPHTTPSessions));
	this->SetVal(qtssRTPSvrCurConn, 		&fNumRTPSessions, 			sizeof(fNumRTPSessions));
	this->SetVal(qtssRTPSvrTotalConn, 		&fTotalRTPSessions, 		sizeof(fTotalRTPSessions));
	this->SetVal(qtssRTPSvrCurBandwidth, 	&fCurrentRTPBandwidthInBits,sizeof(fCurrentRTPBandwidthInBits));
	this->SetVal(qtssRTPSvrTotalBytes, 		&fTotalRTPBytes, 			sizeof(fTotalRTPBytes));
	this->SetVal(qtssRTPSvrAvgBandwidth, 	&fAvgRTPBandwidthInBits, 	sizeof(fAvgRTPBandwidthInBits));
	this->SetVal(qtssRTPSvrCurPackets, 		&fRTPPacketsPerSecond, 		sizeof(fRTPPacketsPerSecond));
	this->SetVal(qtssRTPSvrTotalPackets, 	&fTotalRTPPackets, 			sizeof(fTotalRTPPackets));
	this->SetVal(qtssSvrStartupTime, 		&fStartupTime_UnixMilli, 	sizeof(fStartupTime_UnixMilli));
	this->SetVal(qtssSvrGMTOffsetInHrs, 	&fGMTOffset, 				sizeof(fGMTOffset));
	this->SetVal(qtssSvrCPULoadPercent, 	&fCPUPercent, 				sizeof(fCPUPercent));

	sServer = this;
}


void QTSServerInterface::LogError(QTSS_ErrorVerbosity inVerbosity, char* inBuffer)
{
	QTSS_RoleParams theParams;
	theParams.errorParams.inVerbosity = inVerbosity;
	theParams.errorParams.inBuffer = inBuffer;

	for (UInt32 x = 0; x < QTSServerInterface::GetNumModulesInRole(QTSSModule::kErrorLogRole); x++)
		(void)QTSServerInterface::GetModule(QTSSModule::kErrorLogRole, x)->CallDispatch(QTSS_ErrorLog_Role, &theParams);

	// If this is a fatal error, set the proper attribute in the RTSPServer dictionary
	if ((inVerbosity == qtssFatalVerbosity) && (sServer != NULL))
	{
		QTSS_ServerState theState = qtssFatalErrorState;
		(void)sServer->SetValue(qtssSvrState, 0, &theState, sizeof(theState));
	}
}

void QTSServerInterface::KillAllRTPSessions()
{
	OSMutexLocker locker(fRTPMap->GetMutex());
	for (OSRefHashTableIter theIter(fRTPMap->GetHashTable()); !theIter.IsDone(); theIter.Next())
	{
		OSRef* theRef = theIter.GetCurrent();
		RTPSessionInterface* theSession = (RTPSessionInterface*)theRef->GetObject();
		theSession->Signal(Task::kKillEvent);
	}	
}


#pragma mark __RTP_STATS_UPDATER_TASK__

RTPStatsUpdaterTask::RTPStatsUpdaterTask()
: 	Task(), fLastBandwidthTime(0), fLastBandwidthAvg(0), fLastBytesSent(0)
{
	this->Signal(Task::kStartEvent);
}

SInt64 RTPStatsUpdaterTask::Run()
{

	QTSServerInterface* theServer = QTSServerInterface::sServer;
	
	// All of this must happen atomically wrt dictionary values we are manipulating
	OSMutexLocker locker(&theServer->fMutex);
	
	//First update total bytes. This must be done because total bytes is a 64 bit number,
	//so no atomic functions can apply.
	//
	// NOTE: The line below is not thread safe on non-PowerPC platforms. This is
	// because the fPeriodicRTPBytes variable is being manipulated from within an
	// atomic_add. On PowerPC, assignments are atomic, so the assignment below is ok.
	// On a non-PowerPC platform, the following would be thread safe:
	//unsigned int periodicBytes = atomic_add(&theServer->fPeriodicRTPBytes, 0);
	unsigned int periodicBytes = theServer->fPeriodicRTPBytes;
	(void)atomic_sub(&theServer->fPeriodicRTPBytes, periodicBytes);
	theServer->fTotalRTPBytes += periodicBytes;
	
	// Same deal for packet totals
	unsigned int periodicPackets = theServer->fPeriodicRTPPackets;
	(void)atomic_sub(&theServer->fPeriodicRTPPackets, periodicPackets);
	theServer->fTotalRTPPackets += periodicPackets;
	
	// ..and for lost packet totals
	unsigned int periodicPacketsLost = theServer->fPeriodicRTPPacketsLost;
	(void)atomic_sub(&theServer->fPeriodicRTPPacketsLost, periodicPacketsLost);
	theServer->fTotalRTPPacketsLost += periodicPacketsLost;
	
	SInt64 curTime = OS::Milliseconds();
	
	//for cpu percent
	clock_t cpuTime = clock();
	Float32 cpuTimeInSec = (Float32) cpuTime / CLOCKS_PER_SEC;
	
	//also update current bandwidth statistic
	if (fLastBandwidthTime != 0)
	{
		Assert(curTime > fLastBandwidthTime);
		UInt32 delta = (UInt32)(curTime - fLastBandwidthTime);
		if (delta < 1000)
			WarnV(delta >= 1000, "Timer is off");
		
		//do the bandwidth computation using floating point divides
		//for accuracy and speed.
		Float32 bits = (Float32)(periodicBytes * 8);
		Float32 theTime = (Float32)delta;
		theTime /= 1000;
		bits /= theTime;
		Assert(bits >= 0);
		theServer->fCurrentRTPBandwidthInBits = (UInt32)bits;
		
		//do the same computation for packets per second
		Float32 packetsPerSecond = (Float32)periodicPackets;
		packetsPerSecond /= theTime;
		Assert(packetsPerSecond >= 0);
		theServer->fRTPPacketsPerSecond = (UInt32)packetsPerSecond;
		
		//do the computation for cpu percent
		Float32 diffTime = cpuTimeInSec - theServer->fCPUTimeUsedInSec;
#ifdef __Win32__
		theServer->fCPUPercent = 0.0;	
#else
		theServer->fCPUPercent = (diffTime/theTime) * 100;	
#endif

	}
	
	fLastBandwidthTime = curTime;
	
	//for cpu percent
	theServer->fCPUTimeUsedInSec	= cpuTimeInSec;	
	
	//also compute average bandwidth, a much more smooth value. This is done with
	//the fLastBandwidthAvg, a timestamp of the last time we did an average, and
	//fLastBytesSent, the number of bytes sent when we last did an average.
	if ((fLastBandwidthAvg != 0) && (curTime > (fLastBandwidthAvg +
		(theServer->GetPrefs()->GetAvgBandwidthUpdateTimeInSecs() * 1000))))
	{
		UInt32 delta = (UInt32)(curTime - fLastBandwidthAvg);
		SInt64 bytesSent = theServer->fTotalRTPBytes - fLastBytesSent;
		Assert(bytesSent >= 0);
		
		//do the bandwidth computation using floating point divides
		//for accuracy and speed.
		Float32 bits = (Float32)(bytesSent * 8);
		Float32 theAvgTime = (Float32)delta;
		theAvgTime /= 1000;
		bits /= theAvgTime;
		Assert(bits >= 0);
		theServer->fAvgRTPBandwidthInBits = (UInt32)bits;

		fLastBandwidthAvg = curTime;
		fLastBytesSent = theServer->fTotalRTPBytes;
		
		//if the bandwidth is above the bandwidth setting, disconnect 1 user by sending them
		//a BYE RTCP packet.
		SInt32 maxKBits = theServer->GetPrefs()->GetMaxKBitsBandwidth();
		if ((maxKBits > -1) && (theServer->fAvgRTPBandwidthInBits > ((UInt32)maxKBits * 1024)))
		{
			//we need to make sure that all of this happens atomically wrt the session map
			OSMutexLocker locker(theServer->GetRTPSessionMap()->GetMutex());
			RTPSessionInterface* theSession = this->GetNewestSession(theServer->fRTPMap);
			if (theSession != NULL)
				if ((curTime - theSession->GetSessionCreateTime()) <
						theServer->GetPrefs()->GetSafePlayDurationInSecs() * 1000)
					theSession->Signal(Task::kKillEvent);
		}
	}
	else if (fLastBandwidthAvg == 0)
	{
		fLastBandwidthAvg = curTime;
		fLastBytesSent = theServer->fTotalRTPBytes;
	}
	
	(void)this->GetEvents();//we must clear the event mask!
	return theServer->GetPrefs()->GetTotalBytesUpdateTimeInSecs() * 1000;
}

RTPSessionInterface* RTPStatsUpdaterTask::GetNewestSession(OSRefTable* inRTPSessionMap)
{
	//Caller must lock down the RTP session map
	SInt64 theNewestPlayTime = 0;
	RTPSessionInterface* theNewestSession = NULL;
	
	//use the session map to iterate through all the sessions, finding the most
	//recently connected client
	for (OSRefHashTableIter theIter(inRTPSessionMap->GetHashTable()); !theIter.IsDone(); theIter.Next())
	{
		OSRef* theRef = theIter.GetCurrent();
		RTPSessionInterface* theSession = (RTPSessionInterface*)theRef->GetObject();
		Assert(theSession->GetSessionCreateTime() > 0);
		if (theSession->GetSessionCreateTime() > theNewestPlayTime)
		{
			theNewestPlayTime = theSession->GetSessionCreateTime();
			theNewestSession = theSession;
		}
	}
	return theNewestSession;
}

#pragma mark __PARAM_RETRIEVAL_FUNCTIONS__


Bool16 QTSServerInterface::CurrentUnixTimeMilli(QTSS_FunctionParams* funcParamsPtr)
{

	// This param retrieval function must be invoked each time it is called,
	// because the time is continually changing
	if (qtssGetValuePtrEnterFunc != funcParamsPtr->selector)
		return false;
		
 	QTSServerInterface* theServer = QTSServerInterface::sServer;

	theServer->fCurrentTime_UnixMilli = OS::TimeMilli_To_UnixTimeMilli(OS::Milliseconds());	
	
	// Return the result
	funcParamsPtr->io.bufferPtr =  &theServer->fCurrentTime_UnixMilli;
	funcParamsPtr->io.valueLen = sizeof(theServer->fCurrentTime_UnixMilli);
	
	return true;
}

//param retrieval functions described in .h file
Bool16 QTSServerInterface::GetTotalUDPSockets(QTSS_FunctionParams* funcParamsPtr)
{

	// This param retrieval function must be invoked each time it is called,
	// because the number of sockets can be continually changing
	if (qtssGetValuePtrEnterFunc != funcParamsPtr->selector)
		return false;
		
	QTSServerInterface* theServer = (QTSServerInterface*)funcParamsPtr->object;
	// Multiply by 2 because this is returning the number of socket *pairs*
	theServer->fTotalUDPSockets = theServer->fSocketPool->GetSocketQueue()->GetLength() * 2;
	
	// Return the result
	funcParamsPtr->io.bufferPtr =  &theServer->fTotalUDPSockets;
	funcParamsPtr->io.valueLen = sizeof(theServer->fTotalUDPSockets);
	
	return true;
}

Bool16 QTSServerInterface::IsOutOfDescriptors(QTSS_FunctionParams *funcParamsPtr)
{
	// FUNCTIONPTR
	// Always return handled and the value
	// Do nothing on exit of get
	// Return 1 for num values
	// This param retrieval function must be invoked each time it is called,
	// because whether we are out of descriptors or not is continually changing
	if (qtssGetValuePtrEnterFunc != funcParamsPtr->selector)
		return false;
	
	QTSServerInterface* theServer = (QTSServerInterface*)funcParamsPtr->object;
	
	theServer->fIsOutOfDescriptors = false;
	for (UInt32 x = 0; x < theServer->fNumListeners; x++)
	{
		if (theServer->fListeners[x]->IsOutOfDescriptors())
		{
			theServer->fIsOutOfDescriptors = true;
			break;
		}
	}
	// Return the result
	funcParamsPtr->io.bufferPtr = &theServer->fIsOutOfDescriptors;
	funcParamsPtr->io.valueLen = sizeof(theServer->fIsOutOfDescriptors);
	return true;	
}
//static RTPSessionInterface *sSessionPtr;

Bool16 QTSServerInterface::SessionAttribute(QTSS_FunctionParams* funcParamsPtr)
{

	// This param retrieval function must be invoked each time it is called,
	// because the number of sockets can be continually changing
	switch(funcParamsPtr->selector)
	{	
		case qtssGetValuePtrEnterFunc:
		case qtssGetNumValuesEnterFunc:
			break;
			
		default: 
			return false;
	}
		
	QTSServerInterface* theServer = (QTSServerInterface*)funcParamsPtr->object;
	
	switch(funcParamsPtr->selector)
	{	

		case qtssGetValuePtrEnterFunc:
		{
			funcParamsPtr->io.bufferPtr = (void *) NULL;
			funcParamsPtr->io.valueLen = 0; 

			OSMutexLocker locker(theServer->GetRTPSessionMap()->GetMutex());
			OSRefHashTable* theRTPHashTablePtr = theServer->GetRTPSessionMap()->GetHashTable();
			OSRefHashTableIter theIter(theRTPHashTablePtr);
			
			UInt32 numSessionsInList = theServer->GetNumRTPSessions();
			UInt32 theIndexValue = funcParamsPtr->attributeIndex;
			
			for (UInt32 streamCount = 0;theIter.GetCurrent() && streamCount < numSessionsInList ; theIter.Next(), streamCount++)			
			{
		
				RTPSessionInterface** rtp_SessionPtr = (RTPSessionInterface**)theIter.GetCurrent()->GetObjectPtr();
				if (NULL == rtp_SessionPtr) continue;
				
				if (theIndexValue == streamCount)
				{	
					//printf("QTSServerInterface::SessionAttribute qtssGetValuePtrEnterFunc Found Session = %lu index = %lu \n", rtp_Session,theIndexValue);
					funcParamsPtr->io.bufferPtr = (void *) rtp_SessionPtr;
					funcParamsPtr->io.valueLen = sizeof(rtp_SessionPtr);
					//printf("QTSServerInterface::SessionAttribute qtssGetValuePtrEnterFunc Found io.bufferPtr = %lu len = %lu \n",funcParamsPtr->io.bufferPtr,funcParamsPtr->io.valueLen);
					break;
				}
			}
			
			if (funcParamsPtr->io.bufferPtr == NULL || funcParamsPtr->io.valueLen == 0)
				funcParamsPtr->handledError = QTSS_ValueNotFound;

		}
		break;
		case qtssGetNumValuesEnterFunc:
		{
			//printf("QTSServerInterface::SessionAttribute qtssGetNumValuesEnterFunc = %lu \n", theServer->fNumRTPSessions);
			*(UInt32*)funcParamsPtr->io.bufferPtr = theServer->fNumRTPSessions;
			funcParamsPtr->io.valueLen = sizeof(theServer->fNumRTPSessions);
		}
		break;
	}
		
	return true;
}


Bool16 QTSServerInterface::GetNumUDPBuffers(QTSS_FunctionParams* funcParamsPtr)
{
	// This param retrieval function must be invoked each time it is called,
	// because whether we are out of descriptors or not is continually changing
	//QTSServerInterface* theServer = (QTSServerInterface*)inServer;
	
	//theServer->fNumUDPBuffers = RTPPacketResender::GetNumRetransmitBuffers();

	// Return the result
	//*outLen = sizeof(theServer->fNumUDPBuffers);
	//return &theServer->fNumUDPBuffers;
	return true;	
}

Bool16 QTSServerInterface::GetNumWastedBytes(QTSS_FunctionParams* funcParamsPtr)
{
	// This param retrieval function must be invoked each time it is called,
	// because whether we are out of descriptors or not is continually changing
	//QTSServerInterface* theServer = (QTSServerInterface*)inServer;
	
	//theServer->fUDPWastageInBytes = RTPPacketResender::GetWastedBufferBytes();

	// Return the result
	//*outLen = sizeof(theServer->fUDPWastageInBytes);
	//return &theServer->fUDPWastageInBytes;	
	return true;	
}

#pragma mark __ERROR_LOG_STREAM__

QTSS_Error	QTSSErrorLogStream::Write(void* inBuffer, UInt32 inLen, UInt32* outLenWritten, UInt32 inFlags)
{
	// For the error log stream, the flags are considered to be the verbosity
	// of the error.
	if (inFlags >= qtssIllegalVerbosity)
		inFlags = qtssMessageVerbosity;
		
 	QTSServerInterface::LogError(inFlags, (char*)inBuffer);
 	if (outLenWritten != NULL)
 		*outLenWritten = inLen;
 		
	return QTSS_NoErr;
}

void QTSSErrorLogStream::LogAssert(char* inMessage)
{
 	QTSServerInterface::LogError(qtssAssertVerbosity, inMessage);
}


