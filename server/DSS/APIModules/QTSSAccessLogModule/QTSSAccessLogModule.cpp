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
	File:		QTSSAccessLogModule.cpp

	Contains:	Implementation of object defined in QTSSAccessLogModule.h
					

*/

#include "QTSSAccessLogModule.h"
#include "QTSSModuleUtils.h"
#include "QTSSRollingLog.h"
#include "OSMutex.h"
#include "MyAssert.h"
#include "OSMemory.h"
#include <time.h>
#include "StringParser.h"
#include "StringFormatter.h"
#include "StringTranslator.h"
#include "StrPtrLen.h"
#include "UserAgentParser.h"

#if __MacOSXServer__
	#include <mach/mach.h>	/* for mach cpu state struct and functions */
#endif

#define TESTUNIXTIME 0

class QTSSAccessLog;

// STATIC DATA

// Default values for preferences
static Bool16	sDefaultLogEnabled			= false;
static char*	sDefaultLogName		 		= "StreamingServer";
#ifdef __MacOSX__
static char*	sDefaultLogDir		 		= "/Library/QuickTimeStreaming/Logs/";
#elif __Win32__
static char*	sDefaultLogDir		 		= "c:\\Program Files\\Darwin Streaming Server\\Logs";
#else
static char*	sDefaultLogDir		 		= "/var/streaming/logs/";
#endif
static UInt32	sDefaultMaxLogBytes 		= 51200000;
static UInt32	sDefaultRollInterval 		= 7;
static char*	sVoidField 					= "-";

// Current values for preferences
static Bool16	sLogEnabled			= false;
static UInt32	sMaxLogBytes 		= 51200000;
static UInt32	sRollInterval 		= 7;

static OSMutex*				sLogMutex 	= NULL;//Log module isn't reentrant
static QTSSAccessLog*		sAccessLog 	= NULL;
static QTSS_ServerObject	sServer 	= NULL;
static QTSS_ModulePrefsObject sPrefs 	= NULL;


// 2 added fields at end c-username sc(realm)
static char* sLogHeader =	"#Software: %s\n"
					"#Version: %s\n"	//%s == version
					"#Date: %s\n"	//%s == date/time
					"#Fields: c-ip date time c-dns cs-uri-stem c-starttime x-duration c-rate c-status c-playerid"
						" c-playerversion c-playerlanguage cs(User-Agent) cs(Referer) c-hostexe c-hostexever c-os"
						" c-osversion c-cpu filelength filesize avgbandwidth protocol transport audiocodec videocodec"
						" channelURL sc-bytes c-bytes s-pkts-sent c-pkts-received c-pkts-lost-client c-pkts-lost-net"
						" c-pkts-lost-cont-net c-resendreqs c-pkts-recovered-ECC c-pkts-recovered-resent c-buffercount"
						" c-totalbuffertime c-quality s-ip s-dns s-totalclients s-cpu-util cs-uri-query c-username sc(Realm) sc-respmsg \n";



//**************************************************
// CLASS DECLARATIONS
//**************************************************

#if __MacOSXServer__
class XCPUStats 
{
	public:
		static void Init();
		static void GetStats( double *dpLoad, long *lpTicks);
		
	#if __MacOSXServer__
		static void GetMachineInfo(machine_info* ioMachineInfo) 
		{		if (!XCPUStats::sInited)
					XCPUStats::Init();
				*ioMachineInfo = sMachineInfo;
		}
	#endif

		
private:
		static Bool16 sInited;
		static int	_CPUCount ;
		static int	_CPUSlots [16] ;
		
		static long _laLastTicks [CPU_STATE_MAX] ;
		static struct machine_info sMachineInfo;
};

Bool16 	XCPUStats::sInited = false;
int		XCPUStats::_CPUCount = 1;
int		XCPUStats::_CPUSlots [16] = {-1};

		long XCPUStats::_laLastTicks[CPU_STATE_MAX];
		struct machine_info XCPUStats::sMachineInfo  = {};
void XCPUStats::Init()
{
    if (xxx_host_info (current_task (), &sMachineInfo) == KERN_SUCCESS) {
        register int		i, j = 0 ;
        struct machine_slot	slot ;

        _CPUCount = 0 ;
        for (i = sMachineInfo.max_cpus ; i-- ; )
            if ((xxx_slot_info (current_task (), i, &slot)
                 == KERN_SUCCESS) && (slot.is_cpu == 1)) {
                _CPUSlots[j++] = i ;
                _CPUCount++ ;
            }
        _CPUSlots[j] = -1 ;
		
	}
	
	XCPUStats::sInited = true;
	
	
	double cpuLoad [3];
	long cpuTicks[CPU_STATE_MAX];
	XCPUStats::GetStats(cpuLoad, cpuTicks);
}

//courtesy of CJ
void XCPUStats::GetStats ( double *dpLoad, long *lpTicks)
{
    register int	i ;
    register long	lTicks ;
    long			laSave [CPU_STATE_MAX];

	if (!XCPUStats::sInited)
		XCPUStats::Init();

    // Get the curent load average.
    if (-1 == getloadavg (dpLoad, 3))
    	dpLoad[0] = dpLoad[1] = dpLoad[2] = 0.0 ;

    // Return if mach says it doesn't have any CPUs.
    if (_CPUSlots[0] == -1) {
        lpTicks[0] = lpTicks[1] = lpTicks[2] = 0 ;
        return ;
    }

    // Accumulate all CPU ticks into one entry.
    laSave[0] = laSave[1] = laSave[2] = 0 ;
    for (i = _CPUCount ; i-- ; ) {
        struct machine_slot	slot ;

        // Get the percentages of CPU usage for each CPU.
        if ((xxx_slot_info (current_task (), _CPUSlots[i], &slot)
             == KERN_SUCCESS) && (slot.is_cpu == 1)) {
            laSave[CPU_STATE_USER] += slot.cpu_ticks[CPU_STATE_USER] ;
            laSave[CPU_STATE_SYSTEM] += slot.cpu_ticks[CPU_STATE_SYSTEM] ;
            laSave[CPU_STATE_IDLE] += slot.cpu_ticks[CPU_STATE_IDLE] ;
        }
    }

    // Get the difference between samples.
    for (lTicks = 0, i = CPU_STATE_MAX ; i-- ; ) {
        lpTicks[i] = laSave[i] - _laLastTicks[i] ;
        _laLastTicks[i] = laSave[i] ;
        lTicks += lpTicks[i] ;
        lpTicks[i] *= 100 ;
    }

    // Scale it to a percentage.
    if (lTicks) {
        lpTicks[CPU_STATE_USER] /= lTicks ;
        lpTicks[CPU_STATE_SYSTEM] /= lTicks ;
        lpTicks[CPU_STATE_IDLE] /= lTicks ;
    }
}
#endif

