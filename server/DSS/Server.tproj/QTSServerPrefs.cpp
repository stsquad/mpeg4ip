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
	File:		QTSSPrefs.cpp

	Contains:	Implements class defined in QTSSPrefs.h.

	Change History (most recent first):
	
*/

#include "QTSServerPrefs.h"
#include "MyAssert.h"
#include "OSMemory.h"
#include "QTSSDataConverter.h"
 
#ifndef __Win32__
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
 
#pragma mark __STATIC_DATA__

char* QTSServerPrefs::sAdditionalDefaultPorts[] =
{
	"7070",
	NULL
};

QTSServerPrefs::PrefInfo QTSServerPrefs::sPrefInfo[] =
{
	{ kDontAllowMultipleValues, "0", 		NULL					},	//rtsp_timeout
	{ kDontAllowMultipleValues,	"180", 		NULL					},	//real_rtsp_timeout
	{ kDontAllowMultipleValues,	"60",		NULL					},	//rtp_timeout
	{ kDontAllowMultipleValues,	"1000",		NULL					},	//maximum_connections
	{ kDontAllowMultipleValues,	"102400",	NULL					},	//maximum_bandwidth
#ifdef __MacOSX__
	{ kDontAllowMultipleValues,	"/Library/QuickTimeStreaming/Movies",	NULL	},	//movie_folder
#elif __Win32__
	{ kDontAllowMultipleValues,	"c:\\Program Files\\Darwin Streaming Server\\Movies",	NULL	},	//movie_folder
#else
	{ kDontAllowMultipleValues,	"/usr/local/movies",NULL			},	//movie_folder
#endif
	{ kDontAllowMultipleValues,	"0",		NULL					},	//bind_ip_addr
	{ kDontAllowMultipleValues,	"true",		NULL					},	//break_on_assert
	{ kDontAllowMultipleValues,	"true",		NULL					},	//auto_restart
	{ kDontAllowMultipleValues,	"1",		NULL					},	//total_bytes_update
	{ kDontAllowMultipleValues,	"60",		NULL					},	//average_bandwidth_update
	{ kDontAllowMultipleValues,	"600",		NULL					},	//safe_play_duration
#ifdef __MacOSX__
	{ kDontAllowMultipleValues,	"/usr/sbin/StreamingServerModules",	NULL	},	//module_folder
#elif __Win32__
	{ kDontAllowMultipleValues,	"c:\\Program Files\\Darwin Streaming Server\\QTSSModules",	NULL	},	//module_folder
#else
	{ kDontAllowMultipleValues,	"/usr/local/sbin/StreamingServerModules",	NULL	},	//module_folder
#endif
	{ kDontAllowMultipleValues,	"Error",	NULL					},	//error_logfile_name
#ifdef __MacOSX__
	{ kDontAllowMultipleValues,	"/Library/QuickTimeStreaming/Logs/",	NULL	},	//error_logfile_dir
#elif __Win32__
	{ kDontAllowMultipleValues,	"c:\\Program Files\\Darwin Streaming Server\\Logs",	NULL	},	//error_logfile_dir
#else
	{ kDontAllowMultipleValues,	"/var/streaming/logs",	NULL		},	//error_logfile_dir
#endif
	{ kDontAllowMultipleValues,	"0",		NULL					},	//error_logfile_interval
	{ kDontAllowMultipleValues,	"256000",	NULL					},	//error_logfile_size
	{ kDontAllowMultipleValues,	"2",		NULL					},	//error_logfile_verbosity
	{ kDontAllowMultipleValues,	"true",		NULL					},	//screen_logging
	{ kDontAllowMultipleValues,	"true",		NULL					},	//error_logging
	{ kDontAllowMultipleValues,	"50",		NULL					},	//tcp_min_thin_delay_tolerance
	{ kDontAllowMultipleValues,	"1000",		NULL					},	//tcp_max_thin_delay_tolerance
	{ kDontAllowMultipleValues,	"2500",		NULL					},	//tcp_max_video_delay_tolerance
	{ kDontAllowMultipleValues,	"4000",		NULL					},	//tcp_max_audio_delay_tolerance
	{ kDontAllowMultipleValues,	"8192",		NULL					},	//min_tcp_buffer_size
	{ kDontAllowMultipleValues,	"32768",	NULL					},	//max_tcp_buffer_size
	{ kDontAllowMultipleValues,	".33",		NULL					},	//tcp_seconds_to_buffer
	{ kDontAllowMultipleValues,	"false",	NULL					},	//do_report_http_connection_ip_address
	{ kDontAllowMultipleValues,	"Streaming Server",	NULL			},	//default_authorization_realm
	{ kDontAllowMultipleValues,	"/var/run/qtss.pid",NULL			},	//pid_file_name
	{ kDontAllowMultipleValues,	"",			NULL					},	//set_user_name
	{ kDontAllowMultipleValues,	"",			NULL					},	//set_group_name
	{ kDontAllowMultipleValues,	"false",	NULL					},	//append_source_addr_in_transport
	{ kAllowMultipleValues,		"554",		sAdditionalDefaultPorts	},	//rtsp_ports
	{ kDontAllowMultipleValues,	"200",		NULL					},	//max_retransmit_delay
	{ kDontAllowMultipleValues,	"128",		NULL					},	//default_window_size
	{ kDontAllowMultipleValues,	"false",	NULL					},	//ack_logging_enabled
	{ kDontAllowMultipleValues,	"100",		NULL					},	//rtcp_poll_interval
	{ kDontAllowMultipleValues,	"64",		NULL					},	//rtcp_rcv_buf_size
	{ kDontAllowMultipleValues,	"true",		NULL					},	//reliable_udp_slow_start
	{ kDontAllowMultipleValues,	"true",		NULL					},	//reliable_udp_enabled
	{ kDontAllowMultipleValues,	"6",		NULL					},	//max_overbuffer_time
	{ kDontAllowMultipleValues,	"15",		NULL					},	//tcp_thick_min_interval
	{ kDontAllowMultipleValues,	"0",		NULL					},	//rtp_packet_drop_pct
	{ kDontAllowMultipleValues,	"true",		NULL					},	//share_window_stats
	{ kDontAllowMultipleValues,	"",			NULL					}	//alt_transport_src_ipaddr
};



