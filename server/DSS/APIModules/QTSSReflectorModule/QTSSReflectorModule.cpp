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
#include "OSArrayObjectDeleter.h"

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

// STATIC DATA

// ref to the prefs dictionary object
static OSRefTable* 		sSessionMap = NULL;
static const StrPtrLen 	kCacheControlHeader("no-cache");
static QTSS_PrefsObject sServerPrefs = NULL;
static QTSS_ModulePrefsObject 		sPrefs = NULL;

//
// Prefs
static Bool16	sAllowNonSDPURLs = true;
static Bool16	sDefaultAllowNonSDPURLs = true;

// Important strings
static StrPtrLen	sSDPSuffix(".sdp");

// FUNCTION PROTOTYPES

static QTSS_Error QTSSReflectorModuleDispatch(QTSS_Role inRole, QTSS_RoleParamPtr inParams);
static QTSS_Error 	Register(QTSS_Register_Params* inParams);
static QTSS_Error Initialize(QTSS_Initialize_Params* inParams);
static QTSS_Error Shutdown();
static QTSS_Error ProcessRTSPRequest(QTSS_StandardRTSP_Params* inParams);
static QTSS_Error DoAnnounce(QTSS_StandardRTSP_Params* inParams);
static QTSS_Error DoDescribe(QTSS_StandardRTSP_Params* inParams);
ReflectorSession* FindOrCreateSession(StrPtrLen* inPath, QTSS_RTSPRequestObject inRequest, StrPtrLen* inData = NULL);
static QTSS_Error DoSetup(QTSS_StandardRTSP_Params* inParams);
static QTSS_Error DoPlay(QTSS_StandardRTSP_Params* inParams, ReflectorSession* inSession);
static QTSS_Error DestroySession(QTSS_ClientSessionClosing_Params* inParams);
static void RemoveOutput(ReflectorOutput* inOutput, ReflectorSession* inSession);
static ReflectorSession* DoSessionSetup(QTSS_StandardRTSP_Params* inParams, QTSS_AttributeID inPathType);
static QTSS_Error RereadPrefs();



// FUNCTION IMPLEMENTATIONS
#pragma mark __QTSS_REFLECTOR_MODULE__

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
		case QTSS_ClientSessionClosing_Role:
			return DestroySession(&inParams->clientSessionClosingParams);
		case QTSS_Shutdown_Role:
			return Shutdown();
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
	
	// Add text messages attributes
	static char*		sExpectedDigitFilenameName		= "QTSSReflectorModuleExpectedDigitFilename";
	static char*		sReflectorBadTrackIDErrName		= "QTSSReflectorModuleBadTrackID";

	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sExpectedDigitFilenameName, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sExpectedDigitFilenameName, &sExpectedDigitFilenameErr);

	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sReflectorBadTrackIDErrName, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sReflectorBadTrackIDErrName, &sReflectorBadTrackIDErr);
	
	// Add an RTP session attribute for tracking ReflectorSession objects
	static char*		sOutputName			= "QTSSReflectorModuleOutput";
	static char*		sSessionName		= "QTSSReflectorModuleSession";
	static char*		sStreamCookieName	= "QTSSReflectorModuleStreamCookie";
	static char*		sRequestBufferName	= "QTSSReflectorModuleRequestBuffer";
	static char*		sRequestBufferLenName="QTSSReflectorModuleRequestBufferLen";

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
	sSessionMap = NEW OSRefTable();
	sServerPrefs = inParams->inPrefs;
	
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
	static QTSS_RTSPMethod sSupportedMethods[] = { qtssDescribeMethod, qtssSetupMethod, qtssTeardownMethod, qtssPlayMethod, qtssPauseMethod, qtssAnnounceMethod, qtssRecordMethod };
	QTSSModuleUtils::SetupSupportedMethods(inParams->inServer, sSupportedMethods, 7);

	RereadPrefs();
	
	return QTSS_NoErr;
}

QTSS_Error RereadPrefs()
{
	//
	// Use the standard GetPref routine to retrieve the correct values for our preferences
	QTSSModuleUtils::GetPref(sPrefs, "allow_non_sdp_urls", 	qtssAttrDataTypeBool16,
								&sAllowNonSDPURLs, &sDefaultAllowNonSDPURLs, sizeof(sDefaultAllowNonSDPURLs));
	return QTSS_NoErr;
}

QTSS_Error Shutdown()
{
#if QTSS_REFLECTOR_EXTERNAL_MODULE
	TaskThreadPool::RemoveThreads();
#endif
	return QTSS_NoErr;
}

QTSS_Error ProcessRTSPRequest(QTSS_StandardRTSP_Params* inParams)
{
	QTSS_RTSPMethod* theMethod = NULL;
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
		return QTSS_RequestFailed;
	
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

ReflectorSession* DoSessionSetup(QTSS_StandardRTSP_Params* inParams, QTSS_AttributeID inPathType)
{
	if (sAllowNonSDPURLs)
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
		
		return FindOrCreateSession(&thePathPtr, inParams->inRTSPRequest);
	}
	else
	{
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
				return FindOrCreateSession(&theFullPath, inParams->inRTSPRequest);
		}
		return NULL;
	}
}

