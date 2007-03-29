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
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * mpeg2t_thread.c
 */
#include "mpeg2t_private.h"
#include "player_session.h"
#include "player_media.h"
#include "player_util.h"
#include "our_config_file.h"
#include "media_utils.h"
#include "codec_plugin_private.h"
#include "mpeg2f_bytestream.h"
#include "mpeg2t_file.h"
#include "mp4av.h"

//#define DEBUG_MPEG2F_SEARCH 1
#ifdef _WIN32
DEFINE_MESSAGE_MACRO(mpeg2f_message, "mpeg2f")
#else
#define mpeg2f_message(loglevel, fmt...) message(loglevel, "mpeg2f", fmt)
#endif

int create_media_for_mpeg2t_file (CPlayerSession *psptr, 
				  const char *name,
				  int have_audio_driver, 
				  control_callback_vft_t *cc_vft)
{
  FILE *ifile;
  uint8_t buffer[188];

  ifile = fopen(name, FOPEN_READ_BINARY);
  if (ifile == NULL) {
    psptr->set_message("Couldn't open file %s", name);
    return -1;
  }

  if (fread(buffer, 1, 1, ifile) <= 0) {
    psptr->set_message("Couldn't read file %s", name);
    fclose(ifile);
    return -1;
  }
  if (buffer[0] != MPEG2T_SYNC_BYTE) {
    psptr->set_message("File is not mpeg2 transport %s", name);
    fclose(ifile);
    return -1;
  }
  CMpeg2tFile *tfile = new CMpeg2tFile(name, ifile);

  int ret;
  psptr->session_set_seekable(0);
  ret = tfile->create(psptr);
  if (ret < 0) {
    delete tfile;
    return ret;
  }
  ret = tfile->create_media(psptr, cc_vft);

  if (ret < 0) {
    player_error_message("mpeg2t file found");
    delete tfile;
  }
  psptr->set_session_desc(0, name);
  return ret;
}

/*
 * Close routine - clean up the file when we're done
 */
static void close_mpeg2t_file (void *data)
{
  CMpeg2tFile *f = (CMpeg2tFile *)data;
  delete f;
}

CMpeg2tFile::CMpeg2tFile (const char *name, FILE *ifile)
{
  m_ifile = ifile;
  m_buffer_size = 0;
  m_buffer_on = 0;
  m_file_mutex = SDL_CreateMutex();
  m_buffer = NULL;
  m_mpeg2t = NULL;
}

CMpeg2tFile::~CMpeg2tFile (void)
{
  fclose(m_ifile);
  CHECK_AND_FREE(m_buffer);
  if (m_mpeg2t != NULL) 
    delete_mpeg2t_transport(m_mpeg2t);
  SDL_DestroyMutex(m_file_mutex);
  m_file_mutex = NULL;
}

/*
 * Create - will determine pids and psts ranges in file.  Will also
 * loop through the file and determine CFilePosRec points at percentages
 */
