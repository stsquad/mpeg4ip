
#include "mpeg4ip.h"
#include "mpeg4ip_getopt.h"
#include "libmpeg3.h"

int main(int argc, char** argv)
{
  char* usageString = "[--video] [--audio] <file-name>\n";
  int dump_audio = 0, dump_video = 0;
  char *infilename;
  char outfilename[FILENAME_MAX], *suffix;
  size_t suffix_len;
  FILE *outfile;
  mpeg3_t *infile;
  /* begin processing command line */
  char* progName = argv[0];
  uint8_t *inbuf;
  long inlen;

  while (1) {
    int c = -1;
    int option_index = 0;
    static struct option long_options[] = {
      { "audio", 0, 0, 'a' },
      { "video", 0, 0, 'v' },
      { "version", 0, 0, 'V' },
      { NULL, 0, 0, 0 }
    };

    c = getopt_long_only(argc, argv, "v::V",
			 long_options, &option_index);
    
    if (c == -1)
      break;

    switch (c) {
    case 'v':
      dump_video = 1;
      break;
    case 'a':
      dump_audio = 1;
      break;
    case 'V':
      fprintf(stderr, "%s - %s version %s\n", 
	      progName, MPEG4IP_PACKAGE, MPEG4IP_VERSION);
      exit(0);
    default:
      fprintf(stderr, "%s: unknown option specified, ignoring: %c\n", 
	      progName, c);
    }
  }

  if ((argc - optind) < 1) {
    fprintf(stderr, "usage: %s %s\n", progName, usageString);
    exit(1);
  }

  if (dump_audio == 0 && dump_video == 0) {
    dump_audio = dump_video = 1;
  }

  while (optind < argc) {
    infilename = argv[optind];
    optind++;
    suffix = strrchr(infilename, '.');
    suffix_len = suffix -  infilename;
    if (suffix == NULL) {
      fprintf(stderr, "can't find suffix in %s\n", infilename);
      continue;
    }
    if (mpeg3_check_sig(infilename) != 1) {
      fprintf(stderr, "%s is not a valid mpeg file\n", 
	      infilename);
      continue;
    }
    if (dump_audio) {
      char *slash;
      unsigned char *buf = NULL;
      uint32_t len = 0;
      uint32_t maxlen = 0;
      slash = strrchr(infilename, '/');
      if (slash != NULL) {
	slash++;
	suffix_len = suffix - slash;
	memcpy(outfilename, slash, suffix_len);
      } else {
	memcpy(outfilename, infilename, suffix_len);
      }
      infile = mpeg3_open(infilename);
      if (mpeg3_total_astreams(infile) == 0) {
	fprintf(stderr, "no audio streams in %s\n", infilename);
	mpeg3_close(infile);
	continue;
      }
      switch (mpeg3_get_audio_format(infile, 0)) {
      case AUDIO_MPEG:
	strcpy(outfilename + suffix_len, ".mp3");
	break;
      case AUDIO_AC3:
	strcpy(outfilename + suffix_len, ".ac3");
	break;
      case AUDIO_PCM:
	strcpy(outfilename + suffix_len, ".pcm");
	break;
      case AUDIO_AAC:
	strcpy(outfilename + suffix_len, ".aac");
	break;
      }
      outfile = fopen(outfilename, FOPEN_WRITE_BINARY);
      while (mpeg3_read_audio_frame(infile, 
				    &buf, 
				    &len,
				    &maxlen, 
				    0) >= 0) {
	fwrite(buf, len, 1, outfile);
      }
      CHECK_AND_FREE(buf);
      fclose(outfile);
      mpeg3_close(infile);
    }
	
    if (dump_video) {
      char *slash;
      slash = strrchr(infilename, '/');
      if (slash != NULL) {
	slash++;
	suffix_len = suffix - slash;
	memcpy(outfilename, slash, suffix_len);
      } else {
	memcpy(outfilename, infilename, suffix_len);
      }
      strcpy(outfilename + suffix_len, ".m2v");
      
      infile = mpeg3_open(infilename);
      if (mpeg3_total_vstreams(infile) == 0) {
	fprintf(stderr, "no video streams in %s\n", infilename);
	mpeg3_close(infile);
	continue;
      }
      outfile = fopen(outfilename, FOPEN_WRITE_BINARY);
      while (mpeg3_read_video_chunk_resize(infile, 
					   (unsigned char **)&inbuf, 
					   &inlen,
					   0) == 0) {
	fwrite(inbuf, inlen, 1, outfile);
	mpeg3_read_video_chunk_cleanup(infile, 0);
      }
      fclose(outfile);
      mpeg3_close(infile);
    }

  }
  return 0;
}
