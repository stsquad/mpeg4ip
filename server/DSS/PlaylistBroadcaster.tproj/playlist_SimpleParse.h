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

#ifndef SimpleParser_H
#define SimpleParser_H


#include "OSHeaders.h"


class SimpleString {
	
	public:			
			SInt32 fLen;
			char *fTheString;
			
			SimpleString(char *theString = NULL);
			void Init();
			void SetString(char *theString, SInt32 len);
		 	SInt32 GetString(char *theString, SInt32 len);
};

class SimpleParser {


	
	protected:
			SimpleString fSource;
	public:
	
			 SimpleParser() : fSource(NULL) {} ;
			 SimpleParser(SimpleString *source) : fSource(*source) {} ;
		~SimpleParser() {};
		
	static char sWordDelimeters[];
	static char sLineDelimeters[];

	void SetSource(SimpleString *source) {fSource = *source;};	
		
	static int CountDelimeters( SimpleString *source, char *delimeters);
	
	static bool Compare(SimpleString *str1, SimpleString*str2);		
	
	static bool FindString( SimpleString *source,  SimpleString *find, SimpleString *resultString);
	
	static bool FindNextString( SimpleString *sourcePtr,  SimpleString *currentPtr,  SimpleString *findPtr, SimpleString *resultStringPtr);

	static bool GetString( SimpleString *source,  SimpleString *find, SimpleString *resultString);
					
	static bool FindDelimeter( SimpleString *source, char *delimeter, SimpleString *resultString);
	
	static bool GetLine( SimpleString *sourcePtr, SimpleString *resultStringPtr);				
	
	static bool GetWord( SimpleString *sourcePtr, SimpleString *resultStringPtr);
	
	static bool GetNextThing( SimpleString *sourcePtr, SimpleString *currentPtr, char *findChars, SimpleString *resultStringPtr);

	static bool GetNextLine( SimpleString *sourcePtr, SimpleString *currentLine, SimpleString *resultStringPtr);

	static bool GetNextWord( SimpleString *sourcePtr, SimpleString *currentWord, SimpleString *resultStringPtr);
};



#endif
