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

#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#ifndef __MACOS__
#include <sys/resource.h>
#include <sys/wait.h>
#endif

#ifndef __Win32__
#include <unistd.h>
#endif

#ifdef __solaris__	
#include "daemon.h"
#endif

#include "FilePrefsSource.h"
#ifdef __MacOSXServer__
#include "NetInfoPrefsSource.h"
#endif

#include "RunServer.h"
#include "QTSServer.h"
#include "QTSSExpirationDate.h"
#include "GenerateXMLPrefs.h"


void sigcatcher(int sig, int /*sinfo*/, struct sigcontext* /*sctxt*/);

void sigcatcher(int sig, int /*sinfo*/, struct sigcontext* /*sctxt*/)
{
#if DEBUG
	//printf("Signal %d caught\n", sig);
#endif

	//
	// SIGHUP means we should reread our preferences
	if (sig == SIGHUP)
		(void)QTSServer::RereadPrefsService(NULL);
		
	//Try to shut down gracefully the first time, shutdown forcefully the next time
	if (sig == SIGINT)
	{
		if (QTSServerInterface::GetServer()->GetServerState() == qtssRunningState)
		{
			QTSS_ServerState theShutDownState = qtssShuttingDownState;
			(void)QTSS_SetValue(QTSServerInterface::GetServer(), qtssSvrState, 0, &theShutDownState, sizeof(theShutDownState));
		}
		else
			exit(1);
	}
			
}

extern "C" {
typedef int (*EntryFunction)(int input);
}