class QTSSAccessLog : public QTSSRollingLog
{
	public:
	
		QTSSAccessLog() : QTSSRollingLog() {}
		virtual ~QTSSAccessLog() {}
	
		virtual char* GetLogName() { return QTSSModuleUtils::GetStringPref(sPrefs, "request_logfile_name", sDefaultLogName); }
		virtual char* GetLogDir()  { return QTSSModuleUtils::GetStringPref(sPrefs, "request_logfile_dir", sDefaultLogDir); }
		virtual UInt32 GetRollIntervalInDays() 	{ return sRollInterval; }
		virtual UInt32 GetMaxLogBytes()			{ return sMaxLogBytes; }
		virtual time_t WriteLogHeader(FILE *inFile);
	
};

// FUNCTION PROTOTYPES

static QTSS_Error 	QTSSAccessLogModuleDispatch(QTSS_Role inRole, QTSS_RoleParamPtr inParamBlock);
static QTSS_Error 	Register(QTSS_Register_Params* inParams);
static QTSS_Error 	Initialize(QTSS_Initialize_Params* inParams);
static QTSS_Error 	RereadPrefs();
static QTSS_Error 	Shutdown();
static QTSS_Error	PostProcess(QTSS_StandardRTSP_Params* inParams);
static QTSS_Error	ClientSessionClosing(QTSS_ClientSessionClosing_Params* inParams);
static QTSS_Error 	LogRequest(	QTSS_ClientSessionObject inClientSession,
							QTSS_RTSPSessionObject inRTSPSession,QTSS_CliSesClosingReason *inCloseReasonPtr);
static void 			CheckAccessLogState(Bool16 forceEnabled);
static QTSS_Error 	RollAccessLog(QTSS_ServiceFunctionArgsPtr inArgs);
static void 		ReplaceSpaces(StrPtrLen *sourcePtr, StrPtrLen *destPtr, char *replaceStr);


#pragma mark __QTSS_ACCESS_LOG__

// FUNCTION IMPLEMENTATIONS

QTSS_Error QTSSAccessLogModule_Main(void* inPrivateArgs)
{
	return _stublibrary_main(inPrivateArgs, QTSSAccessLogModuleDispatch);
}

QTSS_Error QTSSAccessLogModuleDispatch(QTSS_Role inRole, QTSS_RoleParamPtr inParamBlock)
{
	switch (inRole)
	{
		case QTSS_Register_Role:
			return Register(&inParamBlock->regParams);
		case QTSS_Initialize_Role:
			return Initialize(&inParamBlock->initParams);
		case QTSS_RereadPrefs_Role:
			return RereadPrefs();
		case QTSS_RTSPPostProcessor_Role:
			return PostProcess(&inParamBlock->rtspPostProcessorParams);
		case QTSS_ClientSessionClosing_Role:
			return ClientSessionClosing(&inParamBlock->clientSessionClosingParams);
		case QTSS_Shutdown_Role:
			return Shutdown();
	}
	return QTSS_NoErr;
}


QTSS_Error Register(QTSS_Register_Params* inParams)
{
	sLogMutex = NEW OSMutex();
	
	// Do role & service setup
	
	(void)QTSS_AddRole(QTSS_Initialize_Role);
//	(void)QTSS_AddRole(QTSS_RTSPPostProcessor_Role); // QTSS_ClientSessionClosing_Role is the only role necessary
	(void)QTSS_AddRole(QTSS_ClientSessionClosing_Role);
	(void)QTSS_AddRole(QTSS_RereadPrefs_Role);
	(void)QTSS_AddRole(QTSS_Shutdown_Role);

	(void)QTSS_AddService("RollAccessLog", &RollAccessLog);
	
	// Tell the server our name!
	static char* sModuleName = "QTSSAccessLogModule";
	::strcpy(inParams->outModuleName, sModuleName);

	return QTSS_NoErr;
}


QTSS_Error Initialize(QTSS_Initialize_Params* inParams)
{
	QTSSModuleUtils::Initialize(inParams->inMessages, inParams->inServer, inParams->inErrorLogStream);
	sServer = inParams->inServer;
	sPrefs = QTSSModuleUtils::GetModulePrefsObject(inParams->inModule);
	
	RereadPrefs();
	CheckAccessLogState(false);
		
	//format a date for the startup time
	char theDateBuffer[QTSSRollingLog::kMaxDateBufferSizeInBytes];
	Bool16 result = QTSSRollingLog::FormatDate(theDateBuffer);
	
	char tempBuffer[1024];
	if (result)
		::sprintf(tempBuffer, "#Streaming Server Startup STARTUP %s\n", theDateBuffer);

	// log startup message to error log as well.
	if ((result) && (sAccessLog != NULL))
		sAccessLog->WriteToLog(tempBuffer, !kAllowLogToRoll);
	
#if __MacOSXServer__				
	XCPUStats::Init();
#endif		
	return QTSS_NoErr;
}


QTSS_Error RereadPrefs()
{
	QTSSModuleUtils::GetPref(sPrefs, "request_logging", 	qtssAttrDataTypeBool16,
								&sLogEnabled, &sDefaultLogEnabled, sizeof(sLogEnabled));
	QTSSModuleUtils::GetPref(sPrefs, "request_logfile_size", 	qtssAttrDataTypeUInt32,
								&sMaxLogBytes, &sDefaultMaxLogBytes, sizeof(sMaxLogBytes));
	QTSSModuleUtils::GetPref(sPrefs, "request_logfile_interval", 	qtssAttrDataTypeUInt32,
								&sRollInterval, &sDefaultRollInterval, sizeof(sRollInterval));
	return QTSS_NoErr;
}


