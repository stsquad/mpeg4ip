
#include "mpeg4ip.h"

int main (int argc, char **argv)
{
  int out_fd;
  char filename[32];
  FILE *tmpfile, *infile, *outfile;
  uint64_t bytes_to_skip;

  if (argc != 3) {
    printf("Usage: %s <number of bytes> <filename>\n", 
	   argv[0]);
    exit(-1);
  }

  bytes_to_skip = strtoull(argv[1], NULL, 0);
  struct stat statbuf;
  
  if (stat(argv[2], &statbuf) < 0) {
    printf("Could not stat %s\n", argv[2]);
    exit(-1);
  }
  
  if (bytes_to_skip >= (uint64_t)statbuf.st_size) {
    printf("Too many bytes to remove "U64" out of "U64"\n", 
	   bytes_to_skip, statbuf.st_size);
    exit(-1);
  }
  
  strcpy(filename, "XXXXXX");

  out_fd = mkstemp(filename);
  if (out_fd < 0) {
    printf("Couldn't create temporary file\n");
    exit(-1);
  }
  tmpfile = fdopen(out_fd, "wb");

  
  infile = fopen(argv[2], FOPEN_READ_BINARY);
  
  fseek(infile, bytes_to_skip, SEEK_SET);
  uint8_t buffer[4096];
  uint32_t bytes_read;
  while ((bytes_read = fread(buffer, 1, 4096, infile)) != 0) {
    fwrite(buffer, 1, bytes_read, tmpfile);
  }
  fclose(infile);
  fclose(tmpfile);
  infile = fopen(filename, FOPEN_READ_BINARY);
  outfile = fopen(argv[2], FOPEN_WRITE_BINARY);
  while ((bytes_read = fread(buffer, 1, 4096, infile)) != 0) {
    fwrite(buffer, 1, bytes_read, outfile);
  }
  fclose(infile);
  fclose(outfile);
  
  unlink(filename);
  return 0;
}
