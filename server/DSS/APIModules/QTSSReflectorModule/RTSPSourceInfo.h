/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
	File:		RTSPSourceInfo.h

	Contains:	

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	
*/

#ifndef __RTSP_SOURCE_INFO_H__
#define __RTSP_SOURCE_INFO_H__

#include "QTSS.h"
#include "StrPtrLen.h"
#include "RCFSourceInfo.h"
#include "RTSPClient.h"
#include "RelayPrefsSource.h"
#include "ClientSocket.h"

class RTSPSourceInfo : public RCFSourceInfo
{
	public:
	
		// Specify whether the client should be blocking or non-blocking
		RTSPSourceInfo(UInt32 inSocketType) : 	fClientSocket(inSocketType),
												fClient(&fClientSocket, false),
												fNumSetupsComplete(0),
												fDescribeComplete(false)  {}
		virtual ~RTSPSourceInfo() {}
		
		// Call this immediately after the constructor. This object will parse
		// the config file and extract the necessary information to connect to an rtsp server.
		// Specify the config file line index where the "rtsp_source" line resides
		QTSS_Error	ParsePrefs(		RelayPrefsSource* inPrefsSource, UInt32 inStartIndex);

		// Connects, sends a DESCRIBE, and parses the incoming SDP data. After this
		// function completes sucessfully, GetLocalSDP returns the data, and the
		// SourceInfo & DestInfo arrays will be set up. Also sends SETUPs for all the
		// tracks, and finishes by issuing a PLAY.
		//
		// These functions return QTSS_NoErr if the transaction has completed
		// successfully. Otherwise, they return:
		//
		// EAGAIN: the transaction is still in progress, the call should be reissued
		// QTSS_RequestFailed: the remote host responded with an error.
		// Any other error means that the remote host was unavailable or refused the connection
		QTSS_Error	Describe();
		QTSS_Error	SetupAndPlay();
		
		// This function works the same way as the above ones, and should be
		// called before destroying the object to let the remote host know that
		// we are going away.
		QTSS_Error	Teardown();

		// This function uses the Parsed SDP file, and strips out all the network information,
		// producing an SDP file that appears to be local.
		virtual char*	GetLocalSDP(UInt32* newSDPLen);
		virtual StrPtrLen*	GetSourceID() { return fClient.GetURL(); }
		
		// This object looks for this keyword in the FilePrefsSource, where it
		// expects the IP address, port, and URL.
		static StrPtrLen&	GetRTSPSourceString() { return sKeyString; }
		
		RTSPClient*	GetRTSPClient() 	{ return &fClient; }
		Bool16		IsDescribeComplete(){ return fDescribeComplete; }
		
	private:

		TCPClientSocket	fClientSocket;
		RTSPClient		fClient;
		UInt32		fNumSetupsComplete;
		Bool16		fDescribeComplete;
		StrPtrLen	fLocalSDP;
		
		static StrPtrLen sKeyString;
};
#endif // __RTSP_SOURCE_INFO_H__

