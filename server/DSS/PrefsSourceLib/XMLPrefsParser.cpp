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
	File:		XMLPrefsParser.cpp

	Contains:	Prototype implementation of XMLPrefsParser object.


*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef __Win32__
#include <unistd.h>
#endif

#include "XMLPrefsParser.h"
#include "OSMemory.h"
#include "OSHeaders.h"

static const UInt32 kPrefArrayMinSize = 20;

XMLPrefsParser::XMLPrefsParser(char* inPath)
: 	fPrefArray(NULL),
	fNumPrefs(0),
	fPrefArraySize(kPrefArrayMinSize)
{
	StrPtrLen thePath(inPath);
	fFilePath = thePath.GetAsCString();

	fPrefArray = NEW XMLPrefElem*[fPrefArraySize];
	::memset(fPrefArray, 0, sizeof(XMLPrefElem*) * fPrefArraySize);
}

XMLPrefsParser::~XMLPrefsParser()
{
	delete [] fFilePath;
	delete [] fPrefArray;
}


SInt32 	XMLPrefsParser::GetBaseIndexForModule(const char* inModuleName) const 
{
	if (inModuleName == NULL)
		inModuleName = "";
		
	for (UInt32 x = 0; x < fNumPrefs; x++)
	{
		if (::strcmp(inModuleName, fPrefArray[x]->fModuleName) == 0)
			return x;
	}
	return -1;
}

UInt32	XMLPrefsParser::GetNumPrefValuesByIndex(const UInt32 inPrefsIndex) const 
{
	if (inPrefsIndex >= fNumPrefs)
		return 0;
		
	return fPrefArray[inPrefsIndex]->fNumValues;
}

UInt32	XMLPrefsParser::GetNumPrefsByModule(char* inModuleName) const 
{
	UInt32 theNumPrefs = 0;
	
	if (inModuleName == NULL)
		inModuleName = "";
		
	for (UInt32 x = 0; x < fNumPrefs; x++)
	{
		if (::strcmp(inModuleName, fPrefArray[x]->fModuleName) == 0)
			theNumPrefs++;
		else if (theNumPrefs > 0)
			break;
	}
	return theNumPrefs;
}

char*	XMLPrefsParser::GetPrefValueByIndex(const UInt32 inPrefsIndex, const UInt32 inValueIndex,
											char** outPrefName, char** outDataType) const 
{
	if (inPrefsIndex >= fNumPrefs)
		return NULL;
	
	if (outPrefName != NULL)
		*outPrefName = fPrefArray[inPrefsIndex]->fPrefName;

	if (outDataType != NULL)
		*outDataType = fPrefArray[inPrefsIndex]->fPrefDataType;
	
	if (fPrefArray[inPrefsIndex]->fNumValues <= inValueIndex)
		return NULL;
			
	return fPrefArray[inPrefsIndex]->fPrefValues[inValueIndex];
}

SInt32	XMLPrefsParser::GetPrefValueIndexByName(	const char* inModuleName,
													const char* inPrefName) const
{
	for (UInt32 theIndex = this->GetBaseIndexForModule(inModuleName); theIndex < fNumPrefs; theIndex++)
	{
		if (::strcmp(fPrefArray[theIndex]->fPrefName, inPrefName) == 0)
			return theIndex;
	}
	return -1;
}