int main(int argc, char * argv[]) 
{
	extern char* optarg;
	
	// on write, don't send signal for SIGPIPE, just set errno to EPIPE
	// and return -1
	//signal is a deprecated and potentially dangerous function
	//(void) ::signal(SIGPIPE, SIG_IGN);
	struct sigaction act;
	
#if defined(sun) || defined(i386) || defined(__powerpc__)
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = (void(*)(int))&sigcatcher;
#else
	act.sa_mask = 0;
	act.sa_flags = 0;
	act.sa_handler = (void(*)(...))&sigcatcher;
#endif
	(void)::sigaction(SIGPIPE, &act, NULL);
	(void)::sigaction(SIGHUP, &act, NULL);
	(void)::sigaction(SIGINT, &act, NULL);

#if __MacOSXServer__ || __MacOSX__ || __solaris__
	//grow our pool of file descriptors to the max!
	struct rlimit rl;
               
	rl.rlim_cur = Socket::kMaxNumSockets;
	rl.rlim_max = Socket::kMaxNumSockets;

	setrlimit (RLIMIT_NOFILE, &rl);
#endif
	
	//First thing to do is to read command-line arguments.
	int ch;
	UInt16 thePort = 0; //port can be set on the command line
	int statsUpdateInterval = 0;
	
	Bool16 dontFork = false;
	Bool16 theXMLPrefsExist = true;
	
#ifdef __MACOS__
	// On MacOS, grab config files out of current directory
	static char* sDefaultConfigFilePath = "streamingserver.conf";
	static char* sDefaultXMLFilePath = "streamingserver.xml";
#elif __MacOSXServer__
	// On MacOSXServer, default config source is NetInfo
	static char* sDefaultConfigFilePath = NULL;
	static char* sDefaultXMLFilePath = "/etc/streaming/streamingserver.xml";
#else
	// Otherwise, grab files out of /etc
	static char* sDefaultConfigFilePath = "/etc/streamingserver.conf";
	static char* sDefaultXMLFilePath = "/etc/streaming/streamingserver.xml";
#endif

	char* theConfigFilePath = sDefaultConfigFilePath;
	char* theXMLFilePath = sDefaultXMLFilePath;
	
	
#ifndef __MACOS__
	while ((ch = getopt(argc,argv, "vdZfp:c:xo:s:")) != EOF)
	{
		switch(ch)
		{
			case 'v':
				printf("%s/%s-%s Built on: %s\n", 	QTSServerInterface::GetServerName().Ptr,
													QTSServerInterface::GetServerVersion().Ptr,
													QTSServerInterface::GetServerPlatform().Ptr,
													QTSServerInterface::GetServerBuildDate().Ptr);
				printf("-d: Run in the foreground\n");
				//printf("-f: Use config file at /etc/streamingserver.conf\n");
				printf("-p XXX: Specify the default RTSP listening port of the server\n");
				printf("-c /myconfigpath.xml: Specify a config file\n");
				printf("-o /myconfigpath.conf: Specify a DSS 1.x / 2.x config file\n");
				printf("-x: Force create new .xml config file from 1.x / 2.x config\n");
				printf("-s n: Display server stats in the console every \"n\" seconds\n");
				::exit(0);	
			case 'd':
				dontFork = true;
				break;
			case 'f':
				theXMLFilePath  = "/etc/streaming/streamingserver.xml";
				break;
			case 'p':
				thePort = ::atoi(optarg);
				break;
			case 's':
				statsUpdateInterval = ::atoi(optarg);
				break;
			case 'c':
				theXMLFilePath = optarg;
				break;
			case 'o':
				theConfigFilePath = optarg;
				break;
			case 'x':
				theXMLPrefsExist = false; // Force us to generate a new XML prefs file
				break;
			default:
				break;
		}
	}
	
	// Check expiration date
	QTSSExpirationDate::PrintExpirationDate();
	if (QTSSExpirationDate::IsSoftwareExpired())
	{
		printf("Streaming Server has expired\n");
		::exit(0);
	}

	//Unless the command line option is set, fork & daemonize the process at this point
	if (!dontFork)
	{
#ifdef __sgi
		// for some reason, this method doesn't work right on IRIX 6.4 unless the first arg
		// is _DF_NOFORK.  if the first arg is 0 (as it should be) the result is a server
		// that is essentially paralized and doesn't respond to much at all.  So for now,
		// leave the first arg as _DF_NOFORK
		if (_daemonize(_DF_NOFORK, STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO) != 0)
#else
		if (daemon(0,0) != 0)
#endif
		{
#if DEBUG
			printf("Failed to daemonize process. Error = %d\n", OSThread::GetErrno());
#endif
			exit(-1);
		}
	}
#endif //#ifndef __MACOS__

	XMLPrefsParser theXMLParser(theXMLFilePath);
	
	//
	// Check to see if the XML file exists as a directory. If it does,
	// just bail because we do not want to overwrite a directory
	if (theXMLParser.DoesFileExistAsDirectory())
	{
		printf("Directory located at location where streaming server prefs file should be.\n");
		exit(-1);
	}

	// If we aren't forced to create a new XML prefs file, whether
	// we do or not depends solely on whether the XML prefs file exists currently.
	if (theXMLPrefsExist)
		theXMLPrefsExist = theXMLParser.DoesFileExist();
	
	if (!theXMLPrefsExist)
	{
		
		//
		// The XML prefs file doesn't exist, so let's create an old-style
		// prefs source in order to generate a fresh XML prefs file.
		
		if (theConfigFilePath != NULL)
		{	
			FilePrefsSource* filePrefsSource = new FilePrefsSource(true); // Allow dups
			
			if ( filePrefsSource->InitFromConfigFile(theConfigFilePath) )
				printf("Could not load configuration file at %s. Generating a new prefs file at %s\n", theConfigFilePath, theXMLFilePath);
			
			if (GenerateAllXMLPrefs(filePrefsSource, &theXMLParser))
			{
				printf("Fatal Error: Could not create new prefs file at: %s. (%d)\n", theXMLFilePath, OSThread::GetErrno());
				::exit(-1);
			}
		}
	#if __MacOSXServer__
		else
		{
			NetInfoPrefsSource* netInfoPrefsSource = new NetInfoPrefsSource();
			
			if (GenerateStandardXMLPrefs(netInfoPrefsSource, &theXMLParser))
			{
				printf("Fatal Error: Could not create new prefs file at: %s. (%d)\n", theConfigFilePath, OSThread::GetErrno());
				::exit(-1);
			}
			
		}
	#endif
	}

	//
	// Parse the configs from the XML file
	int xmlParseErr = theXMLParser.Parse();
	if (xmlParseErr)
	{
		printf("Fatal Error: Could not load configuration file at %s. (%d)\n", theXMLFilePath, OSThread::GetErrno());
		::exit(-1);
	}
	
	//Construct a Prefs Source object to get server text messages
	FilePrefsSource theMessagesSource;
	theMessagesSource.InitFromConfigFile("qtssmessages.txt");
	
	//check if we should do auto restart. If so, we should fork the process at this point,
	//have the child run the server. Parent will just wait for the child to die.
	SInt32 autoStartIndex = theXMLParser.GetPrefValueIndexByName( NULL, "auto_restart");
	char* autoStartSetting = theXMLParser.GetPrefValueByIndex( autoStartIndex, 0, NULL, NULL );
	
#ifndef __MACOS__
	int statloc = 0;
	if ((!dontFork) && (autoStartSetting != NULL) && (::strcmp(autoStartSetting, "true") == 0))
	{
		//loop until the server exits normally. If the server doesn't exit
		//normally, then restart it.
		while (true)
		{
			pid_t processID = fork();
			Assert(processID >= 0);
			if (processID > 0)
			{
				::wait(&statloc);
				
#if DEBUG
				::printf("Child Process %d exited with status %d\n", processID, statloc);
#endif
				if (statloc == 0)
					return 0;
			}
			else if (processID == 0)
				break;
			else
				exit(-1);
			//eek. If you auto-restart too fast, you might start the new one before the OS has
			//cleaned up from the old one, resulting in startup errors when you create the new
			//one. Waiting for a second seems to work
			sleep(1);
		}
	}
#endif //#ifndef __MACOS__
	
	//we have to do this again for the child process, because sigaction states
	//do not span multiple processes.
	(void)::sigaction(SIGPIPE, &act, NULL);
	(void)::sigaction(SIGHUP, &act, NULL);
	(void)::sigaction(SIGINT, &act, NULL);
	
	//This function starts, runs, and shuts down the server
	if (::StartServer(&theXMLParser, &theMessagesSource, thePort, statsUpdateInterval) != qtssFatalErrorState)
		::RunServer();
	
}