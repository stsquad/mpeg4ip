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
	File:		RTCPTask.h

	Contains:	A task object that processes all incoming RTCP packets
				for the server, and passes each one onto the task for
				which it belongs. 

*/

#ifndef __RTCP_TASK_H__
#define __RTCP_TASK_H__

#include "Task.h"

class RTCPTask : public Task
{
	public:
		//This task handles all incoming RTCP data. It just polls, so make sure
		//to start the polling process by signalling a start event.
		RTCPTask() : Task() { this->Signal(Task::kStartEvent); }
		virtual ~RTCPTask() {}
	
	private:
		virtual SInt64 Run();
};

#endif //__RTCP_TASK_H__
