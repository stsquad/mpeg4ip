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
	File:		main.cpp

	Contains:	main function to drive streaming server.

	

*/

#include <errno.h>

#include "RunServer.h"

#include "OS.h"
#include "OSMemory.h"
#include "OSThread.h"
#include "Socket.h"
#include "SocketUtils.h"
#include "ev.h"

#include "Task.h"
#include "IdleTask.h"
#include "TimeoutTask.h"

#include "QTSServerInterface.h"
#include "QTSServer.h"

QTSServer* sServer = NULL;
int sStatusUpdateInterval = 0;

QTSS_ServerState StartServer(XMLPrefsParser* inPrefsSource, PrefsSource* inMessagesSource, UInt16 inPortOverride, int statsUpdateInterval)
{
	//Mark when we are done starting up. If auto-restart is enabled, we want to make sure
	//to always exit with a status of 0 if we encountered a problem WHILE STARTING UP. This
	//will prevent infinite-auto-restart-loop type problems
	Bool16 doneStartingUp = false;
	
	sStatusUpdateInterval = statsUpdateInterval;
	
	//Initialize utility classes
	OS::Initialize();
	OSThread::Initialize();
	Socket::Initialize();
	SocketUtils::Initialize();

#if !MACOSXEVENTQUEUE
	::select_startevents();//initialize the select() implementation of the event queue
#endif
	
	//start the server
	QTSSDictionaryMap::Initialize();
	QTSServerInterface::Initialize();// this must be called before constructing the server object
	sServer = NEW QTSServer();
	sServer->Initialize(inPrefsSource, inMessagesSource, inPortOverride);

	//Make sure to do this stuff last. Because these are all the threads that
	//do work in the server, this ensures that no work can go on while the server
	//is in the process of staring up
	if (sServer->GetServerState() != qtssFatalErrorState)
	{
		IdleTask::Initialize();
		Socket::StartThread();
	
		//
		// On Win32, in order to call modwatch the Socket EventQueue thread must be
		// created first. Modules call modwatch from their initializer, and we don't
		// want to prevent them from doing that, so module initialization is separated
		// out from other initialization, and we start the Socket EventQueue thread first.
		// The server is still prevented from doing anything as of yet, because there
		// aren't any TaskThreads yet.
		sServer->InitModules();
	}
	
	if (sServer->GetServerState() != qtssFatalErrorState)
	{
		TaskThreadPool::AddThreads(OS::GetNumProcessors());
	#if DEBUG
		printf("Number of task threads: %lu\n",OS::GetNumProcessors());
	#endif
		// Start up the server's global tasks, and start listening
		sServer->StartTasks();
		TimeoutTask::Initialize(); 	// The TimeoutTask mechanism is task based,
									// we therefore must do this after adding task threads
	}

	QTSS_ServerState theServerState = sServer->GetServerState();
	if (theServerState != qtssFatalErrorState)
	{
		doneStartingUp = true;
		printf("Streaming Server done starting up\n");
		OSMemory::SetMemoryError(ENOMEM);
	}
	
	//
	// Tell the caller whether the server started up or not
	return theServerState;
}

void RunServer()
{	
	//just wait until someone stops the server or a fatal error occurs.
	QTSS_ServerState theServerState = sServer->GetServerState();
	while ((theServerState != qtssShuttingDownState) &&
			(theServerState != qtssFatalErrorState))
	{
		OSThread::Sleep(1000);
		
		if (sStatusUpdateInterval)
		{
			static int loopCount = 0;
	
			if ( (loopCount % sStatusUpdateInterval) == 0)		//every 10 times through the loop
			{		
				char* thePrefStr = "";
				UInt32 theLen = 0;

		
				if ( (loopCount % (sStatusUpdateInterval*10)) == 0 )
					printf("  RTP-Conns RTSP-Conns HTTP-Conns  kBits/Sec   Pkts/Sec    TotConn   TotBytes   TotPktsLost\n");


				(void)QTSS_GetValueAsString(sServer, qtssRTPSvrCurConn, 0, &thePrefStr);
				printf( "%11s", thePrefStr);

				(void)QTSS_GetValueAsString(sServer, qtssRTSPCurrentSessionCount, 0, &thePrefStr);
				printf( "%11s", thePrefStr);
				
				(void)QTSS_GetValueAsString(sServer, qtssRTSPHTTPCurrentSessionCount, 0, &thePrefStr);
				printf( "%11s", thePrefStr);
				
				UInt32 curBandwidth = 0;
				theLen = sizeof(curBandwidth);
				(void)QTSS_GetValue(sServer, qtssRTPSvrCurBandwidth, 0, &curBandwidth, &theLen);
				printf( "%11lu", curBandwidth/1024);

				(void)QTSS_GetValueAsString(sServer, qtssRTPSvrCurPackets, 0, &thePrefStr);
				printf( "%11s", thePrefStr);

				(void)QTSS_GetValueAsString(sServer, qtssRTPSvrTotalConn, 0, &thePrefStr);
				printf( "%11s", thePrefStr);

				(void)QTSS_GetValueAsString(sServer, qtssRTPSvrTotalBytes, 0, &thePrefStr);
				printf( "%11s", thePrefStr);

				printf( "%11"_64BITARG_"u", sServer->GetTotalRTPPacketsLost());
				
				
				printf( "\n");
			}
			
			loopCount++;

		}

		theServerState = sServer->GetServerState();
		if (theServerState == qtssIdleState)
			sServer->KillAllRTPSessions();
	}
		
	//first off, make sure that the server can't do any work
	TaskThreadPool::RemoveThreads();
	
	//now that the server is definitely stopped, it is safe to initate
	//the shutdown process
	delete sServer;
	
	//ok, we're ready to exit. If we're quitting because of some fatal error
	//while running the server, make sure to let the parent process know by
	//exiting with a nonzero status. Otherwise, exit with a 0 status
	if (theServerState == qtssFatalErrorState)
		::exit (-1);//signals parent process to restart server
}
