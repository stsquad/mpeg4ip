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
	File:		RTSPRequest.cpp

	Contains:	Implementation of RTSPRequest class.

	
	
*/


#include "RTSPRequest.h"
#include "RTSPProtocol.h"
#include "QTSServerInterface.h"

#include "RTSPSessionInterface.h"
#include "StringParser.h"
#include "StringTranslator.h"
#include "OS.h"
#include "QTSS.h"
#include "QTSSModuleUtils.h"
#include "base64.h"
#include "OSArrayObjectDeleter.h"
#include "DateTranslator.h"
#include "SocketUtils.h"

UInt8
RTSPRequest::sURLStopConditions[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1, //0-9      //'\t' is a stop condition
	1, 0, 0, 1, 0, 0, 0, 0, 0, 0, //10-19    //'\r' & '\n' are stop conditions
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //20-29
	0, 0, 1, 0, 0, 0, 0, 0, 0, 0, //30-39    //' '
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //40-49
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //50-59
	0, 0, 0, 1, 0, 0, 0, 0, 0, 0, //60-69	//'?' 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //70-79
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //80-89
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //90-99
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //100-109
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //110-119
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //120-129
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //130-139
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //140-149
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //150-159
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //160-169
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //170-179
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //180-189
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //190-199
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //200-209
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //210-219
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //220-229
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //230-239
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //240-249
	0, 0, 0, 0, 0, 0 			 //250-255
};

static StrPtrLen	sDefaultRealm("Streaming Server");

//Parses the request
QTSS_Error RTSPRequest::Parse()
{
	StringParser parser(this->GetValue(qtssRTSPReqFullRequest));
	Assert(this->GetValue(qtssRTSPReqFullRequest)->Ptr != NULL);

	//parse status line.
	QTSS_Error error = ParseFirstLine(parser);

	//handle any errors that come up	
	if (error != QTSS_NoErr)
		return error;
		
	error = this->ParseHeaders(parser);
	if (error != QTSS_NoErr)
		return error;
	
	//Response headers should set themselves up to reflect what's in the request headers
	fResponseKeepAlive = fRequestKeepAlive;
	
	//Make sure that there was some path that was extracted from this request. If not, there is no way
	//we can process the request, so generate an error
	if (this->GetValue(qtssRTSPReqFilePath)->Len == 0)
		return QTSSModuleUtils::SendErrorResponse(this, qtssClientBadRequest, qtssMsgNoURLInRequest);
	
	return QTSS_NoErr;
}

//returns: StatusLineTooLong, SyntaxError, BadMethod
QTSS_Error RTSPRequest::ParseFirstLine(StringParser &parser)
{	
	//first get the method
	StrPtrLen theParsedData;
	parser.ConsumeWord(&theParsedData);
	this->SetVal(qtssRTSPReqMethodStr, theParsedData.Ptr, theParsedData.Len);
	
	
	//THIS WORKS UNDER THE ASSUMPTION THAT:
	//valid HTTP/1.1 headers are: GET, HEAD, POST, PUT, OPTIONS, DELETE, TRACE
	fMethod = RTSPProtocol::GetMethod(theParsedData);
	if (fMethod == qtssIllegalMethod)
		return QTSSModuleUtils::SendErrorResponse(this, qtssClientBadRequest, qtssMsgBadRTSPMethod, &theParsedData);
	
	//no longer assume this is a space... instead, just consume whitespace
	parser.ConsumeWhitespace();

	//now parse the uri
	QTSS_Error err = ParseURI(parser);
	if (err != QTSS_NoErr)
		return err;

	//no longer assume this is a space... instead, just consume whitespace
	parser.ConsumeWhitespace();

	//if there is a version, consume the version string
	StrPtrLen versionStr;
	parser.ConsumeUntil(&versionStr, StringParser::sEOLMask);
	
	//check the version
	if (versionStr.Len > 0)
		fVersion = RTSPProtocol::GetVersion(versionStr);

	//go past the end of line
	if (!parser.ExpectEOL())
		return QTSSModuleUtils::SendErrorResponse(this, qtssClientBadRequest, qtssMsgNoRTSPVersion);
	return QTSS_NoErr;
}

