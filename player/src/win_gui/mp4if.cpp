
#include "stdafx.h"
#include "mp4if.h"
#include "our_config_file.h"

CMP4If::CMP4If (CString &name)
{
	m_process = new CMP4Process(name);
	m_audio_volume = config.get_config_value(CONFIG_VOLUME);
	m_state = MP4IF_STATE_PLAY;
	m_fullscreen_state = FALSE;
	m_screen_size = 1;
}

CMP4If::~CMP4If (void)
{
	delete m_process;
}

int CMP4If::get_initial_response (CString &errmsg)
{
	int retval;
	msg_initial_resp_t msg;

	retval = m_process->get_initial_response(&msg, errmsg);
	if (retval >= 0) {
		m_is_seekable = msg.session_is_seekable;
		m_has_audio = msg.has_audio;
		m_max_time = msg.max_time;
	}
	return retval;
}

int CMP4If::set_audio_volume (int val)
{
	int ret;
	if (m_process == NULL) {
		return -1;
	}

	ret = client_read_config();

	if (ret == 0) {
		m_audio_volume = val;
	} else {
		return ret;
	}
	return 0;
}

int CMP4If::get_mute (void) 
{ 
	return config.GetBoolValue(CONFIG_MUTED); 
}

int CMP4If::toggle_mute (void)
{
	int ret;
	if (m_process == NULL) {
		return -1;
	}

	config.SetBoolValue(CONFIG_MUTED,
			    !config.GetBoolValue(CONFIG_MUTED));

	ret = client_read_config();

	return ret;
}

int CMP4If::set_screen_size (int val)
{
	int ret;
	uint64_t retvalue;
	if (m_process == NULL) {
		return -1;
	}

	ret = m_process->send_message(CLIENT_MSG_SET_SCREEN_SIZE,
								  &retvalue,
								  (const char *)&val,
								  sizeof(int));
	if (ret == 0) {
		m_screen_size = val;
	} else {
		return ret;
	}
	return 0;
}

int CMP4If::play (void)
{
	int ret;
	uint64_t retvalue;
	client_msg_play_t cmp;

	if (m_process == NULL) {
		return -1;
	}
	if (m_state == MP4IF_STATE_PAUSE)
		cmp.start_from_begin = FALSE;
	else 
		cmp.start_from_begin = TRUE;
	cmp.start_time = 0.0;
	
	ret = m_process->send_message(CLIENT_MSG_PLAY,
								  &retvalue,
								  (const char *)&cmp, 
								  sizeof(cmp));
	m_state = MP4IF_STATE_PLAY;
	return ret;
}

int CMP4If::pause (void)
{
	int ret;
	uint64_t retvalue;
	if (m_process == NULL) {
		return -1;
	}
	if (m_state == MP4IF_STATE_PLAY) {
		ret = m_process->send_message(CLIENT_MSG_PAUSE,
			&retvalue);
		m_state = MP4IF_STATE_PAUSE;
	}
	return ret;
}

int CMP4If::stop (void)
{
	int ret = pause();
	m_state = MP4IF_STATE_STOP;
	return ret;
}

int CMP4If::seek_to (double time)
{
	if (m_state == MP4IF_STATE_PLAY) {
		pause();
	}

	int ret;
	uint64_t retvalue;
	if (m_process == NULL) {
		return -1;
	}

	client_msg_play_t cmp;
	cmp.start_from_begin = time == 0.0 ? TRUE : FALSE;
	cmp.start_time = time;
	ret = m_process->send_message(CLIENT_MSG_PLAY,
								  &retvalue,
								  (const char *)&cmp, 
								  sizeof(cmp));
	m_state = MP4IF_STATE_PLAY;
	return ret;
}

int CMP4If::client_read_config (void)
{
	config.WriteVariablesToRegistry("Software\\Mpeg4ip", "Config");
	return m_process->send_message(CLIENT_READ_CONFIG_FILE, NULL);
}

bool CMP4If::get_current_time (uint64_t *time)
{
	return m_process->send_message(CLIENT_MSG_GET_CURRENT_TIME, time);
}
