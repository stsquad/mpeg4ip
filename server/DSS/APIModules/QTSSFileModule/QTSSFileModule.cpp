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
	File:		QTSSFileModule.cpp

	Contains:	Implementation of module described in QTSSFileModule.h. 
					


*/

#include <string.h>

#include "QTSSFileModule.h"

#include "QTRTPFile.h"
#include "QTFile.h"
#include "OSMemory.h"
#include "OSArrayObjectDeleter.h"

#include "SDPSourceInfo.h"
#include "StringFormatter.h"
#include "QTSSModuleUtils.h"

#include <errno.h>

#include "QTSS.h"

class FileSession
{
	public:
	
		FileSession() : fAdjustedPlayTime(0), fNextPacketLen(0), fLastQualityCheck(0),
						fDeleteSDP(false), fAllowNegativeTTs(false), fSpeed(1),
						fStartTime(-1), fStopTime(-1), fStopTrackID(0), fStopPN(0)
		{}
		
		~FileSession() {}
		
		QTRTPFile 			fFile;
		SInt64				fAdjustedPlayTime;
		QTSS_PacketStruct	fPacketStruct;
		int					fNextPacketLen;
		SInt64				fLastQualityCheck;
		SDPSourceInfo		fSDPSource;
		Bool16				fDeleteSDP;
		Bool16				fAllowNegativeTTs;
		Float32 			fSpeed;
		Float64 			fStartTime;
		Float64 			fStopTime;
		
		UInt32				fStopTrackID;
		UInt64				fStopPN;
};

// ref to the prefs dictionary object
static QTSS_ModulePrefsObject 		sPrefs;
static QTSS_PrefsObject 			sServerPrefs;

static StrPtrLen sSDPSuffix(".sdp");
static StrPtrLen sSDPHeader1("v=0\r\ns=");
static StrPtrLen sSDPHeader2;
static StrPtrLen sSDPHeader3("c=IN IP4 ");
static StrPtrLen sSDPHeader4("\r\na=control:*\r\n");

// ATTRIBUTES IDs

static QTSS_AttributeID sFileSessionAttr				= qtssIllegalAttrID;

static QTSS_AttributeID	sSeekToNonexistentTimeErr		= qtssIllegalAttrID;
static QTSS_AttributeID	sNoSDPFileFoundErr				= qtssIllegalAttrID;
static QTSS_AttributeID	sBadQTFileErr					= qtssIllegalAttrID;
static QTSS_AttributeID	sFileIsNotHintedErr				= qtssIllegalAttrID;
static QTSS_AttributeID	sExpectedDigitFilenameErr		= qtssIllegalAttrID;
static QTSS_AttributeID	sTrackDoesntExistErr			= qtssIllegalAttrID;

// OTHER DATA

static UInt32				sFlowControlProbeInterval	= 100;
static UInt32				sDefaultFlowControlProbeInterval= 100;
static Float32				sMaxAllowedSpeed			= 4;
static Float32				sDefaultMaxAllowedSpeed		= 4;

static const StrPtrLen 				kCacheControlHeader("must-revalidate");
static const QTSS_RTSPStatusCode	kNotModifiedStatus 			= qtssRedirectNotModified;
// FUNCTIONS

static QTSS_Error QTSSFileModuleDispatch(QTSS_Role inRole, QTSS_RoleParamPtr inParamBlock);
static QTSS_Error 	Register(QTSS_Register_Params* inParams);
static QTSS_Error Initialize(QTSS_Initialize_Params* inParamBlock);
static QTSS_Error RereadPrefs();
static QTSS_Error ProcessRTSPRequest(QTSS_StandardRTSP_Params* inParamBlock);
static QTSS_Error DoDescribe(QTSS_StandardRTSP_Params* inParamBlock);
static QTSS_Error CreateQTRTPFile(QTSS_StandardRTSP_Params* inParamBlock, char* inPath, FileSession** outFile);
static QTSS_Error DoSetup(QTSS_StandardRTSP_Params* inParamBlock);
static QTSS_Error DoPlay(QTSS_StandardRTSP_Params* inParamBlock);
static QTSS_Error SendPackets(QTSS_RTPSendPackets_Params* inParams);
static QTSS_Error DestroySession(QTSS_ClientSessionClosing_Params* inParams);
static void			DeleteFileSession(FileSession* inFileSession);



QTSS_Error QTSSFileModule_Main(void* inPrivateArgs)
{
	return _stublibrary_main(inPrivateArgs, QTSSFileModuleDispatch);
}

QTSS_Error 	QTSSFileModuleDispatch(QTSS_Role inRole, QTSS_RoleParamPtr inParamBlock)
{
	switch (inRole)
	{
		case QTSS_Register_Role:
			return Register(&inParamBlock->regParams);
		case QTSS_Initialize_Role:
			return Initialize(&inParamBlock->initParams);
		case QTSS_RereadPrefs_Role:
			return RereadPrefs();
		case QTSS_RTSPRequest_Role:
			return ProcessRTSPRequest(&inParamBlock->rtspRequestParams);
		case QTSS_RTPSendPackets_Role:
			return SendPackets(&inParamBlock->rtpSendPacketsParams);
		case QTSS_ClientSessionClosing_Role:
			return DestroySession(&inParamBlock->clientSessionClosingParams);
	}
	return QTSS_NoErr;
}

