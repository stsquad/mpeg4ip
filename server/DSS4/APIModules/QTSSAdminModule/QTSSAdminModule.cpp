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
	File:		QTSSAdminModule.cpp

	Contains:	Implements Admin module

	$Log: QTSSAdminModule.cpp,v $
	Revision 1.1  2002/02/27 19:48:51  wmaycisco
	Adding Darwin server
	
	Revision 1.1  2002/02/25 23:04:19  wmay
	DSS 4.0 commit
	
	Revision 1.28  2001/10/12 22:13:22  sussery
	add HTML body to auth failed message.
	Bug #:
	Submitted by:
	Reviewed by:
	
	Revision 1.27  2001/10/08 18:05:26  mythili
	changed QTSSModuleUtils calls from Get***Pref*** to Get***Attribute***
	
	Revision 1.26  2001/08/29 17:35:03  mythili
	changed admin module to accept any valid user that belongs to the group given by the AdministratorGroup pref
	
	Revision 1.25  2001/08/03 07:01:28  murata
	fix build for RH linux 7.2
	Bug #:
	Submitted by:
	Reviewed by:
	
	Revision 1.24  2001/08/02 19:09:02  murata
	Fix bad ModuleUtils initialize calls with bad parameter.
	Bug #:
	Submitted by:
	Reviewed by:
	
	Revision 1.23  2001/06/07 19:49:14  sussery
	removed obsolete MacOSXServer code
	Bug #:
	Submitted by:
	Reviewed by:
	
	Revision 1.22  2001/03/16 22:22:16  mythili
	took out AdministratorPassword pref from admin module
	
	Revision 1.21  2001/03/13 22:24:03  murata
	Replace copyright notice for license 1.0 with license 1.2 and update the copyright year.
	Bug #:
	Submitted by:
	Reviewed by:
	
	Revision 1.20  2001/03/12 22:20:51  mythili
	fixed error in parsing authname and password code in the admin module, and setting of passwords with whitespace through the web admin for the administrator password
	
	Revision 1.19  2001/02/21 01:14:06  mythili
	memory leak fix for realm ptr
	
	Revision 1.18  2001/02/20 23:13:00  murata
	Fix authentication enabled pref and cleanup.
	Bug #:
	Submitted by:
	Reviewed by:
	
	Revision 1.17  2001/02/20 17:49:13  mythili
	include crypt.h for solaris
	
	Revision 1.16  2001/02/19 23:43:20  murata
	Fix memleaks
	Bug #:
	Submitted by:
	Reviewed by:
	
	Revision 1.15  2001/02/18 23:08:28  mythili
	modified admin module to use auth callbacks
	
	Revision 1.14  2001/02/16 00:55:05  murata
	Add request denied html as feedback for access disabled.
	Bug #:
	Submitted by:
	Reviewed by:
	
	Revision 1.13  2001/01/30 22:21:23  murata
	Reorganize and clean up test code. Default local loopback to 127.0.0.* instead of 127.*.*.*
	Bug #:
	Submitted by:
	Reviewed by:
	
	Revision 1.12  2001/01/28 05:11:38  murata
	Default to authentication required.
	Bug #:
	Submitted by:
	Reviewed by:
	
	Revision 1.11  2001/01/28 04:47:58  murata
	Remove mem leaks and clean up preference code to use the ModuleUtils routines.
	Bug #:
	Submitted by:
	Reviewed by:
	
	Revision 1.10  2001/01/22 22:10:41  murata
	Fix remote access to admin module.
	Bug #:
	Submitted by:
	Reviewed by:
	
	Revision 1.9  2000/12/20 21:29:11  mythili
	fixed the typo in earlier fix
	
	Revision 1.8  2000/12/20 21:11:12  mythili
	fixed minor but quicklook blck bug
	
	Revision 1.7  2000/12/19 00:56:57  murata
	Add IPAccessList to prefs file if it isn't found.
	Bug #:
	Submitted by:
	Reviewed by:
	
	Revision 1.6  2000/12/01 00:49:51  murata
	update prefs for enabling and disabling features.
	Bug #:
	Submitted by:
	Reviewed by:
	
	Revision 1.5  2000/11/16 22:47:11  murata
	Add module description and up the version.
	Bug #:
	Submitted by:
	Reviewed by:
	
	Revision 1.4  2000/10/24 19:59:44  serenyi
	Updated to reflect latest QTSS API changes
	
	Revision 1.3  2000/10/13 08:20:27  serenyi
	import
	
	Revision 1.7  2000/10/05 04:07:25  mythili
	john added line to close session after sending 401 response
	
	Revision 1.6  2000/09/29 06:41:06  serenyi
	Working on getting Win32 server to compile
	
	Revision 1.5  2000/09/29 01:02:22  murata
	Change password pref name to "AdministratorPassword"
	
	Revision 1.4  2000/09/27 21:10:53  lecroy
	change AdministratorPassword back to Administrator
	
	Revision 1.3  2000/09/26 01:44:03  lecroy
	renamed  <Administrator> attribute to <AdministratorPassword>
	
	Revision 1.2  2000/09/25 22:29:08  serenyi
	Got rid of permissions argument to QTSS_Add***Attribute
	
	Revision 1.1.1.1  2000/08/31 00:30:22  serenyi
	Mothra Repository
	
	Revision 1.18  2000/08/22 13:49:54  murata
	Get the server's version header.
	
	Revision 1.17  2000/08/22 01:14:02  murata
	No features or bug fixes just some code cleanup.
	
	Revision 1.16  2000/08/15 22:49:19  murata
	Change version #.
	
	Revision 1.14  2000/08/15 02:13:28  murata
	*** empty log message ***
	
	Revision 1.13  2000/08/15 01:01:35  murata
	Change from SInt64 to UInt32 for millisecond time value.
	
	Revision 1.12  2000/08/11 19:54:28  murata
	Change version and turn on local only as default. Optimize query data gathering.
	
	Revision 1.11  2000/08/10 01:11:10  murata
	Latest changes.
	
	Revision 1.10  2000/08/05 02:45:01  murata
	Fix constant and resynch with API changes.
	
	Revision 1.9  2000/08/05 01:51:29  murata
	Change version to today's date.
	
	Revision 1.8  2000/08/05 01:46:36  murata
	Fix some bugs.
	
	Revision 1.7  2000/08/03 00:20:27  murata
	Change version
	
	Revision 1.6  2000/08/03 00:18:32  murata
	Fix bugs.
	
	Revision 1.5  2000/07/28 12:35:38  murata
	Fix to call re-read prefs for adding to array lists.
	Added local loop back check and IP address list filter.
	
	Revision 1.4  2000/07/27 02:39:29  murata
	up the version number.
	
	Revision 1.3  2000/07/27 02:38:57  murata
	Added feature: HTTP 1.1 Basic authentication
	Added feature: module preference to enable authentication (off by default for now will change later)
	Added feature: Administrator password as module preference (NULL by default)
	Added feature: filter1 through filter10 for get commands (no filter = all)
	Fixed bug: re-read preferences service is called only on set.
	
	Revision 1.2  2000/07/26 01:10:13  murata
	Fix update of preferences by calling re-read prefs service.
	
	Revision 1.1  2000/07/24 23:50:25  murata
	tuff for Ghidra/Rodan
	
	Revision 1.2  2000/06/29 23:54:56  serenyi
	Fixes for Win32 build
	
	Revision 1.1  2000/06/17 04:20:11  murata
	Add QTSSAdminModule and fix Project Builder,Jam and Posix Make builds.
	Bug #:
	Submitted by:
	Reviewed by:
	

	

