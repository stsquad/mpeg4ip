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
	File:		QTSSAccessLogModule.cpp

	Contains:	Implementation of object defined in QTSSAccessLogModule.h
					
	
	

*/

#include "StringParser.h"
#include "StringFormatter.h"
#include "StrPtrLen.h"
#include "UserAgentParser.h"



UserAgentParser::UserAgentFields UserAgentParser::sFieldIDs[] = 
{   /* fAttrName, len, id */
	{ "qtid", 	strlen("qtid"),		eQtid	},
	{ "qtver",	strlen("qtver"),	eQtver	},
	{ "lang",	strlen("lang"),		eLang	},
	{ "os",		strlen("os"),		eOs		},
	{ "osver",	strlen("osver"),	eOsver	},
	{ "cpu",	strlen("cpu"),		eCpu	}
};

	
UInt8 UserAgentParser::sEOLWhitespaceEqualMask[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1, //0-9   	// \t is a stop
	1, 0, 0, 1, 0, 0, 0, 0, 0, 0, //10-19    //'\r' & '\n' are stop conditions
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //20-29
	0, 0, 1, 0, 0, 0, 0, 0, 0, 0, //30-39 	' '  is a stop
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //40-49
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //50-59
	0, 1, 0, 0, 0, 0, 0, 0, 0, 0, //60-69   '=' is a stop
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

UInt8 UserAgentParser::sEOLSemicolonCloseParenMask[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1, //0-9   	// \t is a stop
	1, 0, 0, 1, 0, 0, 0, 0, 0, 0, //10-19    //'\r' & '\n' are stop conditions
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //20-29
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //30-39 	
	0, 1, 0, 0, 0, 0, 0, 0, 0, 0, //40-49   ')'  is a stop
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1, //50-59	';' is a stop
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //60-69  
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


void UserAgentParser::Parse(StrPtrLen *inStream)
{
	StrPtrLen tempID;
	StrPtrLen tempData;
	StringParser parser(inStream);
	StrPtrLen startFields;
		

	memset(&fFieldData,0,sizeof(fFieldData) );
	
	parser.ConsumeUntil(&startFields, '(' ); // search for '(' if not found does nothing
	while (startFields.Len != 0)
	{
		//stop when we reach an empty line.
		tempID.Set(NULL,0);
		tempData.Set(NULL,0);
		
		parser.ConsumeLength(NULL, 1); // step past '(' or ';' if not found or at end of line does nothing
		parser.ConsumeWhitespace(); // search for non-white space if not found does nothing
		parser.ConsumeUntil(&tempID, sEOLWhitespaceEqualMask ); // search for end of id (whitespace or =)if not found does nothing
		if (tempID.Len == 0) break;
	
		parser.ConsumeUntil(NULL, '=' ); // find the '='
		parser.ConsumeLength(NULL, 1); // step past if not found or at end of line does nothing
		parser.ConsumeUntil(&tempData, sEOLSemicolonCloseParenMask ); // search for end of data if not found does nothing
		if (tempData.Len == 0) break;
		
		StrPtrLen 	testID;
		UInt32 		fieldID;
		for (short testField = 0; testField < UserAgentParser::eNumAttributes; testField++)
		{
			testID.Set(sFieldIDs[testField].fFieldName,sFieldIDs[testField].fLen);
			fieldID = sFieldIDs[testField].fID;
			if ( (fFieldData[fieldID].fFound == false) && testID.Equal(tempID) ) 
			{ 	
				fFieldData[fieldID].fData = tempData;
				fFieldData[fieldID].fFound = true;				
			}
		}
		
	}
}
