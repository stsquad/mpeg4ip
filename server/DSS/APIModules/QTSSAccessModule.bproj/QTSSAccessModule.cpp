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
	File:		QTSSAccessModule.cpp

	Contains:	Implementation of QTSSAccessModule. 
					
	

*/

#include "QTSSAccessModule.h"


#include "OSArrayObjectDeleter.h"
#include "QTSS_Private.h"
#include "StrPtrLen.h"
#include "OSMemory.h"
#include "MyAssert.h"
#include "StringFormatter.h"
#include "StrPtrLen.h"
#include "StringParser.h"
#include "QTSSModuleUtils.h"
#include "base64.h"
#include "OS.h"
#include "AccessChecker.h"

#ifndef __Win32__
#include <unistd.h>
#endif

#include <fcntl.h>
#include <errno.h>

#pragma mark _QTSS_AUTHENTICATE_MODULE_

// ATTRIBUTES

// STATIC DATA


#define MODPREFIX_ "modAccess_"

static StrPtrLen	sSDPSuffix(".sdp");
static OSMutex*		sUserMutex 				= NULL;

//static Bool16			sDefaultAuthenticationEnabled	= true;
//static Bool16			sAuthenticationEnabled			= true;

#ifdef __Win32__
static char* sDefaultUsersFilePath = "c:\\Program Files\\Darwin Streaming Server\\qtusers";
#else
static char* sDefaultUsersFilePath = "/etc/streaming/qtusers";
#endif
static char* sUsersFilePath = NULL;

#ifdef __Win32__
static char* sDefaultGroupsFilePath = "c:\\Program Files\\Darwin Streaming Server\\qtgroups";
#else
static char* sDefaultGroupsFilePath = "/etc/streaming/qtgroups";
#endif
static char* sGroupsFilePath = NULL;

static char* sDefaultAccessFileName = "qtaccess";
static char* sAccessFileName = NULL;

static QTSS_AttributeID	sBadNameMessageAttrID				= qtssIllegalAttrID;
const int kBuffLen = 512;

static QTSS_StreamRef 			sErrorLogStream = NULL;
static QTSS_TextMessagesObject  sMessages = NULL;
static QTSS_ModulePrefsObject	sPrefs = NULL;

// FUNCTION PROTOTYPES

static QTSS_Error QTSSAccessModuleDispatch(QTSS_Role inRole, QTSS_RoleParamPtr inParams);
static QTSS_Error Register();
static QTSS_Error Initialize(QTSS_Initialize_Params* inParams);
static QTSS_Error Shutdown();
static QTSS_Error RereadPrefs();
static QTSS_Error AuthenticateRTSPRequest(QTSS_StandardRTSP_Params* inParams);
static bool QTSSAccess(QTSS_StandardRTSP_Params* inParams, const char* pathBuff, const char* movieRootDir, char* ioRealmName);
static char* GetCheckedFileName();


// FUNCTION IMPLEMENTATIONS
#pragma mark __QTSS_AUTHENTICATE_MODULE__

QTSS_Error QTSSAccessModule_Main(void* inPrivateArgs)
{
	return _stublibrary_main(inPrivateArgs, QTSSAccessModuleDispatch);
}


QTSS_Error 	QTSSAccessModuleDispatch(QTSS_Role inRole, QTSS_RoleParamPtr inParams)
{
	switch (inRole)
	{
		case QTSS_Register_Role:
			return Register();
		break;
		
		case QTSS_Initialize_Role:
			return Initialize(&inParams->initParams);
		break;
		
		case QTSS_RereadPrefs_Role:
			return RereadPrefs();
		break;
			
		case QTSS_RTSPAuthorize_Role:
//			if (sAuthenticationEnabled)
				return AuthenticateRTSPRequest(&inParams->rtspRequestParams);
		break;
			
		case QTSS_Shutdown_Role:
			return Shutdown();
		break;
	}
	
	return QTSS_NoErr;
}

QTSS_Error Register()
{
	// Do role & attribute setup
	(void)QTSS_AddRole(QTSS_Initialize_Role);
	(void)QTSS_AddRole(QTSS_RereadPrefs_Role);
	(void)QTSS_AddRole(QTSS_RTSPAuthorize_Role);
		
	// Add AuthenticateName and Password attributes
	static char*		sBadAccessFileName	= "QTSSAccessModuleBadAccessFileName";
	
	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sBadAccessFileName, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sBadAccessFileName, &sBadNameMessageAttrID);
	
	return QTSS_NoErr;
}


QTSS_Error Initialize(QTSS_Initialize_Params* inParams)
{
	// Setup module utils
	QTSSModuleUtils::Initialize(inParams->inMessages, inParams->inPrefs, inParams->inErrorLogStream);
	sErrorLogStream = inParams->inErrorLogStream;
	sMessages = inParams->inMessages;
	sPrefs = QTSSModuleUtils::GetModulePrefsObject(inParams->inModule);
	sUserMutex = NEW OSMutex();
	RereadPrefs();

	return QTSS_NoErr;
}

QTSS_Error Shutdown()
{
	return QTSS_NoErr;
}

