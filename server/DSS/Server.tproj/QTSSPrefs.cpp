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
	File:		QTSSPrefs.cpp

	Contains:	Implements class defined in QTSSPrefs.h.

	Change History (most recent first):
	
*/

#include "QTSSPrefs.h"
#include "MyAssert.h"
#include "OSMemory.h"
#include "QTSSDataConverter.h"
#include "OSArrayObjectDeleter.h"
 

QTSSPrefs::QTSSPrefs(XMLPrefsParser* inPrefsSource, StrPtrLen* inModuleName, QTSSDictionaryMap* inMap,
						Bool16 areInstanceAttrsAllowed)
:	QTSSDictionary(inMap, &fPrefsMutex),
	fPrefsSource(inPrefsSource),
	fModuleName(NULL)
{
	this->SetInstanceAttrsAllowed(areInstanceAttrsAllowed);
	
	if (inModuleName != NULL)
		fModuleName = inModuleName->GetAsCString();
}

void QTSSPrefs::RereadPreferences()
{
	QTSS_Error theErr = QTSS_NoErr;
	
	//
	// Keep track of which pref attributes should remain. All others
	// will be removed
	UInt32 initialNumAttrs = 0;
	if (this->GetInstanceDictMap() != NULL)
	{	initialNumAttrs = this->GetInstanceDictMap()->GetNumNonRemovedAttrs();
	};
	
	Bool16* foundThisPref = NEW Bool16[initialNumAttrs];
	::memset(foundThisPref, 0, sizeof(Bool16) * initialNumAttrs);
	
	UInt32 theNumPrefs = fPrefsSource->GetNumPrefsByModule(fModuleName);
	UInt32 theBaseIndex = fPrefsSource->GetBaseIndexForModule(fModuleName);
	
	OSMutexLocker locker(&fPrefsMutex);
	
	// Use the names of the attributes in the attribute map as the key values for
	// finding preferences in the config file.
	
	for (UInt32 x = theBaseIndex; x < theNumPrefs + theBaseIndex; x++)
	{
		char* thePrefTypeStr = NULL;
		char* thePrefName = NULL;
		(void)fPrefsSource->GetPrefValueByIndex(x, 0, &thePrefName, &thePrefTypeStr);
		
		//
		// What type is this data type?
		QTSS_AttrDataType thePrefType = QTSSDataConverter::GetDataTypeForTypeString(thePrefTypeStr);

		//
		// Check to see if there is an attribute with this name already in the
		// instance map. If one matches, then we don't need to add this attribute.
		QTSSAttrInfoDict* theAttrInfo = NULL;
		if (this->GetInstanceDictMap() != NULL)
			(void)this->GetInstanceDictMap()->GetAttrInfoByName(thePrefName,
																&theAttrInfo,
																true );
		UInt32 theLen = sizeof(QTSS_AttrDataType);
		QTSS_AttributeID theAttrID = qtssIllegalAttrID;
		
		if ( theAttrInfo == NULL )
		{
			theAttrID = this->AddPrefAttribute(thePrefName, thePrefType);
			this->SetPrefValuesFromFile(x, theAttrID, 0);
		}
		else
		{
			QTSS_AttrDataType theAttrType = qtssAttrDataTypeUnknown;
			theErr = theAttrInfo->GetValue(qtssAttrDataType, 0, &theAttrType, &theLen);
			Assert(theErr == QTSS_NoErr);
			
			theLen = sizeof(theAttrID);
			theErr = theAttrInfo->GetValue(qtssAttrID, 0, &theAttrID, &theLen);
			Assert(theErr == QTSS_NoErr);

			if (theAttrType != thePrefType)
			{
				//
				// This is not the same pref as before, because the data types
				// are different. Remove the old one from the map, add the new one.
				(void)this->RemoveInstanceAttribute(theAttrID);
				theAttrID = this->AddPrefAttribute(thePrefName, thePrefType);
			}
			else
			{
				//
				// This pref already exists
			}
			
			//
			// Set the values
			this->SetPrefValuesFromFile(x, theAttrID, 0);

			// Mark this pref as found.
			SInt32 theIndex = this->GetInstanceDictMap()->ConvertAttrIDToArrayIndex(theAttrID);
			Assert(theIndex >= 0);
			foundThisPref[theIndex] = true;
		}
	}
	
	// Remove all attributes that no longer apply
	if (this->GetInstanceDictMap() != NULL && initialNumAttrs > 0)
	{	for (SInt32 a = initialNumAttrs -1; a >= 0; a--) // count down because we are using an index to delete.
		{
			if (!foundThisPref[a])	
			{	QTSSAttrInfoDict* theAttrInfoPtr = NULL;
				theErr = this->GetInstanceDictMap()->GetAttrInfoByIndex(a, &theAttrInfoPtr);
				Assert(theErr == QTSS_NoErr);
				if (theErr != QTSS_NoErr) continue;
		
				UInt32 theLen = sizeof(QTSS_AttrDataType);
				QTSS_AttributeID theAttrID = qtssIllegalAttrID;
					
				theLen = sizeof(theAttrID);
				theErr = theAttrInfoPtr->GetValue(qtssAttrID, 0, &theAttrID, &theLen);
				Assert(theErr == QTSS_NoErr);
				if (theErr != QTSS_NoErr) continue;
							
				if(!this->GetInstanceDictMap()->IsRemoved(a))
				{	this->GetInstanceDictMap()->RemoveAttribute(theAttrID);
				}
			}
		}
	}
	
	delete [] foundThisPref;
}
		
