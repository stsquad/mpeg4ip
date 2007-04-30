
#include "mpeg4ip.h"
#include "mp4av.h"

static unsigned char to_hex (char *ptr)
{
  if (isdigit(*ptr)) {
    return (*ptr - '0');
  }
  return (tolower(*ptr) - 'a' + 10);
}

int main (int argc, char **argv)
{
  
  size_t len;
  argc--;
  argv++;
  if (argc != 1) {
    printf("Only 1 argument allowed\n");
    return(-1);
  }

  len = strlen(*argv);
  if (len & 0x1) {
    printf("must have even number of numbers\n");
    return(-1);
  }

  len /= 2;

  uint8_t *vol = (uint8_t *)malloc(len);
  if (vol == NULL) return -1;
  char *ptr;
  uint8_t *bptr = vol;
  ptr = *argv;
  size_t ix = len;
  while (ix > 0) {
    *bptr++ = (to_hex(ptr) << 4) | (to_hex(ptr + 1));
    ptr += 2;
    ix--;
  }

  u_int8_t TimeBits;
  u_int16_t TimeTicks;
  u_int16_t FrameDuration;
  u_int16_t FrameWidth;
  u_int16_t FrameHeight;

  if (MP4AV_Mpeg4ParseVol(vol, len, 
			  &TimeBits,
			  &TimeTicks,
			  &FrameDuration,
			  &FrameWidth,
			  &FrameHeight) == false) {
    printf("Can't decode VOL\n");
  } else {
    printf("Vol decoded as timebits %u ticks %u duration %u\n",
	   TimeBits, TimeTicks, FrameDuration);
    printf("Width %u height %u\n", FrameWidth, FrameHeight);
  }

  free(vol);
  return(0);
}