*/


#ifndef __Win32__
	#include <unistd.h>		/* for getopt() et al */
#endif

#include <time.h>
#include <stdio.h>		/* for printf */
#include <stdlib.h>		/* for getloadavg & other useful stuff */
#include "QTSSAdminModule.h"
#include "OSArrayObjectDeleter.h"
#include "StringParser.h"
#include "StrPtrLen.h"
#include "QTSSModuleUtils.h"
#include "OSHashTable.h"
#include "OSMutex.h"
#include "StrPtrLen.h"
#include "OSRef.h"
#include "AdminElementNode.h"
#include "base64.h"
#include "OSMemory.h"
#include "md5.h"
#include "md5digest.h"

#if __solaris__ || __linux__
         #include <crypt.h>
#endif

#define DEBUG_ADMIN_MODULE 0


//**************************************************
#define kAuthNameAndPasswordBuffSize 512
#define kPasswordBuffSize kAuthNameAndPasswordBuffSize/2

// STATIC DATA
//**************************************************
#if DEBUG_ADMIN_MODULE
static UInt32					sRequestCount= 0;
#endif

static QTSS_Initialize_Params sQTSSparams;

//static char* sResponseHeader = "HTTP/1.0 200 OK\r\nServer: QTSS\r\nConnection: Close\r\nContent-Type: text/plain\r\n\r\n";
static char* sResponseHeader = "HTTP/1.0 200 OK";
static char* sUnauthorizedResponseHeader = "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"QTSS/modules/admin\"\r\nServer: QTSS\r\nConnection: Close\r\nContent-Type: text/plain\r\n\r\n";
static char* sPermissionDeniedHeader = "HTTP/1.1 403 Forbidden\r\nConnection: Close\r\nContent-Type: text/html\r\n\r\n";
static char* sHTMLBody =  "<HTML><BODY>\n<P><b>Your request was denied by the server.</b></P>\n</BODY></HTML>\r\n\r\n";
	

static char* sVersionHeader = NULL;
static char* sConnectionHeader = "Connection: Close";
static char* kDefaultHeader = "Server: QTSS";
static char* sContentType = "Content-Type: text/plain";
static char* sEOL = "\r\n";
static char* sEOM = "\r\n\r\n";
static char* sAuthRealm = "QTSS/modules/admin";
static char* sAuthResourceLocalPath = "/modules/admin/";

static QTSS_ServerObject		sServer = NULL;
static QTSS_ModuleObject		sModule = NULL;
static QTSS_ModulePrefsObject	sModulePrefs = NULL;
static QTSS_PrefsObject 		sServerPrefs = NULL;
static AdminClass				*sAdminPtr = NULL;
static QueryURI 				*sQueryPtr = NULL;
static OSMutex*					sAdminMutex = NULL;//admin module isn't reentrant
static UInt32					sVersion=11162000;
static char *sDesc="Implements HTTP based Admin Protocol for accessing server attributes";
static char decodedLine[kAuthNameAndPasswordBuffSize] = { 0 };
static char codedLine[kAuthNameAndPasswordBuffSize] = { 0 }; 
static QTSS_TimeVal				sLastRequestTime = 0;
static UInt32	 				sSessID = 0;

// ATTRIBUTES
//**************************************************
enum { kMaxRequestTimeIntervalMilli = 1000, kDefaultRequestTimeIntervalMilli = 50 };
static UInt32 sDefaultRequestTimeIntervalMilli = kDefaultRequestTimeIntervalMilli;
static UInt32 sRequestTimeIntervalMilli = kDefaultRequestTimeIntervalMilli;

static Bool16 sAuthenticationEnabled = true;
static Bool16 sDefaultAuthenticationEnabled = true;

static Bool16 sLocalLoopBackOnlyEnabled = true;
static Bool16 sDefaultLocalLoopBackOnlyEnabled = true;

static Bool16 sEnableRemoteAdmin = true;
static Bool16 sDefaultEnableRemoteAdmin = true;

static QTSS_AttributeID		sIPAccessListID = qtssIllegalAttrID;
static char*			sIPAccessList = NULL;
static char*			sLocalLoopBackAddress = "127.0.0.*";

static char*	sAdministratorGroup = NULL;
static char*	sDefaultAdministratorGroup = "admin";

#define kNumComponents 4
static StrPtrLen sLocalLoopComponents[kNumComponents];
#define kRemoteAddressSize 20

//static char*			sAdministrator = "streamingadmin";

static Bool16			sFlushing = false;
static QTSS_AttributeID	sFlushingID = qtssIllegalAttrID;
static char*			sFlushingName = "QTSSAdminModuleFlushingState";
static UInt32			sFlushingLen = sizeof(sFlushing);

static QTSS_AttributeID         sAuthenticatedID = qtssIllegalAttrID;
static char*			sAuthenticatedName = "QTSSAdminModuleAuthenticatedState";

//**************************************************

static QTSS_Error QTSSAdminModuleDispatch(QTSS_Role inRole, QTSS_RoleParamPtr inParams);
static QTSS_Error Register(QTSS_Register_Params* inParams);
static QTSS_Error Initialize(QTSS_Initialize_Params* inParams);
static QTSS_Error FilterRequest(QTSS_Filter_Params* inParams);
static QTSS_Error RereadPrefs();
static QTSS_Error AuthorizeAdminRequest(QTSS_StandardRTSP_Params* inParams);
								
static Bool16 IsValidAddress(char *theAddressPtr, StrPtrLen *parseResultPtr);
#define SetValidAddress(theAddressPtr, parseResultPtr) IsValidAddress(theAddressPtr, parseResultPtr);

