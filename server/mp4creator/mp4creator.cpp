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
 * Copyright (C) Cisco Systems Inc. 2001-2004.  All Rights Reserved.
 * 
 * Portions created by Ximpo Group Ltd. are
 * Copyright (C) Ximpo Group Ltd. 2003, 2004.  All Rights Reserved.
 *
 * Contributor(s): 
 *		Dave Mackie			dmackie@cisco.com
 *		Alix Marchandise-Franquet	alix@cisco.com
 *		Ximpo Group Ltd.		mp4v2@ximpo.com
 */

#define MP4CREATOR_GLOBALS
#include "mp4creator.h"
#include "mpeg4ip_getopt.h"
#include "mpeg.h"

// forward declarations
// AMR defines
#define AMR_TYPE_NONE 0
#define AMR_TYPE_AMR 1
#define AMR_TYPE_AMRWB 2

#define AMR_MAGIC_LEN_AMR 6
#define AMR_MAGIC_AMR "#!AMR\n"

#define AMR_MAGIC_LEN_AMRWB 9
#define AMR_MAGIC_AMRWB "#!AMR-WB\n"

MP4TrackId* CreateMediaTracks(
			      MP4FileHandle mp4File, 
			      const char* inputFileName,
			      bool doEncrypt);

void CreateHintTrack(
		     MP4FileHandle mp4File, 
		     MP4TrackId mediaTrackId,
		     const char* payloadName, 
		     bool interleave, 
		     u_int16_t maxPayloadSize,
		     bool doEncrypt);

void ExtractTrack(
		  MP4FileHandle mp4File, 
		  MP4TrackId trackId, 
		  const char* outputFileName);

static bool Is3GPP(MP4FileHandle mp4File);

// external declarations

// track creators
MP4TrackId* AviCreator(MP4FileHandle mp4File, const char* aviFileName, 
		       bool doEncrypt);

MP4TrackId AacCreator(MP4FileHandle mp4File, FILE* inFile, bool doEncrypt);

MP4TrackId Mp3Creator(MP4FileHandle mp4File, FILE* inFile, bool doEncrypt);

MP4TrackId Mp4vCreator(MP4FileHandle mp4File, FILE* inFile, bool doEncrypt,
		       bool doVariableRate);

MP4TrackId AmrCreator(MP4FileHandle mp4File, FILE* inFile, bool doEncrypt);

MP4TrackId H263Creator(MP4FileHandle mp4File, FILE* inFile,
                       u_int8_t h263Profile, u_int8_t h263Level,
                       bool setBitrates, u_int8_t cbrTolerance);

MP4TrackId H264Creator(MP4FileHandle mp4File, FILE *inFile);
u_int8_t h263Profile = 0;
u_int8_t h263Level = 10;
u_int8_t H263CbrTolerance = 0;
bool setBitrates = false;
static bool allowVariableFrameRate = false;
static bool allowAvi = false;

