#ifndef __no_repeat__
#define __no_repeat__
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



#include "PLDoubleLinkedList.h"
#include "SimplePlayListElement.h"

#include "OSHeaders.h"


class NoRepeat : public PLDoubleLinkedList<SimplePlayListElement> {

	public: 
		NoRepeat( UInt32 numNoRepeats );
		virtual ~NoRepeat();
		
		bool IsInList( const char* name ); // return true if name is in list, false if not
		bool AddToList( const char* name );// return true if could be added to list, no dupes allowd
		PLDoubleLinkedListNode<SimplePlayListElement>* AddToList( PLDoubleLinkedListNode<SimplePlayListElement>* node );

	protected:
		static bool		CompareNameToElement( PLDoubleLinkedListNode<SimplePlayListElement>*node, void *name );
		UInt32			mMaxElements;

};

#endif
