#ifndef __MP4AV_AC3_H__
#define __MP4AV_AC3_H__ 1

#define AC3_SYNC_WORD 0x0b77

#ifdef __cplusplus
extern "C" {
#endif
int MP4AV_Ac3ParseHeader(const uint8_t *buf, 
			 uint32_t buflen,
			 const uint8_t **ppFrame,
			 uint32_t *bitrate,
			 uint32_t *freq,
			 uint32_t *framesize,
			 uint32_t *chans);

#ifdef __cplusplus
}
#endif

#endif
