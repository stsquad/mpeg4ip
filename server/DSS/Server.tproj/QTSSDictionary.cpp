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
	fMap(inMap), fInstanceMap(NULL), fMutexP(inMutex), fInstanceAttrsAllowed(false)
{
	fAttributes = NEW DictValueElement[inMap->GetNumAttrs()];
}

QTSSDictionary::~QTSSDictionary()
{
	delete [] fAttributes;
	delete fInstanceMap;
	delete [] fInstanceAttrs;
}

inline void QTSSDictionary::SetFunctionTypeParams(QTSS_FunctionParams *funcParamsPtr, QTSS_AttributeFuncSelector selector, QTSS_AttributeID inAttrID, UInt32 inIndex)
{
	memset(&funcParamsPtr->io,0,sizeof(funcParamsPtr->io));
	
	funcParamsPtr->selector 		= selector;
	funcParamsPtr->object 			= (QTSS_Object*) this;
	funcParamsPtr->attributeID 		= inAttrID;
	funcParamsPtr->attributeIndex 	= inIndex;
	funcParamsPtr->handledError		= QTSS_NoErr;
}

											
Bool16  QTSSDictionary::GetSetValuePtrFunc(QTSS_AttrParamFunctionPtr theFuncPtr,QTSS_AttributeFuncSelector selector,QTSS_AttributeID inAttrID, UInt32 inIndex, void** ioValueBuffer, UInt32* ioValueLen, QTSS_Error* handledErrorPtr)
{
	Assert(handledErrorPtr);

	if (NULL == theFuncPtr)
		return false;

	QTSS_FunctionParams funcParams;
	SetFunctionTypeParams(&funcParams,selector,inAttrID, inIndex);
	funcParams.io.bufferPtr = *ioValueBuffer;
	funcParams.io.valueLen = *ioValueLen;
		
	Bool16 handled = theFuncPtr(&funcParams);
	if (handled)
	{	*ioValueBuffer		= funcParams.io.bufferPtr;
		*ioValueLen			= funcParams.io.valueLen;
		*handledErrorPtr 	= funcParams.handledError;
	}
		
	return handled;
}

Bool16  QTSSDictionary::GetSetNumValuePtrFunc(QTSS_AttrParamFunctionPtr theFuncPtr,QTSS_AttributeFuncSelector selector,QTSS_AttributeID inAttrID, UInt32 inIndex, UInt32 *numValuesPtr, UInt32* ioValueLenPtr, QTSS_Error* handledErrorPtr)
{
	Assert(handledErrorPtr);

	if (NULL == theFuncPtr)
		return false;

	QTSS_FunctionParams funcParams;
	SetFunctionTypeParams(&funcParams,selector,inAttrID, inIndex);
	funcParams.io.bufferPtr = numValuesPtr;
	funcParams.io.valueLen = *ioValueLenPtr;
		
	Bool16 handled = theFuncPtr(&funcParams);
	if (handled)
	{	*numValuesPtr		= *(UInt32 *)funcParams.io.bufferPtr;
		*ioValueLenPtr		= funcParams.io.valueLen;
		*handledErrorPtr 	= funcParams.handledError;
	}
	
	return handled;
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
	if ((outValueBuffer == NULL) || (outValueLen == NULL))
		return QTSS_BadArgument;
	if (theMap->IsRemoved(theMapIndex))
		return QTSS_AttrDoesntExist;
	if ((!isInternal) && (!theMap->IsPreemptiveSafe(theMapIndex)))
		return QTSS_NotPreemptiveSafe;

	UInt32 numValues = this->GetNumValues(inAttrID); // Call internal routine to allow for attribute functions. We do this before the remove in case it causes an internal counter to decrement

	if (inIndex > (SInt32) numValues -1)
		return QTSS_BadIndex;
	
	// Retrieve the attribute
	char *buffer = theAttrs[theMapIndex].fAttributeData.Ptr;
	buffer += theAttrs[theMapIndex].fAttributeData.Len * inIndex;
	*outValueLen = theAttrs[theMapIndex].fAttributeData.Len;
	*outValueBuffer = buffer;
	
	
	QTSS_Error handledError = QTSS_NoErr;
	QTSS_AttrParamFunctionPtr funcPtr = (QTSS_AttrParamFunctionPtr)theMap->GetAttrFunction(theMapIndex);
	Bool16 handled = false;
	if (funcPtr != NULL)
		handled = this->GetSetValuePtrFunc(funcPtr, qtssGetValuePtrEnterFunc, inAttrID, inIndex, outValueBuffer, outValueLen,&handledError);
		
	if (handled && handledError != QTSS_NoErr)
		return handledError;
			
	if (!handled)
	{
		// Retrieve the attribute again in case it was set during the callback but not marked handled
		buffer = theAttrs[theMapIndex].fAttributeData.Ptr;
		buffer += theAttrs[theMapIndex].fAttributeData.Len * inIndex;
		*outValueLen = theAttrs[theMapIndex].fAttributeData.Len;
		*outValueBuffer = buffer;
	}
#if DEBUG
	else
		// Make sure we aren't outside the bounds of attribute memory
		Assert(theAttrs[theMapIndex].fAllocatedLen >=
			(theAttrs[theMapIndex].fAttributeData.Len * (theAttrs[theMapIndex].fNumAttributes + 1)));

#endif

	if (funcPtr != NULL)
		handled = this->GetSetValuePtrFunc(funcPtr, qtssGetValuePtrExitFunc, inAttrID, inIndex, outValueBuffer, outValueLen,&handledError);

	if (handled && handledError != QTSS_NoErr)
		return handledError;
		
	// Return an error if there is no data for this attribute
	if (*outValueLen == 0)
		return QTSS_ValueNotFound;
			
	return QTSS_NoErr;
}



