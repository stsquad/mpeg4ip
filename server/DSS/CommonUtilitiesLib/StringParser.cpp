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
	File:		StringParser.cpp

	Contains:	Implementation of StringParser class.  
					
	
	
*/

#include "StringParser.h"

UInt8 StringParser::sNonWordMask[] =
{
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //0-9 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //10-19 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //20-29
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //30-39 
	1, 1, 1, 1, 1, 0, 1, 1, 1, 1, //40-49 - is a word
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //50-59
	1, 1, 1, 1, 1, 0, 0, 0, 0, 0, //60-69 //stop on every character except a letter
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //70-79
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //80-89
	0, 1, 1, 1, 1, 0, 1, 0, 0, 0, //90-99 _ is a word
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //100-109
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //110-119
	0, 0, 0, 1, 1, 1, 1, 1, 1, 1, //120-129
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //130-139
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //140-149
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //150-159
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //160-169
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //170-179
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //180-189
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //190-199
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //200-209
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //210-219
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //220-229
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //230-239
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //240-249
	1, 1, 1, 1, 1, 1 			 //250-255
};

UInt8 StringParser::sWordMask[] =
{
	// Inverse of the above
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0-9 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //10-19 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //20-29
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //30-39 
	0, 0, 0, 0, 0, 1, 0, 0, 0, 0, //40-49 - is a word
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //50-59
	0, 0, 0, 0, 0, 1, 1, 1, 1, 1, //60-69 //stop on every character except a letter
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //70-79
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //80-89
	1, 0, 0, 0, 0, 1, 0, 1, 1, 1, //90-99 _ is a word
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //100-109
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //110-119
	1, 1, 1, 0, 0, 0, 0, 0, 0, 0, //120-129
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

UInt8 StringParser::sDigitMask[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0-9
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //10-19 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //20-29
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //30-39
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, //40-49 //stop on every character except a number
	1, 1, 1, 1, 1, 1, 1, 1, 0, 0, //50-59
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
	0, 0, 0, 0, 0, 0			 //250-255
};

UInt8 StringParser::sEOLMask[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0-9   
	1, 0, 0, 1, 0, 0, 0, 0, 0, 0, //10-19    //'\r' & '\n' are stop conditions
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //20-29
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //30-39 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //40-49
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //50-59
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

UInt8 StringParser::sWhitespaceMask[] =
{
	1, 1, 1, 1, 1, 1, 1, 1, 1, 0, //0-9      // stop on '\t'
	0, 0, 0, 0, 1, 1, 1, 1, 1, 1, //10-19  	 // '\r', \v', '\f' & '\n'
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //20-29
	1, 1, 0, 1, 1, 1, 1, 1, 1, 1, //30-39   //  ' '
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //40-49
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //50-59
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //60-69
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //70-79
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //80-89
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //90-99
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //100-109
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //110-119
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //120-129
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //130-139
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //140-149
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //150-159
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //160-169
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //170-179
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //180-189
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //190-199
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //200-209
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //210-219
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //220-229
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //230-239
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //240-249
	1, 1, 1, 1, 1, 1 			 //250-255
};

UInt8 StringParser::sEOLWhitespaceMask[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1, //0-9   	// \t is a stop
	1, 1, 1, 1, 0, 0, 0, 0, 0, 0, //10-19    //'\r' & '\n' are stop conditions
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //20-29
	0, 0, 1, 0, 0, 0, 0, 0, 0, 0, //30-39 	' '  is a stop
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //40-49
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //50-59
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



void StringParser::ConsumeUntil(StrPtrLen* outString, char inStop)
{
	Assert(fStartGet != NULL);
	Assert(fEndGet != NULL);
	Assert(fStartGet <= fEndGet);

	char *originalStartGet = fStartGet;

	while ((fStartGet < fEndGet) && (*fStartGet != inStop))
		fStartGet++;
		
	if (outString != NULL)
	{
		outString->Ptr = originalStartGet;
		outString->Len = fStartGet - originalStartGet;
	}
}

void StringParser::ConsumeUntil(StrPtrLen* outString, UInt8 *inMask)
{
	Assert(fStartGet != NULL);
	Assert(fEndGet != NULL);
	Assert(fStartGet <= fEndGet);

	char *originalStartGet = fStartGet;

	while ((fStartGet < fEndGet) && (!inMask[*fStartGet]))
		fStartGet++;

	if (outString != NULL)
	{
		outString->Ptr = originalStartGet;
		outString->Len = fStartGet - originalStartGet;
	}
}

void StringParser::ConsumeLength(StrPtrLen* spl, SInt32 inLength)
{
	Assert(fStartGet != NULL);
	Assert(fEndGet != NULL);
	Assert(fStartGet <= fEndGet);

	//sanity check to make sure we aren't being told to run off the end of the
	//buffer
	if ((fEndGet - fStartGet) < inLength)
		inLength = fEndGet - fStartGet;
	
	if (spl != NULL)
	{
		spl->Ptr = fStartGet;
		spl->Len = inLength;
	}
	fStartGet += inLength;
}


UInt32 StringParser::ConsumeInteger(StrPtrLen* outString)
{
	Assert(fStartGet != NULL);
	Assert(fEndGet != NULL);
	Assert(fStartGet <= fEndGet);

	UInt32 theValue = 0;
	char *originalStartGet = fStartGet;
	
	while ((fStartGet < fEndGet) && (*fStartGet >= '0') && (*fStartGet <= '9'))
	{
		theValue = (theValue * 10) + (*fStartGet - '0');
		fStartGet++;
	}

	if (outString != NULL)
	{
		outString->Ptr = originalStartGet;
		outString->Len = fStartGet - originalStartGet;
	}
	return theValue;
}

Float32 StringParser::ConsumeFloat()
{
	Float32 theFloat = 0;
	while ((fStartGet < fEndGet) && (*fStartGet >= '0') && (*fStartGet <= '9'))
	{
		theFloat = (theFloat * 10) + (*fStartGet - '0');
		fStartGet++;
	}
	if ((fStartGet < fEndGet) && (*fStartGet == '.'))
		fStartGet++;
	Float32 multiplier = .1;
	while ((fStartGet < fEndGet) && (*fStartGet >= '0') && (*fStartGet <= '9'))
	{
		theFloat += (multiplier * (*fStartGet - '0'));
		multiplier *= .1;
		fStartGet++;
	}
	return theFloat;
}


Bool16	StringParser::Expect(char stopChar)
{
	if (fStartGet >= fEndGet)
		return false;
	if(*fStartGet != stopChar)
		return false;
	else
	{
		fStartGet++;
		return true;
	}
}
Bool16 StringParser::ExpectEOL()
{
	//This function processes all legal forms of HTTP / RTSP eols.
	//They are: \r (alone), \n (alone), \r\n
	Bool16 retVal = false;
	if ((fStartGet < fEndGet) && ((*fStartGet == '\r') || (*fStartGet == '\n')))
	{
		retVal = true;
		fStartGet++;
		//check for a \r\n, which is the most common EOL sequence.
		if ((fStartGet < fEndGet) && ((*(fStartGet - 1) == '\r') && (*fStartGet == '\n')))
			fStartGet++;
	}
	return retVal;
}

void StringParser::UnQuote(StrPtrLen* outString)
{
	// If a string is contained within double or single quotes 
	// then UnQuote() will remove them. - [sfu]
	
	// sanity check
	if (outString->Ptr == NULL || outString->Len < 2)
		return;
		
	// remove begining quote if it's there.
	if (outString->Ptr[0] == '"' || outString->Ptr[0] == '\'')
	{
		outString->Ptr++; outString->Len--;
	}
	// remove ending quote if it's there.
	if ( outString->Ptr[outString->Len-1] == '"' || 
	     outString->Ptr[outString->Len-1] == '\'' )
	{
		outString->Len--;
	}
}

#if STRINGPARSERTESTING
Bool16 StringParser::Test()
{
	static char* string1 = "RTSP 200 OK\r\nContent-Type: MeowMix\r\n\t   \n3450";
	
	StrPtrLen theString(string1, strlen(string1));
	
	StringParser victim(&theString);
	
	StrPtrLen rtsp;
	SInt32 theInt = victim.ConsumeInteger();
	if (theInt != 0)
		return false;
	victim.ConsumeWord(&rtsp);
	if ((rtsp.len != 4) && (strncmp(rtsp.Ptr, "RTSP", 4) != 0))
		return false;
		
	victim.ConsumeWhiteSpace();
	theInt = victim.ConsumeInteger();
	if (theInt != 200)
		return false;
		
	return true;
}
#endif
