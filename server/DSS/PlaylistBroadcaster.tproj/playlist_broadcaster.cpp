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


#include "playlist_broadcaster.h"
#include "OS.h"
#include "OSThread.h"

QTFileBroadcaster::QTFileBroadcaster() 
{
	fRTPFilePtr = NULL;
	fMovieSDPParser = NULL;
	fBasePort = 0;
	fDebug = false;
	fDeepDebug = false;

// transmit time trackers
	fLastTransmitTime = 0.0;
	
	fStreamStartTime  = 0;
	fMovieStartTime  = 0;
	fMovieEndTime = 0;
	fMovieIntervalTime = 0;
	fMovieTimeDiffMilli = 0;
	
	fMovieStart = false;
	fSendTimeOffset = 0.0;
	fMovieDuration = 0.0;
	fMovieTracks = 0;
	fMappedMovieTracks = 0;
	fNumMoviesPlayed = 0;
	fPlay = true;
	fSend = true;
}

QTFileBroadcaster::~QTFileBroadcaster()
{ 
	LogFileClose(); 
	if (fRTPFilePtr != NULL)
	{ 	delete fRTPFilePtr; 
	} 
}


int QTFileBroadcaster::SetUp(PLBroadcastDef *broadcastDefPtr)
{
	int result = -1;
	int numStreams = 0;
	
	PlayListUtils::Initialize();
	do 
	{
		if (! broadcastDefPtr) { result = eParam; 			break; };	
		if (!broadcastDefPtr->mSDPFile) { result = eSDPFileInvalidName; break; };
			
		int nameLen = strlen(broadcastDefPtr->mSDPFile);
		if (nameLen > 255) 	{ result = eSDPFileInvalidName; break; };			
		if (0 ==  nameLen) 	{ result = eSDPFileInvalidName; break; };	
		
		
		result = fStreamSDPParser.ReadSDP(broadcastDefPtr->mSDPFile);
		if (result != 0)
		{ 	if (result < 0) { result = eSDPFileNotFound; 	break; };
			if (result > 0) { result = eSDPFileInvalid; 	break; };
		}	
		
		if (!broadcastDefPtr->mBasePort) { result = eSDPFileNoPorts; break; };
		int portLen = strlen(broadcastDefPtr->mBasePort);
		if (0 ==  portLen) { result = eSDPFileNoPorts; 		break; };	
		if (portLen > 5  )  { result = eSDPFileInvalidPort; break; };
					
		int basePort = atoi(broadcastDefPtr->mBasePort);
		if (basePort == 0) { result = eSDPFileNoPorts;  	break; };	
		if 	( (basePort < 1000) || (basePort > 65535) ) 
			{ result = eSDPFileInvalidPort; 				break; };	
		
		numStreams = fStreamSDPParser.GetNumTracks();		
		if (numStreams == 0) { result = eSDPFileNoMedia; 	break; };
		
		UDPSocketPair *socketArrayPtr = fSocketlist.SetSize(numStreams);
		if (socketArrayPtr == NULL) { result = eMem; 		break; };

		// Bind SDP file defined ports to active stream ports
		{ 
			UInt16 			streamIndex = 0;
			UInt16 			rtpPort = 0;
			UInt16 			rtcpPort = 0;
			TypeMap*		mediaTypePtr;
			char			sdpIPAddress[32];
			SimpleString* ipStringPtr = fStreamSDPParser.GetIPString();
			
			if ( (NULL == ipStringPtr) || (ipStringPtr->fLen >= 32) )
			{ 	
				result = eSDPFileInvalid; 				
				break; 
			}
				
			memcpy(sdpIPAddress,ipStringPtr->fTheString,ipStringPtr->fLen);
			sdpIPAddress[ipStringPtr->fLen] = '\0';
			
			UDPSocketPair *aSocketPair = fSocketlist.Begin();
			
			while (aSocketPair != NULL)
			{	
				mediaTypePtr = fStreamSDPParser.fSDPMediaList.SetPos(streamIndex);
				
				if (mediaTypePtr == NULL) 
				{  	result = eSDPFileInvalid;	
					break;	
				}
									
				if (mediaTypePtr->fPort == 0) 
				{  
					result = eSDPFileInvalid;	
					break;	
				}

				rtpPort = mediaTypePtr->fPort;
				rtcpPort = rtpPort + 1;
				
				result = fSocketlist.OpenAndBind(aSocketPair, rtpPort,rtcpPort,sdpIPAddress) ;
				if (result != 0) 
				{ 
					result = eSDPFileInvalidPort; 
					break; 
				}

				aSocketPair = fSocketlist.Next();
				streamIndex++;
			}
			
			if (result != 0) 
				break;
		}

			
		MediaStream *mediaArrayPtr = fMediaStreamList.SetSize(numStreams);
		if (mediaArrayPtr == NULL) 
		{ 
			result = eMem; 
			break; 
		}
				
		for (int i = 0; i < numStreams; i ++)
		{	UDPSocketPair 	*socketPairPtr = fSocketlist.SetPos(i);
			MediaStream 	*mediaStreamPtr = fMediaStreamList.SetPos(i);
			TypeMap 		*streamMediaTypePtr = fStreamSDPParser.fSDPMediaList.SetPos(i);
			
			if (socketPairPtr && mediaStreamPtr && streamMediaTypePtr)
			{
				mediaStreamPtr->fData.fSocketPair = socketPairPtr;
				streamMediaTypePtr->fMediaStreamPtr = mediaStreamPtr;
				mediaStreamPtr->fData.fStreamMediaTypePtr = streamMediaTypePtr;
			}
			else 
			{ 
				result = eMem;
				break; 
			}
		}
		
		
		fMediaStreamList.SetUpStreamSSRCs();
		fStreamStartTime = PlayListUtils::Milliseconds(); 
		fMediaStreamList.StreamStarted(fStreamStartTime);
		result = 0;
		LogFileOpen();
		
	} while (false);
	
	return result;
}