UInt32	XMLPrefsParser::AddPref( 	char* inModuleName, char* inPrefName,
									const char* inPrefDataType )
{
	//
	// Check to see if the pref already exists
	SInt32 existingPref = this->GetPrefValueIndexByName(inModuleName, inPrefName);
	if (existingPref >= 0)
		return (UInt32)existingPref;

	if (inModuleName == NULL)
		inModuleName = "";
		
	fNumPrefs++;
	//
	// Reallocate the Pref Array if necessary
	if (fPrefArraySize <= fNumPrefs)
	{
		fPrefArraySize *= 2;
		XMLPrefElem** theNewArray = NEW XMLPrefElem*[fPrefArraySize];
		::memset(theNewArray, 0, sizeof(XMLPrefElem*) * fPrefArraySize);
		::memcpy(theNewArray, fPrefArray, fNumPrefs * sizeof(XMLPrefElem*));
		delete [] fPrefArray;
		fPrefArray = theNewArray;
	}

	//	
	// Find the right index to put this pref in.
	UInt32 newIndex = 0;
	for ( ; newIndex < (fNumPrefs - 1); newIndex++)
	{
		if (::strcmp(inModuleName, fPrefArray[newIndex]->fModuleName) == 0)
			break;
	}
	//
	// Move all the other prefs ahead 
	for (UInt32 y = fNumPrefs; y > newIndex; y--)
		fPrefArray[y] = fPrefArray[y-1];
	//
	// Create a new pref
	StrPtrLen theModName(inModuleName);
	StrPtrLen thePrefName(inPrefName);
	StrPtrLen thePrefDataType((char *)inPrefDataType);
	
	XMLPrefElem* theElem = NEW XMLPrefElem();
	
	theElem->fModuleName = theModName.GetAsCString();
	theElem->fPrefName = thePrefName.GetAsCString();
	theElem->fPrefDataType = thePrefDataType.GetAsCString();

	fPrefArray[newIndex] = theElem;
	return newIndex;
}
							
void	XMLPrefsParser::AddPrefValue(const UInt32 inPrefIndex, char* inNewValue)
{
	if (inPrefIndex >= fNumPrefs)
		return;
		
	XMLPrefElem* theElem = fPrefArray[inPrefIndex];
	
	//
	// Reallocate the Value Array
	char** theNewArray = NEW char*[theElem->fNumValues + 1];
	::memcpy(theNewArray, theElem->fPrefValues, theElem->fNumValues * sizeof(char*));
	delete [] theElem->fPrefValues;
	theElem->fPrefValues = theNewArray;
	
	//
	// Add the new value
	StrPtrLen theNewValue(inNewValue);
	theElem->fPrefValues[theElem->fNumValues] = theNewValue.GetAsCString();
	theElem->fNumValues++;
}

void	XMLPrefsParser::ChangePrefType( const UInt32 inPrefIndex, char* inNewPrefDataType)
{
    if (inPrefIndex >= fNumPrefs)
            return;
    XMLPrefElem* theElem = fPrefArray[inPrefIndex];

    delete [] theElem->fPrefDataType;
    StrPtrLen newDataType(inNewPrefDataType);
    theElem->fPrefDataType = newDataType.GetAsCString();
}
							
void	XMLPrefsParser::SetPrefValue( 	const UInt32 inPrefIndex, const UInt32 inValueIndex,
										char* inNewValue)
{
	if (inPrefIndex >= fNumPrefs)
		return;	
	XMLPrefElem* theElem = fPrefArray[inPrefIndex];
	if (theElem->fNumValues == inValueIndex)
	{
		this->AddPrefValue(inPrefIndex, inNewValue);
		return;
	}
	else if (theElem->fNumValues < inValueIndex)
		return;
	
	delete [] theElem->fPrefValues[inValueIndex];

	StrPtrLen theNewValue(inNewValue);
	theElem->fPrefValues[inValueIndex] = theNewValue.GetAsCString();
}
		
