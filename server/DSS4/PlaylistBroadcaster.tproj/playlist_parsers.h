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

#ifndef playlist_parsers_H
#define playlist_parsers_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "playlist_elements.h"
#include "OSHeaders.h"
#include "playlist_SimpleParse.h"
#include "playlist_utils.h"
#include "playlist_lists.h"

#if (!__MACOS__)
#ifndef __Win32__
	#include <sys/types.h>
#endif
#endif

#if (__MACOS__) 
	#include "bogusdefs.h"
#endif



class TextLine : public SimpleParser 
{
	public: 
		ArrayList<SimpleString> fWords;
		int Parse (SimpleString *textStrPtr);
};



class LineAndWordsParser : public SimpleParser 
{
	public: 
		ArrayList<TextLine> fLines;
		int Parse (SimpleString *textStrPtr);
};


class SDPFileParser : SimpleParser
{
	public:
				SDPFileParser(void) : fNumTracks(0), fSDPBuff(NULL)  {}
				~SDPFileParser(void);
	SInt32		ReadSDP(char *theFile);
	SInt32 		ParseSDP(char *theBuff) ;
	SInt32		GetNumTracks() {return fNumTracks;} ;
	bool 		IsCommented(SimpleString *aLine);
	TextLine* 	GetRTPMapLines(TextLine *theLinePtr,TypeMap *theTypeMapPtr);
	bool 		GetRTPMap(TextLine *theLinePtr,PayLoad *payloadPtr);
	bool 		GetMediaFromLine(TextLine *theLinePtr, TypeMap *theTypeMapPtr);
	void 		GetPayLoadsFromLine(TextLine *theLinePtr, TypeMap *theTypeMapPtr);
	TextLine* 	GetTrackID(TextLine *theLinePtr,TypeMap *theTypeMapPtr);
	bool 		ParseIPString(TextLine *theLinePtr);
	SimpleString* GetIPString() { return &fIPAddressString; };
	
	ArrayList<SimpleString> fQTTextLines;
	SInt32		GetNumQTTextLines() {return fNumQTTextLines;} ;
	bool 		GetQTTextFromLine(TextLine *theLinePtr);
	
	SDPMediaList fSDPMediaList;
	protected:
	
	UInt32	TimeScaleLookUp(int payLoadID, SimpleString *typeStringPtr);
	short 	CountMediaEntries() ;
	short   CountRTPMapEntries() ;
	short	CountQTTextLines();
	
	UInt32 	fNumQTTextLines;
	UInt32 	fNumTracks;
	LineAndWordsParser 	fParser;
	SimpleString	    fIPAddressString;
	private: 
		enum { cMaxBytes = 4096}; // maximum accepted sdp file size
		char *fSDPBuff;
};


#endif //playlist_parsers_H
