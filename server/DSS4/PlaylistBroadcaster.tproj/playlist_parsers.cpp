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

#include "playlist_parsers.h"


SDPFileParser::~SDPFileParser()
{
	if (fSDPBuff) 
	{	delete[] fSDPBuff;
		fSDPBuff = NULL;
	}
}

bool SDPFileParser::IsCommented(SimpleString *theString)
{
	bool result = false;	
	if ( *theString->fTheString == '#' ) result = true; // It's commented if the first non-white char is #
	return result;
}



int TextLine::Parse (SimpleString *textStrPtr)
{
	short count = 0;
	
	do
	{
		if (!textStrPtr) break;

		count = CountDelimeters(textStrPtr,sWordDelimeters);
		if (count < 1) break;
		
		fWords.SetSize(count);		
		fSource = *textStrPtr;		
		SimpleString 	*listStringPtr = fWords.Begin();
		
		SimpleString 	currentString;
		currentString.SetString(textStrPtr->fTheString, 0);
		
		for ( short i = 0; i < count; i ++)
		{	GetNextThing(textStrPtr,&currentString, sWordDelimeters, &currentString);
			*listStringPtr = currentString;
			listStringPtr++;
		} 

	} while (false);
		
	
	return count;
}

int LineAndWordsParser::Parse (SimpleString *textStrPtr)
{
	short 		 	count = 0;	
	
	do
	{
		if (!textStrPtr) break;
					
		count = CountDelimeters(textStrPtr,sLineDelimeters);
		if (count < 1) break;

		fLines.SetSize(count);		
		fSource = *textStrPtr;
		TextLine 		*listStringPtr = fLines.Begin();
			
		SimpleString 	currentString;
		currentString.SetString(textStrPtr->fTheString, 0);
		
		for ( short i = 0; i < count; i ++)
		{	GetNextThing(textStrPtr,&currentString, sLineDelimeters, &currentString);
			listStringPtr->Parse(&currentString);
			listStringPtr++;
		} 
	} while (false);
	
	return count;
}


short SDPFileParser::CountQTTextLines() 
{
	short numlines = 0;	
	TextLine 		*theLinePtr = fParser.fLines.Begin();

	while (theLinePtr)
	{	if (GetQTTextFromLine(theLinePtr))
			numlines ++;
		
		theLinePtr = fParser.fLines.Next();
	};
	
	return numlines;
}


short SDPFileParser::CountMediaEntries() 
{
	bool commented = false;	
	bool isEqual = false;
	short numTracks = 0;
	
	TextLine 		*theLinePtr = fParser.fLines.Begin();
	SimpleString 	*firstWordPtr;
	SimpleString 	mediaString("m");
	
	while (theLinePtr)
	{	
		do 
		{	firstWordPtr = theLinePtr->fWords.Begin();
			if (!firstWordPtr) break;
						
			commented = IsCommented(firstWordPtr);
			if (commented) break;
	
			isEqual = Compare(firstWordPtr, &mediaString);
			if (!isEqual) break;
			 
			numTracks ++;

		} while (false);
		
		theLinePtr = fParser.fLines.Next();
	};
	
	return numTracks;
}

short SDPFileParser::CountRTPMapEntries() 
{
	short startPos = fParser.fLines.GetPos();
	short result = 0;
	TextLine *theLinePtr = fParser.fLines.Get();
	SimpleString mediaString("m"); 
	SimpleString attributeString("a"); 
	SimpleString mapString("rtpmap"); 
	SimpleString *aWordPtr;
	bool isEqual;
	
	while (theLinePtr)
	{	
		aWordPtr = theLinePtr->fWords.Begin();
		if (aWordPtr)			
		{
			isEqual = Compare(aWordPtr, &attributeString);
			if (isEqual)  // see if this attribute is a rtpmap line
			{	
				aWordPtr = theLinePtr->fWords.SetPos(1);			
				isEqual = Compare(aWordPtr, &mapString);
				if (isEqual) result ++;
			}
			else // could be a comment or some other attribute
			{	isEqual = Compare(aWordPtr, &mediaString);
				if (isEqual) break; // its another media line so stop
			}
		}
		theLinePtr = fParser.fLines.Next();
	};
	
	fParser.fLines.SetPos(startPos);
	
	return result;
}


void SDPFileParser::GetPayLoadsFromLine(TextLine *theLinePtr, TypeMap *theTypeMapPtr)
{
	short count = 0;
	SimpleString *aWordPtr = theLinePtr->fWords.SetPos(5);// get protocol ID str
	while (aWordPtr)
	{	count ++;
		aWordPtr = theLinePtr->fWords.Next();// get next protocol ID str
	}	
	theTypeMapPtr->fPayLoads.SetSize(count);
	
	short* idPtr = theTypeMapPtr->fPayLoads.Begin();// get protocol ID ref
	aWordPtr = theLinePtr->fWords.SetPos(5);// get protocol ID str
	while (aWordPtr && idPtr)
	{	*idPtr = (short) atoi(aWordPtr->fTheString);
		aWordPtr = theLinePtr->fWords.Next();// get next protocol ID str
		idPtr = theTypeMapPtr->fPayLoads.Next();// get next protocol ID ref
	}
}

