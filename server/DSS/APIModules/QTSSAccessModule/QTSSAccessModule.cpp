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
#include "base64.h"
#include "OS.h"
#include "AccessChecker.h"
#include "QTAccessFile.h"
#include "QTSSModuleUtils.h"

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

static QTSS_AttributeID	sBadNameMessageAttrID				= qtssIllegalAttrID;
static QTSS_AttributeID	sUsersFileNotFoundMessageAttrID		= qtssIllegalAttrID;
static QTSS_AttributeID	sGroupsFileNotFoundMessageAttrID	= qtssIllegalAttrID;
static QTSS_AttributeID	sBadUsersFileMessageAttrID			= qtssIllegalAttrID;
static QTSS_AttributeID	sBadGroupsFileMessageAttrID			= qtssIllegalAttrID;

static QTSS_StreamRef 			sErrorLogStream = NULL;
static QTSS_TextMessagesObject  sMessages = NULL;
static QTSS_ModulePrefsObject	sPrefs = NULL;
static QTSS_PrefsObject 		sServerPrefs = NULL;

static AccessChecker**			sAccessCheckers;
static UInt32					sNumAccessCheckers = 0;
static UInt32					sAccessCheckerArraySize = 0;


// FUNCTION PROTOTYPES

static QTSS_Error QTSSAccessModuleDispatch(QTSS_Role inRole, QTSS_RoleParamPtr inParams);
static QTSS_Error Register();
static QTSS_Error Initialize(QTSS_Initialize_Params* inParams);
static QTSS_Error Shutdown();
static QTSS_Error RereadPrefs();
static QTSS_Error AuthenticateRTSPRequest(QTSS_RTSPAuth_Params* inParams);
static QTSS_Error AccessAuthorizeRTSPRequest(QTSS_StandardRTSP_Params* inParams);

static void 	  FindUsersAndGroupsFiles(char* inAccessFilePath, char** outUsersFilePath, char** outGroupsFilePath);
static char* 	  GetCheckedFileName();

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
		
		case QTSS_RTSPAuthenticate_Role:
//			if (sAuthenticationEnabled)
				return AuthenticateRTSPRequest(&inParams->rtspAthnParams);
		break;
		
		case QTSS_RTSPAuthorize_Role:
//			if (sAuthenticationEnabled)
				return AccessAuthorizeRTSPRequest(&inParams->rtspRequestParams);
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
	(void)QTSS_AddRole(QTSS_RTSPAuthenticate_Role);
	(void)QTSS_AddRole(QTSS_RTSPAuthorize_Role);
		
	// Add AuthenticateName and Password attributes
	static char*		sBadAccessFileName	= "QTSSAccessModuleBadAccessFileName";
	static char*		sUsersFileNotFound	= "QTSSAccessModuleUsersFileNotFound";
	static char*		sGroupsFileNotFound	= "QTSSAccessModuleGroupsFileNotFound";
	static char*		sBadUsersFile		= "QTSSAccessModuleBadUsersFile";
	static char*		sBadGroupsFile		= "QTSSAccessModuleBadGroupsFile";
	
	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sBadAccessFileName, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sBadAccessFileName, &sBadNameMessageAttrID);
	
	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sUsersFileNotFound, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sUsersFileNotFound, &sUsersFileNotFoundMessageAttrID);
	
	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sGroupsFileNotFound, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sGroupsFileNotFound, &sGroupsFileNotFoundMessageAttrID);
	
	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sBadUsersFile, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sBadUsersFile, &sBadUsersFileMessageAttrID);
	
	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sBadGroupsFile, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sBadGroupsFile, &sBadGroupsFileMessageAttrID);
	
	return QTSS_NoErr;
}


QTSS_Error Initialize(QTSS_Initialize_Params* inParams)
{
	// Create an array of AccessCheckers
	sAccessCheckers = NEW AccessChecker*[2];
	sAccessCheckers[0] = NEW AccessChecker();
	sNumAccessCheckers = 1;
	sAccessCheckerArraySize = 2;

	// Setup module utils
	QTSSModuleUtils::Initialize(inParams->inMessages, inParams->inPrefs, inParams->inErrorLogStream);
	sErrorLogStream = inParams->inErrorLogStream;
	sMessages = inParams->inMessages;
	sPrefs = QTSSModuleUtils::GetModulePrefsObject(inParams->inModule);
	sServerPrefs = inParams->inPrefs;
	sUserMutex = NEW OSMutex();
	RereadPrefs();
	QTAccessFile::Initialize();
	
	return QTSS_NoErr;
}