QTSSAttrInfoDict::AttrInfo	QTSServerPrefs::sAttributes[] =
{   /*fields:   fAttrName, fFuncPtr, fAttrDataType, fAttrPermission */
	/* 0 */ { "rtsp_timeout",			NULL,					qtssAttrDataTypeUInt32,					qtssAttrModeRead | qtssAttrModeWrite | qtssAttrModeWrite },
	/* 1 */ { "real_rtsp_timeout",		NULL,					qtssAttrDataTypeUInt32,					qtssAttrModeRead | qtssAttrModeWrite },
	/* 2 */ { "rtp_timeout",			NULL,					qtssAttrDataTypeUInt32,					qtssAttrModeRead | qtssAttrModeWrite },
	/* 3 */ { "maximum_connections",	NULL,					qtssAttrDataTypeSInt32,					qtssAttrModeRead | qtssAttrModeWrite },
	/* 4 */ { "maximum_bandwidth",		NULL,					qtssAttrDataTypeSInt32,					qtssAttrModeRead | qtssAttrModeWrite },
	/* 5 */ { "movie_folder",			NULL,					qtssAttrDataTypeCharArray,				qtssAttrModeRead | qtssAttrModeWrite },
	/* 6 */ { "bind_ip_addr",			NULL,					qtssAttrDataTypeCharArray,				qtssAttrModeRead | qtssAttrModeWrite },
	/* 7 */ { "break_on_assert",		NULL,					qtssAttrDataTypeBool16,					qtssAttrModeRead | qtssAttrModeWrite },
	/* 8 */ { "auto_restart",			NULL,					qtssAttrDataTypeBool16,					qtssAttrModeRead | qtssAttrModeWrite },
	/* 9 */ { "total_bytes_update",		NULL,					qtssAttrDataTypeUInt32,					qtssAttrModeRead | qtssAttrModeWrite },
	/* 10 */ { "average_bandwidth_update",	NULL,				qtssAttrDataTypeUInt32,					qtssAttrModeRead | qtssAttrModeWrite },
	/* 11 */ { "safe_play_duration",	NULL,					qtssAttrDataTypeUInt32,					qtssAttrModeRead | qtssAttrModeWrite },
	/* 12 */ { "module_folder",			NULL,					qtssAttrDataTypeCharArray,				qtssAttrModeRead | qtssAttrModeWrite },
	/* 13 */ { "error_logfile_name",	NULL,					qtssAttrDataTypeCharArray,				qtssAttrModeRead | qtssAttrModeWrite },
	/* 14 */ { "error_logfile_dir",		NULL,					qtssAttrDataTypeCharArray,				qtssAttrModeRead | qtssAttrModeWrite },
	/* 15 */ { "error_logfile_interval",NULL,					qtssAttrDataTypeUInt32,					qtssAttrModeRead | qtssAttrModeWrite },
	/* 16 */ { "error_logfile_size",	NULL,					qtssAttrDataTypeUInt32,					qtssAttrModeRead | qtssAttrModeWrite },
	/* 17 */ { "error_logfile_verbosity",NULL,					qtssAttrDataTypeUInt32,					qtssAttrModeRead | qtssAttrModeWrite },
	/* 18 */ { "screen_logging",		NULL,					qtssAttrDataTypeBool16,					qtssAttrModeRead | qtssAttrModeWrite },
	/* 19 */ { "error_logging",			NULL,					qtssAttrDataTypeBool16,					qtssAttrModeRead | qtssAttrModeWrite },
	/* 20 */ { "tcp_min_thin_delay_tolerance",			NULL,					qtssAttrDataTypeSInt32,	qtssAttrModeRead | qtssAttrModeWrite },
	/* 21 */ { "tcp_max_thin_delay_tolerance",			NULL,					qtssAttrDataTypeSInt32,	qtssAttrModeRead | qtssAttrModeWrite },
	/* 22 */ { "tcp_max_video_delay_tolerance",			NULL,					qtssAttrDataTypeSInt32,	qtssAttrModeRead | qtssAttrModeWrite },
	/* 23 */ { "tcp_max_audio_delay_tolerance",			NULL,					qtssAttrDataTypeSInt32,	qtssAttrModeRead | qtssAttrModeWrite },
	/* 24 */ { "min_tcp_buffer_size",					NULL,					qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModeWrite },
	/* 25 */ { "max_tcp_buffer_size",					NULL,					qtssAttrDataTypeUInt32,	qtssAttrModeRead | qtssAttrModeWrite },
	/* 26 */ { "tcp_seconds_to_buffer",					NULL,					qtssAttrDataTypeFloat32,qtssAttrModeRead | qtssAttrModeWrite },
	/* 27 */ { "do_report_http_connection_ip_address",	NULL,					qtssAttrDataTypeBool16,	qtssAttrModeRead | qtssAttrModeWrite },
	/* 28 */ { "default_authorization_realm",			NULL,					qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModeWrite },
	/* 29 */ { "pid_file_name",							NULL,					qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModeWrite },
	/* 30 */ { "set_user_name",							NULL,					qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModeWrite },
	/* 31 */ { "set_group_name",						NULL,					qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModeWrite },
	/* 32 */ { "append_source_addr_in_transport",		NULL,					qtssAttrDataTypeBool16,		qtssAttrModeRead | qtssAttrModeWrite },
	/* 33 */ { "rtsp_port",								NULL,					qtssAttrDataTypeUInt16,		qtssAttrModeRead | qtssAttrModeWrite },
	/* 34 */ { "max_retransmit_delay",					NULL,					qtssAttrDataTypeUInt32,		qtssAttrModeRead | qtssAttrModeWrite },
	/* 35 */ { "default_window_size",					NULL,					qtssAttrDataTypeUInt32,		qtssAttrModeRead | qtssAttrModeWrite },
	/* 36 */ { "ack_logging_enabled",					NULL,					qtssAttrDataTypeBool16,		qtssAttrModeRead | qtssAttrModeWrite },
	/* 37 */ { "rtcp_poll_interval",					NULL,					qtssAttrDataTypeUInt32,		qtssAttrModeRead | qtssAttrModeWrite },
	/* 38 */ { "rtcp_rcv_buf_size",						NULL,					qtssAttrDataTypeUInt32,		qtssAttrModeRead | qtssAttrModeWrite },
	/* 39 */ { "reliable_udp_slow_start",				NULL,					qtssAttrDataTypeBool16,		qtssAttrModeRead | qtssAttrModeWrite },
	/* 40 */ { "reliable_udp_enabled",					NULL,					qtssAttrDataTypeBool16,		qtssAttrModeRead | qtssAttrModeWrite },
	/* 41 */ { "max_overbuffer_time",					NULL,					qtssAttrDataTypeUInt32,		qtssAttrModeRead | qtssAttrModeWrite },
	/* 42 */ { "tcp_thick_min_interval",				NULL,					qtssAttrDataTypeSInt32,		qtssAttrModeRead | qtssAttrModeWrite },
	/* 43 */ { "rtp_packet_drop_pct",					NULL,					qtssAttrDataTypeUInt32,		qtssAttrModeRead | qtssAttrModeWrite },
	/* 44 */ { "share_window_stats",					NULL,					qtssAttrDataTypeBool16,		qtssAttrModeRead | qtssAttrModeWrite },
	/* 45 */ { "alt_transport_src_ipaddr",				NULL,					qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModeWrite }
};

