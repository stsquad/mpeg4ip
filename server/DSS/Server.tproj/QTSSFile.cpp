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
	File:		QTSSFile.h

	Contains:	 
					
	$Log: QTSSFile.cpp,v $
	Revision 1.5  2001/10/11 20:39:10  wmaycisco
	Sync 10/11/2001. 1:40PM
	
	Revision 1.3  2001/10/01 22:08:40  dmackie
	DSS 3.0.1 import
	
	Revision 1.5  2001/03/13 22:24:39  murata
	Replace copyright notice for license 1.0 with license 1.2 and update the copyright year.
	Bug #:
	Submitted by:
	Reviewed by:
	
	Revision 1.4  2000/10/11 07:06:15  serenyi
	import
	
	Revision 1.1.1.1  2000/08/31 00:30:48  serenyi
	Mothra Repository
	
	Revision 1.3  2000/06/09 22:45:45  serenyi
	Added module dictionaries, stuff to parse XML config format
	
	Revision 1.2  2000/06/02 06:56:05  serenyi
	First checkin for module dictionaries
	
	Revision 1.1  2000/05/22 06:05:28  serenyi
	Added request body support, API additions, SETUP without DESCRIBE support, RTCP bye support
	
	
	
*/

#include "QTSSFile.h"
#include "QTSServerInterface.h"

