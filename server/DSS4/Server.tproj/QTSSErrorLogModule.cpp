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
	File:		QTSSErrorLogModule.cpp

	Contains:	Implementation of object defined in .h file.

	

*/

#include <string.h>
#include "QTSSErrorLogModule.h"
#include "QTSSMessages.h"
#include "QTSSRollingLog.h"
#include "QTSServerInterface.h"
#include "QTSSExpirationDate.h"
#include "OSMemory.h"
#include "OS.h"


// STATIC FUNCTIONS

// The dispatch function for this module
static QTSS_Error 	QTSSErrorLogModuleDispatch(QTSS_Role inRole, QTSS_RoleParamPtr inParamBlock);

// A service routine allowing other modules to roll the log
static QTSS_Error 	RollErrorLog(QTSS_ServiceFunctionArgsPtr inArgs);

static QTSS_Error 	Register(QTSS_Register_Params* inParams);
static QTSS_Error	Shutdown();
		
static QTSS_Error	LogError(QTSS_RoleParamPtr inParamBlock);
static void			CheckErrorLogState();

static QTSS_Error	StateChange(QTSS_StateChange_Params* stateChangeParams);
static void			WriteStartupMessage();
static void			WriteShutdownMessage();

// QTSSERRORLOG CLASS DEFINITION

class QTSSErrorLog : public QTSSRollingLog
{
	public:
	
		QTSSErrorLog() : QTSSRollingLog() {}
		virtual ~QTSSErrorLog() {}
	
		virtual char* GetLogName() 	{ return QTSServerInterface::GetServer()->GetPrefs()->GetErrorLogName();}
		
		virtual char* GetLogDir() 	{ return QTSServerInterface::GetServer()->GetPrefs()->GetErrorLogDir();}
		
		virtual UInt32 GetRollIntervalInDays()	{ return QTSServerInterface::GetServer()->GetPrefs()->GetErrorRollIntervalInDays();}
		
		virtual UInt32 GetMaxLogBytes()	{ return QTSServerInterface::GetServer()->GetPrefs()->GetMaxErrorLogBytes();}
	
};

// STATIC DATA

static OSMutex*			sLogMutex = NULL;//Log module isn't reentrant
static QTSSErrorLog*	sErrorLog = NULL;
static char				sLastErrorString[1024] = "";
static int				sDupErrorStringCount = 0;
static Bool16			sStartedUp = false;


// FUNCTION IMPLEMENTATIONS

QTSS_Error QTSSErrorLogModule_Main(void* inPrivateArgs)
{
	return _stublibrary_main(inPrivateArgs, QTSSErrorLogModuleDispatch);
}


QTSS_Error QTSSErrorLogModuleDispatch(QTSS_Role inRole, QTSS_RoleParamPtr inParamBlock)
{
	switch (inRole)
	{
		case QTSS_Register_Role:
			return Register(&inParamBlock->regParams);
		case QTSS_StateChange_Role:
			return StateChange(&inParamBlock->stateChangeParams);
		case QTSS_ErrorLog_Role:
			return LogError(inParamBlock);
		case QTSS_Shutdown_Role:
			return Shutdown();
	}
	return QTSS_NoErr;
}


// ROLE METHODS

QTSS_Error Register(QTSS_Register_Params* inParams)
{
	sLogMutex = NEW OSMutex();
	
	// Do role & service setup
	
	(void)QTSS_AddRole(QTSS_ErrorLog_Role);
	(void)QTSS_AddRole(QTSS_Shutdown_Role);
	(void)QTSS_AddRole(QTSS_StateChange_Role);
	
	(void)QTSS_AddService("RollErrorLog", &RollErrorLog);
	
	// Unlike most modules, all initialization for this module happens in
	// the register role. This is so that this error log can begin logging
	// errors ASAP.
	
	CheckErrorLogState();
	WriteStartupMessage();
	
	// Tell the server our name!
	static char* sModuleName = "QTSSErrorLogModule";
	::strcpy(inParams->outModuleName, sModuleName);

	return QTSS_NoErr;
}

QTSS_Error Shutdown()
{
	WriteShutdownMessage();
	return QTSS_NoErr;
}

QTSS_Error StateChange(QTSS_StateChange_Params* stateChangeParams)
{
	if (stateChangeParams->inNewState == qtssIdleState)
	{
		WriteShutdownMessage();
	}
	else if (stateChangeParams->inNewState == qtssRunningState)
	{
		// Always force our preferences to be reread when we change
		// the server's state back to the start -- [sfu]	
		QTSS_ServiceID id;
		(void) QTSS_IDForService(QTSS_REREAD_PREFS_SERVICE, &id);			
		(void) QTSS_DoService(id, NULL);
		WriteStartupMessage();
	}
	
	return QTSS_NoErr;
}


