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
	File:		QTSSReflectorModule.cpp

	Contains:	Implementation of QTSSReflectorModule class. 
					
	
	
*/

#include "QTSSReflectorModule.h"
#include "QTSSModuleUtils.h"
#include "ReflectorSession.h"
#include "OSArrayObjectDeleter.h"
#include "QTSS_Private.h"

#include "OSMemory.h"
#include "OSRef.h"
#include "IdleTask.h"
#include "Task.h"
#include "OS.h"
#include "Socket.h"
#include "SocketUtils.h"
#include "FilePrefsSource.h"
#include "StringParser.h"
#include "QTAccessFile.h"
#include "QTSSModuleUtils.h"

//ReflectorOutput objects
#include "RTPSessionOutput.h"

//SourceInfo objects
#include "SDPSourceInfo.h"


#if DEBUG
#define REFLECTOR_MODULE_DEBUGGING 0
#else
#define REFLECTOR_MODULE_DEBUGGING 0
#endif

// ATTRIBUTES
static QTSS_AttributeID			sOutputAttr		 			=	qtssIllegalAttrID;
static QTSS_AttributeID			sSessionAttr				=	qtssIllegalAttrID;
static QTSS_AttributeID			sStreamCookieAttr			=	qtssIllegalAttrID;
static QTSS_AttributeID			sRequestBodyAttr			=	qtssIllegalAttrID;
static QTSS_AttributeID			sBufferOffsetAttr			=	qtssIllegalAttrID;
static QTSS_AttributeID			sExpectedDigitFilenameErr	=	qtssIllegalAttrID;
static QTSS_AttributeID			sReflectorBadTrackIDErr		=	qtssIllegalAttrID;
static QTSS_AttributeID			sDuplicateBroadcastStreamErr=	qtssIllegalAttrID;
static QTSS_AttributeID			sClientBroadcastSessionAttr	=	qtssIllegalAttrID;
static QTSS_AttributeID			sRTSPBroadcastSessionAttr	=	qtssIllegalAttrID;
static QTSS_AttributeID			sAnnounceRequiresSDPinNameErr  = qtssIllegalAttrID;
static QTSS_AttributeID			sAnnounceDisabledNameErr  	= qtssIllegalAttrID;
static QTSS_AttributeID			sSDPcontainsInvalidMinimumPortErr  = qtssIllegalAttrID;
static QTSS_AttributeID			sSDPcontainsInvalidMaximumPortErr  = qtssIllegalAttrID;
static QTSS_AttributeID			sStaticPortsConflictErr  = qtssIllegalAttrID;
static QTSS_AttributeID			sInvalidPortRangeErr  = qtssIllegalAttrID;

// STATIC DATA

// ref to the prefs dictionary object
static OSRefTable* 		sSessionMap = NULL;
static const StrPtrLen 	kCacheControlHeader("no-cache");
static QTSS_PrefsObject sServerPrefs = NULL;
static QTSS_ServerObject sServer = NULL;
static QTSS_ModulePrefsObject 		sPrefs = NULL;

//
// Prefs
static Bool16	sAllowNonSDPURLs = true;
static Bool16	sDefaultAllowNonSDPURLs = true;

static Bool16	sAnnounceEnabled = true;
static Bool16	sDefaultAnnounceEnabled = true;
static Bool16	sBroadcastPushEnabled = true;
static Bool16	sDefaultBroadcastPushEnabled = true;
static Bool16	sAllowDuplicateBroadcasts = false;
static Bool16	sDefaultAllowDuplicateBroadcasts = false;

static UInt32	sMaxBroadcastAnnounceDuration = 0;
static UInt32	sDefaultMaxBroadcastAnnounceDuration = 0;
static UInt16	sMinimumStaticSDPPort = 0;
static UInt16	sDefaultMinimumStaticSDPPort = 20000;
static UInt16	sMaximumStaticSDPPort = 0;
static UInt16	sDefaultMaximumStaticSDPPort = 65535;

static Bool16	sTearDownClientsOnDisconnect = false;
static Bool16	sDefaultTearDownClientsOnDisconnect = false;

static Bool16	sOneSSRCPerStream = true;
static Bool16	sDefaultOneSSRCPerStream = true;
				
static UInt32	sTimeoutSSRCSecs = 30;
static UInt32	sDefaultTimeoutSSRCSecs = 30;
				
				
static UInt16 sLastMax = 0;
static UInt16 sLastMin = 0;

static Bool16	sEnforceStaticSDPPortRange = false;
static Bool16	sDefaultEnforceStaticSDPPortRange = false;
// Important strings
static StrPtrLen	sSDPSuffix(".sdp");
static StrPtrLen	sMOVSuffix(".mov");

const int kBuffLen = 512;

// FUNCTION PROTOTYPES

static QTSS_Error QTSSReflectorModuleDispatch(QTSS_Role inRole, QTSS_RoleParamPtr inParams);
static QTSS_Error 	Register(QTSS_Register_Params* inParams);
static QTSS_Error Initialize(QTSS_Initialize_Params* inParams);
static QTSS_Error Shutdown();
static QTSS_Error ProcessRTSPRequest(QTSS_StandardRTSP_Params* inParams);
static QTSS_Error DoAnnounce(QTSS_StandardRTSP_Params* inParams);
static QTSS_Error DoDescribe(QTSS_StandardRTSP_Params* inParams);
ReflectorSession* FindOrCreateSession(StrPtrLen* inPath, QTSS_StandardRTSP_Params* inParams, StrPtrLen* inData = NULL,Bool16 isPush=false, Bool16 *foundSessionPtr = NULL);
static QTSS_Error DoSetup(QTSS_StandardRTSP_Params* inParams);
static QTSS_Error DoPlay(QTSS_StandardRTSP_Params* inParams, ReflectorSession* inSession);
static QTSS_Error DestroySession(QTSS_ClientSessionClosing_Params* inParams);
static void RemoveOutput(ReflectorOutput* inOutput, ReflectorSession* inSession);
static ReflectorSession* DoSessionSetup(QTSS_StandardRTSP_Params* inParams, QTSS_AttributeID inPathType,Bool16 isPush=false,Bool16 *foundSessionPtr= NULL);
static QTSS_Error RereadPrefs();
static QTSS_Error ProcessRTPData(QTSS_IncomingData_Params* inParams);
static QTSS_Error ReflectorAuthorizeRTSPRequest(QTSS_StandardRTSP_Params* inParams);
static Bool16 InfoPortsOK(QTSS_StandardRTSP_Params* inParams, SDPSourceInfo* theInfo, StrPtrLen* inPath);

						
inline void KeepSession(QTSS_RTSPRequestObject theRequest,Bool16 keep)
{
	(void)QTSS_SetValue(theRequest, qtssRTSPReqRespKeepAlive, 0, &keep, sizeof(keep));
}
	



// FUNCTION IMPLEMENTATIONS

QTSS_Error QTSSReflectorModule_Main(void* inPrivateArgs)
{
	return _stublibrary_main(inPrivateArgs, QTSSReflectorModuleDispatch);
}


QTSS_Error 	QTSSReflectorModuleDispatch(QTSS_Role inRole, QTSS_RoleParamPtr inParams)
{
	switch (inRole)
	{
		case QTSS_Register_Role:
			return Register(&inParams->regParams);
		case QTSS_Initialize_Role:
			return Initialize(&inParams->initParams);
		case QTSS_RereadPrefs_Role:
			return RereadPrefs();
		case QTSS_RTSPPreProcessor_Role:
			return ProcessRTSPRequest(&inParams->rtspRequestParams);
		case QTSS_RTSPIncomingData_Role:
			return ProcessRTPData(&inParams->rtspIncomingDataParams);
		case QTSS_ClientSessionClosing_Role:
			return DestroySession(&inParams->clientSessionClosingParams);
		case QTSS_Shutdown_Role:
			return Shutdown();
		case QTSS_RTSPAuthorize_Role:
			return ReflectorAuthorizeRTSPRequest(&inParams->rtspRequestParams);
	}
	return QTSS_NoErr;
}


