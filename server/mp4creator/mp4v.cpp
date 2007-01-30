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
 * Portions created by Ximpo Group Ltd. are
 * Copyright (C) Ximpo Group Ltd. 2003, 2004.  All Rights Reserved.
 *
 * Contributor(s):
 *		Dave Mackie                dmackie@cisco.com
 *              Alix Marchandise-Franquet  alix@cisco.com
 *		Ximpo Group Ltd.           mp4v2@ximpo.com
 */

//#define DEBUG_MP4V 1
//#define DEBUG_MP4V_TS 1
/*
 * Notes:
 *  - file formatted with tabstops == 4 spaces
 */

#include <mp4creator.h>

typedef struct mpeg4_frame_t {
  mpeg4_frame_t *next;
  MP4Timestamp frameTimestamp;
  int vopType;
} mpeg4_frame_t;

/*
 * Load the next syntatic object from the file
 * into the supplied buffer, which better be large enough!
 *
 * Objects are delineated by 4 byte start codes (0x00 0x00 0x01 0x??)
 * Objects are padded to byte boundaries, and the encoder assures that
 * the start code pattern is never generated for encoded data
 */
static int LoadNextObject(FILE* inFile,
        u_int8_t* pBuf, u_int32_t* pBufSize, u_int8_t* pObjCode)
{
    static u_int8_t state = 0;
    static u_int8_t nextObjCode = 0;

    *pBufSize = 0;

    /* initial state, never read from file before */
    if (state == 0) {
        /*
         * go looking for first sync word
         * we discard anything before this
         */
        state = 1;

        while (state < 5) {
            /* read a byte */
            u_char b;

            if (fread(&b, 1, 1, inFile) == 0) {
                // EOF or IO error
                return 0;
            }

            switch (state) {
                case 1:
                    if (b == 0) {
                        state = 2;
                    }
                    break;
                case 2:
                    if (b == 0) {
                        state = 3;
                    }
                    break;
                case 3:
                    if (b == 1) {
                        state = 4;
                    }
                    break;
                case 4:
                    pBuf[0] = 0;
                    pBuf[1] = 0;
                    pBuf[2] = 1;
                    pBuf[3] = b;
                    (*pObjCode) = (u_int)b;
                    *pBufSize = 4;
                    state = 5;
                    break;
            }
        }

        /* we're primed now */
        state = 1;

    } else if (state == 5) {
        state = 0;
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
                } else if (b == 0) {
                    pBuf[(*pBufSize)++] = 0;
                    // state remains 3
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
                ASSERT(false);
        }
    }
}

