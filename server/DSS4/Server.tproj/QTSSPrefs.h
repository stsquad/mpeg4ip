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
	Contains:	Object store for module preferences.
	
	
	
*/

#ifndef __QTSSMODULEPREFS_H__
#define __QTSSMODULEPREFS_H__

#include "QTSS.h"
#include "QTSSDictionary.h"
#include "QTSSModuleUtils.h"

#include "StrPtrLen.h"
#include "XMLPrefsParser.h"

class QTSSPrefs : public QTSSDictionary
{
	public:

		QTSSPrefs(	XMLPrefsParser* inPrefsSource,
					StrPtrLen* inModuleName,
					QTSSDictionaryMap* inMap,
					Bool16 areInstanceAttrsAllowed,
					QTSSPrefs* parentDictionary = NULL );
		virtual ~QTSSPrefs() { if (fPrefName != NULL) delete [] fPrefName; }
		
		//This is callable at any time, and is thread safe wrt to the accessors
		void 		RereadPreferences();
		void 		RereadObjectPreferences(ContainerRef container);
		
		//
		// ACCESSORS
		OSMutex*	GetMutex() { return &fPrefsMutex; }
		
		ContainerRef GetContainerRefForObject(QTSSPrefs* object);
		ContainerRef GetContainerRef();
		
	protected:

		XMLPrefsParser* fPrefsSource;
		OSMutex	fPrefsMutex;
		char*	fPrefName;
		QTSSPrefs* fParentDictionary;
	
		//
		// SET PREF VALUES FROM FILE
		//
		// Places all the values at inPrefIndex of the prefs file into the attribute
		// with the specified ID. This attribute must already exist.
		//
		// Specify inNumValues if you wish to restrict the number of values retrieved
		// from the text file to a certain number, otherwise specify 0.
		void SetPrefValuesFromFile(ContainerRef container, UInt32 inPrefIndex, QTSS_AttributeID inAttrID, UInt32 inNumValues = 0);
		void SetPrefValuesFromFileWithRef(ContainerRef pref, QTSS_AttributeID inAttrID, UInt32 inNumValues = 0);
		void SetObjectValuesFromFile(ContainerRef pref, QTSS_AttributeID inAttrID, UInt32 inNumValues, char* prefName);

		//
		// SET PREF VALUE
		//
		// Places the specified value into the attribute with inAttrID, at inAttrIndex
		// index. This function does the conversion, and uses the converted size of the
		// value when setting the value. If you wish to override this size, specify inValueSize,
		// otherwise it can be 0.
		void SetPrefValue(QTSS_AttributeID inAttrID, UInt32 inAttrIndex,
						const char* inPrefValue, QTSS_AttrDataType inPrefType, UInt32 inValueSize = 0);

		
	protected:
	
		//
		// Completion routines for SetValue and RemoveValue write back to the config source
		virtual void	RemoveValueComplete(UInt32 inAttrIndex, QTSSDictionaryMap* inMap,
										UInt32 inValueIndex);
										
		virtual void 	SetValueComplete(UInt32 inAttrIndex, QTSSDictionaryMap* inMap,
								UInt32 inValueIndex, const void* inNewValue, UInt32 inNewValueLen);
		
		virtual void	RemoveInstanceAttrComplete(UInt32 inAttrindex, QTSSDictionaryMap* inMap);
		
		virtual QTSSDictionary* CreateNewDictionary(QTSSDictionaryMap* inMap, OSMutex* inMutex);

	private:
	
		QTSS_AttributeID AddPrefAttribute(const char* inAttrName, QTSS_AttrDataType inDataType);
						
};
#endif //__QTSSMODULEPREFS_H__
