#include "mpeg4ip.h"

#define H264_START_CODE 0x000001
#define H264_PREVENT_3_BYTE 0x000003


uint32_t h264_find_next_start_code (uint8_t *pBuf, 
				    uint32_t bufLen)
{
  uint32_t val;
  uint32_t offset;

  offset = 0;
  if (pBuf[0] == 0 && pBuf[1] == 0 && pBuf[2] == 1) {
    pBuf += 3;
    offset = 3;
  }
  val = 0xffffffff;
  while (offset < bufLen - 3) {
    val <<= 8;
    val &= 0x00ffffff;
    val |= *pBuf++;
    offset++;
    if (val == H264_START_CODE) {
      return offset - 3;
    }
  }
  return 0;
}

  
    
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
    bytes += buffer_on;
    if (buffer_on != 0) {
      buffer_on = buffer_size - buffer_on;
      memmove(buffer, &buffer[buffer_size - buffer_on], buffer_on);
    }
    buffer_size = fread(buffer + buffer_on, 
			1, 
			MAX_BUFFER - buffer_on, 
			m_file);
    buffer_size += buffer_on;
    buffer_on = 0;

    bool done = false;
    do {
      uint32_t ret;
      ret = h264_find_next_start_code(buffer + buffer_on, 
				      buffer_size - buffer_on);
      if (ret == 0) {
	done = true;
	if (buffer_on == 0) {
	  fprintf(stderr, "couldn't find start code in buffer from 0\n");
	  exit(-1);
	}
      } else {
	// have a complete NAL from buffer_on to end
	printf("buffer on "X64" "X64" %u len %u %02x %02x %02x %02x\n",
	       bytes + buffer_on, 
	       bytes + buffer_on + ret,
	       buffer_on, 
	       ret,
	       buffer[buffer_on],
	       buffer[buffer_on+1],
	       buffer[buffer_on+2],
	       buffer[buffer_on+3]);
	buffer_on += ret; // buffer_on points to next code
      }
    } while (done == false);
  }
  fclose(m_file);
  return 0;
}
