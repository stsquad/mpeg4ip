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
	File:		AccessCheck.h

	Contains:	
					
	Created By: Chris LeCroy

	Created: Mon, Sep 27, 1999 @ 4:26 PM
*/

#ifndef _QTSSACCESSCHECK_H_
#define _QTSSACCESSCHECK_H_

#include "QTSS.h"
#include "StrPtrLen.h"
#include <stdio.h>

class AccessChecker
{
/*
	Access check logic:
	
	If "modAccess_enabled" == "enabled,
	Starting at URL dir, walk up directories to Movie Folder until a "qtaccess" file is found
		If not found, 
			allow access
		If found, 
			send a challenge to the client
			verify user against QTSSPasswd
			verify that user or member group is in the lowest ".qtacess"
			walk up directories until a ".qtaccess" is found
			If found,
				allow access
			If not found, 
				deny access
				
	ToDo:
		would probably be a good idea to do some caching of ".qtacces" data to avoid
		multiple directory walks
*/

public:
	AccessChecker(const char* inMovieRootDir, const char* inQTAccessFileName, const char* inDefaultUsersFilePath, const char* inDefaultGroupsFilePath);
	virtual ~AccessChecker();
	bool CheckAccess(const char* inUsername, const char* inPassword);
	bool CheckPassword(const char* inUsername, const char* inPassword);
	void GetPassword(const char* inUsername, char* ioPassword);
	bool CheckUserAccess(const char* inUsername);
	bool CheckGroupMembership(const char* inUsername, const StrPtrLen& inGroupName);
	bool GetAccessFile(const char* dirPath);

	inline char* GetRealmHeaderPtr() {return fRealmHeader;}


protected:
	char* fRealmHeader;
	char* fMovieRootDir;
	char* fQTAccessFileName;
	char* fGroupsFilePath;
	char* fUsersFilePath;
	FILE*  fAccessFile;
	FILE*  fUsersFile;
	FILE*  fGroupsFile;
	
	static const char* kDefaultUsersFilePath;
	static const char* kDefaultGroupsFilePath;
	static const char* kDefaultAccessFileName;
	static const char* kDefaultRealmHeader;
	
	void GetAccessFileInfo(const  char* inQTAccessDir);
	
};

#endif //_QTSSACCESSCHECK_H_