QTSS_Error Register(QTSS_Register_Params* inParams)
{
	// Do role & attribute setup
	(void)QTSS_AddRole(QTSS_Initialize_Role);
	(void)QTSS_AddRole(QTSS_Shutdown_Role);
	(void)QTSS_AddRole(QTSS_RTSPPreProcessor_Role);
	(void)QTSS_AddRole(QTSS_ClientSessionClosing_Role);
	(void)QTSS_AddRole(QTSS_RTSPIncomingData_Role); // call me with interleaved RTP streams on the RTSP session
	(void)QTSS_AddRole(QTSS_RTSPAuthorize_Role);
	
	// Add text messages attributes
	static char*		sExpectedDigitFilenameName		= "QTSSReflectorModuleExpectedDigitFilename";
	static char*		sReflectorBadTrackIDErrName		= "QTSSReflectorModuleBadTrackID";
	static char*		sDuplicateBroadcastStreamName	= "QTSSReflectorModuleDuplicateBroadcastStream";
	static char*		sAnnounceRequiresSDPinName  	= "QTSSReflectorModuleAnnounceRequiresSDPSuffix";
	static char*		sAnnounceDisabledName 			= "QTSSReflectorModuleAnnounceDisabled";
	
	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sDuplicateBroadcastStreamName, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sDuplicateBroadcastStreamName, &sDuplicateBroadcastStreamErr);

	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sAnnounceRequiresSDPinName, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sAnnounceRequiresSDPinName, &sAnnounceRequiresSDPinNameErr);

	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sAnnounceDisabledName, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sAnnounceDisabledName, &sAnnounceDisabledNameErr);


	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sExpectedDigitFilenameName, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sExpectedDigitFilenameName, &sExpectedDigitFilenameErr);

	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sReflectorBadTrackIDErrName, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sReflectorBadTrackIDErrName, &sReflectorBadTrackIDErr);
	
	static char* sSDPcontainsInvalidMinumumPortErrName 	= "QTSSReflectorModuleSDPPortMinimumPort";
	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sSDPcontainsInvalidMinumumPortErrName, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sSDPcontainsInvalidMinumumPortErrName, &sSDPcontainsInvalidMinimumPortErr);

	static char* sSDPcontainsInvalidMaximumPortErrName 	= "QTSSReflectorModuleSDPPortMaximumPort";
	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sSDPcontainsInvalidMaximumPortErrName, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sSDPcontainsInvalidMaximumPortErrName, &sSDPcontainsInvalidMaximumPortErr);
	
	static char* sStaticPortsConflictErrName 	= "QTSSReflectorModuleStaticPortsConflict";
	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sStaticPortsConflictErrName, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sStaticPortsConflictErrName, &sStaticPortsConflictErr);

	static char* sInvalidPortRangeErrName 	= "QTSSReflectorModuleStaticPortPrefsBadRange";
	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sInvalidPortRangeErrName, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sInvalidPortRangeErrName, &sInvalidPortRangeErr);
	
	// Add an RTP session attribute for tracking ReflectorSession objects
	static char*		sOutputName			= "QTSSReflectorModuleOutput";
	static char*		sSessionName		= "QTSSReflectorModuleSession";
	static char*		sStreamCookieName	= "QTSSReflectorModuleStreamCookie";
	static char*		sRequestBufferName	= "QTSSReflectorModuleRequestBuffer";
	static char*		sRequestBufferLenName="QTSSReflectorModuleRequestBufferLen";
	static char*		sBroadcasterSessionName="QTSSReflectorModuleBroadcasterSession";

	(void)QTSS_AddStaticAttribute(qtssClientSessionObjectType, sOutputName, NULL, qtssAttrDataTypeVoidPointer);
	(void)QTSS_IDForAttr(qtssClientSessionObjectType, sOutputName, &sOutputAttr);

	(void)QTSS_AddStaticAttribute(qtssClientSessionObjectType, sSessionName, NULL, qtssAttrDataTypeVoidPointer);
	(void)QTSS_IDForAttr(qtssClientSessionObjectType, sSessionName, &sSessionAttr);

	(void)QTSS_AddStaticAttribute(qtssRTPStreamObjectType, sStreamCookieName, NULL, qtssAttrDataTypeVoidPointer);
	(void)QTSS_IDForAttr(qtssRTPStreamObjectType, sStreamCookieName, &sStreamCookieAttr);

	(void)QTSS_AddStaticAttribute(qtssRTSPRequestObjectType, sRequestBufferName, NULL, qtssAttrDataTypeVoidPointer);
	(void)QTSS_IDForAttr(qtssRTSPRequestObjectType, sRequestBufferName, &sRequestBodyAttr);

	(void)QTSS_AddStaticAttribute(qtssRTSPRequestObjectType, sRequestBufferLenName, NULL, qtssAttrDataTypeUInt32);
	(void)QTSS_IDForAttr(qtssRTSPRequestObjectType, sRequestBufferLenName, &sBufferOffsetAttr);
	
	(void)QTSS_AddStaticAttribute(qtssClientSessionObjectType, sBroadcasterSessionName, NULL, qtssAttrDataTypeVoidPointer);
	(void)QTSS_IDForAttr(qtssClientSessionObjectType, sBroadcasterSessionName, &sClientBroadcastSessionAttr);
	
	// keep the same attribute name for the RTSPSessionObject as used int he ClientSessionObject
	(void)QTSS_AddStaticAttribute(qtssRTSPSessionObjectType, sBroadcasterSessionName, NULL, qtssAttrDataTypeVoidPointer);
	(void)QTSS_IDForAttr(qtssRTSPSessionObjectType, sBroadcasterSessionName, &sRTSPBroadcastSessionAttr);
			
	// Reflector session needs to setup some parameters too.
	ReflectorStream::Register();
	// RTPSessionOutput needs to do the same
	RTPSessionOutput::Register();

	// Tell the server our name!
	static char* sModuleName = "QTSSReflectorModule";
	::strcpy(inParams->outModuleName, sModuleName);

	return QTSS_NoErr;
}


QTSS_Error Initialize(QTSS_Initialize_Params* inParams)
{
	// Setup module utils
	QTSSModuleUtils::Initialize(inParams->inMessages, inParams->inServer, inParams->inErrorLogStream);
	QTAccessFile::Initialize();
	sSessionMap = NEW OSRefTable();
	sServerPrefs = inParams->inPrefs;
	sServer = inParams->inServer;
#if QTSS_REFLECTOR_EXTERNAL_MODULE
	// The reflector is dependent on a number of objects in the Common Utilities
	// library that get setup by the server if the reflector is internal to the
	// server.
	//
	// So, if the reflector is being built as a code fragment, it must initialize
	// those pieces itself
#if !MACOSXEVENTQUEUE
	::select_startevents();//initialize the select() implementation of the event queue
#endif
	OS::Initialize();
	Socket::Initialize();
	SocketUtils::Initialize();

	const UInt32 kNumReflectorThreads = 8;
	TaskThreadPool::AddThreads(kNumReflectorThreads);
	IdleTask::Initialize();
	Socket::StartThread();
#endif
	
	sPrefs = QTSSModuleUtils::GetModulePrefsObject(inParams->inModule);

	// Call helper class initializers
	ReflectorStream::Initialize(sPrefs);
	ReflectorSession::Initialize();
	
	// Report to the server that this module handles DESCRIBE, SETUP, PLAY, PAUSE, and TEARDOWN
	static QTSS_RTSPMethod sSupportedMethods[] = { qtssDescribeMethod, qtssSetupMethod, qtssTeardownMethod, qtssPlayMethod, qtssPauseMethod, qtssAnnounceMethod };
	QTSSModuleUtils::SetupSupportedMethods(inParams->inServer, sSupportedMethods, 6);

	RereadPrefs();
	
	return QTSS_NoErr;
}

