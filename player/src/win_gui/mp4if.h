#include "mpeg4ip.h"
#include "mp4process.h"

class CMP4If {
public:
	CMP4If(CString &name);
	~CMP4If(void);
	bool is_valid(void) { return m_process && m_process->is_thread_created(); };

	int get_initial_response (CString &errmsg);

	int is_seekable (void) { return m_is_seekable; };
	double get_max_time (void) { return m_max_time; };
	int has_audio (void) { return m_has_audio; };

	int get_audio_volume (void) { return m_audio_volume; };
	int set_audio_volume (int val);
	
	int get_mute (void);
	int toggle_mute(void);
	
	int get_screen_size (void) { return m_screen_size; };
	int set_screen_size(int val);
	void set_fullscreen_state (bool fullscreen_state) {
		m_fullscreen_state = fullscreen_state;
	};
	bool get_fullscreen_state (void) { return m_fullscreen_state; };
	int client_read_config(void);
	int play(void);
	int pause(void);
	int stop(void);
	int seek_to(double time);

	int get_state (void) { return m_state; };
	bool get_current_time (uint64_t *time);
private:
	CMP4Process *m_process;
	int m_has_audio;
	int m_is_seekable;
	int m_audio_volume;
	int m_is_mute;
	int m_screen_size;
	double m_max_time;
	int m_state;
	bool m_fullscreen_state;
};
#define MP4IF_STATE_PLAY 0
#define MP4IF_STATE_PAUSE 1
#define MP4IF_STATE_STOP 2