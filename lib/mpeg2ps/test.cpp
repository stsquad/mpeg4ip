
#include <mpeg4ip.h>
#include <mpeg4ip_getopt.h>
#include <mpeg2_ps.h>
#include <mpeg2ps_private.h>

int main (int argc, char *argv[])
{
  bool verbose = false;
  const char *ProgName = *argv;
  while (true) {
    int c = -1;
    int option_index = 0;
    static const char *usage = "usage";
    static struct option long_options[] = {
      { "help", 0, 0, '?' },
      { "version", 0, 0, 'v'},
      { "verbose", 0, 0, 'V'},
      { NULL, 0, 0, 0 }
    };

    c = getopt_long_only(argc, argv, "?vV",
			 long_options, &option_index);

    if (c == -1)
      break;
    
    switch (c) {
    case '?':
      printf("%s", usage);
      exit(1);
      break;
    case 'V':
      verbose = true;
      break;
    case 'v':
      printf("%s - %s version %s", 
	      ProgName, MPEG4IP_PACKAGE, MPEG4IP_VERSION);
      exit(1);
    }
  }

  argc -= optind;
  argv += optind;
  mpeg2ps_t *ps;
  mpeg2ps_set_loglevel(LOG_DEBUG);
  while (argc > 0) {
    
    ps = mpeg2ps_init(*argv);
    argc--;
    argv++;
    if (ps == NULL) {
      printf("%s is not a valid file\n", *(argv - 1));
      continue;
    }
    printf("max time is "U64"\n", mpeg2ps_get_max_time_msec(ps));
    uint8_t *buffer;
    uint32_t buflen;
    uint64_t ts;
    uint32_t freq_ts;
#if 0
    uint8_t ftype;
    mpeg2ps_seek_video_frame(ps, 0, mpeg2ps_get_max_time_msec(ps) / 2);
    if (mpeg2ps_get_video_frame(ps, 0, &buffer, &buflen, &ftype, TS_MSEC, &ts) == false) {
      printf("couldn't read frame\n");
    } else {
      printf("frame - len %d type %d ts "U64"\n", 
	     buflen, ftype, ts);
    }
    mpeg2ps_seek_audio_frame(ps, 0, mpeg2ps_get_max_time_msec(ps) / 2);

    if (mpeg2ps_get_audio_frame(ps, 0, &buffer, &buflen, TS_MSEC, &freq_ts, &ts) == false) {
      printf("couldn't read frame\n");
    } else {
      printf("frame - len %d freq_Ts %u ts "U64"\n", 
	     buflen, freq_ts, ts);
    }
#else
    uint frame_cnt = 0;
    while (mpeg2ps_get_audio_frame(ps, 0, &buffer, &buflen, TS_MSEC, &freq_ts, &ts)) {
      frame_cnt++;
      printf("%u - len %u freq %u ts "X64"\n", 
	     frame_cnt, buflen, freq_ts, ts);
    }
#endif
    mpeg2ps_close(ps);
  }
  return (1);

}