QTSS_Error QTSSDictionary::GetValue(QTSS_AttributeID inAttrID, UInt32 inIndex,
											void* ioValueBuffer, UInt32* ioValueLen)
{

	if (ioValueLen == NULL)
		return QTSS_BadArgument;
	
	// If there is a mutex, lock it and get a pointer to the proper attribute
	OSMutexLocker locker(fMutexP);

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
	
	//
	// Make sure this is a real attribute
	if (theMapIndex < 0)
		return QTSS_AttrDoesntExist;
	if (theMap->IsRemoved(theMapIndex))
		return QTSS_AttrDoesntExist;

	QTSS_Error handledError = QTSS_NoErr;
	QTSS_AttrParamFunctionPtr funcPtr = (QTSS_AttrParamFunctionPtr)theMap->GetAttrFunction(theMapIndex);
	Bool16 handled = false;
	if (funcPtr != NULL)
		handled = this->GetSetValuePtrFunc(funcPtr, qtssGetValueEnterFunc, inAttrID, inIndex, &ioValueBuffer, ioValueLen,&handledError);

	if (handled && handledError != QTSS_NoErr)
		return handledError;
	
	if (!handled)
	{
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
	}
	
	if (funcPtr != NULL)
		handled = this->GetSetValuePtrFunc(funcPtr, qtssGetValueExitFunc, inAttrID, inIndex, &ioValueBuffer, ioValueLen,&handledError);

	if (handled && handledError != QTSS_NoErr)
		return handledError;


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
	
	*outString = QTSSDataConverter::ConvertTypeToString(tempValueBuffer, tempValueLen, theMap->GetAttrType(theMapIndex));
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
	if ((inBuffer == NULL) && (inLen > 0))
		return QTSS_BadArgument;
	
	SInt32 numValues = this->GetNumValues(inAttrID); // Call internal routine to allow for attribute functions. We do this before the remove in case it causes an internal counter to decrement

	QTSS_Error handledError = QTSS_NoErr;
	void *attributeBufferPtr = (void *) inBuffer;
	Bool16 handled = this->GetSetValuePtrFunc((QTSS_AttrParamFunctionPtr)theMap->GetAttrFunction(theMapIndex), qtssSetValueEnterFunc, inAttrID, inIndex, &attributeBufferPtr, &inLen,&handledError);
	if (handled && handledError != QTSS_NoErr)
		return handledError;

	if (!handled)
	{
		// If this attribute is iterated, this new value
		// must be the same size as all the others.
		if (((SInt32)(numValues -1)  > 0) &&
			(inLen != theAttrs[theMapIndex].fAttributeData.Len))
			return QTSS_BadArgument;
		
		//
		// Can't put empty space into the array of values
		if ((theAttrs[theMapIndex].fAttributeData.Len == 0) && (inIndex > 0))
			return QTSS_BadIndex;

		if ((SInt32) inIndex > (SInt32) numValues)
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
		if ( (SInt32) inIndex > (SInt32) ( (SInt32) numValues -1))
			this->SetNumValues(inAttrID,inIndex +1);

		// Copy the new data to the right place in our data buffer
		attributeBufferPtr = theAttrs[theMapIndex].fAttributeData.Ptr + (inLen * inIndex);
		::memcpy(attributeBufferPtr, inBuffer, inLen);
		theAttrs[theMapIndex].fAttributeData.Len = inLen;
	}

	handled = this->GetSetValuePtrFunc((QTSS_AttrParamFunctionPtr) theMap->GetAttrFunction(theMapIndex), qtssSetValueExitFunc, inAttrID, inIndex, &attributeBufferPtr, &inLen,&handledError);

	//
	// Call the completion routine
	if (!(inFlags & kDontCallCompletionRoutine))
		this->SetValueComplete(theMapIndex, theMap, inIndex, attributeBufferPtr, inLen);

	if (handled && handledError != QTSS_NoErr)
		return handledError;
	
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
		
	SInt32 numValues = (SInt32) this->GetNumValues(inAttrID); // Call internal routine to allow for attribute functions. We do this before the remove in case it causes an internal counter to decrement

	UInt32 theValueLen = theAttrs[theMapIndex].fAttributeData.Len;
	void *attributePtr = theAttrs[theMapIndex].fAttributeData.Ptr + (theValueLen * inIndex);
	
	QTSS_Error handledError = QTSS_NoErr;
	QTSS_AttrParamFunctionPtr funcPtr = (QTSS_AttrParamFunctionPtr)theMap->GetAttrFunction(theMapIndex);
	Bool16 handled = false;
	
	if (NULL!=funcPtr)
		handled = this->GetSetValuePtrFunc(funcPtr, qtssRemoveValueEnterFunc, inAttrID, inIndex, &attributePtr, &theValueLen,&handledError);
	
	if (handled && handledError != QTSS_NoErr)
		return handledError;
	
	if (!handled)
	{
		//
		// If there are values after this one in the array, move them.
		::memmove(	theAttrs[theMapIndex].fAttributeData.Ptr + (theValueLen * inIndex),
					theAttrs[theMapIndex].fAttributeData.Ptr + (theValueLen * (inIndex + 1)),
					theValueLen * ( (theAttrs[theMapIndex].fNumAttributes) - inIndex));
	}
		
	//
	// Update our number of attributes. fNumAttributes is not really the number of attributes,
	// it is # - 1, so if it is 0, set the real # of attributes to 0 by setting the data.Len to 0.
	// Confusing?
	this->SetNumValues(inAttrID, numValues -1); // // Call internal routine to allow for attribute functions. The count is either 1 less than before the remove or the same as the counter if it was auto decremented by the remove

	if (NULL!=funcPtr)
		handled = this->GetSetValuePtrFunc(funcPtr, qtssRemoveValueExitFunc, inAttrID, inIndex, NULL, 0,&handledError);

	//
	// Call the completion routine
	if (!(inFlags & kDontCallCompletionRoutine))
		this->RemoveValueComplete(theMapIndex, theMap, inIndex);
		
	if (handled && handledError != QTSS_NoErr)
		return handledError;

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
	
	SInt32 theMapIndex = theMap->ConvertAttrIDToArrayIndex(inAttrID);
	if (theMapIndex < 0)
		return 0;
	
	UInt32	numValues = 0; 
	UInt32  numValuesLen = sizeof(UInt32);
	
	QTSS_Error handledError = QTSS_NoErr;
	QTSS_AttrParamFunctionPtr funcPtr = (QTSS_AttrParamFunctionPtr)theMap->GetAttrFunction(theMapIndex);
	Bool16 handled = false;
	
	if (NULL!=funcPtr)
		handled = GetSetNumValuePtrFunc(funcPtr, qtssGetNumValuesEnterFunc, inAttrID, 0, &numValues, &numValuesLen,&handledError);
	
	if (!handled)
	{
		if (theAttrs[theMapIndex].fAttributeData.Len == 0)
			numValues = 0;
		else
			numValues = theAttrs[theMapIndex].fNumAttributes + 1;
	}
	
	if (NULL!=funcPtr)
		handled = GetSetNumValuePtrFunc(funcPtr, qtssGetNumValuesExitFunc, inAttrID, 0,  &numValues, &numValuesLen,&handledError);

	return numValues;
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

	UInt32 *numValuesPtr = &inNumValues;
	UInt32  numValuesLen = sizeof(inNumValues);
	
	QTSS_Error handledError = QTSS_NoErr;
	QTSS_AttrParamFunctionPtr funcPtr = (QTSS_AttrParamFunctionPtr)theMap->GetAttrFunction(theMapIndex);
	Bool16 handled = false;
	if (funcPtr != NULL)
		handled = this->GetSetValuePtrFunc(funcPtr, qtssSetNumValuesEnterFunc, inAttrID, 0,(void **) &numValuesPtr, &numValuesLen,&handledError);
	if (handled)
		return;
	
	if (!handled)
	{

		if (inNumValues == 0)
			theAttrs[theMapIndex].fAttributeData.Len = 0;
		else
		{	theAttrs[theMapIndex].fNumAttributes = inNumValues -1;
		}
	}
	
	if (funcPtr != NULL)
		(void) this->GetSetValuePtrFunc(funcPtr, qtssSetNumValuesExitFunc, inAttrID, 0,(void **) &numValuesPtr, &numValuesLen,&handledError);
	
	
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
	Assert( ((UInt32) inBuf % 4) == 0 );
#endif

}