QTSS_Error Register(QTSS_Register_Params* inParams)
{
	// Register for roles
	(void)QTSS_AddRole(QTSS_Initialize_Role);
	(void)QTSS_AddRole(QTSS_RTSPRequest_Role);
	(void)QTSS_AddRole(QTSS_ClientSessionClosing_Role);
	(void)QTSS_AddRole(QTSS_RereadPrefs_Role);

	// Add text messages attributes
	static char*		sSeekToNonexistentTimeName	= "QTSSFileModuleSeekToNonExistentTime";
	static char*		sNoSDPFileFoundName			= "QTSSFileModuleNoSDPFileFound";
	static char*		sBadQTFileName				= "QTSSFileModuleBadQTFile";
	static char*		sFileIsNotHintedName		= "QTSSFileModuleFileIsNotHinted";
	static char*		sExpectedDigitFilenameName	= "QTSSFileModuleExpectedDigitFilename";
	static char*		sTrackDoesntExistName		= "QTSSFileModuleTrackDoesntExist";
	
	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sSeekToNonexistentTimeName, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sSeekToNonexistentTimeName, &sSeekToNonexistentTimeErr);

	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sNoSDPFileFoundName, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sNoSDPFileFoundName, &sNoSDPFileFoundErr);

	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sBadQTFileName, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sBadQTFileName, &sBadQTFileErr);

	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sFileIsNotHintedName, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sFileIsNotHintedName, &sFileIsNotHintedErr);

	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sExpectedDigitFilenameName, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sExpectedDigitFilenameName, &sExpectedDigitFilenameErr);

	(void)QTSS_AddStaticAttribute(qtssTextMessagesObjectType, sTrackDoesntExistName, NULL, qtssAttrDataTypeCharArray);
	(void)QTSS_IDForAttr(qtssTextMessagesObjectType, sTrackDoesntExistName, &sTrackDoesntExistErr);
	
	// Add an RTP session attribute for tracking FileSession objects
	static char*		sFileSessionName	= "QTSSFileModuleSession";

	(void)QTSS_AddStaticAttribute(qtssClientSessionObjectType, sFileSessionName, NULL, qtssAttrDataTypeVoidPointer);
	(void)QTSS_IDForAttr(qtssClientSessionObjectType, sFileSessionName, &sFileSessionAttr);
	
	// Tell the server our name!
	static char* sModuleName = "QTSSFileModule";
	::strcpy(inParams->outModuleName, sModuleName);

	return QTSS_NoErr;
}

QTSS_Error Initialize(QTSS_Initialize_Params* inParams)
{
	QTSSModuleUtils::Initialize(inParams->inMessages, inParams->inServer, inParams->inErrorLogStream);
	sPrefs = QTSSModuleUtils::GetModulePrefsObject(inParams->inModule);
	sServerPrefs = inParams->inPrefs;
	QTRTPFile::Initialize();

	static StrPtrLen sEHeader("\r\ne=");
	static StrPtrLen sUHeader("\r\nu=");
	static StrPtrLen sHTTP("http://");
	static StrPtrLen sAdmin("admin@");
	
	QTSS_Object theServer = inParams->inServer;
	
	// Read our preferences
	RereadPrefs();
	
	//build the sdp that looks like: \r\ne=http://streaming.apple.com\r\ne=qts@apple.com\r\n.

	// Get the default DNS name of the server
	StrPtrLen theDefaultDNS;
	(void)QTSS_GetValuePtr(theServer, qtssSvrDefaultDNSName, 0,
									(void**)&theDefaultDNS.Ptr, &theDefaultDNS.Len);
	
	StrPtrLen sdpURL;
	StrPtrLen adminEmail;
	sdpURL.Ptr = QTSSModuleUtils::GetStringPref(sPrefs, "sdp_url", "");
	sdpURL.Len = ::strlen(sdpURL.Ptr);
	
	adminEmail.Ptr = QTSSModuleUtils::GetStringPref(sPrefs, "admin_email", "");
	adminEmail.Len = ::strlen(adminEmail.Ptr);
	
	UInt32 sdpURLLen = sdpURL.Len;
	UInt32 adminEmailLen = adminEmail.Len;
	
	if (sdpURLLen == 0)
		sdpURLLen = theDefaultDNS.Len + sHTTP.Len + 1;
	if (adminEmailLen == 0)
		adminEmailLen = theDefaultDNS.Len + sAdmin.Len;	
	
	//calculate the length of the string & allocate memory
	sSDPHeader2.Len = (sEHeader.Len * 2) + sdpURLLen + adminEmailLen + 10;
	sSDPHeader2.Ptr = NEW char[sSDPHeader2.Len];

	//write it!
	StringFormatter sdpFormatter(sSDPHeader2);
	sdpFormatter.Put(sUHeader);

	//if there are preferences for default URL & admin email, use those. Otherwise, build the
	//proper string based on default dns name.
	if (sdpURL.Len == 0)
	{
		sdpFormatter.Put(sHTTP);
		sdpFormatter.Put(theDefaultDNS);
		sdpFormatter.PutChar('/');
	}
	else
		sdpFormatter.Put(sdpURL);
	
	sdpFormatter.Put(sEHeader);
	
	//now do the admin email.
	if (adminEmail.Len == 0)
	{
		sdpFormatter.Put(sAdmin);
		sdpFormatter.Put(theDefaultDNS);
	}
	else
		sdpFormatter.Put(adminEmail);
		
	sdpFormatter.PutEOL();
	sSDPHeader2.Len = (UInt32)sdpFormatter.GetCurrentOffset();
	
	delete [] sdpURL.Ptr;
	delete [] adminEmail.Ptr;
	
	// Report to the server that this module handles DESCRIBE, SETUP, PLAY, PAUSE, and TEARDOWN
	static QTSS_RTSPMethod sSupportedMethods[] = { qtssDescribeMethod, qtssSetupMethod, qtssTeardownMethod, qtssPlayMethod, qtssPauseMethod };
	QTSSModuleUtils::SetupSupportedMethods(inParams->inServer, sSupportedMethods, 5);

	return QTSS_NoErr;
}

QTSS_Error RereadPrefs()
{
	QTSSModuleUtils::GetPref(sPrefs, "flow_control_probe_interval", 	qtssAttrDataTypeUInt32,
								&sFlowControlProbeInterval, &sDefaultFlowControlProbeInterval, sizeof(sFlowControlProbeInterval));

	QTSSModuleUtils::GetPref(sPrefs, "max_allowed_speed", 	qtssAttrDataTypeFloat32,
								&sMaxAllowedSpeed, &sDefaultMaxAllowedSpeed, sizeof(sMaxAllowedSpeed));

	return QTSS_NoErr;
}

QTSS_Error ProcessRTSPRequest(QTSS_StandardRTSP_Params* inParamBlock)
{
	QTSS_RTSPMethod* theMethod = NULL;
	UInt32 theMethodLen = 0;
	if ((QTSS_GetValuePtr(inParamBlock->inRTSPRequest, qtssRTSPReqMethod, 0,
			(void**)&theMethod, &theMethodLen) != QTSS_NoErr) || (theMethodLen != sizeof(QTSS_RTSPMethod)))
	{
		Assert(0);
		return QTSS_RequestFailed;
	}
	
	QTSS_Error err = QTSS_NoErr;
	switch (*theMethod)
	{
		case qtssDescribeMethod:
			err = DoDescribe(inParamBlock);
			break;
		case qtssSetupMethod:
			err = DoSetup(inParamBlock);
			break;
		case qtssPlayMethod:
			err = DoPlay(inParamBlock);
			break;
		case qtssTeardownMethod:
			(void)QTSS_Teardown(inParamBlock->inClientSession);
			(void)QTSS_SendStandardRTSPResponse(inParamBlock->inRTSPRequest, inParamBlock->inClientSession, 0);
			break;
		case qtssPauseMethod:
			(void)QTSS_Pause(inParamBlock->inClientSession);
			(void)QTSS_SendStandardRTSPResponse(inParamBlock->inRTSPRequest, inParamBlock->inClientSession, 0);
			break;
		default:
			break;
	}
	if (err != QTSS_NoErr)
		(void)QTSS_Teardown(inParamBlock->inClientSession);

	return QTSS_NoErr;
}