// main routine
int main(int argc, char** argv)
{
  char* usageString = 
    " <options> <mp4-file>\n"
    "  Options:\n"
    "  -aac-old-file-format    Use old file format with 58 bit adts headers\n"
    "  -aac-profile=[2|4]      Force AAC to mpeg2 or mpeg4 profile\n"
    "  -allow-avi-files        Allow avi files\n"
    "  -calcH263Bitrates       Calculate and add bitrate information\n"
    "  -create=<input-file>    Create track from <input-file>\n"
    "    input files can be of type: .263 .aac .amr .mp3 .divx .mp4v .m4v .cmp .xvid\n"
    "  -encrypt[=<track-id>]   Encrypt a track, also -E\n"
    "  -extract=<track-id>     Extract a track\n"
    "  -delete=<track-id>      Delete a track\n"
    "  -force3GPCompliance     Force making the file 3GP compliant. This disables ISMA compliance.\n"
    "  -forceH263Profile=<profile> Force using H.263 Profile <profile> (default is 0)\n"
    "  -forceH263Level=<level>     Force using H.263 level <level> (default is 10)\n"
    "  -H263CbrTolerance=<value>   Define H.263 CBR tolerance of [value] (default: 10%)\n"
    "  -hint[=<track-id>]      Create hint track, also -H\n"
    "  -interleave             Use interleaved audio payload format, also -I\n"
    "  -list                   List tracks in mp4 file\n"
    "  -make-isma-10-compliant Insert bifs and od tracks required for some ISMA players (also -i)\n"
    "  -mpeg4-video-profile=<level> Mpeg4 video profile override\n"
    "  -mtu=<size>             Maximum Payload size for RTP packets in hint track\n"
    "  -optimize               Optimize mp4 file layout\n"
    "  -payload=<payload>      Rtp payload type \n"
    "                          (use 3119 or mpa-robust for mp3 rfc 3119 support)\n"
    "  -rate=<fps>             Video frame rate, e.g. 30 or 29.97\n"
    "  -timescale=<ticks>      Time scale (ticks per second)\n"
    "  -use64bits              Use for large files\n"
    "  -use64bitstime          Use for 64 Bit times (not QT player compatible)\n"
    "  -variable-frame-rate    Enable variable frame rate for mpeg4 video\n"
    "  -verbose[=[1-5]]        Enable debug messages\n"
    "  -version                Display version information\n"
    ;

  bool doCreate = false;
  bool doEncrypt = false;
  bool doExtract = false;
  bool doDelete = false;
  bool doHint = false;
  bool doList = false;
  bool doOptimize = false;
  bool doInterleave = false;
  bool doIsma = false;
  uint64_t createFlags = 0;
  char* mp4FileName = NULL;
  char* inputFileName = NULL;
  char* outputFileName = NULL;
  char* payloadName = NULL;
  MP4TrackId hintTrackId = MP4_INVALID_TRACK_ID;
  MP4TrackId encryptTrackId = MP4_INVALID_TRACK_ID;
  MP4TrackId extractTrackId = MP4_INVALID_TRACK_ID;
  MP4TrackId deleteTrackId = MP4_INVALID_TRACK_ID;
  u_int16_t maxPayloadSize = 1460;
  bool force3GPCompliance = false;
  char* p3gppSupportedBrands[2] = {"3gp5", "3gp4"};
  uint32_t newverbosity;

  Verbosity = MP4_DETAILS_ERROR;
  VideoFrameRate = 0;		// determine from input file
  TimeScaleSpecified = false;
  Mp4TimeScale = 90000;
  VideoProfileLevelSpecified = false;
  aacUseOldFile = false;
  aacProfileLevel = 0;

  // begin processing command line
  ProgName = argv[0];

  while (true) {
    int c = -1;
    int option_index = 0;
    static struct option long_options[] = {
      { "aac-old-file-format", 0, 0, 'a' },
      { "aac-profile", 1, 0, 'A'},
      { "allow-avi-files", 0, 0, 'B'},
      { "calcH263Bitrates", 0, 0, 'C'},
      { "create", 1, 0, 'c' },
      { "delete", 1, 0, 'd' },
      { "extract", 2, 0, 'e' },
      { "encrypt", 2, 0, 'E' },
      { "force3GPCompliance", 0, 0, 'G'},
      { "forceH263Profile", 1, 0, 'P'},
      { "forceH263Level", 1, 0, 'L'},
      { "H263CbrTolerance", 1, 0, 'T' },
      { "help", 0, 0, '?' },
      { "hint", 2, 0, 'H' },
      { "interleave", 0, 0, 'I' },
      { "list", 0, 0, 'l' },
      { "make-isma-10-compliant", 0, 0, 'i' },
      { "mpeg4-video-profile", 1, 0, 'M' },
      { "mtu", 1, 0, 'm' },
      { "optimize", 0, 0, 'O' },
      { "payload", 1, 0, 'p' },
      { "rate", 1, 0, 'r' },
      { "timescale", 1, 0, 't' },
      { "use64bits", 0, 0, 'u' },
      { "use64bitstime", 0, 0, 'U' },
      { "variable-frame-rate", 0, 0, 'Z'},
      { "verbose", 2, 0, 'v' },
      { "version", 0, 0, 'V' },
      { NULL, 0, 0, 0 }
    };

    c = getopt_long_only(argc, argv, "aBc:Cd:e:E::GH::iIlL:m:Op:P:r:t:T:uUv::VZ",
			 long_options, &option_index);

    if (c == -1)
      break;
    
    switch (c) {
    case 'a':
      aacUseOldFile = true;
      break;
    case 'A':
      if (sscanf(optarg, "%u", &aacProfileLevel) != 1 ||
	  (aacProfileLevel != 2 && aacProfileLevel != 4)) {
	fprintf(stderr,
		"%s: bad mpeg version specified %d\n",
		ProgName, aacProfileLevel);
	exit(EXIT_COMMAND_LINE);
      }
      fprintf(stderr, "Warning - you have changed the AAC profile level.  This is not recommended\nIf you have problems with the resultant file, it is your own fault\nDo not contact project creators\n");
      break;
    case 'B':
      allowAvi = true;
      break;
    case 'c':
      doCreate = true;
      inputFileName = optarg;
      break;
    case 'C':
      setBitrates = true;
      break;
    case 'd':
      if (optarg == NULL) {
	fprintf(stderr, "%s:no track-id specified for delete\n", ProgName);
	exit(EXIT_COMMAND_LINE);
      }
      if (sscanf(optarg, "%u", &deleteTrackId) != 1) {
	fprintf(stderr, 
		"%s: bad track-id specified: %s\n",
		ProgName, optarg);
	exit(EXIT_COMMAND_LINE);
      }
      doDelete = true;
      break;
    case 'e':
      if (optarg == NULL) {
	fprintf(stderr, "%s:no track-id specified for extract\n", ProgName);
	exit(EXIT_COMMAND_LINE);
      }

      if (sscanf(optarg, "%u", &extractTrackId) != 1) {
	fprintf(stderr, 
		"%s: bad track-id specified: %s\n",
		ProgName, optarg);
	exit(EXIT_COMMAND_LINE);
      }
      doExtract = true;
      break;
    case 'E':
      doEncrypt = true;
      if (optarg) {
        // if the short version of option is given, optarg has 
        // an = at the beginning. this causes sscanf to fail. 
        // if the long version of the option is given, there 
        // is no =
        if ( optarg[0] == '=' ) optarg[0] = ' ';
	if (sscanf(optarg, "%d", &encryptTrackId) != 1) {
	  fprintf(stderr, 
		  "%s: bad track-id specified: %s\n",
		  ProgName, optarg);
	  exit(EXIT_COMMAND_LINE);
	}
      }	
      break;
    case 'G':
      force3GPCompliance = true;
      break;
    case 'H':
      doHint = true;
      if (optarg) {
        // if the short version of option is given, optarg has 
        // an = at the beginning. this causes sscanf to fail. 
        // if the long version of the option is given, there 
        // is no =
        if ( optarg[0] == '=' ) optarg[0] = ' ';
	if (sscanf(optarg, "%u", &hintTrackId) != 1) {
	  fprintf(stderr, 
		  "%s: bad track-id specified: %s\n",
		  ProgName, optarg);
	  exit(EXIT_COMMAND_LINE);
	}
      }
      break;
    case 'i':
      doIsma = true;
      break;
    case 'I':
      doInterleave = true;
      break;
    case 'l':
      doList = true;
      break;
    case 'L':
      if (!optarg || (sscanf(optarg, "%hhu", &h263Level) != 1)) {
        fprintf(stderr,
		"%s: bad h263Level specified: %s\n",
		ProgName, optarg ? optarg : "<none>");
	exit(EXIT_COMMAND_LINE);
      }
      break;
    case 'm':
      u_int32_t mtu;
      if (optarg == NULL) {
	fprintf(stderr, "%s:no mtu specified\n", ProgName);
	exit(EXIT_COMMAND_LINE);
      }
      if (sscanf(optarg, "%u", &mtu) != 1 || mtu < 64) {
	fprintf(stderr, 
		"%s: bad mtu specified: %s\n",
		ProgName, optarg);
	exit(EXIT_COMMAND_LINE);
      }
      maxPayloadSize = mtu - 40;	// subtract IP, UDP, and RTP hdrs
      break;
    case 'M':
      if (optarg == NULL) {
	fprintf(stderr, "%s:no video profile specifed\n", ProgName);
	exit(EXIT_COMMAND_LINE);
      }
      if (sscanf(optarg, "%i", &VideoProfileLevel) != 1 ||
	  (VideoProfileLevel < 0 || VideoProfileLevel > MPEG4_FGSP_L5)) {
	fprintf(stderr,
		"%s: bad mpeg4 video profile level specified %d\n",
		ProgName, VideoProfileLevel);
	exit(EXIT_COMMAND_LINE);
      }
      VideoProfileLevelSpecified = TRUE;
      break;
    case 'O':
      doOptimize = true;
      break;
    case 'p':
      payloadName = optarg;
      break;
    case 'P':
      if (!optarg || (sscanf(optarg, "%hhu", &h263Profile) != 1)) {
        fprintf(stderr,
		"%s: bad h263Profile specifed: %s\n",
		ProgName, optarg ? optarg : "<none>");
	exit(EXIT_COMMAND_LINE);
      }
      break;
    case 'r':
      if (optarg == NULL) {
	fprintf(stderr, "%s:no rate specifed\n", ProgName);
	exit(EXIT_COMMAND_LINE);
      }
      if (sscanf(optarg, "%lf", &VideoFrameRate) != 1) {
	fprintf(stderr, 
		"%s: bad rate specified: %s\n",
		ProgName, optarg);
	exit(EXIT_COMMAND_LINE);
      }
      break;
    case 't':
      if (optarg == NULL) {
	fprintf(stderr, "%s:no time scale specifed\n", ProgName);
	exit(EXIT_COMMAND_LINE);
      }
      if (sscanf(optarg, "%u", &Mp4TimeScale) != 1) {
	fprintf(stderr, 
		"%s: bad timescale specified: %s\n",
		ProgName, optarg);
	exit(EXIT_COMMAND_LINE);
      }
      TimeScaleSpecified = true;
      break;
    case 'T':
      if (!optarg || (sscanf(optarg, "%hhu", &H263CbrTolerance) != 1)) {
        fprintf(stderr,
		"%s: bad H263CbrTolerance specified: %s\n",
		ProgName, optarg ? optarg : "<none>");
	exit(EXIT_COMMAND_LINE);
      }
      break;
    case 'u':
      createFlags |= MP4_CREATE_64BIT_DATA;
      break;
    case 'U':
      createFlags |= MP4_CREATE_64BIT_TIME;
      break;  
    case 'v':
      Verbosity |= (MP4_DETAILS_READ | MP4_DETAILS_WRITE);
      if (optarg) {
	u_int32_t level;
	if (sscanf(optarg, "%u", &level) == 1) {
	  if (level >= 2) {
	    Verbosity |= MP4_DETAILS_TABLE;
	  } 
	  if (level >= 3) {
	    Verbosity |= MP4_DETAILS_SAMPLE;
	  } 
	  if (level >= 4) {
	    Verbosity |= MP4_DETAILS_HINT;
	  } 
	  if (level >= 5) {
	    Verbosity = MP4_DETAILS_ALL;
	  } 
	}
      }
      break;
    case '?':
      fprintf(stderr, "usage: %s %s", ProgName, usageString);
      exit(EXIT_SUCCESS);
    case 'V':
      fprintf(stderr, "%s - %s version %s\n", 
	      ProgName, MPEG4IP_PACKAGE, MPEG4IP_VERSION);
      exit(EXIT_SUCCESS);
    case 'Z':
      allowVariableFrameRate = true;
      break;
    default:
      fprintf(stderr, "%s: unknown option specified, ignoring: %c\n", 
	      ProgName, c);
    }
  }

  // check that we have at least one non-option argument
  if ((argc - optind) < 1) {
    fprintf(stderr, "usage: %s %s", ProgName, usageString);
    exit(EXIT_COMMAND_LINE);
  }
  
  if ((argc - optind) == 1) {
    mp4FileName = argv[optind++];
  } else {
    // it appears we have two file names
    if (doExtract) {
      mp4FileName = argv[optind++];
      outputFileName = argv[optind++];
    } else {
      if (inputFileName == NULL) {
	// then assume -c for the first file name
	doCreate = true;
	inputFileName = argv[optind++];
      }
      mp4FileName = argv[optind++];
    }
  }

  // warn about extraneous non-option arguments
  if (optind < argc) {
    fprintf(stderr, "%s: unknown options specified, ignoring: ", ProgName);
    while (optind < argc) {
      fprintf(stderr, "%s ", argv[optind++]);
    }
    fprintf(stderr, "\n");
  }

  // operations consistency checks
  
  if (!doList && !doCreate && !doHint && !doEncrypt  
      && !doOptimize && !doExtract && !doDelete && !doIsma) {
    fprintf(stderr, 
	    "%s: no operation specified\n",
	    ProgName);
    exit(EXIT_COMMAND_LINE);
  }
  if ((doCreate || doHint || doEncrypt || doIsma) && doExtract) {
    fprintf(stderr, 
	    "%s: extract operation must be done separately\n",
	    ProgName);
    exit(EXIT_COMMAND_LINE);
  }
  if ((doCreate || doHint || doEncrypt || doIsma) && doDelete) {
    fprintf(stderr, 
	    "%s: delete operation must be done separately\n",
	    ProgName);
    exit(EXIT_COMMAND_LINE);
  }
  if (doExtract && doDelete) {
    fprintf(stderr, 
	    "%s: extract and delete operations must be done separately\n",
	    ProgName);
    exit(EXIT_COMMAND_LINE);
  }

  // end processing of command line

  if (doList) {
    // just want the track listing
    char* info = MP4FileInfo(mp4FileName);

    if (!info) {
      fprintf(stderr, 
	      "%s: can't open %s\n", 
	      ProgName, mp4FileName);
      exit(EXIT_INFO);
    }

    fputs(info, stdout);
    free(info);
    exit(EXIT_SUCCESS);
  }

  // test if mp4 file exists
  bool mp4FileExists = (access(mp4FileName, F_OK) == 0);

  MP4FileHandle mp4File;

  if (doCreate || doHint) {
    if (!mp4FileExists) {
      if (doCreate) {
	const char* extension = strrchr(inputFileName, '.');
        if (extension == NULL) {
          fprintf(stderr,
                  "%s: unknown file type: %s: file has no extension\n", ProgName, inputFileName);
          exit(EXIT_COMMAND_LINE);
        }

	if ((!strcmp(extension, ".amr")) || (!strcmp(extension, ".263"))) {
		mp4File = MP4CreateEx(mp4FileName,
				      Verbosity,
				      createFlags,
				      1,  // add ftyp atom
				      0,  // don't add iods
				      p3gppSupportedBrands[0],
				      0x0001,
				      p3gppSupportedBrands,
				      sizeof(p3gppSupportedBrands) / sizeof(p3gppSupportedBrands[0]));
	} else {
	  mp4File = MP4Create(mp4FileName, Verbosity,
			      createFlags);
	}
	if (mp4File) {
	  MP4SetTimeScale(mp4File, Mp4TimeScale);
	}
      } else if (doEncrypt) {
	fprintf(stderr,
		"%s: can't encrypt track in file that doesn't exist\n", 
		ProgName);
	exit(EXIT_CREATE_FILE);
      } else {
	fprintf(stderr,
		"%s: can't hint track in file that doesn't exist\n", 
		ProgName);
	exit(EXIT_CREATE_FILE);
      }
    } else {
      if (createFlags != 0) {
	fprintf(stderr, "Must specify 64 bits on new file only");
	exit(EXIT_CREATE_FILE);
      }
      mp4File = MP4Modify(mp4FileName, Verbosity);
    }

    if (!mp4File) {
      // mp4 library should have printed a message
      exit(EXIT_CREATE_FILE);
    }

    bool allMpeg4Streams = true;

    if (doCreate) {
      MP4TrackId* pCreatedTrackIds = 
	CreateMediaTracks(mp4File, inputFileName, 
			  doEncrypt);
      if (pCreatedTrackIds == NULL) {
	MP4Close(mp4File);
	exit(EXIT_CREATE_MEDIA);
      }

      // decide if we can raise the ISMA compliance tag in SDP
      // we do this if audio and/or video are MPEG-4
      MP4TrackId* pTrackId = pCreatedTrackIds;

      while (*pTrackId != MP4_INVALID_TRACK_ID) {			       
	const char *type =
	  MP4GetTrackType(mp4File, *pTrackId);
	// look for objectTypeId (GetTrackEsdsObjectTypeId)
	uint64_t temp;
	newverbosity = Verbosity & ~(MP4_DETAILS_ERROR);
	MP4SetVerbosity(mp4File, newverbosity);
	bool ret = 
	  MP4GetTrackIntegerProperty(mp4File, *pTrackId,
				     "mdia.minf.stbl.stsd.*.esds.decConfigDescr.objectTypeId",
				     &temp);
	MP4SetVerbosity(mp4File, Verbosity);
	if (ret) {
	  if (!strcmp(type, MP4_AUDIO_TRACK_TYPE)) { 
	    allMpeg4Streams &=
	      (MP4GetTrackEsdsObjectTypeId(mp4File, *pTrackId) 
	       == MP4_MPEG4_AUDIO_TYPE);
	    
	  } else if (!strcmp(type, MP4_VIDEO_TRACK_TYPE)) { 
	    allMpeg4Streams &=
	      (MP4GetTrackEsdsObjectTypeId(mp4File, *pTrackId)
	       == MP4_MPEG4_VIDEO_TYPE);
	  }
	}
	pTrackId++;
      }

      if (doHint) {
		  pTrackId = pCreatedTrackIds;

	while (*pTrackId != MP4_INVALID_TRACK_ID) {
	  CreateHintTrack(mp4File, *pTrackId, payloadName, 
			  doInterleave, maxPayloadSize, doEncrypt);
	  pTrackId++;
	}
      }
    } else if (doHint) {
      // in this case, we only hint the track specified in the command line

      CreateHintTrack(mp4File, hintTrackId, payloadName, 
		      doInterleave, maxPayloadSize, doEncrypt);

      uint32_t trackNum;

      trackNum = MP4GetNumberOfTracks(mp4File);
      for (uint32_t ix = 0; ix < trackNum; ix++) {
	MP4TrackId trackId = MP4FindTrackId(mp4File, ix);
	
	const char *type =
	  MP4GetTrackType(mp4File, trackId);
	uint64_t temp;
	if (MP4GetTrackIntegerProperty(mp4File, trackId,
					"mdia.minf.stbl.stsd.*.esds.decConfigDescr.objectTypeId", 
				       &temp)) {
	  if (!strcmp(type, MP4_AUDIO_TRACK_TYPE)) { 
	    newverbosity = Verbosity & ~(MP4_DETAILS_ERROR);
	    MP4SetVerbosity(mp4File, newverbosity);
	    allMpeg4Streams &=
	      (MP4GetTrackEsdsObjectTypeId(mp4File, trackId) 
	       == MP4_MPEG4_AUDIO_TYPE);
	    MP4SetVerbosity(mp4File, Verbosity);
			    
	  } else if (!strcmp(type, MP4_VIDEO_TRACK_TYPE)) { 
	    newverbosity = Verbosity & ~(MP4_DETAILS_ERROR);
	    MP4SetVerbosity(mp4File, newverbosity);
	    allMpeg4Streams &=
	      (MP4GetTrackEsdsObjectTypeId(mp4File, trackId) 
	       == MP4_MPEG4_VIDEO_TYPE);
	    MP4SetVerbosity(mp4File, Verbosity);
	  }
	}
      }
    }

    char *buffer;
    char *value;
    newverbosity = Verbosity & ~(MP4_DETAILS_ERROR);
    MP4SetVerbosity(mp4File, newverbosity);
    bool retval = MP4GetMetadataTool(mp4File, &value);
    MP4SetVerbosity(mp4File, Verbosity);
    if (retval && value != NULL) {
      if (strncasecmp("mp4creator", value, strlen("mp4creator")) != 0) {
	buffer = (char *)malloc(strlen(value) + 80);
	sprintf(buffer, "%s mp4creator %s", value, MPEG4IP_VERSION);
	MP4SetMetadataTool(mp4File, buffer);
	free(buffer);
      }
    } else {
      buffer = (char *)malloc(80);
      sprintf(buffer, "mp4creator %s", MPEG4IP_VERSION);
      MP4SetMetadataTool(mp4File, buffer);
    }
    bool is3GPP = Is3GPP(mp4File);
    
    MP4Close(mp4File);
    if (is3GPP || (force3GPCompliance)) {
      // If we created the file, CreateEX already takes care of this...
      MP4Make3GPCompliant(mp4FileName,
                          Verbosity,
                          p3gppSupportedBrands[0],
                          0x0001,
                          p3gppSupportedBrands,
                          sizeof(p3gppSupportedBrands) / sizeof(p3gppSupportedBrands[0]));
    } 
  } else if (doEncrypt) { 
    // just encrypting, not creating nor hinting, but may already be hinted
    if (!mp4FileExists) {
      fprintf(stderr,
	      "%s: can't encrypt track in file that doesn't exist\n", 
	      ProgName);
      exit(EXIT_CREATE_FILE);
    }
    
    mp4File = MP4Read(mp4FileName, Verbosity);
    if (!mp4File) {
      // mp4 library should have printed a message
      exit(EXIT_CREATE_FILE);
    }

    char tempName[PATH_MAX];
    if (outputFileName == NULL) {
      snprintf(tempName, sizeof(tempName), 
	       "enc-%s", mp4FileName);
      outputFileName = tempName;
    }

    MP4FileHandle outputFile = MP4CreateEx(outputFileName, Verbosity,
					   createFlags, true, true);
    MP4SetTimeScale(outputFile, Mp4TimeScale);
    u_int32_t numTracks = MP4GetNumberOfTracks(mp4File);
    for (u_int32_t i = 0; i < numTracks; i++) {
      MP4TrackId curTrackId = MP4FindTrackId(mp4File, i);
      const char *type = MP4GetTrackType(mp4File, curTrackId);
      // encrypt only the specified track
      // if no track was specified in the command-line, encrypt all
      // audio and video tracks 
      // hint tracks are removed, so need to rehint afterwards
      // if the track is already encrypted, just copy it
      if (((curTrackId == encryptTrackId) 
	  || ((encryptTrackId == MP4_INVALID_TRACK_ID) 
	      && !strcmp(type, MP4_AUDIO_TRACK_TYPE))
	  || ((encryptTrackId == MP4_INVALID_TRACK_ID) 
	      && !strcmp(type, MP4_VIDEO_TRACK_TYPE)))
	  && !(MP4IsIsmaCrypMediaTrack(mp4File,curTrackId))){

	ismacryp_session_id_t ismaCrypSId;
        ismacryp_keytype_t key_type;

        if ( !strcmp(type, MP4_VIDEO_TRACK_TYPE) ) { 
           key_type = KeyTypeVideo;
        }
        else {
           if ( !strcmp(type, MP4_AUDIO_TRACK_TYPE) ) 
              key_type = KeyTypeAudio;
           else
              key_type = KeyTypeOther;
        }

	if (ismacrypInitSession(&ismaCrypSId, key_type) != ismacryp_rc_ok) {
	  fprintf(stderr, 
		  "%s: could not initialize the ISMAcryp session\n", ProgName);
	  exit(EXIT_ISMACRYP_INIT);
	}
        mp4v2_ismacrypParams *icPp =  (mp4v2_ismacrypParams *) malloc(sizeof(mp4v2_ismacrypParams));
        memset(icPp, 0, sizeof(mp4v2_ismacrypParams));

        if (ismacrypGetScheme(ismaCrypSId, &(icPp->scheme_type)) != ismacryp_rc_ok) {
	  fprintf(stderr, 
             "%s: could not get ismacryp scheme type. sid %d\n", ProgName, ismaCrypSId);
	  ismacrypEndSession(ismaCrypSId);
	  exit(EXIT_ISMACRYP_END);
        }
        if (ismacrypGetSchemeVersion(ismaCrypSId, &(icPp->scheme_version)) != ismacryp_rc_ok) {
	  fprintf(stderr, 
            "%s: could not get ismacryp scheme ver. sid %d\n", ProgName, ismaCrypSId);
	  ismacrypEndSession(ismaCrypSId);
	  exit(EXIT_ISMACRYP_END);
        }
        if (ismacrypGetKMSUri(ismaCrypSId, &(icPp->kms_uri)) != ismacryp_rc_ok) {
	  fprintf(stderr, 
            "%s: could not get ismacryp kms uri. sid %d\n", ProgName, ismaCrypSId);
          CHECK_AND_FREE(icPp->kms_uri);
	  ismacrypEndSession(ismaCrypSId);
	  exit(EXIT_ISMACRYP_END);
        }
       if ( ismacrypGetSelectiveEncryption(ismaCrypSId, &(icPp->selective_enc)) != ismacryp_rc_ok ) {
	  fprintf(stderr, 
            "%s: could not get ismacryp selec enc. sid %d\n", ProgName, ismaCrypSId);
	  ismacrypEndSession(ismaCrypSId);
	  exit(EXIT_ISMACRYP_END);
        }
        if (ismacrypGetKeyIndicatorLength(ismaCrypSId, &(icPp->key_ind_len)) != ismacryp_rc_ok) {
	  fprintf(stderr, 
            "%s: could not get ismacryp key ind len. sid %d\n", ProgName, ismaCrypSId);
	  ismacrypEndSession(ismaCrypSId);
	  exit(EXIT_ISMACRYP_END);
        }
        if (ismacrypGetIVLength(ismaCrypSId, &(icPp->iv_len)) != ismacryp_rc_ok) {
	  fprintf(stderr, 
            "%s: could not get ismacryp iv len. sid %d\n", ProgName, ismaCrypSId);
	  ismacrypEndSession(ismaCrypSId);
	  exit(EXIT_ISMACRYP_END);
        }
 
	//MP4EncAndCopyTrack(mp4File, curTrackId, ismaCrypSId, outputFile);
	MP4EncAndCopyTrack(mp4File, curTrackId, icPp,
                              ismacrypEncryptSampleAddHeader2,
                              (u_int32_t) ismaCrypSId,
                              outputFile);

	if (ismacrypEndSession(ismaCrypSId) != ismacryp_rc_ok) {
	  fprintf(stderr, 
		  "%s: could not end the ISMAcryp session\n", ProgName);
	  exit(EXIT_ISMACRYP_END);
	}
      } else {
	MP4CopyTrack(mp4File, curTrackId, outputFile);
      }
    }

    MP4Close(mp4File);
    MP4Close(outputFile);
  } else if (doExtract) {
    if (!mp4FileExists) {
      fprintf(stderr,
	      "%s: can't extract track in file that doesn't exist\n", 
	      ProgName);
      exit(EXIT_CREATE_FILE);
    }

    mp4File = MP4Read(mp4FileName, Verbosity);
    if (!mp4File) {
      // mp4 library should have printed a message
      exit(EXIT_CREATE_FILE);
    }

    char tempName[PATH_MAX];
    if (outputFileName == NULL) {
      snprintf(tempName, sizeof(tempName), 
	       "%s.t%u", mp4FileName, extractTrackId);
      outputFileName = tempName;
    }

    ExtractTrack(mp4File, extractTrackId, outputFileName);

    MP4Close(mp4File);

  } else if (doDelete) {
    if (!mp4FileExists) {
      fprintf(stderr,
	      "%s: can't delete track in file that doesn't exist\n", 
	      ProgName);
      exit(EXIT_CREATE_FILE);
    }

    mp4File = MP4Modify(mp4FileName, Verbosity);
    if (!mp4File) {
      // mp4 library should have printed a message
      exit(EXIT_CREATE_FILE);
    }

    MP4DeleteTrack(mp4File, deleteTrackId);

    MP4Close(mp4File);

    doOptimize = true;	// to purge unreferenced track data
  }
  
  if (doIsma) {
    MP4MakeIsmaCompliant(mp4FileName, Verbosity);
  }

  if (doOptimize) {
    if (!MP4Optimize(mp4FileName, NULL, Verbosity)) {
      // mp4 library should have printed a message
      exit(EXIT_OPTIMIZE_FILE);
    }
  }

 return(EXIT_SUCCESS);
}

