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
	File:		QTSSDictionary.cpp

	Contains:	Implementation of object defined in QTSSDictionary.h
					

*/


#include "QTSSDictionary.h"

#include <string.h>
#include <stdio.h>

#include "MyAssert.h"
#include "OSMemory.h"
#include "QTSSDataConverter.h"

#include <errno.h>


#pragma mark __QTSS_DICTIONARY__

QTSSDictionary::QTSSDictionary(QTSSDictionaryMap* inMap, OSMutex* inMutex) 
: 	fInstanceAttrs(NULL), fInstanceArraySize(0),
	fMap(inMap), fInstanceMap(NULL), fMutexP(inMutex)
{
	fAttributes = NEW DictValueElement[inMap->GetNumAttrs()];
}

QTSSDictionary::~QTSSDictionary()
{
	this->DeleteAttributeData(fAttributes, fMap->GetNumAttrs());
	delete [] fAttributes;
	delete fInstanceMap;
	this->DeleteAttributeData(fInstanceAttrs, fInstanceArraySize);
	delete [] fInstanceAttrs;
}

QTSS_Error QTSSDictionary::GetValuePtr(QTSS_AttributeID inAttrID, UInt32 inIndex,
											void** outValueBuffer, UInt32* outValueLen,
											Bool16 isInternal)
{
	// Check first to see if this is a static attribute or an instance attribute
	QTSSDictionaryMap* theMap = fMap;
	DictValueElement* theAttrs = fAttributes;
	if (QTSSDictionaryMap::IsInstanceAttrID(inAttrID))
	{
		theMap = fInstanceMap;
		theAttrs = fInstanceAttrs;
	}

	if (theMap == NULL)
		return QTSS_AttrDoesntExist;
	
	SInt32 theMapIndex = theMap->ConvertAttrIDToArrayIndex(inAttrID);

	if (theMapIndex < 0)
		return QTSS_AttrDoesntExist;
	if (theMap->IsRemoved(theMapIndex))
		return QTSS_AttrDoesntExist;
	if ((!isInternal) && (!theMap->IsPreemptiveSafe(theMapIndex)))
		return QTSS_NotPreemptiveSafe;
	// An iterated attribute cannot have a param retrieval function
	if ((inIndex > 0) && (theMap->GetAttrFunction(theMapIndex) != NULL))
		return QTSS_BadIndex;
	// Check to make sure the index parameter is legal
	if (inIndex > theAttrs[theMapIndex].fNumAttributes)
		return QTSS_BadIndex;
		
		
	// Retrieve the parameter
	char* theBuffer = theAttrs[theMapIndex].fAttributeData.Ptr;
	*outValueLen = theAttrs[theMapIndex].fAttributeData.Len;
	
	if ((*outValueLen == 0) && (theMap->GetAttrFunction(theMapIndex) != NULL))
	{
		// If the parameter doesn't have a value assigned yet, and there is an attribute
		// retrieval function provided, invoke that function now.
		
		theBuffer = (char*)theMap->GetAttrFunction(theMapIndex)(this, outValueLen);

		//If the param retrieval function didn't return an explicit value for this attribute,
		//refetch the parameter out of the array, in case the function modified it.
		
		if (*outValueLen == 0)
		{
			theBuffer = theAttrs[theMapIndex].fAttributeData.Ptr;
			*outValueLen = theAttrs[theMapIndex].fAttributeData.Len;
		}
		
	}
#if DEBUG
	else
		// Make sure we aren't outside the bounds of attribute memory
		Assert(theAttrs[theMapIndex].fAllocatedLen >=
			(theAttrs[theMapIndex].fAttributeData.Len * (theAttrs[theMapIndex].fNumAttributes + 1)));
#endif

	// Return an error if there is no data for this attribute
	if (*outValueLen == 0)
		return QTSS_ValueNotFound;
			
	// Adjust for the iteration, if any
	theBuffer += theAttrs[theMapIndex].fAttributeData.Len * inIndex;
	*outValueBuffer = theBuffer;
	
	return QTSS_NoErr;
}



QTSS_Error QTSSDictionary::GetValue(QTSS_AttributeID inAttrID, UInt32 inIndex,
											void* ioValueBuffer, UInt32* ioValueLen)
{
	// If there is a mutex, lock it and get a pointer to the proper attribute
	OSMutexLocker locker(fMutexP);

	void* tempValueBuffer = NULL;
	UInt32 tempValueLen = 0;
	QTSS_Error theErr = this->GetValuePtr(inAttrID, inIndex, &tempValueBuffer, &tempValueLen, true);
	if (theErr != QTSS_NoErr)
		return theErr;
		
	if (theErr == QTSS_NoErr)
	{
		// If caller provided a buffer that's too small for this attribute, report that error
		if (tempValueLen > *ioValueLen)
			theErr = QTSS_NotEnoughSpace;
			
		// Only copy out the attribute if the buffer is big enough
		if ((ioValueBuffer != NULL) && (theErr == QTSS_NoErr))
			::memcpy(ioValueBuffer, tempValueBuffer, tempValueLen);
			
		// Always set the ioValueLen to be the actual length of the attribute.
		*ioValueLen = tempValueLen;
	}

	return QTSS_NoErr;
}

