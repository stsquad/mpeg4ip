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
	File:		QTSSModuleUtils.cpp

	Contains:	Implements utility routines defined in QTSSModuleUtils.h.
					
*/

#include "QTSSModuleUtils.h"
#include "QTSS_Private.h"

#include "StrPtrLen.h"
#include "OSArrayObjectDeleter.h"
#include "OSMemory.h"
#include "MyAssert.h"
#include "StringFormatter.h"
#include "QTSSDataConverter.h"
#include "ResizeableStringFormatter.h"

QTSS_TextMessagesObject 	QTSSModuleUtils::sMessages = NULL;
QTSS_ServerObject 			QTSSModuleUtils::sServer = NULL;
QTSS_StreamRef 				QTSSModuleUtils::sErrorLog = NULL;


void	QTSSModuleUtils::Initialize(QTSS_TextMessagesObject inMessages,
									QTSS_ServerObject inServer,
									QTSS_StreamRef inErrorLog)
{
	sMessages = inMessages;
	sServer = inServer;
	sErrorLog = inErrorLog;
}

void QTSSModuleUtils::ReadEntireFile(char* inPath, StrPtrLen* outData)
{	
	//
	// Use the QTSS file system API to read the file
	QTSS_Object theFileObject = NULL;
	QTSS_Error theErr = QTSS_OpenFileObject(inPath, 0, &theFileObject);
	if (theErr != QTSS_NoErr)
		return;
		
	UInt64* theLength = NULL;
	UInt32 theParamLen = 0;
	theErr = QTSS_GetValuePtr(theFileObject, qtssFlObjLength, 0, (void**)&theLength, &theParamLen);
	Assert(theParamLen == sizeof(UInt64));
	if (theParamLen != sizeof(UInt64))
		return;
		
	
	// Allocate memory for the file data
	outData->Ptr = NEW char[*theLength + 1];
	outData->Len = *theLength;
	
	// Read the data
	UInt32 recvLen = 0;
	theErr = QTSS_Read(theFileObject, outData->Ptr, outData->Len, &recvLen);
	if (theErr != QTSS_NoErr)
	{
		delete [] outData->Ptr;
		outData->Len = 0;
		return;
	}
	Assert(outData->Len == recvLen);
	
	// Close the file
	theErr = QTSS_CloseFileObject(theFileObject);
	Assert(theErr == QTSS_NoErr);
}

void	QTSSModuleUtils::SetupSupportedMethods(QTSS_Object inServer, QTSS_RTSPMethod* inMethodArray, UInt32 inNumMethods)
{
	// Report to the server that this module handles DESCRIBE, SETUP, PLAY, PAUSE, and TEARDOWN
	UInt32 theNumMethods = 0;
	(void)QTSS_GetNumValues(inServer, qtssSvrHandledMethods, &theNumMethods);
	
	for (UInt32 x = 0; x < inNumMethods; x++)
		(void)QTSS_SetValue(inServer, qtssSvrHandledMethods, theNumMethods++, (void*)&inMethodArray[x], sizeof(inMethodArray[x]));
}

void 	QTSSModuleUtils::LogError(	QTSS_ErrorVerbosity inVerbosity,
									QTSS_AttributeID inTextMessage,
									UInt32 /*inErrNumber*/,
									char* inArgument,
									char* inArg2)
{
	static char* sEmptyArg = "";
	
	if (sMessages == NULL)
		return;
		
	// Retrieve the specified text message from the text messages dictionary.
	
	StrPtrLen theMessage;
	(void)QTSS_GetValuePtr(sMessages, inTextMessage, 0, (void**)&theMessage.Ptr, &theMessage.Len);
	if ((theMessage.Ptr == NULL) || (theMessage.Len == 0))
		(void)QTSS_GetValuePtr(sMessages, qtssMsgNoMessage, 0, (void**)&theMessage.Ptr, &theMessage.Len);

	if ((theMessage.Ptr == NULL) || (theMessage.Len == 0))
		return;
	
	// ::sprintf and ::strlen will crash if inArgument is NULL
	if (inArgument == NULL)
		inArgument = sEmptyArg;
	if (inArg2 == NULL)
		inArg2 = sEmptyArg;
	
	// Create a new string, and put the argument into the new string.
	
	UInt32 theMessageLen = theMessage.Len + ::strlen(inArgument) + ::strlen(inArg2);

	OSCharArrayDeleter theLogString(NEW char[theMessageLen + 1]);
	::sprintf(theLogString.GetObject(), theMessage.Ptr, inArgument, inArg2);
	Assert(theMessageLen >= ::strlen(theLogString.GetObject()));
	
	(void)QTSS_Write(sErrorLog, theLogString.GetObject(), ::strlen(theLogString.GetObject()),
						NULL, inVerbosity);
}


