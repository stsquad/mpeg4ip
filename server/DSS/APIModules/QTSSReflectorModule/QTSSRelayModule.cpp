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

#include "QTSSRelayModule.h"
#include "QTSSModuleUtils.h"
#include "ReflectorSession.h"
#include "ReflectorStream.h"
#include "OSArrayObjectDeleter.h"
#include "QTSS_Private.h"

#include "OSMemory.h"
#include "OSRef.h"
#include "IdleTask.h"
#include "Task.h"
#include "OS.h"
#include "Socket.h"
#include "SocketUtils.h"
#include "RelayPrefsSource.h"
#include "OSArrayObjectDeleter.h"

//ReflectorOutput objects
#include "RTPSessionOutput.h"
#include "RelayOutput.h"

//SourceInfo objects
#include "SDPSourceInfo.h"
#include "RelaySDPSourceInfo.h"
#include "RCFSourceInfo.h"
#include "RTSPSourceInfo.h"


#if DEBUG
#define REFLECTOR_MODULE_DEBUGGING 0
#else
#define REFLECTOR_MODULE_DEBUGGING 0
#endif

// STATIC DATA

static char* 				sRelayPrefs		 			= NULL;
static char* 				sRelayStatsURL	 			= NULL;
static StrPtrLen			sRequestHeader;
static QTSS_ModulePrefsObject sPrefs					= NULL;

#ifndef __Win32__
static char*	sDefaultRelayPrefs		 		= "/etc/streaming/streamingrelay.conf";
#else
static char*	sDefaultRelayPrefs		 		= "c:\\Program Files\\Darwin Streaming Server\\streamingrelay.cfg";
#endif


static OSQueue* 	sSessionQueue = NULL;

// Important strings
static StrPtrLen	sSDPSuffix(".sdp");
static StrPtrLen	sRCFSuffix(".rcf");
static StrPtrLen	sIncludeStr("Include");
static StrPtrLen	sRelaySourceStr("relay_source");
static StrPtrLen	sRTSPSourceStr("rtsp_source");

// This struct is used when Rereading Relay Prefs
struct SourceInfoQueueElem
{
	SourceInfoQueueElem(SourceInfo* inInfo, Bool16 inRTSPInfo) :fElem(this), fSourceInfo(inInfo),
																fIsRTSPSourceInfo(inRTSPInfo),
																fShouldDelete(true) {}
	~SourceInfoQueueElem() {}
	
	OSQueueElem fElem;
	SourceInfo* fSourceInfo;
	Bool16		fIsRTSPSourceInfo;
	Bool16		fShouldDelete;
};


// FUNCTION PROTOTYPES

static QTSS_Error QTSSRelayModuleDispatch(QTSS_Role inRole, QTSS_RoleParamPtr inParams);
static QTSS_Error 	Register(QTSS_Register_Params* inParams);
static QTSS_Error Initialize(QTSS_Initialize_Params* inParams);
static QTSS_Error Shutdown();
static QTSS_Error RereadPrefs();
static ReflectorSession* FindOrCreateSession(SourceInfoQueueElem* inElem);
static void RemoveOutput(ReflectorOutput* inOutput, ReflectorSession* inSession);

static QTSS_Error Filter(QTSS_StandardRTSP_Params* inParams);
static void FindReflectorSessions(OSQueue* inSessionQueue);
static void RereadRelayPrefs();
static void FindRelayConfigFiles(OSQueue* inQueue);
static void ClearRelayConfigFiles(OSQueue* inQueue);
static void FindSourceInfos(OSQueue* inFileQueue, OSQueue* inSessionQueue);
static void ClearSourceInfos(OSQueue* inQueue);
static void EnQueuePath(OSQueue* inQueue, char* inFilePath);


// FUNCTION IMPLEMENTATIONS
#pragma mark __QTSS_RELAY_MODULE__

QTSS_Error QTSSRelayModule_Main(void* inPrivateArgs)
{
	return _stublibrary_main(inPrivateArgs, QTSSRelayModuleDispatch);
}


