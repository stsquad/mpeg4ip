
#include "mpeg4ip.h"
#include "mpeg4ip_getopt.h"
#include "http.h"
#include "sdp.h"
#include <time.h>

static int display_sdp (session_desc_t *sdp)
{
  size_t len;
  printf("Sid "U64" name %s\n", sdp->session_id, sdp->session_name);
  if (sdp->control_string == NULL) {
    // broadcast - try time
    if (sdp->time_desc != NULL) {
      if (sdp->time_desc->start_time == 0 &&
	  sdp->time_desc->start_time == 0) {
	printf("Continuous program\n");
      } else {
	const char *status;
	char start_time[40], end_time[40];
	if (time(NULL) >= sdp->time_desc->start_time) {
	  if (time(NULL) <= sdp->time_desc->end_time) {
	    status = " - ACTIVE";
	  } else {
	    status = " - COMPLETED";
	  }
	} else {
	  status = "";
	}
	ctime_r(&sdp->time_desc->start_time, start_time);
	len = strlen(start_time);
	len -= 1; // get last offset
	while (isspace(start_time[len])) {
	  start_time[len] = '\0';
	  len--;
	}
	ctime_r(&sdp->time_desc->end_time, end_time);
	len = strlen(end_time);
	len -= 1; // get last offset
	while (isspace(end_time[len])) {
	  end_time[len] = '\0';
	  len--;
	}

	printf("Starts %s - ends %s %s\n",
	       start_time, end_time, status);
      }
    }
  }
  printf("----------------------------------------------------------------------\n");
  return 0;
}

static int get_program_list (const char *content_manager,
			     bool save_sid,
			     uint64_t sid)
{
  http_client_t *http_client;
  http_resp_t *http_resp;
  int ret;
  sdp_decode_info_t *sdp_info;
  session_desc_t *sdp, *ptr;
  int translated;
  int retvalue;
  int ix;

  http_resp = NULL;
  http_client = http_init_connection(content_manager);

  if (http_client == NULL) {
    fprintf(stderr, "Cannot create http client with %s\n", 
	    content_manager);
    return -1;
  }
  retvalue = -1;
  ret = http_get(http_client, NULL, &http_resp);
  
  if (ret > 0) {
    sdp_info = set_sdp_decode_from_memory(http_resp->body);
    if ((sdp_decode(sdp_info, &sdp, &translated) == 0) &&
	(translated > 0)) {
      retvalue = 0;
      for (ix = 0; ix < translated; ix++) {
	if (save_sid) {
	  if (sid == sdp->session_id) {
	    char buffer[80];
	    sprintf(buffer, U64".sdp", sid);
	    sdp_encode_one_to_file(sdp, buffer, 0);
	    retvalue = 1;
	  }
	} else {
	  display_sdp(sdp);
	}
	ptr = sdp->next;
	sdp->next = NULL;
	sdp_free_session_desc(sdp);
	sdp = ptr;
      }
    } else {
      fprintf(stderr, "No programs\n");
    }
  }
  http_resp_free(http_resp);
  http_free_connection(http_client);
  return retvalue;
}

int main (int argc, char **argv)
{
  char* ProgName = argv[0];
  bool save_sid;
  uint64_t sid;

  save_sid = false;
  sid = 0;
  while (true) {
    int c = -1;
    int option_index = 0;
    static struct option long_options[] = {
      { "version", 0, 0, 'V' },
      { "debug", 0, 0, 'd' },
      { "save", 1, 0, 's' },
      { NULL, 0, 0, 0 }
    };
    
    c = getopt_long_only(argc, argv, "Vds:",
			 long_options, &option_index);
    
    if (c == -1)
      break;
    
    switch (c) {
    case '?':
      fprintf(stderr, "%s - usage [-v] [-d] [-s sid] <content manager>\n", ProgName);
      fprintf(stderr, " -d - debug\n");
      fprintf(stderr, " -v - version\n");
      fprintf(stderr, " -s sid - save sdp for session id <sid>\n");
      exit(0);
    case 'd':
      sdp_set_loglevel(LOG_DEBUG);
      http_set_loglevel(LOG_DEBUG);
      break;
    case 'V':
      fprintf(stderr, "%s - %s version %s\n", ProgName, 
	      MPEG4IP_PACKAGE, MPEG4IP_VERSION);
      exit(0);
    case 's':
      sid = strtoull(optarg, NULL, 10);
      save_sid = true;
      break;
    }
  }
  
  while (optind < argc) {
    char *cm = argv[optind++];
    char buffer[1024];
    if (!save_sid)
      printf("Content manager %s Scheduled Programs\n", cm);
    snprintf(buffer, sizeof(buffer), "http://%s/iptvfiles/guide.sdf", 
	     cm);
    if (get_program_list(buffer, save_sid, sid) > 0) {
      printf("Program "U64" found in Scheduled programs\n", sid);
      return 0;
    }
    if (!save_sid)
      printf("Content manager %s On-Demand Programs\n", cm);
    snprintf(buffer, sizeof(buffer), "http://%s/servlet/OdPublish", cm);
    if (get_program_list(buffer, save_sid, sid) > 0) {
      printf("Program "U64" found in On-demand programs\n", sid);
      return 0;
    }
  }

  return 0;
}
