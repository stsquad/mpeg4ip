#include "mp4av.h"

static uint32_t MP4AV_Ac3FindSyncCode (const uint8_t *buf, 
				       uint32_t buflen)
{
  uint32_t end = buflen - 6;
  uint32_t offset = 0;
  while (offset <= end) {
    if (buf[offset] == 0x0b && buf[offset + 1] == 0x77) {
      return offset;
    }
    offset++;
  }
  return buflen;
}

static const uint32_t frmsizecod_to_bitrate[] = {
  32000, 40000, 48000, 56000, 64000, 80000, 96000,
  112000, 128000, 160000, 192000, 224000, 256000,
  320000, 384000, 448000, 512000, 576000, 640000
};

static const uint32_t frmsizecod2_to_framesize[] = {
  96, 120, 144, 168, 192, 240, 288, 336, 384, 480, 576, 672,
  768, 960, 1152, 1344, 1536, 1728, 1920
};

static const uint32_t frmsizecod1_to_framesize[] = {
  69, 87, 104, 121, 139, 174, 208, 243, 278, 348, 417, 487,
  557, 696, 835, 975, 1114, 1253, 1393
};
static const uint32_t frmsizecod0_to_framesize[] = {
  64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 
  512, 640, 768, 896, 1024, 1152, 1280
};

static const uint32_t acmod_to_chans[] = {
  2, 1, 2, 3, 3, 4, 4, 5
};

extern "C" int MP4AV_Ac3ParseHeader (const uint8_t *buf, 
			  uint32_t buflen,
			  const uint8_t **ppFrame,
			  uint32_t *pbitrate,
			  uint32_t *pfreq,
			  uint32_t *pframesize,
			  uint32_t *pchans)
{
  uint32_t offset;
  uint32_t fscod, frmsizecod, bsid;
  uint32_t acmod;
  if (buflen < 6) return 0;
  offset = MP4AV_Ac3FindSyncCode(buf, buflen);
  if (offset >= buflen) return 0;

  buf += offset;
  if (ppFrame != NULL)
    *ppFrame = buf;

  fscod = (buf[4] >> 6) & 0x3;
  frmsizecod = (buf[4] & 0x3f);
  bsid = (buf[5] >> 3) & 0x1f;
  acmod = (buf[6] >> 5) & 0x7;

#if 0
  printf("fscod %x frmsizecod %x bsid %x acmod %x\n", 
	 fscod, frmsizecod, bsid, acmod);
#endif
  if (bsid >= 12) return -1;

  if (pbitrate != NULL) {
    *pbitrate = frmsizecod_to_bitrate[frmsizecod / 2];
    if (bsid > 8) {
      *pbitrate = *pbitrate >> (bsid - 8);
    }
  }
  uint32_t freq;
  uint32_t framesize;
  switch (fscod) {
  case 0:
    freq = 48000;
    framesize = frmsizecod0_to_framesize[frmsizecod / 2] * 2;
    break;
  case 1:
    freq = 44100;
    framesize = (frmsizecod1_to_framesize[frmsizecod / 2] +
		  (frmsizecod & 0x1)) * 2;
    break;
  case 2:
    freq = 32000;
    framesize = frmsizecod2_to_framesize[frmsizecod / 2] * 2;
    break;
  default:
    return -1;
  }
  if (pfreq != NULL) *pfreq = freq;
  if (pframesize != NULL) *pframesize = framesize;
  if (pchans != NULL) {
    *pchans = acmod_to_chans[acmod];
    uint16_t maskbit = 0x100;
    if ((acmod & 0x1) && acmod != 1) maskbit >>= 2;
    if (acmod & 0x4) maskbit >>= 2;
    if (acmod == 0x2) maskbit += 2;
    uint16_t b67 = (buf[6] << 8) | buf[7];
    if ((b67 & maskbit) != 0) *pchans += 1;
  }
  return 1;
}