char* QTSSModuleUtils::GetFullPath(	QTSS_RTSPRequestObject inRequest,
									QTSS_AttributeID whichFileType,
									UInt32* outLen,
									StrPtrLen* suffix)
{
	Assert(outLen != NULL);
	
	// Get the proper file path attribute. This may return an error if
	// the file type is qtssFilePathTrunc attr, because there may be no path
	// once its truncated. That's ok. In that case, we just won't append a path.
	StrPtrLen theFilePath;
	(void)QTSS_GetValuePtr(inRequest, whichFileType, 0, (void**)&theFilePath.Ptr, &theFilePath.Len);

	StrPtrLen theRootDir;
	QTSS_Error theErr = QTSS_GetValuePtr(inRequest, qtssRTSPReqRootDir, 0, (void**)&theRootDir.Ptr, &theRootDir.Len);
	Assert(theErr == QTSS_NoErr);

	//construct a full path out of the root dir path for this request,
	//and the url path.
	*outLen = theFilePath.Len + theRootDir.Len + 2;
	if (suffix != NULL)
		*outLen += suffix->Len;
	
	char* theFullPath = NEW char[*outLen];
	
	//write all the pieces of the path into this new buffer.
	StringFormatter thePathFormatter(theFullPath, *outLen);
	thePathFormatter.Put(theRootDir);
	thePathFormatter.Put(theFilePath);
	if (suffix != NULL)
		thePathFormatter.Put(*suffix);
	thePathFormatter.PutTerminator();

	*outLen = *outLen - 2;
	return theFullPath;
}

QTSS_Error	QTSSModuleUtils::AppendRTPMetaInfoHeader( 	QTSS_RTSPRequestObject inRequest,
														StrPtrLen* inRTPMetaInfoHeader,
														RTPMetaInfoPacket::FieldID* inFieldIDArray)
{
	//
	// For formatting the response header
	char tempBuffer[128];
	ResizeableStringFormatter theFormatter(tempBuffer, 128);
	
	StrPtrLen theHeader(*inRTPMetaInfoHeader);
	
	//
	// For marking which fields were requested by the client
	Bool16 foundFieldArray[RTPMetaInfoPacket::kNumFields];
	::memset(foundFieldArray, 0, sizeof(Bool16) * RTPMetaInfoPacket::kNumFields);
	
	char* theEndP = theHeader.Ptr + theHeader.Len;
	
	while (theHeader.Ptr <= (theEndP - sizeof(RTPMetaInfoPacket::FieldName)))
	{
		RTPMetaInfoPacket::FieldName* theFieldName = (RTPMetaInfoPacket::FieldName*)theHeader.Ptr;
		RTPMetaInfoPacket::FieldIndex theFieldIndex = RTPMetaInfoPacket::GetFieldIndexForName(ntohs(*theFieldName));
		
		//
		// This field is not supported (not in the field ID array), so
		// don't put it in the response
		if ((theFieldIndex == RTPMetaInfoPacket::kIllegalField) ||
			(inFieldIDArray[theFieldIndex] == RTPMetaInfoPacket::kFieldNotUsed))
		{
			theHeader.Ptr += 3;
			continue;
		}
		
		//
		// Mark that this field has been requested by the client
		foundFieldArray[theFieldIndex] = true;
		
		//
		// This field is good to go... put it in the response	
		theFormatter.Put(theHeader.Ptr, sizeof(RTPMetaInfoPacket::FieldName));
		
		if (inFieldIDArray[theFieldIndex] != RTPMetaInfoPacket::kUncompressed)
		{
			//
			// If the caller wants this field to be compressed (there
			// is an ID associated with the field), put the ID in the response
			theFormatter.PutChar('=');
			theFormatter.Put(inFieldIDArray[theFieldIndex]);
		}
		
		//
		// Field separator
		theFormatter.PutChar(';');
			
		//
		// Skip onto the next field name in the header
		theHeader.Ptr += 3;
	}

	//
	// Go through the caller's FieldID array, and turn off the fields
	// that were not requested by the client.
	for (UInt32 x = 0; x < RTPMetaInfoPacket::kNumFields; x++)
	{
		if (!foundFieldArray[x])
			inFieldIDArray[x] = RTPMetaInfoPacket::kFieldNotUsed;
	}
	
	//
	// No intersection between requested headers and supported headers!
	if (theFormatter.GetCurrentOffset() == 0)
		return QTSS_ValueNotFound; // Not really the greatest error!
		
	//
	// When appending the header to the response, strip off the last ';'.
	// It's not needed.
	return QTSS_AppendRTSPHeader(inRequest, qtssXRTPMetaInfoHeader, theFormatter.GetBufPtr(), theFormatter.GetCurrentOffset() - 1);
}