static Bool16 AddressAllowed(StrPtrLen *testPtr, StrPtrLen *allowedPtr);


#if !DEBUG_ADMIN_MODULE
	#define APITests_DEBUG() 
	#define ShowQuery_DEBUG()
#else
void ShowQuery_DEBUG()
{
	printf("======REQUEST #%lu======\n",++sRequestCount);
	StrPtrLen* 	aStr;
	aStr = sQueryPtr->GetURL();
	printf("URL="); PRINT_STR(aStr); 

	aStr = sQueryPtr->GetQuery();
	printf("Query="); PRINT_STR(aStr); 

	aStr = sQueryPtr->GetParameters();
	printf("Parameters="); PRINT_STR(aStr); 

	aStr = sQueryPtr->GetCommand();
	printf("Command="); PRINT_STR(aStr); 
	printf("CommandID=%ld \n",sQueryPtr->GetCommandID());
	aStr = sQueryPtr->GetValue();
	printf("Value="); PRINT_STR(aStr); 
	aStr = sQueryPtr->GetType();
	printf("Type="); PRINT_STR(aStr); 
	aStr = sQueryPtr->GetAccess();
	printf("Access="); PRINT_STR(aStr); 
}		

void APITests_DEBUG()
{
	if (0)
	{	printf("QTSSAdminModule start tests \n");
	
		if (0)
		{
			printf("admin called locked \n");
			const int ksleeptime = 15;
			printf("sleeping for %d seconds \n",ksleeptime);
			sleep(ksleeptime);
			printf("done sleeping \n");
			printf("QTSS_GlobalUnLock \n");
			(void) QTSS_GlobalUnLock();
			printf("again sleeping for %d seconds \n",ksleeptime);
			sleep(ksleeptime);
		}
	
		if (0)
		{
			printf(" GET VALUE PTR TEST \n");

			QTSS_Object *sessionsPtr = NULL;
			UInt32		paramLen = sizeof(sessionsPtr);
			UInt32		numValues = 0;
			QTSS_Error 	err = 0;
			
			err = QTSS_GetNumValues (sServer, qtssSvrClientSessions, &numValues);
			err = QTSS_GetValuePtr(sServer, qtssSvrClientSessions, 0, (void**)&sessionsPtr, &paramLen);
			printf("Admin Module Num Sessions = %lu sessions[0] = %ld err = %ld paramLen =%lu\n", numValues, (SInt32) *sessionsPtr,err,paramLen);
	
			UInt32		numAttr = 0;
			if (sessionsPtr)
			{	err = QTSS_GetNumAttributes (*sessionsPtr, &numAttr);
				printf("Admin Module Num attributes = %lu sessions[0] = %ld  err = %ld\n", numAttr, (SInt32) *sessionsPtr,err);
		
				QTSS_Object theAttributeInfo;
				char nameBuff[128];
				UInt32 len = 127;
				for (UInt32 i = 0; i < numAttr; i++)
				{	err = QTSS_GetAttrInfoByIndex(*sessionsPtr, i, &theAttributeInfo);
					nameBuff[0] = 0;len = 127;
					err = QTSS_GetValue (theAttributeInfo, qtssAttrName,0, nameBuff,&len);
					nameBuff[len] = 0;
					printf("found %s \n",nameBuff);
				}
			}
		}
		
		if (0)
		{
			printf(" GET VALUE TEST \n");

			QTSS_Object sessions = NULL;
			UInt32		paramLen = sizeof(sessions);
			UInt32		numValues = 0;
			QTSS_Error 	err = 0;
			
			err = QTSS_GetNumValues (sServer, qtssSvrClientSessions, &numValues);
			err = QTSS_GetValue(sServer, qtssSvrClientSessions, 0, (void*)&sessions, &paramLen);
			printf("Admin Module Num Sessions = %lu sessions[0] = %ld err = %ld paramLen = %lu\n", numValues, (SInt32) sessions,err, paramLen);
			
			if (sessions)
			{
				UInt32		numAttr = 0;
				err = QTSS_GetNumAttributes (sessions, &numAttr);
				printf("Admin Module Num attributes = %lu sessions[0] = %ld  err = %ld\n", numAttr,(SInt32) sessions,err);
				
				QTSS_Object theAttributeInfo;
				char nameBuff[128];
				UInt32 len = 127;
				for (UInt32 i = 0; i < numAttr; i++)
				{	err = QTSS_GetAttrInfoByIndex(sessions, i, &theAttributeInfo);
					nameBuff[0] = 0;len = 127;
					err = QTSS_GetValue (theAttributeInfo, qtssAttrName,0, nameBuff,&len);
					nameBuff[len] = 0;
					printf("found %s \n",nameBuff);
				}
			}
		}
		

		if (0)
		{
			printf("----------------- Start test ----------------- \n");
			printf(" GET indexed pref TEST \n");

			QTSS_Error 	err = 0;
			
			UInt32		numAttr = 1;
			err = QTSS_GetNumAttributes (sModulePrefs, &numAttr);
			printf("Admin Module Num preference attributes = %lu err = %ld\n", numAttr, err);
				
			QTSS_Object theAttributeInfo;
			char valueBuff[512];
			char nameBuff[128];
			QTSS_AttributeID theID;
			UInt32 len = 127;
			UInt32 i = 0;
			printf("first pass over preferences\n");
			for ( i = 0; i < numAttr; i++)
			{	err = QTSS_GetAttrInfoByIndex(sModulePrefs, i, &theAttributeInfo);
				nameBuff[0] = 0;len = 127;
				err = QTSS_GetValue (theAttributeInfo, qtssAttrName,0, nameBuff,&len);
				nameBuff[len]=0;

				theID = qtssIllegalAttrID; len = sizeof(theID);
				err = QTSS_GetValue (theAttributeInfo, qtssAttrID,0, &theID,&len);
				printf("found preference=%s \n",nameBuff);
			}
			valueBuff[0] = 0;len = 512;
			err = QTSS_GetValue (sModulePrefs, theID,0, valueBuff,&len);valueBuff[len] = 0;
			printf("Admin Module QTSS_GetValue name = %s id = %ld value=%s err = %ld\n", nameBuff,theID, valueBuff, err);
			err = QTSS_SetValue (sModulePrefs,theID,0, valueBuff,len);
			printf("Admin Module QTSS_SetValue name = %s id = %ld value=%s err = %ld\n", nameBuff,theID, valueBuff, err);
			
			{	QTSS_ServiceID id;
				(void) QTSS_IDForService(QTSS_REREAD_PREFS_SERVICE, &id);			
				(void) QTSS_DoService(id, NULL);
			}

			valueBuff[0] = 0;len = 512;
			err = QTSS_GetValue (sModulePrefs, theID,0, valueBuff,&len);valueBuff[len] = 0;
			printf("Admin Module QTSS_GetValue name = %s id = %ld value=%s err = %ld\n", nameBuff,theID, valueBuff, err);
			err = QTSS_SetValue (sModulePrefs,theID,0, valueBuff,len);
			printf("Admin Module QTSS_SetValue name = %s id = %ld value=%s err = %ld\n", nameBuff,theID, valueBuff, err);
				
			printf("second pass over preferences\n");
			for ( i = 0; i < numAttr; i++)
			{	err = QTSS_GetAttrInfoByIndex(sModulePrefs, i, &theAttributeInfo);
				nameBuff[0] = 0;len = 127;
				err = QTSS_GetValue (theAttributeInfo, qtssAttrName,0, nameBuff,&len);
				nameBuff[len]=0;

				theID = qtssIllegalAttrID; len = sizeof(theID);
				err = QTSS_GetValue (theAttributeInfo, qtssAttrID,0, &theID,&len);
				printf("found preference=%s \n",nameBuff);
			}
			printf("----------------- Done test ----------------- \n");
		}
			
	}
}