QTSS_Error DoAnnounce(QTSS_StandardRTSP_Params* inParams)
{
	//
	// If this is SDP data, the reflector has the ability to write the data
	// to the file system location specified by the URL.
	
	//
	// This is a completely stateless action. No ReflectorSession gets created (obviously).
	
	//
	// Eventually, we should really require access control before we do this.
	
	//
	// Get the full path to this file
	StrPtrLen theFullPath;
	QTSS_Error theErr = QTSS_GetValuePtr(inParams->inRTSPRequest, qtssRTSPReqLocalPath, 0, (void**)&theFullPath.Ptr, &theFullPath.Len);

	//
	// Check for a .sdp at the end
	if (theFullPath.Len > sSDPSuffix.Len)
	{
		StrPtrLen endOfPath(theFullPath.Ptr + (theFullPath.Len - sSDPSuffix.Len), sSDPSuffix.Len);
		if (!endOfPath.Equal(sSDPSuffix))
			return QTSS_NoErr;
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
		return QTSS_NoErr;
	}

	//
	// Check for the existence of 2 attributes in the request: a pointer to our buffer for
	// the request body, and the current offset in that buffer. If these attributes exist,
	// then we've already been here for this request. If they don't exist, add them.
	UInt32 theBufferOffset = 0;
	char* theRequestBody = NULL;

	theLen = sizeof(theRequestBody);
	theErr = QTSS_GetValue(inParams->inRTSPRequest, sRequestBodyAttr, 0, &theRequestBody, &theLen);

	if (theErr != QTSS_NoErr)
	{
		//
		// First time we've been here for this request. Create a buffer for the content body and
		// shove it in the request.
		theRequestBody = NEW char[*theContentLenP + 1];
		theLen = sizeof(theRequestBody);
		theErr = QTSS_SetValue(inParams->inRTSPRequest, sRequestBodyAttr, 0, &theRequestBody, theLen);
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
	Assert(theErr != QTSS_RequestFailed); // THIS CAN HAPPEN! FIXME!
	
	if (theErr == QTSS_RequestFailed)
	{
		//
		// NEED TO RETURN RTSP ERROR RESPONSE
	}
	
	if ((theErr == QTSS_WouldBlock) || (theLen < (*theContentLenP - theBufferOffset)))
	{
		//
		// Update our offset in the buffer
		theBufferOffset += theLen;
		(void)QTSS_SetValue(inParams->inRTSPRequest, sBufferOffsetAttr, 0, &theBufferOffset, sizeof(theBufferOffset));
		
		//
		// The entire content body hasn't arrived yet. Request a read event and wait for it.
		// Our DoAnnounce function will get called again when there is more data.
		theErr = QTSS_RequestEvent(inParams->inRTSPRequest, QTSS_ReadableEvent);
		Assert(theErr == QTSS_NoErr);
		return QTSS_NoErr;
	}
	
	//
	// If we've gotten here, we have the entire content body in our buffer. Use this
	// SDP info to setup a ReflectorSession, like we would ordinarily

	StrPtrLen sdpData(theRequestBody, *theContentLenP);
	ReflectorSession* theSession = FindOrCreateSession(&theFullPath, inParams->inRTSPRequest, &sdpData);

	//
	// The act of session the sSessionAttr (and not marking the sOutputAttr) in the client
	// session will identify this session as an "incoming data" session.
	(void)QTSS_SetValue(inParams->inClientSession, sSessionAttr, 0, &theSession, sizeof(theSession));
	
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

	// If there alrady  was an RTPSessionOutput attached to this Client Session,
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
	iovec theDescribeVec[2] = { 0 };
	
	Assert(theSession->GetLocalSDP()->Ptr != NULL);
	theDescribeVec[1].iov_base = theSession->GetLocalSDP()->Ptr;
	theDescribeVec[1].iov_len = theSession->GetLocalSDP()->Len;
	(void)QTSS_AppendRTSPHeader(inParams->inRTSPRequest, qtssCacheControlHeader,
								kCacheControlHeader.Ptr, kCacheControlHeader.Len);
	QTSSModuleUtils::SendDescribeResponse(inParams->inRTSPRequest, inParams->inClientSession,
											&theDescribeVec[0], 2, theSession->GetLocalSDP()->Len);	
	return QTSS_NoErr;
}

ReflectorSession* FindOrCreateSession(StrPtrLen* inPath, QTSS_RTSPRequestObject inRequest, StrPtrLen* inData)
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
		SDPSourceInfo* theInfo = NULL;
		
		//
		// If no file data is provided by the caller, read the file data out of the file.
		// If file data is provided, use that as our SDP data
		if (inData == NULL)
			QTSSModuleUtils::ReadEntireFile(inPath->Ptr, &theFileData);
		else
			theFileData = *inData;
			
		if (theFileData.Len > 0)
			theInfo = NEW SDPSourceInfo(theFileData.Ptr, theFileData.Len);
		else
			return NULL;
			
		if (!theInfo->IsReflectable())
		{
			delete theInfo;
			return NULL;
		}
			
		//create a reflector session, and bind the sockets
		theSession = NEW ReflectorSession(inPath);
		QTSS_Error theErr = theSession->SetupReflectorSession(theInfo, inRequest);
		if (theErr != QTSS_NoErr)
		{
			delete theSession;
			return NULL;
		}
		
		//put the session's ID into the session map.
		theErr = sSessionMap->Register(theSession->GetRef());
		Assert(theErr == QTSS_NoErr);

		//unless we do this, the refcount won't increment (and we'll delete the session prematurely
		OSRef* debug = sSessionMap->Resolve(inPath);
		Assert(debug == theSession->GetRef());
	}
	else
		theSession = (ReflectorSession*)theSessionRef->GetObject();
			
		Assert(theSession != NULL);
	return theSession;
}