void	XMLPrefsParser::RemovePrefValue(const UInt32 inPrefIndex, const UInt32 inValueIndex)
{
	if (inPrefIndex >= fNumPrefs)
		return;	
	XMLPrefElem* theElem = fPrefArray[inPrefIndex];
        Assert(theElem != NULL);
	if (NULL == theElem) 
		return;
		
	if (inValueIndex >= theElem->fNumValues)
		return;
		

	delete theElem->fPrefValues[inValueIndex];
	theElem->fPrefValues[inValueIndex] = NULL;
	
	for (UInt32 x = inValueIndex; x < (theElem->fNumValues -1); x++)
		theElem->fPrefValues[x] = theElem->fPrefValues[x+1];

	theElem->fPrefValues[theElem->fNumValues -1] = NULL;
	theElem->fNumValues--;
	
	//
	// If there are no more values, just delete the whole pref
	if (theElem->fNumValues == 0)
	{
		delete theElem;
		
		for (UInt32 y = inPrefIndex; y < (fNumPrefs - 1); y++)
			fPrefArray[y] = fPrefArray[y+1];
			
		fPrefArray[fNumPrefs - 1] = NULL;
		fNumPrefs--;
	}
}

void	XMLPrefsParser::RemovePref( const UInt32 inPrefIndex )
{
    if (inPrefIndex >= fNumPrefs)
            return;
    XMLPrefElem* theElem = fPrefArray[inPrefIndex];
    Assert(theElem != NULL);
    if (NULL == theElem)
            return;

	// RemovePrefValue moves data if it exists past the removed index so delete from the end 
	for (SInt32 x = theElem->fNumValues - 1; x >= 0; x--) 
		this->RemovePrefValue( inPrefIndex, x );   
		
}

Bool16	XMLPrefsParser::DoesFileExist()
{
	Bool16 itExists = false;
	fFile.Set(fFilePath);
	if ((fFile.GetLength() > 0) && (!fFile.IsDir()))
		itExists = true;
	fFile.Close();
	
	return itExists;
}

Bool16	XMLPrefsParser::DoesFileExistAsDirectory()
{
	Bool16 itExists = false;
	fFile.Set(fFilePath);
	if (fFile.IsDir())
		itExists = true;
	fFile.Close();
	
	return itExists;
}

Bool16	XMLPrefsParser::CanWriteFile()
{
	//
	// First check if it exists for reading
	FILE* theFile = ::fopen(fFilePath, "r");
	if (theFile == NULL)
		return true;
	
	::fclose(theFile);
	
	//
	// File exists for reading, check if we can write it
	theFile = ::fopen(fFilePath, "a");
	if (theFile == NULL)
		return false;
		
	//
	// We can read and write
	::fclose(theFile);
	return true;
}
	
static const StrPtrLen kServer("SERVER");
static const StrPtrLen kModule("MODULE");
static const StrPtrLen kPref("PREF");
static const StrPtrLen kListPref("LIST-PREF");
static const StrPtrLen kChar("CharArray");

static UInt8 sQuoteBraceMask[256] = { 0 };

int XMLPrefsParser::Parse()
{
	sQuoteBraceMask['>'] = 1;
	sQuoteBraceMask['"'] = 1;
	
	if (fPrefArray != NULL)
	{
		//
		// Clear out old data if necessary
		delete [] fPrefArray;
		
		fNumPrefs = 0;
		fPrefArraySize = 0;
	}

	fFile.Set(fFilePath);
	if (fFile.GetLength() == 0)
		return true;
	
	char* theFileData = NEW char[fFile.GetLength() + 1];
	UInt32 theLengthRead = 0;
	fFile.Read(0, theFileData, fFile.GetLength(), &theLengthRead); 
	
	StrPtrLen theDataPtr(theFileData, theLengthRead);
	StringParser theParser(&theDataPtr);
	
	while (theParser.GetDataRemaining() > 0)
	{
		(void)theParser.GetThru(NULL, '<');
		
		StrPtrLen thePrefType;
		theParser.ConsumeWord(&thePrefType);
		
		if (thePrefType.Equal(kServer))
		{
			StrPtrLen theEmptyStr("");
			this->ParsePrefsSection(&theParser, &theEmptyStr);
		}
		else if (thePrefType.Equal(kModule))
		{
			(void)theParser.GetThru(NULL, '"');
			StrPtrLen theModName;
			theParser.ConsumeWord(&theModName);
			this->ParsePrefsSection(&theParser, &theModName);
		}
	}
	
	//
	// Clear out the prefs queue and construct the prefs array
	fPrefArraySize = fNumPrefs = fPrefsQueue.GetLength();
	if (fPrefArraySize < kPrefArrayMinSize)
		fPrefArraySize = kPrefArrayMinSize;
		
	fPrefArray = NEW XMLPrefElem*[fPrefArraySize];
	::memset(fPrefArray, 0, sizeof(XMLPrefElem*) * fPrefArraySize);

	UInt32 y = 0;
	for (OSQueueIter theIter(&fPrefsQueue); !theIter.IsDone(); y++, theIter.Next())
	{
		OSQueueElem* theElem = theIter.GetCurrent();		
		fPrefArray[y] = (XMLPrefElem*)theElem->GetEnclosingObject();
		Assert(y < fNumPrefs);
	}
	for (UInt32 z = 0; z < fNumPrefs; z++)
		fPrefsQueue.Remove(&fPrefArray[z]->fQueueElem);
	Assert(fPrefsQueue.GetLength() == 0);
	
	fFile.Close();
	return false;
}

