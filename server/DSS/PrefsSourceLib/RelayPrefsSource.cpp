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
	File:		FilePrefsSource.cpp

	Contains:	Implements object defined in FilePrefsSource.h.

	Written by:	Chris LeCroy

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	Change History (most recent first):

	Revision 1.3  2001/10/01 22:08:39  dmackie
	DSS 3.0.1 import
	
	Revision 1.3  2001/03/13 22:24:27  murata
	Replace copyright notice for license 1.0 with license 1.2 and update the copyright year.
	Bug #:
	Submitted by:
	Reviewed by:
	
	Revision 1.2  2000/10/11 07:20:04  serenyi
	import
	
	Revision 1.2  2000/09/29 19:28:15  serenyi
	Fixed yet more win32 issues
	
	Revision 1.1.1.1  2000/08/31 00:30:40  serenyi
	Mothra Repository
	
	Revision 1.1  2000/07/05 15:54:25  serenyi
	Made it so the relay configuration file can either have " lines or non " lines
	
	Revision 1.9  2000/01/21 23:52:48  rtucker
	
	logging changes
	
	Revision 1.8  2000/01/19 19:39:38  serenyi
	Moved to new NEW macro
	
	Revision 1.7  1999/12/21 01:43:47  lecroy
	*** empty log message ***
	
	Revision 1.6  1999/11/15 01:23:20  serenyi
	Added in 4-char debugging codes, merged in solaris changes
	
	Revision 1.5  1999/11/02 04:26:10  serenyi
	Don't check errno on an fopen!
	
	Revision 1.4  1999/10/27 17:56:22  serenyi
	Integrated Matthias's MacOS / GUSI changes
	
	Revision 1.3  1999/10/21 22:07:24  lecroy
	made access & fileprivs modules dynamic, added AFTER_BUILD scripts to strip them and move them around, renamed QTSSPasswd to qtpasswd, probably some other stuff too...
	
	Revision 1.2  1999/10/07 08:56:04  serenyi
	Changed bool to Bool16, changed QTSS_Dictionary to QTSS_Object
	
	Revision 1.1.1.1  1999/09/22 23:01:32  lecroy
	importing
	
	Revision 1.1.1.1  1999/09/13 16:16:43  rtucker
	initial checkin from rrv14
	
	
	Revision 1.1  1999/06/01 20:50:03  serenyi
	new files for linux
	

*/

#include <string.h>
#include <stdio.h>

#include <errno.h>

#include "RelayPrefsSource.h"
#include "MyAssert.h"
#include "OSMemory.h"

const int kMaxLineLen = 2048;
const int kMaxValLen = 1024;

class RelayKeyValuePair
{
 public:
 
	char*	GetValue() { return fValue; }
	
 private:
 	friend class RelayPrefsSource;

	RelayKeyValuePair(const char* inKey, const char* inValue, RelayKeyValuePair* inNext);
	~RelayKeyValuePair();

	char* fKey;
	char* fValue;
	RelayKeyValuePair* fNext;

	 void ResetValue(const char* inValue);
};


RelayKeyValuePair::RelayKeyValuePair(const char* inKey, const char* inValue, RelayKeyValuePair* inNext) :
	fKey(NULL),
	fValue(NULL),
	fNext(NULL)
{
	fKey = NEW char[::strlen(inKey)+1];
	::strcpy(fKey, inKey);
	fValue = NEW char[::strlen(inValue)+1];
	::strcpy(fValue, inValue);
	fNext = inNext;
}


RelayKeyValuePair::~RelayKeyValuePair()
{
	delete[] fKey;
	delete[] fValue;
}

void RelayKeyValuePair::ResetValue(const char* inValue)
{
	delete[] fValue;
	fValue = NEW char[::strlen(inValue)+1];
	::strcpy(fValue, inValue);
}


RelayPrefsSource::RelayPrefsSource( Bool16 allowDuplicates)
:	fKeyValueList(NULL),
	fNumKeys(0),
	fAllowDuplicates(allowDuplicates)
{
	
}

RelayPrefsSource::~RelayPrefsSource()
{
	while (fKeyValueList != NULL)
	{
		RelayKeyValuePair* keyValue = fKeyValueList;
		fKeyValueList = fKeyValueList->fNext;
		delete keyValue;
	}

}

int RelayPrefsSource::GetValue(const char* inKey, char* ioValue)
{
	return (this->FindValue (inKey, ioValue) != NULL);
}


