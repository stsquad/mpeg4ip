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
 *              Ximpo Group Ltd.                mp4v2@ximpo.com
 */

#include <mp4creator.h>

#define AMR_HEADER_LENGTH 6
#define AMR_MAGIC_NUMBER "#!AMR\n"
#define AMR_FIXED_TIMESCALE 8000

#define AMRWB_HEADER_LENGTH 9
#define AMRWB_MAGIC_NUMBER "#!AMR-WB\n"
#define AMRWB_FIXED_TIMESCALE 16000

static u_int8_t amrHeader[AMRWB_HEADER_LENGTH];

#define AMR_TYPE_NONE 0
#define AMR_TYPE_AMR 1
#define AMR_TYPE_AMRWB 2
typedef u_int8_t AMR_TYPE;

static AMR_TYPE GetFirstHeader(FILE* inFile)
{
  /* go back to start of file */
  rewind(inFile);

  /* read the header */
  fread(amrHeader, 1, AMR_HEADER_LENGTH, inFile);

  /* Check whether the magic number fits AMR or AMRWB specs... */
  if (memcmp(amrHeader, AMR_MAGIC_NUMBER, AMR_HEADER_LENGTH)) {
      if (!memcmp(amrHeader, AMRWB_MAGIC_NUMBER, AMR_HEADER_LENGTH)) {
          fread(amrHeader + AMR_HEADER_LENGTH, 1, AMRWB_HEADER_LENGTH - AMR_HEADER_LENGTH, inFile);
          if (!memcmp(amrHeader, AMRWB_MAGIC_NUMBER, AMRWB_HEADER_LENGTH)) {
              return AMR_TYPE_AMRWB;
          }
      }
      return AMR_TYPE_NONE;
  }
  return AMR_TYPE_AMR;
}


static bool LoadNextAmrFrame(FILE* inFile, 
                            u_int8_t* sampleBuffer, 
                            u_int32_t* sampleSize, 
                            u_int8_t* outMode, 
                            AMR_TYPE amrType)
{
    short blockSize[16]   = { 12, 13, 15, 17, 19, 20, 26, 31,  5, -1, -1, -1, -1, -1, -1, 0}; // mode 15 is NODATA
    short blockSizeWB[16] = { 17, 23, 32, 36, 40, 46, 50, 58, 60, 5, 5, -1, -1, -1, 0, 0 };
    short* pBlockSize;
    u_int8_t mode;
    u_int8_t decMode;
    bool readModeOnly = false;

    if ((!sampleBuffer) || (!sampleSize)) {
        readModeOnly = true;
    }

    if (feof(inFile)) {
        return false;
    }

    if (fread(&mode, sizeof(u_int8_t), 1, inFile) != 1) {
        return false;
    }

    decMode = (mode >> 3) & 0x000F;

    if (amrType == AMR_TYPE_AMR) {
        pBlockSize = blockSize;
    } else {
        pBlockSize = blockSizeWB;
    }
    // Check whether we have a legal mode
    if (pBlockSize[decMode] == -1) {
      return false;
    }

    if (!readModeOnly) {
        // Check buffer space (including the mode)
        if ( *sampleSize < (u_int32_t)pBlockSize[decMode] + 1) {
            return false;
        }

        sampleBuffer[0] = mode;

        if (pBlockSize[decMode] != 0) {
          if (fread(&sampleBuffer[1], sizeof(u_int8_t), pBlockSize[decMode], inFile) != (u_int32_t)pBlockSize[decMode]) {
            return false;
          }
        }

        *sampleSize = pBlockSize[decMode] + 1;
    } else { // Skip current frame
        fseek(inFile, pBlockSize[decMode], SEEK_CUR);
    }

    if (outMode != NULL)
        *outMode = decMode;

    return true;

}

MP4TrackId AmrCreator(MP4FileHandle mp4File, FILE* inFile, bool doEncrypt)
{
  // collect all the necessary meta information
  u_int16_t modeSet = 0; //0x8100;
  u_int8_t curMode;
  AMR_TYPE amrType;

  u_int8_t sampleBuffer[10 * 1024];
  u_int32_t sampleSize = sizeof(sampleBuffer);
  MP4SampleId sampleId = 1;

  MP4TrackId trackId;

  amrType = GetFirstHeader(inFile);

  if (amrType == AMR_TYPE_NONE) {
      fprintf(stderr,
              "%s: data in file doesn't appear to be valid audio\n",
              ProgName);
      return MP4_INVALID_TRACK_ID;
  }
  // add the new audio track
  trackId = MP4AddAmrAudioTrack(mp4File,
    amrType == AMR_TYPE_AMR ? AMR_FIXED_TIMESCALE : AMRWB_FIXED_TIMESCALE,
    0, // modeSet
    0, // modeChangePeriod
    1, // framesPerSample
    amrType == AMR_TYPE_AMRWB ? true : false); // isAmrWB

  if (trackId == MP4_INVALID_TRACK_ID) {
      fprintf(stderr,
              "%s: can't create audio track\n", ProgName);
      return MP4_INVALID_TRACK_ID;
  }

  if (MP4GetNumberOfTracks(mp4File, MP4_AUDIO_TRACK_TYPE) == 1) {
      MP4SetAudioProfileLevel(mp4File, 0xFE);
  }

  while (LoadNextAmrFrame(inFile, sampleBuffer, &sampleSize, &curMode, amrType)) {
      modeSet |= (1 << curMode);

      if (!MP4WriteSample(mp4File, trackId, sampleBuffer, sampleSize, MP4_INVALID_DURATION)) {
          fprintf(stderr,
                  "%s: can't write audio frame %u\n", ProgName, sampleId);
          MP4DeleteTrack(mp4File, trackId);
          return MP4_INVALID_TRACK_ID;
      }
      sampleId++;
      sampleSize = sizeof(sampleBuffer);
  }

  MP4SetAmrModeSet(mp4File, trackId, modeSet);

  return trackId;
}

