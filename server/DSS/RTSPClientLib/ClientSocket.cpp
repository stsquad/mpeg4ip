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
	File:		ClientSocket.cpp

	
	
*/

#include "ClientSocket.h"
#include "OSMemory.h"
#include "base64.h"
#include "MyAssert.h"

#define CLIENT_SOCKET_DEBUG 0

#pragma mark __CLIENT_SOCKET__

ClientSocket::ClientSocket()
:	fHostAddr(0),
	fHostPort(0),
	fEventMask(0),
	fSocketP(NULL),
	fSendBuffer(fSendBuf, 0),
	fSentLength(0)
{}

OS_Error ClientSocket::Open(TCPSocket* inSocket)
{
	OS_Error theErr = OS_NoErr;
	if (!inSocket->IsBound())
	{
		theErr = inSocket->Open();
		if (theErr == OS_NoErr)
			theErr = inSocket->Bind(0, 0);

		if (theErr != OS_NoErr)
			return theErr;
	}
	return theErr;
}

OS_Error ClientSocket::Connect(TCPSocket* inSocket)
{
	OS_Error theErr = this->Open(inSocket);
	if (theErr != OS_NoErr)
		return theErr;

	if (!inSocket->IsConnected())
	{
		theErr = inSocket->Connect(fHostAddr, fHostPort);
		if ((theErr == EINPROGRESS) || (theErr == EAGAIN))
		{
			fSocketP = inSocket;
			fEventMask = EV_RE | EV_WR;
			return theErr;
		}
	}
	return theErr;
}

OS_Error ClientSocket::SendSendBuffer(TCPSocket* inSocket)
{
	OS_Error theErr = OS_NoErr;
	UInt32 theLengthSent = 0;
	
	do
	{
		//
		// Loop, trying to send the entire message.
		theErr = inSocket->Send(fSendBuffer.Ptr + fSentLength, fSendBuffer.Len - fSentLength, &theLengthSent);
		fSentLength += theLengthSent;
		
	} while (theLengthSent > 0);
	
	if (theErr == OS_NoErr)
		fSendBuffer.Len = fSentLength = 0; // Message was sent
	else
	{
		// Message wasn't entirely sent. Caller should wait for a read event on the POST socket
		fSocketP = inSocket;
		fEventMask = EV_WR;
	}
	return theErr;
}

#pragma mark __TCP_CLIENT_SOCKET__

TCPClientSocket::TCPClientSocket(UInt32 inSocketType)
 : fSocket(NULL, inSocketType)
{
	//
	// It is necessary to open the socket right when we construct the
	// object because the QTSSSplitterModule that uses this class uses
	// the socket file descriptor in the QTSS_CreateStreamFromSocket call.
	fSocketP = &fSocket;
	this->Open(&fSocket);
}

OS_Error TCPClientSocket::Send(const char* inData, const UInt32 inLength)
{
	if (fSendBuffer.Len == 0)
	{
		Assert(inLength < kSendBufferLen);
		::memcpy(fSendBuffer.Ptr, inData, inLength);
		fSendBuffer.Len = inLength;
	}
	
	OS_Error theErr = this->Connect(&fSocket);
	if (theErr != OS_NoErr)
		return theErr;
		
	return this->SendSendBuffer(&fSocket);
}
			
OS_Error TCPClientSocket::Read(void* inBuffer, const UInt32 inLength, UInt32* outRcvLen)
{
	this->Connect(&fSocket);
	OS_Error theErr = fSocket.Read(inBuffer, inLength, outRcvLen);
	if (theErr != OS_NoErr)
		fEventMask = EV_RE;
	return theErr;
}

#pragma mark __HTTP_CLIENT_SOCKET__

HTTPClientSocket::HTTPClientSocket(const StrPtrLen& inURL, UInt32 inCookie, UInt32 inSocketType)
: 	fCookie(inCookie),
	fSocketType(inSocketType),
	fGetReceived(0),
	
	fGetSocket(NULL, inSocketType),
	fPostSocket(NULL)
{
	fURL.Ptr = NEW char[inURL.Len + 1];
	fURL.Len = inURL.Len;
	::memcpy(fURL.Ptr, inURL.Ptr, inURL.Len);
	fURL.Ptr[fURL.Len] = '\0';
}

HTTPClientSocket::~HTTPClientSocket()
{
	delete [] fURL.Ptr;
}