#endif

inline void KeepSession(QTSS_RTSPRequestObject theRequest,Bool16 keep)
{
	(void)QTSS_SetValue(theRequest, qtssRTSPReqRespKeepAlive, 0, &keep, sizeof(keep));
}

// FUNCTION IMPLEMENTATIONS

QTSS_Error QTSSAdminModule_Main(void* inPrivateArgs)
{
	return _stublibrary_main(inPrivateArgs, QTSSAdminModuleDispatch);
}


QTSS_Error 	QTSSAdminModuleDispatch(QTSS_Role inRole, QTSS_RoleParamPtr inParams)
{
 	switch (inRole)
	{
		case QTSS_Register_Role:
			return Register(&inParams->regParams);
		case QTSS_Initialize_Role:
			return Initialize(&inParams->initParams);
		case QTSS_RTSPFilter_Role:
		{	if (!sEnableRemoteAdmin) 
				break;
			return FilterRequest(&inParams->rtspFilterParams);
		}
		case QTSS_RTSPAuthorize_Role:
				return AuthorizeAdminRequest(&inParams->rtspRequestParams);
		case QTSS_RereadPrefs_Role:
			return RereadPrefs();
	}
	return QTSS_NoErr;
}


QTSS_Error Register(QTSS_Register_Params* inParams)
{
	// Do role & attribute setup
	(void)QTSS_AddRole(QTSS_Initialize_Role);
	(void)QTSS_AddRole(QTSS_RTSPFilter_Role);
	(void)QTSS_AddRole(QTSS_RereadPrefs_Role);
	(void)QTSS_AddRole(QTSS_RTSPAuthorize_Role);
	
	(void)QTSS_AddStaticAttribute(qtssRTSPRequestObjectType, sFlushingName, NULL, qtssAttrDataTypeBool16);
	(void)QTSS_IDForAttr(qtssRTSPRequestObjectType, sFlushingName, &sFlushingID);

	(void)QTSS_AddStaticAttribute(qtssRTSPRequestObjectType, sAuthenticatedName, NULL, qtssAttrDataTypeBool16);
	(void)QTSS_IDForAttr(qtssRTSPRequestObjectType, sAuthenticatedName, &sAuthenticatedID);

	// Tell the server our name!
	static char* sModuleName = "QTSSAdminModule";
	::strcpy(inParams->outModuleName, sModuleName);

	return QTSS_NoErr;
}

QTSS_Error RereadPrefs()
{	

	delete [] sVersionHeader;
	sVersionHeader = QTSSModuleUtils::GetStringAttribute(sServer, "qtssSvrRTSPServerHeader", kDefaultHeader);

	delete [] sIPAccessList;
	sIPAccessList = QTSSModuleUtils::GetStringAttribute(sModulePrefs, "IPAccessList", sLocalLoopBackAddress);
	sIPAccessListID = QTSSModuleUtils::GetAttrID(sModulePrefs, "IPAccessList");

	QTSSModuleUtils::GetAttribute(sModulePrefs, "Authenticate", 	qtssAttrDataTypeBool16, &sAuthenticationEnabled, &sDefaultAuthenticationEnabled, sizeof(sAuthenticationEnabled));
	QTSSModuleUtils::GetAttribute(sModulePrefs, "LocalAccessOnly", 	qtssAttrDataTypeBool16, &sLocalLoopBackOnlyEnabled, &sDefaultLocalLoopBackOnlyEnabled, sizeof(sLocalLoopBackOnlyEnabled));
	QTSSModuleUtils::GetAttribute(sModulePrefs, "RequestTimeIntervalMilli", 	qtssAttrDataTypeUInt32, &sRequestTimeIntervalMilli, &sDefaultRequestTimeIntervalMilli, sizeof(sRequestTimeIntervalMilli));
	QTSSModuleUtils::GetAttribute(sModulePrefs, "enable_remote_admin", 	qtssAttrDataTypeBool16, &sEnableRemoteAdmin, &sDefaultEnableRemoteAdmin, sizeof(sDefaultEnableRemoteAdmin));
	
	delete [] sAdministratorGroup;
	sAdministratorGroup = QTSSModuleUtils::GetStringAttribute(sModulePrefs, "AdministratorGroup", sDefaultAdministratorGroup);
	
	if (sRequestTimeIntervalMilli > kMaxRequestTimeIntervalMilli) 
	{	sRequestTimeIntervalMilli = kMaxRequestTimeIntervalMilli;
	}

	(void)QTSS_SetValue(sModule, qtssModDesc, 0, sDesc, strlen(sDesc)+1);	
	(void)QTSS_SetValue(sModule, qtssModVersion, 0, &sVersion, sizeof(sVersion));	

	return QTSS_NoErr;
}

