#include "mp4av.h"

extern "C" uint32_t MP4AV_AmrGetSamplingWindow (uint32_t freq)
{
  return freq == 16000 ? 320 : 160;
}

extern "C" bool MP4AV_AmrGetNextFrame (const uint8_t *buf, 
				       uint32_t buflen,
				       uint32_t *frame_len,
				       bool have_amr_nb)
{

    static const short blockSize[16]   = { 12, 13, 15, 17, 19, 20, 26, 31,  5, -1, -1, -1, -1, -1, -1, 0}; // mode 15 is NODATA
    static const short blockSizeWB[16] = { 17, 23, 32, 36, 40, 46, 50, 58, 60, 5, 5, -1, -1, -1, 0, 0 };
    const short* pBlockSize;
    u_int8_t decMode;

    decMode = (*buf >> 3) & 0x000F;

    if (have_amr_nb) {
        pBlockSize = blockSize;
    } else {
        pBlockSize = blockSizeWB;
    }
    // Check whether we have a legal mode
    if (pBlockSize[decMode] == -1) {
      return false;
    }

    *frame_len = pBlockSize[decMode] + 1;
    if (buflen < *frame_len) 
      return false;
    return true;
}