//returns: SyntaxError if there was an error in the uri. Or InternalServerError
QTSS_Error RTSPRequest::ParseURI(StringParser &parser)
{
	//read in the complete URL, set it to be the qtssAbsoluteURLParam
	StrPtrLen theAbsURL;

	//  RTSPRequestInterface::sPathURLStopConditions stop on ? as well as sURLStopConditions
	parser.ConsumeUntil(&theAbsURL, sURLStopConditions );

	// set qtssRTSPReqAbsoluteURL to the URL throught the path component; will be : <protocol>://<host-addr>/<path>
	this->SetVal(qtssRTSPReqAbsoluteURL, &theAbsURL);
	
	StringParser urlParser(&theAbsURL);
	
	//we always should have a slash before the uri.
	//If not, that indicates this is a full URI. Also, this could be a '*' OPTIONS request
	if ((*theAbsURL.Ptr != '/') && (*theAbsURL.Ptr != '*'))
	{
		//if it is a full URL, store the host name off in a separate parameter
		StrPtrLen theRTSPString;
		urlParser.ConsumeLength(&theRTSPString, 7); //consume "rtsp://"
		//assign the host field here to the proper QTSS param
		StrPtrLen theHost;
		urlParser.ConsumeUntil(&theHost, '/');
		fHeaderDictionary.SetVal(qtssHostHeader, &theHost);
	}
	
	//whatever is in this position in the URl must be the relative URL. Store that
	//in the qtssURLParam.
	this->SetVal(qtssRTSPReqURI, urlParser.GetCurrentPosition(), urlParser.GetDataReceivedLen() - urlParser.GetDataParsedLen());

	// parse the query string from the url if present.
	// init qtssRTSPReqQueryString dictionary to an empty string
	StrPtrLen queryString;
	this->SetVal(qtssRTSPReqQueryString, queryString.Ptr, queryString.Len);
	
	if ( parser.GetDataRemaining() > 0 )
	{	
		if ( parser.PeekFast() == '?' )
		{		
			// we've got some CGI param
			parser.ConsumeLength(&queryString, 1); // toss '?'
			
			// consume the rest of the line..
			parser.ConsumeUntilWhitespace(&queryString);
			
			this->SetVal(qtssRTSPReqQueryString, queryString.Ptr, queryString.Len);
		}
	}
	
	//
	// If the is a '*', this must be an OPTIONS request. If not,
	// return an error. If it is, return right now because '*' is not a path
	// so the below functions don't make any sense.
	if ((*theAbsURL.Ptr == '*') && (theAbsURL.Len == 1))
	{
		if (fMethod != qtssOptionsMethod)
			return QTSSModuleUtils::SendErrorResponse(this, qtssClientBadRequest, qtssMsgURLInBadFormat, &theAbsURL);

		this->SetVal(qtssRTSPReqFilePath, &theAbsURL);
		return QTSS_NoErr;
	}


	//path strings are statically allocated. Therefore, if they are longer than
	//this length we won't be able to handle the request.
	StrPtrLen* theURLParam = this->GetValue(qtssRTSPReqURI);
	if (theURLParam->Len > RTSPRequestInterface::kMaxFilePathSizeInBytes)
		return QTSSModuleUtils::SendErrorResponse(this, qtssClientBadRequest, qtssMsgURLTooLong, theURLParam);

	//decode the URL, put the result in the separate buffer for the file path,
	//set the file path StrPtrLen to the proper value
	SInt32 theBytesWritten = StringTranslator::DecodeURL(theURLParam->Ptr, theURLParam->Len,
												fFilePath, RTSPRequestInterface::kMaxFilePathSizeInBytes);
	//if negative, an error occurred, reported as an QTSS_Error
	//we also need to leave room for a terminator.
	if ((theBytesWritten < 0) || (theBytesWritten == RTSPRequestInterface::kMaxFilePathSizeInBytes))
	{
		return QTSSModuleUtils::SendErrorResponse(this, qtssClientBadRequest, qtssMsgURLInBadFormat, theURLParam);
	}

	// Convert from a / delimited path to a local file system path
	StringTranslator::DecodePath(fFilePath, theBytesWritten);
	
	//setup the proper QTSS param
	fFilePath[theBytesWritten] = '\0';
	this->SetVal(qtssRTSPReqFilePath, fFilePath, theBytesWritten);
	return QTSS_NoErr;
}