int CMpeg2tFile::create (CPlayerSession *psptr)
{
  m_mpeg2t = create_mpeg2_transport();
  if (m_mpeg2t == NULL) {
    psptr->set_message("Couldn't create mpeg2 transport");
    fclose(m_ifile);
    return -1;
  }
  // nice, large buffers to process
  m_buffer_size_max = 188 * 2000;
  m_buffer = (uint8_t *)malloc(m_buffer_size_max);

  if (m_buffer == NULL) {
    psptr->set_message("Malloc error");
    return -1;
  }
  m_buffer[0] = MPEG2T_SYNC_BYTE;
  m_buffer_size = fread(&m_buffer[1], 1, m_buffer_size_max - 1, m_ifile) + 1;

  bool done = false;
  mpeg2t_pid_t *pidptr;
  uint32_t buflen_used;
  bool have_psts = false;
  uint64_t earliest_psts = 0;
  mpeg2t_es_t *es_pid;

  int olddebuglevel;
  olddebuglevel = config.get_config_value(CONFIG_MPEG2T_DEBUG);
  if (olddebuglevel != LOG_DEBUG)
    mpeg2t_set_loglevel(LOG_CRIT);
  m_mpeg2t->save_frames_at_start = 1;
  /*
   * We need to determine which PIDs are present, and try to establish
   * a starting psts.  We also want to establish what type of video and
   * audio are in the mix.  Note: if we try to run this on a file that
   * we don't understand the video, this could take a while, because the
   * info never gets loaded.
   */
  do {
    m_buffer_on = 0;
    while (m_buffer_on + 188 < m_buffer_size && done == false) {
      
      pidptr = mpeg2t_process_buffer(m_mpeg2t, 
				     &m_buffer[m_buffer_on],
				     m_buffer_size - m_buffer_on,
				     &buflen_used);
      m_buffer_on += buflen_used;
      if (pidptr != NULL && pidptr->pak_type == MPEG2T_ES_PAK) {
	es_pid = (mpeg2t_es_t *)pidptr;
	mpeg2t_frame_t *fptr;

	// determine earliest PS_TS
	while ((fptr = mpeg2t_get_es_list_head(es_pid)) != NULL) {
	  if (fptr->have_ps_ts != 0 || fptr->have_dts != 0) {
	    uint64_t ps_ts = 0;
	    bool store_psts = true;
	    if (fptr->have_dts != 0) {
	      ps_ts = fptr->dts;
	    } else {
	      if (es_pid->is_video == 1) { // mpeg2
		// video - make sure we get the first I frame, then we can
		// get the real timestamp
		if (fptr->frame_type != 1) {
		  store_psts = false;
		} else {
		  ps_ts = fptr->ps_ts;
		  uint16_t temp_ref = MP4AV_Mpeg3PictHdrTempRef(fptr->frame + fptr->pict_header_offset);
		  ps_ts -= ((temp_ref + 1) * es_pid->tick_per_frame);
		}
	      } else {
		ps_ts = fptr->ps_ts;
	      }
	    }
	    if (store_psts) {
	      // when we have the first psts for a ES_PID, turn off
	      // parsing frames for that PID.
	      mpeg2t_set_frame_status(es_pid, MPEG2T_PID_NOTHING);
	      if (have_psts) {
		earliest_psts = MIN(earliest_psts, ps_ts);
	      } else {
		earliest_psts = ps_ts;
		have_psts = true;
	      }
	    }
	  }
	  mpeg2t_free_frame(fptr);
	}

	// Each time, search through and see if there are any ES_PIDs 
	// that have not returned a psts.  We're done when the info is
	// loaded for all the es pids.
	pidptr = m_mpeg2t->pas.pid.next_pid;
	bool finished = true;
	while (pidptr != NULL && finished) {
	  if (pidptr->pak_type == MPEG2T_ES_PAK) {
	    es_pid = (mpeg2t_es_t *)pidptr;
	    if (es_pid->info_loaded == 0) {
	      finished = false;
	    }
	  }
	  pidptr = pidptr->next_pid;
	}
	done = finished || have_psts;
      }
    }
    if (done == false) {
      m_buffer_size = fread(m_buffer, 1, m_buffer_size_max, m_ifile);
    }
  } while (m_buffer_size >=188 && done == false);

  if (done == false) {
    psptr->set_message("Could not find information in TS");
    mpeg2t_set_loglevel(olddebuglevel);
    return -1;
  }

#ifdef DEBUG_MPEG2F_SEARCH
  mpeg2f_message(LOG_DEBUG, "initial psts is "U64, earliest_psts);
#endif
  m_start_psts = earliest_psts;

  // Now, we'll try to build a rough index for the file
  // enable psts reading for the pid
  for (pidptr = m_mpeg2t->pas.pid.next_pid; pidptr != NULL; pidptr = pidptr->next_pid) {
    if (pidptr->pak_type == MPEG2T_ES_PAK) {
      es_pid = (mpeg2t_es_t *)pidptr;
      mpeg2t_set_frame_status(es_pid, MPEG2T_PID_REPORT_PSTS);
    }
  }
  m_file_record.record_point(0, earliest_psts, 0);
  fpos_t fpos;
  uint64_t end;
  uint64_t perc, cur;

  // find out the length of the file.
  struct stat filestat;
  if (fstat(fileno(m_ifile), &filestat) != 0) {
    return -1;
  }
  end = filestat.st_size;
  perc = end;
  // perc is what size of the file to skip through to get a rough
  // timetable.  We want to do 10% chunks, or 100Mb chunks, whichever is
  // less.
  while (perc > TO_U64(100000000)) {
    perc /= 2;
  }
  if (perc > (end / TO_U64(10))) {
    perc = end / TO_U64(10);
  }
  if (perc < (end / TO_U64(50))) {
    perc = end / TO_U64(50);
  }
#ifdef DEBUG_MPEG2F_SEARCH
  mpeg2f_message(LOG_DEBUG, "perc is "U64" "U64, perc, (perc * TO_U64(100)) / end );
#endif

  cur = perc;

  bool is_seekable = true;
  uint64_t last_psts, ts;
  last_psts = earliest_psts;
  uint64_t check_end;
  if (end < m_buffer_size_max * 2) {
    check_end = end / 2;
  } else
    check_end = end - (m_buffer_size_max * 2);
  // Now - skip to the next perc chunk, and try to find the next psts
  // we'll record this info.
  do {
#ifdef DEBUG_MPEG2F_SEARCH
    mpeg2f_message(LOG_DEBUG, "current "U64" end "U64, cur, end);
#endif
    VAR_TO_FPOS(fpos, cur);
    fsetpos(m_ifile, &fpos);
    done = false;
    uint64_t count = 0;
    m_buffer_on = 0;
    m_buffer_size = 0;
    do {
      if (m_buffer_on + 188 > m_buffer_size) {
	if (m_buffer_on < m_buffer_size) {
	  memmove(m_buffer, m_buffer + m_buffer_on, 
		  m_buffer_size - m_buffer_on);
	  m_buffer_on = m_buffer_size - m_buffer_on;
	} else {
	  m_buffer_on = 0;
	}
	m_buffer_size = fread(m_buffer + m_buffer_on, 
			      1, 
			      (188 * 10) - m_buffer_on, 
			      m_ifile);

	count += m_buffer_size - m_buffer_on;
	m_buffer_size += m_buffer_on;
	m_buffer_on = 0;
	if (m_buffer_size < 188) {
	  m_buffer_size = 0;
	  done = true;
	}
      }

      pidptr = mpeg2t_process_buffer(m_mpeg2t,
				     m_buffer + m_buffer_on, 
				     m_buffer_size - m_buffer_on, 
				     &buflen_used);
      m_buffer_on += buflen_used;
      if (pidptr != NULL && pidptr->pak_type == MPEG2T_ES_PAK) {
	es_pid = (mpeg2t_es_t *)pidptr;
	// If we have a psts, record it.
	// If it is less than the previous one, we've got a discontinuity, so
	// we can't seek.
	if (es_pid->have_ps_ts || es_pid->have_dts) {
	  ts = es_pid->have_ps_ts ? es_pid->ps_ts : es_pid->dts;
	  if (ts < last_psts) {
	    player_error_message("pid %x psts "U64" is less than prev record point "U64, 
				 es_pid->pid.pid, ts, last_psts);
	    cur = end;
	    is_seekable = false;
	  } else {
#ifdef DEBUG_MPEG2F_SEARCH
	    mpeg2f_message(LOG_DEBUG, "pid %x psts "U64" %d", 
			       pidptr->pid, ts, 
			       es_pid->is_video);
#endif
	    m_file_record.record_point(cur, ts, 0);
	  }
	  done = true;
	}
      }

    } while (done == false && count < perc / 2);
    cur += perc;

  } while (cur < check_end);

  //mpeg2f_message(LOG_DEBUG, "starting end search");
  // Now, we'll go to close to the end of the file, and look for a 
  // final PSTS.  This gives us a rough estimate of the elapsed time
  long seek_offset;
  seek_offset = 0;
  seek_offset -= (m_buffer_size_max) * 2;
  fseek(m_ifile, seek_offset, SEEK_END);
  m_buffer_on = m_buffer_size = 0;
  uint64_t max_psts;
  max_psts = m_start_psts;
  do {
    while (m_buffer_on + 188 <= m_buffer_size) {
      
      pidptr = mpeg2t_process_buffer(m_mpeg2t, 
				     &m_buffer[m_buffer_on],
				     m_buffer_size - m_buffer_on,
				     &buflen_used);
      m_buffer_on += buflen_used;
      if (pidptr != NULL && pidptr->pak_type == MPEG2T_ES_PAK) {
	es_pid = (mpeg2t_es_t *)pidptr;
	if (es_pid->have_ps_ts) {
	  es_pid->have_ps_ts = 0;
	  max_psts = MAX(es_pid->ps_ts, max_psts);
	} else if (es_pid->have_dts) {
	  es_pid->have_dts = 0;
	  max_psts = MAX(es_pid->dts, max_psts);
	}
      }
    }
    if (m_buffer_size > m_buffer_on) {
      memmove(m_buffer, m_buffer + m_buffer_on, m_buffer_size - m_buffer_on);
    }
    m_buffer_on = m_buffer_size - m_buffer_on;
    m_buffer_size = fread(m_buffer + m_buffer_on, 1, 
			  m_buffer_size_max - m_buffer_on, m_ifile);
    m_buffer_size += m_buffer_on;
    m_buffer_on = 0;
    if (m_buffer_size < 188) m_buffer_size = 0;
  } while (m_buffer_size > 188) ;
  m_last_psts = max_psts;
  // Calculate the rough max time; hopefully it will be greater than the
  // initial...
  m_max_time = max_psts;
  m_max_time -= m_start_psts;
  m_max_time /= 90000.0;
#ifdef DEBUG_MPEG2F_SEARCH
  player_debug_message("last psts is "U64" "U64" %g", max_psts,
		       (max_psts - m_start_psts) / TO_U64(90000),
		       m_max_time);
#endif
  mpeg2t_set_loglevel(olddebuglevel);

  if (is_seekable) {
    psptr->session_set_seekable(1);
  }
  m_ts_seeked_in_msec = UINT64_MAX;
  rewind(m_ifile);

  return 0;
}