#pragma mark __QTSS_SERVER_PREFS__

QTSServerPrefs::QTSServerPrefs(XMLPrefsParser* inPrefsSource, Bool16 inWriteMissingPrefs)
:	QTSSPrefs(inPrefsSource, NULL, QTSSDictionaryMap::GetMap(QTSSDictionaryMap::kPrefsDictIndex), false),
	fRTSPTimeoutInSecs(0),
	fRTSPTimeoutString(fRTSPTimeoutBuf, 0),
	fRealRTSPTimeoutInSecs(0),
	fRTPTimeoutInSecs(0),
	fMaximumConnections(0),
	fMaxBandwidthInKBits(0),
	fIPAddress(0),
	fBreakOnAssert(false),
	fAutoRestart(false),
	fTBUpdateTimeInSecs(0),
	fABUpdateTimeInSecs(0),
	fSafePlayDurationInSecs(0),
	fErrorRollIntervalInDays(0),
	fErrorLogBytes(0),
	fErrorLogVerbosity(0),
	fScreenLoggingEnabled(false),
	fErrorLogEnabled(false),
	fTCPMinThinDelayToleranceInMSec(0),
	fTCPMaxThinDelayToleranceInMSec(0),
	fTCPVideoDelayToleranceInMSec(0),
	fTCPAudioDelayToleranceInMSec(0),
	fTCPThickIntervalInSec(0),
	fMinTCPBufferSizeInBytes(0),
	fMaxTCPBufferSizeInBytes(0),
	fTCPSecondsToBuffer(0),
	fDoReportHTTPConnectionAddress(false),
	fAppendSrcAddrInTransport(false),
	fMaxRetransDelayInMsec(0),
	fDefaultWindowSizeInK(0),
	fIsAckLoggingEnabled(false),
	fRTCPPollIntervalInMsec(0),
	fRTCPSocketRcvBufSizeInK(0),
	fIsSlowStartEnabled(false),
	fIsReliableUDPEnabled(false),
	fMaxOverbufferTime(0),
	fRTPPacketDropPct(0),
	fShareWindowStats(0)
{
	SetupAttributes();
	RereadServerPreferences(inWriteMissingPrefs);
}

