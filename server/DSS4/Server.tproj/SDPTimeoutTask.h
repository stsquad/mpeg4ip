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
	File:		SDPTimeoutTask.h

	Contains:	A task object that examines SDP files for timed out
				session availability and deltes those files

*/

#ifndef __SDP_TIMEOUT_TASK_H__
#define __SDP_TIMEOUT_TASK_H__

#include "Task.h"

class SDPTimeoutTask : public Task
{	enum {kSearchFiles, kCheckingFiles};
	public:
		SDPTimeoutTask() : Task() { this->Signal(Task::kStartEvent); }
		virtual ~SDPTimeoutTask() {}
		SInt16 FindSDPFiles(char *thePath, FILE *outputFilePtr, SInt16 depth);

	
	private:
		virtual SInt64 Run();
		
		UInt32 fCurrentListFile;
		static char 		*sTempFile;
		static int 			sState;
		static UInt32 		sInterval;
		static StrPtrLen 	sFileList;
		static Bool16 		sFirstRun;


};

#endif //__SDP_TIMEOUT_TASK_H__