QTSS_Error	QTSSDictionary::AddInstanceAttribute(	const char* inAttrName,
													QTSS_AttrParamFunctionPtr inFuncPtr,
													QTSS_AttrDataType inDataType,
													QTSS_AttrPermission inPermission )
{
	if (!fInstanceAttrsAllowed)
		return QTSS_InstanceAttrsNotAllowed;
		
	OSMutexLocker locker(fMutexP);

	if (fInstanceMap == NULL)
		fInstanceMap = new QTSSDictionaryMap( 0,	QTSSDictionaryMap::kAllowRemoval |
													QTSSDictionaryMap::kIsInstanceMap );
	
	//
	// Add the attribute into the Dictionary Map.
	QTSS_Error theErr = fInstanceMap->AddAttribute(inAttrName, inFuncPtr, inDataType, inPermission);
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

	sDictionaryMaps[kServerDictIndex] 		= new QTSSDictionaryMap(qtssSvrNumParams);
	sDictionaryMaps[kPrefsDictIndex] 		= new QTSSDictionaryMap(qtssPrefsNumParams);
	sDictionaryMaps[kTextMessagesDictIndex] = new QTSSDictionaryMap(qtssMsgNumParams);
	sDictionaryMaps[kServiceDictIndex] 		= new QTSSDictionaryMap(0);
	sDictionaryMaps[kRTPStreamDictIndex] 	= new QTSSDictionaryMap(qtssRTPStrNumParams);
	sDictionaryMaps[kClientSessionDictIndex]= new QTSSDictionaryMap(qtssCliSesNumParams);
	sDictionaryMaps[kRTSPSessionDictIndex] 	= new QTSSDictionaryMap(qtssRTSPSesNumParams);
	sDictionaryMaps[kRTSPRequestDictIndex] 	= new QTSSDictionaryMap(qtssRTSPReqNumParams);
	sDictionaryMaps[kRTSPHeaderDictIndex] 	= new QTSSDictionaryMap(qtssNumHeaders);
	sDictionaryMaps[kFileDictIndex] 		= new QTSSDictionaryMap(qtssFlObjNumParams);
	sDictionaryMaps[kModuleDictIndex] 		= new QTSSDictionaryMap(qtssModNumParams);
	sDictionaryMaps[kModulePrefsDictIndex] 	= new QTSSDictionaryMap(0);

}