QTSS_Error Initialize(QTSS_Initialize_Params* inParams)
{	
	sAdminMutex = NEW OSMutex();
	ElementNode_InitPtrArray();
	// Setup module utils
	QTSSModuleUtils::Initialize(inParams->inMessages, inParams->inServer, inParams->inErrorLogStream);
	(void) SetValidAddress(sLocalLoopBackAddress, &sLocalLoopComponents[0]);

	sQTSSparams = *inParams;
	sServer = inParams->inServer;
	sModule = inParams->inModule;
	sModulePrefs = QTSSModuleUtils::GetModulePrefsObject(sModule);
	sServerPrefs = inParams->inPrefs;
	
	RereadPrefs();
	
	return QTSS_NoErr;
}

	
void ReportErr(QTSS_Filter_Params* inParams, UInt32 err)
{	
	StrPtrLen* urlPtr = sQueryPtr->GetURL();
	StrPtrLen* evalMessagePtr = sQueryPtr->GetEvalMsg();
	char temp[32];
	
	if (urlPtr && evalMessagePtr)	
	{	sprintf(temp,"(%lu)",err);
		(void)QTSS_Write(inParams->inRTSPRequest, "error:", strlen("error:"), NULL, 0);
		(void)QTSS_Write(inParams->inRTSPRequest, temp, strlen(temp), NULL, 0);
		if (sQueryPtr->VerboseParam())
		{	(void)QTSS_Write(inParams->inRTSPRequest, ";URL=", strlen(";URL="), NULL, 0);
			if (urlPtr) (void)QTSS_Write(inParams->inRTSPRequest, urlPtr->Ptr, urlPtr->Len, NULL, 0);
		}
		if (sQueryPtr->DebugParam())
		{
			(void)QTSS_Write(inParams->inRTSPRequest, ";", strlen(";"), NULL, 0);
			(void)QTSS_Write(inParams->inRTSPRequest, evalMessagePtr->Ptr, evalMessagePtr->Len, NULL, 0);				
		}
		(void)QTSS_Write(inParams->inRTSPRequest, "\r\n\r\n", 4, NULL, 0);
	}
}

Bool16 IsValidAddress(char *theAddressPtr, StrPtrLen *parseResultPtr)
{
	Bool16 valid = false;
	StrPtrLen sourceStr(theAddressPtr);
	StringParser IP_Paser(&sourceStr);
	StrPtrLen *piecePtr = parseResultPtr;
	while (IP_Paser.GetDataRemaining() > 0) 
	{	IP_Paser.ConsumeUntil(piecePtr,'.');	
		if (piecePtr->Len == 0) break;
		IP_Paser.ConsumeLength(NULL, 1);
		if (piecePtr == &parseResultPtr[kNumComponents -1])
		{	valid = true;
			break;
		}
		piecePtr++;		
	};
	
	return valid;
}


Bool16 AddressAllowed(StrPtrLen *testPtr, StrPtrLen *allowedPtr)
{
	Bool16 allowed = false;
	SInt16 component= 0;
	for (; component < 4 ; component ++)
	{
		if (allowedPtr->Ptr[0] != '*' || allowedPtr->Len != 1)
			if ( !testPtr->Equal(*allowedPtr) ) break;	
		allowedPtr ++;
		testPtr ++;
	};	
	if (component == 4)
	{	allowed = true;
	}
	return allowed;
}

inline Bool16 AcceptAddress(char *theAddressPtr)
{
	StrPtrLen ipComponentTest[kNumComponents];
	if (!IsValidAddress(theAddressPtr, &ipComponentTest[0]) )
		return false;
	
	Bool16 isLocalRequest = AddressAllowed(&ipComponentTest[0], &sLocalLoopComponents[0]);
	if (isLocalRequest)
		return true;
	
	if (sLocalLoopBackOnlyEnabled && !isLocalRequest)
		return false;
	
	StrPtrLen ipComponentPref[kNumComponents];
	UInt32 paramLen;
	QTSS_Error err;
	UInt32 numValues = 0;
	(void) QTSS_GetNumValues (sModulePrefs, sIPAccessListID, &numValues);
	char acceptAddress[kRemoteAddressSize] = {0};
	while (numValues > 0)
	{	numValues --;
		paramLen = kRemoteAddressSize;
		err = QTSS_GetValue(sModulePrefs, sIPAccessListID, numValues, (void*)acceptAddress, &paramLen);
		if (paramLen == 0 || err != QTSS_NoErr)
			continue;
		acceptAddress[paramLen] = 0;
		if (!IsValidAddress(acceptAddress, &ipComponentPref[0]))
			continue;
	
		if (AddressAllowed(&ipComponentTest[0], &ipComponentPref[0]))
			return true;
	}

	return false;
}

inline Bool16 IsAdminRequest(StringParser *theFullRequestPtr)
{
	Bool16 handleRequest = false;
	if (theFullRequestPtr != NULL) do
	{
		StrPtrLen	strPtr;
		theFullRequestPtr->ConsumeWord(&strPtr);
		if ( !strPtr.Equal(StrPtrLen("GET")) ) break;	//it's a "Get" request
		
		theFullRequestPtr->ConsumeWhitespace();
		if ( !theFullRequestPtr->Expect('/') ) break;	
				
		theFullRequestPtr->ConsumeWord(&strPtr);
		if ( strPtr.Len == 0 || !strPtr.Equal(StrPtrLen("modules") )	) break;
		if (!theFullRequestPtr->Expect('/') ) break;
			
		theFullRequestPtr->ConsumeWord(&strPtr);
		if ( strPtr.Len == 0 || !strPtr.Equal(StrPtrLen("admin") ) ) break;
		handleRequest = true;
		
	} while (false);

	return handleRequest;
}

inline void ParseAuthNameAndPassword(StrPtrLen *codedStrPtr, StrPtrLen* namePtr, StrPtrLen* passwordPtr)
 {
	
	if (!codedStrPtr || (codedStrPtr->Len >= kAuthNameAndPasswordBuffSize) ) 
	{	return; 
	}
	
	StrPtrLen	codedLineStr;
	StrPtrLen	nameAndPassword;
	memset(decodedLine,0,kAuthNameAndPasswordBuffSize);
	memset(codedLine,0,kAuthNameAndPasswordBuffSize);
	
	memcpy (codedLine,codedStrPtr->Ptr,codedStrPtr->Len);
	codedLineStr.Set((char*) codedLine, codedStrPtr->Len);	
	(void) Base64decode(decodedLine, codedLineStr.Ptr);
	
	nameAndPassword.Set((char*) decodedLine, strlen(decodedLine));
	StringParser parsedNameAndPassword(&nameAndPassword);
	
	parsedNameAndPassword.ConsumeUntil(namePtr,':');			
	parsedNameAndPassword.ConsumeLength(NULL, 1);
	//parsedNameAndPassword.ConsumeUntilWhitespace(passwordPtr);
	// password can have whitespace, so read until the end of the line, not just until whitespace
	parsedNameAndPassword.ConsumeUntil(passwordPtr, StringParser::sEOLMask);
	
	namePtr->Ptr[namePtr->Len]= 0;
	passwordPtr->Ptr[passwordPtr->Len]= 0;
	
	//printf("decoded nameAndPassword="); PRINT_STR(&nameAndPassword); 
	//printf("decoded name="); PRINT_STR(namePtr); 
	//printf("decoded password="); PRINT_STR(passwordPtr); 

	return;
};

