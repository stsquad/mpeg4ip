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
	File:		RelayPrefsSource.h

	Contains:	Implements the PrefsSource interface, getting the prefs from a file.

	Written by:	Chris LeCroy

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	Change History (most recent first):

	$Log: RelayPrefsSource.h,v $
	Revision 1.2  2001/05/09 21:04:37  cahighlander
	Sync to 0.6.2
	
	Revision 1.1.1.1  2001/02/26 23:27:30  dmackie
	Import of Apple Darwin Streaming Server 3 Public Preview
	
	Revision 1.1.1.1  2000/08/31 00:30:40  serenyi
	Mothra Repository
	
	Revision 1.1  2000/07/05 15:54:25  serenyi
	Made it so the relay configuration file can either have " lines or non " lines
	
	Revision 1.4  2000/01/21 23:52:48  rtucker
	
	logging changes
	
	Revision 1.3  1999/12/21 01:43:47  lecroy
	*** empty log message ***
	
	Revision 1.2  1999/10/07 08:56:04  serenyi
	Changed bool to Bool16, changed QTSS_Dictionary to QTSS_Object
	
	Revision 1.1.1.1  1999/09/22 23:01:31  lecroy
	importing
	
	Revision 1.1.1.1  1999/09/13 16:16:43  rtucker
	initial checkin from rrv14
	
	
	Revision 1.1  1999/06/01 20:50:03  serenyi
	new files for linux
	

*/

#ifndef __RELAYPREFSSOURCE_H__
#define __RELAYPREFSSOURCE_H__

#include "PrefsSource.h"
#include "OSHeaders.h"

class RelayKeyValuePair; //only used in the implementation

class RelayPrefsSource : public PrefsSource
{
	public:
	
		RelayPrefsSource( Bool16 allowDuplicates = false );
		virtual ~RelayPrefsSource(); 
	
		virtual int		GetValue(const char* inKey, char* ioValue);
		virtual int		GetValueByIndex(const char* inKey, UInt32 inIndex, char* ioValue);

		// Allows caller to iterate over all the values in the file.
		char*			GetValueAtIndex(UInt32 inIndex);
		char*			GetKeyAtIndex(UInt32 inIndex);
		UInt32			GetNumKeys() { return fNumKeys; }
		
        int InitFromConfigFile(const char* configFilePath);

	private:
	
protected:

 		void 			SetValue(const char* inKey, const char* inValue);
		
       RelayKeyValuePair* 	FindValue(const char* inKey, char* ioValue);
        RelayKeyValuePair* 	fKeyValueList;
        UInt32 			fNumKeys;
        Bool16 fAllowDuplicates;
};

#endif //__FILEPREFSSOURCE_H__