Bool16 isSDP(QTSS_StandardRTSP_Params* inParamBlock)
{
	char* path = NULL;
	UInt32 len = 0;
	QTSS_Error theErr = QTSS_GetValuePtr(inParamBlock->inRTSPRequest, qtssRTSPReqLocalPath, 0, (void**)&path, &len);
	Assert(theErr == QTSS_NoErr);

	if (sSDPSuffix.Len > len)
		return false;
		
	StrPtrLen thePath(&path[len - sSDPSuffix.Len],sSDPSuffix.Len);
	return thePath.Equal(sSDPSuffix);
}

QTSS_Error DoDescribe(QTSS_StandardRTSP_Params* inParamBlock)
{
	if (isSDP(inParamBlock))
		return QTSSModuleUtils::SendErrorResponse(inParamBlock->inRTSPRequest, qtssClientNotFound, sNoSDPFileFoundErr);

	//
	// Get the FileSession for this DESCRIBE, if any.
	UInt32 theLen = sizeof(FileSession*);
	FileSession* 	theFile = NULL;
	QTSS_Error		theErr = QTSS_NoErr;

	(void)QTSS_GetValue(inParamBlock->inClientSession, sFileSessionAttr, 0, (void*)&theFile, &theLen);

	// Generate the complete file path
	UInt32 thePathLen = 0;
	OSCharArrayDeleter thePath(QTSSModuleUtils::GetFullPath(inParamBlock->inRTSPRequest, qtssRTSPReqFilePath,
																						&thePathLen, &sSDPSuffix));
		
	//first locate the target movie
	thePath.GetObject()[thePathLen - sSDPSuffix.Len] = '\0';//truncate the .sdp
	
	if ( theFile != NULL )	
	{
		//
		// There is already a file for this session. This can happen if there are multiple DESCRIBES,
		// or a DESCRIBE has been issued with a Session ID, or some such thing.
		StrPtrLen	moviePath( theFile->fFile.GetMoviePath() );
		StrPtrLen	requestPath( thePath.GetObject() );
		
		//
		// This describe is for a different file. Delete the old FileSession.
		if ( !requestPath.Equal( moviePath ) )
		{
			DeleteFileSession(theFile);
			theFile = NULL;
			
			// NULL out the attribute value, just in case.
			(void)QTSS_SetValue(inParamBlock->inClientSession, sFileSessionAttr, 0, &theFile, sizeof(theFile));
		}
	}

	if ( theFile == NULL )
	{	
		theErr = CreateQTRTPFile(inParamBlock, thePath.GetObject(), &theFile);
		if (theErr != QTSS_NoErr)
			return theErr;
	
		// Store this newly created file object in the RTP session.
		theErr = QTSS_SetValue(inParamBlock->inClientSession, sFileSessionAttr, 0, &theFile, sizeof(theFile));
	}
	
	//replace the sacred character we have trodden on in order to truncate the path.
	thePath.GetObject()[thePathLen - sSDPSuffix.Len] = sSDPSuffix.Ptr[0];


	iovec theSDPVec[10];//1 for the RTSP header, 6 for the sdp header, 1 for the sdp body
	::memset(&theSDPVec[0], 0, sizeof(theSDPVec));
	
	// Check to see if there is an sdp file, if so, return that file instead
	// of the built-in sdp.
	StrPtrLen theSDPData;
	(void)QTSSModuleUtils::ReadEntireFile(thePath.GetObject(), &theSDPData);
	
	if (theSDPData.Len > 0)
	{
		//Now that we have the file data, send an appropriate describe
		//response to the client
		theFile->fDeleteSDP = true;
		theSDPVec[1].iov_base = theSDPData.Ptr;
		theSDPVec[1].iov_len = theSDPData.Len;
		
		QTSSModuleUtils::SendDescribeResponse(inParamBlock->inRTSPRequest, inParamBlock->inClientSession,
																&theSDPVec[0], 3, theSDPData.Len);	
	}
	else
	{
		// Before generating the SDP and sending it, check to see if there is an If-Modified-Since
		// date. If there is, and the content hasn't been modified, then just return a 304 Not Modified
		QTSS_TimeVal* theTime = NULL;
		(void) QTSS_GetValuePtr(inParamBlock->inRTSPRequest, qtssRTSPReqIfModSinceDate, 0, (void**)&theTime, &theLen);
		if ((theLen == sizeof(QTSS_TimeVal)) && (*theTime > 0))
		{
			// There is an If-Modified-Since header. Check it vs. the content.
			if (*theTime == theFile->fFile.GetQTFile()->GetModDate())
			{
				theErr = QTSS_SetValue( inParamBlock->inRTSPRequest, qtssRTSPReqStatusCode, 0,
										&kNotModifiedStatus, sizeof(kNotModifiedStatus) );
				Assert(theErr == QTSS_NoErr);
				// Because we are using this call to generate a 304 Not Modified response, we do not need
				// to pass in a RTP Stream
				theErr = QTSS_SendStandardRTSPResponse(inParamBlock->inRTSPRequest, inParamBlock->inClientSession, 0);
				Assert(theErr == QTSS_NoErr);
				return QTSS_NoErr;
			}
		}
		
		//if there is no sdp file on disk, let's generate one.
		UInt32 totalSDPLength = sSDPHeader1.Len;
		theSDPVec[1].iov_base = sSDPHeader1.Ptr;
		theSDPVec[1].iov_len = sSDPHeader1.Len;
		
		//filename goes here
		(void)QTSS_GetValuePtr(inParamBlock->inRTSPRequest, qtssRTSPReqFilePath, 0,
													(void**)&theSDPVec[2].iov_base, (UInt32*)&theSDPVec[2].iov_len);
		totalSDPLength += theSDPVec[2].iov_len;
		
		//url & admin email goes here
		theSDPVec[3].iov_base = sSDPHeader2.Ptr;
		theSDPVec[3].iov_len = sSDPHeader2.Len;
		totalSDPLength += sSDPHeader2.Len;

		//connection header
		theSDPVec[4].iov_base = sSDPHeader3.Ptr;
		theSDPVec[4].iov_len = sSDPHeader3.Len;
		totalSDPLength += sSDPHeader3.Len;
		
		//append IP addr
		(void)QTSS_GetValuePtr(inParamBlock->inRTSPSession, qtssRTSPSesLocalAddrStr, 0,
													(void**)&theSDPVec[5].iov_base, (UInt32*)&theSDPVec[5].iov_len);
		totalSDPLength += theSDPVec[5].iov_len;

		//last static sdp line
		theSDPVec[6].iov_base = sSDPHeader4.Ptr;
		theSDPVec[6].iov_len = sSDPHeader4.Len;
		totalSDPLength += sSDPHeader4.Len;
		
		//now append content-determined sdp ( cached in QTRTPFile )
		int theSDPBodyLen = 0;
		theSDPVec[7].iov_base = theSDPData.Ptr = theFile->fFile.GetSDPFile(&theSDPBodyLen);
		theSDPData.Len = theSDPVec[7].iov_len = theSDPBodyLen;
		totalSDPLength += theSDPBodyLen;
		
		//static StrPtrLen sBufDelayStr("a=x-bufferdelay:8");
		//theSDPVec[8].iov_base = sBufDelayStr.Ptr;
		//theSDPVec[8].iov_len = sBufDelayStr.Len;
		//totalSDPLength += sBufDelayStr.Len;

		Assert(theSDPBodyLen > 0);
		Assert(theSDPVec[2].iov_base != NULL);
		//ok, we have a filled out iovec. Let's send the response!
		
		// Append the Last Modified header to be a good caching proxy citizen before sending the Describe
		(void)QTSS_AppendRTSPHeader(inParamBlock->inRTSPRequest, qtssLastModifiedHeader,
										theFile->fFile.GetQTFile()->GetModDateStr(), DateBuffer::kDateBufferLen);
		(void)QTSS_AppendRTSPHeader(inParamBlock->inRTSPRequest, qtssCacheControlHeader,
										kCacheControlHeader.Ptr, kCacheControlHeader.Len);
		QTSSModuleUtils::SendDescribeResponse(inParamBlock->inRTSPRequest, inParamBlock->inClientSession,
																		&theSDPVec[0], 8, totalSDPLength);	
	}
	
	Assert(theSDPData.Ptr != NULL);
	Assert(theSDPData.Len > 0);
	
	//now parse the sdp. We need to do this in order to extract payload information.
	//The SDP parser object will not take responsibility of the memory (one exception... see above)
		theFile->fSDPSource.Parse(theSDPData.Ptr, theSDPData.Len);
	return QTSS_NoErr;
}

