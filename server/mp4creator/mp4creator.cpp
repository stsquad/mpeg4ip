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
 * Copyright (C) Cisco Systems Inc. 2001-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie			dmackie@cisco.com
 *		Alix Marchandise-Franquet	alix@cisco.com
 */

#define MP4CREATOR_GLOBALS
#include "mp4creator.h"
#include "mpeg4ip_getopt.h"
#include "mpeg.h"

// forward declarations

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

// external declarations

// track creators
MP4TrackId* AviCreator(MP4FileHandle mp4File, const char* aviFileName, 
		       bool doEncrypt);

MP4TrackId AacCreator(MP4FileHandle mp4File, FILE* inFile, bool doEncrypt);

MP4TrackId Mp3Creator(MP4FileHandle mp4File, FILE* inFile, bool doEncrypt);

MP4TrackId Mp4vCreator(MP4FileHandle mp4File, FILE* inFile, bool doEncrypt);


// main routine
int main(int argc, char** argv)
{
  char* usageString = 
    " <options> <mp4-file>\n"
    "  Options:\n"
    "  -aac-old-file-format    Use old file format with 58 bit adts headers\n"
    "  -aac-profile=[2|4]      Force AAC to mpeg2 or mpeg4 profile\n"
    "  -create=<input-file>    Create track from <input-file>\n"
    "    input files can be of type: .aac .mp3 .divx .mp4v .m4v .cmp .xvid\n"
    "  -extract=<track-id>     Extract a track\n"
    "  -encrypt[=<track-id>]   Encrypt a track, also -E\n"
    "  -delete=<track-id>      Delete a track\n"
    "  -hint[=<track-id>]      Create hint track, also -H\n"
    "  -interleave             Use interleaved audio payload format, also -I\n"
    "  -list                   List tracks in mp4 file\n"
    "  -mpeg4-video-profile=<level> Mpeg4 video profile override\n"
    "  -mtu=<size>             MTU for hint track\n"
    "  -optimize               Optimize mp4 file layout\n"
    "  -payload=<payload>      Rtp payload type \n"
    "                          (use 3119 or mpa-robust for mp3 rfc 3119 support)\n"
    "  -rate=<fps>             Video frame rate, e.g. 30 or 29.97\n"
    "  -timescale=<ticks>      Time scale (ticks per second)\n"
    "  -use64bits              Use for large files\n"
    "  -use64bitstime          Use for 64 Bit times (not QT player compatible)\n"
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
      { "create", 1, 0, 'c' },
      { "delete", 1, 0, 'd' },
      { "extract", 2, 0, 'e' },
      { "encrypt", 2, 0, 'E' },
      { "help", 0, 0, '?' },
      { "hint", 2, 0, 'H' },
      { "interleave", 0, 0, 'I' },
      { "list", 0, 0, 'l' },
      { "mpeg4-video-profile", 1, 0, 'M' },
      { "mtu", 1, 0, 'm' },
      { "optimize", 0, 0, 'O' },
      { "payload", 1, 0, 'p' },
      { "rate", 1, 0, 'r' },
      { "timescale", 1, 0, 't' },
      { "use64bits", 0, 0, 'u' },
      { "use64bitstime", 0, 0, 'U' },
      { "verbose", 2, 0, 'v' },
      { "version", 0, 0, 'V' },
      { NULL, 0, 0, 0 }
    };

    c = getopt_long_only(argc, argv, "ac:d:e:E::H::Ilm:Op:r:t:uUv::V",
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
    case 'c':
      doCreate = true;
      inputFileName = optarg;
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
    case 'I':
      doInterleave = true;
      break;
    case 'l':
      doList = true;
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
      if (sscanf(optarg, "%u", &VideoProfileLevel) != 1 ||
	  (VideoProfileLevel < 0 || VideoProfileLevel > 3)) {
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
    case 'r':
      if (optarg == NULL) {
	fprintf(stderr, "%s:no rate specifed\n", ProgName);
	exit(EXIT_COMMAND_LINE);
      }
      if (sscanf(optarg, "%f", &VideoFrameRate) != 1) {
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
      && !doOptimize && !doExtract && !doDelete) {
    fprintf(stderr, 
	    "%s: no operation specified\n",
	    ProgName);
    exit(EXIT_COMMAND_LINE);
  }
  if ((doCreate || doHint || doEncrypt) && doExtract) {
    fprintf(stderr, 
	    "%s: extract operation must be done separately\n",
	    ProgName);
    exit(EXIT_COMMAND_LINE);
  }
  if ((doCreate || doHint || doEncrypt) && doDelete) {
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
	mp4File = MP4Create(mp4FileName, Verbosity,
			    createFlags);
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
	
	if (!strcmp(type, MP4_AUDIO_TRACK_TYPE)) { 
	  allMpeg4Streams &=
	    (MP4GetTrackEsdsObjectTypeId(mp4File, *pTrackId) 
	     == MP4_MPEG4_AUDIO_TYPE);
	  
	} else if (!strcmp(type, MP4_VIDEO_TRACK_TYPE)) { 
	  allMpeg4Streams &=
	    (MP4GetTrackEsdsObjectTypeId(mp4File, *pTrackId)
	     == MP4_MPEG4_VIDEO_TYPE);
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
	if (!strcmp(type, MP4_AUDIO_TRACK_TYPE)) { 
	  allMpeg4Streams &=
	    (MP4GetTrackEsdsObjectTypeId(mp4File, trackId) 
	     == MP4_MPEG4_AUDIO_TYPE);
			    
	} else if (!strcmp(type, MP4_VIDEO_TRACK_TYPE)) { 
	  allMpeg4Streams &=
	    (MP4GetTrackEsdsObjectTypeId(mp4File, trackId) 
	     == MP4_MPEG4_VIDEO_TYPE);
	}
      }
    }

    char *buffer;
    char *value;
    uint32_t newverbosity;
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
    
    MP4Close(mp4File);
    MP4MakeIsmaCompliant(mp4FileName, Verbosity, allMpeg4Streams);

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

    MP4FileHandle outputFile = MP4Create(outputFileName, Verbosity,
					 createFlags);
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
          if (icPp->kms_uri != NULL) free(icPp->kms_uri);
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
    bool allMpeg4Streams = true;
    MP4MakeIsmaCompliant(outputFileName, Verbosity, allMpeg4Streams);

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

    pTrackIds = AviCreator(mp4File, inputFileName, doEncrypt);

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
    trackIds[0] = Mp4vCreator(mp4File, inFile, doEncrypt);

  } else if ((strcasecmp(extension, ".mpg") == 0) ||
	     (strcasecmp(extension, ".mpeg") == 0)) {
    fclose(inFile);
    inFile = NULL;

    pTrackIds = MpegCreator(mp4File, inputFileName, doEncrypt);

  } else {
    fprintf(stderr, 
	    "%s: unknown file type\n", ProgName);
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
    u_int8_t audioType = MP4GetTrackEsdsObjectTypeId(mp4File, mediaTrackId);

    switch (audioType) {
    case MP4_MPEG4_AUDIO_TYPE:
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
    default:
      fprintf(stderr, 
	      "%s: can't hint non-MPEG4/non-MP3 audio type\n", ProgName);
    }
  } else if (!strcmp(trackType, MP4_VIDEO_TRACK_TYPE)) {
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

void ExtractTrack( MP4FileHandle mp4File, MP4TrackId trackId, 
		   const char* outputFileName)
{
  int openFlags = O_WRONLY | O_TRUNC | OPEN_CREAT;

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

  if (!strcmp(trackType, MP4_VIDEO_TRACK_TYPE)) {
    if (MP4_IS_MPEG4_VIDEO_TYPE(MP4GetTrackEsdsObjectTypeId(mp4File, trackId))) {
      prependES = true;
    }
  } else if (!strcmp(trackType, MP4_AUDIO_TRACK_TYPE)) {
    if (MP4_IS_AAC_AUDIO_TYPE(MP4GetTrackEsdsObjectTypeId(mp4File, trackId))) {
      prependADTS = true;
    }
  }

  MP4SampleId numSamples = 
    MP4GetTrackNumberOfSamples(mp4File, trackId);
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
	
	MP4GetTrackESConfiguration(mp4File, trackId, 
				   &pConfig, &configSize);
				
	write(outFd, pConfig, configSize);

	free(pConfig);
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