PayLoad * QTFileBroadcaster::FindPayLoad(short id, ArrayList<PayLoad> *PayLoadListPtr)
{
	PayLoad *thePayLoadPtr = PayLoadListPtr->Begin();
	while (thePayLoadPtr)
	{
		if (thePayLoadPtr->payloadID == id) 
			break;
			
		thePayLoadPtr = PayLoadListPtr->Next();
	}
	return thePayLoadPtr;
}


bool QTFileBroadcaster::CompareRTPMaps(TypeMap *movieMediaTypePtr, TypeMap *streamMediaTypePtr, short moviePayLoadID)
{
	bool found = false;
		
	do
	{
		PayLoad *moviePayLoadPtr = FindPayLoad(moviePayLoadID, &movieMediaTypePtr->fPayLoadTypes);
		if (!moviePayLoadPtr) break;
		
		PayLoad *streamPayLoadPtr = streamMediaTypePtr->fPayLoadTypes.Begin();
		while (streamPayLoadPtr)
		{
			if (moviePayLoadPtr->timeScale == streamPayLoadPtr->timeScale) 
			{	found =  SimpleParser::Compare(&moviePayLoadPtr->payLoadString,&streamPayLoadPtr->payLoadString );
			}
			
			if (found) 
			{	moviePayLoadPtr->payloadID = streamPayLoadPtr->payloadID; // map movie ID to match stream id
				break;
			}
			streamPayLoadPtr = streamMediaTypePtr->fPayLoadTypes.Next();
		}
			
		
	} while (false);
	return found;
}