QTSS_Error CreateQTRTPFile(QTSS_StandardRTSP_Params* inParamBlock, char* inPath, FileSession** outFile)
{	
	*outFile = NEW FileSession();
	QTRTPFile::ErrorCode theErr = (*outFile)->fFile.Initialize(inPath);
	if (theErr != QTRTPFile::errNoError)
	{
		delete *outFile;
		*outFile = NULL;
		
		if (theErr == QTRTPFile::errFileNotFound)
			return QTSSModuleUtils::SendErrorResponse(	inParamBlock->inRTSPRequest,
														qtssClientNotFound,
														sNoSDPFileFoundErr);
		if (theErr == QTRTPFile::errInvalidQuickTimeFile)
			return QTSSModuleUtils::SendErrorResponse(	inParamBlock->inRTSPRequest,
														qtssUnsupportedMediaType,
														sBadQTFileErr);
		if (theErr == QTRTPFile::errNoHintTracks)
			return QTSSModuleUtils::SendErrorResponse(	inParamBlock->inRTSPRequest,
														qtssUnsupportedMediaType,
														sFileIsNotHintedErr);
		if (theErr == QTRTPFile::errInternalError)
			return QTSSModuleUtils::SendErrorResponse(	inParamBlock->inRTSPRequest,
														qtssServerInternal,

														sBadQTFileErr);


		AssertV(0, theErr);
	}
	
	return QTSS_NoErr;
}