inline Bool16 HasAuthentication(StringParser *theFullRequestPtr, StrPtrLen* namePtr, StrPtrLen* passwordPtr)
{
//	Authorization: Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ==
	Bool16 hasAuthentication = false;
	StrPtrLen	strPtr;	
	while (theFullRequestPtr->GetDataRemaining() > 0)
	{
		theFullRequestPtr->ConsumeWhitespace();		
		theFullRequestPtr->ConsumeUntilWhitespace(&strPtr);
		if ( strPtr.Len == 0 || !strPtr.Equal(StrPtrLen("Authorization:")) ) 
			continue;	
				
		theFullRequestPtr->ConsumeWhitespace();		
		theFullRequestPtr->ConsumeUntilWhitespace(&strPtr);
		if ( strPtr.Len == 0 || !strPtr.Equal(StrPtrLen("Basic") )	) 
			continue;	

		theFullRequestPtr->ConsumeWhitespace();					
		theFullRequestPtr->ConsumeUntil(&strPtr, StringParser::sEOLMask);
		if ( strPtr.Len == 0 ) 
			break;

		(void) ParseAuthNameAndPassword(&strPtr,namePtr, passwordPtr);
		if (namePtr->Len == 0) 
			break;
		
		hasAuthentication = true;
		
	};
	
	return hasAuthentication;
}

Bool16	Authenticate(QTSS_RTSPRequestObject request, StrPtrLen* namePtr, StrPtrLen* passwordPtr)
{
	Bool16 authenticated = true;
	
	char* authName = namePtr->GetAsCString();
	OSCharArrayDeleter authNameDeleter(authName);
	
	char* authMoviesDir = NULL;
	OSCharArrayDeleter authMovieDirDeleter(authMoviesDir);
	
	(void)QTSS_GetValueAsString(sServerPrefs, qtssPrefsMovieFolder, 0, &authMoviesDir);
	QTSS_ActionFlags authAction = qtssActionFlagsRead | qtssActionFlagsWrite;
	
	// authenticate callback to retrieve the password 
	QTSS_Error err = QTSS_Authenticate(authName, sAuthResourceLocalPath, authMoviesDir, authAction, qtssAuthBasic, request);
	if (err != QTSS_NoErr) {
		printf("QTSSAdminModule::Authenticate: QTSS_Authenticate failed\n");
		return false; // Couldn't even call QTSS_Authenticate...abandon!
	}
	
	// Get the user profile object from the request object that was created in the authenticate callback
	QTSS_UserProfileObject theUserProfile = NULL;
	UInt32 len = sizeof(QTSS_UserProfileObject);
	err = QTSS_GetValue(request, qtssRTSPReqUserProfile, 0, (void*)&theUserProfile, &len);
	Assert(len == sizeof(QTSS_UserProfileObject));
	if (err != QTSS_NoErr)
		authenticated = false;

	if(err == QTSS_NoErr) {
		char* reqPassword = passwordPtr->GetAsCString();
		OSCharArrayDeleter reqPasswordDeleter(reqPassword);
		char* userPassword = NULL;	
		(void) QTSS_GetValueAsString(theUserProfile, qtssUserPassword, 0, &userPassword);
		OSCharArrayDeleter userPasswordDeleter(userPassword);
	
		if(userPassword == NULL) {
			authenticated = false;
		}
		else {
#ifdef __Win32__
			// The password is md5 encoded for win32
			char md5EncodeResult[120];
			MD5Encode(reqPassword, userPassword, md5EncodeResult, sizeof(md5EncodeResult));
			if(::strcmp(userPassword, md5EncodeResult) != 0)
				authenticated = false;
#else
			if(::strcmp(userPassword, (char*)crypt(reqPassword, userPassword)) != 0)
				authenticated = false;
#endif
		}
	}
	
	char* realm = NULL;
	Bool16 allowed = true;
	//authorize callback to check authorization
	// allocates memory for realm
	err = QTSS_Authorize(request, realm, &allowed);
	if(err != QTSS_NoErr) {
		printf("QTSSAdminModule::Authenticate: QTSS_Authorize failed\n");
		return false; // Couldn't even call QTSS_Authorize...abandon!
	}
	
	// we don't use the realm returned by the callback, but instead 
	// use our own.
	// delete the memory allocated for realm because we don't need it!
	delete [] realm;
	
	if(authenticated && allowed)
		return true;
		
	return false;
}


QTSS_Error AuthorizeAdminRequest(QTSS_StandardRTSP_Params* inParams)
{
	Bool16 allowed = false;

	QTSS_RTSPRequestObject request = inParams->inRTSPRequest;
	
	// get the resource path
	// if the path does not match the admin path, don't handle the request
	char* resourcePath = QTSSModuleUtils::GetLocalPath_Copy(request);
	OSCharArrayDeleter resourcePathDeleter(resourcePath);
	if(strcmp(sAuthResourceLocalPath, resourcePath) != 0)
		return QTSS_NoErr;
	
	// get the type of request
	QTSS_ActionFlags action = QTSSModuleUtils::GetRequestActions(request);
	if(!(action & (qtssActionFlagsRead | qtssActionFlagsWrite)))
		return QTSS_NoErr;
		
	QTSS_UserProfileObject theUserProfile = QTSSModuleUtils::GetUserProfileObject(request);
	if (NULL == theUserProfile)
		return QTSS_RequestFailed;
		
	// This code authorizes the request if the username matches the hardcoded sAdministrator name
	/*char* username = QTSSModuleUtils::GetUserName_Copy(theUserProfile);
	OSCharArrayDeleter usernameDeleter(username);
	
	(void) QTSS_SetValue(request,qtssRTSPReqURLRealm, 0, sAuthRealm, ::strlen(sAuthRealm));
	
	StrPtrLen usernamePtr;
	
	if(username != NULL) 
		usernamePtr.Set(username, ::strlen(username));
	
	if ((username== NULL) || !usernamePtr.Equal(sAdministrator)) {
		if (QTSS_NoErr != QTSS_SetValue(request,qtssRTSPReqUserAllowed, 0, &allowed, sizeof(allowed)))
			return QTSS_RequestFailed; // Bail on the request. The Server will handle the error
		return QTSS_NoErr;
	}
	*/
	
	(void) QTSS_SetValue(request,qtssRTSPReqURLRealm, 0, sAuthRealm, ::strlen(sAuthRealm));
	
	// Authorize the user if the user belongs to the AdministratorGroup (this is an admin module pref)
	UInt32 numGroups = 0;
	char** groupsArray = QTSSModuleUtils::GetGroupsArray_Copy(theUserProfile, &numGroups);
	
	if ((groupsArray != NULL) && (numGroups != 0))
	{
		UInt32 index = 0;
		for (index = 0; index < numGroups; index++)
		{
			if(strcmp(sAdministratorGroup, groupsArray[index]) == 0)
			{
				allowed = true;
				break;
			}
		}
	
		// delete the memory allocated in QTSSModuleUtils::GetGroupsArray_Copy call	
		delete [] groupsArray;
	}
	
	if(!allowed)
	{
		if (QTSS_NoErr != QTSS_SetValue(request,qtssRTSPReqUserAllowed, 0, &allowed, sizeof(allowed)))
			return QTSS_RequestFailed; // Bail on the request. The Server will handle the error
	}
	
	return QTSS_NoErr;
}