void XMLPrefsParser::ParsePrefsSection(StringParser* inParser, StrPtrLen* inModuleName)
{
	StrPtrLen theWord;

	//
	// Go through the prefs one by one, adding an XML prefs elem for each one
	while (inParser->GetDataRemaining() > 0)
	{
		inParser->GetThru(NULL, '<');
		
		if ((inParser->GetDataRemaining() > 0) && (inParser->PeekFast() == '/'))
		{
			inParser->ConsumeLength(NULL, 1); // Skip past '/'
			
			//
			// Stop condition - look for </SERVER> or </MODULE>
			inParser->ConsumeWord(&theWord);
			
			if (theWord.Equal(kServer) || theWord.Equal(kModule))
				return;
		}
		
		inParser->ConsumeWord(&theWord);
		
		if (theWord.Equal(kPref) || theWord.Equal(kListPref))
		{
			//
			// Begin parsing out the pref
			XMLPrefElem* theElem = NEW XMLPrefElem();
			theElem->fModuleName = inModuleName->GetAsCString();
			
			inParser->GetThru(NULL, '"');
			
			// Parse out pref name
			StrPtrLen thePrefName;
			inParser->GetThru(&thePrefName, '"');
			theElem->fPrefName = thePrefName.GetAsCString();
			
			Bool16 hasType = false;
			inParser->ConsumeUntil(NULL, sQuoteBraceMask);
			if (inParser->Expect('"'))
			{
				// Parse out pref type
				StrPtrLen thePrefType;
				inParser->GetThru(&thePrefType, '"');
				theElem->fPrefDataType = thePrefType.GetAsCString();
				hasType = true;
			}
			else
			{
				inParser->ConsumeLength(NULL, 1);
				theElem->fPrefDataType = kChar.GetAsCString();
			}
			
			if (theWord.Equal(kListPref))
				this->ParseListPrefData(inParser, theElem, hasType);
			else
			{
				//
				// This is a normal pref (not a list-pref).
				// Get the 1 value and store it in the elem
				if (hasType)
					inParser->GetThru(NULL, '>');

				StrPtrLen thePrefData;
				inParser->GetThru(&thePrefData, '<');
				
				if (thePrefData.Len > 0)
				{
					theElem->fNumValues = 1;
					theElem->fPrefValues = NEW char*[1];
					theElem->fPrefValues[0] = thePrefData.GetAsCString();
				}
				else
					theElem->fNumValues = 0;
			}
			
			//
			// Add this pref elem to the queue.
			fPrefsQueue.EnQueue(&theElem->fQueueElem);
		}
	}
}