bool QTFileBroadcaster::CompareMediaTypes(TypeMap *movieMediaTypePtr, TypeMap *streamMediaTypePtr)
{
	bool found = false;
	
	found = SimpleParser::Compare(&movieMediaTypePtr->fTheTypeStr,&streamMediaTypePtr->fTheTypeStr);
	if (found)
	{	
		found = false; 
		
		short *movieIDPtr = movieMediaTypePtr->fPayLoads.Begin();
		while (movieIDPtr && !found)
		{
			short *streamIDPtr = streamMediaTypePtr->fPayLoads.Begin();
			while (streamIDPtr && !found)
			{
				
				if (*movieIDPtr >= 96)
					found = CompareRTPMaps(movieMediaTypePtr, streamMediaTypePtr, *movieIDPtr);
				else
					found = (*streamIDPtr == *movieIDPtr) ? true : false;
			
				streamIDPtr = streamMediaTypePtr->fPayLoads.Next();		
			}

			movieIDPtr = movieMediaTypePtr->fPayLoads.Next();
		}
	}
	
	
	return found;
}


int QTFileBroadcaster::MapMovieToStream()
{
	int 			result = -1;
	bool 			matches = false;
	ArrayList<bool> map;
	bool 			*isMappedPtr;
	int 			masterPos = 0;	
	int 			mappedTracks = 0;
	
	map.SetSize(fStreamSDPParser.fSDPMediaList.Size());	
	
	isMappedPtr = map.Begin();
	
	while (isMappedPtr)
	{	*isMappedPtr = false;
		isMappedPtr = map.Next();
	}
	
	TypeMap *movieMediaTypePtr = fMovieSDPParser->fSDPMediaList.Begin();
	
	while (movieMediaTypePtr)
	{
		TypeMap *streamMediaTypePtr = fStreamSDPParser.fSDPMediaList.Begin();
		
		while (streamMediaTypePtr)
		{
			matches = CompareMediaTypes(movieMediaTypePtr, streamMediaTypePtr);
			
			if (matches)
			{
				masterPos = fStreamSDPParser.fSDPMediaList.GetPos();
				isMappedPtr = map.SetPos(masterPos);
				if (isMappedPtr == NULL) 
					break;
				
				if (false == *isMappedPtr) 
				{	
					movieMediaTypePtr->fMediaStreamPtr = streamMediaTypePtr->fMediaStreamPtr;
					*isMappedPtr = true;
					mappedTracks++;
					break; 
				}
			}
			streamMediaTypePtr = fStreamSDPParser.fSDPMediaList.Next();
		}
		movieMediaTypePtr = fMovieSDPParser->fSDPMediaList.Next();
	}

	result = mappedTracks;

	return result;

}


UInt32 QTFileBroadcaster::GetSDPTracks(QTRTPFile *newRTPFilePtr)
{
	char*	sdpBuffer;
	int  	bufferLen = 0;
	UInt32 	result = 0;
	
	sdpBuffer = newRTPFilePtr->GetSDPFile(&bufferLen);
	fMovieSDPParser->ParseSDP(sdpBuffer);
	result = fMovieSDPParser->GetNumTracks();

	return result;
}


int QTFileBroadcaster::AddTrackAndStream(QTRTPFile *newRTPFilePtr)
{
	TypeMap*	movieMediaTypePtr = fMovieSDPParser->fSDPMediaList.Begin();
	UInt32 		trackID;
	char*		cookie;
	int 		err = -1;
	
	while (movieMediaTypePtr)
	{
		if (movieMediaTypePtr->fMediaStreamPtr != NULL)
		{
			trackID = movieMediaTypePtr->fTrackID;
			if (trackID == 0) break;
			
			
			movieMediaTypePtr->fMediaStreamPtr->fData.fMovieMediaTypePtr = movieMediaTypePtr;
			movieMediaTypePtr->fMediaStreamPtr->fData.fRTPFilePtr = newRTPFilePtr;
			movieMediaTypePtr->fMediaStreamPtr->fSend = fSend;

			cookie = (char *) movieMediaTypePtr->fMediaStreamPtr;
    		err = newRTPFilePtr->AddTrack(trackID, false);
			if (err != 0)
				break;
			
			newRTPFilePtr->SetTrackCookie(trackID, cookie);		
 			newRTPFilePtr->SetTrackSSRC(trackID, (UInt32) 0); // set later
 			
 			err = newRTPFilePtr->Seek(0.0);
 			if (err != QTRTPFile::errNoError)
 				break;
 			
 			if (movieMediaTypePtr->fTimeScale <= 0.0)
 			{ 	err = -1;
 				break;
 			}
		}	
	
		movieMediaTypePtr = fMovieSDPParser->fSDPMediaList.Next();
	}
	
	return err;
}