QTSS_Error DoSetup(QTSS_StandardRTSP_Params* inParamBlock)
{

	if (isSDP(inParamBlock))
		return QTSSModuleUtils::SendErrorResponse(inParamBlock->inRTSPRequest, qtssClientNotFound, sNoSDPFileFoundErr);

	//setup this track in the file object 
	FileSession* theFile = NULL;
	UInt32 theLen = sizeof(FileSession*);
	QTSS_Error theErr = QTSS_GetValue(inParamBlock->inClientSession, sFileSessionAttr, 0, (void*)&theFile, &theLen);
	if ((theErr != QTSS_NoErr) || (theLen != sizeof(FileSession*)))
	{
		char* theFullPath = NULL;
		theErr = QTSS_GetValuePtr(inParamBlock->inRTSPRequest, qtssRTSPReqLocalPath, 0, (void**)&theFullPath, &theLen);
		Assert(theErr == QTSS_NoErr);
		// This is possible, as clients are not required to send a DESCRIBE. If we haven't set
		// anything up yet, set everything up
		theErr = CreateQTRTPFile(inParamBlock, theFullPath, &theFile);
		if (theErr != QTSS_NoErr)
			return theErr;

		int theSDPBodyLen = 0;
		char* theSDPData = theFile->fFile.GetSDPFile(&theSDPBodyLen);

		//now parse the sdp. We need to do this in order to extract payload information.
		//The SDP parser object will not take responsibility of the memory (one exception... see above)
		theFile->fSDPSource.Parse(theSDPData, theSDPBodyLen);

		// Store this newly created file object in the RTP session.
		theErr = QTSS_SetValue(inParamBlock->inClientSession, sFileSessionAttr, 0, &theFile, sizeof(theFile));
	}


	//unless there is a digit at the end of this path (representing trackID), don't
	//even bother with the request
	StrPtrLen theDigit;
	(void)QTSS_GetValuePtr(inParamBlock->inRTSPRequest, qtssRTSPReqFileDigit, 0, (void**)&theDigit.Ptr, &theDigit.Len);
	if (theDigit.Len == 0)
		return QTSSModuleUtils::SendErrorResponse(inParamBlock->inRTSPRequest,
													qtssClientBadRequest, sExpectedDigitFilenameErr);
	UInt32 theTrackID = ::strtol(theDigit.Ptr, NULL, 10);

	QTRTPFile::ErrorCode qtfileErr = theFile->fFile.AddTrack(theTrackID, true);
	
	//if we get an error back, forward that error to the client
	if (qtfileErr == QTRTPFile::errTrackIDNotFound)
		return QTSSModuleUtils::SendErrorResponse(inParamBlock->inRTSPRequest,
													qtssClientNotFound, sTrackDoesntExistErr);
	else if (qtfileErr != QTRTPFile::errNoError)
		return QTSSModuleUtils::SendErrorResponse(inParamBlock->inRTSPRequest,
													qtssUnsupportedMediaType, sBadQTFileErr);

	// Before setting up this track, check to see if there is an If-Modified-Since
	// date. If there is, and the content hasn't been modified, then just return a 304 Not Modified
	QTSS_TimeVal* theTime = NULL;
	(void) QTSS_GetValuePtr(inParamBlock->inRTSPRequest, qtssRTSPReqIfModSinceDate, 0, (void**)&theTime, &theLen);
	if ((theLen == sizeof(QTSS_TimeVal)) && (*theTime > 0))
	{
		// There is an If-Modified-Since header. Check it vs. the content.
		if (*theTime == theFile->fFile.GetQTFile()->GetModDate())
		{
			theErr = QTSS_SetValue( inParamBlock->inRTSPRequest, qtssRTSPReqStatusCode, 0,
											&kNotModifiedStatus, sizeof(kNotModifiedStatus) );
			Assert(theErr == QTSS_NoErr);
			// Because we are using this call to generate a 304 Not Modified response, we do not need
			// to pass in a RTP Stream
			theErr = QTSS_SendStandardRTSPResponse(inParamBlock->inRTSPRequest, inParamBlock->inClientSession, 0);
			Assert(theErr == QTSS_NoErr);
			return QTSS_NoErr;
		}
	}

	//find the payload for this track ID (if applicable)
	StrPtrLen* thePayload = NULL;
	UInt32 thePayloadType = qtssUnknownPayloadType;
	Float32 bufferDelay = (Float32) 3.0; // FIXME need a constant defined for 3.0 value. It is used multiple places
	
	for (UInt32 x = 0; x < theFile->fSDPSource.GetNumStreams(); x++)
	{
		SourceInfo::StreamInfo* theStreamInfo = theFile->fSDPSource.GetStreamInfo(x);
		if (theStreamInfo->fTrackID == theTrackID)
		{
			thePayload = &theStreamInfo->fPayloadName;
			thePayloadType = theStreamInfo->fPayloadType;
			bufferDelay = theStreamInfo->fBufferDelay;
			break;
		}	
	}
	//Create a new RTP stream			
	QTSS_RTPStreamObject newStream = NULL;
	theErr = QTSS_AddRTPStream(inParamBlock->inClientSession, inParamBlock->inRTSPRequest, &newStream, 0);
	if (theErr != QTSS_NoErr)
		return theErr;
	
	// Set the payload type, payload name & timescale of this track
	SInt32 theTimescale = theFile->fFile.GetTrackTimeScale(theTrackID);
	
	theErr = QTSS_SetValue(newStream, qtssRTPStrBufferDelayInSecs, 0, &bufferDelay, sizeof(bufferDelay));
	Assert(theErr == QTSS_NoErr);
	theErr = QTSS_SetValue(newStream, qtssRTPStrPayloadName, 0, thePayload->Ptr, thePayload->Len);
	Assert(theErr == QTSS_NoErr);
	theErr = QTSS_SetValue(newStream, qtssRTPStrPayloadType, 0, &thePayloadType, sizeof(thePayloadType));
	Assert(theErr == QTSS_NoErr);
	theErr = QTSS_SetValue(newStream, qtssRTPStrTimescale, 0, &theTimescale, sizeof(theTimescale));
	Assert(theErr == QTSS_NoErr);
	theErr = QTSS_SetValue(newStream, qtssRTPStrTrackID, 0, &theTrackID, sizeof(theTrackID));
	Assert(theErr == QTSS_NoErr);
	
	// Set the number of quality levels. Allow up to 6
	static UInt32 sNumQualityLevels = 5;
	
	theErr = QTSS_SetValue(newStream, qtssRTPStrNumQualityLevels, 0, &sNumQualityLevels, sizeof(sNumQualityLevels));
	Assert(theErr == QTSS_NoErr);
	
	// Get the SSRC of this track
	UInt32* theTrackSSRC = NULL;
	UInt32 theTrackSSRCSize = 0;
	(void)QTSS_GetValuePtr(newStream, qtssRTPStrSSRC, 0, (void**)&theTrackSSRC, &theTrackSSRCSize);

	// The RTP stream should ALWAYS have an SSRC assuming QTSS_AddStream succeeded.
	Assert((theTrackSSRC != NULL) && (theTrackSSRCSize == sizeof(UInt32)));
	
	//give the file some info it needs.
	theFile->fFile.SetTrackSSRC(theTrackID, *theTrackSSRC);
	theFile->fFile.SetTrackCookies(theTrackID, newStream, thePayloadType);
	
	StrPtrLen theHeader;
	theErr = QTSS_GetValuePtr(inParamBlock->inRTSPHeaders, qtssXRTPMetaInfoHeader, 0, (void**)&theHeader.Ptr, &theHeader.Len);
	if (theErr == QTSS_NoErr)
	{
		//
		// If there is an x-RTP-Meta-Info header in the request, mirror that header in the
		// response. We will support any fields supported by the QTFileLib.
		RTPMetaInfoPacket::FieldID* theFields = NEW RTPMetaInfoPacket::FieldID[RTPMetaInfoPacket::kNumFields];
		::memcpy(theFields, QTRTPFile::GetSupportedRTPMetaInfoFields(), sizeof(RTPMetaInfoPacket::FieldID) * RTPMetaInfoPacket::kNumFields);

		//
		// This function does the work of appending the response header based on the
		// fields we support and the requested fields.
		theErr = QTSSModuleUtils::AppendRTPMetaInfoHeader(inParamBlock->inRTSPRequest, &theHeader, theFields);

		//
		// This returns QTSS_NoErr only if there are some valid, useful fields
		Bool16 isVideo = false;
		if (thePayloadType == qtssVideoPayloadType)
			isVideo = true;
		if (theErr == QTSS_NoErr)
			theFile->fFile.SetTrackRTPMetaInfo(theTrackID, theFields, isVideo);
	}
	
	//
	// Our array has now been updated to reflect the fields requested by the client.
	//send the setup response
	(void)QTSS_AppendRTSPHeader(inParamBlock->inRTSPRequest, qtssLastModifiedHeader,
								theFile->fFile.GetQTFile()->GetModDateStr(), DateBuffer::kDateBufferLen);
	(void)QTSS_AppendRTSPHeader(inParamBlock->inRTSPRequest, qtssCacheControlHeader,
								kCacheControlHeader.Ptr, kCacheControlHeader.Len);
	theErr = QTSS_SendStandardRTSPResponse(inParamBlock->inRTSPRequest, newStream, 0);
	Assert(theErr == QTSS_NoErr);
	return QTSS_NoErr;
}