int CMpeg2tFile::create_video (CPlayerSession *psptr,
			       mpeg2t_t *decoder,
			       video_query_t *vq,
			       uint video_offset,
			       int &sdesc)
{
  uint ix;
  CPlayerMedia *mptr;
  codec_plugin_t *plugin;
  int created = 0;

  // Loop through the vq structure, and set up a new player media
  for (ix = 0; ix < video_offset; ix++) {
    mpeg2t_pid_t *pidptr;
    mpeg2t_es_t *es_pid;
    pidptr = mpeg2t_lookup_pid(decoder,vq[ix].track_id);
    if (pidptr->pak_type != MPEG2T_ES_PAK) {
      mpeg2f_message(LOG_CRIT, "mpeg2t video type is not es pak - pid %x",
		     vq[ix].track_id);
      exit(1);
    }
    es_pid = (mpeg2t_es_t *)pidptr;
    if (vq[ix].enabled != 0 && created == 0) {
      created = 1;
      mptr = new CPlayerMedia(psptr, VIDEO_SYNC);
      if (mptr == NULL) {
	return (-1);
      }
      video_info_t *vinfo;
      vinfo = MALLOC_STRUCTURE(video_info_t);
      vinfo->height = vq[ix].h;
      vinfo->width = vq[ix].w;
      plugin = check_for_video_codec(STREAM_TYPE_MPEG2_TRANSPORT_STREAM,
				     NULL,
				     NULL,
				     vq[ix].type,
				     vq[ix].profile,
				     vq[ix].config, 
				     vq[ix].config_len,
				     &config);

      int ret = mptr->create_video_plugin(plugin, 
					  STREAM_TYPE_MPEG2_TRANSPORT_STREAM,
					  NULL,
					  vq[ix].type,
					  vq[ix].profile,
					  NULL, // sdp info
					  vinfo, // video info
					  vq[ix].config,
					  vq[ix].config_len);

      if (ret < 0) {
	mpeg2f_message(LOG_ERR, "Failed to create plugin data");
	psptr->set_message("Failed to start plugin");
	delete mptr;
	return -1;
      }

      CMpeg2fVideoByteStream *vbyte;
      vbyte = new CMpeg2fVideoByteStream(this, es_pid);
      if (vbyte == NULL) {
	mpeg2f_message(LOG_CRIT, "failed to create byte stream");
	delete mptr;
	return (-1);
      }
      ret = mptr->create_media("video", vbyte, false);
      if (ret != 0) {
	mpeg2f_message(LOG_CRIT, "failed to create from file");
	return (-1);
      }
      if (es_pid->info_loaded) {
	char buffer[80];
	if (mpeg2t_write_stream_info(es_pid, buffer, 80) >= 0) {
	  psptr->set_session_desc(sdesc, buffer);
	  sdesc++;
	}
      }
      mpeg2t_set_frame_status(es_pid, MPEG2T_PID_SAVE_FRAME);
    }  else {
      mpeg2t_set_frame_status(es_pid, MPEG2T_PID_NOTHING);
    }
  }
  return created;
}

