
#ifndef __AUDIO_CONVERT_H__
#define __AUDIO_CONVERT_H__
#include "mpeg4ip.h"
#ifdef __cplusplus
extern "C" {
#endif

  void audio_convert_format(void *to,
			    const void *from,
			    uint32_t samples,
			    audio_format_t from_format,
			    uint32_t m_to_channels,
			    uint32_t m_from_channels);

#ifdef __cplusplus
}
#endif
#endif