QTSS_Error Shutdown()
{
	//log shutdown message
	//format a date for the shutdown time
	char theDateBuffer[QTSSRollingLog::kMaxDateBufferSizeInBytes];
	Bool16 result = QTSSRollingLog::FormatDate(theDateBuffer);
	
	char tempBuffer[1024];
	if (result)
		::sprintf(tempBuffer, "#Streaming Server Shutdown SHUTDOWN %s\n", theDateBuffer);

	if ((result) && (sAccessLog != NULL))
		sAccessLog->WriteToLog(tempBuffer, !kAllowLogToRoll);
		
	return QTSS_NoErr;
}



QTSS_Error PostProcess(QTSS_StandardRTSP_Params* inParams)
{
	QTSS_RTSPMethod* theMethod = NULL;
	UInt32 theLen = 0;
	QTSS_Error theErr = QTSS_NoErr;
	
	if (0) do  // shouldn't be called
	{
		theErr = QTSS_GetValuePtr(inParams->inRTSPRequest, qtssRTSPReqMethod, 0, (void**)&theMethod, &theLen);
		if ((theMethod == NULL) || (theLen != sizeof(QTSS_RTSPMethod)))
		{	
			theErr = QTSS_RequestFailed;
			break;
		}

		QTSS_RTSPStatusCode* theStatus = NULL;
		theErr = QTSS_GetValuePtr(inParams->inRTSPRequest, qtssRTSPReqStatusCode, 0, (void**)&theStatus, &theLen);
		if (theErr != QTSS_NoErr) break;
		
		if ((theStatus == NULL) || (theLen != sizeof(QTSS_RTSPStatusCode)))
		{	theErr = QTSS_RequestFailed;
			break;
		}

		//only log the request if it is a TEARDOWN, or if its an error
		if ((*theMethod != qtssTeardownMethod) && (*theStatus == qtssSuccessOK))
		{
			theErr = QTSS_NoErr;
			break;
		}
		
		theErr = LogRequest(inParams->inClientSession, inParams->inRTSPSession, NULL);
		if (theErr != QTSS_NoErr) break;
	
	} while (false);
	
	return theErr;
}

#if TESTUNIXTIME
void TestUnixTime(time_t theTime, char *ioDateBuffer);
void TestUnixTime(time_t theTime, char *ioDateBuffer)
{
	Assert(NULL != ioDateBuffer);
	
	//use ansi routines for getting the date.
	time_t calendarTime = theTime;
	Assert(-1 != calendarTime);
	if (-1 == calendarTime)
		return;
		
	struct tm* theLocalTime = ::localtime(&calendarTime);
	Assert(NULL != theLocalTime);
	if (NULL == theLocalTime)
		return;
		
	//date needs to look like this for common log format: 29/Sep/1998:11:34:54 -0700
	//this wonderful ANSI routine just does it for you.
	//::strftime(ioDateBuffer, kMaxDateBufferSize, "%d/%b/%Y:%H:%M:%S", theLocalTime);
	::strftime(ioDateBuffer,QTSSRollingLog::kMaxDateBufferSizeInBytes, "%Y-%m-%d %H:%M:%S", theLocalTime);	
	return;
}
#endif


QTSS_Error ClientSessionClosing(QTSS_ClientSessionClosing_Params* inParams)
{
	return LogRequest(inParams->inClientSession, NULL, &inParams->inReason);
}

void ReplaceSpaces(StrPtrLen *sourcePtr, StrPtrLen *destPtr, char *replaceStr)
{

	if ( (NULL != destPtr) && (NULL != destPtr->Ptr) && (0 < destPtr->Len) ) destPtr->Ptr[0] = 0;
	do
	{
		if 	(  (NULL == sourcePtr) 
			|| (NULL == destPtr)
			|| (NULL == sourcePtr->Ptr)
			|| (NULL == destPtr->Ptr) 
			|| (0 == sourcePtr->Len) 
			|| (0 == destPtr->Len) 
			)	 break;
		
		if (0 == sourcePtr->Ptr[0]) 
		{	 
			destPtr->Len = 0;
			break;
		}
	
		const StrPtrLen replaceValue(replaceStr);
		StringFormatter formattedString(destPtr->Ptr, destPtr->Len);
		StringParser sourceStringParser(sourcePtr);
		StrPtrLen preStopChars;
		
		do
		{	sourceStringParser.ConsumeUntil(&preStopChars, StringParser::sEOLWhitespaceMask);
			if (preStopChars.Len > 0)
			{	formattedString.Put(preStopChars);
				if (true == sourceStringParser.Expect(' ') )
				{	formattedString.Put(replaceValue.Ptr, replaceValue.Len);
				}
			}

		} while ( preStopChars.Len != 0);
		
		destPtr->Set(formattedString.GetBufPtr(), formattedString.GetBytesWritten() );
		
	} while (false);
}	


