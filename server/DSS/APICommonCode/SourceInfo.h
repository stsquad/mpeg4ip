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
	File:		SourceInfo.h

	Contains:	This object contains an interface for getting at any bit
				of "interesting" information regarding a content source in a
				format - independent manner.
				
				For instance, the derived object SDPSourceInfo parses an
				SDP file and retrieves all the SourceInfo information from that file.
					
	

*/

#ifndef __SOURCE_INFO_H__
#define __SOURCE_INFO_H__

#include "QTSS.h"
#include "StrPtrLen.h"
#include "OSQueue.h"

class SourceInfo
{
	public:
	
		SourceInfo() : 	fStreamArray(NULL), fNumStreams(0),
						fOutputArray(NULL), fNumOutputs(0) {}
		virtual ~SourceInfo() {}// Relies on derived classes to delete any allocated memory
		
		enum
		{
			eDefaultBufferDelay = 3
		};
		
		// Returns whether this source is reflectable.
		Bool16	IsReflectable();

		// Each source is comprised of a set of streams. Those streams have
		// the following metadata.
		struct StreamInfo
		{
			StreamInfo() : fSrcIPAddr(0), fDestIPAddr(0), fPort(0), fTimeToLive(0), fPayloadType(0), fTrackID(0), fBufferDelay((Float32) eDefaultBufferDelay), fIsTCP(false){}
			StreamInfo(const StreamInfo& copy);// Doesn't copy dynamically allocated data
			~StreamInfo() {}
			
			UInt32 fSrcIPAddr;	// Src IP address of content (this may be 0 if not known for sure)
			UInt32 fDestIPAddr;	// Dest IP address of content (destination IP addr for source broadcast!)
			UInt16 fPort;		// Dest (RTP) port of source content
			UInt16 fTimeToLive; // Ttl for this stream
			QTSS_RTPPayloadType fPayloadType;	// Payload type of this stream
			StrPtrLen fPayloadName; // Payload name of this stream
			UInt32 fTrackID;	// ID of this stream
			Float32 fBufferDelay; // buffer delay (default is 3 seconds)
			Bool16	fIsTCP; 	// Is this a TCP broadcast? If this is the case, the port and ttl are not valid
		};
		
		// Returns the number of StreamInfo objects (number of Streams in this source)
		UInt32		GetNumStreams() { return fNumStreams; }
		StreamInfo*	GetStreamInfo(UInt32 inStreamIndex);
		StreamInfo*	GetStreamInfoByTrackID(UInt32 inTrackID);
		
		// If this source is to be Relayed, it may have "Output" information. This
		// tells the reader where to forward the incoming streams onto. There may be
		// 0 -> N OutputInfo objects in this SourceInfo. Each OutputInfo refers to a
		// single, complete copy of ALL the input streams. The fPortArray field
		// contains one RTP port for each incoming stream.
		struct OutputInfo
		{
			OutputInfo() : fDestAddr(0), fLocalAddr(0), fTimeToLive(0), fPortArray(NULL), fAlreadySetup(false) {}
			OutputInfo(const OutputInfo& copy);// Doesn't copy dynamically allocated data
			~OutputInfo() {}
			
			// Returns true if the two are equal
			Bool16 Equal(const OutputInfo& info);

			UInt32 fDestAddr;		// Destination address to forward the input onto
			UInt32 fLocalAddr;		// Address of local interface to send out on (may be 0)
			UInt16 fTimeToLive;		// Time to live for resulting output (if multicast)
			UInt16* fPortArray;		// 1 destination RTP port for each Stream.
			Bool16	fAlreadySetup;	// A flag used in QTSSReflectorModule.cpp
		};

		// Returns the number of OutputInfo objects.
		UInt32		GetNumOutputs() { return fNumOutputs; }
		UInt32 		GetNumNewOutputs(); // Returns # of outputs not already setup

		OutputInfo*	GetOutputInfo(UInt32 inOutputIndex);
		
		// GetLocalSDP. This may or may not be supported by sources. Typically, if
		// the source is reflectable, this must be supported. It returns a newly
		// allocated buffer (that the caller is responsible for) containing an SDP
		// description of the source, stripped of all network info.
		virtual char*	GetLocalSDP(UInt32* /*newSDPLen*/) { return NULL; }
		
	protected:
		
		//utility function used by IsReflectable
		Bool16 IsReflectableIPAddr(UInt32 inIPAddr);

		StreamInfo*	fStreamArray;
		UInt32		fNumStreams;
		
		OutputInfo* fOutputArray;
		UInt32 		fNumOutputs;
};

#endif //__SOURCE_INFO_H__