MP4TrackId* CreateMediaTracks(MP4FileHandle mp4File, const char* inputFileName,
			      bool doEncrypt)
{
  FILE* inFile = fopen(inputFileName, "rb");

  if (inFile == NULL) {
    fprintf(stderr, 
	    "%s: can't open file %s: %s\n",
	    ProgName, inputFileName, strerror(errno));
    return NULL;
  }

  struct stat s;
  if (fstat(fileno(inFile), &s) < 0) {
    fprintf(stderr, 
	    "%s: can't stat file %s: %s\n",
	    ProgName, inputFileName, strerror(errno));
    return NULL;
  }

  if (s.st_size == 0) {
    fprintf(stderr, 
	    "%s: file %s is empty\n",
	    ProgName, inputFileName);
    return NULL;
  }

  const char* extension = strrchr(inputFileName, '.');
  if (extension == NULL) {
    fprintf(stderr, 
	    "%s: no file type extension\n", ProgName);
    return NULL;
  }

  static MP4TrackId trackIds[2] = {
    MP4_INVALID_TRACK_ID, MP4_INVALID_TRACK_ID
  };
  MP4TrackId* pTrackIds = trackIds;

  if (!strcasecmp(extension, ".avi")) {
    fclose(inFile);
    inFile = NULL;
    if (allowAvi) {
      fprintf(stderr, "You are creating an mp4 file from an avi file\n"
	      "This is not recommended due to the use of b-frames\n");
      
      pTrackIds = AviCreator(mp4File, inputFileName, doEncrypt);
    } else {
      fprintf(stderr, "Creating with an avi file is not recommended.\n"
	      "Use avi2raw on the audio and video parts of the avi file,\n"
	      "Then use mp4creator with the extracted files\n"
	      "Use avidump to find the video frame rate\n"
	      "Alternatively, use the --allow-avi-files option\n");
      exit(-1);
    }
  } else if (!strcasecmp(extension, ".aac")) {
    trackIds[0] = AacCreator(mp4File, inFile, doEncrypt);

  } else if (!strcasecmp(extension, ".mp3")
	     || !strcasecmp(extension, ".mp1")
	     || !strcasecmp(extension, ".mp2")) {
    trackIds[0] = Mp3Creator(mp4File, inFile, doEncrypt);

  } else if (!strcasecmp(extension, ".divx")
	     || !strcasecmp(extension, ".mp4v")
	     || !strcasecmp(extension, ".m4v")
	     || !strcasecmp(extension, ".xvid")
	     || !strcasecmp(extension, ".cmp")) {
    trackIds[0] = Mp4vCreator(mp4File, inFile, doEncrypt, allowVariableFrameRate);

  } else if ((strcasecmp(extension, ".mpg") == 0) ||
	     (strcasecmp(extension, ".mpeg") == 0)) {
    fclose(inFile);
    inFile = NULL;

    pTrackIds = MpegCreator(mp4File, inputFileName, doEncrypt);

  } else if (strcasecmp(extension, ".amr") == 0) {
	  trackIds[0] = AmrCreator(mp4File, inFile, false);
  } else if (strcasecmp(extension, ".263") == 0) {
	  trackIds[0] = H263Creator(mp4File, inFile, h263Profile, h263Level,
                                    setBitrates, H263CbrTolerance);
  } else if ((strcasecmp(extension, ".h264") == 0) ||
	     (strcasecmp(extension, ".264") == 0)) {
    trackIds[0] = H264Creator(mp4File, inFile);
  } else {
    fprintf(stderr, 
	    "%s: unknown file extension in %s\n", ProgName, inputFileName);
    return NULL;
  }

  if (inFile) {
    fclose(inFile);
  }

  if (pTrackIds == NULL || pTrackIds[0] == MP4_INVALID_TRACK_ID) {
    return NULL;
  }

  return pTrackIds;
}

