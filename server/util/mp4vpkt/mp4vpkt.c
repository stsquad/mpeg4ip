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

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <quicktime.h>
#include <netinet/in.h>
#include <mpeg4ip.h>


/* hard limits on range of RTP packet sizes */
#define MIN_PKTSIZE	512
#define MAX_PKTSIZE	64*1024

/* MPEG-4 Visual Start Codes */
#define MIN_VID_OBJ_START		0x00
#define MAX_VID_OBJ_START		0x1F
#define MIN_VID_OBJ_LAYER_START	0x20
#define MAX_VID_OBJ_LAYER_START	0x2F
#define VO_SEQUENCE_START		0xB0
#define VO_SEQUENCE_END			0xB1
#define USER_DATA_START			0xB2
#define GROUP_OF_VOP_START		0xB3
#define VO_START				0xB5
#define VOP_START				0xB6

/* globals */
char* progName;

/* forward declarations */
static int loadNextObject(FILE* inFile, u_char* buf, u_int* pBufSize, u_int* pObjCode);
static u_int nextResyncMark(u_char* pBuf, u_int startPos, u_int bufSize);
static bool isConfigObject(u_int objCode);
static u_char getVOPCodingType(u_char* vopBuf, u_int vopSize);
static char* strObjCode(u_int objCode);
static u_int getIodProfileLevel(char* s);
static u_int getVideoProfileLevel(char* s);
static char* binaryToAscii(u_char* pBuf, u_int bufSize);

/*
 * mp4vpkt
 * required arg1 should be the MPEG-4 visual ES bitstream file
 * required arg2 should be the target packetized file (either QT or MP4)
 */ 