QTSS_Error QTSSDictionary::GetValueAsString(QTSS_AttributeID inAttrID, UInt32 inIndex, char** outString)
{
	void* tempValueBuffer;
	UInt32 tempValueLen = 0;

	if (outString == NULL)	
		return QTSS_BadArgument;
		
	OSMutexLocker locker(fMutexP);
	QTSS_Error theErr = this->GetValuePtr(inAttrID, inIndex, &tempValueBuffer,
											&tempValueLen, true);
	if (theErr != QTSS_NoErr)
		return theErr;
		
	QTSSDictionaryMap* theMap = fMap;
	if (QTSSDictionaryMap::IsInstanceAttrID(inAttrID))
		theMap = fInstanceMap;

	Assert(theMap != NULL);
	SInt32 theMapIndex = theMap->ConvertAttrIDToArrayIndex(inAttrID);
	Assert(theMapIndex >= 0);
	
	*outString = QTSSDataConverter::ValueToString(tempValueBuffer, tempValueLen, theMap->GetAttrType(theMapIndex));
	return QTSS_NoErr;
}



QTSS_Error QTSSDictionary::SetValue(QTSS_AttributeID inAttrID, UInt32 inIndex,
										const void* inBuffer,  UInt32 inLen,
										UInt32 inFlags)
{
	// Check first to see if this is a static attribute or an instance attribute
	QTSSDictionaryMap* theMap = fMap;
	DictValueElement* theAttrs = fAttributes;
	if (QTSSDictionaryMap::IsInstanceAttrID(inAttrID))
	{
		theMap = fInstanceMap;
		theAttrs = fInstanceAttrs;
	}
	
	if (theMap == NULL)
		return QTSS_AttrDoesntExist;
	
	SInt32 theMapIndex = theMap->ConvertAttrIDToArrayIndex(inAttrID);
	
	// If there is a mutex, make this action atomic.
	OSMutexLocker locker(fMutexP);
	
	if (theMapIndex < 0)
		return QTSS_AttrDoesntExist;
	if ((!(inFlags & kDontObeyReadOnly)) && (!theMap->IsWriteable(theMapIndex)))
		return QTSS_ReadOnly;
	if (theMap->IsRemoved(theMapIndex))
		return QTSS_AttrDoesntExist;
	
	UInt32 numValues = 0;
	if (theAttrs[theMapIndex].fAttributeData.Len > 0)
		numValues = theAttrs[theMapIndex].fNumAttributes + 1;

	// If this attribute is iterated, this new value
	// must be the same size as all the others.
	if ((numValues > 1) && (inLen != theAttrs[theMapIndex].fAttributeData.Len))
		return QTSS_BadArgument;
	
	//
	// Can't put empty space into the array of values
	if (inIndex > numValues)
		return QTSS_BadIndex;

	if ((inLen * (inIndex + 1)) > theAttrs[theMapIndex].fAllocatedLen)
	{
		// We need to reallocate this buffer.
		UInt32 theLen = 2 * (inLen * (inIndex + 1));// Allocate 2wice as much as we need
		char* theNewBuffer = NEW char[theLen];
		// Copy out the old attribute data
		::memcpy(theNewBuffer, theAttrs[theMapIndex].fAttributeData.Ptr,
					theAttrs[theMapIndex].fAllocatedLen);
		
		// Now get rid of the old stuff. Delete the buffer
		// if it was already allocated internally
		if (theAttrs[theMapIndex].fAllocatedInternally)
			delete [] theAttrs[theMapIndex].fAttributeData.Ptr;
		
		// Finally, update this attribute structure with all the new values.
		theAttrs[theMapIndex].fAttributeData.Ptr = theNewBuffer;
		theAttrs[theMapIndex].fAllocatedLen = theLen;
		theAttrs[theMapIndex].fAllocatedInternally = true;
	}
		
	// At this point, we should always have enough space to write what we want
	Assert(theAttrs[theMapIndex].fAllocatedLen >= (inLen * (inIndex + 1)));
	
	// Set the number of attributes to be proper
	if (inIndex > theAttrs[theMapIndex].fNumAttributes)
	{
		//
		// We should never have to increment num attributes by more than 1
		Assert((theAttrs[theMapIndex].fNumAttributes + 1) == inIndex);
		theAttrs[theMapIndex].fNumAttributes++;
	}

	// Copy the new data to the right place in our data buffer
	void *attributeBufferPtr = theAttrs[theMapIndex].fAttributeData.Ptr + (inLen * inIndex);
	::memcpy(attributeBufferPtr, inBuffer, inLen);
	theAttrs[theMapIndex].fAttributeData.Len = inLen;

	//
	// Call the completion routine
	if (fMap->CompleteFunctionsAllowed() && !(inFlags & kDontCallCompletionRoutine))
		this->SetValueComplete(theMapIndex, theMap, inIndex, attributeBufferPtr, inLen);
	
	return QTSS_NoErr;
}



