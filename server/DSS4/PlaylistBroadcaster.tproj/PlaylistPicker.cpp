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
#include <stdio.h>
#include "MyAssert.h"
#include "OS.h"
	

#include "PlaylistPicker.h"

/*
	PlaylistPicker has 3 modes
		- sequential looping through the items in the play list(s)
		in the order they are entered.
		
		- above w/ looping
		
		- weighted picking "randomly" from weighted buckets
		

*/




PlaylistPicker::PlaylistPicker( UInt32 numBuckets, UInt32 numNoRepeats )
{
	// weighted random ctor
	
	mFirstElement = NULL;
	mNumToPickFrom = 0;
	mBuckets = numBuckets;
	mIsSequentialPicker = false;
	mRecentMoviesListSize = numNoRepeats;

/* changed by emil@popwire.com (see relaod.txt for info) */
	mRemoveFlag = false;
	mStopFlag = false;
/* ***************************************************** */
	mLastResult = (UInt32) OS::Milliseconds();

	mPickCounts = new long[numBuckets];
	UInt32 x;
	
	for ( x = 0; x < mBuckets; x++ )
	{	mPickCounts[x] = 0;
		mElementLists[x] = new PLDoubleLinkedList<SimplePlayListElement>;
		Assert( mElementLists[x] );
		
	}
	
	

	mUsedElements = new NoRepeat( numNoRepeats );

	Assert( mUsedElements );
}

PlaylistPicker::PlaylistPicker(bool doLoop)
{
	// sequential ctor
	
	mFirstElement = NULL;
	mIsSequentialLooping = doLoop;
	
	mIsSequentialPicker = true;
	mWhichSequentialBucket = 0;
	mRecentMoviesListSize = 0;
	
	mNumToPickFrom = 0;
	mBuckets = 2;	// alternating used/remaining pick buckets
/* changed by emil@popwire.com (see relaod.txt for info) */
	mRemoveFlag = false;
	mStopFlag = false;
	fLastPick = NULL;
/* ***************************************************** */
	
	
	mPickCounts = new long[mBuckets];
	
	
	UInt32 	bucketIndex;
	
	for ( bucketIndex = 0; bucketIndex < mBuckets; bucketIndex++ )
	{	
		mPickCounts[bucketIndex] = 0;
		mElementLists[bucketIndex] = new PLDoubleLinkedList<SimplePlayListElement>;
		Assert( mElementLists[bucketIndex] );
		
	}


	mUsedElements = NULL;

}

PlaylistPicker::~PlaylistPicker()
{
	UInt32		bucketIndex;
	
	delete mUsedElements;
	
	for ( bucketIndex = 0; bucketIndex < mBuckets; bucketIndex++ )
	{	
		delete mElementLists[bucketIndex] ;
		
	}
	
	delete [] mPickCounts;
}


UInt32 PlaylistPicker::Random()
{
	UInt32 seed = 1664525L * mLastResult + 1013904223L; //1013904223 is prime .. Knuth D.E.	
	::srand( seed );
	
	UInt32 result = ::rand();

	mLastResult = result;
	return result;
}


char* PlaylistPicker::PickOne()
{
	
	char*	foundName = NULL;	// pointer to name of pick we find, caller deletes.
	
	
	if ( mIsSequentialPicker )
	{	
		if ( mElementLists[mWhichSequentialBucket]->GetNumNodes() == 0 && mIsSequentialLooping )
		{	// ran out of items switch to other list.
			if ( mWhichSequentialBucket == 0 )
				mWhichSequentialBucket = 1;
			else
				mWhichSequentialBucket = 0;
	
		}
			
		if ( mElementLists[mWhichSequentialBucket]->GetNumNodes() > 0 )
		{
			PLDoubleLinkedListNode<SimplePlayListElement>*	node;
			
			
			node = mElementLists[mWhichSequentialBucket]->GetFirst();
			
			Assert( node );
			
			int	nameLen = ::strlen( node->fElement->mElementName );
			
			foundName = new char[ nameLen +1 ];
			
			Assert( foundName );
			
			if ( foundName )
			{	
				int	usedBucketIndex;
				
				::strcpy( foundName, node->fElement->mElementName );
			
				// take him out of the bucket since he's now in play
				mElementLists[mWhichSequentialBucket]->RemoveNode( node );
				
				
				if ( mWhichSequentialBucket == 0 )
					usedBucketIndex = 1;
				else
					usedBucketIndex = 0;
					
/* changed by emil@popwire.com (see relaod.txt for info) */
				if(!mRemoveFlag)
/* ***************************************************** */
					mElementLists[usedBucketIndex]->AddNodeToTail( node );
/* changed by emil@popwire.com (see relaod.txt for info) */
				else 
					mNumToPickFrom--;
/* ***************************************************** */
			
			}
		
		}


	}
	else
	{
		SInt32		bucketIndex;
		UInt32		minimumBucket = 0;
		UInt32		avaiableToPick;
		UInt32		theOneToPick;
		SInt32		topBucket;
		
		
		// find the highest bucket with some elements.
		bucketIndex = this->GetNumBuckets() - 1;
		
		while ( bucketIndex >= 0 &&  mElementLists[bucketIndex]->GetNumNodes() == 0 )
		{
			bucketIndex--;
		}
		
		
		// adjust to 1 based  so we can use MOD
		topBucket = bucketIndex + 1;
		
		//printf( "topBucket %li \n", topBucket );
		 	
		if (topBucket > 0)
			minimumBucket = this->Random() % topBucket; // find our minimum bucket

		//printf( "minimumBucket %li \n", minimumBucket );
		
		// pick randomly from the movies in this and higher buckets
		// sum the available elements, then pick randomly from them.
		
		avaiableToPick = 0;
		
		bucketIndex = minimumBucket;
		
		while ( bucketIndex < topBucket )
		{
			avaiableToPick += mElementLists[bucketIndex]->GetNumNodes();
				
			bucketIndex++;
		}
		
		//printf( "avaiableToPick %li \n", avaiableToPick );
		
		// was anyone left??
		
		if ( avaiableToPick )
		{
			theOneToPick = this->Random() % avaiableToPick;		
			//printf( "theOneToPick %li \n", theOneToPick );

			// now walk through the lists unitl we get to the list
			// that contains our pick, then pick that one.
			bucketIndex = minimumBucket;
			
			while ( bucketIndex < topBucket && foundName == NULL )
			{
			
				//printf( "theOneToPick %li, index %li numelements %li\n", theOneToPick , bucketIndex, mElementLists[bucketIndex]->GetNumNodes());
				
				if ( theOneToPick >= mElementLists[bucketIndex]->GetNumNodes() )
					theOneToPick -= mElementLists[bucketIndex]->GetNumNodes();
				else
				{	//printf( "will pick theOneToPick %li, index %li \n", theOneToPick, bucketIndex);
					foundName = this->PickFromList( mElementLists[bucketIndex], theOneToPick );
					if ( foundName )
						mPickCounts[bucketIndex]++;
				}

				bucketIndex++;
			}
		
			// we messed up if we don't have a name at this point.
			Assert( foundName );
		}
	}
	
	fLastPick = foundName;
	return foundName;

}