QTSS_Error Shutdown()
{
	//cleanup
	
	// delete all the AccessCheckers
	UInt32 index;
	for(index = 0; index < sNumAccessCheckers; index++)
		delete sAccessCheckers[index];
	delete[] sAccessCheckers;
	sNumAccessCheckers = 0;
	
	// delete the main users and groups path
	
	//if(sUsersFilePath != sDefaultUsersFilePath) 
	// sUsersFilePath is assigned by a call to QTSSModuleUtils::GetStringPref which always
	// allocates memory even if it just returns the default value
	delete[] sUsersFilePath;
	sUsersFilePath = NULL;
	
	//if(sGroupsFilePath != sDefaultGroupsFilePath)
	// sGroupsFilePath is assigned by a call to QTSSModuleUtils::GetStringPref which always
	// allocates memory even if it just returns the default value
	delete[] sGroupsFilePath;
	sGroupsFilePath = NULL;
	
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
		QTSSModuleUtils::LogError(qtssWarningVerbosity,sBadNameMessageAttrID, 0, theBadCharMessage, result);
				
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
	
	//if(sUsersFilePath != sDefaultUsersFilePath)
	// sUsersFilePath is assigned by a call to QTSSModuleUtils::GetStringPref which always
	// allocates memory even if it just returns the default value
	// delete this old memory before reassigning it to new memory
	delete[] sUsersFilePath;
	sUsersFilePath = NULL;
		
	//if(sGroupsFilePath != sDefaultGroupsFilePath)
	// sGroupsFilePath is assigned by a call to QTSSModuleUtils::GetStringPref which always
	// allocates memory even if it just returns the default value
	// delete this old memory before reassigning it to new memory
	delete[] sGroupsFilePath;
	sGroupsFilePath = NULL;
	
	sUsersFilePath = QTSSModuleUtils::GetStringPref(sPrefs, MODPREFIX_"usersfilepath", sDefaultUsersFilePath);
	sGroupsFilePath = QTSSModuleUtils::GetStringPref(sPrefs, MODPREFIX_"groupsfilepath", sDefaultGroupsFilePath);
	// GetCheckedFileName always allocates memory
	char* accessFile = GetCheckedFileName();
	// QTAccessFile::SetAccessFileName makes its own copy, 
	// so delete the previous allocated memory after this call
	QTAccessFile::SetAccessFileName(accessFile);
	delete [] accessFile;
	
	if(sAccessCheckers[0]->HaveFilePathsChanged(sUsersFilePath, sGroupsFilePath))
	{
		sAccessCheckers[0]->UpdateFilePaths(sUsersFilePath, sGroupsFilePath);
		UInt32 err;
		err = sAccessCheckers[0]->UpdateUserProfiles();
		if(err & AccessChecker::kUsersFileNotFoundErr)
			QTSSModuleUtils::LogError(qtssWarningVerbosity,sUsersFileNotFoundMessageAttrID, 0, sUsersFilePath, NULL);
		else if(err & AccessChecker::kBadUsersFileErr)
			QTSSModuleUtils::LogError(qtssWarningVerbosity,sBadUsersFileMessageAttrID, 0, sUsersFilePath, NULL);
		if(err & AccessChecker::kGroupsFileNotFoundErr)
			QTSSModuleUtils::LogError(qtssWarningVerbosity,sGroupsFileNotFoundMessageAttrID, 0, sGroupsFilePath, NULL);
		else if(err & AccessChecker::kBadGroupsFileErr)
			QTSSModuleUtils::LogError(qtssWarningVerbosity,sBadGroupsFileMessageAttrID, 0, sGroupsFilePath, NULL);
	}
	
	return QTSS_NoErr;
}