int RelayPrefsSource::GetValueByIndex(const char* inKey, UInt32 inIndex, char* ioValue)
{
	RelayKeyValuePair* thePair = this->FindValue (inKey, NULL);
	if (thePair == NULL)
		return false;
	
	char* valuePtr = thePair->fValue;

	//this function makes the assumption that fValue doesn't start with whitespace
	Assert(*valuePtr != '\t');
	Assert(*valuePtr != ' ');
	
	for (UInt32 count = 0; ((count < inIndex) && (valuePtr != '\0')); count++)
	{
		//go through all the "words" on this line (delimited by whitespace)
		//until we hit the one specified by inIndex

		//we aren't at the proper word yet, so skip...
		while ((*valuePtr != ' ') && (*valuePtr != '\t') && (*valuePtr != '\0'))
			valuePtr++;

		//skip over all the whitespace between words
		while ((*valuePtr == ' ') || (*valuePtr == '\t'))
			valuePtr++;
		
	}
	
	//We've exhausted the data on this line before getting to our pref,
	//so return an error.
	if (*valuePtr == '\0')
		return false;

	//if we are here, then valuePtr is pointing to the beginning of the right word
	while ((*valuePtr != ' ') && (*valuePtr != '\t') && (*valuePtr != '\0'))
		*ioValue++ = *valuePtr++;
	*ioValue = '\0';
	
	return true;
}

char* RelayPrefsSource::GetValueAtIndex(UInt32 inIndex)
{
	// Iterate through the queue until we have the right entry
	RelayKeyValuePair* thePair = fKeyValueList;
	while ((thePair != NULL) && (inIndex-- > 0))
		thePair = thePair->fNext;
		
	if (thePair != NULL)
		return thePair->fValue;
	return NULL;
}

char* RelayPrefsSource::GetKeyAtIndex(UInt32 inIndex)
{
	// Iterate through the queue until we have the right entry
	RelayKeyValuePair* thePair = fKeyValueList;
	while ((thePair != NULL) && (inIndex-- > 0))
		thePair = thePair->fNext;
		
	if (thePair != NULL)
		return thePair->fKey;
	return NULL;
}


int RelayPrefsSource::InitFromConfigFile(const char* configFilePath)
{
	/* 
		load config from specified file.  return non-zero
		in the event of significant error(s).
	
	*/
	
    int err = 0;
    char bufLine[kMaxLineLen] = { 0 };
    char key[kMaxValLen];
    char value[kMaxLineLen];
	
    FILE* fileDesc = ::fopen( configFilePath, "r");

    if (fileDesc == NULL)
    {
    	// report some problem here...
    	err = OSThread::GetErrno();
    	
    	Assert( err );
    }
    else
    {
    
        while (fgets(bufLine, sizeof(bufLine) - 1, fileDesc) != NULL)
        {
            if (bufLine[0] != '#' && bufLine[0] != '\0')
            {
                int i = 0;
                int n = 0;

                while ( bufLine[i] == ' ' || bufLine[i] == '\t')
                        { ++i;}

                n = 0;
                while ( bufLine[i] != ' ' &&
                         bufLine[i] != '\t' &&
                         bufLine[i] != '\n' &&
                         bufLine[i] != '\r' &&
                         bufLine[i] != '\0' &&
						 n < (kMaxLineLen - 1) )
                {
                    key[n++] = bufLine[i++];
                }
                key[n] = '\0';

				//
				// DMS - added specifically for the purpose of parsing relay config
				// files, we now skip over a quote if it exists. Same thing with the
				// end of the line, below.
                while (bufLine[i] == ' ' || bufLine[i] == '\t' || bufLine[i] == '"')
                {++i;}

                n = 0;
                while ((bufLine[i] != '\n') && (bufLine[i] != '\0') && 
						(bufLine[i] != '\r') && (bufLine[i] != '"') && (n < kMaxLineLen - 1))
                {
                          value[n++] = bufLine[i++];
                }
                value[n] = '\0';

                if (key[0] != '#' && key[0] != '\0' && value[0] != '\0')
                {
                    //printf("Adding config setting  <key=\"%s\", value=\"%s\">\n", key, value);
                    this->SetValue(key, value);
                }
                else
                {
                        //assert(false);
                }
            }
        }


        int closeErr = ::fclose(fileDesc);
        Assert(closeErr == 0);
    }
    
    return err;
}

void RelayPrefsSource::SetValue(const char* inKey, const char* inValue)
{
	RelayKeyValuePair* keyValue = NULL;
	
	// If the key/value already exists update the value.
	// If duplicate keys are allowed, however, add a new entry regardless
	if ((!fAllowDuplicates) && ((keyValue = this->FindValue(inKey, NULL)) != NULL))
	{
		keyValue->ResetValue(inValue);
	}
	else
	{
		fKeyValueList  = NEW RelayKeyValuePair(inKey, inValue, fKeyValueList);
		fNumKeys++;
	}
}

RelayKeyValuePair* RelayPrefsSource::FindValue(const char* inKey, char* ioValue)
{
	RelayKeyValuePair* keyValue = fKeyValueList;
        if ( ioValue != NULL)
            ioValue[0] = '\0';
	
	while (keyValue != NULL)
	{
		if (::strcmp(inKey, keyValue->fKey) == 0)
		{
			if (ioValue != NULL)
				::strcpy(ioValue, keyValue->fValue);
			return keyValue;
		}
		keyValue = keyValue->fNext;
	}
	
	return NULL;
}