QTSS_Error RereadPrefs()
{
	//
	// Use the standard GetPref routine to retrieve the correct values for our preferences
	QTSSModuleUtils::GetAttribute(sPrefs, "allow_non_sdp_urls", 	qtssAttrDataTypeBool16,
								&sAllowNonSDPURLs, &sDefaultAllowNonSDPURLs, sizeof(sDefaultAllowNonSDPURLs));
																
	QTSSModuleUtils::GetAttribute(sPrefs, "enable_broadcast_announce", 	qtssAttrDataTypeBool16,
								&sAnnounceEnabled, &sDefaultAnnounceEnabled, sizeof(sDefaultAnnounceEnabled));
	QTSSModuleUtils::GetAttribute(sPrefs, "enable_broadcast_push", 	qtssAttrDataTypeBool16,
								&sBroadcastPushEnabled, &sDefaultBroadcastPushEnabled, sizeof(sDefaultBroadcastPushEnabled));
	QTSSModuleUtils::GetAttribute(sPrefs, "max_broadcast_announce_duration_secs", 	qtssAttrDataTypeUInt32,
								&sMaxBroadcastAnnounceDuration, &sDefaultMaxBroadcastAnnounceDuration, sizeof(sDefaultMaxBroadcastAnnounceDuration));
	QTSSModuleUtils::GetAttribute(sPrefs, "allow_duplicate_broadcasts", 	qtssAttrDataTypeBool16,
								&sAllowDuplicateBroadcasts, &sDefaultAllowDuplicateBroadcasts, sizeof(sDefaultAllowDuplicateBroadcasts));
								
	QTSSModuleUtils::GetAttribute(sPrefs, "enforce_static_sdp_port_range", 	qtssAttrDataTypeBool16,
								&sEnforceStaticSDPPortRange, &sDefaultEnforceStaticSDPPortRange, sizeof(sDefaultEnforceStaticSDPPortRange));
	QTSSModuleUtils::GetAttribute(sPrefs, "minimum_static_sdp_port", 	qtssAttrDataTypeUInt16,
								&sMinimumStaticSDPPort, &sDefaultMinimumStaticSDPPort, sizeof(sDefaultMinimumStaticSDPPort));
	QTSSModuleUtils::GetAttribute(sPrefs, "maximum_static_sdp_port", 	qtssAttrDataTypeUInt16,
								&sMaximumStaticSDPPort, &sDefaultMaximumStaticSDPPort, sizeof(sDefaultMaximumStaticSDPPort));
	
	QTSSModuleUtils::GetAttribute(sPrefs, "kill_clients_when_broadcast_stops", 	qtssAttrDataTypeBool16,
								&sTearDownClientsOnDisconnect, &sDefaultTearDownClientsOnDisconnect, sizeof(sDefaultTearDownClientsOnDisconnect));
	QTSSModuleUtils::GetAttribute(sPrefs, "use_one_SSRC_per_stream", 	qtssAttrDataTypeBool16,
								&sOneSSRCPerStream, &sDefaultOneSSRCPerStream, sizeof(sDefaultOneSSRCPerStream));
	QTSSModuleUtils::GetAttribute(sPrefs, "timeout_stream_SSRC_secs", 	qtssAttrDataTypeUInt32,
								&sTimeoutSSRCSecs, &sDefaultTimeoutSSRCSecs, sizeof(sDefaultTimeoutSSRCSecs));

	if (sEnforceStaticSDPPortRange)
	{	Bool16 reportErrors = false;
		if (sLastMax != sMaximumStaticSDPPort)
		{	sLastMax = sMaximumStaticSDPPort;
			reportErrors = true;
		}
		
		if (sLastMin != sMinimumStaticSDPPort)
		{	sLastMin = sMinimumStaticSDPPort;
			reportErrors = true;
		}

		if (reportErrors)
		{
			UInt16 minServerPort = 6970;
			UInt16 maxServerPort = 9999;
			char min[32];
			char max[32];
					
			if  (	( (sMinimumStaticSDPPort <= minServerPort) && (sMaximumStaticSDPPort >= minServerPort) )
				||  ( (sMinimumStaticSDPPort >= minServerPort) && (sMinimumStaticSDPPort <= maxServerPort) )
				)
			{
			 	sprintf(min,"%u",minServerPort);
				sprintf(max,"%u",maxServerPort);	
				QTSSModuleUtils::LogError( 	qtssWarningVerbosity, sStaticPortsConflictErr, 0, min, max);
			}
			
			if  (sMinimumStaticSDPPort > sMaximumStaticSDPPort) 
			{ 
			 	sprintf(min,"%u",sMinimumStaticSDPPort);
				sprintf(max,"%u",sMaximumStaticSDPPort);	
				QTSSModuleUtils::LogError( 	qtssWarningVerbosity, sInvalidPortRangeErr, 0, min, max);
			}
		}
	}							
	return QTSS_NoErr;
}

QTSS_Error Shutdown()
{
#if QTSS_REFLECTOR_EXTERNAL_MODULE
	TaskThreadPool::RemoveThreads();
#endif
	return QTSS_NoErr;
}

QTSS_Error ProcessRTPData(QTSS_IncomingData_Params* inParams)
{
	if (!sBroadcastPushEnabled)
		return QTSS_NoErr;
		
	//printf("QTSSReflectorModule:ProcessRTPData inRTSPSession=%lu inClientSession=%lu\n",inParams->inRTSPSession, inParams->inClientSession);
	ReflectorSession* theSession = NULL;
	UInt32 theLen = sizeof(theSession);
	QTSS_Error theErr = QTSS_GetValue(inParams->inRTSPSession, sRTSPBroadcastSessionAttr, 0, &theSession, &theLen);
	//printf("QTSSReflectorModule.cpp:ProcessRTPData	sClientBroadcastSessionAttr=%lu theSession=%lu err=%ld \n",sClientBroadcastSessionAttr, theSession,theErr);
	if (theSession == NULL || theErr != QTSS_NoErr) 
		return QTSS_NoErr;
	
	// it is a broadcaster session
	//printf("QTSSReflectorModule.cpp:is broadcaster session\n");

	SourceInfo* theSoureInfo = theSession->GetSourceInfo();	
	Assert(theSoureInfo != NULL);
	if (theSoureInfo == NULL)
		return QTSS_NoErr;
			

	UInt32	numStreams = theSession->GetNumStreams();
	//printf("QTSSReflectorModule.cpp:ProcessRTPData numStreams=%lu\n",numStreams);

{
/*
   Stream data such as RTP packets is encapsulated by an ASCII dollar
   sign (24 hexadecimal), followed by a one-byte channel identifier,
   followed by the length of the encapsulated binary data as a binary,
   two-byte integer in network byte order. The stream data follows
   immediately afterwards, without a CRLF, but including the upper-layer
   protocol headers. Each $ block contains exactly one upper-layer
   protocol data unit, e.g., one RTP packet.
*/
	char*	packetData= inParams->inPacketData;
	
	UInt8	packetChannel;
	packetChannel = (UInt8) packetData[1];

	UInt16	packetDataLen;
	memcpy(&packetDataLen,&packetData[2],2);
	packetDataLen = ntohs(packetDataLen);
	
	char*	rtpPacket = &packetData[4];
	
	//UInt32	packetLen = inParams->inPacketLen;
	//printf("QTSSReflectorModule.cpp:ProcessRTPData channel=%u theSoureInfo=%lu packetLen=%lu packetDatalen=%u\n",(UInt16) packetChannel,theSoureInfo,inParams->inPacketLen,packetDataLen);

	if (1)
	{
		UInt32 inIndex = packetChannel / 2; // one stream per every 2 channels rtcp channel handled below
		ReflectorStream* theStream = NULL;
		if (inIndex < numStreams) 
		{	theStream = theSession->GetStreamByIndex(inIndex);

			SourceInfo::StreamInfo* theStreamInfo =theStream->GetStreamInfo();	
			UInt16 serverReceivePort =theStreamInfo->fPort;			

			Bool16 isRTCP =false;
			if (theStream != NULL)
			{	if (packetChannel & 1)
				{	serverReceivePort ++;
					isRTCP = true;
				}
				theStream->PushPacket(rtpPacket,packetDataLen, isRTCP);
				//printf("QTSSReflectorModule.cpp:ProcessRTPData Send RTSP packet channel=%u to UDP localServerAddr=%lu serverReceivePort=%lu packetDataLen=%u \n", (UInt16) packetChannel, localServerAddr, serverReceivePort,packetDataLen);
			}
		}
	}
	
}
	return theErr;
}

QTSS_Error ProcessRTSPRequest(QTSS_StandardRTSP_Params* inParams)
{
	QTSS_RTSPMethod* theMethod = NULL;
	//printf("QTSSReflectorModule:ProcessRTSPRequest inClientSession=%lu\n", (UInt32) inParams->inClientSession);
	UInt32 theLen = 0;
	if ((QTSS_GetValuePtr(inParams->inRTSPRequest, qtssRTSPReqMethod, 0,
			(void**)&theMethod, &theLen) != QTSS_NoErr) || (theLen != sizeof(QTSS_RTSPMethod)))
	{
		Assert(0);
		return QTSS_RequestFailed;
	}

	if (*theMethod == qtssAnnounceMethod)
		return DoAnnounce(inParams);
	if (*theMethod == qtssDescribeMethod)
		return DoDescribe(inParams);
	if (*theMethod == qtssSetupMethod)
		return DoSetup(inParams);
		
	RTPSessionOutput** theOutput = NULL;
	QTSS_Error theErr = QTSS_GetValuePtr(inParams->inClientSession, sOutputAttr, 0, (void**)&theOutput, &theLen);
	if ((theErr != QTSS_NoErr) || (theLen != sizeof(RTPSessionOutput*)))
	{	if (*theMethod == qtssPlayMethod)
			return DoPlay(inParams, NULL);
		else
			return QTSS_RequestFailed;
	}
	
	switch (*theMethod)
	{
		case qtssPlayMethod:
			return DoPlay(inParams, (*theOutput)->GetReflectorSession());
		case qtssTeardownMethod:
			// Tell the server that this session should be killed, and send a TEARDOWN response
			(void)QTSS_Teardown(inParams->inClientSession);
			(void)QTSS_SendStandardRTSPResponse(inParams->inRTSPRequest, inParams->inClientSession, 0);
			break;
		case qtssPauseMethod:
			(void)QTSS_Pause(inParams->inClientSession);
			(void)QTSS_SendStandardRTSPResponse(inParams->inRTSPRequest, inParams->inClientSession, 0);
			break;
		default:
			break;
	}			
	return QTSS_NoErr;
}

