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
// SimpleParse:

#ifndef SDPGen_H
#define SDPGen_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#if (__MACOS__)
	#include "bogusdefs.h"
#endif

#if (! __MACOS__)
#ifndef __Win32__
	#include <sys/types.h>
	#include <sys/time.h>
	#include <sys/errno.h>
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <sys/utsname.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <sys/ioctl.h>
	#include <unistd.h>
	#include <netinet/in.h>
	#include <net/if.h>
#endif
#endif
#include "playlist_SimpleParse.h"
#include "QTRTPFile.h"

class SDPGen
{
	enum {eMaxSDPFileSize = 10240};
#if __MACOS__
	enum {ePath_Separator = ':', eExtension_Separator = '.'};
#else
	enum {ePath_Separator = '/', eExtension_Separator = '.'};

#endif
	
	private:
		char *fSDPFileContentsBuf;
		
	protected: 
		QTRTPFile		fRTPFile;
				bool	fKeepTracks;
				bool	fAddIndexTracks;
		 		short 	AddToBuff(char *aSDPFile, short currentFilePos, char *chars);
		 		UInt32	RandomTime(void);
		  		short	GetNoExtensionFileName(char *pathName, char *result, short maxResult);
				char 	*Process(	char *sdpFileName, 
									char * basePort, 
									char *ipAddress, 
									char *anSDPBuffer, 
									char *startTime,
				 					char *endTime,	
				 					char *isDynamic,
									int *error
								);
		
	public: 
			 SDPGen() : fSDPFileContentsBuf(NULL), fKeepTracks(false),fAddIndexTracks(false), fSDPFileCreated(false),fClientBufferDelay(0.0) {	QTRTPFile::Initialize(); };
			~SDPGen() { if (fSDPFileContentsBuf) delete fSDPFileContentsBuf; };
			void KeepSDPTracks(bool enabled) { fKeepTracks = enabled;} ;
			void AddIndexTracks(bool enabled) { fAddIndexTracks = enabled;} ;
			void SetClientBufferDelay(Float32 bufferDelay) { fClientBufferDelay = bufferDelay;} ;
			int Run(  char *movieFilename
				, char *sdpFilename
				, char *basePort
				, char *ipAddress
				, char *buff
				, short buffSize
				, bool overWriteSDP
				, bool forceNewSDP
				, char *startTime
				, char *endTime
				, char *isDynamic
				); 
				
			bool	fSDPFileCreated;
			Float32 fClientBufferDelay;
};

#endif

