

#ifndef __MPEG4IP_BITS_H__
#define __MPEG4IP_BITS_H__ 1

typedef unsigned int (*get_t)(void *);
typedef void (*bookmark_t)(void *, int );
typedef struct _bitfile
{
  void *bs_userdata;
  get_t bs_get_routine;
  bookmark_t bs_bookmark;

  int m_lCounter;
  int m_iBitPosition;
  unsigned int m_iBuffer;
  unsigned int m_uNumOfBitsInBuffer;
  unsigned char m_chDecBuffer;
  int m_bBookmarkOn;
  unsigned int m_uNumOfBitsInBuffer_bookmark;
  unsigned char m_chDecBuffer_bookmark;
  int m_lCounter_bookmark;
  int m_iBitPosition_bookmark;
  unsigned int m_iBuffer_bookmark;
  int m_bookmark_eofstate;
  int m_processed_bits;
  int m_processed_bits_bookmark;
  int m_total;
} bitfile;

void faad_initbits(bitfile *ld, char *buffer);
#define showbits faad_showbits
unsigned int faad_showbits(bitfile *ld, int n);
#define flushbits faad_flushbits
void faad_flushbits(bitfile *ld, int n);
unsigned int faad_getbits(bitfile *ld, unsigned int n);
#define faad_getbits_fast faad_getbits
#define faad_get1bit(ld) faad_getbits(ld, 1)
int faad_byte_align(bitfile *ld);
int faad_get_processed_bits(bitfile *ld);
void faad_bookmark(bitfile *ld, int state);

void faad_init_bytestream(bitfile *ld, get_t get, bookmark_t userdata, void *ud);

void faad_bitdump(bitfile *ld);
#endif
