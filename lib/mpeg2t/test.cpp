
#include "mpeg4ip.h"
#include "mpeg4ip_getopt.h"
#include "mpeg2_transport.h"
#include "mp4av.h"

#define BUFFER_SIZE (188)
class CMpeg2SeqCheck
{
public:
  CMpeg2SeqCheck (void) {
    m_frame = m_seq_len = 0;
    m_seq = NULL;
  };
  ~CMpeg2SeqCheck (void) {
    CHECK_AND_FREE(m_seq);
  }
  void check_frame (mpeg2t_frame_t *p) {
    m_frame++;
    if (p->pict_header_offset == 0) {
      // printf("pict header of 0\n");
      return;
    }
    int ret;
    uint32_t scode, offset, endoffset;
    ret = MP4AV_Mpeg3FindNextStart(p->frame + p->seq_header_offset,
				   p->frame_len - p->seq_header_offset,
				   &offset,
				   &scode);
    if (scode != MPEG3_SEQUENCE_START_CODE) {
      return;
    }
    offset += p->seq_header_offset;
    uint32_t test;
    test = offset;
    while ((ret = MP4AV_Mpeg3FindNextStart(p->frame + test + 4,
					   p->frame_len - test - 4,
					   &endoffset,
					   &scode)) >= 0 &&
	   scode == MPEG3_EXT_START_CODE) {
      test += endoffset + 4;

    }
    endoffset += test + 4;
    uint32_t seq_len;
    if (ret < 0) {
      seq_len = p->frame_len - offset;
    } else {
      seq_len = endoffset - offset;
    }
    dump(p->frame + offset, seq_len);
    if (m_seq == NULL) {
      m_seq = (uint8_t *)malloc(seq_len);
      memcpy(m_seq, p->frame + offset, seq_len);
      m_seq_len = seq_len;
      printf("seq start - len %d\n", seq_len);
    } else {
      if (m_seq_len != seq_len) {
	printf("seq len has changed - old %d new %d\n",
	       m_seq_len, seq_len);
 	m_seq = (uint8_t *)realloc(m_seq, seq_len);
	memcpy(m_seq, p->frame + offset, seq_len);
	m_seq_len = seq_len;
      } else if (memcmp(m_seq, p->frame + offset, seq_len) != 0) {
	printf("seq header has changed\n");
	memcpy(m_seq, p->frame + offset, seq_len);
      } else {
	printf("seq matches\n");
      }
    }
  };
private:
  void dump(uint8_t *d, uint32_t len) {
    uint8_t count = 0;
    while (len > 0) {
      printf("%02x ", *d++);
      count++;
      if (count == 15) {
	count = 0;
	putchar('\n');
      }
      
      len--;
    }
    putchar('\n');
  };
  uint8_t *m_seq;
  uint32_t m_seq_len;
  uint32_t m_frame;
};
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
  int64_t start_offset = 0;
  bool verbose = false;
  fpos_t pos;
  const char *ProgName = argv[0];
  const char *usage = "";
  //  int lastcc, ccset;
  while (true) {
    int c = -1;
    int option_index = 0;
    static struct option long_options[] = {
      { "help", 0, 0, '?' },
      { "version", 0, 0, 'v'},
      { "verbose", 0, 0, 'V'},
      { "start-offset", 1, 0, 's'},
      { NULL, 0, 0, 0 }
    };

    c = getopt_long_only(argc, argv, "?vVs:",
			 long_options, &option_index);

    if (c == -1)
      break;
    
    switch (c) {
    case '?':
      printf("%s", usage);
      exit(1);
    case 's':
      if (sscanf(optarg, D64, &start_offset) != 1) {
	fprintf(stderr, "can't read start-offset arg %s\n", 
		optarg);
	exit(-1);
      };
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

  CMpeg2SeqCheck seq;
  argc -= optind;
  argv += optind;

  buffer = (uint8_t *)malloc(BUFFER_SIZE);

  if (verbose)
    mpeg2t_set_loglevel(LOG_DEBUG);
  else
    mpeg2t_set_loglevel(LOG_ERR);
  mpeg2t = create_mpeg2_transport();
  mpeg2t->save_frames_at_start = 1;
  ifile = fopen(*argv, FOPEN_READ_BINARY);
  if (ifile == NULL) {
    fprintf(stderr, "Couldn't open file %s\n", *argv);
    exit(1);
  }
  if (start_offset != 0) {
    int ret;
    if (start_offset < 0) {
      //start_offset = TO_U64(0) - start_offset;
      ret = fseeko(ifile, start_offset, SEEK_END);
    } else {
      ret = fseeko(ifile, start_offset, SEEK_SET);
    }
    int err = errno;
    printf("%d %s start offset "U64" "D64"\n", ret, strerror(err), ftello(ifile), start_offset);
  }

  buflen = 0;
  readfromfile = 0;
#if 1
    FILE *ofile;
    bool has_pat = false;
    ofile = fopen("es.m4v", FOPEN_WRITE_BINARY);
#endif
  //lastcc = 0;
  while (!feof(ifile)) {
    if (buflen > 0) {
      printf("buflen is %d\n", buflen);
      memmove(buffer, buffer + readfromfile - buflen, buflen);
    }
    fgetpos(ifile, &pos);
    uint64_t position;
    FPOS_TO_VAR(pos, uint64_t, position);
    fprintf(stdout, "file pos 0x%llx %s\n", position - buflen,
	(position - buflen) % 188 == 0 ? "" : "no mult");

    if (position - buflen == 0x11a0) {
      printf("here\n");
    }
    readfromfile = buflen + fread(buffer + buflen, 1, BUFFER_SIZE - buflen, ifile);
    buflen = readfromfile;
    ptr = buffer;
    done_with_buf = 0;


    while (done_with_buf == 0) {
      pidptr = mpeg2t_process_buffer(mpeg2t, ptr, buflen, &offset);
      printf("processed %d\n", offset);
      ptr += offset;
      buflen -= offset;
      if (buflen < 188) {
	done_with_buf = 1;
      }
#if 1
      if (pidptr != NULL && pidptr->pak_type == MPEG2T_PAS_PAK) {
	fwrite(ptr - 188, 1, 188, ofile);
	has_pat = true;
	fprintf(stderr, "have pat\n");
      }
      if (pidptr != NULL && pidptr->pak_type == MPEG2T_PROG_MAP_PAK) {
	fwrite(ptr - 188, 1, 188, ofile);
	fclose(ofile);
	exit(1);
      }
#endif
      if (pidptr != NULL && pidptr->pak_type == MPEG2T_ES_PAK) {
	mpeg2t_frame_t *p;
	es_pid = (mpeg2t_es_t *)pidptr;

	while ((p = mpeg2t_get_es_list_head(es_pid)) != NULL) {
	  printf("Pid %x %d frame len %5d psts %d "U64" dts %d "U64, 
		 pidptr->pid,
		 es_pid->stream_type,
		 p->frame_len,
		 p->have_ps_ts, p->ps_ts,
		 p->have_dts, p->dts);
	  if (es_pid->is_video &&
	      es_pid->stream_type == 1 || es_pid->stream_type == 2) {
	    printf(" type %d\n", p->frame_type);
	    seq.check_frame(p);
	    fwrite(p->frame, 1, p->frame_len, ofile);
	  } else {
	    printf("\n");
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

