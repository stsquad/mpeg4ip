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
 * Copyright (C) Cisco Systems Inc. 2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May (wmay@cisco.com)
 */

#include "mpeg4ip.h"
#include "mp4av.h"

static double mpeg3_frame_rate_table[16] =
{
  0.0,   /* Pad */
  24000.0/1001.0,       /* Official frame rates */
  24.0,
  25.0,
  30000.0/1001.0,
  30.0,
  50.0,
  ((60.0*1000.0)/1001.0),
  60.0,

  1,                    /* Unofficial economy rates */
  5, 
  10,
  12,
  15,
  0,
  0,
};

#define MPEG3_SEQUENCE_START_CODE        0x000001b3
#define MPEG3_PICTURE_START_CODE         0x00000100
#define MPEG3_GOP_START_CODE             0x000001b8

extern "C" int MP4AV_Mpeg3ParseSeqHdr (uint8_t *pbuffer,
				       uint32_t buflen,
				       uint32_t *height,
				       uint32_t *width,
				       double *frame_rate)
{
  uint32_t framerate_code;
#if 1
  uint32_t value, ix;
  buflen -= 6;
  for (ix = 0; ix < buflen; ix++, pbuffer++) {
    value = (pbuffer[0] << 24) | (pbuffer[1] << 16) | (pbuffer[2] << 8) | 
      pbuffer[3];

    if (value == MPEG3_SEQUENCE_START_CODE) {
      pbuffer += sizeof(uint32_t);
      *height = (pbuffer[0] << 4) | ((pbuffer[1] >> 4) &0xf);
      *width = ((pbuffer[1] & 0xf) << 4) | pbuffer[2];
      framerate_code = pbuffer[3] & 0xf;
      *frame_rate = mpeg3_frame_rate_table[framerate_code];
      return 0;
    }
  }
  return -1;

#else
  // if you want to do the whole frame
  int ix;
  CBitstream bs(pbuffer, buflen);

  
  try {

    while (bs.PeekBits(32) != MPEG3_SEQUENCE_START_CODE) {
      bs.GetBits(8);
    }

    bs.GetBits(32); // start code

    *height = bs.GetBits(12);
    *width = bs.GetBits(12);
    bs.GetBits(4);
    framerate_code = bs.GetBits(4);
    *frame_rate = mpeg3_frame_rate_table[framerate_code];

    bs.GetBits(18); // bitrate
    bs.GetBits(1);  // marker bit
    bs.GetBits(10); // vbv buffer
    bs.GetBits(1); // constrained params
    if (bs.GetBits(1)) {  // intra_quantizer_matrix
      for (ix = 0; ix < 64; ix++) {
	bs.GetBits(8);
      }
    }
    if (bs.GetBits(1)) { // non_intra_quantizer_matrix
      for (ix = 0; ix < 64; ix++) {
	bs.GetBits(8);
      }
    }
  } catch (...) {
    return false;
  }
  return true;
#endif
}

extern "C" int MP4AV_Mpeg3PictHdrType (uint8_t *pbuffer)
{
  pbuffer += sizeof(uint32_t);
  return ((pbuffer[1] >> 3) & 0x7);
}

extern "C" int MP4AV_Mpeg3FindGopOrPictHdr (uint8_t *pbuffer,
					    uint32_t buflen)
{
  uint32_t value;
  uint32_t offset;

  for (offset = 0; offset < buflen; offset++, pbuffer++) {
    value = (pbuffer[0] << 24) | (pbuffer[1] << 16) | (pbuffer[2] << 8) | 
      pbuffer[3];

    if (value == MPEG3_PICTURE_START_CODE) {
      if (MP4AV_Mpeg3PictHdrType (pbuffer) == 1) {
	return 0;
      } else {
	return -1;
      }
    } else if (value == MPEG3_GOP_START_CODE) {
      return 1;
    }
  }
  return -1;
}