QTSS_Error QTSSDictionary::RemoveValue(QTSS_AttributeID inAttrID, UInt32 inIndex, UInt32 inFlags)
{
	// Check first to see if this is a static attribute or an instance attribute
	QTSSDictionaryMap* theMap = fMap;
	DictValueElement* theAttrs = fAttributes;
	if (QTSSDictionaryMap::IsInstanceAttrID(inAttrID))
	{
		theMap = fInstanceMap;
		theAttrs = fInstanceAttrs;
	}
	
	if (theMap == NULL)
		return QTSS_AttrDoesntExist;
	
	SInt32 theMapIndex = theMap->ConvertAttrIDToArrayIndex(inAttrID);
	
	// If there is a mutex, make this action atomic.
	OSMutexLocker locker(fMutexP);
	
	if (theMapIndex < 0)
		return QTSS_AttrDoesntExist;
	if ((!(inFlags & kDontObeyReadOnly)) && (!theMap->IsWriteable(theMapIndex)))
		return QTSS_ReadOnly;
	if (theMap->IsRemoved(theMapIndex))
		return QTSS_AttrDoesntExist;
	if ((theMap->GetAttrFunction(theMapIndex) != NULL) && (inIndex > 0))
		return QTSS_BadIndex;
		
	UInt32 numValues = 0;
	if (theAttrs[theMapIndex].fAttributeData.Len > 0)
		numValues = theAttrs[theMapIndex].fNumAttributes + 1;

	UInt32 theValueLen = theAttrs[theMapIndex].fAttributeData.Len;

	//
	// If there are values after this one in the array, move them.
	::memmove(	theAttrs[theMapIndex].fAttributeData.Ptr + (theValueLen * inIndex),
				theAttrs[theMapIndex].fAttributeData.Ptr + (theValueLen * (inIndex + 1)),
				theValueLen * ( (theAttrs[theMapIndex].fNumAttributes) - inIndex));
	
	//
	// Update our number of values
	if (theAttrs[theMapIndex].fNumAttributes > 0)
		theAttrs[theMapIndex].fNumAttributes--;
	else
		theAttrs[theMapIndex].fAttributeData.Len = 0;

	//
	// Call the completion routine
	if (fMap->CompleteFunctionsAllowed() && !(inFlags & kDontCallCompletionRoutine))
		this->RemoveValueComplete(theMapIndex, theMap, inIndex);
		
	return QTSS_NoErr;
}

UInt32	QTSSDictionary::GetNumValues(QTSS_AttributeID inAttrID)
{
	// Check first to see if this is a static attribute or an instance attribute
	QTSSDictionaryMap* theMap = fMap;
	DictValueElement* theAttrs = fAttributes;
	if (QTSSDictionaryMap::IsInstanceAttrID(inAttrID))
	{
		theMap = fInstanceMap;
		theAttrs = fInstanceAttrs;
	}

	if (theMap == NULL)
		return 0;

	// fNumAttributes is offset from 0, we need to convert this to a
	// normal count of how many values there are.
	
	SInt32 theMapIndex = theMap->ConvertAttrIDToArrayIndex(inAttrID);
	if (theMapIndex < 0)
		return 0;

	if (theAttrs[theMapIndex].fAttributeData.Len == 0)
		return 0;
	else
		return theAttrs[theMapIndex].fNumAttributes + 1;
}

void	QTSSDictionary::SetNumValues(QTSS_AttributeID inAttrID, UInt32 inNumValues)
{
	// Check first to see if this is a static attribute or an instance attribute
	QTSSDictionaryMap* theMap = fMap;
	DictValueElement* theAttrs = fAttributes;
	if (QTSSDictionaryMap::IsInstanceAttrID(inAttrID))
	{
		theMap = fInstanceMap;
		theAttrs = fInstanceAttrs;
	}

	if (theMap == NULL)
		return;

	SInt32 theMapIndex = theMap->ConvertAttrIDToArrayIndex(inAttrID);
	if (theMapIndex < 0)
		return;

	if (inNumValues == 0)
		theAttrs[theMapIndex].fAttributeData.Len = 0;
	else
		theAttrs[theMapIndex].fNumAttributes = inNumValues -1;
}

