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
	File:		main.cpp

	Contains:	main function to drive streaming server.


*/

#include <stdio.h>
#include <stdlib.h>
#include "getopt.h"

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

#if defined (__solaris__) || defined (__osf__)
#include "daemon.h"
#endif

#if __MacOSX__ || __FreeBSD__
#include <sys/sysctl.h>
#endif

#include "FilePrefsSource.h"
#include "RunServer.h"
#include "QTSServer.h"
#include "QTSSExpirationDate.h"
#include "GenerateXMLPrefs.h"

static int sSigIntCount = 0;

void usage();

void usage()
{
	printf("%s/%s-%s Built on: %s\n", 	QTSServerInterface::GetServerName().Ptr,
										QTSServerInterface::GetServerVersion().Ptr,
										QTSServerInterface::GetServerPlatform().Ptr,
										QTSServerInterface::GetServerBuildDate().Ptr);
	printf("usage: %s [ -d | -p port | -v | -c /myconfigpath.xml | -o /myconfigpath.conf | -x | -S numseconds | -I | -h ]\n", QTSServerInterface::GetServerName().Ptr);
        printf("-d: Run in the foreground\n");
	printf("-p XXX: Specify the default RTSP listening port of the server\n");
	printf("-c /myconfigpath.xml: Specify a config file\n");
	printf("-o /myconfigpath.conf: Specify a DSS 1.x / 2.x config file to build XML file from\n");
	printf("-x: Force create new .xml config file from 1.x / 2.x config\n");
	printf("-S n: Display server stats in the console every \"n\" seconds\n");
	printf("-I: Start the server in the idle state\n");
	printf("-h: Prints usage\n");
}

void sigcatcher(int sig, int /*sinfo*/, struct sigcontext* /*sctxt*/);