QTSS_Error DoPlay(QTSS_StandardRTSP_Params* inParamBlock)
{
	if (isSDP(inParamBlock))
		return QTSSModuleUtils::SendErrorResponse(inParamBlock->inRTSPRequest, qtssClientNotFound, sNoSDPFileFoundErr);

	//Have the file seek to the proper time.
	FileSession** theFile = NULL;
	UInt32 theLen = 0;
	QTSS_Error theErr = QTSS_GetValuePtr(inParamBlock->inClientSession, sFileSessionAttr, 0, (void**)&theFile, &theLen);
	if ((theErr != QTSS_NoErr) || (theLen != sizeof(FileSession*)))
		return QTSS_RequestFailed;

	QTRTPFile::ErrorCode qtFileErr = QTRTPFile::errNoError;

	//
	// How much are we going to tell the client to back up?
	Float32 theBackupTime = 0;

	char* thePacketRangeHeader = NULL;
	theErr = QTSS_GetValuePtr(inParamBlock->inRTSPHeaders, qtssXPacketRangeHeader, 0, (void**)&thePacketRangeHeader, &theLen);
	if (theErr == QTSS_NoErr)
	{
		StrPtrLen theRangeHdrPtr(thePacketRangeHeader, theLen);
		StringParser theRangeParser(&theRangeHdrPtr);
		
		theRangeParser.ConsumeUntil(NULL, StringParser::sDigitMask);
		UInt64 theStartPN = theRangeParser.ConsumeInteger();
		
		theRangeParser.ConsumeUntil(NULL, StringParser::sDigitMask);
		(*theFile)->fStopPN = theRangeParser.ConsumeInteger();

		theRangeParser.ConsumeUntil(NULL, StringParser::sDigitMask);
		(*theFile)->fStopTrackID = theRangeParser.ConsumeInteger();

		qtFileErr = (*theFile)->fFile.SeekToPacketNumber((*theFile)->fStopTrackID, theStartPN);
		(*theFile)->fStartTime = (*theFile)->fFile.GetRequestedSeekTime();
	}
	else
	{
		Float64* theStartTimeP = 0;
		theErr = QTSS_GetValuePtr(inParamBlock->inRTSPRequest, qtssRTSPReqStartTime, 0, (void**)&theStartTimeP, &theLen);
		if ((theErr != QTSS_NoErr) || (theLen != sizeof(Float64)))
			return QTSSModuleUtils::SendErrorResponse(	inParamBlock->inRTSPRequest,
														qtssClientBadRequest, sSeekToNonexistentTimeErr);
														
		Float32* theMaxBackupTime = NULL;
		theErr = QTSS_GetValuePtr(inParamBlock->inRTSPRequest, qtssRTSPReqPrebufferMaxTime, 0, (void**)&theMaxBackupTime, &theLen);
		Assert(theMaxBackupTime != NULL);

		if (*theMaxBackupTime == -1)
		{
			//
			// If this is an old client (doesn't send the x-prebuffer header), maintain the old
			// behavior - back up to a key frame, and do not adjust the buffer time
			qtFileErr = (*theFile)->fFile.Seek(*theStartTimeP);
			(*theFile)->fStartTime = *theStartTimeP;
			
			//
			// This is an old client, so if we don't 0 out the negative transmit times,
			// we won't burst out packets.
			(*theFile)->fAllowNegativeTTs = false;
		}
		else
		{
			qtFileErr = (*theFile)->fFile.Seek(*theStartTimeP, *theMaxBackupTime);
			Float64 theFirstPacketTransmitTime = (*theFile)->fFile.GetFirstPacketTransmitTime();
			theBackupTime = *theStartTimeP - theFirstPacketTransmitTime;
			Assert(theBackupTime >= 0);
			
			//
			// For oddly authored movies, there are situations in which the packet
			// transmit time can be before the sample time. In that case, the backup
			// time may exceed the max backup time. In that case, just make the backup
			// time the max backup time.
#if 0
			printf("Backup time: %f. Max backup time: %f\n", theBackupTime, *theMaxBackupTime);
#endif
			if (theBackupTime > *theMaxBackupTime)
				theBackupTime = *theMaxBackupTime;
			//
			// If client specifies that it can do extra buffering (new client), use the first
			// packet transmit time as the start time for this play burst. We don't need to
			// burst any packets because the client can do the extra buffering
			(*theFile)->fStartTime = *theStartTimeP - theBackupTime;
			
			(*theFile)->fAllowNegativeTTs = true;
		}
	}
	
	if (qtFileErr == QTRTPFile::errCallAgain)
	{
		//
		// If we are doing RTP-Meta-Info stuff, we might be asked to get called again here.
		// This is simply because seeking might be a long operation and we don't want to
		// monopolize the CPU, but there is no other reason to wait, so just set a timeout of 0
		theErr = QTSS_SetIdleTimer(1);
		Assert(theErr == QTSS_NoErr);
		return theErr;
	}
	else if (qtFileErr != QTRTPFile::errNoError)
		return QTSSModuleUtils::SendErrorResponse(	inParamBlock->inRTSPRequest,
													qtssClientBadRequest, sSeekToNonexistentTimeErr);
														
	//make sure to clear the next packet the server would have sent!
	(*theFile)->fPacketStruct.packetData = NULL;
	
	// Set the movie duration and size parameters
	Float64 movieDuration = (*theFile)->fFile.GetMovieDuration();
	(void)QTSS_SetValue(inParamBlock->inClientSession, qtssCliSesMovieDurationInSecs, 0, &movieDuration, sizeof(movieDuration));
	
	UInt64 movieSize = (*theFile)->fFile.GetAddedTracksRTPBytes();
	(void)QTSS_SetValue(inParamBlock->inClientSession, qtssCliSesMovieSizeInBytes, 0, &movieSize, sizeof(movieSize));
	
	UInt32 bitsPerSecond =	(*theFile)->fFile.GetBytesPerSecond() * 8;
	(void)QTSS_SetValue(inParamBlock->inClientSession, qtssCliSesMovieAverageBitRate, 0, &bitsPerSecond, sizeof(bitsPerSecond));

	//
	// For the purposes of the speed header, check to make sure all tracks are
	// over a reliable transport
	Bool16 allTracksReliable = true;
	
	// Set the timestamp & sequence number parameters for each track.
	QTSS_RTPStreamObject* theRef = NULL;
	for (	UInt32 theStreamIndex = 0;
			QTSS_GetValuePtr(inParamBlock->inClientSession, qtssCliSesStreamObjects, theStreamIndex, (void**)&theRef, &theLen) == QTSS_NoErr;
			theStreamIndex++)
	{
		UInt32* theTrackID = NULL;
		theErr = QTSS_GetValuePtr(*theRef, qtssRTPStrTrackID, 0, (void**)&theTrackID, &theLen);
		Assert(theErr == QTSS_NoErr);
		Assert(theTrackID != NULL);
		Assert(theLen == sizeof(UInt32));
		
		SInt16 theSeqNum = (*theFile)->fFile.GetNextTrackSequenceNumber(*theTrackID);
		SInt32 theTimestamp = (*theFile)->fFile.GetSeekTimestamp(*theTrackID);
		
		Assert(theRef != NULL);
		
		theErr = QTSS_SetValue(*theRef, qtssRTPStrFirstSeqNumber, 0, &theSeqNum, sizeof(theSeqNum));
		Assert(theErr == QTSS_NoErr);
		theErr = QTSS_SetValue(*theRef, qtssRTPStrFirstTimestamp, 0, &theTimestamp, sizeof(theTimestamp));
		Assert(theErr == QTSS_NoErr);

		if (allTracksReliable)
		{
			QTSS_RTPTransportType theTransportType = qtssRTPTransportTypeUDP;
			theLen = sizeof(theTransportType);
			theErr = QTSS_GetValue(*theRef, qtssRTPStrTransportType, 0, &theTransportType, &theLen);
			Assert(theErr == QTSS_NoErr);
			
			if (theTransportType == qtssRTPTransportTypeUDP)
				allTracksReliable = false;
		}
	}
	
	//Tell the server to start playing this movie. We do want it to send RTCP SRs, but
	//we DON'T want it to write the RTP header
	theErr = QTSS_Play(inParamBlock->inClientSession, inParamBlock->inRTSPRequest, qtssPlayFlagsSendRTCP);
	if (theErr != QTSS_NoErr)
		return theErr;

	SInt64* thePlayTime = 0;
	theErr = QTSS_GetValuePtr(inParamBlock->inClientSession, qtssCliSesPlayTimeInMsec, 0, (void**)&thePlayTime, &theLen);
	Assert(theErr == QTSS_NoErr);
	Assert(thePlayTime != NULL);
	Assert(theLen == sizeof(SInt64));
	(*theFile)->fAdjustedPlayTime = *thePlayTime - ((SInt64)((*theFile)->fStartTime * 1000));
	
	//
	// This module supports the Speed header if the client wants the stream faster than normal.
	Float32 theSpeed = 1;
	theLen = sizeof(theSpeed);
	theErr = QTSS_GetValue(inParamBlock->inRTSPRequest, qtssRTSPReqSpeed, 0, &theSpeed, &theLen);
	Assert(theErr != QTSS_BadArgument);
	Assert(theErr != QTSS_NotEnoughSpace);
	
	if (theErr == QTSS_NoErr)
	{
		if (theSpeed > sMaxAllowedSpeed)
			theSpeed = sMaxAllowedSpeed;
		if ((theSpeed <= 0) || (!allTracksReliable))
			theSpeed = 1;
	}
	
	(*theFile)->fSpeed = theSpeed;
	
	if (theSpeed != 1)
	{
		//
		// If our speed is not 1, append the RTSP speed header in the response
		char speedBuf[32];
		::sprintf(speedBuf, "%10.5f", theSpeed);
		StrPtrLen speedBufPtr(speedBuf);
		(void)QTSS_AppendRTSPHeader(inParamBlock->inRTSPRequest, qtssSpeedHeader,
									speedBufPtr.Ptr, speedBufPtr.Len);
	}
	
	//
	// Record the requested stop time, if there is one
	(*theFile)->fStopTime = -1;
	theLen = sizeof((*theFile)->fStopTime);
	theErr = QTSS_GetValue(inParamBlock->inRTSPRequest, qtssRTSPReqStopTime, 0, &(*theFile)->fStopTime, &theLen);
	
	//
	// Append x-Prebuffer header if provided & nonzero prebuffer needed
	if (theBackupTime > 0)
	{
		char prebufferBuf[32];
		::sprintf(prebufferBuf, "time=%.5f", theBackupTime);
		StrPtrLen backupTimePtr(prebufferBuf);
		(void)QTSS_AppendRTSPHeader(inParamBlock->inRTSPRequest, qtssXPreBufferHeader,
									backupTimePtr.Ptr, backupTimePtr.Len);
	
	}

	(void)QTSS_SendStandardRTSPResponse(inParamBlock->inRTSPRequest, inParamBlock->inClientSession, qtssPlayRespWriteTrackInfo);
	return QTSS_NoErr;
}