ReflectorSession* DoSessionSetup(QTSS_StandardRTSP_Params* inParams, QTSS_AttributeID inPathType,Bool16 isPush, Bool16 *foundSessionPtr)
{
	//printf("QTSSReflectorModule.cpp:DoSessionSetup\n");
	
	StrPtrLen theFullPath;
	QTSS_Error theErr = QTSS_GetValuePtr(inParams->inRTSPRequest, qtssRTSPReqLocalPath, 0, (void**)&theFullPath.Ptr, &theFullPath.Len);

	if (theFullPath.Len > sMOVSuffix.Len )
	{	StrPtrLen endOfPath2(&theFullPath.Ptr[theFullPath.Len -  sMOVSuffix.Len], sMOVSuffix.Len);
		if (endOfPath2.Equal(sMOVSuffix)) // it is a .mov so it is not meant for us
		{ 	return NULL;
		}
	}


	if (sAllowNonSDPURLs && !isPush)
	{
		// Check and see if the full path to this file matches an existing ReflectorSession
		StrPtrLen thePathPtr;
		OSCharArrayDeleter sdpPath(QTSSModuleUtils::GetFullPath(	inParams->inRTSPRequest,
																	inPathType,
																	&thePathPtr.Len, &sSDPSuffix));
		
		thePathPtr.Ptr = sdpPath.GetObject();

		// If the actual file path has a .sdp in it, first look for the URL without the extra .sdp
		if (thePathPtr.Len > (sSDPSuffix.Len * 2))
		{
			// Check and see if there is a .sdp in the file path.
			// If there is, truncate off our extra ".sdp", cuz it isn't needed
			StrPtrLen endOfPath(&sdpPath.GetObject()[thePathPtr.Len - (sSDPSuffix.Len * 2)], sSDPSuffix.Len);
			if (endOfPath.Equal(sSDPSuffix))
			{
				sdpPath.GetObject()[thePathPtr.Len - sSDPSuffix.Len] = '\0';
				thePathPtr.Len -= sSDPSuffix.Len;
			}
		}
		
		return FindOrCreateSession(&thePathPtr, inParams);
	}
	else
	{
		if (!sDefaultBroadcastPushEnabled)
			return NULL;
		//
		// We aren't supposed to auto-append a .sdp, so just get the URL path out of the server
		StrPtrLen theFullPath;
		QTSS_Error theErr = QTSS_GetValuePtr(inParams->inRTSPRequest, qtssRTSPReqLocalPath, 0, (void**)&theFullPath.Ptr, &theFullPath.Len);
		Assert(theErr == QTSS_NoErr);
		
		if (theFullPath.Len > sSDPSuffix.Len)
		{
			//
			// Check to make sure this path has a .sdp at the end. If it does,
			// attempt to get a reflector session for this URL.
			StrPtrLen endOfPath2(&theFullPath.Ptr[theFullPath.Len - sSDPSuffix.Len], sSDPSuffix.Len);
			if (endOfPath2.Equal(sSDPSuffix))
				return FindOrCreateSession(&theFullPath, inParams,NULL, isPush,foundSessionPtr);
		}
		return NULL;
	}
}

QTSS_Error DoAnnounce(QTSS_StandardRTSP_Params* inParams)
{
	if (!sAnnounceEnabled)
		return QTSSModuleUtils::SendErrorResponse(inParams->inRTSPRequest, qtssPreconditionFailed, sAnnounceDisabledNameErr);

	//
	// If this is SDP data, the reflector has the ability to write the data
	// to the file system location specified by the URL.
	
	//
	// This is a completely stateless action. No ReflectorSession gets created (obviously).
	
	//
	// Eventually, we should really require access control before we do this.
	//printf("QTSSReflectorModule:DoAnnounce\n");
	//
	// Get the full path to this file
	StrPtrLen theFullPath;
	QTSS_Error theErr = QTSS_GetValuePtr(inParams->inRTSPRequest, qtssRTSPReqLocalPath, 0, (void**)&theFullPath.Ptr, &theFullPath.Len);

	//printf("QTSSReflectorModule:DoAnnounce theFullPath.Ptr = %s\n",theFullPath.Ptr);
	//
	// Check for a .sdp at the end
	if (theFullPath.Len > sSDPSuffix.Len)
	{
		StrPtrLen endOfPath(theFullPath.Ptr + (theFullPath.Len - sSDPSuffix.Len), sSDPSuffix.Len);
		if (!endOfPath.Equal(sSDPSuffix))
			return QTSSModuleUtils::SendErrorResponse(inParams->inRTSPRequest, qtssPreconditionFailed, sAnnounceRequiresSDPinNameErr);

	}
	
	//
	// Ok, this is an sdp file. Retreive the entire contents of the SDP.
	// This has to be done asynchronously (in case the SDP stuff is fragmented across
	// multiple packets. So, we have to have a simple state machine.

	//
	// We need to know the content length to manage memory
	UInt32 theLen = 0;
	UInt32* theContentLenP = NULL;
	theErr = QTSS_GetValuePtr(inParams->inRTSPRequest, qtssRTSPReqContentLen, 0, (void**)&theContentLenP, &theLen);
	if ((theErr != QTSS_NoErr) || (theLen != sizeof(UInt32)))
	{
		//
		// RETURN ERROR RESPONSE: ANNOUNCE WITHOUT CONTENT LENGTH
		return QTSSModuleUtils::SendErrorResponse(inParams->inRTSPRequest, qtssClientBadRequest,0);
	}

	//
	// Check for the existence of 2 attributes in the request: a pointer to our buffer for
	// the request body, and the current offset in that buffer. If these attributes exist,
	// then we've already been here for this request. If they don't exist, add them.
	UInt32 theBufferOffset = 0;
	char* theRequestBody = NULL;

	theLen = sizeof(theRequestBody);
	theErr = QTSS_GetValue(inParams->inRTSPRequest, sRequestBodyAttr, 0, &theRequestBody, &theLen);

	//printf("QTSSReflectorModule:DoAnnounce theRequestBody =%s\n",theRequestBody);
	if (theErr != QTSS_NoErr)
	{
		//
		// First time we've been here for this request. Create a buffer for the content body and
		// shove it in the request.
		theRequestBody = NEW char[*theContentLenP + 1];
		memset(theRequestBody,0,*theContentLenP + 1);
		theLen = sizeof(theRequestBody);
		theErr = QTSS_SetValue(inParams->inRTSPRequest, sRequestBodyAttr, 0, &theRequestBody, theLen);// SetValue creates an internal copy.
		Assert(theErr == QTSS_NoErr);
		
		//
		// Also store the offset in the buffer
		theLen = sizeof(theBufferOffset);
		theErr = QTSS_SetValue(inParams->inRTSPRequest, sBufferOffsetAttr, 0, &theBufferOffset, theLen);
		Assert(theErr == QTSS_NoErr);
	}
	
	theLen = sizeof(theBufferOffset);
	theErr = QTSS_GetValue(inParams->inRTSPRequest, sBufferOffsetAttr, 0, &theBufferOffset, &theLen);

	//
	// We have our buffer and offset. Read the data.
	theErr = QTSS_Read(inParams->inRTSPRequest, theRequestBody + theBufferOffset, *theContentLenP - theBufferOffset, &theLen);
	Assert(theErr != QTSS_BadArgument);

	if (theErr == QTSS_RequestFailed)
	{
		OSCharArrayDeleter charArrayPathDeleter(theRequestBody);
		//
		// NEED TO RETURN RTSP ERROR RESPONSE
		return QTSSModuleUtils::SendErrorResponse(inParams->inRTSPRequest, qtssClientBadRequest,0);
	}
	
	if ((theErr == QTSS_WouldBlock) || (theLen < (*theContentLenP - theBufferOffset)))
	{
		//
		// Update our offset in the buffer
		theBufferOffset += theLen;
		(void)QTSS_SetValue(inParams->inRTSPRequest, sBufferOffsetAttr, 0, &theBufferOffset, sizeof(theBufferOffset));
		//printf("QTSSReflectorModule:DoAnnounce Request some more data \n");
		//
		// The entire content body hasn't arrived yet. Request a read event and wait for it.
		// Our DoAnnounce function will get called again when there is more data.
		theErr = QTSS_RequestEvent(inParams->inRTSPRequest, QTSS_ReadableEvent);
		Assert(theErr == QTSS_NoErr);
		return QTSS_NoErr;
	}

	Assert(theErr == QTSS_NoErr);
	
/*
	//
	// If we've gotten here, we have the entire content body in our buffer.
*/
	SDPSourceInfo theSDPSourceInfo(theRequestBody, strlen(theRequestBody));
	OSCharArrayDeleter charArrayPathDeleter(theRequestBody);
						
	if (!InfoPortsOK(inParams,&theSDPSourceInfo,&theFullPath)) // All validity checks like this check should be done before touching the file.
	{	return QTSS_NoErr; // InfoPortsOK is sending back the error.
	}
	
	{ // write the file !! need error reporting
		FILE* theSDPFile= ::fopen(theFullPath.Ptr, "wb");//open 
		if (theSDPFile != NULL)
		{	Bool16 resetSDPTime = false;
		
			if (sMaxBroadcastAnnounceDuration != 0) // we have to check the duration
			{	
				UInt32 theSDPDuration = theSDPSourceInfo.GetDurationSecs();
				//printf("SDP defined duration = %lu sMaxBroadcastAnnounceDuration = %lu\n",theSDPDuration,sMaxBroadcastAnnounceDuration);
				if (0 == theSDPDuration || theSDPDuration > sMaxBroadcastAnnounceDuration)
					resetSDPTime = true;
			}
			
			if (resetSDPTime)
			{	// this should be logged somewhere or the broadcaster sent a message?
				// this could also be handled as an error by the server.
				
				time_t timeNow = OS::UnixTime_Secs();
				StrPtrLen sdpData(theRequestBody);
				StringParser sdp(&sdpData);
				StrPtrLen sdpLine;
				char tempChar;
				//printf("QTSSReflectorModule Annonce: current t=%lu \n", theSDPSourceInfo.UnixSecs_to_NTPSecs(theSDPSourceInfo.GetStartTimeUnixSecs()));
				UInt32 startTime = theSDPSourceInfo.GetStartTimeUnixSecs();
				UInt32 endTime = 0;
				if (startTime == 0) // was an undefined start make it a fixed start time of now
				{	startTime = timeNow;
				}
				startTime = theSDPSourceInfo.UnixSecs_to_NTPSecs(startTime);
				endTime = startTime + sMaxBroadcastAnnounceDuration;
					 
				//printf("QTSSReflectorModule Annonce: Setting new SDP Time Line in file %s\n",theFullPath.Ptr);
				//printf("QTSSReflectorModule Annonce: new t=%lu %lu\n", startTime, endTime);

				::fprintf(theSDPFile, "t=%lu %lu\n", startTime, endTime);
				while (sdp.GetThruEOL(&sdpLine)) 
				{	// cheating here should parse through find all 0 end or later than new end and replace with new end
					if ( (sdpLine.Len > 0) && ('t' != *sdpLine.Ptr) ) // remove any old t lines 
					{	tempChar = sdpLine.Ptr[sdpLine.Len]; sdpLine.Ptr[sdpLine.Len] = 0;
						::fprintf(theSDPFile, "%s\n", sdpLine.Ptr);
						 sdpLine.Ptr[sdpLine.Len] = tempChar;
					}	
				}
			}
			else
			{
				::fprintf(theSDPFile, "%s", theRequestBody);
			}
			::fflush(theSDPFile);
			::fclose(theSDPFile);	

		}
		else
		{	return QTSSModuleUtils::SendErrorResponse(inParams->inRTSPRequest, qtssClientBadRequest,0);
		}
	}

	//printf("QTSSReflectorModule:DoAnnounce SendResponse OK=200\n");
	
	return QTSS_SendStandardRTSPResponse(inParams->inRTSPRequest, inParams->inClientSession, 0);
}

