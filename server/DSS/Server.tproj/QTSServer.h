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
	File:		QTSServer.h

	Contains:	This object is responsible for bringing up & shutting down
				the server. It also loads & initializes all modules.
	
	

*/

#ifndef __QTSSERVER_H__
#define __QTSSERVER_H__

#include "QTSServerInterface.h"

class SDPTimeoutTask;
class RTCPTask;
class RTSPListenerSocket;
class RTPSocketPool;

class QTSServer : public QTSServerInterface
{
	public:

		QTSServer() {}
		virtual ~QTSServer();

		//
		// Initialize
		//
		// This function starts the server. If it returns true, the server has
		// started up sucessfully. If it returns false, a fatal error occurred
		// while attempting to start the server.
		//
		// This function *must* be called before the server creates any threads,
		// because one of its actions is to change the server to the right UID / GID.
		// Threads will only inherit these if they are created afterwards.
		Bool16 Initialize(XMLPrefsParser* inPrefsSource, PrefsSource* inMessagesSource,
							UInt16 inPortOverride);
		
		//
		// InitModules
		//
		// Initialize *does not* do much of the module initialization tasks. This
		// function may be called after the server has created threads, but the
		// server must not be in a state where it can do real work. In other words,
		// call this function right after calling Initialize.					
		void InitModules(QTSS_ServerState inEndState);
		
		//
		// StartTasks
		//
		// The server has certain global tasks that it runs for things like stats
		// updating and RTCP processing. This function must be called to start those
		// going, and it must be called after Initialize				
		void StartTasks();


		//
		// RereadPrefsService
		//
		// This service is registered by the server (calling "RereadPreferences").
		// It rereads the preferences. Anyone can call this to reread the preferences,
		// and it may be called safely at any time, though it will fail with a
		// QTSS_OutOfState if the server isn't in the qtssRunningState.
		
		static QTSS_Error RereadPrefsService(QTSS_ServiceFunctionArgsPtr inArgs);

		//
		// CreateListeners
		//
		// This function may be called multiple times & at any time.
		// It updates the server's listeners to reflect what the preferences say.
		// Returns false if server couldn't listen on one or more of the ports, true otherwise
		Bool16 					CreateListeners(Bool16 startListeningNow, QTSServerPrefs* inPrefs, UInt16 inPortOverride);

		//
		// SetDefaultIPAddr
		//
		// Sets the IP address related attributes of the server.
		Bool16 					SetDefaultIPAddr();
		
	private:
	
		//
		// GLOBAL TASKS
		RTCPTask* 			fRTCPTask;
		RTPStatsUpdaterTask*fStatsTask;
		SDPTimeoutTask		*fSDPTimeoutTask;
		SessionTimeoutTask	*fSessionTimeoutTask;
		static char*		sPortPrefString;
		static XMLPrefsParser* sPrefsSource;
		static PrefsSource* sMessagesSource;
		
		//
		// Module loading & unloading routines
		
		static QTSS_Callbacks	sCallbacks;
		
		// Sets up QTSS API callback routines
		void					InitCallbacks();
		
		// Loads compiled-in modules
		void					LoadCompiledInModules();

		// Loads modules from disk
		void 					LoadModules(QTSServerPrefs* inPrefs);
		void					CreateModule(char* inModuleFolderPath, char* inModuleName);
		
		// Adds a module to the module array
		Bool16				 	AddModule(QTSSModule* inModule);
		
		// Call module init roles
		void 					DoInitRole();
		void 					SetupPublicHeader();
		UInt32					GetRTSPIPAddr(QTSServerPrefs* inPrefs);
		
		// Build & destroy the optimized role / module arrays for invoking modules
		void 					BuildModuleRoleArrays();
		void 					DestroyModuleRoleArrays();
		
		Bool16					SetupUDPSockets();

		Bool16					SwitchPersonality();
		
#ifndef __MACOS__
#ifndef __Win32__
		static pid_t 			sMainPid;
#endif
#endif
				
		friend class RTPSocketPool;
};
#endif // __QTSSERVER_H__