//throws eHTTPNoMoreData and eHTTPOutOfBuffer
QTSS_Error RTSPRequest::ParseHeaders(StringParser& parser)
{
	StrPtrLen theKeyWord;
	Bool16 isStreamOK;
	
	//Repeat until we get a \r\n\r\n, which signals the end of the headers
	
	while ((parser.PeekFast() != '\r') && (parser.PeekFast() != '\n')) 
	{
		//First get the header identifier
		
		isStreamOK = parser.GetThru(&theKeyWord, ':');
		if (!isStreamOK)
			return QTSSModuleUtils::SendErrorResponse(this, qtssClientBadRequest, qtssMsgNoColonAfterHeader, this->GetValue(qtssRTSPReqFullRequest));
							
		if (parser.PeekFast() == ' ') {	// handle space, if any
			isStreamOK = parser.Expect(' ');
			Assert(isStreamOK);
		}
		
		//Look up the proper header enumeration based on the header string.
		//Use the enumeration to look up the dictionary ID of this header,
		//and set that dictionary attribute to be whatever is in the body of the header
		
		UInt32 theHeader = RTSPProtocol::GetRequestHeader(theKeyWord);

		StrPtrLen theHeaderVal;
		isStreamOK = parser.GetThruEOL(&theHeaderVal);

		// If this is an unknown header, ignore it. Otherwise, set the proper
		// dictionary attribute
		if (theHeader != qtssIllegalHeader)
		{
			Assert(theHeader < qtssNumHeaders);
			fHeaderDictionary.SetVal(theHeader, &theHeaderVal);
		}
		if (!isStreamOK)
			return QTSSModuleUtils::SendErrorResponse(this, qtssClientBadRequest, qtssMsgNoEOLAfterHeader);
		
		//some headers require some special processing. If this code begins
		//to get out of control, we made need to come up with a function pointer table
		switch (theHeader)
		{
			case qtssTransportHeader: 			ParseTransportHeader(); break;
			case qtssRangeHeader:				ParseRangeHeader(); 	break;
			case qtssIfModifiedSinceHeader:		ParseIfModSinceHeader();break;
			case qtssXRetransmitHeader:			ParseRetransmitHeader();break;
			case qtssContentLengthHeader:		ParseContentLengthHeader();break;
			case qtssSpeedHeader:				ParseSpeedHeader();		break;
			case qtssXRTPOptionsHeader:			ParseRTPOptionsHeader();break;
			default:	break;
		}
	}

	// Tell the session what the request body length is for this request
	// so that it can prevent people from reading past the end of the request.
	StrPtrLen* theContentLengthBody = fHeaderDictionary.GetValue(qtssContentLengthHeader);
	if (theContentLengthBody->Len > 0)
	{
		StringParser theHeaderParser(fHeaderDictionary.GetValue(qtssContentLengthHeader));
		this->GetSession()->SetRequestBodyLength(theHeaderParser.ConsumeInteger(NULL));
	}
	
	isStreamOK = parser.ExpectEOL();
	Assert(isStreamOK);
	return QTSS_NoErr;
}

void RTSPRequest::ParseTransportHeader()
{
	StringParser theTransParser(fHeaderDictionary.GetValue(qtssTransportHeader));
	
	//transport header from client: Transport: RTP/AVP;unicast;client_port=5000-5001\r\n
	//								Transport: RTP/AVP;multicast;ttl=15;destination=229.41.244.93;client_port=5000-5002\r\n
	//								Transport: RTP/AVP/TCP;unicast\r\n
	
	//
	// A client may send multiple transports to the server, comma separated.
	// In this case, the server should just pick one and use that. We currently
	// just use the first one and discard the rest.
	(void)theTransParser.ConsumeUntil(&fFirstTransport, ',');
	
	StringParser theFirstTransportParser(&fFirstTransport);
	
	StrPtrLen theTransportSubHeader;
	(void)theFirstTransportParser.GetThru(&theTransportSubHeader, ';');
	
	while (theTransportSubHeader.Len > 0)
	{
		// Extract the relevent information from the relevent subheader.
		// So far we care about 3 sub-headers
		
		switch (*theTransportSubHeader.Ptr)
		{
			case 'R':	// RTP/AVP/??? Is this TCP or UDP?
			{
				if ((theTransportSubHeader.Len == 11) && (theTransportSubHeader.Ptr[8] == 'T'))
					fTransportType = qtssRTPTransportTypeTCP;
				break;
			}
			case 'c':	//client_port sub-header
			{
				this->ParseClientPortSubHeader(&theTransportSubHeader);
				break;
			}
			case 'd':	//destination sub-header
			{
				static StrPtrLen sDestinationSubHeader("destination");
				
				//Parse the header, extract the destination address
				this->ParseAddrSubHeader(&theTransportSubHeader, &sDestinationSubHeader, &fDestinationAddr);
				break;
			}
			case 's':	//source sub-header
			{
				//Same as above code
				static StrPtrLen sSourceSubHeader("source");
				this->ParseAddrSubHeader(&theTransportSubHeader, &sSourceSubHeader, &fSourceAddr);
				break;
			}
			case 't':	//time-to-live sub-header
			{
				this->ParseTimeToLiveSubHeader(&theTransportSubHeader);
				break;
			}
		}
		
		// Move onto the next parameter
		(void)theFirstTransportParser.GetThru(&theTransportSubHeader, ';');
	}
}