QTSS_Error LogRequest( QTSS_ClientSessionObject inClientSession,
							QTSS_RTSPSessionObject inRTSPSession, QTSS_CliSesClosingReason *inCloseReasonPtr)
{
	static StrPtrLen sUnknownStr(sVoidField);
	static StrPtrLen sTCPStr("TCP");
	static StrPtrLen sUDPStr("UDP");
	
	//Fetch the URL, user agent, movielength & movie bytes to log out of the RTP session
	enum { 	
			eTempLogItemSize	= 256, // must be same or larger than others
			eURLSize 			= 256, 
			eUserAgentSize 		= 256, 
			ePlayerIDSize		= 32, 
			ePlayerVersionSize 	= 32, 
			ePlayerLangSize		= 32, 
			ePlayerOSSize		= 32, 
			ePlayerOSVersSize	= 32,
			ePlayerCPUSize 		= 32 
		};
	
	char tempLogItemBuf[eTempLogItemSize] = { 0 };	
	StrPtrLen tempLogStr(tempLogItemBuf, eTempLogItemSize -1);

	///inClientSession should never be NULL
	//inRTSPRequest may be NULL if this is a timeout
	
	OSMutexLocker locker(sLogMutex);
	CheckAccessLogState(false);
	if (sAccessLog == NULL)
		return QTSS_NoErr;
		
	//if logging is on, then log the request... first construct a timestamp
	char theDateBuffer[QTSSRollingLog::kMaxDateBufferSizeInBytes];
	Bool16 result = QTSSRollingLog::FormatDate(theDateBuffer);
	

	//for now, just ignore the error.
	if (!result)
		theDateBuffer[0] = '\0';
	
	UInt32 theLen = sizeof(QTSS_RTSPSessionObject);
	QTSS_RTSPSessionObject theRTSPSession = inRTSPSession;
	if (theRTSPSession == NULL)
		(void)QTSS_GetValue(inClientSession, qtssCliSesLastRTSPSession, 0, (void*) &theRTSPSession, &theLen);
	
	// Get lots of neat info to log from the various dictionaries

	// Each attribute must be copied out to ensure that it is NULL terminated.
	// To ensure NULL termination, just memset the buffers to 0, and make sure that
	// the last byte of each array is untouched.
	
	Float64* movieDuration = NULL;
	UInt64* movieSizeInBytes = NULL;
	UInt32* movieAverageBitRatePtr = 0;
	UInt32 clientPacketsReceived = 0;
	UInt32 clientPacketsLost = 0;
	StrPtrLen* theTransportType = &sUnknownStr;
	SInt64* theCreateTime = NULL;
		
	char localIPAddrBuf[20] = { 0 };
	StrPtrLen localIPAddr(localIPAddrBuf, 19);

	char localDNSBuf[70] = { 0 };
	StrPtrLen localDNS(localDNSBuf, 69);

	char remoteAddrBuf[20] = { 0 };	
	StrPtrLen remoteAddr(remoteAddrBuf, 19);
	
	char playerIDBuf[ePlayerIDSize] = { 0 };	
	StrPtrLen playerID(playerIDBuf, ePlayerIDSize -1) ;

	// First, get networking info from the RTSP session
	(void)QTSS_GetValue(inClientSession, qtssCliRTSPSessLocalAddrStr, 0, localIPAddr.Ptr, &localIPAddr.Len);
	(void)QTSS_GetValue(inClientSession, qtssCliRTSPSessLocalDNS, 0, localDNS.Ptr, &localDNS.Len);
	(void)QTSS_GetValue(inClientSession, qtssCliRTSPSessRemoteAddrStr, 0, remoteAddr.Ptr, &remoteAddr.Len);
	(void)QTSS_GetValue(inClientSession, qtssCliRTSPSessRemoteAddrStr, 0, playerID.Ptr, &playerID.Len);		
		
	UInt32* rtpBytesSent = NULL;
	UInt32* rtpPacketsSent = NULL;
	
	
	char urlBuf[eURLSize] = { 0 };	
	StrPtrLen url(urlBuf, eURLSize -1);
	(void)QTSS_GetValue(inClientSession, qtssCliSesPresentationURL, 0, url.Ptr, &url.Len);
	//? this is the item we used to log, but it may break conventional log analysis prgrams expecting
	// a file pathname here.
	//(void)QTSS_GetValue(inClientSession, qtssCliSesFullURL, 0, url.Ptr, &url.Len);

	(void)QTSS_GetValuePtr(inClientSession, qtssCliSesMovieDurationInSecs, 0, (void**)&movieDuration, &theLen);
	(void)QTSS_GetValuePtr(inClientSession, qtssCliSesMovieSizeInBytes, 0, (void**)&movieSizeInBytes, &theLen);
	(void)QTSS_GetValuePtr(inClientSession, qtssCliSesMovieAverageBitRate, 0, (void**)&movieAverageBitRatePtr, &theLen);
	(void)QTSS_GetValuePtr(inClientSession, qtssCliSesCreateTimeInMsec, 0, (void**)&theCreateTime, &theLen);
	(void)QTSS_GetValuePtr(inClientSession, qtssCliSesRTPBytesSent, 0, (void**)&rtpBytesSent, &theLen);
	(void)QTSS_GetValuePtr(inClientSession, qtssCliSesRTPPacketsSent, 0, (void**)&rtpPacketsSent, &theLen);
	
	tempLogStr.Ptr[0] = 0; tempLogStr.Len = eUserAgentSize;
	(void)QTSS_GetValue(inClientSession, qtssCliSesFirstUserAgent, 0, tempLogStr.Ptr, &tempLogStr.Len);
	
	char userAgentBuf[eUserAgentSize] = { 0 };	
	StrPtrLen userAgent(userAgentBuf, eUserAgentSize -1);
	ReplaceSpaces(&tempLogStr, &userAgent, "%20");
	
	UserAgentParser userAgentParser(&userAgent);
	
//	StrPtrLen* playerID = userAgentParser.GetUserID() ;
	StrPtrLen* playerVersion = userAgentParser.GetUserVersion() ;
	StrPtrLen* playerLang = userAgentParser.GetUserLanguage()  ;
	StrPtrLen* playerOS = userAgentParser.GetrUserOS()  ;
	StrPtrLen* playerOSVers = userAgentParser.GetUserOSVersion()  ;
	StrPtrLen* playerCPU = userAgentParser.GetUserCPU()  ;
	
//	char playerIDBuf[ePlayerIDSize] = {};	
	char playerVersionBuf[ePlayerVersionSize] = { 0 };	
	char playerLangBuf[ePlayerLangSize] = { 0 };		
	char playerOSBuf[ePlayerOSSize] = { 0 };	
	char playerOSVersBuf[ePlayerOSVersSize] = { 0 };	
	char playerCPUBuf[ePlayerCPUSize] = { 0 };	
	
	UInt32 size;
//	(ePlayerIDSize < playerID->Len ) ? size = ePlayerIDSize -1 : size = playerID->Len;
//	if (playerID->Ptr != NULL) memcpy (playerIDBuf, playerID->Ptr, size);

	(ePlayerVersionSize < playerVersion->Len ) ? size = ePlayerVersionSize -1 : size = playerVersion->Len;
	if (playerVersion->Ptr != NULL) memcpy (playerVersionBuf, playerVersion->Ptr, size);

	(ePlayerLangSize < playerLang->Len ) ? size = ePlayerLangSize -1 : size = playerLang->Len;
	if (playerLang->Ptr != NULL) memcpy (playerLangBuf, playerLang->Ptr, size);

	(ePlayerOSSize < playerOS->Len ) ? size = ePlayerOSSize -1 : size = playerOS->Len;
	if (playerOS->Ptr != NULL)  memcpy (playerOSBuf, playerOS->Ptr, size);

	(ePlayerOSVersSize < playerOSVers->Len ) ? size = ePlayerOSVersSize -1 : size = playerOSVers->Len;
	if (playerOSVers->Ptr != NULL) memcpy (playerOSVersBuf, playerOSVers->Ptr, size);
	
	(ePlayerCPUSize < playerCPU->Len ) ? size = ePlayerCPUSize -1 : size = playerCPU->Len;
	if (playerCPU->Ptr != NULL) memcpy (playerCPUBuf, playerCPU->Ptr, size);

	
	// clientPacketsReceived, clientPacketsLost, videoPayloadName and audioPayloadName
	// are all stored on a per-stream basis, so let's iterate through all the streams,
	// finding this information

	char videoPayloadNameBuf[32]  = { 0 };	
	StrPtrLen videoPayloadName(videoPayloadNameBuf, 31);

	char audioPayloadNameBuf[32] = { 0 };	
	StrPtrLen audioPayloadName(audioPayloadNameBuf, 31);

	UInt32 qualityLevel = 0;
	UInt32 clientBufferTime = 0;
	UInt32 theStreamIndex = 0;
	Bool16* isTCPPtr = NULL;
	QTSS_RTPStreamObject theRTPStreamObject = NULL;
	
	for (	UInt32 theStreamObjectLen = sizeof(theRTPStreamObject);
			QTSS_GetValue(inClientSession, qtssCliSesStreamObjects, theStreamIndex, (void*)&theRTPStreamObject, &theStreamObjectLen) == QTSS_NoErr;
			theStreamIndex++, theStreamObjectLen = sizeof(theRTPStreamObject))
	{
		
		UInt32* streamPacketsReceived = NULL;
		UInt32* streamPacketsLost = NULL;
		(void)QTSS_GetValuePtr(theRTPStreamObject, qtssRTPStrTotPacketsRecv, 0, (void**)&streamPacketsReceived, &theLen);
		(void)QTSS_GetValuePtr(theRTPStreamObject, qtssRTPStrTotalLostPackets, 0, (void**)&streamPacketsLost, &theLen);
		
		// Add up packets received and packets lost to come up with a session wide total
		if (streamPacketsReceived != NULL)
			clientPacketsReceived += *streamPacketsReceived;
		if (streamPacketsLost != NULL)
			clientPacketsLost += *streamPacketsLost;
		
		// Identify the video and audio codec types
		QTSS_RTPPayloadType* thePayloadType = NULL;
		(void)QTSS_GetValuePtr(theRTPStreamObject, qtssRTPStrPayloadType, 0, (void**)&thePayloadType, &theLen);
		if (thePayloadType != NULL)
		{
			if (*thePayloadType == qtssVideoPayloadType)
				(void)QTSS_GetValue(theRTPStreamObject, qtssRTPStrPayloadName, 0, videoPayloadName.Ptr, &videoPayloadName.Len);
			else if (*thePayloadType == qtssAudioPayloadType)	
				(void)QTSS_GetValue(theRTPStreamObject, qtssRTPStrPayloadName, 0, audioPayloadName.Ptr, &audioPayloadName.Len);
		}
		
		// If any one of the streams is being delivered over UDP instead of TCP,
		// report in the log that the transport type for this session was UDP.
		if (isTCPPtr == NULL)
		{	
			(void)QTSS_GetValuePtr(theRTPStreamObject, qtssRTPStrIsTCP, 0, (void**)&isTCPPtr, &theLen);
			if (isTCPPtr != NULL)
			{	if (*isTCPPtr == false)
					theTransportType = &sUDPStr;
				else
					theTransportType = &sTCPStr;
			}
		}
		
			
		Float32* clientBufferTimePtr = NULL;
		(void)QTSS_GetValuePtr(theRTPStreamObject, qtssRTPStrBufferDelayInSecs, 0, (void**)&clientBufferTimePtr, &theLen);
		if 	( (clientBufferTimePtr != NULL) && (*clientBufferTimePtr != 0) )
		{	if ( *clientBufferTimePtr  > clientBufferTime)
				clientBufferTime = (UInt32) (*clientBufferTimePtr + .5); // round up to full seconds
		}
		
	}

	if (*rtpPacketsSent == 0) // no packets sent
		qualityLevel = 0; // no quality
	else
	{
		if ( (clientPacketsReceived == 0) && (clientPacketsLost == 0) ) // no info from client 
			qualityLevel = 100; //so assume 100
		else
		{	
			float qualityPercent =  (float) clientPacketsReceived / (float) (clientPacketsReceived + clientPacketsLost);
			qualityPercent += (float).005; // round up
			qualityLevel = (UInt32) ( (float) 100.0 * qualityPercent); // average of sum of packet counts for all streams
		}
	}
	
	//we may not have an RTSP request. Assume that the status code is 504 timeout, if there is an RTSP
	//request, though, we can find out what the real status code of the response is
	static UInt32 sTimeoutCode = 504;	
	UInt32* theStatusCode = &sTimeoutCode;
	theLen = sizeof(UInt32);
	(void)QTSS_GetValuePtr(inClientSession, qtssCliRTSPReqRealStatusCode, 0, (void **) &theStatusCode, &theLen);
//	printf("qtssCliRTSPReqRealStatusCode = %lu \n", *theStatusCode);
		
	
	if (inCloseReasonPtr) do
	{
		if (*theStatusCode < 300) // it was a succesful RTSP request but...
		{	
			if (*inCloseReasonPtr == qtssCliSesCloseTimeout) // there was a timeout
			{	
					*theStatusCode = sTimeoutCode;
//					printf(" log timeout ");
					break;
			}

			if (*inCloseReasonPtr == qtssCliSesCloseClientTeardown) // there was a teardown
			{
			
				static QTSS_CliSesClosingReason sReason = qtssCliSesCloseClientTeardown;
				QTSS_CliSesClosingReason* theReasonPtr = &sReason;
				theLen = sizeof(QTSS_CliSesTeardownReason);
				(void)QTSS_GetValuePtr(inClientSession, qtssCliTeardownReason, 0, (void **) &theReasonPtr, &theLen);
//				printf("qtssCliTeardownReason = %lu \n", *theReasonPtr);

				if (*theReasonPtr == qtssCliSesTearDownClientRequest) //  the client asked for a tear down
				{
//					printf(" client requests teardown  ");
					break;
				}
				
				if (*theReasonPtr == qtssCliSesTearDownUnsupportedMedia) //  An error occured while streaming the file.
				{	
						*theStatusCode = 415;
//						printf(" log UnsupportedMedia ");
						break;
				}
	
				*theStatusCode = 500; // some unknown reason for cancelling the connection
			}
			
//			printf("return status ");
			// just use the qtssCliRTSPReqRealStatusCode for the reason
		}
				
	} while (false);

//	printf(" = %lu \n", *theStatusCode);

		
	// Find out what time it is
	SInt64 curTime = QTSS_Milliseconds();
	
	UInt32 numCurClients = 0;
	theLen = sizeof(numCurClients);
	(void)QTSS_GetValue(sServer, qtssRTPSvrCurConn, 0, &numCurClients, &theLen);

/*
	IMPORTANT!!!!
	
	Some values such as cpu, #conns, need to be grabbed as the session starts, not when the teardown happened (I think)

*/	

#if TESTUNIXTIME
	char thetestDateBuffer[QTSSRollingLog::kMaxDateBufferSizeInBytes];
	TestUnixTime(QTSS_MilliSecsTo1970Secs(*theCreateTime), thetestDateBuffer);
	printf("%s\n",thetestDateBuffer);
#endif
	
	float zeroFloat = 0;
	UInt64 zeroUInt64 = 0;
	

	
	UInt32 cpuUtilized = 0; // percent
	{

		#if __MacOSXServer__
				double cpuLoad [3] ;
			    long cpuTicks[CPU_STATE_MAX] ;
				XCPUStats::GetStats(cpuLoad, cpuTicks) ;
				cpuUtilized = 100 - cpuTicks[CPU_STATE_IDLE];
//				printf("CPU Stats: idle = %ld%% , user = %ld%% , system = %ld%% , load: %.2f, %.2f, %.2f/n", 
//								cpuTicks[CPU_STATE_IDLE], cpuTicks[CPU_STATE_USER], cpuTicks[CPU_STATE_SYSTEM], cpuLoad[0], cpuLoad[1], cpuLoad[2]);
		#endif
	}	
		
	char lastUserName[eTempLogItemSize] ={ 0 };
	StrPtrLen lastUserNameStr(lastUserName,eTempLogItemSize);
	
	char lastURLRealm[eTempLogItemSize] ={ 0 };
	StrPtrLen lastURLRealmStr(lastURLRealm,eTempLogItemSize);
	
	//printf("logging of saved params are in dictionary \n");
		
	tempLogStr.Ptr[0] = 0; tempLogStr.Len = eTempLogItemSize;
	(void)QTSS_GetValue(inClientSession, qtssCliRTSPSesUserName, 0, tempLogStr.Ptr, &tempLogStr.Len);
	ReplaceSpaces(&tempLogStr, &lastUserNameStr, "%20");
	//printf("qtssRTSPSesLastUserName dictionary item = %s len = %ld\n",lastUserNameStr.Ptr,lastUserNameStr.Len);
		
	tempLogStr.Ptr[0] = 0; tempLogStr.Len = eTempLogItemSize;
	(void)QTSS_GetValue(inClientSession, qtssCliRTSPSesURLRealm, 0, tempLogStr.Ptr, &tempLogStr.Len);
	ReplaceSpaces(&tempLogStr, &lastURLRealmStr, "%20");
	//printf("qtssRTSPSesLastURLRealm dictionary  item = %s len = %ld\n",lastURLRealmStr.Ptr,lastURLRealmStr.Len);	

	char respMsgBuffer[1024] = { 0 };
	StrPtrLen theRespMsg;	
	(void)QTSS_GetValuePtr(inClientSession, qtssCliRTSPReqRespMsg, 0, (void**)&theRespMsg.Ptr, &theRespMsg.Len);
	StrPtrLen respMsgEncoded(respMsgBuffer, 1024 -1);
	SInt32 theErr = StringTranslator::EncodeURL(theRespMsg.Ptr, theRespMsg.Len, respMsgEncoded.Ptr, respMsgEncoded.Len);
	if (theErr <= 0)
		respMsgEncoded.Ptr[0] = '\0';
	else
	{
		respMsgEncoded.Len = theErr;
		respMsgEncoded.Ptr[respMsgEncoded.Len] = '\0';
	}
	
char tempLogBuffer[2048];
// compatible fields (no respMsgEncoded field)
::sprintf(tempLogBuffer, "%s %s %s %s %lu %lu %ld %ld %s %s %s %s %s %s %s %s %s %s %0.0f %"_64BITARG_"d %lu %s %s %s %s %s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %s %s %lu %lu %s %s \n",	
//::sprintf(tempLogBuffer, "%s %s %s %s %lu %lu %ld %ld %s %s %s %s %s %s %s %s %s %s %0.0f %"_64BITARG_"d %lu %s %s %s %s %s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %s %s %lu %lu %s %s %s\n",	
//below has the 3 last fields added for apple only fields - we'd like to add these, but need to make sure log analysis apps can handle it
//::sprintf(tempLogBuffer, "%s %s %s %s %lu %lu %ld %ld %s %s %s %s %s %s %s %s %s %s %0.0f %"_64BITARG_"d %lu %s %s %s %s %s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %s %s %lu %lu %s %s %s %s\n",	
									(remoteAddr.Ptr[0] == '\0') ? sVoidField : remoteAddr.Ptr,	//c-ip*
									(theDateBuffer[0] == '\0') ? sVoidField : theDateBuffer,	//date* time*
									sVoidField,	//c-dns
									(url.Ptr[0] == '\0') ? sVoidField : url.Ptr,		//cs-uri-stem*
									theCreateTime == NULL ? 0UL : (UInt32) QTSS_MilliSecsTo1970Secs(*theCreateTime),	//c-starttime 
									theCreateTime == NULL ? 0UL : (UInt32) (QTSS_MilliSecsTo1970Secs(curTime)  - QTSS_MilliSecsTo1970Secs(*theCreateTime)), //QTSS_MilliSecsTo1970Secs(curTime - *theCreateTime),	//x-duration*  
									1UL,	//c-rate 
									*theStatusCode,	//c-status*
									(playerIDBuf[0] == '\0') ? sVoidField : playerIDBuf,	//c-playerid*
									(playerVersionBuf[0] == '\0') ? sVoidField : playerVersionBuf,	//c-playerversion
									(playerLangBuf[0] == '\0') ? sVoidField : playerLangBuf,	//c-playerlanguage*
									(userAgent.Ptr[0] == '\0') ? sVoidField : userAgent.Ptr,	//cs(User-Agent) 
									sVoidField,	//cs(Referer) 
									sVoidField,	//c-hostexe
									sVoidField,	//c-hostexever*
									(playerOSBuf[0] == '\0') ? sVoidField : playerOSBuf,	//c-os*
									(playerOSVersBuf[0] == '\0') ? sVoidField : playerOSVersBuf,	//c-osversion
									(playerCPUBuf[0] == '\0') ? sVoidField : playerCPUBuf,	//c-cpu*
									movieDuration == NULL ? zeroFloat : *movieDuration,	//filelength in secs*	
									movieSizeInBytes == NULL ? zeroUInt64 : *movieSizeInBytes,	//filesize in bytes*
									movieAverageBitRatePtr == NULL ? (UInt32) 0 : *movieAverageBitRatePtr,	//avgbandwidth in bits per second
									"RTP",	//protocol 
									theTransportType->Ptr,	//transport 
									(audioPayloadName.Ptr[0] == '\0') ? sVoidField : audioPayloadName.Ptr,	//audiocodec*
									(videoPayloadName.Ptr[0] == '\0') ? sVoidField : videoPayloadName.Ptr,	//videocodec*
									sVoidField,	//channelURL
									rtpBytesSent == NULL ? 0UL : *rtpBytesSent,	//sc-bytes*
									0UL,	//cs-bytes  
									rtpPacketsSent == NULL ? 0UL : *rtpPacketsSent,	//s-pkts-sent*
									clientPacketsReceived,	//c-pkts-recieved
									clientPacketsLost,	//c-pkts-lost-client*			
									0UL,		//c-pkts-lost-net 
									0UL,		//c-pkts-lost-cont-net 
									0UL,		//c-resendreqs*
									0UL,		//c-pkts-recovered-ECC*
									0UL,		//c-pkts-recovered-resent
									1UL,		//c-buffercount 
									clientBufferTime,		//c-totalbuffertime*
									qualityLevel,//c-quality 
									(localIPAddr.Ptr[0] == '\0') ? sVoidField : localIPAddr.Ptr,	//s-ip 
									(localDNS.Ptr[0] == '\0') ? sVoidField : localDNS.Ptr,	//s-dns
									numCurClients,	//s-totalclients
									cpuUtilized,	//s-cpu-util
									sVoidField	//cs-uri-query 
		
									//  below are Apple only fields
									
									,(lastUserName[0] == '\0') ? sVoidField : lastUserName
									,(lastURLRealm[0] == '\0') ? sVoidField : lastURLRealm

									, (respMsgEncoded.Ptr[0] == '\0') ? sVoidField : respMsgEncoded.Ptr
									);


	Assert(::strlen(tempLogBuffer) < 2048);
	
	//finally, write the log message
	sAccessLog->WriteToLog(tempLogBuffer, kAllowLogToRoll);
	return QTSS_NoErr;
}


