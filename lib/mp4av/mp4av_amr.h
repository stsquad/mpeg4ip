
#ifndef __MP4AV_AMR_H__
#define __MP4AV_AMR_H__ 1

#ifdef __cplusplus
extern "C" {
#endif

  uint32_t MP4AV_AmrGetSamplingWindow(uint32_t freq);

  bool MP4AV_AmrGetNextFrame(const uint8_t *buf,
			     uint32_t buflen,
			     uint32_t *frame_len,
			     bool have_amr_nb);
#ifdef __cplusplus
}
#endif
#endif