int main(int argc, char** argv)
{
	/* variables controlable from command line */
	u_int frameWidth = 320;			/* --width=<uint> */
	u_int frameHeight = 240;		/* --height=<uint> */
	float frameRate = 30.0;			/* --rate=<float> */
	char* profileLevel = "Simple@L3";	/* --profile=<string> */
	u_int maxPayloadSize = 1460;  	/* --payloadsize=<uint> */
	u_int bFrameFrequency = 0;		/* --bfrequency=<uint> */
	bool force = FALSE;				/* --force */
	bool merge = FALSE;				/* --merge */
	bool trace = FALSE;				/* --trace */
	u_int dump = 0;					/* --dump or --dump=<uint>*/
	bool mp4VideoPackets = FALSE;	/* --videopackets */

	/* internal variables */
	char* mp4FileName = NULL;
	char* qtFileName = NULL;
	FILE* mp4File = NULL;
	quicktime_t* qtFile = NULL;
	int videoTrack = 0;
	int hintTrack = 0;
	u_int payloadNumber = 0;
	int timeScale = 90000;
	u_int maxBytesPerSec = 0;
	u_int iodProfileLevel = 0;

	/* begin process command line */
	progName = argv[0];
	while (1) {
		int c = -1;
		int option_index = 0;
		static struct option long_options[] = {
			{ "bfrequency", 1, 0, 'b' },
			{ "dump", 2, 0, 'd' },
			{ "force", 0, 0, 'f' },
			{ "height", 1, 0, 'h' },
			{ "merge", 0, 0, 'm' },
			{ "outformat", 1, 0, 'o' },
			{ "profile", 1, 0, 'p' },
			{ "rate", 1, 0, 'r' },
			{ "payloadsize", 1, 0, 's' },
			{ "trace", 0, 0, 't' },
			{ "videopackets", 0, 0, 'v' },
			{ "width", 1, 0, 'w' },
			{ NULL, 0, 0, 0 }
		};

		c = getopt_long_only(argc, argv, "b:dfh:mo:p:r:s:tvw:",
			long_options, &option_index);

		if (c == -1)
			break;

		/* TBD a help option would be nice */

		switch (c) {
		case 'b': {
			u_int i;
			if (sscanf(optarg, "%u", &i) < 1) {
				fprintf(stderr, 
					"%s: bad bfrequency specified: %s\n",
					 progName, optarg);
			} else {
				bFrameFrequency = i;
			}
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
		case 'h': {
			/* --height <pixels> */
			u_int i;
			if (sscanf(optarg, "%u", &i) < 1) {
				fprintf(stderr, 
					"%s: bad height specified: %s\n",
					 progName, optarg);
			} else {
				/* currently no range checking */
				frameHeight = i;
			}
			break;
		}
		case 'm': {
			merge = TRUE;
			break;
		}
		case 'p': {
			/* -profile=<string> */
			u_int i;
			if (!getVideoProfileLevel(optarg)) {
				fprintf(stderr, 
					"%s: unknown profile specified: %s\n",
					 progName, optarg);
			}
			profileLevel = optarg;
			break;
		}
		case 'r': {
			/* -rate <fps> */
			float f;
			if (sscanf(optarg, "%f", &f) < 1) {
				fprintf(stderr, 
					"%s: bad rate specified: %s\n",
					 progName, optarg);
			} else if (f < 1.0 || f > 30.0) {
				fprintf(stderr,
					"%s: rate must be between 1 and 30\n",
					progName);
			} else {
				frameRate = f;
			}
			break;
		}
		case 's': {
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
		case 't': {
			trace = TRUE;
			break;
		}
		case 'v': {
			mp4VideoPackets = TRUE;
			break;
		}
		case 'w': {
			/* -width <pixels> */
			u_int i;
			if (sscanf(optarg, "%u", &i) < 1) {
				fprintf(stderr, 
					"%s: bad width specified: %s\n",
					 progName, optarg);
			} else {
				/* currently no range checking */
				frameWidth = i;
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
			"usage: %s <video-file> <mp4-file>\n",
			progName);
		exit(1);
	}

	/* point to the specified file names */
	mp4FileName = argv[optind++];
	qtFileName = argv[optind++];

	/* warn about extraneous non-option arguments */
	if (optind < argc) {
		fprintf(stderr, "%s: unknown options specified, ignoring: ", progName);
		while (optind < argc) {
			fprintf(stderr, "%s ", argv[optind++]);
		}
		fprintf(stderr, "\n");
	}

	/* end processing of command line */

	/* open MPEG-4 file for reading */
	mp4File = fopen(mp4FileName, "r");
	if (mp4File == NULL) {
		/* error, file doesn't exist or we can't read it */
		fprintf(stderr,
			"%s: error opening %s: %s\n",
			progName, mp4FileName, strerror(errno));
		exit(2);
	}
	
	/* check if Quicktime file already exists */
	if (access(qtFileName, X_OK) == 0) {
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
		fclose(mp4File);
		exit(4);
	}

	/*
	 * Due to limitations of the quicktime library we using,
	 * we can only handle one video track. So either take an error,
	 * or destroy the existing video track if -force is specified.
	 */
	if (quicktime_video_tracks(qtFile) > 0 && !force) {
		fprintf(stderr, 
			"%s: error in %s: a video track already exists\n",
			progName, qtFileName);
		exit(5);
	}

	/* set movie time scale to match that of video */
	quicktime_set_time_scale(qtFile, timeScale);

	/*
	 * create our new video track, destroying any previous ones
     * would be nice to scan for this information in the VOLs,
     * instead of requiring the correct info on the command line
	 * but that requires fairly deep parsing
	 */
	quicktime_set_video(qtFile, 1, 
		frameWidth, frameHeight, frameRate, timeScale, "mp4v");
	videoTrack = 0;

	/* set profile-level id in initial object descriptor */
	iodProfileLevel = getIodProfileLevel(profileLevel);
	if (iodProfileLevel == 0) {
		iodProfileLevel = 0xFE;
	}
	quicktime_set_iod_video_profile_level(qtFile, iodProfileLevel);

	/* create video hint track */
	hintTrack = quicktime_set_video_hint(qtFile, videoTrack, 
		"MP4V-ES", &payloadNumber, maxPayloadSize); 

	if (trace) {
		fprintf(stdout, "  Sample  ObjName           ObjSize  RTP Pkts (num size)\n");
	}

	{
	u_int sampleNum = 1;
	u_int hintSampleNum = 1;
	u_int rtpPktNum = 1;
	u_int rtpPktSize = 0;
	u_int frameNum = 1;
	u_int bytesThisSec = 0;
	u_int startRtpPktNumThisSec = rtpPktNum;
	u_char hintBuf[64*1024];	/* Should be big enough to handle a hint! */
	u_int hintBufSize = 0;

	/* initialize the first hint sample */
	quicktime_init_hint_sample(hintBuf, &hintBufSize);

	/*
	 * add a sacrifical first packet to the first hint sample
	 * this is sent before the first RTCP SR
	 * which seems to be unfriendly to the client
	 * the UCL RTP library at least seems to be unhappy about it
	 */
	quicktime_add_hint_packet(hintBuf, &hintBufSize, 
		payloadNumber, rtpPktNum);
	quicktime_add_hint_sample_data(hintBuf, &hintBufSize, 1, 0, 0);
	rtpPktNum++;

	/* add a second packet to the first hint */
	quicktime_add_hint_packet(hintBuf, &hintBufSize, 
		payloadNumber, rtpPktNum);

	while (1) {
		static bool starting = TRUE;
		static u_char objBuf[64*1024]; /* 64K is max for a compliant bitstream */
		u_int objBufSize = sizeof(objBuf);
		static u_char configObjBuf[4*1024]; 	
		static u_int configObjBufSize = 0;
		u_int objCode;
		u_char vopCodingType;
		u_int32_t frameRenderingOffset = 0;

		/* get the next MPEG-4 object from the file */
		if (!loadNextObject(mp4File, objBuf, &objBufSize, &objCode)) {
			/* EOF or IO error */
			break;
		}

		/* startup specific code */
		if (starting) {
			if (isConfigObject(objCode)) {
				/* we have a video configuration object */

				/* append it to our running buffer of such objects */
				if (configObjBufSize + objBufSize > sizeof(configObjBuf)) {
					fprintf(stderr,
						"%s: Error, too much decoder config information present\n",
						progName);
					break;
				}
				memcpy(&configObjBuf[configObjBufSize], objBuf, objBufSize);
				configObjBufSize += objBufSize;

				/* TBD if we see VOSH, what checks do we want to perform? */

			} else if (objCode == VOP_START) {
				/* 
				 * we've found the first VOP
				 * so we don't expect any more configuration objects
				 */
				starting = FALSE;

				if (configObjBufSize == 0) {
					/* print warning */
					fprintf(stderr,
						"%s: Warning, no decoder config information present\n",
						progName);
				} else {
					char 	sdpBuf[1024];
					char*	sConfig;

					/* create the appropriate MP4 decoder config */
					quicktime_set_mp4_video_decoder_config(qtFile,
						videoTrack, configObjBuf, configObjBufSize); 

					/* convert all the configuration objects to ASCII form */
					sConfig = binaryToAscii(configObjBuf, configObjBufSize);

					/* create the appropriate SDP attribute */
					if (sConfig) {
						sprintf(sdpBuf,
							"a=fmtp:%u profile-level-id=%u; config=%s\015\012",
							payloadNumber,
							getVideoProfileLevel(profileLevel),
							sConfig); 

						/* add this to the QT file's sdp atom */
						quicktime_add_video_sdp(qtFile, sdpBuf, 
							videoTrack, hintTrack);

						/* cleanup, don't want to leak memory */
						free(sConfig);
					}
				}
			}
		} /* end starting */

		/* if the object is a VOP, aka a frame */ 
		if (objCode == VOP_START) {

			/* get the coding type */
			vopCodingType = getVOPCodingType(objBuf, objBufSize);

			/* 
			 * adjust rendering time offsets when B frames are used
			 */
			if (bFrameFrequency) {
				if (vopCodingType == 'B') {
					frameRenderingOffset = 0;
				} else {
					if (frameNum == 1 && vopCodingType == 'I') {
						/* 
						 * The first I frame presentation time is delayed
						 * by 1 frame duration so it can fill
						 * the hole created by the P frames
						 */
						frameRenderingOffset = 1 * (timeScale / frameRate);
					} else {

						/* 
						 * frame presentation times are early 
						 * by (bFrameFrequency + 1) * frame duration
						 * due to their being pulled forward in the encoded
						 * bitstream, so adjust for that here
						 */
						frameRenderingOffset = 
							(bFrameFrequency + 1) * (timeScale / frameRate);
					}
				}
			} else {
				frameRenderingOffset = 0;
			}
		} else {
			vopCodingType = '\0';
		}

		/*
		 * write the object to the output file,
		 * each is a sample and gets its own chunk 
		 */
		quicktime_write_video_frame(qtFile, &objBuf[0], objBufSize, 
			videoTrack, (vopCodingType == 'I'), 0, frameRenderingOffset);

		/* add this to our count of bytes sent in this 1 second period */
		bytesThisSec += objBufSize;

		/* if the object is a VOP, increment our frame count */
		if (objCode == VOP_START) {
			frameNum++;
		}

		/* trace for debugging */
		if (trace) {
			if (objCode == VOP_START) {
				char temp[64];
				sprintf(temp, "%s %c", strObjCode(objCode), vopCodingType);
				fprintf(stdout, "%8u  %-16s ", 
					sampleNum, temp);
			} else {
				fprintf(stdout, "%8u  %-16s ", 
					sampleNum, strObjCode(objCode));
			}
			fprintf(stdout, "%8u ", objBufSize);
		}		
		
		/* now figure out what to put in the hint track */

		/* if object is a VOP (Video Object Plane), aka a frame */
		if (objCode == VOP_START) {
			u_int curPos = 0;
			u_int nextPos = 2; /* init'ed for the sake of mp4VideoPackets */ 
			u_int length = 0;

			do { /* while there is data to process in the VOP */

				/* determine the next block of video data for the pkt */
				/*
				 * TBD reference codec only generates 
				 * video packets for I frames ???
				 */
				if (mp4VideoPackets && vopCodingType == 'I') { 
					/* using MPEG-4 video packets */

					/* get position of next resync marker */
					nextPos = nextResyncMark(objBuf, nextPos + 2, objBufSize);
					length = nextPos - curPos;

					/* check that object is of reasonable size */
					if (length > maxPayloadSize) {
						fprintf((trace ? stdout : stderr),
							"%s: warning VIDPKT of %u bytes exceeds payloadsize of %u\n",
							progName, length, maxPayloadSize);

						/* TBD option to split videoPackets 
						 * if they're too large? or skip or exit
						 */
					}

					/* check if video packet will exceed current RTP packet */
					if (length > maxPayloadSize - rtpPktSize) {
						/* start a new RTP pkt */
						rtpPktNum++;
						rtpPktSize = 0;

						/* create a new packet in the current sample */
						quicktime_add_hint_packet(hintBuf, &hintBufSize, 
							payloadNumber, rtpPktNum); 
					}

				} else { /* dealing with VOP as a unit */
					length = MIN((objBufSize - curPos), 
						maxPayloadSize - rtpPktSize);
					nextPos = curPos + length; 
				}	

				/* 
				 * Adjust RTP timestamps when B frames are used so that they
				 * correctly reflect the sampling/rendering time of the frame
				 *
				 * TBD do we do this for MP4 files? They should be using the
				 * ctts atom to achieve the same thing. 
				 */
				if (bFrameFrequency) {
					quicktime_set_rtp_hint_timestamp_offset(
						hintBuf, &hintBufSize, frameRenderingOffset);
				}

				/* add dataEntry to current packetEntry */
				quicktime_add_hint_sample_data(hintBuf, &hintBufSize, 
					sampleNum, curPos, length);
				rtpPktSize += length;

				if (trace) {
					fprintf(stdout, " (%u %u)", rtpPktNum, length);
				}

				/* note if this RTP pkt is part of a B-frame */
				if (vopCodingType == 'B') {
					quicktime_set_hint_Bframe(hintBuf);
				}

				/* note is this is the last RTP pkt of the frame */
				if (nextPos >= objBufSize) {
					quicktime_set_hint_Mbit(hintBuf);
				} 

				if (!mp4VideoPackets || vopCodingType != 'I') {
					/* start a new RTP pkt */
					rtpPktNum++;
					rtpPktSize = 0;

					if (nextPos < objBufSize) {
						/* create a new packet in the current sample */
						quicktime_add_hint_packet(hintBuf, &hintBufSize, 
							payloadNumber, rtpPktNum); 
					}
				}

				/* move along in MPEG-4 object */
				curPos = nextPos;

			} while (nextPos < objBufSize);
			/* VOP has been processed, hint sample complete */

			/* write out hint sample */
			quicktime_write_video_hint(qtFile, hintBuf, hintBufSize, 
				videoTrack, hintTrack, 0, (vopCodingType == 'I'));
			hintSampleNum++;

			if (dump > 1) {
				fprintf(stdout, "\nhint sample %u\n", hintSampleNum - 1);
				quicktime_dump_hint_sample(hintBuf);
			}

#ifdef DEBUG
			{
			/* consistency check on hint sample */
			int hintSize = quicktime_get_hint_size(hintBuf);
			if (hintSize != hintBufSize) {
				fprintf(stdout, "BUG: hintSize %u != hintBufSize %u\n",
					hintSize, hintBufSize);
			}
			}
#endif

			/* start a new hint sample */
			quicktime_init_hint_sample(hintBuf, &hintBufSize);
			quicktime_add_hint_packet(hintBuf, &hintBufSize, 
				payloadNumber, rtpPktNum); 

		} else { /* we have a header of some type */

			/* check that object is of reasonable size */
			if (objBufSize > maxPayloadSize) {
				fprintf((trace ? stdout : stderr),
					"%s: warning MPEG-4 object of %u bytes exceeds payloadsize\n",
					progName, objBufSize);
			}

			/* 
			 * if object won't fit in current RTP pkt,
			 * we need to start a new RTP packet 
			 */
			if (rtpPktSize + objBufSize > maxPayloadSize) {

				/* write hint sample */
				quicktime_write_video_hint(qtFile, 
					hintBuf, hintBufSize, videoTrack, hintTrack, 0, FALSE);
				hintSampleNum++;

				if (dump > 1) {
					fprintf(stdout, "\nhint sample %u\n", hintSampleNum - 1);
					quicktime_dump_hint_sample(hintBuf);
				}

				/* start a new hint sample */
				quicktime_init_hint_sample(hintBuf, &hintBufSize);

				/* start a new RTP packet */
				rtpPktNum++;
				rtpPktSize = 0;
				quicktime_add_hint_packet(hintBuf, &hintBufSize, 
					payloadNumber, rtpPktNum);
			}

			/* put the object into current RTP pkt */
			quicktime_add_hint_sample_data(hintBuf, &hintBufSize, 
				sampleNum, 0, objBufSize);
			rtpPktSize += objBufSize;

			if (trace) {
				fprintf(stdout, " (%u %u)", rtpPktNum, objBufSize);
			}
		}

		if (trace) {
			fprintf(stdout, "\n");
		}

		sampleNum++;

		/* if we're at the end of a 1 second period */
		if ((frameNum % ROUND(frameRate)) == 0) {
			/* compute how many RTP pkts were generated for this second */
			u_int pktsThisSec = rtpPktNum - startRtpPktNumThisSec;

			/* record the current maximum bytes in any given 1 second */
			maxBytesPerSec = MAX(maxBytesPerSec,
				(pktsThisSec * RTP_HEADER_STD_SIZE) + bytesThisSec);

			/* reset counters */
			bytesThisSec = 0;
			startRtpPktNumThisSec = rtpPktNum;
		}

	} /* end of while (1) */
	}

	if (trace) {
		fprintf(stdout, "\n");
	}

	/* record maximum bits per second in Quicktime file */
	quicktime_set_video_hint_max_rate(qtFile, 1000, maxBytesPerSec * 8,
		videoTrack, hintTrack);

	/* write out the Quicktime file */
	quicktime_write(qtFile);

	/* dump quicktime atoms if desired */
	if (dump > 0) {
		quicktime_dump(qtFile);
	}

	/* cleanup */
	fclose(mp4File);
	quicktime_destroy(qtFile);
	exit(0);
}

/*
 * Load the next syntatic object from the file
 * into the supplied buffer, which better be large enough!
 *
 * Objects are delineated by 4 byte start codes (0x00 0x00 0x01 0x??)
 * Objects are padded to byte boundaries, and the encoder assures that
 * the start code pattern is never generated for encoded data
 */
static int loadNextObject(FILE* inFile, u_char* pBuf, u_int* pBufSize, u_int* pObjCode)
{
	static u_int state = 0;
	static u_int nextObjCode = 0;

	*pBufSize = 0;

	/* initial state, never read from file before */
	if (state == 0) {
		/* read 4 bytes */
		if (fread(pBuf, 1, 4, inFile) != 4) {
			/* IO error, or not enough bytes */
			return 0;
		}
		*pBufSize = 4;

		/* check for a valid start code */
		if (pBuf[0] != 0 || pBuf[1] != 0 || pBuf[2] != 1) {
			return 0;
		}
		/* set current object start code */
		(*pObjCode) = (u_int)(pBuf[3]);

		/* move to next state */
		state = 1;

	} else if (state == 5) {
		/* EOF was reached on last call */
		return 0;

	} else {
		/*
		 * We have next object code from prevous call 
		 * insert start code into buffer
		 */
		(*pObjCode) = nextObjCode;
		pBuf[(*pBufSize)++] = 0;
		pBuf[(*pBufSize)++] = 0;
		pBuf[(*pBufSize)++] = 1;
		pBuf[(*pBufSize)++] = (u_char)(*pObjCode);
	}

	/* read bytes, execute state machine */
	while (1) {
		/* read a byte */
		u_char b;

		if (fread(&b, 1, 1, inFile) == 0) {
			/* handle EOF */
			if (feof(inFile)) {
				switch (state) {
				case 3:
					pBuf[(*pBufSize)++] = 0;
					/* fall thru */
				case 2:
					pBuf[(*pBufSize)++] = 0;
					break;
				}
				state = 5;
				return 1;
			}
			/* else error */
			*pBufSize = 0;
			return 0;
		}

		switch (state) {
		case 1:
			if (b == 0) {
				state = 2;
			} else {
				pBuf[(*pBufSize)++] = b;
			}
			break;
		case 2:
			if (b == 0) {
				state = 3;
			} else {
				pBuf[(*pBufSize)++] = 0;
				pBuf[(*pBufSize)++] = b;
				state = 1;
			}
			break;
		case 3:
			if (b == 1) {
				state = 4;
			} else {
				pBuf[(*pBufSize)++] = 0;
				pBuf[(*pBufSize)++] = 0;
				pBuf[(*pBufSize)++] = b;
				state = 1;
			}
			break;
		case 4:
			nextObjCode = b;
			state = 1;
			return 1;
		default:
			fprintf(stderr, "BAD PROGRAMMER\n");
		}
	}
}

/*
 * Find next resync marker or end of buffer 
 */
static u_int nextResyncMark(u_char* pBuf, u_int startPos, u_int bufSize)
{
	u_int nextPos = startPos;
	u_int state = 0;

	while (nextPos < bufSize) {
		switch (state) {
		case 0:
			if (pBuf[nextPos] == 0) {
				state = 1;
			}
			break;
		case 1:
			if (pBuf[nextPos] == 0) {
				state = 2;
			} else {
				state = 0;
			}
			break;
		case 2:
			if ((pBuf[nextPos] & 0x80)) {
				state = 3;
			} else if (pBuf[nextPos] != 0) {
				state = 0;
			}
			break;
		}
		if (state == 3) {
			/* 
			 * found the resync marker
			 * point to start of it
			 */
			nextPos -= 2;
			break;
		}
		nextPos++;
	}

	return nextPos;
}

static bool isConfigObject(u_int objCode)
{
	if (objCode == VO_SEQUENCE_START) {
		return TRUE;
	}
	if (objCode == VO_START) {
		return TRUE;
	}
	if (MIN_VID_OBJ_START <= objCode && objCode <= MAX_VID_OBJ_START) {
		return TRUE;
	}
	if (MIN_VID_OBJ_LAYER_START <= objCode && objCode <= MAX_VID_OBJ_LAYER_START) {
		return TRUE;
	}
	return FALSE;
}

static u_char getVOPCodingType(u_char* vopBuf, u_int vopSize)
{
	if (vopSize < 5) {
		return 0;
	}
	switch (vopBuf[4] >> 6) {
	case 0:
		/* Intra */
		return 'I';
	case 1:
		/* Predictive */
		return 'P';
	case 2:
		/* Bidirectional Predictive */
		return 'B';
	case 3:
		/* Sprite */
		return 'S';
	}
	return 0;
}
		
static char* strObjCode(u_int objCode)
{
	if (MIN_VID_OBJ_START <= objCode && objCode <= MAX_VID_OBJ_START) {
		return "VO_START";
	}
	if (MIN_VID_OBJ_LAYER_START <= objCode && objCode <= MAX_VID_OBJ_LAYER_START) {
		return "VO_LAYER_START";
	}
	switch (objCode) {
	case VO_SEQUENCE_START:
		return "VO_SEQ_START";
	case VO_SEQUENCE_END:
		return "VO_SEQ_END";
	case USER_DATA_START:
		return "USER DATA";	
	case GROUP_OF_VOP_START:
		return "GOV";
	case VO_START:
		return "VO";
	case VOP_START:
		return "VOP";
	default:
		return "???";
	}
}

static u_int getLevel(char *s)
{
	u_int level = 1; /* default is level 1 */
	char* pos;

	/* look for @ as separator */
	pos = strstr(s, "@");
	if (!pos) {
		/* nope, try / as separator instead */
		pos = strstr(s, "/");
	}
	if (pos) {
		while (*pos) {
			if (isdigit(*pos)) {
				/* levels are currently always 1 digit */
				level = *pos - '0';
				break;
			}
			pos++;
		}
	}

	return level;
}

static u_int getIodProfileLevel(char* s)
{
	/* values from ISO/IEC 14496-1 section 8.6.3.2 */
	static struct {
		char* name;		/* profile name */
		u_int baseId;	/* value for level 1 */
	} map[] = {
		{ "basic animated texture", 0x0F },
		{ "scalable texture", 0x12 },
		{ "simple face animation", 0x14 },
		{ "simple scalable", 0x05 },
		{ "core", 0x07 },
		{ "hybrid", 0x0D },
		{ "main", 0x0B },
		{ "n-bit", 0x0C },
		{ "simple", 0x03 },
		{ NULL, 0 }
	};
	u_int level = getLevel(s);
	int i;

	for (i = 0; map[i].name != NULL; i++) {
		if (!strncasecmp(s, map[i].name, strlen(map[i].name))) {
			return map[i].baseId - (level - 1);
		}
	}
	return 0;
}

static u_int getVideoProfileLevel(char* s)
{
	/* values from ISO/IEC 14496-2 Annex G */
	static struct {
		char* name;		/* profile name */
		u_int baseId;	/* value for level 1 */
	} map[] = {
		/* be careful of order */
		{ "advanced coding efficiency", 0xB1 },
		{ "advanced core", 0xC1 },
		{ "advanced real time simple", 0x91 },
		{ "advanced scalable texture", 0xD1 },
		{ "advanced simple", 0xF2 },
		{ "basic animated texture", 0x71 },
		{ "core scalable", 0xA1 },
		{ "core studio", 0xE5 },
		{ "fine granularity scalable", 0xF6 },
		{ "scalable texture", 0x51 },
		{ "simple face animation", 0x61 },
		{ "simple fba", 0x63 },
		{ "simple scalable", 0x11 },
		{ "simple studio", 0xE1 },
		{ "arts", 0x91 },
		{ "ace", 0xB1 },
		{ "core", 0x21 },
		{ "fgs", 0xF6 },
		{ "hybrid", 0x81 },
		{ "main", 0x31 },
		{ "n-bit", 0x41 },
		{ "simple", 0x01 },
		{ NULL, 0 }
	};
	u_int level = getLevel(s);
	int i;

	for (i = 0; map[i].name != NULL; i++) {
		if (!strncasecmp(s, map[i].name, strlen(map[i].name))) {
			return map[i].baseId + level - 1;
		}
	}
	return 0;
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

