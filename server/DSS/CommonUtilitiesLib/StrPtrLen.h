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
	File:		StrPtrLen.h

	Contains:	Definition of class that tracks a string ptr and a length.
				Note: this is NOT a string container class! It is a string PTR container
				class. It therefore does not copy the string and store it internally. If
				you deallocate the string to which this object points to, and continue 
				to use it, you will be in deep doo-doo.
				
				It is also non-encapsulating, basically a struct with some simple methods.
					

*/

#ifndef __STRPTRLEN_H__
#define __STRPTRLEN_H__

#include <string.h>
#include "OSHeaders.h"

#define STRPTRLENTESTING 0

class StrPtrLen
{
	public:

		//CONSTRUCTORS/DESTRUCTOR
		//These are so tiny they can all be inlined
		StrPtrLen() : Ptr(NULL), Len(0) {}
		StrPtrLen(char* sp) : Ptr(sp), Len(sp != NULL ? strlen(sp) : 0) {}
		StrPtrLen(char *sp, UInt32 len) : Ptr(sp), Len(len) {}
		~StrPtrLen() {}
		
		//OPERATORS:
		Bool16 Equal(const StrPtrLen &compare) const;
		Bool16 EqualIgnoreCase(const char* compare, const UInt32 len) const;
		Bool16 Equal(const char* compare) const;
		Bool16 NumEqualIgnoreCase(const char* compare, const UInt32 len) const;
		
		StrPtrLen& operator=(const StrPtrLen& newStr) { Ptr = newStr.Ptr; Len = newStr.Len;
														return *this; }
                char operator[](int i) { /*Assert(i<Len);i*/ return Ptr[i]; }
		void Set(char* inPtr, UInt32 inLen) { Ptr = inPtr; Len = inLen; }

		//This is a non-encapsulating interface. The class allows you to access its
		//data.
		char* 		Ptr;
		UInt32		Len;

		// convert to a "NEW'd" zero terminated char array
		char* GetAsCString() const;
		
#if STRPTRLENTESTING
		static Bool16	Test();
#endif

	private:

		static UInt8 	sCaseInsensitiveMask[];
};
#endif // __STRPTRLEN_H__

