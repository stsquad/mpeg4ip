
#include "mpeg4ip.h"
#include "wmp4client.h"
#include "client_process.h"
#include <rtsp/rtsp_client.h>
#include "player_session.h"
#include "player_media.h"
#include "player_util.h"
#include "our_msg_queue.h"
#include "ip_port.h"
#include "media_utils.h"
#include "playlist.h"
#include "our_config_file.h"
#include <rtp/debug.h>
#include <libhttp/http.h>
#include "codec_plugin_private.h"

static int session_paused;
static int screen_size = 2;
static int fullscreen = 0;
static int volume;
CPlayerSession *psptr;

void set_configs (void)
{
	config.ReadVariablesFromRegistry("Software\\Mpeg4ip", "Config");
	rtsp_set_loglevel(config.get_config_value(CONFIG_RTSP_DEBUG));
	rtp_set_loglevel(config.get_config_value(CONFIG_RTP_DEBUG));
	sdp_set_loglevel(config.get_config_value(CONFIG_SDP_DEBUG));
	http_set_loglevel(config.get_config_value(CONFIG_HTTP_DEBUG));
	volume = config.get_config_value(CONFIG_VOLUME);
	if (config.get_config_value(CONFIG_MUTED) != 0)
		volume = 0;
	if (psptr != NULL) {
		psptr->set_audio_volume(volume);
	}
}
	

int main (int argc, char **argv)
{
	CClientProcess proc;
	SDL_sem *master_sem;
	CMsgQueue master_queue;
	char errmsg[512];
	config.InitializeIndexes();
	config.ReadVariablesFromRegistry("Software\\Mpeg4ip", "Config");

	open_output("wmp4player.log");
	if (proc.enable_communication() < 0) {
		printf("Could not enable communications\n");
		Sleep(10 * 1000);
		close_output();
		return -1;
	}
	initialize_plugins();
	argv++;
	argc--;

	psptr = NULL;
	master_sem = NULL;
	if (*argv == NULL) {
		proc.initial_response(-1, psptr,"No argument to process");
	} else {
		rtsp_set_error_func(player_library_message);
		rtp_set_error_msg_func(player_library_message);
		sdp_set_error_func(player_library_message);
		http_set_error_func(player_library_message);
	
		master_sem = SDL_CreateSemaphore(0);
		psptr = new CPlayerSession(&master_queue, master_sem,
								   *argv);
		int ret = -1;
		errmsg[0] = '\0';
		if (psptr != NULL) {
			ret = parse_name_for_session(psptr, *argv, errmsg, sizeof(errmsg), NULL);
			if (ret < 0) {
				//player_debug_message("%s %s", errmsg, name);
				delete psptr;
				psptr = NULL;
			} else {
				
				if (ret > 0) {
					player_debug_message(errmsg);
				}
				
				psptr->set_up_sync_thread();
				psptr->set_screen_location(100, 100);
				
				psptr->set_screen_size(screen_size);
				psptr->set_audio_volume(config.get_config_value(CONFIG_VOLUME));
				if (psptr->play_all_media(TRUE) != 0) {
					snprintf(errmsg, sizeof(*errmsg), "Couldn't start playing");
					delete psptr;
					psptr = NULL;
				}
			}
		}
		proc.initial_response(ret, psptr, errmsg);
	}
	// Either way - we need to loop and wait until we get the
	// terminate message
	int running = 0;
	int state = 0;
	while (running == 0) {
		CMsg *msg;
		running = proc.receive_messages(psptr);
		while ((msg = master_queue.get_message()) != NULL) {
			switch (msg->get_value()) {
			case MSG_SESSION_FINISHED:
				proc.send_message(GUI_MSG_SESSION_FINISHED);
				break;
			case MSG_RECEIVED_QUIT:
				proc.send_message(GUI_MSG_RECEIVED_QUIT);
				break;
			case MSG_SDL_KEY_EVENT:
				uint32_t len;
				void *sdl_event = (void *)msg->get_message(len);
				proc.send_message(GUI_MSG_SDL_KEY,
								  sdl_event,
								  len);
				break;
			}
			delete msg;
		}
		if (psptr != NULL) state = psptr->sync_thread(state);
	}
	if (psptr != NULL) {
		delete psptr;
	}
	if (master_sem != NULL) {
		SDL_DestroySemaphore(master_sem);
	}
	  // remove invalid global ports
	
	close_plugins();
	config.WriteVariablesToRegistry("Software\\Mpeg4ip", "Config");
	close_output();
	return 0;
}

int getIpAddressFromInterface (const char *ifname,
			       struct in_addr *retval)
{
  int fd;
  int ret = -1;
#ifndef _WIN32
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd > 0) {
    struct ifreq ifr;
    strcpy(ifr.ifr_name, ifname);
    ifr.ifr_addr.sa_family = AF_INET;
    if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
      *retval = ((struct sockaddr_in *)(&ifr.ifr_addr))->sin_addr;
      ret = 0;
    } 
    closesocket(fd);
  }
#endif
  return ret;
}