void	QTSSDictionary::SetVal(	QTSS_AttributeID inAttrID,
									void* inValueBuffer,
									UInt32 inBufferLen)
{ 
	Assert(inAttrID >= 0);
	Assert((UInt32)inAttrID < fMap->GetNumAttrs());
	fAttributes[inAttrID].fAttributeData.Ptr = (char*)inValueBuffer;
	fAttributes[inAttrID].fAttributeData.Len = inBufferLen;
	fAttributes[inAttrID].fAllocatedLen = inBufferLen;
	
	// This function doesn't set fNumAttributes & fAllocatedInternally.
}

void	QTSSDictionary::SetEmptyVal(QTSS_AttributeID inAttrID, void* inBuf, UInt32 inBufLen)
{
	Assert(inAttrID >= 0);
	Assert((UInt32)inAttrID < fMap->GetNumAttrs());
	fAttributes[inAttrID].fAttributeData.Ptr = (char*)inBuf;
	fAttributes[inAttrID].fAllocatedLen = inBufLen;

#if !ALLOW_NON_WORD_ALIGN_ACCESS
	//if (((UInt32) inBuf % 4) > 0)
	//	printf("bad align by %d\n",((UInt32) inBuf % 4) );
	Assert( ((PointerSizedInt) inBuf % 4) == 0 );
#endif

}


QTSS_Error	QTSSDictionary::AddInstanceAttribute(	const char* inAttrName,
													QTSS_AttrFunctionPtr inFuncPtr,
													QTSS_AttrDataType inDataType,
													QTSS_AttrPermission inPermission )
{
	if (!fMap->InstanceAttrsAllowed())
		return QTSS_InstanceAttrsNotAllowed;
		
	OSMutexLocker locker(fMutexP);

	//
	// Check to see if this attribute exists in the static map. If it does,
	// we can't add it as an instance attribute, so return an error
	QTSSAttrInfoDict* throwAway = NULL;
	QTSS_Error theErr = fMap->GetAttrInfoByName(inAttrName, &throwAway);
	if (theErr == QTSS_NoErr)
		return QTSS_AttrNameExists;
	
	if (fInstanceMap == NULL)
	{
		UInt32 theFlags = QTSSDictionaryMap::kAllowRemoval | QTSSDictionaryMap::kIsInstanceMap;
		if (fMap->CompleteFunctionsAllowed())
			theFlags |= QTSSDictionaryMap::kCompleteFunctionsAllowed;
			
		fInstanceMap = new QTSSDictionaryMap( 0, theFlags );
	}
	
	//
	// Add the attribute into the Dictionary Map.
	theErr = fInstanceMap->AddAttribute(inAttrName, inFuncPtr, inDataType, inPermission);
	if (theErr != QTSS_NoErr)
		return theErr;
	
	//
	// Check to see if our DictValueElement array needs to be reallocated	
	if (fInstanceMap->GetNumAttrs() >= fInstanceArraySize)
	{
		UInt32 theNewArraySize = fInstanceArraySize * 2;
		if (theNewArraySize == 0)
			theNewArraySize = QTSSDictionaryMap::kMinArraySize;
		Assert(theNewArraySize > fInstanceMap->GetNumAttrs());
		
		DictValueElement* theNewArray = NEW DictValueElement[theNewArraySize];
		if (fInstanceAttrs != NULL)
		{
			::memcpy(theNewArray, fInstanceAttrs, sizeof(DictValueElement) * fInstanceArraySize);

			//
			// Delete the old instance attr structs, this does not delete the actual attribute memory
			delete [] fInstanceAttrs;
		}
		fInstanceAttrs = theNewArray;
		fInstanceArraySize = theNewArraySize;
	}
	return QTSS_NoErr;
}
QTSS_Error	QTSSDictionary::RemoveInstanceAttribute(QTSS_AttributeID inAttr)
{
	OSMutexLocker locker(fMutexP);

	if (fInstanceMap != NULL)
	{	
		this->SetNumValues(inAttr,(UInt32) 0); // make sure to set num values to 0 since it is a deleted attribute
		fInstanceMap->RemoveAttribute(inAttr);
	}
	else
		return QTSS_BadArgument;
	
	//
	// Call the completion routine
	SInt32 theMapIndex = fInstanceMap->ConvertAttrIDToArrayIndex(inAttr);
	this->RemoveInstanceAttrComplete(theMapIndex, fInstanceMap);
	
	return QTSS_NoErr;
}

