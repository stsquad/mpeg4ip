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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "QTRTPFile.h"

#if __MACOS__
	#include <SIOUX.h>
#endif


/* getopt() replacements */
#ifdef __MACOS__
	int optind = 0;
	
	int optcharcount = 0;
	int optchar[] = { ' ' };
	
	int getopt(int argc, char *argv[], char * optstring);
	int getopt(int /*argc*/, char ** /*argv*/, char * /*optstring*/)
	{
		if( optind >= optcharcount )
			return -1;
		
		return optchar[optind++];
	}
#endif


//extern char		*optarg;
//extern int		optind;
	
int main(int argc, char *argv[]) {
	// Temporary vars
	int			ch;

	// General vars
	int				fd;
	
	const char		*MovieFilename;
	QTRTPFile		*RTPFile;
	bool			Debug = false, DeepDebug = false;


/* argc/argv preloading */
#if __MACOS__
	char			*argv1 = "Movies:cruelintentions.new.mov";
	char			*argv2 = "5";
	char			*argv3 = "6";
	
	argc = 3;
	argv[0] = argv1;
	argv[1] = argv2;
	argv[2] = argv3;
#endif

/* SIOUX setup */
#if __MACOS__
	SIOUXSettings.asktosaveonclose = 0;
#endif

	//
	// Read our command line options
	while( (ch = getopt(argc, argv, "dD")) != -1 ) {
		switch( ch ) {
			case 'd':
				Debug = true;
			break;

			case 'D':
				Debug = true;
				DeepDebug = true;
			break;
		}
	}

	argc -= optind;
	argv += optind;

	//
	// Validate our arguments.
	if( argc < 1 ) {
		printf("usage: QTRTPFileTest <filename> <track#n> <track#n+1> ..\n");
		exit(1);
	}
	
	MovieFilename = *argv++;
	argc--;


	//
	// Open the movie.
	RTPFile = new QTRTPFile(Debug, DeepDebug);
	switch( RTPFile->Initialize(MovieFilename) ) {
		case QTRTPFile::errNoError:
	    case QTRTPFile::errNoHintTracks:
		break;

		case QTRTPFile::errFileNotFound:
			printf("Error!  File not found \"%s\"!\n", MovieFilename);
			exit(1);
		case QTRTPFile::errInvalidQuickTimeFile:
			printf("Error!  Invalid movie file \"%s\"!\n", MovieFilename);
			exit(1);
		case QTRTPFile::errInternalError:
			printf("Error!  Internal error opening movie file \"%s\"!\n", MovieFilename);
			exit(1);
	}
	
	
	//
	// Get the SDP file and print it out.
	{
		// General vars
		char		*SDPFile;
		int			SDPFileLength;
		
		
		//
		// Get the file
		SDPFile = RTPFile->GetSDPFile(&SDPFileLength);
		if( SDPFile == NULL ) {
			printf("Error!  Could not get SDP file!\n");
			exit(1);
		}
		
		write(1, SDPFile, SDPFileLength);
		write(1, "\n", 1);
	}

	//
	// Open our file to write the packets out to.
	fd = open("track.cache", O_CREAT | O_TRUNC | O_WRONLY, 0664);
	if( fd == -1 ) {
		printf("Error!  Could not create output file!\n");
		exit(1);
	}
	
	
	//
	// Add the tracks that we're interested in.
	while(argc--) {
		switch( RTPFile->AddTrack(atoi(*argv)) ) {
			case QTRTPFile::errNoError:
	        case QTRTPFile::errNoHintTracks:
			break;

			case QTRTPFile::errFileNotFound:
			case QTRTPFile::errInvalidQuickTimeFile:
			case QTRTPFile::errInternalError:
				printf("Error!  Invalid movie file \"%s\"!\n", MovieFilename);
				exit(1);
		}

		RTPFile->SetTrackCookie(atoi(*argv), (char *)atoi(*argv));
		(void)RTPFile->GetSeekTimestamp(atoi(*argv));
		argv++;
	}
	
	
	//
	// Display some stats about the movie.
	
	printf("Total RTP bytes of all added tracks: %"_64BITARG_"u\n", RTPFile->GetAddedTracksRTPBytes());
	
#if 0
#if HAVE_64BIT_PRINTF
	printf("Total RTP bytes of all added tracks: %qu\n", RTPFile->GetAddedTracksRTPBytes());
#else
	printf("Total RTP bytes of all added tracks: %lu\n", (UInt32)RTPFile->GetAddedTracksRTPBytes());
#endif
#endif

	//
	// Seek to the beginning of the movie.
	if( RTPFile->Seek(0.0) != QTRTPFile::errNoError ) {
		printf("Error!  Couldn't seek to time 0.0!\n");
		exit(1);
	}
	
#if 0
	//
	// Write out the header.
	tempInt16 = 1;
	write(fd, (char *)&tempInt16, 2);	// isCompletelyWritten
	tempInt16 = 0;
	write(fd, (char *)&tempInt16, 2);	// padding1
	tempFloat64 = file.GetDurationInSeconds();
	write(fd, (char *)&tempFloat64, 8);	// movieLength
	tempInt32 = HintTrack->GetRTPTimescale();
	write(fd, (char *)&tempInt32, 4);	// rtpTimescale
	tempInt16 = HintTrack->GetRTPSequenceNumberRandomOffset();
	write(fd, (char *)&tempInt16, 2);	// seqNumRandomOffset
	tempInt16 = 0;
	write(fd, (char *)&tempInt16, 2);	// padding2
	tempFloat32 = HintTrack->GetTotalRTPBytes() / file.GetDurationInSeconds();
	write(fd, (char *)&tempFloat32, 4);	// dataRate
#endif

	//
	// Suck down packets..
	UInt32		NumberOfPackets = 0;
	Float64		TotalInterpacketDelay = 0.0,
				LastPacketTime = 0.0;
	while(1) {
		// Temporary vars
		UInt16	tempInt16;

		// General vars
		char	*Packet;
		int		PacketLength;
		long	Cookie;
		UInt32	RTPTimestamp;
		UInt16	RTPSequenceNumber;
		

		//
		// Get the next packet.
		Float64 TransmitTime = RTPFile->GetNextPacket(&Packet, &PacketLength, (void **)&Cookie);
		if( Packet == NULL )
			break;

		memcpy(&RTPSequenceNumber, Packet + 2, 2);
		RTPSequenceNumber = ntohs(RTPSequenceNumber);
		memcpy(&RTPTimestamp, Packet + 4, 4);
		RTPTimestamp = ntohl(RTPTimestamp);
		
		printf("TransmitTime = %.2f; Cookie = %ld; SeqNum = %u; TS = %lu\n", TransmitTime, Cookie, RTPSequenceNumber, RTPTimestamp);
		
		//
		// Write out the packet header.
		write(fd, (char *)&TransmitTime, 8);	// transmitTime
		tempInt16 = PacketLength;
		write(fd, (char *)&tempInt16, 2);		// packetLength
		tempInt16 = 0;
		write(fd, (char *)&tempInt16, 2);		// padding1
		
		//
		// Write out the packet.
		write(fd, Packet, PacketLength);
		
		//
		// Compute the Inter-packet delay and keep track of it.
		if( TransmitTime >= LastPacketTime ) {
			TotalInterpacketDelay += TransmitTime - LastPacketTime;
			LastPacketTime = TransmitTime;
			NumberOfPackets++;
		}
		
#if 0
		//
		// Seek..
		if( (TransmitTime > 5.0) && (TransmitTime < 6.0) ) {
			if( RTPFile->Seek(30.0) != QTRTPFile::errNoError ) {
				printf("Error!  Couldn't seek to time 30.0!\n");
				exit(1);
			}
		} else if( (TransmitTime > 31.0) && (TransmitTime < 32.0) ) {
			if( RTPFile->Seek(10.0) != QTRTPFile::errNoError ) {
				printf("Error!  Couldn't seek to time 10.0!\n");
				exit(1);
			}
		} else if( (TransmitTime > 11.0) && (TransmitTime < 15.0) ) {
			RTPFile->SetTrackKeyframesOnlyOption(6, true);
		} else if( (TransmitTime > 15.0) ) {
			RTPFile->SetTrackKeyframesOnlyOption(6, false);
		}
#endif
	}
	
	//
	// Compute and display the Inter-packet delay.
	if( NumberOfPackets > 0 )
	#if HAVE_64BIT_PRINTF
		printf("Average Inter-packet delay: %quus\n", (UInt64)((TotalInterpacketDelay / NumberOfPackets) * 1000 * 1000));
	#else
		printf("Average Inter-packet delay: %luus\n", (UInt32)((TotalInterpacketDelay / NumberOfPackets) * 1000 * 1000));
	#endif
	
	//
	// Close the output file.
	close(fd);

	//
	// Close our RTP file.
	delete RTPFile;

	return 0;
}