QTSS_Error AuthenticateRTSPRequest(QTSS_RTSPAuth_Params* inParams)
{
	QTSS_RTSPRequestObject	theRTSPRequest = inParams->inRTSPRequest;
	UInt32 fileErr;
	
	OSMutexLocker locker(sUserMutex);

	if  ( (NULL == inParams) || (NULL == inParams->inRTSPRequest) )
		return QTSS_RequestFailed;

	// Get the user profile object from the request object
	QTSS_UserProfileObject theUserProfile = NULL;
	UInt32 len = sizeof(QTSS_UserProfileObject);
	QTSS_Error theErr = QTSS_GetValue(theRTSPRequest, qtssRTSPReqUserProfile, 0, (void*)&theUserProfile, &len);
	Assert(len == sizeof(QTSS_UserProfileObject));
	if (theErr != QTSS_NoErr)
		return theErr;
		
	Bool16 defaultPaths = true;
	// Check for a users and groups file in the access file
	// For this, first get local file path and root movie directory
	//get the local file path
	char*	pathBuffStr = QTSSModuleUtils::GetLocalPath_Copy(theRTSPRequest);
	OSCharArrayDeleter pathBuffDeleter(pathBuffStr);
	if (NULL == pathBuffStr)
		return QTSS_RequestFailed;
	//get the root movie directory
	char*	movieRootDirStr = QTSSModuleUtils::GetMoviesRootDir_Copy(theRTSPRequest);
	OSCharArrayDeleter movieRootDeleter(movieRootDirStr);
	if (NULL == movieRootDirStr)
		return QTSS_RequestFailed;
	// Now get the access file path
	char* accessFilePath = QTAccessFile::GetAccessFile_Copy(movieRootDirStr, pathBuffStr);
	OSCharArrayDeleter accessFilePathDeleter(accessFilePath);
	// Parse the access file for the AuthUserFile and AuthGroupFile keywords
	char* usersFilePath = NULL;
	char* groupsFilePath = NULL;
	// Allocates memory for usersFilePath and groupsFilePath
	FindUsersAndGroupsFiles(accessFilePath, &usersFilePath, &groupsFilePath);
	
	if((usersFilePath != NULL) || (groupsFilePath != NULL)) 
		defaultPaths = false;
		
	if(usersFilePath == NULL)
		usersFilePath = sUsersFilePath;
		
	if(groupsFilePath == NULL)
		groupsFilePath = sGroupsFilePath;
		
	AccessChecker* currentChecker = NULL;
	UInt32 index;
	
	// If the default users and groups file are not the ones we need
	if(!defaultPaths) 
	{
		// check if there is one AccessChecker that matches the needed paths
		// Don't have to check for the first one (or element zero) because it has the default paths
		for(index = 1; index < sNumAccessCheckers; index++)
		{
			// If an access checker that matches the users and groups file paths is found
			if(!sAccessCheckers[index]->HaveFilePathsChanged(usersFilePath, groupsFilePath))
			{
				currentChecker = sAccessCheckers[index];
				break;
			}						
		}
		// If an existing AccessChecker for the needed paths isn't found
		if(currentChecker == NULL)
		{
			// Grow the AccessChecker array if needed
			if(sNumAccessCheckers == sAccessCheckerArraySize)
			{
				AccessChecker** oldAccessCheckers = sAccessCheckers;
				sAccessCheckers = NEW AccessChecker*[sAccessCheckerArraySize * 2];
				for(index = 0; index < sNumAccessCheckers; index++)
				{
					sAccessCheckers[index] = oldAccessCheckers[index];
				}
				sAccessCheckerArraySize *= 2;
				delete [] oldAccessCheckers;
			}
		
			// And create a new AccessChecker for the paths
			sAccessCheckers[sNumAccessCheckers] = NEW AccessChecker();
			sAccessCheckers[sNumAccessCheckers]->UpdateFilePaths(usersFilePath, groupsFilePath);
			fileErr = sAccessCheckers[sNumAccessCheckers]->UpdateUserProfiles();
			
			if(fileErr & AccessChecker::kUsersFileNotFoundErr)
				QTSSModuleUtils::LogError(qtssWarningVerbosity,sUsersFileNotFoundMessageAttrID, 0, usersFilePath, NULL);
			else if(fileErr & AccessChecker::kBadUsersFileErr)
				QTSSModuleUtils::LogError(qtssWarningVerbosity,sBadUsersFileMessageAttrID, 0, usersFilePath, NULL);
			if(fileErr & AccessChecker::kGroupsFileNotFoundErr)
				QTSSModuleUtils::LogError(qtssWarningVerbosity,sGroupsFileNotFoundMessageAttrID, 0, groupsFilePath, NULL);
			else if(fileErr & AccessChecker::kBadGroupsFileErr)
				QTSSModuleUtils::LogError(qtssWarningVerbosity,sBadGroupsFileMessageAttrID, 0, groupsFilePath, NULL);
				
			currentChecker = sAccessCheckers[sNumAccessCheckers];
			sNumAccessCheckers++;
		}
		
		// If the usersFilePath is the same as the main users file path, no new memory is allocated
		// because FindUsersAndGroupsFiles returned NULL, and we just copied the pointer
		// Otherwise, delete the memory allocated
		if(strcmp(usersFilePath, sUsersFilePath) != 0)	
			delete usersFilePath;
		
		// If the groupsFilePath is the same as the main groups file path, no new memory is allocated
		// because FindUsersAndGroupsFiles returned NULL, and we just copied the pointer
		// Otherwise, delete the memory allocated
		if(strcmp(groupsFilePath, sGroupsFilePath) != 0)	
			delete groupsFilePath;
	}
	else
	{
		currentChecker = sAccessCheckers[0];
	}
	
	// Before retrieving the user profile information
	// check if the groups/users files have been modified and update them otherwise
    fileErr = currentChecker->UpdateUserProfiles();
	
	/*
	// This is for logging the errors if users file and/or the groups file is not found or corrupted
	char* usersFile = currentChecker->GetUsersFilePathPtr();
	char* groupsFile = currentChecker->GetGroupsFilePathPtr();
	
	if(fileErr & AccessChecker::kUsersFileNotFoundErr)
		QTSSModuleUtils::LogError(qtssWarningVerbosity,sUsersFileNotFoundMessageAttrID, 0, usersFile, NULL);
	else if(fileErr & AccessChecker::kBadUsersFileErr)
		QTSSModuleUtils::LogError(qtssWarningVerbosity,sBadUsersFileMessageAttrID, 0, usersFile, NULL);
	if(fileErr & AccessChecker::kGroupsFileNotFoundErr)
		QTSSModuleUtils::LogError(qtssWarningVerbosity,sGroupsFileNotFoundMessageAttrID, 0, groupsFile, NULL);
	else if(fileErr & AccessChecker::kBadGroupsFileErr)
		QTSSModuleUtils::LogError(qtssWarningVerbosity,sBadGroupsFileMessageAttrID, 0, groupsFile, NULL);
      */

	// Retrieve the password data and group information for the user and set them
	// in the qtssRTSPReqUserProfile attr
	// The password data is crypt of the real password for Basic authentication
	// and it is MD5(username:realm:password) for Digest authentication
	
	// Get the authentication scheme from the request object
	QTSS_AuthScheme theAuthScheme = qtssAuthNone;
	len = sizeof(theAuthScheme);
	theErr = QTSS_GetValue(theRTSPRequest, qtssRTSPReqAuthScheme, 0, (void*)&theAuthScheme, &len);
	Assert(len == sizeof(theAuthScheme));
	if (theErr != QTSS_NoErr)
		return theErr;
	
	// Set the qtssUserRealm to the realm value retrieved from the users file
	// This should be used for digest auth scheme, and if no realm is found in the qtaccess file, then
	// it should be used for basic auth scheme.
	// No memory is allocated; just a pointer is returned
	StrPtrLen* authRealm = currentChecker->GetAuthRealm();
	(void)QTSS_SetValue(theUserProfile, qtssUserRealm, 0, (void*)(authRealm->Ptr), (authRealm->Len));
	
	
	// Get the username from the user profile object
	char*	usernameBuf = NULL;
	theErr = QTSS_GetValueAsString(theUserProfile, qtssUserName, 0, &usernameBuf);
	OSCharArrayDeleter usernameBufDeleter(usernameBuf);
	StrPtrLen username(usernameBuf);
	if (theErr != QTSS_NoErr)
		return theErr;
	
	// No memory is allocated; just a pointer to the profile is returned
	AccessChecker::UserProfile* profile = currentChecker->RetrieveUserProfile(&username);
	
	if(profile == NULL)
		return QTSS_NoErr;
		
	// Set the qtssUserPassword attribute to either the crypted password or the digest password
	// based on the authentication scheme
	if(theAuthScheme == qtssAuthBasic) 
		(void)QTSS_SetValue(theUserProfile, qtssUserPassword, 0, (void*)((profile->cryptPassword).Ptr), (profile->cryptPassword).Len);
	else if(theAuthScheme == qtssAuthDigest)
		(void)QTSS_SetValue(theUserProfile, qtssUserPassword, 0, (void*)((profile->digestPassword).Ptr), (profile->digestPassword).Len);	
	
	
	// Set the multivalued qtssUserGroups attr to the groups the user belongs to, if any
	UInt32 maxLen = profile->maxGroupNameLen;
	for(index = 0; index < profile->numGroups; index++) 
	{
		UInt32 curLen = ::strlen(profile->groups[index]);
		if(curLen < maxLen) 
		{
			char* groupWithPaddedZeros = NEW char[maxLen];	// memory allocated
			::memcpy(groupWithPaddedZeros, profile->groups[index], curLen);
			::memset(groupWithPaddedZeros+curLen, '\0', maxLen-curLen);
			(void)QTSS_SetValue(theUserProfile, qtssUserGroups, index, (void*)groupWithPaddedZeros, maxLen);
			delete [] groupWithPaddedZeros;					// memory deleted
		}
		else 
		{
			(void)QTSS_SetValue(theUserProfile, qtssUserGroups, index, (void*)(profile->groups[index]), maxLen);
		}
	}
	
	return QTSS_NoErr;
}