void QTSServerPrefs::Initialize()
{
	for (UInt32 x = 0; x < qtssPrefsNumParams; x++)
		QTSSDictionaryMap::GetMap(QTSSDictionaryMap::kPrefsDictIndex)->
			SetAttribute(x, sAttributes[x].fAttrName, sAttributes[x].fFuncPtr,
							sAttributes[x].fAttrDataType, sAttributes[x].fAttrPermission);
}


void QTSServerPrefs::SetupAttributes()
{
	this->SetVal(qtssPrefsRTSPTimeout, 		&fRTSPTimeoutInSecs, 		sizeof(fRTSPTimeoutInSecs));
	this->SetVal(qtssPrefsRealRTSPTimeout, 	&fRealRTSPTimeoutInSecs, 	sizeof(fRealRTSPTimeoutInSecs));
	this->SetVal(qtssPrefsRTPTimeout, 		&fRTPTimeoutInSecs, 		sizeof(fRTPTimeoutInSecs));
	this->SetVal(qtssPrefsMaximumConnections,&fMaximumConnections, 		sizeof(fMaximumConnections));
	this->SetVal(qtssPrefsMaximumBandwidth, &fMaxBandwidthInKBits, 		sizeof(fMaxBandwidthInKBits));
	this->SetVal(qtssPrefsBreakOnAssert, 	&fBreakOnAssert, 			sizeof(fBreakOnAssert));
	this->SetVal(qtssPrefsAutoRestart, 		&fAutoRestart, 				sizeof(fAutoRestart));
	this->SetVal(qtssPrefsTotalBytesUpdate, &fTBUpdateTimeInSecs, 		sizeof(fTBUpdateTimeInSecs));
	this->SetVal(qtssPrefsAvgBandwidthUpdate,&fABUpdateTimeInSecs, 		sizeof(fABUpdateTimeInSecs));
	this->SetVal(qtssPrefsSafePlayDuration, &fSafePlayDurationInSecs, 	sizeof(fSafePlayDurationInSecs));

	this->SetVal(qtssPrefsErrorRollInterval, &fErrorRollIntervalInDays, sizeof(fErrorRollIntervalInDays));
	this->SetVal(qtssPrefsMaxErrorLogSize, 	&fErrorLogBytes, 			sizeof(fErrorLogBytes));
	this->SetVal(qtssPrefsErrorLogVerbosity, &fErrorLogVerbosity, 		sizeof(fErrorLogVerbosity));
	this->SetVal(qtssPrefsScreenLogging, 	&fScreenLoggingEnabled, 	sizeof(fScreenLoggingEnabled));
	this->SetVal(qtssPrefsErrorLogEnabled, 	&fErrorLogEnabled, 			sizeof(fErrorLogEnabled));


	this->SetVal(qtssPrefsTCPMinThinDelayToleranceInMSec, 	&fTCPMinThinDelayToleranceInMSec, sizeof(fTCPMinThinDelayToleranceInMSec));
	this->SetVal(qtssPrefsTCPMaxThinDelayToleranceInMSec, 	&fTCPMaxThinDelayToleranceInMSec, sizeof(fTCPMaxThinDelayToleranceInMSec));
	this->SetVal(qtssPrefsTCPVideoDelayToleranceInMSec, 	&fTCPVideoDelayToleranceInMSec,   sizeof(fTCPVideoDelayToleranceInMSec));
	this->SetVal(qtssPrefsTCPAudioDelayToleranceInMSec, 	&fTCPAudioDelayToleranceInMSec,   sizeof(fTCPAudioDelayToleranceInMSec));

	this->SetVal(qtssPrefsMinTCPBufferSizeInBytes, 	&fMinTCPBufferSizeInBytes,	sizeof(fMinTCPBufferSizeInBytes));
	this->SetVal(qtssPrefsMaxTCPBufferSizeInBytes, 	&fMaxTCPBufferSizeInBytes,	sizeof(fMaxTCPBufferSizeInBytes));
	this->SetVal(qtssPrefsTCPSecondsToBuffer, 	&fTCPSecondsToBuffer,   		sizeof(fTCPSecondsToBuffer));

	this->SetVal(qtssPrefsDoReportHTTPConnectionAddress, 	&fDoReportHTTPConnectionAddress,   sizeof(fDoReportHTTPConnectionAddress));
	this->SetVal(qtssPrefsSrcAddrInTransport, 	&fAppendSrcAddrInTransport,   sizeof(fAppendSrcAddrInTransport));

	this->SetVal(qtssPrefsMaxRetransDelayInMsec, 	&fMaxRetransDelayInMsec,   sizeof(fMaxRetransDelayInMsec));
	this->SetVal(qtssPrefsDefaultWindowSizeInK, 	&fDefaultWindowSizeInK,   sizeof(fDefaultWindowSizeInK));
	this->SetVal(qtssPrefsAckLoggingEnabled, 		&fIsAckLoggingEnabled,   sizeof(fIsAckLoggingEnabled));
	this->SetVal(qtssPrefsRTCPPollIntervalInMsec, 	&fRTCPPollIntervalInMsec,   sizeof(fRTCPPollIntervalInMsec));
	this->SetVal(qtssPrefsRTCPSockRcvBufSizeInK, 	&fRTCPSocketRcvBufSizeInK,   sizeof(fRTCPSocketRcvBufSizeInK));
	this->SetVal(qtssPrefsRelUDPSlowStartEnabled, 	&fIsSlowStartEnabled,   sizeof(fIsSlowStartEnabled));
	this->SetVal(qtssPrefsRelUDPEnabled, 			&fIsReliableUDPEnabled,   sizeof(fIsReliableUDPEnabled));
	this->SetVal(qtssPrefsMaxOverbufferTimeInSec, 	&fMaxOverbufferTime,   sizeof(fMaxOverbufferTime));
	this->SetVal(qtssPrefsTCPThickIntervalInSec, 	&fTCPThickIntervalInSec,   sizeof(fTCPThickIntervalInSec));
	this->SetVal(qtssPrefsTCPThickIntervalInSec, 	&fTCPThickIntervalInSec,   sizeof(fTCPThickIntervalInSec));
	this->SetVal(qtssPrefsRelUDPShareWindowStats, 	&fShareWindowStats,   sizeof(fShareWindowStats));
}



