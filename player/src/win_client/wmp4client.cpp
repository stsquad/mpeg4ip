
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
char *convert_number (char *transport, uint32_t *value)
{
  *value = 0;
  while (isdigit(*transport)) {
    *value *= 10;
    *value += *transport - '0';
    transport++;
  }
  return (transport);
}

char *convert_hex (char *transport, uint32_t *value)
{
  *value = 0;
  while (isxdigit(*transport)) {
    *value *= 16;
    if (isdigit(*transport))
      *value += *transport - '0';
    else
      *value += tolower(*transport) - 'a' + 10;
    transport++;
  }
  return (transport);
}
void set_configs (void)
{
	config.ReadVariablesFromRegistry("Software\\Mpeg4ip", "Config");
	rtsp_set_loglevel(config.get_config_value(CONFIG_RTSP_DEBUG));
	rtp_set_loglevel(config.get_config_value(CONFIG_RTP_DEBUG));
	sdp_set_loglevel(config.get_config_value(CONFIG_SDP_DEBUG));
	http_set_loglevel(config.get_config_value(CONFIG_HTTP_DEBUG));
	volume = config.get_config_value(CONFIG_VOLUME);
	if (config.GetBoolValue(CONFIG_MUTED))
		volume = 0;
	if (psptr != NULL) {
		psptr->set_audio_volume(volume);
	}
}
	

int main (int argc, char **argv)
{
	SDL_sem *master_sem;
	CClientProcess proc;
	CMsgQueue master_queue;

	argv++;
	argc--;
	int deb = 0;
	if (*argv[0] == '-') {
		argv++;
		argc--;
		deb = 1;
	}
	
	open_output("wmp4player.log");
	if (proc.enable_communication(deb) < 0) {
		printf("Could not enable communications\n");
		Sleep(10 * 1000);
		close_output();
		return -1;
	}
	initialize_plugins(&config);
	config.InitializeIndexes();
	config.ReadVariablesFromRegistry("Software\\Mpeg4ip", "Config");

	psptr = NULL;
	master_sem = NULL;
	if (*argv == NULL) {
		proc.initial_response(-1, psptr,"No argument to process");
	} else {
		rtsp_set_error_func(library_message);
		rtp_set_error_msg_func(library_message);
		sdp_set_error_func(library_message);
		http_set_error_func(library_message);
	
		master_sem = SDL_CreateSemaphore(0);
			
		psptr = start_session(&master_queue, 
							  master_sem, 
							  NULL,
							  *argv,
							  NULL,
							  config.get_config_value(CONFIG_VOLUME),
			                  100, 
			                  100,
			                  screen_size);
		if (psptr == NULL) {
			proc.initial_response(-1, NULL, NULL);
		} else {
		proc.initial_response(0, psptr, psptr->get_message());
		}
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
			case MSG_SESSION_WARNING:
				proc.send_string(GUI_MSG_SESSION_WARNING, psptr->get_message());
				break;
			case MSG_SESSION_ERROR:
				player_error_message("error is \"%s\"", psptr->get_message());
				proc.send_string(GUI_MSG_SESSION_ERROR, psptr->get_message());
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
	

	config.WriteVariablesToRegistry("Software\\Mpeg4ip", "Config");
	close_plugins();
	close_output();
	return 0;
}

int getIpAddressFromInterface (const char *ifname,
			       struct in_addr *retval)
{
  int ret = -1;
#ifndef _WIN32
  int fd;
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
