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
	File:		QTSSFile.h

	Contains:	 
					
	$Log: QTSSFile.h,v $
	Revision 1.3  2001/06/01 20:52:37  wmaycisco
	Sync to our latest code base.
	
	Revision 1.1.1.1  2001/02/26 23:27:31  dmackie
	Import of Apple Darwin Streaming Server 3 Public Preview
	
	Revision 1.1.1.1  2000/08/31 00:30:49  serenyi
	Mothra Repository
	
	Revision 1.2  2000/06/02 06:56:05  serenyi
	First checkin for module dictionaries
	
	Revision 1.1  2000/05/22 06:05:28  serenyi
	Added request body support, API additions, SETUP without DESCRIBE support, RTCP bye support
	
	
	
*/

#include "QTSSDictionary.h"
#include "QTSSModule.h"

#include "OSFileSource.h"
#include "EventContext.h"

class QTSSFile : public QTSSDictionary
{
	public:
	
		QTSSFile();
		virtual ~QTSSFile() {}
		
		static void		Initialize();
		
		//
		// Opening & Closing
		QTSS_Error			Open(char* inPath, QTSS_OpenFileFlags inFlags);
		void				Close();
		
		//
		// Implementation of stream functions.
		virtual QTSS_Error	Read(void* ioBuffer, UInt32 inLen, UInt32* outLen);
		
		virtual QTSS_Error	Seek(UInt64 inNewPosition);
		
		virtual QTSS_Error	Advise(UInt64 inPosition, UInt32 inAdviseSize);
		
		virtual QTSS_Error	RequestEvent(QTSS_EventType inEventMask);
		
	private:

		QTSSModule* fModule;
		UInt64		fPosition;
		QTSSFile*	fThisPtr;
		
		//
		// File attributes
		UInt64		fLength;
		time_t		fModDate;

		static QTSSAttrInfoDict::AttrInfo	sAttributes[];
};