void QTSServerPrefs::RereadServerPreferences(Bool16 inWriteMissingPrefs)
{
	OSMutexLocker locker(&fPrefsMutex);
	QTSSDictionaryMap* theMap = QTSSDictionaryMap::GetMap(QTSSDictionaryMap::kPrefsDictIndex);
	
	for (UInt32 x = 0; x < theMap->GetNumAttrs(); x++)
	{
		//
		// Look for a pref in the file that matches each pref in the dictionary
		const char* thePrefTypeStr = NULL;
		char* thePrefName = NULL;
		
		UInt32 theIndex = fPrefsSource->GetPrefValueIndexByName( NULL, theMap->GetAttrName(x) );
		char* thePrefValue = fPrefsSource->GetPrefValueByIndex(	theIndex, 0, &thePrefName,
																(char**)&thePrefTypeStr);
		
		if ((thePrefValue == NULL) && (x < qtssPrefsNumParams)) // Only generate errors for server prefs
		{
			//
			// There is no pref, use the default and log an error
			if (::strlen(sPrefInfo[x].fDefaultValue) > 0)
			{
				//
				// Only log this as an error if there is a default (an empty string
				// doesn't count). If there is no default, we will constantly print
				// out an error message...
				QTSSModuleUtils::LogError( 	qtssWarningVerbosity,
											qtssServerPrefMissing,
											0,
											sAttributes[x].fAttrName,
											sPrefInfo[x].fDefaultValue);
			}
			
			this->SetPrefValue(x, 0, sPrefInfo[x].fDefaultValue, sAttributes[x].fAttrDataType);
			if (sPrefInfo[x].fAdditionalDefVals != NULL)
			{
				//
				// Add additional default values if they exist
				for (UInt32 y = 0; sPrefInfo[x].fAdditionalDefVals[y] != NULL; y++)
					this->SetPrefValue(x, y+1, sPrefInfo[x].fAdditionalDefVals[y], sAttributes[x].fAttrDataType);
			}
			
			if (inWriteMissingPrefs)
			{
				//
				// Add this value into the file, cuz we need it.
				UInt32 newPrefIndex = fPrefsSource->AddPref( NULL, sAttributes[x].fAttrName, QTSSDataConverter::GetDataTypeStringForType(sAttributes[x].fAttrDataType));
				fPrefsSource->AddPrefValue(newPrefIndex, sPrefInfo[x].fDefaultValue);
				
				if (sPrefInfo[x].fAdditionalDefVals != NULL)
				{
					for (UInt32 a = 0; sPrefInfo[x].fAdditionalDefVals[a] != NULL; a++)
						fPrefsSource->AddPrefValue(newPrefIndex, sPrefInfo[x].fAdditionalDefVals[a]);
				}
			}
			continue;
		}
		
		QTSS_AttrDataType theType = QTSSDataConverter::GetDataTypeForTypeString(thePrefTypeStr);
		
		if ((x < qtssPrefsNumParams) && (theType != sAttributes[x].fAttrDataType)) // Only generate errors for server prefs
		{
			//
			// The pref in the file has the wrong type, use the default and log an error
			
			if (::strlen(sPrefInfo[x].fDefaultValue) > 0)
			{
				//
				// Only log this as an error if there is a default (an empty string
				// doesn't count). If there is no default, we will constantly print
				// out an error message...
				QTSSModuleUtils::LogError( 	qtssWarningVerbosity,
											qtssServerPrefWrongType,
											0,
											sAttributes[x].fAttrName,
											sPrefInfo[x].fDefaultValue);
			}
			
			this->SetPrefValue(x, 0, sPrefInfo[x].fDefaultValue, sAttributes[x].fAttrDataType);
			if (sPrefInfo[x].fAdditionalDefVals != NULL)
			{
				//
				// Add additional default values if they exist
				for (UInt32 z = 0; sPrefInfo[x].fAdditionalDefVals[z] != NULL; z++)
					this->SetPrefValue(x, z+1, sPrefInfo[x].fAdditionalDefVals[z], sAttributes[x].fAttrDataType);
			}

			if (inWriteMissingPrefs)
			{
				//
				// Remove it out of the file and add in the default.
				fPrefsSource->RemovePref(theIndex);
				UInt32 newPrefIndex2 = fPrefsSource->AddPref( NULL, sAttributes[x].fAttrName, QTSSDataConverter::GetDataTypeStringForType(sAttributes[x].fAttrDataType));
				fPrefsSource->AddPrefValue(newPrefIndex2, sPrefInfo[x].fDefaultValue);
				if (sPrefInfo[x].fAdditionalDefVals != NULL)
				{
					for (UInt32 b = 0; sPrefInfo[x].fAdditionalDefVals[b] != NULL; b++)
						fPrefsSource->AddPrefValue(newPrefIndex2, sPrefInfo[x].fAdditionalDefVals[b]);
				}
			}
			continue;
		}
		
		UInt32 theNumValues = 0;
		if ((x < qtssPrefsNumParams) && (!sPrefInfo[x].fAllowMultipleValues))
			theNumValues = 1;
			
		this->SetPrefValuesFromFile(theIndex, x, theNumValues);
	}

        //
        // In case we made any changes, write out the prefs file
        (void)fPrefsSource->WritePrefsFile();
}


