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
	File:		RTSPResponseStream.h

	Contains:	Object that provides a "buffered WriteV" service. Clients
				can call this function to write to a socket, and buffer flow
				controlled data in different ways.
				
				It is derived from StringFormatter, which it uses as an output
				stream buffer. The buffer may grow infinitely.
*/

#ifndef __RTSP_RESPONSE_STREAM_H__
#define __RTSP_RESPONSE_STREAM_H__

#include "ResizeableStringFormatter.h"
#include "TCPSocket.h"
#include "TimeoutTask.h"
#include "QTSS.h"

class RTSPResponseStream : public ResizeableStringFormatter
{
	public:
	
		// This object provides some flow control buffering services.
		// It also refreshes the timeout whenever there is a successful write
		// on the socket.
		RTSPResponseStream(TCPSocket* inSocket, TimeoutTask* inTimeoutTask)
			: 	ResizeableStringFormatter(fOutputBuf, kOutputBufferSizeInBytes),
				fSocket(inSocket), fBytesSentInBuffer(0), fTimeoutTask(inTimeoutTask) {}
		
		virtual ~RTSPResponseStream() {}

		// WriteV
		//
		// This function takes an input ioVec and writes it to the socket. If any
		// data has been written to this stream via Put, that data gets written first.
		//
		// In the event of flow control on the socket, less data than what was
		// requested, or no data at all, may be sent. Specify what you want this
		// function to do with the unsent data via inSendType.
		//
		// kAlwaysBuffer: 	Buffer any unsent data internally.
		// kAllOrNothing: 	If no data could be sent, return EWOULDBLOCK. Otherwise,
		//					buffer any unsent data.
		// kDontBuffer:		Never buffer any data.
		//
		// If some data ends up being buffered, outLengthSent will = inTotalLength,
		// and the return value will be QTSS_NoErr 
		
		enum
		{
			kDontBuffer		= 0,
			kAllOrNothing	= 1,
			kAlwaysBuffer	= 2
		};
		QTSS_Error WriteV(iovec* inVec, UInt32 inNumVectors, UInt32 inTotalLength,
								UInt32* outLengthSent, UInt32 inSendType);

		// Flushes any buffered data to the socket. If all data could be sent,
		// this returns QTSS_NoErr, otherwise, it returns EWOULDBLOCK
		QTSS_Error Flush();
		
	private:
	
		enum
		{
			kOutputBufferSizeInBytes = 512	//UInt32
		};
		
		//The default buffer size is allocated inline as part of the object. Because this size
		//is good enough for 99.9% of all requests, we avoid the dynamic memory allocation in most
		//cases. But if the response is too big for this buffer, the BufferIsFull function will
		//allocate a larger buffer.
		char					fOutputBuf[kOutputBufferSizeInBytes];
		TCPSocket*				fSocket;
		UInt32					fBytesSentInBuffer;
		TimeoutTask*			fTimeoutTask;
		
		friend class RTSPRequestInterface;
};

#endif // __RTSP_RESPONSE_STREAM_H__