void PlaylistPicker::CleanList()
{
	bool temp = mIsSequentialLooping; 
	char *thePick = NULL;
	bool tempRemove = mRemoveFlag;
	mRemoveFlag = true;
	mIsSequentialLooping = false; 
	while( (thePick = this->PickOne()) != NULL )
		delete thePick; 
	mIsSequentialLooping = temp;
	mRemoveFlag = tempRemove;
};

char*	PlaylistPicker::PickFromList( PLDoubleLinkedList<SimplePlayListElement>* list, UInt32 elementIndex )
{
	PLDoubleLinkedListNode<SimplePlayListElement>*	plNode;
	char*		foundName = NULL;
	
	
	plNode = list->GetNthNode( elementIndex );

	if ( plNode )
	{
		int	nameLen = ::strlen(plNode->fElement->mElementName );
		
		foundName = new char[ nameLen +1 ];
		
		Assert( foundName );
		
		if ( foundName )
		{	
			::strcpy( foundName, plNode->fElement->mElementName );
		
			// take him out of the bucket since he's now in play
			list->RemoveNode( plNode );
			
			mNumToPickFrom--;
		
			// add him to our used list, and possibly
			// get an older one to put back into play
			PLDoubleLinkedListNode<SimplePlayListElement>* recycleNode = mUsedElements->AddToList( plNode );
			
			// if we got an old one to recycle, do so.
			if ( recycleNode )
				this->AddNode( recycleNode );
		}
	}

	return foundName;

}

bool PlaylistPicker::AddToList( const char* name, int weight )
{
	bool											addedSuccesfully;
	PLDoubleLinkedListNode<SimplePlayListElement>*	node;
	SimplePlayListElement*							element;
	

	node = NULL;
	addedSuccesfully = false;
	element = new SimplePlayListElement(name);
	if (mFirstElement == NULL)
		mFirstElement = element->mElementName;
	
	Assert( element );
	
	
	if ( element )
	{	element->mElementWeight = weight;
		node = new PLDoubleLinkedListNode<SimplePlayListElement>(element);

		Assert( node );
	}
	
	if ( node )
		addedSuccesfully = AddNode(node);
	
		
	return addedSuccesfully;
}

bool PlaylistPicker::AddNode(  PLDoubleLinkedListNode<SimplePlayListElement>* node )
{	
	bool	addSucceeded = false;

	
	Assert( node );
	Assert( node->fElement );
	
	
	if ( mIsSequentialPicker )	// make picks in sequential order, not weighted random
	{
		// add all to bucket 0
		mElementLists[0]->AddNodeToTail( node );
		
		addSucceeded = true;
		mNumToPickFrom++;
	
	}
	else
	{	
		int		weight;
		
		weight = node->fElement->mElementWeight;
	
		// weights are 1 based, correct to zero based for use as array myIndex
		weight--;
		
		Assert( weight >= 0 );
		
		Assert(  (UInt32)weight < mBuckets );
	
		if ( (UInt32)weight < mBuckets )
		{
			// the elements weighting defines the list it is in.
			mElementLists[weight]->AddNode( node );
			
			addSucceeded = true;
			mNumToPickFrom++;
		
		}
	}
	
	return addSucceeded;

}