void XMLPrefsParser::ParseListPrefData(StringParser* inParser, XMLPrefElem* inPrefElem, Bool16 inHasType)
{
	OSQueue theValueQueue;
	
	while (inParser->GetDataRemaining() > 0)
	{
		if (inHasType)
			inParser->GetThru( NULL, '<');
		inHasType = true;
		
		if ((inParser->GetDataRemaining() > 0) && (inParser->PeekFast() == '/'))
		{
			inParser->ConsumeLength(NULL, 1); // Skip past '/'
			
			//
			// Stop condition - look for </LIST-PREF>
			StrPtrLen theWord;
			inParser->ConsumeWord(&theWord);
			
			if (theWord.Equal(kListPref))
				break;
			
			inParser->GetThru(NULL, '>');
		}
		
		inParser->GetThru(NULL, '>'); // Skip past "VALUE"
		
		// Parse out the value
		ValueQueueElem* theValueElem = NEW ValueQueueElem;
		inParser->GetThru(&theValueElem->fValue, '<');
		theValueQueue.EnQueue(&theValueElem->fQueueElem);			
	}
	
	//
	// Take all the accumulated values, copy them into inPrefElem
	inPrefElem->fNumValues = theValueQueue.GetLength();
	inPrefElem->fPrefValues = NEW char*[inPrefElem->fNumValues];
	
	for (UInt32 x = 0; x < inPrefElem->fNumValues; x++)
	{
		ValueQueueElem* dequeuedValue = (ValueQueueElem*)theValueQueue.DeQueue()->GetEnclosingObject();
		inPrefElem->fPrefValues[x] = dequeuedValue->fValue.GetAsCString();
		delete dequeuedValue;
	}
	Assert(theValueQueue.GetLength() == 0);
}

static char* kFileHeader[] = 
{
	"<?xml version =\"1.0\"?>",
	"<!-- The Document Type Definition (DTD) for the file -->",
	"<!DOCTYPE CONFIGURATION [",
	"<!ELEMENT CONFIGURATION (SERVER, MODULE*)>",
	"<!ELEMENT SERVER (PREF|LIST-PREF)*>",
	"<!ELEMENT MODULE (PREF|LIST-PREF)*>",
	"<!ATTLIST MODULE",
	"\tNAME CDATA #REQUIRED>",
	"<!ELEMENT PREF (#PCDATA)>",
	"<!ATTLIST PREF",
	"\tNAME CDATA #REQUIRED",
	"\tTYPE (UInt8|SInt8|UInt16|SInt16|UInt32|SInt32|UInt64|SInt64|Float32|Float64|Bool16|Bool8|char) \"char\">",
	"<!ELEMENT LIST-PREF (VALUE*)>",
	"<!ELEMENT VALUE (#PCDATA)>",
	"<!ATTLIST LIST-PREF",
	"\tNAME CDATA #REQUIRED",
	"\tTYPE  (UInt8|SInt8|UInt16|SInt16|UInt32|SInt32|UInt64|SInt64|Float32|Float64|Bool16|Bool8|char) \"char\">",
	"]>",
	"<CONFIGURATION>",
	NULL
};