void QTSSPrefs::SetPrefValuesFromFile(UInt32 inPrefIndex, QTSS_AttributeID inAttrID, UInt32 inNumValues)
{
	//
	// We have an attribute ID for this pref, it is in the map and everything.
	// Now, let's add all the values that are in the pref file.

	UInt32 numPrefValues = inNumValues;
	if (inNumValues == 0)
		numPrefValues = fPrefsSource->GetNumPrefValuesByIndex( inPrefIndex );
		
	UInt32 maxPrefValueSize = 0;
	char* thePrefTypeStr = NULL;
	char* thePrefName = NULL;
	char* thePrefValue = NULL;
	QTSS_Error theErr = QTSS_NoErr;
	
	//
	// We have to loop through all the values associated with this pref twice:
	// first, to figure out the length (in bytes) of the longest value, secondly
	// to actually copy these values into the dictionary.
	
	QTSS_AttrDataType thePrefType = qtssAttrDataTypeUnknown;
	
	for (UInt32 y = 0; y < numPrefValues; y++)
	{
		UInt32 tempMaxPrefValueSize = 0;
		
		thePrefValue = fPrefsSource->GetPrefValueByIndex(	inPrefIndex, y,
															&thePrefName, &thePrefTypeStr);

		thePrefType = QTSSDataConverter::GetDataTypeForTypeString(thePrefTypeStr);
		theErr = QTSSDataConverter::ConvertStringToType( 	thePrefValue, thePrefType,
															NULL, &tempMaxPrefValueSize );
		Assert(theErr == QTSS_NotEnoughSpace);
		
		if (tempMaxPrefValueSize > maxPrefValueSize)
			maxPrefValueSize = tempMaxPrefValueSize;
	}
	
	for (UInt32 z = 0; z < numPrefValues; z++)
	{
		thePrefValue = fPrefsSource->GetPrefValueByIndex(	inPrefIndex, z,
															&thePrefName, &thePrefTypeStr);
		this->SetPrefValue(inAttrID, z, thePrefValue, thePrefType, maxPrefValueSize);
	}
	
	//
	// Make sure the dictionary knows exactly how many values are associated with
	// this pref
	this->SetNumValues(inAttrID, numPrefValues);
}