QTSS_Error SendPackets(QTSS_RTPSendPackets_Params* inParams)
{
	static const UInt32 kQualityCheckIntervalInMsec = 250;	// v331=v107

	FileSession** theFile = NULL;
	UInt32 theLen = 0;
	QTSS_Error theErr = QTSS_GetValuePtr(inParams->inClientSession, sFileSessionAttr, 0, (void**)&theFile, &theLen);
	Assert(theErr == QTSS_NoErr);
	Assert(theLen == sizeof(FileSession*));
	bool isBeginningOfWriteBurst = true;
	
	QTRTPFile::RTPTrackListEntry* theLastPacketTrack = (*theFile)->fFile.GetLastPacketTrack();
	
	while (true)
	{	
		if ((*theFile)->fPacketStruct.packetData == NULL)
		{
			Float64 theTransmitTime = (*theFile)->fFile.GetNextPacket((char**)&(*theFile)->fPacketStruct.packetData, &(*theFile)->fNextPacketLen);
			if ( QTRTPFile::errNoError != (*theFile)->fFile.Error() )
			{
				QTSS_CliSesTeardownReason reason = qtssCliSesTearDownUnsupportedMedia;
				(void) QTSS_SetValue(inParams->inClientSession, qtssCliTeardownReason, 0, &reason, sizeof(reason));
				(void)QTSS_Teardown(inParams->inClientSession);
				return QTSS_RequestFailed;
			}
			
			theLastPacketTrack = (*theFile)->fFile.GetLastPacketTrack();
			
			//
			// Check to see if we should stop playing now
			
			if (((*theFile)->fStopTime != -1) && (theTransmitTime > (*theFile)->fStopTime))
			{
				// We should indeed stop playing
				(void)QTSS_Pause(inParams->inClientSession);
				inParams->outNextPacketTime = qtssDontCallSendPacketsAgain;
				return QTSS_NoErr;
			}
			if (((*theFile)->fStopTrackID != 0) && ((*theFile)->fStopTrackID == theLastPacketTrack->TrackID) && (theLastPacketTrack->HTCB->fCurrentPacketNumber > (*theFile)->fStopPN))
			{
				// We should indeed stop playing
				(void)QTSS_Pause(inParams->inClientSession);
				inParams->outNextPacketTime = qtssDontCallSendPacketsAgain;
				return QTSS_NoErr;
			}
			
			//
			// Find out what our play speed is. Send packets out at the specified rate,
			// and do so by altering the transmit time of the packet based on the Speed rate.
			Float64 theOffsetFromStartTime = theTransmitTime - (*theFile)->fStartTime;
			theTransmitTime = (*theFile)->fStartTime + (theOffsetFromStartTime / (*theFile)->fSpeed);
			
			//
			// correct for first packet xmit times that are < 0
			if (( theTransmitTime < 0.0 ) && ( !(*theFile)->fAllowNegativeTTs ))
				theTransmitTime = 0.0;
			
			(*theFile)->fPacketStruct.packetTransmitTime = (*theFile)->fAdjustedPlayTime + ((SInt64)(theTransmitTime * 1000));

		}
		
		//We are done playing all streams!
		if ((*theFile)->fPacketStruct.packetData == NULL)
		{
			//TODO not quite good to the last drop -- we -really- should guarantee this, also reflector
			// a write of 0 len to QTSS_Write will flush any buffered data if we're sending over tcp
			//(void)QTSS_Write((QTSS_Object)(*theFile)->fFile.GetLastPacketTrack()->Cookie1, NULL, 0, NULL, qtssWriteFlagsIsRTP);
			inParams->outNextPacketTime = qtssDontCallSendPacketsAgain;
			return QTSS_NoErr;
		}
		
		//we have a packet that needs to be sent now
		Assert(theLastPacketTrack != NULL);

		//If the stream is video, we need to make sure that QTRTPFile knows what quality level we're at
		if (inParams->inCurrentTime > ((*theFile)->fLastQualityCheck + kQualityCheckIntervalInMsec))
		{
			QTSS_RTPPayloadType thePayloadType = (QTSS_RTPPayloadType)theLastPacketTrack->Cookie2;
			if (thePayloadType == qtssVideoPayloadType)
			{
				(*theFile)->fLastQualityCheck = inParams->inCurrentTime;

				// Get the current quality level in the stream, and this stream's TrackID.
				UInt32* theQualityLevel = 0;
				theErr = QTSS_GetValuePtr((QTSS_Object)theLastPacketTrack->Cookie1, qtssRTPStrQualityLevel, 0, (void**)&theQualityLevel, &theLen);
				Assert(theErr == QTSS_NoErr);
				Assert(theQualityLevel != NULL);
				Assert(theLen == sizeof(UInt32));
		
				(*theFile)->fFile.SetTrackQualityLevel(theLastPacketTrack, *theQualityLevel);
			}
		}

		// Send the packet!
		QTSS_WriteFlags theFlags = qtssWriteFlagsIsRTP;
		if (isBeginningOfWriteBurst)
			theFlags |= qtssWriteFlagsWriteBurstBegin;
		theErr = QTSS_Write((QTSS_Object)theLastPacketTrack->Cookie1, &(*theFile)->fPacketStruct, (*theFile)->fNextPacketLen, NULL, theFlags);
		
		isBeginningOfWriteBurst = false;
		
		if ( theErr == QTSS_WouldBlock )
		{	
			//
			// In the case of a QTSS_WouldBlock error, the packetTransmitTime field of the packet struct will be set to
			// the time to wakeup, or -1 if not known.
			// If the time to wakeup is not given by the server, just give a fixed guess interval
			if ((*theFile)->fPacketStruct.suggestedWakeupTime == -1)
				inParams->outNextPacketTime = sFlowControlProbeInterval;	// for buffering, try me again in # MSec
			else
			{
				Assert((*theFile)->fPacketStruct.suggestedWakeupTime > inParams->inCurrentTime);
				inParams->outNextPacketTime = (*theFile)->fPacketStruct.suggestedWakeupTime - inParams->inCurrentTime;
			}
			
			//printf("Call again: %qd\n", inParams->outNextPacketTime);
				
			return QTSS_NoErr;
		}
		else
			(*theFile)->fPacketStruct.packetData = NULL;
	}
	Assert(0);
	return QTSS_NoErr;
}

QTSS_Error DestroySession(QTSS_ClientSessionClosing_Params* inParams)
{
	FileSession** theFile = NULL;
	UInt32 theLen = 0;
	QTSS_Error theErr = QTSS_GetValuePtr(inParams->inClientSession, sFileSessionAttr, 0, (void**)&theFile, &theLen);
	if ((theErr != QTSS_NoErr) || (theLen != sizeof(FileSession*)) || (theFile == NULL))
		return QTSS_RequestFailed;

	//
	// Tell the ClientSession how many samples we skipped because of stream thinning
	UInt32 theNumSkippedSamples = (*theFile)->fFile.GetNumSkippedSamples();
	(void)QTSS_SetValue(inParams->inClientSession, qtssCliSesFramesSkipped, 0, &theNumSkippedSamples, sizeof(theNumSkippedSamples));
	
	DeleteFileSession(*theFile);
	return QTSS_NoErr;
}

void	DeleteFileSession(FileSession* inFileSession)
{
	// If we're supposed to delete the SDP data, do so now.
	if (inFileSession->fDeleteSDP)
		delete [] inFileSession->fSDPSource.GetSDPData()->Ptr;
		
	delete inFileSession;
}