QTSSDictionaryMap::QTSSDictionaryMap(UInt32 inNumReservedAttrs, UInt32 inFlags)
: 	fNextAvailableID(inNumReservedAttrs), fNumValidAttrs(inNumReservedAttrs),fAttrArraySize(inNumReservedAttrs), fFlags(inFlags)
{
	if (fAttrArraySize < kMinArraySize)
		fAttrArraySize = kMinArraySize;
	fAttrArray = NEW QTSSAttrInfoDict*[fAttrArraySize];
	::memset(fAttrArray, 0, sizeof(QTSSAttrInfoDict*) * fAttrArraySize);
}

QTSS_Error QTSSDictionaryMap::TestAttributeExistsByName(const char* inAttrName)
{
	if (inAttrName == NULL || ::strlen(inAttrName) > QTSS_MAX_ATTRIBUTE_NAME_SIZE)
		return QTSS_BadArgument;
	
	for (UInt32 count = 0; count < fNextAvailableID; count++)
	{
		if ( fAttrArray[count]->fAttrInfo.fAttrPermission & qtssPrivateAttrModeRemoved )
			continue;

		if (::strcmp(&fAttrArray[count]->fAttrInfo.fAttrName[0], inAttrName) == 0)
			return QTSS_AttrNameExists;
	}
	
	return QTSS_AttrDoesntExist;
}

