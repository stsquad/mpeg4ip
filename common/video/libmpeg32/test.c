#include "libmpeg3.h"
#include "stdio.h"





int main (int argc, char **argv)
{
  char *fname, *ofname;
  mpeg3_t *file;
  long max_frames, frame_on;
  char *y_output, *u_output, *v_output;
  int ret, w, h;
  FILE *ofile;

  argc--;
  argv++;

  if (argc == 0) {
    printf("Need an argument\n");
    exit(1);
  }
  fname = *argv;
  argc--;
  argv++;
  if (argc != 0) {
    ofname = *argv;
    ofile = fopen(ofname, "w");
  } else {
    ofile = NULL;
  }

  if (mpeg3_check_sig(fname) == 0) {
    printf("file %s is not mpeg compatible\n", fname);
    exit(1);
  }

  file = mpeg3_open(fname);
  if (file == NULL) {
    printf("error with mpeg3 open\n");
    exit(1);
  }

  mpeg3_set_mmx(file, 1);

#if 0
  if (mpeg3_has_video(file) != 0) {
    printf("video in file\n");
    printf("total vstreams = %d\n", mpeg3_total_vstreams(file));
    max_frames = mpeg3_video_frames(file,0);
    w = mpeg3_video_width(file, 0);
    h = mpeg3_video_height(file, 0);
    printf("max frames %d w %d h %d\n", max_frames, w, h);
    for (frame_on = 0; frame_on < max_frames; frame_on++) {
      ret = mpeg3_read_yuvframe_ptr(file,
				    &y_output,
				    &u_output,
				    &v_output,
				    0);
      if (ret == 0 && ofile != NULL) {
	fwrite(y_output, w, h, ofile);
	fwrite(u_output, w/2, h/2, ofile);
	fwrite(v_output, w/2, h/2, ofile);
      }
    }
  }
#else
  {
    unsigned char *output;
    long size = 0, max_size;
    long byte;
    int have_frame;
    w = 0;

    max_size = 4096;
    output = malloc(4096);
    frame_on = 0;
    do {
      have_frame = mpeg3_read_video_chunk_resize(file,
						 &output,
						 &size,
						 &max_size,
						 0);
      if (have_frame == 0 && size > 4) {
	ret = mpeg3_read_yuvframe_ptr_test(file,
					   output,
					   size - 1,
					   &y_output,
					   &u_output,
					   &v_output,
					   0);
	if (ret == 0 && ofile != NULL) {
	  if (w == 0) {
	    w = mpeg3_video_width(file, 0);
	    h = mpeg3_video_height(file, 0);
	    printf("w %d h %d\n", w, h);
	  } else {
	  fwrite(y_output, w, h, ofile);
	  fwrite(u_output, w/2, h/2, ofile);
	  fwrite(v_output, w/2, h/2, ofile);
	  }
	}
	output[0] = output[size - 4];
	output[1] = output[size - 3];
	output[2] = output[size - 2];
	output[3] = output[size - 1];
	size = 4;
	frame_on++;
      }
	
    } while (have_frame == 0);
  }
#endif
  mpeg3_close(file);
  if (ofile != NULL) {
    fclose(ofile);
  }
  return 0;
}