bool SDPFileParser::GetQTTextFromLine(TextLine *theLinePtr)
{
//a=x-qt-text-cpy:xxxxx
//a=x-qt-text-nam:xxxxxx
//a=x-qt-text-inf:xxxxxxx

	bool result = false;
	SimpleString *aWordPtr;	
	char *xString ="a=x-qt-text";
	do 
	{
		aWordPtr = theLinePtr->fWords.Begin();
		if (!aWordPtr) break;
		
		bool isEqual = (0 == strncmp(aWordPtr->fTheString, xString,strlen(xString) ) ) ? true: false;
		if (!isEqual) break;
		
		result = true;
	
	} while (false);
	
	return result;
}


bool SDPFileParser::GetMediaFromLine(TextLine *theLinePtr, TypeMap *theTypeMapPtr)
{
	bool result = false;
	SimpleString *aWordPtr;	
	SimpleString mediaString("m");

	do 
	{
		aWordPtr = theLinePtr->fWords.Begin();
		if (!aWordPtr) break;
			
		bool isEqual = Compare(aWordPtr, &mediaString);
		if (!isEqual) break;

		aWordPtr = theLinePtr->fWords.SetPos(1);// get type 
		if (!aWordPtr) break;
		
		theTypeMapPtr->fTheTypeStr = *aWordPtr;
				
		aWordPtr = theLinePtr->fWords.SetPos(2);// get movie port 
		if (!aWordPtr) break;
		
		theTypeMapPtr->fPort = atoi(aWordPtr->fTheString);
		
		aWordPtr = theLinePtr->fWords.SetPos(3);// get protocol 
		if (!aWordPtr) break;
		
		theTypeMapPtr->fProtocolStr = *aWordPtr;

		GetPayLoadsFromLine(theLinePtr, theTypeMapPtr);
				
		result = true;
	} while (false);
	
	return result;
	
}

bool SDPFileParser::GetRTPMap(TextLine *theLinePtr,PayLoad *payloadPtr)
{
	bool lineOK = false;
	SimpleString *aWordPtr;
	
	do
	{		
		if (!theLinePtr || !payloadPtr) break;
		
		aWordPtr = theLinePtr->fWords.SetPos(2); // the Payload ID
		if (!aWordPtr) break;	
		payloadPtr->payloadID = atoi(aWordPtr->fTheString);
	
		aWordPtr = theLinePtr->fWords.Next(); // the Payload type string
		if (!aWordPtr) break;
		payloadPtr->payLoadString = *aWordPtr;
		
		aWordPtr = theLinePtr->fWords.Next(); // the Payload type string
		if (!aWordPtr) break;
		payloadPtr->timeScale = atoi(aWordPtr->fTheString);
		
		lineOK = true;
		
	} while (false);
	
	return lineOK;

}

TextLine *SDPFileParser::GetRTPMapLines(TextLine *theLinePtr,TypeMap *theTypeMapPtr)
{ 
	
	do
	{		
		if (!theLinePtr || !theTypeMapPtr) break;
			
		short numAttributes = CountRTPMapEntries();
		theTypeMapPtr->fPayLoadTypes.SetSize(numAttributes);
		PayLoad *payloadPtr = theTypeMapPtr->fPayLoadTypes.Begin(); 
		
		if (!payloadPtr) break;

		while( theLinePtr && (numAttributes > 0) ) 
		{
			bool haveMAP = GetRTPMap(theLinePtr,payloadPtr);
			theLinePtr = fParser.fLines.Next(); // skip to next line
			
			if (haveMAP)
			{	numAttributes --;
				payloadPtr = theTypeMapPtr->fPayLoadTypes.Next(); //skip to next payload entry
				if (!payloadPtr) break;
			}
			
		}
	} while (false);
	
	return theLinePtr;
}

TextLine * SDPFileParser::GetTrackID(TextLine *theLinePtr,TypeMap *theTypeMapPtr)
{
        SimpleString *aFieldPtr;
	SimpleString *aWordPtr;
	while(theLinePtr)
	{
	        SimpleString mediaString("m");
	        do 
	        {
		       SimpleString attributeString("a");
		       SimpleString trackString("trackID"); 
		       
		       aFieldPtr = theLinePtr->fWords.Begin();
		       if (!aFieldPtr) break;
			
		       bool isEqual = Compare(aFieldPtr, &attributeString);
		       if (!isEqual) break;

		       aWordPtr = theLinePtr->fWords.SetPos(2);			
		       if (!aWordPtr) break;
		       
		       isEqual = Compare(aWordPtr, &trackString);
		       if (!isEqual) break;

		       aWordPtr = theLinePtr->fWords.SetPos(3);			
		       if (!aWordPtr) break;
		
		       theTypeMapPtr->fTrackID = atoi(aWordPtr->fTheString);
		       
		} while (false);
	
		if(!aFieldPtr)
		       break;
		if(Compare(aFieldPtr, &mediaString))
		       break;
		theLinePtr = fParser.fLines.Next();
	}
	
	return theLinePtr;

}