QTSS_Error QTSSDictionaryMap::TestAttributeExistsByID(QTSS_AttributeID inID)
{
	SInt32 theIndex = this->ConvertAttrIDToArrayIndex(inID);
	if (theIndex < 0)
		return QTSS_AttrDoesntExist;
		
	if (this->IsRemoved(theIndex))
		return QTSS_AttrDoesntExist;
		
	return QTSS_AttrNameExists;
}

QTSS_Error QTSSDictionaryMap::AddAttribute(	const char* inAttrName,
											QTSS_AttrParamFunctionPtr inFuncPtr,
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
										QTSS_AttrParamFunctionPtr inFuncPtr,
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
	UInt32 max = this->GetNumNonRemovedAttrs();
	if (fFlags & kAllowRemoval)
	{
		// If this dictionary map allows attributes to be removed, then
		// the iteration index and array indexes won't line up exactly, so
		// we have to iterate over the whole map all the time
		actualIndex = inIndex + 1;
		for (UInt32 x = 0; x < max; x++)
		{	if (fAttrArray[x] && !(fAttrArray[x]->fAttrInfo.fAttrPermission & qtssPrivateAttrModeRemoved) )
			{	actualIndex --;
			}
			
			if (actualIndex == 0)
			{	actualIndex = x;
				break;
			}
		}
			
	}
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

SInt32	QTSSDictionaryMap::ConvertAttrIDToArrayIndex(QTSS_AttributeID inAttrID)
{
	SInt32 theIndex = inAttrID & 0x7FFFFFFF;
	if ((theIndex < 0) || (theIndex >= (SInt32)fNextAvailableID))
		return -1;
	else
		return theIndex;
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
	 	default:							return kIllegalDictionary;
	 }
	 return kIllegalDictionary;
}


#if __DICTIONARY_TESTING__
static void* TestAttrFunc(QTSS_Object inServer, UInt32* outLen);
static void* TestAttrFunc2(QTSS_Object inServer, UInt32* outLen);

void* TestAttrFunc(QTSS_Object inServer, UInt32* outLen)
{
	return NULL;
}
void* TestAttrFunc2(QTSS_Object inServer, UInt32* outLen)
{
	return NULL;
}