void	QTSSPrefs::SetPrefValue(QTSS_AttributeID inAttrID, UInt32 inAttrIndex,
								const char* inPrefValue, QTSS_AttrDataType inPrefType, UInt32 inValueSize)
						
{
	static const UInt32 kMaxPrefValueSize = 1024;
	char convertedPrefValue[kMaxPrefValueSize];
	Assert(inValueSize < kMaxPrefValueSize);
	
	UInt32 convertedBufSize = kMaxPrefValueSize;
	QTSS_Error theErr = QTSSDataConverter::ConvertStringToType
		(inPrefValue, inPrefType, convertedPrefValue, &convertedBufSize );
	Assert(theErr == QTSS_NoErr);
	
	if (inValueSize == 0)
		inValueSize = convertedBufSize;
		
	this->SetValue(inAttrID, inAttrIndex, convertedPrefValue, inValueSize, QTSSDictionary::kDontObeyReadOnly | QTSSDictionary::kDontCallCompletionRoutine);							

}

QTSS_AttributeID QTSSPrefs::AddPrefAttribute(const char* inAttrName, QTSS_AttrDataType inDataType)
{
	QTSS_Error theErr = this->AddInstanceAttribute( inAttrName, NULL, inDataType, qtssAttrModeRead | qtssAttrModeWrite);
	Assert(theErr == QTSS_NoErr);
	
	QTSS_AttributeID theID = qtssIllegalAttrID;
	theErr = this->GetInstanceDictMap()->GetAttrID( inAttrName, &theID);
	Assert(theErr == QTSS_NoErr);
	
	return theID;
}

void	QTSSPrefs::RemoveValueComplete(UInt32 inAttrIndex, QTSSDictionaryMap* inMap,
										UInt32 inValueIndex)
{
	SInt32 thePrefIndex = fPrefsSource->GetPrefValueIndexByName( fModuleName, inMap->GetAttrName(inAttrIndex));
	Assert(thePrefIndex >= 0);
	if (thePrefIndex >= 0)
		fPrefsSource->RemovePrefValue( thePrefIndex, inValueIndex);
	
	if (fPrefsSource->WritePrefsFile())
		QTSSModuleUtils::LogError(qtssWarningVerbosity, qtssMsgCantWriteFile, 0);
}

void	QTSSPrefs::RemoveInstanceAttrComplete(UInt32 inAttrIndex, QTSSDictionaryMap* inMap)
{
	SInt32 thePrefIndex = fPrefsSource->GetPrefValueIndexByName( fModuleName, inMap->GetAttrName(inAttrIndex));
	Assert(thePrefIndex >= 0);
	if (thePrefIndex >= 0)
	{
		UInt32 numValues = fPrefsSource->GetNumPrefValuesByIndex(thePrefIndex);
		for (UInt32 x = 0; x < numValues; x++)
			fPrefsSource->RemovePrefValue( thePrefIndex, x);
	}
	
	if (fPrefsSource->WritePrefsFile())
		QTSSModuleUtils::LogError(qtssWarningVerbosity, qtssMsgCantWriteFile, 0);
}

void QTSSPrefs::SetValueComplete(UInt32 inAttrIndex, QTSSDictionaryMap* inMap,
									UInt32 inValueIndex, const void* inNewValue, UInt32 inNewValueLen)
{
	UInt32 thePrefIndex = fPrefsSource->AddPref(fModuleName, inMap->GetAttrName(inAttrIndex), (char*)QTSSDataConverter::GetDataTypeStringForType(inMap->GetAttrType(inAttrIndex)));
	OSCharArrayDeleter theValueAsString(QTSSDataConverter::ConvertTypeToString(inNewValue, inNewValueLen, inMap->GetAttrType(inAttrIndex)));
	fPrefsSource->SetPrefValue(thePrefIndex, inValueIndex, theValueAsString.GetObject());

	if (fPrefsSource->WritePrefsFile())
		QTSSModuleUtils::LogError(qtssWarningVerbosity, qtssMsgCantWriteFile, 0);
}