#include "all.h"
#include "filestream.h"
#include "port.h"

int stop_now;
int	istrans = 0;
int framebits;

extern FILE_STREAM *input_file;

void reset_bits()
{
    cword = 0;
    nbits = 0;
}

#if 0
// wmay - getshort eliminated
long getshort()
{
	int c[2];

	//	if (!stop_now)  commented out - check in read_byte_filestream
	{
	  c[0] = read_byte_filestream(input_file);
	  c[1] = read_byte_filestream(input_file);
	}
	//	else
	//{
	//	return 0;
	//}

	framebits += 16;
    return (c[0]<<8) | c[1];
}
#endif

int byte_align()
{
    int i=0;
    
    while (nbits & ((1<<3)-1))
	{
		nbits -= 1;
		i += 1;
    }

    return(i);
}

static long bit_mask[33] =
{
  0,
  0x00000001,
  0x00000003,
  0x00000007,
  0x0000000f,
  0x0000001f,
  0x0000003f,
  0x0000007f,
  0x000000ff,
  0x000001ff,
  0x000003ff,
  0x000007ff,
  0x00000fff,
  0x00001fff,
  0x00003fff,
  0x00007fff,
  0x0000ffff,
  0x0001ffff,
  0x0003ffff,
  0x0007ffff,
  0x000fffff,
  0x001fffff,
  0x003fffff,
  0x007fffff,
  0x00ffffff,
  0x01ffffff,
  0x03ffffff,
  0x07ffffff,
  0x0fffffff,
  0x1fffffff,
  0x3fffffff,
  0x7fffffff,
  0xffffffff
};

__inline long getbits(int n)
{
  long ret;
  int diff;
  
  if (n <= nbits) {
    ret = (cword >> (nbits - n)) & bit_mask[n];
    nbits -= n;
    return (ret);
  }
  diff = n - nbits;
  ret = cword << (diff);
  cword = 0;
  switch (((diff - 1) / 8)) {
  case 0:
    cword = read_byte_filestream(input_file);
    nbits = 8;
    break;
  case 1:
    cword = read_byte_filestream(input_file);
    cword <<= 8;
    cword |= read_byte_filestream(input_file);
    nbits = 16;
    break;
  case 2:
    cword = read_byte_filestream(input_file);
    cword <<= 8;
    cword |= read_byte_filestream(input_file);
    cword <<= 8;
    cword |= read_byte_filestream(input_file);
    nbits = 24;
    break;
  case 3:
    cword = read_byte_filestream(input_file);
    cword <<= 8;
    cword |= read_byte_filestream(input_file);
    cword <<= 8;
    cword |= read_byte_filestream(input_file);
    cword <<= 8;
    cword |= read_byte_filestream(input_file);
    nbits = 32;
    break;
  }
  framebits += nbits;
  if (stop_now) return -1;
  ret |= cword >> (nbits - diff);
  nbits -= diff;
  ret &= bit_mask[n];
  return (ret);
}

void startblock()
{
    /* get adif header */

    if (adif_header_present)
	{
		if (get_adif_header() < 0)
			adif_header_present = 0;
		else
			adif_header_present = 1;
    }
    
    byte_align();	/* start of block is byte aligned */
}
