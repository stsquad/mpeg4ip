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
	Contains:	Object store for RTSP server preferences.
	
	
	
*/

#ifndef __QTSSERVERPREFS_H__
#define __QTSSERVERPREFS_H__

#include "StrPtrLen.h"
#include "QTSSPrefs.h"
#include "XMLPrefsParser.h"

class QTSServerPrefs : public QTSSPrefs
{
	public:

		// INITIALIZE
		//
		// This function sets up the dictionary map. Must be called before instantiating
		// the first RTSPPrefs object.
	
		static void Initialize();

		QTSServerPrefs(XMLPrefsParser* inPrefsSource, Bool16 inWriteMissingPrefs);
		virtual ~QTSServerPrefs() {}
		
		//This is callable at any time, and is thread safe wrt to the accessors.
		//Pass in true if you want this function to update the prefs file if
		//any defaults need to be used. False otherwise
		void RereadServerPreferences(Bool16 inWriteMissingPrefs);
		
		//Individual accessor methods for preferences.
		
		//Amount of idle time after which respective protocol sessions are timed out
		//(stored in seconds)
		
		//This is the value we advertise to clients (lower than the real one)
		UInt32	GetRTSPTimeoutInSecs()	{ return fRTSPTimeoutInSecs; }
		UInt32	GetRTPTimeoutInSecs()	{ return fRTPTimeoutInSecs; }
		StrPtrLen*	GetRTSPTimeoutAsString() { return &fRTSPTimeoutString; }
		
		//This is the real timeout
		UInt32	GetRealRTSPTimeoutInSecs(){ return fRealRTSPTimeoutInSecs; }
		
		//-1 means unlimited
		SInt32	GetMaxConnections()			{ return fMaximumConnections; }
		SInt32	GetMaxKBitsBandwidth()		{ return fMaxBandwidthInKBits; }
		
		// for tcp thinning
		SInt32	GetTCPMinThinDelayToleranceInMSec()		{ return fTCPMinThinDelayToleranceInMSec; }
		SInt32	GetTCPMaxThinDelayToleranceInMSec()		{ return fTCPMaxThinDelayToleranceInMSec; }
		SInt32	GetTCPVideoDelayToleranceInMSec()		{ return fTCPVideoDelayToleranceInMSec;   }
		SInt32	GetTCPAudioDelayToleranceInMSec()		{ return fTCPAudioDelayToleranceInMSec;   }
		SInt32	GetTCPThickIntervalInSec()				{ return fTCPThickIntervalInSec;   }
		
		// Thinning algorithm parameters
		SInt32	GetDropAllPacketsTimeInMsec()			{ return fDropAllPacketsTimeInMsec; }
		SInt32	GetThinAllTheWayTimeInMsec()			{ return fThinAllTheWayTimeInMsec; }
		SInt32	GetOptimalDelayTimeInMsec()				{ return fOptimalDelayInMsec; }
		SInt32	GetStartThickingTimeInMsec()			{ return fStartThickingTimeInMsec; }
		UInt32	GetQualityCheckIntervalInMsec()			{ return fQualityCheckIntervalInMsec; }
				
		// for tcp buffer size scaling
		UInt32	GetMinTCPBufferSizeInBytes()			{ return fMinTCPBufferSizeInBytes; }
		UInt32	GetMaxTCPBufferSizeInBytes()			{ return fMaxTCPBufferSizeInBytes; }
		Float32	GetTCPSecondsToBuffer()					{ return fTCPSecondsToBuffer; }
		
		//for joining HTTP sessions from behind a round-robin DNS
		Bool16	GetDoReportHTTPConnectionAddress() 		{ return fDoReportHTTPConnectionAddress;  }
		
		//for debugging, mainly
		Bool16		ShouldServerBreakOnAssert()			{ return fBreakOnAssert; }
		Bool16		IsAutoRestartEnabled()				{ return fAutoRestart; }

		UInt32		GetTotalBytesUpdateTimeInSecs()		{ return fTBUpdateTimeInSecs; }
		UInt32		GetAvgBandwidthUpdateTimeInSecs()	{ return fABUpdateTimeInSecs; }
		UInt32		GetSafePlayDurationInSecs()			{ return fSafePlayDurationInSecs; }
		
		// For the compiled-in error logging module
		
		Bool16	IsErrorLogEnabled() 			{ return fErrorLogEnabled; }
		Bool16	IsScreenLoggingEnabled()		{ return fScreenLoggingEnabled; }

		UInt32	GetMaxErrorLogBytes() 			{ return fErrorLogBytes; }
		UInt32	GetErrorRollIntervalInDays() 	{ return fErrorRollIntervalInDays; }
		UInt32	GetErrorLogVerbosity() 			{ return fErrorLogVerbosity; }
		
		Bool16	GetAppendSrcAddrInTransport()	{ return fAppendSrcAddrInTransport; }
		
		//
		// For UDP retransmits
		UInt32	IsReliableUDPEnabled()			{ return fReliableUDP; }
		UInt32	GetMaxRetransmitDelayInMsec()	{ return fMaxRetransDelayInMsec; }
		Bool16	IsAckLoggingEnabled()			{ return fIsAckLoggingEnabled; }
		UInt32	GetRTCPPollIntervalInMsec()		{ return fRTCPPollIntervalInMsec; }
		UInt32	GetRTCPSocketRcvBufSizeinK()	{ return fRTCPSocketRcvBufSizeInK; }
		UInt32	GetOverbufferBucketIntervalInMsec(){ return fOverbufferBucketIntervalInMsec; }
		UInt32	GetMaxSendAheadTimeInSecs()		{ return fMaxSendAheadTimeInSecs; }
		Bool16	IsSlowStartEnabled()			{ return fIsSlowStartEnabled; }
		Bool16	GetDefaultWindowSizeInK()		{ return fDefaultWindowSizeInK; }
		Bool16	GetReliableUDPPrintfsEnabled()	{ return fReliableUDPPrintfs; }