void CreateHintTrack(MP4FileHandle mp4File, MP4TrackId mediaTrackId,
		     const char* payloadName, bool interleave, 
		     u_int16_t maxPayloadSize, bool doEncrypt)
{

  bool rc = FALSE;

  if (MP4GetTrackNumberOfSamples(mp4File, mediaTrackId) == 0) {
    fprintf(stderr, 
	    "%s: couldn't create hint track, no media samples\n", ProgName);
    MP4Close(mp4File);
    exit(EXIT_CREATE_HINT);
  }

  // vector out to specific hinters
  const char* trackType = MP4GetTrackType(mp4File, mediaTrackId);

  if (doEncrypt || MP4IsIsmaCrypMediaTrack(mp4File, mediaTrackId)) {

    ismacryp_session_id_t icSID;
    mp4av_ismacrypParams *icPp =  (mp4av_ismacrypParams *) malloc(sizeof(mp4av_ismacrypParams));
    memset(icPp, 0, sizeof(mp4av_ismacrypParams));

    if (!strcmp(trackType, MP4_AUDIO_TRACK_TYPE)) {
       if (ismacrypInitSession(&icSID, KeyTypeAudio) != 0) {
          fprintf(stderr, 
	      "%s: can't hint, error in init ismacryp session\n", ProgName);
          goto quit_error;
       }
    }
    else if (!strcmp(trackType, MP4_VIDEO_TRACK_TYPE)) {
       if (ismacrypInitSession(&icSID, KeyTypeVideo) != 0) {
          fprintf(stderr, 
	      "%s: can't hint, error in init ismacryp session\n", ProgName);
          goto quit_error;
       }
    }
    else {
      fprintf(stderr, 
	      "%s: can't hint track type %s\n", ProgName, trackType);
      goto quit_error;
    }
    
    // get all the ismacryp parameters needed by the hinters:
    if (ismacrypGetKeyCount(icSID, &(icPp->key_count)) != 0) {
      fprintf(stderr, 
	      "%s: can't hint, error getting key count for session %d\n", 
               ProgName, icSID);
      goto quit_error;
    }
    if (ismacrypGetKeyIndicatorLength(icSID, &(icPp->key_ind_len)) != 0) {
      fprintf(stderr, 
	      "%s: can't hint, error getting key ind len for session %d\n", 
               ProgName, icSID);
      goto quit_error;
    }

    if (ismacrypGetKeyIndPerAU(icSID, &(icPp->key_ind_perau)) != 0) {
      fprintf(stderr, 
	      "%s: can't hint, error getting key ind per au for session %d\n", 
               ProgName, icSID);
      goto quit_error;
    }
    if (ismacrypGetSelectiveEncryption(icSID, &(icPp->selective_enc)) != 0) {
      fprintf(stderr, 
	      "%s: can't hint, error getting selective enc for session %d\n", 
               ProgName, icSID);
      goto quit_error;
    }
    if (ismacrypGetDeltaIVLength(icSID, &(icPp->delta_iv_len)) != 0) {
      fprintf(stderr, 
	      "%s: can't hint, error getting delta iv len for session %d\n", 
               ProgName, icSID);
      goto quit_error;
    }
    if (ismacrypGetIVLength(icSID, &(icPp->iv_len)) != 0) {
      fprintf(stderr, 
	      "%s: can't hint, error getting iv len for session %d\n", 
               ProgName, icSID);
      goto quit_error;
    }
    if (ismacrypGetScheme(icSID, (ismacryp_scheme_t *) &(icPp->scheme)) != 0) {
      fprintf(stderr, 
	      "%s: can't hint, error getting scheme for session %d\n", 
               ProgName, icSID);
      goto quit_error;
    }
    if (ismacrypGetKey(icSID, 1,&(icPp->key_len),&(icPp->salt_len),
                       &(icPp->key),&(icPp->salt),&(icPp->key_life)) != 0) {
      fprintf(stderr, 
	      "%s: can't hint, error getting scheme for session %d\n", 
               ProgName, icSID);
      goto quit_error;
    }

    goto ok_continue;
    quit_error:
    ismacrypEndSession(icSID);
    MP4Close(mp4File);
    exit(EXIT_CREATE_HINT);
    ok_continue:

    if (!strcmp(trackType, MP4_AUDIO_TRACK_TYPE)) {
      rc = MP4AV_RfcCryptoAudioHinter(mp4File, mediaTrackId, 
                                      icPp, 
				      interleave, maxPayloadSize, 
				      "enc-mpeg4-generic");
      ismacrypEndSession(icSID);
    } else if (!strcmp(trackType, MP4_VIDEO_TRACK_TYPE)) {
      rc = MP4AV_RfcCryptoVideoHinter(mp4File, mediaTrackId, 
                                      icPp, 
                                      maxPayloadSize,
				      "enc-mpeg4-generic");
      ismacrypEndSession(icSID);
    } else {
      fprintf(stderr, 
	      "%s: can't hint track type %s\n", ProgName, trackType);
    }
  }
  else if (!strcmp(trackType, MP4_AUDIO_TRACK_TYPE)) {
    const char *media_data_name;
    media_data_name = MP4GetTrackMediaDataName(mp4File, mediaTrackId);
    
    if (strcasecmp(media_data_name, "mp4a") == 0) {
      u_int8_t audioType = MP4GetTrackEsdsObjectTypeId(mp4File, mediaTrackId);

      switch (audioType) {
      case MP4_INVALID_AUDIO_TYPE:
      case MP4_MPEG4_AUDIO_TYPE:
	if (payloadName && 
	    (strcasecmp(payloadName, "latm") == 0 ||
	     strcasecmp(payloadName, "mp4a-latm") == 0)) {
	  rc = MP4AV_Rfc3016LatmHinter(mp4File, mediaTrackId, 
				       maxPayloadSize);
	  break;
	}
      case MP4_MPEG2_AAC_MAIN_AUDIO_TYPE:
      case MP4_MPEG2_AAC_LC_AUDIO_TYPE:
      case MP4_MPEG2_AAC_SSR_AUDIO_TYPE:
	rc = MP4AV_RfcIsmaHinter(mp4File, mediaTrackId, 
				 interleave, maxPayloadSize);
	break;
      case MP4_MPEG1_AUDIO_TYPE:
      case MP4_MPEG2_AUDIO_TYPE:
	if (payloadName && 
	    (!strcasecmp(payloadName, "3119") 
	     || !strcasecmp(payloadName, "mpa-robust"))) {
	  rc = MP4AV_Rfc3119Hinter(mp4File, mediaTrackId, 
				   interleave, maxPayloadSize);
	} else {
	  rc = MP4AV_Rfc2250Hinter(mp4File, mediaTrackId, 
				   false, maxPayloadSize);
	}
	break;
      case MP4_PCM16_BIG_ENDIAN_AUDIO_TYPE:
      case MP4_PCM16_LITTLE_ENDIAN_AUDIO_TYPE:
	rc = L16Hinter(mp4File, mediaTrackId, maxPayloadSize);
	break;
      case MP4_ULAW_AUDIO_TYPE:
      case MP4_ALAW_AUDIO_TYPE:
	rc = G711Hinter(mp4File, mediaTrackId, maxPayloadSize);
	break;
      default:
	fprintf(stderr, 
		"%s: can't hint non-MPEG4/non-MP3 audio type\n", ProgName);
      }
    } else if (strcasecmp(media_data_name, "samr") == 0 ||
	       strcasecmp(media_data_name, "sawb") == 0) {
      rc = MP4AV_Rfc3267Hinter(mp4File, mediaTrackId, maxPayloadSize);
    }
  } else if (!strcmp(trackType, MP4_VIDEO_TRACK_TYPE)) {
    const char *media_data_name;
    media_data_name = MP4GetTrackMediaDataName(mp4File, mediaTrackId);
    
    if (strcasecmp(media_data_name, "mp4v") == 0) {
      u_int8_t videoType = MP4GetTrackEsdsObjectTypeId(mp4File, mediaTrackId);
      
      switch (videoType) {
      case MP4_MPEG4_VIDEO_TYPE:
	rc = MP4AV_Rfc3016Hinter(mp4File, mediaTrackId, maxPayloadSize);
	break;
      case MP4_MPEG1_VIDEO_TYPE:
      case MP4_MPEG2_SIMPLE_VIDEO_TYPE:
      case MP4_MPEG2_MAIN_VIDEO_TYPE:
	rc = Mpeg12Hinter(mp4File, mediaTrackId, maxPayloadSize);
	break;
      default:
	fprintf(stderr, 
		"%s: can't hint non-MPEG4 video type\n", ProgName);
	break;
      }
    } else if (strcasecmp(media_data_name, "avc1") == 0) {
      // h264;
      rc = MP4AV_H264Hinter(mp4File, mediaTrackId, maxPayloadSize);
    } else if (strcasecmp(media_data_name, "s263") == 0) {
      rc = MP4AV_Rfc2429Hinter(mp4File, mediaTrackId, maxPayloadSize);
    }
  } else {
    fprintf(stderr, 
	    "%s: can't hint track type %s\n", ProgName, trackType);
  }

  if (!rc) {
    fprintf(stderr, 
	    "%s: error hinting track %u\n", ProgName, mediaTrackId);
    MP4Close(mp4File);
    exit(EXIT_CREATE_HINT);
  }
}

