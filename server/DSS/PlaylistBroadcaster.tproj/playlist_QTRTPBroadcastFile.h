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
// $Id: playlist_QTRTPBroadcastFile.h,v 1.1 2001/02/27 00:56:49 cahighlander Exp $
//
// QTRTPFile:
//   An interface to QTFile for TimeShare.

#ifndef QTRTPBroadcastFile_H
#define QTRTPBroadcastFile_H


//
// Includes
#include "OSHeaders.h"
#include "QTRTPFile.h"

#if __MACOS__
	#include "BogusDefs.h"
#else
#ifndef __Win32__
	#include <sys/stat.h>
#endif
#endif



class QTRTPBroadcastFile  : public QTRTPFile {


public:

	bool FindTrackSSRC( UInt32 SSRC);
	

};

#endif // QTRTPBroadcastFile