QTSS_Error DoDescribe(QTSS_StandardRTSP_Params* inParams)
{
	ReflectorSession* theSession = DoSessionSetup(inParams, qtssRTSPReqFilePath);
	if (theSession == NULL)
		return QTSS_RequestFailed;

	RTPSessionOutput** theOutput = NULL;
	UInt32 theLen = 0;
	QTSS_Error theErr = QTSS_GetValuePtr(inParams->inClientSession, sOutputAttr, 0, (void**)&theOutput, &theLen);

	// If there already  was an RTPSessionOutput attached to this Client Session,
	// destroy it.
	if (theErr == QTSS_NoErr)
		RemoveOutput(*theOutput, (*theOutput)->GetReflectorSession());

	//ok, we've found or setup the proper reflector session, create an RTPSessionOutput object,
	//and add it to the session's list of outputs
	RTPSessionOutput* theNewOutput = NEW RTPSessionOutput(inParams->inClientSession, theSession, sServerPrefs, sStreamCookieAttr );
	theSession->AddOutput(theNewOutput);

	// And vice-versa, store this reflector session in the RTP session.
	(void)QTSS_SetValue(inParams->inClientSession, sOutputAttr, 0, &theNewOutput, sizeof(theNewOutput));
		
	// Finally, send the DESCRIBE response
	
	//above function has signalled that this request belongs to us, so let's respond
	iovec theDescribeVec[2] = { {0 }};
	
	Assert(theSession->GetLocalSDP()->Ptr != NULL);
	theDescribeVec[1].iov_base = theSession->GetLocalSDP()->Ptr;
	theDescribeVec[1].iov_len = theSession->GetLocalSDP()->Len;
	(void)QTSS_AppendRTSPHeader(inParams->inRTSPRequest, qtssCacheControlHeader,
								kCacheControlHeader.Ptr, kCacheControlHeader.Len);
	QTSSModuleUtils::SendDescribeResponse(inParams->inRTSPRequest, inParams->inClientSession,
											&theDescribeVec[0], 2, theSession->GetLocalSDP()->Len);	
	return QTSS_NoErr;
}

Bool16 InfoPortsOK(QTSS_StandardRTSP_Params* inParams, SDPSourceInfo* theInfo, StrPtrLen* inPath)
{	// Check the ports based on the Pref whether to enforce a static SDP port range.

	Bool16 isOK = true;

	if (sEnforceStaticSDPPortRange)
	{	UInt16 theInfoPort = 0;
		for (UInt32 x = 0; x < theInfo->GetNumStreams(); x++)
		{	theInfoPort = theInfo->GetStreamInfo(x)->fPort;
			QTSS_AttributeID theErrorMessageID = qtssIllegalAttrID;
			if (theInfoPort != 0)
			{	if  (theInfoPort < sMinimumStaticSDPPort) 
					theErrorMessageID = sSDPcontainsInvalidMinimumPortErr;
				else if (theInfoPort > sMaximumStaticSDPPort)
					theErrorMessageID = sSDPcontainsInvalidMaximumPortErr;
			}
			
			if (theErrorMessageID != qtssIllegalAttrID)
			{	
				char thePort[32];
				sprintf(thePort,"%u",theInfoPort);

				char *thePath = inPath->GetAsCString();
				OSCharArrayDeleter charArrayPathDeleter(thePath);
				
				char *thePathPort = NEW char[inPath->Len + 32];
				OSCharArrayDeleter charArrayPathPortDeleter(thePathPort);
				
				sprintf(thePathPort,"%s:%s",thePath,thePort);
				(void) QTSSModuleUtils::LogError(qtssWarningVerbosity, theErrorMessageID, 0, thePathPort);
				
				StrPtrLen thePortStr(thePort);					
				(void) QTSSModuleUtils::SendErrorResponse(inParams->inRTSPRequest, qtssUnsupportedMediaType, theErrorMessageID,&thePortStr);

				return false;
			}
		}
	}
	
	return isOK;
}

ReflectorSession* FindOrCreateSession(StrPtrLen* inPath, QTSS_StandardRTSP_Params* inParams, StrPtrLen* inData, Bool16 isPush, Bool16 *foundSessionPtr)
{
	// This function assumes that inPath is NULL terminated
	// Ok, look for a reflector session matching this full path as the ID
	OSMutexLocker locker(sSessionMap->GetMutex());
	OSRef* theSessionRef = sSessionMap->Resolve(inPath);
	ReflectorSession* theSession = NULL;
	 
	if (theSessionRef == NULL)
	{
		//If this URL doesn't already have a reflector session, we must make a new
		//one. The first step is to create an SDPSourceInfo object.
		
		StrPtrLen theFileData;
		StrPtrLen theFileDeleteData;
		
		//
		// If no file data is provided by the caller, read the file data out of the file.
		// If file data is provided, use that as our SDP data
		if (inData == NULL)
		{	(void)QTSSModuleUtils::ReadEntireFile(inPath->Ptr, &theFileDeleteData);
			theFileData = theFileDeleteData;
		}
		else
			theFileData = *inData;
		OSCharArrayDeleter fileDataDeleter(theFileDeleteData.Ptr); 
			
		if (theFileData.Len <= 0)
			return NULL;
			
		SDPSourceInfo* theInfo = NEW SDPSourceInfo(theFileData.Ptr, theFileData.Len); // will make a copy
			
		if (!theInfo->IsReflectable())
		{	delete theInfo;
			return NULL;
		}
		if ( !theInfo->IsActiveNow() && !isPush)
		{	delete theInfo;
			return NULL;
		}
		
		if (!InfoPortsOK(inParams, theInfo, inPath))
		{	delete theInfo;
			return NULL;
		}
		//
		// Setup a ReflectorSession and bind the sockets. If we are negotiating,
		// make sure to let the session know that this is a Push Session so
		// ports may be modified.
		UInt32 theSetupFlag = ReflectorSession::kMarkSetup;
		if (isPush)
			theSetupFlag |= ReflectorSession::kIsPushSession;
		
		theSession = NEW ReflectorSession(inPath);
		
		// SetupReflectorSession stores theInfo in theSession so DONT delete the Info if we fail here, leave it alone.
		// deleting the session will delete the info.
		QTSS_Error theErr = theSession->SetupReflectorSession(theInfo, inParams, theSetupFlag,sOneSSRCPerStream, sTimeoutSSRCSecs);
		if (theErr != QTSS_NoErr || theSession == NULL)
		{	delete theSession;
			return NULL;
		}
//		infoPtrDeleter.ClearObject(); // don't delete info we are going to keep it in the session
		
		//printf("Created reflector session = %lu theInfo=%lu \n", (UInt32) theSession,(UInt32)theInfo);
		//put the session's ID into the session map.
		theErr = sSessionMap->Register(theSession->GetRef());
		Assert(theErr == QTSS_NoErr);

		//unless we do this, the refcount won't increment (and we'll delete the session prematurely
		if (!isPush)
		{	OSRef* debug = sSessionMap->Resolve(inPath);
			Assert(debug == theSession->GetRef());
		}
	}
	else
	{
	
		if (isPush) 
			sSessionMap->Release(theSessionRef); // don't need if a push;// don't need if a push; A Release is necessary or we will leak ReflectorSessions.

		if (foundSessionPtr)
			*foundSessionPtr = true;
			
		StrPtrLen theFileData;
		SDPSourceInfo* theInfo = NULL;
		
		if (inData == NULL)
			(void)QTSSModuleUtils::ReadEntireFile(inPath->Ptr, &theFileData);
		OSCharArrayDeleter charArrayDeleter(theFileData.Ptr);
			
		if (theFileData.Len <= 0)
			return NULL;
		
		theInfo = NEW SDPSourceInfo(theFileData.Ptr, theFileData.Len);
		if (theInfo == NULL) 
			return NULL;
			
		if ( !theInfo->IsActiveNow() && !isPush)
		{	delete theInfo;
			return NULL;
		}
		
		if (!InfoPortsOK(inParams, theInfo, inPath))
		{	delete theInfo;
			return NULL;
		}
		
		delete theInfo;
		
		theSession = (ReflectorSession*)theSessionRef->GetObject(); 
		if (isPush && theSession)
		{
			UInt32 theSetupFlag = ReflectorSession::kMarkSetup | ReflectorSession::kIsPushSession;
			QTSS_Error theErr = theSession->SetupReflectorSession(NULL, inParams, theSetupFlag);
			if (theErr != QTSS_NoErr)
			{	return NULL;
			}
		}
	}
			
	Assert(theSession != NULL);

	return theSession;
}