QTSS_Error DoSetup(QTSS_StandardRTSP_Params* inParams)
{
	ReflectorSession* theSession = NULL;
	
	// Check to see if we have a RTPSessionOutput for this Client Session. If we don't,
	// we should make one
	RTPSessionOutput** theOutput = NULL;
	UInt32 theLen = 0;
	QTSS_Error theErr = QTSS_GetValuePtr(inParams->inClientSession, sOutputAttr, 0, (void**)&theOutput, &theLen);
	if (theLen != sizeof(RTPSessionOutput*))
	{
		//
		// This may be an incoming data session. If that's the case, there will be a Reflector
		// Session in the ClientSession
		theLen = sizeof(theSession);
		theErr = QTSS_GetValue(inParams->inClientSession, sSessionAttr, 0, &theSession, &theLen);
		
		if (theErr != QTSS_NoErr)
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
	}
	else
		theSession = (*theOutput)->GetReflectorSession();

	//unless there is a digit at the end of this path (representing trackID), don't
	//even bother with the request
	StrPtrLen theDigit;
	(void)QTSS_GetValuePtr(inParams->inRTSPRequest, qtssRTSPReqFileDigit, 0, (void**)&theDigit.Ptr, &theDigit.Len);
	if (theDigit.Len == 0)
		return QTSSModuleUtils::SendErrorResponse(inParams->inRTSPRequest, qtssClientBadRequest,
													sExpectedDigitFilenameErr);
	UInt32 theTrackID = ::strtol(theDigit.Ptr, NULL, 10);
	
	//
	// If this is an incoming data session, skip everything having to do with setting up a new
	// RTP Stream. All we want to do is associate the right channel numbers...
	if (theOutput == NULL)
	{
		
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
	// Tell the session what the bitrate of this reflection is. This is nice for logging,
	// it also allows the server to scale the TCP buffer size appropriately if we are
	// interleaving the data over TCP. This must be set before calling QTSS_Play so the
	// server can use it from within QTSS_Play
	UInt32 bitsPerSecond =	inSession->GetBitRate();
	(void)QTSS_SetValue(inParams->inClientSession, qtssCliSesMovieAverageBitRate, 0, &bitsPerSecond, sizeof(bitsPerSecond));

	//Server shouldn't send RTCP (reflector does it), server shouldn't write rtp header either
	QTSS_Error theErr = QTSS_Play(inParams->inClientSession, inParams->inRTSPRequest, 0);
	if (theErr != QTSS_NoErr)
		return theErr;
	
	//and send a standard play response
	(void)QTSS_SendStandardRTSPResponse(inParams->inRTSPRequest, inParams->inClientSession, 0);
	return QTSS_NoErr;
}

QTSS_Error DestroySession(QTSS_ClientSessionClosing_Params* inParams)
{
	RTPSessionOutput** theOutput = NULL;
	UInt32 theLen = 0;
	QTSS_Error theErr = QTSS_GetValuePtr(inParams->inClientSession, sOutputAttr, 0, (void**)&theOutput, &theLen);
	if ((theErr != QTSS_NoErr) || (theLen != sizeof(RTPSessionOutput*)) || (theOutput == NULL))
		return QTSS_RequestFailed;
	
	RemoveOutput(*theOutput, (*theOutput)->GetReflectorSession());
	return QTSS_NoErr;
}

void RemoveOutput(ReflectorOutput* inOutput, ReflectorSession* inSession)
{
	// This function removes the output from the ReflectorSession, then
	// checks to see if the session should go away. If it should, this deletes it
	inSession->RemoveOutput(inOutput);

	//check if the ReflectorSession should be deleted
	//(it should if its ref count has dropped to 0)
	OSMutexLocker locker (sSessionMap->GetMutex());
	//decrement the ref count
	sSessionMap->Release(inSession->GetRef());
	if (inSession->GetRef()->GetRefCount() == 0)
	{
		sSessionMap->UnRegister(inSession->GetRef());
		delete inSession;
	}	
	delete inOutput;
}