QTSS_Error	QTSSModuleUtils::SendErrorResponse(	QTSS_RTSPRequestObject inRequest,
														QTSS_RTSPStatusCode inStatusCode,
														QTSS_AttributeID inTextMessage,
														StrPtrLen* inStringArg)
{
	static Bool16 sFalse = false;
	
	//set RTSP headers necessary for this error response message
	(void)QTSS_SetValue(inRequest, qtssRTSPReqStatusCode, 0, &inStatusCode, sizeof(inStatusCode));
	(void)QTSS_SetValue(inRequest, qtssRTSPReqRespKeepAlive, 0, &sFalse, sizeof(sFalse));
	
	//send the response header. In all situations where errors could happen, we
	//don't really care, cause there's nothing we can do anyway!
	(void)QTSS_SendRTSPHeaders(inRequest);
	
	// Retrieve the specified message out of the text messages dictionary.
	StrPtrLen theMessage;
	(void)QTSS_GetValuePtr(sMessages, inTextMessage, 0, (void**)&theMessage.Ptr, &theMessage.Len);

	if ((theMessage.Ptr == NULL) || (theMessage.Len == 0))
	{
		// If we couldn't find the specified message, get the default
		// "No Message" message, and return that to the client instead.
		
		(void)QTSS_GetValuePtr(sMessages, qtssMsgNoMessage, 0, (void**)&theMessage.Ptr, &theMessage.Len);
	}
	Assert(theMessage.Ptr != NULL);
	Assert(theMessage.Len > 0);
	
	// Allocate a temporary buffer for the error message, and format the error message
	// into that buffer
	UInt32 theMsgLen = theMessage.Len + 1;
	if (inStringArg != NULL)
		theMsgLen += inStringArg->Len;
	OSCharArrayDeleter theErrorMsg(NEW char[theMsgLen + 1]);
	StringFormatter theErrorMsgFormatter(theErrorMsg, theMsgLen);
	
	//
	// Look for a %s in the string, and if one exists, replace it with the
	// argument passed into this function.
	
	//we can safely assume that message is in fact NULL terminated
	char* stringLocation = ::strstr(theMessage.Ptr, "%s");
	if (stringLocation != NULL)
	{
		//write first chunk
		theErrorMsgFormatter.Put(theMessage.Ptr, stringLocation - theMessage.Ptr);
		
		if (inStringArg != NULL && inStringArg->Len > 0)
		{
			//write string arg if it exists
			theErrorMsgFormatter.Put(inStringArg->Ptr, inStringArg->Len);
			stringLocation += 2;
		}
		//write last chunk
		theErrorMsgFormatter.Put(stringLocation, (theMessage.Ptr + theMessage.Len) - stringLocation);
	}
	else
		theErrorMsgFormatter.Put(theMessage);
	
	//
	// Now that we've formatted the message into the temporary buffer,
	// write it out to the request stream and the Client Session object
	(void)QTSS_Write(inRequest, theErrorMsgFormatter.GetBufPtr(), theErrorMsgFormatter.GetBytesWritten(), NULL, 0);
	(void)QTSS_SetValue(inRequest, qtssRTSPReqRespMsg, 0, theErrorMsgFormatter.GetBufPtr(), theErrorMsgFormatter.GetBytesWritten());
	
	return QTSS_RequestFailed;
}