int CMpeg2tFile::create_audio (CPlayerSession *psptr,
			       mpeg2t_t *decoder,
			       audio_query_t *aq,
			       uint audio_offset,
			       int &sdesc)
{
  uint ix;
  CPlayerMedia *mptr;
  codec_plugin_t *plugin;
  int created = 0;

  for (ix = 0; ix < audio_offset; ix++) {
    mpeg2t_pid_t *pidptr;
    mpeg2t_es_t *es_pid;
    pidptr = mpeg2t_lookup_pid(decoder,aq[ix].track_id);
    if (pidptr->pak_type != MPEG2T_ES_PAK) {
      mpeg2f_message(LOG_CRIT, "mpeg2t video type is not es pak - pid %x",
		     aq[ix].track_id);
      exit(1);
    }
    es_pid = (mpeg2t_es_t *)pidptr;
    if (aq[ix].enabled != 0 && created == 0) {
      created = 1;
      mptr = new CPlayerMedia(psptr, AUDIO_SYNC);
      if (mptr == NULL) {
	return (-1);
      }
      audio_info_t *ainfo;
      ainfo = MALLOC_STRUCTURE(audio_info_t);
      ainfo->freq = aq[ix].sampling_freq;
      ainfo->chans = aq[ix].chans;
      ainfo->bitspersample = 0;
      plugin = check_for_audio_codec(STREAM_TYPE_MPEG2_TRANSPORT_STREAM,
				     NULL,
				     NULL,
				     aq[ix].type,
				     aq[ix].profile,
				     aq[ix].config, 
				     aq[ix].config_len,
				     &config);

      int ret = mptr->create_audio_plugin(plugin, 
					  STREAM_TYPE_MPEG2_TRANSPORT_STREAM,
					  NULL,
					  aq[ix].type,
					  aq[ix].profile,
					  NULL, // sdp info
					  ainfo, // video info
					  aq[ix].config,
					  aq[ix].config_len);

      if (ret < 0) {
	mpeg2f_message(LOG_ERR, "Failed to create plugin data");
	psptr->set_message("Failed to start plugin");
	delete mptr;
	return -1;
      }

      CMpeg2fAudioByteStream *abyte;
      abyte = new CMpeg2fAudioByteStream(this, es_pid);
      if (abyte == NULL) {
	mpeg2f_message(LOG_CRIT, "failed to create byte stream");
	delete mptr;
	return (-1);
      }
      ret = mptr->create_media("audio", abyte, false);
      if (ret != 0) {
	mpeg2f_message(LOG_CRIT, "failed to create from file");
	return (-1);
      }
      if (es_pid->info_loaded) {
	char buffer[80];
	if (mpeg2t_write_stream_info(es_pid, buffer, 80) >= 0) {
	  psptr->set_session_desc(sdesc, buffer);
	  sdesc++;
	}
      }
      mpeg2t_set_frame_status(es_pid, MPEG2T_PID_SAVE_FRAME);
    }  else {
      mpeg2t_set_frame_status(es_pid, MPEG2T_PID_NOTHING);
    }
  }
  return created;
}