// ONLY call when performing a setup.
void DeleteReflectorPushSession(QTSS_StandardRTSP_Params* inParams, ReflectorSession* theSession, Bool16 foundSession)
{
	ReflectorSession* stopSessionProcessing = NULL;
	QTSS_Error theErr  = QTSS_SetValue(inParams->inClientSession, sClientBroadcastSessionAttr, 0, &stopSessionProcessing, sizeof(stopSessionProcessing));
	Assert(theErr == QTSS_NoErr);

	if (foundSession) 
		return; // we didn't allocate the session so don't delete

	OSRef* theSessionRef = theSession->GetRef();
	if (theSessionRef != NULL) 
	{				
		theSession->TearDownAllOutputs(); // just to be sure because we are about to delete the session.
		sSessionMap->UnRegister(theSessionRef);// we had an error while setting up-- don't let anyone get the session
		delete theSession;	
	}
}

QTSS_Error AddRTPStream(ReflectorSession* theSession,QTSS_StandardRTSP_Params* inParams, QTSS_RTPStreamObject *newStreamPtr)
{		
	// Ok, this is completely crazy but I can't think of a better way to do this that's
	// safe so we'll do it this way for now. Because the ReflectorStreams use this session's
	// stream queue, we need to make sure that each ReflectorStream is not reflecting to this
	// session while we call QTSS_AddRTPStream. One brutal way to do this is to grab each
	// ReflectorStream's mutex, which will stop every reflector stream from running.
	Assert(newStreamPtr != NULL);
	
	if (theSession != NULL)
		for (UInt32 x = 0; x < theSession->GetNumStreams(); x++)
			theSession->GetStreamByIndex(x)->GetMutex()->Lock();
	
	//
	// Turn off reliable UDP transport, because we are not yet equipped to
	// do overbuffering.
	QTSS_Error theErr = QTSS_AddRTPStream(inParams->inClientSession, inParams->inRTSPRequest, newStreamPtr, qtssASFlagsForceUDPTransport);

	if (theSession != NULL)
		for (UInt32 y = 0; y < theSession->GetNumStreams(); y++)
			theSession->GetStreamByIndex(y)->GetMutex()->Unlock();

	return theErr;
}

QTSS_Error DoSetup(QTSS_StandardRTSP_Params* inParams)
{
	ReflectorSession* theSession = NULL;
	//printf("QTSSReflectorModule.cpp:DoSetup \n");

	// See if this is a push from a Broadcaster
	UInt32 theLen = 0;
	UInt32 *transportModePtr = NULL;
	QTSS_Error theErr  = QTSS_GetValuePtr(inParams->inRTSPRequest, qtssRTSPReqTransportMode, 0, (void**)&transportModePtr, &theLen);
	Bool16 isPush = (transportModePtr != NULL && *transportModePtr == qtssRTPTransportModeReceive) ? true : false;
	Bool16 foundSession = false;
	
	// Check to see if we have a RTPSessionOutput for this Client Session. If we don't,
	// we should make one
	RTPSessionOutput** theOutput = NULL;
	theErr = QTSS_GetValuePtr(inParams->inClientSession, sOutputAttr, 0, (void**)&theOutput, &theLen);
	if (theLen != sizeof(RTPSessionOutput*))
	{
		//
		// This may be an incoming data session. If that's the case, there will be a Reflector
		// Session in the ClientSession
		//theLen = sizeof(theSession);
		//theErr = QTSS_GetValue(inParams->inClientSession, sSessionAttr, 0, &theSession, &theLen);
		
		if (theErr != QTSS_NoErr  && !isPush)
		{
			// This is not an incoming data session...
			// Do the standard ReflectorSession setup, create an RTPSessionOutput
			theSession = DoSessionSetup(inParams, qtssRTSPReqFilePathTrunc);
			if (theSession == NULL)
				return QTSS_RequestFailed;
			
			RTPSessionOutput* theNewOutput = NEW RTPSessionOutput(inParams->inClientSession, theSession, sServerPrefs, sStreamCookieAttr );
			theSession->AddOutput(theNewOutput);
			(void)QTSS_SetValue(inParams->inClientSession, sOutputAttr, 0, &theNewOutput, sizeof(theNewOutput));
		}
		else 
		{			
			theSession = DoSessionSetup(inParams, qtssRTSPReqFilePathTrunc,isPush,&foundSession); 
			if (theSession == NULL)
				return QTSS_RequestFailed;	
				
			// This is an incoming data session. Set the Reflector Session in the ClientSession
			theErr = QTSS_SetValue(inParams->inClientSession, sClientBroadcastSessionAttr, 0, &theSession, sizeof(theSession));
			Assert(theErr == QTSS_NoErr);
			//printf("QTSSReflectorModule.cpp:SETsession	sClientBroadcastSessionAttr=%lu theSession=%lu err=%ld \n",(UInt32)sClientBroadcastSessionAttr, (UInt32) theSession,theErr);
		}
	}
	else
	{	theSession = (*theOutput)->GetReflectorSession();
		if (theSession == NULL)
			return QTSS_RequestFailed;	
	}

	//unless there is a digit at the end of this path (representing trackID), don't
	//even bother with the request
	StrPtrLen theDigit;
	(void)QTSS_GetValuePtr(inParams->inRTSPRequest, qtssRTSPReqFileDigit, 0, (void**)&theDigit.Ptr, &theDigit.Len);
	if (theDigit.Len == 0)
	{	if (isPush)
			DeleteReflectorPushSession(inParams,theSession, foundSession);
		return QTSSModuleUtils::SendErrorResponse(inParams->inRTSPRequest, qtssClientBadRequest,sExpectedDigitFilenameErr);
	}
													
	UInt32 theTrackID = ::strtol(theDigit.Ptr, NULL, 10);
	
	//
	// If this is an incoming data session, skip everything having to do with setting up a new
	// RTP Stream. 
	if (isPush)
	{
		//printf("QTSSReflectorModule.cpp:DoSetup is push setup\n");

		// Get info about this trackID
		SourceInfo::StreamInfo* theStreamInfo = theSession->GetSourceInfo()->GetStreamInfoByTrackID(theTrackID);
		// If theStreamInfo is NULL, we don't have a legit track, so return an error
		if (theStreamInfo == NULL)
		{	
			DeleteReflectorPushSession(inParams,theSession, foundSession);
			return QTSSModuleUtils::SendErrorResponse(inParams->inRTSPRequest, qtssClientBadRequest,
														sReflectorBadTrackIDErr);
		}
		
		if (!sAllowDuplicateBroadcasts && theStreamInfo->fSetupToReceive) 
		{
			DeleteReflectorPushSession(inParams,theSession, foundSession);	
			return QTSSModuleUtils::SendErrorResponse(inParams->inRTSPRequest, qtssPreconditionFailed, sDuplicateBroadcastStreamErr);
		}

		UInt16 theReceiveBroadcastStreamPort = theStreamInfo->fPort;
		theErr = QTSS_SetValue(inParams->inRTSPRequest, qtssRTSPReqSetUpServerPort, 0, &theReceiveBroadcastStreamPort, sizeof(theReceiveBroadcastStreamPort));
		Assert(theErr == QTSS_NoErr);
		

		QTSS_RTPStreamObject newStream = NULL;
		theErr = AddRTPStream(theSession,inParams,&newStream);
		Assert(theErr == QTSS_NoErr);
		if (theErr != QTSS_NoErr)
		{	
			DeleteReflectorPushSession(inParams,theSession, foundSession);
			return QTSSModuleUtils::SendErrorResponse(inParams->inRTSPRequest, qtssClientBadRequest, 0);
		}
			
		//send the setup response

		(void)QTSS_AppendRTSPHeader(inParams->inRTSPRequest, qtssCacheControlHeader,
									kCacheControlHeader.Ptr, kCacheControlHeader.Len);

		(void)QTSS_SendStandardRTSPResponse(inParams->inRTSPRequest, newStream, 0);
		
		theStreamInfo->fSetupToReceive = true;
		// This is an incoming data session. Set the Reflector Session in the ClientSession
		theErr = QTSS_SetValue(inParams->inClientSession, sClientBroadcastSessionAttr, 0, &theSession, sizeof(theSession));
		Assert(theErr == QTSS_NoErr);
			
		if (theSession != NULL)
			theSession->AddBroadcasterClientSession(inParams);
			
		return QTSS_NoErr;		
	}

	
	// Get info about this trackID
	SourceInfo::StreamInfo* theStreamInfo = theSession->GetSourceInfo()->GetStreamInfoByTrackID(theTrackID);
	// If theStreamInfo is NULL, we don't have a legit track, so return an error
	if (theStreamInfo == NULL)
		return QTSSModuleUtils::SendErrorResponse(inParams->inRTSPRequest, qtssClientBadRequest,
													sReflectorBadTrackIDErr);
													
	StrPtrLen* thePayloadName = &theStreamInfo->fPayloadName;
	QTSS_RTPPayloadType thePayloadType = theStreamInfo->fPayloadType;
	
	QTSS_RTPStreamObject newStream = NULL;
	{
		// Ok, this is completely crazy but I can't think of a better way to do this that's
		// safe so we'll do it this way for now. Because the ReflectorStreams use this session's
		// stream queue, we need to make sure that each ReflectorStream is not reflecting to this
		// session while we call QTSS_AddRTPStream. One brutal way to do this is to grab each
		// ReflectorStream's mutex, which will stop every reflector stream from running.
		
		for (UInt32 x = 0; x < theSession->GetNumStreams(); x++)
			theSession->GetStreamByIndex(x)->GetMutex()->Lock();
			
		theErr = QTSS_AddRTPStream(inParams->inClientSession, inParams->inRTSPRequest, &newStream, 0);

		for (UInt32 y = 0; y < theSession->GetNumStreams(); y++)
			theSession->GetStreamByIndex(y)->GetMutex()->Unlock();
			
		if (theErr != QTSS_NoErr)
			return theErr;
	}
	
	// Set up dictionary items for this stream
	theErr = QTSS_SetValue(newStream, qtssRTPStrPayloadName, 0, thePayloadName->Ptr, thePayloadName->Len);
	Assert(theErr == QTSS_NoErr);
	theErr = QTSS_SetValue(newStream, qtssRTPStrPayloadType, 0, &thePayloadType, sizeof(thePayloadType));
	Assert(theErr == QTSS_NoErr);
	theErr = QTSS_SetValue(newStream, qtssRTPStrTrackID, 0, &theTrackID, sizeof(theTrackID));
	Assert(theErr == QTSS_NoErr);

	// Place the stream cookie in this stream for future reference
	void* theStreamCookie = theSession->GetStreamCookie(theTrackID);
	Assert(theStreamCookie != NULL);
	theErr = QTSS_SetValue(newStream, sStreamCookieAttr, 0, &theStreamCookie, sizeof(theStreamCookie));
	Assert(theErr == QTSS_NoErr);

	// Set the number of quality levels.
	static UInt32 sNumQualityLevels = ReflectorSession::kNumQualityLevels;
	theErr = QTSS_SetValue(newStream, qtssRTPStrNumQualityLevels, 0, &sNumQualityLevels, sizeof(sNumQualityLevels));
	Assert(theErr == QTSS_NoErr);
	
	//send the setup response
	(void)QTSS_AppendRTSPHeader(inParams->inRTSPRequest, qtssCacheControlHeader,
								kCacheControlHeader.Ptr, kCacheControlHeader.Len);
	(void)QTSS_SendStandardRTSPResponse(inParams->inRTSPRequest, newStream, 0);
	return QTSS_NoErr;
}


