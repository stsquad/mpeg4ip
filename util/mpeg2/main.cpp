#include "mpeg4ip.h"
#include "mp4av.h"

int main (int argc, char **argv)
{
#define MAX_BUFFER 65536

  uint8_t buffer[MAX_BUFFER];
  uint32_t buffer_on, buffer_size;
  uint64_t bytes = 0;
  FILE *m_file;

  argc--;
  argv++;

  m_file = fopen(*argv, FOPEN_READ_BINARY);

  if (m_file == NULL) {
    fprintf(stderr, "file %s not found\n", *argv);
    exit(-1);
  }

  buffer_on = buffer_size = 0;
  while (!feof(m_file)) {
    if (buffer_on != 0) {
      bytes += buffer_size - 4;
      memmove(buffer, &buffer[buffer_size - 4], 4);
      buffer_on = 4;
    }
    buffer_size = fread(buffer + buffer_on, 
			1, 
			MAX_BUFFER - buffer_on, 
			m_file);
    buffer_size += buffer_on;
    buffer_on = 0;
    while (buffer_on + 4 < buffer_size) {
      uint32_t optr, scode;
      if (MP4AV_Mpeg3FindNextStart(buffer + buffer_on,
				   buffer_size - buffer_on - 4,
				   &optr, 
				   &scode) < 0) {
	// nothing in buffer;
	buffer_on = buffer_size;
      } else {
	buffer_on += optr;
#define MPEG3_SEQUENCE_START_CODE        0x000001b3
#define MPEG3_PICTURE_START_CODE         0x00000100
#define MPEG3_GOP_START_CODE             0x000001b8
#define MPEG3_EXT_START_CODE             0x000001b5
	if (scode == MPEG3_PICTURE_START_CODE && buffer_on + 6 < buffer_size) {
	  printf("%08x %8"U64F" type %u temp %u\n", 
		  scode, 
		 bytes + buffer_on,
		 MP4AV_Mpeg3PictHdrType(buffer + buffer_on),
		  MP4AV_Mpeg3PictHdrTempRef(buffer + buffer_on));
	} else {
	  printf("%08x %8"U64F"\n", scode, bytes + buffer_on);
	}
	buffer_on += 4;
      }
    }

  }
  fclose(m_file);
  return 0;
}
