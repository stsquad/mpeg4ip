
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

  while (argc > 0) {
    int fd = open(*argv, OPEN_RDONLY);
    
#define WRITE_ES_TO_FILE 1
#define DUMP 1
#if 0
    mpeg2ps_stream_t *sptr = mpeg2ps_stream_create(fd, 0xbd);
    sptr->m_substream_id = 0x80;
    
    const uint8_t *buffer;
    uint32_t buflen;
    bool eof = false;
#ifdef WRITE_ES_TO_FILE 
    FILE *outfile = fopen("test.mp3", FOPEN_WRITE_BINARY);
#endif
    uint32_t cnt = 0;
    uint64_t len = 0;
    mpeg2ps_ts_t ts;
    memset(&ts, 0, sizeof(ts));
    while (mpeg2ps_stream_read_frame(sptr, &buffer, &buflen, &eof, &ts)) {
#ifdef WRITE_ES_TO_FILE 
      fwrite(buffer, 1, buflen, outfile);
#endif
      len += buflen;
#ifdef DUMP
      //printf("frame - len %d\n", buflen);
      printf("ts %p dts %p pts %p\n", &ts, &ts.dts, &ts.pts);
      if (ts.have_pts && ts.have_dts) {
	printf ("frame - len %d pts "U64" dts "U64" total "X64" %x %x\n", 
		buflen, ts.pts, ts.dts, len,
		buffer[0], buffer[1]);
      } else if (ts.have_pts) {
	printf ("frame - len %d pts "U64" total "X64" %x %x\n", 
		buflen, ts.pts, len,
		buffer[0], buffer[1]);
      } else if (ts.have_dts) {
	printf ("frame - len %d dts "U64" total "X64" %x %x\n", 
		buflen, ts.dts, len,
		buffer[0], buffer[1]);
      } else {
	printf ("frame - len %d total "X64" %x %x\n", buflen, len,
		buffer[0], buffer[1]);
      }
      
      if (len == 0xb91f2) {
	printf("here\n");
      }
      //buffer[0], buffer[1], buffer[2], buffer[3]);
#endif
      if (eof) {
	printf("have eof\n");
      }
      cnt++;
      memset(&ts, 0, sizeof(ts));
    }
    printf("%u frames read\n", cnt);
#ifdef WRITE_ES_TO_FILE 
    fclose(outfile);
#endif
    mpeg2ps_stream_destroy(sptr);
#endif
    printf("trying file %s\n", *argv);
    mpeg2ps_scan_file(fd);
    close(fd);
    argc--;
    argv++;
  }
  return (1);

}