char*	QTSServerPrefs::GetMovieFolder(char* inBuffer, UInt32* ioLen)
{
	OSMutexLocker locker(&fPrefsMutex);

	// Get the movie folder attribute
	StrPtrLen* theMovieFolder = this->GetValue(qtssPrefsMovieFolder);

	// If the movie folder path fits inside the provided buffer, copy it there
	if (theMovieFolder->Len < *ioLen)
		::memcpy(inBuffer, theMovieFolder->Ptr, theMovieFolder->Len);
	else
	{
		// Otherwise, allocate a buffer to store the path
		inBuffer = NEW char[theMovieFolder->Len + 2];
		::memcpy(inBuffer, theMovieFolder->Ptr, theMovieFolder->Len);
	}
	*ioLen = theMovieFolder->Len;
	return inBuffer;
}

void QTSServerPrefs::GetTransportSrcAddr(StrPtrLen* ioBuf)
{
	OSMutexLocker locker(&fPrefsMutex);

	// Get the movie folder attribute
	StrPtrLen* theTransportAddr = this->GetValue(qtssPrefsAltTransportIPAddr);

	// If the movie folder path fits inside the provided buffer, copy it there
	if ((theTransportAddr->Len > 0) && (theTransportAddr->Len < ioBuf->Len))
	{
		::memcpy(ioBuf->Ptr, theTransportAddr->Ptr, theTransportAddr->Len);
		ioBuf->Len = theTransportAddr->Len;
	}
	else
		ioBuf->Len = 0;
}

char* QTSServerPrefs::GetStringPref(QTSS_AttributeID inAttrID)
{
	StrPtrLen theBuffer;
	(void)this->GetValue(inAttrID, 0, NULL, &theBuffer.Len);
	theBuffer.Ptr = NEW char[theBuffer.Len + 1];
	theBuffer.Ptr[0] = '\0';
	
	if (theBuffer.Len > 0)
	{
		QTSS_Error theErr = this->GetValue(inAttrID, 0, theBuffer.Ptr, &theBuffer.Len);
		if (theErr == QTSS_NoErr)
			theBuffer.Ptr[theBuffer.Len] = 0;
	}
	return theBuffer.Ptr;
}