inline Bool16 AcceptSession(QTSS_RTSPSessionObject inRTSPSession)
{	
	char remoteAddress[kRemoteAddressSize] = {0};
	StrPtrLen theClientIPAddressStr;
	theClientIPAddressStr.Set(remoteAddress,kRemoteAddressSize);
	QTSS_Error err = QTSS_GetValue(inRTSPSession, qtssRTSPSesRemoteAddrStr, 0, (void*)theClientIPAddressStr.Ptr, &theClientIPAddressStr.Len);
	if (err != QTSS_NoErr) return false;
	
	return AcceptAddress(theClientIPAddressStr.Ptr);	
}

Bool16 StillFlushing(QTSS_Filter_Params* inParams,Bool16 flushing)
{	

	QTSS_Error err = QTSS_NoErr;
	if (flushing) 
	{	
		err = QTSS_Flush(inParams->inRTSPRequest);
		//printf("Flushing session=%lu QTSS_Flush err =%ld\n",sSessID,err);	
	}
	if (err == QTSS_WouldBlock) // more to flush later
	{	
		sFlushing = true;
		(void) QTSS_SetValue(inParams->inRTSPRequest, sFlushingID, 0, (void*)&sFlushing, sFlushingLen);
		err = QTSS_RequestEvent(inParams->inRTSPRequest, QTSS_WriteableEvent);
		KeepSession(inParams->inRTSPRequest,true);
		//printf("Flushing session=%lu QTSS_RequestEvent err =%ld\n",sSessID,err);
	}
	else 
	{
		sFlushing = false;
		(void) QTSS_SetValue(inParams->inRTSPRequest, sFlushingID, 0, (void*)&sFlushing, sFlushingLen);
		KeepSession(inParams->inRTSPRequest,false);
	
		if (flushing) // we were flushing so reset the LastRequestTime
		{	
			sLastRequestTime = QTSS_Milliseconds();
			//printf("Done Flushing session=%lu\n",sSessID);
			return true;
		}
	}
	
	return sFlushing;
}

Bool16 IsAuthentic(QTSS_Filter_Params* inParams,StringParser *fullRequestPtr)
{	//printf("sAuthenticationEnabled \n");
	Bool16 isAuthentic = false;

	if (!sAuthenticationEnabled)
	{	isAuthentic = true;
		 (void) QTSS_SetValue(inParams->inRTSPRequest, sAuthenticatedID, 0, (void*)&isAuthentic, sizeof(isAuthentic));
		return isAuthentic;
	}
	
	StrPtrLen authenticateName;
	StrPtrLen authenticatePassword;
	Bool16 hasAuthentication = HasAuthentication(fullRequestPtr,&authenticateName,&authenticatePassword);
	if (hasAuthentication) 
	{	//printf("hasAuthentication\n");
		isAuthentic = Authenticate(inParams->inRTSPRequest, &authenticateName,&authenticatePassword);
	}
	
	(void) QTSS_SetValue(inParams->inRTSPRequest, sAuthenticatedID, 0, (void*)&isAuthentic, sizeof(isAuthentic));

	return isAuthentic;
}

inline Bool16 InWaitInterval(QTSS_Filter_Params* inParams)
{
	QTSS_TimeVal nextExecuteTime = sLastRequestTime + sRequestTimeIntervalMilli;
	QTSS_TimeVal currentTime = QTSS_Milliseconds();
	SInt32 waitTime = 0;
	if (currentTime < nextExecuteTime)
	{	
		waitTime = (SInt32) (nextExecuteTime - currentTime) + 1;
		//printf("(currentTime < nextExecuteTime) sSessID = %lu waitTime =%ld currentTime = %qd nextExecute = %qd interval=%lu\n",sSessID, waitTime, currentTime, nextExecuteTime,sRequestTimeIntervalMilli);
		(void)QTSS_SetIdleTimer(waitTime);
		KeepSession(inParams->inRTSPRequest,true);
		
		//printf("-- call me again after %ld millisecs session=%lu \n",waitTime,sSessID);
		return true;
	}
	sLastRequestTime = QTSS_Milliseconds();
	//printf("handle sessID=%lu time=%qd \n",sSessID,currentTime);
	return false;
}

inline void GetQueryData(QTSS_RTSPRequestObject theRequest)
{
	sAdminPtr = NEW AdminClass();
	Assert(sAdminPtr != NULL);
	if (sAdminPtr == NULL) 
	{	//printf ("NEW AdminClass() failed!! \n");
		return;
	}
	if (sAdminPtr != NULL) 
	{
		sAdminPtr->Initialize(&sQTSSparams,sQueryPtr);	// Get theData
	}
}

inline void SendHeader(QTSS_StreamRef inStream)
{
	(void)QTSS_Write(inStream, sResponseHeader, ::strlen(sResponseHeader), NULL, 0);
	(void)QTSS_Write(inStream, sEOL, ::strlen(sEOL), NULL, 0);				
	(void)QTSS_Write(inStream, sVersionHeader, ::strlen(sVersionHeader), NULL, 0);		
	(void)QTSS_Write(inStream, sEOL, ::strlen(sEOL), NULL, 0);				
	(void)QTSS_Write(inStream, sConnectionHeader, ::strlen(sConnectionHeader), NULL, 0);		
	(void)QTSS_Write(inStream, sEOL, ::strlen(sEOL), NULL, 0);		
	(void)QTSS_Write(inStream, sContentType, ::strlen(sContentType), NULL, 0);		
	(void)QTSS_Write(inStream, sEOM, ::strlen(sEOM), NULL, 0);		
}

