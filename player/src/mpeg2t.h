
#ifndef __MPEG2T_H__
#define __MPEG2T_H__ 1


int create_mpeg2t_session(CPlayerSession *psptr,
			  const char *name, 
			  char *errmsg, 
			  uint32_t errlen, 
			  //int have_audio_driver,
			  control_callback_vft_t *cc_vft);
#endif