// Memory allocated for outUsersFilePath and outGroupsFilePath - remember to delete it!
void FindUsersAndGroupsFiles(char* inAccessFilePath, char** outUsersFilePath, char** outGroupsFilePath)
{
	if (inAccessFilePath == NULL)
		return;
		
	*outUsersFilePath = NULL;
	*outGroupsFilePath = NULL;
	//Assert(outUsersFilePath == NULL);
	//Assert(outGroupsFilePath == NULL);
	
	StrPtrLen accessFileBuf;
	(void)QTSSModuleUtils::ReadEntireFile(inAccessFilePath, &accessFileBuf);
	OSCharArrayDeleter accessFileBufDeleter(accessFileBuf.Ptr);
	
	StringParser accessFileParser(&accessFileBuf);
	StrPtrLen line;
	
	while( accessFileParser.GetDataRemaining() != 0 ) 
	{
		accessFileParser.GetThruEOL(&line);						// Read each line	
		StringParser lineParser(&line);
		lineParser.ConsumeWhitespace();							//skip over leading whitespace
		if (lineParser.GetDataRemaining() == 0)					// must be an empty line
			continue;

		char firstChar = lineParser.PeekFast();
		if ( (firstChar == '#') || (firstChar == '\0') )
	  		continue;											//skip over comments and blank lines...
		
		StrPtrLen word;
		lineParser.ConsumeUntilWhitespace(&word);
		if (word.Equal("AuthUserFile") )
		{
			lineParser.ConsumeWhitespace();
			lineParser.ConsumeUntilWhitespace(&word);		
			if(*outUsersFilePath != NULL)				// we are encountering the AuthUserFile keyword twice!
				delete[] *outUsersFilePath;				// The last one found takes precedence...delete the previous path
			*outUsersFilePath = word.GetAsCString();
			continue;
		}
		
		if (word.Equal("AuthGroupFile") )
		{
			lineParser.ConsumeWhitespace();
			lineParser.ConsumeUntilWhitespace(&word);
			if(*outGroupsFilePath != NULL)				// we are encountering the AuthGroupFile keyword twice!
				delete[] *outGroupsFilePath;				// The last one found takes precedence...delete the previous path		
			*outGroupsFilePath = word.GetAsCString();
			continue;
		}
	}
}


QTSS_Error AccessAuthorizeRTSPRequest(QTSS_StandardRTSP_Params* inParams)
{	
	Bool16 allowNoAccessFiles = true;
	QTSS_ActionFlags noAction = qtssActionFlagsWrite;
	QTSS_ActionFlags authorizeAction = qtssActionFlagsRead;

	return  QTAccessFile::AuthorizeRequest(inParams,allowNoAccessFiles, noAction, authorizeAction);
}
