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


#include <stdlib.h>
#include <string.h>


#include "NoRepeat.h"



NoRepeat::NoRepeat ( UInt32 numNoRepeats )
	: PLDoubleLinkedList<SimplePlayListElement>()
{
	mMaxElements = numNoRepeats;
}

NoRepeat::~NoRepeat()
{
	// we have nothing to do, just let the PLDoubleLinkedList clear itself
}

bool NoRepeat::CompareNameToElement( PLDoubleLinkedListNode<SimplePlayListElement>* node, void* name )
{
	if ( !::strcmp( node->fElement->mElementName, (const char*)name ) )
		return true;
	
	return false;
}


bool NoRepeat::IsInList( const char* name )
{
	PLDoubleLinkedListNode<SimplePlayListElement>* whichElement;
	
	whichElement = ForEachUntil( CompareNameToElement, (void*)name );
	
	if ( whichElement )
		return true;
	
	return false;
	
}


PLDoubleLinkedListNode<SimplePlayListElement>* NoRepeat::AddToList( PLDoubleLinkedListNode<SimplePlayListElement>* node )
{
	AddNode(node);
	PLDoubleLinkedListNode<SimplePlayListElement>* oldTail = NULL;
	
	if ( fNumNodes > mMaxElements )
	{	oldTail = fTail;
		this->RemoveNode( fTail );
	}
	
	return oldTail;
}

bool NoRepeat::AddToList( const char* name )
{
	Assert( false );
	
	bool addedSuccesfully = false;

	if ( !this->IsInList( name ) )
	{
		SimplePlayListElement* 							element;
		PLDoubleLinkedListNode<SimplePlayListElement>*	node = NULL;

		element = new SimplePlayListElement(name);
		
		Assert( element );
		
		if ( element )
			node = new PLDoubleLinkedListNode<SimplePlayListElement>(element);
	
		Assert( node );
		
		if ( node )
		{	
			PLDoubleLinkedListNode<SimplePlayListElement>* deadNode;
			
			deadNode = this->AddToList(node);
			
			delete deadNode;
			addedSuccesfully = true;
		
		}
		
	}
	
	return addedSuccesfully;
}