int	XMLPrefsParser::WritePrefsFile()
{

	char theBuffer[8192];
	ResizeableStringFormatter theFileWriter(theBuffer, 8192);

	//
	// Write the file header
	for (UInt32 a = 0; kFileHeader[a] != NULL; a++)
	{
		theFileWriter.Put(kFileHeader[a]);
		theFileWriter.Put(kEOLString);
	}
	
	if (fNumPrefs > 0)
	{
		char* lastModuleName = fPrefArray[0]->fModuleName;
		this->WriteModuleStart(&theFileWriter, lastModuleName);
		
		for (UInt32 y = 0; y < fNumPrefs; y++)
		{
			XMLPrefElem* theElem = fPrefArray[y];

			if (::strcmp(theElem->fModuleName, lastModuleName) != 0)
			{
				this->WriteModuleEnd(&theFileWriter, lastModuleName);
				this->WriteModuleStart(&theFileWriter, theElem->fModuleName);
			}
			lastModuleName = theElem->fModuleName;
			
			char typeBuffer[256];
			char prefBuffer[1024];
			if (::strcmp(theElem->fPrefDataType, kChar.Ptr) == 0)
				::sprintf(typeBuffer, ">");
			else
				::sprintf(typeBuffer, " TYPE=\"%s\">", theElem->fPrefDataType);
			
			if (theElem->fNumValues == 0)
				continue;
			else if (theElem->fNumValues == 1)
			{
				//::sprintf(prefBuffer, "\t<PREF NAME=\"%s\"%s%s</PREF>%s\n", theElem->fPrefName, typeBuffer, theElem->fPrefValues[0], kEOLString);
				theFileWriter.Put("\t<PREF NAME=\"");
				theFileWriter.Put(theElem->fPrefName);
				theFileWriter.Put("\"");
				theFileWriter.Put(typeBuffer);
				theFileWriter.Put(theElem->fPrefValues[0]);
				theFileWriter.Put("</PREF>");
				theFileWriter.Put(kEOLString);
			}
			else
			{
				::sprintf(prefBuffer, "\t<LIST-PREF NAME=\"%s\"%s%s", theElem->fPrefName, typeBuffer, kEOLString);
				theFileWriter.Put(prefBuffer);

				for (UInt32 x = 0; x < theElem->fNumValues; x++)
				{
					//::sprintf(prefBuffer, "\t\t<VALUE>%s</VALUE>%s", theElem->fPrefValues[x], kEOLString);
					theFileWriter.Put("\t\t<VALUE>");
					theFileWriter.Put(theElem->fPrefValues[x]);
					theFileWriter.Put("</VALUE>");
					theFileWriter.Put(kEOLString);
				}
				::sprintf(prefBuffer, "\t</LIST-PREF>%s", kEOLString);
				theFileWriter.Put(prefBuffer);
			}
		}
		
		this->WriteModuleEnd(&theFileWriter, lastModuleName);
	}
	
	theFileWriter.Put("</CONFIGURATION>");
	theFileWriter.Put(kEOLString);

	//
	// New libC code. This seems to work better on Win32
	theFileWriter.PutTerminator();
	FILE* theFile = ::fopen(fFilePath, "w");
	if (theFile == NULL)
		return true;
		
	::fprintf(theFile, "%s", theFileWriter.GetBufPtr());
	::fclose(theFile);
	
#ifndef __Win32__
	::chmod(fFilePath, S_IRUSR | S_IWUSR | S_IRGRP);
#endif
#if 0
	//
	// Old POSIX code. This doesn't work well on Win32
	int theFile = ::open(fFilePath, O_WRONLY | O_CREAT | O_TRUNC);
	if (theFile == -1)
		return true;
		
#ifndef __Win32__
	::chmod(fFilePath, S_IRUSR | S_IWUSR );
#endif

	int theErr = ::write(theFile, theFileWriter.GetBufPtr(), theFileWriter.GetCurrentOffset());
	Assert((UInt32)theErr == theFileWriter.GetCurrentOffset());
	if ((UInt32)theErr != theFileWriter.GetCurrentOffset())
		return true;
		
	(void)::close(theFile);
#endif
	
	return false;
}

void	XMLPrefsParser::WriteModuleStart(ResizeableStringFormatter* inWriter, char* inModuleName)
{
	char buffer[512];
	if (::strlen(inModuleName) == 0)
		::sprintf(buffer, "<SERVER>%s",kEOLString);
	else
		::sprintf(buffer, "<MODULE NAME=\"%s\">%s",inModuleName, kEOLString);
		
	inWriter->Put(buffer);
}

void	XMLPrefsParser::WriteModuleEnd(ResizeableStringFormatter* inWriter, char* inModuleName)
{
	char buffer[512];
	if (::strlen(inModuleName) == 0)
		::sprintf(buffer, "</SERVER>%s", kEOLString);
	else
		::sprintf(buffer, "</MODULE>%s", kEOLString);

	inWriter->Put(buffer);
}