QTSS_Error 	QTSSRelayModuleDispatch(QTSS_Role inRole, QTSS_RoleParamPtr inParams)
{
	switch (inRole)
	{
		case QTSS_Register_Role:
			return Register(&inParams->regParams);
		case QTSS_Initialize_Role:
			return Initialize(&inParams->initParams);
		case QTSS_RereadPrefs_Role:
			return RereadPrefs();
		case QTSS_RTSPFilter_Role:
			return Filter(&inParams->rtspRequestParams);
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
	(void)QTSS_AddRole(QTSS_RTSPFilter_Role);
	(void)QTSS_AddRole(QTSS_RereadPrefs_Role);
	
	// Reflector session needs to setup some parameters too.
	ReflectorStream::Register();

	// Tell the server our name!
	static char* sModuleName = "QTSSRelayModule";
	::strcpy(inParams->outModuleName, sModuleName);

	return QTSS_NoErr;
}


QTSS_Error Initialize(QTSS_Initialize_Params* inParams)
{
	// Setup module utils
	QTSSModuleUtils::Initialize(inParams->inMessages, inParams->inServer, inParams->inErrorLogStream);
	sSessionQueue = NEW OSQueue();
	sPrefs = QTSSModuleUtils::GetModulePrefsObject(inParams->inModule);

	// Call helper class initializers
	ReflectorSession::Initialize();
		
#if QTSS_RELAY_EXTERNAL_MODULE
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

	//
	// Instead of passing our own module prefs object, as one might expect,
	// here we pass in the QTSSReflectorModule's, because the prefs that
	// apply to ReflectorStream are stored in that module's prefs
	StrPtrLen theReflectorModule("QTSSReflectorModule");
	QTSS_ModulePrefsObject theReflectorPrefs =
		QTSSModuleUtils::GetModulePrefsObject(QTSSModuleUtils::GetModuleObjectByName(theReflectorModule));

	// Call helper class initializers
	ReflectorStream::Initialize(theReflectorPrefs);
	RereadPrefs();

	return QTSS_NoErr;
}

QTSS_Error Shutdown()
{
#if QTSS_REFLECTOR_EXTERNAL_MODULE
	TaskThreadPool::RemoveThreads();
#endif
	return QTSS_NoErr;
}

QTSS_Error RereadPrefs()
{
	delete [] sRelayPrefs;
	delete [] sRelayStatsURL;
	
	sRelayPrefs = QTSSModuleUtils::GetStringPref(sPrefs, "relay_prefs_file", sDefaultRelayPrefs);
	sRelayStatsURL = QTSSModuleUtils::GetStringPref(sPrefs, "relay_stats_url", "");

	RereadRelayPrefs();
	return QTSS_NoErr;
}


ReflectorSession* FindOrCreateSession(SourceInfoQueueElem* inElem)
{
	ReflectorSession* theSession = NULL;

	// Check to see if this ReflectorSession already exists on the queue
	for (OSQueueIter theIter(sSessionQueue); !theIter.IsDone(); theIter.Next())
	{
		theSession = (ReflectorSession*)theIter.GetCurrent()->GetEnclosingObject();
		if (theSession->Equal(inElem->fSourceInfo))
			return theSession;
	}
	
	// There is no matching session, so create a new one
	theSession = NEW ReflectorSession(NULL, inElem->fSourceInfo);
	QTSS_Error theErr = theSession->SetupReflectorSession(inElem->fSourceInfo, NULL);
	inElem->fShouldDelete = false; // SourceInfo will be deleted by the ReflectorSession
	if (theErr != QTSS_NoErr)
	{
		delete theSession;
		return NULL;
	}
	else if (inElem->fIsRTSPSourceInfo)
	{
		// if this is an RTSPSourceInfo, finish setting it up
		theErr = ((RTSPSourceInfo*)inElem->fSourceInfo)->SetupAndPlay();
		if (theErr != QTSS_NoErr)
		{
			delete theSession;
			return NULL;
		}
	}
	
	// Format SourceInfo HTML for the stats web page
	if (inElem->fIsRTSPSourceInfo)
		theSession->FormatHTML(((RTSPSourceInfo*)inElem->fSourceInfo)->GetRTSPClient()->GetURL());
	else
		theSession->FormatHTML(NULL);
	
	sSessionQueue->EnQueue(theSession->GetQueueElem());
	return theSession;	
}


void RemoveOutput(ReflectorOutput* inOutput, ReflectorSession* inSession)
{
	// This function removes the output from the ReflectorSession, then
	// checks to see if the session should go away. If it should, this deletes it
	inSession->RemoveOutput(inOutput);
	
	if (inSession->GetNumOutputs() == 0)
	{
		sSessionQueue->Remove(inSession->GetQueueElem());
		delete inSession;
	}
	delete inOutput;
}

QTSS_Error Filter(QTSS_StandardRTSP_Params* inParams)
{
	UInt32 theLen = 0;
	char* theFullRequest = NULL;
	(void)QTSS_GetValuePtr(inParams->inRTSPRequest, qtssRTSPReqFullRequest, 0, (void**)&theFullRequest, &theLen);
	
	OSMutexLocker locker(RelayOutput::GetQueueMutex());

	// Check to see if this is a request we should handle
	if ((sRequestHeader.Ptr == NULL) || (sRequestHeader.Len == 0))
		return QTSS_NoErr;
	if ((theFullRequest == NULL) || (theLen < sRequestHeader.Len))
		return QTSS_NoErr;
	if (::memcmp(theFullRequest, sRequestHeader.Ptr, sRequestHeader.Len) != 0)
		return QTSS_NoErr;

	// Keep-alive should be off!
	Bool16 theFalse = false;
	(void)QTSS_SetValue(inParams->inRTSPRequest, qtssRTSPReqRespKeepAlive, 0, &theFalse, sizeof(theFalse));

	static StrPtrLen	sResponseHeader("HTTP/1.0 200 OK\r\nServer: QTSSReflectorModule/2.0\r\nConnection: Close\r\nContent-Type: text/html\r\n\r\n<HTML><TITLE>Relay Stats</TITLE><BODY>");
	(void)QTSS_Write(inParams->inRTSPRequest, sResponseHeader.Ptr, sResponseHeader.Len, NULL, 0);
	
	// First build up a queue of all ReflectorSessions with RelayOutputs. This way,
	// we can present a list that's sorted.
	OSQueue theSessionQueue;
	FindReflectorSessions(&theSessionQueue);
	
	static StrPtrLen	sNoRelays("<H2>There are no currently active relays</H2>");
	if (theSessionQueue.GetLength() == 0)
		(void)QTSS_Write(inParams->inRTSPRequest, sNoRelays.Ptr, sNoRelays.Len, NULL, 0);
	
	// Now go through our ReflectorSessionQueue, writing the info for each ReflectorSession,
	// and all the outputs associated with that session
	for (OSQueueIter iter(&theSessionQueue); !iter.IsDone(); iter.Next())
	{
		ReflectorSession* theSession = (ReflectorSession*)iter.GetCurrent()->GetEnclosingObject();
		(void)QTSS_Write(inParams->inRTSPRequest, theSession->GetSourceInfoHTML()->Ptr, theSession->GetSourceInfoHTML()->Len, NULL, 0);

		for (OSQueueIter iter2(RelayOutput::GetOutputQueue()); !iter2.IsDone(); iter2.Next())
		{
			RelayOutput* theOutput = (RelayOutput*)iter2.GetCurrent()->GetEnclosingObject();
			if (theSession == theOutput->GetReflectorSession())
			{
				(void)QTSS_Write(inParams->inRTSPRequest, theOutput->GetOutputInfoHTML()->Ptr, theOutput->GetOutputInfoHTML()->Len, NULL, 0);

				// Write current stats for this output
				char theStatsBuf[1024];
				::sprintf(theStatsBuf, "Current stats for this relay: %lu packets per second. %lu bits per second. %qu packets since it started. %qu bits since it started<P>", theOutput->GetCurPacketsPerSecond(), theOutput->GetCurBitsPerSecond(), theOutput->GetTotalPacketsSent(), theOutput->GetTotalBytesSent());
				(void)QTSS_Write(inParams->inRTSPRequest, &theStatsBuf[0], ::strlen(theStatsBuf), NULL, 0);
			}
		}
	}
	static StrPtrLen 	sResponseEnd("</BODY></HTML>");
	(void)QTSS_Write(inParams->inRTSPRequest, sResponseEnd.Ptr, sResponseEnd.Len, NULL, 0);
	
	for (OSQueueIter iter3(&theSessionQueue); !iter3.IsDone(); )
	{
		// Cleanup the memory we had allocated in FindReflectorSessions
		OSQueueElem* theElem = iter3.GetCurrent();
		iter3.Next();
		theSessionQueue.Remove(theElem);
		delete theElem;
	}
	return QTSS_NoErr;
}

void FindReflectorSessions(OSQueue* inSessionQueue)
{
	for (OSQueueIter theIter(RelayOutput::GetOutputQueue()); !theIter.IsDone(); theIter.Next())
	{
		RelayOutput* theOutput = (RelayOutput*)theIter.GetCurrent()->GetEnclosingObject();
		Bool16 found = false;
		
		// Check to see if we've already seen this ReflectorSession
		for (OSQueueIter theIter2(inSessionQueue); !theIter2.IsDone(); theIter2.Next())
		{
			if (theOutput->GetReflectorSession() == theIter2.GetCurrent()->GetEnclosingObject())
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			// We haven't seen this one yet, so put it on the queue.
			OSQueueElem* theElem = NEW OSQueueElem(theOutput->GetReflectorSession());
			inSessionQueue->EnQueue(theElem);
		}
	}
}

void RereadRelayPrefs()
{
	// Find all the Relay configuration files
	OSQueue fileQueue;
	FindRelayConfigFiles(&fileQueue);
	
	// Now that we have the config files, go through each one,
	// constructing a SourceInfo object for each relayable thingy
	OSQueue sourceInfoQueue;
	FindSourceInfos(&fileQueue, &sourceInfoQueue);
	
	// Ok, we have all our SourceInfo objects. Now, lets alter the list of Relay
	// outputs to match
	
	// Go through the queue of Relay outputs, removing the source Infos that
	// are already going
	OSMutexLocker locker(RelayOutput::GetQueueMutex());

	// We must reread the relay stats URL here, because we have the RelayOutput
	// mutex so we know that Filter can't be filtering now
	delete [] sRequestHeader.Ptr;
	sRequestHeader.Ptr = NULL;
	sRequestHeader.Len = 0;

	if ((sRelayStatsURL != NULL) && (::strlen(sRelayStatsURL) > 0))
	{
		// sRequestHeader line should look like: GET /sRelayStatsURL HTTP
		sRequestHeader.Ptr = NEW char[::strlen(sRelayStatsURL) + 15];
		::strcpy(sRequestHeader.Ptr, "GET /");
		::strcat(sRequestHeader.Ptr, sRelayStatsURL);
		::strcat(sRequestHeader.Ptr, " HTTP");
		sRequestHeader.Len = ::strlen(sRequestHeader.Ptr);
	}

	for (OSQueueIter iter(RelayOutput::GetOutputQueue()); !iter.IsDone(); )
	{
		RelayOutput* theOutput = (RelayOutput*)iter.GetCurrent()->GetEnclosingObject();
		
		Bool16 stillActive = false;
		
		// Check to see if this RelayOutput matches one of the streams in the
		// SourceInfo queue. If it does, this has 2 implications:
		//
		// 1) The stream in the SourceInfo that matches should NOT be setup as a
		// new RelayOutput, because it already exists. (Equal will set a flag in
		// the StreamInfo object saying as much)
		//
		// 2) This particular RelayOutput should remain (not be deleted).
		for (OSQueueIter iter2(&sourceInfoQueue); !iter2.IsDone(); iter2.Next())
		{
			SourceInfo* theInfo =
				((SourceInfoQueueElem*)iter2.GetCurrent()->GetEnclosingObject())->fSourceInfo;
			if (theOutput->Equal(theInfo))
			{
				stillActive = true;
				break;
			}
		}
		iter.Next();	

		// This relay output is no longer on our list
		if (!stillActive)
			RemoveOutput(theOutput, theOutput->GetReflectorSession());
	}
	
	// We've pruned the list of RelayOutputs of all outputs that are no longer
	// going on. Now, all that is left to do is to create any new outputs (and sessions)
	// that are just starting up
	
	for (OSQueueIter iter3(&sourceInfoQueue); !iter3.IsDone(); iter3.Next())
	{
		SourceInfoQueueElem* theElem = (SourceInfoQueueElem*)iter3.GetCurrent()->GetEnclosingObject();
		if (theElem->fSourceInfo->GetNumNewOutputs() == 0) 	// Because we've already checked for Outputs
			continue;							// That already exist, we know which ones are new
			
		ReflectorSession* theSession = FindOrCreateSession(theElem);
		if (theSession == NULL)
			continue;// if something very bad happened while trying to bind the source ports
					// (perhaps another process already has them or whatnot

		for (UInt32 x = 0; x < theElem->fSourceInfo->GetNumOutputs(); x++)
		{
			SourceInfo::OutputInfo* theOutputInfo = theElem->fSourceInfo->GetOutputInfo(x);
			if (theOutputInfo->fAlreadySetup)
				continue;
				
			RelayOutput* theOutput = NEW RelayOutput(theElem->fSourceInfo, x, theSession);
			theOutput->BindSocket();// ALERT!! WE ARE NOT CHECKING ERRORS HERE!!!
			theSession->AddOutput(theOutput);
		}
	}
	
	// We're done with the file queue now, free up the memory
	ClearRelayConfigFiles(&fileQueue);
	ClearSourceInfos(&sourceInfoQueue);
}

void FindRelayConfigFiles(OSQueue* inQueue)
{
	//
	// If there is no relay config file anymore, we can just bail
	Assert(sRelayPrefs != NULL);
	if (::strlen(sRelayPrefs) == 0)
		return;
		
	// This function goes through all the recursion in the relay config
	// files, and finds all of them. Each file path is put into a queue
	
	char* currentPath = sRelayPrefs;
	EnQueuePath(inQueue, sRelayPrefs);
	for (OSQueueIter theIter(inQueue); !theIter.IsDone(); )
	{
		// When we have parsed all the files on the list (and found no
		// additional files), thn we are done
		Assert(currentPath != NULL);
		RelayPrefsSource theParser(true);//Allow duplicate key names in the file
		(void)theParser.InitFromConfigFile(currentPath);
		
		for (UInt32 x = 0; x < theParser.GetNumKeys(); x++)
		{
			StrPtrLen theKeyStr(theParser.GetKeyAtIndex(x));
			StrPtrLen theBodyStr(theParser.GetValueAtIndex(x));
			
			if ((theKeyStr.EqualIgnoreCase(sIncludeStr.Ptr, sIncludeStr.Len)) &&
				(theBodyStr.Len > sRCFSuffix.Len))
			{
				if (::memcmp(theBodyStr.Ptr + (theBodyStr.Len - sRCFSuffix.Len), sRCFSuffix.Ptr, sRCFSuffix.Len) == 0)
				{
					Bool16 found = false;
					
					// Make sure this path is not already in the list. If this happens,
					// and we don't detect it, we'd be looping here forever.
					for (OSQueueIter theIter2(inQueue); !theIter2.IsDone(); theIter2.Next())
					{
						if (theBodyStr.Equal((char*)theIter2.GetCurrent()->GetEnclosingObject()))
						{
							found = true;
							break;
						}
					}
					// If this path is not currently on the list, add it
					if (!found)
						EnQueuePath(inQueue, theBodyStr.Ptr);
				}
			}
		}
		
		// Move onto the next path. There maybe none left, in which case
		// we should break out of the loop above (currentPath just stays NULL)
		currentPath = NULL;
		theIter.Next();
		OSQueueElem* theNextFile = theIter.GetCurrent();
		if (theNextFile != NULL)
			currentPath = (char*)theNextFile->GetEnclosingObject();
	}
}

void ClearRelayConfigFiles(OSQueue* inQueue)
{
	for (OSQueueIter theIter(inQueue); !theIter.IsDone(); )
	{
		// Get the current queue element, and make sure to move onto the
		// next one, as the current one is going away.
		OSQueueElem* theElem = theIter.GetCurrent();
		theIter.Next();

		inQueue->Remove(theElem);
		char* thePath = (char*)theElem->GetEnclosingObject();
		delete [] thePath;
		delete theElem;
	}
}

void FindSourceInfos(OSQueue* inFileQueue, OSQueue* inSessionQueue)
{
	SourceInfoQueueElem* theElem = NULL;

	for (OSQueueIter theFileIter(inFileQueue); !theFileIter.IsDone(); theFileIter.Next())
	{
		RelayPrefsSource theParser(true);
		(void)theParser.InitFromConfigFile((char*)theFileIter.GetCurrent()->GetEnclosingObject());
		
		// For this pass over the file, order matters, because groups of lines go together.
		// RelayPrefsSource loads in all the prefs lines in reverse order, and we want to
		// iterate in top to bottom order, so that's why this loop is in reverse
		for (SInt32 x = theParser.GetNumKeys() - 1; x >= 0; x--)
		{
			StrPtrLen theKeyStr(theParser.GetKeyAtIndex(x));
			StrPtrLen theBodyStr(theParser.GetValueAtIndex(x));
			
			// Check to see if this line is an SDP include. If so, put it on our list
			if ((theKeyStr.EqualIgnoreCase(sIncludeStr.Ptr, sIncludeStr.Len)) &&
				(theBodyStr.Len > sSDPSuffix.Len))
			{
				if (::memcmp(theBodyStr.Ptr + (theBodyStr.Len - sSDPSuffix.Len), sSDPSuffix.Ptr, sSDPSuffix.Len) == 0)
				{
					StrPtrLen theFileData;
					(void)QTSSModuleUtils::ReadEntireFile(theBodyStr.Ptr, &theFileData);
					if (theFileData.Len > 0) // There is a Relay SDP file!
					{
						RelaySDPSourceInfo* theInfo = NEW RelaySDPSourceInfo(&theFileData);

						// Only keep this sdp file around if there are input streams &
						// output streams described in it.
						if ((theInfo->GetNumStreams() == 0) || (theInfo->GetNumOutputs() == 0))
							delete theInfo;
						else
						{
							theElem = NEW SourceInfoQueueElem(theInfo, false);
							inSessionQueue->EnQueue(&theElem->fElem);
						}
						delete [] theFileData.Ptr;// We don't need the file data anymore
					}
				}
			}
			// If the line begins with "relay_source" it is the start of inlined
			// relay configuration information.
			else if (theKeyStr.EqualIgnoreCase(sRelaySourceStr.Ptr, sRelaySourceStr.Len))
			{
				RCFSourceInfo* theRCFInfo = NEW RCFSourceInfo(&theParser, x);
				// Only keep this sdp file around if there are input streams &
				// output streams described in it.
				if ((theRCFInfo->GetNumStreams() == 0) || (theRCFInfo->GetNumOutputs() == 0))
					delete theRCFInfo;
				else
				{
					theElem = NEW SourceInfoQueueElem(theRCFInfo, false);
					inSessionQueue->EnQueue(&theElem->fElem);
				}
			}
			else if (theKeyStr.EqualIgnoreCase(sRTSPSourceStr.Ptr, sRTSPSourceStr.Len))
			{
				// This is an rtsp source, so go through the describe, setup, play connect
				// process before moving on.
				RTSPSourceInfo* theRTSPInfo = NEW RTSPSourceInfo(0);
				QTSS_Error theErr = theRTSPInfo->ParsePrefs(&theParser, x);
				if (theErr == QTSS_NoErr)
					theErr = theRTSPInfo->Describe();
				if (theErr == QTSS_NoErr)
				{
					// We have to do this *after* doing the DESCRIBE because parsing
					// the relay destinations depends on information in the SDP
					theRTSPInfo->ParseRelayDestinations(&theParser, x-1);
					
					if ((theRTSPInfo->GetNumStreams() == 0) || (theRTSPInfo->GetNumOutputs() == 0))
						delete theRTSPInfo;
					else
					{
						theElem = NEW SourceInfoQueueElem(theRTSPInfo, true);
						inSessionQueue->EnQueue(&theElem->fElem);
					}
				}
				else
					delete theRTSPInfo;
			}
		}
	}
}

void ClearSourceInfos(OSQueue* inQueue)
{
	for (OSQueueIter theIter(inQueue); !theIter.IsDone(); )
	{
		// Get the current queue element, and make sure to move onto the
		// next one, as the current one is going away.
		SourceInfoQueueElem* theElem = (SourceInfoQueueElem*)theIter.GetCurrent()->GetEnclosingObject();

		theIter.Next();
	
		inQueue->Remove(&theElem->fElem);
		
		// If we're supposed to delete this SourceInfo, then do so
		if (theElem->fShouldDelete)
			delete theElem->fSourceInfo;
		delete theElem;
	}
}


void EnQueuePath(OSQueue* inQueue, char* inFilePath)
{
	char* filePathCopy = NEW char[::strlen(inFilePath) + 1];
	::strcpy(filePathCopy, inFilePath);
	
	OSQueueElem* newElem = NEW OSQueueElem(filePathCopy);
	inQueue->EnQueue(newElem);
}