		//
		// Optionally require that reliable UDP content be in certain folders
		Bool16 IsPathInsideReliableUDPDir(StrPtrLen* inPath);

		// Movie folder pref. If the path fits inside the buffer provided,
		// the path is copied into that buffer. Otherwise, a new buffer is allocated
		// and returned.
		char*	GetMovieFolder(char* inBuffer, UInt32* ioLen);
		
		//
		// Transport addr pref. Caller must provide a buffer big enough for an IP addr
		void	GetTransportSrcAddr(StrPtrLen* ioBuf);
				
		// String preferences. Note that the pointers returned here is allocated
		// memory that you must delete!
		
		char*	GetErrorLogDir()
			{ return this->GetStringPref(qtssPrefsErrorLogDir); }
		char*	GetErrorLogName()
			{ return this->GetStringPref(qtssPrefsErrorLogName); }

		char*	GetModuleDirectory()
			{ return this->GetStringPref(qtssPrefsModuleFolder); }
			
		char*	GetAuthorizationRealm()
			{ return this->GetStringPref(qtssPrefsDefaultAuthorizationRealm); }

		char*	GetRunUserName()
			{ return this->GetStringPref(qtssPrefsRunUserName); }
		char*	GetRunGroupName()
			{ return this->GetStringPref(qtssPrefsRunGroupName); }


		Bool16	AutoDeleteSDPFiles()		{ return fauto_delete_sdp_files; }
		QTSS_AuthScheme GetAuthScheme()		{ return fAuthScheme; }
		
		UInt32 DeleteSDPFilesInterval()		{ return fsdp_file_delete_interval_seconds; }
		
	private:

		UInt32 		fRTSPTimeoutInSecs;
		char		fRTSPTimeoutBuf[20];
		StrPtrLen	fRTSPTimeoutString;
		UInt32 		fRealRTSPTimeoutInSecs;
		UInt32 		fRTPTimeoutInSecs;
		
		SInt32 	fMaximumConnections;
		SInt32	fMaxBandwidthInKBits;
		
		Bool16	fBreakOnAssert;
		Bool16	fAutoRestart;
		UInt32	fTBUpdateTimeInSecs;
		UInt32	fABUpdateTimeInSecs;
		UInt32	fSafePlayDurationInSecs;
		
		UInt32	fErrorRollIntervalInDays;
		UInt32	fErrorLogBytes;
		UInt32	fErrorLogVerbosity;
		Bool16	fScreenLoggingEnabled;
		Bool16	fErrorLogEnabled;
		
		// tcp (http) thinning parameters
		SInt32	fTCPMinThinDelayToleranceInMSec;
		SInt32	fTCPMaxThinDelayToleranceInMSec;
		SInt32	fTCPVideoDelayToleranceInMSec;
		SInt32	fTCPAudioDelayToleranceInMSec;
		SInt32	fTCPThickIntervalInSec;
		
		SInt32	fDropAllPacketsTimeInMsec;
		SInt32	fThinAllTheWayTimeInMsec;
		SInt32	fOptimalDelayInMsec;
		SInt32	fStartThickingTimeInMsec;
		UInt32	fQualityCheckIntervalInMsec;

		UInt32	fMinTCPBufferSizeInBytes;
		UInt32	fMaxTCPBufferSizeInBytes;
		Float32	fTCPSecondsToBuffer;

		Bool16	fDoReportHTTPConnectionAddress;
		Bool16	fAppendSrcAddrInTransport;

		UInt32	fMaxRetransDelayInMsec;
		UInt32	fDefaultWindowSizeInK;
		Bool16	fIsAckLoggingEnabled;
		UInt32	fRTCPPollIntervalInMsec;
		UInt32	fRTCPSocketRcvBufSizeInK;
		Bool16	fIsSlowStartEnabled;
		UInt32	fOverbufferBucketIntervalInMsec;
		UInt32	fMaxSendAheadTimeInSecs;
		Bool16  fauto_delete_sdp_files;
		QTSS_AuthScheme fAuthScheme;
		UInt32	fsdp_file_delete_interval_seconds;
		Bool16	fAutoStart;
		Bool16	fReliableUDP;
		Bool16	fReliableUDPPrintfs;
		
		enum
		{
			kAllowMultipleValues = 1,
			kDontAllowMultipleValues = 0
		};
		
		struct PrefInfo
		{
			UInt32 	fAllowMultipleValues;
			char*	fDefaultValue;
			char**	fAdditionalDefVals; // For prefs with multiple default values
		};
			
		void SetupAttributes();
		void UpdateAuthScheme();

		//
		// Returns the string preference with the specified ID. If there
		// was any problem, this will return an empty string.
		char* GetStringPref(QTSS_AttributeID inAttrID);
		
		static QTSSAttrInfoDict::AttrInfo	sAttributes[];
		static PrefInfo sPrefInfo[];
		
		// Prefs that have multiple default values (rtsp_ports) have
		// to be dealt with specially
		static char*	sAdditionalDefaultPorts[];
};
#endif //__QTSSPREFS_H__
