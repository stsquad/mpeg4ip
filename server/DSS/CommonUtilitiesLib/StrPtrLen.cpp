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
	File:		StrPtrLen.cpp

	Contains:	Implementation of class defined in StrPtrLen.h.  
					

	

*/


#include <ctype.h>
#include "StrPtrLen.h"
#include "MyAssert.h"
#include "OS.h"
#include "OSMemory.h"


UInt8		StrPtrLen::sCaseInsensitiveMask[] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, //0-9 
	10, 11, 12, 13, 14, 15, 16, 17, 18, 19, //10-19 
	20, 21, 22, 23, 24, 25, 26, 27, 28, 29, //20-29
	30, 31, 32, 33, 34, 35, 36, 37, 38, 39, //30-39 
	40, 41, 42, 43, 44, 45, 46, 47, 48, 49, //40-49
	50, 51, 52, 53, 54, 55, 56, 57, 58, 59, //50-59
	60, 61, 62, 63, 64, 97, 98, 99, 100, 101, //60-69 //stop on every character except a letter
	102, 103, 104, 105, 106, 107, 108, 109, 110, 111, //70-79
	112, 113, 114, 115, 116, 117, 118, 119, 120, 121, //80-89
	122, 91, 92, 93, 94, 95, 96, 97, 98, 99, //90-99
	100, 101, 102, 103, 104, 105, 106, 107, 108, 109, //100-109
	110, 111, 112, 113, 114, 115, 116, 117, 118, 119, //110-119
	120, 121, 122, 123, 124, 125, 126, 127, 128, 129 //120-129
};


char* StrPtrLen::GetAsCString() const
{
	// convert to a "NEW'd" zero terminated char array
	// caler is reponsible for the newly allocated memory
	char *theString = NEW char[Len+1];
	
	if ( Ptr && Len > 0 )
		::memcpy( theString, Ptr, Len );
	
	theString[Len] = 0;
	
	return theString;
}


Bool16 StrPtrLen::Equal(const StrPtrLen &compare) const
{
	if ((compare.Len == Len) && (memcmp(compare.Ptr, Ptr, Len) == 0))
		return true;
	else
		return false;
}

Bool16 StrPtrLen::Equal(const char* compare) const
{
	if ((::strlen(compare) == Len) && (memcmp(compare, Ptr, Len) == 0))
		return true;
	else
		return false;
}



Bool16 StrPtrLen::NumEqualIgnoreCase(const char* compare, const UInt32 len) const
{
	// compare thru the first "len: bytes
	Assert(compare != NULL);
	
	if (len <= Len)
	{
		for (UInt32 x = 0; x < len; x++)
			if (sCaseInsensitiveMask[Ptr[x]] != sCaseInsensitiveMask[compare[x]])
				return false;
		return true;
	}
	return false;
}

Bool16 StrPtrLen::EqualIgnoreCase(const char* compare, const UInt32 len) const
{
	Assert(compare != NULL);
	if (len == Len)
	{
		for (UInt32 x = 0; x < len; x++)
			if (sCaseInsensitiveMask[Ptr[x]] != sCaseInsensitiveMask[compare[x]])
				return false;
		return true;
	}
	return false;
}

#if STRPTRLENTESTING
Bool16	StrPtrLen::Test()
{
	static char* test1 = "2347.;.][';[;]abcdefghijklmnopqrstuvwxyz#%#$$#";
	static char* test2 = "2347.;.][';[;]ABCDEFGHIJKLMNOPQRSTUVWXYZ#%#$$#";
	static char* test3 = "Content-Type:";
	static char* test4 = "cONTent-TYPe:";
	static char* test5 = "cONTnnt-TYPe:";
	static char* test6 = "cONTent-TY";
	
	StrPtrLen theVictim1(test1, strlen(test1));
	if (!theVictim1.EqualIgnoreCase(test2, strlen(test2))
		return false;
		
	if (theVictim1.EqualIgnoreCase(test3, strlen(test3))
		return false;
	if (!theVictim1.EqualIgnoreCase(test1, strlen(test1))
		return false;

	StrPtrLen theVictim2(test3, strlen(test3));
	if (!theVictim2.EqualIgnoreCase(test4, strlen(test4))
		return false;
	if (theVictim2.EqualIgnoreCase(test5, strlen(test5))
		return false;
	if (theVictim2.EqualIgnoreCase(test6, strlen(test6))
		return false;
	return true;
}


#endif






