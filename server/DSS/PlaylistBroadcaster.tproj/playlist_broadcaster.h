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

#ifndef playlist_broadcaster_H
#define playlist_broadcaster_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "OSHeaders.h"
#include "playlist_utils.h"
#include "playlist_elements.h"
#include "playlist_lists.h"
#include "playlist_parsers.h"
#include "QTRTPFile.h"
#include "PLBroadcastDef.h"

#if (!__MACOS__)
#ifndef __Win32__
	#include <sys/types.h>
	#include <fcntl.h>
#endif
#endif

#if (__MACOS__)
	#include "bogusdefs.h"
#endif

static PlayListUtils gUtils;


class QTFileBroadcaster 
{

protected:
	
	QTRTPFile				*fRTPFilePtr ;
	SDPFileParser 			fStreamSDPParser;
	SDPFileParser 			*fMovieSDPParser;
	SocketList 				fSocketlist;
	MediaStreamList			fMediaStreamList;
	int						fBasePort;
	bool					fDebug;
	bool					fDeepDebug;

// transmit time trackers
	Float64					fLastTransmitTime;
	
	SInt64					fStreamStartTime;
	SInt64					fMovieStartTime;
	SInt64					fMovieEndTime;
	SInt64					fMovieIntervalTime;
	SInt64					fMovieTimeDiffMilli;
	
	bool					fMovieStart;
	Float64					fSendTimeOffset;
	Float64					fMovieDuration;
	int						fMovieTracks;
	int						fMappedMovieTracks;
	UInt64					fNumMoviesPlayed;
		
	PayLoad * 				FindPayLoad(short id, ArrayList<PayLoad> *PayLoadListPtr);
	bool 					CompareRTPMaps(TypeMap *movieMediaTypePtr, TypeMap *streamMediaTypePtr, short id);
	bool					CompareMediaTypes(TypeMap *movieMediaTypePtr, TypeMap *streamMediaTypePtr);
	UInt32 					GetSDPTracks(QTRTPFile *newRTPFilePtr);
	int 					SetUpAMovie(char *movieFileName);
	int 					AddTrackAndStream(QTRTPFile *newRTPFilePtr);
	int 					MapMovieToStream();
	int 					Play();
	Float64 				Sleep(Float64 transmitTime);
	void 					SetDebug(bool debug) {fDebug = debug;};
	void 					SetDeepDebug(bool debug) {fDeepDebug = debug;};
	
public:
		QTFileBroadcaster();
		~QTFileBroadcaster();
	
	
static	int		EvalErrorCode(QTRTPFile::ErrorCode err);
		int		SetUp(PLBroadcastDef *broadcastDefPtr) ;
		int 	PlayMovie(char *movieFileName);
		int 	GetMovieTrackCount() { return fMovieTracks; };
		int		GetMappedMovieTrackCount() { return fMappedMovieTracks; };
	
		bool	fPlay;
		bool	fSend;
	
	enum { eClientBufferSecs = 0 };
	
	enum ErrorID 
	{	// General errors
		 eNoErr 			= 0
		,eParam  			= 1			
		,eMem	 			= 2  		
		,eInternalError 	= 3
		
		// Setup Errors
		,eNoAvailableSockets = 4 	
		,eSDPFileNotFound	 = 5
		,eSDPDestAddrInvalid = 6	
		,eSDPFileInvalid 	 = 7
		,eSDPFileNoMedia 	 = 8
		,eSDPFileNoPorts 	 = 9
		,eSDPFileInvalidPort = 10
		,eSDPFileInvalidName = 11	
		// eMem also
		
		// Play Errors,
		,eMovieFileNotFound	 		= 12
		,eMovieFileNoHintedTracks 	= 13
		,eMovieFileNoSDPMatches 	= 14
		,eMovieFileInvalid 			= 15
		,eMovieFileInvalidName 		= 16	
	};

	
private:

};

#endif //playlist_broadcaster_H
