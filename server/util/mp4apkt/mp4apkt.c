/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is MPEG4IP.
 *
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 *
 * Contributor(s):
 *		Dave Mackie		dmackie@cisco.com
 */

/*
 * Notes:
 *  - file formatted with tabstops == 4 spaces
 *  - TBD == "To Be Done"
 */

#include <mpeg4ip.h>
#include <mpeg4ip_getopt.h>
#include <quicktime.h>

/* hard limits on range of RTP packet sizes */
#define MIN_PKTSIZE 512
#define MAX_PKTSIZE	64*1024

#define NUM_BITS_IN_BYTE 8
/* number of milliseconds per second */
#define NUM_MS_IN_SEC 1000 
/* max packet duration for the RTP transport profile 1 in ms */ 
#define RTP_PROFILE_1_MAX 500 

/* globals */
char* progName;

/* extern declarations */
extern bool loadNextAacFrame(FILE* inFile, u_int8_t* buf, u_int* pBufSize,
	bool stripAdts);
extern bool getAacProfile(FILE* inFile, u_int* pProfile);
extern bool getAacSamplingRate(FILE* inFile, u_int* pSamplingRate);
extern bool getAacSamplingRateIndex(FILE* inFile, u_int* pSamplingRateIndex);
extern bool getAacChannelConfig(FILE* inFile, u_int* pChannelConfig);

extern bool loadNextMp3Frame(FILE* inFile, u_char* buf, u_int* pBufSize);
extern bool getMp3SamplingParams(FILE* inFile,
	u_int* pSamplingRate, u_int* pSamplingWindow);
extern bool getMpegLayer(FILE* inFile, u_int* pLayer);
extern void quicktime_set_rtp_hint_timestamp_offset(u_char* hintBuf, 
													u_int* pHintBufSize, int offset);
extern u_int16_t getMaxAACFrameSize (FILE *inFile, int numFrames);

/* forward declarations */
static char* binaryToAscii(u_char* pBuf, u_int bufSize);
void interleaveHint(FILE* mpegFile,	quicktime_t* qtFile, int audioTrack,
					u_int frameRate, u_int frameDuration, bool stripAdts,
					u_int maxPayloadSize, bool trace, u_int payloadNumber,
					u_int dump, int hintTrack, u_int numBufs);


struct SampleHintData {
  u_int frameNum;
  u_int32_t curPos;
  u_int32_t length;
  u_int frameBufSize;
};

/*
 * mp4apkt
 * required arg1 should be the AAC file (ADTS format) or MP3 file
 * required arg2 should be the target packetized QT file
 */
