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

#ifndef playlist_array_H
#define playlist_array_H

// ************************
//
// TEMPLATE ARRAYLIST 
//
// ************************

template <class T> class ArrayList
{

	public:
		T *fDataArray;
		short	fSize;
		short	fCurrent;

		ArrayList()
		{	fDataArray = NULL;
			fSize = 0;
			fCurrent = 0;
		}
		
		~ArrayList() 
		{	delete [] fDataArray;
		};

		 T *SetSize(short size) 
		 { 	if (fDataArray != NULL) delete [] fDataArray; 
		 	fDataArray = new T[size]; 
		 	if (fDataArray) 
		 	{ 	fSize = size; 
		 	}
		 	
		 	return fDataArray;
		 }

		 T *Get()
		{
			T *resultPtr = NULL;	
			if (fDataArray != NULL)
			{
				if ( (fCurrent >= 0) && (fCurrent < fSize )  )
				{	resultPtr = &fDataArray[fCurrent];
				}
			}
			return  resultPtr;
		}

		 T *OffsetPos(short offset)
		{
			T *resultPtr = NULL;	
			
			do
			{
				if (fDataArray == NULL) break;
				short temp = fCurrent + offset;

				if (temp < 0)
				{
					fCurrent = 0; // peg at begin and return NULL
					break;
				}
				
				if (temp < fSize) 
				{		
					resultPtr = &fDataArray[temp];
					fCurrent = temp;
					break;
				}	
					
				fCurrent = fSize -1; // peg at end and return NULL
			} while (false);
			
			return  resultPtr;
		}

		 T *SetPos(short current)
		{
			T *resultPtr = NULL;	

			do
			{
				if (fDataArray == NULL) break;
				
				if (current < 0)
				{
					fCurrent = 0; // peg at begin and return NULL
					break;
				}
				
				if (current < fSize) 
				{
					resultPtr = &fDataArray[current];
					fCurrent = current;
					break;
				}
					
				fCurrent = fSize -1; // peg at end and return NULL
				
			} while (false);

			return  resultPtr;
		}

		 short GetPos() { return fCurrent; };
	 	 T *Next()  	{ return OffsetPos(1); };
		 T *Begin() 	{ return SetPos(0); };
		 short Size() 	{ return fSize;};
};


#endif