void CheckAccessLogState(Bool16 forceEnabled)
{
	//this function makes sure the logging state is in synch with the preferences.
	//extern variable declared in QTSSPreferences.h
	//check error log.
	if ((NULL == sAccessLog) && (forceEnabled || sLogEnabled))
	{
		sAccessLog = NEW QTSSAccessLog();
		sAccessLog->EnableLog();
	}

	if ((NULL != sAccessLog) && ((!forceEnabled) && (!sLogEnabled)))
	{
		delete sAccessLog;
		sAccessLog = NULL;
	}
}

// SERVICE ROUTINES

QTSS_Error RollAccessLog(QTSS_ServiceFunctionArgsPtr /*inArgs*/)
{
	const Bool16 kForceEnable = true;

	OSMutexLocker locker(sLogMutex);
	//calling CheckLogState is a kludge to allow logs
	//to be rolled while logging is disabled.

	CheckAccessLogState(kForceEnable);
	
	if (sAccessLog != NULL )
		sAccessLog->RollLog();
		
	CheckAccessLogState(!kForceEnable);
	return QTSS_NoErr;
}


#pragma mark __QTSS_ACCESS_LOG_OBJECT__


time_t QTSSAccessLog::WriteLogHeader(FILE *inFile)
{
	time_t calendarTime = QTSSRollingLog::WriteLogHeader(inFile);

	//format a date for the startup time
	char theDateBuffer[QTSSRollingLog::kMaxDateBufferSizeInBytes] = { 0 };
	Bool16 result = QTSSRollingLog::FormatDate(theDateBuffer);
	
	char tempBuffer[1024] = { 0 };
	if (result)
	{
		StrPtrLen serverName;
		(void)QTSS_GetValuePtr(sServer, qtssSvrServerName, 0, (void**)&serverName.Ptr, &serverName.Len);
		StrPtrLen serverVersion;
		(void)QTSS_GetValuePtr(sServer, qtssSvrServerVersion, 0, (void**)&serverVersion.Ptr, &serverVersion.Len);
		::sprintf(tempBuffer, sLogHeader, serverName.Ptr , serverVersion.Ptr, theDateBuffer);
		this->WriteToLog(tempBuffer, !kAllowLogToRoll);
	}
		
	return calendarTime;
}