void	QTSSModuleUtils::SendDescribeResponse(QTSS_RTSPRequestObject inRequest,
													QTSS_ClientSessionObject inSession,
													iovec* describeData,
													UInt32 inNumVectors,
													UInt32 inTotalLength)
{
	//write content size header
	char buf[32];
	::sprintf(buf, "%ld", inTotalLength);
	(void)QTSS_AppendRTSPHeader(inRequest, qtssContentLengthHeader, &buf[0], ::strlen(&buf[0]));

	(void)QTSS_SendStandardRTSPResponse(inRequest, inSession, 0);
	(void)QTSS_WriteV(inRequest, describeData, inNumVectors, inTotalLength, NULL);
}

QTSS_ModulePrefsObject QTSSModuleUtils::GetModulePrefsObject(QTSS_ModuleObject inModObject)
{
	QTSS_ModulePrefsObject thePrefsObject = NULL;
	UInt32 theLen = sizeof(thePrefsObject);
	QTSS_Error theErr = QTSS_GetValue(inModObject, qtssModPrefs, 0, &thePrefsObject, &theLen);
	Assert(theErr == QTSS_NoErr);
	
	return thePrefsObject;
}

QTSS_ModulePrefsObject QTSSModuleUtils::GetModuleObjectByName(const StrPtrLen& inModuleName)
{
	QTSS_ModuleObject theModule = NULL;
	UInt32 theLen = sizeof(theModule);
	
	for (int x = 0; QTSS_GetValue(sServer, qtssSvrModuleObjects, x, &theModule, &theLen) == QTSS_NoErr; x++)
	{
		Assert(theModule != NULL);
		Assert(theLen == sizeof(theModule));
		
		StrPtrLen theName;
		QTSS_Error theErr = QTSS_GetValuePtr(theModule, qtssModName, 0, (void**)&theName.Ptr, &theName.Len);
		Assert(theErr == QTSS_NoErr);
		
		if (inModuleName.Equal(theName))
			return theModule;
			
#if DEBUG
		theModule = NULL;
		theLen = sizeof(theModule);
#endif
	}
	return NULL;
}

void	QTSSModuleUtils::GetPref(QTSS_Object inPrefsObject, char* inPrefName, QTSS_AttrDataType inType, 
												void* ioBuffer, void* inDefaultValue, UInt32 inBufferLen)
{
	//
	// Check to make sure this pref is the right type. If it's not, this will coerce
	// it to be the right type. This also returns the id of the attribute
	QTSS_AttributeID theID = QTSSModuleUtils::CheckPrefDataType(inPrefsObject, inPrefName, inType, inDefaultValue, inBufferLen);

	//
	// Get the pref value.
	QTSS_Error theErr = QTSS_GetValue(inPrefsObject, theID, 0, ioBuffer, &inBufferLen);
	
	//
	// Caller should KNOW how big this pref is
	Assert(theErr != QTSS_NotEnoughSpace);
	
	if (theErr != QTSS_NoErr)
	{
		//
		// If we couldn't get the pref value for whatever reason, just use the
		// default if it was provided.
		::memcpy(ioBuffer, inDefaultValue, inBufferLen);

		if (inBufferLen > 0)
		{
			//
			// Log an error for this pref only if there was a default value provided.
			OSCharArrayDeleter theValueStr(QTSSDataConverter::ConvertTypeToString(inDefaultValue, inBufferLen, inType));
			QTSSModuleUtils::LogError( 	qtssWarningVerbosity,
										qtssServerPrefMissing,
										0,
										inPrefName,
										theValueStr.GetObject());
		}
		
		//
		// Create an entry for this pref in the pref file							
		QTSSModuleUtils::CreatePrefAttr(inPrefsObject, inPrefName, inType, inDefaultValue, inBufferLen);
	}
}

char*	QTSSModuleUtils::GetStringPref(QTSS_ModulePrefsObject inPrefsObject, char* inPrefName, char* inDefaultValue)
{
	UInt32 theDefaultValLen = 0;
	if (inDefaultValue != NULL)
		theDefaultValLen = ::strlen(inDefaultValue);
	
	//
	// Check to make sure this pref is the right type. If it's not, this will coerce
	// it to be the right type
	QTSS_AttributeID theID = QTSSModuleUtils::CheckPrefDataType(inPrefsObject, inPrefName, qtssAttrDataTypeCharArray, inDefaultValue, theDefaultValLen);

	char* theString = NULL;
	(void)QTSS_GetValueAsString(inPrefsObject, theID, 0, &theString);
	if (theString != NULL)
		return theString;
	
	//
	// If we get here the pref must be missing, so create it in the file, log
	// an error.
	
	QTSSModuleUtils::CreatePrefAttr(inPrefsObject, inPrefName, qtssAttrDataTypeCharArray, inDefaultValue, theDefaultValLen);
	
	//
	// Return the default if it was provided. Only log an error if the default value was provided
	if (theDefaultValLen > 0)
	{
		QTSSModuleUtils::LogError( 	qtssWarningVerbosity,
									qtssServerPrefMissing,
									0,
									inPrefName,
									inDefaultValue);
	}
	
	if (inDefaultValue != NULL)
	{
		//
		// Whether to return the default value or not from this function is dependent
		// solely on whether the caller passed in a non-NULL pointer or not.
		// This ensures that if the caller wants an empty-string returned as a default
		// value, it can do that.
		theString = NEW char[theDefaultValLen + 1];
		::strcpy(theString, inDefaultValue);
		return theString;
	}
	return NULL;
}

