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

#ifndef playlist_lists_H
#define playlist_lists_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#if (!__MACOS__)
#ifndef __Win32__
	#include <sys/types.h>
	#include <fcntl.h>
#endif
#endif

#if (__MACOS__)
	#include "bogusdefs.h"
#endif

#include "OSHeaders.h"
#include "playlist_array.h"
#include "playlist_elements.h"
#include "playlist_utils.h"



// ************************
//
// SOCKET LIST
//
// ************************


class SocketList : public ArrayList<UDPSocketPair> {
	
};

// ************************
//
// MEDIA STREAM LIST
//
// ************************


class MediaStreamList : public ArrayList<MediaStream> {

	protected:
	
	public:
		SInt16 UpdateStreams();
		void UpdateSenderReportsOnStreams();
		void SetUpStreamSSRCs();
		void StreamStarted(SInt64 startTime);
		void MovieStarted(SInt64 startTime);
		void MovieEnded(SInt64 endTime);
};

// ************************
//
// SDP MEDIA LIST
//
// ************************


class SDPMediaList : public ArrayList<TypeMap> {
		
};

#endif //playlist_lists_H