static void extract_h264_track (MP4FileHandle mp4File, 
				MP4TrackId trackId,
				int outFd,
				const char *outputFileName)
{
  uint8_t **seqheader, **pictheader;
  uint32_t *pictheadersize, *seqheadersize;
  uint32_t ix;
  uint8_t header[4] = {0, 0, 0, 1};
  MP4GetTrackH264SeqPictHeaders(mp4File, trackId, 
				&seqheader, &seqheadersize,
				&pictheader, &pictheadersize);
  for (ix = 0; seqheadersize[ix] != 0; ix++) {
    write(outFd, header, 4);
    write(outFd, seqheader[ix], seqheadersize[ix]);
  }
  for (ix = 0; pictheadersize[ix] != 0; ix++) {
    write(outFd, header, 4);
    write(outFd, pictheader[ix], pictheadersize[ix]);
  }
  
  MP4SampleId numSamples = 
    MP4GetTrackNumberOfSamples(mp4File, trackId);
  u_int8_t* pSample;
  u_int32_t sampleSize;
  uint32_t buflen_size;
  MP4GetTrackH264LengthSize(mp4File, trackId, &buflen_size);

  // extraction loop
  for (MP4SampleId sampleId = 1 ; sampleId <= numSamples; sampleId++) {
    pSample = NULL;
    sampleSize = 0;
    int rc = MP4ReadSample(
			 mp4File, 
			 trackId, 
			 sampleId, 
			 &pSample, 
			 &sampleSize);
    if (rc == 0) {
      fprintf(stderr, "%s: read sample %u for %s failed\n",
	      ProgName, sampleId, outputFileName);
      exit(EXIT_EXTRACT_TRACK);
    }
    uint32_t read_offset = 0;
    uint32_t nal_len;
    bool first = true;
#if 0
    printf("\n\nsample id: %u\n", sampleId);
#endif
    do {
#if 0
      printf("read offset %u buflen %u sample Size %u\n", 
	     read_offset, buflen_size, sampleSize);
#endif
      if (read_offset + buflen_size >= sampleSize) {
	fprintf(stderr, 
		"extra bytes at end of sample %d - nal len size %u, %u bytes left", 
		sampleId, buflen_size, sampleSize - read_offset);
	read_offset = sampleSize;
      } else {
	if (buflen_size == 1) {
	  nal_len = pSample[read_offset];
	} else {
	  nal_len = (pSample[read_offset] << 8) | pSample[read_offset + 1];
	  if (buflen_size == 4) {
	  nal_len <<= 16;
	  nal_len |= (pSample[read_offset + 2] << 8) | pSample[read_offset + 3];
	  }
	}
#if 0
	printf("read offset %u nallen %u sample Size %u %d\n", 
	       read_offset, nal_len, sampleSize, first);
#endif
	if (read_offset + nal_len > sampleSize) {
	  fprintf(stderr, 
		  "nal length past end of buffer - sample %u size %u frame offset %u left %u\n",
		  sampleId, nal_len, read_offset, sampleSize - read_offset);
	  read_offset = sampleSize;
	} else {
	  if (first) {
	    write(outFd, header, 4);
	    first = false;
	  } else {
	    write(outFd, header + 1, 3);
	  }
	  // printf("sample id %d size %u %x\n", sampleId, nal_len, nal_len);
	  write(outFd, pSample + read_offset + buflen_size,
		nal_len);
	  read_offset += nal_len + buflen_size;
	}
      }
    } while (read_offset < sampleSize);
    free(pSample);
  }
  close(outFd);
}