QTSS_Error LogError(QTSS_RoleParamPtr inParamBlock)
{
	Assert(NULL != inParamBlock->errorParams.inBuffer);
	if (inParamBlock->errorParams.inBuffer == NULL)
		return QTSS_NoErr;
		
	QTSServerPrefs* thePrefs = QTSServerInterface::GetServer()->GetPrefs();
		
	OSMutexLocker locker(sLogMutex);
	if (thePrefs->GetErrorLogVerbosity() >= inParamBlock->errorParams.inVerbosity)
	{
		
		//is this error message the same as the last one we received?
		if ( ::strcmp(inParamBlock->errorParams.inBuffer, sLastErrorString) == 0 )
		{	//yes?  increment count and bail if it's not the first time we've seen this message (otherwise fall thourhg and write it to the log)
			sDupErrorStringCount++;
			return QTSS_NoErr;
		}
		else 
		{
			//we have a new error message, write a "previous line" message before writing the new log entry
			if ( sDupErrorStringCount > 1 )
			{
			/***  clean this up - lots of duplicate code ***/
			
				//The error logger is the bottleneck for any and all messages printed by the server.
				//For debugging purposes, these messages can be printed to stdout as well.
				if (thePrefs->IsScreenLoggingEnabled())
					::printf("--last message repeated %d times\n", sDupErrorStringCount);
			
				CheckErrorLogState();
				
				if (sErrorLog == NULL)
					return QTSS_NoErr;
					
				//timestamp the error
				char theDateBuffer[QTSSRollingLog::kMaxDateBufferSizeInBytes];
				Bool16 result = QTSSRollingLog::FormatDate(theDateBuffer, false);
				//for now, just ignore the error.
				if (!result)
					theDateBuffer[0] = '\0';
		
				Assert(strlen(theDateBuffer) + ::strlen(inParamBlock->errorParams.inBuffer) < 1024);
				char tempBuffer[1024];
				::sprintf(tempBuffer, "%s: --last message repeated %d times\n", theDateBuffer, sDupErrorStringCount);
				
				sErrorLog->WriteToLog(tempBuffer, kAllowLogToRoll);
	
				sDupErrorStringCount = 0;
			}
			
			::strcpy(sLastErrorString, inParamBlock->errorParams.inBuffer);
		
		}

		//The error logger is the bottleneck for any and all messages printed by the server.
		//For debugging purposes, these messages can be printed to stdout as well.
		if (thePrefs->IsScreenLoggingEnabled())
			::printf("%s\n", inParamBlock->errorParams.inBuffer);
		
		CheckErrorLogState();
		
		if (sErrorLog == NULL)
			return QTSS_NoErr;
			
		//timestamp the error
		char theDateBuffer[QTSSRollingLog::kMaxDateBufferSizeInBytes];
		Bool16 result = QTSSRollingLog::FormatDate(theDateBuffer, false);
		//for now, just ignore the error.
		if (!result)
			theDateBuffer[0] = '\0';

		Assert(strlen(theDateBuffer) + ::strlen(inParamBlock->errorParams.inBuffer) < 1024);
		char tempBuffer[1024];
		::sprintf(tempBuffer, "%s: %s\n", theDateBuffer, inParamBlock->errorParams.inBuffer);
		
		sErrorLog->WriteToLog(tempBuffer, kAllowLogToRoll);
	}
	return QTSS_NoErr;
}


void CheckErrorLogState()
{
	//this function makes sure the logging state is in synch with the preferences.
	//extern variable declared in QTSSPreferences.h

	QTSServerPrefs* thePrefs = QTSServerInterface::GetServer()->GetPrefs();

	//check error log.
	if ((NULL == sErrorLog) && (thePrefs->IsErrorLogEnabled()))
	{
		sErrorLog = NEW QTSSErrorLog();
		sErrorLog->EnableLog();
	}

	if ((NULL != sErrorLog) && (!thePrefs->IsErrorLogEnabled()))
	{
		sErrorLog->Delete(); //sErrorLog is a task object, so don't delete it directly
		sErrorLog = NULL;
	}
}

// SERVICE ROUTINES

QTSS_Error RollErrorLog(QTSS_ServiceFunctionArgsPtr /*inArgs*/)
{
	OSMutexLocker locker(sLogMutex);
	if (sErrorLog != NULL)
		sErrorLog->RollLog();
	return QTSS_NoErr;
}

void	WriteStartupMessage()
{
	if (sStartedUp)
		return;
		
	sStartedUp = true;
	
	//format a date for the startup time
	char theDateBuffer[QTSSRollingLog::kMaxDateBufferSizeInBytes];
	Bool16 result = QTSSRollingLog::FormatDate(theDateBuffer, false);
	
	char tempBuffer[1024];
	if (result)
		::sprintf(tempBuffer, "# Streaming STARTUP %s\n", theDateBuffer);
		
	// log startup message to error log as well.
	if ((result) && (sErrorLog != NULL))
		sErrorLog->WriteToLog(tempBuffer, kAllowLogToRoll);
	
	//write the expire date to the log
	if ( QTSSExpirationDate::WillSoftwareExpire() && sErrorLog != NULL )
	{
		QTSSExpirationDate::sPrintExpirationDate(tempBuffer);
		sErrorLog->WriteToLog(tempBuffer, kAllowLogToRoll);
	}
}

void	WriteShutdownMessage()
{
	if (!sStartedUp)
		return;
		
	sStartedUp = false;
	
	//log shutdown message
	//format a date for the shutdown time
	char theDateBuffer[QTSSRollingLog::kMaxDateBufferSizeInBytes];
	Bool16 result = QTSSRollingLog::FormatDate(theDateBuffer, false);
	
	char tempBuffer[1024];
	if (result)
		::sprintf(tempBuffer, "# Streaming SHUTDOWN %s\n", theDateBuffer);

	if ( result && sErrorLog != NULL )
		sErrorLog->WriteToLog(tempBuffer, kAllowLogToRoll);
}

