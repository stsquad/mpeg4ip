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
	File:		RTSPRequestInterface.cp

	Contains:	Implementation of class defined in RTSPRequestInterface.h
	

	

*/


//INCLUDES:
#ifndef __Win32__
#include <sys/types.h>
#include <sys/uio.h>
#endif

#include "RTSPRequestInterface.h"
#include "RTSPSessionInterface.h"
#include "RTSPRequestStream.h"

#include "StringParser.h"
#include "OSMemory.h"
#include "OSThread.h"
#include "DateTranslator.h"

#include "QTSSPrefs.h"
#include "QTSServerInterface.h"

char 		RTSPRequestInterface::sPremadeHeader[kStaticHeaderSizeInBytes];
StrPtrLen	RTSPRequestInterface::sPremadeHeaderPtr(sPremadeHeader, kStaticHeaderSizeInBytes);
StrPtrLen	RTSPRequestInterface::sColonSpace(": ", 2);

QTSSAttrInfoDict::AttrInfo	RTSPRequestInterface::sAttributes[] =
{   /*fields:   fAttrName, fFuncPtr, fAttrDataType, fAttrPermission */
	/* 0 */ { "qtssRTSPReqFullRequest",			NULL,					qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 1 */ { "qtssRTSPReqMethodStr",			NULL,					qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 2 */ { "qtssRTSPReqFilePath",			NULL,					qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 3 */ { "qtssRTSPReqURI",					NULL,					qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 4 */ { "qtssRTSPReqFilePathTrunc",		GetTruncatedPath,		qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 5 */ { "qtssRTSPReqFileName",			GetFileName,			qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 6 */ { "qtssRTSPReqFileDigit",			GetFileDigit,			qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 7 */ { "qtssRTSPReqAbsoluteURL",			NULL,					qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 8 */ { "qtssRTSPReqTruncAbsoluteURL",	GetAbsTruncatedPath,	qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 9 */ { "qtssRTSPReqMethod",				NULL,					qtssAttrDataTypeUInt32,		qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 10 */ { "qtssRTSPReqStatusCode",			NULL,					qtssAttrDataTypeUInt32,		qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite },
	/* 11 */ { "qtssRTSPReqStartTime",			NULL,					qtssAttrDataTypeFloat64,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 12 */ { "qtssRTSPReqStopTime",			NULL,					qtssAttrDataTypeFloat64,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 13 */ { "qtssRTSPReqRespKeepAlive",		NULL,					qtssAttrDataTypeBool16,		qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite },
	/* 14 */ { "qtssRTSPReqRootDir",			NULL,					qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite },
	/* 15 */ { "qtssRTSPReqRealStatusCode",		GetRealStatusCode,		qtssAttrDataTypeUInt32,		qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 16 */ { "qtssRTSPReqStreamRef",			NULL,					qtssAttrDataTypeQTSS_StreamRef,	qtssAttrModeRead | qtssAttrModePreempSafe },
	
	/* 17 */ { "qtssRTSPReqUserName",			NULL,					qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 18 */ { "qtssRTSPReqUserPassword",		NULL,					qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 19 */ { "qtssRTSPReqUserAllowed",		NULL,					qtssAttrDataTypeBool16,		qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite },
	/* 20 */ { "qtssRTSPReqURLRealm",			NULL,					qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite },
	/* 21 */ { "qtssRTSPReqLocalPath",			NULL,					qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 22 */ { "qtssRTSPReqIfModSinceDate",		NULL,					qtssAttrDataTypeTimeVal,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 23 */ { "qtssRTSPReqQueryString",		NULL,					qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 24 */ { "qtssRTSPReqRespMsg",			NULL,					qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite },
	/* 25 */ { "qtssRTSPReqContentLen",			NULL,					qtssAttrDataTypeUInt32,		qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 26 */ { "qtssRTSPReqSpeed",				NULL,					qtssAttrDataTypeFloat32,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 27 */ { "qtssRTSPReqLateTolerance",		NULL,					qtssAttrDataTypeFloat32,	qtssAttrModeRead | qtssAttrModePreempSafe }
};



void  RTSPRequestInterface::Initialize()
{
	//make a partially complete header
	StringFormatter headerFormatter(sPremadeHeaderPtr.Ptr, kStaticHeaderSizeInBytes);
	PutStatusLine(&headerFormatter, qtssSuccessOK, RTSPProtocol::k10Version);
	headerFormatter.Put(QTSServerInterface::GetServerHeader());
	headerFormatter.PutEOL();
	headerFormatter.Put(RTSPProtocol::GetHeaderString(qtssCSeqHeader));
	headerFormatter.Put(sColonSpace);
	sPremadeHeaderPtr.Len = headerFormatter.GetCurrentOffset();
	Assert(sPremadeHeaderPtr.Len < kStaticHeaderSizeInBytes);
	
	//Setup all the dictionary stuff
	for (UInt32 x = 0; x < qtssRTSPReqNumParams; x++)
		QTSSDictionaryMap::GetMap(QTSSDictionaryMap::kRTSPRequestDictIndex)->
			SetAttribute(x, sAttributes[x].fAttrName, sAttributes[x].fFuncPtr,
							sAttributes[x].fAttrDataType, sAttributes[x].fAttrPermission);
	
	QTSSDictionaryMap* theHeaderMap = QTSSDictionaryMap::GetMap(QTSSDictionaryMap::kRTSPHeaderDictIndex);
	for (UInt32 y = 0; y < qtssNumHeaders; y++)
		theHeaderMap->SetAttribute(y, RTSPProtocol::GetHeaderString(y).Ptr, NULL, qtssAttrDataTypeCharArray, qtssAttrModeRead | qtssAttrModePreempSafe);
}

//CONSTRUCTOR / DESTRUCTOR: very simple stuff
RTSPRequestInterface::RTSPRequestInterface(RTSPSessionInterface *session)
:	QTSSDictionary(QTSSDictionaryMap::GetMap(QTSSDictionaryMap::kRTSPRequestDictIndex)),
	//fMethod(0), //parameter need not be set
	fStatus(qtssSuccessOK), //parameter need not be set
	fRealStatusCode(0),
	fRequestKeepAlive(true),
	//fResponseKeepAlive(true), //parameter need not be set
	fVersion(RTSPProtocol::k10Version),
	fStartTime(-1),
	fStopTime(-1),
	fClientPortA(0),
	fClientPortB(0),
	fTtl(0),
	fDestinationAddr(0),
	fSourceAddr(0),
	fTransportType(qtssRTPTransportTypeUDP),
	fContentLength(0),
	fIfModSinceDate(0),
	fSpeed(0),
	fLateTolerance(-1),
	fFullPath(NULL),
	fStreamRef(this),
	fWindowSize(0),
	fAckTimeout(0),
	fMovieFolderPtr(&fMovieFolderPath[0]),
	fHeaderDictionary(QTSSDictionaryMap::GetMap(QTSSDictionaryMap::kRTSPHeaderDictIndex)),
	fAllowed(true),
	fSession(session),
	fOutputStream(session->GetOutputStream()),
	fStandardHeadersWritten(false)
{
	//Setup QTSS parameters that can be setup now. These are typically the parameters that are actually
	//pointers to binary variable values. Because these variables are just member variables of this object,
	//we can properly initialize their pointers right off the bat.
	RTSPRequestStream* input = session->GetInputStream();
	this->SetVal(qtssRTSPReqFullRequest, input->GetRequestBuffer()->Ptr, input->GetRequestBuffer()->Len);
	this->SetVal(qtssRTSPReqMethod, &fMethod, sizeof(fMethod));
	this->SetVal(qtssRTSPReqStatusCode, &fStatus, sizeof(fStatus));
	this->SetVal(qtssRTSPReqRespKeepAlive, &fResponseKeepAlive, sizeof(fResponseKeepAlive));
	this->SetVal(qtssRTSPReqStreamRef, &fStreamRef, sizeof(fStreamRef));
	this->SetVal(qtssRTSPReqContentLen, &fContentLength, sizeof(fContentLength));
	this->SetVal(qtssRTSPReqSpeed, &fSpeed, sizeof(fSpeed));
	this->SetVal(qtssRTSPReqLateTolerance, &fLateTolerance, sizeof(fLateTolerance));
	
	// Get the default root directory from QTSSPrefs, and store that in the proper parameter
	// Note that the GetMovieFolderPath function may allocate memory, so we check for that
	// in this object's destructor and free that memory if necessary.
	UInt32 pathLen = kMovieFolderBufSizeInBytes;
	fMovieFolderPtr = QTSServerInterface::GetServer()->GetPrefs()->GetMovieFolder(fMovieFolderPtr, &pathLen);
	this->SetVal(qtssRTSPReqRootDir, fMovieFolderPtr, pathLen);
	
	//There are actually other attributes that point to member variables that we COULD setup now, but they are attributes that
	//typically aren't set for every request, so we lazy initialize those when we parse the request

	this->SetVal(qtssRTSPReqUserAllowed, &fAllowed, sizeof(fAllowed));

}

void RTSPRequestInterface::AppendHeader(QTSS_RTSPHeader inHeader, StrPtrLen* inValue)
{
	if (!fStandardHeadersWritten)
		this->WriteStandardHeaders();
		
	fOutputStream->Put(RTSPProtocol::GetHeaderString(inHeader));
	fOutputStream->Put(sColonSpace);
	fOutputStream->Put(*inValue);
	fOutputStream->PutEOL();
}

void RTSPRequestInterface::PutStatusLine(StringFormatter* putStream, QTSS_RTSPStatusCode status,
										RTSPProtocol::RTSPVersion version)
{
	putStream->Put(RTSPProtocol::GetVersionString(version));
	putStream->PutSpace();
	putStream->Put(RTSPProtocol::GetStatusCodeAsString(status));
	putStream->PutSpace();
	putStream->Put(RTSPProtocol::GetStatusCodeString(status));
	putStream->PutEOL();	
}

void RTSPRequestInterface::AppendDateAndExpires()
{
	if (!fStandardHeadersWritten)
		this->WriteStandardHeaders();

	Assert(OSThread::GetCurrent() != NULL);
	DateBuffer* theDateBuffer = OSThread::GetCurrent()->GetDateBuffer();
	theDateBuffer->InexactUpdate(); // Update the date buffer to the current date & time
	StrPtrLen theDate(theDateBuffer->GetDateBuffer(), DateBuffer::kDateBufferLen);
	
	// Append dates, and have this response expire immediately
	this->AppendHeader(qtssDateHeader, &theDate);
	this->AppendHeader(qtssExpiresHeader, &theDate);
}


void RTSPRequestInterface::AppendSessionHeaderWithTimeout( StrPtrLen* inSessionID, StrPtrLen* inTimeout )
{

	// Append a session header if there wasn't one already
	if ( GetHeaderDictionary()->GetValue(qtssSessionHeader)->Len == 0)
	{	
		if (!fStandardHeadersWritten)
			this->WriteStandardHeaders();

		static StrPtrLen	sTimeoutString(";timeout=");

		// Just write out the session header and session ID
		fOutputStream->Put( RTSPProtocol::GetHeaderString(qtssSessionHeader ) );
		fOutputStream->Put(sColonSpace);
		fOutputStream->Put( *inSessionID );
		
		
		if ( inTimeout != NULL )
		{
			fOutputStream->Put( sTimeoutString );
			fOutputStream->Put( *inTimeout );
		}
		
		
		fOutputStream->PutEOL();
	}
}

void RTSPRequestInterface::AppendTransportHeader(StrPtrLen* serverPortA,
													StrPtrLen* serverPortB,
													StrPtrLen* channelA,
													StrPtrLen* channelB,
													StrPtrLen* serverIPAddr)
{
	static StrPtrLen	sServerPortString(";server_port=");
	static StrPtrLen	sSourceString(";source=");
	static StrPtrLen	sInterleavedString(";interleaved=");

	if (!fStandardHeadersWritten)
		this->WriteStandardHeaders();

	// Just write out the same transport header the client sent to us.
	fOutputStream->Put(RTSPProtocol::GetHeaderString(qtssTransportHeader));
	fOutputStream->Put(sColonSpace);
	fOutputStream->Put(fFirstTransport);
	
	//The source IP addr is optional, only append it if it is provided
	if (serverIPAddr != NULL)
	{
		fOutputStream->Put(sSourceString);
		fOutputStream->Put(*serverIPAddr);
	}
	
	// Append the server ports, if provided.
	if (serverPortA != NULL)
	{
		fOutputStream->Put(sServerPortString);
		fOutputStream->Put(*serverPortA);
		fOutputStream->PutChar('-');
		fOutputStream->Put(*serverPortB);
	}
	
	// Append channel #'s, if provided
	if (channelA != NULL)
	{
		fOutputStream->Put(sInterleavedString);
		fOutputStream->Put(*channelA);
		fOutputStream->PutChar('-');
		fOutputStream->Put(*channelB);
	}

	fOutputStream->PutEOL();
}

void RTSPRequestInterface::AppendContentBaseHeader(StrPtrLen* theURL)
{
	if (!fStandardHeadersWritten)
		this->WriteStandardHeaders();

	fOutputStream->Put(RTSPProtocol::GetHeaderString(qtssContentBaseHeader));
	fOutputStream->Put(sColonSpace);
	fOutputStream->Put(*theURL);
	fOutputStream->PutChar('/');
	fOutputStream->PutEOL();
}

void RTSPRequestInterface::AppendRTPInfoHeader(QTSS_RTSPHeader inHeader,
												StrPtrLen* url, StrPtrLen* seqNumber,
												StrPtrLen* ssrc, StrPtrLen* rtpTime)
{
	static StrPtrLen sURL("url=", 4);
	static StrPtrLen sSeq(";seq=", 5);
	static StrPtrLen sSsrc(";ssrc=", 6);
	static StrPtrLen sRTPTime(";rtptime=", 9);

	if (!fStandardHeadersWritten)
		this->WriteStandardHeaders();

	fOutputStream->Put(RTSPProtocol::GetHeaderString(inHeader));
	if (inHeader != qtssSameAsLastHeader)
		fOutputStream->Put(sColonSpace);
		
	//Only append the various bits of RTP information if they actually have been
	//providied
	if ((url != NULL) && (url->Len > 0))
	{
		fOutputStream->Put(sURL);
		fOutputStream->Put(*url);
	}
	if ((seqNumber != NULL) && (seqNumber->Len > 0))
	{
		fOutputStream->Put(sSeq);
		fOutputStream->Put(*seqNumber);
	}
	if ((ssrc != NULL) && (ssrc->Len > 0))
	{
		fOutputStream->Put(sSsrc);
		fOutputStream->Put(*ssrc);
	}
	if ((rtpTime != NULL) && (rtpTime->Len > 0))
	{
		fOutputStream->Put(sRTPTime);
		fOutputStream->Put(*rtpTime);
	}
	fOutputStream->PutEOL();
}


void RTSPRequestInterface::WriteStandardHeaders()
{
	static StrPtrLen	sCloseString("Close", 5);

	Assert(sPremadeHeader != NULL);
	fStandardHeadersWritten = true; //must be done here to prevent recursive calls
	
	//if this is a "200 OK" response (most HTTP responses), we have some special
	//optmizations here
	if (fStatus == qtssSuccessOK)
	{
		fOutputStream->Put(sPremadeHeaderPtr);
		StrPtrLen* cSeq = fHeaderDictionary.GetValue(qtssCSeqHeader);
		Assert(cSeq != NULL);
		if (cSeq->Len > 1)
			fOutputStream->Put(*cSeq);
		else if (cSeq->Len == 1)
			fOutputStream->PutChar(*cSeq->Ptr);
		fOutputStream->PutEOL();
	}
	else
	{
		//other status codes just get built on the fly
		PutStatusLine(fOutputStream, fStatus, RTSPProtocol::k10Version);
		fOutputStream->Put(QTSServerInterface::GetServerHeader());
		fOutputStream->PutEOL();
		AppendHeader(qtssCSeqHeader, fHeaderDictionary.GetValue(qtssCSeqHeader));
	}

	//append sessionID header
	StrPtrLen* incomingID = fHeaderDictionary.GetValue(qtssSessionHeader);
	if ((incomingID != NULL) && (incomingID->Len > 0))
		AppendHeader(qtssSessionHeader, incomingID);

	//follows the HTTP/1.1 convention: if server wants to close the connection, it
	//tags the response with the Connection: close header
	if (!fResponseKeepAlive)
		AppendHeader(qtssConnectionHeader, &sCloseString);
}

void RTSPRequestInterface::SendHeader()
{
	if (!fStandardHeadersWritten)
		this->WriteStandardHeaders();
	fOutputStream->PutEOL();
}

QTSS_Error
RTSPRequestInterface::Write(void* inBuffer, UInt32 inLength, UInt32* outLenWritten, UInt32 /*inFlags*/)
{
	//now just write whatever remains into the output buffer
	fOutputStream->Put((char*)inBuffer, inLength);
	
	if (outLenWritten != NULL)
		*outLenWritten = inLength;
		
	return QTSS_NoErr;
}

QTSS_Error
RTSPRequestInterface::WriteV(iovec* inVec, UInt32 inNumVectors, UInt32 inTotalLength, UInt32* outLenWritten)
{
	(void)fOutputStream->WriteV(inVec, inNumVectors, inTotalLength, NULL,
												RTSPResponseStream::kAlwaysBuffer);
	if (outLenWritten != NULL)
		*outLenWritten = inTotalLength;
	return QTSS_NoErr;	
}

#pragma mark __PARAM_RETRIEVAL_FUNCTIONS__
//param retrieval functions described in .h file
Bool16 RTSPRequestInterface::GetAbsTruncatedPath(QTSS_FunctionParams* funcParamsPtr)
{
	//We only want this param retrieval function to be invoked once, so set the
	//qtssRTSPReqFilePathTruncParam to be a valid pointer

	if (funcParamsPtr->selector != qtssGetValuePtrEnterFunc)
		return false;
		
	if (funcParamsPtr->io.valueLen != 0) // already set do nothing
		return false;
	
	//This function works for 2 attributes, because they are almost the same: qtssRTSPReqFilePathTruncParam, qtssRTSPReqTruncAbsoluteURLParam
	
	RTSPRequestInterface* theRequest = (RTSPRequestInterface*)funcParamsPtr->object;
	theRequest->SetVal(qtssRTSPReqTruncAbsoluteURL, theRequest->GetValue(qtssRTSPReqAbsoluteURL));

	//Adjust the length to truncate off the last file in the path
	
	StrPtrLen* theAbsTruncPathParam = theRequest->GetValue(qtssRTSPReqTruncAbsoluteURL);
	theAbsTruncPathParam->Len--;
	while (theAbsTruncPathParam->Ptr[theAbsTruncPathParam->Len] != kPathDelimiterChar)
	{
		theAbsTruncPathParam->Len--;
	}
	
	funcParamsPtr->io.bufferPtr = theAbsTruncPathParam->Ptr;
	funcParamsPtr->io.valueLen = theAbsTruncPathParam->Len;

	return true;
}
//param retrieval functions described in .h file
Bool16 RTSPRequestInterface::GetTruncatedPath(QTSS_FunctionParams* funcParamsPtr)
{
	//We only want this param retrieval function to be invoked once, so set the
	//qtssRTSPReqFilePathTruncParam to be a valid pointer

	if (funcParamsPtr->selector != qtssGetValuePtrEnterFunc)
		return false;
		
	if (funcParamsPtr->io.valueLen != 0) // already set do nothing
		return false;
	
	//This function works for 2 attributes, because they are almost the same: qtssRTSPReqFilePathTruncParam, qtssRTSPReqTruncAbsoluteURLParam
	
	RTSPRequestInterface* theRequest = (RTSPRequestInterface*)funcParamsPtr->object;
	theRequest->SetVal(qtssRTSPReqFilePathTrunc, theRequest->GetValue(qtssRTSPReqFilePath));

	//Adjust the length to truncate off the last file in the path
	
	StrPtrLen* theTruncPathParam = theRequest->GetValue(qtssRTSPReqFilePathTrunc);
	theTruncPathParam->Len--;
	while (theTruncPathParam->Ptr[theTruncPathParam->Len] != kPathDelimiterChar)
	{
		theTruncPathParam->Len--;
	}
	funcParamsPtr->io.bufferPtr = theTruncPathParam->Ptr;
	funcParamsPtr->io.valueLen = theTruncPathParam->Len;

	return true;
}

Bool16 RTSPRequestInterface::GetFileName(QTSS_FunctionParams* funcParamsPtr)
{

	//We only want this param retrieval function to be invoked once, so set the
	//qtssRTSPReqFilePathTruncParam to be a valid pointer

	if (funcParamsPtr->selector != qtssGetValuePtrEnterFunc)
		return false;
		
	if (funcParamsPtr->io.valueLen != 0) // already set do nothing
		return false;
	
	RTSPRequestInterface* theRequest = (RTSPRequestInterface*)funcParamsPtr->object;
	theRequest->SetVal(qtssRTSPReqFileName, theRequest->GetValue(qtssRTSPReqFilePath));

	StrPtrLen* theFileNameParam = theRequest->GetValue(qtssRTSPReqFileName);

	//paranoid check
	if (theFileNameParam->Len == 0)
		return false;
		
	//walk back in the file name until we hit a /
	SInt32 x = theFileNameParam->Len - 1;
	for (; x > 0; x--)
		if (theFileNameParam->Ptr[x] == kPathDelimiterChar)
			break;
	//once we do, make the tempPtr point to the next character after the slash,
	//and adjust the length accordingly
	if (theFileNameParam->Ptr[x] == kPathDelimiterChar)
	{
		theFileNameParam->Ptr = (&theFileNameParam->Ptr[x]) + 1;
		theFileNameParam->Len -= (x + 1);
	}
	
	funcParamsPtr->io.bufferPtr = theFileNameParam->Ptr;
	funcParamsPtr->io.valueLen = theFileNameParam->Len;

	return true;
}


Bool16 RTSPRequestInterface::GetFileDigit(QTSS_FunctionParams* funcParamsPtr)
{
	//We only want this param retrieval function to be invoked once, so set the
	//qtssRTSPReqFileDigitParam to be a valid pointer

	if (funcParamsPtr->selector != qtssGetValuePtrEnterFunc)
		return false;
		
	if (funcParamsPtr->io.valueLen != 0) // already set do nothing
		return false;
	
	RTSPRequestInterface* theRequest = (RTSPRequestInterface*)funcParamsPtr->object;
	theRequest->SetVal(qtssRTSPReqFileDigit, theRequest->GetValue(qtssRTSPReqFilePath));

	StrPtrLen* theFileDigit = theRequest->GetValue(qtssRTSPReqFileDigit);

	UInt32	theFilePathLen = theRequest->GetValue(qtssRTSPReqFilePath)->Len;
	theFileDigit->Ptr += theFilePathLen - 1;
	theFileDigit->Len = 0;
	while ((StringParser::sDigitMask[*theFileDigit->Ptr] != 0) &&
			(theFileDigit->Len <= theFilePathLen))
	{
		theFileDigit->Ptr--;
		theFileDigit->Len++;
	}
	//termination condition means that we aren't actually on a digit right now.
	//Move pointer back onto the digit
	theFileDigit->Ptr++;
	
	funcParamsPtr->io.bufferPtr = theFileDigit->Ptr;
	funcParamsPtr->io.valueLen = theFileDigit->Len;

	return true;
}

Bool16 RTSPRequestInterface::GetRealStatusCode(QTSS_FunctionParams* funcParamsPtr)
{
	// Set the fRealStatusCode variable based on the current fStatusCode.
	// Return it to the caller (this means this function will be invoked each
	// time the parameter is requested, which is what we want).
	if (funcParamsPtr->selector != qtssGetValuePtrEnterFunc)
		return false;
	
	RTSPRequestInterface* theReq = (RTSPRequestInterface*)funcParamsPtr->object;
	theReq->fRealStatusCode = RTSPProtocol::GetStatusCode(theReq->fStatus);
	
	funcParamsPtr->io.bufferPtr = &theReq->fRealStatusCode;
	funcParamsPtr->io.valueLen = sizeof(theReq->fRealStatusCode);
	
	return true;

}