int CMpeg2tFile::create_media (CPlayerSession *psptr, 
			       control_callback_vft_t *cc_vft)
{
  uint audio_count, video_count;
  video_query_t *vq;
  audio_query_t *aq;
  int ret;
  int sdesc = 1;
  int total_enabled;
  mpeg2t_check_streams(&vq, &aq, m_mpeg2t, audio_count, video_count, 
		       psptr, cc_vft);
  ret = create_video(psptr, m_mpeg2t, vq, video_count, sdesc);
  if (ret < 0) {
    free(aq);
    free(vq);
    return -1;
  }
  total_enabled = ret;

  ret = create_audio(psptr, m_mpeg2t, aq, audio_count, sdesc);
  free(aq);
  free(vq);
  if (ret < 0) {
    return -1;
  }
  total_enabled += ret;

  psptr->set_media_close_callback(close_mpeg2t_file, (void *)this);

  const char *errmsg = psptr->get_message();
  return *errmsg != '\0' ? 1 : 0 ;

}  

/*
 * eof - if we have data in the buffer, we don't have eof.  If we don't, 
 * check if we've hit the eof
 */  
int CMpeg2tFile::eof (void)
{
  int ret;
  if (m_buffer_on + 188 <= m_buffer_size) return 0;

  lock_file_mutex();
  ret = feof(m_ifile);
  unlock_file_mutex();
  return ret;
}

