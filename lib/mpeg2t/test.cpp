#include "mpeg4ip.h"
#include "mpeg2_transport.h"
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

  //  int lastcc, ccset;

  buffer = (uint8_t *)malloc(BUFFER_SIZE);

  mpeg2t_set_loglevel(LOG_DEBUG);
  mpeg2t = create_mpeg2_transport();
  mpeg2t->save_frames_at_start = 1;
  argc--;
  argv++;
  ifile = fopen(*argv, FOPEN_READ_BINARY);
  buflen = 0;
  readfromfile = 0;
  //lastcc = 0;
  while (!feof(ifile)) {
    if (buflen > 0) {
      memmove(buffer, buffer + readfromfile - buflen, buflen);
    }
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
	  printf("Pid %x %d frame psts len %d %d "U64, 
		 pidptr->pid,
		 es_pid->stream_type,
		 p->frame_len,
		 p->have_ps_ts, p->ps_ts);
	  if (es_pid->is_video) {
	    printf(" type %d\n", p->frame_type);
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
  return 0;
}

