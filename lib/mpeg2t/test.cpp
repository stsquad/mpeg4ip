#include "systems.h"
#include "mpeg2_transport.h"

int main (int argc, char **argv)
{
  FILE *ifile, *ofile;
  uint8_t buffer[16*188], *ptr;
  uint32_t buflen, readfromfile;
  uint32_t offset;
  int done_with_buf;
  mpeg2t_t *mpeg2t;
  mpeg2t_es_t *es_pid;
  mpeg2t_pid_t *pidptr;

  //  int lastcc, ccset;

  mpeg2t_set_loglevel(LOG_DEBUG);
  mpeg2t = create_mpeg2_transport();
  argc--;
  argv++;
  ifile = fopen(*argv, FOPEN_READ_BINARY);
  ofile = fopen("raw.mp3", FOPEN_WRITE_BINARY);
  buflen = 0;
  readfromfile = 0;
  //lastcc = 0;
  while (!feof(ifile)) {
    if (buflen > 0) {
      memmove(buffer, buffer + readfromfile - buflen, buflen);
    }
    readfromfile = buflen = fread(buffer + buflen, 1, 16*188 - buflen, ifile);
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
	mpeg2t_frame_t *mp3, *p;
	es_pid = (mpeg2t_es_t *)pidptr;
	mp3 = es_pid->list;
	es_pid->list = NULL;
	while (mp3 != NULL) {
	  printf("Wrote %d frame psts len %d %d %llu %llx\n", 
		 es_pid->stream_type,
		 mp3->frame_len,
		 mp3->have_ps_ts, mp3->ps_ts, mp3->ps_ts);
	  //fwrite(mp3->frame, mp3->frame_len, 1, ofile);
	  p = mp3;
	  mp3 = mp3->next_frame;
	  free(p);
	}
      }
    }
  }
  fclose(ifile);
  fclose(ofile);
  return 0;
}