bool SDPFileParser::ParseIPString(TextLine *theLinePtr)
{
	bool	result = false;
	SimpleString *aWordPtr;
	do 
	{
		if (theLinePtr->fWords.Size() != 4) break;
		
		SimpleString attributeString("c");
		SimpleString ipIDString("IP4"); 
		
		aWordPtr = theLinePtr->fWords.Begin();
		if (!aWordPtr) break;
			
		bool isEqual = Compare(aWordPtr, &attributeString);
		if (!isEqual) break;

		aWordPtr = theLinePtr->fWords.SetPos(2);			
		if (!aWordPtr) break;
		
		isEqual = Compare(aWordPtr, &ipIDString);
		if (!isEqual) break;

		aWordPtr = theLinePtr->fWords.SetPos(3);			
		if (!aWordPtr) break;
		
		fIPAddressString.SetString(aWordPtr->fTheString, aWordPtr->fLen);
		result = true;
		
	} while (false);
		
	return result;

}
SInt32 SDPFileParser::ParseSDP(char *theBuff) 
{
	SInt32 result = 0;  
	bool found = false;
	
	SimpleString source(theBuff);
	fSource.SetString( theBuff, strlen(theBuff) );
	fParser.Parse(&source);
	

//	Test parse
#if 0
	printf("-----------------------------------------------------\n");
	char tempString[256];
	TextLine *theLine = fParser.fLines.Begin();
	while (theLine)
	{	SimpleString *theWord = theLine->fWords.Begin();
		while (theWord)
		{	theWord->GetString(tempString,256);
			printf(tempString);
			theWord = theLine->fWords.Next();
			if (theWord) printf(" _ ");
		}
		theLine = fParser.fLines.Next();
		printf("\n");
	}
	// exit (0);
#endif

	fNumQTTextLines = CountQTTextLines();
	fQTTextLines.SetSize(fNumQTTextLines);
	SimpleString *theQTTextPtr = fQTTextLines.Begin();
	
	fNumTracks = CountMediaEntries();	
	fSDPMediaList.SetSize(fNumTracks);
	
	TextLine *theLinePtr = fParser.fLines.Begin();
	TypeMap *theTypeMapPtr = fSDPMediaList.Begin();
	
	bool foundIP = false;
	while (theLinePtr && theTypeMapPtr)
	{	
		if (foundIP == false)
		{	foundIP = ParseIPString(theLinePtr);
		}

		if (theQTTextPtr && GetQTTextFromLine(theLinePtr))
		{	SimpleString *srcLinePtr = theLinePtr->fWords.Begin();			
			theQTTextPtr->SetString(srcLinePtr->fTheString, strcspn(srcLinePtr->fTheString, "\r\n") );
			theQTTextPtr = fQTTextLines.Next();
		}
		
		found = GetMediaFromLine(theLinePtr, theTypeMapPtr);
		if (found)
		{
			theLinePtr = fParser.fLines.Next();
			if (!theLinePtr) break;
			
			theLinePtr = GetRTPMapLines(theLinePtr,theTypeMapPtr);
			if (!theLinePtr) break;
			
			theLinePtr = GetTrackID(theLinePtr,theTypeMapPtr);
			if (!theLinePtr) break;
			
			theTypeMapPtr = fSDPMediaList.Next();	
			continue;
		}
		
		theLinePtr = fParser.fLines.Next();
	};
	
	return result;
}



SInt32 SDPFileParser::ReadSDP(char *filename)
{
	int result = -1;
	long int bytes= 0;
	
	FILE *f = NULL;
	
	if (fSDPBuff != NULL) 
	{	delete[] fSDPBuff;
		fSDPBuff = NULL;
	}
	
	do 
	{
		f = ::fopen(filename, "r");
		if (f == NULL) break;
				
		result = 1;
		result = ::fseek(f, 0, SEEK_SET);
		if (result != 0) break;
		
		fSDPBuff = new char[cMaxBytes + 1];
		if (NULL == fSDPBuff) break;
		fSDPBuff[cMaxBytes] = 0;
		
		bytes = ::fread(fSDPBuff, sizeof(char), cMaxBytes, f);
		if (bytes < 1) break;
		fSDPBuff[bytes] = 0;
		
		result = ParseSDP(fSDPBuff);
		if (result != 0) break;
		
		result = 0;
		
	} while (false);
		
	if (f != NULL) 
	{	::fclose (f);
		f = NULL;
	}
	
	return result;
}