void sigcatcher(int sig, int /*sinfo*/, struct sigcontext* /*sctxt*/)
{
#if DEBUG
	//printf("Signal %d caught\n", sig);
#endif

	//
	// SIGHUP means we should reread our preferences
	if (sig == SIGHUP)
	{
		RereadPrefsTask* task = new RereadPrefsTask;
		task->Signal(Task::kStartEvent);
	}
		
	//Try to shut down gracefully the first time, shutdown forcefully the next time
	if (sig == SIGINT)
	{
		//
		// Tell the server that there has been a SigInt, the main thread will start
		// the shutdown process because of this
		if (sSigIntCount == 0)
			QTSServerInterface::GetServer()->SetSigInt();
		sSigIntCount++;
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
	
#if defined(sun) || defined(i386) || defined(__powerpc__) || defined (__osf__)
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

#if __MacOSX__ || __solaris__
	//grow our pool of file descriptors to the max!
	struct rlimit rl;
               
	rl.rlim_cur = Socket::kMaxNumSockets;
	rl.rlim_max = Socket::kMaxNumSockets;

	setrlimit (RLIMIT_NOFILE, &rl);
#endif

#if __MacOSX__ || __FreeBSD__
        //
        // These 2 OSes have problems with large socket buffer sizes. Make sure they allow even
        // ridiculously large ones, because we may need them to receive a large volume of ACK packets
        // from the client
        
        //
        // We raise the limit imposed by the kernel by calling the sysctl system call.
	int mib[CTL_MAXNAME];
        mib[0] = CTL_KERN;
        mib[1] = KERN_IPC;
        mib[2] = KIPC_MAXSOCKBUF;
        mib[3] = 0;

        int maxSocketBufferSizeVal = 2048 * 1024; // Allow up to 2 MB. That is WAY more than we should need
        int sysctlErr = ::sysctl(mib, 3, 0, 0, &maxSocketBufferSizeVal, sizeof(maxSocketBufferSizeVal));
        //Assert(sysctlErr == 0);
#endif
	
	//First thing to do is to read command-line arguments.
	int ch;
	int thePort = 0; //port can be set on the command line
	int statsUpdateInterval = 0;
	QTSS_ServerState theInitialState = qtssRunningState;
	
	Bool16 dontFork = false;
	Bool16 theXMLPrefsExist = true;
	
#ifdef __MACOS__
	// On MacOS, grab config files out of current directory
	static char* sDefaultConfigFilePath = "streamingserver.conf";
	static char* sDefaultXMLFilePath = "streamingserver.xml";
#elif __MacOSX__
	// On MacOSX, grab files out of /Library/QuickTimeStreaming/Config
    // Since earlier versions of QTSS installed the config file at /etc/streamingserver.conf 
    // leave the default config file path in the old location
	static char* sDefaultConfigFilePath = "/etc/streamingserver.conf";
	static char* sDefaultXMLFilePath = "/Library/QuickTimeStreaming/Config/streamingserver.xml";
#else
	// Otherwise, grab files out of /etc
	static char* sDefaultConfigFilePath = "/etc/streamingserver.conf";
	static char* sDefaultXMLFilePath = "/etc/streaming/streamingserver.xml";
#endif

	char* theConfigFilePath = sDefaultConfigFilePath;
	char* theXMLFilePath = sDefaultXMLFilePath;
	
	
#ifndef __MACOS__
	while ((ch = getopt(argc,argv, "vdfxp:c:o:S:Ih")) != EOF) // opt: means requires option
	{
		switch(ch)
		{
			case 'v':
				usage();
				::exit(0);	
			case 'd':
				dontFork = true;
				
				#if __linux__
					OSThread::WrapSleep(true);
				#endif
				
				break;
			case 'f':
				#if __MacOSX__
					theXMLFilePath  = "/Library/QuickTimeStreaming/Config/streamingserver.xml";
				#else
					theXMLFilePath  = "/etc/streaming/streamingserver.xml";
				#endif
				break;
			case 'p':
				Assert(optarg != NULL);// this means we didn't declare getopt options correctly or there is a bug in getopt.
				thePort = ::atoi(optarg);
				break;
			case 'S':
				Assert(optarg != NULL);// this means we didn't declare getopt options correctly or there is a bug in getopt.
				statsUpdateInterval = ::atoi(optarg);
				break;
			case 'c':
				Assert(optarg != NULL);// this means we didn't declare getopt options correctly or there is a bug in getopt.
				theXMLFilePath = optarg;
				break;
			case 'o':
				Assert(optarg != NULL);// this means we didn't declare getopt options correctly or there is a bug in getopt.
				theConfigFilePath = optarg;
				break;
			case 'x':
				theXMLPrefsExist = false; // Force us to generate a new XML prefs file
				break;
			case 'I':
				theInitialState = qtssIdleState;
				break;
			case 'h':
				usage();
				::exit(0);
			default:
				break;
		}
	}
	
	// Check port
	if (thePort < 0 || thePort > 65535)
	{ 
		printf("Invalid port value = %d max value = 65535\n",thePort);
		exit (-1);
	}

	// Check expiration date
	QTSSExpirationDate::PrintExpirationDate();
	if (QTSSExpirationDate::IsSoftwareExpired())
	{
		printf("Streaming Server has expired\n");
		::exit(0);
	}


	XMLPrefsParser theXMLParser(theXMLFilePath);
	
	//
	// Check to see if the XML file exists as a directory. If it does,
	// just bail because we do not want to overwrite a directory
	if (theXMLParser.DoesFileExistAsDirectory())
	{
		printf("Directory located at location where streaming server prefs file should be.\n");
		exit(-1);
	}
	
	//
	// Check to see if we can write to the file
	if (!theXMLParser.CanWriteFile())
	{
		printf("Cannot write to the streaming server prefs file.\n");
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
				printf("Could not load configuration file at %s.\n Generating a new prefs file at %s\n", theConfigFilePath, theXMLFilePath);
			
			if (GenerateAllXMLPrefs(filePrefsSource, &theXMLParser))
			{
				printf("Fatal Error: Could not create new prefs file at: %s. (%d)\n", theXMLFilePath, OSThread::GetErrno());
				::exit(-1);
			}
		}
	}

	//
	// Parse the configs from the XML file
	int xmlParseErr = theXMLParser.Parse();
	if (xmlParseErr)
	{
		printf("Fatal Error: Could not load configuration file at %s. (%d)\n", theXMLFilePath, OSThread::GetErrno());
		::exit(-1);
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
	
	//Construct a Prefs Source object to get server text messages
	FilePrefsSource theMessagesSource;
	theMessagesSource.InitFromConfigFile("qtssmessages.txt");
	
	//check if we should do auto restart. If so, we should fork the process at this point,
	//have the child run the server. Parent will just wait for the child to die.
	ContainerRef server =theXMLParser.GetRefForServer();
	ContainerRef pref = theXMLParser.GetPrefRefByName( server, "auto_restart" );
	char* autoStartSetting = NULL;
	if (pref != NULL)
		autoStartSetting = theXMLParser.GetPrefValueByRef( pref, 0, NULL, NULL );
	
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
	
#ifdef __solaris__	
	// Set Priority Type to Real Time, timeslice = 100 milliseconds. Change the timeslice upwards as needed. This keeps the server priority above the playlist broadcaster which is a time-share scheduling type.
	char commandStr[64];
	sprintf(commandStr, "priocntl -s -c RT -t 10 -i pid %d", (int) getpid()); 
	(void) ::system(commandStr);	
#endif

	//This function starts, runs, and shuts down the server
	if (::StartServer(&theXMLParser, &theMessagesSource, thePort, statsUpdateInterval, theInitialState) != qtssFatalErrorState)
		::RunServer();
	
}