QTSS_Error DoPlay(QTSS_StandardRTSP_Params* inParams, ReflectorSession* inSession)
{
	QTSS_Error theErr = QTSS_NoErr;
	if (inSession == NULL) // it is a broadcast session so store the broadcast session.
	{	if (!sDefaultBroadcastPushEnabled)
			return QTSS_RequestFailed;

		UInt32 theLen = sizeof(inSession);
		theErr = QTSS_GetValue(inParams->inClientSession, sClientBroadcastSessionAttr, 0, &inSession, &theLen);
		if (theErr != QTSS_NoErr)
			return QTSS_RequestFailed;
			
		Assert(inSession != NULL);
		
		theErr = QTSS_SetValue(inParams->inRTSPSession, sRTSPBroadcastSessionAttr, 0, &inSession, sizeof(inSession));
		if (theErr != QTSS_NoErr)
			return QTSS_RequestFailed;
	
		//printf("QTSSReflectorModule:SET for att err=%ld id=%ld\n",theErr,inParams->inRTSPSession);
			
{ // this code needs to be cleaned up
		// Check and see if the full path to this file matches an existing ReflectorSession
		StrPtrLen thePathPtr;
		OSCharArrayDeleter sdpPath(QTSSModuleUtils::GetFullPath(	inParams->inRTSPRequest,
																	qtssRTSPReqFilePath,
																	&thePathPtr.Len, &sSDPSuffix));
		
		thePathPtr.Ptr = sdpPath.GetObject();

		// If the actual file path has a .sdp in it, first look for the URL without the extra .sdp
		if (thePathPtr.Len > (sSDPSuffix.Len * 2))
		{
			// Check and see if there is a .sdp in the file path.
			// If there is, truncate off our extra ".sdp", cuz it isn't needed
			StrPtrLen endOfPath(&sdpPath.GetObject()[thePathPtr.Len - (sSDPSuffix.Len * 2)], sSDPSuffix.Len);
			if (endOfPath.Equal(sSDPSuffix))
			{
				sdpPath.GetObject()[thePathPtr.Len - sSDPSuffix.Len] = '\0';
				thePathPtr.Len -= sSDPSuffix.Len;
			}
		}
		// do all above so we can add the session to the map with Resolve here.
		// we must only do this once.
		OSRef* debug = sSessionMap->Resolve(&thePathPtr);
		Assert(debug == inSession->GetRef());

}
	
		KeepSession(inParams->inRTSPRequest,true);
		//printf("QTSSReflectorModule.cpp:DoPlay (PUSH) inRTSPSession=%lu inClientSession=%lu\n",(UInt32)inParams->inRTSPSession,(UInt32)inParams->inClientSession);
	}
	else// it is NOT a broadcaster session
	{	
		// Tell the session what the bitrate of this reflection is. This is nice for logging,
		// it also allows the server to scale the TCP buffer size appropriately if we are
		// interleaving the data over TCP. This must be set before calling QTSS_Play so the
		// server can use it from within QTSS_Play
		UInt32 bitsPerSecond =	inSession->GetBitRate();
		(void)QTSS_SetValue(inParams->inClientSession, qtssCliSesMovieAverageBitRate, 0, &bitsPerSecond, sizeof(bitsPerSecond));

		//Server shouldn't send RTCP (reflector does it), but the server should append the server info app packet
		QTSS_Error theErr = QTSS_Play(inParams->inClientSession, inParams->inRTSPRequest, qtssPlayFlagsAppendServerInfo);
		if (theErr != QTSS_NoErr)
			return theErr;
	}
	
	//and send a standard play response
	(void)QTSS_SendStandardRTSPResponse(inParams->inRTSPRequest, inParams->inClientSession, 0);
	return QTSS_NoErr;
}