void QTSSDictionary::Test()
{
	QTSSDictionaryMap theMap(0);
	Assert(theMap.GetNumAttrs() == 0);
	UInt32 id1;
	QTSS_Error theErr = theMap.AddAttribute("foo", NULL, true, &id1);
	Assert(theErr == QTSS_NoErr);
	UInt32 id2;
	theErr = theMap.AddAttribute("foo2", TestAttrFunc, false, &id2);
	Assert(theErr == QTSS_NoErr);
	UInt32 id3;
	theErr = theMap.AddAttribute("foo3", TestAttrFunc2, false, &id3);
	Assert(theErr == QTSS_NoErr);
	UInt32 id4;
	theErr = theMap.AddAttribute("foo4", TestAttrFunc2, false, &id4);
	Assert(theErr == QTSS_NoErr);
	UInt32 id5;
	theErr = theMap.AddAttribute("foo4dfdsfdfaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", TestAttrFunc2, false, &id5);
	Assert(theErr == QTSS_BadArgument);
	theErr = theMap.AddAttribute("foo4dfdsfdfaaaaaaaaaaaaaa", TestAttrFunc2, false, NULL);
	Assert(theErr == QTSS_BadArgument);
	UInt32 id6;
	theErr = theMap.AddAttribute("foo6", NULL, false, &id6);
	Assert(theErr == QTSS_NoErr);	
	theMap.SetAttribute(id4, "foo7", NULL, false, false);
	Assert(theMap.GetNumAttrs() == 5);
	
	QTSSDictionary theDict(&theMap);
	char boofer[100];
	UInt32 len = 0;
	
	theErr = theDict.SetValue(id2, 2, "poopie", 6);
	Assert(theErr == QTSS_BadIndex);
	theErr = theDict.SetValue(id2, 0, "poopie", 6);
	Assert(theErr == QTSS_NoErr);
	theErr = theDict.SetValue(id2, 0, "heinosity", 9);
	Assert(theErr == QTSS_NoErr);
	theErr = theDict.SetValue(id2, 0, "poopieheinosity", 15);
	Assert(theErr == QTSS_NoErr);
	
	theErr = theDict.GetValue(342, 3, (void**)NULL, (UInt32*)NULL);
	Assert(theErr == QTSS_BadArgument);
	theErr = theDict.GetValue(-2, 0, boofer, &len);
	Assert(theErr == QTSS_BadArgument);
	theErr = theDict.GetValue(-1, 0, boofer, &len);
	Assert(theErr == QTSS_BadArgument);

	theErr = theDict.GetValue(id1, 0, boofer, &len);
	Assert(theErr == QTSS_BadIndex);
	theErr = theDict.SetValue(id1, 2, "foop", 4);
	Assert(theErr == QTSS_NoErr);
	theErr = theDict.SetValue(id1, 1, "foopie", 6);
	Assert(theErr == QTSS_BadArgument);
	theErr = theDict.SetValue(id1, 0, "foopie", 6);
	Assert(theErr == QTSS_BadArgument);
	theErr = theDict.SetValue(id4, 0, "foopie", 6);
	Assert(theErr == EPERM);
	theErr = theDict.SetValue(id1, 0, "doop", 4);
	Assert(theErr == QTSS_NoErr);
	
	UInt32 theLen = 0;
	char* theBuffer = NULL;
	theErr = theDict.GetValue(id1, 3, (void**)&theBuffer, (UInt32*)&theLen);
	Assert(theErr == QTSS_BadIndex);
	theErr = theDict.GetValue(id1, 1, (void**)&theBuffer, (UInt32*)&theLen);
	Assert(theErr == QTSS_NoErr);
	theErr = theDict.GetValue(id1, 0, (void**)&theBuffer, (UInt32*)&theLen);
	Assert(theErr == QTSS_NoErr);
	Assert(theLen == 4);
	Assert(::strncmp(theBuffer, "doop", 4) == 0);
	theErr = theDict.GetValue(id1, 2, (void**)&theBuffer, (UInt32*)&theLen);
	Assert(theErr == QTSS_NoErr);
	Assert(theLen == 4);
	Assert(::strncmp(theBuffer, "foop", 4) == 0);
	
	char theBuf[10];
	theLen = 3;
	theErr = theDict.GetValue(id1, 2, (void*)theBuf, (UInt32*)&theLen);
	Assert(theErr == E2BIG);
	Assert(theLen == 4);
	theErr = theDict.GetValue(id1, 2, (void*)NULL, (UInt32*)&theLen);
	Assert(theErr == QTSS_NoErr);
	Assert(theLen == 4);
	theErr = theDict.GetValue(id1, 2, (void*)theBuf, (UInt32*)NULL);
	Assert(theErr == QTSS_BadArgument);
	theLen = 6;
	theErr = theDict.GetValue(id1, 2, theBuf, &theLen);
	Assert(theErr == QTSS_NoErr);
	Assert(theLen == 4);
	Assert(::strncmp(theBuf, "foop", 4) == 0);
	theErr = theDict.GetValue(id1, 0, theBuf, &theLen);
	Assert(theErr == QTSS_NoErr);
	Assert(theLen == 4);
	Assert(::strncmp(theBuf, "doop", 4) == 0);

	theErr = theDict.GetValue(id2, 1, theBuf, &theLen);
	Assert(theErr == QTSS_BadIndex);

	theLen = 15;
	theErr = theDict.GetValue(id2, 0, theBuf, &theLen);
	Assert(theErr == QTSS_NoErr);
	Assert(theLen == 15);
	Assert(::strncmp(theBuf, "poopieheinosity", 15) == 0);

	theErr = theDict.GetValue(id2, 0, (void**)&theBuffer, (UInt32*)&theLen);
	Assert(theErr == EPERM);
}
#endif