/*



Log file format recognized by Lariat Stats				
				
#Fields: c-ip date time c-dns cs-uri-stem c-starttime x-duration c-rate c-status c-playerid c-playerversion c-playerlanguage cs(User-Agent) cs(Referer) c-hostexe c-hostexever c-os c-osversion c-cpu filelength filesize avgbandwidth protocol transport audiocodec videocodec channelURL sc-bytes c-bytes s-pkts-sent c-pkts-received c-pkts-lost-client c-pkts-lost-net c-pkts-lost-cont-net c-resendreqs c-pkts-recovered-ECC c-pkts-recovered-resent c-buffercount c-totalbuffertime c-quality s-ip s-dns s-totalclients s-cpu-util				
e.g. 157.56.87.123 1998-01-19 22:53:59 foo.bar.com mms://ddelval1/56k_5min_1.asf 1 16 1 200 - 5.1.51.119 0409 - - dshow.exe 5.1.51.119 Windows_NT 4.0.0.1381 Pentium 78 505349 11000 mms UDP MPEG_Layer-3 MPEG-4_Video_High_Speed_Compressor_(MT) - 188387 188387 281 281 0 0 0 0 0 0 1 5 100 157.56.87.123 foo.bar.com 1 0 				

Notes:				
In the table below, W3C/Custom - refers to whether the fields are supported by the W3C file format or if it is a Custom NetShow field				
All fields are space-delimited				
Fields not used by Lariat Stats should be present in the log file in some form, preferably a "-"				
Log files should be named according to the format: Filename.YYMMDDNNN.log, where Filename is specific to the server type, YYMMDD is the date of creation, and NNN is a 3 digit number used when there are multiple logs from the same date (e.g. 000, 001, 002, etc.)				

Field Name	Value	W3C/Custom	Example value	Used by Stats
				
c-ip 	IP address of client	W3C	157.100.200.300	y
date 	Date of the access	W3C	11-16-98	y
time 	Time of the access (HH:MM:SS)	W3C	15:30:30	y
c-dns 	Resolved dns of the client	W3C	fredj.ford.com	n
cs-uri-stem 	Requested file	W3C	mms://server/sample.asf	y
c-starttime 	Start time	W3C	0   [in seconds, no fractions]	n
x-duration	Duration of the session (s)	W3C	31   [in seconds, no fractions]	y
c-rate 	Rate file was played by client	NetShow Custom	1   [1= play, -5=rewind, +5=fforward]	n
c-status 	http return code	NetShow Custom	200  [mapped to http/rtsp status codes; 200 is success, 404 file not found...]	y
c-playerid 	unique player ID	NetShow Custom	[a GUID value]	y
c-playerversion	player version	NetShow Custom	3.0.0.1212   	
c-playerlanguage	player language	NetShow Custom	EN   [two letter country code]	y
cs(User-Agent) 	user agent	W3C	Mozilla/2.0+(compatible;+MSIE+3.0;+Windows 95)  - this is a sample user-agent string	n
cs(Referer) 	referring URL	W3C	http://www.gte.com	n
c-hostexe	host program	NetShow Custom	iexplore.exe   [iexplore.exe, netscape.exe, dshow.exe, nsplay.exe, vb.exe, etc…]	n
c-hostexever	version	NetShow Custom	4.70.1215 	y
c-os	os	NetShow Custom	Windows   [Windows, Windows NT, Unix-[flavor], Mac-[flavor]]	y
c-osversion	os version	NetShow Custom	4.0.0.1212	n
c-cpu 	cpu type	NetShow Custom	Pentium   [486, Pentium, Alpha %d, Mac?, Unix?]	y
filelength 	file length (s)	NetShow Custom	60   [in seconds, no fractions]	y
filesize 	file size (bytes)	NetShow Custom	86000   [ie: 86kbytes]	y
avgbandwidth		NetShow Custom	24300   [ie: 24.3kbps] 	n
protocol 		NetShow Custom	MMS   [mms, http]	n
transport 		NetShow Custom	UDP   [udp, tcp, or mc]	n
audiocodec		NetShow Custom	MPEG-Layer-3	y
videocodec		NetShow Custom	MPEG4	y
channelURL		NetShow Custom	http://server/channel.nsc	n
sc-bytes	bytes sent by server	W3C	30000   [30k bytes sent from the server to the client]	y
cs-bytes 	bytes received by client	W3C	28000   [bytes received]	n
s-pkts-sent	packets sent	NetShow Custom	55	y
c-pkts-recieved 	packets received	NetShow Custom	50	n
c-pkts-lost-client	packets lost	NetShow Custom	5	y
c-pkts-lost-net 		NetShow Custom	2   [renamed from “erasures”; refers to packets lost at the network layer]	n
c-pkts-lost-cont-net 		NetShow Custom	2   [continuous packets lost at the network layer]	n
c-resendreqs	packets resent	NetShow Custom	5	y
c-pkts-recovered-ECC 	packets resent successfully	NetShow Custom	1   [this refers to packets recovered in the client layer]	y
c-pkts-recovered-resent		NetShow Custom	5   [this refers to packets recovered via udp resend]	n
c-buffercount 		NetShow Custom	1	n
c-totalbuffertime 	seconds buffered	NetShow Custom	20   [in seconds]	y
c-quality 	quality measurement	NetShow Custom	89   [in percent]	n
s-ip 	server ip	W3C	155.12.1.234   [entered by the unicast server]	n
s-dns	server dns	W3C	foo.company.com	n
s-totalclients	total connections at time of access	NetShow Custom	201   [total clients]	n
s-cpu-util	cpu utilization at time of access	NetShow Custom	40   [in percent]	n
cs-uri-query 		W3C	language=EN&rate=1&CPU=486&protocol=MMS&transport=UDP&quality=89&avgbandwidth=24300	n

*/