int main(int argc, char** argv)
{
	/* variables controlable from command line */
	u_int dump = 0;					/* --dump or --dump=<uint> */
	bool force = FALSE;				/* --force */
	bool merge = FALSE;				/* --merge */
	u_int maxPayloadSize = 1460;  	/* --payloadsize=<uint> */
	u_int samplesPerSecond = 0;		/* --samplingrate=<uint> */
	u_int samplesPerFrame = 0;		/* --samplingwindow=<uint> */
	bool trace = FALSE;				/* --trace */
	bool stripAdts = TRUE;			/* --adts */
	bool interleave = FALSE;        /* --interleave */

	/* internal variables */
	char* mpegFileName = NULL;
	char* qtFileName = NULL;
	FILE* mpegFile = NULL;
	quicktime_t* qtFile = NULL;
	int audioTrack = 0;
	int hintTrack = 0;
	char* compression = NULL;
	char* payloadName = NULL;
	u_int payloadNumber = 0;
	u_int timeScale;
	u_int frameRate;
	u_int frameDuration;
	u_int maxBytesPerSec = 0;
	u_int mpegLayer;
	enum { UNKNOWN_FILE, AAC_FILE, MP3_FILE } fileType = UNKNOWN_FILE;

	/* begin process command line */
	progName = argv[0];
	while (1) {
		int c = -1;
		int option_index = 0;
		static struct option long_options[] = {
			{ "adts", 0, 0, 'a' },
			{ "dump", 2, 0, 'd' },
			{ "force", 0, 0, 'f' },
			{ "merge", 0, 0, 'm' },
			{ "payloadsize", 1, 0, 'p' },
			{ "samplingrate", 1, 0, 'r' },
			{ "trace", 0, 0, 't' },
			{ "samplingwindow", 1, 0, 'w' },
			{ "interleave", 0, 0, 'i' },
			{ NULL, 0, 0, 0 }
		};

		c = getopt_long_only(argc, argv, "adfms:t",
			long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 'a': {
			stripAdts = FALSE;
			break;
		}
		case 'd': {
			u_int i;
			if (optarg) {
				if (sscanf(optarg, "%u", &i) < 1) {
					dump = 1;
				} else {
					dump = i;
				}
			} else {
				dump = 1;
			}
			break;
		}
		case 'f': {
			force = TRUE;
			break;
		}
		case 'i': {
		  interleave = TRUE;
		  break;
		}
		case 'm': {
			merge = TRUE;
			break;
		}
		case 'p': {
			/* --payloadsize <bytes> */
			u_int i;
			if (sscanf(optarg, "%u", &i) < 1) {
				fprintf(stderr,
					"%s: bad payloadsize specified: %s\n",
					 progName, optarg);
 			} else if (i < MIN_PKTSIZE || i > MAX_PKTSIZE) {
				fprintf(stderr,
					"%s: payloadsize must be between %u and %u\n",
					progName, MIN_PKTSIZE, MAX_PKTSIZE);
			} else {
				maxPayloadSize = i;
			}
			break;
		}
		case 'r': {
			/* --samplingrate <samples/sec> */
			u_int i;
			if (sscanf(optarg, "%u", &i) < 1) {
				fprintf(stderr,
					"%s: bad samplingrate specified: %s\n",
					 progName, optarg);
			} else {
				/*
				 * we just trust the user on the value
				 * Note: the default is to look at the ADTS header
				 * and determine this automatically
				 */
				samplesPerSecond = i;
			}
			break;
		}
		case 't': {
			trace = TRUE;
			break;
		}
		case 'w': {
			/* --samplingwindow <samples/frame> */
			u_int i;
			if (sscanf(optarg, "%u", &i) < 1) {
				fprintf(stderr,
					"%s: bad samplingwindow specified: %s\n",
					 progName, optarg);
			} else {
				/*
				 * we just trust the user on the value
				 */
				samplesPerFrame = i;
			}
			break;
		}
		case '?':
			break;
		default:
			fprintf(stderr, "%s: unknown option specified, ignoring: %c\n",
				progName, c);
		}
	}

	/* check that we have at least two non-option arguments */
	if ((argc - optind) < 2) {
		fprintf(stderr,
			"usage: %s <audio-file> <mp4-file>\n",
			progName);
		exit(1);
	}

	/* point to the specified file names */
	mpegFileName = argv[optind++];
	qtFileName = argv[optind++];

	/* provisionally set file type based on file name extension */
	if (!strcmp(&mpegFileName[strlen(mpegFileName)-4], ".aac")) {
		fileType = AAC_FILE;
	} else if (!strcmp(&mpegFileName[strlen(mpegFileName)-4], ".mp3")) {
		fileType = MP3_FILE;
	} /* else fileType = UNKNOWN_FILE */

	/* warn about extraneous non-option arguments */
	if (optind < argc) {
		fprintf(stderr, "%s: unknown options specified, ignoring: ", progName);
		while (optind < argc) {
			fprintf(stderr, "%s ", argv[optind++]);
		}
		fprintf(stderr, "\n");
	}

	/* end processing of command line */

	/* open MPEG file for reading */
	mpegFile = fopen(mpegFileName, "rb");
	if (mpegFile == NULL) {
		/* error, file doesn't exist or we can't read it */
		fprintf(stderr,
			"%s: error opening %s: %s\n",
			progName, mpegFileName, strerror(errno));
		exit(2);
	}

	/* now check the data that's actually in the file */
	if (!getMpegLayer(mpegFile, &mpegLayer)) {
		fprintf(stderr,
			"%s: data in file doesn't look like MPEG audio\n",
			progName);
		exit(7);
	}
	if (mpegLayer == 0) {
		/* AAC */
		if (fileType == MP3_FILE) {
			fprintf(stderr,
					"%s: Warning, data in file looks like AAC, "
					"using that despite mp3 file extension\n",
					progName);
		}
		fileType = AAC_FILE;
	} else if (mpegLayer == 1) {
		/* MP3 */
		if (fileType == AAC_FILE) {
			fprintf(stderr,
					"%s: Warning, data in file looks like MP3, "
					"using that despite aac file extension\n",
					progName);
		}
		fileType = MP3_FILE;
	} else {
		fprintf(stderr,
			"%s: data in file doesn't look like MPEG audio\n",
			progName);
		exit(8);
	}

	/* check if Quicktime file already exists */
#ifndef WIN32
	if (access(qtFileName, X_OK) == 0) {
#else
	if (_access(qtFileName, X_OK) == 0) {
#endif
		/* exists, check signature */
		if (!quicktime_check_sig(qtFileName)) {
			/* error, not a Quicktime file? */
			fprintf(stderr,
				"%s: error %s: not a valid Quicktime file\n",
				progName, qtFileName);
			exit(3);
		}
	}

	/* open the Quicktime file */
	qtFile = quicktime_open(qtFileName, 1, 1, merge);
	if (qtFile == NULL) {
		fprintf(stderr,
			"%s: error %s: %s\n",
			progName, qtFileName, strerror(errno));
		fclose(mpegFile);
		exit(4);
	}

	/*
	 * Due to limitations of the quicktime library we using,
	 * we can only handle one audio track. So either take an error,
	 * or destroy the existing audio track if -force is specified.
	 */
	if (quicktime_audio_tracks(qtFile) > 0 && !force) {
		fprintf(stderr,
			"%s: error in %s: a audio track already exists\n",
			progName, qtFileName);
		exit(5);
	}

	/*
	 * We need to know the sampling parameters used
	 * in order to create the new audio track
	 */
	if (fileType == MP3_FILE) {
		compression = "mp3 ";
		if (!getMp3SamplingParams(mpegFile,
		  &samplesPerSecond, &samplesPerFrame)) {
			fprintf(stderr,
				"%s: error can't read any audio frames from %s\n",
				progName, mpegFileName);
			exit(6);
		}
	} else { /* fileType == AAC_FILE */
		compression = "mp4a";
		if (!getAacSamplingRate(mpegFile, &samplesPerSecond)) {
			fprintf(stderr,
				"%s: error can't read any audio frames from %s\n",
				progName, mpegFileName);
			exit(6);
		}
		if (samplesPerFrame == 0) {
			samplesPerFrame = 1024;
		}
	}

	timeScale = samplesPerSecond;		/* ticks/sec */
	frameDuration = samplesPerFrame;	/* ticks/frame */
	frameRate = samplesPerSecond / samplesPerFrame; /* frames/sec */

	/* create our new audio track, destroying any previous ones */
	quicktime_set_audio(qtFile, 2, samplesPerSecond, 16,
		0, timeScale, frameDuration, compression);
	audioTrack = 0;

	if (fileType == AAC_FILE) {
		// high quality audio profile
		quicktime_set_iod_audio_profile_level(qtFile, 1);
	}

	/* create audio hint track */
	switch (fileType) {
	case AAC_FILE:
	  payloadName = "mpeg4-simple-A2";
	  payloadNumber = 0;	/* dynamically assign it */
	  break;
	case MP3_FILE:
		payloadName = "MPA";
		payloadNumber = 14;
		break;
	}

	hintTrack = quicktime_set_audio_hint(qtFile, audioTrack,
		payloadName, &payloadNumber, maxPayloadSize);

	if (fileType == AAC_FILE) {
		u_int8_t aacConfigBuf[2];
		int aacConfigBufSize = sizeof(aacConfigBuf);
		u_int profile, samplingRateIndex, channelConfig;
		char 	sdpBuf[1024];
		char*	sConfig;

		/* create the appropriate MP4 decoder config */

		/*
		 * AudioObjectType 			5 bits
		 * samplingFrequencyIndex 	4 bits
		 * channelConfiguration 	4 bits
		 * GA_SpecificConfig
		 * 	FrameLengthFlag 		1 bit 1024 or 960
		 * 	DependsOnCoreCoder		1 bit always 0
		 * 	ExtensionFlag 			1 bit always 0
		 */
		memset(aacConfigBuf, 0, sizeof(aacConfigBuf));
		getAacProfile(mpegFile, &profile);
		getAacSamplingRateIndex(mpegFile, &samplingRateIndex);
		getAacChannelConfiguration(mpegFile, &channelConfig);

		aacConfigBuf[0] =
			((profile + 1) << 3) | ((samplingRateIndex & 0xe) >> 1);
		aacConfigBuf[1] =
			((samplingRateIndex & 0x1) << 7) | (channelConfig << 3);
		if (samplesPerFrame != 1024) {
			aacConfigBuf[1] |= (1 << 2);
		}

		quicktime_set_mp4_audio_decoder_config(qtFile,
			audioTrack, aacConfigBuf, aacConfigBufSize);

		/* convert the configuration object to ASCII form */
		sConfig = binaryToAscii(aacConfigBuf, aacConfigBufSize);

		/* create the appropriate SDP attribute */
		if (sConfig) {
		  sprintf(sdpBuf,
				  "a=fmtp:%u streamtype=5; profile-level-id=15; "
				  "config=%s; SizeLength=13; IndexLength=3; "
				  "IndexDeltaLength=3; Profile=1\015\012",
				  payloadNumber,
				  sConfig);

			/* add this to the QT file's sdp atom */
			quicktime_add_audio_sdp(qtFile, sdpBuf,
				audioTrack, hintTrack);

			/* cleanup, don't want to leak memory */
			free(sConfig);
		}
		/* set the session sdp */
		quicktime_set_session_sdp(qtFile, "a=isma-compliance:1,1,1\015\012");
	}

	if (trace) {
		fprintf(stdout, "  Sample    Size  RTP Pkts (num size)\n");
	}

	if (interleave == 1) { 
	  u_int numFrames = 0;
	  u_int numFramesPerPkt = 0;
	  u_int numBufs = 0;
	  /* get the max size of first 1000 frames in file */
	  /* note: 1000 is arbitrary... */
	  u_int16_t frameSize = getMaxAACFrameSize(mpegFile, 1000);
	  if (frameSize == 0) {
		fprintf(stdout, "error: no frames in file, exiting");
		exit(EXIT_FAILURE);
	  }
	  /* compute number of strides for interleaving */
	  numFrames = RTP_PROFILE_1_MAX * samplesPerSecond 
		/ (samplesPerFrame * NUM_MS_IN_SEC);
	  if (maxPayloadSize < frameSize) {
		// go back to non i/l case!
		interleave = 0;
	  } else {
		numFramesPerPkt = maxPayloadSize / frameSize;
		numBufs = numFrames / numFramesPerPkt;
		interleaveHint(mpegFile,	qtFile, audioTrack, frameRate, 
					   frameDuration, stripAdts,
					   maxPayloadSize, trace, payloadNumber, dump, hintTrack, 
					   numBufs);
	  }
	}

	if (interleave == 0) {
	  u_int hintSampleNum = 1;
	  u_int rtpPktNum = 1;
	  u_int rtpPktSize = 0;
	  u_int frameNum = 1;
	  u_int framesThisHint = 0;
	  u_int bytesThisSec = 0;
	  u_int startRtpPktNumThisSec = rtpPktNum;
	  u_char hintBuf[4*1024];	/* Should be big enough to handle a hint! */
	  u_int hintBufSize = 0;
	  u_int16_t zero16 = 0;
	  u_int32_t zero32 = 0;
	  struct SampleHintData ** sampleDataBuf = NULL;
	  u_int numSamples = 0;
	  u_int frameHeadBufSize = 1024;
	  u_int16_t * frameHeadBuf = NULL;
	  u_int numFrameHeads = 0;

	  if (fileType == AAC_FILE) {
		frameHeadBuf = (u_int16_t *) malloc(frameHeadBufSize * sizeof(u_int16_t));
		if (frameHeadBuf == NULL) {
		  fprintf(stderr, "%s: error: memory allocation failed\n", progName);
		  exit(EXIT_FAILURE);
		}
	  }

	  /* initialize the first hint sample */
	  quicktime_init_hint_sample(hintBuf, &hintBufSize);
	  
	  /*
	   * add a sacrifical first packet to the first hint sample
	   * this is sent before the first RTCP SR
	   * which seems to be unfriendly to the client
	   * the UCL RTP library at least seems to be unhappy about it
	   */
	  quicktime_add_hint_packet(hintBuf, &hintBufSize, payloadNumber, rtpPktNum);
	  switch (fileType) {
	  case AAC_FILE:
		/* need headers length header?? */
		quicktime_add_hint_immed_data(hintBuf, &hintBufSize,
									  (u_char*)&zero16, sizeof(u_int16_t));
		break;
	  case MP3_FILE:
		quicktime_add_hint_immed_data(hintBuf, &hintBufSize,
									  (u_char*)&zero32, sizeof(u_int32_t));
		break;
	  }
	  /* amf: can add directly b/c already end of packet -- right? */
	  quicktime_add_hint_sample_data(hintBuf, &hintBufSize, 1, 0, 0); 
	  rtpPktNum++;
	  
	  /* add a second packet to the first hint */
	  quicktime_add_hint_packet(hintBuf, &hintBufSize, 
								payloadNumber, rtpPktNum);
	  
	  while (1) {
		static u_char frameBuf[8*1024];
		u_int frameBufSize = sizeof(frameBuf);
		bool rc = FALSE;

		/* get the next frame from the file */
		switch (fileType) {
		case AAC_FILE:
		  rc = loadNextAacFrame(mpegFile, frameBuf, &frameBufSize, stripAdts);
		  break;
		case MP3_FILE:
		  rc = loadNextMp3Frame(mpegFile, frameBuf, &frameBufSize);
		  break;
		}
		if (!rc) {
		  /* EOF or IO error */
		  break;
		}
		/* write the object, each is a sample and gets its own chunk */
		quicktime_write_audio_frame(qtFile,
									&frameBuf[0], frameBufSize, audioTrack);

		/* add this to our count of bytes sent in this 1 second period */
		bytesThisSec += frameBufSize;

		/* trace for debugging */
		if (trace) {
		  fprintf(stdout, "%8u %8u ", frameNum, frameBufSize);
		}

		{ /* now figure out what to put in the hint track */
		  u_int32_t curPos = 0;
		  u_int32_t nextPos = 0;
		  u_int32_t length = 0;
		  u_int16_t frameSize;

		  /*
		   * if the current RTP pkt has data in it
		   * AND this frame will not fit into current RTP pkt
		   * write the pkt, start next
		   */
		  if (rtpPktSize > 0 && frameBufSize + sizeof(u_int16_t)
			  > maxPayloadSize - rtpPktSize) {
			
			if (fileType == AAC_FILE) {
			  u_int i = 0;
			  u_int16_t headersLen = htons((u_int16_t)numFrameHeads *
										 sizeof(u_int16_t) * NUM_BITS_IN_BYTE);
			  /* add frames headers length header */
			  quicktime_add_hint_immed_data(hintBuf, &hintBufSize,
											(u_char*)&(headersLen), 
											sizeof(u_int16_t));
			  /* add all AU headers for the packet */
			  for (i = 0; i < numFrameHeads; i++) {
				quicktime_add_hint_immed_data(hintBuf, &hintBufSize,
											  (u_char*)(frameHeadBuf+i),
											  sizeof(u_int16_t)); 
			  }
			  /* reset counters */
			  numFrameHeads = 0;
			  /* add sample data for current pkt into hint */
			  for (i = 0; i < numSamples; i++) {
				quicktime_add_hint_sample_data(hintBuf, &hintBufSize,
											   (*(sampleDataBuf+i))->frameNum,
											   (*(sampleDataBuf+i))->curPos,
											   (*(sampleDataBuf+i))->length); 
			  }
			  /* reset buffer & counter */
			  for (i = 0; i < numSamples; i++) {
				free(*(sampleDataBuf+i));
			  }
			  numSamples = 0;
			}

			/* start a new RTP pkt */
			rtpPktNum++;
			rtpPktSize = 0;

			/* write out hint sample */
			quicktime_write_audio_hint(qtFile, hintBuf, hintBufSize, 
									   audioTrack, hintTrack, 
									   framesThisHint * frameDuration);
			hintSampleNum++;
			framesThisHint = 0;

			if (dump > 1) {
			  fprintf(stdout, "\nhint sample %u\n", hintSampleNum - 1);
			  quicktime_dump_hint_sample(hintBuf);
			}

			/* start a new hint sample */
			quicktime_init_hint_sample(hintBuf, &hintBufSize);
			quicktime_add_hint_packet(hintBuf, &hintBufSize,
									  payloadNumber, rtpPktNum);
		  }
		  framesThisHint++;

		  /* add payload header to buffer(will be added as immed data later) */
		  if (fileType == AAC_FILE) {
			/* AAC payload is 2 byte frame size */
			rtpPktSize += sizeof(u_int16_t); /* for AU header length field */
			frameSize = htons(((u_int16_t)frameBufSize) << 3);
			/* headers buffer */
			if (numFrameHeads >= frameHeadBufSize) {
			  /* reallocate mem */
			  u_int16_t * tmp;
			  frameHeadBufSize *= 2;
			  tmp = realloc(frameHeadBuf, 
							frameHeadBufSize * sizeof(u_int16_t));
			  if (tmp == NULL) {
				fprintf(stderr,
						"%s: error: memory allocation failed\n",
						progName);
				exit(EXIT_FAILURE);
			  }
			  frameHeadBuf = tmp;
			}
			frameHeadBuf[numFrameHeads] = frameSize;
			numFrameHeads += 1;
			rtpPktSize += sizeof(u_int16_t);
		  } else if (fileType == MP3_FILE) {
			/* MP3 payload (RFC-2250) is 4 bytes of zeros */
			quicktime_add_hint_immed_data(hintBuf, &hintBufSize,
										  (u_char*)&zero32, sizeof(u_int32_t));
			rtpPktSize += sizeof(u_int32_t);
		  }

		  do { /* while there is data to process in the frame */
			/* determine the next block of audio data for the pkt */
			length = MIN((frameBufSize - curPos),
						 maxPayloadSize - rtpPktSize);
			nextPos = curPos + length;

			/* add sample data */
			if (fileType == AAC_FILE) {
			  struct SampleHintData ** tmp = (struct SampleHintData **)
				realloc(sampleDataBuf,
						(numSamples+1) *
						sizeof(struct SampleHintData *));
			  if (tmp == NULL) {
				fprintf(stderr,
						"%s: error: memory allocation failed\n",
						progName);
				exit(EXIT_FAILURE);
			  }
			  sampleDataBuf = tmp;
			  *(sampleDataBuf + numSamples) = (struct SampleHintData *) 
				                              calloc(1, 
											   sizeof(struct SampleHintData));
			  if (sampleDataBuf+numSamples == NULL) {
				fprintf(stderr, 
						"%s: error: memory allocation failed\n", 
						progName);
				exit(EXIT_FAILURE);
			  }
			  /* finally buffer the sample hint data */
			  (*(sampleDataBuf+numSamples))->frameNum = frameNum;
			  (*(sampleDataBuf+numSamples))->curPos = curPos;
			  (*(sampleDataBuf+numSamples))->length = length;
			  numSamples++;
			}
			else if (fileType == MP3_FILE) {
			  quicktime_add_hint_sample_data(hintBuf, &hintBufSize,
											 frameNum, curPos, length);
			}
			rtpPktSize += length;
			
			if (trace) {
			  fprintf(stdout, " (%u %u)", rtpPktNum, rtpPktSize);
			}

			/* if there is more frame data - frame will be fragmented*/
			if (nextPos < frameBufSize) {
			  if (fileType == AAC_FILE) {
				u_int i = 0;
				u_int16_t headersLen = htons((u_int16_t)numFrameHeads
										   * sizeof(u_int16_t) 
										   * NUM_BITS_IN_BYTE);
				/* add frame headers length header */
				quicktime_add_hint_immed_data(hintBuf, &hintBufSize,
											  (u_char*)&headersLen, 
											  sizeof(u_int16_t));
				/* add all AU headers for the packet */
				for (i=0; i < numFrameHeads; i++) {
				  quicktime_add_hint_immed_data(hintBuf, &hintBufSize,
												(u_char*)(frameHeadBuf+i),
												sizeof(u_int16_t));
				}
				/* reset counter */
				numFrameHeads = 0;
				/* add sample data for current pkt into hint */
				for (i = 0; i < numSamples; i++) {
				  quicktime_add_hint_sample_data(hintBuf, &hintBufSize,
												 (*(sampleDataBuf+i))->frameNum,
												 (*(sampleDataBuf+i))->curPos,
												 (*(sampleDataBuf+i))->length);
				}
				/* reset buffer & counter */
				for (i = 0; i < numSamples; i++) {
				  free((void *)*(sampleDataBuf+i));
				}
				numSamples = 0;
			  }
			  /* start a new RTP pkt */
			  rtpPktNum++;
			  rtpPktSize = 0;
			  /* create a new packet in the current sample */
			  quicktime_add_hint_packet(hintBuf, &hintBufSize,
										payloadNumber, rtpPktNum);

			  if (fileType == MP3_FILE) {
				/* MP3 payload (RFC-2250) */
				u_int16_t fragOffset;

				/* 2 bytes of zero (that's efficient!) */
				quicktime_add_hint_immed_data(hintBuf, &hintBufSize,
											  (u_char*)&zero16, 
											  sizeof(u_int16_t));
				rtpPktSize += sizeof(u_int16_t);

				/* and 2 byte fragment offset */
				fragOffset = htons((u_int16_t)curPos);
				quicktime_add_hint_immed_data(hintBuf, &hintBufSize,
											  (u_char*)&fragOffset, 
											  sizeof(u_int16_t));
				rtpPktSize += sizeof(u_int16_t);
			  } else {
				/* AAC payload is 2 byte frame size */
				rtpPktSize += sizeof(u_int16_t); /* for AU header length field */
				frameSize = htons(((u_int16_t)frameBufSize) << 3);
				/* headers buffer */
				frameHeadBuf[numFrameHeads] = frameSize;
				numFrameHeads += 1;
				rtpPktSize += sizeof(u_int16_t);
			  }
			}
			/* move along in frame buffer */
			curPos = nextPos;
		  } while (nextPos < frameBufSize);
		  /* frame has been processed */
		  
		  /* set Mbit */
		  /* setting it here ensures that it will only be set in 
		   * the last packet when a frame is fragmented */
		  quicktime_set_hint_Mbit(hintBuf);

		  /* if we've fragmented the frame */
		  /* we aren't allowed to put more frames in this RTP pkt */
		  if (frameBufSize > maxPayloadSize - 2 * sizeof(u_int16_t)) {
			/* start a new RTP pkt */
			rtpPktNum++;
			rtpPktSize = 0;
		  }

		  /* if we're ready to start a new RTP packet at this point */
		  if (rtpPktSize == 0) {
			if (fileType == AAC_FILE) {
			  u_int i = 0;
			  u_int16_t headersLen = htons((u_int16_t)numFrameHeads
										 * sizeof(u_int16_t) 
										 * NUM_BITS_IN_BYTE);
			  /* add header info */
			  quicktime_add_hint_immed_data(hintBuf, &hintBufSize,
											(u_char*)&headersLen, 
											sizeof(u_int16_t));

			  /* add all AU headers for the packet */
			  for (i=0; i < numFrameHeads; i++) {
				quicktime_add_hint_immed_data(hintBuf, &hintBufSize,
											  (u_char*)(frameHeadBuf+i),
											  sizeof(u_int16_t));

			  }
			  numFrameHeads = 0;

			  /* add sample data for current pkt into hint */
			  for (i=0; i < numSamples; i++) {
				quicktime_add_hint_sample_data(hintBuf, &hintBufSize,
											   (*(sampleDataBuf+i))->frameNum,
											   (*(sampleDataBuf+i))->curPos,
											   (*(sampleDataBuf+i))->length);
			  }

			  /* reset buffer & counter */
			  for (i = 0; i < numSamples; i++) {
				free((void *)*(sampleDataBuf+i));
			  }
			  numSamples = 0;
			}

			/* write out the hint sample */
			quicktime_write_audio_hint(qtFile, hintBuf, hintBufSize,
									   audioTrack, hintTrack, 
									   framesThisHint * frameDuration);
			hintSampleNum++;
			framesThisHint = 0;

			if (dump > 1) {
			  fprintf(stdout, "\nhint sample %u\n", hintSampleNum - 1);
			  quicktime_dump_hint_sample(hintBuf);
			}

			/* start a new hint sample */
			quicktime_init_hint_sample(hintBuf, &hintBufSize);
			quicktime_add_hint_packet(hintBuf, &hintBufSize,
									  payloadNumber, rtpPktNum);
		  }

		} /* end of hint track processing block */

		if (trace) {
		  fprintf(stdout, "\n");
		}

		frameNum++;	/* on to the next audio frame */

		/* if we're at the end of a 1 second period */
		if ((frameNum % frameRate) == 0) {
		  /* compute how many RTP pkts were generated for this second */
		  u_int pktsThisSec = rtpPktNum - startRtpPktNumThisSec;

		  /* record the current maximum bytes in any given 1 second */
		  maxBytesPerSec = MAX(maxBytesPerSec,
							   (pktsThisSec * RTP_HEADER_STD_SIZE) 
							   + bytesThisSec);
		  
		  /* reset counters */
		  bytesThisSec = 0;
		  startRtpPktNumThisSec = rtpPktNum;
		}

	  } /* end of while (1) */

	  /* if we have a final pending hint */
	  if (rtpPktSize > 0) {
		if (fileType == AAC_FILE) {
		  u_int i = 0;
		  u_int16_t headersLen = htons((u_int16_t)numFrameHeads
									 * sizeof(u_int16_t) * NUM_BITS_IN_BYTE);
		  /* add frame headers length header */
		  quicktime_add_hint_immed_data(hintBuf, &hintBufSize,
										(u_char*)&headersLen, sizeof(u_int16_t));

		  /* add all AU headers for the packet */
		  for (i = 0; i < numFrameHeads; i++) {
			quicktime_add_hint_immed_data(hintBuf, &hintBufSize,
										  (u_char*)(frameHeadBuf+i),
										  sizeof(u_int16_t));

		  }
		  /* add sample data for current pkt into hint */
		  for (i = 0; i < numSamples; i++) {
			quicktime_add_hint_sample_data(hintBuf, &hintBufSize,
										   (*(sampleDataBuf+i))->frameNum,
										   (*(sampleDataBuf+i))->curPos,
										   (*(sampleDataBuf+i))->length);
		  }

		  /* free buffers */
		  for (i = 0; i < numSamples; i++) {
			free((void *)*(sampleDataBuf+i));
		  }
		  free ((void *)(sampleDataBuf));
		  numSamples = 0;
		}

		/* write out hint sample */
		quicktime_write_audio_hint(qtFile, hintBuf, hintBufSize,
								   audioTrack, hintTrack, 
								   framesThisHint * frameDuration);
		hintSampleNum++;
		framesThisHint = 0;

		if (dump > 1) {
		  fprintf(stdout, "\nhint sample %u\n", hintSampleNum - 1);
		  quicktime_dump_hint_sample(hintBuf);
		}
	  }

	  if (trace) {
		fprintf(stdout, "\n");
	  }

	  /* record maximum bits per second in Quicktime file */
	  quicktime_set_audio_hint_max_rate(qtFile, 1000, maxBytesPerSec * 8,
										audioTrack, hintTrack);

	} /* end of non-interleave case */

	/* write out the Quicktime file */
	quicktime_write(qtFile);

	/* dump quicktime atoms if desired */
	if (dump > 0) {
		quicktime_dump(qtFile);
	}

	/* cleanup */
	fclose(mpegFile);
	quicktime_destroy(qtFile);
	
	exit(0);
}

static char* binaryToAscii(u_char* pBuf, u_int bufSize)
{
	char* s = (char*)malloc(2 * bufSize + 1);
	u_int i;

	if (s == NULL) {
		return NULL;
	}

	for (i = 0; i < bufSize; i++) {
		sprintf(&s[i*2], "%02x", pBuf[i]);
	}

	return s;	/* N.B. caller is responsible for free'ing s */
}


 struct ILeaveData {
   bool isused;
   bool iscur;
   bool isFragment;
   u_int rtpPktSize;
   u_int numFrameHeads;
   u_int frameHeadBufSize;
   u_int sampleBufSize;
   u_int numSamples;
   u_int frameNum; /* used to compute AU index delta */
   u_int rtpPktDuration;
 };


/* * get next index in the buffer (buffer wraps around * */
u_int getNextPos(u_int cur, u_int numBufs)
{
  if (cur == numBufs-1)
	return (0);
  else
	return (cur + 1);
}

void createBuffers(u_int numBufs, u_int16_t *** frameHeadBuf,  
				   struct SampleHintData *** sampleDataBuf,
				   struct ILeaveData ** ilDataBuf)
{
  int i = 0;
  
  /* allocate for interleave data */
  *ilDataBuf = (struct ILeaveData *) malloc(numBufs * sizeof(struct ILeaveData));
  if (*ilDataBuf == NULL ) {
	fprintf(stderr, "%s: error: memory allocation failed\n", progName);
	exit(EXIT_FAILURE);
  }
  memset(*ilDataBuf, 0, numBufs * sizeof(struct ILeaveData));
  for (i = 0; i < numBufs; i++) {
	(*ilDataBuf)[i].frameHeadBufSize = 1024;
	(*ilDataBuf)[i].sampleBufSize = 1024;
  }

  /* allocate mem for frame header bufs */
  *frameHeadBuf = (u_int16_t **) malloc(numBufs * sizeof(u_int16_t *));
  if (*frameHeadBuf == NULL) {
	fprintf(stderr, "%s: error: memory allocation failed\n", progName);
	exit(EXIT_FAILURE);
  }
  memset(*frameHeadBuf, 0, numBufs * sizeof(u_int16_t *));
  for (i = 0; i < numBufs; i++) {
	(*frameHeadBuf)[i] = (u_int16_t *) malloc((*ilDataBuf)[i].frameHeadBufSize 
											  * sizeof(u_int16_t));
	if ((*frameHeadBuf)[i] == NULL) {
	  fprintf(stderr, "%s: error: memory allocation failed\n", progName);
	  exit(EXIT_FAILURE);
	}
	memset((*frameHeadBuf)[i], 0, 
		   (*ilDataBuf)[i].frameHeadBufSize * sizeof(u_int16_t));
  }

  /* allocate sample data buf */
  *sampleDataBuf =  (struct SampleHintData **) 
	malloc(numBufs * sizeof(struct SampleHintData *));
  if ((*sampleDataBuf) == NULL) {
	fprintf(stderr, "%s: error: memory allocation failed\n", progName);
	exit(EXIT_FAILURE);
  }
  memset((*sampleDataBuf), 0, numBufs * sizeof(struct sampleHintData *));
  for (i = 0; i < numBufs; i++) {
	(*sampleDataBuf)[i] = (struct  SampleHintData *) 
	                      malloc((*ilDataBuf)[i].sampleBufSize
								 * sizeof(struct SampleHintData));
	if ((*sampleDataBuf)[i] == NULL) {
	  fprintf(stderr, "%s: error: memory allocation failed\n", progName);
	  exit(EXIT_FAILURE);
	}
	memset((*sampleDataBuf)[i], 0, 
		   (*ilDataBuf)[i].sampleBufSize * sizeof(struct SampleHintData));
  }
}

void freeBuffers(u_int numBufs, u_int16_t ** frameHeadBuf,  
				 struct SampleHintData ** sampleDataBuf,
				 struct ILeaveData * ilDataBuf)
{
  int i;
  /* */
  for (i = 0; i < numBufs; i++) {
	free(frameHeadBuf[i]);
	free(sampleDataBuf[i]);
  }
  free(frameHeadBuf);
  free(sampleDataBuf);
  free(ilDataBuf);
}

/* * write the packets start through cur to the hint buffer * */
void writePkts(u_int cur, u_int start, u_int numBufs, u_int * rtpPktNum, 
			   u_char * hintBuf, u_int * hintBufSize, 
			   u_int16_t ** frameHeadBuf,
			   struct SampleHintData ** sampleDataBuf,
			   struct ILeaveData * ilDataBuf, quicktime_t* qtFile, 
			   int audioTrack, u_int payloadNumber, int hintTrack, 
			   u_int frameDuration, u_int dump, u_int * maxBytesPerSec, 
			   u_int frameRate, u_int *framesWritten, bool trace)
{
  int j = 0;
  u_int framesThisHint = 0;
  u_int hintSampleNum = 1;
  static u_int bytesThisSec = 0;
  static u_int framesThisSec = 0;
  static u_int startRtpPktNumThisSec = 1;
  static u_int frameCount = 1; 

  /* figure out how many packets to write */
  int count;
  int curPkt = start;
  if (cur >= start) {
	count = cur - start;
  } else {
	count = (numBufs - start) + cur;/* took out +1 - will reg i/l still work?*/
  }
  /* write the packets */
  for (j = 0; j <= count; j++) {
	u_int i = 0;
	u_int16_t headersLen;

	/* fix timestamp */
	int offset = ((*(*(sampleDataBuf+curPkt))).frameNum - frameCount);
	int tsOffset = ((*(*(sampleDataBuf+curPkt))).frameNum - frameCount) 
	               * frameDuration;

	if (trace) { 
	  fprintf(stdout, "framecount: %u, offset: %d, tsoffset: %d, ", 
			  frameCount, offset, tsOffset);
	}
	if (tsOffset != 0) {
	  quicktime_set_rtp_hint_timestamp_offset(hintBuf, hintBufSize, tsOffset);
	}

	/* only increment frame count if not a fragment (unless last frag) */
	if (ilDataBuf[curPkt].isFragment == FALSE)
	  frameCount += ilDataBuf[curPkt].numSamples; 

	headersLen = htons((u_int16_t)ilDataBuf[curPkt].numFrameHeads *
					  sizeof(u_int16_t) * NUM_BITS_IN_BYTE);

	
	if (ilDataBuf[curPkt].isFragment == FALSE)
	  framesThisHint += ilDataBuf[curPkt].numSamples;

	/* add frames headers length header */
	quicktime_add_hint_immed_data(hintBuf, hintBufSize,
								  (u_char*)&(headersLen), sizeof(u_int16_t));
	/* add all AU headers for the packet */
	for (i = 0; i < ilDataBuf[curPkt].numFrameHeads; i++) {
	  quicktime_add_hint_immed_data(hintBuf, hintBufSize,
									(u_char*)(*(frameHeadBuf+curPkt)+i),
									sizeof(u_int16_t));
	}
	/* reset counters */
	ilDataBuf[curPkt].numFrameHeads = 0;
	if (trace) {
	  fprintf(stdout, "rtp: %u, frameNums: ", *rtpPktNum);
	}
	/* add sample data for current pkt into hint */
	for (i = 0; i < ilDataBuf[curPkt].numSamples; i++) {
	  quicktime_add_hint_sample_data(hintBuf, hintBufSize,
									 (*(*(sampleDataBuf+curPkt) +i)).frameNum,
									 (*(*(sampleDataBuf+curPkt) +i)).curPos,
									 (*(*(sampleDataBuf+curPkt) +i)).length);
	  
	  if (trace) {
		fprintf(stdout, "%u-%u-%u, ", (*(*(sampleDataBuf+curPkt) +i)).frameNum,
				(*(*(sampleDataBuf+curPkt) +i)).curPos,
				(*(*(sampleDataBuf+curPkt) +i)).length);
	  }
	  framesThisSec++;
	  bytesThisSec += (*(*(sampleDataBuf+curPkt) +i)).frameBufSize;
	  if ((framesThisSec % frameRate) == 0) {
		/* num packets generated this sec */
		u_int pktsThisSec = *rtpPktNum - startRtpPktNumThisSec;
		/* current maximum bytes in any given 1 second */
		*maxBytesPerSec = MAX(*maxBytesPerSec,
							 (pktsThisSec * RTP_HEADER_STD_SIZE) + bytesThisSec);
		/* reset counters */
		bytesThisSec = 0;
		startRtpPktNumThisSec = *rtpPktNum;
	  } 
	  (*framesWritten)++;
	}
	/* set Mbit - once per packet unless not last pkt of a fragment */
	if (ilDataBuf[curPkt].isFragment == FALSE)
	  quicktime_set_hint_Mbit(hintBuf);
	
	/* deal with hints */
	(*rtpPktNum)++;
	if (ilDataBuf[curPkt].isFragment == FALSE) {
	  quicktime_write_audio_hint(qtFile, hintBuf, *hintBufSize,
								 audioTrack, hintTrack, 
								 framesThisHint * frameDuration);
	  hintSampleNum++;
	  if (dump > 1) {
		fprintf(stdout, "\nhint sample %u\n", hintSampleNum - 1);
		quicktime_dump_hint_sample(hintBuf);
	  }
	  quicktime_init_hint_sample(hintBuf, hintBufSize);
	  framesThisHint = 0;
	}
	quicktime_add_hint_packet(hintBuf, hintBufSize, payloadNumber, *rtpPktNum);

	ilDataBuf[curPkt].isused = FALSE;
	ilDataBuf[curPkt].iscur = FALSE;
	ilDataBuf[curPkt].rtpPktSize = 0;
	ilDataBuf[curPkt].rtpPktDuration = 0;
	ilDataBuf[curPkt].numFrameHeads = 0;
	ilDataBuf[curPkt].numSamples = 0;
	ilDataBuf[curPkt].frameNum = 0;
	ilDataBuf[curPkt].isFragment = FALSE;

	curPkt = getNextPos(curPkt, numBufs);
	if (trace) {
	  fprintf(stdout, "\n");
	}
  }
}

/* * For interleaving: add sample data to buffer * */
void addSampleData(struct SampleHintData *** sampleDataBuf, u_int cur, 
				   struct ILeaveData * ilDataBuf, u_int frameNum, 
				   u_int32_t length, u_int32_t curPos, u_int secondsPerFrame,
				   u_int frameBufSize)
{
  if (ilDataBuf[cur].numSamples >= ilDataBuf[cur].sampleBufSize) {
	struct SampleHintData * tmp = NULL;
	tmp = (struct SampleHintData *) realloc((*sampleDataBuf)[cur],
											ilDataBuf[cur].sampleBufSize * 2
											* sizeof(struct SampleHintData));
	if (tmp == NULL) {
	  /* exit */
	  fprintf(stderr, "%s: error: memory allocation failed\n", progName);
	  exit(EXIT_FAILURE);
	}
	ilDataBuf[cur].sampleBufSize *= 2;
	(*sampleDataBuf)[cur] = tmp;
  }
  /* finally buffer the sample hint data */
  (*(*(*sampleDataBuf+cur)
	 + ilDataBuf[cur].numSamples)).frameNum = frameNum;
  (*(*(*sampleDataBuf+cur)
	 + ilDataBuf[cur].numSamples)).curPos = curPos;
  (*(*(*sampleDataBuf+cur)
	 + ilDataBuf[cur].numSamples)).length = length;
  (*(*(*sampleDataBuf+cur)
	 + ilDataBuf[cur].numSamples)).frameBufSize = frameBufSize;
		
  ilDataBuf[cur].numSamples++;
  ilDataBuf[cur].frameNum = frameNum;
  ilDataBuf[cur].isused = TRUE;
  ilDataBuf[cur].rtpPktSize += length;
  ilDataBuf[cur].rtpPktDuration += secondsPerFrame; 

}


/* * Main task for Interleaving * */
void interleaveHint(FILE* mpegFile,	quicktime_t* qtFile, int audioTrack,
					u_int frameRate, u_int frameDuration, bool stripAdts,
					u_int maxPayloadSize, bool trace, u_int payloadNumber,
					u_int dump, int hintTrack, u_int numBufs)

{
  u_int maxBytesPerSec = 0;
  const u_int MAX_AU_INDEX = 7;
  u_int rtpPktNum = 1;
  u_int frameNum = 1;
  u_char hintBuf[4*1024];	/* Should be big enough to handle a hint! */
  u_int hintBufSize = 0;
  u_int16_t zero16 = 0;
  struct SampleHintData ** sampleDataBuf = NULL; /* pts to numBufs ptrs to data */
  u_int16_t **frameHeadBuf = NULL; /* pts to numBufs header buffers */
  u_int cur = 0; /* current buffer processed */
  u_int start = 0;
  u_int auIndex = 0;
  struct ILeaveData * ilDataBuf = NULL; /* interleave data buffer */
  u_int framesWritten=0;
  u_int framesRead = 0;
  u_int secondsPerFrame = 1 / frameRate; /* num seconds per frame */
  int i = 0;  
  /* */

  createBuffers(numBufs, &frameHeadBuf, &sampleDataBuf, &ilDataBuf);

  /* initialize the first hint sample */
  quicktime_init_hint_sample(hintBuf, &hintBufSize);

  /* add a sacrifical first packet to the first hint sample */
  quicktime_add_hint_packet(hintBuf, &hintBufSize, payloadNumber, rtpPktNum);
  /* amf: need headers length header? */
  quicktime_add_hint_immed_data(hintBuf, &hintBufSize,
								(u_char*)&zero16, sizeof(u_int16_t));
  quicktime_add_hint_sample_data(hintBuf, &hintBufSize, 1, 0, 0);
  rtpPktNum++;

  /* add a second packet to the first hint */
  quicktime_add_hint_packet(hintBuf, &hintBufSize, payloadNumber, rtpPktNum);
 
  while (1) {
	static u_char frameBuf[8*1024];
	u_int frameBufSize = sizeof(frameBuf);
	bool rc = FALSE;
	/* get the next frame from the file */
	rc = loadNextAacFrame(mpegFile, frameBuf, &frameBufSize, stripAdts);
	if (!rc) {
	  /* EOF or IO error */
	  break;
	}

	framesRead++;
	auIndex = getNextPos(auIndex, MAX_AU_INDEX);

	/* write the object, each is a sample and gets its own chunk */
	quicktime_write_audio_frame(qtFile,
								&frameBuf[0], frameBufSize, audioTrack);

	/* trace for debugging */
	if (trace) {
	  fprintf(stdout, "%8u %8u ", frameNum, frameBufSize);
	}

	{ /* now figure out what to put in the hint track */
	  u_int32_t nextPos = 0;
	  u_int32_t curPos = 0;
	  u_int32_t length = 0;
	  u_int16_t auHeader = 0;
	  u_int16_t auInfo = 0;

	  /*
	   * if the current RTP pkt has data in it
	   * AND this frame will not fit into current RTP pkt
	   * write the start to cur pkts, and
	   * find out where frame belongs 
	   */
	  while (ilDataBuf[cur].rtpPktSize > 0 &&
			 frameBufSize + sizeof(u_int16_t)
			 > maxPayloadSize - ilDataBuf[cur].rtpPktSize
			 || ilDataBuf[cur].rtpPktDuration + secondsPerFrame 
			 > RTP_PROFILE_1_MAX) {
		/* frame does not fit in current pkt so write it */
		writePkts(cur, start, numBufs, &rtpPktNum,
				  hintBuf, &hintBufSize,
				  frameHeadBuf,  sampleDataBuf, ilDataBuf,
				  qtFile, audioTrack, payloadNumber,
				  hintTrack, frameDuration, dump, &maxBytesPerSec, 
				  frameRate, &framesWritten, trace);

		cur = getNextPos(cur, numBufs);
		start = cur;
	  }
	  /* frame will fit into cur 
	   * - or cur is empty and frame will be fragmented */

	  /* add payload header to cur buffer 
	   * for AU header length field */
	  ilDataBuf[cur].rtpPktSize += sizeof(u_int16_t);
	  auInfo = ilDataBuf[cur].numFrameHeads ?
		(frameNum - ilDataBuf[cur].frameNum - 1) : auIndex;
	  auHeader = htons((((u_int16_t)frameBufSize) << 3) | auInfo);

	  /* headers buffer */
	  if (ilDataBuf[cur].numFrameHeads >= ilDataBuf[cur].frameHeadBufSize) {
		/* reallocate mem - double size */
		u_int16_t * tmp;
		ilDataBuf[cur].frameHeadBufSize *= 2;
		tmp = realloc(frameHeadBuf[cur], ilDataBuf[cur].frameHeadBufSize 
					  * sizeof(u_int16_t));
		if (tmp == NULL) {	 /* clean up and exit */
		  fprintf(stderr, "%s: error: memory allocation failed\n", progName);
		  exit(EXIT_FAILURE);
		}
		frameHeadBuf[cur] = tmp;
	  }
	  (*(frameHeadBuf + cur))[ilDataBuf[cur].numFrameHeads] = auHeader;
	  ilDataBuf[cur].numFrameHeads += 1;
	  ilDataBuf[cur].rtpPktSize += sizeof(u_int16_t);

	  do { 
		/* while there is data to process in the frame */
		/* determine the next block of audio data for the pkt */
		length = MIN((frameBufSize - curPos),
					 maxPayloadSize - ilDataBuf[cur].rtpPktSize);
		nextPos = curPos + length;

		/* add sample data */
		addSampleData(&sampleDataBuf, cur, ilDataBuf, frameNum, 
					  length, curPos, secondsPerFrame, frameBufSize);

		if (trace) {
		  fprintf(stdout, " (%u %u)", rtpPktNum,
				  ilDataBuf[cur].rtpPktSize);
		}
		
		/* frame will be fragmented */
		if (nextPos < frameBufSize) {
		   /* ensures that the Mbit will not be set */
		  ilDataBuf[cur].isFragment = TRUE; 

		  writePkts(cur, start, numBufs, &rtpPktNum,
					hintBuf, &hintBufSize,
					frameHeadBuf,  sampleDataBuf,
					ilDataBuf, qtFile, audioTrack, payloadNumber,
					hintTrack, frameDuration, dump, &maxBytesPerSec, 
					frameRate, &framesWritten, trace);

		  /* start a new RTP pkt */
		  cur = getNextPos(cur, numBufs);
		  ilDataBuf[cur].iscur = TRUE;
		  start = cur;
		  ilDataBuf[cur].rtpPktSize = 0;
		  ilDataBuf[cur].rtpPktDuration = 0;
		  /* add payload header to cur buffer */
		  /* for AU header length field */
		  ilDataBuf[cur].rtpPktSize += sizeof(u_int16_t);
		  auInfo = ilDataBuf[cur].numFrameHeads ?
			(frameNum - ilDataBuf[cur].frameNum - 1) : auIndex;
		  auHeader = htons((((u_int16_t)frameBufSize) << 3) | auInfo);
		  /* headers buffer */
		  if (ilDataBuf[cur].numFrameHeads >= ilDataBuf[cur].frameHeadBufSize) {
			/* reallocate mem - double size */
			u_int16_t * tmp;
			ilDataBuf[cur].frameHeadBufSize *= 2;
			tmp = realloc(frameHeadBuf[cur], ilDataBuf[cur].frameHeadBufSize 
						  * sizeof(u_int16_t));
			if (tmp == NULL) {	 /* clean up and exit */
			  fprintf(stderr, "%s: error: memory allocation failed\n", progName);
			  exit(EXIT_FAILURE);
			}
			frameHeadBuf[cur] = tmp;
		  }
		  (*(frameHeadBuf + cur))[ilDataBuf[cur].numFrameHeads] = auHeader;
		  ilDataBuf[cur].numFrameHeads += 1;
		  ilDataBuf[cur].rtpPktSize += sizeof(u_int16_t);
		}

		/* move along in frame buffer */
		curPos = nextPos;
	  } while (nextPos < frameBufSize);
	  /* frame has been processed */

	  /* if we've fragmented the frame */
	  /* we aren't allowed to put more frames in this RTP pkt
	   * need to write all previous pkts */
	  if (frameBufSize > maxPayloadSize - 2 * sizeof(u_int16_t)) {
		writePkts(cur, start, numBufs, &rtpPktNum,
				  hintBuf, &hintBufSize,
				  frameHeadBuf,  sampleDataBuf,
				  ilDataBuf, qtFile, audioTrack, payloadNumber,
				  hintTrack, frameDuration, dump, &maxBytesPerSec, 
				  frameRate, &framesWritten, trace);
		start =  getNextPos(cur, numBufs);
	  }
	}
	/* end of hint track processing block */
	if (trace) {
	  fprintf(stdout, "\n");
	}

	frameNum++;	/* on to the next audio frame */

	/* go to next pkt buffer */
	ilDataBuf[cur].iscur = FALSE;
	cur = getNextPos(cur, numBufs);
	ilDataBuf[cur].iscur = TRUE;

  } /* end of while (1) */

  /* final pending hints: empty all buffers */
  for (i = 0; i < numBufs; i++) {
	if (ilDataBuf[cur].rtpPktSize > 0) {
	  writePkts(cur, start, numBufs, &rtpPktNum, 
				hintBuf, &hintBufSize,
				frameHeadBuf,  sampleDataBuf, ilDataBuf, 
				qtFile, audioTrack, payloadNumber,
				hintTrack, frameDuration, dump, &maxBytesPerSec, 
				frameRate, &framesWritten, trace);
	  ilDataBuf[cur].rtpPktSize = 0;
	  ilDataBuf[cur].rtpPktDuration = 0;
	} 
	cur = getNextPos(cur, numBufs);
	start = cur;
	ilDataBuf[cur].iscur = TRUE;
  }

  if (trace) {
	fprintf(stdout, "\n");
  	fprintf(stdout, "frames read: %u, frames written: %u\n", 
			framesRead, framesWritten);
  }
  /* record maximum bits per second in Quicktime file */
  quicktime_set_audio_hint_max_rate(qtFile, 1000,
									maxBytesPerSec * 8,
									audioTrack, hintTrack);
  freeBuffers(numBufs, frameHeadBuf, sampleDataBuf, ilDataBuf);
}
