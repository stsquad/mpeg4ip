#ifndef __MPEG2T_PRIVATE_H__
#define __MPEG2T_PRIVATE_H__ 1


// mpeg2t_mp3.c
int process_mpeg2t_mpeg_audio(mpeg2t_es_t *es_pid, const uint8_t *esptr,
			      uint32_t buflen);
// mpeg2_video.c
int process_mpeg2t_mpeg_video(mpeg2t_es_t *es_pid, 
			      const uint8_t *esptr, 
			      uint32_t buflen);

void mpeg2t_malloc_es_work(mpeg2t_es_t *es_pid, uint32_t frame_len);


#endif