int QTFileBroadcaster::SetUpAMovie(char *theMovieFileName)
{
	int err = -1;
	QTRTPFile *newRTPFilePtr = NULL;
	do {
		fMovieTracks = 0;
		fMappedMovieTracks = 0;
		
		if (fMovieSDPParser != NULL) delete fMovieSDPParser;
   		if (fRTPFilePtr != NULL) delete fRTPFilePtr;
   		
  		fMovieSDPParser = NULL;
		fRTPFilePtr = NULL;
		
		if (!theMovieFileName) 			{ err = eMovieFileInvalidName; break; }		
		int nameLen = strlen(theMovieFileName);						
		if (nameLen > 255) 				{ err = eMovieFileInvalidName; break; }
		if (0 == nameLen) 				{ err = eMovieFileInvalidName; break; }
		
		fMovieSDPParser = new SDPFileParser;
		if (NULL == fMovieSDPParser) 	{ err = eMem; break;}
		
		newRTPFilePtr = new QTRTPFile();
		if (NULL == newRTPFilePtr) 		{ err = eMem; break;}
		
		QTRTPFile::ErrorCode result = newRTPFilePtr->Initialize(theMovieFileName);
		err = EvalErrorCode(result);
		if (err) break;
		
		fMovieTracks = GetSDPTracks(newRTPFilePtr);
		if (fMovieTracks < 1) 			{ err = eMovieFileNoHintedTracks; break; }
		
		fMappedMovieTracks = MapMovieToStream();
		if (fMappedMovieTracks < 1)  	{ err = eMovieFileNoSDPMatches; break; }

		err = AddTrackAndStream(newRTPFilePtr);		
		if (err != 0) 					{ err = eMovieFileInvalid; break; }
		
	} while (false);
	
	if (err) 
	{	if (newRTPFilePtr) 
			delete newRTPFilePtr;
		newRTPFilePtr = NULL;
	}
	
	fRTPFilePtr = newRTPFilePtr;
	
	return err;
}

#if __MACOS__
void usleep(unsigned int);
void usleep(unsigned int)
{

}
#endif


Float64 QTFileBroadcaster::Sleep(Float64 transmitTimeMilli)
{
	Float64 sleepTime;
	Float64 timeToSend;
	Float64 currentTime = PlayListUtils::Milliseconds();
	
	if (fMovieStart) 
	{	
		fMovieStart = false;
	}
	

	timeToSend = fMovieStartTime + transmitTimeMilli;
	sleepTime =  timeToSend - currentTime;
					

	const Float64 maxSleepIntervalMilli = 100.0;
	const Float64 minSleepIntervalMilli = 1.0;
	Float64 intervalTime = sleepTime;
	UInt32 sleepMilli = 0;
	if (intervalTime > maxSleepIntervalMilli)
	{	while (intervalTime >= maxSleepIntervalMilli)
		{	
			intervalTime -= maxSleepIntervalMilli;
			sleepMilli = (UInt32) (maxSleepIntervalMilli);
			OSThread::Sleep(sleepMilli);
			fMediaStreamList.UpdateSenderReportsOnStreams();
			//printf("sleepIntervalMilli %u \n",sleepMilli);
		}
	}
	if (intervalTime >= minSleepIntervalMilli)
	{	sleepMilli = (unsigned int) (intervalTime);
		OSThread::Sleep((unsigned int) sleepMilli);
		//printf("sleepMilli %u \n",sleepMilli);
	}
	fMediaStreamList.UpdateSenderReportsOnStreams();
	
	return sleepTime;
}

