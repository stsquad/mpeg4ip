
#ifndef __MPEG2_PS_H__
#define __MPEG2_PS_H__ 1
#include "mpeg4ip.h"

#define MPEG2_PS_START      0x00000100
#define MPEG2_PS_START_MASK 0xffffff00
#define MPEG2_PS_PACKSTART  0x000001BA

#define MPEG2_PS_SYSSTART   0x000001BB

#define MPEG2_PS_END        0x000001B9

typedef struct mpeg2_ps_t {
  int m_fd;
  uint8_t *m_buffer;
  uint32_t m_buffer_size_max;
  uint32_t m_buffer_size;
  uint32_t m_buffer_on;
} mpeg2_ps_t;

typedef struct mpeg2_pes_t {
  uint8_t stream_id;
  uint16_t pes_packet_len;
  uint16_t pes_header_len;
  int have_pes_header;
} mpeg2_pes_t;

#ifdef __cplusplus 
extern "C" {
#endif
mpeg2_ps_t *init_mpeg2_ps(const char *filename);
void destroy_mpeg2_ps(mpeg2_ps_t *ifp);

bool read_mpeg2_pack(mpeg2_ps_t *ifp);
#ifdef __cplusplus
}
#endif
#endif