void ExtractTrack (MP4FileHandle mp4File, MP4TrackId trackId, 
		   const char* outputFileName)
{
  int openFlags = O_WRONLY | O_TRUNC | OPEN_CREAT;
  u_int8_t amrType = AMR_TYPE_NONE;
  int outFd = open(outputFileName, openFlags, 0644);


  if (outFd == -1) {
    fprintf(stderr, "%s: can't open %s: %s\n",
	    ProgName, outputFileName, strerror(errno));
    exit(EXIT_EXTRACT_TRACK);
  }

  // some track types have special needs
  // to properly recreate their raw ES file

  bool prependES = false;
  bool prependADTS = false;

  const char* trackType =
    MP4GetTrackType(mp4File, trackId);
  const char *media_data_name = 
    MP4GetTrackMediaDataName(mp4File, trackId);

  if (!strcmp(trackType, MP4_VIDEO_TRACK_TYPE)) {
    if (strcmp(media_data_name, "avc1") == 0) {
      extract_h264_track(mp4File, trackId, outFd, outputFileName);
      return;
    }
    prependES = true;
  } else if (!strcmp(trackType, MP4_AUDIO_TRACK_TYPE)) {

    if (strcmp(media_data_name, "mp4a") == 0) {
      uint8_t type = 	
	MP4GetTrackEsdsObjectTypeId(mp4File, trackId);
      if (MP4_IS_AAC_AUDIO_TYPE(type) || 
	  type == MP4_MPEG4_INVALID_AUDIO_TYPE)
      prependADTS = true;
    } else if (strcmp(media_data_name, "sawb") == 0) {
      amrType = AMR_TYPE_AMRWB;
    } else if (strcmp(media_data_name, "samr") == 0) {
      amrType = AMR_TYPE_AMR;
    }
    switch (amrType) {
    case AMR_TYPE_AMR:
      if (write(outFd, AMR_MAGIC_AMR, AMR_MAGIC_LEN_AMR) != AMR_MAGIC_LEN_AMR) {
        fprintf(stderr, "%s: can't write to file: %s\n",
		ProgName, strerror(errno));
        return;
      }
      break;
    case AMR_TYPE_AMRWB:
      if (write(outFd, AMR_MAGIC_AMRWB, AMR_MAGIC_LEN_AMRWB) != AMR_MAGIC_LEN_AMRWB) {
        fprintf(stderr, "%s: can't write to file: %s\n",
		ProgName, strerror(errno));
        return;
      }
      break;
    default:
      break;
    }
  }

  MP4SampleId numSamples = 
#if 1
    MP4GetTrackNumberOfSamples(mp4File, trackId);
#else
  1;
#endif
  u_int8_t* pSample;
  u_int32_t sampleSize;

  // extraction loop
  for (MP4SampleId sampleId = 1 ; sampleId <= numSamples; sampleId++) {
    int rc;

    // signal to ReadSample() 
    // that it should malloc a buffer for us
    pSample = NULL;
    sampleSize = 0;

    if (prependADTS) {
      // need some very specialized work for these
      MP4AV_AdtsMakeFrameFromMp4Sample(
				       mp4File,
				       trackId,
				       sampleId,
				       aacProfileLevel,
				       &pSample,
				       &sampleSize);
    } else {
      // read the sample
      rc = MP4ReadSample(
			 mp4File, 
			 trackId, 
			 sampleId, 
			 &pSample, 
			 &sampleSize);

      if (rc == 0) {
	fprintf(stderr, "%s: read sample %u for %s failed\n",
		ProgName, sampleId, outputFileName);
	exit(EXIT_EXTRACT_TRACK);
      }

      if (prependES && sampleId == 1) {
	u_int8_t* pConfig = NULL;
	u_int32_t configSize = 0;
	
	if (MP4GetTrackESConfiguration(mp4File, trackId, 
				       &pConfig, &configSize)) {
			
	  if (configSize != 0) {
	    write(outFd, pConfig, configSize);
	  }
	  CHECK_AND_FREE(pConfig);
	}

      }
    }

    rc = write(outFd, pSample, sampleSize);

    if (rc == -1 || (u_int32_t)rc != sampleSize) {
      fprintf(stderr, "%s: write to %s failed: %s\n",
	      ProgName, outputFileName, strerror(errno));
      exit(EXIT_EXTRACT_TRACK);
    }

    free(pSample);
  }

  // close ES file
  close(outFd);
}

bool Is3GPP(MP4FileHandle mp4File)
{
  u_int32_t numberOfTracks = MP4GetNumberOfTracks(mp4File);
  u_int32_t i;
 

  for (i = 0 ; i < numberOfTracks ; i++) {
    MP4TrackId trackId = MP4FindTrackId(mp4File, i);

    const char *type, *media_data_name;
    type = MP4GetTrackType(mp4File, trackId);
    media_data_name = MP4GetTrackMediaDataName(mp4File, trackId);

    if (strcmp(type, MP4_VIDEO_TRACK_TYPE) == 0) {
      if (strcmp(media_data_name, "s263") != 0) {
	return false;
      }
    } else if (strcmp(type, MP4_AUDIO_TRACK_TYPE) == 0) {
      if (strcmp(media_data_name, "samr") != 0 &&
	  strcmp(media_data_name, "sawb") != 0)
	return false;
    }
  }
  return true;
}