int QTFileBroadcaster::Play()
{
	SInt16 	err = 0;
	Float64 transmitTime = 0;
	MediaStream	*theStreamPtr = NULL;	
	RTpPacket 	rtpPacket;
	unsigned int sleptTime;
	SInt32 movieStartOffset = 0; //z
	Bool16		negativeTime = false;
	fMovieDuration = fRTPFilePtr->GetMovieDuration();
	fSendTimeOffset = 0.0;
	fMovieStart = true;
	fNumMoviesPlayed ++;
	
	if (fMovieEndTime > 0) // take into account the movie load time as well as the last movie early end.
	{	UInt64 timeNow = PlayListUtils::Milliseconds();
		fMovieIntervalTime = timeNow - fMovieEndTime;

		SInt32 earlySleepTimeMilli = (SInt32)(fMovieTimeDiffMilli - fMovieIntervalTime);
		earlySleepTimeMilli -= 40; // Don't sleep the entire time we need some time to execute or else we will be late
		if (earlySleepTimeMilli > 0)
		{	OSThread::Sleep( earlySleepTimeMilli);
		}
	}
	
	fMovieStartTime = PlayListUtils::Milliseconds();	
	fMediaStreamList.MovieStarted(fMovieStartTime);	
		
	while (true) 
	{
		
		transmitTime = fRTPFilePtr->GetNextPacket(&rtpPacket.fThePacket, &rtpPacket.fLength, (void **)&theStreamPtr);
		err = fRTPFilePtr->Error();
		if (err != QTRTPFile::errNoError)   {err = eMovieFileInvalid; break; } // error getting packet
		if (NULL == rtpPacket.fThePacket)	{err = 0; break; } // end of movie not an error
		if (NULL == theStreamPtr) 			{err = eMovieFileInvalid; break; }// an error
		
		transmitTime *= (Float64) PlayListUtils::eMilli; // convert to milliseconds
		if (transmitTime < 0.0 && negativeTime == false) // Deal with negative transmission times
		{	movieStartOffset += transmitTime / 15;
			negativeTime = true;
		}
		sleptTime = (unsigned int) Sleep(transmitTime);
			
		err = theStreamPtr->Send(&rtpPacket);
		if (err != 0)  { err = eInternalError; break; } 
		
		err = fMediaStreamList.UpdateStreams();
		if (err != 0)  { err = eInternalError; break; } 
		
	};
	
	fMovieEndTime = (SInt64) PlayListUtils::Milliseconds();	
	fMediaStreamList.MovieEnded(fMovieEndTime);

	// see if the movie duration is greater than the time it took to send the packets.
	// the difference is a delay that we insert before playing the next movie.
	SInt64 playDurationMilli = (SInt64) fMovieEndTime - (SInt64) fMovieStartTime;
	fMovieTimeDiffMilli =  ((SInt64) ( (Float64) fMovieDuration * (Float64) PlayListUtils::eMilli)) - (SInt64) playDurationMilli;
	fMovieTimeDiffMilli-= (movieStartOffset/2);

	return err;
}

int QTFileBroadcaster::PlayMovie(char *movieFileName)
{
	
	int err = eMovieFileInvalidName;
	if (movieFileName != NULL)
	{	
	
		err = SetUpAMovie(movieFileName);
		
		if (!err && fPlay)
		{	 err = Play();
		}
	}
	return err;
}


int QTFileBroadcaster::EvalErrorCode(QTRTPFile::ErrorCode err)
{	
	int result = eNoErr;
	
	switch( err ) 
	{
		case QTRTPFile::errNoError:
		break;

		case QTRTPFile::errFileNotFound:
			result = eMovieFileNotFound;
		break;
		
		case QTRTPFile::errNoHintTracks:
			result = eMovieFileNoHintedTracks;
		break;

		case QTRTPFile::errInvalidQuickTimeFile:
			result = eMovieFileInvalid;
		break;
		
		case QTRTPFile::errInternalError:
			result = eInternalError;
		break;
		
		default:
			result = eInternalError;
	}
	return result;
}
