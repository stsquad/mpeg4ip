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
	File:		XMLPrefsParser.h

	Contains:	A generic interface for pulling prefs.


*/

#ifndef __XML_PREFS_PARSER__
#define __XML_PREFS_PARSER__

#include "OSFileSource.h"
#include "OSQueue.h"
#include "StringParser.h"
#include "ResizeableStringFormatter.h"

class XMLPrefsParser
{
	public:
	
		XMLPrefsParser(char* inPath);
		~XMLPrefsParser();
	
		//
		// Check for existence, man.
		Bool16	DoesFileExist();
		Bool16	DoesFileExistAsDirectory();
		
		//
		// PARSE & WRITE THE FILE. Returns true if there was an error
		int 	Parse();

		// Completely replaces old prefs file. Returns true if there was an error
		int		WritePrefsFile();

		//
		// ACCESSORS
		
		//
		// Returns the first pref index for this module. Returns -1 if
		// this module name doesn't exist
		SInt32 	GetBaseIndexForModule(const char* inModuleName) const;
		
		//
		// Returns the number of pref values for the pref at this index
		UInt32	GetNumPrefValuesByIndex(const UInt32 inPrefsIndex) const;
		
		//
		// Returns the number of prefs associated with this given module
		UInt32	GetNumPrefsByModule(char* inModuleName) const;
		
		//
		// Returns the total number of prefs
		UInt32	GetNumTotalPrefs() const { return fNumPrefs; }

		//
		// Returns the pref value at the specfied location
		char*	GetPrefValueByIndex(const UInt32 inPrefsIndex, const UInt32 inValueIndex,
										char** outPrefName, char** outDataType) const;
										
		SInt32	GetPrefValueIndexByName(	const char* inModuleName,
											const char* inPrefName) const;
		
		//
		// MODIFIERS
		
		//
		// Creates a new pref. Returns the index of that pref. If pref already
		// exists, returns existing index.
		UInt32	AddPref( 	char* inModuleName, char* inPrefName,
							const char* inPrefDataType );

                void	ChangePrefType( const UInt32 inPrefIndex, char* inNewPrefDataType);
							
		void	AddPrefValue(	const UInt32 inPrefIndex, char* inNewValue);
		
		//
		// If this value index does not exist yet, and it is one higher than
		// the highest one, this function implictly adds the new value.
		void	SetPrefValue( 	const UInt32 inPrefIndex, const UInt32 inValueIndex,
								char* inNewValue);
		
		//
		// Removes the pref entirely if # of values drops to 0
		void	RemovePrefValue( 	const UInt32 inPrefIndex, const UInt32 inValueIndex);

                //
                // Just calls RemovePrefValue until the pref goes away
                void	RemovePref( const UInt32 inPrefIndex );
                
	private:
	
		struct XMLPrefElem
		{
			XMLPrefElem() : 	fModuleName(NULL),
								fPrefName(NULL),
								fPrefDataType(NULL),
								fPrefValues(NULL),
								fNumValues(0),
								fQueueElem(this) {}
			~XMLPrefElem() {
								delete fModuleName;
								delete fPrefName;
								delete fPrefDataType;
								delete [] fPrefValues; 
							}
								
			char* fModuleName;
			char* fPrefName;
			char* fPrefDataType;
			char** fPrefValues;
			UInt32 fNumValues;
			OSQueueElem fQueueElem;
		};
		
		struct ValueQueueElem
		{
			ValueQueueElem() : fQueueElem(this) {}
			~ValueQueueElem() {}
			
			StrPtrLen fValue;
			OSQueueElem fQueueElem;
		};


		void ParseListPrefData(StringParser* inParser, XMLPrefElem* inPrefElem, Bool16 inHasType);
		void ParsePrefsSection(StringParser* inParser, StrPtrLen* inModuleName);
		
		void WriteModuleStart(ResizeableStringFormatter* inWriter, char* inModuleName);
		void WriteModuleEnd(ResizeableStringFormatter* inWriter, char* inModuleName);
		
		XMLPrefElem**	fPrefArray;
		UInt32			fNumPrefs;
		UInt32			fPrefArraySize;
		OSQueue			fPrefsQueue;
		
		OSFileSource 	fFile;
		char*			fFilePath;
};

#endif //__XML_PREFS_PARSER__