QTSS_Error QTSSDictionary::GetAttrInfoByIndex(UInt32 inIndex, QTSSAttrInfoDict** outAttrInfoDict)
{
	if (outAttrInfoDict == NULL)
		return QTSS_BadArgument;
		
	OSMutexLocker locker(fMutexP);

	UInt32 numInstanceValues = 0;
	UInt32 numStaticValues = fMap->GetNumNonRemovedAttrs();
	
	if (fInstanceMap != NULL)
		numInstanceValues = fInstanceMap->GetNumNonRemovedAttrs();
	
	if (inIndex >= (numStaticValues + numInstanceValues))
		return QTSS_AttrDoesntExist;
	
	if ( (numStaticValues > 0)  && (inIndex < numStaticValues) )
		return fMap->GetAttrInfoByIndex(inIndex, outAttrInfoDict);
	else
	{
		Assert(fInstanceMap != NULL);
		return fInstanceMap->GetAttrInfoByIndex(inIndex - numStaticValues, outAttrInfoDict);
	}
}

QTSS_Error QTSSDictionary::GetAttrInfoByID(QTSS_AttributeID inAttrID, QTSSAttrInfoDict** outAttrInfoDict)
{
	if (outAttrInfoDict == NULL)
		return QTSS_BadArgument;
		
	if (QTSSDictionaryMap::IsInstanceAttrID(inAttrID))
	{
		OSMutexLocker locker(fMutexP);

		if (fInstanceMap != NULL)
			return fInstanceMap->GetAttrInfoByID(inAttrID, outAttrInfoDict);
	}
	else
		return fMap->GetAttrInfoByID(inAttrID, outAttrInfoDict);
			
	return QTSS_AttrDoesntExist;
}

QTSS_Error QTSSDictionary::GetAttrInfoByName(const char* inAttrName, QTSSAttrInfoDict** outAttrInfoDict)
{
	if (outAttrInfoDict == NULL)
		return QTSS_BadArgument;
		
	// Retrieve the Dictionary Map for this object type
	QTSS_Error theErr = fMap->GetAttrInfoByName(inAttrName, outAttrInfoDict);
	
	if (theErr == QTSS_AttrDoesntExist)
	{
		OSMutexLocker locker(fMutexP);
		if (fInstanceMap != NULL)
			theErr = fInstanceMap->GetAttrInfoByName(inAttrName, outAttrInfoDict);
	}
	return theErr;
}

void QTSSDictionary::DeleteAttributeData(DictValueElement* inDictValues, UInt32 inNumValues)
{
	for (UInt32 x = 0; x < inNumValues; x++)
	{
		if (inDictValues[x].fAllocatedInternally)
			delete [] inDictValues[x].fAttributeData.Ptr;
	}
}


#pragma mark __QTSS_ATTR_INFO_DICT__