void  RTSPRequest::ParseRangeHeader()
{
	StringParser theRangeParser(fHeaderDictionary.GetValue(qtssRangeHeader));

	// Setup the start and stop time dictionary attributes
	this->SetVal(qtssRTSPReqStartTime, &fStartTime, sizeof(fStartTime));
	this->SetVal(qtssRTSPReqStopTime, &fStopTime, sizeof(fStopTime));

	theRangeParser.ConsumeLength(NULL, 4);//consume "npt="
	fStartTime = (Float64)theRangeParser.ConsumeFloat();
	//see if there is a stop time as well.
	if (theRangeParser.GetDataRemaining() > 1)
	{
		theRangeParser.ConsumeLength(NULL, 1);
		fStopTime = (Float64)theRangeParser.ConsumeFloat();
	}
}

void  RTSPRequest::ParseRetransmitHeader()
{
	StringParser theRetransmitParser(fHeaderDictionary.GetValue(qtssXRetransmitHeader));
	StrPtrLen theProtName;
	Bool16 foundRetransmitProt = false;
			
	do
	{
		theRetransmitParser.ConsumeWord(&theProtName);
		foundRetransmitProt = theProtName.Equal(RTSPProtocol::GetRetransmitProtocolName());
	}
	while ( (!foundRetransmitProt) &&
			(theRetransmitParser.GetThru(NULL, ',')) );

	if (!foundRetransmitProt)
		return;
	
	//
	// We are using Reliable RTP as the transport for this stream,
	// but if there was a previous transport header that indicated TCP,
	// do not set the transport to be reliable UDP
	if (fTransportType == qtssRTPTransportTypeUDP)
		fTransportType = qtssRTPTransportTypeReliableUDP;

	StrPtrLen theProtArg;
	while (theRetransmitParser.GetThru(&theProtArg, '='))	
	{
		//
		// Parse out params
		static const StrPtrLen kAckTimeout("ack-timeout");
		static const StrPtrLen kWindow("window");
		 
		if (theProtArg.Equal(kWindow))
			fWindowSize = theRetransmitParser.ConsumeInteger(NULL);
		else if (theProtArg.Equal(kAckTimeout))
			fAckTimeout = theRetransmitParser.ConsumeInteger(NULL);
			
		theRetransmitParser.ConsumeLength(NULL, 1); //Skip past ';'
	}
}

void  RTSPRequest::ParseContentLengthHeader()
{
	StringParser theContentLenParser(fHeaderDictionary.GetValue(qtssContentLengthHeader));
	fContentLength = theContentLenParser.ConsumeInteger(NULL);
}

void  RTSPRequest::ParseIfModSinceHeader()
{
	fIfModSinceDate = DateTranslator::ParseDate(fHeaderDictionary.GetValue(qtssIfModifiedSinceHeader));

	// Only set the param if this is a legal date
	if (fIfModSinceDate != 0)
		this->SetVal(qtssRTSPReqIfModSinceDate, &fIfModSinceDate, sizeof(fIfModSinceDate));
}

void RTSPRequest::ParseSpeedHeader()
{
	StringParser theSpeedParser(fHeaderDictionary.GetValue(qtssSpeedHeader));
	fSpeed = theSpeedParser.ConsumeFloat();
}

void RTSPRequest::ParseRTPOptionsHeader()
{
	StringParser theRTPOptionsParser(fHeaderDictionary.GetValue(qtssXRTPOptionsHeader));
	StrPtrLen theRTPOptionsSubHeader;

	do
	{
		static StrPtrLen sLateTolerance("late-tolerance");
		
		if (theRTPOptionsSubHeader.NumEqualIgnoreCase(sLateTolerance.Ptr, sLateTolerance.Len))
		{
			StringParser theLateTolParser(&theRTPOptionsSubHeader);
			theLateTolParser.ConsumeLength(NULL, sLateTolerance.Len + 1);
			fLateTolerance = theLateTolParser.ConsumeInteger(NULL);
			fLateToleranceStr = theRTPOptionsSubHeader;
		}
		
		(void)theRTPOptionsParser.GetThru(&theRTPOptionsSubHeader, ';');
		
	} while(theRTPOptionsSubHeader.Len > 0);
}


