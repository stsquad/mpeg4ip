int   patch_up(char *name);
char* snap_filename(char *base, char *channel, char *ext);

int   write_jpeg(char *filename, struct ng_video_buf *buf,
		 int quality, int gray);
int   write_ppm(char *filename, struct ng_video_buf *buf);
int   write_pgm(char *filename, struct ng_video_buf *buf);