inline void SendResult(QTSS_StreamRef inStream)
{
	SendHeader(inStream);		
	if (sAdminPtr != NULL)
		sAdminPtr->RespondToQuery(inStream,sQueryPtr,sQueryPtr->GetRootID());
		
}

inline Bool16 GetRequestAuthenticatedState(QTSS_Filter_Params* inParams) 
{
        Bool16 result = false;
	UInt32 paramLen = sizeof(result);
	QTSS_Error err = QTSS_GetValue(inParams->inRTSPRequest, sAuthenticatedID, 0, (void*)&result, &paramLen);
	if(err != QTSS_NoErr)
	{
	       paramLen = sizeof(result);
	       result = false;
	       err =QTSS_SetValue(inParams->inRTSPRequest, sAuthenticatedID, 0, (void*)&result, paramLen);
	}	  
	return result;
}

inline Bool16 GetRequestFlushState(QTSS_Filter_Params* inParams)
{	Bool16 result = false;
	UInt32 paramLen = sizeof(result);
	QTSS_Error err = QTSS_GetValue(inParams->inRTSPRequest, sFlushingID, 0, (void*)&result, &paramLen);
	if (err != QTSS_NoErr)
	{	paramLen = sizeof(result);
		result = false;
		//printf("no flush val so set to false session=%lu err =%ld\n",sSessID, err);
		err =QTSS_SetValue(inParams->inRTSPRequest, sFlushingID, 0, (void*)&result, paramLen);
		//printf("QTSS_SetValue flush session=%lu err =%ld\n",sSessID, err);
	}
	return result;
}

QTSS_Error FilterRequest(QTSS_Filter_Params* inParams)
{
	if (NULL == inParams || NULL == inParams->inRTSPSession || NULL == inParams->inRTSPRequest)
	{	Assert(0);
		return QTSS_NoErr;
	}

	OSMutexLocker locker(sAdminMutex);
	//check to see if we should handle this request. Invokation is triggered
	//by a "GET /" request
	
	QTSS_Error err = QTSS_NoErr;
	QTSS_RTSPRequestObject theRequest = inParams->inRTSPRequest;

	UInt32 paramLen = sizeof(sSessID);
	err = QTSS_GetValue(inParams->inRTSPSession, qtssRTSPSesID, 0, (void*)&sSessID, &paramLen);		
	if (err != QTSS_NoErr) 
		return QTSS_NoErr;

	StrPtrLen theFullRequest;
	err = QTSS_GetValuePtr(theRequest, qtssRTSPReqFullRequest, 0, (void**)&theFullRequest.Ptr, &theFullRequest.Len);
	if (err != QTSS_NoErr) 
		return QTSS_NoErr;
		
	
	StringParser fullRequest(&theFullRequest);
		
	if ( !IsAdminRequest(&fullRequest) ) 
		return QTSS_NoErr;
		
	if ( !AcceptSession(inParams->inRTSPSession) )
	{	(void)QTSS_Write(inParams->inRTSPRequest, sPermissionDeniedHeader, ::strlen(sPermissionDeniedHeader), NULL, 0);		
		(void)QTSS_Write(inParams->inRTSPRequest, sHTMLBody, ::strlen(sHTMLBody), NULL, 0);
		KeepSession(theRequest,false);
		return QTSS_NoErr;
	}
	
	if(!GetRequestAuthenticatedState(inParams)) // must authenticate before handling
	{
		if(QTSS_IsGlobalLocked()) // must NOT be global locked
	     	return QTSS_RequestFailed;
	     	
		if (!IsAuthentic(inParams,&fullRequest)) 
		{	
			(void)QTSS_Write(inParams->inRTSPRequest, sUnauthorizedResponseHeader, ::strlen(sUnauthorizedResponseHeader), NULL, 0);		
			(void)QTSS_Write(inParams->inRTSPRequest, sHTMLBody, ::strlen(sHTMLBody), NULL, 0);
			KeepSession(theRequest,false);
			return QTSS_NoErr;
		}
	}
	
	if (true == GetRequestFlushState(inParams)) 
	{	StillFlushing(inParams,true);
		return QTSS_NoErr;
	}
		
	if (!QTSS_IsGlobalLocked())
	{		
		if (InWaitInterval(inParams)) 
			return QTSS_NoErr; 

		//printf("New Request Wait for GlobalLock session=%lu\n",sSessID);
		(void)QTSS_RequestGlobalLock();
		KeepSession(theRequest,true);
		return QTSS_NoErr; 
	}
	
	//printf("Handle request session=%lu\n",sSessID);
	APITests_DEBUG();
	
	if (sQueryPtr != NULL) 
	{	delete sQueryPtr;
		sQueryPtr = NULL;	
	}
	sQueryPtr = NEW QueryURI(&theFullRequest);
	if (sQueryPtr == NULL) return QTSS_NoErr;
	
	ShowQuery_DEBUG();
	
	if (sAdminPtr != NULL) 
	{	delete sAdminPtr;
		sAdminPtr = NULL;
	}
	UInt32 result = sQueryPtr->EvalQuery(NULL, NULL);
	if (result == 0) do
	{
		if( ElementNode_CountPtrs() > 0)
		{	ElementNode_ShowPtrs();
			Assert(0);
		}
			
		GetQueryData(theRequest);
		
		SendResult(theRequest);	
		delete sAdminPtr;
		sAdminPtr = NULL;
		
		if (sQueryPtr && !sQueryPtr->QueryHasReponse())
		{	UInt32 err = 404;
			(void) sQueryPtr->EvalQuery(&err,NULL);
			ReportErr(inParams, err);
			break;
		}

		if (sQueryPtr && sQueryPtr->QueryHasReponse())
		{	ReportErr(inParams, sQueryPtr->GetEvaluResult());
		}
		
		if (sQueryPtr->fIsPref && sQueryPtr->GetEvaluResult() == 0)
		{	QTSS_ServiceID id;
			(void) QTSS_IDForService(QTSS_REREAD_PREFS_SERVICE, &id);			
			(void) QTSS_DoService(id, NULL);
		}
	} while(false);
	else
	{
		SendHeader(theRequest);			
		ReportErr(inParams, sQueryPtr->GetEvaluResult());
	}
	
	if (sQueryPtr != NULL) 
	{	delete sQueryPtr;
		sQueryPtr = NULL;
	}
	
	(void) StillFlushing(inParams,true);
	return QTSS_NoErr;

}