void RTSPRequest::ParseAddrSubHeader(StrPtrLen* inSubHeader, StrPtrLen* inHeaderName, UInt32* outAddr)
{
	StringParser theSubHeaderParser(inSubHeader);

	// First make sure this is the proper subheader
	if (!inSubHeader->Equal(*inHeaderName))
		return;
		
	//Find the IP address
	theSubHeaderParser.ConsumeUntil(NULL, StringParser::sDigitMask);

	//Set the addr string param.
	StrPtrLen theAddr(theSubHeaderParser.GetCurrentPosition(), theSubHeaderParser.GetDataRemaining());
	
	//Convert the string to a UInt32 IP address
	char theTerminator = theAddr.Ptr[theAddr.Len];
	theAddr.Ptr[theAddr.Len] = '\0';
	
	*outAddr = SocketUtils::ConvertStringToAddr(theAddr.Ptr);
	
	theAddr.Ptr[theAddr.Len] = theTerminator;
}

void RTSPRequest::ParseClientPortSubHeader(StrPtrLen* inClientPortSubHeader)
{
	static StrPtrLen sClientPortSubHeader("client_port=");

	StringParser theSubHeaderParser(inClientPortSubHeader);

	// Skip over to the first port
	StrPtrLen theFirstBit;
	theSubHeaderParser.ConsumeUntil(&theFirstBit, StringParser::sDigitMask);
	
	// Make sure this is the client port subheader
	if (!theFirstBit.Equal(sClientPortSubHeader))
		return;

	// Store the two client ports as integers
	fClientPortA = (UInt16)theSubHeaderParser.ConsumeInteger(NULL);
	theSubHeaderParser.ConsumeLength(NULL, 1);
	fClientPortB = (UInt16)theSubHeaderParser.ConsumeInteger(NULL);
}

void RTSPRequest::ParseTimeToLiveSubHeader(StrPtrLen* inTimeToLiveSubHeader)
{
	static StrPtrLen sTimeToLiveSubHeader("ttl=");

	StringParser theSubHeaderParser(inTimeToLiveSubHeader);

	// Skip over to the first part
	StrPtrLen theFirstBit;
	theSubHeaderParser.ConsumeUntil(&theFirstBit, StringParser::sDigitMask);
	// Make sure this is the ttl subheader
	if (!theFirstBit.Equal(sTimeToLiveSubHeader))
		return;
	
	// Parse out the time to live...
	fTtl = (UInt16)theSubHeaderParser.ConsumeInteger(NULL);
}

QTSS_Error RTSPRequest::ParseAuthNameAndPassword(void)
 {
	char decodedLine[kAuthNameAndPasswordBuffSize] = { 0 };
	char codedLine[kAuthNameAndPasswordBuffSize] = { 0 };
	
	QTSS_Error	theErr = QTSS_NoErr;
	QTSSDictionary *theRTSPHeaders = this->GetHeaderDictionary();
	StrPtrLen	*authLine = theRTSPHeaders->GetValue(qtssAuthorizationHeader);
	if (authLine != NULL) do //once
	{	
		//PrintfStrPtrLen(  &authLine );
		if (0 == authLine->Len) break;
		
		Assert( authLine->Len < kAuthNameAndPasswordBuffSize );
		if (authLine->Len >= kAuthNameAndPasswordBuffSize) authLine->Len = kAuthNameAndPasswordBuffSize -1;
		memcpy (codedLine,authLine->Ptr,authLine->Len);
		authLine->Set(codedLine, authLine->Len);
	
		StrPtrLen	authWord("");
		StringParser parsedAuthLine(authLine);
		parsedAuthLine.ConsumeUntilWhitespace(&authWord);
		if (!authWord.Equal(StrPtrLen("Basic",strlen("Basic") ) ) ) break;
		
		parsedAuthLine.ConsumeWhitespace();
		parsedAuthLine.ConsumeUntilWhitespace(&authWord);
		if (0 == authWord.Len ) break;		
		if (authWord.Len > 1024) break;
		
		(void) Base64decode(decodedLine, authWord.Ptr);
		StrPtrLen	nameAndPassword(decodedLine);
		StrPtrLen	name("");
		StrPtrLen	password("");
		StrPtrLen	dump("");
		StringParser parsedNameAndPassword(&nameAndPassword);
		
		//PrintfStrPtrLen(  &nameAndPassword );

		parsedNameAndPassword.ConsumeUntil(&name,':');			
		parsedNameAndPassword.ConsumeLength(&dump, 1);
		parsedNameAndPassword.GetThruEOL(&password);
				
		//PrintfStrPtrLen(  &name );
		//PrintfStrPtrLen(  &password );
		
		(void) this->SetValue(qtssRTSPReqUserName,0,  name.Ptr , name.Len, QTSSDictionary::kDontObeyReadOnly);
		(void) this->SetValue(qtssRTSPReqUserPassword,0,  password.Ptr , password.Len, QTSSDictionary::kDontObeyReadOnly);
		
	} while (false);
	
	return theErr;
};


