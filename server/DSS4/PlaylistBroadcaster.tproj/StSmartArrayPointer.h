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
#ifndef __StSmartArrayPointer__
#define __StSmartArrayPointer__

template <class T>
class StSmartArrayPointer
{
	public:
		StSmartArrayPointer(T* victim) : fT(victim)  {}
		~StSmartArrayPointer() { delete [] fT; }
		
		void SetObject(T* victim) 
		{ 
			//can't use a normal assert here because "Assert.h" eventually includes this file....
			#ifdef ASSERT
				//char s[65]; 
				if (fT != NULL) printf ("_Assert: StSmartArrayPointer::SetObject() %s, %d\n", __FILE__, __LINE__); 
			#endif 

			fT = victim; 
		}
		
		T* GetObject() { return fT; }
		
		operator T*() { return fT; }
	
	private:
	
		T* fT;
};

typedef StSmartArrayPointer<char> OSCharArrayDeleter;
#endif
