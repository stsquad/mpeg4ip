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
#include <ctype.h> 
#include "MyAssert.h"

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
		Bool16 EqualIgnoreCase(const StrPtrLen &compare) const { return EqualIgnoreCase(compare.Ptr, compare.Len); }
		Bool16 Equal(const char* compare) const;
		Bool16 NumEqualIgnoreCase(const char* compare, const UInt32 len) const;
		
		void Delete() { delete [] Ptr; Ptr = NULL; Len = 0; }
		char *ToUpper() { for (UInt32 x = 0; x < Len ; x++) Ptr[x] = ::toupper (Ptr[x]); return Ptr;}
		
		char *FindStringCase(char *queryCharStr, StrPtrLen *resultStr, Bool16 caseSensitive) const;

		char *FindString(StrPtrLen *queryStr, StrPtrLen *outResultStr)				{ 	Assert(queryStr != NULL);	Assert(queryStr->Ptr != NULL); Assert(0 == queryStr->Ptr[queryStr->Len]);
																						return FindStringCase(queryStr->Ptr, outResultStr,true); 	
																					}
		
		char *FindStringIgnoreCase(StrPtrLen *queryStr, StrPtrLen *outResultStr)	{	Assert(queryStr != NULL);	Assert(queryStr->Ptr != NULL); Assert(0 == queryStr->Ptr[queryStr->Len]); 
																						return FindStringCase(queryStr->Ptr, outResultStr,false); 
																					}

		char *FindString(StrPtrLen *queryStr) 										{	Assert(queryStr != NULL);	Assert(queryStr->Ptr != NULL); Assert(0 == queryStr->Ptr[queryStr->Len]); 
																						return FindStringCase(queryStr->Ptr, NULL,true); 	
																					}
		
		char *FindStringIgnoreCase(StrPtrLen *queryStr) 							{ 	Assert(queryStr != NULL);	Assert(queryStr->Ptr != NULL); Assert(0 == queryStr->Ptr[queryStr->Len]); 
																						return FindStringCase(queryStr->Ptr, NULL,false); 
																					}
																					
		char *FindString(char *queryCharStr) 										{ return FindStringCase(queryCharStr, NULL,true); 	}
		char *FindStringIgnoreCase(char *queryCharStr) 								{ return FindStringCase(queryCharStr, NULL,false); 	}
		char *FindString(char *queryCharStr, StrPtrLen *outResultStr)				{ return FindStringCase(queryCharStr, outResultStr,true); 	}
		char *FindStringIgnoreCase(char *queryCharStr, StrPtrLen *outResultStr)		{ return FindStringCase(queryCharStr, outResultStr,false); 	}

		char *FindString(StrPtrLen &query, StrPtrLen *outResultStr)					{ return FindString( &query, outResultStr);				}
		char *FindStringIgnoreCase(StrPtrLen &query, StrPtrLen *outResultStr)		{ return FindStringIgnoreCase( &query, outResultStr); 	}
		char *FindString(StrPtrLen &query)											{ return FindString( &query); 			}
		char *FindStringIgnoreCase(StrPtrLen &query) 								{ return FindStringIgnoreCase( &query); }
		
		StrPtrLen& operator=(const StrPtrLen& newStr) { Ptr = newStr.Ptr; Len = newStr.Len;
														return *this; }
         char operator[](int i) { /*Assert(i<Len);i*/ return Ptr[i]; }
		void Set(char* inPtr, UInt32 inLen) { Ptr = inPtr; Len = inLen; }
		void Set(char* inPtr) { Ptr = inPtr; Len = (inPtr) ?  ::strlen(inPtr) : 0; }

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

// BE VERY VERY SURE THIS IS SAFE TO USE BEFORE YOU DO.
// It puts a 0 at the end of the buffer and puts it back when it is destroyed.
// - make sure you are referencing a ptr to a buffer that contains this buffer segment
// - make sure you are thread safe
// - make sure the StrPtrLen you are passing is not a static char array
// - make sure you don't change the string

class StrPtrLenTerminator
{
	public:
		StrPtrLenTerminator(StrPtrLen* victim) : fLastChar(0), fStrPtr(victim)  
		{	if (victim && victim->Ptr) 
			{	fLastChar = victim->Ptr[victim->Len]; 
				victim->Ptr[victim->Len] = 0;
			}
		}
		~StrPtrLenTerminator() 
		{ 	if (fStrPtr && fStrPtr->Ptr) 
				fStrPtr->Ptr[fStrPtr->Len] = fLastChar;
		}
		
		void SetObject(StrPtrLen* victim) 
		{
			Assert(fStrPtr == NULL);
			fStrPtr = victim; 
			if (victim && victim->Ptr) 
			{	fLastChar = victim->Ptr[victim->Len]; 
				victim->Ptr[victim->Len] = 0;
			}
		}
		StrPtrLen* GetObject() { return fStrPtr; }
	
	private:
		char	   fLastChar;
		StrPtrLen* fStrPtr;
};

#endif // __STRPTRLEN_H__
