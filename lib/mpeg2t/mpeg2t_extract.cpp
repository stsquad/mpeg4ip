
#include "mpeg4ip.h"
#include "mpeg4ip_getopt.h"
#include "mpeg2_transport.h"
#include "mp4av.h"
// strip_filename.  In buffer (should be MAXPATHLEN), store the
// filename, without the path, and without the suffix.
static void strip_filename (const char *name, char *buffer)
{
  const char *suffix, *slash;
  if (name != NULL) {
    suffix = strrchr(name, '.');
    slash = strrchr(name, '/');
    if (slash == NULL) slash = name;
    else slash++;
    if (suffix == NULL) 
      suffix = slash + strlen(slash);
    memcpy(buffer, slash, suffix - slash);
    buffer[suffix - slash] = '\0';
  } else {
    strcpy(buffer, "out");
  }
}

#define BUFFER_SIZE (1000 * 188)
int main (int argc, char **argv)
{
  FILE *ifile;
  uint8_t *buffer, *ptr;
  uint32_t buflen, readfromfile;
  uint32_t offset;
  int done_with_buf;
  mpeg2t_t *mpeg2t;
  mpeg2t_es_t *es_pid;
  mpeg2t_pid_t *pidptr;
  bool verbose = false;
  fpos_t pos;
  const char *ProgName = argv[0];
  const char *usage = "";
  uint16_t pid = 0;
  const char *filename;

  //  int lastcc, ccset;
  while (true) {
    int c = -1;
    int option_index = 0;
    static struct option long_options[] = {
      { "help", 0, 0, '?' },
      { "version", 0, 0, 'v'},
      { "verbose", 0, 0, 'V'},
      { "pid", 1, 0, 'p' },
      { NULL, 0, 0, 0 }
    };

    c = getopt_long_only(argc, argv, "?vVp:",
			 long_options, &option_index);

    if (c == -1)
      break;
    
    switch (c) {
    case '?':
      printf("%s", usage);
      exit(1);
    case 'V':
      verbose = true;
      break;
    case 'v':
      printf("%s - %s version %s", 
	      ProgName, MPEG4IP_PACKAGE, MPEG4IP_VERSION);
      exit(1);
    case 'p': {
      int readval;
      if (sscanf(optarg, "%i", &readval) != 1) {
	fprintf(stderr, "can't read pid value %s\n", optarg);
	exit(1);
      }
      if (readval > 0x1fff) {
	fprintf(stderr, "illegal pid value %s\n", optarg);
      }
      pid = readval;
    }
    }
  }

  if (pid == 0) {
    fprintf(stderr, "must specify pid to extract\n");
    exit(-1);
  }
   
  filename = argv[optind];

  buffer = (uint8_t *)malloc(BUFFER_SIZE);

  if (verbose)
    mpeg2t_set_loglevel(LOG_DEBUG);
  else
    mpeg2t_set_loglevel(LOG_ERR);
  mpeg2t = create_mpeg2_transport();
  mpeg2t->save_frames_at_start = 1;
  ifile = fopen(filename, FOPEN_READ_BINARY);
  if (ifile == NULL) {
    fprintf(stderr, "Couldn't open file %s\n", filename);
    exit(1);
  }

  buflen = 0;
  readfromfile = 0;
  char outfilename[MAXPATHLEN], spid[20];
  strip_filename(filename, outfilename);
  sprintf(spid, ".%u", pid);
  strcat(outfilename, spid);
    FILE *ofile;
    //    bool has_pat = false;
    ofile = fopen(outfilename, FOPEN_WRITE_BINARY);

  //lastcc = 0;
  while (!feof(ifile)) {
    if (buflen > 0) {
      memmove(buffer, buffer + readfromfile - buflen, buflen);
    }
    fgetpos(ifile, &pos);
    uint64_t position;
    FPOS_TO_VAR(pos, uint64_t, position);
    fprintf(stdout, "file pos %llu\n", position);
    readfromfile = buflen + fread(buffer + buflen, 1, BUFFER_SIZE - buflen, ifile);
    buflen = readfromfile;
    ptr = buffer;
    done_with_buf = 0;


    while (done_with_buf == 0) {
      pidptr = mpeg2t_process_buffer(mpeg2t, ptr, buflen, &offset);
      ptr += offset;
      buflen -= offset;
      if (buflen < 188) {
	done_with_buf = 1;
      }
      if (pidptr != NULL && pidptr->pak_type == MPEG2T_ES_PAK) {
	mpeg2t_frame_t *p;
	es_pid = (mpeg2t_es_t *)pidptr;

	while ((p = mpeg2t_get_es_list_head(es_pid)) != NULL) {
	  if (pidptr->pid == pid) {
	    printf("Pid %x %d frame len %5d psts %d "U64" dts %d "U64"\n", 
		   pidptr->pid,
		   es_pid->stream_type,
		   p->frame_len,
		   p->have_ps_ts, p->ps_ts,
		   p->have_dts, p->dts);
	    
	    fwrite(p->frame, 1, p->frame_len, ofile);
	  }
	  mpeg2t_free_frame(p);
	}
      }
    }
  }
  free(buffer);
  fclose(ifile);
  fclose(ofile);
  return 0;
}

