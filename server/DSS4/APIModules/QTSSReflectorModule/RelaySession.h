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
	File:		RelaySession.h

	Contains:	Subclass of ReflectorSession. It has two static
				attributes (QTSSRelayModule Attributes object
				and the ReflectorSession attribute ID)	

*/

#include "QTSS.h"
#include "ReflectorSession.h"
#include "StrPtrLen.h"
#include "SourceInfo.h"
#include "RTSPSourceInfo.h"

#ifndef _RELAY_SESSION_
#define _RELAY_SESSION_

class RelaySession : public ReflectorSession
{
	public:
		
		// Call Register in the Relay Module's Register Role
		static void Register();
		
		//
		// Initialize
		// Call Initialize in the Relay Module's Initialize Role
		static void Initialize(QTSS_Object inAttrObject);
		
		RelaySession(StrPtrLen* inSourceID, SourceInfo* inInfo = NULL):ReflectorSession(inSourceID, inInfo){};
		~RelaySession();
         
		QTSS_Error SetupRelaySession(SourceInfo* inInfo);
        
                QTSS_Object	GetRelaySessionObject() { return fRelaySessionObject; }
		static QTSS_AttributeID		sRelayOutputObject;

                static char			sRelayUserAgent[128];
                
	private:
		
		QTSS_Object					fRelaySessionObject;
		
		// gets set in the initialize method
		static QTSS_Object			relayModuleAttributesObject;
		
		static QTSS_ObjectType		qtssRelaySessionObjectType;
        
		static QTSS_AttributeID		sRelaySessionObjectID;
                static QTSS_AttributeID		sRelayName;
		static QTSS_AttributeID		sSourceType;
		static QTSS_AttributeID		sSourceIPAddr;
		static QTSS_AttributeID		sSourceInIPAddr;
		static QTSS_AttributeID		sSourceUDPPorts;
		static QTSS_AttributeID		sSourceRTSPPort;
		static QTSS_AttributeID		sSourceURL;
		static QTSS_AttributeID		sSourceUsername;
		static QTSS_AttributeID		sSourcePassword;
		static QTSS_AttributeID		sSourceTTL;
                
                
};

#endif