/*
 * get_frame_for_pid - process further into the file, until we get
 * a frame.  This has the effect of perhaps loading the frame for the
 * other streams.
 */
void CMpeg2tFile::get_frame_for_pid (mpeg2t_es_t *es_pid)
{
  mpeg2t_pid_t *pidptr;
  uint32_t buflen_used;

  lock_file_mutex();
  do {
    while (m_buffer_on + 188 < m_buffer_size) {
      pidptr = mpeg2t_process_buffer(m_mpeg2t, 
				     &m_buffer[m_buffer_on],
				     m_buffer_size - m_buffer_on,
				     &buflen_used);
      m_buffer_on += buflen_used;
      if (es_pid->list != NULL) {
	unlock_file_mutex();
	return;
      }
    }
    if (!feof(m_ifile)) {
      if (m_buffer_on < m_buffer_size) {
	memmove(m_buffer, m_buffer + m_buffer_on, m_buffer_size - m_buffer_on);
	m_buffer_on = m_buffer_size - m_buffer_on;
      } else {
	m_buffer_on = 0;
      }
      m_buffer_size = fread(m_buffer + m_buffer_on, 1, 
			    m_buffer_size_max - m_buffer_on, m_ifile);
      m_buffer_size += m_buffer_on;
      m_buffer_on = 0;
      if (m_buffer_size < 188) m_buffer_size = 0;
    }
  } while (!feof(m_ifile));

  unlock_file_mutex();
}

/*
 * seek_to - get close to where they want to go
 */
void CMpeg2tFile::seek_to (uint64_t ts_in_msec)
{
  lock_file_mutex();
  clearerr(m_ifile);
  // If we've already seeked, indicate that we haven't (this will not
  // work for 3 streams, but I'm lazy now
  if (m_ts_seeked_in_msec == ts_in_msec) {
    m_ts_seeked_in_msec = UINT64_MAX;
    unlock_file_mutex();
    return;
  }

  // clear the buffer on and buffer size - this will force a new
  // read
  m_buffer_on = m_buffer_size = 0;

  // If they are looking for < 1 second, just rewind
  m_ts_seeked_in_msec = ts_in_msec;
  if (ts_in_msec < TO_U64(1000)) {
    rewind(m_ifile);
    unlock_file_mutex();
    return;
  }
  
  uint64_t pts_seeked;
  // come 1 second or so earlier - this is so we don't need to track
  // pts vs dts, but can just get close
  ts_in_msec -= TO_U64(1000); 
  pts_seeked = ts_in_msec * TO_U64(90);
  pts_seeked += m_start_psts;

  const frame_file_pos_t *start_pos;

  start_pos = m_file_record.find_closest_point(pts_seeked);
  
  if (start_pos == NULL) {
    rewind(m_ifile);
    unlock_file_mutex();
    return;
  }
#ifdef DEBUG_MPEG2F_SEARCH
  mpeg2f_message(LOG_DEBUG, "Looking for pts "U64" found "U64, 
		       pts_seeked, start_pos->timestamp);
#endif

  fpos_t fpos;
  VAR_TO_FPOS(fpos, start_pos->file_position);
  fsetpos(m_ifile, &fpos);
  // Now, if we wanted, we could start reading frames, and read until 
  // we got an i frame close to where we wanted to be.  I'm lazy now, so
  // I won't
  unlock_file_mutex();
}
