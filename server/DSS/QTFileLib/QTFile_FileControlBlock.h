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
// $Id: QTFile_FileControlBlock.h,v 1.2 2001/05/09 21:04:37 cahighlander Exp $
//
// QTFile_FileControlBlock:
//   All the per-client stuff for QTFile.


#ifndef _QTFILE_FILECONTROLBLOCK_H_
#define _QTFILE_FILECONTROLBLOCK_H_

//
// Includes
#include "OSHeaders.h"
#include "OSFileSource.h"

#if DSS_SERVER_INTERNAL
#include "QTSS.h"
#endif


//
// Class state cookie
class QTFile_FileControlBlock {

 public:
    //
    // Constructor and destructor.
    QTFile_FileControlBlock(void);
    virtual ~QTFile_FileControlBlock(void);
	
    //Sets this object to reference this file
    void Set(const char *inPath);
		
    //Advise: this advises the OS that we are going to be reading soon from the
    //following position in the file
    // void Advise(OSFileSource *dflt, UInt64 advisePos, UInt32 adviseAmt);
	
#if DSS_SERVER_INTERNAL
    Bool16 Read(QTSS_Object *dflt, UInt64 inPosition, void* inBuffer, UInt32 inLength);
#else
    Bool16 Read(OSFileSource *dflt, UInt64 inPosition, void* inBuffer, UInt32 inLength);
#endif
    //
    // Buffer management functions
    void AdjustDataBuffer(UInt32 BufferSize);
    
    // QTSS_ErrorCode Close();
	
    Bool16 IsValid()
    	{
#if DSS_SERVER_INTERNAL
			return fDataFD != NULL;
#else    	
    		return fDataFD.IsValid();
#endif
    	}

	
private:
    //
    // File descriptor for this control block
#if DSS_SERVER_INTERNAL
    QTSS_Object fDataFD;
#else
    OSFileSource fDataFD;
#endif

    enum
    {
		kDataBufferUnitSizeExp		= 15	// 32Kbytes
    };
    //
    // Data buffer cache
    char				*fDataBufferPool;

    UInt32				fDataBufferSize;
    UInt64				fDataBufferPosStart, fDataBufferPosEnd;

    char				*fCurrentDataBuffer, *fPreviousDataBuffer;
    UInt32				fCurrentDataBufferLength, fPreviousDataBufferLength;
};

#endif //_QTFILE_FILECONTROLBLOCK_H_
