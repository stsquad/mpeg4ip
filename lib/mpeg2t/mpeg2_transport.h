


#ifndef __MPEG2_TRANSPORT_H__
#define __MPEG2_TRANSPORT_H__

#define MPEG2T_SYNC_BYTE 0x47

typedef struct mpeg2t_frame_t {
  struct mpeg2t_frame_t *next_frame;
  int have_ps_ts;
  uint64_t ps_ts;
  uint8_t *frame;
  uint32_t frame_len;
} mpeg2t_frame_t;

typedef enum mpeg2t_pak_type {
  MPEG2T_PAS_PAK,
  MPEG2T_PROG_MAP_PAK,
  MPEG2T_ES_PAK,
} mpeg2t_pak_type;

typedef struct mpeg2t_pid_t {
  struct mpeg2t_pid_t *next_pid;
  uint32_t lastcc;
  uint32_t data_len;
  uint16_t pid;
  uint8_t *data;
  uint32_t data_len_loaded;
  uint32_t data_len_max;
  int collect_pes;
  mpeg2t_pak_type pak_type;
} mpeg2t_pid_t;

  
typedef struct mpeg2t_pas_t {
  mpeg2t_pid_t pid;
  uint16_t transport_stream_id;
  uint8_t version_number;
  int current_next_indicator;
} mpeg2t_pas_t;

typedef struct mpeg2t_pmap_t {
  mpeg2t_pid_t pid;
  uint16_t program_number;
  int received;
  uint8_t version_number;
  uint8_t *prog_info;
  uint32_t prog_info_len;
} mpeg2t_pmap_t;

typedef struct mpeg2t_es_t {
  mpeg2t_pid_t pid;
  uint8_t stream_type;
  uint32_t es_info_len;
  uint8_t *es_data;
  uint8_t stream_id;
  mpeg2t_frame_t *work;
  uint32_t work_max_size;
  int work_state;
  uint32_t work_loaded;
  mpeg2t_frame_t *list;
  char left_buff[6];
  int left;
  int have_ps_ts;
  uint64_t ps_ts;
  uint32_t header;
  int have_seq_header;
  uint32_t seq_header_offset;
  uint32_t pict_header_offset;
} mpeg2t_es_t;

typedef struct mpeg2t_t {
  mpeg2t_pas_t pas;
} mpeg2t_t;

#ifdef __cplusplus 
extern "C" {
#endif
uint32_t mpeg2t_find_sync_byte(const uint8_t *buffer, uint32_t buflen);
uint32_t mpeg2t_transport_error_indicator(const uint8_t *pHdr);
uint32_t mpeg2t_payload_unit_start_indicator(const uint8_t *pHdr);
uint16_t mpeg2t_pid(const uint8_t *pHdr);
uint32_t mpeg2t_adaptation_control(const uint8_t *pHdr);
uint32_t mpeg2t_continuity_counter(const uint8_t *pHdr);

const uint8_t *mpeg2t_transport_payload_start(const uint8_t *pHdr, 
					      uint32_t *payload_len);

mpeg2t_t *create_mpeg2_transport(void);
mpeg2t_es_t *mpeg2t_process_buffer(mpeg2t_t *ptr, 
				   const uint8_t *buffer, 
				   uint32_t buflen,
				   uint32_t *buflen_used);

  void mpeg2t_set_loglevel(int loglevel);

  void mpeg2t_set_error_func(error_msg_func_t func);
#ifdef __cplusplus
}
#endif
#endif