OS_Error HTTPClientSocket::Read(void* inBuffer, const UInt32 inLength, UInt32* outRcvLen)
{
	//
	// Bring up the GET connection if we need to
	if (!fGetSocket.IsConnected())
	{
#if CLIENT_SOCKET_DEBUG
		printf("HTTPClientSocket::Read: Sending GET\n");
#endif
		::sprintf(fSendBuffer.Ptr, "GET %s HTTP/1.0\r\nX-SessionCookie: %lu\r\nAccept: application/x-rtsp-rtp-interleaved\r\nUser-Agent: QTSS/2.0\r\n\r\n", fURL.Ptr, fCookie);
		fSendBuffer.Len = ::strlen(fSendBuffer.Ptr);
		Assert(fSentLength == 0);
	}

	OS_Error theErr = this->Connect(&fGetSocket);
	if (theErr != OS_NoErr)
		return theErr;
	
	if (fSendBuffer.Len > 0)
	{
		theErr = this->SendSendBuffer(&fGetSocket);
		if (theErr != OS_NoErr)
			return theErr;
		fSentLength = 1; // So we know to execute the receive code below.
	}
	
	// We are done sending the GET. If we need to receive the GET response, do that here
	if (fSentLength > 0)
	{
		*outRcvLen = 0;
		do
		{
			// Loop, trying to receive the entire response.
			theErr = fGetSocket.Read(&fSendBuffer.Ptr[fGetReceived], kSendBufferLen - fGetReceived, outRcvLen);
			fGetReceived += *outRcvLen;
			
			// Check to see if we've gotten a \r\n\r\n. If we have, then we've received
			// the entire GET
			fSendBuffer.Ptr[fGetReceived] = '\0';
			char* theGetEnd = ::strstr(fSendBuffer.Ptr, "\r\n\r\n");

			if (theGetEnd != NULL)
			{
				// We got the entire GET response, so we are ready to move onto
				// real RTSP response data. First skip past the \r\n\r\n
				theGetEnd += 4;

#if CLIENT_SOCKET_DEBUG
				printf("HTTPClientSocket::Read: Received GET response\n");
#endif
				
				// Whatever remains is part of an RTSP request, so move that to
				// the beginning of the buffer and blow away the GET
				*outRcvLen = fGetReceived - (theGetEnd - fSendBuffer.Ptr);
				::memcpy(inBuffer, theGetEnd, *outRcvLen);
				fGetReceived = fSentLength = 0;
				return OS_NoErr;
			}
			
			Assert(fGetReceived < inLength);
		} while (*outRcvLen > 0);
		
#if CLIENT_SOCKET_DEBUG
		printf("HTTPClientSocket::Read: Waiting for GET response\n");
#endif
		// Message wasn't entirely received. Caller should wait for a read event on the GET socket
		Assert(theErr != OS_NoErr);
		fSocketP = &fGetSocket;
		fEventMask = EV_RE;
		return theErr;
	}
	
	theErr = fGetSocket.Read(&((char*)inBuffer)[fGetReceived], inLength - fGetReceived, outRcvLen);
	if (theErr != OS_NoErr)
	{
#if CLIENT_SOCKET_DEBUG
		printf("HTTPClientSocket::Read: Waiting for data\n");
#endif
		fSocketP = &fGetSocket;
		fEventMask = EV_RE;
	}
#if CLIENT_SOCKET_DEBUG
	else
		printf("HTTPClientSocket::Read: Got some data\n");
#endif
	return theErr;
}

OS_Error HTTPClientSocket::Send(const char* inData, const UInt32 inLength)
{
	//
	// Bring up the POST connection if we need to
	if (fPostSocket == NULL)
		fPostSocket = NEW TCPSocket(NULL, fSocketType);
		
	if (!fPostSocket->IsConnected())
	{
#if CLIENT_SOCKET_DEBUG
		printf("HTTPClientSocket::Send: Sending POST\n");
#endif
		::sprintf(fSendBuffer.Ptr, "POST %s HTTP/1.0\r\nX-SessionCookie: %lu\r\nAccept: application/x-rtsp-rtp-interleaved\r\nUser-Agent: QTSS/2.0\r\n\r\n", fURL.Ptr, fCookie);
		fSendBuffer.Len = ::strlen(fSendBuffer.Ptr);
		fSendBuffer.Len += ::Base64encode(fSendBuffer.Ptr + fSendBuffer.Len, inData, inLength);
		Assert(fSendBuffer.Len < ClientSocket::kSendBufferLen);
		fSendBuffer.Len = ::strlen(fSendBuffer.Ptr); //Don't trust what the above function returns for a length
	}
	
	OS_Error theErr = this->Connect(fPostSocket);
	if (theErr != OS_NoErr)
		return theErr;

	//
	// If we have nothing to send currently, this should be a new message, in which case
	// we can encode it and send it
	if (fSendBuffer.Len == 0)
	{
		fSendBuffer.Len = ::Base64encode(fSendBuffer.Ptr, inData, inLength);
		Assert(fSendBuffer.Len < ClientSocket::kSendBufferLen);
		fSendBuffer.Len = ::strlen(fSendBuffer.Ptr); //Don't trust what the above function returns for a length
	}
	
#if CLIENT_SOCKET_DEBUG
	printf("HTTPClientSocket::Send: Sending data\n");
#endif
	return this->SendSendBuffer(fPostSocket);
}
