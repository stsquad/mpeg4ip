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
	File:		QTSSModuleUtils.h

	Contains:	Utility routines for modules to use.
					
*/


#ifndef _QTSS_MODULE_UTILS_H_
#define _QTSS_MODULE_UTILS_H_

#include <stdlib.h>
#include "QTSS.h"
#include "StrPtrLen.h"
#include "RTPMetaInfoPacket.h"

class QTSSModuleUtils
{
	public:
	
		static void		Initialize(	QTSS_TextMessagesObject inMessages,
									QTSS_ServerObject inServer,
									QTSS_StreamRef inErrorLog);
	
		// Read the complete contents of the file at inPath into the StrPtrLen.
		// This function allocates memory for the file data.
		static void 	ReadEntireFile(char* inPath, StrPtrLen* outData);

		// If your module supports RTSP methods, call this function from your QTSS_Initialize
		// role to tell the server what those methods are.
		static void		SetupSupportedMethods(	QTSS_Object inServer,
												QTSS_RTSPMethod* inMethodArray,
												UInt32 inNumMethods);
												
		// Using a message out of the text messages dictionary is a common
		// way to log errors to the error log. Here is a function to
		// make that process very easy.
		
		static void 	LogError(	QTSS_ErrorVerbosity inVerbosity,
									QTSS_AttributeID inTextMessage,
									UInt32 inErrNumber,
									char* inArgument = NULL,
									char* inArg2 = NULL);
									
		// This function constructs a C-string of the full path to the file being requested.
		// You may opt to append an optional suffix, or pass in NULL. You are responsible
		// for disposing this memory

		static char* GetFullPath(	QTSS_RTSPRequestObject inRequest,
									QTSS_AttributeID whichFileType,
									UInt32* outLen,
									StrPtrLen* suffix = NULL);

		//
		// This function does 2 things:
		// 1. 	Compares the enabled fields in the field ID array with the fields in the
		//		x-RTP-Meta-Info header. Turns off the fields in the array that aren't in the request.
		//
		// 2.	Appends the x-RTP-Meta-Info header to the response, using the proper
		//		fields from the array, as well as the IDs provided in the array
		static QTSS_Error	AppendRTPMetaInfoHeader( QTSS_RTSPRequestObject inRequest,
														StrPtrLen* inRTPMetaInfoHeader,
														RTPMetaInfoPacket::FieldID* inFieldIDArray);

		// This function sends an error to the RTSP client. You must provide a
		// status code for the error, and a text message ID to describe the error.
		//
		// It always returns QTSS_RequestFailed.

		static QTSS_Error	SendErrorResponse(	QTSS_RTSPRequestObject inRequest,
														QTSS_RTSPStatusCode inStatusCode,
														QTSS_AttributeID inTextMessage,
														StrPtrLen* inStringArg = NULL);

		//Modules most certainly don't NEED to use this function, but it is awfully handy
		//if they want to take advantage of it. Using the SDP data provided in the iovec,
		//this function sends a standard describe response.
		//NOTE: THE FIRST ENTRY OF THE IOVEC MUST BE EMPTY!!!!
		static void	SendDescribeResponse(QTSS_RTSPRequestObject inRequest,
													QTSS_ClientSessionObject inSession,
													iovec* describeData,
													UInt32 inNumVectors,
													UInt32 inTotalLength);
													
		//
		// SEARCH FOR A SPECIFIC MODULE OBJECT							
		static QTSS_ModulePrefsObject GetModuleObjectByName(const StrPtrLen& inModuleName);
		
		//
		// GET MODULE PREFS OBJECT
		static QTSS_ModulePrefsObject GetModulePrefsObject(QTSS_ModuleObject inModObject);
		
		//
		// GET PREF
		//
		// This function retrieves a pref with the specified name and type
		// out of the specified module prefs object.
		//
		// Caller should pass in a buffer for ioBuffer that is large enough
		// to hold the pref value. inBufferLen should be set to the length
		// of this buffer.
		//
		// Pass in a buffer containing a default value to use for the pref
		// in the inDefaultValue parameter. If the pref isn't found, or is
		// of the wrong type, the default value will be copied into ioBuffer.
		// Also, this function adds the default value to the prefs file if it is not
		// found or is of the wrong type. If no default value is provided, the
		// pref is still added but no value is assigned to it.
		//
		// Pass in NULL for the default value or 0 for the default value length if it is not known.
		//
		// This function logs an error if there was a default value provided.
		static void	GetPref(QTSS_ModulePrefsObject inPrefsObject, char* inPrefName, QTSS_AttrDataType inType,
							void* ioBuffer, void* inDefaultValue, UInt32 inBufferLen);
							
		//
		// GET STRING PREF
		//
		// Does the same thing as GetPref, but does it for string prefs. Returns a newly
		// allocated buffer with the pref value inside it.
		//
		// Pass in NULL for the default value or an empty string if the default is not known.
		static char* GetStringPref(QTSS_ModulePrefsObject inPrefsObject, char* inPrefName, char* inDefaultValue);

		//
		// GET ATTR ID
		//
		// Given an attribute in an object, returns its attribute ID
		// or qtssIllegalAttrID if it isn't found.
		static QTSS_AttributeID GetAttrID(QTSS_Object inObject, char* inPrefName);
		
	private:
	
		//
		// Used in the implementation of the above functions
		static QTSS_AttributeID CheckPrefDataType(QTSS_Object inPrefsObject, char* inPrefName, QTSS_AttrDataType inType, void* inDefaultValue, UInt32 inBufferLen);
		static QTSS_AttributeID	CreatePrefAttr(QTSS_ModulePrefsObject inPrefsObject, char* inPrefName, QTSS_AttrDataType inType, void* inDefaultValue, UInt32 inBufferLen);

		static QTSS_TextMessagesObject 	sMessages;
		static QTSS_ServerObject 		sServer;
		static QTSS_StreamRef 			sErrorLog;
};

#endif //_QTSS_MODULE_UTILS_H_