void RTSPRequest::SetupAuthLocalPath(void)
{
	 QTSS_AttributeID theID = qtssRTSPReqFilePath;

	//
	// Get the truncated path on a setup, because setups have the trackID appended
	if (qtssSetupMethod == fMethod)
		theID = qtssRTSPReqFilePathTrunc;	

	UInt32 theLen = 0;
	fFullPath = QTSSModuleUtils::GetFullPath(this, theID, &theLen, NULL);
	this->SetVal(qtssRTSPReqLocalPath, fFullPath, theLen);
}

QTSS_Error RTSPRequest::SendChallenge(void)
{	
	QTSS_Error theErr = QTSS_NoErr;
	char *prefRealmPtr = NULL;
	
	do 
	{	
		char realmBuff[kRealmBuffSize] = "Basic realm=\"";
		StrPtrLen challenge(realmBuff);
		StrPtrLen whichRealm;
		
		// Get the module's realm
		StrPtrLen moduleRealm;
		theErr = this->GetValuePtr(qtssRTSPReqURLRealm, 0,  (void **) &moduleRealm.Ptr, &moduleRealm.Len);
		if ( (QTSS_NoErr == theErr) && (moduleRealm.Len > 0) )
		{
			whichRealm = moduleRealm;	
		}
		else
		{
			theErr = QTSS_NoErr;
			// Get the default realm from the config file or use the static default if config realm is not found
			QTSServerInterface* theServer = QTSServerInterface::GetServer();
			prefRealmPtr = theServer->GetPrefs()->GetAuthorizationRealm(); // allocates memory
			Assert(prefRealmPtr != NULL);
			if (prefRealmPtr != NULL)
			{	whichRealm.Set(prefRealmPtr, strlen(prefRealmPtr));
			}	
			else
			{
				whichRealm = sDefaultRealm;
			}
		}
		
		int realmLen = whichRealm.Len + challenge.Len + 2; // add 2 based on double quote char + end of string 0x00
		if (realmLen > kRealmBuffSize) // The realm is too big so use the default realm
		{	Assert(0);
			whichRealm = sDefaultRealm;
		}
		memcpy(&challenge.Ptr[challenge.Len],whichRealm.Ptr,whichRealm.Len);
		int newLen = challenge.Len + whichRealm.Len;
		
		challenge.Ptr[newLen] = '"'; // add the terminating "" this was accounted for with the size check above
		challenge.Ptr[newLen + 1] = 0;// add the 0 terminator this was accounted for with the size check above
		challenge.Len = newLen +1; // set the real size of the string excluding the 0.
		
		#if (0)
		{  // test code
			char test[256];
			memcpy(test,sDefaultRealm.Ptr,sDefaultRealm.Len);
			printf("the static realm =%s \n",test);

			memcpy(test,prefRealm.Ptr,prefRealm.Len);
			printf("the Pref realm =%s \n",test);

			memcpy(test,moduleRealm.Ptr,moduleRealm.Len);
			printf("the moduleRealm  =%s \n",test);
		
			memcpy(test,whichRealm.Ptr,whichRealm.Len);
			printf("the challenge realm  =%s \n",test);
			
			memcpy(test,challenge.Ptr,challenge.Len);
			printf("the challenge string  =%s len = %ld\n",test, challenge.Len);
		}
		#endif

		fStatus = qtssClientUnAuthorized;
		this->SetResponseKeepAlive(true);
		this->AppendHeader(qtssWWWAuthenticateHeader, &challenge);
		this->SendHeader();

	
	} while (false);
	
	if (prefRealmPtr != NULL)
	{	
		delete[] prefRealmPtr;	
	}
	
	return theErr;
}