QTSSAttrInfoDict::AttrInfo	QTSSAttrInfoDict::sAttributes[] =
{
	/* 0 */ { "qtssAttrName",		NULL,		qtssAttrDataTypeCharArray,	qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 1 */ { "qtssAttrID",			NULL,		qtssAttrDataTypeUInt32,		qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 2 */ { "qtssAttrDataType",	NULL,		qtssAttrDataTypeUInt32,		qtssAttrModeRead | qtssAttrModePreempSafe },
	/* 3 */ { "qtssAttrPermissions",NULL,		qtssAttrDataTypeUInt32,		qtssAttrModeRead | qtssAttrModePreempSafe }
};

QTSSAttrInfoDict::QTSSAttrInfoDict()
: QTSSDictionary(QTSSDictionaryMap::GetMap(QTSSDictionaryMap::kAttrInfoDictIndex)), fID(qtssIllegalAttrID)
{}

QTSSAttrInfoDict::~QTSSAttrInfoDict() {}


#pragma mark __QTSS_DICTIONARY_MAP__

QTSSDictionaryMap*		QTSSDictionaryMap::sDictionaryMaps[kNumDictionaries];

void QTSSDictionaryMap::Initialize()
{
	//
	// Have to do this one first because this dict map is used by all the other
	// dict maps.
	sDictionaryMaps[kAttrInfoDictIndex] 	= new QTSSDictionaryMap(qtssAttrInfoNumParams);

	// Setup the Attr Info attributes before constructing any other dictionaries
	for (UInt32 x = 0; x < qtssAttrInfoNumParams; x++)
		sDictionaryMaps[kAttrInfoDictIndex]->SetAttribute(x, QTSSAttrInfoDict::sAttributes[x].fAttrName,
															QTSSAttrInfoDict::sAttributes[x].fFuncPtr,
															QTSSAttrInfoDict::sAttributes[x].fAttrDataType,
															QTSSAttrInfoDict::sAttributes[x].fAttrPermission);

	sDictionaryMaps[kServerDictIndex] 		= new QTSSDictionaryMap(qtssSvrNumParams, QTSSDictionaryMap::kCompleteFunctionsAllowed);
	sDictionaryMaps[kPrefsDictIndex] 		= new QTSSDictionaryMap(qtssPrefsNumParams, QTSSDictionaryMap::kInstanceAttrsAllowed | QTSSDictionaryMap::kCompleteFunctionsAllowed);
	sDictionaryMaps[kTextMessagesDictIndex] = new QTSSDictionaryMap(qtssMsgNumParams);
	sDictionaryMaps[kServiceDictIndex] 		= new QTSSDictionaryMap(0);
	sDictionaryMaps[kRTPStreamDictIndex] 	= new QTSSDictionaryMap(qtssRTPStrNumParams);
	sDictionaryMaps[kClientSessionDictIndex]= new QTSSDictionaryMap(qtssCliSesNumParams);
	sDictionaryMaps[kRTSPSessionDictIndex] 	= new QTSSDictionaryMap(qtssRTSPSesNumParams);
	sDictionaryMaps[kRTSPRequestDictIndex] 	= new QTSSDictionaryMap(qtssRTSPReqNumParams);
	sDictionaryMaps[kRTSPHeaderDictIndex] 	= new QTSSDictionaryMap(qtssNumHeaders);
	sDictionaryMaps[kFileDictIndex] 		= new QTSSDictionaryMap(qtssFlObjNumParams);
	sDictionaryMaps[kModuleDictIndex] 		= new QTSSDictionaryMap(qtssModNumParams);
	sDictionaryMaps[kModulePrefsDictIndex] 	= new QTSSDictionaryMap(0, QTSSDictionaryMap::kInstanceAttrsAllowed | QTSSDictionaryMap::kCompleteFunctionsAllowed);
	sDictionaryMaps[kQTSSUserProfileDictIndex] = new QTSSDictionaryMap(qtssUserNumParams);
}

QTSSDictionaryMap::QTSSDictionaryMap(UInt32 inNumReservedAttrs, UInt32 inFlags)
: 	fNextAvailableID(inNumReservedAttrs), fNumValidAttrs(inNumReservedAttrs),fAttrArraySize(inNumReservedAttrs), fFlags(inFlags)
{
	if (fAttrArraySize < kMinArraySize)
		fAttrArraySize = kMinArraySize;
	fAttrArray = NEW QTSSAttrInfoDict*[fAttrArraySize];
	::memset(fAttrArray, 0, sizeof(QTSSAttrInfoDict*) * fAttrArraySize);
}

QTSS_Error QTSSDictionaryMap::AddAttribute(	const char* inAttrName,
											QTSS_AttrFunctionPtr inFuncPtr,
											QTSS_AttrDataType inDataType,
											QTSS_AttrPermission inPermission)
{
	if (inAttrName == NULL || ::strlen(inAttrName) > QTSS_MAX_ATTRIBUTE_NAME_SIZE)
		return QTSS_BadArgument;

	for (UInt32 count = 0; count < fNextAvailableID; count++)
	{
		if	(::strcmp(&fAttrArray[count]->fAttrInfo.fAttrName[0], inAttrName) == 0)
		{	// found the name in the dictionary
			if (fAttrArray[count]->fAttrInfo.fAttrPermission & qtssPrivateAttrModeRemoved )
			{ // it is a previously removed attribute
				if (fAttrArray[count]->fAttrInfo.fAttrDataType == inDataType)
				{ //same type so reuse the attribute
					QTSS_AttributeID attrID = fAttrArray[count]->fID; 
					this->UnRemoveAttribute(attrID); 
					fAttrArray[count]->fAttrInfo.fFuncPtr = inFuncPtr; // reset
					fAttrArray[count]->fAttrInfo.fAttrPermission = inPermission;// reset
					return QTSS_NoErr; // nothing left to do. It is re-added.
				}
				
				// a removed attribute with the same name but different type--so keep checking 
				continue;
			}
			// an error, an active attribute with this name exists
			return QTSS_AttrNameExists;
		}
	}

	if (fAttrArraySize == fNextAvailableID)
	{
		// If there currently isn't an attribute array, or if the current array
		// is full, allocate a new array and copy all the old stuff over to the new array.
		
		UInt32 theNewArraySize = fAttrArraySize * 2;
		if (theNewArraySize == 0)
			theNewArraySize = kMinArraySize;
		
		QTSSAttrInfoDict** theNewArray = NEW QTSSAttrInfoDict*[theNewArraySize];
		::memset(theNewArray, 0, sizeof(QTSSAttrInfoDict*) * theNewArraySize);
		if (fAttrArray != NULL)
		{
			::memcpy(theNewArray, fAttrArray, sizeof(QTSSAttrInfoDict*) * fAttrArraySize);
			delete [] fAttrArray;
		}
		fAttrArray = theNewArray;
		fAttrArraySize = theNewArraySize;
	}
	
	QTSS_AttributeID theID = fNextAvailableID;
	fNextAvailableID++;
	fNumValidAttrs++;
	if (fFlags & kIsInstanceMap)
		theID |= 0x80000000; // Set the high order bit to indicate this is an instance attr

	// Copy the information into the first available element
	// Currently, all attributes added in this fashion are always writeable
	this->SetAttribute(theID, inAttrName, inFuncPtr, inDataType, inPermission);	
	return QTSS_NoErr;
}

void QTSSDictionaryMap::SetAttribute(	QTSS_AttributeID inID, 
										const char* inAttrName,
										QTSS_AttrFunctionPtr inFuncPtr,
										QTSS_AttrDataType inDataType,
										QTSS_AttrPermission inPermission )
{
	UInt32 theIndex = QTSSDictionaryMap::ConvertAttrIDToArrayIndex(inID);
	UInt32 theNameLen = ::strlen(inAttrName);
	Assert(theNameLen < QTSS_MAX_ATTRIBUTE_NAME_SIZE);
	Assert(fAttrArray[theIndex] == NULL);
	
	fAttrArray[theIndex] = NEW QTSSAttrInfoDict;
	
	//Copy the information into the first available element
	fAttrArray[theIndex]->fID = inID;
		
	::strcpy(&fAttrArray[theIndex]->fAttrInfo.fAttrName[0], inAttrName);
	fAttrArray[theIndex]->fAttrInfo.fFuncPtr = inFuncPtr;
	fAttrArray[theIndex]->fAttrInfo.fAttrDataType = inDataType;	
	fAttrArray[theIndex]->fAttrInfo.fAttrPermission = inPermission;
	
	fAttrArray[theIndex]->SetVal(qtssAttrName, &fAttrArray[theIndex]->fAttrInfo.fAttrName[0], theNameLen);
	fAttrArray[theIndex]->SetVal(qtssAttrID, &fAttrArray[theIndex]->fID, sizeof(fAttrArray[theIndex]->fID));
	fAttrArray[theIndex]->SetVal(qtssAttrDataType, &fAttrArray[theIndex]->fAttrInfo.fAttrDataType, sizeof(fAttrArray[theIndex]->fAttrInfo.fAttrDataType));
	fAttrArray[theIndex]->SetVal(qtssAttrPermissions, &fAttrArray[theIndex]->fAttrInfo.fAttrPermission, sizeof(fAttrArray[theIndex]->fAttrInfo.fAttrPermission));
}

QTSS_Error	QTSSDictionaryMap::RemoveAttribute(QTSS_AttributeID inAttrID)
{
	SInt32 theIndex = this->ConvertAttrIDToArrayIndex(inAttrID);
	if (theIndex < 0)
		return QTSS_AttrDoesntExist;
	
	Assert(fFlags & kAllowRemoval);
	if (!(fFlags & kAllowRemoval))
		return QTSS_BadArgument;
	
	//printf("QTSSDictionaryMap::RemoveAttribute arraySize=%lu numNonRemove= %lu fAttrArray[%lu]->fAttrInfo.fAttrName=%s\n",this->GetNumAttrs(), this->GetNumNonRemovedAttrs(), theIndex,fAttrArray[theIndex]->fAttrInfo.fAttrName);
	//
	// Don't actually touch the attribute or anything. Just flag the
	// it as removed.
	fAttrArray[theIndex]->fAttrInfo.fAttrPermission |= qtssPrivateAttrModeRemoved;
	fNumValidAttrs--;
	Assert(fNumValidAttrs < 1000000);
	return QTSS_NoErr;
}

QTSS_Error	QTSSDictionaryMap::UnRemoveAttribute(QTSS_AttributeID inAttrID)
{
	if (this->ConvertAttrIDToArrayIndex(inAttrID) == -1)
		return QTSS_AttrDoesntExist;
	
	SInt32 theIndex = this->ConvertAttrIDToArrayIndex(inAttrID);
	if (theIndex < 0)
		return QTSS_AttrDoesntExist;
		
	fAttrArray[theIndex]->fAttrInfo.fAttrPermission &= ~qtssPrivateAttrModeRemoved;
	
	fNumValidAttrs++;
	return QTSS_NoErr;
}

QTSS_Error	QTSSDictionaryMap::GetAttrInfoByName(const char* inAttrName, QTSSAttrInfoDict** outAttrInfoObject,
													Bool16 returnRemovedAttr)
{
	if (outAttrInfoObject == NULL)
		return QTSS_BadArgument;

	for (UInt32 count = 0; count < fNextAvailableID; count++)
	{
		if (::strcmp(&fAttrArray[count]->fAttrInfo.fAttrName[0], inAttrName) == 0)
		{
			if ((fAttrArray[count]->fAttrInfo.fAttrPermission & qtssPrivateAttrModeRemoved) && (!returnRemovedAttr))
				continue;
				
			*outAttrInfoObject = fAttrArray[count];
			return QTSS_NoErr;
		}	
	}
	return QTSS_AttrDoesntExist;
}

QTSS_Error	QTSSDictionaryMap::GetAttrInfoByID(QTSS_AttributeID inID, QTSSAttrInfoDict** outAttrInfoObject)
{
	if (outAttrInfoObject == NULL)
		return QTSS_BadArgument;

	SInt32 theIndex = this->ConvertAttrIDToArrayIndex(inID);
	if (theIndex < 0)
		return QTSS_AttrDoesntExist;
	
	if (fAttrArray[theIndex]->fAttrInfo.fAttrPermission & qtssPrivateAttrModeRemoved)
		return QTSS_AttrDoesntExist;
		
	*outAttrInfoObject = fAttrArray[theIndex];
	return QTSS_NoErr;
}

QTSS_Error	QTSSDictionaryMap::GetAttrInfoByIndex(UInt32 inIndex, QTSSAttrInfoDict** outAttrInfoObject)
{
	if (outAttrInfoObject == NULL)
		return QTSS_BadArgument;
	if (inIndex >= this->GetNumNonRemovedAttrs())
		return QTSS_AttrDoesntExist;
		
	UInt32 actualIndex = inIndex;
	UInt32 max = this->GetNumAttrs();
	if (fFlags & kAllowRemoval)
	{
		// If this dictionary map allows attributes to be removed, then
		// the iteration index and array indexes won't line up exactly, so
		// we have to iterate over the whole map all the time
		actualIndex = 0;
		for (UInt32 x = 0; x < max; x++)
		{	if (fAttrArray[x] && (fAttrArray[x]->fAttrInfo.fAttrPermission & qtssPrivateAttrModeRemoved) )
			{	continue;
			}
				
			if (actualIndex == inIndex)
			{	actualIndex = x;
				break;
			}
			actualIndex++;
		}
	}
	//printf("QTSSDictionaryMap::GetAttrInfoByIndex arraySize=%lu numNonRemove= %lu fAttrArray[%lu]->fAttrInfo.fAttrName=%s\n",this->GetNumAttrs(), this->GetNumNonRemovedAttrs(), actualIndex,fAttrArray[actualIndex]->fAttrInfo.fAttrName);
	Assert(actualIndex < fNextAvailableID);
	Assert(!(fAttrArray[actualIndex]->fAttrInfo.fAttrPermission & qtssPrivateAttrModeRemoved));
	*outAttrInfoObject = fAttrArray[actualIndex];
	return QTSS_NoErr;
}

QTSS_Error	QTSSDictionaryMap::GetAttrID(const char* inAttrName, QTSS_AttributeID* outID)
{
	if (outID == NULL)
		return QTSS_BadArgument;

	QTSSAttrInfoDict* theAttrInfo = NULL;
	QTSS_Error theErr = this->GetAttrInfoByName(inAttrName, &theAttrInfo);
	if (theErr == QTSS_NoErr)
		*outID = theAttrInfo->fID;

	return theErr;
}

UInt32	QTSSDictionaryMap::GetMapIndex(QTSS_ObjectType inType)
{
	 switch (inType)
	 {
	 	case qtssRTPStreamObjectType:		return kRTPStreamDictIndex;
	 	case qtssClientSessionObjectType:	return kClientSessionDictIndex;
	 	case qtssRTSPSessionObjectType:		return kRTSPSessionDictIndex;
	 	case qtssRTSPRequestObjectType:		return kRTSPRequestDictIndex;
	 	case qtssRTSPHeaderObjectType:		return kRTSPHeaderDictIndex;
	 	case qtssServerObjectType:			return kServerDictIndex;
	 	case qtssPrefsObjectType:			return kPrefsDictIndex;
	 	case qtssTextMessagesObjectType:	return kTextMessagesDictIndex;
	 	case qtssFileObjectType:			return kFileDictIndex;
	 	case qtssModuleObjectType:			return kModuleDictIndex;
	 	case qtssModulePrefsObjectType:		return kModulePrefsDictIndex;
	 	case qtssAttrInfoObjectType:		return kAttrInfoDictIndex;
	 	case qtssUserProfileObjectType:		return kQTSSUserProfileDictIndex;
	 	default:							return kIllegalDictionary;
	 }
	 return kIllegalDictionary;
}