QTSSAttrInfoDict::AttrInfo	QTSSFile::sAttributes[] = 
{   /*fields:   fAttrName, fFuncPtr, fAttrDataType, fAttrPermission */
	/* 0 */ { "qtssFlObjStream", 				NULL, 	qtssAttrDataTypeQTSS_StreamRef,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 1 */ { "qtssFlObjFileSysModuleName", 	NULL, 	qtssAttrDataTypeCharArray,		qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 2 */ { "qtssFlObjLength",				NULL, 	qtssAttrDataTypeUInt64,			qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite },
	/* 3 */ { "qtssFlObjPosition",				NULL, 	qtssAttrDataTypeUInt64,			qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 4 */ { "qtssFlObjModDate",				NULL, 	qtssAttrDataTypeUInt64,			qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite }
};

void	QTSSFile::Initialize()
{
	for (UInt32 x = 0; x < qtssFlObjNumParams; x++)
		QTSSDictionaryMap::GetMap(QTSSDictionaryMap::kFileDictIndex)->
			SetAttribute(x, sAttributes[x].fAttrName, sAttributes[x].fFuncPtr, sAttributes[x].fAttrDataType, sAttributes[x].fAttrPermission);
}

QTSSFile::QTSSFile()
:	QTSSDictionary(QTSSDictionaryMap::GetMap(QTSSDictionaryMap::kFileDictIndex)),
	fModule(NULL),
	fPosition(0),
	fThisPtr(this),
	fLength(0),
	fModDate(0)
{
	//
	// The stream is just a pointer to this thing
	this->SetVal(qtssFlObjStream, &fThisPtr, sizeof(fThisPtr));
	this->SetVal(qtssFlObjLength, &fLength, sizeof(fLength));
	this->SetVal(qtssFlObjPosition, &fPosition, sizeof(fPosition));
	this->SetVal(qtssFlObjModDate, &fModDate, sizeof(fModDate));
}

QTSS_Error	QTSSFile::Open(char* inPath, QTSS_OpenFileFlags inFlags)
{
	//
	// Because this is a role being executed from inside a callback, we need to
	// make sure that QTSS_RequestEvent will not work.
	Task* curTask = NULL;
	QTSS_ModuleState* theState = (QTSS_ModuleState*)OSThread::GetMainThreadData();
	if (OSThread::GetCurrent() != NULL)
		theState = (QTSS_ModuleState*)OSThread::GetCurrent()->GetThreadData();
		
	if (theState != NULL)
		curTask = theState->curTask;
	
	QTSS_RoleParams theParams;
	theParams.openFileParams.inPath = inPath;
	theParams.openFileParams.inFlags = inFlags;
	theParams.openFileParams.inFileObject = this;

	QTSS_Error theErr = QTSS_FileNotFound;
	UInt32 x = 0;
	
	for ( ; x < QTSServerInterface::GetNumModulesInRole(QTSSModule::kOpenFilePreProcessRole); x++)
	{
		theErr = QTSServerInterface::GetModule(QTSSModule::kOpenFilePreProcessRole, x)->CallDispatch(QTSS_OpenFilePreProcess_Role, &theParams);
		if (theErr != QTSS_FileNotFound)
		{
			fModule = QTSServerInterface::GetModule(QTSSModule::kOpenFilePreProcessRole, x);
			break;
		}
	}
	
	if (theErr == QTSS_FileNotFound)
	{
		// None of the prepreprocessors claimed this file. Invoke the default file handler
		if (QTSServerInterface::GetNumModulesInRole(QTSSModule::kOpenFileRole) > 0)
		{
			fModule = QTSServerInterface::GetModule(QTSSModule::kOpenFileRole, 0);
			theErr = QTSServerInterface::GetModule(QTSSModule::kOpenFileRole, 0)->CallDispatch(QTSS_OpenFile_Role, &theParams);

		}
	}
	
	//
	// Reset the curTask to what it was before this role started
	if (theState != NULL)
		theState->curTask = curTask;

	return theErr;
}

void	QTSSFile::Close()
{
	Assert(fModule != NULL);
	
	QTSS_RoleParams theParams;
	theParams.closeFileParams.inFileObject = this;
	(void)fModule->CallDispatch(QTSS_CloseFile_Role, &theParams);
}


QTSS_Error	QTSSFile::Read(void* ioBuffer, UInt32 inBufLen, UInt32* outLengthRead)
{
	Assert(fModule != NULL);
	UInt32 theLenRead = 0;

	//
	// Invoke the owning QTSS API module. Setup a param block to do so.
	QTSS_RoleParams theParams;
	theParams.readFileParams.inFileObject = this;
	theParams.readFileParams.inFilePosition = fPosition;
	theParams.readFileParams.ioBuffer = ioBuffer;
	theParams.readFileParams.inBufLen = inBufLen;
	theParams.readFileParams.outLenRead = &theLenRead;
	
	QTSS_Error theErr = fModule->CallDispatch(QTSS_ReadFile_Role, &theParams);
	
	fPosition += theLenRead;
	if (outLengthRead != NULL)
		*outLengthRead = theLenRead;
		
	return theErr;
}
															
QTSS_Error	QTSSFile::Seek(UInt64 inNewPosition)
{
	UInt64* theFileLength = NULL;
	UInt32 theParamLength = 0;
	
	(void)this->GetValuePtr(qtssFlObjLength, 0, (void**)&theFileLength, &theParamLength);
	
	if (theParamLength != sizeof(UInt64))
		return QTSS_RequestFailed;
		
	if (inNewPosition > *theFileLength)
		return QTSS_RequestFailed;
		
	fPosition = inNewPosition;
	return QTSS_NoErr;
}
		
QTSS_Error	QTSSFile::Advise(UInt64 inPosition, UInt32 inAdviseSize)
{
	Assert(fModule != NULL);

	//
	// Invoke the owning QTSS API module. Setup a param block to do so.
	QTSS_RoleParams theParams;
	theParams.adviseFileParams.inFileObject = this;
	theParams.adviseFileParams.inPosition = inPosition;
	theParams.adviseFileParams.inSize = inAdviseSize;

	return fModule->CallDispatch(QTSS_AdviseFile_Role, &theParams);
}

QTSS_Error	QTSSFile::RequestEvent(QTSS_EventType inEventMask)
{
	Assert(fModule != NULL);

	//
	// Invoke the owning QTSS API module. Setup a param block to do so.
	QTSS_RoleParams theParams;
	theParams.reqEventFileParams.inFileObject = this;
	theParams.reqEventFileParams.inEventMask = inEventMask;

	return fModule->CallDispatch(QTSS_RequestEventFile_Role, &theParams);
}
