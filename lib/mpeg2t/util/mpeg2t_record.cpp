#define XOPEN_SOURCE
#include "mpeg4ip.h"
#include "mpeg4ip_getopt.h"
#include <sys/ioctl.h>
#include <net/if.h>
#include <net_udp.h>
#include <poll.h>
#include "mpeg2_transport.h"

static int getIpAddressFromInterface (const char *ifname,
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

typedef struct pes_record_t {
  uint8_t *buffer;
  uint32_t max_len;
  uint32_t current_len;
} pes_record_t;

static bool record_pes (mpeg2t_pid_t *pid,
			uint8_t *buffer)
{
  mpeg2t_es_t *es_pid = (mpeg2t_es_t *)pid;
  pes_record_t *pes;
  if (es_pid->es_userdata == NULL) {
    pes = MALLOC_STRUCTURE(pes_record_t);
    es_pid->es_userdata = pes;
    pes->buffer = (uint8_t *) malloc(188*10);
    pes->max_len = 188 * 10;
    pes->current_len = 0;
  }
  pes = (pes_record_t *)es_pid->es_userdata;
  if (es_pid->stream_type == 1 || es_pid->stream_type == 2) {
    // video
    if (es_pid->have_seq_header) {
      return true;
    }
  }
  uint32_t pes_indicator;
  pes_indicator = mpeg2t_payload_unit_start_indicator(buffer);
  if (pes->current_len == 0) {
    if (pes_indicator == 0) {
      return false;
    }
  } else {
    if (pes_indicator != 0) {
      pes->current_len = 0;
    }
  }

  if (pes->current_len + 188 > pes->max_len) {
    pes->buffer = (uint8_t *)realloc(pes->buffer, pes->max_len + 10 * 188);
    pes->max_len += 10 * 188;
  }
  memcpy(pes->buffer + pes->current_len, buffer, 188);
  pes->current_len += 188;
  return false;
}

const char *ProgName;
static const char *usage = 
"usage: %s <options> <time> <date> <duration> <multicast addr:port> <filename>\n"
" Options:\n"
" --version     Display Version\n"
" --help        Display this message\n"
" --overwrite   Overwrite existing file\n"
;

int main (int argc, char **argv)
{
  bool overwrite = false;
  ProgName = argv[0];
  
  while (true) {
    int c = -1;
    int option_index = 0;
    static struct option long_options[] = {
      { "help", 0, 0, '?' },
      { "version", 0, 0, 'v'},
      { "overwrite", 0, 0, 'o'},
      { NULL, 0, 0, 0 }
    };

    c = getopt_long_only(argc, argv, "?v",
			 long_options, &option_index);

    if (c == -1)
      break;
    
    switch (c) {
    case '?':
      fprintf(stderr, usage, ProgName);
      exit(1);
    case 'v':
      fprintf(stderr, "%s - %s version %s", 
	      ProgName, PACKAGE, VERSION);
      exit(1);
    case 'o':
      overwrite = true;
      break;
    }
  }

  argc -= optind;
  argv += optind;
  char *start_time, *date, *address, *filename, *interface;
  unsigned long duration;
  unsigned long templong;
  struct in_addr addr;
  in_port_t port = 0;
  int err;
  start_time = date = address = filename = interface = NULL;
  duration = 0;
  for (; argc > 0; argc--, argv++) {
    char *temp;
    temp = strchr(*argv, ':');
    if (temp != NULL) {
      // start_time or address/port
      *temp = '\0';
      err = inet_aton(*argv, &addr);
#define ALREADY_DETECTED(a) if ((a) != NULL) { fprintf(stderr, "Already detected " #a " %s not %s\n", a, *argv); exit(1); }

      if (err > 0 && strchr(*argv, '.') != NULL) {
	ALREADY_DETECTED(address);
	address = *argv;
	if (!IN_MULTICAST(htonl(addr.s_addr))) {
	  fprintf(stderr, "address %sis not multicast\n", address);
	  exit(1);
	}
	templong = strtoul(temp + 1, NULL, 10);
	if (templong < 1024 || templong > 65535) { 
	  fprintf(stderr, "port range must be 1024 to 65535\n");
	  exit(1);
	}
	port = templong;
	// leave address "broken" into 2
      } else {
	*temp = ':';
	ALREADY_DETECTED(start_time);
	start_time = *argv;
      }
      continue;
    } 
    temp = strrchr(*argv, '.');
    if (temp != NULL) {
      if (strcasecmp(temp, ".mpg") == 0 ||
	  strcasecmp(temp, ".mp2t") == 0 ||
	  strcasecmp(temp, ".mpeg") == 0) {
	ALREADY_DETECTED(filename);
	filename = *argv;
	continue;
      }
    }
    temp = strchr(*argv, '/');
    if (temp != NULL) {
      ALREADY_DETECTED(date);
      date = *argv;
      continue;
    }
    if (strcasecmp(*argv, "now") == 0) {
      ALREADY_DETECTED(start_time);
      start_time = *argv;
      continue;
    }
    if (strcasecmp(*argv, "today") == 0) {
      ALREADY_DETECTED(date);
      // null for date
      continue;
    }
    temp = *argv;
    templong = strtoul(*argv, &temp, 10);
    if (temp != NULL && *temp == '\0') {
      if (duration != 0) {
	fprintf(stderr, "duration already specifed %lu not %s\n",
		duration, *argv);
	exit(1);
      }
      duration = templong;
      continue;
    }
    ALREADY_DETECTED(interface);
    interface = *argv;
  }

  if (start_time == NULL) {
    fprintf(stderr, "Must specify start start_time or now\n");
    exit(1);
  }
  if (duration == 0) {
    fprintf(stderr, "please specify duration\n");
    exit(1);
  }
  if (address == NULL) {
    fprintf(stderr, "no address specified\n");
    exit(1);
  }
  if (filename == NULL) {
    fprintf(stderr, "no filename specified\n");
    exit(1);
  }
  #define OUTPUT_VAR(a) fprintf(stderr, #a " is %s\n", a)
  OUTPUT_VAR(start_time);
  OUTPUT_VAR(date);
  fprintf(stderr, "duration is %lu\n", duration);
  OUTPUT_VAR(address);
  OUTPUT_VAR(filename);
  OUTPUT_VAR(interface);
  struct tm start_tm;
  time_t now, start_timet = 0;
  bool start_now = false;
  char *res;
  time(&now);
  localtime_r(&now, &start_tm);
  start_tm.tm_sec = 0;
  if (strcasecmp(start_time, "now") == 0) {
    // now
    start_now = true;
    if (date != NULL) {
      date = NULL;
    }
  } else {
    res = strptime(start_time, "%H:%M", &start_tm);
    if (res == NULL || *res != '\0') {
      fprintf(stderr, "time format is not value %s\n", time);
      exit(1);
    }
    if (date != NULL) {
      res = strptime(date, "%m/%d/%y", &start_tm);
      if (res == NULL || *res != '\0') {
	res = strptime(date, "%m/%d/%Y", &start_tm);
	if (res == NULL || *res != '\0') {
	  fprintf(stderr, "Date format is not correct %s\n", date);
	  exit(1);
	}
      }
    }
    start_timet = mktime(&start_tm);
    if (difftime(now, start_timet) > 0.0) {
      if (date) {
	fprintf(stderr, "time %s on %s is in past\n", start_time, date);
	exit(1);
      } 
      fprintf(stderr, "time %s is in past\n", start_time);
      exit(1);
    }
  }
  struct stat statbuf;
  if (stat(filename, &statbuf) == 0 && overwrite == false) {
    fprintf(stderr, "File %s exists - use --overwrite\n", filename);
    exit(1);
  }
  // valid start time is in start_timet
  // duration is seconds
  // address is in address, addr and port
  // filename is done
  //  duration *= 60; 
  struct in_addr if_addr;
  if (interface != NULL &&
      getIpAddressFromInterface(interface, &if_addr) < 0) {
    fprintf(stderr, "Bad interface %s\n", interface);
    exit(1);
  }


  if (start_now == false) {
    // need to wait it out...
    now = time(NULL);
    fprintf(stderr, "sleeping for %ld seconds\n", start_timet - now);
    sleep(start_timet - now);
  } else {
    start_timet = time(NULL);
  }
  // we're ready to begin

  socket_udp *udp;
  if (interface != NULL) {
    udp = udp_init_if(address, 
		      inet_ntoa(if_addr),
		      port, 
		      0, 
		      15);
  } else {
    udp = udp_init(address, port, 0, 15);
  }
  if (udp == NULL) {
    fprintf(stderr, "Can't open socket\n");
    exit(-1);
  }
  FILE *outfile = fopen(filename, FOPEN_WRITE_BINARY);
  if (outfile == NULL) {
    fprintf(stderr, "Couldn't open file %s %s", filename, strerror(errno));
    exit(1);
  }
  time_t end_timet = start_timet + duration;
  uint state = 0;
  uint8_t buffer[65535];
  uint32_t buflen_left = 0;
  uint64_t bytes_written = 0;
  int data_socket = udp_fd(udp);
  mpeg2t_t *mp2t = create_mpeg2_transport();

  do {
    struct pollfd pollit;
    pollit.fd = data_socket;
    pollit.events = POLLIN | POLLPRI;
    pollit.revents = 0;
    poll(&pollit, 1, 1000); // wait 1 second
    if ((pollit.revents & (POLLIN | POLLPRI)) != 0) {
      // data in socket - read and do our thing
      uint32_t buflen = udp_recv(udp, 
				 buffer + buflen_left,
				 sizeof(buffer) - buflen_left);
      buflen += buflen_left;
      uint16_t rpid;
      uint32_t offset = 0;
      switch (state) {
      case 0:
	if (buffer[0] != MPEG2T_SYNC_BYTE) {
	  break;
	}
	state = 1;
	fprintf(stderr, "acquired sync\n");
	// fall into state 1
      case 1: {
	bool found_pas = false;
	while (buflen >= 188 && found_pas == false) {
	  rpid = mpeg2t_pid(buffer + offset);
	  if (rpid == 0) {
	    fprintf(stderr, "acquired PAS\n");
	    found_pas = true;
	  } else {
	    offset += 188;
	    buflen -= 188;
	  }
	}
	if (buflen > 0) {
	  memmove(buffer, buffer + offset, buflen);
	}
	if (found_pas == false) 
	  break;
	state = 2;
      }
	// fall into state 2
      case 2: {
	// process until we get a sequence header...
	bool found_seq = false;
	offset = 0;
	while (buflen >= 188 && found_seq == false) {
	  uint32_t used;
	  mpeg2t_process_buffer(mp2t,
				buffer + offset,
				188,
				&used);
	  // processed the buffer.  Let's see what it does:
	  rpid = mpeg2t_pid(buffer + offset);
	  if (rpid != 0x1fff) {
	    mpeg2t_pid_t *pidptr = mpeg2t_lookup_pid(mp2t, rpid);
	    if (pidptr != NULL) {
	      switch (pidptr->pak_type) {
	      case MPEG2T_PAS_PAK:
	      case MPEG2T_PROG_MAP_PAK:
		// write it:
		fwrite(buffer + offset, 1, 188, outfile);
		break;
	      case MPEG2T_ES_PAK: 
		// the guts
		found_seq = record_pes(pidptr, buffer);
		if (found_seq) {  
		  // we have a seq number in a pes packet
		  pidptr = mp2t->pas.pid.next_pid;
		  while (pidptr != NULL) {
		    if (pidptr->pak_type == MPEG2T_ES_PAK) {
		      mpeg2t_es_t *es_pid = (mpeg2t_es_t *)pidptr;
		      pes_record_t *pes = (pes_record_t *)es_pid->es_userdata;
		      if (pes->current_len > 0) {
			fprintf(stderr, "pid %x wrote pes %d bytes\n", 
				pidptr->pid, pes->current_len);
			fwrite(pes->buffer, pes->current_len, 1, outfile);
		      }
		    }
		    pidptr = pidptr->next_pid;
		  }
		}
		break;
	      }
	    } else {
	      // don't write this - fwrite(buffer + offset, 1, 188, outfile);
	    }
	  } else {
	    // maybe write spacer ?
	  }
	  if (found_seq == false) {
	    offset += 188;
	    buflen -= 188;
	  }
	}
	if (buflen > 0) {
	  memmove(buffer, buffer + offset, buflen);
	}
	buflen_left = buflen;
	if (found_seq == false) {
	  break;
	}
	state = 3;
      }
      case 3: {
	uint32_t bytes;
	bytes = buflen / 188;
	bytes *= 188;
	fwrite(buffer, 1, bytes, outfile);
	buflen_left = buflen - bytes;
	if (buflen_left > 0) {
	  memmove(buffer, buffer + bytes, buflen_left);
	}
	bytes_written += bytes;
	break;
      }
      }
    }
    now = time(NULL);
  } while (now < end_timet);  
  fclose(outfile);
  fprintf(stderr, "Wrote %llu bytes\n", bytes_written);
    
   exit(0);
}
  
      