char* GetCheckedFileName()
{
	char		*result = NULL;
	static char *badChars = "/'\"";
	char		theBadCharMessage[] = "' '";
	char		*theBadChar = NULL;
	result = QTSSModuleUtils::GetStringPref(sPrefs, MODPREFIX_"qtaccessfilename", sDefaultAccessFileName);
	StrPtrLen searchStr(result);
	
	theBadChar = strpbrk(searchStr.Ptr, badChars);
	if ( theBadChar!= NULL) 
	{
		theBadCharMessage[1] = theBadChar[0];
		QTSSModuleUtils::LogError(qtssWarningVerbosity,sBadNameMessageAttrID, 0, theBadCharMessage, sDefaultAccessFileName);
				
		delete[] result;
		result = NEW char[::strlen(sDefaultAccessFileName) + 2];
		::strcpy(result, sDefaultAccessFileName);	
	}
	return result;
}

QTSS_Error RereadPrefs()
{
	OSMutexLocker locker(sUserMutex);
	
	//
	// Use the standard GetPref routine to retrieve the correct values for our preferences
	//QTSSModuleUtils::GetPref(sPrefs, MODPREFIX_"enabled", 	qtssAttrDataTypeBool16,
	//						&sAuthenticationEnabled, &sDefaultAuthenticationEnabled, sizeof(sAuthenticationEnabled));

	delete[] sUsersFilePath;
	delete[] sGroupsFilePath;
	delete[] sAccessFileName;
	
	sUsersFilePath = QTSSModuleUtils::GetStringPref(sPrefs, MODPREFIX_"usersfilepath", sDefaultUsersFilePath);
	sGroupsFilePath = QTSSModuleUtils::GetStringPref(sPrefs, MODPREFIX_"groupsfilepath", sDefaultGroupsFilePath);
	sAccessFileName = GetCheckedFileName();

	return QTSS_NoErr;
}

bool QTSSAccess(QTSS_StandardRTSP_Params* inParams, 
				const char* pathBuff, 
				const char* movieRootDir,
				char* ioRealmName)
{
	QTSS_RTSPRequestObject	theRTSPRequest = inParams->inRTSPRequest;		
	
	AccessChecker accessChecker(movieRootDir, sAccessFileName, sUsersFilePath, sGroupsFilePath);
	
	//If there are no access files, then allow world access
	if ( !accessChecker.GetAccessFile(pathBuff) ) 
		return true;

	if ( accessChecker.GetRealmHeaderPtr() != NULL )
		::strcpy(ioRealmName, accessChecker.GetRealmHeaderPtr());
		
	char*	nameBuffStr = NULL;
	QTSS_Error theErr = QTSS_GetValueAsString(theRTSPRequest, qtssRTSPReqUserName, 0, &nameBuffStr);
	OSCharArrayDeleter nameBuffDeleter(nameBuffStr);
	if (theErr != QTSS_NoErr)
		return false;
	
	char*	passwordBuffStr = NULL;
	theErr = QTSS_GetValueAsString(theRTSPRequest,qtssRTSPReqUserPassword, 0, &passwordBuffStr);
	OSCharArrayDeleter passwordBuffDeleter(passwordBuffStr);
	if (theErr != QTSS_NoErr)
		return false;
				
	if ( accessChecker.CheckAccess(nameBuffStr, passwordBuffStr) )
		return true;

	return false;
}


QTSS_Error AuthenticateRTSPRequest(QTSS_StandardRTSP_Params* inParams)
{
	QTSS_RTSPRequestObject	theRTSPRequest = inParams->inRTSPRequest;
	
	OSMutexLocker locker(sUserMutex);

	if  ( (NULL == inParams) || (NULL == inParams->inRTSPRequest) )
		return QTSS_RequestFailed;
	
	//get the local file path
	char*	pathBuffStr = NULL;
	QTSS_Error theErr = QTSS_GetValueAsString(theRTSPRequest, qtssRTSPReqLocalPath, 0, &pathBuffStr);
	OSCharArrayDeleter pathBuffDeleter(pathBuffStr);
	if (theErr != QTSS_NoErr)
		return QTSS_RequestFailed;	

	//get the root movie directory
	char*	movieRootDirStr = NULL;
	theErr = QTSS_GetValueAsString(theRTSPRequest,qtssRTSPReqRootDir, 0, &movieRootDirStr);
	OSCharArrayDeleter movieRootDeleter(movieRootDirStr);
	if (theErr != QTSS_NoErr)
		return false;

	//check if this user is allowed to see this movie
	char realmName[kBuffLen] = { 0 };
	StrPtrLen	realmNameStr(realmName,kBuffLen -1);
	Bool16 allowRequest = ::QTSSAccess(inParams, pathBuffStr, movieRootDirStr, realmName);


	if ( realmName[0] != '\0' ) 	//set the realm if we have one
	{
		(void) QTSS_SetValue(theRTSPRequest,qtssRTSPReqURLRealm, 0, realmName, strlen(realmName) );
	}
	

	if (allowRequest)
	{
		return QTSS_NoErr;	//everything's kosher - let the request continue
	}
	else	//request denied
	{
		// We are denying the request so pass false back to the server.
		theErr = QTSS_SetValue(theRTSPRequest,qtssRTSPReqUserAllowed, 0, &allowRequest, sizeof(allowRequest));
		if (theErr != QTSS_NoErr) 
			return QTSS_RequestFailed; // Bail on the request. The Server will handle the error
		
	}

	return QTSS_NoErr;
}







