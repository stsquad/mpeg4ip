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
	File:		QTSSSocket.cpp

	Contains:	A QTSS Stream object for a generic socket
	
	Written By: Denis Serenyi
			
	$Log: QTSSSocket.cpp,v $
	Revision 1.2  2001/05/09 21:04:37  cahighlander
	Sync to 0.6.2
	
	Revision 1.1.1.1  2001/02/26 23:27:31  dmackie
	Import of Apple Darwin Streaming Server 3 Public Preview
	
	Revision 1.1.1.1  2000/08/31 00:30:49  serenyi
	Mothra Repository
	
	Revision 1.1  2000/05/22 06:05:28  serenyi
	Added request body support, API additions, SETUP without DESCRIBE support, RTCP bye support
	
	
	
*/

#include "QTSSSocket.h"

QTSS_Error	QTSSSocket::RequestEvent(QTSS_EventType inEventMask)
{
	int theMask = 0;
	
	if (inEventMask & QTSS_ReadableEvent)
		theMask |= EV_RE;
	if (inEventMask & QTSS_WriteableEvent)
		theMask |= EV_WR;
		
	fEventContext.SetTask(this->GetTask());
	fEventContext.RequestEvent(theMask);
	return QTSS_NoErr;
}