QTSS_AttributeID QTSSModuleUtils::GetAttrID(QTSS_Object inPrefsObject, char* inPrefName)
{
	//
	// Get the attribute ID of this pref.
	QTSS_Object theAttrInfo = NULL;
	QTSS_Error theErr = QTSS_GetAttrInfoByName(inPrefsObject, inPrefName, &theAttrInfo);
	if (theErr != QTSS_NoErr)
		return qtssIllegalAttrID;

	QTSS_AttributeID theID = qtssIllegalAttrID;	
	UInt32 theLen = sizeof(theID);
	theErr = QTSS_GetValue(theAttrInfo, qtssAttrID, 0, &theID, &theLen);
	Assert(theErr == QTSS_NoErr);

	return theID;
}

QTSS_AttributeID	QTSSModuleUtils::CheckPrefDataType(QTSS_Object inPrefsObject, char* inPrefName, QTSS_AttrDataType inType, void* inDefaultValue, UInt32 inBufferLen)
{
	//
	// Get the attribute type of this pref.
	QTSS_Object theAttrInfo = NULL;
	QTSS_Error theErr = QTSS_GetAttrInfoByName(inPrefsObject, inPrefName, &theAttrInfo);
	if (theErr != QTSS_NoErr)
		return qtssIllegalAttrID;

	QTSS_AttrDataType thePrefType = qtssAttrDataTypeUnknown;
	UInt32 theLen = sizeof(thePrefType);
	theErr = QTSS_GetValue(theAttrInfo, qtssAttrDataType, 0, &thePrefType, &theLen);
	Assert(theErr == QTSS_NoErr);
	
	QTSS_AttributeID theID = qtssIllegalAttrID;	
	theLen = sizeof(theID);
	theErr = QTSS_GetValue(theAttrInfo, qtssAttrID, 0, &theID, &theLen);
	Assert(theErr == QTSS_NoErr);

	if (thePrefType != inType)
	{
		OSCharArrayDeleter theValueStr(QTSSDataConverter::ConvertTypeToString(inDefaultValue, inBufferLen, inType));
		QTSSModuleUtils::LogError( 	qtssWarningVerbosity,
									qtssServerPrefWrongType,
									0,
									inPrefName,
									theValueStr.GetObject());
									
		theErr = QTSS_RemoveInstanceAttribute( inPrefsObject, theID );
		Assert(theErr == QTSS_NoErr);
		return  QTSSModuleUtils::CreatePrefAttr(inPrefsObject, inPrefName, inType, inDefaultValue, inBufferLen);
	}
	return theID;
}

QTSS_AttributeID	QTSSModuleUtils::CreatePrefAttr(QTSS_ModulePrefsObject inPrefsObject, char* inPrefName, QTSS_AttrDataType inType, void* inDefaultValue, UInt32 inBufferLen)
{
	QTSS_Error theErr = QTSS_AddInstanceAttribute(inPrefsObject, inPrefName, NULL, inType);
	Assert((theErr == QTSS_NoErr) || (theErr == QTSS_AttrNameExists));
	
	QTSS_AttributeID theID = QTSSModuleUtils::GetAttrID(inPrefsObject, inPrefName);
	Assert(theID != qtssIllegalAttrID);
		
	//
	// Caller can pass in NULL for inDefaultValue, in which case we don't add the default
	if (inBufferLen > 0)
	{
		theErr = QTSS_SetValue(inPrefsObject, theID, 0, inDefaultValue, inBufferLen);
		Assert(theErr == QTSS_NoErr);
	}
	return theID;
}