MP4TrackId Mp4vCreator(MP4FileHandle mp4File, FILE* inFile, bool doEncrypt,
		       bool allowVariableFrameRate)
{
    bool rc;

    u_int8_t sampleBuffer[256 * 1024 * 2];
    u_int8_t* pCurrentSample = sampleBuffer;
    u_int32_t maxSampleSize = sizeof(sampleBuffer) / 2;
    u_int32_t prevSampleSize = 0;

    // the current syntactical object
    // typically 1:1 with a sample
    // but not always, i.e. non-VOP's
    u_int8_t* pObj = pCurrentSample;
    u_int32_t objSize;
    u_int8_t objType;

    // the current sample
    MP4SampleId sampleId = 1;
    MP4Timestamp currentSampleTime = 0;

    // the last reference VOP
    MP4SampleId refVopId = 1;
    MP4Timestamp refVopTime = 0;

    // track configuration info
    u_int8_t videoProfileLevel = MPEG4_SP_L3;
    u_int8_t timeBits = 15;
    u_int16_t timeTicks = 30000;
    u_int16_t frameDuration = 3000;
    u_int16_t frameWidth = 320;
    u_int16_t frameHeight = 240;
    u_int32_t esConfigSize = 0;
    int vopType = 0;
    int prevVopType = 0;
    bool foundVOSH = false, foundVO = false, foundVOL = false;
    u_int32_t lastVopTimeIncrement = 0;
    bool variableFrameRate = false;
    bool lastFrame = false;
    bool haveBframes = false;
    mpeg4_frame_t *head = NULL, *tail = NULL;

    // start reading objects until we get the first VOP
    while (LoadNextObject(inFile, pObj, &objSize, &objType)) {
        // guard against buffer overflow
        if (pObj + objSize >= pCurrentSample + maxSampleSize) {
            fprintf(stderr,
                    "%s: buffer overflow, invalid video stream?\n", ProgName);
            return MP4_INVALID_TRACK_ID;
        }
#ifdef DEBUG_MP4V
        if (Verbosity & MP4_DETAILS_SAMPLE) {
            printf("MP4V type %x size %u\n",
                    objType, objSize);
        }
#endif

        if (objType == MP4AV_MPEG4_VOSH_START) {
            MP4AV_Mpeg4ParseVosh(pObj, objSize,
                    &videoProfileLevel);
            foundVOSH = true;
        } else if (objType == MP4AV_MPEG4_VO_START) {
            foundVO = true;
        } else if (objType == MP4AV_MPEG4_VOL_START) {
            MP4AV_Mpeg4ParseVol(pObj, objSize,
                    &timeBits, &timeTicks, &frameDuration,
                    &frameWidth, &frameHeight);

            foundVOL = true;
#ifdef DEBUG_MP4V
            printf("ParseVol: timeBits %u timeTicks %u frameDuration %u\n",
                    timeBits, timeTicks, frameDuration);
#endif

        } else if (foundVOL == true || objType == MP4AV_MPEG4_VOP_START) {
            esConfigSize = pObj - pCurrentSample;
            // ready to set up mp4 track
            break;
        }
        /* XXX why do we need this if ?
         * It looks like it will remove this object ... XXX */
	// It does.  On Purpose.  wmay 6/2004
        if (objType != MP4AV_MPEG4_USER_DATA_START) {
            pObj += objSize;
        }
    }

    if (foundVOSH == false) {
        fprintf(stderr,
                "%s: no VOSH header found in MPEG-4 video.\n"
                "This can cause problems with players other than mp4player. \n",
                ProgName);
    } else {
        if (VideoProfileLevelSpecified &&
                videoProfileLevel != VideoProfileLevel) {
            fprintf(stderr,
                    "%s: You have specified a different video profile level than was detected in the VOSH header\n"
                    "The level you specified was %d and %d was read from the VOSH\n",
                    ProgName, VideoProfileLevel, videoProfileLevel);
        }
    }
    if (foundVO == false) {
        fprintf(stderr,
                "%s: No VO header found in mpeg-4 video.\n"
                "This can cause problems with players other than mp4player\n",
                ProgName);
    }
    if (foundVOL == false) {
        fprintf(stderr,
                "%s: fatal: No VOL header found in mpeg-4 video stream\n",
                ProgName);
        return MP4_INVALID_TRACK_ID;
    }

    // convert frame duration to canonical time scale
    // note zero value for frame duration signals variable rate video
    if (timeTicks == 0) {
        timeTicks = 1;
    }
    u_int32_t mp4FrameDuration = 0;

    if (VideoFrameRate) {
      mp4FrameDuration = (u_int32_t)(((double)Mp4TimeScale) / VideoFrameRate);    
    } else if (frameDuration) {
	  VideoFrameRate = frameDuration;
	  VideoFrameRate /= timeTicks;
	  mp4FrameDuration = (Mp4TimeScale * frameDuration) / timeTicks;
    } else {
      if (allowVariableFrameRate == false ) {
	fprintf(stderr,
		"%s: variable rate video stream signalled,"
		" please specify average frame rate with -r option\n"
		" or --variable-frame-rate argument\n",
		ProgName);
	return MP4_INVALID_TRACK_ID;
      }

        variableFrameRate = true;
    }

    ismacryp_session_id_t ismaCrypSId;
    mp4v2_ismacrypParams *icPp =  (mp4v2_ismacrypParams *) malloc(sizeof(mp4v2_ismacrypParams));
    memset(icPp, 0, sizeof(mp4v2_ismacrypParams));


    // initialize ismacryp session if encrypting
    if (doEncrypt) {

        if (ismacrypInitSession(&ismaCrypSId,KeyTypeVideo) != 0) {
            fprintf(stderr, "%s: could not initialize the ISMAcryp session\n",
                    ProgName);
            return MP4_INVALID_TRACK_ID;
        }
        if (ismacrypGetScheme(ismaCrypSId, &(icPp->scheme_type)) != ismacryp_rc_ok) {
            fprintf(stderr, "%s: could not get ismacryp scheme type. sid %d\n",
                    ProgName, ismaCrypSId);
            ismacrypEndSession(ismaCrypSId);
            return MP4_INVALID_TRACK_ID;
        }
        if (ismacrypGetSchemeVersion(ismaCrypSId, &(icPp->scheme_version)) != ismacryp_rc_ok) {
            fprintf(stderr, "%s: could not get ismacryp scheme ver. sid %d\n",
                    ProgName, ismaCrypSId);
            ismacrypEndSession(ismaCrypSId);
            return MP4_INVALID_TRACK_ID;
        }
        if (ismacrypGetKMSUri(ismaCrypSId, &(icPp->kms_uri)) != ismacryp_rc_ok) {
            fprintf(stderr, "%s: could not get ismacryp kms uri. sid %d\n",
                    ProgName, ismaCrypSId);
            CHECK_AND_FREE(icPp->kms_uri);
            ismacrypEndSession(ismaCrypSId);
            return MP4_INVALID_TRACK_ID;
        }
        if ( ismacrypGetSelectiveEncryption(ismaCrypSId, &(icPp->selective_enc)) != ismacryp_rc_ok ) {
            fprintf(stderr, "%s: could not get ismacryp selec enc. sid %d\n",
                    ProgName, ismaCrypSId);
            ismacrypEndSession(ismaCrypSId);
            return MP4_INVALID_TRACK_ID;
        }
        if (ismacrypGetKeyIndicatorLength(ismaCrypSId, &(icPp->key_ind_len)) != ismacryp_rc_ok) {
            fprintf(stderr, "%s: could not get ismacryp key ind len. sid %d\n",
                    ProgName, ismaCrypSId);
            ismacrypEndSession(ismaCrypSId);
            return MP4_INVALID_TRACK_ID;
        }
        if (ismacrypGetIVLength(ismaCrypSId, &(icPp->iv_len)) != ismacryp_rc_ok) {
            fprintf(stderr, "%s: could not get ismacryp iv len. sid %d\n",
                    ProgName, ismaCrypSId);
            ismacrypEndSession(ismaCrypSId);
            return MP4_INVALID_TRACK_ID;
        }
    }

    // create the new video track
    MP4TrackId trackId;
    if (doEncrypt) {
        trackId =
            MP4AddEncVideoTrack(
                    mp4File,
                    Mp4TimeScale,
                    mp4FrameDuration,
                    frameWidth,
                    frameHeight,
                    icPp,
                    MP4_MPEG4_VIDEO_TYPE);
    } else {
        trackId =
            MP4AddVideoTrack(
                    mp4File,
                    Mp4TimeScale,
                    mp4FrameDuration,
                    frameWidth,
                    frameHeight,
                    MP4_MPEG4_VIDEO_TYPE);
    }

    if (trackId == MP4_INVALID_TRACK_ID) {
        fprintf(stderr,
                "%s: can't create video track\n", ProgName);
        return MP4_INVALID_TRACK_ID;
    }

    if (VideoProfileLevelSpecified) {
        videoProfileLevel = VideoProfileLevel;
    }
    if (MP4GetNumberOfTracks(mp4File, MP4_VIDEO_TRACK_TYPE) == 1) {
        MP4SetVideoProfileLevel(mp4File, videoProfileLevel);
    }
    printf("es config size is %d\n", esConfigSize);
    if (esConfigSize) {
        MP4SetTrackESConfiguration(mp4File, trackId,
                pCurrentSample, esConfigSize);

        // move past ES config, so it doesn't go into first sample
        pCurrentSample += esConfigSize;
    }
    // Move the current frame to the beginning of the
    // buffer
    memmove(sampleBuffer, pCurrentSample, pObj - pCurrentSample + objSize);
    pObj = sampleBuffer + (pObj - pCurrentSample);
    pCurrentSample = sampleBuffer;
    MP4Timestamp prevFrameTimestamp = 0;

    // now process the rest of the video stream
    while ( true ) {
        if ( objType != MP4AV_MPEG4_VOP_START ) {
	  // keep it in the buffer until a VOP comes along
	  // Actually, do nothings, since we only want VOP
	  // headers in the stream - wmay 6/2004
	  //pObj += objSize;

        } else { // we have VOP
            u_int32_t sampleSize = (pObj + objSize) - pCurrentSample;

            vopType = MP4AV_Mpeg4GetVopType(pObj, objSize);

	    mpeg4_frame_t *fr = MALLOC_STRUCTURE(mpeg4_frame_t);
	    if (head == NULL) {
	      head = tail = fr;
	    } else {
	      tail->next = fr;
	      tail = fr;
	    }
	    fr->vopType = vopType;
	    fr->frameTimestamp = currentSampleTime;
	    fr->next = NULL;
            if ( variableFrameRate ) {
                // variable frame rate:  recalculate "mp4FrameDuration"
                if ( lastFrame ) {
                    // last frame
                    mp4FrameDuration = Mp4TimeScale / timeTicks;
                } else {
                    // not the last frame
                    u_int32_t vopTimeIncrement;
                    MP4AV_Mpeg4ParseVop(pObj, objSize, &vopType, timeBits, timeTicks, &vopTimeIncrement);
                    u_int32_t vopTime = vopTimeIncrement - lastVopTimeIncrement;
                    mp4FrameDuration = (Mp4TimeScale * vopTime) / timeTicks;
                    lastVopTimeIncrement = vopTimeIncrement % timeTicks;
                }
	    }
            if ( prevSampleSize > 0 ) { // not the first time
                // fill sample data & length to write
                u_int8_t* sampleData2Write = NULL;
                u_int32_t sampleLen2Write = 0;
                if ( doEncrypt ) {
                    if ( ismacrypEncryptSampleAddHeader(ismaCrypSId,
                                sampleSize,
                                sampleBuffer,
                                &sampleLen2Write,
                                &sampleData2Write) != 0 ) {
                        fprintf(stderr,
                                "%s: can't encrypt video sample and add header %u\n",
                                ProgName, sampleId);
                    }
                } else {
                    sampleData2Write = sampleBuffer;
                    sampleLen2Write = prevSampleSize;
                }

		
            if (variableFrameRate == false) {
	      double now_calc;
	      now_calc = sampleId;
	      now_calc *= Mp4TimeScale;
	      now_calc /= VideoFrameRate;
	      MP4Timestamp now_ts = (MP4Timestamp)now_calc;
	      mp4FrameDuration = now_ts - prevFrameTimestamp;
	      prevFrameTimestamp = now_ts;
	      currentSampleTime = now_ts;
	    }
                // Write the previous sample
                rc = MP4WriteSample(mp4File, trackId,
                        sampleData2Write, sampleLen2Write,
                        mp4FrameDuration, 0, prevVopType == VOP_TYPE_I);

                if ( doEncrypt && sampleData2Write ) {
                    // buffer allocated by encrypt function.
                    // must free it!
                    free(sampleData2Write);
                }

                if ( !rc ) {
                    fprintf(stderr,
                            "%s: can't write video frame %u\n",
                            ProgName, sampleId);
                    MP4DeleteTrack(mp4File, trackId);
                    return MP4_INVALID_TRACK_ID;
                }

                // deal with rendering time offsets
                // that can occur when B frames are being used
                // which is the case for all profiles except Simple Profile
		haveBframes |= (prevVopType == VOP_TYPE_B);

		if ( lastFrame ) {
		  // finish read frames
		  break;
		}
                sampleId++;
            } // not the first time

            currentSampleTime += mp4FrameDuration;

            // Move the current frame to the beginning of the
            // buffer
            memmove(sampleBuffer, pCurrentSample, sampleSize);
            prevSampleSize = sampleSize;
            prevVopType = vopType;
            // reset pointers
            pObj = pCurrentSample = sampleBuffer + sampleSize;
        } // we have VOP

        // load next object from bitstream
        if (!LoadNextObject(inFile, pObj, &objSize, &objType)) {
            if (objType != MP4AV_MPEG4_VOP_START)
                break;
            lastFrame = true;
            objSize = 0;
            continue;
        }
        // guard against buffer overflow
        if (pObj + objSize >= pCurrentSample + maxSampleSize) {
            fprintf(stderr,
                    "%s: buffer overflow, invalid video stream?\n", ProgName);
            MP4DeleteTrack(mp4File, trackId);
            return MP4_INVALID_TRACK_ID;
        }
#ifdef DEBUG_MP4V
        if (Verbosity & MP4_DETAILS_SAMPLE) {
            printf("MP4V type %x size %u\n",
                    objType, objSize);
        }
#endif
    }
    bool doRenderingOffset = false;
    switch (videoProfileLevel) {
    case MPEG4_SP_L0:
    case MPEG4_SP_L1:
    case MPEG4_SP_L2:
    case MPEG4_SP_L3:
      break;
    default:
      doRenderingOffset = true;
      break;
    }
   
    if (doRenderingOffset && haveBframes) {
      // only generate ctts (with rendering offset for I, P frames) when
      // we need one.  We saved all the frames types and timestamps above - 
      // we can't use MP4ReadSample, because the end frames might not have
      // been written 
      refVopId = 1;
      refVopTime = 0;
      MP4SampleId maxSamples = MP4GetTrackNumberOfSamples(mp4File, trackId);
      // start with sample 2 - we know the first one is a I frame
      mpeg4_frame_t *fr = head->next; // skip the first one
      for (MP4SampleId ix = 2; ix <= maxSamples; ix++) {
	if (fr->vopType != VOP_TYPE_B) {
#ifdef DEBUG_MP4V_TS
            printf("sample %u %u renderingOffset "U64"\n",
		   refVopId, fr->vopType, fr->frameTimestamp - refVopTime);
#endif
	  MP4SetSampleRenderingOffset(mp4File, trackId, refVopId, 
				      fr->frameTimestamp - refVopTime);
	  refVopId = ix;
	  refVopTime = fr->frameTimestamp;
	}
	fr = fr->next;
      }
      
#ifdef DEBUG_MP4V_TS
      printf("sample %u %u renderingOffset "U64"\n",
	     refVopId, fr->vopType, fr->frameTimestamp - refVopTime);
#endif
      MP4SetSampleRenderingOffset(mp4File, trackId, refVopId, 
				  fr->frameTimestamp - refVopTime);
    }

    while (head != NULL) {
      tail = head->next;
      free(head);
      head = tail;
    }
    // terminate session if encrypting
    if (doEncrypt) {
        if (ismacrypEndSession(ismaCrypSId) != 0) {
            fprintf(stderr,
                    "%s: could not end the ISMAcryp session\n",
                    ProgName);
        }
    }

    return trackId;
}