QTSS_Error DestroySession(QTSS_ClientSessionClosing_Params* inParams)
{
	RTPSessionOutput** 	theOutput = NULL;
	ReflectorOutput* 	outputPtr = NULL;
	ReflectorSession* 	theSession = NULL;
	
	UInt32 theLen = sizeof(theSession);
	QTSS_Error theErr = QTSS_GetValue(inParams->inClientSession, sClientBroadcastSessionAttr, 0, &theSession, &theLen);
	//printf("QTSSReflectorModule.cpp:DestroySession	sClientBroadcastSessionAttr=%lu theSession=%lu err=%ld \n",(UInt32)sClientBroadcastSessionAttr, (UInt32)theSession,theErr);
	if (theSession != NULL) // it is a broadcaster session
	{	
		ReflectorSession* 	deletedSession = NULL;
		theErr = QTSS_SetValue(inParams->inClientSession, sClientBroadcastSessionAttr, 0, &deletedSession, sizeof(deletedSession));

		SourceInfo* theSoureInfo = theSession->GetSourceInfo();	
		if (theSoureInfo == NULL)
			return QTSS_NoErr;
			
		UInt32	numStreams = theSession->GetNumStreams();
		SourceInfo::StreamInfo* theStreamInfo = NULL;
			
		for (UInt32 index = 0; index < numStreams; index++)
		{	theStreamInfo = theSoureInfo->GetStreamInfo(index);
			if (theStreamInfo != NULL)
				theStreamInfo->fSetupToReceive = false;
		}

		//printf("QTSSReflectorModule.cpp:DestroySession broadcaster theSession=%lu\n", (UInt32) theSession);
		theSession->RemoveSessionFromOutput(inParams->inClientSession);
 		RemoveOutput(NULL, theSession);
 	}
	else
	{
		theLen = 0;
		theErr = QTSS_GetValuePtr(inParams->inClientSession, sOutputAttr, 0, (void**)&theOutput, &theLen);
		if ((theErr != QTSS_NoErr) || (theLen != sizeof(RTPSessionOutput*)) || (theOutput == NULL))
			return QTSS_RequestFailed;
		theSession = (*theOutput)->GetReflectorSession();
	
		if (theOutput != NULL)
			outputPtr = (ReflectorOutput*) *theOutput;
			
		if (outputPtr != NULL)
			RemoveOutput(outputPtr, theSession);
				
	}

	return QTSS_NoErr;
}

void RemoveOutput(ReflectorOutput* inOutput, ReflectorSession* inSession)
{
	//printf("QTSSReflectorModule.cpp:RemoveOutput\n");
	// This function removes the output from the ReflectorSession, then
	Assert(inSession);
	if (inSession != NULL)
	{
		if (inOutput != NULL)
		{	inSession->RemoveOutput(inOutput);
			//printf("QTSSReflectorModule.cpp:RemoveOutput it is a client session\n");
		}
		else
		{ // it is a Broadcaster session
			//printf("QTSSReflectorModule.cpp:RemoveOutput it is a broadcaster session\n");
			SourceInfo* theInfo = inSession->GetSourceInfo();         
			Assert(theInfo);
			
			if (theInfo->IsRTSPControlled())
			{
				FileDeleter(inSession->GetSourcePath());
			}
			
			if (sTearDownClientsOnDisconnect)
				inSession->TearDownAllOutputs();
		}
	
		//printf("QTSSReflectorModule.cpp:RemoveOutput refcount =%lu\n", inSession->GetRef()->GetRefCount() );

		//check if the ReflectorSession should be deleted
		//(it should if its ref count has dropped to 0)
		OSMutexLocker locker (sSessionMap->GetMutex());
		//decrement the ref count
		
		OSRef* theSessionRef = inSession->GetRef();
		if (theSessionRef != NULL) 
		{				
			if (theSessionRef->GetRefCount() == 0)
			{	sSessionMap->UnRegister(theSessionRef);// we had an error while setting up
				delete inSession;
			}
			else if (theSessionRef->GetRefCount() == 1)
			{		
				//printf("QTSSReflector.cpp:RemoveOutput Delete SESSION=%lu\n",(UInt32)inSession);
				sSessionMap->Release(theSessionRef);
				sSessionMap->UnRegister(theSessionRef);  // the last session so get rid of the ref
				delete inSession;
			}
			else
			{
				//printf("QTSSReflector.cpp:RemoveOutput Release SESSION=%lu\n",(UInt32)inSession);
				sSessionMap->Release(theSessionRef); //  one of the sessions on the ref is ending just decrement the count
			}
				
		}
	}
	
	delete inOutput;
}
QTSS_Error AuthorizeRequest(QTSS_StandardRTSP_Params* inParams, Bool16 allowNoAccessFiles, QTSS_ActionFlags noAction, QTSS_ActionFlags authorizeAction)
{
	if  ( (NULL == inParams) || (NULL == inParams->inRTSPRequest) )
		return QTSS_RequestFailed;

	QTSS_RTSPRequestObject	theRTSPRequest = inParams->inRTSPRequest;
	
	// get the type of request
	// Don't touch write requests
	QTSS_ActionFlags action = QTSSModuleUtils::GetRequestActions(theRTSPRequest);
	if(action == qtssActionFlagsNoFlags)
		return QTSS_RequestFailed;
	
	if( (action & noAction) != 0)
		return QTSS_NoErr; // we don't handle
	
	//get the local file path
	char*	pathBuffStr = QTSSModuleUtils::GetLocalPath_Copy(theRTSPRequest);
	OSCharArrayDeleter pathBuffDeleter(pathBuffStr);
	if (NULL == pathBuffStr)
		return QTSS_RequestFailed;

	//get the root movie directory
	char*	movieRootDirStr = QTSSModuleUtils::GetMoviesRootDir_Copy(theRTSPRequest);
	OSCharArrayDeleter movieRootDeleter(movieRootDirStr);
	if (NULL == movieRootDirStr)
		return QTSS_RequestFailed;
	
	QTSS_UserProfileObject theUserProfile = QTSSModuleUtils::GetUserProfileObject(theRTSPRequest);
	if (NULL == theUserProfile)
		return QTSS_RequestFailed;

	char* accessFilePath = QTAccessFile::GetAccessFile_Copy(movieRootDirStr, pathBuffStr);
	OSCharArrayDeleter accessFilePathDeleter(accessFilePath);
	
	if (NULL == accessFilePath) // we are done nothing to do
	{	if (QTSS_NoErr != QTSS_SetValue(theRTSPRequest,qtssRTSPReqUserAllowed, 0, &allowNoAccessFiles, sizeof(allowNoAccessFiles)))
			return QTSS_RequestFailed; // Bail on the request. The Server will handle the error
		return QTSS_NoErr;
	}
	
	
	char* username = QTSSModuleUtils::GetUserName_Copy(theUserProfile);
	OSCharArrayDeleter usernameDeleter(username);
	
	UInt32 numGroups = 0;
	char** groupCharPtrArray = 	QTSSModuleUtils::GetGroupsArray_Copy(theUserProfile, &numGroups);
	OSArrayObjectDeleter<char*> OSCharPtrArrayDeleter(groupCharPtrArray);
	
	StrPtrLen accessFileBuf;
	(void)QTSSModuleUtils::ReadEntireFile(accessFilePath, &accessFileBuf);
	OSCharArrayDeleter accessFileBufDeleter(accessFileBuf.Ptr);

	char realmName[kBuffLen] = { 0 };
	StrPtrLen	realmNameStr(realmName,kBuffLen -1);
	
	//check if this user is allowed to see this movie
	Bool16 allowRequest = QTAccessFile::AccessAllowed(username, groupCharPtrArray, numGroups,  &accessFileBuf, authorizeAction,&realmNameStr);

	if ( realmNameStr.Ptr[0] != '\0' ) 	//set the realm if we have one
		(void) QTSS_SetValue(theRTSPRequest,qtssRTSPReqURLRealm, 0, realmNameStr.Ptr, ::strlen(realmNameStr.Ptr) );
	
	// We are denying the request so pass false back to the server.
	if (QTSS_NoErr != QTSS_SetValue(theRTSPRequest,qtssRTSPReqUserAllowed, 0, &allowRequest, sizeof(allowRequest)))
		return QTSS_RequestFailed; // Bail on the request. The Server will handle the error
		
	return QTSS_NoErr;
}

Bool16 IsLocalSession(QTSS_StandardRTSP_Params* inParams)
{	
	QTSS_RTSPSessionObject inRTSPSession = inParams->inRTSPSession;
	char ipAddress[32];ipAddress[0] = 0;
	StrPtrLen theClientIPAddressStr;
	theClientIPAddressStr.Set(ipAddress,32);
	(void) QTSS_GetValue(inRTSPSession, qtssRTSPSesRemoteAddrStr, 0, (void*)theClientIPAddressStr.Ptr, &theClientIPAddressStr.Len);

	if (theClientIPAddressStr.Len > 0) // remove the 4th field if there is one
	{	SInt16 len = theClientIPAddressStr.Len -1;
		while (len > 0 && theClientIPAddressStr.Ptr[len] != '.')	
		{	theClientIPAddressStr.Ptr[len] = 0;
			len --;
		}
		if (len > 0)
			theClientIPAddressStr.Len = ::strlen(theClientIPAddressStr.Ptr);
	}
	
	return theClientIPAddressStr.Equal("127.0.0."); // just check the first 3 fields. Allow any 4th field
}

QTSS_Error ReflectorAuthorizeRTSPRequest(QTSS_StandardRTSP_Params* inParams)
{	
	if (IsLocalSession(inParams))
		return QTSS_NoErr;

	Bool16 allowNoAccessFiles = false;
	QTSS_ActionFlags noAction = qtssActionFlagsRead;
	QTSS_ActionFlags authorizeAction = qtssActionFlagsWrite;

	return QTAccessFile::AuthorizeRequest(inParams,allowNoAccessFiles, noAction, authorizeAction);
}

